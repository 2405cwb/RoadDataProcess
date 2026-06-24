#include "StdAfx.h"
#include "hdSysSetting.h"
#include "..\hdCore\tinyxml.h"
#include "..\hdCommon\hdCommon.h"

namespace hd
{
CHdSysSetting* CHdSysSetting::sysSetting = NULL;

CHdSysSetting::CHdSysSetting(void)
{
	//m_IsFirstRun = 1; // 默认第一次启动
	commonSetting.UILanguage = 0;// 中文（简体）代号,1英文
	commonSetting.autoSaveTime = 30;
	commonSetting.renderSimple = 5000000;
	commonSetting.loadSimple = 20000000;
	commonSetting.showNormal = 0;
	commonSetting.editMode = 1;

	//importSetting.maxCol = 8000;
	//importSetting.maxRow = 4000;
	//importSetting.maxIntensity = 3000;
	//importSetting.units = hdMeters;

	//exportSetting.colSimple = 1;
	//exportSetting.rowSimple = 1;
	//exportSetting.maxDistance = 500.0f;
	//exportSetting.minDistance = 0.0f;

	matchSetting.sphereRadius = 0.0725f;
	matchSetting.radiusStdDev = 0.003f;//5.26.2014 修改by dongdai from 0.0008f;
	matchSetting.blockSize = 120;
	matchSetting.chessboardSize = 0.2f; //5.26.2014 修改by dongdai from 0.3f;

	matchSetting.strReportPath = "D:\\HDScene\\";

	filterSetting.maxDistance = 500.0f;
	filterSetting.maxIntensity = 3000;
	filterSetting.minIntensity = 70;
	filterSetting.AddFilter = 0;
	filterSetting.MaxGridSize = 10.0;
	filterSetting.MaxTriaSIzeLen = 20.0;

	// 测量设置默认值 
	measureSetting.selPointColor = RGB(0, 0, 255);
	measureSetting.selPLineColor = RGB(255,255,0);
	measureSetting.measureLineColor = RGB(255,255,0);
	measureSetting.cadPointColor = RGB(255,0,0);
	measureSetting.nSendMode = 0;
	measureSetting.nSelPtsMode = 0;
	measureSetting.nPickPointMode = 1;
	measureSetting.dNearFieldSize = 0.1f;

	measureSetting.selPointSize=1;
	measureSetting.MarkPointSize=5;
	measureSetting.selLineWidth=1;
	measureSetting.meaLineWidth=1;

	// iscan工程设置
	iScanSetting.panoOnFullSize = 0;
	iScanSetting.showByPos = 0;
	iScanSetting.loadByObbBox = 0;
	iScanSetting.pcdDistInPanoView = 50.0f;
	iScanSetting.m_nIScanDeviceType = 0; // 0表示为车载，1表示为船载
	iScanSetting.renderSimple=2000000;//快速相机下点云显示阈值
	iScanSetting.renderSimple_3D=2000000;//3D相机下点云显示阈值

	// 渲染设置
	renderSetting.bkColor = 0;
	renderSetting.renderStyle = 0;
	renderSetting.ptSize = 2;
	renderSetting.fMinIntenThre = 0.15;
	renderSetting.fMaxIntenThre = 0.05;

	// 点云触点拟合面参数设置
	fitRowColSetting.nRow = 10;
	fitRowColSetting.nCol = 20;
	fitRowColSetting.nRowCut = 2;
	fitRowColSetting.nColCut = 4;
	fitRowColSetting.nFitCount = 4;
	//



	// 定义一个静态对象，程序退出时自动调用hdSettingCleaner析构销毁CHdSysSetting
	static hdSettingCleaner mCleaner;
}


CHdSysSetting::~CHdSysSetting(void)
{
}

//! 删除静态唯一对象
void CHdSysSetting::destroySysSetting()
{
	if (sysSetting)
	{
		delete sysSetting;
		sysSetting = NULL;
	}
}
CHdSysSetting* CHdSysSetting::getSysSetting()
{
	if (sysSetting == NULL)
	{
		sysSetting = new CHdSysSetting;
		string filePath = getCurrentDir();
		filePath += "sysSetting.xml";
	
		TiXmlDocument doc(filePath.c_str());
		if (!doc.LoadFile())
		{
			return sysSetting;
		}

		TiXmlElement* rootElement = doc.RootElement();
		if (rootElement)
		{
			string strValue;
			string strText;

			// 读取常规属性设置
			TiXmlElement* element = rootElement->FirstChildElement("commonSetting");
			TiXmlElement* childElement = element->FirstChildElement();
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				//if (strValue == "IsFirstRun")
				//{
				//	sysSetting->m_IsFirstRun = atoi(strText.c_str());
				//}

				if (strValue == "UILanguage")
				{
					sysSetting->commonSetting.UILanguage = atoi(strText.c_str());
				}
			
				if (strValue == "autoSaveTime")
				{
					sysSetting->commonSetting.autoSaveTime = atoi(strText.c_str());
				}	

				if (strValue == "renderSimple")
				{
					sysSetting->commonSetting.renderSimple = atoi(strText.c_str());
				}	

				if (strValue == "loadSimple")
				{
					sysSetting->commonSetting.loadSimple = atoi(strText.c_str());
				}	

				if (strValue == "showNormal")
				{
					sysSetting->commonSetting.showNormal = atoi(strText.c_str());
				}

				if (strValue == "editMode")
				{
					sysSetting->commonSetting.editMode = atoi(strText.c_str());
				}

				if (strValue == "showVector")
				{
					sysSetting->commonSetting.showVector = atoi(strText.c_str());
				}

				if (strValue == "spaceFilterDist")
				{
					sysSetting->commonSetting.space_filter_dist = atof(strText.c_str());
				}

				if (strValue == "loadSimpleMode")
				{
					sysSetting->commonSetting.load_simple_mode  = atoi(strText.c_str());
				}

				childElement = childElement->NextSiblingElement();
			}

			// 读取选择设置
			element = rootElement->FirstChildElement("selectionSetting");
			childElement = element->FirstChildElement();
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "inside")
				{
					sysSetting->selectionSetting.inside = atoi(strText.c_str());
				}
				else if (strValue == "selectMode")
				{
					sysSetting->selectionSetting.selectMode = atoi(strText.c_str());
				}
				childElement = childElement->NextSiblingElement();
			}

