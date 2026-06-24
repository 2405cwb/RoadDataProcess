#include "hnDataManager.h"	//这个要放最前面，不然会出问题
#include "depthCaculate.h"
#include "../hnConfigService/configService.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace hnApp;

depthCaculate::depthCaculate()
{
	//获取是否进行深度计算
	configService config;
	QString configName = QApplication::applicationDirPath() + "/config/XRSetting.ini";
	config.loadCfg(configName);
	m_isOPenDepthCaculate = config.getValueDft("PROJECT", "IS_DEAP_CALCULATE", "0").toInt();	
	 
	//读取深度计算相关配置文件
	this->readDepthDiseaseConfig(); 

}

int depthCaculate::checkDepthResult(hnRoadDiseaseInfo & diseaseInfo)
{
	hnPro::hnProject* curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (curProject == NULL)
	{
		return -1;
	}

	auto setting = curProject->getCurProSetInfo();
	auto strRoadStandard = HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard);

	QVector<DepthDisease> depthCalculateInfos;
	if (m_depthDiseases.contains(strRoadStandard))
	{
		depthCalculateInfos = m_depthDiseases[strRoadStandard];
	}
	else
	{
		return -2;
	}

	double depth = diseaseInfo.dDepth * 1000;
	QString diseaseTypeName = QString::fromLocal8Bit(diseaseInfo.strDisName);
	bool isFind = false;
	for (auto depthCalculateInfo : depthCalculateInfos)
	{
		const QString name = depthCalculateInfo.getName();
		const QString depthInfoFrameType = depthCalculateInfo.getFrameType();
		QString diseaseFrameType = diseaseInfo.nDrawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式");
		if (diseaseInfo.nDrawType ==2)
		{
			/////
			diseaseFrameType = QString::fromLocal8Bit("人工模式");
		}
		if (diseaseTypeName.contains(name) && diseaseFrameType == depthInfoFrameType)
		{
			const bool depthValueOk = depthCalculateInfo.m_thresholdDown < depth && depth <= depthCalculateInfo.m_thresholdUp;
			const bool diseaseAreaOk = depthCalculateInfo.m_thresholdAreaDown < depth && depth <= depthCalculateInfo.m_thresholdAreaUp;

			if (depthValueOk || diseaseAreaOk)
			{
				isFind = true;
				diseaseInfo.dDepth = depth / 1000.0;
				QString grad = depthCalculateInfo.getGrad();
				//等级不符合情况
				if (false == diseaseTypeName.contains(grad) && false == grad.isEmpty())
				{
					//修正病害名字
					diseaseTypeName = name + "." + grad;
					strcpy(diseaseInfo.strDisName, diseaseTypeName.toLocal8Bit().data());
					//修正等级
					diseaseInfo.setLevel(grad.toLocal8Bit().data());

					return -3;
				}
				//正常情况
				else
				{
					return 0;
				}
			}
			else
			{
				continue;
			}
		}
	}
	if (false == isFind)
	{
		return -4;
	}
	return 0;
}

bool depthCaculate::isDeformationDisease(const QString & diseaseTypeName)
{
	hnPro::hnProject* curProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (curProject == NULL)
	{
		return false;
	}

	auto setting = curProject->getCurProSetInfo();
	auto strRoadStandard = HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard);

	//沉陷类病害，深度计算的判断
	QVector<DepthDisease> depthDiss;
	if (m_depthDiseases.contains(strRoadStandard))
	{
		depthDiss = m_depthDiseases[strRoadStandard];
	}
	else
	{
		return false;
	}

	for (auto dis : depthDiss)
	{
		if (diseaseTypeName.contains(dis.getName()))
		{
			return true;
		}
	}
	return false;
}

QMap<HnProjectEnums::StandardParmTypeEnum,QVector<DepthDisease>> depthCaculate::getDepthDiseaseConfigDatas()
{
	return m_depthDiseases;
}

void depthCaculate::setOpenDepthCaculate(const bool isOpen)
{
	m_isOPenDepthCaculate = isOpen;
}

bool depthCaculate::getOpenDepthCaculate()
{
	return m_isOPenDepthCaculate;
}

bool depthCaculate::caculateBigFrameDiseaseDepth(hnRoadDiseaseInfo & disease)
{
	if (!disease.vec3dRect.empty())
	{
		//人工模式绘制病害 检查沉陷类病害计算
		hn3dRectI rect3d = disease.vec3dRect.at(0);

		if (false == this->checkDepth(disease))
		{
			return false;
		}

		return true;
	}
	else
	{
		return false;
	}

	return false;
}

