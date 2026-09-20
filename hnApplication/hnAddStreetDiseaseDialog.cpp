#include "hnAddStreetDiseaseDialog.h"
#include "hnDiseaseService.h"
#include <QTreeWidget>
#include <QHeaderView>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMessageBox>
#include <QSet>

hnAddStreetDiseaseDialog::hnAddStreetDiseaseDialog(QVector<hnDiseaseSetInfo> LJInfo,
    QVector<hnDiseaseSetInfo> YXInfo, QWidget* parent)
    : QDialog(parent), m_tree(new QTreeWidget(this)), m_search(new QLineEdit(this)),
      m_customName(new QComboBox(this)), m_customCount(new QSpinBox(this)),
      m_customRemark(new QLineEdit(this)), m_location(new QLabel(this)), m_side(0)
{
    setWindowTitle(QStringLiteral("添加景观病害"));
    resize(700, 580);
    setMinimumSize(560, 400);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_location);
    QTabWidget* tabs = new QTabWidget(this);
    layout->addWidget(tabs);
    QWidget* standardPage = new QWidget(tabs);
    QVBoxLayout* standardLayout = new QVBoxLayout(standardPage);
    m_search->setPlaceholderText(QStringLiteral("搜索标准病害名称…"));
    m_search->setClearButtonEnabled(true);
    standardLayout->addWidget(m_search);
    m_tree->setHeaderLabels(QStringList() << QStringLiteral("选择病害") << QStringLiteral("数量 / 长度 / 面积") << QStringLiteral("计量方式"));
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->setColumnWidth(1, 180);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    standardLayout->addWidget(m_tree);
    addStandardGroup(QStringLiteral("沿线设施"), YXInfo);
    addStandardGroup(QStringLiteral("路基损坏"), LJInfo);
    m_tree->expandAll();
    tabs->addTab(standardPage, QStringLiteral("标准病害"));
    QWidget* customPage = new QWidget(tabs);
    QFormLayout* customLayout = new QFormLayout(customPage);
    m_customName->setObjectName(QStringLiteral("customStreetName"));
    m_customCount->setObjectName(QStringLiteral("customStreetCount"));
    m_customRemark->setObjectName(QStringLiteral("customStreetRemark"));
    m_tree->setObjectName(QStringLiteral("standardStreetDiseases"));
    m_customName->setEditable(true);
    m_customName->setInsertPolicy(QComboBox::NoInsert);
    m_customName->lineEdit()->setPlaceholderText(QStringLiteral("输入名称，或选择本工程已有名称"));
    m_customCount->setRange(1, 1000000);
    m_customRemark->setPlaceholderText(QStringLiteral("可选，填写现场情况"));
    customLayout->addRow(QStringLiteral("病害名称"), m_customName);
    customLayout->addRow(QStringLiteral("数量（个）"), m_customCount);
    customLayout->addRow(QStringLiteral("备注"), m_customRemark);
    QLabel* hint = new QLabel(QStringLiteral("自定义记录随工程保存，可导出明细，不参与标准病害扣分。"), customPage);
    hint->setWordWrap(true);
    customLayout->addRow(hint);
    tabs->addTab(customPage, QStringLiteral("自定义病害"));
    auto service = hnApp::hnDataManager::getDataManager()->getDiseaseService();
    QSet<QString> names;
    for (const auto& disease : service->getAllStreetDiseases())
        if (disease.ndiseaseType == 3) names.insert(QString::fromLocal8Bit(disease.strDisName));
    QStringList sortedNames = names.toList(); sortedNames.sort();
    m_customName->addItems(sortedNames);
    m_customName->setCurrentIndex(-1);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &hnAddStreetDiseaseDialog::saveSelection);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_search, &QLineEdit::textChanged, this, &hnAddStreetDiseaseDialog::filterDiseases);
}
hnAddStreetDiseaseDialog::~hnAddStreetDiseaseDialog() {}
void hnAddStreetDiseaseDialog::setCurrentHnMile(const hnMile& mile)
{
    m_currentMile = mile;
    m_location->setText(QStringLiteral("当前桩号：%1 米　相对里程：%2 米").arg(mile.dTrueMile, 0, 'f', 2).arg(mile.dEnclMile, 0, 'f', 2));
}
void hnAddStreetDiseaseDialog::setImageSide(int side) { m_side = side == 1 ? 1 : 0; }
QVector<hnRoadDiseaseInfo> hnAddStreetDiseaseDialog::getSelectDiseases() { return m_selectDiseases; }
void hnAddStreetDiseaseDialog::addStandardGroup(const QString& name, const QVector<hnDiseaseSetInfo>& infos)
{
    QTreeWidgetItem* group = new QTreeWidgetItem(m_tree, QStringList() << name);
    for (const auto& info : infos)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(group, QStringList() << QString::fromLocal8Bit(info.strDiseaseTypeName));
        item->setToolTip(0, item->text(0));
        item->setCheckState(0, Qt::Unchecked);
        item->setData(0, Qt::UserRole, m_settings.size());
        m_settings.append(info);
        QDoubleSpinBox* amount = new QDoubleSpinBox(m_tree);
        amount->setDecimals(info.dEffectMeasure == 1 ? 0 : 2);
        amount->setRange(0, 1000000);
        amount->setValue(info.dEffectMeasure == 1 ? 1 : 0);
        amount->setKeyboardTracking(false);
        m_tree->setItemWidget(item, 1, amount);
        item->setText(2, info.dEffectMeasure == 1 ? QStringLiteral("个") : QStringLiteral("长度 / 面积"));
    }
}
void hnAddStreetDiseaseDialog::filterDiseases(const QString& text)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* group = m_tree->topLevelItem(i);
        bool visible = false;
        for (int j = 0; j < group->childCount(); j++)
        {
            QTreeWidgetItem* item = group->child(j);
            bool match = item->text(0).contains(text.trimmed(), Qt::CaseInsensitive);
            item->setHidden(!match); visible = visible || match;
        }
        group->setHidden(!visible);
    }
}
void hnAddStreetDiseaseDialog::saveSelection()
{
    // 先校验所有选中项，再开启事务，避免某条输入有误时部分写入。
    m_selectDiseases.clear();
    for (int i = 0; i < m_tree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* group = m_tree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); j++)
        {
            QTreeWidgetItem* item = group->child(j);
            if (item->checkState(0) != Qt::Checked) continue;
            const hnDiseaseSetInfo& info = m_settings.at(item->data(0, Qt::UserRole).toInt());
            double amount = qobject_cast<QDoubleSpinBox*>(m_tree->itemWidget(item, 1))->value();
            if (amount <= 0)
            {
                QMessageBox::warning(this, QStringLiteral("请填写数量"), item->text(0) + QStringLiteral("的数量、长度或面积必须大于零。")); return;
            }
            hnRoadDiseaseInfo disease;
            disease.ndiseaseType = info.nDiseaseType; disease.dArea = amount; disease.nLevel = info.nLevel;
            disease.diseaseWeight = info.fWidget; disease.nDrawType = m_side;
            strcpy_s(disease.strDiseaseTableName, info.strDBTableName);
            strcpy_s(disease.strDisName, info.strDiseaseTypeName);
            m_selectDiseases.append(disease);
        }
    }
    QString name = m_customName->currentText().trimmed();
    if (!name.isEmpty())
    {
        hnRoadDiseaseInfo disease;
        const QByteArray nameBytes = name.toLocal8Bit();
        const QByteArray remarkBytes = m_customRemark->text().toLocal8Bit();
        if (name.contains(',') || name.contains('\n') || nameBytes.size() >= sizeof(disease.strDisName) || remarkBytes.size() >= sizeof(disease.strRemark))
        {
            QMessageBox::warning(this, QStringLiteral("输入不完整"), QStringLiteral("名称不能含逗号、换行，名称或备注不能超过数据库字段长度。")); return;
        }
        disease.ndiseaseType = 3; disease.nDrawType = 10 + m_side; disease.dArea = m_customCount->value(); disease.diseaseWeight = 0;
        strcpy_s(disease.strDiseaseTableName, "UserStreetDisease");
        strcpy_s(disease.strDisName, nameBytes.constData());
        strcpy_s(disease.strRemark, remarkBytes.constData());
        m_selectDiseases.append(disease);
    }
    if (m_selectDiseases.isEmpty())
    {
        QMessageBox::information(this, QStringLiteral("请选择病害"), QStringLiteral("请勾选标准病害，或填写自定义病害名称。")); return;
    }
    auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
    auto db = project->getDB();
    bool ok = db->executeDB("BEGIN IMMEDIATE");
    if (!ok) { QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法启动数据库事务。")); return; }
    try
    {
        for (auto& disease : m_selectDiseases)
        {
            disease.dMileage = disease.dDmi = disease.dDmiStart = disease.dDmiEnd = m_currentMile.dEnclMile;
            disease.dRoadWidth = project->getCurProSetInfo().dRoadWidth;
            disease.nRSurfaceType = m_currentMile.roadType;
            strcpy_s(disease.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(m_currentMile.roadStandard).toLocal8Bit().constData());
            disease.nID = db->getDiseaseTable()->getMaxID(disease.strDiseaseTableName);
            if (!db->getDiseaseTable()->writeSingleDatas_Service(project->getCurProSetInfo(), disease)) { ok = false; break; }
        }
        if (ok) ok = db->executeDB("COMMIT");
    }
    catch (...) { ok = false; }
    if (!ok)
    {
        db->executeDB("ROLLBACK"); m_selectDiseases.clear();
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("病害未保存，请检查成果数据库是否可写。")); return;
    }
    hnApp::hnDataManager::getDataManager()->getDiseaseService()->invalidateCache();
    accept();
}
