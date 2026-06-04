#include "hnOutExcelManage.h"
#include <QApplication>
#include <QEventLoop>
#include <QMessageBox> 
#include "..\hnCommon\hnRoadStruct.h" 
#include <QFileInfo>  
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QDir>
#include "hnDiseaseSumAreaCaculate.h" 
#include "hnXlsxInterface.h"
#include "..\hnQtCommon\MyCommonMethods.h"
#include <QPainter> 
#include "hnOutExcelMile.h"
#include "..\HighAccConvertPlane\HighAccuracyPositioning.h"
#include "..\hnApplication\hnDiseaseService.h"
namespace
{
	const QString kReportProjectSection = QStringLiteral("二三维设置信息");
	const QString kReportRoadWidthKey = QStringLiteral("报表路面宽度");

	QString reportProjectInfoPath(hnPro::hnProject* project)
	{
		if (!project)
		{
			return QString();
		}
		if (project->get2DProject())
		{
			return QDir::toNativeSeparators(project->get2DProject()->getBasePath() + QStringLiteral("/ProjectInfo.txt"));
		}
		QString projectPath = project->getAbsulotelyPath();
		if (projectPath.isEmpty())
		{
			return QString();
		}
		return QDir::toNativeSeparators(projectPath + QStringLiteral("/ProjectInfo.txt"));
	}

	int keyValueSeparatorIndex(const QString& line)
	{
		int halfIndex = line.indexOf(':');
		int fullIndex = line.indexOf(QStringLiteral("："));
		if (halfIndex < 0)
		{
			return fullIndex;
		}
		if (fullIndex < 0)
		{
			return halfIndex;
		}
		return qMin(halfIndex, fullIndex);
	}

	double readReportRoadWidth(hnPro::hnProject* project)
	{
		if (!project)
		{
			return 0;
		}

		double defaultRoadWidth = project->getCurProSetInfo().dRoadWidth;
		QFile file(reportProjectInfoPath(project));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			return defaultRoadWidth;
		}

		QTextStream in(&file);
		in.setCodec(QTextCodec::codecForName("UTF-8"));
		QString currentSection;
		while (!in.atEnd())
		{
			QString line = in.readLine().trimmed();
			if (line.startsWith('[') && line.endsWith(']'))
			{
				currentSection = line.mid(1, line.length() - 2).trimmed();
				continue;
			}
			if (currentSection != kReportProjectSection)
			{
				continue;
			}

			int sepIndex = keyValueSeparatorIndex(line);
			if (sepIndex <= 0)
			{
				continue;
			}
			QString key = line.left(sepIndex).trimmed();
			if (key != kReportRoadWidthKey && key != QStringLiteral("检测路面宽度"))
			{
				continue;
			}
			bool ok = false;
			double reportRoadWidth = line.mid(sepIndex + 1).trimmed().toDouble(&ok);
			if (ok && reportRoadWidth > 0)
			{
				return reportRoadWidth;
			}
		}
		return defaultRoadWidth;
	}
}
hnOutExcelManage::hnOutExcelManage()
{
	 
}

void hnOutExcelManage::OutExcelManager(const QString& excelDir, 
	const QString&selectModelTxt,
	HnProjectEnums::StandardParmTypeEnum standard, 
	hnCommon::ROAD_WORK_TYPE DrawType, 
	ReportItem*reportItem, 
	hnPro::hnProject*curProject,
	double sMile, double eMile,
	int & progressValue,
	QProgressDialog* process,
	int progressScale
)
{  
	 
	if (sMile==0 && eMile==0)
	{
		sMile = curProject->getCurProSetInfo().dBegMile;
		eMile = curProject->getCurProSetInfo().dEndMile;
	}
		if (!reportItem->isChecked || !reportItem->isShow)
		{
			return;
		}
		QString segment = reportItem->segments[reportItem->selectSegmentIndex];
		QStringList segmentList = segment.split(',');

		for each (QString  splitValueStr in segmentList)
		{
			double splitValue = splitValueStr.toDouble();
			if (process)
			{
				process->setLabelText(QStringLiteral("正在生成报表：%1  分段：%2m").arg(reportItem->displayName).arg(splitValue));
				process->setValue(progressValue * progressScale + qMax(1, progressScale / 4));
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}

			switch (standard)
			{
				
			case HnProjectEnums::DegreeRoad2018:
			{ 
				switch (DrawType)
				{
				case hnCommon::ROAD_WORK_LARGE_RECT:
					outBigRectExcel2018(excelDir, selectModelTxt, reportItem->index, splitValue, curProject,sMile,eMile);

					break;
				case hnCommon::ROAD_WORK_SMALL_RECT:
					outExcelSmallRectDegreeRoad2018(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				case hnCommon::DESIGN:
					outDesignExcel2018(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				default:
					break;
				} 
			}
			break;
			case HnProjectEnums::CityRoad:
				switch (DrawType)
				{
				case hnCommon::ROAD_WORK_LARGE_RECT:
					outExcelCityRoad(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				case hnCommon::ROAD_WORK_SMALL_RECT:
					outExcelSmallRectCityRoad(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				case hnCommon::DESIGN:
					outExcelDesignCityRoad(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				default:
					break;
				}
				break;
			case HnProjectEnums::RuralRoadlowLevel:
			{
				switch (DrawType)
				{
				case hnCommon::ROAD_WORK_LARGE_RECT:
					outBigRectExcelRuralRoadlowLevelRoad(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				case hnCommon::ROAD_WORK_SMALL_RECT:
					ouSmallRectlRuralRoadlowLevelRoad(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				case hnCommon::DESIGN:
					outDesignExcel2018(excelDir, selectModelTxt, reportItem->index, splitValue, curProject, sMile, eMile);

					break;
				default:
					break;
				} 
				break;
			} 
			default:
				break; 
			}	
		
		
			if (process)
			{ 
				progressValue++;
				process->setValue(progressValue * progressScale);
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
		} 
}

	void hnOutExcelManage::OutExcelManager_Street(
		const QString& excelDir,
		int key,
		const QVector<double>& splits,
		HnProjectEnums::StandardParmTypeEnum standard,
		hnCommon::ROAD_WORK_TYPE DrawType,
		hnPro::hnProject*curProject,
		double sMile,
		double eMile ,
		int & progressValue,
		QProgressDialog* process,
		int progressScale
	)
	{
		PROJECT_TYPE projectType = curProject->getProjectType();
		for each (double  splitValue in splits)
		{
			if (process)
			{
				process->setLabelText(QStringLiteral("正在生成景观报表：%1  分段：%2m").arg(key).arg(splitValue));
				process->setValue(progressValue * progressScale + qMax(1, progressScale / 4));
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
			switch (standard)
			{
			case HnProjectEnums::DegreeRoad2018:

			{
				if (projectType == PROJECT_TYPE::PROJECT_JD_3D_TYPE || projectType == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
				{
				}
				else
				{
					outExcelStreetDegreeRoad2018(excelDir, key, splitValue, curProject,sMile,eMile);
				}
				break;
			}
			case HnProjectEnums::RuralRoadlowLevel:

				if (projectType == PROJECT_TYPE::PROJECT_JD_3D_TYPE || projectType == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
				{
				}
				else
				{
					outExcelStreetRuralRoadlowLevelRoad(excelDir, key, splitValue, curProject,sMile,eMile);
				}
				break;
			default:
				break;
			}
			if (process)
			{
				progressValue++;
				process->setValue(progressValue * progressScale);
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
		}


	}

	 
bool hnOutExcelManage::initSegmentInterval(
	hnPro::hnProject * project,
	 const double lenth, 
	const MyQtCommon::MyEquipment& equip, 
	double sMile,
	double eMile)
{
	HnProjectEnums::StandardParmTypeEnum standard = project->getBaseStandard();
	m_outExcelMileManage = QSharedPointer<hnOutExcelMileManage>
		(new hnOutExcelMileManage(standard, project, sMile, eMile, lenth, equip, readReportRoadWidth(project)));
	
	if (!m_outExcelMileManage->getDataComplete())
	{
		QApplication::restoreOverrideCursor();
		return false;
		//QMessageBox::about(, QStringLiteral("错误"), curProject->get2DProName() + QStringLiteral("_存在指标未进行计算，请根据提示进行IRM计算!"));
	}
	return true;
}

void hnOutExcelManage::outBigRectExcel2018(const QString& saveExcelDir,const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile )
{
	PROJECT_TYPE projectType =  curProject->getProjectType();
	QString drawType = QStringLiteral("人工模式");
	QString standardName = QStringLiteral("等级公路 JTG H20-2018");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";
	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{

		switch (key)
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true; 
				if (m_xrSetting->outSpeedAndMarkExcel)
				{
					setEquip.SPEED = true;

				}
				/*
				HnProjectEnums::StandardParmTypeEnum standard, hnPro::hnProject * project,
				double sMile, double eMile,
				*/
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportCZSDPJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面磨耗评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				setEquip.MPD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMMHPJDJB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 3:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel(saveExcelDir,modeleBasePath, xlslen, curProject);


		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				
				exportLMBHMJTJB_RECT(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir,modeleBasePath, xlslen, curProject, sMile,eMile);
			}
		}
		break;
		case 5:		//路面跳车等级评价统计表  //有问题
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.JUMP = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMTCPJDJJLB(saveExcelDir,modeleBasePath, xlslen, setEquip,curProject);
			}
		}
		break;
		case 6:		//CPMS路面调查统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportCPMS_LMBHDCB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 7:		//路面综合评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 8:		//技术状况评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true; 
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true; 
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportJSZKPDMXB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;

		case 9:	//路面构造深度SMTD评价等级统计表  
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 10:	//路面构造深度MPD评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 11: //路面几何状况检测数据统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.JHXX = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMJHZKJCSJTJB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 12:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		
			break;
		default:
			break;
		}
	}
}

void hnOutExcelManage::outExcelSmallRectDegreeRoad2018(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	QString drawType = QStringLiteral("自动化模式");
	QString standardName = QStringLiteral("等级公路 JTG H20-2018");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";

	//获取double/float类型小数点后保留的位数
	//m_decimalDigits = m_xrSetting->sheetRoundingOffNum;

	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{
		switch (key )
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportCZSDPJDJJLB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面磨耗评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMMHPJDJB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 3:		//路面破损评价等级记录表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_Smart(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_Smart(saveExcelDir,modeleBasePath, xlslen, curProject,sMile,eMile);
			}

		}
		break;
		case 5:		//路面跳车等级评价统计表  //有问题
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.JUMP = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMTCPJDJJLB(saveExcelDir,modeleBasePath, xlslen, setEquip,curProject);
			}

		}
		break;
		case 6:		//CPMS路面调查统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportCPMS_LMBHDCB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 7:		//路面综合评价等级表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case 8:		//技术状况评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportJSZKPDMXB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;

		case 9:	//路面构造深度SMTD评价等级统计表  
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 10:	//路面构造深度MPD评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 11: //路面几何状况检测数据统计表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.JHXX = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMJHZKJCSJTJB(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		case  12:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			
		}
		break;

		default:
			break;
		}
	}
	else if (selectModelTxt == QStringLiteral("贵州客户定制"))
	{
		switch (key)
		{
		case 0:
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel_GZQT(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
			break;
		case 1:
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.JUMP = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				if (xlslen != 10)
				{
					break;
				}
				exportLMTCPJDJJLB_GZQT(saveExcelDir,modeleBasePath, xlslen, setEquip,curProject);
			}
			break;
		case 2:
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMMHPJDJB_GZQT(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

			break;

		case 3:
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportCZSDPJDJJLB_GZQT(saveExcelDir,modeleBasePath, xlslen,curProject);
			}
			break;

		case 4:
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_Smart_GZQT(saveExcelDir,modeleBasePath, xlslen, curProject);
			}
			else
			{
				/*MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_Smart(modeleBasePath, xlslen);*/
			}
			break;
		case 5:
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel_GZQT(saveExcelDir,modeleBasePath, xlslen, curProject);
		}
		break;
		}
	}
}

void hnOutExcelManage::outDesignExcel2018(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	 
	QString drawType = QStringLiteral("设计模式");
	QString standardName = QStringLiteral("等级公路 JTG H20-2018");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";

	//获取double/float类型小数点后保留的位数
	//m_decimalDigits = m_xrSetting->sheetRoundingOffNum;

	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{
		switch (key)
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportCZSDPJDJJLB(saveExcelDir,modeleBasePath, xlslen,curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面磨耗评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				setEquip.MPD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMMHPJDJB(saveExcelDir,modeleBasePath, xlslen, curProject);
			}

		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 3:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel(saveExcelDir, modeleBasePath, xlslen, curProject);


		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject,sMile,eMile);
			}
		}
		break;
		case 5:		//路面跳车等级评价统计表  //有问题
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.JUMP = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMTCPJDJJLB(saveExcelDir, modeleBasePath, xlslen, setEquip,curProject);
			}
			else
			{

			}
		}
		break;
		case 6:		//CPMS路面调查统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportCPMS_LMBHDCB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 7:		//路面综合评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 8:		//技术状况评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.RUT = true;
			setEquip.IRI = true;
			setEquip.JUMP = true;
			setEquip.MPD = true;
			setEquip.JHXX = true;
			setEquip.SMTD = true; 
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportJSZKPDMXB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;

		case 9:	//路面构造深度SMTD评价等级统计表  
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB(saveExcelDir, modeleBasePath, xlslen,curProject);
			}

		}
		break;
		case 10:	//路面构造深度MPD评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 11: //路面几何状况检测数据统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.JHXX = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMJHZKJCSJTJB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case  12:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}
	else if (selectModelTxt == QStringLiteral("定制报表"))
	{
		switch (key)
		{
		case  0:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exporDesignSnDiseaseSum(saveExcelDir, modeleBasePath, xlslen, curProject);
			break;
		}

		default:
			break;
		}

	}
}

void hnOutExcelManage::outExcelCityRoad(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	QString drawType = QStringLiteral("人工模式");
	QString standardName = QStringLiteral("城镇道路");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";

	//获取double/float类型小数点后保留的位数
	//m_decimalDigits = m_xrSetting->sheetRoundingOffNum;

	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{
		switch (key)
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportCZSDPJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面构造深度评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB_City(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 3:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			if (m_xrSetting->outSpeedAndMarkExcel)
			{
				setEquip.SPEED = true;
			}
			setEquip.ROAD = true;
			
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel_City(saveExcelDir, modeleBasePath, xlslen, curProject);


		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen,curProject, sMile,eMile);
			}
		}
		break;
		//case 5:		//路面综合评价等级表
		//{
		//	MyQtCommon::MyEquipment setEquip;
		//	setEquip.ROAD = true;
		//	setEquip.RUT = true;
		//	setEquip.IRI = true;
		//	setEquip.JUMP = true;
		//	setEquip.MPD = true;
		//	setEquip.JHXX = true;
		//	setEquip.SMTD = true;
		//	setEquip.SPEED = true;
		//	initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		//	exportLMZHexcel(modeleBasePath, xlslen);
		//}
		//break;  
		case 5:	//路面构造深度MPD评价等级统计表 磨耗
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case  6:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}

}

