#pragma once

#include <QWidget>
#include "..\hnApplication\hnStreetCameraView.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QSlider>
#include <QRadioButton>
using namespace hnApp;

//class hnStreetWidget : public QWidget
class hnStreetWidget : public QMainWindow
{
	Q_OBJECT

public:
	hnStreetWidget(QWidget *parent = Q_NULLPTR);
	~hnStreetWidget();

public:
	// 初始化视图
	void initView();

	// 清空数组
	void clearPix();

	int getStreetShowModel() { return m_showModelIndex; }

    QString currentStreetImagePath() const;

protected:

	virtual void resizeEvent(QResizeEvent *);

protected:
	void enterEvent(QEvent *event) override;

signals:
	void signal_enterWidget();

protected:
	// 重新加载数据
	void reloadData();

signals:
	void signal_imageIdxChanged(int imageIdx);


signals:
	//信号 数据库添加病害
	//void signal_addDisease(hnRoadDiseaseInfo disease,bool modify);
signals:
	//信号 数据库删除病害
//	void signal_deleteDisease(hnRoadDiseaseInfo disease);

	void signal_updateBrightness(int value);
public slots :
    // 更新图像
    void updateViewImage(int nIndex);


	void slot_updateBrightness(int value);




private:
	void updateCurViewImage();

public:
	// 单景观
	hnStreetCameraView* m_pStreetView;

	// 双景观
	hnStreetCameraView* m_pStreetViewDouble;


	hnStreetCameraView* getLeftPixWidget() { return m_pStreetView; }
	hnStreetCameraView* getRightPixWidget() { return m_pStreetViewDouble; }
private:
	// 是否双景观
	bool m_bDoubleStreet;

	//当前的帧号
	int m_currentFrameIdx;

private:
	//主布局
	QHBoxLayout *m_mainLayout;
	
	 
	//选择显示模式，仅双景观存在  0 左 1右 2双边
	QComboBox * showModelComBox;

	//选择是否进入测量宽度模式
	QRadioButton * caculateRoadWidthBtn;

	//工具栏
	QToolBar * toolBar;


	//左景观视图
	QLabel *m_leftImgLabel;
	 
	//右景观视图
	QLabel *m_rightImgLabel; 

	//亮度调节
	QSlider * brightnessSlider;

	bool m_rightPicNeedRotate;

	//用户在界面上选择 先择显示的 模式 0 双景观   1左景观 2右景观
	int m_showModelIndex = 0 ;
};