			//// 读取导入数据属性设置
			//element = rootElement->FirstChildElement("importSetting");
			//childElement = element->FirstChildElement();
			//while (childElement)
			//{
			//	strValue = childElement->Value();
			//	strText = childElement->GetText();

			//	if (strValue == "units")
			//	{
			//		sysSetting->importSetting.units = (hdUnits)atoi(strText.c_str());
			//	}
			//	else if (strValue == "maxRow")
			//	{
			//		sysSetting->importSetting.maxRow = atoi(strText.c_str());
			//	}
			//	else if (strValue == "maxCol")
			//	{
			//		sysSetting->importSetting.maxCol = atoi(strText.c_str());
			//	}
			//	else if (strValue == "maxIntensity")
			//	{
			//		sysSetting->importSetting.maxIntensity = atoi(strText.c_str());
			//	}
			//	childElement = childElement->NextSiblingElement();
			//}

			//// 读取导出数据设置
			//element = rootElement->FirstChildElement("exportSetting");
			//childElement = element->FirstChildElement();
			//while (childElement)
			//{
			//	strValue = childElement->Value();
			//	strText = childElement->GetText();

			//	if (strValue == "maxDistance")
			//	{
			//		sysSetting->exportSetting.maxDistance = (float)atof(strText.c_str());
			//	}
			//	else if (strValue == "minDistance")
			//	{
			//		sysSetting->exportSetting.minDistance = (float)atof(strText.c_str());
			//	}
			//	else if (strValue == "rowSimple")
			//	{
			//		sysSetting->exportSetting.rowSimple = atoi(strText.c_str());
			//	}
			//	else if (strValue == "colSimple")
			//	{
			//		sysSetting->exportSetting.colSimple = atoi(strText.c_str());
			//	}
			//
			//	childElement = childElement->NextSiblingElement();
			//}