void hnOutExcelManage::outExcelSmallRectCityRoad(const QString& saveExcelDir, const QString& selectModelTxt,int key, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	QString drawType = QStringLiteral("自动化模式");
	QString standardName = QStringLiteral("城镇道路");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";

	//获取double/float类型小数点后保留的位数
	//m_decimalDigits = m_xrSetting->sheetRoundingOffNum;

	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{
		switch (key)
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportCZSDPJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面构造深度评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB_City(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 3:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel_City(saveExcelDir, modeleBasePath, xlslen, curProject);


		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject,sMile,eMile);
			}
		}
		break;
		//case 5:		//路面综合评价等级表
		//{
		//	MyQtCommon::MyEquipment setEquip;
		//	setEquip.ROAD = true;
		//	setEquip.RUT = true;
		//	setEquip.IRI = true;
		//	setEquip.JUMP = true;
		//	setEquip.MPD = true;
		//	setEquip.JHXX = true;
		//	setEquip.SMTD = true;
		//	setEquip.SPEED = true;
		//	initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		//	exportLMZHexcel(modeleBasePath, xlslen);
		//}
		//break;  
		case 5:	//路面构造深度MPD评价等级统计表 磨耗
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break; 
		case  6:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}
}


void hnOutExcelManage::outExcelDesignCityRoad(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen,  hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	QString drawType = QStringLiteral("设计模式");
	QString standardName = QStringLiteral("城镇道路");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";

	//获取double/float类型小数点后保留的位数
	//m_decimalDigits = m_xrSetting->sheetRoundingOffNum;

	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{
		switch (key)
		{
		case 0:		//车辙深度评价登记记录表
		{
			if (curProject->get2DProject()->_IsRut)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.RUT = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportCZSDPJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//车辙数据异常
			//汇总文字待输出  20230619
		}
		break;

		case 1:		//路面构造深度评价等级统计表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.SMTD = true;  //磨耗
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

				exportLMGZSDPJDJJLB_City(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case 2:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 3:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel_City(saveExcelDir, modeleBasePath, xlslen, curProject);


		}
		break;
		case 4:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject,sMile,eMile);
			}
		}
		break;
		//case 5:		//路面综合评价等级表
		//{
		//	MyQtCommon::MyEquipment setEquip;
		//	setEquip.ROAD = true;
		//	setEquip.RUT = true;
		//	setEquip.IRI = true;
		//	setEquip.JUMP = true;
		//	setEquip.MPD = true;
		//	setEquip.JHXX = true;
		//	setEquip.SMTD = true;
		//	setEquip.SPEED = true;
		//	initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		//	exportLMZHexcel(modeleBasePath, xlslen);
		//}
		//break;  
		case 5:	//路面构造深度MPD评价等级统计表 磨耗
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.MPD = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportGZSD_MPD_PJDJJLB(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
		}
		break;
		case  6:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}
}

void hnOutExcelManage::outBigRectExcelRuralRoadlowLevelRoad(const QString& saveExcelDir, const QString&selectModelTxt,int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();
	QString drawType = QStringLiteral("人工模式");
	QString standardName = QStringLiteral("低等级农村公路");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";
	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{

		switch (key)
		{

		case 0:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				if (m_xrSetting->outSpeedAndMarkExcel)
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 1:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true; 
			if (m_xrSetting->outSpeedAndMarkExcel)
			{
				setEquip.SPEED = true;
			}
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 2:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject,sMile,eMile);
			}
		}
		break;

		case 3:		//CPMS路面调查统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportCPMS_LMBHDCB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 4:		//路面综合评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.STREET = true;
			setEquip.IRI = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 5:		//技术状况评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.IRI = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportJSZKPDMXB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case  6:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}
}

void hnOutExcelManage::ouSmallRectlRuralRoadlowLevelRoad(const QString& saveExcelDir, const QString&selectModelTxt, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	PROJECT_TYPE projectType = curProject->getProjectType();

	QString drawType = QStringLiteral("自动化模式");
	QString standardName = QStringLiteral("低等级农村公路");
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + drawType + "//" + selectModelTxt + "//";
	if (selectModelTxt == QStringLiteral("单项指标出表"))
	{

		switch (key)
		{

		case 0:		//路面平整度评价等级记录表
		{
			if (curProject->get2DProject()->_IsIRIMTD)
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.IRI = true;
				setEquip.SPEED = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportIRIexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
			}

			//平整度数据待验证
			//汇总文字
		}
		break;
		case 1:		//路面破损评价等级记录表
		{

			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			{
				setEquip.SPEED = true;
			} 
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);

			exportLMPSexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 2:		//路面病害面积统计表
		{
			if (projectType == PROJECT_TYPE::PROJECT_2D_TYPE || projectType == PROJECT_TYPE::PROJECT_23D_TYPE)
			{

				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				exportLMBHMJTJB_Smart(saveExcelDir, modeleBasePath, xlslen, curProject);
			}
			else
			{
				MyQtCommon::MyEquipment setEquip;
				setEquip.ROAD = true;
				initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
				export3DLMBHMJTJB_RECT(saveExcelDir, modeleBasePath, xlslen, curProject,sMile,eMile);
			}
		}
		break;

		case 3:		//CPMS路面调查统计表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportCPMS_LMBHDCB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 4:		//路面综合评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.IRI = true;
			if (m_xrSetting->outSpeedAndMarkExcel)
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportLMZHexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case 5:		//技术状况评价等级表
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.ROAD = true;
			setEquip.STREET = true;
			setEquip.IRI = true;
			setEquip.SPEED = true;
			initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
			exportJSZKPDMXB(saveExcelDir, modeleBasePath, xlslen, curProject);
		}
		break;
		case  6:
		{
			MyQtCommon::MyEquipment setEquip;
			setEquip.GPS = true;
			initSegmentInterval(curProject, xlslen, setEquip, sMile, eMile);
			exportGPSExcel(saveExcelDir, modeleBasePath, xlslen, curProject);

		}
		break;
		default:
			break;
		}
	}
}

bool hnOutExcelManage::exportProjectInfoSheet(Document &xlsx, hnPro::hnProject*curProject)
{
	if (!xlsx.selectSheet(QString::fromLocal8Bit("工程信息")))
	{
		return false;
	}

	//获取样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat;

	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//省

	QString province = QString::fromLocal8Bit(projectInfo.strProvince);
	contextFormat = xlsx.cellAt("B2")->format();
	xlsx.write(QString("B2"), province, contextFormat);
	//市
	QString city = QString::fromLocal8Bit(projectInfo.strCity);
	contextFormat = xlsx.cellAt("B3")->format();
	xlsx.write(QString("B3"), city, contextFormat);
	//县
	QString county = QString::fromLocal8Bit(projectInfo.strCounty);
	contextFormat = xlsx.cellAt("B4")->format();
	xlsx.write(QString("B4"), county, contextFormat);
	//道路编号
	QString roadNum = QString::fromLocal8Bit(projectInfo.strNumber);
	contextFormat = xlsx.cellAt("B5")->format();
	xlsx.write(QString("B5"), roadNum, contextFormat);
	//道路名称
	QString roadName = QString::fromLocal8Bit(projectInfo.strRoadName);
	contextFormat = xlsx.cellAt("B6")->format();
	xlsx.write(QString("B6"), roadName, contextFormat);
	//起点桩号

	contextFormat = xlsx.cellAt("B7")->format();
	xlsx.write(QString("B7"), m_outExcelMileManage->getStartMile(), contextFormat);
	//行车方向
	QString direction = projectInfo.nLineType == 1 ? QString::fromLocal8Bit("上行") : QString::fromLocal8Bit("下行");
	contextFormat = xlsx.cellAt("B8")->format();
	xlsx.write(QString("B8"), direction, contextFormat);
	//公路等级
	QString roadLevel = QString::fromLocal8Bit(projectInfo.strRoadLevel);
	contextFormat = xlsx.cellAt("B9")->format();
	xlsx.write(QString("B9"), roadLevel, contextFormat);
	//车道
	QString lane = QString::fromLocal8Bit(projectInfo.strRoadNO);
	contextFormat = xlsx.cellAt("B10")->format();
	xlsx.write(QString("B10"), lane, contextFormat);
	//采集日期 
	contextFormat = xlsx.cellAt("B11")->format();
	xlsx.write(QString("B11"), projectInfo.strDate, contextFormat);
	//工程开始时刻

	contextFormat = xlsx.cellAt("B12")->format();
	xlsx.write(QString("B12"), projectInfo.strTimer, contextFormat);
	//检测员
	QString detectPeople = QString::fromLocal8Bit(projectInfo.strSurveyor);
	contextFormat = xlsx.cellAt("B13")->format();
	xlsx.write(QString("B13"), detectPeople, contextFormat);
	//检测天气
	QString wheather = QString::fromLocal8Bit(projectInfo.strWeather);
	contextFormat = xlsx.cellAt("B14")->format();
	xlsx.write(QString("B14"), wheather, contextFormat);
	//路面材质 
	QString marksPath = curProject->get2DProject()->getFullRoadTypeMarkFilePath();
	QStringList marks = MyCommonMethods::ReadAllLines(marksPath, "utf-8");
	QString roadType;
	for (int i = 0; i < marks.size(); ++i)
	{
		roadType += marks[i] + "\n";
	}
	//xlsx.setRowHeight(15, 30 * marks.size());

	contextFormat = xlsx.cellAt("B15")->format();
	contextFormat.setTextWrap(true);
	xlsx.write(QString("B15"), roadType, contextFormat);
	//终点桩号

	contextFormat = xlsx.cellAt("B16")->format();
	xlsx.write(QString("B16"), m_outExcelMileManage->getEndMile(), contextFormat);
	//检测里程（km）

	contextFormat = xlsx.cellAt("B17")->format();
	xlsx.write(QString("B17"), qAbs(m_outExcelMileManage->getStartMile() - m_outExcelMileManage->getEndMile()) * 0.001, contextFormat);

	xlsx.write(QString("A18"), QStringLiteral("路面宽度（m）"), contextFormat);
	xlsx.write(QString("B18"), readReportRoadWidth(curProject), contextFormat);
	QString standard = HnProjectEnums::roadTypeEnumToQString_ForExcel(curProject->getBaseStandard());

	xlsx.write(QString("A19"), QStringLiteral("道路规范"), contextFormat);
	xlsx.write(QString("B19"), standard, contextFormat);
	const char * drawType = hnCommon::workTypeToQString(curProject->getBaseDrawType());
	xlsx.write(QString("A20"), QStringLiteral("绘制模式"), contextFormat);
	QString drawTypeStr = QString::fromLocal8Bit(drawType);
	xlsx.write(QString("B20"), drawTypeStr, contextFormat);

	return true;
}

bool hnOutExcelManage::exportIRIexcel_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("IRI.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet(QStringLiteral("十米RQI")))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始

	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	int forMatRowIndex = 4;
	for (int i = 0; i < excelMiles.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 4;
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		//车道	
		format = xlsx.cellAt("C4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");

		xlsx.writeAndFormat(rowIndex, colCount++, projectInfo.strNumber, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, sMile, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, eMile, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, RoadNum, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftIriValue(), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightIriValue(), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getMaxIriValue(), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getIriExcelStr(), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getIriEvaluateStr("H", rowIndex), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, roadType, forMatRowIndex);
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getSpeed(), forMatRowIndex);
			//备注
			auto markFormat = xlsx.cellAt("K4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.writeAndFormat(rowIndex, colCount++, mark, forMatRowIndex);
		}
	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);

	if (!saveExcel(saveExcelDir,tableName,xlsx))
	{
		return false;
	} 
	return true;
}

