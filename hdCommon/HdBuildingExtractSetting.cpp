#include "StdAfx.h"
#include "HdBuildingExtractSetting.h"
#include "..\hdCore\tinyxml.h"
#include "hdSceneStr.h"
namespace hd
{
	

	// 读取配置文件HdBuildingExtractSetting.xml中的建筑物面片提取信息
	bool CHdBuildingExtractSetting::ReadHdBuildingExtractSetting()
	{

		
		TiXmlDocument doc(m_strConfigPath.c_str());

		// 打开失败返回
		if (!doc.LoadFile())
		{
			return false;
		}

	
		TiXmlElement* rootElement = doc.RootElement();

		if (rootElement)
		{

			string strValue;
			string strText;

			// 读取建筑物点云提取参数
			TiXmlElement* element = rootElement->FirstChildElement("BuildingPcdExtractFactor");

			// 读取失败返回
			if (!element)
			{
				return false;
			}

			TiXmlElement* childElement = element->FirstChildElement();
			
			// 读取失败返回
			if (!childElement)
			{
				return false;
			}

			// 遍历存取建筑物点云提取参数
			while(childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "AnalystLoop")
				{
					m_iAnalystLoop = atoi(strText.c_str());
				}	

				if (strValue == "SearchRange")
				{
					m_fSearchRange = (float)atof(strText.c_str());
				}

				if (strValue == "Planar")
				{
					m_dPlanar = atof(strText.c_str());
				}

				if (strValue == "nz")
				{
					m_dnz = atof(strText.c_str());
				}

				childElement = childElement->NextSiblingElement();

			}  // 遍历存取建筑物点云提取参数

			// 读取建筑物轮廓线模型提取参数
			element = rootElement->FirstChildElement("BuildingPcdLineExtFactor");
			
			// 为空返回
			if (element == NULL)
			{
				return false;
			}

			childElement = element->FirstChildElement();

			// 如果为空，则返回
			if (!childElement)
			{
				return false;
			}

			// 遍历建筑物点云轮廓线所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "LineNum")
				{
				    m_ilineNum = atoi(strText.c_str());
				}

				if (strValue == "DsitThreshold")
				{
					m_dsit_threshold = atof(strText.c_str());
				}

				childElement = childElement->NextSiblingElement();
			} // 遍历建筑物点云轮廓线所有子节点

			// 遍历建筑物点云面片对象所有节点
			element = rootElement->FirstChildElement("BuildingFaceObjFactor");
		
			// 为空，返回
			if (element == NULL)
			{
				return false ;
			}

			childElement = element->FirstChildElement();

			// 为空，返回
			if (!childElement)
			{
				return false;
			}

