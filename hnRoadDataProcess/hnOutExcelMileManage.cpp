#include "hnOutExcelMileManage.h"
#include "hnReportDiseaseSeverity.h"
#include "hnCityReportSegmenter.h"
#include <QtMath>
#include "hnRuralEvaluationUnitMerger.h"
#include "hnReportProjectInfo.h"
#include <algorithm>
#include "..\hnQtCommon\MyCommonMethods.h"
#include <stdexcept>
#include <iostream>
#include <memory>
#include<QTextCodec>
#include "..\hnProject\hn2DProject.h"
#include <QString>
#include <QMessageBox>
#include <QDir>
#include <QMap>
#include <QFileInfo>
#include "..\hnApplication\hnDataManager.h"
namespace
{
	QString irmDataError(
		const QString& projectName,
		const QString& metric,
		const QString& side,
		const QString& inputPath,
		int observedCount,
		int expectedCount,
		bool isPartial)
	{
		const QString header =
			QStringLiteral("工程：") + projectName
			+ QStringLiteral("\r\n指标：") + metric
			+ QStringLiteral("\r\n侧别：") + side
			+ QStringLiteral("\r\n输入文件：")
			+ QDir::toNativeSeparators(
				QFileInfo(inputPath).absoluteFilePath());

		if (isPartial)
		{
			return header
				+ QStringLiteral("\r\n记录数：已读取 ")
				+ QString::number(observedCount)
				+ QStringLiteral("，预计 ")
				+ QString::number(expectedCount)
				+ QStringLiteral(
					"。\r\n"
					"数据不完整。"
					"请执行：数据处理 -> 清空IRM结果 -> ")
				+ QStringLiteral("【")
				+ metric
				+ QStringLiteral("】 -> IRM计算。");
		}

		return header
			+ QStringLiteral(
				"\r\n"
				"文件缺失。"
				"请核对/恢复上述文件；"
				"若原始采集数据完整，请执行："
				"数据处理 -> 清空IRM结果 -> ")
			+ QStringLiteral("【")
			+ metric
			+ QStringLiteral("】 -> IRM计算。");
	}
}

#include "MileAndDmi.h"
#include "..\hnApplication\hnDiseaseService.h"
#ifdef DEBUG
#include "..\hnProject\hn2DProject.h"
#include "..\hnQtCommon\ExcelGPS.h"


using namespace   hnPro;
#endif // DEBUG



hnOutExcelMileManage::hnOutExcelMileManage(HnProjectEnums::StandardParmTypeEnum standard, hnPro::hnProject* project,
	double sMile, double eMile, double xlslen, const MyQtCommon::MyEquipment& equip, bool ignoreMarksForExport)
	:m_standard(standard),
	m_sMile(sMile),
	m_eMile(eMile),
	m_xlslen(xlslen),
	m_project(project),
	m_equipMentList(equip),
	m_iriOk(true), m_pbState(true), m_rutState(true), m_speedOk(true), m_mtdState(true), m_mpdState(true), m_gpsState(true), m_jhxxState(true), m_dataCompletion(false)
{
	m_xrSetting = HnXRSettings::getInstance();
	m_cityDistanceSegments = m_standard == HnProjectEnums::CityRoad && m_xrSetting->PartType == 1;
	m_useDmiFormat = m_xrSetting->outExcelFormatDmi && !m_cityDistanceSegments;
	if (m_cityDistanceSegments)
	{
		m_xlslen = m_xrSetting->PartType_Dmi_Len > 0 ? m_xrSetting->PartType_Dmi_Len : 200;
	}
	//m_levels = hnApp::hnDataManager::getDataManager()->getRoadLevel(m_standard);
	//获得配置信息
	m_projectSet = project->getCurProSetInfo();
	m_direction = m_projectSet.nLineType;
	//获得出表各个规范需要的参数 
	//获取到当前项目的所有桩号 
	QVector<hnMile> mils = project->getCurrentMileVector();
	if (mils.size() <= 0)
	{
		return;
	}
	//根据分段进行筛选  
	if (m_direction == -1)
	{
		if (m_sMile < m_eMile)
		{
			double temp = 0;
			temp = m_sMile;
			m_sMile = m_eMile;
			m_eMile = temp;
		}
	}
	QVector<hnMile> m_filteredmileVec = getMilesInRange(mils, m_direction, m_sMile, m_eMile);
	if (m_cityDistanceSegments && m_filteredmileVec.isEmpty())
	{
		// 校桩记录可能稀疏；辅助十米区间仍从本次导出起点生成。
		hnMile first = mils.first();
		first.dTrueMile = m_sMile;
		m_filteredmileVec.push_back(first);
	}
	//剩下步骤  设置各项指标参数
	//根据用户分段区间进行分段如 10,100,1000

	auto marks = m_cityDistanceSegments ? cityReportMarks() : m_project->getCurrentMarkVector();
	// 国检非 DR 成果必须忽略所有打标，包括农村公路材质边界。
	if (ignoreMarksForExport) marks.clear();
	if (m_cityDistanceSegments)
	{
		m_roadSplietVec = splitCityMile(marks);
	}
	else if (m_filteredmileVec.isEmpty() || m_xlslen <= 0)
	{
		return;
	}
	else if (m_useDmiFormat)
	{
		m_roadSplietVec = splitMile_dmi(m_filteredmileVec, m_xlslen);

	}
	else
	{
		m_roadSplietVec = splitMile(m_filteredmileVec, m_xlslen);

	}

	if (m_roadSplietVec.isEmpty())
	{
		return;
	}
	//处理打标文件 对前一步分段进行处理

	if (m_useDmiFormat && m_standard != HnProjectEnums::RuralRoadlowLevel)
	{
		for (auto& mark : marks)
		{

			double realMile = getCloseMile(mark.dEnclMile);

			mark.dTrueMile = realMile;
		}
	}
	// 农村路属性边界必须参与评定，不受打标备注输出选项控制。
	if (!this->m_xrSetting->outMileWithMark
		&& m_standard != HnProjectEnums::RuralRoadlowLevel && !m_cityDistanceSegments)
	{
		marks.clear();
	}
	handelMark(m_roadSplietVec, marks, qAbs(m_xlslen - 1000.0) < 0.001);

	//添加车辙病害
	if (m_equipMentList.ROAD
		&& (m_project->getBaseStandard() == HnProjectEnums::DegreeRoad2018) || (m_project->getBaseStandard() == HnProjectEnums::CityRoad))
	{
		if (m_useDmiFormat)
		{

			m_roadSplit_rutDis10m_Vec = splitMile_dmi(m_filteredmileVec, 10);
		}
		else
		{
			m_roadSplit_rutDis10m_Vec = splitMile(m_filteredmileVec, 10);

		}

		if (m_roadSplietVec.isEmpty())
	{
		return;
	}
	//处理打标文件 对前一步分段进行处理
		auto markRuts = m_cityDistanceSegments ? cityReportMarks() : m_project->getCurrentMarkVector();
		if (ignoreMarksForExport) markRuts.clear();
		if (m_useDmiFormat)
		{
			for (auto& mark : markRuts)
			{

				double realMile = getCloseMile(mark.dEnclMile);

				mark.dTrueMile = realMile;
			}
		}
		if (!this->m_xrSetting->outMileWithMark && !m_cityDistanceSegments)
		{
			markRuts.clear();
		}
		handelMark(m_roadSplit_rutDis10m_Vec, markRuts);

		if (m_xrSetting->czDisOutSelectExcel == 1 || (m_xrSetting->czDisOutSelectExcel == 2 && (m_project->getCurProSetInfo().nGradIndex > 1)))
		{
			QVector <double > sval;
			QVector<double >smile;
			getRutDisVal(m_roadSplit_rutDis10m_Vec, sval, smile);
			setRutDis(sval, smile);
		}

	}

	m_dataCompletion = handelRoadSplietVec(m_roadSplietVec);
	//跳车需要十米分段
	if (m_project->get2DProject()->_IsIRIMTD && m_equipMentList.JUMP)
	{

		if (m_useDmiFormat)
		{

			m_roadSplit_pwi10m_Vec = splitMile_dmi(m_filteredmileVec, 10);
		}
		else
		{
			m_roadSplit_pwi10m_Vec = splitMile(m_filteredmileVec, 10);

		}

		handelMark(m_roadSplit_pwi10m_Vec, marks);

		m_pbState = writePbiValue(m_roadSplit_pwi10m_Vec);
		m_dataCompletion = m_dataCompletion && m_pbState;

		handelPbiValues(m_roadSplietVec, m_roadSplit_pwi10m_Vec);
		writeSpeedValue(m_roadSplit_pwi10m_Vec, 10);
	}
}

hnOutExcelMileManage::~hnOutExcelMileManage()
{

}
void hnOutExcelMileManage::reportExcelError(const QString& message)
{
	const QString normalizedMessage = message.trimmed();
	if (!normalizedMessage.isEmpty() && !m_xrSetting->ExcelErrorMessageList.contains(normalizedMessage))
	{
		m_xrSetting->ExcelErrorMessageList.append(normalizedMessage);
	}
}



QVector<hnOutExcelMile> hnOutExcelMileManage::getRoadMessageVec()
{
	if (m_xrSetting->outExcelNeedSort)
	{
		if (m_project->getCurProSetInfo().nLineType == -1)
		{
			QVector<hnOutExcelMile> result;
			result.reserve(m_roadSplietVec.size());
			for (auto it = m_roadSplietVec.rbegin(); it != m_roadSplietVec.rend(); ++it)
			{
				result.append(*it);
			}
			return result;
		}
	}
	return m_roadSplietVec;
}

QVector<hnOutExcelMile> hnOutExcelMileManage::getRoadMessage_10m_Vec()
{
	if (m_xrSetting->outExcelNeedSort)
	{
		if (m_project->getCurProSetInfo().nLineType == -1)
		{
			QVector<hnOutExcelMile> result;
			result.reserve(m_roadSplit_pwi10m_Vec.size());
			for (auto it = m_roadSplit_pwi10m_Vec.rbegin(); it != m_roadSplit_pwi10m_Vec.rend(); ++it)
			{
				result.append(*it);
			}
			return result;
		}
	}

	return m_roadSplit_pwi10m_Vec;
}

QVector<hnMile> hnOutExcelMileManage::getMilesInRange(const QVector<hnMile>& miles, int line, double sMile, double eMile)
{
	QVector<hnMile> filteredMiles;
	for (auto it = miles.begin(); it != miles.end(); ++it)
	{
		if ((line == 1) && (it->dTrueMile >= sMile) && (it->dTrueMile <= eMile)
			|| (line == -1) && (it->dTrueMile <= sMile) && (it->dTrueMile >= eMile))
		{
			filteredMiles.push_back(*it);
		}
	}
	if (filteredMiles.size() > 1)
	{
		if (line == -1)
		{

			filteredMiles[0].dTrueMile = sMile;
			filteredMiles[filteredMiles.size() - 1].dTrueMile = eMile;
		}
		else
		{
			filteredMiles[0].dTrueMile = sMile;
			filteredMiles[filteredMiles.size() - 1].dTrueMile = eMile;
		}
	}

	return filteredMiles;
}

