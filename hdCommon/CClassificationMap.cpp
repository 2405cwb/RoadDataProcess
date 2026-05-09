#include "StdAfx.h"
#include "CClassificationMap.h"
#include "..\hdCore\tinyxml.h"
#include "hdSceneStr.h"

namespace hd
{
	CClassificationMap::CClassificationMap(void)
	{
		// 初始化，使用las格式的规范设定类别映射表
		memset(m_nClassIDs,0,sizeof(unsigned int)*256);
		m_nClassCount = 13;
		for (int i = 0; i< m_nClassCount;i++)
		{
			m_nClassIDs[i] = 1;
		}
		// 0 ，1 为相同的颜色
		m_sClassColor[0] =  classification[0];
		for (int i = 1;i< 256;i++)
		{
			m_nColorIndex[i] = i;
			m_sClassColor[i] = classification[i - 1];
		}

		// 特别设置低洼点的初始颜色
		m_sClassColor[7] = classification[28];
		// 按照las中类别的定义进行初始化
		m_sClassNames[0] = HDSCENE_IDS_CLASSIFY_COLOR_DEFAULT; 
		m_sClassNames[1] = HDSCENE_IDS_CLASSIFY_COLOR_UNCLASSIFIED;
		m_sClassNames[2] = HDSCENE_IDS_CLASSIFY_COLOR_GROUND;
		m_sClassNames[3] = HDSCENE_IDS_CLASSIFY_COLOR_LOWVEG;
		m_sClassNames[4] = HDSCENE_IDS_CLASSIFY_COLOR_MEDVEG;
		m_sClassNames[5] = HDSCENE_IDS_CLASSIFY_COLOR_HIGHVEG;
		m_sClassNames[6] = HDSCENE_IDS_CLASSIFY_COLOR_BUILDING;
		m_sClassNames[7] = HDSCENE_IDS_CLASSIFY_COLOR_LOWPOINT;
		m_sClassNames[8] = HDSCENE_IDS_CLASSIFY_COLOR_MODELPOINT;  
		m_sClassNames[9] = HDSCENE_IDS_CLASSIFY_COLOR_WATER;
		m_sClassNames[10] = HDSCENE_IDS_CLASSIFY_COLOR_SUNNOISE;
		m_sClassNames[11] = HDSCENE_IDS_CLASSIFY_COLOR_ROAD;
		m_sClassNames[12] = HDSCENE_IDS_CLASSIFY_COLOR_ROADMARK;

		for (int i = 13;i< 256;i++)
		{
			m_sClassNames[i] = "";
		}
		m_IsUpdate = false;
	}

	CClassificationMap::~CClassificationMap(void)
	{

	}

	bool CClassificationMap::DeleteClass( unsigned int id )
	{
		if (id >= 256 || id >=  m_nClassCount)
		{
			return false;
		}

		// 从id + 1开始，类别向前移动一个单位
		for (int i = id; i < m_nClassCount; i++)
		{
			m_nClassIDs[i] = m_nClassIDs[i + 1];
			m_sClassNames[i] = m_sClassNames[i + 1];
			m_sClassColor[i] = m_sClassColor[i + 1];
		}
		// 类别数减1
		m_nClassCount--;
		return true;
	}

	int CClassificationMap::GetIDbyClassName( string& name )
	{
		for (int i = 0;i< 256;i++)
		{
			if (strcmp(name.c_str(),m_sClassNames[i].c_str())== 0)
			{
				return i;
			}
		}
		return -1;
	}

	string& CClassificationMap::GetClassNamebyID( unsigned int id )
	{
		return m_sClassNames[id];
	}

	COLORREF CClassificationMap::GetColorValueByID(unsigned int id)
	{
		return m_sClassColor[id];
	}

	int CClassificationMap::GetColorIndexByID( unsigned int id )
	{
		return m_nColorIndex[id];
	}

	COLORREF CClassificationMap::GetColorByID(unsigned int id)
	{
		int crIdx = m_nColorIndex[id];

		return classification[crIdx];
	}

	COLORREF CClassificationMap::GetColorByColorID(unsigned int crIdx)
	{
		return classification[crIdx];
	}

	void CClassificationMap::ClearClass()
	{
		memset(m_nClassIDs,0,sizeof(unsigned int)*256);
		m_nClassIDs[0] = 1;
		//fengjing i = 1 改为 i= 0
		for (int i = 0;i< m_nClassCount;i++)
		{
			m_sClassNames[i].clear();
		}
		m_nClassCount = 1;
	}

