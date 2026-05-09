#pragma once

#include <QDialog>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>

class adjustImageWidget : public QWidget
{
	Q_OBJECT

public:
	adjustImageWidget(QWidget *parent = Q_NULLPTR);
	~adjustImageWidget();
	
signals:
	//信号 二维视图 对比度值变化
	void signal_2dContrastIntensityChanged(double intensity);
signals:
	//信号 二维视图 重置对比度值
	void signal_2dResetContrastIntensity();
signals:
	//信号 三维视图 对比度值变化
	void signal_3dContrastIntensityChanged(double intensity);
signals:
	//信号 三维视图 重置对比度值
	void signal_3dResetContrastIntensity();
protected:
	//槽函数  对比度滑动条值变化
	void slot_contrastSliderValueChanged(int value);

signals:
	//信号 二维视图 亮度值变化
	void signal_2dBrightnessIntensityChanged(double intensity);
signals:
	//信号 二维视图 重置亮度值
	void signal_2dResetBrightnessIntensity();
signals:
	//信号 三维视图 亮度变化
	void signal_3dBrightnessIntensityChanged(double intensity);
signals:
	//信号 三维视图 重置亮度值
	void signal_3dResetBrightnessIntensity();
protected:
	// 槽函数  亮度滑动条值变化
	void slot_brightnessSliderValueChanged(int value);


	//槽函数  重置按钮点击
	void slot_onResetButtonClicked();

private:
	//初始化界面
	void initWidgets();
	//初始化布局
	void initLayouts();
	//初始化信号槽连接
	void initSignalSlots();

private:
	QPushButton *m_resetButton;		//重置按钮

	QCheckBox *m_2dCheckBox;		//二维视图的checkBox
	QCheckBox *m_3dCheckBox;		//三维视图的checkBox

	QSlider *m_contrastSlider;		//对比度滑动条
	QSlider *m_brightnessSlider;	//亮度滑动条
	QSlider *m_sharpenSlider;		//锐化滑动条

	QLabel *m_contrastLabel;		//对比度值的label
	QLabel *m_brightnessLabel;		//亮度值的label
	QLabel *m_sharpenLabel;			//锐化度度值的label

};
