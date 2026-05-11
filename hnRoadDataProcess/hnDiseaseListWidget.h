/*! hnDiseaseListWidget.h
********************************************************************************
<PRE>
模块名       : hnRoadDataProcess
文件名       : hnDiseaseListWidget.h
相关文件     : 
文件实现功能 : 病害列表显示窗口
作者         : 陈智超
版本         : 1.0
--------------------------------------------------------------------------------
备注         :
--------------------------------------------------------------------------------
修改记录 :
日 期        版本     修改人              修改内容
2023/05/10   1.0      陈智超			  创建初版
</PRE>
*******************************************************************************/


#pragma once
#include "../hnApplication/hnDataManager.h"
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "../hnDataTable/hnDataTable.h"
#include <QSortFilterProxyModel>
#include "customTableView.h"
#include <QComboBox>
#include <QSortFilterProxyModel> 
Q_DECLARE_METATYPE(hnRoadDiseaseInfo)
class hnDiseaseListWidget : public QWidget
{
	Q_OBJECT

public:
	hnDiseaseListWidget(QWidget *parent = Q_NULLPTR);
	~hnDiseaseListWidget();

public:
	// 更新所有病害
	void updateAllDiseases();

	// 添加一个病害
	void addDisease(const hnRoadDiseaseInfo &disease,bool modify);

	// 删除一个病害
	void deleteDisease(const hnRoadDiseaseInfo &disease);

	// 编辑一个病害
	void editDisease(const hnRoadDiseaseInfo &disease);

	//选中上一个病害
	void seclectLastRowDisease();

	//选中上一个
	void seclectNextRowDisease();
	
private:
	// 模型添加病害
	void modelAddDisease(QStandardItemModel &model, const hnRoadDiseaseInfo &disease, bool selectAfterAdd = true);
	// 模型删除病害
	void modelDeleteDisease(QStandardItemModel &model, const hnRoadDiseaseInfo &disease);

	QStandardItem* createNumericItem(const QString &text);
signals:
	void signal_road2dFrameIdxChanged(int frameNum);

	//设置病害进入被选中模式
	void signal_setDiseaseIsChecked(int id);
signals:
	void signal_road3dFrameIdxChanged(int frameNum);
signals:
	void signal_streetFrameIdxChanged(int frameNum);

signals:
		void signal_deleteDisease(const hnRoadDiseaseInfo& disease);
		//删除病害后更新界面
		void signal_updateView();
protected:
	void keyPressEvent(QKeyEvent *event);

private slots:
	//槽函数 表的某个表格双击
	void slot_itemDoubleClicked(const QModelIndex &index);

	void on_section_clicked(int logicalIndex);
	void on_section_doubleClicked(int logicalIndex);

	void slot_MenuClicked(const QPoint &pos);

	void slot_selectDiseaseTypeIndexChanged(int index);

	//删除病害
	void deleteDiseases(QModelIndexList selectedIndexes);

	
	public slots:
	//用户选中病害
	void slot_selectDisease(const hnRoadDiseaseInfo& disease);

	
	
private:
	//路面病害跳转
	void road2dDiseaseJump(const double encoderMile,const int diseaseID);

	//三维点云视图跳转
	void road3dDiseaseJump(const double encoderMile, const int diseaseID);

	//景观图片跳转
	void streetJump(const double encoderMile);

private:
	//初始化表头
	void initTableHeader(QStandardItemModel *model);

	//获取所有病害
	QVector<hnRoadDiseaseInfo>  getAllDisease();

	//添加病害到表上
	void addDiseaseToTable(QVector<hnRoadDiseaseInfo> diseases, QStandardItemModel *model);

	//病害等级int转QString
	QString diseaseLevelIntToQString(int intLevel);

	//填充下拉框
	void populateComboBox();


	//有序插入并选中
	void insertAndSelectRow(QStandardItemModel* model, 
		QTableView *view, 
		const QList<QStandardItem*>&newRow,
		int mileageColIndex);


	hnRoadDiseaseInfo getUserSelectDisease(int index );

	QModelIndex getUserSelectIndex(QSortFilterProxyModel * proxy,
		const hnRoadDiseaseInfo& disease,
		int searchCol = 0, int role = Qt::UserRole);


	//移动滚动条
	void MoveScrollBar(QTableView * view, QModelIndex index);
	 
private:
	//id所在的列
	int m_idColumn;
	//表名所在的列
	int m_tableNameColumn;
	//中心里程所在的列
	int m_centerMileColumn;

	//病害类型列
	int m_diseaseTypeColumn;

private:
	customTableView *m_view;
	QStandardItemModel *m_model;
	QComboBox * filterComboBox;
	//代理模型
//	QSortFilterProxyModel*  proxyModel;
	//筛选病害类型
	QSortFilterProxyModel* sortDiseaseTypeModel;
};
