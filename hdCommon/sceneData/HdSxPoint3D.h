/*! HdPoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdPoint.h
相关文件     :

文件实现功能 : 定义点对象类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      姚立    
2013/03/07   1.1      龚书林			 修改继承
</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "..\HD3DPoint.h"
#include <string>
using namespace std;
namespace hd
{
	class CHdSxPoint3D:
		public CHD3DPoint
	{
	public:
		CHdSxPoint3D(void)
			:CHD3DPoint(0.0,0.0,0.0),m_fRow(-1.0), m_fCol(-1.0),m_bFeaturePt(false)
		{
		}
		CHdSxPoint3D(double x, double y, double z)
			:CHD3DPoint(x,y,z),m_fRow(-1.0), m_fCol(-1.0),m_bFeaturePt(false)
		{
		}
		CHdSxPoint3D(double x,double y, double z,double row,double col)
			:CHD3DPoint(x,y,z),m_fRow(row), m_fCol(col),m_bFeaturePt(false)
		{
		}
		CHdSxPoint3D(const CHdSxPoint3D &_pt)
			:CHD3DPoint(_pt.m_x,_pt.m_y,_pt.m_z),m_fCol(_pt.m_fCol),m_fRow(_pt.m_fRow),m_bFeaturePt(false)
		{
		}
			
		virtual ~CHdSxPoint3D(void){}

	public:

		double m_fRow;	//对应的点云的行比例
		double m_fCol;	//对应的点云的列比例

	private:
		bool m_bFeaturePt;          //是否为房屋特征点

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_POINT;}

		//! 判断点是否为平面或快速视图获取到的点
		bool isPixelPoint() const
		{
			return (m_fRow >= 0.0 && m_fRow <= 1.0 &&
				m_fCol >= 0.0 && m_fCol <= 1.0);
		}
		void setRowColRatio(double fRow, double fCol)
		{
			m_fRow = fRow;
			m_fCol = fCol;
		}
		virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* pointElement = new TiXmlElement("Point");
				element->LinkEndChild(pointElement);

				xmlElement = new TiXmlElement("Point_X");
				pointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_x);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Point_Y");
				pointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_y);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Point_Z");
				pointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_z);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("RowRatio");
				pointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_fRow);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("ColRatio");
				pointElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%lf", m_fCol);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("FeaturePt");
				pointElement->LinkEndChild(xmlElement);
				int nFeature = 0;
				if (m_bFeaturePt)
				{
					nFeature = 1;
				}
				sprintf_s(strTemp,128, "%d", nFeature);
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

					if (strValue == "Point_X")
					{
						sscanf_s(strText.data(), "%lf", &m_x);
					}
					else if (strValue == "Point_Y")
					{
						sscanf_s(strText.data(), "%lf", &m_y);
					}
					else if (strValue == "Point_Z")
					{
						sscanf_s(strText.data(), "%lf", &m_z);
					}
					else if (strValue == "RowRatio")
					{
						sscanf_s(strText.data(), "%lf", &m_fRow);
					}
					else if (strValue == "ColRatio")
					{
						sscanf_s(strText.data(), "%lf", &m_fCol);
					}
					else if (strValue == "FeaturePt")
					{
						int nFeaturePt = 0;
						sscanf_s(strText.data(), "%d",&nFeaturePt);

						m_bFeaturePt = nFeaturePt > 0 ? true:false;
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}

		void SetFeaturePt(bool bFeaturePt) {m_bFeaturePt = bFeaturePt; }
		bool GetFeaturePt() { return m_bFeaturePt; }



	};
}


