/*! HdStationInfo.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdStationInfo.h
相关文件     : 
文件实现功能 : 道路横断面数据
作者         : 李夏亮
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/01/15   1.0      李夏亮				创建
</PRE>
*******************************************************************************/
#pragma once
#include "HD3DPoint.h"
#include "sceneData/HdSxPolyline3D.h"
#include <vector>
using namespace std;
namespace hd
{
	//桩号数据
	class CHdStationInfo
	{
	public:

		//构造函数
		CHdStationInfo()
		{
			m_dStation = 0.0;
			m_dThickness = 0.0;
			m_dStakeLeft = 0.0;
			m_dStakeRight = 0.0;
			m_strPcdName = "";
		    m_pPolyline3D = NULL;
			m_nBegIndex = -1;
		}

		//析构函数
		virtual ~CHdStationInfo()
		{
			if (m_pPolyline3D)
			{
				delete m_pPolyline3D;
				m_pPolyline3D =NULL;
			}

			for (int i = 0; i < m_vecFeatPt.size(); i++)
			{
				if (m_vecFeatPt[i])
				{
					delete m_vecFeatPt[i];
					m_vecFeatPt[i] = NULL;
				}
			}

			m_vecFeatPt.clear();

			for (int i = 0; i < m_vecOtherPt.size(); i++)
			{
				if (m_vecOtherPt[i])
				{
					delete m_vecOtherPt[i];
					m_vecOtherPt[i] = NULL;
				}
			}

			m_vecOtherPt.clear();

		}

		////拷贝构造函数
		//CHdStationInfo::CHdStationInfo(CHdStationInfo& other)
		//{
		//	m_dStation = other.m_dStation;
		//	m_pCenterPt = other.m_pCenterPt;           
		//	m_pNormal = other.m_pNormal;            
		//	m_dThickness = other.m_dThickness;
		//	m_dStakeLeft = other.m_dStakeLeft;
		//	m_dStakeRight = other.m_dStakeRight;
		//	m_strPcdName = other.m_strPcdName;

		//	if (other.m_pPolyline3D == NULL)
		//	{
		//		m_pPolyline3D = NULL;
		//	}
		//	else
		//	{
		//		m_pPolyline3D = new CHdSxPolyline3D(*(other.m_pPolyline3D));
		//	}
		//}

		//// 赋值运算
		//CHdStationInfo& operator=(const CHdStationInfo& other)
		//{
		//	m_dStation = other.m_dStation;
		//	m_pCenterPt = other.m_pCenterPt;           
		//	m_pNormal = other.m_pNormal;            
		//	m_dThickness = other.m_dThickness;
		//	m_dStakeLeft = other.m_dStakeLeft;
		//	m_dStakeRight = other.m_dStakeRight;
		//	m_strPcdName = other.m_strPcdName;
		//	
		//	if (other.m_pPolyline3D == NULL)
		//	{
		//		if (m_pPolyline3D)
		//		{
		//			delete m_pPolyline3D;
		//			m_pPolyline3D = NULL;
		//		}
		//	}
		//	else
		//	{
		//		if (m_pPolyline3D)
		//		{
		//			delete m_pPolyline3D;
		//			m_pPolyline3D = NULL;
		//		}

		//		m_pPolyline3D = new CHdSxPolyline3D(*(other.m_pPolyline3D));
		//	}

		//	return *this;
		//}