bool hnOutExcelMileManage::handelRoadSplietVec(QVector<hnOutExcelMile>& miles)
{
	bool hasLeftIRI = false;
	bool hasRightIRI = false;

	if (m_project->get2DProject()->_IsIRIMTD && m_equipMentList.IRI)
	{
		m_iriOk = writeIriValue(hasLeftIRI, hasRightIRI, m_xlslen);
	}
	if (m_equipMentList.SPEED)
	{
		m_speedOk = writeSpeedValue(m_roadSplietVec, m_xlslen);
	}
	if (m_project->get2DProject()->_IsRut && m_equipMentList.RUT)
	{
		m_rutState = writeRutValue();
	}
	if (m_equipMentList.MPD)
	{
		m_mpdState = writeMpdValue();
	}
	if (m_equipMentList.SMTD)
	{
		m_mtdState = writeMtdValue();
	}
	if (m_equipMentList.JHXX)
	{
		m_jhxxState = writeJHXXValue(m_xlslen);
	}

	if (m_equipMentList.GPS)
	{
		m_gpsState = writeGpsStrValue();
	}

	// 各指标只依赖自己的数据。车速是辅助输出列，缺失时不得阻断其他指标评分。
	bool ok = m_iriOk && m_rutState && m_mpdState && m_mtdState && m_gpsState && m_jhxxState;

	//将每段代表的hnMile赋值进去,计算指标得分
	QVector<hnCommon::hnRoadDiseaseInfo> diss;

	//景观
	QVector<hnCommon::hnRoadDiseaseInfo> streetDiss;
	QMap<int, QVector<hnDiseaseSetInfo>> streetSettingInfoMap;
	QString standard = HnProjectEnums::roadTypeEnumToQString(m_standard);
	if (m_equipMentList.ROAD)
	{

		hnApp::hnDataManager::getDataManager()->getDiseaseService()->setProject(m_project);
		//if (m_project->getCurProSetInfo().nDrawType==2)
		//{
		//  m_project->getDB()->getDiseaseTable()->readDesignDiseases(standard, m_project->trueMileToEncl(m_sMile), m_project->trueMileToEncl(m_eMile),diss); 
		//}
		//else
		//{
		//	//过滤路面材质不一样,绘制类型不一样,  道路宽度等不一样的病害
		//	m_project->getDB()->getDiseaseTable()->readRoadDiseaseData(m_projectSet, m_project->getCurrentMileVector(), diss, m_project->getCurrentMarkVector(), m_project->getRoadSpace());
		//}

		diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();


		if (rutDiss.size() > 0)
		{
			diss.append(rutDiss);
		}


	}
	hnReportDiseaseSeverity severity;
	severity.apply(diss, m_project);
	if (m_equipMentList.STREET)
	{

		hnApp::hnDataManager::getDataManager()->getDiseaseService()->setProject(m_project);
		//	m_project->getDB()->getDiseaseTable()->readStreetData(standard, m_project->getCurrentMileVector(), streetDiss, m_project->getCurProSetInfo().nLineType, m_project->getStreetSpace());
		streetDiss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllStreetDiseases();
		QVector<hnDiseaseSetInfo> tciDiseaseSetInfos = hnApp::hnDataManager::getDataManager()->
			getCurrentProjectStreetDiseases(m_project->getBaseStandard(), 1);

		//m_project->getDB()->getDiseaseTable()->readStreetData(standard,m_project->getCurrentMileVector(), streetDiss, m_project->getCurProSetInfo().nLineType, m_project->getStreetSpace());
		streetDiss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllStreetDiseases();
		QVector<hnDiseaseSetInfo> sciDiseaseSetInfos = hnApp::hnDataManager::getDataManager()->
			getCurrentProjectStreetDiseases(m_project->getBaseStandard(), 2);
		streetSettingInfoMap.insert(1, tciDiseaseSetInfos);
		streetSettingInfoMap.insert(2, sciDiseaseSetInfos);
	}


	for (hnOutExcelMile& excelMile : miles)
	{
		const bool needsRoadParameters =
			(m_iriOk && m_project->get2DProject()->_IsIRIMTD && m_equipMentList.IRI) ||
			(m_rutState && m_project->get2DProject()->_IsRut && m_equipMentList.RUT) ||
			(m_mpdState && m_equipMentList.MPD) ||
			(m_mtdState && m_equipMentList.SMTD) ||
			m_equipMentList.ROAD;
		const bool parameterReady = !needsRoadParameters || excelMile.StartCalculate(false);
		if (!parameterReady)
		{
			ok = false;
		}
		if (parameterReady && m_iriOk &&
			m_project->get2DProject()->_IsIRIMTD && m_equipMentList.IRI)
		{
			if (!excelMile.calculateRQIScore(hasLeftIRI, hasRightIRI))
			{
				ok = false;
			}
		}
		if (parameterReady && m_rutState &&
			m_project->get2DProject()->_IsRut && m_equipMentList.RUT)
		{
			if (!excelMile.calculateRUTScore())
			{
				ok = false;
			}
		}
		if (parameterReady && m_equipMentList.ROAD)  //涉及到 pci  病害的 都需要
		{
			//if (m_xrSetting->czDisOutSelectExcel == 1 || (m_xrSetting->czDisOutSelectExcel == 2 && (m_project->getCurProSetInfo().nGradIndex > 1)))
			//{
			//	//计算车辙病害
			//	QVector<hnCommon::hnRoadDiseaseInfo> rutDis;
			//	 if (m_project->getBaseStandard()== HnProjectEnums::CityRoad)
			//	 {
			//		 excelMile.GetRutDis(rutDis);
			//	 }
			//	 else
			//	 {
			//	 }
			//} 
			if (m_project->getBaseStandard() == HnProjectEnums::CityRoad && (m_project->getProjectType() == PROJECT_23D_TYPE || m_project->getProjectType() == PROJECT_2D_TYPE))
			{
				//城镇道路自动计算路况差病害
				//1.先对病害进行排序
				getLuKuangCha(diss);
			}


			if (!excelMile.calculateDrScore(hnReportProjectInfo::roadWidth(m_project), diss))
			{
				ok = false;
			}
		}
		if (m_equipMentList.STREET) //涉及到景观的
		{
			//计算Tci
			excelMile.calcaulateStreetScore(1, streetDiss, streetSettingInfoMap[1]);
			//计算Sci
			excelMile.calcaulateStreetScore(2, streetDiss, streetSettingInfoMap[2]);
		}
	}
	return ok;
#ifdef DEBUG

	QStringList mileList;
	for (auto mile : miles)
	{
		//qDebug() <<
		QString mileInfo = mile;
		mileList.push_back(mileInfo);
	}
	//QString MilePath =project->get2;
	QString path = m_project->get2DProject()->getBasePath() + "\\" + QString::number(m_xlslen) + QStringLiteral(" 分段测试文件.txt");
	MyCommonMethods::writeAllLines(path, mileList, QTextCodec::codecForName("utf-8"));
#endif // CWB_测试

}

void hnOutExcelMileManage::getLuKuangCha(QVector<hnCommon::hnRoadDiseaseInfo>& arrdis)
{
	if (m_project->get2DProject()->_IsRut)
	{
		if (!LoadRutData())
		{
			return;
		}
	}
	else
	{
		return;
	}
	double thresh = 15;
	double depth = -1;
	if (arrdis.size() > 0)
	{
		QVector<hnCommon::hnRoadDiseaseInfo> newDiss;

		hnCommon::hnRoadDiseaseInfo curdis;

		for (int i = 0; i < arrdis.size(); ++i)
		{
			curdis = arrdis[i];
			QString disName = QString::fromLocal8Bit(curdis.strDisName);
			if (disName.contains(QStringLiteral("路框差")))
			{
				double depth = 0;
				if (m_project->getProjectType() == PROJECT_2D_TYPE)
				{
					//depth = IsLuKuangCha(projectpath, prjinfo, arrdis[i]);
					//if (depth < thresh)
					//{
					//	//	arrdis.removeAt(i);
					//	//	--i;
					//}
					//else
					//{
					//	newDiss.push_back(curdis);
					//}
					newDiss.push_back(curdis);
				}
				else if (m_project->getProjectType() == PROJECT_23D_TYPE)
				{
					depth = curdis.dDepth;
					if (depth < thresh)
					{
						//	arrdis.removeAt(i);
						//	--i;
					}
					else
					{
						newDiss.push_back(curdis);
					}
				}


			}
			else
			{
				newDiss.push_back(curdis);
			}

		}
		arrdis = newDiss;
	}
}

bool hnOutExcelMileManage::LoadRutData()
{
	hnPro::hn2DProject* project = m_project->get2DProject();
	QString rutLeftDataDirStr = project->getRutResultPath() + "\\camera0\\data";
	QString rutRightDataDirStr = project->getRutResultPath() + "\\camera1\\data";
	QDir rutLeftDir(rutLeftDataDirStr);
	if (rutLeftDir.exists())
	{
		QString rutLeftIniFileStr = project->getLeftRutPath() + "\\rutcfg.ini";
		QFile rutIniFile(rutLeftIniFileStr);
		if (!QFile::exists(rutLeftIniFileStr))
		{
			THROW_FILE(QStringLiteral("丢失车辙配置文件:") + rutLeftIniFileStr);
			return false;
		}
		RutParm rutLeftParm(rutLeftIniFileStr);
		rutparm_L = rutLeftParm;
		if (project->_RutMode == 1)
		{
			QString rutRightIniFileStr = project->getRightRutPath() + "\\rutcfg.ini";
			QFile rutIniFile(rutRightIniFileStr);
			if (!QFile::exists(rutRightIniFileStr))
			{
				THROW_FILE(QStringLiteral("丢失车辙配置文件:") + rutRightIniFileStr);
				return false;
			}
			RutParm rutRightParm(rutRightIniFileStr);
			rutparm_R = rutRightParm;
		}
		else
		{
			rutparm_R = rutLeftParm;
		}

		rutfilepaths_L = MyCommonMethods::getMyAllDirFile(rutLeftDataDirStr, QStringList("*.dtw"));
		if (project->_RutMode == 1)
		{
			rutfilePaths_R = MyCommonMethods::getMyAllDirFile(rutRightDataDirStr, QStringList("*.dtw"));
		}
		rbarr.resize(rutparm_L._hpixel * rutparm_L._pixsize);
		profile.resize(rutparm_L._hpixel);
		profileZ.resize(rutparm_L._hpixel);
		profileZtmp.resize(rutparm_L._hpixel);
	}
	else
	{
		return false;
	}
	return true;
}

bool hnOutExcelMileManage::getRutDisVal(QVector<hnOutExcelMile>& miles, QVector<double>& sRutVals, QVector<double>& sMiles)
{
	QString resultPath = m_project->get2DProject()->getBasePath();
	QString leftpath = resultPath + "\\Rut\\camera0\\orirut.txt";
	QString rightPath = resultPath + "\\Rut\\camera1\\orirut.txt";

	QFile file(leftpath);
	QStringList LStrs;

	bool ok = false;
	QStringList RStrs;
	if (file.exists())
	{

		LStrs = MyCommonMethods::ReadAllLines(leftpath);
		double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
		if (LStrs.size() / 10 < endDmi - endDmi / 5)
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("左侧"), leftpath, LStrs.size(), qRound(endDmi * 10.0), true);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

		}

		ok = true;
	}
	else
	{
		try
		{
			QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("左侧"), leftpath, 0, 0, false);
			string mes = message.toLocal8Bit();
			reportExcelError(message);
			//throw std::runtime_error(mes.c_str());
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	QFile file1(rightPath);
	if (m_project->get2DProject()->_RutMode == 1)
	{
		if (file1.exists())
		{

			RStrs = MyCommonMethods::ReadAllLines(rightPath);
			double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
			if (RStrs.size() / 10 < endDmi - endDmi / 5)
			{
				try
				{
					QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("右侧"), rightPath, RStrs.size(), qRound(endDmi * 10.0), true);
					string mes = message.toLocal8Bit();
					reportExcelError(message);
					//	throw std::runtime_error(mes.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << e.what() << std::endl;
				}

			}

			ok = true;
		}
		else
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("右侧"), rightPath, 0, 0, false);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}

	if (ok == false)
	{
		return false;
	}
	//const double BaseLen = 10;
	int len = miles.size();

	QVector<double>lval(len);
	QVector<double>rval(len);
	QVector<double>sval(len);
	QString LStrLine, RStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0;
	double BaseLen = 0.1;
	for (int i = 0; i < len; i++)
	{
		double dimS = miles[i].getStartDmi();
		auto dimE = miles[i].getEndDmi();
		if (miles[i].RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE)
		{
			double suml = 0, sumr = 0, sums = 0, ltval = 0, rtval = 0;
			QStringList tmtd;
			int lvalnum = 0, rvalnum = 0, svalnum = 0;

			QStringList tval;
			double temp = m_project->get2DProject()->_DMIScale;
			startidx = MyCommonMethods::MathRoundToInt(dimS * m_project->get2DProject()->_DMIScale / BaseLen);
			endidx = MyCommonMethods::MathRoundToInt(dimE * temp / BaseLen);
			for (ValStridx = startidx; ValStridx < endidx; ++ValStridx)
			{
				if (m_project->get2DProject()->_RutMode == 1)
				{
					if (ValStridx < LStrs.size())
					{
						LStrLine = LStrs[ValStridx];
						tval = LStrLine.split(',');
						if (tval.size() < 2)
						{
							continue;
						}
						ltval = qAbs(tval[1].toDouble()) + m_xrSetting->rutLeftCorrect;
						suml += ltval;
						++lvalnum;
					}
					if (ValStridx < RStrs.size())
					{
						RStrLine = RStrs[ValStridx];
						tval = RStrLine.split(',');
						if (tval.size() < 2)
						{
							continue;
						}
						rtval = qAbs(tval[1].toDouble()) + m_xrSetting->rutRightCorrect;
						sumr += rtval;
						++rvalnum;
					}
					sums += qMax(ltval, rtval);
					++svalnum;
				}
				else
				{
					if (ValStridx < LStrs.size())
					{
						LStrLine = LStrs[ValStridx];
						tval = LStrLine.split(',');
						if (tval.size() <= 3)
						{
							continue;
						}
						ltval = qAbs(tval[1].toDouble()) + m_xrSetting->rutLeftCorrect;
						rtval = qAbs(tval[3].toDouble()) + m_xrSetting->rutRightCorrect;
						suml += ltval;
						sumr += rtval;
						sums += qMax(ltval, rtval);
						++lvalnum;
						++rvalnum;
						++svalnum;
					}
				}

			}

			if (svalnum > 0)
			{
				double value = sums / svalnum;
				sRutVals.push_back(value);
				sMiles.push_back(miles[i].getStartMile());

			}
			else
			{
				miles[i].setRutDisVlaue(0);
				sRutVals.push_back(0);
				sMiles.push_back(miles[i].getStartMile());

			}

		}
		else
		{
			sRutVals.push_back(0);
			sMiles.push_back(miles[i].getStartMile());

		}

	}
	return true;
}

