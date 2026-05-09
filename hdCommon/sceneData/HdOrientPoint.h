/*! HdOrientPoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdOrientPoint.h
相关文件     : HDObject.h 

文件实现功能 : 定义定向点类
作者         : 危迟
版本         : 1.0

</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdOrientPoint :
		public CHDObject
	{
	public:
		CHdOrientPoint(void):m_strName("OrientPoint_new") {}
		CHdOrientPoint(const char* strName):m_strName(strName) {}
		virtual ~CHdOrientPoint(void) {}

	public:
		CHdSxPoint3D m_Position;
		string	 m_strName;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_ORIENTPOINT; }

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* labelElement = new TiXmlElement("OrientPoint");
				element->LinkEndChild(labelElement);

				xmlElement = new TiXmlElement("Name");
				labelElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				m_Position.Serialize(labelElement, bSave);
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