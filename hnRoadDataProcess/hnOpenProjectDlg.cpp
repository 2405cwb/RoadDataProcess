#include "hnOpenProjectDlg.h"
#include <QDoubleValidator>
#include <QFile>
#include <QtXml/QDomDocument>
#include <QKeyEvent>
#include <QTextCodec>  
#include <QMessageBox>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QCheckBox>
#include <QXmlStreamReader>
#include "..\hnQtCommon\BaseException.h"

hnOpenProjectDlg::hnOpenProjectDlg(const hnCommon::PROJECT_TYPE type, const QStringList &roadTypeNames, std::vector<hnCommon::hnProjectDataInfo> projectSettingInfos, QWidget *parent)
	: QDialog(parent)
{
#if 1
	//安装事件过滤
	installEventFilter(this);
	//获取传入的所有公路等级
	this->m_roadTypeNames = roadTypeNames;

	//获取传入的工程信息
	this->m_projectDataInfos = projectSettingInfos;

	this->m_mainGridLayout = new QGridLayout(this);

	this->m_scrollAreaGridLayout = new QGridLayout();
	this->m_scrollAreaGridLayout->setHorizontalSpacing(20);
	this->m_scrollAreaGridLayout->setVerticalSpacing(10);

	this->m_projectNameLabel = new QLabel(QString::fromLocal8Bit("工程名称"), this);
	this->m_scrollAreaGridLayout->addWidget(m_projectNameLabel, 0, 0);

	this->m_roadTypeLabel = new QLabel(QString::fromLocal8Bit("初始道路等级"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadTypeLabel, 0, 1);

	this->m_drawDiseaseModelLabel = new QLabel(QString::fromLocal8Bit("病害模式"), this);
	this->m_scrollAreaGridLayout->addWidget(m_drawDiseaseModelLabel, 0, 2);

	this->m_roadWidthLabel = new QLabel(QString::fromLocal8Bit("道路宽度"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadWidthLabel, 0, 3);

	this->m_roadMaterialLabel = new QLabel(QString::fromLocal8Bit("初始道路材质"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadMaterialLabel, 0, 4);

	this->m_roadLevelLabel = new QLabel(QString::fromLocal8Bit("初始道路等级"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadLevelLabel, 0, 5);

	this->m_roadStartMileLabel = new QLabel(QString::fromLocal8Bit("原始起点"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadStartMileLabel, 0, 6);

	this->m_roadEndMileLable= new QLabel(QString::fromLocal8Bit("原始终点"), this);
	this->m_scrollAreaGridLayout->addWidget(m_roadEndMileLable, 0, 7);

	m_proType = type;
	//遍历工程信息,向界面上添加工程信息
	this->addProjectInfoToWidget();
	this->m_okPushButton->setFocus();
#endif
}

std::vector<hnCommon::hnProjectDataInfo> hnOpenProjectDlg::getProjectSettingInfo()
{
	return this->m_projectDataInfos;
}

void hnOpenProjectDlg::slot_onOkPushButtonClicked(bool isClicked)
{
	if (m_projectDataInfos.empty())
	{
		QMessageBox::information(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("当前无工程"));
		return;
	}

	int rowCount = 1;

	std::vector<hnCommon::hnProjectDataInfo> dataInfos;
	for each (auto projectDataInfo in m_projectDataInfos)
	{
		 
		//获取工程名字checkBox的选择状态
		QCheckBox *checkBox = (QCheckBox *)m_scrollAreaGridLayout->itemAtPosition(rowCount, 0)->widget();
		if (checkBox)
		{
			if (false == checkBox->checkState())
			{
				rowCount++;
				continue;
			}
		}
		else
		{
			rowCount++;
			continue;
		}

		//等级标准类型
		QComboBox* roadTypeComboBox = (QComboBox*)m_scrollAreaGridLayout->itemAtPosition(rowCount, 1)->widget();
		if (roadTypeComboBox)
		{
			strcpy(projectDataInfo.proSetInfo.strRoadStandard, roadTypeComboBox->currentText().toLocal8Bit().data());
		}

		//人工模式自动化模式类型
		QComboBox *drawDiseaseModel = (QComboBox*)m_scrollAreaGridLayout->itemAtPosition(rowCount, 2)->widget();
		if (drawDiseaseModel)
		{
			projectDataInfo.proSetInfo.nDrawType = drawDiseaseModel->currentText() == QString::fromLocal8Bit("人工模式") ? 0 : 1;
			if (drawDiseaseModel->currentText() == QString::fromLocal8Bit("设计模式"))
			{
				projectDataInfo.proSetInfo.nDrawType = 2;
			}
		}
		//路面宽度
		QLineEdit *roadWithLineEdit = (QLineEdit*)m_scrollAreaGridLayout->itemAtPosition(rowCount, 3)->widget();
		if (roadWithLineEdit)
		{
			projectDataInfo.proSetInfo.dRoadWidth = roadWithLineEdit->text().toDouble();
		}
		//道路材质
		QComboBox *raodMaterial = qobject_cast<QComboBox*>(m_scrollAreaGridLayout->itemAtPosition(rowCount, 4)->widget());
		if (raodMaterial)
		{
			projectDataInfo.proSetInfo.nRSurfaceType = raodMaterial->currentText() == QStringLiteral("沥青") ? 0 : raodMaterial->currentText() == QStringLiteral("水泥") ? 1 : 2;
		}
		//道路等级
		QComboBox *roadLevelComboBox = qobject_cast<QComboBox*>(m_scrollAreaGridLayout->itemAtPosition(rowCount, 5)->widget());
		QString roadStandard = QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard);
		QStringList grads = this->getRoadLevels(roadStandard);
		if (roadLevelComboBox)
		{
			QString curGradStr = roadLevelComboBox->currentText();
			int gradIdx = grads.indexOf(curGradStr);
			if (gradIdx !=-1)
			{
				strcpy(projectDataInfo.proSetInfo.strRoadLevel, roadLevelComboBox->currentText().toLocal8Bit().data());
				projectDataInfo.proSetInfo.nGradIndex = gradIdx;
			}
			else
			{
				THROW_RUNTIME(QStringLiteral("当前工程%1道路等级【%2】不存在!").arg(QString::fromLocal8Bit(projectDataInfo.strProJectName)).arg(curGradStr));

			}

		}

		rowCount++;
		dataInfos.push_back(projectDataInfo);
	
	}
	//写入xml配置文件
	this->writeProjectInfoToFile1(dataInfos);

	m_projectDataInfos = dataInfos;
	

	this->accept();
}


void hnOpenProjectDlg::slot_onCancelPushButtonClicked(bool isClicked)
{
	this->reject();
}

void hnOpenProjectDlg::slot_onRoadTypeComboBoxCurrentTextChanged(const QString & text)
{
	QObject *senderObj = sender();
	if (!senderObj)
	{
		return;
	}
	QComboBox *comboBox = nullptr;
	comboBox = qobject_cast<QComboBox*>(senderObj);
	if (comboBox == nullptr)
	{
		return;
	}
	QString mytext = comboBox->currentText();
	int row, column, rowSpan, columnSpan;
	if (!m_scrollAreaGridLayout)
	{
		return;
	}
	int comboBoxIdx = m_scrollAreaGridLayout->indexOf(comboBox);
	this->m_scrollAreaGridLayout->getItemPosition(comboBoxIdx, &row, &column, &rowSpan, &columnSpan);

	QString roadTypeQString = text;

	HnProjectEnums::StandardParmTypeEnum roadTypeEnum = HnProjectEnums::roadTypeQStringToEnum(text);


	//获取该标准的道路绘制模式
	QComboBox *drawTypeComboBox = qobject_cast<QComboBox*>(this->m_scrollAreaGridLayout->itemAtPosition(row, 2)->widget());
	if (drawTypeComboBox)
	{
		//清空该行道路材质的comboBox
		drawTypeComboBox->clear();
		//添加新的道路材质
		drawTypeComboBox->addItems(this->getDrawTypes(roadTypeQString));
	}


	 

	//获取该行道路材质的comboBox
	QComboBox *roadMaterialComboBox = qobject_cast<QComboBox*>(this->m_scrollAreaGridLayout->itemAtPosition(row, 4)->widget());
	if (roadMaterialComboBox)
	{
		//清空该行道路材质的comboBox
		roadMaterialComboBox->clear();
		//添加新的道路材质
		roadMaterialComboBox->addItems(this->getRoadMeterials(roadTypeQString));
	}


	//获取该行路面等级的comboBox
	QComboBox *roadLevelComboBox = qobject_cast<QComboBox*>(this->m_scrollAreaGridLayout->itemAtPosition(row, 5)->widget());
	if (roadLevelComboBox)
	{
		//清空该行路面等级的comboBox
		roadLevelComboBox->clear();
		//添加新的路面等级
		roadLevelComboBox->addItems(this->getRoadLevels(roadTypeQString));
	}

}

void hnOpenProjectDlg::slot_onCheckAllCheckBoxCheckStateChangeed(bool state)
{
	int rowCount = m_scrollAreaGridLayout->rowCount();
	for (int row = 0; row < rowCount; row++)
	{
		auto item = m_scrollAreaGridLayout->itemAtPosition(row, 0);
		if (item)
		{
			QWidget * widget = item->widget();

			QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget);
			if (checkBox)
			{
				checkBox->setChecked(state);
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}
	}
}

void hnOpenProjectDlg::slot_onApplyAllPushButtonClicked(bool isClicked)
{
	applyAllDialog dialog(m_roadTypeNames);

	
	const int rc = dialog.exec();
	if (QDialog::Accepted == rc)
	{
		//获取顶上的三个参数
		QString roadType = dialog.getRoadType();
		QString drawDiseaseModel = dialog.getFrameType();
		QString roadWidth = dialog.getRoadWidth();
		//把下面的所有工程属性应用上面的参数
		int rowCount = 1;
		for (auto idx = 0; idx < m_projectDataInfos.size(); idx++)
		{
			QComboBox *comboBox1 = (QComboBox *)this->m_scrollAreaGridLayout->itemAtPosition(rowCount, 1)->widget();
			comboBox1->setCurrentText(roadType);
			QComboBox *comboBox2 = (QComboBox *)this->m_scrollAreaGridLayout->itemAtPosition(rowCount, 2)->widget();
			comboBox2->setCurrentText(drawDiseaseModel);
			QLineEdit *lineEdit = (QLineEdit *)this->m_scrollAreaGridLayout->itemAtPosition(rowCount, 3)->widget();
			lineEdit->setText(roadWidth);
			rowCount++;
		}
	}
	else
	{
		return;
	}
}


 
QMap<QString, QMap<QString,QString>> hnOpenProjectDlg::readKeyValueFile(QString fileNme)
{
	QMap<QString, QMap<QString, QString>> keyValueMap;
	QFile file(fileNme);
	if (file.open(QIODevice::ReadOnly|QIODevice::Text))
	{
		QTextStream in(&file);
		in.setCodec("UTF-8");
		QString currentSection;
		while (!in.atEnd())
		{
			QString line = in.readLine();
			if (line.startsWith(QStringLiteral("["))&&line.endsWith(QStringLiteral("]")))
			{
				currentSection = line.mid(1, line.length() - 2);
			}
			else
			{
				QStringList keyValue = line.split(QStringLiteral(":"));
				bool hasValue = false;
				if (keyValue.size()<=1)
				{
					hasValue = false;
					keyValue = line.split(QStringLiteral("："));
					if (keyValue.size()<=1)
					{
						hasValue = false;
					}
					else
					{
						hasValue = true;
					}
				}
				else
				{
					hasValue = true;
				}
				if (keyValue.size()==2)
				{
					QString key = keyValue[0].trimmed();
					QString value = keyValue[1].trimmed();
					keyValueMap[currentSection][key] = value;
				}
			}
		}
		file.close();
	}
	return keyValueMap;
}

void hnOpenProjectDlg::writeKeyValue(QMap<QString, QMap<QString, QString>>& keyValueMap, QString section, QString key, QString value)
{
	if (keyValueMap.contains(section)&&keyValueMap[section].contains(key))
	{
		keyValueMap[section][key] = value;
	}
}

void hnOpenProjectDlg::writeKeyValueFile(QString fileName, QMap<QString, QMap<QString, QString>> keyValueMap)
{
	QFile file(fileName);
	if (file.open(QIODevice::WriteOnly|QIODevice::Text))
	{
		QTextStream out(&file);
		out.setCodec("UTF-8");
		for (auto it = keyValueMap.begin();it!=keyValueMap.end();++it)
		{
			out <<QStringLiteral( "[") << it.key() << QStringLiteral("]\n");
			for (auto jt = it.value().begin(); jt != it.value().end(); ++jt)
			{
				out << jt.key() << QStringLiteral("：" )<< jt.value() << QStringLiteral("\n");
			}
			out << QStringLiteral("\n");
		}
		file.close();
	}
}



void hnOpenProjectDlg::addProjectInfoToWidget()
{
	int rowCount = 1;

	for each (auto projectDataInfo in this->m_projectDataInfos)
	{
		QString roadStandard = QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard);
		bool isNew2dProject = false;
		if (roadStandard.isNull())
		{
			//二维软件 未打开过的新工程
			isNew2dProject = true;
		}
		HnProjectEnums::StandardParmTypeEnum type = HnProjectEnums::roadTypeQStringToEnum(roadStandard);
		roadStandard = HnProjectEnums::roadTypeEnumToQString(type);
		//工程名字checkBox
		QCheckBox * checkBox;
		if (projectDataInfo.proSetInfo.nWorkType == PROJECT_TYPE::PROJECT_23D_TYPE)
		{
			checkBox = new QCheckBox(QString::fromLocal8Bit(projectDataInfo.str2DProName), this);
		}
		else if (projectDataInfo.proSetInfo.nWorkType == PROJECT_TYPE::PROJECT_2D_TYPE)
		{
			checkBox = new QCheckBox(QString::fromLocal8Bit(projectDataInfo.str2DProName), this);
		}
		else
		{
			checkBox = new QCheckBox(QString::fromLocal8Bit(projectDataInfo.str3DProName), this);
		}
	
		checkBox->setCheckState(Qt::Checked);
		this->m_scrollAreaGridLayout->addWidget(checkBox, rowCount, 0);

		//道路类型comboBoxprojectSettingInfo.strRoadType
		QComboBox *roadStandardComboBox = new QComboBox(this);

		roadStandardComboBox->addItems(this->m_roadTypeNames);
		roadStandardComboBox->setCurrentText(roadStandard);

		this->m_scrollAreaGridLayout->addWidget(roadStandardComboBox, rowCount, 1);
		//框选模式comboBox
		QComboBox *drawDiseaseModelComboBox = new QComboBox(this); 
		//drawDiseaseModelComboBox->addItem(QString::fromLocal8Bit("自动化模式"));
		//添加新的道路材质
		drawDiseaseModelComboBox->addItems(this->getDrawTypes(roadStandard));
		//TEST 打包暂时禁用设计模式
#if 1
		//drawDiseaseModelComboBox->addItem(QString::fromLocal8Bit("设计模式"));
#endif
		QString drawDiseaseModel = projectDataInfo.proSetInfo.nDrawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式");
		if (2 == projectDataInfo.proSetInfo.nDrawType)
		{
			drawDiseaseModel = QString::fromLocal8Bit("设计模式");
		}


		drawDiseaseModelComboBox->setCurrentText(drawDiseaseModel);
		this->m_scrollAreaGridLayout->addWidget(drawDiseaseModelComboBox, rowCount, 2);
		//道路宽度lineEdit
		QLineEdit *lineEdit = new QLineEdit(this);
		lineEdit->setMaximumWidth(50);
		lineEdit->setText(QString::number(projectDataInfo.proSetInfo.dRoadWidth));
		this->m_scrollAreaGridLayout->addWidget(lineEdit, rowCount, 3);
		//路面材质，沥青、水泥等。
		QComboBox *roadMaterialComboBox = new QComboBox(this);
		QStringList stands = this->getRoadMeterials(roadStandard);
		roadMaterialComboBox->addItems(stands);
		this->m_scrollAreaGridLayout->addWidget(roadMaterialComboBox, rowCount, 4);
		int idx = projectDataInfo.proSetInfo.nRSurfaceType;

		if (idx>=stands.size())
		{
			roadMaterialComboBox->setCurrentIndex(0);
		}
		else
		{
			roadMaterialComboBox->setCurrentIndex(idx);
		}
	
		QString material = projectDataInfo.proSetInfo.nRSurfaceType == 0 ? QStringLiteral("沥青") : projectDataInfo.proSetInfo.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		roadMaterialComboBox->setCurrentText(material);
		//道路等级
		QComboBox *roadLevelComboBox = new QComboBox(this);
		this->m_scrollAreaGridLayout->addWidget(roadLevelComboBox, rowCount, 5);
		QStringList grads = this->getRoadLevels(roadStandard);
		roadLevelComboBox->addItems(grads);

		QString curGrad = QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadLevel);
		int gradIdx = grads.indexOf(curGrad);
		if (gradIdx!=-1)
		{
			roadLevelComboBox->setCurrentText(curGrad);
			//
			projectDataInfo.proSetInfo.nGradIndex = gradIdx;
		}
		else
		{
			roadLevelComboBox->setCurrentText(0);
			//
			projectDataInfo.proSetInfo.nGradIndex = 0;
			//THROW_RUNTIME(QStringLiteral("当前工程%1道路等级【%2】不存在!").arg(QString::fromLocal8Bit( projectDataInfo.strProJectName)).arg(curGrad));
		} 
		connect(roadStandardComboBox, &QComboBox::currentTextChanged,
			this, &hnOpenProjectDlg::slot_onRoadTypeComboBoxCurrentTextChanged);


		

		QLineEdit *lineSmileEdit = new QLineEdit(this);
		lineSmileEdit->setMaximumWidth(50);
		lineSmileEdit->setReadOnly(true);
		lineSmileEdit->setText(QString::number(projectDataInfo.proSetInfo.dBegMile,'f',0));
		this->m_scrollAreaGridLayout->addWidget(lineSmileEdit, rowCount, 6);

		QLineEdit *lineEmileEdit = new QLineEdit(this);
		lineEmileEdit->setReadOnly(true);
		lineEmileEdit->setMaximumWidth(50);
		lineEmileEdit->setText(QString::number(projectDataInfo.proSetInfo.dEndMile,'f',0));
		this->m_scrollAreaGridLayout->addWidget(lineEmileEdit, rowCount, 7);
		rowCount++;
	}

	//添加确定按钮
	m_okPushButton = new QPushButton(QString::fromLocal8Bit("确定"));
	m_okPushButton->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
	connect(m_okPushButton, &QPushButton::clicked, this, &hnOpenProjectDlg::slot_onOkPushButtonClicked);

	//添加取消按钮
	m_cancelPushButton = new QPushButton(QString::fromLocal8Bit("取消"));
	m_cancelPushButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	connect(m_cancelPushButton, &QPushButton::clicked, this, &hnOpenProjectDlg::slot_onCancelPushButtonClicked);

	//全选
	m_checkAllCheckBox = new QCheckBox(QString::fromLocal8Bit("全选"));
	m_checkAllCheckBox->setCheckState(Qt::Checked);
	connect(m_checkAllCheckBox, &QCheckBox::stateChanged, this, &hnOpenProjectDlg::slot_onCheckAllCheckBoxCheckStateChangeed);

	//一键配置
	m_applyAllPushButton = new QPushButton(QString::fromLocal8Bit("一键配置"));
	connect(m_applyAllPushButton, &QPushButton::clicked, this, &hnOpenProjectDlg::slot_onApplyAllPushButtonClicked);

	this->m_scrollArea = new QScrollArea();
	this->m_scrollAreaWidget = new QWidget();

	this->m_scrollAreaWidget->setLayout(m_scrollAreaGridLayout);
	this->m_scrollArea->setWidget(m_scrollAreaWidget);

	this->m_mainGridLayout->addWidget(m_scrollArea, 0, 0);

	int scrollBarWidth = this->m_scrollArea->verticalScrollBar()->sizeHint().width();
	m_hBoxLayout = new QHBoxLayout();
	m_hBoxLayout->setSpacing(10);
	m_hBoxLayout->addItem(new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Minimum));
	m_hBoxLayout->addWidget(m_checkAllCheckBox);
	m_hBoxLayout->addWidget(m_applyAllPushButton);
	QSpacerItem *spaserItem = new QSpacerItem(50, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_hBoxLayout->addItem(spaserItem);
	m_hBoxLayout->addWidget(m_okPushButton);
	m_hBoxLayout->addWidget(m_cancelPushButton);
	m_hBoxLayout->addItem(new QSpacerItem(scrollBarWidth, 20, QSizePolicy::Fixed, QSizePolicy::Minimum));

	m_mainGridLayout->addLayout(m_hBoxLayout, 1, 0);
	this->setLayout(m_mainGridLayout);
	
	this->setWindowTitle(QString::fromLocal8Bit("打开工程"));

	this->adjustWidgetSize();
}

// 
//
//void hnOpenProjectDlg::writeProjectInfoToXml(const std::vector<hnCommon::hnProjectDataInfo> selectProjects)
//{
//		if (m_proType != hnCommon::PROJECT_TYPE::PROJECT_2D_TYPE)
//		{
//			QString xmlPath = QString::fromLocal8Bit(m_projectDataInfos.at(0).strProjectPath) + "\\ProjectInfo.xml";
//			QFile file0(xmlPath);
//			if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
//			{
//				qDebug() << QStringLiteral("projectinfo.xml文件不存在!");
//			}
//			QDomDocument doc;
//			if (!doc.setContent(&file0))
//			{
//				file0.close();
//			}
//			file0.close();
//			for each (auto projectDataInfo in selectProjects)
//			{
//				//cwb 将用户更改后的数据写入 project.xml
//				//获取根节点  
//				QDomElement root = doc.documentElement();
//				//获取指定节点
//				QDomNodeList nodes = doc.elementsByTagName(QStringLiteral("工程信息"));
//				for (int i = 0; i < nodes.length(); ++i)
//				{
//					int d = nodes.length();
//					QDomNode node = nodes.at(i);
//					auto element = node.toElement();
//					QDomNodeList chils = element.elementsByTagName(QStringLiteral("二维工程名"));
//					d = chils.length();
//					QString projectName = chils.at(0).firstChild().nodeValue();
//					QString project2DName = QString::fromLocal8Bit(projectDataInfo.str2DProName);
//					if (project2DName.contains(projectName))
//					{
//						{
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路类型")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard));
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						}
//						{
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路等级")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadLevel));
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						}
//						{
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("路面材质")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QString roadSurfaceType = projectDataInfo.proSetInfo.nRSurfaceType == 0 ? QStringLiteral("沥青") : projectDataInfo.proSetInfo.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(roadSurfaceType);
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						}
//						{
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("行车方向")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QString line = projectDataInfo.proSetInfo.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行");
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(line);
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						}
//						{
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路宽度")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QString width = QString::number(projectDataInfo.proSetInfo.dRoadWidth);
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(width);
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						} {
//							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("病害绘制模式")).at(0);
//							QDomNode oldNode = tempNode0.firstChild();
//							if (oldNode.isText())
//							{
//								QString oldValue = oldNode.nodeValue();
//								QString drawType = QString::number(projectDataInfo.proSetInfo.nDrawType);
//								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(drawType);
//								tempNode0.replaceChild(newTextNode, oldNode);
//							}
//						}
//
//					}
//				}  
//
//
//				writeProjectInfoToTxt(projectDataInfo);
//			} 
//			QFile file(xmlPath);
//			if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) //清空原来的内容并写新的内容
//			{
//				QTextStream out(&file);
//				out.setCodec("UTF-8");
//				out << doc.toString();
//				file.close();
//			}
//		
//
//		}
//		else
//		{
//			for each (auto projectDataInfo in selectProjects)
//			{
//				writeProjectInfoToTxt(projectDataInfo); 
//			}
//		} 	
//}

void hnOpenProjectDlg::writeProjectInfoToTxt(const hnCommon::hnProjectDataInfo projectDataInfo, bool is2d)
{ 
		QString surface = projectDataInfo.proSetInfo.nRSurfaceType == 0 ? QStringLiteral("沥青") : projectDataInfo.proSetInfo.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		//将用户设置信息写入文本
		QString txtPath = QString::fromLocal8Bit(projectDataInfo.strProjectPath) + "\\ProjectInfo.txt";
		if (!is2d)
		{
			txtPath = QString::fromLocal8Bit(projectDataInfo.strProjectPath) + "\\"+ QString::fromLocal8Bit( projectDataInfo.str2DProName)+ "\\ProjectInfo.txt";
		}

		QFile file0(txtPath);
		if (!file0.open(QIODevice::ReadWrite | QIODevice::Text))
		{
			qDebug() << QStringLiteral("projectinfo.txt文件不存在!");
		}
		else
		{
			bool hasSettingInfo = false;
			QTextStream stream(&file0);
			stream.setCodec("UTF-8");
			stream.seek(0);
			while (!stream.atEnd())
			{
				QString txtLine = stream.readLine();
				qDebug() << txtLine;
				if (txtLine.contains(QStringLiteral("二三维设置信息")))
				{
					hasSettingInfo = true;
					break;
				}

			}
			file0.close();
			if (hasSettingInfo)
			{

				QMap<QString, QMap<QString, QString>>  keyValueMap = readKeyValueFile(txtPath);

				writeKeyValue(keyValueMap, QStringLiteral("工程采集信息"), QStringLiteral("公路等级"), QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadLevel));
				writeKeyValue(keyValueMap, QStringLiteral("工程采集信息"), QStringLiteral("路面材质"), surface);
				writeKeyValue(keyValueMap, QStringLiteral("二三维设置信息"), QStringLiteral("道路类型"), QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard));
				writeKeyValue(keyValueMap, QStringLiteral("二三维设置信息"), QStringLiteral("道路宽度"), QString::number(projectDataInfo.proSetInfo.dRoadWidth));
				writeKeyValue(keyValueMap, QStringLiteral("二三维设置信息"), QStringLiteral("病害绘制模式"), QString::number(projectDataInfo.proSetInfo.nDrawType));
				writeKeyValueFile(txtPath, keyValueMap);
			}
			else
			{
				QMap<QString, QMap<QString, QString>>  keyValueMap = readKeyValueFile(txtPath);
				writeKeyValue(keyValueMap, QStringLiteral("工程采集信息"), QStringLiteral("公路等级"), QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadLevel));
				writeKeyValue(keyValueMap, QStringLiteral("工程采集信息"), QStringLiteral("路面材质"), surface);
				keyValueMap[QStringLiteral("二三维设置信息")][QStringLiteral("道路类型")] = QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard);
				keyValueMap[QStringLiteral("二三维设置信息")][QStringLiteral("道路宽度")] = QString::number(projectDataInfo.proSetInfo.dRoadWidth);
				keyValueMap[QStringLiteral("二三维设置信息")][QStringLiteral("病害绘制模式")] = QString::number(projectDataInfo.proSetInfo.nDrawType);
				writeKeyValueFile(txtPath, keyValueMap);
			}

		}
	
}