void hnOutExcelMileManage::setRutDis(const QVector<double>sRutVlas, const QVector<double>sMiles)
{
	rutDiss.clear();
	if (sRutVlas.size() == 0)
	{
		return;
	}
	QString grad = QString::fromLocal8Bit(m_project->getCurProSetInfo().strRoadLevel);
	hnRoadTypeSetInfo setInfo;
	hnApp::hnDataManager::getDataManager()->getRoadTypeSetInfo(m_standard, grad, ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE, setInfo);
	QVector<double> rutThresh;
	if (setInfo.dRutThreslodDown != 0)
	{
		rutThresh.resize(2);
		rutThresh[0] = setInfo.dRutThreslodDown;
		rutThresh[1] = setInfo.dRutThreslodUp;
	}
	else
	{
		rutThresh.resize(1);
		rutThresh[0] = setInfo.dRutThreslodUp;

	}
	int len = sRutVlas.size();
	//自动计算的车辙病害

	QVector<int> disdegree;
	disdegree.resize(len);
	bool bflag = false;
	for (int i = 0; i < len; ++i)
	{
		bflag = false;
		for (int j = rutThresh.size() - 1; j >= 0; --j)
		{
			if (sRutVlas[i] > rutThresh[j])
			{
				disdegree[i] = j;
				bflag = true;
				break;
			}

		}
		if (bflag)
		{
			continue;
		}
		disdegree[i] = -1;
	}
	QVector<QString> degreestr = { QStringLiteral("轻"), QStringLiteral("重") };
	int oldtype = disdegree[0];
	int dislen = 0;

	for (int i = 1; i < len; ++i)
	{
		if (oldtype == -1)
		{
			oldtype = disdegree[i];
			dislen = 0;
			continue;
		}
		if (oldtype != disdegree[i])
		{
			dislen++;
			hnCommon::hnRoadDiseaseInfo tempdis;
			tempdis.dMileage = m_project->trueMileToEncl(sMiles[i]);
			tempdis.dDmi = m_project->trueMileToEncl(sMiles[i]);
			tempdis.dDmiStart = m_project->trueMileToEncl(sMiles[i]);
			tempdis.dDmiEnd = m_project->trueMileToEncl(sMiles[i]) + dislen;
			tempdis.dReaWidth = m_xrSetting->RutDisWidth;
			tempdis.dRealLen = dislen;
			tempdis.dWidth = tempdis.dReaWidth;
			tempdis.dLength = tempdis.dRealLen;
			tempdis.dArea = tempdis.dRealLen * tempdis.dReaWidth;
			tempdis.nDrawType = m_project->getCurProSetInfo().nDrawType;
			tempdis.nRSurfaceType = 0;
			tempdis.dRoadWidth = hnReportProjectInfo::roadWidth(m_project);
			tempdis.diseaseWeight = 1;
			strcpy(tempdis.strDiseaseTableName, "DisCZ");
			if (m_project->getBaseStandard() == HnProjectEnums::CityRoad)
			{
				tempdis.nLevel = 0;
				strcpy(tempdis.strDisName, "车辙");
			}
			else
			{
				if (degreestr[oldtype] == QStringLiteral("轻"))
				{
					tempdis.nLevel = 1;
					strcpy(tempdis.strDisName, "车辙.轻");
				}
				else
				{
					tempdis.nLevel = 2;
					strcpy(tempdis.strDisName, "车辙.重");

				}
				rutDiss.push_back(tempdis);
				dislen = 0;
			}


		}
	}
}

void hnOutExcelMileManage::handelPbiValues(QVector<hnOutExcelMile>& roadpart, const QVector<hnOutExcelMile>& mile_10m)
{
	int maxormean = 0;
	auto sett = m_project->get2DProject();

	int startidx = 0, endidx = 0, ValStridx = 0;
	int len = roadpart.size();
	auto projectInfo = m_project->getCurProSetInfo();
	int direction = projectInfo.nLineType;
	for (int i = 0; i < len; i++)
	{

		auto& nowExcelMile = roadpart[i];

		auto params = nowExcelMile.getRoadTypeSetInfo();
		QString PBI_KFBZ = QString::fromLocal8Bit(params.strPBI_KFBZ);
		QStringList kfbzStr = PBI_KFBZ.split(" ");

		QVector<double>  thresh;
		for each(auto var in kfbzStr)
		{
			thresh.push_back(var.toDouble());
		}
		double ltval = 0, rtval = 0, htval = 0;

		for (int tt = startidx; tt < mile_10m.size(); ++tt)//
		{

			if (nowExcelMile.getStartDmi() <= mile_10m[tt].getStartDmi())
			{
				startidx = tt;
				break;
			}
		}
		for (int tt = endidx; tt < mile_10m.size(); ++tt)//
		{
			if (nowExcelMile.getEndDmi() <= mile_10m[tt].getEndDmi())
			{
				endidx = tt;
				break;
			}
		}

		for (ValStridx = startidx; ValStridx <= endidx; ValStridx++)
		{
			ltval = 0; rtval = 0; htval = 0;
			if (ValStridx < mile_10m.size())
			{
				ltval = mile_10m[ValStridx].getLeftPbValue(10);
			}
			if (sett->_IsDIRIMTD)
			{
				if (ValStridx < mile_10m.size())
				{
					rtval = mile_10m[ValStridx].getRightPbValue(10);
				}
			}
			if (ltval != 0 && rtval != 0)
			{
				if (maxormean == 0)  //默认使用最大跳车，可扩展
				{
					htval = qMax(ltval, rtval);
				}
				else
				{
					htval = (ltval + rtval) / 2;
				}
			}
			else if (ltval != 0) htval = ltval;
			else if (rtval != 0) htval = rtval;
			for (int j = 0; j < thresh.size(); ++j)
			{

				//DeltaHVal[i] = htval;
				if (htval < thresh[j])
				{
					nowExcelMile.setPbiNumbers(j);
					break;
				}

			}
		}
	}
}

QVector<hnCommon::hnMarkInfo> hnOutExcelMileManage::cityReportMarks() const
{
	QVector<hnCommon::hnMarkInfo> result;
	QVector<hnCommon::hnMarkInfo> units;
	const auto marks = m_project->getCurrentMarkVector();
	for (auto mark : marks)
	{
		mark.dTrueMile = m_project->enclToTrueMile(mark.dEnclMile);
		if (mark.nType == 1 && !m_xrSetting->roadCrossingShow)
		{
			units.push_back(mark);
		}
		else
		{
			result.push_back(mark);
		}
	}
	// 与二维一致，成对的出入口取采集里程中点，再转换为桩号。
	std::stable_sort(units.begin(), units.end(), [](const hnCommon::hnMarkInfo& a, const hnCommon::hnMarkInfo& b) { return a.dEnclMile < b.dEnclMile; });
	for (int i = 1; i < units.size(); i += 2)
	{
		auto mark = units[i - 1];
		mark.dEnclMile = qFloor(qAbs(units[i].dEnclMile - mark.dEnclMile) / 2.0) + mark.dEnclMile;
		mark.dTrueMile = m_project->enclToTrueMile(mark.dEnclMile);
		result.push_back(mark);
	}
	return result;
}

QVector<hnOutExcelMile> hnOutExcelMileManage::splitCityMile(const QVector<hnCommon::hnMarkInfo>& marks)
{
	std::vector<double> units;
	for (const auto& mark : marks)
	{
		// 路口和材质重新起算；等级等属性由后续打标处理在原分段中截断。
		if (mark.nType == 0 || mark.nType == 1)
		{
			units.push_back(mark.dTrueMile);
		}
	}
	const auto boundaries = hnCityReportSegmenter().split(m_sMile, m_eMile, m_xlslen, units);
	QVector<hnOutExcelMile> result;
	hnMile mileInfo;
	for (size_t i = 1; i < boundaries.size(); ++i)
	{
		hnOutExcelMile segment(m_project, m_standard);
		segment.setSurveyWidth(hnReportProjectInfo::roadWidth(m_project));
		segment.setStartMile(boundaries[i - 1]);
		segment.setEndMile(boundaries[i]);
		segment.setStartDmi(m_project->trueMileToEncl(boundaries[i - 1]));
		segment.setEndDmi(m_project->trueMileToEncl(boundaries[i]));
		segment.setUnitStr(QString());
		segment.RoadSurface = static_cast<ROAD_SURFACE_TYPE>(m_projectSet.nRSurfaceType);
		segment.RoadSurfaceStr = segment.RoadSurface == 0 ? QStringLiteral("沥青") : segment.RoadSurface == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		segment.RoadDegreestr = QString::fromLocal8Bit(m_projectSet.strRoadLevel);
		segment.RoadGrad = mileInfo.GradStrToGrad(segment.RoadDegreestr);
		result.push_back(segment);
	}
	return result;
}

QVector<hnOutExcelMile>  hnOutExcelMileManage::splitMile(const QVector<hnMile>& filteredmileVec, double xlsLen)
{

	QVector<hnOutExcelMile> miles;
	hnMile firsthnMile = filteredmileVec.at(0);
	double firstMile = firsthnMile.dTrueMile;
	QString defaultRoadLevel = firsthnMile.roadGradStr;
	int defaultRoadGrad = firsthnMile.roadGrad;
	hnCommon::ROAD_SURFACE_TYPE defaultRoadType = firsthnMile.roadType;
	double curmile = firstMile;
	//根据分段区间初步分段
	while (m_direction * (m_eMile - curmile) > 0)
	{
		hnOutExcelMile excelMile(m_project, m_standard);
		excelMile.setSurveyWidth(hnReportProjectInfo::roadWidth(m_project));
		if (m_direction > 0)
		{

			curmile = ((int)(curmile / xlsLen) + m_direction) * xlsLen;
		}
		else
		{
			curmile = ((int)((curmile + xlsLen - 1) / xlsLen) + m_direction) * xlsLen;
		}
		if (m_direction * (m_eMile - curmile) < 0)
		{
			curmile = m_eMile;
		}
		excelMile.setStartMile(firstMile);
		excelMile.setEndMile(curmile);

		excelMile.setStartDmi(m_project->trueMileToEncl(firstMile));
		excelMile.setEndDmi(m_project->trueMileToEncl(curmile));
		excelMile.setUnitStr("");
		excelMile.RoadSurface = defaultRoadType;
		excelMile.RoadDegreestr = defaultRoadLevel;
		excelMile.RoadSurfaceStr = defaultRoadType == 0 ? QStringLiteral("沥青") : defaultRoadType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		excelMile.RoadGrad = defaultRoadGrad;
		//excelMile.StartCalculate();
		miles.push_back(excelMile);
		firstMile = curmile;
	}
	for (int t = 0; t < miles.size(); ++t)
	{
		hnOutExcelMile& excelMile = miles[t];
		excelMile.RoadSurface = static_cast<ROAD_SURFACE_TYPE>(m_projectSet.nRSurfaceType);
		excelMile.RoadSurfaceStr = m_projectSet.nRSurfaceType == 0 ? QStringLiteral("沥青") : m_projectSet.nRSurfaceType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");

		//道路等级
		excelMile.RoadDegreestr = QString::fromLocal8Bit(m_projectSet.strRoadLevel);
		excelMile.RoadGrad = firsthnMile.GradStrToGrad(excelMile.RoadDegreestr);
	}
	return miles;
}

