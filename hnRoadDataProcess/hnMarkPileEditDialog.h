#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include "../hnApplication/hnDataManager.h"

// 单条编辑草稿。只有保存成功才关闭，取消不会调用工程写入接口。
class hnMarkPileEditDialog : public QDialog
{
public:
    hnMarkPileEditDialog(hnPro::hnProject* project, bool pileMode, int id, QWidget* parent = nullptr)
        : QDialog(parent), m_project(project), m_pileMode(pileMode), m_id(id),
          m_mile(new QDoubleSpinBox(this)), m_dmi(new QDoubleSpinBox(this)),
          m_type(new QComboBox(this)), m_content(new QComboBox(this)),
          m_position(new QGroupBox(QStringLiteral("同时修改采集位置"), this)),
          m_error(new QLabel(this)), m_neighbors(new QLabel(this)), m_initialMile(0), m_initialDmi(0), m_valid(false)
    {
        setObjectName(QStringLiteral("markPileEditDialog"));
        setWindowTitle(pileMode ? QStringLiteral("修改校桩") : QStringLiteral("修改打标"));
        resize(560, 360);
        m_mile->setObjectName(QStringLiteral("editTrueMile"));
        m_dmi->setObjectName(QStringLiteral("editDmi"));
        m_type->setObjectName(QStringLiteral("editMarkType"));
        m_content->setObjectName(QStringLiteral("editMarkContent"));
        m_position->setObjectName(QStringLiteral("editPositionGroup"));
        m_error->setObjectName(QStringLiteral("editError"));
        m_error->setStyleSheet(QStringLiteral("color: #b42318;"));
        m_error->setWordWrap(true);
        m_neighbors->setWordWrap(true);
        QVBoxLayout* layout = new QVBoxLayout(this);
        const auto settings = project->getCurProSetInfo();
        QLabel* range = new QLabel(QStringLiteral("工程桩号范围：%1 ～ %2 米\n填写米数，例如 K6+514 填写 6514。")
            .arg(qMin(settings.dBegMile, settings.dEndMile), 0, 'f', 3)
            .arg(qMax(settings.dBegMile, settings.dEndMile), 0, 'f', 3), this);
        range->setWordWrap(true);
        layout->addWidget(range);
        QFormLayout* form = new QFormLayout();
        layout->addLayout(form);
        m_mile->setDecimals(3); m_mile->setRange(-1000000000, 1000000000); m_mile->setKeyboardTracking(false);
        m_dmi->setDecimals(3); m_dmi->setRange(-1000000000, 1000000000); m_dmi->setKeyboardTracking(false);
        form->addRow(QStringLiteral("真实桩号（米）"), m_mile);
        if (pileMode)
        {
            m_type->hide(); m_content->hide();
            for (const auto& pile : project->getCurrentMilePileVector())
                if (pile.nID == id) { m_pile = pile; m_valid = true; break; }
            m_mile->setValue(m_pile.dTrueMile); m_dmi->setValue(m_pile.dEnclMile);
            m_position->setCheckable(true);
            QFormLayout* positionLayout = new QFormLayout(m_position);
            positionLayout->addRow(QStringLiteral("相对里程（米）"), m_dmi);
            QPushButton* current = new QPushButton(QStringLiteral("使用当前图像位置"), m_position);
            positionLayout->addRow(current);
            connect(current, &QPushButton::clicked, this, &hnMarkPileEditDialog::useCurrentPosition);
            m_position->setChecked(false);
            layout->addWidget(m_position);
            layout->addWidget(m_neighbors);
            layout->addWidget(new QLabel(QStringLiteral("保存后同步打标桩号，打标和病害的采集位置保持不变。"), this));
            connect(m_dmi, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &hnMarkPileEditDialog::updatePositionHint);
            updatePositionHint();
        }
        else
        {
            m_position->hide(); m_dmi->hide();
            for (const auto& mark : project->getCurrentMarkVector())
                if (mark.nID == id) { m_mark = mark; m_valid = true; break; }
            m_mile->setValue(m_mark.dTrueMile);
            m_type->addItem(QStringLiteral("路面材质"), 0);
            m_type->addItem(QStringLiteral("路面单元"), 1);
            m_type->addItem(QStringLiteral("路面等级"), 2);
            m_type->addItem(QStringLiteral("路面情况"), 4);
            // 原有标准打标允许纠正位置，仍不开放新增标准类型。
            if (m_mark.nType == 3) m_type->addItem(QStringLiteral("路面标准"), 3);
            m_type->setCurrentIndex(m_type->findData(m_mark.nType));
            form->addRow(QStringLiteral("打标类型"), m_type);
            form->addRow(QStringLiteral("打标内容"), m_content);
            layout->addWidget(m_neighbors);
            connect(m_type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &hnMarkPileEditDialog::updateContentOptions);
            connect(m_mile, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &hnMarkPileEditDialog::updatePositionHint);
            updateContentOptions();
            updatePositionHint();
        }
        m_initialMile = m_mile->value(); m_initialDmi = m_dmi->value();
        layout->addStretch(); layout->addWidget(m_error);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存修改"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &hnMarkPileEditDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    void accept() override
    {
        if (!m_valid || hnApp::hnDataManager::getDataManager()->getCurrentProject() != m_project)
        {
            m_error->setText(QStringLiteral("当前工程或记录已变化，请关闭后重新打开修改窗口。")); return;
        }
        QString error;
        bool success = false;
        m_mile->interpretText(); m_dmi->interpretText();
        if (m_pileMode)
        {
            auto candidate = m_pile;
            if (m_mile->value() != m_initialMile) candidate.dTrueMile = m_mile->value();
            if (m_position->isChecked() && m_dmi->value() != m_initialDmi) candidate.dEnclMile = m_dmi->value();
            success = m_project->updateMilePile(m_id, candidate, &error);
        }
        else
        {
            auto candidate = m_mark;
            if (m_mile->value() != m_initialMile) candidate.dTrueMile = m_mile->value();
            candidate.nType = m_type->currentData().toInt();
            QByteArray content = m_content->currentText().trimmed().toLocal8Bit();
            if (content.isEmpty() || content.size() >= sizeof(candidate.strMark))
            {
                m_error->setText(QStringLiteral("打标内容不能为空或超过数据库字段长度，请缩短后保存。")); return;
            }
            strcpy_s(candidate.strMark, content.constData());
            success = m_project->updateMark(m_id, candidate, &error);
        }
        if (!success) { m_error->setText(error); return; }
        QDialog::accept();
    }

private:
    void useCurrentPosition()
    {
        if (hnApp::hnDataManager::getDataManager()->getCurrentProject() == m_project)
            m_dmi->setValue(m_project->getCurrentRoadMile().dEnclMile);
    }
    void updateContentOptions()
    {
        const int type = m_type->currentData().toInt();
        m_content->clear();
        m_content->setEditable(type == 1 || type == 4);
        auto manager = hnApp::hnDataManager::getDataManager();
        if (type == 0)
            for (const auto& text : manager->getRoadSurfaceType()) m_content->addItem(text);
        if (type == 2)
            for (const auto& text : manager->getRoadLevel(m_project->getBaseStandard())) m_content->addItem(text);
        if (type == 3) m_content->addItem(QString::fromLocal8Bit(m_mark.strMark));
        if (type == m_mark.nType)
        {
            const QString original = QString::fromLocal8Bit(m_mark.strMark);
            int index = m_content->findText(original);
            if (index < 0) { m_content->addItem(original); index = m_content->count() - 1; }
            m_content->setCurrentIndex(index);
        }
    }
    void updatePositionHint()
    {
        if (!m_pileMode)
        {
            m_neighbors->setText(QStringLiteral("对应相对里程：%1 米（保存时自动换算）")
                .arg(m_project->trueMileToEncl(m_mile->value()), 0, 'f', 3));
            return;
        }
        QString before = QStringLiteral("无"), after = QStringLiteral("无");
        double previousDmi = -1e100, nextDmi = 1e100;
        for (const auto& pile : m_project->getCurrentMilePileVector())
        {
            if (pile.nID == m_id) continue;
            QString text = QStringLiteral("桩号 %1 / 相对里程 %2 米").arg(pile.dTrueMile, 0, 'f', 3).arg(pile.dEnclMile, 0, 'f', 3);
            if (pile.dEnclMile <= m_dmi->value() && pile.dEnclMile > previousDmi) { previousDmi = pile.dEnclMile; before = text; }
            if (pile.dEnclMile >= m_dmi->value() && pile.dEnclMile < nextDmi) { nextDmi = pile.dEnclMile; after = text; }
        }
        m_neighbors->setText(QStringLiteral("前一校桩：%1\n后一校桩：%2\n桩号应按工程方向保持顺序。").arg(before).arg(after));
    }
    hnPro::hnProject* m_project;
    bool m_pileMode;
    int m_id;
    QDoubleSpinBox* m_mile;
    QDoubleSpinBox* m_dmi;
    QComboBox* m_type;
    QComboBox* m_content;
    QGroupBox* m_position;
    QLabel* m_error;
    QLabel* m_neighbors;
    double m_initialMile;
    double m_initialDmi;
    bool m_valid;
    hnCommon::hnMarkInfo m_mark;
    hnCommon::hnMilePile m_pile;
};