void hnOpenProjectDlg::writeProjectInfoToFile1(const std::vector<hnCommon::hnProjectDataInfo> selectProjects)
{
	{

		if (m_proType != hnCommon::PROJECT_TYPE::PROJECT_2D_TYPE)
		{ 
			for each (auto projectDataInfo in selectProjects)
			{
				writeProjectInfoToTxt(projectDataInfo,false);
				QString xmlPath = QString::fromLocal8Bit(projectDataInfo.strProjectPath) + "\\ProjectInfo.xml";
				QFile file0(xmlPath);
				if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					qDebug() << QStringLiteral("projectinfo.xml文件不存在!");
				}
				QDomDocument doc;
				if (!doc.setContent(&file0))
				{
					file0.close();
				}
				file0.close();

				//cwb 将用户更改后的数据写入 project.xml
				//获取根节点  
				QDomElement root = doc.documentElement();
				//获取指定节点
				QDomNodeList nodes = doc.elementsByTagName(QStringLiteral("工程信息"));
				for (int i = 0; i < nodes.length(); ++i)
				{
					int d = nodes.length();
					QDomNode node = nodes.at(i);
					auto element = node.toElement();
					QDomNodeList chils = element.elementsByTagName(QStringLiteral("二维工程名"));
					d = chils.length();
					QString projectName = chils.at(0).firstChild().nodeValue();
					QString project2DName = QString::fromLocal8Bit(projectDataInfo.str2DProName);
					if (project2DName.contains(projectName))
					{
						{
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路类型")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadStandard));
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						}
						{
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路等级")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(QString::fromLocal8Bit(projectDataInfo.proSetInfo.strRoadLevel));
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						}
						{
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("路面材质")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QString roadSurfaceType = projectDataInfo.proSetInfo.nRSurfaceType == 0 ? QStringLiteral("沥青") : projectDataInfo.proSetInfo.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(roadSurfaceType);
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						}
						{
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("行车方向")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QString line = projectDataInfo.proSetInfo.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行");
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(line);
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						}
						{
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("道路宽度")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QString width = QString::number(projectDataInfo.proSetInfo.dRoadWidth);
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(width);
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						} {
							QDomNode tempNode0 = element.elementsByTagName(QStringLiteral("病害绘制模式")).at(0);
							QDomNode oldNode = tempNode0.firstChild();
							if (oldNode.isText())
							{
								QString oldValue = oldNode.nodeValue();
								QString drawType = QString::number(projectDataInfo.proSetInfo.nDrawType);
								QDomText newTextNode = tempNode0.ownerDocument().createTextNode(drawType);
								tempNode0.replaceChild(newTextNode, oldNode);
							}
						}

					}
				}

				QFile file(xmlPath);
				if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) //清空原来的内容并写新的内容
				{
					QTextStream out(&file);
					out.setCodec("UTF-8");
					out << doc.toString();
					file.close();
				}
			}
		}
		else
		{
			for each (auto projectDataInfo in selectProjects)
			{
				writeProjectInfoToTxt(projectDataInfo,true);
			}
		}
	}



		
}