QVector<hnOutExcelMile> hnOutExcelMileManage::splitMile_dmi(const QVector<hnMile>& filteredmileVec, double xlsLen)
{
	//乾通 里程分段
	QVector<hnOutExcelMile> miles;
	hnMile firsthnMile = filteredmileVec.at(0);
	double firstMile = firsthnMile.dTrueMile;
	QString defaultRoadLevel = firsthnMile.roadGradStr;
	int defaultRoadGrad = firsthnMile.roadGrad;
	hnCommon::ROAD_SURFACE_TYPE defaultRoadType = firsthnMile.roadType;
	double curmile = firstMile;
	MileAndDmi firstMileAndDmi(m_project, curmile, curmile, 0);
	//根据公里桩号分好区间

	QVector<MileAndDmi> oriSplit_1000;
	oriSplit_1000.push_back(firstMileAndDmi);
	int const_1000 = 1000;
	while (m_direction * (m_eMile - curmile) > 0)
	{
		if (m_direction > 0)
		{
			curmile = ((int)(curmile / const_1000) + m_direction) * const_1000;
		}
		else
		{
			curmile = ((int)((curmile + const_1000 - 1) / const_1000) + m_direction) * const_1000;
		}
		if (m_direction * (m_eMile - curmile) < 0)
		{
			curmile = m_eMile;
		}
		double realDmi = m_project->trueMileToEncl(curmile);
		MileAndDmi curMileAndDmi(m_project, curmile, curmile, realDmi);
		oriSplit_1000.push_back(curMileAndDmi);
	}

	QVector<MileAndDmi> resultSplit_1000;


	for (int i = 0; i < oriSplit_1000.size() - 1; ++i)
	{
		MileAndDmi sMile = oriSplit_1000.at(i);
		MileAndDmi eMile = oriSplit_1000.at(i + 1);
		//获得区间起点的里程
		double sDmi = m_project->trueMileToEncl(sMile.ReadMile);
		//获取区间终点的里程
		resultSplit_1000.push_back(sMile);

		double eDmi = m_project->trueMileToEncl(eMile.ReadMile);
		double curDmi = sMile.ReadMile + m_direction * (eDmi - sDmi);
		/* if (qAbs(curDmi - eMile.ReadMile)<1)
		 {
			 continue;
		 }*/

		MileAndDmi curMileAndDmi(m_project, curDmi, 0, eDmi);
		resultSplit_1000.push_back(curMileAndDmi);
	}
	QVector<MileAndDmi>result;
	if (xlsLen != 1000)
	{
		for (int i = 0; i < resultSplit_1000.size() - 1; i += 2)
		{
			MileAndDmi sMile = resultSplit_1000.at(i);
			MileAndDmi eDmi = resultSplit_1000.at(i + 1);
			double curmile = sMile.ShowMile;
			result.push_back(sMile);
			int startIdx = 0;
			while (m_direction * (eDmi.ShowMile - curmile) > 0)
			{

				if (m_direction > 0)
				{
					curmile = ((int)(curmile / xlsLen) + m_direction) * xlsLen;
				}
				else
				{
					curmile = ((int)((curmile + xlsLen - 1) / xlsLen) + m_direction) * xlsLen;
				}
				if (m_direction * (eDmi.ShowMile - curmile) < 0)
				{
					curmile = eDmi.ShowMile;
				}
				MileAndDmi curMileAndDmi(m_project, curmile, 0, sMile.Dmi + (m_direction * (curmile - sMile.ShowMile)));
				/* if (startIdx == 0)
				 {
					 startIdx++;
				 }
				 else
				 {
					 MileAndDmi lastMileAndDmi = result.last();
					 result.push_back(lastMileAndDmi);

				 }*/
				result.push_back(curMileAndDmi);
			}

		}

		/*	 MileAndDmi allLastMileAndDmi = resultSplit_1000.last();
			 result.push_back(allLastMileAndDmi);*/
	}
	else
	{
		result = resultSplit_1000;
	}

	for (int i = 0; i < result.size() - 1; i += 2)
	{
		hnOutExcelMile excelMile(m_project, m_standard);
		excelMile.setSurveyWidth(hnReportProjectInfo::roadWidth(m_project));
		MileAndDmi sMile_Dmi = result[i];
		MileAndDmi eMile_Dmi = result[i + 1];
		////计算得到此时里程对应的真实桩号
		excelMile.setStartMile(sMile_Dmi.ShowMile);
		excelMile.setEndMile(eMile_Dmi.ShowMile);
		excelMile.setStartDmi(sMile_Dmi.Dmi);
		excelMile.setEndDmi(eMile_Dmi.Dmi);

		excelMile.setUnitStr("");
		excelMile.RoadSurface = defaultRoadType;
		excelMile.RoadDegreestr = defaultRoadLevel;
		excelMile.RoadSurfaceStr = defaultRoadType == 0 ? QStringLiteral("沥青") : defaultRoadType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		excelMile.RoadGrad = defaultRoadGrad;
		//excelMile.StartCalculate();
		miles.push_back(excelMile);
	}
	int len = result.size();
	if (len % 2 != 0)
	{
		hnOutExcelMile excelMile(m_project, m_standard);
		excelMile.setSurveyWidth(hnReportProjectInfo::roadWidth(m_project));
		MileAndDmi sMile_Dmi = result[len - 2];
		MileAndDmi eMile_Dmi = result[len - 1];
		////计算得到此时里程对应的真实桩号
		excelMile.setStartMile(sMile_Dmi.ShowMile);
		excelMile.setEndMile(eMile_Dmi.ShowMile);
		excelMile.setStartDmi(sMile_Dmi.Dmi);
		excelMile.setEndDmi(eMile_Dmi.Dmi);

		excelMile.setUnitStr("");
		excelMile.RoadSurface = defaultRoadType;
		excelMile.RoadDegreestr = defaultRoadLevel;
		excelMile.RoadSurfaceStr = defaultRoadType == 0 ? QStringLiteral("沥青") : defaultRoadType == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
		excelMile.RoadGrad = defaultRoadGrad;
		//excelMile.StartCalculate();
		miles.push_back(excelMile);
	}


	return miles;
}

void hnOutExcelMileManage::handelMark(QVector<hnOutExcelMile>& miles,
	const QVector<hnCommon::hnMarkInfo> marks, bool evaluateKilometer)
{
	//由打标的信息，再将区间隔断 
	//mark从小到大排序
	hnCommon::hnMarkInfo curMark;
	int	markType = 0;
	QString strType = "";
	QString markValue = "";
	QString markFullMsg = "";

	QVector<hnCommon::hnMarkInfo> validMarks;
	if (!miles.isEmpty())
	{
		const double minDmi = qMin(miles.first().getStartDmi(), miles.last().getEndDmi()) - 0.5;
		const double maxDmi = qMax(miles.first().getStartDmi(), miles.last().getEndDmi()) + 0.5;
		for (const auto& mark : marks)
		{
			if (mark.dEnclMile <= maxDmi && (mark.dEnclMile >= minDmi
				|| m_standard == HnProjectEnums::RuralRoadlowLevel || m_cityDistanceSegments))
			{
				validMarks.push_back(mark);
			}
		}
		std::stable_sort(validMarks.begin(), validMarks.end(), [](const hnCommon::hnMarkInfo& a, const hnCommon::hnMarkInfo& b) { return a.dEnclMile < b.dEnclMile; });
		// 分段导出继承起点之前的属性；这些标记在起点生效，不生成范围外单元。
		if (m_standard == HnProjectEnums::RuralRoadlowLevel || m_cityDistanceSegments)
		{
			for (auto& mark : validMarks)
			{
				if (mark.dEnclMile < miles.first().getStartDmi())
				{
					mark.dEnclMile = miles.first().getStartDmi();
					mark.dTrueMile = miles.first().getStartMile();
				}
			}
		}
	}
	if (validMarks.size() > 0)
	{
		curMark = validMarks.at(0);
	}
	else
	{
		// 无打标的路线也必须处理首尾不足500米的单元。
		if (evaluateKilometer)
		{
			hnRuralEvaluationUnitMerger().merge(miles);
		}
		return;
	}
	auto curMile = miles[0];
	ROAD_SURFACE_TYPE RoadSurface = curMile.RoadSurface;
	QString RoadSurfaceStr = curMile.RoadSurfaceStr;
	QString RoaddegreeStr = curMile.RoadDegreestr;
	int RoadGrad = curMile.RoadGrad;
	HnProjectEnums::StandardParmTypeEnum standard = curMile.Type;
	QString unitStr = curMile.getUnitStr();
	hnMile temp;
	for (int i = 0, j = 0; i < miles.size(); i++)
	{
		//0-路面材质；1-路面单元；2-路面等级; 3-路面标准；4-路面情况
		while (j < validMarks.size() &&
			(miles[i].getStartDmi() <= validMarks[j].dEnclMile && miles[i].getEndDmi() > validMarks[j].dEnclMile)
			)
		{
			markValue = QString::fromLocal8Bit(validMarks[j].strMark);
			switch (validMarks[j].nType)
			{
			case  0:
			{
				int value = markValue == QStringLiteral("沥青") ? 0 : markValue == QStringLiteral("水泥") ? 1 : 2;
				ROAD_SURFACE_TYPE surface = static_cast<ROAD_SURFACE_TYPE>(value);
				if (miles[i].RoadSurface != surface)
				{
					hnOutExcelMile	newMile(m_project, miles[i].Type);
					RoadSurface = surface;


					double endDmi = miles[i].getEndDmi();
					if (m_useDmiFormat)
					{
						//打标位置的桩号 等于上一段的桩号+里程差
						double length = qAbs(validMarks[j].dEnclMile - miles[i].getStartDmi());
						double dmiMile = (length * m_direction) + miles[i].getStartMile();
						newMile.setStartMile(dmiMile);
						newMile.setEndMile(miles[i].getEndMile());
						miles[i].setEndMile(dmiMile);
					}
					else
					{
						newMile.setStartMile(validMarks[j].dTrueMile);
						newMile.setEndMile(miles[i].getEndMile());
						miles[i].setEndMile(validMarks[j].dTrueMile);
						miles[i].setEndDmi(validMarks[j].dEnclMile);
					}


					miles[i].setEndDmi(validMarks[j].dEnclMile);
					newMile.setStartDmi(validMarks[j].dEnclMile);
					newMile.setEndDmi(endDmi);
					//更新当前分段的长度

					//先赋值为前一个分段的值
					initMarkMehtodExcelMile(newMile, miles[i]);
					newMile.RoadSurface = RoadSurface;

					RoadSurfaceStr = RoadSurface == 0 ? QStringLiteral("沥青") : RoadSurface == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
					newMile.RoadSurfaceStr = RoadSurfaceStr;
					miles.insert(i + 1, newMile);
				}
				else
				{
					miles[i].RoadSurface = surface;
					miles[i].RoadSurfaceStr = value == 0 ? QStringLiteral("沥青") : RoadSurface == 1 ? QStringLiteral("水泥") : QStringLiteral("砂石");
				}

				break;
			}
			case  1:
			{
				if (m_xrSetting->outRoadUnitMark
					|| m_standard == HnProjectEnums::RuralRoadlowLevel || m_cityDistanceSegments)
				{
					QString value = markValue;
					unitStr = value;

					if (miles[i].getUnitStr() != value)
					{
						hnOutExcelMile	newMile(m_project, miles[i].Type);
						double endDmi = miles[i].getEndDmi();
						if (m_useDmiFormat)
						{
							//打标位置的桩号 等于上一段的桩号+里程差
							double length = qAbs(validMarks[j].dEnclMile - miles[i].getStartDmi());
							double dmiMile = (length * m_direction) + miles[i].getStartMile();
							newMile.setStartMile(dmiMile);
							newMile.setEndMile(miles[i].getEndMile());
							miles[i].setEndMile(dmiMile);
						}
						else
						{
							newMile.setStartMile(validMarks[j].dTrueMile);
							newMile.setEndMile(miles[i].getEndMile());
							miles[i].setEndMile(validMarks[j].dTrueMile);
							miles[i].setEndDmi(validMarks[j].dEnclMile);
						}

						miles[i].setEndDmi(validMarks[j].dEnclMile);
						newMile.setStartDmi(validMarks[j].dEnclMile);
						newMile.setEndDmi(endDmi);



						initMarkMehtodExcelMile(newMile, miles[i]);
						newMile.setUnitStr(value);
						miles.insert(i + 1, newMile);
					}
					else
					{
						miles[i].setUnitStr(value);
					}
				}
				break;
			}

			case  2:
			{
				if (miles[i].RoadDegreestr != markValue)
				{
					hnOutExcelMile	newMile(m_project, miles[i].Type);

					RoaddegreeStr = markValue;
					double endDmi = miles[i].getEndDmi();
					if (m_useDmiFormat)
					{
						//打标位置的桩号 等于上一段的桩号+里程差
						double length = qAbs(validMarks[j].dEnclMile - miles[i].getStartDmi());
						double dmiMile = (length * m_direction) + miles[i].getStartMile();
						newMile.setStartMile(dmiMile);
						newMile.setEndMile(miles[i].getEndMile());
						miles[i].setEndMile(dmiMile);
					}
					else
					{
						newMile.setStartMile(validMarks[j].dTrueMile);
						newMile.setEndMile(miles[i].getEndMile());

						miles[i].setEndMile(validMarks[j].dTrueMile);
						miles[i].setEndDmi(validMarks[j].dEnclMile);
					}

					miles[i].setEndDmi(validMarks[j].dEnclMile);
					newMile.setStartDmi(validMarks[j].dEnclMile);
					newMile.setEndDmi(endDmi);


					//先赋值为前一个分段的值
					initMarkMehtodExcelMile(newMile, miles[i]);
					newMile.RoadDegreestr = markValue;
					newMile.RoadGrad = temp.GradStrToGrad(markValue);
					// Road grade is a segment calculation property.  Do not duplicate it in the generic remark column;
					// the original, stake-qualified grade marker is emitted separately when marker output is requested.
					unitStr = newMile.getUnitStr();
					RoadGrad = newMile.RoadGrad;
					miles.insert(i + 1, newMile);
				}
				else
				{
					miles[i].RoadDegreestr = markValue;
				}

				break;
			}

			case  3:
			{
				HnProjectEnums::StandardParmTypeEnum type = HnProjectEnums::roadTypeQStringToEnum(markValue);
				standard = type;
				if (miles[i].Type != type)
				{
					hnOutExcelMile	newMile(m_project, miles[i].Type);


					double endDmi = miles[i].getEndDmi();
					if (m_useDmiFormat)
					{
						//打标位置的桩号 等于上一段的桩号+里程差
						double length = qAbs(validMarks[j].dEnclMile - miles[i].getStartDmi());
						double dmiMile = (length * m_direction) + miles[i].getStartMile();
						newMile.setStartMile(dmiMile);
						newMile.setEndMile(miles[i].getEndMile());
						miles[i].setEndMile(dmiMile);
					}
					else
					{
						newMile.setStartMile(validMarks[j].dTrueMile);
						newMile.setEndMile(miles[i].getEndMile());
						miles[i].setEndDmi(validMarks[j].dEnclMile);
						miles[i].setEndMile(validMarks[j].dTrueMile);
					}

					miles[i].setEndDmi(validMarks[j].dEnclMile);
					newMile.setStartDmi(validMarks[j].dEnclMile);
					newMile.setEndDmi(endDmi);


					//先赋值为前一个分段的值
					initMarkMehtodExcelMile(newMile, miles[i]);
					newMile.Type = type;
					miles.insert(i + 1, newMile);
				}
				else
				{
					miles[i].Type = type;
				}
				break;

			}
			default:

				break;
			}
			j++;
		}
		if (i + 1 < miles.size())
		{
			miles[i + 1].RoadDegreestr = RoaddegreeStr;
			miles[i + 1].RoadGrad = RoadGrad;

			miles[i + 1].RoadSurface = RoadSurface;
			miles[i + 1].RoadSurfaceStr = RoadSurfaceStr;
			miles[i + 1].Type = standard;
			miles[i + 1].setUnitStr(unitStr);
		}


	}


	for (int i = 0; i < miles.size(); ++i)
	{
		if (miles[i].getStartMile() == miles[i].getEndMile())
		{
			miles.removeAt(i--);
		}
	}

	// 在备注带上桩号之前合并，避免备注差异被误认为路段属性发生变化。
	if (evaluateKilometer)
	{
		hnRuralEvaluationUnitMerger().merge(miles);
	}
	if (!m_xrSetting->outMileWithMark)
	{
		return;
	}
	//给备注添加信息
	for (int i = 0; i < miles.size(); ++i)
	{
		for (int markIndex = 0; markIndex < marks.size(); ++markIndex)
		{


			if (miles.at(i).getStartDmi() <= marks.at(markIndex).dEnclMile
				&& marks.at(markIndex).dEnclMile < miles.at(i).getEndDmi())
			{
				int qian = (int)(marks.at(markIndex).dTrueMile / 1000);
				int bai = qRound(marks.at(markIndex).dTrueMile - qian * 1000);
				QString mile = "K" + QString::number(qian) + "+" + QString::number(bai).rightJustified(3, '0');
				QString markTypeStr = "";
				QString markValue = QString::fromLocal8Bit(marks.at(markIndex).strMark);
				switch (marks.at(markIndex).nType)//打标类型:0 - 路面材质；1 - 路面单元；2 - 路面等级; 3 - 路面标准
				{

				case 0:
					markTypeStr = QStringLiteral("路面材质 ") + markValue;
					break;
				case  1:
					markTypeStr = QStringLiteral("路面单元 ") + markValue;
					break;
				case 2:
					markTypeStr = QStringLiteral("路面等级 ") + markValue;
					break;
				case 3:
					markTypeStr = QStringLiteral("路面标准 ") + markValue;
					break;
				case 4:
					markTypeStr = QStringLiteral("路面情况 ") + markValue;
					break;
				default:
					break;
				}
				if (miles[i].getUnitStr().isNull() || miles[i].getUnitStr() == "")
				{
					QString unitStr = miles[i].getUnitStr() + mile + ":" + markTypeStr;

					miles[i].setUnitStr(unitStr);
				}
				else
				{
					QString unitStr = miles[i].getUnitStr() + ("\n" + mile + ":" + markTypeStr);
					miles[i].setUnitStr(unitStr);
				}

			}
		}
	}
}

