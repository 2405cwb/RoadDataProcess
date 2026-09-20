#include "addDiseaseDialog.h"
#include <QDebug>
#include <QtMath>
#include <QShortcut>
#include "..\hnConfigService\HnXRSettings.h"
addDiseaseDialog::addDiseaseDialog(const QList<QPair<QString, QString>>& diseaseNameAndKey,bool showDisProperty)
{

	m_xrSetting = HnXRSettings::getInstance();
	m_isShowDiseaseInfo = showDisProperty;

	//初始化病害参数控件
	this->initDiseaseInfoControls();

	//初始化病害类型控件
	this->initDiseaseTypeControls(diseaseNameAndKey);
	
	//初始化病害备注特性
	initMarkInfo();
	 

}

QString addDiseaseDialog::getDiseaseTypeName()
{
	return this->m_diseaseTypeName;
}

void addDiseaseDialog::setDiseaseAttributeEnabled(const bool enabeled)
{
	m_diseaseWidthLineEdit->setEnabled(enabeled);
	m_diseaseLenthLineEdit->setEnabled(enabeled);
	m_areaLineEdit->setEnabled(enabeled);
	m_depthLineEdit->setEnabled(enabeled);
}

void addDiseaseDialog::setDiseaseInfo(double lenth, double width, double area, double depth)
{
	m_diseaseWidthLineEdit->setText(QString::number(width, 'f', 4));
	m_diseaseLenthLineEdit->setText(QString::number(lenth, 'f', 4));
	m_areaLineEdit->setText(QString::number(area, 'f', 4));
	m_depthLineEdit->setText(QString("%1").arg(depth * 1000, 0, 'f', 2));
}



QString addDiseaseDialog::getDiseaseMarkInfo()
{
	QString text = "";
	if (m_xrSetting->diseaseMark)
	{
		text = markEdit->currentText();
		//该文本是否已存在
		QStringList marks;
		if (!m_xrSetting->diseaseMarkTxts.isEmpty())
		{
			  marks = m_xrSetting->diseaseMarkTxts.split(',');
		}
		
		
		 
		int index = marks.indexOf(text);
		if (index!=-1)
		{
			if (index>0)
			{
				QString item = marks.takeAt(index);
				marks.prepend(item);
			}
		}
		else
		{
			//如果是新的备注类型
			marks.prepend(text); 
		}
		QString marksRecord = marks.mid(0, 5).join(',');
		m_xrSetting->diseaseMarkTxt = text;
		m_xrSetting->diseaseMarkTxts = marksRecord;
		m_xrSetting->writeData();
	}
 	return  text;
}

bool addDiseaseDialog::isContinuousDrawingEnabled() const
{
	return m_continuousDrawingCheckBox && m_continuousDrawingCheckBox->isChecked();
}

void addDiseaseDialog::slot_onRadioButtonToggled(bool checked)
{
	auto radioButton = qobject_cast<QRadioButton*>(sender());
	if (checked && radioButton)
	{
		emit this->signal_diseaseRadioButtonToggled(radioButton->text());
		
		//处理选中病害
		QString txt = radioButton->text();
		QString disName =  txt.split("(").first();
		this->m_diseaseTypeName = disName;
		this->accept();
	}
}



void addDiseaseDialog::initDiseaseInfoControls()
{
	this->m_diseaseInfoGridLayout->addWidget(this->m_diseaseLenthLabel, 0, 0);
	this->m_diseaseInfoGridLayout->addWidget(this->m_diseaseLenthLineEdit, 0, 1);
	this->m_diseaseInfoGridLayout->addItem(this->m_lenthWidthSpacerItem, 0, 2);
	this->m_diseaseInfoGridLayout->addWidget(this->m_diseaseWidthLabel, 0, 3);
	this->m_diseaseInfoGridLayout->addWidget(this->m_diseaseWidthLineEdit, 0, 4);

	//面积
	m_areaLabel = new QLabel(QString::fromLocal8Bit("面积(㎡)"));
	m_areaLineEdit = new QLineEdit;
	//深度
	m_depthLabel = new QLabel(QString::fromLocal8Bit("深度(m)"));
	m_depthLineEdit = new QLineEdit;

	m_diseaseInfoGridLayout->addWidget(m_areaLabel, 1, 0);
	m_diseaseInfoGridLayout->addWidget(m_areaLineEdit, 1, 1);
	m_diseaseInfoGridLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2);
	m_diseaseInfoGridLayout->addWidget(m_depthLabel, 1, 3);
	m_diseaseInfoGridLayout->addWidget(m_depthLineEdit, 1, 4);

	if (m_isShowDiseaseInfo)
	{
		this->m_mainGridLayout->addWidget(m_diseaseInfoGroupBox);
	}
	
}

void addDiseaseDialog::initDiseaseTypeControls(const QList<QPair<QString, QString>>& diseaseNameAndKey)
{
	this->m_diseaseTypeGroupBox->setTitle(QString::fromLocal8Bit("病害类型"));
	int columnCount = 0;
	int rowCount = 0;
	const int maxColumnCount = qSqrt(diseaseNameAndKey.size());

	for (const auto &it:diseaseNameAndKey)
	{
		if (columnCount >= maxColumnCount)
		{
			rowCount++;
			columnCount = 0;
		}
		QString disName = it.first;
		QString disKey = it.second;
		QString btnName = QString("%1(%2)").arg(disName).arg(disKey);
		QRadioButton *button = new QRadioButton(btnName, this);

		QShortcut * shortcut = new QShortcut(QKeySequence(disKey), this);
		
		connect(shortcut, &QShortcut::activated, [button]()
		{
			button->setChecked(true);
		});
	
		connect(button, &QRadioButton::toggled, this, &addDiseaseDialog::slot_onRadioButtonToggled);
		this->m_diseaseTypeGridLayout->addWidget(button, rowCount, columnCount);
		columnCount++;
	}

	 
	this->m_diseaseTypeGridLayout->setSpacing(25);	//设置布局内控件之间的距离
	this->m_mainGridLayout->addWidget(m_diseaseTypeGroupBox);
	m_continuousDrawingCheckBox = new QCheckBox(QStringLiteral("\u8fde\u7eed\u7ed8\u5236\u76f8\u540c\u75c5\u5bb3"), this);
	m_continuousDrawingCheckBox->setToolTip(QStringLiteral("\u540e\u7eed\u7ed8\u5236\u4e0d\u518d\u5f39\u51fa\u75c5\u5bb3\u7c7b\u578b\u9009\u62e9\u7a97\u53e3"));
	this->m_mainGridLayout->addWidget(m_continuousDrawingCheckBox);
}

void addDiseaseDialog::initMarkInfo()
{
	if (m_xrSetting->diseaseMark)
	{
		markEdit = new QComboBox();
		markEdit->setEditable(true);
		if (!m_xrSetting->diseaseMarkTxts.isEmpty())
		{
			QStringList items = m_xrSetting->diseaseMarkTxts.split(',');
			markEdit->addItems(items); 
			markEdit->setCurrentText(m_xrSetting->diseaseMarkTxt);
		}
	
		m_markInfoGridLayout->addWidget(markEdit, 1, 0);
		m_mainGridLayout->addWidget(m_markInfoGroupBox); 
	}
	else
	{

	}

}