bool depthCaculate::caculateLittleFrameDiseaseDepth(hnRoadDiseaseInfo &disease,
	rectAlgorithm &algorithm,
	QRect(rectAlgorithm::*mergeRectsFun)(QVector<QRect> littleRects),
	drawDiseases &widget,
	std::vector<hnCommon::hn3dRectI>(drawDiseases::*generateLargeFrameHn3dRectFun)(const QRect &unitedRect),
	QVector<QRect> tmpLittleFrameDiseaseRects)
{
	//自动化模式要特殊处理 检查沉陷类病害计算
	if (!disease.vec3dRect.empty())
	{
		//先把自动化模式的大image矩形合并成一个矩形
		QRect unitedRect = (algorithm.*mergeRectsFun)(tmpLittleFrameDiseaseRects);

		//然后再把合并后的矩形转换成3drect
		auto rect3d = (widget.*generateLargeFrameHn3dRectFun)(unitedRect);
		if (!rect3d.empty())
		{
			double depth = 0.0;
			QString diseaseTypeName = QString::fromLocal8Bit(disease.strDisName);

			if(false == this->checkDepth(disease))
			{
				return false;
			}
			disease.dDepth = depth;
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool depthCaculate::checkDepth(hnRoadDiseaseInfo & diseaseInfo)
{
	if (false == m_isOPenDepthCaculate)
	{
		return false;
	}
	if (true == diseaseInfo.vec3dRect.empty())
	{
		return false;
	}

	//判断是否为变形类病害
	bool isDeformation = this->isDeformationDisease(QString::fromLocal8Bit(diseaseInfo.strDisName));
	auto rect3d = diseaseInfo.vec3dRect.at(0);

	if (isDeformation)
	{
		//进行深度计算
		if (false == hnDataManager::getDataManager()->getDiseaseDepth(rect3d.p0, rect3d.p3, rect3d.p1, diseaseInfo.dDepth))
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("深度计算失败，请检查是否融合点云"));
			return false;
		}

		//判断深度计算的结果是否正确
		const double area = diseaseInfo.dArea;

		//告知用户深度计算的结果 显示的时候显示成毫米
		depthDialog dialog;
		dialog.setDepth(diseaseInfo.dDepth);

		auto reply = dialog.exec();
		if (QDialog::Accepted == reply)
		{
			diseaseInfo.dDepth = dialog.getDepth();

			const int rc = this->checkDepthResult(diseaseInfo);
			if (-1 == rc)
			{
				QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("工程未打开，取消该病害的绘制"));
				return false;
			}
			else if (-2 == rc)
			{
				QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该病害的深度值不符合规范，取消该病害的绘制"));
				return false;
			}
			else if (-3 == rc)
			{
				QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该病害的严重程度与深度结果不一致，已经进行修正"));
			}
			else if (-4 == rc)
			{
				QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("该病害的深度值不符合规范，取消该病害的绘制"));
				return false;
			}

			return true;
		}
		else
		{
			return false;
		}
	}
	return true;
}

void depthCaculate::readDepthDiseaseConfig()
{
   QString tempStr = 	QCoreApplication::applicationDirPath();
	QString configPath = QCoreApplication::applicationDirPath() + "\\config\\depthDisease.json";

	QFile jsonFile(configPath);
	if (!jsonFile.open(QIODevice::ReadOnly))
	{
		return;
	}
	QByteArray jsonData = jsonFile.readAll();

	// Parse JSON data into a QJsonDocument
	QJsonParseError err;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &err);

	QString errorMsg = err.errorString();
	// Get the root object of the JSON document
	QJsonObject rootObject = jsonDoc.object();

	for each (auto key in rootObject.keys())
	{
		auto strRoadStandard = HnProjectEnums::roadTypeQStringToEnum(key);
		//HnProjectEnums::StandardParmTypeEnum type = HnProjectEnums::roadTypeQStringToEnum(key); 
		auto subArray = rootObject.value(key).toArray();
		QVector<DepthDisease> diss;
		for (auto value : subArray)
		{
			auto object = value.toObject();
			QString name = object["name"].toString();
			QString grad = object["grad"].toString();
			DepthDisease dis;
			dis.m_thresholdUp = object["thresholdUp"].toInt();
			dis.m_thresholdDown = object["thresholdDown"].toInt();
			dis.m_thresholdAreaUp = object["thresholdAreaUp"].toInt();
			dis.m_thresholdAreaDown = object["thresholdAreaDown"].toInt();
			QString frameType = object["frameType"].toString();
			dis.setFrameType(frameType);
			dis.setName(name);
			dis.setGrad(grad); 
			diss.push_back(dis);
		}
		m_depthDiseases.insert(strRoadStandard, diss);
	}
}