bool hnOutExcelMileManage::writeIriValue(bool& hasLeftIRI, bool& hasRightIRI, double BaseLen)
{
	//获得平整度
	QString iriResultPath = m_project->get2DProject()->getIRIPath();
	QString leftIriPath = iriResultPath + "\\DAQ0\\" + "IRI_" + QString::number(10) + "m.txt";
	QString reftIriPath = iriResultPath + "\\DAQ1\\" + "IRI_" + QString::number(10) + "m.txt";
	QFile file(leftIriPath);
	QStringList lists;
	QVector<double> lValue;
	QVector<double> RValue;
	bool ok = false;
	QStringList rists;
	if (file.exists())
	{
		hasLeftIRI = true;
		lists = MyCommonMethods::ReadAllLines(leftIriPath);
		double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
		if (lists.size() * 10 < endDmi - endDmi / 5)
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("平整度"), QStringLiteral("左侧"), leftIriPath, lists.size(), qRound(endDmi / 10.0), true);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//	throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

		}
		for (QString line : lists)
		{
			QStringList split = line.split("\t");
			if (split.size() <= 1)
			{
				split = line.split(" ");
			}
			if (split.size() > 1)
			{
				double value = split.at(1).toDouble();
				lValue.push_back(value);
			}
		}
		ok = true;
	}
	else
	{
		try
		{

			QString message = irmDataError(m_project->get2DProName(), QStringLiteral("平整度"), QStringLiteral("左侧"), leftIriPath, 0, 0, false);
			reportExcelError(message);
			string mes = message.toLocal8Bit();

			//	throw std::runtime_error(mes.c_str());
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}

	}

	QFile file1(reftIriPath);
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		if (file1.exists())
		{
			hasRightIRI = true;
			rists = MyCommonMethods::ReadAllLines(reftIriPath);
			double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
			if (rists.size() * 10 < endDmi - endDmi / 5)
			{
				try
				{
					QString message = irmDataError(m_project->get2DProName(), QStringLiteral("平整度"), QStringLiteral("右侧"), reftIriPath, rists.size(), qRound(endDmi / 10.0), true);
					string mes = message.toLocal8Bit();
					reportExcelError(message);
					//throw std::runtime_error(mes.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << e.what() << std::endl;
				}

			}
			for (QString line : rists)
			{
				QStringList split = line.split("\t");
				if (split.size() <= 1)
				{
					split = line.split(" ");
				}
				if (split.size() > 1)
				{
					double value = split.at(1).toDouble();
					RValue.push_back(value);
				}
			}
			ok = true;
		}
		else
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("平整度"), QStringLiteral("右侧"), reftIriPath, 0, 0, false);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//	throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}

	if (ok == false)
	{
		return false;
	}
	BaseLen = 10;
	int len = m_roadSplietVec.size();
	std::unique_ptr<double[]> lval(new double[len]);
	std::unique_ptr<double[]>rval(new double[len]);
	QString LStrLine, RStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0;

	for (int i = 0; i < len; i++)
	{
		double suml = 0, sumr = 0;
		QStringList tmtd;
		int lvalnum = 0, rvalnum = 0;
		auto v1 = m_roadSplietVec[i].getStartDmi();
		auto v2 = m_roadSplietVec[i].getEndDmi();
		startidx = MyCommonMethods::MathRoundToInt((v1 - 0.5) / BaseLen);
		endidx = MyCommonMethods::MathRoundToInt(v2 / BaseLen);
		if (startidx >= endidx)
		{
			if (startidx < lValue.size())
			{
				lastvalL = lValue[startidx];

				suml += lastvalL;
				++lvalnum;
			}
			if (m_project->get2DProject()->_IsDIRIMTD)
			{
				if (startidx < RValue.size())
				{
					lastvalR = RValue[startidx];

					sumr += lastvalR;
					++rvalnum;
				}
			}

		}
		else
		{
			for (ValStridx = startidx; ValStridx < endidx; ValStridx++)
			{
				if (ValStridx < lValue.size())
				{
					lastvalL = lValue[ValStridx];
					suml += lastvalL;
					++lvalnum;
				}
				if (m_project->get2DProject()->_IsDIRIMTD)
				{
					if (ValStridx < RValue.size())
					{
						lastvalR = RValue[ValStridx];
						sumr += lastvalR;
						++rvalnum;
					}

				}
			}
		}
		if (lvalnum > 0)
		{
			suml /= lvalnum;
		}
		if (rvalnum > 0)
		{
			sumr /= rvalnum;
		}

		if (lvalnum > 0)
		{
			lval[i] = suml;
		}
		else if (rvalnum > 0)
		{
			lval[i] = sumr;
		}
		else if (i > 0)
		{
			lval[i] = lval[i - 1];
		}
		else
		{
			lval[i] = 0;
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rvalnum > 0)
			{
				rval[i] = sumr;
			}
			else if (lvalnum > 0)
			{
				rval[i] = suml;
			}
			else if (i > 0)
			{
				rval[i] = rval[i - 1];
			}
			else
			{
				rval[i] = 0;
			}
		}
	}
	if (len >= 2)
	{
		if (lval[0] == 0)
		{
			lval[0] = lval[1];
		}
		if (lval[len - 1] == 0)
		{
			lval[len - 1] = lval[len - 2];
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rval[0] == 0)
			{
				rval[0] = rval[1];
			}
			if (rval[len - 1] == 0)
			{
				rval[len - 1] = rval[len - 2];
			}
		}
	}

	QVector<double> lValues;
	double lastHasValue = 0;
	for (int i = 0; i < len; ++i)
	{
		if (qAbs(lval[i]) < 0.0001)
		{
			lValues.push_back(lastHasValue);
		}
		else
		{
			lValues.push_back(lval[i]);
			lastHasValue = lval[i];
		}

		m_roadSplietVec[i].setLeftIriValue(lValues[i]);
	}
	QVector<double> rValues;
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		for (int i = 0; i < len; ++i)
		{
			if (qAbs(rval[i]) < 0.0001)
			{
				rValues.push_back(lastHasValue);
			}
			else
			{
				rValues.push_back(rval[i]);
				lastHasValue = rval[i];
			}

			m_roadSplietVec[i].setRightIriValue(rValues[i]);
		}
	}

	return true;
}

bool hnOutExcelMileManage::writeSpeedValue(QVector<hnOutExcelMile>& miles, double BaseLen)
{
	bool hasValue = false;
	QString  resultPath = m_project->get2DProject()->getIRIPath();
	const QString leftSpeedPath = resultPath + "\\DAQ0\\" + "Speed_" + QString::number(10) + "m.txt";
	const QString rightSpeedPath = resultPath + "\\DAQ1\\" + "Speed_" + QString::number(10) + "m.txt";
	QString speedPath = leftSpeedPath;
	QFile file(speedPath);
	QStringList  lists;
	QVector<double >lValue;
	QVector<double >RValue;
	if (file.exists())
	{
		lists = MyCommonMethods::ReadAllLines(speedPath);
		for (QString line : lists)
		{
			QStringList split = line.split(" ");
			if (split.size() <= 1)
			{
				split = line.split("\t");
			}
			if (split.size() > 1)
			{

				double value = split.at(1).toDouble();
				if (split.at(1).compare("inf", Qt::CaseInsensitive) == 0 || value < 0)
				{
					lValue.push_back(0);
				}
				else
				{
					lValue.push_back(value);
				}
				hasValue = true;
			}
		}

	}
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		speedPath = rightSpeedPath;
		lists = MyCommonMethods::ReadAllLines(speedPath);
		for (QString line : lists)
		{
			QStringList split = line.split(" ");
			if (split.size() <= 1)
			{
				split = line.split("\t");
			}
			if (split.size() > 1)
			{
				hasValue = true;
				double value = split.at(1).toDouble();
				if (split.at(1).compare("inf", Qt::CaseInsensitive) == 0 || value < 0)
				{
					RValue.push_back(0);
				}
				else
				{
					RValue.push_back(value);
				}
			}
		}
	}
	if (!hasValue)
	{
		QStringList checkedPaths;
		checkedPaths.append(leftSpeedPath);
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			checkedPaths.append(rightSpeedPath);
		}
		reportExcelError(m_project->get2DProName()
			+ QStringLiteral("\r\n未读取到车速数据，车速列将留空，不影响其他指标计算。\r\n检查文件：")
			+ checkedPaths.join(QStringLiteral("\r\n")));
		return false;
	}
	int len = m_roadSplietVec.size();
	std::unique_ptr<double[]> lval(new double[len] {0});
	std::unique_ptr<double[]>rval(new double[len] {0});
	QString LStrLine, RStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0;
	BaseLen = 10;
	for (int i = 0; i < len; i++)
	{
		double suml = 0, sumr = 0;
		QStringList tmtd;
		int lvalnum = 0, rvalnum = 0;
		double sdmi = m_roadSplietVec[i].getStartDmi();
		double edmi = m_roadSplietVec[i].getEndDmi();
		startidx = MyCommonMethods::MathRoundToInt((sdmi - 0.5) / BaseLen);
		endidx = MyCommonMethods::MathRoundToInt((edmi) / BaseLen);
		if (startidx >= endidx)
		{
			if (startidx < lValue.size())
			{
				lastvalL = lValue[startidx];

				suml += lastvalL;
				++lvalnum;
			}
			if (m_project->get2DProject()->_IsDIRIMTD)
			{
				if (startidx < RValue.size())
				{
					lastvalR = RValue[startidx];
					sumr += lastvalR;
					++rvalnum;
				}
			}

		}
		else
		{
			for (ValStridx = startidx; ValStridx < endidx; ValStridx++)
			{
				if (ValStridx < lValue.size())
				{
					lastvalL = lValue[ValStridx];
					suml += lastvalL;
					++lvalnum;
				}
				if (m_project->get2DProject()->_IsDIRIMTD)
				{
					if (ValStridx < RValue.size())
					{
						lastvalR = RValue[ValStridx];
					}
					sumr += lastvalR;
					++rvalnum;
				}
			}
		}
		if (lvalnum > 0)
		{
			suml /= lvalnum;
		}
		if (rvalnum > 0)
		{
			sumr /= rvalnum;
		}

		if (lvalnum > 0)
		{
			lval[i] = suml;
		}
		if (rvalnum > 0)
		{
			rval[i] = sumr;
		}
	}
	if (len >= 2)
	{
		if (lval[0] == 0)
		{
			lval[0] = lval[1];
		}
		if (lval[len - 1] == 0)
		{
			lval[len - 1] = lval[len - 2];
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rval[0] == 0)
			{
				rval[0] = rval[1];
			}
			if (rval[len - 1] == 0)
			{
				rval[len - 1] = rval[len - 2];
			}
		}
	}

	QVector<double> lValues;
	double lastHasValue = 0;
	for (int i = 0; i < len; ++i)
	{
		if (qAbs(lval[i]) < 0.0001)
		{
			lValues.push_back(lastHasValue);
		}
		else
		{
			lValues.push_back(lval[i]);
			lastHasValue = lval[i];
		}

		miles[i].setSpeed(lValues[i]);
	}
	QVector<double> rValues;
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		for (int i = 0; i < len; ++i)
		{
			if (qAbs(rval[i]) < 0.0001)
			{
				rValues.push_back(lastHasValue);
			}
			else
			{
				rValues.push_back(rval[i]);
				lastHasValue = rval[i];
			}
			double value = (miles[i].getSpeed() + rValues[i]) / 2;

			double minValue = qMin(lValues[i], rValues[i]);

			miles[i].setSpeed(minValue);
		}
	}
	return true;
}

