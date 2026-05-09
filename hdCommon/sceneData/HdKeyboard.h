/*! HdKeyboard.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdKeyboard.h
相关文件     : HDObject.h 

文件实现功能 : 定义标靶对象类
作者         : 危迟
版本         : 1.0

</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdKeyboard :
		public CHDObject
	{
	public:
		CHdKeyboard(void):m_strName("Chessboard_new") {}
		CHdKeyboard(const char* strName):m_strName(strName) {}
		virtual ~CHdKeyboard(void) {}

	public:
		CHdSxPoint3D m_Position;
		// 添加法向量属性
		CHD3DPoint m_Normal;
		string	 m_strName;
		double	 m_stdDev;//5.26.2014 添加by dongdai;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_KEYBOARD; }

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* ChessElement = new TiXmlElement("Chessboard");
				element->LinkEndChild(ChessElement);

				xmlElement = new TiXmlElement("Name");
				ChessElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				char strTemp[128];
				xmlElement = new TiXmlElement("FitStdDev");
				ChessElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_stdDev);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				m_Position.Serialize(ChessElement, bSave);
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
					else if (strValue == "FitStdDev")
					{
						sscanf_s(strText.data(), "%lf", &m_stdDev);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}

	};
}