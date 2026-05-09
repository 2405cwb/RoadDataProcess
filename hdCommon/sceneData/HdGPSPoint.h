/*! CHdGPSPoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HDGPSPoint.h
相关文件     : HDPoint.h 

文件实现功能 : 定义GPS控制点对象类
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/12/08   1.1      危迟
2013/03/07   1.1      龚书林
</PRE>
*******************************************************************************/
#pragma once
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdGPSPoint :
		public CHDObject
	{
	public:
		CHdGPSPoint():m_strName(""),m_fGPSX(0.0f),m_fGPSY(0.0f),m_fGPSZ(0.0f)
		{
		}
		~CHdGPSPoint(void) {}


	public:
		string		m_strName;		//点名
		//string		m_strScanName;   //记录GPS点所在的测站名
		/*CHdPoint	m_Position;*/	
		//float		m_fScanX;
		//float		m_fScanY;
		//float		m_fScanZ;		//记录GPS点在测站中的位置
		double		m_fGPSX;
		double		m_fGPSY;
		double		m_fGPSZ;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_GPS_CTRLPT; }
		
	public:
		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* gpsPointElement = new TiXmlElement("GPSPoint");
				element->LinkEndChild(gpsPointElement);

				xmlElement = new TiXmlElement("Name");
				gpsPointElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				//m_Position.Serialize(gpsPointElement, true);

				xmlElement = new TiXmlElement("GPS_XYZ");
				gpsPointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf,%lf,%lf", m_fGPSX, m_fGPSY, m_fGPSZ);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);
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
					else if (strValue == "GPS_XYZ")
					{
						sscanf_s(strText.data(), "%lf,%lf,%lf", &m_fGPSX, &m_fGPSY, &m_fGPSZ);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}
	};

}