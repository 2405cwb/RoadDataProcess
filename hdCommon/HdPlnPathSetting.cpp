#include "StdAfx.h"
#include "HdPlnPathSetting.h"
#include "..\hdCore\tinyxml.h"
#include "hdSceneStr.h"
namespace hd
{

	CHdPlnPathSetting::CHdPlnPathSetting(void)
	{

	}


	CHdPlnPathSetting::~CHdPlnPathSetting(void)
	{

	}


	// 读取配置文件HdScenePln.xml中的插件信息
	void CHdPlnPathSetting::ReadHdScenePlnXml()
	{
		string filePath = getCurrentDir();
		filePath += "HdScenePln.xml";
		TiXmlDocument doc(filePath.c_str());

		if (!doc.LoadFile())
		{
			return;
		}

		// 读之前先清空插件列表
		m_vecPlnPaths.clear();

		TiXmlElement* rootElement = doc.RootElement();
		if (rootElement)
		{
			TiXmlElement* element = rootElement->FirstChildElement("PlnPath");

			// 读取失败返回
			if (!element)
			{
				return;
			}

			TiXmlElement* childElement = element->NextSiblingElement();

			// 存取第一个插件的路径
			string path = element->Attribute("plnPathName");
			m_vecPlnPaths.push_back(path);

			// 读取失败返回
			if (!childElement)
			{
				return;
			}

			// 遍历存取插件路径
			while(childElement)
			{
				string path = childElement->Attribute("plnPathName");
				m_vecPlnPaths.push_back(path);
				childElement = childElement->NextSiblingElement();
			}
		}
	}

	// 写入插件路径信息至配置文件HdScenePln.xml
	void CHdPlnPathSetting::WriteHdScenePlnXml()
	{
		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );
		TiXmlElement* rootElement = new TiXmlElement("HD_3LS_SCENE_PLUGIN_SETTING");

		doc.LinkEndChild(decl);
		doc.LinkEndChild(rootElement);


		for (int i = 0; i < m_vecPlnPaths.size(); i++)
		{
			TiXmlElement *plnPath = new TiXmlElement("PlnPath");
			rootElement->LinkEndChild(plnPath);

			//保存插件路径
			plnPath->SetAttribute("plnPathName", m_vecPlnPaths[i].c_str());   
		}

		string filePath = getCurrentDir();
		filePath += "HdScenePln.xml";
		doc.SaveFile(filePath.c_str());
	}
}