bool hnOutExcelManage::exportLMZHexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	//报表模板/等级公路2018/人工模式/
	QString tableName = QString::fromLocal8Bit("路面综合评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	//给表名加上米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}


	Format contextFormat = xlsxInterface.getContentFormat();
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{

		auto nowExcelMile = excelMiles.at(i);

		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A3")->format());
		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B3")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, xlsx.cellAt("C3")->format());
		//车道	
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D3")->format());
		//PCI	
		QString pci = nowExcelMile.getPCIExcelStr();
		xlsx.write(QString("E%1").arg(rowCount), pci, xlsx.cellAt("E3")->format());
		//评价等级
		//E3就是pci对应的单元格 
		xlsx.write(QString("F%1").arg(rowCount), nowExcelMile.getPciEvaluateStr("E", rowCount), xlsx.cellAt("F3")->format());
		//RQI 
		QString IriExcelValue = nowExcelMile.getIriExcelStr();
		xlsx.write(QString("G%1").arg(rowCount), IriExcelValue, xlsx.cellAt("G3")->format());
		//评价等级

		QString grade = nowExcelMile.getIriEvaluateStr("G", rowCount);
		xlsx.write(QString("H%1").arg(rowCount), grade, xlsx.cellAt("H3")->format());
		//RDI 
		//评价等级

		xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getRutExcelStr(), xlsx.cellAt("I3")->format());
		grade = nowExcelMile.getRutEvaluateStr("I", rowCount);
		xlsx.write(QString("J%1").arg(rowCount), grade, xlsx.cellAt("J3")->format());

		//PBI
		QString pbi = nowExcelMile.getPBIScore();
		xlsx.write(QString("K%1").arg(rowCount), pbi, xlsx.cellAt("K3")->format());
		//PBI评价 
		xlsx.write(QString("L%1").arg(rowCount), nowExcelMile.getPbiEvaluateStr("K", rowCount), xlsx.cellAt("L3")->format());

		//PWI
		double p2i = nowExcelMile.getPwiValue();
		xlsx.write(QString("M%1").arg(rowCount), p2i, xlsx.cellAt("M3")->format());
		//PBI评价 
		xlsx.write(QString("N%1").arg(rowCount), nowExcelMile.getPwiEvaluateStr(), xlsx.cellAt("N3")->format());
		//PWI评价 
		QString pqiStr = nowExcelMile.getPqiValue(rowCount, "E", "G", "I", "K", "M");
		//PQI
		xlsx.write(QString("O%1").arg(rowCount), pqiStr, xlsx.cellAt("O3")->format());


		QString pqiEvaluateStr = nowExcelMile.getPqiEvaluateStr(rowCount, "O");
		xlsx.write(QString("P%1").arg(rowCount), pqiEvaluateStr, xlsx.cellAt("P3")->format());


		//DR(%)	
		double dr = nowExcelMile.getDRScore();

		//路面材质	
		QString roadType = nowExcelMile.RoadSurfaceStr;
		xlsx.write(QString("Q%1").arg(rowCount), roadType, contextFormat);
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("R%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("R3")->format());
			//备注
			auto markFormat = xlsx.cellAt("S3")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("S%1").arg(rowCount), mark, markFormat);
		}
	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(3, 2, 3);
			xlsx.reverseRowsFrom(2, 19, 3);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(18);
		xlsx.deleteLastColumn(18);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);


	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMZHexcel_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	//报表模板/等级公路2018
	QString tableName = QString::fromLocal8Bit("综合车全指标.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	//给表名加上米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	//获取内容的样式
	//hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	int formatRowIndex = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	QVector<hnOutExcelMile> excel10Miles = m_outExcelMileManage->getRoadMessage_10m_Vec();
	if (xlsx.selectSheet(QString::fromLocal8Bit("全指标10m数据")))
	{

		for (int i = 0; i < excelMiles.size(); ++i)
		{
			int colCount = 1;
			int rowIndex = i + 4;
			auto nowExcelMile = excelMiles.at(i);
			//开始桩号	 
			double sMile = nowExcelMile.getStartMile();
			xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strNumber), formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, sMile, formatRowIndex);
			//结束桩号	
			double eMile = nowExcelMile.getEndMile();
			xlsx.writeAndFormat(rowIndex, colCount++, eMile, formatRowIndex);

			//车道	
			QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
			xlsx.writeAndFormat(rowIndex, colCount++, RoadNum, formatRowIndex);
			//路面材质	
			QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
				nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
			xlsx.writeAndFormat(rowIndex, colCount++, roadType, formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftIriValue(), formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightIriValue(), formatRowIndex);
			//要求最大平整度
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getMaxIriValue(), formatRowIndex);

			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftRutValue(), formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightRutValue(), formatRowIndex);

			//要求最大车辙 
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getMaxRutValue(), formatRowIndex);

			if (xlslen == 10)
			{
				xlsx.writeAndFormat(rowIndex, colCount++, excel10Miles[i].getLeftPbValue(10), formatRowIndex);
				xlsx.writeAndFormat(rowIndex, colCount++, excel10Miles[i].getRightPbValue(10), formatRowIndex);
				xlsx.writeAndFormat(rowIndex, colCount++, excel10Miles[i].getPbEvaluateStr(), formatRowIndex);
			}
			else
			{
				xlsx.writeAndFormat(rowIndex, colCount++, "", formatRowIndex);
				xlsx.writeAndFormat(rowIndex, colCount++, "", formatRowIndex);
				xlsx.writeAndFormat(rowIndex, colCount++, "", formatRowIndex);
			}

			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftMtdValue(), formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getCenterMtdValue(), formatRowIndex);
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightMtdValue(), formatRowIndex);
			//获取病害
			auto diseases = nowExcelMile.getRoadDisVec();
			//计算面积
			hnDiseaseSumAreaCaculate caculate;
			QVector<double> areas;
			if (nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE)
			{
				areas = caculate.caculateSumArea(diseases, true, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 0, curProject);

				auto areaFormat = xlsx.cellAt("R4")->format();
				for (auto area : qAsConst(areas))
				{
					xlsx.write(rowIndex, colCount++, area, areaFormat);
				}
			}
			else if (nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE)
			{
				areas = caculate.caculateSumArea(diseases, true, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 1, curProject);
				colCount = 31;
				auto areaFormat = xlsx.cellAt("R4")->format();
				for (auto area : qAsConst(areas))
				{
					xlsx.write(rowIndex, colCount++, area, areaFormat);
				}
			}
			else if (nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SS_SURFACE)
			{

			}
		}

		if (m_xrSetting->outExcelNeedSort)
		{
			if (projectInfo.nLineType == -1)
			{
				xlsx.swapColumns(4, 2, 3);
				xlsx.reverseRowsFrom(2, 42, 4);
			}
		}
	}

	//病害列表
	if (xlsx.selectSheet(QStringLiteral("病害列表")))
	{
		//获取病害
		QVector<hnMile> curMiles = curProject->getCurrentMileVector();
		QVector<hnCommon::hnRoadDiseaseInfo> diss;
	//	curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
		diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
		int	formatRowIndex = 3;
		for (int i = 0; i < diss.size(); ++i)
		{
			int rowCount = 3 + i;
			int colCount = 1;

			auto dis = diss.at(i);
			int  startTrueMile = qRound(curProject->enclToTrueMile(dis.dMileage));

			xlsx.writeAndFormat(rowCount, colCount++, projectInfo.strNumber, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, startTrueMile, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, QString::fromLocal8Bit(projectInfo.strRoadNO), formatRowIndex);
			//病害类型  
			QString disName = QString::fromLocal8Bit(dis.strDisName);
			QStringList disSplit = disName.split('.');

			if (disSplit.size() > 1)
			{
				if (disName.contains(QStringLiteral("修补")))
				{
					xlsx.writeAndFormat(rowCount, colCount++, disSplit[1] + disSplit[0], formatRowIndex);
					//病害程度 
					xlsx.writeAndFormat(rowCount, colCount++, "", formatRowIndex);
				}
				else
				{
					xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(0), formatRowIndex);
					xlsx.writeAndFormat(rowCount, colCount++, QStringLiteral("无"), formatRowIndex);
				}

			}
			else
			{
				xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(0), formatRowIndex);
				xlsx.writeAndFormat(rowCount, colCount++, QStringLiteral("无"), formatRowIndex);
			}

			//病害面积(m2) 
			xlsx.writeAndFormat(rowCount, colCount++, dis.dArea, formatRowIndex);
			//具体位置_距右侧标线位置(m) 
			//算出自动化模式小方格横向的中点，然后再用道路宽度减去中点
			double distance = 0.0;
			if (dis.vec2dRect.size() != 0)
			{
				QMap<double, double> centerMap;
				const double widthScale = curProject->getCurProSetInfo().dRadioX;
				const double roadWidth = curProject->getCurProSetInfo().dRoadWidth;
				for (auto hn2drect : qAsConst(dis.vec2dRect))
				{
					double rectCenter = (hn2drect.p0.x + hn2drect.p1.x) / 2.0;
					centerMap.insert(rectCenter, rectCenter);
				}
				distance = roadWidth - (((centerMap.first() + centerMap.last()) / 2.0)* widthScale);
			}

			xlsx.writeAndFormat(rowCount, colCount++, distance, formatRowIndex);

			hnMile curMile;
			double disCurMile = curProject->enclToTrueMile(dis.dMileage);
			double mileLenght = 100000;
			double minLength;
			//根据桩号找到匹配图片
			for (int i = 0; i < curMiles.size(); ++i)
			{

				double dCurMile = curMiles.at(i).dTrueMile;
				double curLength = qAbs(disCurMile - dCurMile);
				double curLengthTemp = disCurMile - dCurMile;

				if (i != 0 && curLength - minLength > 0)
				{
					break;
				}

				minLength = curLength;
				if (curLength < mileLenght)
				{
					mileLenght = curLength;
					if (i > 0)
					{
						if (curLengthTemp*curProject->getCurProSetInfo().nLineType > 0)
						{
							curMile = curMiles.at(i);
						}
						else
						{
							curMile = curMiles.at(i - 1);
						}


					}
					else
					{
						curMile = curMiles.at(i);

					}

				}
			}
			int findIndex = curMile.picturePath.lastIndexOf("/");
			QString subName = curMile.picturePath.mid(findIndex + 1);

			//路面图像名称
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);


			findIndex = curMile.picturePath.indexOf("RoadImg");
			subName = "/" + curMile.picturePath.mid(findIndex);
			//路面图像相对路径
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);
			QString roadType = dis.nRSurfaceType == 0 ? QStringLiteral("沥青") :
				dis.nRSurfaceType == 1 ? QStringLiteral("水泥") : QString("");
			xlsx.writeAndFormat(rowCount, colCount++, roadType, formatRowIndex);
		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.reverseRowsFrom(1, 10, 3);
		}
	}
	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);


	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportCZSDPJDJJLB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("车辙深度评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}
	 
	//获取工程信息

	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	QXlsx::Cell* cell;
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A4")->format());

		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B4")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, xlsx.cellAt("C4")->format());
		//车道
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());
		auto lIriValue = nowExcelMile.getLeftRutValue();
		xlsx.write(QString("E%1").arg(rowCount), lIriValue, xlsx.cellAt("E4")->format());

		auto rIriValue = nowExcelMile.getRightRutValue();
		xlsx.write(QString("F%1").arg(rowCount), rIriValue, xlsx.cellAt("F4")->format());

		auto judgeValue = nowExcelMile.getjudgeRutValue();
		xlsx.write(QString("G%1").arg(rowCount), judgeValue, xlsx.cellAt("G4")->format());

		QString IriExcelValue = nowExcelMile.getRutExcelStr();
		xlsx.write(QString("H%1").arg(rowCount), IriExcelValue, xlsx.cellAt("H4")->format());
		//评价等级

		QString grade = nowExcelMile.getRutEvaluateStr("H", rowCount);
		xlsx.write(QString("I%1").arg(rowCount), grade, xlsx.cellAt("I4")->format());
		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("J%1").arg(rowCount), roadType, xlsx.cellAt("J4")->format());

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			////车速 
			xlsx.write(QString("K%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("K4")->format());
			////备注
			auto markFormat = xlsx.cellAt("L4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("L%1").arg(rowCount), mark, markFormat);
		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
	if (!xlsx.selectSheet(QString::fromLocal8Bit("Sheet2")))
	{
		return false;
	}
	rowCount = 4;
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A4")->format());
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B4")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, xlsx.cellAt("C4")->format());
		//车道 
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());
		auto lIriValue = nowExcelMile.getLeftRutValue();
		xlsx.write(QString("E%1").arg(rowCount), lIriValue, xlsx.cellAt("E4")->format());

		auto rIriValue = nowExcelMile.getRightRutValue();
		xlsx.write(QString("F%1").arg(rowCount), rIriValue, xlsx.cellAt("F4")->format());


		xlsx.write(QString("G%1").arg(rowCount), QString("=MAX(E%1,F%1)").arg(QString::number(rowCount)), xlsx.cellAt("G4")->format());
		QString IriExcelValue = nowExcelMile.getRutMaxExcelStr();
		xlsx.write(QString("H%1").arg(rowCount), IriExcelValue, xlsx.cellAt("H4")->format());
		//评价等级

		QString grade = nowExcelMile.getRutEvaluateStr("H", rowCount);
		xlsx.write(QString("I%1").arg(rowCount), grade, xlsx.cellAt("I4")->format());
		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("J%1").arg(rowCount), roadType, xlsx.cellAt("J4")->format());
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			////车速 
			xlsx.write(QString("K%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("K4")->format());
			////备注

			auto markFormat = xlsx.cellAt("L4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("L%1").arg(rowCount), mark, markFormat);

		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
	//工程信息
	 exportProjectInfoSheet(xlsx, curProject); 
	 if (!saveExcel(saveExcelDir, tableName, xlsx))
	 {
		 return false;
	 }

	return true;
}

bool hnOutExcelManage::exportCZSDPJDJJLB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("Rut.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet(QString::fromLocal8Bit("十米RDI")))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息

	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int formatRowIndex = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	QXlsx::Cell* cell;
	for (int i = 0; i < excelMiles.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 4;
		auto nowExcelMile = excelMiles.at(i);

		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strNumber), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getStartMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getEndMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strRoadNO), formatRowIndex);


		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftRutValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightRutValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getMaxRutValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRutExcelStr(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRutEvaluateStr("H", rowIndex), formatRowIndex);

		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.writeAndFormat(rowIndex, colCount++, roadType, formatRowIndex);
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getSpeed(), formatRowIndex);

			QString mark = nowExcelMile.getUnitStr();
			xlsx.writeAndFormat(rowIndex, colCount++, mark, formatRowIndex);
		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMMHPJDJB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面磨耗评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	//给table名字 加米
	tableName =  addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;


	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		format = xlsx.cellAt("A4")->format();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		format = xlsx.cellAt("B4")->format();
		xlsx.write(QString("B%1").arg(rowCount), sMile, format);
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, format);
		//车道	
		format = xlsx.cellAt("D4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);
		//左 
		format = xlsx.cellAt("E4")->format();
		auto lValue = nowExcelMile.getLeftMtdValue();
		xlsx.write(QString("E%1").arg(rowCount), lValue, format);
		//右 
		auto rValue = nowExcelMile.getRightMtdValue();
		xlsx.write(QString("F%1").arg(rowCount), rValue, xlsx.cellAt("F4")->format());
		//代表 
		auto cValue = nowExcelMile.getCenterMtdValue();
		xlsx.write(QString("G%1").arg(rowCount), cValue, xlsx.cellAt("G4")->format());

		xlsx.write(QString("H%1").arg(rowCount), nowExcelMile.getMtdWrValue(), xlsx.cellAt("H4")->format());

		xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getPwiValueStr(), xlsx.cellAt("I4")->format());
		xlsx.write(QString("J%1").arg(rowCount), nowExcelMile.getPwiEvaluateStr(), xlsx.cellAt("J4")->format());
		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("K%1").arg(rowCount), roadType, xlsx.cellAt("K4")->format());
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("L%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("L4")->format());
			//备注
			auto markFormat = xlsx.cellAt("M4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("M%1").arg(rowCount), mark, markFormat);

		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 13, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(12);
		xlsx.deleteLastColumn(12);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}
	return true;
}

bool hnOutExcelManage::exportIRIexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面平整度评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}

	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();


	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;

	if (m_xrSetting->RQIJudgeType == 0)
	{
		xlsx.write(QString("G2"), QStringLiteral("代表IRI"));

	}
	else
	{
		xlsx.write(QString("G2"), QStringLiteral("最大IRI"));

	}
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		format = xlsx.cellAt("A4")->format();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		format = xlsx.cellAt("B4")->format();

		xlsx.write(QString("B%1").arg(rowCount), sMile, format);
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, format);
		//车道	
		format = xlsx.cellAt("D4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);

		//左平整度
		format = xlsx.cellAt("E4")->format();
		auto lIriValue = nowExcelMile.getLeftIriValue();
		xlsx.write(QString("E%1").arg(rowCount), lIriValue, format);
		//右平整度
		auto rIriValue = nowExcelMile.getRightIriValue();
		xlsx.write(QString("F%1").arg(rowCount), rIriValue, xlsx.cellAt("F4")->format());
		//代表平整度
		auto judgeValue = nowExcelMile.getJudgeIirValue();
		xlsx.write(QString("G%1").arg(rowCount), judgeValue, xlsx.cellAt("G4")->format());

		//RQI
		QString IriExcelValue = nowExcelMile.getIriExcelStr();
		xlsx.write(QString("H%1").arg(rowCount), IriExcelValue, xlsx.cellAt("H4")->format());
		//评价等级
		QString grade = nowExcelMile.getIriEvaluateStr("H", rowCount);
		xlsx.write(QString("I%1").arg(rowCount), grade, xlsx.cellAt("I4")->format());
		//路面材质
		QString roadType = nowExcelMile.RoadSurfaceStr;
		xlsx.write(QString("J%1").arg(rowCount), roadType, xlsx.cellAt("J4")->format());
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速
			xlsx.write(QString("K%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("K4")->format());
			//备注
			auto markFormat = xlsx.cellAt("L4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("L%1").arg(rowCount), mark, markFormat);
		}


	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMPSexcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面破损评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	

	//加载表格模板
	Document xlsx(xlsxTemplatePath); 
	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}
	auto projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		format = xlsx.cellAt("A3")->format();

		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		format = xlsx.cellAt("B3")->format();
		xlsx.write(QString("B%1").arg(rowCount), nowExcelMile.getStartMile(), format);
		//结束桩号	
		xlsx.write(QString("C%1").arg(rowCount), nowExcelMile.getEndMile(), format);
		//车道	
		format = xlsx.cellAt("D3")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);

		//DR(%)	
		format = xlsx.cellAt("E3")->format();
		double dr = nowExcelMile.getDRScore();
		xlsx.write(QString("E%1").arg(rowCount), dr, format);


		//PCI	
		format = xlsx.cellAt("F3")->format();
		QString pci = nowExcelMile.getPCIExcelStr();
		xlsx.write(QString("F%1").arg(rowCount), pci, format);

		//评价等级
		//E3就是pci对应的单元格
		format = xlsx.cellAt("G3")->format();
		xlsx.write(QString("G%1").arg(rowCount), nowExcelMile.getPciEvaluateStr("F", rowCount), format);

		//路面材质	
		format = xlsx.cellAt("H3")->format();
		QString roadType = nowExcelMile.RoadSurfaceStr;
		xlsx.write(QString("H%1").arg(rowCount), roadType, format);

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速	
			format = xlsx.cellAt("I3")->format();

			xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getSpeed(), format);
			//备注
			auto markFormat = xlsx.cellAt("J3")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("J%1").arg(rowCount), mark, markFormat);

		}

		//里程数
		//	百分比
		//	总里程

	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(3, 2, 3);
		}
	}
	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(9);
		xlsx.deleteLastColumn(9);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);

	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMPSexcel_City(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面破损评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}
	auto projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		format = xlsx.cellAt("A3")->format();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		format = xlsx.cellAt("B3")->format();
		xlsx.write(QString("B%1").arg(rowCount), nowExcelMile.getStartMile(), format);
		//结束桩号	
		xlsx.write(QString("C%1").arg(rowCount), nowExcelMile.getEndMile(), format);
		//车道	
		format = xlsx.cellAt("D3")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);

		//PCI	
		format = xlsx.cellAt("E3")->format();
		double pci = nowExcelMile.getPCIExcelStr().toDouble();
		xlsx.write(QString("E%1").arg(rowCount), pci, format);
		//评价等级
		//E3就是pci对应的单元格
		format = xlsx.cellAt("F3")->format();
		xlsx.write(QString("F%1").arg(rowCount), nowExcelMile.getPciEvaluateStr("E", rowCount), format);

		//路面材质	
		format = xlsx.cellAt("G3")->format();
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("G%1").arg(rowCount), roadType, format);

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速	
			format = xlsx.cellAt("H3")->format();

			xlsx.write(QString("H%1").arg(rowCount), nowExcelMile.getSpeed(), format);
			//备注

			auto markFormat = xlsx.cellAt("I3")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("I%1").arg(rowCount), mark, markFormat);
		}

		//里程数
		//	百分比
		//	总里程

	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(3, 2, 3);
		}
	}
	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(9);
		xlsx.deleteLastColumn(9);
	}

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);


	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}





