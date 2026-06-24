/*! HdCtrlPoint.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdCtrlPoint.h
相关文件     : HDObject.h 

文件实现功能 : 定义拼接控制点（同名点）对象类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/07/05   1.0      姚立    
2013/03/07   1.1      龚书林
</PRE>
*******************************************************************************/
#pragma once
#include "..\hdobject.h"
#include "HdSxPoint3D.h"
#include <vector>

using namespace std;

namespace hd
{
	class CHdCtrlPoint :
		public CHDObject
	{
	public:
		string							m_strName;		//记录同名点点名
		std::vector<CHdSxPoint3D*>		m_pPositions;	//记录同名点在不同测站中的位置（三维坐标）
		std::vector<int>				m_scanIndexs;	//记录同名点所在的测站索引，与上面的位置一一对应

	public:
		CHdCtrlPoint(void):m_strName("Control Point") {}
		CHdCtrlPoint(const char* strName):m_strName(strName) {}
		virtual ~CHdCtrlPoint(void) 
		{
			for (std::vector<CHdSxPoint3D*>::iterator it = m_pPositions.begin();
				it != m_pPositions.end();it++)
			{
				if (*it != NULL)
				{
					delete *it;
				}
			}
		}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_CTRLPT; }
		
		inline bool getPoint(int nScanIndex,CHdSxPoint3D** pt)
		{
			size_t i;
			size_t nCtrlPt = m_pPositions.size();
			for (i = 0; i<nCtrlPt; i++)
			{
				if (m_scanIndexs[i] == nScanIndex)
				{
					//CHdSxPoint3D& tmpPt = m_pPositions[i];
					(*pt) = m_pPositions[i];//&tmpPt;
					return true;
				}
			}
			return false;
		}

		inline void setCtrlPoint(int nScanIndex, float x, float y, float z, float fCol = -1, float fRow = -1)
		{
			size_t i, j;
			size_t nCtrlPt = m_pPositions.size();
			for (i = 0; i<nCtrlPt; i++)
			{
				if (m_scanIndexs[i] == nScanIndex)
				{
					CHdSxPoint3D* pt = m_pPositions .at(i);
					pt->m_x = x;
					pt->m_y = y;
					pt->m_z = z;
					pt->m_fRow = fRow;
					pt->m_fCol = fCol;
					break;
				}
			}

			if (i == nCtrlPt)//没有找到包含此站索引的点
			{
				bool bInserted = false;
				CHdSxPoint3D* newPt = new CHdSxPoint3D(x,y,z);
				newPt->setRowColRatio(fRow, fCol);
				//按测站索引顺序插入点
				for (j = 0; j<nCtrlPt; j++)
				{
					if (nScanIndex < m_scanIndexs[j])
					{
						m_pPositions.insert(m_pPositions.begin()+j, newPt);
						m_scanIndexs.insert(m_scanIndexs.begin()+j, nScanIndex);

						bInserted = true;
						break;
					}
				}

				//没有插入点，则放在队列最后面
				if (!bInserted)
				{
					m_pPositions.push_back(newPt);
					m_scanIndexs.push_back(nScanIndex);
				}
			}
		}		

		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[32];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* ctrlPtElement = new TiXmlElement("ControlPoint");
				element->LinkEndChild(ctrlPtElement);

				xmlElement = new TiXmlElement("Name");
				ctrlPtElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strName.data());
				xmlElement->LinkEndChild(xmlText);

				size_t i;
				size_t nCtrlPt = m_pPositions.size();
				for (i = 0; i<nCtrlPt; i++)
				{
					m_pPositions[i]->Serialize(ctrlPtElement, bSave);

					xmlElement = new TiXmlElement("ScanIndex");
					ctrlPtElement->LinkEndChild(xmlElement);
					sprintf_s(strTemp, 32,"%d", m_scanIndexs[i]);
					xmlText = new TiXmlText(strTemp);
					xmlElement->LinkEndChild(xmlText);
				}
			}
			else
			{
				string strValue;
				TiXmlElement* nextElement = element->FirstChildElement();
				while(nextElement)
				{
					strValue = nextElement->Value();

					if (strValue == "Name")
					{
						m_strName = nextElement->GetText();
					}
					else if (strValue == "Point")
					{
						CHdSxPoint3D* newPt = new CHdSxPoint3D;
						newPt->Serialize(nextElement, bSave);
						m_pPositions.push_back(newPt);
					}
					else if (strValue == "ScanIndex")
					{
						int nScanIndex = -1;
						string strText = nextElement->GetText();
						sscanf_s(strText.data(), "%d", &nScanIndex);
						m_scanIndexs.push_back(nScanIndex);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}
	};
}


