/*! HdFeaturePoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdFeaturePoint.h
相关文件     : HDObject.h 

文件实现功能 : 定义特征点对象类
作者         : 张飞
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/11/12   1.0       张飞					创建
2017//02/13  1.1       蔡红云               扩展下序列化HS标靶
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdFeaturePoint :
		public CHDObject
	{
	public:
		CHdFeaturePoint(void):m_strName("FeaturePoint") {};
		CHdFeaturePoint(const char* strName):m_strName(strName) {}
		virtual ~CHdFeaturePoint(void){}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_FEATURE_POINT; }
	public:
		CHdSxPoint3D m_Position;
		string	 m_strName;

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;
				TiXmlElement* FeatureElement = NULL;
				if (GetType() == ESDT_OBJECT_FEATURE_POINT)
				{
					FeatureElement = new TiXmlElement("FeaturePoint");
					element->LinkEndChild(FeatureElement);
				}
				else if(GetType() == ESDT_OBJECT_HS_TARGET)
				{
					FeatureElement = new TiXmlElement("HsTarget");
					element->LinkEndChild(FeatureElement);
				}
				
				xmlElement = new TiXmlElement("Name");
				FeatureElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				m_Position.Serialize(FeatureElement, bSave);
			}
			else
			{
				string strValue;
				string strText;
				TiXmlElement* nextElement = element->FirstChildElement();
				while(nextElement)
				{
					strValue = nextElement->Value();
					if (nextElement->GetText() == NULL)
					{
						strText = "";
					}
					else
						strText = nextElement->GetText();

					if (strValue == "Name")
					{
						m_strName = strText;
					}
					else if (strValue == "Point")
					{
						m_Position.Serialize(nextElement, bSave);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}

	};
}