bool hnOutExcelManage::exportLMMHPJDJB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("PWI.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	//给table名字 加米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	if (!xlsx.selectSheet(QString::fromLocal8Bit("十米PWI")))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始

	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	int formatRowIndex = 4;

	for (int i = 0; i < excelMiles.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 4;
		auto nowExcelMile = excelMiles.at(i);

		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strNumber), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getStartMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getEndMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strRoadNO), formatRowIndex);


		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftMtdValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getCenterMtdValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightMtdValue(), formatRowIndex);

		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getMtdWrValue(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getPwiValueStr(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getPwiEvaluateStr(), formatRowIndex);

		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.writeAndFormat(rowIndex, colCount++, roadType, formatRowIndex);
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getSpeed(), formatRowIndex);
			//备注
			auto markFormat = xlsx.cellAt("L4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.writeAndFormat(rowIndex, colCount++, mark, formatRowIndex);


		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 13, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(12);
		xlsx.deleteLastColumn(12);
	}
	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);
	//保存表格
	QString saveExcelName = saveExcelDir + tableName;

	if (!xlsxInterface.saveExcelFile(xlsx, saveExcelName))
	{
		return false;
	}
	return true;
}

bool hnOutExcelManage::exportLMBHMJTJB_RECT(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面病害面积统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	///获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//获取病害
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	//if (curProject->getCurProSetInfo().nDrawType == 2)
	//{
	//	curProject->getDB()->getDiseaseTable()->readDesignDiseases(HnProjectEnums::roadTypeEnumToQString(curProject->getBaseStandard()), curProject->trueMileToEncl(m_outExcelMileManage->getStartMile()), curProject->trueMileToEncl(m_outExcelMileManage->getEndMile()), diss);

	//}
	//else
	//{
	//	curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->trueMileToEncl(m_outExcelMileManage->getStartMile()), curProject->trueMileToEncl(m_outExcelMileManage->getEndMile()), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
	////	curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
	//}
	diss =hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();

	QVector<hnCommon::hnRoadDiseaseInfo> rutDiss = m_outExcelMileManage->getRutDis();
	if (rutDiss.size() > 0)
	{
		diss.append(rutDiss);
	}
	QVector<hnMile> curMiles = curProject->getCurrentMileVector();
	exportDiseaseAreaSheet(xlsx, diss,curProject);



	std::vector<QString> surfaceTyes = hnApp::hnDataManager::getDataManager()->getRoadSurfaceType();
	for (int i = 0; i<surfaceTyes.size(); ++i)
	{
		//病害统计表
		writeDiseasesStatisticsSheet(xlsx, i, diss, curProject->getBaseStandard(), curProject);

		//病害汇总表
		writeDiseasesSumSheet(xlsx, i, excelMiles, curProject->getBaseStandard(), curProject);
	}


	//工程信息
	exportProjectInfoSheet(xlsx,curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}
	return true;
}

