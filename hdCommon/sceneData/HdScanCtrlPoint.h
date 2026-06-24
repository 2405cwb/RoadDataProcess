#pragma once
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdScanCtrlPoint:
		public CHDObject
	{
	public:
		CHdScanCtrlPoint():m_strName(""),m_strScanName(""), m_fScanX(0.0f),m_fScanY(0.0f),m_fScanZ(0.0f)
		{
			
		}
		CHdScanCtrlPoint(string PointID,string ScanID,float x,float y,float z):m_strName(PointID),m_strScanName(ScanID), m_fScanX(x),m_fScanY(y),m_fScanZ(z)
		{
			
		}
		virtual ~CHdScanCtrlPoint(void) {}


	public:
		string		m_strName;		//点名
		string		m_strScanName;   //记录GPS点所在的测站名
		
		float		m_fScanX;
		float		m_fScanY;
		float		m_fScanZ;		//记录GPS点在测站中的位置
		
	public:

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_SCANCTRLPT; }

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

				xmlElement = new TiXmlElement("ScanName");
				gpsPointElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strScanName.data());
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Scan_XYZ");
				gpsPointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%f,%f,%f", m_fScanX, m_fScanY, m_fScanZ);
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
					else if (strValue == "ScanName")
					{
						m_strScanName = strText;
					}
					else if (strValue == "Scan_XYZ")
					{
						sscanf_s(strText.data(), "%f,%f,%f", &m_fScanX, &m_fScanY, &m_fScanZ);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}
	};
}