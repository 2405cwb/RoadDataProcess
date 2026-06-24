/*! HdPolyline.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdPolyline.h
相关文件     : HdPolyline.cpp

文件实现功能 : 定义多线段对象类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      姚立  
2013/03/07   1.0      龚书林			修改继承
2013/09/24   1.0      龚书林			添加下标
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"
#include <vector>
using namespace std;

namespace hd
{
    //测量线的类型(测量分析工具使用)lixialiang - add 2015/04/21
	enum ENUM_MEASURE_TYPE
	{
		E_MEASURE_LINE = 0,    //量测距离线
		E_MEASURE_HEIGHT,      //量测高度线
		E_MEASURE_ANGLE,       //量测角度线
		E_MEASURE_DIHEDRALANGLE,    //量测二面角角度线
		E_MEASURE_SLOPE,       //量测坡度线
		E_MEASURE_AREA,        //量测区域面积边框线
		E_FIT_LINE,            //拟合线
		E_NO_TYPE,            //不是量测线
	};

	class CHdSxPolyline3D :
		public CHDObject
	{
	public:
		CHdSxPolyline3D(void)
			:CHDObject(){m_pMeasureType = E_NO_TYPE;m_bCADLine = false;m_bCADClose = false;}
		virtual ~CHdSxPolyline3D(void){}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_POLYLINE;}

		virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				TiXmlElement* polylineElement = new TiXmlElement("Polyline");
				element->LinkEndChild(polylineElement);

				for (unsigned int i = 0; i<m_Polyline.size(); i++)
				{
					m_Polyline[i].Serialize(polylineElement, bSave);
				}

				//如果是测量线,写入线段的测量线类型  lixialiang  add 2015/04/22
				if (m_pMeasureType != E_NO_TYPE)
				{
					TiXmlElement* AttrElement = new TiXmlElement("AttriBute");
					polylineElement->LinkEndChild(AttrElement);

					char strTemp[128] = {0};
					TiXmlElement* xmlElement = new TiXmlElement("MeasureType");
					AttrElement->LinkEndChild(xmlElement);
					sprintf_s(strTemp,128, "%d", (int)m_pMeasureType);
					TiXmlText* xmlText = new TiXmlText(strTemp);
					xmlElement->LinkEndChild(xmlText);

					TiXmlElement* xmlElementClose = new TiXmlElement("CADClose");
					AttrElement->LinkEndChild(xmlElementClose);
					sprintf_s(strTemp,128, "%d", m_bCADClose?1:0);
					TiXmlText* xmlTextClose = new TiXmlText(strTemp);
					xmlElementClose->LinkEndChild(xmlTextClose);
				}
			}
			else
			{
				string strValue;
				TiXmlElement* nextElement = element->FirstChildElement();
				while(nextElement)
				{
					strValue = nextElement->Value();
					if (strValue == "Point")
					{
						CHdSxPoint3D newPoint;
						newPoint.Serialize(nextElement, bSave);

						m_Polyline.push_back(newPoint);
					}
					else if (strValue == "AttriBute")
					{
						//如果是测量线,读取线段的测量线类型  lixialiang  add 2015/04/22
						string strValue;
						string strText;
						int nType = 0;
						TiXmlElement* childElement = nextElement->FirstChildElement();
						if(childElement)
						{
							strValue = childElement->Value();
							if (childElement->GetText() == NULL)
							{
								strText = "";
							}
							else
								strText = childElement->GetText();

							if (strValue == "MeasureType")
							{
								sscanf_s(strText.data(), "%d", &nType);
								m_pMeasureType = (ENUM_MEASURE_TYPE)nType;
							}

							if (strValue == "CADClose")
							{
								sscanf_s(strText.data(), "%d", &nType);
								m_bCADClose = (nType == 1);
							}
						}
					}

					nextElement = nextElement->NextSiblingElement();
				}
				RecalclateBox();
			}
		}
		void Clear()
		{
			m_Polyline.clear();
			m_MinPt = m_MaxPt = CHdSxPoint3D(0.0, 0.0, 0.0);
		}
		//多线段编辑接口
		void AddPoint(const CHdSxPoint3D& point)
		{
			//CHdSxPoint3D* newPoint = new CHdSxPoint3D(*point);
			m_Polyline.push_back(point);

			if (m_Polyline.size() == 1)
			{
				m_MaxPt = m_MinPt = point;
			}
			else
			{
				UpdateBox(point);
			}
		}

		void AddPoint(double x, double y, double z)
		{
			CHdSxPoint3D pt(x,y,z);
			m_Polyline.push_back(pt);

			if (m_Polyline.size() == 1)
			{
				m_MaxPt = m_MinPt = pt;
			}
			else
			{
				UpdateBox(pt);
			}
		}

		void DeletePoint(unsigned int nIndex)
		{
			if (nIndex >=0 && nIndex < m_Polyline.size())
			{
				m_Polyline.erase(m_Polyline.begin() + nIndex);

				RecalclateBox();
			}
		}

		void InsertAfter(unsigned int nIndex,const CHdSxPoint3D& point)
		{
			if (nIndex >= 0 && nIndex < m_Polyline.size())
			{
				//CHdPoint* newPoint = new CHdPoint(*point);
				m_Polyline.insert(m_Polyline.begin() + nIndex, point);

				if (m_Polyline.size() == 1)
				{
					m_MaxPt = m_MinPt = point;
				}
				else
				{
					UpdateBox(point);
				}
			}
		}

        bool SetLastPoint(double x, double y, double z)
        {
            size_t nCount = m_Polyline.size();

            if (nCount > 0)
            {
				CHdSxPoint3D& pt = m_Polyline[nCount - 1]; // *(m_Polyline._Myfirst + nCount - 1);
                pt.m_x = x;
                pt.m_y = y;
                pt.m_z = z;

                RecalclateBox();
                return true;
            }

            return false;
        }

		void SetCADLine(bool flag){m_bCADLine = flag;}
		bool GetCADLine(){return m_bCADLine;}

		bool SetLastPoint(double colScale,double rowScale,double x, double y, double z)
		{
			size_t nCount = m_Polyline.size();
			if (nCount > 0)
			{
				CHdSxPoint3D& pt = m_Polyline[nCount - 1];
				pt.m_fRow = rowScale;
				pt.m_fCol = colScale;
				pt.m_x = x;
				pt.m_y = y;
				pt.m_z = z;

				RecalclateBox();
				return true;
			}

			return false;
		}

		void DeleteLastPoint()
		{
			unsigned int nCount = m_Polyline.size();
			DeletePoint(nCount - 1);
		}
			
		unsigned int GetPointCount() const
		{
			return (unsigned int)m_Polyline.size();
		}

		inline CHdSxPoint3D& operator[](unsigned int index)
		{ 
			return m_Polyline[index];
		}

		inline const CHdSxPoint3D& operator[](unsigned int index) const
		{ 
			return m_Polyline.at(index);//*(m_Polyline._Myfirst + index);
		}

		const CHdSxPoint3D& GetPoint(unsigned int nIndex) const
		{
			return m_Polyline.at(nIndex);
		}

		CHdSxPoint3D& GetPoint(unsigned int nIndex)
		{
			return m_Polyline.at(nIndex);
		}

		void SetPoint(unsigned int nIndex,const CHdSxPoint3D& pPoint, bool bReCalcBox = false)
		{
			if (nIndex >= 0 && nIndex < m_Polyline.size())
			{
				m_Polyline[nIndex].m_x = pPoint.m_x;
				m_Polyline[nIndex].m_y = pPoint.m_y;
				m_Polyline[nIndex].m_z = pPoint.m_z;

				if (bReCalcBox)
				{
					RecalclateBox();
				}
			}
		}

		const CHdSxPoint3D& GetBoxMinPt() const { return m_MinPt; }
		const CHdSxPoint3D& GetBoxMaxPt() const { return m_MaxPt; }

		void RecalclateBox()
		{
			size_t i;
			size_t nCount = m_Polyline.size();
			if (nCount > 0)
			{
				m_MinPt = m_MaxPt = m_Polyline[0];
				for (i = 1; i<nCount; i++)
				{
					UpdateBox(m_Polyline[i]);
				}
			}
			else
			{
				m_MinPt = m_MaxPt = CHdSxPoint3D(0.0, 0.0, 0.0);
			}
		}

		//获取量测线的类型
		ENUM_MEASURE_TYPE GetMeasureType(){return m_pMeasureType; }

		//设置量测线的类型
		void SetMeasureType(ENUM_MEASURE_TYPE pMeasureType){ m_pMeasureType = pMeasureType;}

		//三维多段线闭合
		void Close()
		{
			const CHdSxPoint3D& firstPoint = GetPoint(0);
			InsertAfter(m_Polyline.size()-1,firstPoint);
		}

	private:
		void UpdateBox(const CHdSxPoint3D& point)
		{
			if (point.m_x < m_MinPt.m_x)
				m_MinPt.m_x = point.m_x;
			else if (point.m_x > m_MaxPt.m_x)
				m_MaxPt.m_x = point.m_x;

			if (point.m_y < m_MinPt.m_y)
				m_MinPt.m_y = point.m_y;
			else if (point.m_y > m_MaxPt.m_y)
				m_MaxPt.m_y = point.m_y;

			if (point.m_z < m_MinPt.m_z)
				m_MinPt.m_z = point.m_z;
			else if (point.m_z > m_MaxPt.m_z)
				m_MaxPt.m_z = point.m_z;

			if (point.m_fCol < m_MinPt.m_fCol)
				m_MinPt.m_fCol = point.m_fCol;
			else if (point.m_fCol > m_MaxPt.m_fCol)
				m_MaxPt.m_fCol = point.m_fCol;

			if (point.m_fRow < m_MinPt.m_fRow)
				m_MinPt.m_fRow = point.m_fRow;
			else if (point.m_fRow > m_MaxPt.m_fRow)
				m_MaxPt.m_fRow = point.m_fRow;
		}
			
	public:
		bool m_bCADClose;
	private:
		//定义多线段，由多个点构成;
		vector<CHdSxPoint3D>	m_Polyline;
		//定义线段的范围
		CHdSxPoint3D			m_MinPt;
		CHdSxPoint3D			m_MaxPt;

		//量测线的类型
		ENUM_MEASURE_TYPE       m_pMeasureType;

		bool m_bCADLine;
	};


	class CHdDraftPolyLine:
		public CHdSxPolyline3D
	{
	public:
		//! constructor
		CHdDraftPolyLine(void)
			:CHdSxPolyline3D(){};
		virtual ~CHdDraftPolyLine(){};


	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_POLYLINE_ADJACENT;};
	};
}


