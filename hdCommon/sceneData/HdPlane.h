/*! HdPlane.h
********************************************************************************
<PRE>
模块名       : hdWorkspace
文件名       : HdPlane.h
相关文件     : HDObject.h 

文件实现功能 : 定义平面对象类
作者         : 董岱
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容

*******************************************************************************/
#pragma once
#include "HdFitObject.h"
#include "HdSxPoint3D.h"
#include "../point_types.h"
#include <vector>

namespace hd
{
	class CHdPlane :
		public CHdFitObject
	{
	public:
		CHdPlane(void):m_strName("Plane"),m_FitDif(0),m_fitPtCount(0),m_bUsedMSA(true),m_bMatch(false) { }
		CHdPlane(const char* strName):m_strName(strName),m_FitDif(0),m_fitPtCount(0),m_bUsedMSA(true),m_bMatch(false) { }
		virtual ~CHdPlane(void){}

		CHdPlane(CHdSxPoint3D point,string strname,vector<PointXYZIPRGBA> bundlepts, double radiusdif)
			:m_Position(point),m_strName(strname),m_FitDif(radiusdif)
		{
			m_fitPtCount = 0;
			m_bUsedMSA = true;
			m_bMatch = false;
		}

		CHdPlane(CHdPlane& plane)
		{
			m_Position = plane.m_Position;
			m_strName = plane.m_strName;
			m_FitDif = plane.m_FitDif;
			m_fitPtCount = plane.m_fitPtCount;
			m_bUsedMSA = plane.m_bUsedMSA;
			m_bMatch = plane.m_bMatch;

			for (int i = 0; i < 4; i++)
			{
				m_FunM[i] = plane.m_FunM[i];
			}
			m_BundlePts = plane.m_BundlePts;
		}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_PLANE;}

		virtual EHD_FIT_OBJECT_STATE GetState() const 
		{
			if (m_FitDif>=0 && m_FitDif<=0.002 )
				return EFOS_OK;
			else if (m_FitDif > 0.002 )
				return EFOS_NOK; 
			else
				return EFOS_POK;					
		}

	public:
		CHdSxPoint3D	m_Position;		// 中心点位置
		//int		m_NumPts;
		vector<PointXYZIPRGBA> m_BundlePts;//边界点;
		string		m_strName;
		double		m_FitDif;		//拟合偏差;
		//double		m_LocationDif;		//位置偏差;
		int			m_fitPtCount;		// 拟合点数;
		hd::f32		m_FunM[4];			//平面方程三参数ax+by+cz+d=0;
		bool		m_bUsedMSA;			// 在MSA算法中是否可用
		bool        m_bMatch;           // 是否已匹配
		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* planeElement = new TiXmlElement("Plane");
				element->LinkEndChild(planeElement);

				/*xmlElement = new TiXmlElement("Radius");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_Radius);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);*/

				xmlElement = new TiXmlElement("Name");
				planeElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("FitPtCount");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%d", m_fitPtCount);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("RadiusStdDev");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_FitDif);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("BundlePts_Size");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%d", m_BundlePts.size());
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Fun_a");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%f", m_FunM[0]);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Fun_b");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%f", m_FunM[1]);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Fun_c");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%f", m_FunM[2]);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Fun_d");
				planeElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%f", m_FunM[3]);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				for (u32 i = 0; i<m_BundlePts.size();i++)
				{
					xmlElement = new TiXmlElement("BundlePts_x");
					planeElement->LinkEndChild(xmlElement);
					sprintf_s(strTemp,128, "%f", m_BundlePts[i].x);
					xmlText = new TiXmlText(strTemp);
					xmlElement->LinkEndChild(xmlText);

					xmlElement = new TiXmlElement("BundlePts_y");
					planeElement->LinkEndChild(xmlElement);
					sprintf_s(strTemp,128, "%f", m_BundlePts[i].y);
					xmlText = new TiXmlText(strTemp);
					xmlElement->LinkEndChild(xmlText);

					xmlElement = new TiXmlElement("BundlePts_z");
					planeElement->LinkEndChild(xmlElement);
					sprintf_s(strTemp,128, "%f", m_BundlePts[i].z);
					xmlText = new TiXmlText(strTemp);
					xmlElement->LinkEndChild(xmlText);
				}
				m_Position.Serialize(planeElement, bSave);
			}
			else
			{
				int countBundlePts = 0;
				int countx = 0;
				int county = 0;
				int countz = 0 ;
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
					else if (strValue == "RadiusStdDev")
					{
						sscanf_s(strText.data(), "%lf", &m_FitDif);
					}
					else if (strValue == "FitPtCount")
					{
						sscanf_s(strText.data(), "%d", &m_fitPtCount);
					}
					else if (strValue == "BundlePts_Size")
					{
						sscanf_s(strText.data(),"%d", &countBundlePts);
						m_BundlePts.resize(countBundlePts);
					}
					else if (strValue == "BundlePts_x")
					{
						sscanf_s(strText.data(), "%f", &m_BundlePts[countx].x);
						countx++;
					}
					else if (strValue == "BundlePts_y")
					{
						sscanf_s(strText.data(), "%f", &m_BundlePts[county].y);
						county++;
					}
					else if (strValue == "BundlePts_z")
					{
						sscanf_s(strText.data(), "%f", &m_BundlePts[countz].z);
						countz++;
					}
					else if (strValue == "Fun_a")
					{
						sscanf_s(strText.data(), "%f", &m_FunM[0]);
					}
					else if (strValue == "Fun_b")
					{
						sscanf_s(strText.data(), "%f", &m_FunM[1]);
					}
					else if (strValue == "Fun_c")
					{
						sscanf_s(strText.data(), "%f", &m_FunM[2]);
					}
					else if (strValue == "Fun_d")
					{
						sscanf_s(strText.data(), "%f", &m_FunM[3]);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}
	};
}