bool  hnOutExcelMileManage::writePbiValue(QVector<hnOutExcelMile>& miles)
{
	QString iriResultPath = m_project->get2DProject()->getIRIPath();
	QString leftPath = iriResultPath + "\\DAQ0\\" + "resample.txt";
	QString reftPath = iriResultPath + "\\DAQ1\\" + "resample.txt";


	QFile file(leftPath);
	bool ok = false;
	if (file.exists())
	{
		QStringList leftValues = MyCommonMethods::ReadAllLines(leftPath);
		QVector<double> values1;
		readPbiValueFromFile(miles, leftValues, values1);
		for (int i = 0; i < miles.size(); ++i)
		{
			miles[i].StartCalculate(true);
			if (values1.size() > i)
			{
				// 涂工 20230816 人工纠正因子
				QStringList splits = m_xrSetting->MpdInterveneFAactor.split(',');

				if (values1[i] < 50)
				{
					values1[i] = values1[i] * splits.first().toDouble();
				}
				if (values1[i] < 80 && values1[i] >= 50)
				{
					values1[i] = values1[i] * splits.last().toDouble();
				}

				if (i == miles.size() - 1)
				{
					int ddd = 0;
				}
				miles[i].setLeftPbValue(values1.at(i));
			}
		}
		ok = true;

	}
	QFile file1(reftPath);

	if (file1.exists())
	{
		QStringList reftValues = MyCommonMethods::ReadAllLines(reftPath);
		QVector<double> values2;
		try
		{
			readPbiValueFromFile(miles, reftValues, values2);

		}
		catch (exception ex)
		{
			throw ex;
		}

		for (int i = 0; i < miles.size(); ++i)
		{

			// 涂工 20230816 人工纠正因子
			QStringList splits = m_xrSetting->MpdInterveneFAactor.split(',');
			if (values2.size() > i)
			{
				if (values2[i] < 50)
				{
					values2[i] = values2[i] * splits.first().toDouble();
				}
				if (values2[i] < 80 && values2[i] >= 50)
				{
					values2[i] = splits.last().toDouble() * values2[i];
				}
				miles[i].setRightPbValue(values2.at(i));
			}
		}
		//计算PBI 
		ok = true;
	}

	if (!ok)
	{
		return false;
	}
	for (int i = 0; i < miles.size(); ++i)
	{
		miles[i].calculatePBIScore();
	}
	return true;
}

bool hnOutExcelMileManage::writeJHXXValue(double BaseLen)
{
	QString resultDirectory;
	if (m_project->getProjectType() == PROJECT_23D_TYPE && m_project->get2DProject())
		resultDirectory = m_project->get2DProject()->getBasePath();
	else
		resultDirectory = m_project->get3DProPath();
	const QString dataPath = QDir(resultDirectory).filePath(QStringLiteral("Geoalig_10m.txt"));
	QStringList datas = MyCommonMethods::ReadAllLines(dataPath);

	if (datas.size() <= 0)
	{
		QString message = m_project->get2DProName() + QStringLiteral("\r\n尚未计算几何线型!");
		string mes = message.toLocal8Bit();
		reportExcelError(message);
		return false;
	}

	struct GeometryReportRow
	{
		double mileage = 0.0;
		double curvature = 0.0;
		double longitudinalSlope = 0.0;
		double crossSlope = 0.0;
		bool curvatureValid = true;
		bool longitudinalValid = true;
		bool crossValid = true;
	};
	QVector<GeometryReportRow> geometryRows;
	for (int i = 0; i < datas.size(); ++i)
	{
		QStringList split = datas[i].split(',');
		if (split.size() < 4)
			continue;
		bool mileageOk = false, curvatureOk = false, longitudinalOk = false, crossOk = false;
		GeometryReportRow row;
		row.mileage = split[0].toDouble(&mileageOk);
		row.curvature = split[1].toDouble(&curvatureOk);
		row.longitudinalSlope = split[2].toDouble(&longitudinalOk);
		row.crossSlope = split[3].toDouble(&crossOk);
		if (mileageOk && curvatureOk && longitudinalOk && crossOk)
			geometryRows.append(row);
	}
	if (geometryRows.isEmpty())
	{
		reportExcelError(m_project->get2DProName() + QStringLiteral("\r\nGeoalig_10m.txt没有有效数据!"));
		return false;
	}
	std::sort(geometryRows.begin(), geometryRows.end(), [](const GeometryReportRow& left, const GeometryReportRow& right) {
		return left.mileage < right.mileage;
		});

	// 新文件使用质量侧车文件；历史四列文件没有质量文件时保持全部数值有效。
	const QString qualityPath = QDir(resultDirectory).filePath(QStringLiteral("Geoalig_10m.quality.csv"));
	const QStringList qualityLines = MyCommonMethods::ReadAllLines(qualityPath);
	QMap<qint64, QVector<bool> > qualityByMileage;
	for (int i = 1; i < qualityLines.size(); ++i)
	{
		const QStringList fields = qualityLines[i].split(',');
		if (fields.size() < 4)
			continue;
		bool ok = false;
		const double mileage = fields[0].toDouble(&ok);
		if (!ok)
			continue;
		QVector<bool> flags;
		flags << (fields[1].toInt() != 0) << (fields[2].toInt() != 0) << (fields[3].toInt() != 0);
		qualityByMileage.insert(qRound64(mileage * 1000.0), flags);
	}
	bool qualityComplete = !qualityByMileage.isEmpty() && qualityByMileage.size() == geometryRows.size();
	if (qualityComplete)
	{
		for (const GeometryReportRow& row : geometryRows)
		{
			if (!qualityByMileage.contains(qRound64(row.mileage * 1000.0)))
			{
				qualityComplete = false;
				break;
			}
		}
	}
	if (QFileInfo::exists(qualityPath) && !qualityComplete)
	{
		reportExcelError(m_project->get2DProName()
			+ QStringLiteral("\r\n几何线型质量文件与Geoalig_10m.txt不匹配，请重新计算几何线型!"));
		return false;
	}
	if (qualityComplete)
	{
		for (GeometryReportRow& row : geometryRows)
		{
			const QVector<bool> flags = qualityByMileage.value(qRound64(row.mileage * 1000.0));
			if (flags.size() == 3)
			{
				row.curvatureValid = flags[0];
				row.longitudinalValid = flags[1];
				row.crossValid = flags[2];
			}
		}
	}

	BaseLen = 10.0;
	const int len = m_roadSplietVec.size();
	QVector<double>lval(len);
	QVector<double>rval(len);
	QVector<double>aval(len);
	for (int i = 0; i < len; i++)
	{
		double sumCurvature = 0.0, sumLongitudinal = 0.0, sumCross = 0.0;
		int curvatureCount = 0, longitudinalCount = 0, crossCount = 0;
		const double startMileage = m_roadSplietVec[i].getStartDmi();
		const double endMileage = m_roadSplietVec[i].getEndDmi();
		for (const GeometryReportRow& row : geometryRows)
		{
			// 每一行代表[mileage, mileage+10m)，按真实里程判断是否与报表段相交。
			if (row.mileage >= endMileage || row.mileage + BaseLen <= startMileage)
				continue;
			if (row.curvatureValid) { sumCurvature += row.curvature; ++curvatureCount; }
			if (row.longitudinalValid) { sumLongitudinal += row.longitudinalSlope; ++longitudinalCount; }
			if (row.crossValid) { sumCross += row.crossSlope; ++crossCount; }
		}
		if (curvatureCount == 0 || longitudinalCount == 0 || crossCount == 0)
		{
			reportExcelError(m_project->get2DProName()
				+ QStringLiteral("\r\n几何线型在里程%1-%2内缺少有效的曲率、纵坡或横坡数据!")
				.arg(startMileage, 0, 'f', 1).arg(endMileage, 0, 'f', 1));
			return false;
		}
		lval[i] = sumCurvature / curvatureCount;
		rval[i] = sumLongitudinal / longitudinalCount;
		aval[i] = sumCross / crossCount;
	}

	for (int i = 0; i < len; ++i)
	{
		m_roadSplietVec[i].setCurvature(lval[i]);
		m_roadSplietVec[i].setLongitudianalSlope(rval[i]);
		m_roadSplietVec[i].setCrossSlope(aval[i]);
	}

	return true;
}

void hnOutExcelMileManage::readPbiValueFromFile(QVector<hnOutExcelMile>& miles, QStringList& leftValues, QVector<double>& val)
{
	//原始断面长度 50mm  Resample.txt文件0.05米一个值
	double pluselen = 0.05;
	//计算跳车断面长度 100mm
	double baselen = 0.1;
	int skipnum = (int)(baselen / pluselen);
	QVector<double> oriDataD;
	QVector<QString> debugDatas;
	QVector<double>  nextDataD;
	double oriValue = 0, max = 0, min = 0, hval = 0;
	int ValStridx = 0;
	for (int i = 0; i < leftValues.size(); ++i)
	{
		try
		{
			QStringList splits = leftValues[i].split("\t");
			oriValue = splits[2].toDouble();
			oriDataD.push_back(oriValue);
			debugDatas.push_back(leftValues[i]);
			nextDataD.push_back(oriValue);
		}
		catch (...)
		{

			continue;
		}
	}
	//均值滤波
	for (int i = 2; i < leftValues.size() - 2; ++i)
	{
		oriDataD[i] = (nextDataD[i - 2] + nextDataD[i - 1] + nextDataD[i] + nextDataD[i + 1] + nextDataD[i + 2]) / 5;
	}
	//QStringList temp;
	int i = 0;
	for (hnOutExcelMile& excelMile : miles)
	{
		i++;
		if (i == miles.size() - 1)
		{
			int t = 3;
		}
		bool HasData = false;
		//获得分段的 起始里程和终止里程
		double sMile = excelMile.getStartDmi();
		double eMile = excelMile.getEndDmi();

		int startidx = qRound(sMile / pluselen);
		int endidx = qRound(eMile / pluselen);
		if (endidx >= oriDataD.size())
		{

			endidx = oriDataD.size();
		}
		//获得区间
		max = -100000; min = 100000; hval = 0;
		for (ValStridx = startidx; ValStridx < endidx; ValStridx++)
		{
			HasData = true;
			try
			{
				if (ValStridx % skipnum == 0)
				{
					//temp.append(debugDatas[ValStridx]);
					hval = oriDataD.at(ValStridx);

					max = qMax(hval, max);
					min = qMin(hval, min);
				}
			}
			catch (...)
			{

			}
		}
		if (HasData)
		{
			val.push_back(max - min);
		}
	}
	//QString filePath = "D:\\tiaoche\\hDatas.txt";
	//QFile file(filePath);
	//if (file.open(QIODevice::WriteOnly | QIODevice::Text))
	//{
	//	file.close();
	//}
	//MyCommonMethods::writeAllLines("D:\\tiaoche\\hDatas.txt", temp);
}

