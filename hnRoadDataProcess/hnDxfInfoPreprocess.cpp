#include "hnDxfInfoPreprocess.h"
#include "..\hnCommon\hnRoadTypeDef.h" 
hnDxfInfoPreprocess::hnDxfInfoPreprocess(QObject *parent)
	: QObject(parent)
{
	m_currentExcelType = ExcelType::NoType;
	//加载模板表格的表头
	this->loadExcelTypes();
}

hnDxfInfoPreprocess::~hnDxfInfoPreprocess()
{
}

int hnDxfInfoPreprocess::preprocess(bool isNationProvincialRoad, const QStringList & excelFileNames, double beginMile,
	double endMile, const QString & exportPath ,double roadWidth, int direction)
{
	//检查excel文件名字的正确性
	if (!checkExcelFileName(excelFileNames))
	{
		return -1;
	}
	//检查文件是否可以打开
	if (!checkExcelIsOpen(excelFileNames))
	{
		return -2;
	}
	//检查文件sheet名字
	if (!checkExcelSheetName(excelFileNames))
	{
		return -3;
	}
	//检查文件的表格类型，（什么道路等级，人工模式还是自动化模式）如果有多种，就是异常
	if (!checkExcelType(excelFileNames))
	{
		return -4;
	}

	//创建GridDisease_C信息
	GridDisease_C gridDisease;
	QString gridName = convertRegionWithK(beginMile) + "-" + convertRegionWithK(endMile);
	strcpy(gridDisease.strName, gridName.toLocal8Bit());
	strcpy(gridDisease.strBegMile, QString::number(beginMile,'f',3).toLocal8Bit().data());
	strcpy(gridDisease.strEndMile, QString::number(endMile, 'f', 3).toLocal8Bit().data());
	gridDisease.dBegMileage = beginMile;
	gridDisease.dEndMileage = endMile;
	gridDisease.dRoadWidth = roadWidth;
	gridDisease.nRoadTotalNum = excelFileNames.size();
	
	//导出dxf
	this->exportDxf(gridDisease, m_currentExcelType, isNationProvincialRoad, excelFileNames, exportPath, direction);

	return 0;
}


bool hnDxfInfoPreprocess::checkExcelFileName(const QStringList & excelFileNames)
{
	//检查名字是不是都包含 “路面病害面积统计表”
	for (auto fileName : qAsConst(excelFileNames))
	{
		if (!fileName.contains(QString::fromLocal8Bit("路面病害面积统计表")))
		{
			
			return false;
		}
	}
	return true;
}

bool hnDxfInfoPreprocess::checkExcelIsOpen(const QStringList & excelFileNames)
{
	for (auto fileName : qAsConst(excelFileNames))
	{
		Document doc(fileName);
		if (!doc.load())
		{
			return false;
		}
	}
	return true;
}

bool hnDxfInfoPreprocess::checkExcelSheetName(const QStringList & excelFileNames)
{
	QString sheetName = QString::fromLocal8Bit("病害列表");
	for (auto fileName : qAsConst(excelFileNames))
	{
		Document doc(fileName);
		if (!doc.load())
		{
			return false;
		}
		if (!doc.selectSheet(sheetName))
		{
			return false;
		}
	}
	return true;
}