bool hnOutExcelManage::export3DLMBHMJTJB_RECT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile)
{
	QString tableName = QString::fromLocal8Bit("路面病害面积统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get3DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	///获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//获取病害 
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	/*if (curProject->getCurProSetInfo().nDrawType == 2)
	{
		curProject->getDB()->getDiseaseTable()->readDesignDiseases(HnProjectEnums::roadTypeEnumToQString(curProject->getBaseStandard()), sMile, eMile, qdissVec);
		dissVec = qdissVec.toStdVector();
	}
	else
	{
		curProject->getDB()->getDiseaseTable()->read3dRoadDiseaseData(HnProjectEnums::roadTypeEnumToQString(curProject->getBaseStandard()), sMile, eMile, dissVec);
		 
	}*/
	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	//QVector<hnCommon::hnRoadDiseaseInfo> diss = QVector<hnCommon::hnRoadDiseaseInfo>::fromStdVector(dissVec);
	
	exportDiseaseAreaSheet(xlsx, diss,curProject);
	xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害统计表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害统计表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害汇总表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害汇总表"));

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	QString saveExcelName = saveExcelDir + "//" + m_outExcelMileManage->getProject()->get3DProName() + "//" + tableName;

	if (!xlsxInterface.saveExcelFile(xlsx, saveExcelName))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMBHMJTJB_Smart(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面病害面积统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	///获取内容的样式
	hnXlsxInterface xlsxInterface;


	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//获取病害
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	auto curMiles = curProject->getCurrentMileVector();
	//curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());

	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	exportDiseaseAreaSheet(xlsx, diss,curProject);

	std::vector<QString> surfaceTyes = hnApp::hnDataManager::getDataManager()->getRoadSurfaceType();
	for (int i = 0; i < surfaceTyes.size(); ++i)
	{
		//病害统计表
		writeDiseasesStatisticsSheet(xlsx, i, diss, curProject->getBaseStandard(), curProject);

		//病害汇总表
		writeDiseasesSumSheet(xlsx, i, excelMiles, curProject->getBaseStandard(), curProject);
	}

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

void hnOutExcelManage::exportDiseaseAreaSheet(Document& xlsx, const QVector<hnCommon::hnRoadDiseaseInfo> diss, hnPro::hnProject*curProject)
{

	double streetSpace = curProject->get2DProject()->_StreetImgDis;
	if (streetSpace == 10)
	{
		streetSpace = -16;
	}
	else if (streetSpace = 20)
	{
		streetSpace = 10;
	}

	auto projectInfo = curProject->getCurProSetInfo();
	int rowCount = 3;
	int formatRowIndex = 3;
	int colCount = 1;
	//病害列表
	if (xlsx.selectSheet(QStringLiteral("病害列表")))
	{

		rowCount = 3;
		for (int i = 0; i < diss.size(); ++i, ++rowCount)
		{

			colCount = 1;
			auto dis = diss.at(i);
			int  startTrueMile = qRound(curProject->enclToTrueMile(dis.dMileage));
			if (m_xrSetting->outExcelFormatDmi)
			{
				startTrueMile = getCloseMile(dis.dMileage, curProject);
			}
			//开始桩号	  
			xlsx.writeAndFormat(rowCount, colCount++, startTrueMile, formatRowIndex);

			//车道	 
			QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
			xlsx.writeAndFormat(rowCount, colCount++, RoadNum, formatRowIndex);

			//病害类型


			QString disName = QString::fromLocal8Bit(dis.strDisName);
			QStringList disSplit = disName.split('.');
			if (disSplit.size() > 1)
			{
				if (disName.contains(QStringLiteral("修补")))
				{

					xlsx.writeAndFormat(rowCount, colCount++, disSplit[1] + disSplit[0], formatRowIndex);
					xlsx.writeAndFormat(rowCount, colCount++, "", formatRowIndex);


				}
				else
				{
					xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(0), formatRowIndex);

					xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(1), formatRowIndex);

				}

			}
			else
			{
				xlsx.writeAndFormat(rowCount, colCount++, disName, formatRowIndex);
				xlsx.writeAndFormat(rowCount, colCount++, QStringLiteral("无"), formatRowIndex);
			}


			xlsx.writeAndFormat(rowCount, colCount++, dis.dLength, formatRowIndex);

			xlsx.writeAndFormat(rowCount, colCount++, dis.dWidth, formatRowIndex);




			//具体位置_距右侧标线位置(m) 
			//算出自动化模式小方格横向的中点，然后再用道路宽度减去中点
			double distance = 0.0;

			if (dis.vec2dRect.size() != 0)
			{
				if (projectInfo.nDrawType == 1)
				{
					QMap<double, double> centerMap;
					const double widthScale = curProject->getCurProSetInfo().dRadioX;
					const double roadWidth = curProject->getCurProSetInfo().dRoadWidth;
					for (auto hn2drect : qAsConst(dis.vec2dRect))
					{
						double rectCenter = (hn2drect.p0.x + hn2drect.p1.x) / 2.0;
						centerMap.insert(rectCenter, rectCenter);
					}
					distance = roadWidth - (((centerMap.first() + centerMap.last()) / 2.0)* widthScale);
				}
				else
				{
					auto hn2drect = dis.vec2dRect.at(0);
					const double widthScale = curProject->getCurProSetInfo().dRadioX;
					const double roadWidth = curProject->getCurProSetInfo().dRoadWidth;
					distance = roadWidth - ((hn2drect.p0.x + hn2drect.p1.x) / 2.0) * widthScale;
				}
			}
			xlsx.writeAndFormat(rowCount, colCount++, distance, formatRowIndex);

			//病害面积(m2) 
			xlsx.writeAndFormat(rowCount, colCount++, dis.dArea, formatRowIndex);

			if (projectInfo.nDrawType == 1)
			{
				xlsx.writeAndFormat(rowCount, colCount++, -1, formatRowIndex);
				xlsx.writeAndFormat(rowCount, colCount++, -1, formatRowIndex);

			}
			else
			{
				xlsx.writeAndFormat(rowCount, colCount++, dis.dRealLen, formatRowIndex);
				xlsx.writeAndFormat(rowCount, colCount++, dis.dReaWidth, formatRowIndex);
			}

			xlsx.writeAndFormat(rowCount, colCount++, dis.dDepth * 1000, formatRowIndex);
			//面积公式
			xlsx.writeAndFormat(rowCount, colCount++, -1, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, dis.diseaseWeight, formatRowIndex);


			hnMile curMile = curProject->getCloseMileFromDmi(dis.dMileage);
			double streetDis = dis.dMileage + streetSpace;
			if (streetDis <= 0)
			{
				streetDis = 0;
			}
			hnMile curStreetMile = curProject->getCloseStreetMileFromDim(streetDis);

			int findIndex = curMile.picturePath.lastIndexOf("/");
			QString subName = curMile.picturePath.mid(findIndex + 1);

			//路面图像名称 
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);


			findIndex = curMile.picturePath.indexOf("RoadImg");
			subName = "/" + curMile.picturePath.mid(findIndex);
			//路面图像相对路径  
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);


			//路面材质 
			QString roadType = dis.nRSurfaceType == 0 ? QStringLiteral("沥青") :
				dis.nRSurfaceType == 1 ? QStringLiteral("水泥") : QString("砂石");
			xlsx.writeAndFormat(rowCount, colCount++, roadType, formatRowIndex);

			QString disMark = QString::fromLocal8Bit(dis.strRemark);
			xlsx.writeAndFormat(rowCount, colCount++, disMark, formatRowIndex);



			double lat = 0.0;
			double lon = 0.0;
			double height = 0.0;


			if (curProject->getProjectType() == PROJECT_23D_TYPE || curProject->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE
				|| curProject->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
			{
			    QString highGpsPath = 	curProject->get2DProject()->getBasePath() + "\\HighGps2Mile.txt";
				if (QFile::exists(highGpsPath))
				{
					hn2dRectI rect = dis.vec2dRect[0]; 
					//存在高精度数据
					std::unique_ptr<HighAccuracyPositioning> m_highAccuracy = std::make_unique<  HighAccuracyPositioning>(curProject);
					int curPosX = (rect.p0.x+rect.p1.x)/2;
					int curPosY = (rect.p0.y +rect.p2.y)/2;
					double curMile = std::round(curProject->enclToTrueMile(dis.dMileage));


					m_highAccuracy->getHighAccPosition(m_xrSetting->gpsFormat, m_xrSetting->equipType, curMile, curPosX, curPosY, lon, lat, height);
				}
				else
				{
					if (dis.vec3dRect.size() > 0)
					{
						double centerX = (dis.vec3dRect[0].p0.x + dis.vec3dRect[0].p1.x + dis.vec3dRect[0].p2.x + dis.vec3dRect[0].p3.x) / 4;
						double centerY = (dis.vec3dRect[0].p0.y + dis.vec3dRect[0].p1.y + dis.vec3dRect[0].p2.y + dis.vec3dRect[0].p3.y) / 4;
						double centerZ = (dis.vec3dRect[0].p0.z + dis.vec3dRect[0].p1.z + dis.vec3dRect[0].p2.z + dis.vec3dRect[0].p3.z) / 4;
						double mile = (dis.vec3dRect[0].p0.bottomEncoderMile + dis.vec3dRect[0].p1.bottomEncoderMile + dis.vec3dRect[0].p2.bottomEncoderMile + dis.vec3dRect[0].p3.bottomEncoderMile) / 4;
						hnCommon::hn3dPointWithMileI pt(centerX, centerY, centerZ, mile);
						hnDataManager::getDataManager()->getDiseaseLoction(pt, lat, lon, height);
					}
				} 
			}
			else
			{
				QString highGpsPath = curProject->get2DProject()->getBasePath() + "\\HighGps2Mile.txt";
				if (QFile::exists(highGpsPath))
				{
					hn2dRectI rect = dis.vec2dRect[0];
					//存在高精度数据
					std::unique_ptr<HighAccuracyPositioning> m_highAccuracy = std::make_unique<  HighAccuracyPositioning>(curProject);
					int curPosX = (rect.p0.x + rect.p1.x) / 2;
					int curPosY = (rect.p0.y + rect.p2.y) / 2;
					double curMile =  std::round( curProject->enclToTrueMile(dis.dMileage));


					m_highAccuracy->getHighAccPosition(m_xrSetting->gpsFormat, m_xrSetting->equipType, curMile, curPosX, curPosY, lon, lat, height);
				}
				else
				{
					auto gps = curProject->get2DProject()->findCloseGpsInfoFromDmi(dis.dMileage, 0, 0);
					lon = gps._longitude;
					lat = gps._latitude;
					height = gps._elevation;
				}

			
			}
			xlsx.writeAndFormat(rowCount, colCount++, lon, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, lat, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, height, formatRowIndex);

			std::vector<hn2dRectI> points = dis.vec2dRect;
			QMap<int, hn2dRectI> diseaseMap;
			QString PointInfo = "";
			if (m_xrSetting->diseaseExcelOutLocation&& curProject->getCurProSetInfo().nDrawType==1)
			{
				
				for (auto point : points)
				{
					QString point1Str = QString("(%1,%2,%3)").arg(point.p0.m_dmi).arg(point.p0.x).arg(point.p0.y);
					QString point2Str = QString("(%1,%2,%3)").arg(point.p1.m_dmi).arg(point.p1.x).arg(point.p1.y);
					QString point3Str = QString("(%1,%2,%3)").arg(point.p2.m_dmi).arg(point.p2.x).arg(point.p2.y);
					QString point4Str = QString("(%1,%2,%3)").arg(point.p3.m_dmi).arg(point.p3.x).arg(point.p3.y);
					QString Point = QString("[%1,%2,%3,%4]\n").arg(point1Str).arg(point2Str).arg(point3Str).arg(point4Str);

					PointInfo += Point;
				}

				auto markFormat = xlsx.cellAt("U3")->format();
				markFormat.setTextWrap(true);

				xlsx.writeAndFormat(rowCount, colCount++, PointInfo, formatRowIndex);

			}
			else
			{
				xlsx.writeAndFormat(rowCount, colCount++, PointInfo, formatRowIndex);

			}
		

			xlsx.setColumnWidth(colCount, 30);
			xlsx.setColumnWidth(colCount + 1, 32);
			if (m_xrSetting->diseaseExcelOutPicture)
			{
				int cellWidth = 200;
				int cellHeight = 150;
				QFileInfo fileinfo(curMile.picturePath);
				if (fileinfo.exists())
				{
					QImage image(curMile.picturePath);
					auto vec = dis.vec2dRect;
					QPainter painter(&image);
					QPen pen(Qt::red, 5);
					painter.setPen(pen);
					for (auto disPoint : vec)
					{
						if (disPoint.p0.m_dmi == curMile.dEnclMile)
						{
							//往图像上画
							painter.drawLine(QPoint(disPoint.p0.x, disPoint.p0.y), QPoint(disPoint.p1.x, disPoint.p1.y));
							painter.drawLine(QPoint(disPoint.p1.x, disPoint.p1.y), QPoint(disPoint.p2.x, disPoint.p2.y));
							painter.drawLine(QPoint(disPoint.p2.x, disPoint.p2.y), QPoint(disPoint.p3.x, disPoint.p3.y));
							painter.drawLine(QPoint(disPoint.p3.x, disPoint.p3.y), QPoint(disPoint.p0.x, disPoint.p0.y));

						}
					}
					painter.end();
					QImage scaledImage = image.scaled(cellWidth, cellHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

					//设置行高
					xlsx.setRowHeight(rowCount, cellHeight / 1.33);

					xlsx.insertImage(rowCount - 1, colCount - 1, scaledImage);

				}
				QFileInfo fileinfoStreet(curStreetMile.leftStreetPicPath);
				if (fileinfoStreet.exists())
				{
					QImage image(curStreetMile.leftStreetPicPath);

					QImage scaledImage = image.scaled(cellWidth, cellHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					//设置行高
					xlsx.setRowHeight(rowCount, cellHeight / 1.33);

					xlsx.insertImage(rowCount - 1, colCount++, scaledImage);

				}
			}
		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.reverseRowsFrom(1, colCount, 3);
		}
	}
}

bool hnOutExcelManage::exportGPSExcel(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("空间定位数据表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet(QStringLiteral("Sheet1")))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始

	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	int forMatRowIndex = 1;
	for (int i = 0; i < excelMiles.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 2;
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();  
		xlsx.writeAndFormat(rowIndex, colCount++, sMile, forMatRowIndex);  
		_EXCELGPS_ gps = 	nowExcelMile.getStartGpsStr();
		xlsx.writeAndFormat(rowIndex, colCount++, gps._longitude, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, gps._latitude, forMatRowIndex); 
		xlsx.writeAndFormat(rowIndex, colCount++, gps._elevation, forMatRowIndex); 
	}
	 

	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}
	return true;
}

bool hnOutExcelManage::exportLMBHMJTJB_Smart_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("病害统计.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	///获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;

	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	QVector<hnMile> curMiles = curProject->getCurrentMileVector();
	//获取病害
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
//	curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	int formatRowIndex = 3;
	double streetSpace = curProject->get2DProject()->_StreetImgDis;
	if (streetSpace == 10)
	{
		streetSpace = -16;
	}
	else if (streetSpace = 20)
	{
		streetSpace = 10;
	}
	//病害列表
	if (xlsx.selectSheet(QStringLiteral("病害列表")))
	{
		xlsx.setColumnWidth(14, 30);
		xlsx.setColumnWidth(15, 32);
		for (int i = 0; i < diss.size(); ++i)
		{
			int rowCount = 3 + i;
			int colCount = 1;

			auto dis = diss.at(i);
			int  startTrueMile = qRound(curProject->enclToTrueMile(dis.dMileage));
			if (m_xrSetting->outExcelFormatDmi)
			{
				startTrueMile = getCloseMile(dis.dMileage, curProject);
			}

			xlsx.writeAndFormat(rowCount, colCount++, projectInfo.strNumber, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, startTrueMile, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, QString::fromLocal8Bit(projectInfo.strRoadNO), formatRowIndex);
			//病害类型 
			format = xlsx.cellAt("C3")->format();
			QString disName = QString::fromLocal8Bit(dis.strDisName);
			QStringList disSplit = disName.split('.');

			if (disSplit.size() > 1)
			{
				if (disName.contains(QStringLiteral("修补")))
				{
					xlsx.writeAndFormat(rowCount, colCount++, disSplit[1] + disSplit[0], formatRowIndex);
					//病害程度 
					xlsx.writeAndFormat(rowCount, colCount++, "", formatRowIndex);
				}
				else
				{
					xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(0), formatRowIndex);
					xlsx.writeAndFormat(rowCount, colCount++, QStringLiteral("无"), formatRowIndex);
				}

			}
			else
			{
				xlsx.writeAndFormat(rowCount, colCount++, disSplit.at(0), formatRowIndex);
				xlsx.writeAndFormat(rowCount, colCount++, QStringLiteral("无"), formatRowIndex);
			}

			//病害面积(m2) 
			xlsx.writeAndFormat(rowCount, colCount++, dis.dArea, formatRowIndex);
			//具体位置_距右侧标线位置(m) 
			//算出自动化模式小方格横向的中点，然后再用道路宽度减去中点
			double distance = 0.0;
			if (dis.vec2dRect.size() != 0)
			{
				QMap<double, double> centerMap;
				const double widthScale = curProject->getCurProSetInfo().dRadioX;
				const double roadWidth = curProject->getCurProSetInfo().dRoadWidth;
				for (auto hn2drect : qAsConst(dis.vec2dRect))
				{
					double rectCenter = (hn2drect.p0.x + hn2drect.p1.x) / 2.0;
					centerMap.insert(rectCenter, rectCenter);
				}
				distance = roadWidth - (((centerMap.first() + centerMap.last()) / 2.0)* widthScale);
			}

			xlsx.writeAndFormat(rowCount, colCount++, distance, formatRowIndex);

			hnMile curMile = curProject->getCloseMileFromDmi(dis.dMileage);
			double streetDis = dis.dMileage + streetSpace;
			if (streetDis <= 0)
			{
				streetDis = 0;
			}
			hnMile curStreetMile = curProject->getCloseStreetMileFromDim(streetDis);

			int findIndex = curMile.picturePath.lastIndexOf("/");
			QString subName = curMile.picturePath.mid(findIndex + 1);

			//路面图像名称
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);


			findIndex = curMile.picturePath.indexOf("RoadImg");
			subName = "/" + curMile.picturePath.mid(findIndex);
			//路面图像相对路径
			xlsx.writeAndFormat(rowCount, colCount++, subName, formatRowIndex);

			//路面材质
			format = xlsx.cellAt("I3")->format();
			QString roadType = dis.nRSurfaceType == 0 ? QStringLiteral("沥青") :
				dis.nRSurfaceType == 1 ? QStringLiteral("水泥") : QString("");
			xlsx.writeAndFormat(rowCount, colCount++, roadType, formatRowIndex);


			double lat = 0.0;
			double lon = 0.0;
			double height = 0.0;
			if (curProject->getProjectType() == PROJECT_23D_TYPE || curProject->getProjectType() == PROJECT_TYPE::PROJECT_JD_3D_TYPE
				|| curProject->getProjectType() == PROJECT_TYPE::PROJECT_XD_3D_TYPE)
			{
				if (dis.vec3dRect.size() > 0)
				{
					double centerX = (dis.vec3dRect[0].p0.x + dis.vec3dRect[0].p1.x + dis.vec3dRect[0].p2.x + dis.vec3dRect[0].p3.x) / 4;
					double centerY = (dis.vec3dRect[0].p0.y + dis.vec3dRect[0].p1.y + dis.vec3dRect[0].p2.y + dis.vec3dRect[0].p3.y) / 4;
					double centerZ = (dis.vec3dRect[0].p0.z + dis.vec3dRect[0].p1.z + dis.vec3dRect[0].p2.z + dis.vec3dRect[0].p3.z) / 4;
					double mile = (dis.vec3dRect[0].p0.bottomEncoderMile + dis.vec3dRect[0].p1.bottomEncoderMile + dis.vec3dRect[0].p2.bottomEncoderMile + dis.vec3dRect[0].p3.bottomEncoderMile) / 4;
					hnCommon::hn3dPointWithMileI pt(centerX, centerY, centerZ, mile);
					hnDataManager::getDataManager()->getDiseaseLoction(pt, lat, lon, height);
				}

			}
			else
			{
				auto gps = curProject->get2DProject()->findCloseGpsInfoFromDmi(dis.dMileage, 0, 0);
				lon = gps._longitude;
				lat = gps._latitude;
				height = gps._elevation;

			}

			xlsx.writeAndFormat(rowCount, colCount++, lon, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, lat, formatRowIndex);
			xlsx.writeAndFormat(rowCount, colCount++, height, formatRowIndex);



			if (m_xrSetting->diseaseExcelOutPicture)
			{
				int cellWidth = 200;
				int cellHeight = 150;
				QFileInfo fileinfo(curMile.picturePath);
				if (fileinfo.exists())
				{
					QImage image(curMile.picturePath);
					auto vec = dis.vec2dRect;
					QPainter painter(&image);
					QPen pen(Qt::red, 5);
					painter.setPen(pen);
					for (auto disPoint : vec)
					{
						if (disPoint.p0.m_dmi == curMile.dEnclMile)
						{
							//往图像上画
							painter.drawLine(QPoint(disPoint.p0.x, disPoint.p0.y), QPoint(disPoint.p1.x, disPoint.p1.y));
							painter.drawLine(QPoint(disPoint.p1.x, disPoint.p1.y), QPoint(disPoint.p2.x, disPoint.p2.y));
							painter.drawLine(QPoint(disPoint.p2.x, disPoint.p2.y), QPoint(disPoint.p3.x, disPoint.p3.y));
							painter.drawLine(QPoint(disPoint.p3.x, disPoint.p3.y), QPoint(disPoint.p0.x, disPoint.p0.y));

						}
					}
					painter.end();


					QImage scaledImage = image.scaled(cellWidth, cellHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					//设置行高
					xlsx.setRowHeight(rowCount, cellHeight / 1.33);

					xlsx.insertImage(rowCount - 1, colCount - 1, scaledImage);

				}
				QFileInfo fileinfoStreet(curStreetMile.leftStreetPicPath);
				if (fileinfoStreet.exists())
				{
					QImage image(curStreetMile.leftStreetPicPath);

					QImage scaledImage = image.scaled(cellWidth, cellHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
					//设置行高
					xlsx.setRowHeight(rowCount, cellHeight / 1.33);

					xlsx.insertImage(rowCount - 1, colCount, scaledImage);

				}
			}

		}
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.reverseRowsFrom(1, 15, 3);
		}
	}
	//沥青病害统计表
	writeAsphaltDiseasesStatisticsSheet_Smart_QTDZ(xlsx, diss, curProject);

	//沥青病害汇总表
	writeAsphaltDiseasesSumSheet_Smart_QTDZ(xlsx, excelMiles, curProject);

	//水泥病害统计表
	writeCementDiseasesStatisticsSheet_Smart_QTDZ(xlsx, excelMiles, curProject);

	//水泥病害汇总表
	writeCementDiseasesSumSheet_Smart_QTDZ(xlsx, excelMiles, curProject);

	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::export3DLMBHMJTJB_Smart(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject, double sMile, double eMile)
{
	QString tableName = QString::fromLocal8Bit("路面病害面积统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get3DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	 
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();

	//获取病害 
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	//curProject->getDB()->getDiseaseTable()->read3dRoadDiseaseData(HnProjectEnums::roadTypeEnumToQString(curProject->getBaseStandard()), sMile, eMile, dissVec);
	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	 
	exportDiseaseAreaSheet(xlsx, diss,curProject);

	xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害统计表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害统计表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害汇总表"));
	xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害汇总表"));

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	QString saveExcelName = saveExcelDir + "//" + m_outExcelMileManage->getProject()->get3DProName() + "//" + tableName;

	 
	if (!xlsx.saveAs(saveExcelName))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMTCPJDJJLB(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, const MyQtCommon::MyEquipment& equip, hnPro::hnProject*curProject)
{

	QString tableName = QString::fromLocal8Bit("路面跳车评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;


	//给table名字 加米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	exportTC_LMTCZDMGCTJB(xlsx, curProject);
	exportTC_LMTCTJB(xlsx, curProject);

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMTCPJDJJLB_GZQT(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, const MyQtCommon::MyEquipment& equip, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("PBI.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;


	//给table名字 加米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto sett = curProject->get2DProject();

	if (!sett->_IsIRIMTD)
	{
		return false;
	}
	if (!xlsx.selectSheet(QString::fromLocal8Bit("十米PBI")))
	{
		return false;
	}
	auto   projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat = xlsxInterface.getContentFormat();
	QVector<hnOutExcelMile> roadpart = m_outExcelMileManage->getRoadMessage_10m_Vec();
	//下面这些从第n行开始
	int formatRowIndex = 4;
	for (int i = 0; i < roadpart.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 4;
		auto nowExcelMile = roadpart.at(i);
		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strNumber), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getStartMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getEndMile(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, QString::fromLocal8Bit(projectInfo.strRoadNO), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLeftPbValue(10), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getRightPbValue(10), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getjudgePbValue(10), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getPbEvaluateStr(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getPBIScore(), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getPbiEvaluateStr("I", rowIndex), formatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, roadType, formatRowIndex);

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getSpeed(), formatRowIndex);
			//备注
			auto markFormat = xlsx.cellAt("J4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.writeAndFormat(rowIndex, colCount++, mark, formatRowIndex);
		}

	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 13, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(12);
		xlsx.deleteLastColumn(12);
	}
	//工程信息
	exportProjectInfoSheet_GZQT(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportCPMS_LMBHDCB(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("CPMS路面病害调查表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}

	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	///获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();

	if (!curProject)
	{
		return false;
	}

	/*if (WritePrj2CPMSXls(xlsx, QStringLiteral("沥青路面损坏调查表")))
	{
	xlsx.copyRange("A1:Q22", "A25");
	}*/
	if (WritePrj2CPMSXls(xlsx, QStringLiteral("水泥路面损坏调查表"), curProject))
	{
		xlsx.copyRange("A1:R30", "A33");
	}


	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}



bool hnOutExcelManage::exportLMGZSDPJDJJLB(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面构造深度SMTD评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}

	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();


	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		format = xlsx.cellAt("A4")->format();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		format = xlsx.cellAt("B4")->format();
		xlsx.write(QString("B%1").arg(rowCount), sMile, format);
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		format = xlsx.cellAt("C4")->format();
		xlsx.write(QString("C%1").arg(rowCount), eMile, format);
		//车道	
		format = xlsx.cellAt("D4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);

		//左 
		format = xlsx.cellAt("E4")->format();
		auto lValue = nowExcelMile.getLeftMtdValue();
		xlsx.write(QString("E%1").arg(rowCount), lValue, format);
		//右 

		xlsx.write(QString("F%1").arg(rowCount), nowExcelMile.getRightMtdValue(), xlsx.cellAt("F4")->format());


		xlsx.write(QString("G%1").arg(rowCount), nowExcelMile.getRepresentSMtdValue(), xlsx.cellAt("G4")->format());


		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("H%1").arg(rowCount), roadType, xlsx.cellAt("H4")->format());

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("I4")->format());
			//备注 
			auto markFormat = xlsx.cellAt("J4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("J%1").arg(rowCount), mark, markFormat);

		}

	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 10, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(9);
		xlsx.deleteLastColumn(9);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}



bool hnOutExcelManage::exportLMGZSDPJDJJLB_City(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面构造深度评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}

	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();


	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		format = xlsx.cellAt("A4")->format();
		xlsx.write(QString("A%1").arg(rowCount), sMile, format);
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("B%1").arg(rowCount), eMile, format);
		//车道	
		format = xlsx.cellAt("C4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("C%1").arg(rowCount), RoadNum, format);

		//左 
		format = xlsx.cellAt("D4")->format();
		auto lValue = nowExcelMile.getLeftMtdValue();
		xlsx.write(QString("D%1").arg(rowCount), lValue, format);
		//右 
		if (m_outExcelMileManage->getProject()->get2DProject()->_IsDIRIMTD)
		{
			auto rValue = nowExcelMile.getRightMtdValue();
			xlsx.write(QString("E%1").arg(rowCount), rValue, xlsx.cellAt("E4")->format());

			//代表 
			QString str = QString("=ROUND((D%1+E%1)/2,5)").arg(QString::number(rowCount));
			xlsx.write(QString("F%1").arg(rowCount), str, xlsx.cellAt("F4")->format());
		}
		else
		{

			//代表 
			QString str = QString("=ROUND(D%1,5)").arg(QString::number(rowCount));
			xlsx.write(QString("F%1").arg(rowCount), str, xlsx.cellAt("F4")->format());
		}
		xlsx.write(QString("G%1").arg(rowCount), nowExcelMile.getMtdEvaluateStr("F", rowCount), xlsx.cellAt("G4")->format());


		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("H%1").arg(rowCount), roadType, xlsx.cellAt("H4")->format());

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("I4")->format());
			//备注

			auto markFormat = xlsx.cellAt("J4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("J%1").arg(rowCount), mark, markFormat);


		}

	}

	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}


bool hnOutExcelManage::exportGZSD_MPD_PJDJJLB(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面构造深度MPD评价等级记录表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	//给表名加上米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}

	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();


	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		format = xlsx.cellAt("A4")->format();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), format);
		format = xlsx.cellAt("B4")->format();
		xlsx.write(QString("B%1").arg(rowCount), sMile, format);
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		format = xlsx.cellAt("C4")->format();
		xlsx.write(QString("C%1").arg(rowCount), eMile, format);
		//车道	
		format = xlsx.cellAt("D4")->format();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, format);
		//左 
		format = xlsx.cellAt("E4")->format();
		auto lValue = nowExcelMile.getLeftMpdValue();
		xlsx.write(QString("E%1").arg(rowCount), lValue, format);
		//右 
		auto rValue = nowExcelMile.getRightMpdValue();
		xlsx.write(QString("F%1").arg(rowCount), rValue, xlsx.cellAt("F4")->format());
		//代表 
		auto cValue = nowExcelMile.getCenterMpdValue();
		xlsx.write(QString("G%1").arg(rowCount), cValue, xlsx.cellAt("G4")->format());

		if (cValue == 0)
		{
			xlsx.write(QString("H%1").arg(rowCount), 0, xlsx.cellAt("H4")->format());
		}
		else
		{
			QString str = QString("=IF(G%1-MIN(E%1,F%1)>0, 100*(G%1-MIN(E%1,F%1))/G%1,0)").arg(QString::number(rowCount));
			xlsx.write(QString("H%1").arg(rowCount), str, xlsx.cellAt("H4")->format());
		}
		xlsx.write(QString("H%1").arg(rowCount), nowExcelMile.getMpdWrValue(), xlsx.cellAt("H4")->format());
		xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getMpdValueStr(QString("H"), rowCount), xlsx.cellAt("I4")->format());
		xlsx.write(QString("J%1").arg(rowCount), nowExcelMile.getMpdEvaluateStr(QString("I"), rowCount), xlsx.cellAt("J4")->format());

		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");


		xlsx.write(QString("K%1").arg(rowCount), roadType, xlsx.cellAt("K4")->format());
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("L%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("L4")->format());
			//备注
			auto markFormat = xlsx.cellAt("M4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("M%1").arg(rowCount), mark, markFormat);

		}
	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 13, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(12);
		xlsx.deleteLastColumn(12);
	}
	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exportLMJHZKJCSJTJB(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("路面几何状况检测数据统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet(QStringLiteral("Sheet1")))
	{
		return false;
	}
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo(); 
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	Format format;
	QXlsx::Cell* cell;
	int forMatRowIndex = 2;
	for (int i = 0; i < excelMiles.size(); ++i)
	{
		int colCount = 1;
		int rowIndex = i + 4;
		auto nowExcelMile = excelMiles.at(i);
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		double eMile = nowExcelMile.getEndMile();
		xlsx.writeAndFormat(rowIndex, colCount++, sMile, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, eMile, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, projectInfo.strRoadNO, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getCurvature(), forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getLongitudianalSlope()*100, forMatRowIndex);
		xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getCrossSlope()*100, forMatRowIndex);

		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.writeAndFormat(rowIndex, colCount++, roadType, forMatRowIndex);
		 
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.writeAndFormat(rowIndex, colCount++, nowExcelMile.getSpeed(), forMatRowIndex);
			 
			//备注 
			QString mark = nowExcelMile.getUnitStr();
			xlsx.writeAndFormat(rowIndex, colCount++,mark, forMatRowIndex); 
		}

	}

	exportProjectInfoSheet_GZQT(xlsx, curProject);
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}
	return true;

	return false;
}