		void CHdStationInfo::Serialize(TiXmlElement* element,bool bSave)
		{
			//临时变量
			char strTemp[128] = {0};

			//临时点
			CHdSxPoint3D pTempPt;

			//保存结构体数据
			if (bSave)
			{
				//桩号
				TiXmlElement* xmlSNElement = new TiXmlElement("StationName");
				element->LinkEndChild(xmlSNElement);
				sprintf_s(strTemp,128, "%lf", m_dStation);
				TiXmlText* xmlSNText = new TiXmlText(strTemp);
				xmlSNElement->LinkEndChild(xmlSNText);

				//截面中心坐标
				TiXmlElement* xmlElementCp = new TiXmlElement("CenterPt");
				element->LinkEndChild(xmlElementCp);
				pTempPt.m_x = m_pCenterPt.m_x;
				pTempPt.m_y = m_pCenterPt.m_y;
				pTempPt.m_z = m_pCenterPt.m_z;
				pTempPt.Serialize(xmlElementCp,true);

				//截面法向量
				TiXmlElement* xmlElementNm = new TiXmlElement("Normal");
				element->LinkEndChild(xmlElementNm);
				pTempPt.m_x = m_pNormal.m_x;
				pTempPt.m_y = m_pNormal.m_y;
				pTempPt.m_z = m_pNormal.m_z;
				pTempPt.Serialize(xmlElementNm,true);

				//截面厚度
				TiXmlElement* xmlElementTn = new TiXmlElement("Thickness");
				element->LinkEndChild(xmlElementTn);
				memset(strTemp,0,sizeof(strTemp));
				sprintf_s(strTemp,128, "%lf", m_dThickness);
				TiXmlText* xmlTextTn = new TiXmlText(strTemp);
				xmlElementTn->LinkEndChild(xmlTextTn);

				//桩左距离
				TiXmlElement* xmlElementSl = new TiXmlElement("StakeLeft");
				element->LinkEndChild(xmlElementSl);
				memset(strTemp,0,sizeof(strTemp));
				sprintf_s(strTemp,128, "%lf", m_dStakeLeft);
				TiXmlText* xmlTextsl = new TiXmlText(strTemp);
				xmlElementSl->LinkEndChild(xmlTextsl);

				//桩右距离
				TiXmlElement* xmlElementSr = new TiXmlElement("StakeRight");
				element->LinkEndChild(xmlElementSr);
				memset(strTemp,0,sizeof(strTemp));
				sprintf_s(strTemp,128, "%lf", m_dStakeRight);
				TiXmlText* xmlTextSr = new TiXmlText(strTemp);
				xmlElementSr->LinkEndChild(xmlTextSr);

				//点云名称
				TiXmlElement* xmlElementPcd = new TiXmlElement("PcdName");
				element->LinkEndChild(xmlElementPcd);
				TiXmlText* xmlTextPcd = new TiXmlText(m_strPcdName.c_str());
				xmlElementPcd->LinkEndChild(xmlTextPcd);

				//特征线数据
				if (m_pPolyline3D)
				{
					m_pPolyline3D->Serialize(element,true);
				}
			}
			else   
			{
				//读取结构体数据
				string strValue;

				string strText;

				TiXmlElement* nextElement = element->FirstChildElement();
				while(nextElement)
				{
					strValue = nextElement->Value();

					//桩号
					if (strValue == "StationName")
					{
						strText = nextElement->GetText();

						if (strText != "")
						{
							sscanf_s(strText.data(), "%lf", &m_dStation);
						}
					}

					//截面中心坐标
					if (strValue == "CenterPt")
					{
						TiXmlElement* pElement = nextElement->FirstChildElement();

						if (pElement)
						{
							strValue = pElement->Value();

							if (strValue == "Point")
							{
								pTempPt.Serialize(pElement,false);

								m_pCenterPt.m_x = pTempPt.m_x;
								m_pCenterPt.m_y = pTempPt.m_y;
								m_pCenterPt.m_z = pTempPt.m_z;
							}
						}
					}

					//截面法向量
					if (strValue == "Normal")
					{
						TiXmlElement* pElement = nextElement->FirstChildElement();

						if (pElement)
						{
							strValue = pElement->Value();

							if (strValue == "Point")
							{
								pTempPt.Serialize(pElement,false);

								m_pNormal.m_x = pTempPt.m_x;
								m_pNormal.m_y = pTempPt.m_y;
								m_pNormal.m_z = pTempPt.m_z;
							}
						}
					}

					//截面厚度
					if (strValue == "Thickness")
					{
						strText = nextElement->GetText();

						if (strText != "")
						{
							sscanf_s(strText.data(), "%lf", &m_dThickness);
						}
					}

					//桩左距离
					if (strValue == "StakeLeft")
					{
						strText = nextElement->GetText();

						if (strText != "")
						{
							sscanf_s(strText.data(), "%lf", &m_dStakeLeft);
						}
					}

					//桩右距离
					if (strValue == "StakeRight")
					{
						strText = nextElement->GetText();

						if (strText != "")
						{
							sscanf_s(strText.data(), "%lf", &m_dStakeRight);
						}
					}

					//点云名称
					if (strValue == "PcdName")
					{
						strText = nextElement->GetText();

						if (strText != "")
						{
							m_strPcdName = strText;
						}
					}

					//特征线
					if (strValue == "Polyline")
					{
						if (m_pPolyline3D)
						{
							delete m_pPolyline3D;
							m_pPolyline3D = NULL;
						}
						m_pPolyline3D = new CHdSxPolyline3D;
						m_pPolyline3D->Serialize(nextElement,false);
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}

	private:
		//拷贝构造函数
		CHdStationInfo::CHdStationInfo(CHdStationInfo& other);

		// 赋值运算
		CHdStationInfo& operator=(const CHdStationInfo& other);


		

	public:
		double m_dStation;                //桩号
		CHD3DPoint m_pCenterPt;           //截面中心坐标
		CHD3DPoint m_pNormal;             //截面法向量
		double m_dThickness;                //截面厚度
		double m_dStakeLeft;                //桩左距离
		double m_dStakeRight;               //桩右距离
		string m_strPcdName;                //点云名称
		CHdSxPolyline3D* m_pPolyline3D;      //特征线数据
		int m_nBegIndex;                     //起始测点编号
		vector<CHdSxPoint3D*> m_vecFeatPt;    //特征点
		vector<CHdSxPoint3D*> m_vecOtherPt;   //其他点
	};
}