			// 读取配准设置
			element = rootElement->FirstChildElement("matchSetting");
			childElement = element->FirstChildElement();
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "sphereRadius")
				{
					sysSetting->matchSetting.sphereRadius = (float)atof(strText.c_str());
				}
				//else if (strValue == "radiusStdDev")
				//{
				//	sysSetting->matchSetting.radiusStdDev = (float)atof(strText.c_str());
				//}
				//else if (strValue == "blockSize")
				//{
				//	sysSetting->matchSetting.blockSize = atoi(strText.c_str());
				//}
				else if (strValue == "chessboardSize")
				{
					sysSetting->matchSetting.chessboardSize = (float)atof(strText.c_str());
				}
			    else if (strValue == "reportPath")
				{
					sysSetting->matchSetting.strReportPath = strText;
				}
				childElement = childElement->NextSiblingElement();
			}

			// 读取过滤设置
			element = rootElement->FirstChildElement("filterSetting");
			childElement = element->FirstChildElement();
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "maxDistance")
				{
					sysSetting->filterSetting.maxDistance = (float)atof(strText.c_str());
				}

				if (strValue == "maxIntensity")
				{
					sysSetting->filterSetting.maxIntensity = atoi(strText.c_str());
				}
				if (strValue == "minIntensity")
				{
					sysSetting->filterSetting.minIntensity = atoi(strText.c_str());
				}
				if (strValue == "maxGridSize")
				{
					sysSetting->filterSetting.MaxGridSize = (float)atof(strText.c_str());
				}
				if (strValue == "maxTriaSIzeLen")
				{
					sysSetting->filterSetting.MaxTriaSIzeLen = (float)atof(strText.c_str());
				}
				if (strValue == "bAddFilter")
				{
					sysSetting->filterSetting.AddFilter = (float)atof(strText.c_str());
				}
				childElement = childElement->NextSiblingElement();
			}	

			// 质心选取设置
			element = rootElement->FirstChildElement("centriodPickSetting");
			if (element)
			{
				sysSetting->centrioidPicker.minIntensity = atoi(element->Attribute("minIntensity"));
				sysSetting->centrioidPicker.maxIntensity = atoi(element->Attribute("maxIntensity"));
				sysSetting->centrioidPicker.radius = atof(element->Attribute("radius"));
				sysSetting->centrioidPicker.pickCtrlPtMode = atoi(element->Attribute("pickCtrlPointMode") == NULL?
					"0":element->Attribute("pickCtrlPointMode"));
				sysSetting->centrioidPicker.openZoomWnd = atoi(element->Attribute("sltCtrlPtZoomWnd") == NULL?
					"0":element->Attribute("sltCtrlPtZoomWnd"));
 				sysSetting->centrioidPicker.zoomPcdRadius = atof(element->Attribute("ZoomWndPcdRadius") == NULL ?
 					"1.0":element->Attribute("ZoomWndPcdRadius"));
			}

			// 读取选择颜色设置
			element = rootElement->FirstChildElement("measureSetting");
			if (element == NULL)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			// 遍历color节点下所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "selPointColor")
				{
					if (childElement->GetText())
					{
						sysSetting->measureSetting.selPointColor = atol(strText.data());
					}
				}
				else if (strValue == "selPLineColor")
				{
					sysSetting->measureSetting.selPLineColor = atol(strText.data());
				}
				else if (strValue == "measureLineColor")
				{
					sysSetting->measureSetting.measureLineColor = atol(strText.data());
				}
				else if (strValue == "cadPointColor")
				{
					sysSetting->measureSetting.cadPointColor = atol(strText.data());
				}
				else if (strValue == "draw2CADMode")
				{
					sysSetting->measureSetting.nSendMode = atoi(strText.data());
				}
				else if (strValue == "MeasureMode")
				{
					sysSetting->measureSetting.nSelPtsMode = atoi(strText.data());
				}
				else if (strValue == "cadPickPointMode")
				{
					sysSetting->measureSetting.nPickPointMode = atoi(strText.data());
				}
				else if (strValue == "cadNearFieldSize")
				{
					sysSetting->measureSetting.dNearFieldSize = atof(strText.data());
				}
				//
				else if (strValue=="selPointSize")//
				{
					sysSetting->measureSetting.selPointSize = atof(strText.data());

				}
				else if (strValue=="MarkPointSize")
				{
					sysSetting->measureSetting.MarkPointSize = atof(strText.data());

				}
				else if (strValue=="selLineWidth")
				{
					sysSetting->measureSetting.selLineWidth = atof(strText.data());

				}
				else if (strValue=="meaLineWidth")
				{
					sysSetting->measureSetting.meaLineWidth = atof(strText.data());

				}

				childElement = childElement->NextSiblingElement();
			}

			// 719工程系统配置
			element = rootElement->FirstChildElement("i719Setting");
			if (element == NULL)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			// 遍历iScanSet所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "pcdShowDist")
				{
					sysSetting->i719Setting.pcdDistInPanoView = atof(strText.data());
				}

				if (strValue == "simCount")
				{
					sysSetting->i719Setting.ptNumThredInQuickCam = atoi(strText.c_str());
				}

				if (strValue == "simCount3D")
				{
					sysSetting->i719Setting.ptNumThredIn3DCam = atoi(strText.data());
				}

				if (strValue == "autoFilter")
				{
					sysSetting->i719Setting.AutoFilter = atoi(strText.data());
				}

				childElement = childElement->NextSiblingElement();
			}

			// iScan工程系统配置
			element = rootElement->FirstChildElement("iScanSetting");
			if (element == NULL)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			// 遍历iScanSet所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "panoFullSize")
				{
					sysSetting->iScanSetting.panoOnFullSize = atoi(strText.data());
				}

				if (strValue == "showByPos")
				{
					sysSetting->iScanSetting.showByPos = atoi(strText.c_str());
				}

				if (strValue == "loadByObb")
				{
					sysSetting->iScanSetting.loadByObbBox = atoi(strText.data());
				}

				if (strValue == "showPcdDist")
				{
					sysSetting->iScanSetting.pcdDistInPanoView = atof(strText.data());
				}

				if (strValue == "iScanDeviceType")
				{
					sysSetting->iScanSetting.m_nIScanDeviceType = atoi(strText.data());
				}

				if (strValue == "iScanColorRange")
				{
					sysSetting->iScanSetting.m_fColorRange = atof(strText.data());
				}
				if (strValue == "renderSimple")
				{
					sysSetting->iScanSetting.renderSimple = atoi(strText.c_str());
				}
				if (strValue == "renderSimple_3D")
				{
					sysSetting->iScanSetting.renderSimple_3D = atoi(strText.c_str());
				}

				childElement = childElement->NextSiblingElement();
			}


			// 渲染设置 
			element = rootElement->FirstChildElement("RenderSetting");
			if (element == NULL)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			// 遍历iScanSet所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "BkColor")
				{
					sysSetting->renderSetting.bkColor = atoi(strText.data());
				}

				if (strValue == "RenderStyle")
				{
					sysSetting->renderSetting.renderStyle = atoi(strText.c_str());
				}

				if (strValue == "PtSize")
				{
					sysSetting->renderSetting.ptSize = atoi(strText.data());
				}

				if (strValue == "MinIntenThre")
				{
					sysSetting->renderSetting.fMinIntenThre = atof(strText.data());
				}

				if (strValue == "MaxIntenThre")
				{
					sysSetting->renderSetting.fMaxIntenThre = atof(strText.data());
				}

				childElement = childElement->NextSiblingElement();
			}

			// 点云触点拟合面参数设置
			element = rootElement->FirstChildElement("FitRowColSetting");
			if (element == NULL)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			// 遍历FitRowColSetting所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "FitRow")
				{
					sysSetting->fitRowColSetting.nRow = atoi(strText.data());
				}

				if (strValue == "FitCol")
				{
					sysSetting->fitRowColSetting.nCol = atoi(strText.c_str());
				}

				if (strValue == "FitRowCut")
				{
					sysSetting->fitRowColSetting.nRowCut = atoi(strText.data());
				}

				if (strValue == "FitColCut")
				{
					sysSetting->fitRowColSetting.nColCut = atoi(strText.data());
				}

				if (strValue == "FitCount")
				{
					sysSetting->fitRowColSetting.nFitCount = atoi(strText.data());
				}

				childElement = childElement->NextSiblingElement();
			}

			//草图模式参数设置      by liuzhaoliang
			element = rootElement->FirstChildElement("DraftEdit");

			if (NULL == element)
			{
				return sysSetting;
			}
			childElement = element->FirstChildElement();

			//遍历draftedit所有子节点
			while(childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();
				if (strValue == "FilterByHeightMode")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.FilterMode = atoi(strText.data());
					}

				}
				if (strValue == "UpperHeight")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.uHeight = atof(strText.data());
					}

				}
				if (strValue == "LowerHeight")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.lHeight = atof(strText.data());
					}

				}
				if (strValue == "MaxHeight")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.MaxHeight = atof(strText.data());
					}

				}
				if (strValue == "MinHeight")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.MinHeight = atof(strText.data());
					}

				}
				if (strValue == "LoadThres")
				{
					if (childElement->GetText())
					{
						sysSetting->draftSetting.LoadSimpleThres = atoi(strText.data());
					}

				}
				childElement = childElement->NextSiblingElement();
			}

		}
	}

	return sysSetting;
}

