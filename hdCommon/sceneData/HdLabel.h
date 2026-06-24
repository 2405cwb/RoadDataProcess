/*! HdLabel.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdLabel.h
相关文件     : HDObject.h 

文件实现功能 : 定义标签对象类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      姚立    
2012/07/02			  杨峰				  添加标注名称字段,默认标注名称"label_new"
2013/03/07   1.1      龚书林			 修改继承
</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdLabel :
		public CHDObject
	{
	public:
		CHdLabel(void):m_strText(""),m_strName("label_new") {}
		CHdLabel(const char* strName):m_strText(""),m_strName(strName) { }
		virtual ~CHdLabel(void) {}

	public:
		CHdSxPoint3D m_Position;
		string   m_strText;
		string	 m_strName;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_LABEL; }

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* labelElement = new TiXmlElement("Label");
				element->LinkEndChild(labelElement);

				xmlElement = new TiXmlElement("Text");
				labelElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strText.data());
				xmlElement->LinkEndChild(xmlText);

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

					if (strValue == "Text")
					{
						m_strText = strText;
					}
					else if (strValue == "Name")
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


