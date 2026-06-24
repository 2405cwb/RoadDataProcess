/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdSoftTypeConfig.cpp
文件实现功能：此文件是为软件系列进行标识配置
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/1/19	1.0			蔡红云		创建
</PRE>
******************************************************************************************************/
#include "StdAfx.h"
#include "HdSoftTypeConfig.h"
#include "..\hdCore\tinyxml.h"
#include "hdSceneStr.h"

namespace hd
{
	CHdSoftTypeConfig* CHdSoftTypeConfig::m_pSHdSoftTypeConfig = NULL;

	CHdSoftTypeConfig::CHdSoftTypeConfig(void):m_iSoftType(0x100)
	{

	}


	CHdSoftTypeConfig::~CHdSoftTypeConfig(void)
	{

	}

	//! 从配置文件读取，获取系统设置,外部不需要delete
	CHdSoftTypeConfig* CHdSoftTypeConfig::getHdSoftTypeConfig()
	{
		if (m_pSHdSoftTypeConfig== NULL)
		{
			m_pSHdSoftTypeConfig = new CHdSoftTypeConfig;

			string filePath = getCurrentDir();
			filePath += "SoftType.xml";
			TiXmlDocument doc(filePath.c_str());

			if (!doc.LoadFile())
			{
				return m_pSHdSoftTypeConfig ;
			}

			TiXmlElement* rootElement = doc.RootElement();
			if (rootElement)
			{
				TiXmlElement* element = rootElement->FirstChildElement("softType");

				// 读取失败返回
				if (!element)
				{
					return m_pSHdSoftTypeConfig;
				}

				string strText = element->GetText();

				m_pSHdSoftTypeConfig->m_iSoftType = atoi(strText.c_str());
				
			}
			
		}
       
		return m_pSHdSoftTypeConfig;
	}

	//! 删除静态唯一对象
	void CHdSoftTypeConfig::destroyHdSoftTypeConfig()
	{

		if (m_pSHdSoftTypeConfig)
		{
			delete m_pSHdSoftTypeConfig;
			m_pSHdSoftTypeConfig = NULL;
		}

	}
	
	// 写入插件路径信息至配置文件HdScenePln.xml
	void CHdSoftTypeConfig::saveHdSoftTypeConfig()
	{
		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );
		TiXmlElement* rootElement = new TiXmlElement("SOFTTYPE_SETTING");

		doc.LinkEndChild(decl);
		doc.LinkEndChild(rootElement);



		TiXmlElement *pSoftType = new TiXmlElement("softType");
		rootElement->LinkEndChild(pSoftType);


		int type = m_pSHdSoftTypeConfig->m_iSoftType;
		char strtype[128];
		sprintf(strtype,"%d",type);
				
		TiXmlText* pSoftTypeText = new TiXmlText(strtype);
		pSoftType->LinkEndChild(pSoftTypeText);
			
				
		string filePath = getCurrentDir();
		filePath += "SoftType.xml";
		doc.SaveFile(filePath.c_str());
	}
	

}