bool hnOutExcelManage::exportJSZKPDMXB(const QString& saveExcelDir, const QString & modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	//报表模板/等级公路2018/人工模式/
	QString tableName = QString::fromLocal8Bit("技术状况评定明细表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	//给表名加上米
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;

	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);

	if (!xlsx.selectSheet("Sheet1"))
	{
		return false;
	}

	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat = xlsxInterface.getContentFormat();
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//下面这些从第n行开始
	int rowCount = 5;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	xlsx.write(QString("B2"), QString::fromLocal8Bit(projectInfo.strCounty));

	xlsx.write(QString("E2"), QString::fromLocal8Bit(projectInfo.strNumber));
	xlsx.write(QString("G2"), QString::fromLocal8Bit(projectInfo.strRoadLevel));
	xlsx.write(QString("J2"), QString::fromLocal8Bit(projectInfo.getRSurfaceType().c_str()));
	xlsx.write(QString("M2"), QString::fromLocal8Bit(projectInfo.getLineType().c_str()));

	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{

		auto nowExcelMile = excelMiles.at(i);

		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A5")->format());
		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B5")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		double length = qAbs(sMile - eMile);
		xlsx.write(QString("C%1").arg(rowCount), length, xlsx.cellAt("C5")->format());

		//MQI
		/*	QString mqiValue = QString("=D%1*%2+E%1*%3+M%1*%4+N%1*%5").arg(QString::number(rowCount)).arg(QString::number(param.dMQI_WSCI))
		.arg(QString::number(param.dMQI_WPQI)).arg(QString::number(param.dMQI_WBCI)).arg(QString::number(param.dMQI_WTCI));*/
		QString mqiValue = nowExcelMile.getMqiValue(rowCount, "E", "F", "N", "O");
		xlsx.write(QString("D%1").arg(rowCount), mqiValue, xlsx.cellAt("D5")->format());
		//SCI													 
		xlsx.write(QString("E%1").arg(rowCount), nowExcelMile.getSciValue(), xlsx.cellAt("E5")->format());
		//SRI													 
		xlsx.write(QString("L%1").arg(rowCount), 100, xlsx.cellAt("L5")->format());
		//PSSI													 
		xlsx.write(QString("M%1").arg(rowCount), 100, xlsx.cellAt("M5")->format());
		//BCI													 
		xlsx.write(QString("N%1").arg(rowCount), 100, xlsx.cellAt("N5")->format());

		//TCI														 
		xlsx.write(QString("O%1").arg(rowCount), nowExcelMile.getTciValue(), xlsx.cellAt("O5")->format());

		QString pqiStr = nowExcelMile.getPqiValue(rowCount, "G", "H", "I", "J", "K");
		xlsx.write(QString("F%1").arg(rowCount), pqiStr, xlsx.cellAt("F5")->format());
		//PCI				
		QString pci = nowExcelMile.getPCIExcelStr();
		xlsx.write(QString("G%1").arg(rowCount), pci, xlsx.cellAt("G5")->format());
		QString IriExcelValue = nowExcelMile.getIriExcelStr();
		//RQI													 
		xlsx.write(QString("H%1").arg(rowCount), IriExcelValue, xlsx.cellAt("H5")->format());
		//RDI														 
		xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getRutExcelStr(), xlsx.cellAt("I5")->format());
		//PBI														 
		xlsx.write(QString("J%1").arg(rowCount), nowExcelMile.getPBIScore(), xlsx.cellAt("J5")->format());
		//PWI 
		QString valuePwi = nowExcelMile.getPwiValueStr();
		xlsx.write(QString("K%1").arg(rowCount), valuePwi, xlsx.cellAt("K5")->format());

		//路面类型				 
		QString roadType = nowExcelMile.RoadSurfaceStr;
		xlsx.write(QString("P%1").arg(rowCount), roadType, xlsx.cellAt("P5")->format());

	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.reverseRowsFrom(2, 16, 5);
		}
	}



	//工程信息
	exportProjectInfoSheet(xlsx, curProject);



	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}


bool hnOutExcelManage::exportStreetSumExcel(const QString& saveExcelDir, const QString& modelBasePath, int type, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("沿线设施损坏汇总表.xlsx");
	if (type == 0)
	{
		tableName = QString::fromLocal8Bit("沿线设施损坏汇总表.xlsx");
	}
	else if (type == 1)
	{
		tableName = QString::fromLocal8Bit("路基损坏汇总表.xlsx");
	}

	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	if (type == 0)
	{
		if (!xlsx.selectSheet(QStringLiteral("沿线设施损坏汇总表")))
		{
			return false;
		}
	}
	else if (type == 1)
	{
		if (!xlsx.selectSheet(QStringLiteral("路基损坏汇总表")))
		{
			return false;
		}
	}

	auto projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 6;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//auto nowProject = p_OutExcelMileManage->getProject();
	//nowProject.getCurProSetInfo()
	xlsx.write(QString("B2"), curProject->get2DProName(), xlsx.cellAt("B2")->format());
	int disCount = 0;

	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{

		auto nowExcelMile = excelMiles.at(i);
		double startMile = nowExcelMile.getStartMile();
		//开始桩号	 
		xlsx.write(QString("A%1").arg(rowCount), startMile, xlsx.cellAt(QString("A6"))->format());
		//结束桩号	
		double endMile = nowExcelMile.getEndMile();
		xlsx.write(QString("B%1").arg(rowCount), endMile, xlsx.cellAt(QString("B6"))->format());
		QVector<StreetDiseaseManage> disManageVec = nowExcelMile.getStreetYxMap();

		if (type == 0)
		{
			disManageVec = nowExcelMile.getStreetYxMap();
		}
		else if (type == 1)
		{
			disManageVec = nowExcelMile.getStreetLjMap();
		}
		//根据nID进行排序
		qSort(disManageVec.begin(), disManageVec.end(), [&](StreetDiseaseManage t1, StreetDiseaseManage t2) {
			return t1.StreetDis.nID < t2.StreetDis.nID;
		});
		disCount = disManageVec.size();
		for (int colIndex = 0; colIndex < disManageVec.size(); ++colIndex)
		{

			StreetDiseaseManage nowDisManager = disManageVec[colIndex];
			double area = nowDisManager.Area + nowDisManager.Count;

			xlsx.writeAndFormat(rowCount, colIndex + 3, area, 6);
		}
		if (type == 0)
		{
			xlsx.writeAndFormat(rowCount, disManageVec.size() + 3, nowExcelMile.getTciValue(), 6);
			xlsx.writeAndFormat(rowCount, disManageVec.size() + 4, nowExcelMile.getTciEvaluate(), 6);
		}
		else
		{
			xlsx.writeAndFormat(rowCount, disManageVec.size() + 3, nowExcelMile.getSciValue(), 6);
			xlsx.writeAndFormat(rowCount, disManageVec.size() + 4, nowExcelMile.getSciEvaluate(), 6);
		}



	}
	for (int disIndex = 0; disIndex < disCount; ++disIndex)
	{
		QString colChar = MyCommonMethods::myColumIndexToLetter(3 + disIndex);
		QString value = "=SUM(" + colChar + "6:" + colChar + QString::number(rowCount - 1) + ")";
		xlsx.writeAndFormat(3 + disIndex, 2 + disCount + 2 + 2, value, 3);
	}


	//工程信息
	exportProjectInfoSheet(xlsx, curProject);
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}