			// 遍历建筑物点云面片所有子节点
			while (childElement)
			{
				strValue = childElement->Value();
				strText = childElement->GetText();

				if (strValue == "FitTol")
				{
					m_dfitTol = atof(strText.c_str());
				}

				if (strValue == "SimpleTol")
				{
				    m_dsimpleTol = atof(strText.c_str());
				}

				if (strValue == "ObjDisTol")
				{
					m_dobjDisTol = atof(strText.c_str());
				}

				if (strValue == "TolMinPtCount")
				{
					m_itolMinPtCount = atoi(strText.c_str());
				}

				if (strValue == "ObjDx")
				{
					m_fobjDx = (float)atof(strText.c_str());
				}

				if (strValue == "ObjDy")
				{
					m_fobjDy = (float)atof(strText.c_str());
				}

				if (strValue == "ObjDz")
				{
					m_fobjDz = (float)atof(strText.c_str());
				}

				childElement = childElement->NextSiblingElement();
			} 	// 遍历建筑物点云面片所有子节点

		}
		return true;
	}

	// 写入插件路径信息至配置文件HdBuildingExtractSetting.xml
	void CHdBuildingExtractSetting::WriteHdBuildingExtractSetting()
	{
		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );

		// 根结点
		TiXmlElement* rootElement = new TiXmlElement("HD_3LS_SCENE_BLDEXTCT_SETTING");

		doc.LinkEndChild(decl);
		doc.LinkEndChild(rootElement);


		char strText[128];

		// 保存建筑物点云提取参数
		TiXmlElement* BldPcdExtractFactor= new TiXmlElement("BuildingPcdExtractFactor");
		rootElement->LinkEndChild(BldPcdExtractFactor);

		//分析圈数
		TiXmlElement* AnalystLoop = new TiXmlElement("AnalystLoop");
		BldPcdExtractFactor->LinkEndChild(AnalystLoop);
		sprintf(strText, "%d", m_iAnalystLoop);
		TiXmlText* AnalystLoopText = new TiXmlText(strText);
		AnalystLoop->LinkEndChild(AnalystLoopText);

		//临近点搜索范围
		TiXmlElement* SearchRange = new TiXmlElement("SearchRange");
		BldPcdExtractFactor->LinkEndChild(SearchRange);
		sprintf(strText, "%f", m_fSearchRange);
		TiXmlText*SearchRangeText = new TiXmlText(strText);
		SearchRange->LinkEndChild(SearchRangeText);

		//平面性阈值
		TiXmlElement* Planar = new TiXmlElement("Planar");
		BldPcdExtractFactor->LinkEndChild(Planar);
		sprintf(strText,"%lf",m_dPlanar);
		TiXmlText* PlanarText = new TiXmlText(strText);
		Planar->LinkEndChild(PlanarText);

		// 平面Z方向法向量阈值
		TiXmlElement* nz = new TiXmlElement("nz");
		BldPcdExtractFactor->LinkEndChild(nz);
		sprintf(strText,"%lf",m_dnz);
		TiXmlText* nzText = new TiXmlText(strText);
		nz->LinkEndChild(nzText);

		//////////////////////////////////////////////////////////////////////////

		// 保存 建筑物点云轮廓线模型提取参数
		TiXmlElement* BildLineExtModelFactor = new TiXmlElement("BuildingPcdLineExtFactor");
		rootElement->LinkEndChild(BildLineExtModelFactor);

	   // 建筑物轮廓线内点个数
		TiXmlElement*LineNum = new TiXmlElement("LineNum");
		BildLineExtModelFactor->LinkEndChild(LineNum);
		sprintf(strText, "%d", m_ilineNum);
		TiXmlText* LineNumText = new TiXmlText(strText);
		LineNum->LinkEndChild(LineNumText);

		// 点是否属于直线内点的距离阈值
        TiXmlElement*dsit_threshold = new TiXmlElement("DsitThreshold");
		BildLineExtModelFactor->LinkEndChild(dsit_threshold);
		sprintf(strText, "%lf", m_dsit_threshold);
		TiXmlText* dsit_thresholdText = new TiXmlText(strText);
		dsit_threshold->LinkEndChild(dsit_thresholdText);

		//////////////////////////////////////////////////////////////////////////

		// 保存建筑物面片对象参数
		TiXmlElement* BildFaceObjFactor = new TiXmlElement("BuildingFaceObjFactor");
		rootElement->LinkEndChild(BildFaceObjFactor);

		// 建筑物边线拟合节点的阈值
		TiXmlElement*FitTol = new TiXmlElement("FitTol");
		BildFaceObjFactor->LinkEndChild(FitTol);
		sprintf(strText, "%lf", m_dfitTol);
		TiXmlText*FitTolText = new TiXmlText(strText);
		FitTol->LinkEndChild(FitTolText);

		// 建筑物轮廓线简化阈值
		TiXmlElement*SimpleTol = new TiXmlElement("SimpleTol");
		BildFaceObjFactor->LinkEndChild(SimpleTol);
		sprintf(strText, "%lf", m_dsimpleTol);
		TiXmlText*SimpleTolText = new TiXmlText(strText);
		SimpleTol->LinkEndChild(SimpleTolText);

		// 面片对象间隔阈值
		TiXmlElement*ObjDisTol = new TiXmlElement("ObjDisTol");
		BildFaceObjFactor->LinkEndChild(ObjDisTol);
		sprintf(strText, "%lf", m_dobjDisTol);
		TiXmlText*ObjDisTolText = new TiXmlText(strText);
		ObjDisTol->LinkEndChild(ObjDisTolText);

		// 面片对象中所含最小点数
		TiXmlElement*TolMinPtCount = new TiXmlElement("TolMinPtCount");
		BildFaceObjFactor->LinkEndChild(TolMinPtCount);
		sprintf(strText, "%d", m_itolMinPtCount);
		TiXmlText*TolMinPtCountText = new TiXmlText(strText);
		TolMinPtCount->LinkEndChild(TolMinPtCountText);

		// 面片对象尺寸X阈值
		TiXmlElement*ObjDx = new TiXmlElement("ObjDx");
		BildFaceObjFactor->LinkEndChild(ObjDx);
		sprintf(strText, "%f", m_fobjDx);
		TiXmlText*ObjDxText = new TiXmlText(strText);
		ObjDx->LinkEndChild(ObjDxText);

		//  面片对象尺寸Y阈值
		TiXmlElement*ObjDy = new TiXmlElement("ObjDy");
		BildFaceObjFactor->LinkEndChild(ObjDy);
		sprintf(strText, "%f", m_fobjDy);
		TiXmlText*ObjDyText = new TiXmlText(strText);
		ObjDy->LinkEndChild(ObjDyText);

		// 面片对象尺寸Z阈值
		TiXmlElement*ObjDz = new TiXmlElement("ObjDz");
		BildFaceObjFactor->LinkEndChild(ObjDz);
		sprintf(strText, "%f", m_fobjDz);
		TiXmlText*ObjDzText = new TiXmlText(strText);
		ObjDz->LinkEndChild(ObjDzText);
	
		// 保存参数设置到配置文件
		doc.SaveFile(m_strConfigPath.c_str());
	}

}


