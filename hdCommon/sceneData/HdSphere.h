/*! HdLabel.h
********************************************************************************
<PRE>
模块名       : hdWorkspace
文件名       : HdSphere.h
相关文件     : HDObject.h 

文件实现功能 : 定义靶球对象类
作者         : 杨峰
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/07/02   1.0      杨峰    
2012/07/04		      杨峰				  修改靶球继承CHdFitObject类
2013/03/07   1.1      龚书林			 修改继承
</PRE>
*******************************************************************************/
#pragma once
#include "HdFitObject.h"
#include "HdSxPoint3D.h"

namespace hd
{
	class CHdSphere :
		public CHdFitObject
	{
	public:
		CHdSphere(void):m_Radius(0.0725),m_strName("sphere"),m_RadiusDif(0),m_LocationDif(0),m_fitPtCount(0) { }
		CHdSphere(const char* strName):m_Radius(0.0725),m_strName(strName),m_RadiusDif(0),m_LocationDif(0),m_fitPtCount(0) { }
		virtual ~CHdSphere(void){}

		CHdSphere(CHdSxPoint3D point,double radius,string strname,double radiusdif,double locationdof)
			:m_Position(point),m_Radius(radius),m_strName(strname),m_RadiusDif(radiusdif),m_LocationDif(locationdof)
		{
			m_fitPtCount = 0;
		}

		CHdSphere(CHdSphere& sphere)
		{
			m_Position = sphere.m_Position;
			m_Radius = sphere.m_Radius;
			m_strName = sphere.m_strName;
			m_RadiusDif = sphere.m_RadiusDif;
			m_LocationDif = sphere.m_LocationDif;
			m_fitPtCount = sphere.m_fitPtCount;
		}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_SPHERE;}

		virtual EHD_FIT_OBJECT_STATE GetState() const 
		{
			if (m_RadiusDif>=0 && m_RadiusDif<=0.002 && m_LocationDif>=0 && m_LocationDif<=0.005)
				return EFOS_OK;
			else if (m_RadiusDif > 0.002 && m_LocationDif > 0.005)
				return EFOS_NOK; 
			else
				return EFOS_POK;					
		}

	public:
		CHdSxPoint3D	m_Position;
		double		m_Radius;
		string		m_strName;
		double		m_RadiusDif;//半径偏差
		double		m_LocationDif;//位置偏差
		int			m_fitPtCount;// 拟合点数

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* sphereElement = new TiXmlElement("Sphere");
				element->LinkEndChild(sphereElement);

				xmlElement = new TiXmlElement("Radius");
				sphereElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_Radius);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Name");
				sphereElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("FitPtCount");
				sphereElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%d", m_fitPtCount);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("RadiusStdDev");
				sphereElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_RadiusDif);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				m_Position.Serialize(sphereElement, bSave);
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

					if (strValue == "Radius")
					{
						sscanf_s(strText.data(), "%lf", &m_Radius);
					}
					else if (strValue == "Name")
					{
						m_strName = strText;
					}
					else if (strValue == "Point")
					{
						m_Position.Serialize(nextElement, bSave);
					}
					else if (strValue == "RadiusStdDev")
					{
						sscanf_s(strText.data(), "%lf", &m_RadiusDif);
					}
					else if (strValue == "FitPtCount")
					{
						sscanf_s(strText.data(), "%d", &m_fitPtCount);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}
	};
}

