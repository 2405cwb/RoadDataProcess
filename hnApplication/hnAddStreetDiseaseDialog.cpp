#include "hnAddStreetDiseaseDialog.h"
#include "hnDiseaseService.h"
hnAddStreetDiseaseDialog::hnAddStreetDiseaseDialog(QVector<hnCommon::hnDiseaseSetInfo> LJInfo, QVector<hnCommon::hnDiseaseSetInfo>YXInfo, QWidget *parent)
	: QDialog(parent)
{
	this->setWindowTitle(QString::fromLocal8Bit("添加景观病害"));

	QGridLayout *mainGridlayout = new QGridLayout(this);
	this->setLayout(mainGridlayout);

	QGroupBox *YXgroupBox = new QGroupBox(QString::fromLocal8Bit("沿线设施损害"),this);
	mainGridlayout->addWidget(YXgroupBox, 0, 0, 1, 2);
	QGroupBox *LJgroupBox = new QGroupBox(QString::fromLocal8Bit("路基损害"), this);
	mainGridlayout->addWidget(LJgroupBox, 1, 0, 1, 2);

	QPushButton *okPushButton = new QPushButton(QString::fromLocal8Bit("确定"), this);
	connect(okPushButton, &QPushButton::clicked, this, &hnAddStreetDiseaseDialog::slot_onOkPushButtonCliecked);
	mainGridlayout->addWidget(okPushButton, 2, 0, 1, 1);
	QPushButton *cancelPushButton = new QPushButton(QString::fromLocal8Bit("取消"), this);
	connect(cancelPushButton, &QPushButton::clicked, this, &hnAddStreetDiseaseDialog::slot_onCancelPushButtonCliecked);
	mainGridlayout->addWidget(cancelPushButton, 2, 1, 1, 1);

	m_LJlayout = new QGridLayout(this);
	
	m_YXlayout = new QGridLayout(this);

	YXgroupBox->setLayout(m_YXlayout);
	LJgroupBox->setLayout(m_LJlayout);

	this->m_LJDiseaseSetInfo = LJInfo;
	this->m_YXDiseaseSetInfo = YXInfo;

	//初始化路基损害布局 根据传入的信息
	this->initLJLayout();

	//初始化沿线设施损害布局 根据传入的信息
	this->initYXLayout();
	
}

hnAddStreetDiseaseDialog::~hnAddStreetDiseaseDialog()
{
}

void hnAddStreetDiseaseDialog::setCurrentHnMile(const hnMile & mile)
{
	this->m_currentMile = mile;
}

QVector<hnRoadDiseaseInfo> hnAddStreetDiseaseDialog::getSelectDiseases()
{
	return m_selectDiseases;
}

void hnAddStreetDiseaseDialog::initYXLayout()
{	
	this->initDiseaseLayout(m_YXlayout, m_YXDiseaseSetInfo);
}

void hnAddStreetDiseaseDialog::initLJLayout()
{
	this->initDiseaseLayout(m_LJlayout, m_LJDiseaseSetInfo);
}

void hnAddStreetDiseaseDialog::initDiseaseLayout(QGridLayout * layout, const QVector<hnCommon::hnDiseaseSetInfo>& setInfos)
{
	//设置横向边距
	layout->setHorizontalSpacing(30);

	//添加表头
	QLabel *diseaseTypeLabel = new QLabel(QString::fromLocal8Bit("损坏类型"), this);
	diseaseTypeLabel->setMinimumWidth(130);
	layout->addWidget(diseaseTypeLabel, 0, 0);
	layout->addWidget(new QLabel(QString::fromLocal8Bit("损坏长度(m)/面积(㎡)"), this), 0, 1);
	layout->addWidget(new QLabel(QString::fromLocal8Bit("损坏个数"), this), 0, 2);


	//遍历病害信息,初始化控件
	int rowCount = 1;
	for (auto diseaseInfo : qAsConst(setInfos))
	{
		layout->addWidget(new QCheckBox(QString::fromLocal8Bit(diseaseInfo.strDiseaseTypeName), this), rowCount, 0);
		//长度输入框
		QLineEdit *lenthLineEdit = new QLineEdit("0",this);
		layout->addWidget(lenthLineEdit, rowCount, 1);
		//损害个数输入框
		QLineEdit *countLineEdit = new QLineEdit("1",this);
		layout->addWidget(countLineEdit, rowCount, 2);
		if (diseaseInfo.dEffectMeasure == 1)
		{
			//按个数的画，长度/面积的输入框就不能输入了
			lenthLineEdit->setEnabled(false);
		}
		else
		{
			countLineEdit->setEnabled(false);
		}
		//行数+1
		rowCount++;
	}
	
}

