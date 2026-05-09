#include "StdAfx.h"
#include "hnSysSetting.h"
#include "..\..\hdCommon\hdCommon.h"
#include "..\..\hdCore\tinyxml.h"
#include <io.h>
#include <string>
using namespace std;

namespace hnApp
{
	hnSysSetting* hnSysSetting::m_pSysSetting = NULL;

	hnSysSetting::hnSysSetting(void)
		:m_bOpen(false)
	{
	}


	hnSysSetting::~hnSysSetting(void)
	{
	}

	// 获取单实例
	hnSysSetting* hnSysSetting::getSetting()
	{
		if (!m_pSysSetting)
		{
			m_pSysSetting = new hnSysSetting();
		}

		return m_pSysSetting;
	}

	// 析构单实例
	void hnSysSetting::destroySetting()
	{
		if (m_pSysSetting)
		{
			delete m_pSysSetting;
			m_pSysSetting = NULL;
		}
	}

	// 获取参数
	hnPcdShowInfo hnSysSetting::getPcdShowInfo()
	{
		if (m_bOpen)
		{
			return m_pcdShowInfo;
		}

		// 读取参数
		readInfo();

		return m_pcdShowInfo;
	}

	// 设置参数
	void hnSysSetting::setPcdShowInfo(const hnPcdShowInfo& pcdShowInfo)
	{
		m_pcdShowInfo = pcdShowInfo;

		// 写入参数
		writeInfo();
	}

	// 读取参数
	bool hnSysSetting::readInfo()
	{
		string strFolder = getCurrentDir();
		strFolder = strFolder + "hnSetting.xml";

		// 判断文件是否存在
		if (_access(strFolder.c_str(), 0) == -1)
		{
			writeInfo();
			m_bOpen = true;
			return true;
		}

		// 读取文件
		TiXmlDocument doc(strFolder.c_str());

		if (!doc.LoadFile())
		{
			return false;
		}

		TiXmlElement* rootElement = doc.RootElement();
		if (!rootElement)
		{
			return false;
		}

		// 点云显示参数
		TiXmlElement* element = rootElement->FirstChildElement("PcdShowInfo");
		TiXmlElement*childElement = element->FirstChildElement();

		// 节点名称
		string strName = "";

		// 节点值
		string strValue = "";

		while(childElement)
		{
			strName = childElement->Value();
			strValue = childElement->GetText();

			if (strName == "renderType")
			{
				m_pcdShowInfo.m_eRenderType = (ENUM_RENDERSTYLE)(atoi(strValue.c_str()));
			}
			else if (strName == "renderDist")
			{
				m_pcdShowInfo.m_dPcdShowDist = atof(strValue.c_str());
			}
			else if (strName == "show3dCnt")
			{
				m_pcdShowInfo.m_n3DShowCnt = atoi(strValue.c_str());
			}
			else if (strName == "showPanoCnt")
			{
				m_pcdShowInfo.m_nPanoShowCnt = atoi(strValue.c_str());
			}
			else if (strName == "renderSize")
			{
				m_pcdShowInfo.m_eRenderSize = (ENUM_RENDERSIZE)(atoi(strValue.c_str()));
			}

			childElement = childElement->NextSiblingElement();
		}

		return true;
	}

	// 写入参数
	bool hnSysSetting::writeInfo()
	{
		string strFolder = getCurrentDir();
		strFolder = strFolder + "hnSetting.xml";

		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "hngd", "" );
		TiXmlElement* rootElement = new TiXmlElement("SysSetting");

		doc.LinkEndChild(decl);
		doc.LinkEndChild(rootElement);

		// 添加点云显示节点
		TiXmlElement* pcdShowInfoNode = new TiXmlElement("PcdShowInfo");
		rootElement->LinkEndChild(pcdShowInfoNode);

		char strText[256] = {0};

		// 点云节点参数
		// 渲染方式
		TiXmlElement *renderTypeNode = new TiXmlElement("renderType");
		pcdShowInfoNode->LinkEndChild(renderTypeNode);
		memset(strText, 0, 256);
		sprintf(strText, "%d", (int)(m_pcdShowInfo.m_eRenderType));
		TiXmlText* renderTypeText = new TiXmlText(strText);
		renderTypeNode->LinkEndChild(renderTypeText);

		// 显示距离
		TiXmlElement *renderDistNode = new TiXmlElement("renderDist");
		pcdShowInfoNode->LinkEndChild(renderDistNode);
		memset(strText, 0, 256);
		sprintf(strText, "%lf", m_pcdShowInfo.m_dPcdShowDist);
		TiXmlText* renderDistText = new TiXmlText(strText);
		renderDistNode->LinkEndChild(renderDistText);

		// 3d视图点云显示阈值
		TiXmlElement *show3dCntNode = new TiXmlElement("show3dCnt");
		pcdShowInfoNode->LinkEndChild(show3dCntNode);
		memset(strText, 0, 256);
		sprintf(strText, "%d", m_pcdShowInfo.m_n3DShowCnt);
		TiXmlText* show3dCntText = new TiXmlText(strText);
		show3dCntNode->LinkEndChild(show3dCntText);  

		// 全景视图显示阈值
		TiXmlElement *showPanoCntNode = new TiXmlElement("showPanoCnt");
		pcdShowInfoNode->LinkEndChild(showPanoCntNode);
		memset(strText, 0, 256);
		sprintf(strText, "%d", m_pcdShowInfo.m_nPanoShowCnt);
		TiXmlText* showPanoCntText = new TiXmlText(strText);
		showPanoCntNode->LinkEndChild(showPanoCntText);

		// 渲染点大小
		TiXmlElement *renderSizeNode = new TiXmlElement("renderSize");
		pcdShowInfoNode->LinkEndChild(renderSizeNode);
		memset(strText, 0, 256);
		sprintf(strText, "%d", (int)(m_pcdShowInfo.m_eRenderSize));
		TiXmlText* renderSizeText = new TiXmlText(strText);
		renderSizeNode->LinkEndChild(renderSizeText);
		
		// 保存xml
		doc.SaveFile(strFolder.c_str());

		return true;
	}

	// 获取道路设置参数
	hnRoadSysSetInfo hnSysSetting::getRoadSysSetInfo()
	{
		return m_roadSysSetInfo;
	}

	// 设置道路参数
	void hnSysSetting::setRoadSysSetInfo(hnRoadSysSetInfo roadSysSetInfo)
	{
		m_roadSysSetInfo = roadSysSetInfo;

		// 写入参数
		writeInfo();
	}
}