bool hnOutExcelMileManage::writeRutValue()
{

	QString resultPath = m_project->get2DProject()->getBasePath();
	QString leftpath = resultPath + "\\Rut\\camera0\\orirut.txt";
	QString rightPath = resultPath + "\\Rut\\camera1\\orirut.txt";

	QFile file(leftpath);
	QStringList LStrs;

	bool ok = false;
	QStringList RStrs;
	if (file.exists())
	{

		LStrs = MyCommonMethods::ReadAllLines(leftpath);
		double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
		if (LStrs.size() / 10 < endDmi - endDmi / 5)
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("左侧"), leftpath, LStrs.size(), qRound(endDmi * 10.0), true);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

		}

		ok = true;
	}
	else
	{
		try
		{
			QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("左侧"), leftpath, 0, 0, false);
			string mes = message.toLocal8Bit();
			reportExcelError(message);
			//throw std::runtime_error(mes.c_str());
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
	QFile file1(rightPath);
	if (m_project->get2DProject()->_RutMode == 1)
	{
		if (file1.exists())
		{

			RStrs = MyCommonMethods::ReadAllLines(rightPath);
			double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
			if (RStrs.size() / 10 < endDmi - endDmi / 5)
			{
				try
				{
					QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("右侧"), rightPath, RStrs.size(), qRound(endDmi * 10.0), true);
					string mes = message.toLocal8Bit();
					reportExcelError(message);
					//	throw std::runtime_error(mes.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << e.what() << std::endl;
				}

			}

			ok = true;
		}
		else
		{
			try
			{
				QString message = irmDataError(m_project->get2DProName(), QStringLiteral("车辙"), QStringLiteral("右侧"), rightPath, 0, 0, false);
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}
	}

	if (ok == false)
	{
		return false;
	}
	//const double BaseLen = 10;
	int len = m_roadSplietVec.size();

	QVector<double>lval(len);
	QVector<double>rval(len);
	QVector<double>sval(len);
	QString LStrLine, RStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0;
	double BaseLen = 0.1;
	for (int i = 0; i < len; i++)
	{
		double suml = 0, sumr = 0, sums = 0, ltval = 0, rtval = 0;
		QStringList tmtd;
		int lvalnum = 0, rvalnum = 0, svalnum = 0;

		QStringList tval;
		double temp = m_project->get2DProject()->_DMIScale;
		double dimS = m_roadSplietVec[i].getStartDmi();
		auto dimE = m_roadSplietVec[i].getEndDmi();
		startidx = MyCommonMethods::MathRoundToInt(dimS * m_project->get2DProject()->_DMIScale / BaseLen);
		endidx = MyCommonMethods::MathRoundToInt(dimE * temp / BaseLen);
		for (ValStridx = startidx; ValStridx < endidx; ++ValStridx)
		{
			if (m_project->get2DProject()->_RutMode == 1)
			{
				if (ValStridx < LStrs.size())
				{
					LStrLine = LStrs[ValStridx];
					tval = LStrLine.split(',');
					if (tval.size() < 2)
					{
						continue;
					}
					ltval = qAbs(tval[1].toDouble()) + m_xrSetting->rutLeftCorrect;
					suml += ltval;
					++lvalnum;
				}
				if (ValStridx < RStrs.size())
				{
					RStrLine = RStrs[ValStridx];
					tval = RStrLine.split(',');
					if (tval.size() < 2)
					{
						continue;
					}
					rtval = qAbs(tval[1].toDouble()) + m_xrSetting->rutRightCorrect;
					sumr += rtval;
					++rvalnum;
				}
				sums += qMax(ltval, rtval);
				++svalnum;
			}
			else
			{
				if (ValStridx < LStrs.size())
				{
					LStrLine = LStrs[ValStridx];
					tval = LStrLine.split(',');
					if (tval.size() <= 3)
					{
						continue;
					}
					ltval = qAbs(tval[1].toDouble()) + m_xrSetting->rutLeftCorrect;
					rtval = qAbs(tval[3].toDouble()) + m_xrSetting->rutRightCorrect;
					suml += ltval;
					sumr += rtval;
					sums += qMax(ltval, rtval);
					++lvalnum;
					++rvalnum;
					++svalnum;
				}
			}

		}
		if (lvalnum > 0) suml /= lvalnum;
		if (rvalnum > 0) sumr /= rvalnum;
		if (svalnum > 0) sums /= svalnum;

		if (lvalnum > 0) lval[i] = suml;
		else if (rvalnum > 0) lval[i] = sumr;
		else lval[i] = i > 0 ? lval[i - 1] : 0;

		if (rvalnum > 0) rval[i] = sumr;
		else if (lvalnum > 0) rval[i] = suml;
		else rval[i] = i > 0 ? rval[i - 1] : 0;

		if (svalnum > 0) sval[i] = sums;
		else sval[i] = i > 0 ? sval[i - 1] : 0;
	}
	QVector<double> lValues;
	double lastHasValue = 0;
	for (int i = 0; i < len; ++i)
	{
		if (qAbs(lval[i]) < 0.0001)
		{
			lValues.push_back(lastHasValue);
		}
		else
		{
			lValues.push_back(lval[i]);
			lastHasValue = lval[i];
		}

		m_roadSplietVec[i].setLeftRutValue(lValues[i]);
	}
	QVector<double> rValues;

	for (int i = 0; i < len; ++i)
	{
		if (qAbs(rval[i]) < 0.0001)
		{
			rValues.push_back(lastHasValue);
		}
		else
		{
			rValues.push_back(rval[i]);
			lastHasValue = rval[i];
		}

		m_roadSplietVec[i].setRightRutValue(rValues[i]);

		m_roadSplietVec[i].setMaxRutValue(qMax(lValues[i], rValues[i]));
		if (m_xrSetting->rutOutMode == 0)
			m_roadSplietVec[i].setjudgeRutValue(qMax(lValues[i], rValues[i]));
		if (m_xrSetting->rutOutMode == 1)
			m_roadSplietVec[i].setjudgeRutValue((lValues[i] + rValues[i]) / 2);
	}
	QVector<double> sValues;

	if (m_xrSetting->rutOutMode == 2)
	{
		for (int i = 0; i < len; ++i)
		{
			if (qAbs(sval[i]) < 0.0001)
			{
				sValues.push_back(lastHasValue);
			}
			else
			{
				sValues.push_back(sval[i]);
				lastHasValue = sval[i];
			}

			m_roadSplietVec[i].setjudgeRutValue(sValues[i]);
		}
	}

	return true;
}

bool hnOutExcelMileManage::writeMtdValue()
{
	bool hasValue = false;
	QString  resultPath = m_project->get2DProject()->getIRIPath();
	QString lPath = resultPath + "\\Laser0\\" + "MTD_" + QString::number(10) + "m.txt";
	QString rPath = resultPath + "\\Laser1\\" + "MTD_" + QString::number(10) + "m.txt";
	QString cPath = resultPath + "\\Laser2\\" + "MTD_" + QString::number(10) + "m.txt";
	QFile file(lPath);
	QStringList  lists;
	QVector<double >lValue;
	QVector<double >rValue;
	QVector<double >cValue;
	double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
	if (file.exists())
	{
		lists = MyCommonMethods::ReadAllLines(lPath);



		if (lists.size() * 10 < endDmi - endDmi / 5)
		{
			try
			{
				QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【构造深度MTD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//	throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

		}
		for (QString line : lists)
		{
			QStringList split = line.split(" ");
			if (split.size() > 1)
			{
				double value = split.at(1).toDouble();
				lValue.push_back(value);
				hasValue = true;
			}
		}

	}
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		file.setFileName(rPath);
		if (file.exists())
		{
			lists = MyCommonMethods::ReadAllLines(rPath);
			if (lists.size() * 10 < endDmi - endDmi / 5)
			{
				try
				{
					QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【右侧构造深度MTD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
					string mes = message.toLocal8Bit();
					reportExcelError(message);
					//	throw std::runtime_error(mes.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << e.what() << std::endl;
				}

			}

			for (QString line : lists)
			{
				QStringList split = line.split(" ");
				if (split.size() > 1)
				{
					hasValue = true;
					double value = split.at(1).toDouble();
					rValue.push_back(value);
				}
			}
		}



		if (m_project->get2DProject()->_IsMMTD)
		{
			file.setFileName(cPath);
			if (file.exists())
			{
				lists = MyCommonMethods::ReadAllLines(cPath);
				if (lists.size() * 10 < endDmi - endDmi / 5)
				{
					try
					{
						QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【中间构造深度MTD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
						string mes = message.toLocal8Bit();
						reportExcelError(message);
						//	throw std::runtime_error(mes.c_str());
					}
					catch (const std::exception& e)
					{
						std::cerr << e.what() << std::endl;
					}

				}

				for (QString line : lists)
				{
					QStringList split = line.split(" ");
					if (split.size() > 1)
					{
						hasValue = true;
						double value = split.at(1).toDouble();
						cValue.push_back(value);
					}
				}
			}
		}
	}
	if (!hasValue)
	{
		return false;
	}
	int len = m_roadSplietVec.size();
	QVector<double>  lval(len);
	QVector<double> rval(len);
	QVector<double> cval(len);
	QString LStrLine, RStrLine, CStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0, lastvalM = 0;
	double BaseLen = 10;
	for (int i = 0; i < len; i++)
	{
		double suml = 0, sumr = 0, sumc = 0;

		int lvalnum = 0, rvalnum = 0, cvalnum = 0;
		QStringList tmtd;
		startidx = MyCommonMethods::MathRoundToInt((m_roadSplietVec[i].getStartDmi() - 0.5) / BaseLen);
		endidx = MyCommonMethods::MathRoundToInt(m_roadSplietVec[i].getEndDmi() / BaseLen);
		if (startidx >= endidx)
		{
			if (startidx < lValue.size())
			{
				lastvalL = qAbs(lValue[startidx]);

				suml += lastvalL;
				++lvalnum;
			}
			if (m_project->get2DProject()->_IsDIRIMTD)
			{
				if (startidx < rValue.size())
				{
					lastvalR = qAbs(rValue[startidx]);

					sumr += lastvalR;
					++rvalnum;
				}
			}
			if (cValue.size() > 0 && startidx < cValue.size())
			{
				lastvalM = qAbs(cValue[startidx]);

				sumc += lastvalM;
				++cvalnum;
			}

		}
		else
		{
			for (ValStridx = startidx; ValStridx < endidx; ValStridx++)
			{
				if (ValStridx < lValue.size())
				{
					if (ValStridx >= lValue.size())
					{
						continue;
					}
					lastvalL = qAbs(lValue[ValStridx]);

					suml += lastvalL;
					++lvalnum;
				}
				if (m_project->get2DProject()->_IsDIRIMTD)
				{
					if (ValStridx < rValue.size())
					{
						lastvalR = qAbs(rValue[ValStridx]);
					}
					else
					{
						continue;
					}
					sumr += lastvalR;
					++rvalnum;
				}
				if (cValue.size() > 0 && startidx < cValue.size())
				{

					if (ValStridx >= cValue.size())
					{
						continue;
					}
					lastvalM = qAbs(cValue[ValStridx]);

					sumc += lastvalM;
					++cvalnum;
				}
			}
		}
		if (lvalnum > 0)
		{
			suml /= lvalnum;
		}
		if (rvalnum > 0)
		{
			sumr /= rvalnum;
		}
		if (cvalnum > 0)
		{
			sumc /= cvalnum;
		}

		if (lvalnum > 0)
		{
			lval[i] = suml;
		}
		else if (rvalnum > 0)
		{
			lval[i] = sumr;
		}
		else
		{
			lval[i] = i > 0 ? lval[i - 0] : 0;
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rvalnum > 0)
			{
				rval[i] = sumr;
			}
			else if (lvalnum > 0)
			{
				rval[i] = sumr;
			}
			else
			{
				rval[i] = i > 0 ? rval[i - 0] : 0;
			}
			if (cValue.size() > 0)
			{
				if (cvalnum > 0)
				{
					cval[i] = sumc;
				}
				else
				{
					cval[i] = i > 0 ? cval[i - 1] : 0;
				}
			}
		}
	}
	if (len >= 2)
	{
		if (lval[0] == 0)
		{
			lval[0] = lval[1];
		}
		if (lval[len - 1] == 0)
		{
			lval[len - 1] = lval[len - 2];
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rval[0] == 0)
			{
				rval[0] = rval[1];
			}
			if (rval[len - 1] == 0)
			{
				rval[len - 1] = rval[len - 2];
			}
			if (cValue.size() > 0)
			{
				if (cval[0] == 0)
				{
					cval[0] = cval[1];
				}
				if (cval[len - 1] == 0)
				{
					cval[len - 1] = cval[len - 2];
				}
			}

		}
	}

	QVector<double> lValues;
	double lastHasValue = 0;
	for (int i = 0; i < len; ++i)
	{
		if (qAbs(lval[i]) < 0.0001)
		{
			lValues.push_back(lastHasValue);
		}
		else
		{
			lValues.push_back(lval[i]);
			lastHasValue = lval[i];
		}

		m_roadSplietVec[i].setLeftMtdValue(lValues[i]);
	}
	QVector<double> rValues;
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		for (int i = 0; i < len; ++i)
		{
			if (qAbs(rval[i]) < 0.0001)
			{
				rValues.push_back(lastHasValue);
			}
			else
			{
				rValues.push_back(rval[i]);
				lastHasValue = rval[i];
			}

			m_roadSplietVec[i].setRightMtdValue(rValues[i]);
		}
	}
	QVector<double>cValues;
	if (m_project->get2DProject()->_IsMMTD)
	{
		if (cValue.size() > 0)
		{
			for (int i = 0; i < len; ++i)
			{
				if (qAbs(cval[i]) < 0.0001)
				{
					cValues.push_back(lastHasValue);
				}
				else
				{
					cValues.push_back(cval[i]);
					lastHasValue = cval[i];
				}
				m_roadSplietVec[i].setCenterMtdValue(cValues[i]);
			}

		}


	}

	return true;
}

