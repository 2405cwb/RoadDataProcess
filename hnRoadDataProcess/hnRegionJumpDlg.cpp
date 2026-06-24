#include "hnRegionJumpDlg.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnProject/hnProject.h"

using namespace hnApp;

hnRegionJumpDlg::hnRegionJumpDlg(QWidget *parent)
	: QDialog(parent)
{
	this->initUI();

	this->initLayout();

	this->setWindowTitle(QString::fromLocal8Bit("里程跳转"));
}

hnRegionJumpDlg::~hnRegionJumpDlg()
{
}

double hnRegionJumpDlg::getRegion()
{
	return m_region;
}

void hnRegionJumpDlg::initUI()
{
	m_encoderMileRadioButton = new QRadioButton(QString::fromLocal8Bit("里程"));
	m_encoderMileRadioButton->setChecked(true);
	m_trueMileRadioButton = new QRadioButton(QString::fromLocal8Bit("桩号"));
	m_jumpPushButton = new QPushButton(QString::fromLocal8Bit("跳转"));
	connect(m_jumpPushButton, &QPushButton::clicked, this, &hnRegionJumpDlg::slot_onJumpPushButtonClicked);

	m_regionLineEdit = new QLineEdit();
}

void hnRegionJumpDlg::initLayout()
{
	QHBoxLayout *regionSeclectLayout = new QHBoxLayout;
	regionSeclectLayout->addWidget(m_encoderMileRadioButton);
	regionSeclectLayout->addWidget(m_trueMileRadioButton);

	QHBoxLayout *jumpLayout = new QHBoxLayout;
	jumpLayout->addWidget(m_regionLineEdit);
	jumpLayout->addWidget(m_jumpPushButton);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(regionSeclectLayout);
	mainLayout->addLayout(jumpLayout);

	this->setLayout(mainLayout);
}

void hnRegionJumpDlg::slot_onJumpPushButtonClicked()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	m_region = m_regionLineEdit->text().toDouble();

	double encoderMile;
	//如果是编码器里程，就直接赋值
	if (m_encoderMileRadioButton->isChecked())
	{
		encoderMile = m_region;
	}
	//如果是真实里程，就转换成编码器里程
	else
	{
		encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(m_region);
	}

	//异常处理 如果小于0 ，就赋值为0
	if (encoderMile < 0)
	{
		encoderMile = 0;
	}

	//TODO 还需处理单二维的情况

	auto projectType = hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (PROJECT_23D_TYPE == projectType ||
		PROJECT_2D_TYPE == projectType)
	{
		auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		int imageNum = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector().size();
		if (encoderMile > imageNum * projectSetInfo.dRoadLength)
		{
			encoderMile = imageNum * projectSetInfo.dRoadLength;
		}
		double roadLenth = projectSetInfo.dRoadLength;
		if (roadLenth == 0)
		{
			return;
		}
		int frameIdx = encoderMile / roadLenth;
		emit signal_updateScrollValue(frameIdx);
	}
	else
	{
		const int roadHeight = 8;
		const int frameIdx3d = encoderMile / roadHeight;

		emit this->signal_road3dFrameIdxChanged(frameIdx3d);
	}



	
}