bool hnOutExcelManage::exporDesignSnDiseaseSum(const QString& saveExcelDir, const QString& modelBasePath, double xlslen, hnPro::hnProject*curProject)
{
	QString tableName = QString::fromLocal8Bit("水泥病害段落统计表.xlsx");
	QString xlsxTemplatePath = QApplication::applicationDirPath() + modelBasePath + tableName;
	tableName = addMetersToTable(xlslen, tableName);
	tableName = m_outExcelMileManage->getProject()->get2DProName() + "_" + tableName;
	QFile file(xlsxTemplatePath);
	if (!file.exists())
	{
		return false;
	}
	//加载表格模板
	Document xlsx(xlsxTemplatePath);
	auto projectInfo = curProject->getCurProSetInfo();
	///获取内容的样式
	hnXlsxInterface xlsxInterface;
	//Format contextFormat = xlsxInterface.getContentFormat();
	QXlsx::Format format;
	//下面这些从第n行开始
	int rowCount = 3;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	//获取病害
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	//curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diss, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
	diss = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	QVector<hnMile> curMiles = curProject->getCurrentMileVector();
	//病害列表
	if (xlsx.selectSheet(QStringLiteral("Sheet1")))
	{
		//方向
		QString roadNameMsg = curProject->get2DProName() + projectInfo.nLineType == 1 ? QStringLiteral("_上行") : QStringLiteral("_下行");
		for (int i = 0; i<excelMiles.size(); ++i, ++rowCount)
		{
			auto curMile = excelMiles.at(i);


			int  startTrueMile = qRound(curMile.getStartMile());
			int  endTrueMile = qRound(curMile.getEndMile());

			//序号
			format = xlsx.cellAt("A2")->format();
			xlsx.write(QString("A%1").arg(rowCount), i + 1, format);


			format = xlsx.cellAt("B2")->format();
			xlsx.write(QString("B%1").arg(rowCount), startTrueMile, format);

			format = xlsx.cellAt("C2")->format();
			xlsx.write(QString("C%1").arg(rowCount), endTrueMile, format);


			format = xlsx.cellAt("D2")->format();
			xlsx.write(QString("D%1").arg(rowCount), roadNameMsg, format);

			format = xlsx.cellAt("E2")->format();
			xlsx.write(QString("E%1").arg(rowCount), xlslen, format);


			auto nowDiss = curMile.getRoadDisVec();
			double lfLength = 0;
			double psbArea = 0;
			double bjdlCount = 0;
			double xbArea = 0;
			for (size_t d = 0; d < nowDiss.size(); d++)
			{

				hnCommon::_HN_ROAD_DISEASE_INFO_ oneDis = nowDiss[d];
				QString disName = QString::fromLocal8Bit(oneDis.strDisName);
				if (disName.contains(QStringLiteral("裂缝")))
				{
					lfLength += oneDis.dRealLen;
				}
				if (disName.contains(QStringLiteral("破碎板")))
				{
					psbArea += oneDis.dArea;
				}

				if (disName.contains(QStringLiteral("板角断裂")))
				{
					bjdlCount++;
				}
				if (disName.contains(QStringLiteral("修补.块状")))
				{
					xbArea += oneDis.dArea;
				}

			}

			//裂缝(m)
			format = xlsx.cellAt("F2")->format();
			xlsx.write(QString("F%1").arg(rowCount), lfLength, format);

			//破碎版(m²)
			format = xlsx.cellAt("G2")->format();
			xlsx.write(QString("G%1").arg(rowCount), psbArea, format);


			format = xlsx.cellAt("H2")->format();
			xlsx.write(QString("H%1").arg(rowCount), 0, format);


			//板角断裂（块）
			format = xlsx.cellAt("I2")->format();
			xlsx.write(QString("I%1").arg(rowCount), bjdlCount, format);


			//修补(m²)
			format = xlsx.cellAt("J2")->format();
			xlsx.write(QString("J%1").arg(rowCount), xbArea, format);
		}
	}
	//保存表格
	if (!saveExcel(saveExcelDir, tableName, xlsx))
	{
		return false;
	}

	return true;
}



bool hnOutExcelManage::exportTC_LMTCTJB(Document & xlsx, hnPro::hnProject*curProject)
{
	auto sett = curProject->get2DProject();

	if (!sett->_IsIRIMTD)
	{
		return false;
	}

	if (!xlsx.selectSheet(QString::fromLocal8Bit("PBI")))
	{
		return false;
	}
	auto   projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat = xlsxInterface.getContentFormat();
	QVector<hnOutExcelMile> roadpart = m_outExcelMileManage->getRoadMessageVec();
	//下面这些从第n行开始
	int rowCount = 4;
	for (int i = 0; i < roadpart.size(); ++i, rowCount++)
	{
		auto nowExcelMile = roadpart.at(i);

		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A4")->format());
		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B4")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, xlsx.cellAt("C4")->format());
		//车道	
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());

		//跳车程度
		xlsx.write(QString("E%1").arg(rowCount), nowExcelMile.getPbiNumber(1), xlsx.cellAt("E4")->format());
		xlsx.write(QString("F%1").arg(rowCount), nowExcelMile.getPbiNumber(2), xlsx.cellAt("F4")->format());
		xlsx.write(QString("G%1").arg(rowCount), nowExcelMile.getPbiNumber(3), xlsx.cellAt("G4")->format());

		//PBI	
		xlsx.write(QString("H%1").arg(rowCount), nowExcelMile.getPBIScore(), xlsx.cellAt("H4")->format());

		//评价等级 
		xlsx.write(QString("I%1").arg(rowCount), nowExcelMile.getPbiEvaluateStr("H", rowCount), xlsx.cellAt("I4")->format());

		//路面材质	
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("J%1").arg(rowCount), roadType, xlsx.cellAt("J4")->format());


		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("K%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("K4")->format());
			//备注
			auto markFormat = xlsx.cellAt("L4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("L%1").arg(rowCount), mark, markFormat);
		}


	}

	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 12, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(11);
		xlsx.deleteLastColumn(11);
	}
}


bool hnOutExcelManage::exportTC_LMTCZDMGCTJB(Document &xlsx, hnPro::hnProject*curProject)
{

	if (!xlsx.selectSheet(QString::fromLocal8Bit("Δh")))
	{
		return false;
	}
	auto projectInfo = curProject->getCurProSetInfo();
	//获取内容的样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat = xlsxInterface.getContentFormat();

	//下面这些从第n行开始
	int rowCount = 4;
	//获得每一行的数据
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessage_10m_Vec();

	for (int i = 0; i < excelMiles.size(); ++i, rowCount++)
	{
		auto nowExcelMile = excelMiles.at(i);
		auto params = nowExcelMile.getRoadTypeSetInfo();
		//开始桩号	
		double sMile = nowExcelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("A4")->format());
		xlsx.write(QString("B%1").arg(rowCount), sMile, xlsx.cellAt("B4")->format());
		//结束桩号	
		double eMile = nowExcelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), eMile, xlsx.cellAt("C4")->format());
		//车道	
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());

		//左Δh
		double leftH = nowExcelMile.getLeftPbValue(10);
		xlsx.write(QString("E%1").arg(rowCount), leftH, xlsx.cellAt("E4")->format());
		//右Δh
		double rightH = nowExcelMile.getRightPbValue(10);
		xlsx.write(QString("F%1").arg(rowCount), rightH, xlsx.cellAt("F4")->format());
		//代表Δh
		QString judgeH = nowExcelMile.getjudgePbValue(10);
		xlsx.write(QString("G%1").arg(rowCount), judgeH, xlsx.cellAt("G4")->format());

		xlsx.write(QString("H%1").arg(rowCount), nowExcelMile.getPbEvaluateStr(), xlsx.cellAt("H4")->format());

		//路面材质
		QString roadType = nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_LQ_SURFACE ? QStringLiteral("沥青") :
			nowExcelMile.RoadSurface == ROAD_SURFACE_TYPE::ROAD_SN_SURFACE ? QStringLiteral("水泥") : QString("");
		xlsx.write(QString("I%1").arg(rowCount), roadType, xlsx.cellAt("I4")->format());

		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			//车速 
			xlsx.write(QString("J%1").arg(rowCount), nowExcelMile.getSpeed(), xlsx.cellAt("J4")->format());
			//备注
			auto markFormat = xlsx.cellAt("K4")->format();
			markFormat.setTextWrap(true);
			QString mark = nowExcelMile.getUnitStr();
			xlsx.write(QString("K%1").arg(rowCount), mark, markFormat);
		}
	}


	if (m_xrSetting->outExcelNeedSort)
	{
		if (projectInfo.nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 11, 4);
		}
	}

	if (!m_xrSetting->outSpeedAndMarkExcel)
	{
		xlsx.deleteLastColumn(10);
		xlsx.deleteLastColumn(10);
	}


}




bool hnOutExcelManage::exportProjectInfoSheet_GZQT(Document &xlsx, hnPro::hnProject*curProject)
{
	if (!xlsx.selectSheet(QString::fromLocal8Bit("工程信息")))
	{
		return false;
	}
	//获取样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat;
	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();
	//省

	QString province = QString::fromLocal8Bit(projectInfo.strProvince);
	contextFormat = xlsx.cellAt("B2")->format();
	xlsx.write(QString("B2"), province, contextFormat);
	//市
	QString city = QString::fromLocal8Bit(projectInfo.strCity);
	contextFormat = xlsx.cellAt("B3")->format();
	xlsx.write(QString("B3"), city, contextFormat);
	//县
	QString county = QString::fromLocal8Bit(projectInfo.strCounty);
	contextFormat = xlsx.cellAt("B4")->format();
	xlsx.write(QString("B4"), county, contextFormat);
	//道路编号
	QString roadNum = QString::fromLocal8Bit(projectInfo.strNumber);
	contextFormat = xlsx.cellAt("B5")->format();
	xlsx.write(QString("B5"), roadNum, contextFormat);
	//道路名称
	QString roadName = QString::fromLocal8Bit(projectInfo.strRoadName);
	contextFormat = xlsx.cellAt("B6")->format();
	xlsx.write(QString("B6"), roadName, contextFormat);
	//起点桩号

	contextFormat = xlsx.cellAt("B7")->format();
	xlsx.write(QString("B7"), m_outExcelMileManage->getStartMile(), contextFormat);
	//行车方向
	QString direction = projectInfo.nLineType == 1 ? QString::fromLocal8Bit("上行") : QString::fromLocal8Bit("下行");
	contextFormat = xlsx.cellAt("B8")->format();
	xlsx.write(QString("B8"), direction, contextFormat);
	//公路等级
	QString roadLevel = QString::fromLocal8Bit(projectInfo.strRoadLevel);
	contextFormat = xlsx.cellAt("B9")->format();
	xlsx.write(QString("B9"), roadLevel, contextFormat);
	//车道
	QString lane = QString::fromLocal8Bit(projectInfo.strRoadNO);
	contextFormat = xlsx.cellAt("B10")->format();
	xlsx.write(QString("B10"), lane, contextFormat);
	//采集日期 
	contextFormat = xlsx.cellAt("B11")->format();
	xlsx.write(QString("B11"), projectInfo.strDate, contextFormat);
	//工程开始时刻

	contextFormat = xlsx.cellAt("B12")->format();
	xlsx.write(QString("B12"), projectInfo.strTimer, contextFormat);
	//检测员
	QString detectPeople = QString::fromLocal8Bit(projectInfo.strSurveyor);
	contextFormat = xlsx.cellAt("B13")->format();
	xlsx.write(QString("B13"), detectPeople, contextFormat);
	//检测天气
	QString wheather = QString::fromLocal8Bit(projectInfo.strWeather);
	contextFormat = xlsx.cellAt("B14")->format();
	xlsx.write(QString("B14"), wheather, contextFormat);
	//路面材质
	QString roadType = QString::fromLocal8Bit(projectInfo.getRSurfaceType().data());
	contextFormat = xlsx.cellAt("B15")->format();
	xlsx.write(QString("B15"), roadType, contextFormat);
	//终点桩号

	contextFormat = xlsx.cellAt("B16")->format();
	xlsx.write(QString("B16"), m_outExcelMileManage->getEndMile(), contextFormat);
	//检测里程（km） 
	contextFormat = xlsx.cellAt("B17")->format();
	xlsx.write(QString("B17"), qAbs(m_outExcelMileManage->getStartMile() - m_outExcelMileManage->getEndMile()) * 0.001, contextFormat);

	return true;
}

bool hnOutExcelManage::WritePrj2CPMSXls(Document&xlsx, QString sheetName, hnPro::hnProject*curProject)
{
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}
	/*if (!hnDataManager::getDataManager()->isOpenProject())
	{
	return false;
	}*/
	//获取样式
	hnXlsxInterface xlsxInterface;
	Format contextFormat;

	//获取工程信息
	auto projectInfo = curProject->getCurProSetInfo();

	xlsx.write(QString("B3"), QString::fromLocal8Bit(projectInfo.strNumber), xlsx.cellAt("B3")->format());
	QString line = projectInfo.nLineType == 1 ? QStringLiteral("上行") : QStringLiteral("下行");
	xlsx.write(QString("D3"), line, xlsx.cellAt("D3")->format());
	int date = QString::fromLocal8Bit(projectInfo.strDate).toInt();
	xlsx.write(QString("H3"), date, xlsx.cellAt("H3")->format());
	xlsx.write(QString("H4"), projectInfo.dBegMile, xlsx.cellAt("H4")->format());
	xlsx.write(QString("M4"), projectInfo.dEndMile, xlsx.cellAt("M4")->format());
	xlsx.write(QString("M5"), readReportRoadWidth(curProject), xlsx.cellAt("M5")->format());

	return true;
}





bool hnOutExcelManage::writeDiseasesStatisticsSheet(Document &xlsx, int roadType, const QVector<hnCommon::hnRoadDiseaseInfo>& diss, HnProjectEnums::StandardParmTypeEnum type, hnPro::hnProject*curProject)
{
	if (!curProject)
	{
		return false;
	}
	//获取面积
	hnDiseaseSumAreaCaculate caculate;
	QVector<double> areas = caculate.caculateSumArea(diss, false, type, roadType, curProject);

	//判断是否有沥青病害，如果没有，删除相关sheet页
	QString sheetName;
	if (roadType == 0)
	{
		sheetName = QString::fromLocal8Bit("沥青病害统计表");
		if (areas.isEmpty() || caculate.isAllZero(areas))
		{
			xlsx.deleteSheet(sheetName);
			//把汇总表删掉
			xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害汇总表"));
			return false;
		}
	}
	else if (roadType == 1)
	{
		sheetName = QString::fromLocal8Bit("水泥病害统计表");
		if (areas.isEmpty() || caculate.isAllZero(areas))
		{
			xlsx.deleteSheet(sheetName);
			//把汇总表删掉
			xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害汇总表"));
			return false;
		}
	}
	else
	{
		sheetName = QString::fromLocal8Bit("砂石病害统计表");
		if (areas.isEmpty() || caculate.isAllZero(areas))
		{
			xlsx.deleteSheet(sheetName);
			//把汇总表删掉
			xlsx.deleteSheet(QString::fromLocal8Bit("砂石病害汇总表"));
			return false;
		}
	}


	//如果一切正常，往表格里面写入
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}

	//车道宽度
	double roadWidth = readReportRoadWidth(curProject);

	//路段长度
	double roadLenth = qAbs(curProject->getCurProSetInfo().dBegMile - curProject->getCurProSetInfo().dEndMile);

	xlsx.write("B2", roadWidth);
	xlsx.write("F2", roadLenth);

	//遍历面积，写入表格
	int rowCount = 4;
	const int columnCount = 3;
	auto format = xlsx.cellAt("C4")->format();
	for (auto area : qAsConst(areas))
	{
		xlsx.write(rowCount, columnCount, area, format);
		rowCount++;
	}

	return true;
}