bool hnOutExcelMileManage::writeMpdValue()
{
	bool hasValue = false;
	QString  resultPath = m_project->get2DProject()->getIRIPath();
	QString lPath = resultPath + "\\Laser0\\" + "MPD_" + QString::number(10) + "m.txt";
	QString rPath = resultPath + "\\Laser1\\" + "MPD_" + QString::number(10) + "m.txt";
	QString cPath = resultPath + "\\Laser2\\" + "MPD_" + QString::number(10) + "m.txt";
	QFile file(lPath);
	QStringList  lists;
	QVector<double >lValue;
	QVector<double >rValue;
	QVector<double >cValue;
	double endDmi = m_project->trueMileToEncl(m_project->getCurProSetInfo().dEndMile);
	if (file.exists())
	{
		lists = MyCommonMethods::ReadAllLines(lPath);



		if (lists.size() * 10 < endDmi - endDmi / 5)
		{
			try
			{
				QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【磨耗MPD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
				string mes = message.toLocal8Bit();
				reportExcelError(message);
				//	throw std::runtime_error(mes.c_str());
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

		}
		for (QString line : lists)
		{
			QStringList split = line.split(" ");
			if (split.size() > 1)
			{
				double value = split.at(1).toDouble();
				lValue.push_back(value);
				hasValue = true;
			}
			else
			{
				split = line.split("\t");
				if (split.size() > 1)
				{
					double value = split.at(1).toDouble();
					lValue.push_back(value);
					hasValue = true;
				}
			}
		}

	}
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		file.setFileName(rPath);
		if (file.exists())
		{
			lists = MyCommonMethods::ReadAllLines(rPath);
			if (lists.size() * 10 < endDmi - endDmi / 5)
			{
				try
				{
					QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【右侧磨耗MPD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
					string mes = message.toLocal8Bit();
					reportExcelError(message);
					//	throw std::runtime_error(mes.c_str());
				}
				catch (const std::exception& e)
				{
					std::cerr << e.what() << std::endl;
				}

			}

			for (QString line : lists)
			{
				QStringList split = line.split(" ");

				if (split.size() > 1)
				{
					hasValue = true;
					double value = split.at(1).toDouble();
					rValue.push_back(value);
				}
				else
				{
					split = line.split("\t");
					if (split.size() > 1)
					{
						hasValue = true;
						double value = split.at(1).toDouble();
						rValue.push_back(value);
					}
				}
			}
		}



		if (m_project->get2DProject()->_IsMMTD)
		{
			file.setFileName(cPath);
			if (file.exists())
			{
				lists = MyCommonMethods::ReadAllLines(cPath);
				if (lists.size() * 10 < endDmi - endDmi / 5)
				{
					try
					{
						QString message = m_project->get2DProName() + QStringLiteral("\r\n上次【计算IRM】中【中间磨耗MPD】计算到一半退出了软件\n请【清除结果——车辙】后重新【计算IRM】!");
						string mes = message.toLocal8Bit();
						reportExcelError(message);
						//	throw std::runtime_error(mes.c_str());
					}
					catch (const std::exception& e)
					{
						std::cerr << e.what() << std::endl;
					}

				}

				for (QString line : lists)
				{
					QStringList split = line.split("\t");
					if (split.size() > 1)
					{
						hasValue = true;
						double value = split.at(1).toDouble();
						cValue.push_back(value);
					}
					else
					{
						split = line.split(" ");
						if (split.size() > 1)
						{
							hasValue = true;
							double value = split.at(1).toDouble();
							cValue.push_back(value);
						}

					}
				}
			}
		}
	}
	if (!hasValue)
	{
		return false;
	}
	int len = m_roadSplietVec.size();
	//	std::unique_ptr<double[]> lval(new double[len]);
	//	std::unique_ptr<double[]>rval(new double[len]);
	QVector<double> cval(len);
	QVector<double> rval(len);
	QVector<double> lval(len);
	QString LStrLine, RStrLine, CStrLine;
	int startidx = 0, endidx = 0, ValStridx = 0;
	double lastvalL = 0, lastvalR = 0, lastvalM = 0;
	double BaseLen = 10;
	for (int i = 0; i < len; i++)
	{
		double suml = 0, sumr = 0, sumc = 0;

		int lvalnum = 0, rvalnum = 0, cvalnum = 0;
		QStringList tmtd;
		startidx = MyCommonMethods::MathRoundToInt((m_roadSplietVec[i].getStartDmi() - 0.5) / BaseLen);
		endidx = MyCommonMethods::MathRoundToInt(m_roadSplietVec[i].getEndDmi() / BaseLen);
		if (startidx >= endidx)
		{
			if (startidx < lValue.size())
			{
				lastvalL = qAbs(lValue[startidx]);

				suml += lastvalL;
				++lvalnum;
			}
			if (m_project->get2DProject()->_IsDIRIMTD)
			{
				if (startidx < rValue.size())
				{
					lastvalR = qAbs(rValue[startidx]);

					sumr += lastvalR;
					++rvalnum;
				}
			}
			if (cValue.size() > 0 && startidx < cValue.size())
			{
				lastvalM = qAbs(cValue[startidx]);

				sumc += lastvalM;
				++cvalnum;
			}

		}
		else
		{
			for (ValStridx = startidx; ValStridx < endidx; ValStridx++)
			{
				if (ValStridx < lValue.size())
				{
					lastvalL = qAbs(lValue[ValStridx]);
					suml += lastvalL;
					++lvalnum;
				}
				if (m_project->get2DProject()->_IsDIRIMTD)
				{
					if (ValStridx < rValue.size())
					{
						lastvalR = qAbs(rValue[ValStridx]);
					}
					sumr += lastvalR;
					++rvalnum;
				}
				if (cValue.size() > 0 && startidx < cValue.size())
				{
					lastvalM = qAbs(cValue[startidx]);

					sumc += lastvalM;
					++cvalnum;
				}
			}
		}
		if (lvalnum > 0)
		{
			suml /= lvalnum;
		}
		if (rvalnum > 0)
		{
			sumr /= rvalnum;
		}
		if (cvalnum > 0)
		{
			sumc /= cvalnum;
		}

		if (lvalnum > 0)
		{
			lval[i] = suml;
		}
		else if (rvalnum > 0)
		{
			lval[i] = sumr;
		}
		else
		{
			lval[i] = i > 0 ? lval[i - 0] : 0;
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rvalnum > 0)
			{
				rval[i] = sumr;
			}
			else if (lvalnum > 0)
			{
				rval[i] = sumr;
			}
			else
			{
				rval[i] = i > 0 ? rval[i - 0] : 0;
			}
			if (cValue.size() > 0)
			{
				if (cvalnum > 0)
				{
					cval[i] = sumc;
				}
				else
				{
					cval[i] = i > 0 ? cval[i - 1] : 0;
				}
			}
		}
	}
	if (len >= 2)
	{
		if (lval[0] == 0)
		{
			lval[0] = lval[1];
		}
		if (lval[len - 1] == 0)
		{
			lval[len - 1] = lval[len - 2];
		}
		if (m_project->get2DProject()->_IsDIRIMTD)
		{
			if (rval[0] == 0)
			{
				rval[0] = rval[1];
			}
			if (rval[len - 1] == 0)
			{
				rval[len - 1] = rval[len - 2];
			}
			if (cValue.size() > 0)
			{
				if (cval[0] == 0)
				{
					cval[0] = cval[1];
				}
				if (cval[len - 1] == 0)
				{
					cval[len - 1] = cval[len - 2];
				}
			}

		}
	}

	QVector<double> lValues;
	double lastHasValue = 0;
	for (int i = 0; i < len; ++i)
	{
		if (qAbs(lval[i]) < 0.0001)
		{
			lValues.push_back(lastHasValue);
		}
		else
		{
			lValues.push_back(lval[i]);
			lastHasValue = lval[i];
		}

		m_roadSplietVec[i].setLeftMpdValue(lValues[i]);
	}
	QVector<double> rValues;
	if (m_project->get2DProject()->_IsDIRIMTD)
	{
		for (int i = 0; i < len; ++i)
		{
			if (qAbs(rval[i]) < 0.0001)
			{
				rValues.push_back(lastHasValue);
			}
			else
			{
				rValues.push_back(rval[i]);
				lastHasValue = rval[i];
			}

			m_roadSplietVec[i].setRightMpdValue(rValues[i]);
		}
	}
	QVector<double>cValues;
	if (m_project->get2DProject()->_IsMMTD)
	{
		if (cValue.size() > 0)
		{
			for (int i = 0; i < len; ++i)
			{
				if (qAbs(cval[i]) < 0.0001)
				{
					cValues.push_back(lastHasValue);
				}
				else
				{
					cValues.push_back(cval[i]);
					lastHasValue = cval[i];
				}

				m_roadSplietVec[i].setCenterMpdValue(cValues[i]);
			}

		}

	}

	return true;
}

bool hnOutExcelMileManage::writeGpsStrValue()
{
	QString gps2MileFilePath = m_project->get2DProject()->getBasePath() + "\\GPS2Mile.txt";
	QStringList datas = MyCommonMethods::ReadAllLines(gps2MileFilePath);

	if (datas.size() <= 0)
	{
		QString message = m_project->get2DProName() + QStringLiteral("\r\n是否未进行GPS桩号匹配或执行失败!");
		string mes = message.toLocal8Bit();
		reportExcelError(message);
		return false;
	}

	QVector<_EXCELGPS_> gpsInfos;
	for (int i = 0; i < datas.size(); ++i)
	{
		gpsInfos.push_back(_EXCELGPS_(datas[i]));
	}

	int gi = 0;
	int len = m_roadSplietVec.size();
	int direction = m_project->get2DProject()->_Direction;

	fillRoadSplitVec(gpsInfos, m_roadSplietVec, direction);
	return true;


}

void hnOutExcelMileManage::initMarkMehtodExcelMile(hnOutExcelMile& newMile, const hnOutExcelMile& mile)
{
	//newMile.RoadLength = qAbs(newMile.getStartMile() - newMile.getEndMile());
	newMile.RoadDegreestr = mile.RoadDegreestr;
	newMile.RoadSurface = mile.RoadSurface;
	newMile.RoadSurfaceStr = mile.RoadSurfaceStr;
	newMile.setUnitStr(mile.getUnitStr());
	newMile.RoadGrad = mile.RoadGrad;
	newMile.setSurveyWidth(mile.getSurveyWidth());

}

_EXCELGPS_ hnOutExcelMileManage::findNearestGps(const QVector<_EXCELGPS_>& gpsInfos, double targetMile, int line)
{
	if (gpsInfos.isEmpty())
	{
		return nullptr;
	}
	//查找第一个大于或者等于目标桩号的位置
	auto it = std::lower_bound(gpsInfos.begin(), gpsInfos.end(), targetMile, [line](const _EXCELGPS_& gps, double mile) {

		if (line == 1)
		{
			return gps._mile < mile;
		}
		else
		{
			return gps._mile > mile;
		}

		});

	//处理边界清空
	if (it == gpsInfos.begin())
	{
		return (*it);
	}
	if (it == gpsInfos.end())
	{
		return (*(it - 1));
	}

	//比较it和it-1 找到最接近的点
	const _EXCELGPS_& nextGps = *it;
	const _EXCELGPS_& prevGps = *(it - 1);

	if (std::abs(nextGps._mile - targetMile) < std::abs(prevGps._mile - targetMile))
	{
		return nextGps;
	}
	else
	{
		return prevGps;
	}
}

void hnOutExcelMileManage::fillRoadSplitVec(QVector<_EXCELGPS_>gpsInfos, QVector<hnOutExcelMile>& roadSplitVec, int line)
{

	for (auto& roadSplit : roadSplitVec)
	{
		//查找起点最近的gps信息
		_EXCELGPS_ sGps = findNearestGps(gpsInfos, m_project->enclToTrueMile(roadSplit.getStartDmi()), line);

		{
			roadSplit.setStartGpsInfo(sGps);
		}
		//查找起点最近的gps信息
		_EXCELGPS_ eGps = findNearestGps(gpsInfos, m_project->enclToTrueMile(roadSplit.getEndDmi()), line);

		{
			roadSplit.setEndGpsInfo(eGps);
		}
	}

}

double hnOutExcelMileManage::getCloseMile(const double& value)
{

	if (m_roadSplietVec.isEmpty())
	{
		return 0;
	}

	auto it = std::lower_bound(m_roadSplietVec.begin(), m_roadSplietVec.end(), value, [](const hnOutExcelMile& mile, double val)
		{
			return mile.getStartDmi() < val;

		});
	if (it == m_roadSplietVec.begin())
	{
		return -1;
	}
	else if (it == m_roadSplietVec.end())
	{
		return  m_roadSplietVec.end()->getEndDmi();
	}
	else
	{
		auto closeMile = (*(it - 1));
		double distance = value - closeMile.getStartDmi();
		return (distance * m_direction) + closeMile.getStartMile();
	}

}