QStringList hnOpenProjectDlg::getRoadMeterials(const QString & roadStandard)
{
	HnProjectEnums::StandardParmTypeEnum roadStandardEnum = HnProjectEnums::roadTypeQStringToEnum(roadStandard);
	QVector<QString> vector = hnApp::hnDataManager::getDataManager()->getRoadSurfaceTypes(roadStandardEnum);
	QStringList stringList;
	for (auto &str : vector)
	{
		stringList.append(str);
	}
	return stringList;
}

 QStringList hnOpenProjectDlg::getDrawTypes(const QString & roadStandard)
{
	 HnProjectEnums::StandardParmTypeEnum roadStandardEnum = HnProjectEnums::roadTypeQStringToEnum(roadStandard);

	 QVector<QString> vector = hnApp::hnDataManager::getDataManager()->getDrawTypes(roadStandardEnum);
	 QStringList stringList;
	 for (auto &str : vector)
	 {
		 stringList.append(str);
	 }
	 return stringList;
}

QStringList hnOpenProjectDlg::getRoadLevels(const QString & roadStandard)
{
	HnProjectEnums::StandardParmTypeEnum roadStandardEnum = HnProjectEnums::roadTypeQStringToEnum(roadStandard);
	QVector<QString> vector = hnApp::hnDataManager::getDataManager()->getRoadLevel(roadStandardEnum);
	QStringList stringList;
	for (auto &str : vector)
	{
		stringList.append(str);
	}
	return stringList;
}

void hnOpenProjectDlg::adjustWidgetSize()
{
	QSize scrollAreaSize = this->m_scrollAreaGridLayout->sizeHint();
	QSize hBoxLayoutSize = m_hBoxLayout->sizeHint();

	int scrollBarWidth = this->m_scrollArea->verticalScrollBar()->sizeHint().width();

	int height = scrollAreaSize.height() + scrollBarWidth * 2 + hBoxLayoutSize.height() + m_mainGridLayout->verticalSpacing();
	if (height > 600)
	{
		height = 600;
	}

	QSize size;
	size.setHeight(height);
	int width = scrollAreaSize.width() + scrollBarWidth * 2;
	size.setWidth(width);

	this->resize(size);
}

