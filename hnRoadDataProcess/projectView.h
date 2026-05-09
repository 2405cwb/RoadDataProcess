/*! hnIPS2ViewPluginIO.h
********************************************************************************
<PRE>
模块名       :  projectView
文件名       :  projectView.h
相关文件     :  projectView.cpp
文件实现功能 : 实现工程界面 较桩界面 打标界面的显示 与相应指标的修改及 双击条目跳转到相应桩号图像位置
作者         : 程文博
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日期         版本     修改人              修改内容
2024/0825	 1.0     程文博          创建，实现
</PRE>
*******************************************************************************/

#pragma once

#include <QWidget>
#include "ui_projectView.h"
#include "../hnCommon/hnRoadStruct.h"
class projectView : public QWidget
{
	Q_OBJECT

public:
	projectView(QWidget *parent = Q_NULLPTR);
	~projectView();
private:


	signals:
	   //跳转桩号
	void signal_jumpToMile(double value);
	
	//更新所有视图
	void signal_updateAllWidget();
public slots:

//更新工程信息界面 更新打标界面  更新较桩界面
void slot_updateProjectSetting(hnCommon::hnProjectSetInfo setting, QVector<hnCommon::hnMarkInfo> marks, QVector<hnCommon::hnMilePile> pile);

//用户双击界面条目发生跳转
void slot_doubleClickTableVidgetItem(QTableWidgetItem * item);

//打标combox发生变化 相应条目改变
void slot_MarkComboxIndexChanged(int index);

//更新桩号 里程值
void slot_updateMileAndDmi(double mile,double dmi);

//重载项目按钮
void slot_initProjectClicked();

//打标按钮按下
void slot_addMarkClicked();

//较桩按钮按下
void slot_addMilePileClicked();

//打标窗口的右键菜单
void MarkMenuClicked(const QPoint &pos);

//校桩窗口的右键菜单
void MilePileMenuClicked(const QPoint &pos);

//打标列表的删除操作
void deleteMark();

//校桩列表的删除操作
void deletePipe();

//用户输入桩号 更新里程框
void updateDmiTxt(const QString &text);
public:
	//反序排序
	 static bool compareDeScendingMark(const hnCommon::hnMarkInfo& a, const hnCommon::hnMarkInfo& b)
	{
		return a.dTrueMile > b.dTrueMile;
	}
	
	//反序排序
	static bool compareDeScendingPile(const hnCommon::hnMilePile& a, const hnCommon::hnMilePile& b)
	{
		return a.dTrueMile > b.dTrueMile;
	}

private:
//更新打标窗口  打标列表  ,是否上行
	void updateMarkFrom( QVector<hnCommon::hnMarkInfo>& marks,bool isUp);

	//更新较桩窗口 校桩列表  ,是否上行
	void updatePileFrom( QVector<hnCommon::hnMilePile>& pile,bool isUp);

protected:
	void resizeEvent(QResizeEvent *event) override;
private:
	Ui::projectView ui;
};