void CHdSysSetting::saveSetting()
{
	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
	TiXmlElement* rootElement = new TiXmlElement("HD_3LS_SCENE_SETTING");

	doc.LinkEndChild(decl);
	doc.LinkEndChild(rootElement);

	char strText[128];
	//保存常规设置
	TiXmlElement* common = new TiXmlElement("commonSetting");
	rootElement->LinkEndChild(common);
	
	//TiXmlElement* IsFirstRun = new TiXmlElement("IsFirstRun");
	//common->LinkEndChild(IsFirstRun);
	//sprintf(strText, "%d", m_IsFirstRun);
	//TiXmlText* IsFirstRunText = new TiXmlText(strText);
	//IsFirstRun->LinkEndChild(IsFirstRunText);

	// 将用户界面信息写入系统配置
	TiXmlElement* UILanguage = new TiXmlElement("UILanguage");
	common->LinkEndChild(UILanguage);
	sprintf(strText, "%d", commonSetting.UILanguage);
	TiXmlText* UILanguageText = new TiXmlText(strText);
	UILanguage->LinkEndChild(UILanguageText);

	TiXmlElement* autoSaveTime = new TiXmlElement("autoSaveTime");
	common->LinkEndChild(autoSaveTime);
	sprintf(strText, "%d", commonSetting.autoSaveTime);
	TiXmlText* autoSaveTimeText = new TiXmlText(strText);
	autoSaveTime->LinkEndChild(autoSaveTimeText);

	TiXmlElement* renderSimple = new TiXmlElement("renderSimple");
	common->LinkEndChild(renderSimple);
	sprintf(strText, "%d", commonSetting.renderSimple);
	TiXmlText* simpleCountText = new TiXmlText(strText);
	renderSimple->LinkEndChild(simpleCountText);

	TiXmlElement* loadSimple = new TiXmlElement("loadSimple");
	common->LinkEndChild(loadSimple);
	sprintf(strText, "%d", commonSetting.loadSimple);
	TiXmlText* loadSimpleText = new TiXmlText(strText);
	loadSimple->LinkEndChild(loadSimpleText);

	TiXmlElement* showNormal = new TiXmlElement("showNormal");
	common->LinkEndChild(showNormal);
	sprintf(strText, "%d", commonSetting.showNormal);
	TiXmlText* showNormalText = new TiXmlText(strText);
	showNormal->LinkEndChild(showNormalText);

	TiXmlElement* editMode = new TiXmlElement("editMode");
	common->LinkEndChild(editMode);
	sprintf(strText, "%d", commonSetting.editMode);
	TiXmlText* editModeText = new TiXmlText(strText);
	editMode->LinkEndChild(editModeText);

	TiXmlElement* showVector = new TiXmlElement("showVector");
	common->LinkEndChild(showVector);
	sprintf(strText, "%d", commonSetting.showVector);
	TiXmlText* showVectorText = new TiXmlText(strText);
	showVector->LinkEndChild(showVectorText);

	TiXmlElement* space_filter_dist = new TiXmlElement("spaceFilterDist");
	common->LinkEndChild(space_filter_dist);
	sprintf(strText, "%f", commonSetting.space_filter_dist);
	TiXmlText* space_filter_dist_txt = new TiXmlText(strText);
	space_filter_dist->LinkEndChild(space_filter_dist_txt);

	TiXmlElement* load_simple_mode = new TiXmlElement("loadSimpleMode");
	common->LinkEndChild(load_simple_mode);
	sprintf(strText, "%d", commonSetting.load_simple_mode);
	TiXmlText* load_simple_mode_txt = new TiXmlText(strText);
	load_simple_mode->LinkEndChild(load_simple_mode_txt);
	

	//保存选择设置
	TiXmlElement* selection = new TiXmlElement("selectionSetting");
	rootElement->LinkEndChild(selection);
	//selection->SetAttribute("inside",selectionSetting.inside);
	//selection->SetAttribute("selectMode",selectionSetting.selectMode);
	TiXmlElement* inside = new TiXmlElement("inside");
	selection->LinkEndChild(inside);
	sprintf(strText, "%d", selectionSetting.inside);
	TiXmlText* insideText = new TiXmlText(strText);
	inside->LinkEndChild(insideText);

	TiXmlElement* selectMode = new TiXmlElement("selectMode");
	selection->LinkEndChild(selectMode);
	sprintf(strText, "%d", selectionSetting.selectMode);
	TiXmlText* selectModeText = new TiXmlText(strText);
	selectMode->LinkEndChild(selectModeText);

	////保存导入设置
	//TiXmlElement* import = new TiXmlElement("importSetting");
	//rootElement->LinkEndChild(import);

	//TiXmlElement* units = new TiXmlElement("units");
	//import->LinkEndChild(units);
	//sprintf(strText, "%d", (int)importSetting.units);
	//TiXmlText* unitsText = new TiXmlText(strText);
	//units->LinkEndChild(unitsText);

	//TiXmlElement* maxRow = new TiXmlElement("maxRow");
	//import->LinkEndChild(maxRow);
	//sprintf(strText, "%d", importSetting.maxRow);
	//TiXmlText* maxRowText = new TiXmlText(strText);
	//maxRow->LinkEndChild(maxRowText);

	//TiXmlElement* maxCol = new TiXmlElement("maxCol");
	//import->LinkEndChild(maxCol);
	//sprintf(strText, "%d", importSetting.maxCol);
	//TiXmlText* maxColText = new TiXmlText(strText);
	//maxCol->LinkEndChild(maxColText);

	//TiXmlElement* maxIntensity = new TiXmlElement("maxIntensity");
	//import->LinkEndChild(maxIntensity);
	//sprintf(strText, "%d", importSetting.maxIntensity);
	//TiXmlText* maxIntensityText = new TiXmlText(strText);
	//maxIntensity->LinkEndChild(maxIntensityText);

	////保存导出属性
	//TiXmlElement* exportSet = new TiXmlElement("exportSetting");
	//rootElement->LinkEndChild(exportSet);

	//TiXmlElement* maxDistance = new TiXmlElement("maxDistance");
	//exportSet->LinkEndChild(maxDistance);
	//sprintf(strText, "%f", exportSetting.maxDistance);
	//TiXmlText* maxDistanceText = new TiXmlText(strText);
	//maxDistance->LinkEndChild(maxDistanceText);

	//TiXmlElement* minDistance = new TiXmlElement("minDistance");
	//exportSet->LinkEndChild(minDistance);
	//sprintf(strText, "%f", exportSetting.minDistance);
	//TiXmlText* minDistanceText = new TiXmlText(strText);
	//minDistance->LinkEndChild(minDistanceText);

	//TiXmlElement* rowSimple = new TiXmlElement("rowSimple");
	//exportSet->LinkEndChild(rowSimple);
	//sprintf(strText, "%d", exportSetting.rowSimple);
	//TiXmlText* rowSimpleText = new TiXmlText(strText);
	//rowSimple->LinkEndChild(rowSimpleText);

	//TiXmlElement* colSimple = new TiXmlElement("colSimple");
	//exportSet->LinkEndChild(colSimple);
	//sprintf(strText, "%d", exportSetting.colSimple);
	//TiXmlText* colSimpleText = new TiXmlText(strText);
	//colSimple->LinkEndChild(colSimpleText);

	//保存配置属性
	TiXmlElement* match = new TiXmlElement("matchSetting");
	rootElement->LinkEndChild(match);

	TiXmlElement* sphereRadius = new TiXmlElement("sphereRadius");
	match->LinkEndChild(sphereRadius);
	sprintf(strText, "%f", matchSetting.sphereRadius);
	TiXmlText* sphereRadiusText = new TiXmlText(strText);
	sphereRadius->LinkEndChild(sphereRadiusText);

	//TiXmlElement* radiusStdDev = new TiXmlElement("radiusStdDev");
	//match->LinkEndChild(radiusStdDev);
	//sprintf(strText, "%f", matchSetting.radiusStdDev);
	//TiXmlText* radiusStdDevText = new TiXmlText(strText);
	//radiusStdDev->LinkEndChild(radiusStdDevText);

	//TiXmlElement* blockSize = new TiXmlElement("blockSize");
	//match->LinkEndChild(blockSize);
	//sprintf(strText, "%d", matchSetting.blockSize);
	//TiXmlText* blockSizeText = new TiXmlText(strText);
	//blockSize->LinkEndChild(blockSizeText);

	TiXmlElement* chessboardsize = new TiXmlElement("chessboardSize");
	match->LinkEndChild(chessboardsize);
	sprintf(strText, "%f", matchSetting.chessboardSize);
	TiXmlText* chessboardSizeText = new TiXmlText(strText);
	chessboardsize->LinkEndChild(chessboardSizeText);

	TiXmlElement* elemReportPath = new TiXmlElement("reportPath");
	match->LinkEndChild(elemReportPath);
	elemReportPath->LinkEndChild(new TiXmlText(matchSetting.strReportPath.data()));

	//保存过滤属性
	TiXmlElement* filter = new TiXmlElement("filterSetting");
	rootElement->LinkEndChild(filter);

	TiXmlElement* maxDistance = new TiXmlElement("maxDistance");
	filter->LinkEndChild(maxDistance);
	sprintf(strText, "%f", filterSetting.maxDistance);
	TiXmlText* maxDistanceText = new TiXmlText(strText);
	maxDistance->LinkEndChild(maxDistanceText);

	TiXmlElement* maxIntensity = new TiXmlElement("maxIntensity");
	filter->LinkEndChild(maxIntensity);
	sprintf(strText, "%d", filterSetting.maxIntensity);
	TiXmlText* maxIntensityText = new TiXmlText(strText);
	maxIntensity->LinkEndChild(maxIntensityText);

	TiXmlElement *minIntensity = new TiXmlElement("minIntensity");
	filter->LinkEndChild(minIntensity);
	sprintf(strText, "%d", filterSetting.minIntensity);
	TiXmlText *minIntensityText = new TiXmlText(strText);
	minIntensity->LinkEndChild(minIntensityText);

	TiXmlElement* maxGridSize = new TiXmlElement("maxGridSize");
	filter->LinkEndChild(maxGridSize);
	sprintf(strText, "%f", filterSetting.MaxGridSize);
	TiXmlText * maxGridSizeText = new TiXmlText(strText);
	maxGridSize->LinkEndChild(maxGridSizeText);

	TiXmlElement* maxTriaSIzeLen = new TiXmlElement("maxTriaSIzeLen");
	filter->LinkEndChild(maxTriaSIzeLen);
	sprintf(strText, "%f", filterSetting.MaxTriaSIzeLen);
	TiXmlText *maxTriaSIzeLenText = new TiXmlText(strText);
	maxTriaSIzeLen->LinkEndChild(maxTriaSIzeLenText);

	// 是否叠加过滤
	TiXmlElement* bAddFilter = new TiXmlElement("bAddFilter");
	filter->LinkEndChild(bAddFilter);
	sprintf(strText, "%d", filterSetting.AddFilter);
	TiXmlText *AddFilterText = new TiXmlText(strText);
	bAddFilter->LinkEndChild(AddFilterText);

	// 质心选取设置
	TiXmlElement* centriodPick = new TiXmlElement("centriodPickSetting");
	rootElement->LinkEndChild(centriodPick);
	centriodPick->SetAttribute("minIntensity",centrioidPicker.minIntensity);
	centriodPick->SetAttribute("maxIntensity",centrioidPicker.maxIntensity);
	centriodPick->SetDoubleAttribute("radius",centrioidPicker.radius);
	centriodPick->SetAttribute("pickCtrlPointMode",centrioidPicker.pickCtrlPtMode);
	centriodPick->SetAttribute("sltCtrlPtZoomWnd",centrioidPicker.openZoomWnd);
	centriodPick->SetAttribute("ZoomWndPcdRadius",centrioidPicker.zoomPcdRadius);

	// 测量设置
	TiXmlElement* selMeasureSet = new TiXmlElement("measureSetting");
	rootElement->LinkEndChild(selMeasureSet);
	
	// 选中点颜色
	char strSelPointColor[128] = {0};
	TiXmlElement* selPtColorElement = new TiXmlElement("selPointColor");
	selMeasureSet->LinkEndChild(selPtColorElement);
	sprintf(strSelPointColor,"%ld",measureSetting.selPointColor);
	TiXmlText* pSelPtText = new TiXmlText(strSelPointColor);
	selPtColorElement->LinkEndChild(pSelPtText);

	// 选择线颜色
	char strSelPLineColor[128] = {0};
	TiXmlElement* selPLineColorElement = new TiXmlElement("selPLineColor");
	selMeasureSet->LinkEndChild(selPLineColorElement);
	sprintf(strSelPLineColor,"%ld",measureSetting.selPLineColor);
	TiXmlText* pSelPLineText = new TiXmlText(strSelPLineColor);
	selPLineColorElement->LinkEndChild(pSelPLineText);

	// 测量线颜色
	char strMeasLineColor[128] = {0};
	TiXmlElement* measLineElement = new TiXmlElement("measureLineColor");
	selMeasureSet->LinkEndChild(measLineElement);
	sprintf(strMeasLineColor,"%ld",measureSetting.measureLineColor);
	TiXmlText* pMeasLineText = new TiXmlText(strMeasLineColor);
	measLineElement->LinkEndChild(pMeasLineText);

	// 绘至CAD点颜色
	char strCadPointColor[128] = {0};
	TiXmlElement* cadPointElement = new TiXmlElement("cadPointColor");
	selMeasureSet->LinkEndChild(cadPointElement);
	sprintf(strCadPointColor,"%ld",measureSetting.cadPointColor);
	TiXmlText* pCadPointText = new TiXmlText(strCadPointColor);
	cadPointElement->LinkEndChild(pCadPointText);

	// 绘制点到CAD，选择点模式
	char strPickPointMode[64] = {0};
	TiXmlElement* cadPickPointModeElement = new TiXmlElement("cadPickPointMode");
	selMeasureSet->LinkEndChild(cadPickPointModeElement);
	sprintf(strPickPointMode,"%d",measureSetting.nPickPointMode);
	TiXmlText* pPickPointText = new TiXmlText(strPickPointMode);
	cadPickPointModeElement->LinkEndChild(pPickPointText);

	// 发送到CAD模式
	char strSendMode[64] = {0};
	TiXmlElement* draw2CADModeElement = new TiXmlElement("draw2CADMode");
	selMeasureSet->LinkEndChild(draw2CADModeElement);
	sprintf(strSendMode,"%d",measureSetting.nSendMode);
	TiXmlText* pDraw2CADText = new TiXmlText(strSendMode);
	draw2CADModeElement->LinkEndChild(pDraw2CADText);

	// 测量工具以及选择工具选点模式，0 - 针对内存中的点，1 - 针对显示的点 fengjing 20140905
	char strSelPtsMode[64] = {0};
	TiXmlElement* MeasureModeElement = new TiXmlElement("MeasureMode");
	selMeasureSet->LinkEndChild(MeasureModeElement);
	sprintf(strSelPtsMode,"%d",measureSetting.nSelPtsMode);
	TiXmlText* pMeasureModeText = new TiXmlText(strSelPtsMode);
	MeasureModeElement->LinkEndChild(pMeasureModeText);

// 	// 绘制点到CAD，平均点模式
// 	char strIsAveragePoint[32] = {0};
// 	TiXmlElement* cadAveragePointElement = new TiXmlElement("cadAvergePoint");
// 	selMeasureSet->LinkEndChild(cadAveragePointElement);
// 	sprintf(strIsAveragePoint,"%d",measureSetting.nAveragePoint);
// 	TiXmlText* pAveragePointText = new TiXmlText(strIsAveragePoint);
// 	cadAveragePointElement->LinkEndChild(pAveragePointText);

	// 平均点模式,临域范围
	TiXmlElement* NearFieldSizeElement = new TiXmlElement("cadNearFieldSize");
	selMeasureSet->LinkEndChild(NearFieldSizeElement);
	sprintf(strText, "%lf", measureSetting.dNearFieldSize);
	TiXmlText* NearFieldSizeText = new TiXmlText(strText);
	NearFieldSizeElement->LinkEndChild(NearFieldSizeText);

	//选中点的大小
	char strSelPointSize[64] = {0};
	TiXmlElement* selSeaPointSize = new TiXmlElement("selPointSize");
	selMeasureSet->LinkEndChild(selSeaPointSize);
	sprintf(strSelPointSize,"%d",measureSetting.selPointSize);
	TiXmlText* pSelPointSizeText = new TiXmlText(strSelPointSize);
	selSeaPointSize->LinkEndChild(pSelPointSizeText);

	//标记点的大小
	char strMarkPointSize[64] = {0};
	TiXmlElement* toSeaMarkPointSize = new TiXmlElement("MarkPointSize");
	selMeasureSet->LinkEndChild(toSeaMarkPointSize);
	sprintf(strMarkPointSize,"%d",measureSetting.MarkPointSize);
	TiXmlText* pMarkPointSizeText = new TiXmlText(strMarkPointSize);
	toSeaMarkPointSize->LinkEndChild(pMarkPointSizeText);

	//选择线的宽度

	char strLineWidth[64] = {0};
	TiXmlElement* selSeaLineWidth = new TiXmlElement("selLineWidth");
	selMeasureSet->LinkEndChild(selSeaLineWidth);
	sprintf(strLineWidth,"%d",measureSetting.selLineWidth);
	TiXmlText* pSeaLineWidthText = new TiXmlText(strLineWidth);
	selSeaLineWidth->LinkEndChild(pSeaLineWidthText);

	//测量线的宽度

	char strMeaLineWidth[64] = {0};
	TiXmlElement* selMeaSeaLineWidth = new TiXmlElement("meaLineWidth");
	selMeasureSet->LinkEndChild(selMeaSeaLineWidth);
	sprintf(strMeaLineWidth,"%d",measureSetting.meaLineWidth);
	TiXmlText* pMeaSeaLineWidthText = new TiXmlText(strMeaLineWidth);
	selMeaSeaLineWidth->LinkEndChild(pMeaSeaLineWidthText);

	// 719
	TiXmlElement* i719Set = new TiXmlElement("i719Setting");
	rootElement->LinkEndChild(i719Set);

	// 快速浏览时点云显示距离
	TiXmlElement* pcdShowDist = new TiXmlElement("pcdShowDist");
	i719Set->LinkEndChild(pcdShowDist);
	sprintf(strText, "%.1f", i719Setting.pcdDistInPanoView);
	TiXmlText* pcdShowText = new TiXmlText(strText);
	pcdShowDist->LinkEndChild(pcdShowText);

	//快速相机下点云阈值范围参数
	TiXmlElement* simCount = new TiXmlElement("simCount");
	i719Set->LinkEndChild(simCount);
	sprintf(strText, "%d", i719Setting.ptNumThredInQuickCam);
	TiXmlText* simCountText = new TiXmlText(strText);
	simCount->LinkEndChild(simCountText);
	
	//相机下点云阈值范围参数
	TiXmlElement* simCount3D = new TiXmlElement("simCount3D");
	i719Set->LinkEndChild(simCount3D);
	sprintf(strText, "%d", i719Setting.ptNumThredIn3DCam);
	TiXmlText* simCount3DText = new TiXmlText(strText);
	simCount3D->LinkEndChild(simCount3DText);

	// 是否实时过滤
	TiXmlElement* autoFilter = new TiXmlElement("autoFilter");
	i719Set->LinkEndChild(autoFilter);
	sprintf(strText, "%d", i719Setting.AutoFilter);
	TiXmlText* autoFilterText = new TiXmlText(strText);
	autoFilter->LinkEndChild(autoFilterText);

	// iScan工程配置
	TiXmlElement* iScanSet = new TiXmlElement("iScanSetting");
	rootElement->LinkEndChild(iScanSet);

	// iScan全景视图原图显示
	TiXmlElement* panoFullSize = new TiXmlElement("panoFullSize");
	iScanSet->LinkEndChild(panoFullSize);
	sprintf(strText, "%d", iScanSetting.panoOnFullSize);
	TiXmlText* panoFullText = new TiXmlText(strText);
	panoFullSize->LinkEndChild(panoFullText);

	TiXmlElement* showByPos = new TiXmlElement("showByPos");
	iScanSet->LinkEndChild(showByPos);
	sprintf(strText,"%d",iScanSetting.showByPos);
	TiXmlText* byPosText = new TiXmlText(strText);
	showByPos->LinkEndChild(byPosText);

	TiXmlElement* loadByPos = new TiXmlElement("loadByObb");
	iScanSet->LinkEndChild(loadByPos);
	sprintf(strText,"%d",iScanSetting.loadByObbBox);
	TiXmlText* loadByPosText = new TiXmlText(strText);
	loadByPos->LinkEndChild(loadByPosText);

	TiXmlElement* DistInPanoView = new TiXmlElement("showPcdDist");
	iScanSet->LinkEndChild(DistInPanoView);
	sprintf(strText,"%.1f",iScanSetting.pcdDistInPanoView);
	TiXmlText* distInPanoViewText = new TiXmlText(strText);
	DistInPanoView->LinkEndChild(distInPanoViewText);

	TiXmlElement* iScanDeviceType = new TiXmlElement("iScanDeviceType");
	iScanSet->LinkEndChild(iScanDeviceType);
	sprintf(strText,"%d",iScanSetting.m_nIScanDeviceType);
	TiXmlText* iScanDeviceTypeText = new TiXmlText(strText);
	iScanDeviceType->LinkEndChild(iScanDeviceTypeText);

	TiXmlElement* iScanColorRange = new TiXmlElement("iScanColorRange");
	iScanSet->LinkEndChild(iScanColorRange);
	sprintf(strText,"%.1f",iScanSetting.m_fColorRange);
	TiXmlText* iScanColorRangeText = new TiXmlText(strText);
	iScanColorRange->LinkEndChild(iScanColorRangeText);

	TiXmlElement* iScanRenderSimple = new TiXmlElement("renderSimple");
	iScanSet->LinkEndChild(iScanRenderSimple);
	sprintf(strText,"%.1f",iScanSetting.renderSimple);
	TiXmlText* iScanRenderSimpleText = new TiXmlText(strText);
	iScanRenderSimple->LinkEndChild(iScanRenderSimpleText);

	TiXmlElement* iScanRenderSimple_3D = new TiXmlElement("renderSimple_3D");
	iScanSet->LinkEndChild(iScanRenderSimple_3D);
	sprintf(strText,"%.1f",iScanSetting.renderSimple_3D);
	TiXmlText* iScanRenderSimple_3DText = new TiXmlText(strText);
	iScanRenderSimple_3D->LinkEndChild(iScanRenderSimple_3DText);

	// 渲染设置
	TiXmlElement* RenderSet = new TiXmlElement("RenderSetting");
	rootElement->LinkEndChild(RenderSet);

	// 背景色
	TiXmlElement* BkColor = new TiXmlElement("BkColor");
	RenderSet->LinkEndChild(BkColor);
	sprintf(strText,"%d",renderSetting.bkColor);
	TiXmlText* BkColortext = new TiXmlText(strText);
	BkColor->LinkEndChild(BkColortext);

	// 渲染方式
	TiXmlElement* RenderStyle = new TiXmlElement("RenderStyle");
	RenderSet->LinkEndChild(RenderStyle);
	sprintf(strText,"%d",renderSetting.renderStyle);
	TiXmlText* Rendertext = new TiXmlText(strText);
	RenderStyle->LinkEndChild(Rendertext);

	// 点大小
	TiXmlElement* PtSize = new TiXmlElement("PtSize");
	RenderSet->LinkEndChild(PtSize);
	sprintf(strText,"%d",renderSetting.ptSize);
	TiXmlText* PtSizetext = new TiXmlText(strText);
	PtSize->LinkEndChild(PtSizetext);

	// 强度拉伸极小值
	TiXmlElement* MinIntenThre = new TiXmlElement("MinIntenThre");
	RenderSet->LinkEndChild(MinIntenThre);
	sprintf(strText,"%.2f",renderSetting.fMinIntenThre);
	TiXmlText* MinIntenThretext = new TiXmlText(strText);
	MinIntenThre->LinkEndChild(MinIntenThretext);

	// 强度拉伸极大值
	TiXmlElement* MaxIntenThre = new TiXmlElement("MaxIntenThre");
	RenderSet->LinkEndChild(MaxIntenThre);
	sprintf(strText,"%.2f",renderSetting.fMaxIntenThre);
	TiXmlText* MaxIntenThretext = new TiXmlText(strText);
	MaxIntenThre->LinkEndChild(MaxIntenThretext);

	// 点云触点拟合面参数设置
	TiXmlElement* FitSet = new TiXmlElement("FitRowColSetting");
	rootElement->LinkEndChild(FitSet);

	// 行数
	TiXmlElement* FitRow = new TiXmlElement("FitRow");
	FitSet->LinkEndChild(FitRow);
	sprintf(strText,"%d",fitRowColSetting.nRow);
	TiXmlText* FitRowtext = new TiXmlText(strText);
	FitRow->LinkEndChild(FitRowtext);

	// 列数
	TiXmlElement* FitCol = new TiXmlElement("FitCol");
	FitSet->LinkEndChild(FitCol);
	sprintf(strText,"%d",fitRowColSetting.nCol);
	TiXmlText* FitColtext = new TiXmlText(strText);
	FitCol->LinkEndChild(FitColtext);

	// 每次减少行数
	TiXmlElement* FitRowCut = new TiXmlElement("FitRowCut");
	FitSet->LinkEndChild(FitRowCut);
	sprintf(strText,"%d",fitRowColSetting.nRowCut);
	TiXmlText* FitRowCuttext = new TiXmlText(strText);
	FitRowCut->LinkEndChild(FitRowCuttext);

	// 每次减少列数
	TiXmlElement* FitColCut = new TiXmlElement("FitColCut");
	FitSet->LinkEndChild(FitColCut);
	sprintf(strText,"%d",fitRowColSetting.nColCut);
	TiXmlText* FitColCuttext = new TiXmlText(strText);
	FitColCut->LinkEndChild(FitColCuttext);

	// 拟合的次数
	TiXmlElement* FitCount = new TiXmlElement("FitCount");
	FitSet->LinkEndChild(FitCount);
	sprintf(strText,"%d",fitRowColSetting.nFitCount);
	TiXmlText* FitCounttext = new TiXmlText(strText);
	FitCount->LinkEndChild(FitCounttext);

	//草图编辑过滤模式参数设置
	TiXmlElement* DraftEdit = new TiXmlElement("DraftEdit");
	rootElement->LinkEndChild(DraftEdit);

	//高度过滤模式
	TiXmlElement* FilterByHeightMode = new TiXmlElement("FilterByHeightMode");
	DraftEdit->LinkEndChild(FilterByHeightMode);
	sprintf(strText,"%d",draftSetting.FilterMode);
	TiXmlText* FilterByHeightModeText = new TiXmlText(strText);
	FilterByHeightMode->LinkEndChild(FilterByHeightModeText);

	//高于模式下高度参数
	TiXmlElement* UpperHeight = new TiXmlElement("UpperHeight");
	DraftEdit->LinkEndChild(UpperHeight);
	sprintf(strText,"%f",draftSetting.uHeight);
	TiXmlText* UpperHeightText = new TiXmlText(strText);
	UpperHeight->LinkEndChild(UpperHeightText);

	//低于模式下高度参数
	TiXmlElement* LowerHeight = new TiXmlElement("LowerHeight");
	DraftEdit->LinkEndChild(LowerHeight);
	sprintf(strText,"%f",draftSetting.lHeight);
	TiXmlText* LowerHeightText = new TiXmlText(strText);
	LowerHeight->LinkEndChild(LowerHeightText);

	//介于模式下高度上限参数
	TiXmlElement* MaxHeight = new TiXmlElement("MaxHeight");
	DraftEdit->LinkEndChild(MaxHeight);
	sprintf(strText,"%f",draftSetting.MaxHeight);
	TiXmlText* MaxHeightText = new TiXmlText(strText);
	MaxHeight->LinkEndChild(MaxHeightText);

	//介于模式下高度下限参数
	TiXmlElement* MinHeight = new TiXmlElement("MinHeight");
	DraftEdit->LinkEndChild(MinHeight);
	sprintf(strText,"%f",draftSetting.MinHeight);
	TiXmlText* MinHeightText = new TiXmlText(strText);
	MinHeight->LinkEndChild(MinHeightText);

	//介于模式下高度下限参数
	TiXmlElement* LoadThres = new TiXmlElement("LoadThres");
	DraftEdit->LinkEndChild(LoadThres);
	sprintf(strText,"%d",draftSetting.LoadSimpleThres);
	TiXmlText* LoadThresText = new TiXmlText(strText);
	LoadThres->LinkEndChild(LoadThresText);


	string filePath = getCurrentDir();
	filePath += "sysSetting.xml";
	doc.SaveFile(filePath.c_str());
}

}