void hnAddStreetDiseaseDialog::slot_onOkPushButtonCliecked()
{
	//清空对话框选中的病害
	m_selectDiseases.clear();

	//处理路基病害
	this->writeDiseaseDataBase(m_LJlayout, m_LJDiseaseSetInfo);

	//处理沿线设施病害
	this->writeDiseaseDataBase(m_YXlayout, m_YXDiseaseSetInfo);

	this->accept();
}

void hnAddStreetDiseaseDialog::slot_onCancelPushButtonCliecked()
{
	this->reject();
}

void hnAddStreetDiseaseDialog::writeDiseaseDataBase(QGridLayout * layout, const QVector<hnCommon::hnDiseaseSetInfo>& setInfos)
{
	//获取hnMile
	hnMile currentMile = m_currentMile;

	int rowCount = layout->rowCount();
	for (int row = 1;row < rowCount ; row++) 
	{
		QCheckBox *checkBox = qobject_cast<QCheckBox*>(layout->itemAtPosition(row, 0)->widget());
		if (checkBox)
		{
			if (!checkBox->isChecked())
			{
				continue;
			}
		}
		else
		{
			continue;
		}
		QString diseaseTypeName;
		//赋值病害类型信息
		diseaseTypeName = checkBox->text();

		QString diseaseTableName;
		//找到当前的diseseSetInfo
		hnDiseaseSetInfo currentDiseaseSetInfo;
		for (auto diseseSetInfo : setInfos)
		{
			if (QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName) == diseaseTypeName)
			{
				currentDiseaseSetInfo = diseseSetInfo;
				diseaseTableName = QString::fromLocal8Bit(diseseSetInfo.strDBTableName);
				break;
			}
		}
		//根据diseseSetInfo计算属性
		hnRoadDiseaseInfo diseaseInfo;

		if (diseaseTableName.contains("LJ"))
		{
			diseaseInfo.ndiseaseType = 2;
		}
		else if (diseaseTableName.contains("YX"))
		{
			diseaseInfo.ndiseaseType = 1;
		}

		//赋值病害个数/面积
		if (currentDiseaseSetInfo.dEffectMeasure == 1)
		{
			QLineEdit *lineEdit = qobject_cast<QLineEdit*> (layout->itemAtPosition(row, 2)->widget());
			if (lineEdit)
			{
				diseaseInfo.dArea = lineEdit->text().toDouble();
			}
		}
		else
		{
			QLineEdit *lineEdit = qobject_cast<QLineEdit*> (layout->itemAtPosition(row, 1)->widget());
			if (lineEdit)
			{
				diseaseInfo.dArea = lineEdit->text().toDouble();
			}	
		}

		diseaseInfo.dMileage = currentMile.dEnclMile;
		diseaseInfo.dRoadWidth = currentMile.roadWidth;
		diseaseInfo.dDmi = currentMile.dEnclMile;			//里程
		diseaseInfo.nLevel = currentDiseaseSetInfo.nLevel;
		diseaseInfo.nRSurfaceType = currentMile.roadType;
		strcpy(diseaseInfo.strRoadStandard, 
			HnProjectEnums::roadTypeEnumToQString(currentMile.roadStandard).toLocal8Bit().data());
		strcpy(diseaseInfo.strDiseaseTableName, currentDiseaseSetInfo.strDBTableName);
		strcpy(diseaseInfo.strDisName, currentDiseaseSetInfo.strDiseaseTypeName);

		diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());

		

	 
	  if (hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo))
	  {
		  m_selectDiseases.append(diseaseInfo);

	  }

	}
}