bool hnDxfInfoPreprocess::checkExcelType(const QStringList & excelFileNames)
{
	
	if (excelFileNames.size() <= 0)
	{
		return false;
	}

	Document doc(excelFileNames[0]);
	if (!doc.load())
	{
		return false;
	}
	if (!doc.selectSheet(QString::fromLocal8Bit("工程信息")))
	{
		return false;
	}

	QStringList projectInfos;
	QString valueInfo;
	 int row = 2;
	int column = 2;
	 
	while (true)
	{
		auto value = doc.read(row, column);

		if (value.isValid())
		{
			valueInfo = value.toString();
			projectInfos.append(valueInfo);
		}
		else
		{
			break;
		}
		row++;
	}

	QString standardStr = projectInfos[17];
	QString drawType = projectInfos[18];
	
	HnProjectEnums::StandardParmTypeEnum standard =  HnProjectEnums::roadTypeQStringToEnum(standardStr);
	auto draw0 = drawType.toLocal8Bit();
	auto draw1 = draw0.toStdString();
	auto draw2 = draw1.c_str();
	hnCommon::ROAD_WORK_TYPE draw = hnCommon::qstringToWorkType(draw2);
	ExcelType type;

	switch (standard)
	{
	case HnProjectEnums::None:
		switch (draw)
		{
		case hnCommon::ROAD_WORK_LARGE_RECT:
			type = ExcelType::DegreeRoad2018_BigFrame;
			break;
		case hnCommon::ROAD_WORK_SMALL_RECT:
			type = ExcelType::DegreeRoad2018_LittleFrame;

			break;
		case hnCommon::DESIGN:
			type = ExcelType::DegreeRoad2018_BigFrame;
			break;
		default:
			break;
		}
		break;
	case HnProjectEnums::DegreeRoad2018:
		switch (draw)
		{
		case hnCommon::ROAD_WORK_LARGE_RECT:
			type = ExcelType::DegreeRoad2018_BigFrame;
			break;
		case hnCommon::ROAD_WORK_SMALL_RECT:
			type = ExcelType::DegreeRoad2018_LittleFrame;

			break;
		case hnCommon::DESIGN:
			type = ExcelType::DegreeRoad2018_BigFrame;
			break;
		default:
			break;
		}
		break;
	case HnProjectEnums::CityRoad:
		switch (draw)
		{
		case hnCommon::ROAD_WORK_LARGE_RECT:
			type = ExcelType::City_BigFrame;
			break;
		case hnCommon::ROAD_WORK_SMALL_RECT:
			type = ExcelType::NoType;
			break;
		case hnCommon::DESIGN: 
			type = ExcelType::NoType;
			break;
		default:
			break;
		}
		break;
	case HnProjectEnums::RuralRoadlowLevel:
		switch (draw)
		{
		case hnCommon::ROAD_WORK_LARGE_RECT:
			type = ExcelType::LowVillage_BigFrame;
			break;
		case hnCommon::ROAD_WORK_SMALL_RECT:
			type = ExcelType::NoType;

			break;
		case hnCommon::DESIGN:
			type = ExcelType::NoType;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	if (type == ExcelType::NoType)
	{
		return false;
	} 
	m_currentExcelType = type;
	return true;
		
	 
}

void hnDxfInfoPreprocess::loadExcelTypes()
{
	//加载等级公路2018 人工模式的表头 
	m_excelTypes.insert(this->loadDegreeRoad2018_BigFrameHeader(), ExcelType::DegreeRoad2018_BigFrame);
	//m_excelTypes.insert(this->loadDegreeRoad2018_LittelFrameHeader(), ExcelType::DegreeRoad2018_LittleFrame);
	//m_excelTypes.insert(this->loadCity_BigFrameHeader(), ExcelType::City_BigFrame);
	//m_excelTypes.insert(this->loadCity_BigFrameHeader(), ExcelType::LowVillage_BigFrame);


	//获取道路类型和大框小框

}

QStringList hnDxfInfoPreprocess::loadDegreeRoad2018_BigFrameHeader()
{
	QString fileName = QApplication::applicationDirPath() + QString::fromLocal8Bit("/报表模板/等级公路 JTG H20-2018/人工模式/单项指标出表/路面病害面积统计表.xlsx");
	QStringList headers = this->loadExcelHeader(fileName);
	return headers;
}

 QStringList hnDxfInfoPreprocess::loadDegreeRoad2018_LittelFrameHeader()
{
	QString fileName = QApplication::applicationDirPath() + QString::fromLocal8Bit("/报表模板/等级公路 JTG H20-2018/自动化模式/单项指标出表/路面病害面积统计表.xlsx");
	QStringList headers = this->loadExcelHeader(fileName);
	return headers;
}

  QStringList hnDxfInfoPreprocess::loadCity_BigFrameHeader()
 {
	  QString fileName = QApplication::applicationDirPath() + QString::fromLocal8Bit("/报表模板/城镇道路/人工模式/单项指标出表/路面病害面积统计表.xlsx");
	  QStringList headers = this->loadExcelHeader(fileName);
	  return headers;
 }

 QStringList hnDxfInfoPreprocess::loadLowVillage_bigFrameHeader()
  {
	  QString fileName = QApplication::applicationDirPath() + QString::fromLocal8Bit("/报表模板/低等级农村公路/人工模式/单项指标出表/路面病害面积统计表.xlsx");
	  QStringList headers = this->loadExcelHeader(fileName);
	  return headers;
  }

void hnDxfInfoPreprocess::exportDxf(const GridDisease_C & gridDisease, ExcelType type,
	bool isNationProvincialRoad, const QStringList & excelFileNames, const QString & exportPath,int direction)
{
	switch (type)
	{
	case ExcelType::NoType:
		break;
	case ExcelType::DegreeRoad2018_BigFrame:
		exportDegreeRoad2018_BigFrameDxf(gridDisease, isNationProvincialRoad, excelFileNames, exportPath, direction);
		break;
	case ExcelType::DegreeRoad2018_LittleFrame:
		exportDegreeRoad2018_LittelFrameDxf(gridDisease, isNationProvincialRoad, excelFileNames, exportPath, direction);

		break;
	case ExcelType::City_BigFrame:
		break;
	case ExcelType::LowVillage_BigFrame:
		break;
	default:
		break;
	}
}

void hnDxfInfoPreprocess::exportDegreeRoad2018_BigFrameDxf(const GridDisease_C & gridDisease, bool isNationProvincialRoad, 
	const QStringList & excelFileNames, const QString & exportPath, int direction)
{
	std::vector<Disease_C> diseases;
	QString path;
	int roadType = -1;
	 

	if (setDiseases(exportPath, excelFileNames, direction, path, diseases, roadType))
	{

		if (!isNationProvincialRoad)
		{
			OutputXRDxf(path.toLocal8Bit().data(), diseases, gridDisease, direction, roadType);
		}
		else
		{
			OutputXRDxfProvinceRoad(path.toLocal8Bit().data(), diseases, gridDisease, direction,roadType);
		}

	}
	else
	{

	}


}

void hnDxfInfoPreprocess::exportDegreeRoad2018_LittelFrameDxf(const GridDisease_C &gridDisease, bool isNationProvincialRoad, const QStringList &excelFileNames, const QString &exportPath, int direction)
{
	std::vector<Disease_C> diseases;
	QString path; 
	int roadType = -1;
	 
	if (setDiseases(exportPath, excelFileNames, direction, path, diseases, roadType))
	{
		if (!isNationProvincialRoad)
		{
			OutputXRDxf(path.toLocal8Bit().data(), diseases, gridDisease, direction, roadType);
		}
		else
		{
			OutputXRDxfProvinceRoad(path.toLocal8Bit().data(), diseases, gridDisease, direction,roadType);
		}
	}  
}

int hnDxfInfoPreprocess::getDiseaseType(QString type)
{
	if (type == QStringLiteral("沥青"))
	{
		return 1; 
	}
	else if (type == QStringLiteral("水泥"))
	{
		return 2;
	}
	else if (type == QStringLiteral("砂石"))
	{
		return 3;
	}
}

bool hnDxfInfoPreprocess::setDiseases(const QString &exportPath,QStringList excelFileNames, int direction, QString& path, std::vector<Disease_C>& diseases, int & roadType)
{ 
	int roadNum = 1;
	//遍历excel路径，把excel里面的病害添加到diseases里。
	int tempRoadType = -1; 
	bool hasOtherRoadType =false;
	for (auto excelFileName : qAsConst(excelFileNames))
	{
		Document doc(excelFileName);
		if (!doc.isLoadPackage())
		{
			continue;
		}
		QString sheetName = QString::fromLocal8Bit("病害列表");
		if (!doc.selectSheet(sheetName))
		{
			continue;
		}
		int row = 3;

		while (true)
		{
			Disease_C disease;
			auto mile = doc.read(row, 1);
			//碰到空行 就退出
			if (!mile.isValid())
			{
				break;
			}
			disease.beginTrueMile = doc.read(row, 1).toDouble();										//里程	
																										//strcpy(disease.roadNum, QString::number(roadNum).toLocal8Bit().data());		//车道
			disease.roadNum = roadNum;														//车道
			strcpy(disease.diseaseType, doc.read(row, 3).toString().toLocal8Bit().data());	//病害类型
			strcpy(disease.diseaseDegree, doc.read(row, 4).toString().toLocal8Bit().data());	//病害程度
			disease.rectHeight = doc.read(row, 5).toDouble();								//病害框长度
			disease.rectWidth = doc.read(row, 6).toDouble();								//病害框宽度
			disease.distToCenter = doc.read(row, 7).toDouble();								//病害中心位置（距路面图像左边距离）（m）
			disease.diseaseArea = doc.read(row, 8).toDouble();								//病害面积
			disease.calcHeight = doc.read(row, 9).toDouble();								//病害计算长度
			disease.calcWidth = doc.read(row, 10).toDouble();								//病害计算宽度
			QString type = doc.read(row, 16).toString();

			roadType = getDiseaseType(type); 
			if (tempRoadType!=-1)
			{
				if (tempRoadType!= roadType)
				{
					//出现了第二种路面类型
					hasOtherRoadType = true;
					break;
				}
				else
				{
					tempRoadType = roadType;

				}
			}
			else
			{
				tempRoadType = roadType;

			}
			disease.nRoadType = getDiseaseType(type);
			row++;
			diseases.push_back(disease);
		}

		//车道序号增加
		roadNum++;
	}

	
	if (direction > 0)
	{
		path = exportPath + QString::fromLocal8Bit("/上行");
	}
	else
	{
		path = exportPath + QString::fromLocal8Bit("/下行");
	}
	QDir dir(path);
	if (!dir.exists())
	{
		dir.mkpath(path);
	}
	if (hasOtherRoadType)
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("错误"), QString::fromLocal8Bit("请确保病害列表中仅存在一种路面类型的病害，否则无法处理!"));
		return false;
	}
	return true;
}

QStringList hnDxfInfoPreprocess::loadExcelHeader(const QString & fileName)
{
	Document doc(fileName);
	if (!doc.load())
	{
		return QStringList();
	}
	if (!doc.selectSheet(QString::fromLocal8Bit("病害列表")))
	{
		return QStringList();
	}
	QStringList headers;
	QString header;
	const int row = 2;
	int column = 1;

	
	
	while (true)
	{
		auto value = doc.read(row, column);
		
		if (value.isValid())
		{
			header = value.toString();
			headers.append(header);
		}
		else
		{
			break;
		}
		column++;
	}

	return headers;
}

QString hnDxfInfoPreprocess::convertRegionWithK(double region)
{
	//3003.456  转换后 成了 K3+003.456
	double decimalPart = fmod(region, 1000);
	int integerPart = region / 1000;

	QString regionWithK = "K" + QString::number(integerPart) + "+" +
		QString::number(decimalPart, 'f', 3).rightJustified(7, '0');

	return regionWithK;
}


