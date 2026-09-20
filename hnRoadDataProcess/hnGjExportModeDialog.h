#pragma once

#include "hnGjConvertSourceService.h"
#include "../hnQtCommon/MyCommonMethods.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QSettings>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QRadioButton>
#include <QTextBrowser>
#include <QVBoxLayout>

// 在选择提交格式时展示实际成果内容，避免仅凭规范名称选择。
class hnGjExportModeDialog : public QDialog
{
public:
    explicit hnGjExportModeDialog(bool containsRuralRoad, QWidget* parent = nullptr, bool containsDegreeRoad = false)
        : QDialog(parent), m_containsRuralRoad(containsRuralRoad), m_ruralOnly(containsRuralRoad && !containsDegreeRoad),
          m_standard(new QRadioButton(QStringLiteral("检测规程模式"), this)),
          m_national(new QRadioButton(QStringLiteral("农养国省道 · 2026 格式"), this)),
          m_details(new QTextBrowser(this)),
          m_outputGroup(new QGroupBox(QStringLiteral("选择输出内容（默认全选）"), this)),
          m_resultGroup(new QGroupBox(QStringLiteral("结果数据"), m_outputGroup)),
          m_rawGroup(new QGroupBox(QStringLiteral("原始数据"), m_outputGroup))
    {
        setWindowTitle(QStringLiteral("选择国检输出模式"));
        setMinimumSize(780, 620);
        resize(880, 740);
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(20, 18, 20, 16);
        layout->setSpacing(12);
        auto title = new QLabel(QStringLiteral("先确认接收方要求，再选择生成格式"), this);
        QFont heading = title->font();
        heading.setPointSize(12);
        heading.setBold(true);
        title->setFont(heading);
        layout->addWidget(title);
        auto intro = new QLabel(QStringLiteral("先选择规范，再选择要导出的结果数据和原始数据。默认全选；取消某项后不会创建对应目录，也不会预检该项专属数据。"), this);
        intro->setWordWrap(true);
        layout->addWidget(intro);
        auto choices = new QHBoxLayout;
        m_standard->setObjectName(QStringLiteral("standardMode"));
        m_national->setObjectName(QStringLiteral("nationalMode"));
        choices->addWidget(m_standard);
        choices->addWidget(m_national);
        layout->addLayout(choices);
        auto scope = new QLabel(containsRuralRoad
            ? QStringLiteral("当前批次含低等级农村公路：仅支持检测规程模式。等级公路成果按各工程类型生成。")
            : QStringLiteral("当前批次可选两种模式；请与接收方确认使用检测规程格式还是 2026 农养国省道格式。"), this);
        scope->setWordWrap(true);
        scope->setStyleSheet(QStringLiteral("QLabel { background: #eef3f7; color: #33475b; padding: 9px; border: 1px solid #d9e1e8; }"));
        layout->addWidget(scope);
        m_national->setEnabled(!containsRuralRoad);
        auto resultLayout = new QGridLayout(m_resultGroup);
        addOutputCheck(resultLayout, m_dr, QStringLiteral("DR（病害）"), 0, 0);
        addOutputCheck(resultLayout, m_iri, QStringLiteral("IRI（平整度）"), 0, 1);
        addOutputCheck(resultLayout, m_rd, QStringLiteral("RD（车辙）"), 0, 2);
        addOutputCheck(resultLayout, m_pb, QStringLiteral("PB（跳车）"), 1, 0);
        addOutputCheck(resultLayout, m_mpd, QStringLiteral("MPD"), 1, 1);
        addOutputCheck(resultLayout, m_smtd, QStringLiteral("SMTD"), 1, 2);
        auto rawLayout = new QGridLayout(m_rawGroup);
        addOutputCheck(rawLayout, m_lbi, QStringLiteral("LBIFile"), 0, 0);
        addOutputCheck(rawLayout, m_ha, QStringLiteral("HAFile"), 0, 1);
        addOutputCheck(rawLayout, m_ri, QStringLiteral("RIFile / LP"), 0, 2);
        addOutputCheck(rawLayout, m_rdFile, QStringLiteral("RDFile / TP"), 1, 0);
        addOutputCheck(rawLayout, m_tt, QStringLiteral("TTFile"), 1, 1);
        addOutputCheck(rawLayout, m_lf, QStringLiteral("LFile"), 1, 2);
        applyConfiguredLabels();
        auto outputLayout = new QVBoxLayout(m_outputGroup);
        outputLayout->addWidget(m_resultGroup);
        outputLayout->addWidget(m_rawGroup);
        layout->addWidget(m_outputGroup);
        m_details->setObjectName(QStringLiteral("modeDetails"));
        m_details->setOpenLinks(false);
        layout->addWidget(m_details, 1);
        auto rule = new QLabel(QStringLiteral("分段规则：只有 DR 考虑打标；其他成果均不被打标打断。起终点按工程边界保留。"), this);
        rule->setWordWrap(true);
        layout->addWidget(rule);
        auto footer = new QLabel(QStringLiteral("成果位置：各工程的 ConverSource。通过预检并生成成功后，替换该工程原有成果。"), this);
        footer->setWordWrap(true);
        layout->addWidget(footer);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("下一步：确认县区"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttons, &QDialogButtonBox::accepted, this, [this]() { saveSettings(); accept(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_standard, &QRadioButton::toggled, this, &hnGjExportModeDialog::refreshModeConfiguration);
        connect(m_national, &QRadioButton::toggled, this, &hnGjExportModeDialog::refreshModeConfiguration);
        layout->addWidget(buttons);
        QSettings settings(userSettingsPath(), QSettings::IniFormat);
        const bool savedNational = settings.value(
            QStringLiteral("NationalInspectionExport/NationalRoad2026"), false).toBool();
        m_standard->setChecked(!savedNational || containsRuralRoad);
        m_national->setChecked(savedNational && !containsRuralRoad);
        refreshDetails();
        refreshOutputOptions();
        loadOutputSettings();
        refreshOutputOptions();
    }

    hnGjExportStandard selectedStandard() const
    {
        return m_national->isChecked() ? hnGjExportStandard::NationalRoad2026 : hnGjExportStandard::Standard2025;
    }

    hnGjOutputSelection selectedOutputSelection() const
    {
        hnGjOutputSelection selection = hnGjOutputSelection::allFor(selectedStandard());
        selection.dr = selected(m_dr); selection.iri = selected(m_iri);
        selection.rd = selected(m_rd); selection.pb = selected(m_pb);
        selection.mpd = selected(m_mpd); selection.smtd = selected(m_smtd);
        selection.lbiFile = selected(m_lbi); selection.haFile = selected(m_ha);
        selection.riFile = selected(m_ri); selection.rdFile = selected(m_rdFile);
        selection.ttFile = selected(m_tt); selection.lFile = selected(m_lf);
        return selection;
    }

private:
    struct MetricRule
    {
        QString key;
        QString label;
        QString interval;
        QString description;
    };

    struct ModeRule
    {
        QString displayName;
        QString summary;
        QString note;
        QVector<MetricRule> resultData;
        QVector<MetricRule> rawData;
        bool valid = false;
    };

    static QString softwareRulesPath()
    {
        return QDir(QApplication::applicationDirPath())
            .filePath(QStringLiteral("config/GjExportModeConfig.json"));
    }

    static QString userSettingsPath()
    {
        return QDir(MyCommonMethods::GetUserPath()).filePath(QStringLiteral("configSetting.ini"));
    }

    static bool selected(const QCheckBox* box)
    {
        return box && !box->isHidden() && box->isChecked();
    }

    QString roadStandardKey() const
    {
        return m_ruralOnly ? QStringLiteral("RuralRoadlowLevel") : QStringLiteral("DegreeRoad2018");
    }

    QString inspectionStandardKey() const
    {
        return selectedStandard() == hnGjExportStandard::NationalRoad2026
            ? QStringLiteral("NationalRoad2026") : QStringLiteral("Standard2025");
    }

    static QVector<MetricRule> parseMetrics(const QJsonArray& array)
    {
        QVector<MetricRule> metrics;
        for (const QJsonValue& value : array)
        {
            const QJsonObject object = value.toObject();
            MetricRule metric;
            metric.key = object.value(QStringLiteral("key")).toString().trimmed();
            metric.label = object.value(QStringLiteral("label")).toString().trimmed();
            metric.interval = object.value(QStringLiteral("interval")).toString().trimmed();
            metric.description = object.value(QStringLiteral("description")).toString().trimmed();
            if (!metric.key.isEmpty()) metrics.append(metric);
        }
        return metrics;
    }

    ModeRule loadModeRule(const QString& roadKey, const QString& inspectionKey) const
    {
        ModeRule rule;
        QFile file(softwareRulesPath());
        if (!file.open(QIODevice::ReadOnly)) return rule;
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) return rule;
        const QJsonObject roadObject = document.object().value(roadKey).toObject();
        const QJsonObject modeObject = roadObject.value(QStringLiteral("inspectionStandards"))
            .toObject().value(inspectionKey).toObject();
        if (modeObject.isEmpty()) return rule;
        rule.displayName = modeObject.value(QStringLiteral("displayName")).toString();
        rule.summary = modeObject.value(QStringLiteral("summary")).toString();
        rule.note = modeObject.value(QStringLiteral("note")).toString();
        rule.resultData = parseMetrics(modeObject.value(QStringLiteral("resultData")).toArray());
        rule.rawData = parseMetrics(modeObject.value(QStringLiteral("rawData")).toArray());
        rule.valid = true;
        return rule;
    }

    ModeRule currentModeRule() const
    {
        return loadModeRule(roadStandardKey(), inspectionStandardKey());
    }

    QStringList configuredKeys(const QString& category) const
    {
        const ModeRule rule = currentModeRule();
        const QVector<MetricRule> metrics = category == QStringLiteral("Result")
            ? rule.resultData : rule.rawData;
        if (rule.valid)
        {
            QStringList keys;
            for (const MetricRule& metric : metrics) keys.append(metric.key);
            return keys;
        }
        if (category == QStringLiteral("Result"))
            return m_ruralOnly ? QStringList() << QStringLiteral("DR") << QStringLiteral("IRI")
                : QStringList() << QStringLiteral("DR") << QStringLiteral("IRI") << QStringLiteral("RD")
                    << QStringLiteral("PB") << QStringLiteral("MPD") << QStringLiteral("SMTD");
        return m_ruralOnly ? QStringList() << QStringLiteral("LBIFile") << QStringLiteral("RIFile")
            : QStringList() << QStringLiteral("LBIFile") << QStringLiteral("HAFile")
                << QStringLiteral("RIFile") << QStringLiteral("RDFile") << QStringLiteral("TTFile")
                << QStringLiteral("LFile");
    }

    QCheckBox* checkBoxForKey(const QString& key) const
    {
        if (key == QStringLiteral("DR")) return m_dr;
        if (key == QStringLiteral("IRI")) return m_iri;
        if (key == QStringLiteral("RD")) return m_rd;
        if (key == QStringLiteral("PB")) return m_pb;
        if (key == QStringLiteral("MPD")) return m_mpd;
        if (key == QStringLiteral("SMTD")) return m_smtd;
        if (key == QStringLiteral("LBIFile")) return m_lbi;
        if (key == QStringLiteral("HAFile")) return m_ha;
        if (key == QStringLiteral("RIFile")) return m_ri;
        if (key == QStringLiteral("RDFile")) return m_rdFile;
        if (key == QStringLiteral("TTFile")) return m_tt;
        if (key == QStringLiteral("LFile")) return m_lf;
        return nullptr;
    }

    void applyConfiguredLabels()
    {
        const ModeRule current = currentModeRule();
        const QVector<MetricRule> metrics = current.resultData + current.rawData;
        for (const MetricRule& metric : metrics)
        {
            QCheckBox* box = checkBoxForKey(metric.key);
            if (box && !metric.label.isEmpty()) box->setText(metric.label);
        }
        const ModeRule standard = loadModeRule(roadStandardKey(), QStringLiteral("Standard2025"));
        const ModeRule national = loadModeRule(QStringLiteral("DegreeRoad2018"), QStringLiteral("NationalRoad2026"));
        if (!standard.displayName.isEmpty()) m_standard->setText(standard.displayName);
        if (!national.displayName.isEmpty()) m_national->setText(national.displayName);
    }

    void loadOutputSettings()
    {
        QSettings settings(userSettingsPath(), QSettings::IniFormat);
        const QString group = QStringLiteral("NationalInspectionExport");
        settings.beginGroup(group);
        const QString prefix = selectedStandard() == hnGjExportStandard::NationalRoad2026
            ? QStringLiteral("national/") : (m_ruralOnly ? QStringLiteral("rural/") : QStringLiteral("standard/"));
        auto restore = [&](QCheckBox* box, const QString& key)
        {
            const QString fullKey = prefix + key;
            if (settings.contains(fullKey)) box->setChecked(settings.value(fullKey).toBool());
        };
        restore(m_dr, QStringLiteral("DR")); restore(m_iri, QStringLiteral("IRI"));
        restore(m_rd, QStringLiteral("RD")); restore(m_pb, QStringLiteral("PB"));
        restore(m_mpd, QStringLiteral("MPD")); restore(m_smtd, QStringLiteral("SMTD"));
        restore(m_lbi, QStringLiteral("LBIFile")); restore(m_ha, QStringLiteral("HAFile"));
        restore(m_ri, QStringLiteral("RIFile")); restore(m_rdFile, QStringLiteral("RDFile"));
        restore(m_tt, QStringLiteral("TTFile")); restore(m_lf, QStringLiteral("LFile"));
        settings.endGroup();
    }

    void saveSettings() const
    {
        QSettings settings(userSettingsPath(), QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("NationalInspectionExport"));
        settings.setValue(QStringLiteral("NationalRoad2026"), selectedStandard() == hnGjExportStandard::NationalRoad2026);
        const QString prefix = selectedStandard() == hnGjExportStandard::NationalRoad2026
            ? QStringLiteral("national/") : (m_ruralOnly ? QStringLiteral("rural/") : QStringLiteral("standard/"));
        auto save = [&](const QCheckBox* box, const QString& key) { settings.setValue(prefix + key, box->isChecked()); };
        save(m_dr, QStringLiteral("DR")); save(m_iri, QStringLiteral("IRI"));
        save(m_rd, QStringLiteral("RD")); save(m_pb, QStringLiteral("PB"));
        save(m_mpd, QStringLiteral("MPD")); save(m_smtd, QStringLiteral("SMTD"));
        save(m_lbi, QStringLiteral("LBIFile")); save(m_ha, QStringLiteral("HAFile"));
        save(m_ri, QStringLiteral("RIFile")); save(m_rdFile, QStringLiteral("RDFile"));
        save(m_tt, QStringLiteral("TTFile")); save(m_lf, QStringLiteral("LFile"));
        settings.endGroup();
        settings.sync();
    }

    void addOutputCheck(QGridLayout* layout, QCheckBox*& box, const QString& text, int row, int column)
    {
        box = new QCheckBox(text, m_outputGroup);
        box->setChecked(true);
        layout->addWidget(box, row, column);
    }

    void refreshOutputOptions()
    {
        const QStringList resultKeys = configuredKeys(QStringLiteral("Result"));
        const QStringList rawKeys = configuredKeys(QStringLiteral("Raw"));
        const QStringList allKeys = QStringList() << QStringLiteral("DR") << QStringLiteral("IRI")
            << QStringLiteral("RD") << QStringLiteral("PB") << QStringLiteral("MPD")
            << QStringLiteral("SMTD") << QStringLiteral("LBIFile") << QStringLiteral("HAFile")
            << QStringLiteral("RIFile") << QStringLiteral("RDFile") << QStringLiteral("TTFile")
            << QStringLiteral("LFile");
        for (const QString& key : allKeys)
        {
            QCheckBox* box = checkBoxForKey(key);
            if (box) box->setVisible(resultKeys.contains(key) || rawKeys.contains(key));
        }
        m_resultGroup->setVisible(!resultKeys.isEmpty());
        m_rawGroup->setVisible(!rawKeys.isEmpty());
    }

    void refreshModeConfiguration(bool checked)
    {
        if (!checked) return;
        loadOutputSettings();
        applyConfiguredLabels();
        refreshOutputOptions();
        refreshDetails();
    }

    void refreshDetails()
    {
        const ModeRule rule = currentModeRule();
        QString html = QStringLiteral("<html><body style='font-family:Microsoft YaHei; font-size:13px; color:#263442;'>"
            "<h3 style='color:#243f59; font-size:15px; margin:6px 0;'>%1</h3><p>%2</p>"
            "<table width='100%' border='1' cellspacing='0' cellpadding='5' style='border-color:#d9e1e8;'>"
            "<tr bgcolor='#eaf0f5'><th align='left'>分类</th><th align='left'>成果 / 目录</th>"
            "<th align='left'>间距或组织方式</th><th align='left'>生成内容与条件</th></tr>")
            .arg(rule.displayName.isEmpty()
                    ? (selectedStandard() == hnGjExportStandard::NationalRoad2026
                        ? QStringLiteral("农养国省道路况检测数据提交格式 · 2026 年")
                        : QStringLiteral("公路路面技术状况自动化检测规程"))
                    : rule.displayName,
                rule.summary.isEmpty() ? QStringLiteral("请按接收方要求选择需要生成的数据。") : rule.summary);
        const QStringList categories = QStringList() << QStringLiteral("结果数据") << QStringLiteral("原始数据");
        for (int categoryIndex = 0; categoryIndex < categories.size(); ++categoryIndex)
        {
            const QVector<MetricRule> metrics = categoryIndex == 0 ? rule.resultData : rule.rawData;
            for (const MetricRule& metric : metrics)
            {
                QCheckBox* box = checkBoxForKey(metric.key);
                if (!box) continue;
                html += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                    .arg(categories.at(categoryIndex), box->text(),
                        metric.interval.isEmpty() ? QStringLiteral("按规范") : metric.interval,
                        metric.description.isEmpty() ? QStringLiteral("按配置生成") : metric.description);
            }
        }
        html += QStringLiteral("</table><p>%1</p></body></html>")
            .arg(rule.note.isEmpty()
                ? QStringLiteral("SFC、SSR 及 SFCFile 因设备不支持不导出。") : rule.note);
        m_details->setHtml(html);
    }

    bool m_containsRuralRoad;
    bool m_ruralOnly;
    QRadioButton* m_standard;
    QRadioButton* m_national;
    QTextBrowser* m_details;
    QGroupBox* m_outputGroup;
    QGroupBox* m_resultGroup;
    QGroupBox* m_rawGroup;
    QCheckBox* m_dr = nullptr;
    QCheckBox* m_iri = nullptr;
    QCheckBox* m_rd = nullptr;
    QCheckBox* m_pb = nullptr;
    QCheckBox* m_mpd = nullptr;
    QCheckBox* m_smtd = nullptr;
    QCheckBox* m_lbi = nullptr;
    QCheckBox* m_ha = nullptr;
    QCheckBox* m_ri = nullptr;
    QCheckBox* m_rdFile = nullptr;
    QCheckBox* m_tt = nullptr;
    QCheckBox* m_lf = nullptr;
};
