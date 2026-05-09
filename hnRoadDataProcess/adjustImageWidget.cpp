#include "adjustImageWidget.h"
#include "configService.h"

adjustImageWidget::adjustImageWidget(QWidget *parent)
	: QWidget(parent)
{
	this->initWidgets();

	this->initLayouts();

	this->initSignalSlots();

	this->setWindowTitle(QString::fromLocal8Bit("图像调整"));

//	this->setFixedSize(QSize(400, 240));

}

adjustImageWidget::~adjustImageWidget()
{
}

void adjustImageWidget::slot_contrastSliderValueChanged(int value)
{
	double intensity = value / 10.0;
	//通知二维视图更新
	if (m_2dCheckBox->isChecked())
	{
		emit signal_2dContrastIntensityChanged(intensity);

	}
	//通知三维视图更新
	if (m_3dCheckBox->isChecked())
	{
		emit signal_3dContrastIntensityChanged(intensity);
	}
	//设置对比度值label
	m_contrastLabel->setText(QString::number(intensity));
}

void adjustImageWidget::slot_brightnessSliderValueChanged(int value)
{
	double intensity = value / 10.0;
	//通知二维视图更新
	if (m_2dCheckBox->isChecked())
	{
		emit signal_2dBrightnessIntensityChanged(intensity);

	}
	//通知三维视图更新
	if (m_3dCheckBox->isChecked())
	{
		emit signal_3dBrightnessIntensityChanged(intensity);
	}
	//设置亮度值label
	m_brightnessLabel->setText(QString::number(intensity));
}

void adjustImageWidget::slot_onResetButtonClicked()
{
	m_contrastSlider->setValue(10);
	m_brightnessSlider->setValue(10);

	if (m_2dCheckBox->isChecked())
	{
		emit signal_2dResetContrastIntensity();
		emit signal_2dResetBrightnessIntensity();
	}
	if (m_3dCheckBox->isChecked())
	{
		emit signal_3dResetContrastIntensity();
		emit signal_3dResetBrightnessIntensity();
	}
}

void adjustImageWidget::initWidgets()
{
	m_resetButton = new QPushButton(QString::fromLocal8Bit("重置"));

	m_2dCheckBox = new QCheckBox(QString::fromLocal8Bit("二维视图"));
	m_3dCheckBox = new QCheckBox(QString::fromLocal8Bit("三维视图"));

	m_contrastSlider = new QSlider(Qt::Horizontal);
	m_contrastSlider->setMaximum(30);
	m_contrastSlider->setValue(10);

	m_brightnessSlider = new QSlider(Qt::Horizontal);
	m_brightnessSlider->setMaximum(30);
	m_brightnessSlider->setValue(10);

	m_sharpenSlider = new QSlider(Qt::Horizontal);

	m_contrastLabel = new QLabel("1.0");
	m_brightnessLabel = new QLabel("1.0");
	m_sharpenLabel = new QLabel("1.0");
}

void adjustImageWidget::initLayouts()
{
	QHBoxLayout *checkBoxLayout = new QHBoxLayout;
	checkBoxLayout->addWidget(m_2dCheckBox);
	checkBoxLayout->addWidget(m_3dCheckBox);
	checkBoxLayout->addWidget(m_resetButton);

	QGridLayout *mainLayout = new QGridLayout;

	mainLayout->addLayout(checkBoxLayout, 0, 0, 1, 3);

	mainLayout->addWidget(new QLabel(QString::fromLocal8Bit("对比度")), 1, 0);
	mainLayout->addWidget(m_contrastSlider, 1, 1);
	mainLayout->addWidget(m_contrastLabel, 1, 2);

	//功能未实现，暂不启用
#if 0	
	mainLayout->addWidget(new QLabel(QString::fromLocal8Bit("亮度")), 2, 0);
	mainLayout->addWidget(m_brightnessSlider, 2, 1);
	mainLayout->addWidget(m_brightnessLabel, 2, 2);

	mainLayout->addWidget(new QLabel(QString::fromLocal8Bit("锐化度")), 3, 0);
	mainLayout->addWidget(m_sharpenSlider, 3, 1);
	mainLayout->addWidget(m_sharpenLabel, 3, 2);
#endif

	this->setLayout(mainLayout);
}

void adjustImageWidget::initSignalSlots()
{
	//对比度值变化
	connect(m_contrastSlider, &QSlider::valueChanged, this, &adjustImageWidget::slot_contrastSliderValueChanged);

	//对比度重置
	connect(m_resetButton, &QPushButton::clicked, this, &adjustImageWidget::slot_onResetButtonClicked);

	//亮度值变化
	connect(m_brightnessSlider, &QSlider::valueChanged, this, &adjustImageWidget::slot_brightnessSliderValueChanged);
}