bool hnOutExcelManage::writeDiseasesSumSheet(Document &xlsx, int roadType, QVector<hnOutExcelMile> & excelMiles, HnProjectEnums::StandardParmTypeEnum type, hnPro::hnProject*curProject)
{
	QString sheetName = QString::fromLocal8Bit("水泥病害汇总表");
	switch (roadType)
	{
	case  0:
		sheetName = QString::fromLocal8Bit("沥青病害汇总表");
		break;
	case 1:
		sheetName = QString::fromLocal8Bit("水泥病害汇总表");
		break;
	case  2:
		sheetName = QString::fromLocal8Bit("砂石病害汇总表");
		break;
	default:
		break;
	}

	//如果一切正常，往表格里面写入
	if (!xlsx.selectSheet(sheetName))

	{
		return false;
	}
	auto format = xlsx.cellAt("A5")->format();
	int rowCount = 5;
	for (auto excelMile : qAsConst(excelMiles))
	{
		if (excelMile.RoadSurface != roadType)
		{
			continue;
		}
		//起点桩号
		double beginMile = excelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), beginMile, format);
		//终点桩号
		double endMile = excelMile.getEndMile();
		xlsx.write(QString("B%1").arg(rowCount), endMile, format);
		//车道
		auto projectInfo = curProject->getCurProSetInfo();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("C%1").arg(rowCount), RoadNum, format);

		//获取病害
		auto diseases = excelMile.getRoadDisVec();
		//计算面积
		hnDiseaseSumAreaCaculate caculate;
		QVector<double> areas = caculate.caculateSumArea(diseases, true, type, roadType, curProject);
		int columnCount = 4;
		auto areaFormat = xlsx.cellAt("D5")->format();
		for (auto area : qAsConst(areas))
		{
			xlsx.write(rowCount, columnCount, area, areaFormat);
			columnCount++;
		}
		double dr = excelMile.getDRScore();
		xlsx.writeAndFormat(rowCount, columnCount++, dr, 5);

		//PCI	

		QString pci = excelMile.getPCIExcelStr();
		xlsx.writeAndFormat(rowCount, columnCount++, pci, 5);
		//评价等级   
		QString cellColStr = MyCommonMethods::myColumIndexToLetter(columnCount - 1);
		xlsx.writeAndFormat(rowCount, columnCount++, excelMile.getPciEvaluateStr(cellColStr, rowCount), 5);

		rowCount++;

	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.swapColumns(5, 1, 2);

		}
	}
	return true;
}


bool hnOutExcelManage::writeAsphaltDiseasesStatisticsSheet_Smart_QTDZ(Document &xlsx, const QVector<hnCommon::hnRoadDiseaseInfo>& diseases, hnPro::hnProject*curProject)
{
	if (!curProject)
	{
		return false;
	}
	//获取面积
	hnDiseaseSumAreaCaculate caculate;
	QVector<double> areas = caculate.caculateSumArea(diseases, false, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 0, curProject);

	//判断是否有沥青病害，如果没有，删除相关sheet页
	QString sheetName = QString::fromLocal8Bit("沥青病害统计表");
	if (areas.isEmpty() || caculate.isAllZero(areas))
	{
		xlsx.deleteSheet(sheetName);
		//把汇总表删掉
		xlsx.deleteSheet(QString::fromLocal8Bit("沥青病害汇总表"));
		return false;
	}

	//如果一切正常，往表格里面写入
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}

	//车道宽度
	double roadWidth = readReportRoadWidth(curProject);

	//路段长度
	double roadLenth = qAbs(curProject->getCurProSetInfo().dBegMile - curProject->getCurProSetInfo().dEndMile);

	xlsx.write("B2", roadWidth);
	xlsx.write("F2", roadLenth);

	//遍历面积，写入表格
	int rowCount = 4;
	const int columnCount = 3;
	auto format = xlsx.cellAt("C4")->format();
	for (auto area : qAsConst(areas))
	{
		xlsx.write(rowCount, columnCount, area, format);
		rowCount++;
	}

	return true;
}


bool hnOutExcelManage::writeAsphaltDiseasesSumSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject)
{
	QString sheetName = QString::fromLocal8Bit("沥青病害汇总表");
	//如果一切正常，往表格里面写入f
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}
	auto format = xlsx.cellAt("B4")->format();
	int rowCount = 4;
	for (auto excelMile : qAsConst(excelMiles))
	{
		if (excelMile.RoadSurface != 0)
		{
			continue;
		}
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(curProject->getCurProSetInfo().strNumber), xlsx.cellAt("A4")->format());

		//起点桩号
		double beginMile = excelMile.getStartMile();
		xlsx.write(QString("B%1").arg(rowCount), beginMile, format);
		//终点桩号
		double endMile = excelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), endMile, format);
		//车道
		auto projectInfo = curProject->getCurProSetInfo();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());
		//获取病害
		auto diseases = excelMile.getRoadDisVec();
		//计算面积
		hnDiseaseSumAreaCaculate caculate;
		QVector<double> areas = caculate.caculateSumArea(diseases, true, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 0, curProject);
		int columnCount = 5;
		auto areaFormat = xlsx.cellAt("E4")->format();
		for (auto area : qAsConst(areas))
		{
			xlsx.write(rowCount, columnCount, area, areaFormat);
			columnCount++;
		}
		//DR(%)	
		auto drFormat = xlsx.cellAt("R4")->format();
		double dr = excelMile.getDRScore();
		xlsx.write(QString("R%1").arg(rowCount), dr, drFormat);
		//PCI	
		auto pciFormat = xlsx.cellAt("S4")->format();
		QString pci = excelMile.getPCIExcelStr();
		xlsx.write(QString("S%1").arg(rowCount), pci, pciFormat);
		//评价等级
		auto evaluateFormat = xlsx.cellAt("T4")->format();
		xlsx.write(QString("T%1").arg(rowCount), excelMile.getPciEvaluateStr("S", rowCount), format);

		rowCount++;

	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 20, 4);
		}
	}
	return true;
}

bool hnOutExcelManage::writeCementDiseasesStatisticsSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject)
{
	if (!curProject)
	{
		return false;
	}

	//获取病害
	QVector<hnCommon::hnRoadDiseaseInfo> diseases;
	//curProject->getDB()->getDiseaseTable()->readRoadDiseaseData(curProject->getCurProSetInfo(), curProject->getCurrentMileVector(), diseases, curProject->getCurrentMarkVector(), curProject->getRoadSpace());
	diseases = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllRoadDiseases();
	//获取面积
	hnDiseaseSumAreaCaculate caculate;
	QVector<double> areas = caculate.caculateSumArea(diseases, false, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 1, curProject);

	//判断是否有沥青病害，如果没有，删除相关sheet页
	QString sheetName = QString::fromLocal8Bit("水泥病害统计表");
	if (areas.isEmpty() || caculate.isAllZero(areas))
	{
		xlsx.deleteSheet(sheetName);
		//把汇总表删掉
		xlsx.deleteSheet(QString::fromLocal8Bit("水泥病害汇总表"));
		return false;
	}

	//如果一切正常，往表格里面写入
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}

	//车道宽度
	double roadWidth = readReportRoadWidth(curProject);

	//路段长度
	double roadLenth = qAbs(curProject->getCurProSetInfo().dBegMile - curProject->getCurProSetInfo().dEndMile);

	xlsx.write("B2", roadWidth);
	xlsx.write("F2", roadLenth);

	//遍历面积，写入表格
	int rowCount = 4;
	const int columnCount = 3;
	auto format = xlsx.cellAt("C4")->format();
	for (auto area : qAsConst(areas))
	{
		xlsx.write(rowCount, columnCount, area, format);
		rowCount++;
	}

	return true;
}

bool hnOutExcelManage::writeCementDiseasesSumSheet_Smart_QTDZ(Document &xlsx, QVector<hnOutExcelMile> & excelMiles, hnPro::hnProject*curProject)
{
	QString sheetName = QString::fromLocal8Bit("水泥病害汇总表");
	//如果一切正常，往表格里面写入
	if (!xlsx.selectSheet(sheetName))
	{
		return false;
	}
	auto format = xlsx.cellAt("B4")->format();
	int rowCount = 4;
	for (auto excelMile : qAsConst(excelMiles))
	{
		if (excelMile.RoadSurface != 1)
		{
			continue;
		}
		//起点桩号
		double beginMile = excelMile.getStartMile();
		xlsx.write(QString("A%1").arg(rowCount), QString::fromLocal8Bit(curProject->getCurProSetInfo().strNumber), xlsx.cellAt("A4")->format());
		xlsx.write(QString("B%1").arg(rowCount), beginMile, format);
		//终点桩号
		double endMile = excelMile.getEndMile();
		xlsx.write(QString("C%1").arg(rowCount), endMile, format);
		//车道
		auto projectInfo = curProject->getCurProSetInfo();
		QString RoadNum = QString::fromLocal8Bit(projectInfo.strRoadNO);
		xlsx.write(QString("D%1").arg(rowCount), RoadNum, xlsx.cellAt("D4")->format());


		//获取病害
		auto diseases = excelMile.getRoadDisVec();
		//计算面积
		hnDiseaseSumAreaCaculate caculate;
		QVector<double> areas = caculate.caculateSumArea(diseases, true, HnProjectEnums::StandardParmTypeEnum::DegreeRoad2018, 1, curProject);
		int columnCount = 5;
		auto areaFormat = xlsx.cellAt("E4")->format();
		for (auto area : qAsConst(areas))
		{
			xlsx.write(rowCount, columnCount, area, areaFormat);
			columnCount++;
		}
		//DR(%)	
		auto drFormat = xlsx.cellAt("Q4")->format();
		double dr = excelMile.getDRScore();
		xlsx.write(QString("Q%1").arg(rowCount), dr, drFormat);
		//PCI	
		auto pciFormat = xlsx.cellAt("R4")->format();
		QString pci = excelMile.getPCIExcelStr();
		xlsx.write(QString("R%1").arg(rowCount), pci, pciFormat);
		//评价等级
		auto evaluateFormat = xlsx.cellAt("S4")->format();
		xlsx.write(QString("S%1").arg(rowCount), excelMile.getPciEvaluateStr("R", rowCount), format);

		rowCount++;
	}
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{
			xlsx.swapColumns(4, 2, 3);
			xlsx.reverseRowsFrom(2, 19, 4);
		}
	}
	return true;
}



QString hnOutExcelManage::addMetersToTable(const double meter, const QString &tableName)
{
	//加在后面的米
	QString afterMeter = "_" + QString::number(meter) + "m";

	QFileInfo fileInfo(tableName);

	QString resultTableName;
	resultTableName = fileInfo.baseName() + afterMeter + ".xlsx";

	return resultTableName;
}

void hnOutExcelManage::outExcelStreetDegreeRoad2018(const QString& saveExcelDir, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{

	QString standardName = QStringLiteral("等级公路 JTG H20-2018");
	//报表模板/等级公路2018/人工模式/
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + QStringLiteral("景观") + "//";

	//获取double/float类型小数点后保留的位数
	//->m_decimalDigits = ->m_xrSetting->sheetRoundingOffNum;

	switch (key)
	{
	case 0:		//沿线设施损坏汇总表
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportStreetSumExcel(saveExcelDir,modeleBasePath, 0, xlslen, curProject);

	}
	break;

	case 1:		//CPMS沿线设施损坏
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportLMMHPJDJB(saveExcelDir,modeleBasePath, xlslen, curProject);
	}
	break;
	case 2:		//路基损坏汇总表
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportStreetSumExcel(saveExcelDir, modeleBasePath, 1, xlslen, curProject);
	}
	break;
	case 3:		//CPMS路基损坏
	{
		MyQtCommon::MyEquipment setEquip;
		if (m_xrSetting->outSpeedAndMarkExcel)
		{
			setEquip.SPEED = true;
		} 
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportLMPSexcel(saveExcelDir,modeleBasePath, xlslen, curProject);
	}
	break;
	default:
		break;
	}
}

void hnOutExcelManage::outExcelStreetRuralRoadlowLevelRoad(const QString& saveExcelDir, int key, double xlslen, hnPro::hnProject*curProject, double sMile , double eMile )
{
	QString standardName = QStringLiteral("低等级农村公路");
	QString modeleBasePath = QStringLiteral("//报表模板") + "//" + standardName + "//" + QStringLiteral("景观") + "//";

	//获取double/float类型小数点后保留的位数
	//->m_decimalDigits = ->m_xrSetting->sheetRoundingOffNum;

	switch (key)
	{
	case 0:		//沿线设施损坏汇总表
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportStreetSumExcel(saveExcelDir, modeleBasePath, 0, xlslen, curProject);

	}
	break;

	case 1:		//CPMS沿线设施损坏
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportLMMHPJDJB(saveExcelDir, modeleBasePath, xlslen, curProject);
	}
	break;
	case 2:		//路基损坏汇总表
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportStreetSumExcel(saveExcelDir, modeleBasePath, 1, xlslen, curProject);
	}
	break;
	case 3:		//CPMS路基损坏
	{
		MyQtCommon::MyEquipment setEquip;
		setEquip.STREET = true;
		initSegmentInterval(curProject,xlslen, setEquip, sMile, eMile);
		exportLMPSexcel(saveExcelDir, modeleBasePath, xlslen, curProject);
	}
	break;
	default:
		break;
	}
}

double hnOutExcelManage::getCloseMile(const double& value, hnPro::hnProject*curProject)
{
	QVector<hnOutExcelMile> excelMiles = m_outExcelMileManage->getRoadMessageVec();
	QVector<hnOutExcelMile> result;
	if (m_xrSetting->outExcelNeedSort)
	{
		if (curProject->getCurProSetInfo().nLineType == -1)
		{

			result.reserve(excelMiles.size());
			for (auto it = excelMiles.rbegin(); it != excelMiles.rend(); ++it)
			{
				result.append(*it);
			}

		}
		else
		{
			result.reserve(excelMiles.size());
			for (auto it = excelMiles.begin(); it != excelMiles.end(); ++it)
			{
				result.append(*it);
			}
		}
	}
	if (result.isEmpty())
	{
		return 0;
	}

	auto it = std::lower_bound(result.begin(), result.end(), value, [](const hnOutExcelMile& mile, double val)
	{
		return mile.getStartDmi() < val;

	});
	if (it == result.begin())
	{
		return -1;
	}
	else if (it == result.end())
	{
		return  result.end()->getEndDmi();
	}
	else
	{
		auto closeMile = (*(it - 1));
		double distance = value - closeMile.getStartDmi();
		return (distance * curProject->getCurProSetInfo().nLineType) + closeMile.getStartMile();
	}

}

 

bool hnOutExcelManage::saveExcel(const QString& saveDir, const QString& ExcelName, Document &xlsx)
{
	//保存表格
	QString saveExcelName = saveDir + ExcelName;
	if (!xlsx.saveAs(saveExcelName))
	{
		return false;
	}
	return true;
	////保存表格
	//QString saveExcelName = saveDir + ExcelName;

	//if (!xlsxInterface.saveExcelFile(xlsx, saveExcelName))
	//{
	//	return false;
	//}
}

QSharedPointer<hnOutExcelMileManage> hnOutExcelManage::m_outExcelMileManage;

HnXRSettings* hnOutExcelManage::m_xrSetting=HnXRSettings::getInstance();

int hnOutExcelManage::progressDefault = 0 ;
 
//QString hnOutExcelManage::saveExcelDir;