	//bool CClassificationMap::AddNewClass( unsigned int id,string name,unsigned int colorIdx )
	//{
	//	if (id > 255)
	//	{
	//		return false;
	//	}
	//	// 如果指定id类别不存在，则类别数增加1
	//	if (m_nClassIDs[id] == 0)
	//	{
	//		m_nClassCount++;
	//	}
	//	m_nClassIDs[id] = 1;
	//	m_sClassNames[id] = name;
	//	m_nColorIndex[id] = colorIdx;
	//	return true;
	//}


	bool hd::CClassificationMap::AddNewClass( unsigned int id,string name )
	{
		//
		if (id > 255)
		{
			return false;
		}
		// 如果指定id类别不存在，则类别数增加1
		if (m_nClassIDs[id] == 0)
		{
			m_nClassCount++;
		}
		m_nClassIDs[id] = 1;
		m_sClassNames[id] = name;
		m_sClassColor[id] = classification[id];
		return true;
	}

	bool CClassificationMap::AddNewClass(unsigned int id, string name, COLORREF color)
	{
		if (id > 255)
		{
			return false;
		}
		// 如果指定id类别不存在，则类别数增加1
		if (m_nClassIDs[id] == 0)
		{
			m_nClassCount++;
		}
		m_nClassIDs[id] = 1;
		m_sClassNames[id] = name;
		m_sClassColor[id] = color;
		return true;
	}

	bool CClassificationMap::SetClassColorByID(unsigned int id, COLORREF color)
	{
		if (id > 255)
		{
			return false;
		}
		// 如果指定id类别不存在，则类别数增加1
		if (m_nClassIDs[id] == 0)
		{
			m_nClassCount++;
		}

		m_nClassIDs[id] = 1;
		m_sClassColor[id] = color;
		return true;
	}

	void CClassificationMap::ReadClassifyXml()
	{
		string filePath = getCurrentDir();
		filePath += "Classify.xml";

		TiXmlDocument doc(filePath.c_str());
		if (!doc.LoadFile())
		{
			return;
		}

		TiXmlElement* rootElement = doc.RootElement();
		if (rootElement)
		{
			TiXmlElement* element = rootElement->FirstChildElement("ClassifySetting");
			if (element)
			{
				//读取类别数, 包括所有分类的类别数
				m_nClassCount = atoi(element->Attribute("Count"));
			}
			TiXmlElement* childElement = element->NextSiblingElement();
			if (!childElement)
			{
				return;
			}
			//读取类别名称的ID，类别名称以及颜色索引，类别编号从1开始
			int i = 0; 
			while(childElement)
			{
				//1表示类别存在， 0表示类别不存在
				m_nClassIDs[i] = 1;
				m_sClassNames[i].assign(childElement->Attribute("className"));
				m_sClassColor[i] = (COLORREF)atoi(childElement->Attribute("classColor"));
				i++;
				childElement = childElement->NextSiblingElement();
			}
		}
	}

	void CClassificationMap::WriteClassfyXml()
	{
		TiXmlDocument doc;
		TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );
		TiXmlElement* rootElement = new TiXmlElement("HD_3LS_SCENE_CLASSIFY_SETTING");

		doc.LinkEndChild(decl);
		doc.LinkEndChild(rootElement);

		// 保存分类设置 所有分类编号为0，实际类别号从一开始，类别数为总类别数减去1
		TiXmlElement* classify = new TiXmlElement("ClassifySetting");
		rootElement->LinkEndChild(classify);
		classify->SetAttribute("Count", GetClassCount()); 

		int i;
		for (i = 0; i < GetClassCount(); i++)
		{
			TiXmlElement *classType = new TiXmlElement("ClassOfPoints");
			rootElement->LinkEndChild(classType);
			//保存类别名称的ID
			classType->SetAttribute("classID", i);   
			//保存类别名称
			classType->SetAttribute("className", GetClassNamebyID(i).c_str()); 
			//保存类别颜色索引
			classType->SetAttribute("classColor", GetColorValueByID(i));
		}

			string filePath = getCurrentDir();
			filePath += "Classify.xml";
			doc.SaveFile(filePath.c_str());
	}
}


