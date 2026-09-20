#include "../hnQtCommon/hnProgressStyle.h"
#include "../hnQtCommon/hnWindowUiState.h"
#include "hnResultDataDialog.h"
#include "../hnProject/hnProject.h"
#include "../hnQtCommon/MyCommonMethods.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QLabel>

hnResultDataDialog::hnResultDataDialog(hnPro::hnProject* project, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("当前工程成果库数据（只读）")); resize(900, 600);
    QVBoxLayout* layout = new QVBoxLayout(this);
    setStyleSheet(hnProgressStyle::taskDialogStyleSheet());
    new hnWindowUiState(this, QStringLiteral("ResultData"));
    QLabel* title = new QLabel(QStringLiteral("以下内容来自当前 SQLite 成果库，仅供查看，不会修改工程。"), this);
    title->setStyleSheet(QStringLiteral("font-weight:600;color:#1f4e79;padding:6px;")); layout->addWidget(title);
    const auto info = project->getCurProSetInfo();
    QFormLayout* form = new QFormLayout; form->addRow(QStringLiteral("成果库文件"), new QLabel(project->getDbResultFilePath(), this)); form->addRow(QStringLiteral("道路名称 / 编号"), new QLabel(QString::fromLocal8Bit(info.strRoadName)+QStringLiteral(" / ")+QString::fromLocal8Bit(info.strRoadNO),this)); form->addRow(QStringLiteral("道路等级"),new QLabel(QString::fromLocal8Bit(info.strRoadLevel),this)); form->addRow(QStringLiteral("DMI 判断依据"),new QLabel(QStringLiteral("当前成果库校桩"),this)); layout->addLayout(form);
    QTabWidget* tabs = new QTabWidget(this); layout->addWidget(tabs,1);
    QTableWidget* marks = new QTableWidget(this); marks->setColumnCount(4); marks->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("桩号")<<QStringLiteral("相对 DMI")<<QStringLiteral("类型")<<QStringLiteral("内容")); marks->setEditTriggers(QAbstractItemView::NoEditTriggers); marks->setSelectionBehavior(QAbstractItemView::SelectRows); marks->setAlternatingRowColors(true); marks->horizontalHeader()->setStretchLastSection(true);
    const auto markData=project->getCurrentMarkVector(); const QStringList types=QStringList()<<QStringLiteral("路面材质")<<QStringLiteral("路面单元")<<QStringLiteral("道路等级")<<QStringLiteral("路面标准")<<QStringLiteral("路面情况"); marks->setRowCount(markData.size()); for(int i=0;i<markData.size();++i){const auto&m=markData.at(i);marks->setItem(i,0,new QTableWidgetItem(MyCommonMethods::convertMileToString(m.dTrueMile)));marks->setItem(i,1,new QTableWidgetItem(QString::number(m.dEnclMile,'f',2)));marks->setItem(i,2,new QTableWidgetItem(m.nType>=0&&m.nType<types.size()?types.at(m.nType):QStringLiteral("其他")));marks->setItem(i,3,new QTableWidgetItem(QString::fromLocal8Bit(m.strMark)));} tabs->addTab(marks,QStringLiteral("打标（%1）").arg(markData.size()));
    QTableWidget* piles = new QTableWidget(this); piles->setColumnCount(4); piles->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("编号")<<QStringLiteral("桩号")<<QStringLiteral("相对 DMI")<<QStringLiteral("原始 DMI")); piles->setEditTriggers(QAbstractItemView::NoEditTriggers); piles->setSelectionBehavior(QAbstractItemView::SelectRows); piles->setAlternatingRowColors(true); piles->horizontalHeader()->setStretchLastSection(true);
    const auto pileData=project->getCurrentMilePileVector(); piles->setRowCount(pileData.size()); for(int i=0;i<pileData.size();++i){const auto&p=pileData.at(i);piles->setItem(i,0,new QTableWidgetItem(QString::number(p.nID)));piles->setItem(i,1,new QTableWidgetItem(MyCommonMethods::convertMileToString(p.dTrueMile)));piles->setItem(i,2,new QTableWidgetItem(QString::number(p.dEnclMile,'f',2)));piles->setItem(i,3,new QTableWidgetItem(QString::number(p.nDMi,'f',2)));} tabs->addTab(piles,QStringLiteral("校桩（%1）").arg(pileData.size()));
    QDialogButtonBox* buttons=new QDialogButtonBox(QDialogButtonBox::Close,this);connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);layout->addWidget(buttons);
}