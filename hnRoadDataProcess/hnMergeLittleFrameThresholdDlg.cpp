#include "hnMergeLittleFrameThresholdDlg.h"

hnMergeLittleFrameThresholdDlg::hnMergeLittleFrameThresholdDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle(QString::fromLocal8Bit("自动化模式病害导入"));

	this->m_hDisThreshold = 700;
	this->m_vDisThreshold = 700;

	this->ui.hThreshold->setSingleStep(10);
	this->ui.hThreshold->setRange(100, 2000);
	this->ui.hThreshold->setValue(this->m_hDisThreshold);

	this->ui.vThreshold->setSingleStep(10);
	this->ui.vThreshold->setRange(100, 2000);
	this->ui.vThreshold->setValue(this->m_hDisThreshold);

	this->m_isMerge = true;
	this->ui.autoMerge->setChecked(m_isMerge);			// 默认导入时进行病害合并

	this->m_isMap = true;
	this->ui.autoMap->setChecked(m_isMap);				// 默认导入时进行映射

	this->ui.pushButtonOK->setFocus();

	connect(this->ui.pushButtonOK, &QPushButton::clicked,
		[this]() {
		this->accept();
		this->close();
	});
	connect(this->ui.pushButtonCancel, &QPushButton::clicked,
		[this]() {
		this->reject();
		this->close();
	});
	
}

hnMergeLittleFrameThresholdDlg::~hnMergeLittleFrameThresholdDlg()
{
}

int hnMergeLittleFrameThresholdDlg::hDisThreshold()
{
	m_hDisThreshold = this->ui.hThreshold->value();
	return m_hDisThreshold;
}

int hnMergeLittleFrameThresholdDlg::vDisThreshold()
{
	m_vDisThreshold = this->ui.vThreshold->value();
	return m_vDisThreshold;
}

int hnMergeLittleFrameThresholdDlg::importMergeFlag()
{
	m_isMerge = this->ui.autoMerge->isChecked();
	return m_isMerge;
}

int hnMergeLittleFrameThresholdDlg::importMapFlag()
{
	m_isMap = this->ui.autoMap->isChecked();
	return m_isMap;
}


