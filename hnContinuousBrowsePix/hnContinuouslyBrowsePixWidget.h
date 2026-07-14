#pragma once

#pragma region 文件说明
/*! @hnContinuouslyBrowsePixWidget.h
********************************************************************************
<PRE>
模块名       : hnContinuousBrowsePix
文件名       : hnContinuouslyBrowsePixWidget.h 
相关文件     : hnBrowsePixWidget
文件实现功能 : 显示图片的窗口
作者         : 陈智超
版本         : 1.0.0
--------------------------------------------------------------------------------
备注         : 通用的连续显示图片的窗口
--------------------------------------------------------------------------------
修改记录 :
日期        版本     修改人              修改内容
2023/04/17	1.0.0	 陈智超				 创建初版
</PRE>
*******************************************************************************/

#pragma endregion


#include <QWidget>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QRectF>
#include "hnBrowsePixWidget.h"
#include <QPushButton>
#include "..\hnConfigService\HnXRSettings.h"
#include <QCheckBox>
#include <thread>
#include <chrono>
using namespace std;

class HNCONTINUOUSBROWSEPIX_EXPORT hnContinuouslyBrowsePixWidget:public QWidget
{
	Q_OBJECT

	//构造析构
public:
	hnContinuouslyBrowsePixWidget(QWidget *parent = nullptr);
	hnContinuouslyBrowsePixWidget(hnBrowsePixWidget *showPixwidget, QWidget *parent = nullptr);
	~hnContinuouslyBrowsePixWidget();

protected:
	//设置浏览图片的视图 依赖注入
	void setBrowsePixWidget(hnBrowsePixWidget *browsePixWidget,bool showToolBar);

public:
	//加载图片
	void loadPix(const QString &pixDirName);

	void loadPix(const QStringList &pixNames);

	//获取显示图片的label
	hnBrowsePixWidget* getShowPixLabel();

private:
	//初始化
	void init();

	//路面图像上方的工具栏
	void add2dToolbar();

	//三维图像上方的工具栏
	void add3dToolbar();

	//信号槽初始化
	void initSigSlot();


//信号
signals:
	//信号：发送等比例显示窗口image
	void sig_mousePosImageChanged(QImage image);

//槽函数
public slots:

	void slot_setSelectedDiseaseId(int id);

	void slot_updateBrowser();

	
	virtual void slot_BlockValueChanged(int value) = 0;
	
	//跳转到用户输入的位置
	virtual void slot_JumpToUserMile() = 0;

	virtual void slot_Show3dDeepExample() = 0;

	//更新里程文本框
	 void slot_updateDmiLable(int value);
#pragma region 事件重载
protected:
	//鼠标滚轮事件      重载
	void wheelEvent(QWheelEvent *event) override final;
	 
	void keyPressEvent(QKeyEvent *event) override;
protected:
	//进入事件
	void enterEvent(QEvent *event) override;

	void playThePicture();

	int getBrowStep(bool is3d ) const;

	virtual int browseStep() const = 0;

	virtual bool is2DView()  const = 0;

signals:
	void signal_enterWidget();
	void signal_moveMouse(bool up,bool is2D);
signals:
#pragma endregion
protected:
	//记录桩号变化的文本框
	QLineEdit* mileBlock;

	//记录里程变化的文本框
	QLineEdit* dmiBlock;
	 
	//跳转按钮
	QPushButton*  jumpBtn;

	//播放按钮
	QPushButton * playBtn;

	//加速按钮
	QPushButton * addSpeedBtn;

	//减速按钮
	QPushButton * subBtn;

	//开启备注
	QPushButton * markBtn;

	//是否显示高精度定位
	QCheckBox * showGpsBtn;


	//开启矩形框
	QPushButton * diseaseRectShowBtn;

	QPushButton *deepExampleBtn;
	
	
#pragma region 成员变量
private:
	//水平布局器
	QHBoxLayout *m_mainLayout;

public:
	//显示图片的label
	hnBrowsePixWidget *m_browsePixWidget;
	// 自动滚动标志
	bool m_autoPlay;

	//控制播放速度
	int m_playSpeed;

	protected:
	
	HnXRSettings * xrSetting;

	 
#pragma endregion


};
