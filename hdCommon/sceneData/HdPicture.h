/*! CHdPicture.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : CHdPicture.h
相关文件     :  

文件实现功能 : 定义图片对象类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/08/07   1.0      姚立   
2013/03/07   1.1      龚书林			 修改继承
</PRE>
*******************************************************************************/
#pragma once
#include "hdImgCtrlPoint.h"
#include <string>
using namespace std;

namespace hd
{
	class CHdPicture
	{
	public:
		CHdPicture():m_bSphere(false), m_fHoriAngle(0.0f), m_fVertAngle(0.0f),m_strFullName(""),
		m_x(0.0f),m_y(0.0f),m_z(0.0f),m_yaw(0.0f),m_pitch(0.0f),m_roll(0.0f),m_bRegistrated(false){}
		CHdPicture(const char* strName):m_bSphere(false), m_fHoriAngle(0.0f), m_fVertAngle(0.0f),
			m_x(0.0f),m_y(0.0f),m_z(0.0f),m_yaw(0.0f),m_pitch(0.0f),m_roll(0.0f),m_bRegistrated(false)
		{ 
			m_strFullName = strName; 
		}
		virtual ~CHdPicture() 
		{
			removeAll();
		}

		inline const CHdPicture& operator=(const CHdPicture& other)
		{
			m_bSphere = other.m_bSphere;
			m_fHoriAngle = other.m_fHoriAngle;
			m_fVertAngle = other.m_fVertAngle;
			m_strFullName = other.m_strFullName;

			removeAll();
			for (unsigned int i = 0;i<other.m_ImgCtrlPts.size();i++)
			{
				HdImgCtrlPoint* pt = new HdImgCtrlPoint();
				(*pt) = (*other.m_ImgCtrlPts[i]);
				m_ImgCtrlPts.push_back(pt);
			}
			return *this;
		}

		void SetRegistrated(bool bRegistrated){ m_bRegistrated = bRegistrated; }
		bool GetRegistrated(){ return m_bRegistrated; }

		//! 获取点个数 gsl 2012/08/08 
		inline size_t GetCount() const {return m_ImgCtrlPts.size();}
		inline int GetMaxID() const 
		{
			int maxID = 0;
			for(unsigned int i = 0; i< m_ImgCtrlPts.size();i++)
			{
				if (m_ImgCtrlPts[i]->m_ctrlptID > maxID)
				{
					maxID = m_ImgCtrlPts[i]->m_ctrlptID;
				}
			}
			return maxID;
		}
		//! 添加控制点 gsl 2012/08/08 
		inline HdImgCtrlPoint* AddPoint(const HdImgCtrlPoint& pt)
		{
			HdImgCtrlPoint* ptNew = new HdImgCtrlPoint();
			(*ptNew) = pt;
			//int maxID = 0;
			//for(unsigned int i = 0; i< m_ImgCtrlPts.size();i++)
			//{
			//	if (m_ImgCtrlPts[i]->m_ctrlptID > maxID)
			//	{
			//		maxID = m_ImgCtrlPts[i]->m_ctrlptID;
			//	}
			//}
			m_ImgCtrlPts.push_back(ptNew);
			ptNew->m_ctrlptID -= 1;//m_ImgCtrlPts.size();
			return ptNew;
		}

		inline HdImgCtrlPoint* AddPoint(const CHdMCamImgCtrlPoint& pt)
		{
			CHdMCamImgCtrlPoint* ptNew = new CHdMCamImgCtrlPoint();
			(*ptNew) = pt;
			//int maxID = 0;
			//for(unsigned int i = 0; i< m_ImgCtrlPts.size();i++)
			//{
			//	if (m_ImgCtrlPts[i]->m_ctrlptID > maxID)
			//	{
			//		maxID = m_ImgCtrlPts[i]->m_ctrlptID;
			//	}
			//}
			m_ImgCtrlPts.push_back(ptNew);
			//ptNew->m_ctrlptID -= 1;//m_ImgCtrlPts.size();
			return ptNew;
		}
		//! 获取一个控制点 gsl 2012/08/08 
		inline HdImgCtrlPoint* GetPoint(int i)
		{
			size_t nCount =  m_ImgCtrlPts.size();
			if (i < 0 || i >= (int)nCount)
			{
				return NULL;
			}
			return m_ImgCtrlPts[i];
		}
		//! 移除一个控制点 gsl 2012/08/08 
		inline void removePoint(int i)
		{
			size_t nCount =  m_ImgCtrlPts.size();
			if (i < 0 || i >= (int)nCount)
			{
				return;
			}
				
			if (i >= 0 && i < (int)nCount)
			{
				delete m_ImgCtrlPts[i];	
				m_ImgCtrlPts.erase(m_ImgCtrlPts.begin() + i);
			}

			// 删除点后要更新点号
			//if (i != nCount)
			//{
			//	for (int n=0;n< nCount - 1;n++)
			//	{
			//		m_ImgCtrlPts[n]->m_ctrlptID = n + 1;
			//	}
			//}
		}

		//! 获取一个控制点 gsl 2012/08/08 
		inline HdImgCtrlPoint* GetPointByImgID(int ptCtrlIndex)
		{
			size_t nCount =  m_ImgCtrlPts.size();
			for (int i= 0; i < nCount; i++)
			{
				// 删除指定控制点序号 
				if (m_ImgCtrlPts[i]->m_ctrlptID == ptCtrlIndex)
				{
					return m_ImgCtrlPts[i];
				}
			}
			return NULL;
		}

		//! 根据指定的控制点号，移除一个控制点fengjing
		inline void removePointByImgID(int ptCtrlIndex)
		{
			size_t nCount =  m_ImgCtrlPts.size();
			for (int i= 0; i < nCount; i++)
			{
				// 删除指定控制点序号 
			   if (m_ImgCtrlPts[i]->m_ctrlptID == ptCtrlIndex)
			   {
					delete m_ImgCtrlPts[i];	
					m_ImgCtrlPts[i] = NULL;

					m_ImgCtrlPts.erase(m_ImgCtrlPts.begin() + i);
					break;
			   }
			}
		}

		//! 设置全景球体姿态
		inline void setSpherePos(float x,float y,float z,float yaw,float pitch,float roll)
		{
			m_x = x;
			m_y = y;
			m_z = z;
			m_yaw = yaw;
			m_pitch = pitch;
			m_roll = roll;
		}
		//! 移除所有控制点
		inline void removeAll()
		{
			size_t nCount = m_ImgCtrlPts.size();
			for (int i=0;i< (int)nCount;i++)
			{
				delete m_ImgCtrlPts.at(i);
			}
			m_ImgCtrlPts.clear();
		}

		inline void Serialize(const char* strScanPath, TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* pictureElement = new TiXmlElement("Picture");
				element->LinkEndChild(pictureElement);

				xmlElement = new TiXmlElement("bSphere");
				pictureElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%d", m_bSphere);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("HoriAngle");
				pictureElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%.6lf", m_fHoriAngle);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("VertAngle");
				pictureElement->LinkEndChild(xmlElement);
				sprintf_s(strTemp,128, "%.6lf", m_fVertAngle);
				xmlText = new TiXmlText(strTemp);
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("Name");
				pictureElement->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(m_strFullName.substr(m_strFullName.find_last_of('\\')+1).data());
				xmlElement->LinkEndChild(xmlText);

				xmlElement = new TiXmlElement("ImageCtrlPoints");
				pictureElement->LinkEndChild(xmlElement);
				unsigned int i;
				unsigned int nCount = (unsigned int)m_ImgCtrlPts.size();
				for (i = 0; i<nCount; i++)
				{
					m_ImgCtrlPts[i]->Serialize(xmlElement, true);
				}

				char tmpStr[128] = {0};
				xmlElement = new TiXmlElement("SpherePos");

				sprintf_s(tmpStr,128, "%.6lf", m_x);
				xmlElement->SetAttribute("x",tmpStr);
				sprintf_s(tmpStr,128, "%.6lf", m_y);
				xmlElement->SetAttribute("y",tmpStr);
				sprintf_s(tmpStr,128, "%.6lf", m_z);
				xmlElement->SetAttribute("z",tmpStr);

				sprintf_s(tmpStr,128, "%.6lf", m_yaw);
				xmlElement->SetAttribute("yaw",tmpStr);
				sprintf_s(tmpStr,128, "%.6lf", m_pitch);
				xmlElement->SetAttribute("pitch",tmpStr);
				sprintf_s(tmpStr,128, "%.6lf", m_roll);
				xmlElement->SetAttribute("roll",tmpStr);

				pictureElement->LinkEndChild(xmlElement);

				xmlElement = new TiXmlElement("IsRegistrated");
				pictureElement->LinkEndChild(xmlElement);
				int nRegistrated = m_bRegistrated ? 1 : 0;
				sprintf(strTemp, "%d", nRegistrated);
				xmlElement->LinkEndChild(new TiXmlText(strTemp));
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

					if (strValue == "bSphere")
					{
						int nSphere = 0;
						sscanf_s(strText.data(), "%d", &nSphere);
						m_bSphere = (nSphere!=0);
					}
					else if (strValue == "HoriAngle")
					{
						sscanf_s(strText.data(), "%lf", &m_fHoriAngle);
					}
					else if (strValue == "VertAngle")
					{
						sscanf_s(strText.data(), "%lf", &m_fVertAngle);
					}
					else if (strValue == "Name")
					{
						m_strFullName = strScanPath;
						m_strFullName += "Pictures\\" + strText;
					}
					else if (strValue == "ImageCtrlPoints")
					{
						TiXmlElement* imgCtrlPtElement = nextElement->FirstChildElement();
						int ctrlID = 1;
						while (imgCtrlPtElement)
						{
							HdImgCtrlPoint* newImgCtrlPt = new HdImgCtrlPoint();
							newImgCtrlPt->Serialize(imgCtrlPtElement, false);
							newImgCtrlPt->m_ctrlptID = ctrlID;
							ctrlID++;
							m_ImgCtrlPts.push_back(newImgCtrlPt);
							imgCtrlPtElement = imgCtrlPtElement->NextSiblingElement();
						}
					}
					else if (strValue == "SpherePos")
					{
						m_x = atof(nextElement->Attribute("x"));
						m_y = atof(nextElement->Attribute("y"));
						m_z = atof(nextElement->Attribute("z"));
						m_yaw = atof(nextElement->Attribute("yaw"));
						m_pitch = atof(nextElement->Attribute("pitch"));
						m_roll = atof(nextElement->Attribute("roll"));
					}
					else if (strValue == "IsRegistrated")
					{
						int nRegistrated = atoi(strText.data());
						m_bRegistrated = nRegistrated > 0 ? true : false;
					}

					nextElement = nextElement->NextSiblingElement();
				}
			}
		}

	public:
		bool		m_bSphere;			//指示图片是否是球面图片
		double		m_fHoriAngle;		//指示图片拍摄时的水平角
		double		m_fVertAngle;		//指示图片拍摄时的垂直角
		string		m_strFullName;		//指示图片文件所在的全路径
		vector<HdImgCtrlPoint*>	m_ImgCtrlPts;	//图片控制点列表
		bool        m_bSuppment;    // 是否补齐照片
		double		m_x;		    //全景球体位置
		double		m_y;		    //全景球体位置
		double		m_z;		    //全景球体位置
		double		m_yaw;			//航向角
		double		m_pitch;		//俯仰
		double		m_roll;			//翻滚

		bool        m_bRegistrated;     // 是否已配准
	};

	typedef struct HD_MPIC_PARAM 
	{
		// 照片名
		string sImageName;

		// 类型
		int type;

		// 水平角
		float fHoriAngle;

		// 垂直角
		float fVertiAngle;

		// 水平视角范围
		float fHoriAngleRange;

		// 垂直视角范围
		float fVertiAngleRange;

		inline void Serialize(TiXmlElement* element,bool bSave)
		{
			// 保存
			if (bSave)
			{
				char strText[128];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				// 照片名
				xmlElement = new TiXmlElement("name");
				element->LinkEndChild(xmlElement);
				xmlText = new TiXmlText(sImageName.c_str());
				xmlElement->LinkEndChild(xmlText);

				// 类型为M-picture
				xmlElement = new TiXmlElement("type");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%d", type);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// 水平角
				xmlElement = new TiXmlElement("HoriAngle");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%f", fHoriAngle);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// 垂直角
				xmlElement = new TiXmlElement("VertiAngle");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%f", fVertiAngle);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// 水平视角范围
				xmlElement = new TiXmlElement("HoriAngleRange");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%f", fHoriAngleRange);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// 垂直视角范围
				xmlElement = new TiXmlElement("VertiAngleRange");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%f", fVertiAngleRange);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);
			}
			else
			{
				string strValue;
				string strText;
				TiXmlElement* paramChildElement = element->FirstChildElement();
				while(paramChildElement)
				{
					strValue = paramChildElement->Value();
					if (paramChildElement->GetText() == NULL)
					{
						strText = "";
					}
					else
						strText = paramChildElement->GetText();

					// 照片名
					if (strValue == "name")
					{
						sImageName = strText;
					}
					else if (strValue == "type")
					{
						type = (int)(atoi(strText.data()));
					}
					else if (strValue == "HoriAngle")
					{
						fHoriAngle = (float)(atof(strText.data()));
					}
					else if (strValue == "VertiAngle")
					{
						fVertiAngle = (float)(atof(strText.data()));
					}
					else if (strValue == "HoriAngleRange")
					{
						fHoriAngleRange = (float)(atof(strText.data()));
					}
					else if (strValue == "VertiAngleRange")
					{
						fVertiAngleRange = (float)(atof(strText.data()));
					}

					paramChildElement = paramChildElement->NextSiblingElement();
				}
			}
		}
	};

	typedef struct HD_MCAM_INNPARAM 
	{
		// 类型 0-ZFI内置相机 1-HDSY D800 14mm广角 2-HDSY D800 鱼眼
		int type;

		// fx
		double fx;

		// fy
		double fy;

		// cx
		double cx;

		// cy
		double cy;

		// k1
		double k1;

		// k2
		double k2;

		// k3
		double k3;

		// k4
		double k4;

		// k5
		double k5;

		// k6
		double k6;

		// p1
		double p1;

		// p2
		double p2;

		// lmd1
		double lmd1;

		// lmd2
		double lmd2;

		// lmd3
		double lmd3;

		// armX
		double fArmX;

		// armY
		double fArmY;

		// armZ
		double fArmZ;

		// phi
		double fRPhi;

		// omega
		double fROmg;

		// kappa
		double fRKap;

		// piccount 相片个数
		int nCount;

		inline void InitValue()
		{
			type = 0;
			nCount = 42;
			fx = fy = cx = cy = k1 = k2 = k3 = k4 = k5 = k6 = p1 = p2 = lmd1 = lmd2 = lmd3 = 0;
			fArmX = fArmY = fArmZ = fRPhi = fROmg = fRKap = 0;
		}

		inline void Serialize(TiXmlElement* element,bool bSave)
		{
			// 保存
			if (bSave)
			{
				char strText[128];
			
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;
				
				// type
				xmlElement = new TiXmlElement("type");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%d", type);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// type
				xmlElement = new TiXmlElement("count");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%d", nCount);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// fx
				xmlElement = new TiXmlElement("fx");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fx);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// fy
				xmlElement = new TiXmlElement("fy");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fy);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// cx
				xmlElement = new TiXmlElement("cx");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", cx);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// cy
				xmlElement = new TiXmlElement("cy");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", cy);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				if (type == 0)
				{
					// k1
					xmlElement = new TiXmlElement("k1");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k1);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// k2
					xmlElement = new TiXmlElement("k2");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k2);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

				}
				else if (type == 1)
				{
					// k1
					xmlElement = new TiXmlElement("k1");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k1);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// k2
					xmlElement = new TiXmlElement("k2");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k2);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// k3
					xmlElement = new TiXmlElement("k3");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k3);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// k4
					xmlElement = new TiXmlElement("k4");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k4);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);
					
					// k5
					xmlElement = new TiXmlElement("k5");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k5);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// k6
					xmlElement = new TiXmlElement("k6");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", k6);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// p1
					xmlElement = new TiXmlElement("p1");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", p1);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// p2
					xmlElement = new TiXmlElement("p2");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", p2);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);
				}
				else if (type == 2)
				{
					// lmd1
					xmlElement = new TiXmlElement("lmd1");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", lmd1);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// lmd2
					xmlElement = new TiXmlElement("lmd2");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", lmd2);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);

					// lmd3
					xmlElement = new TiXmlElement("lmd3");
					element->LinkEndChild(xmlElement);
					sprintf_s(strText,128, "%lf", lmd3);	
					xmlText = new TiXmlText(strText);
					xmlElement->LinkEndChild(xmlText);
				}

				// armx
				xmlElement = new TiXmlElement("ArmX");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fArmX);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// army
				xmlElement = new TiXmlElement("ArmY");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fArmY);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// armz
				xmlElement = new TiXmlElement("ArmZ");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fArmZ);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// phi
				xmlElement = new TiXmlElement("Phi");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fRPhi);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// omega
				xmlElement = new TiXmlElement("Omega");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fROmg);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);

				// kappa
				xmlElement = new TiXmlElement("Kappa");
				element->LinkEndChild(xmlElement);
				sprintf_s(strText,128, "%lf", fRKap);	
				xmlText = new TiXmlText(strText);
				xmlElement->LinkEndChild(xmlText);
			}
			else
			{
				InitValue();
				string strValue;
				string strText;
				TiXmlElement* paramChildElement = element->FirstChildElement();
				while(paramChildElement)
				{
					strValue = paramChildElement->Value();
					if (paramChildElement->GetText() == NULL)
					{
						strText = "";
					}
					else
						strText = paramChildElement->GetText();

					if (strValue == "type")
					{
						type = atoi(strText.data());
					}
					if (strValue == "count")
					{
						nCount = atoi(strText.data());
					}
					// 
					if (strValue == "fx")
					{
						fx = atof(strText.data());
					}
					else if (strValue == "fy")
					{
						fy = atof(strText.data());
					}
					else if (strValue == "cx")
					{
						cx = atof(strText.data());
					}
					else if (strValue == "cy")
					{
						cy = atof(strText.data());
					}
					else if (strValue == "k1")
					{
						k1 = atof(strText.data());
					}
					else if (strValue == "k2")
					{
						k2 = atof(strText.data());
					}
					else if (strValue == "k3")
					{
						k3 = atof(strText.data());
					}
					else if (strValue == "k4")
					{
						k4 = atof(strText.data());
					}
					else if (strValue == "k5")
					{
						k5 = atof(strText.data());
					}
					else if (strValue == "k6")
					{
						k6 = atof(strText.data());
					}
					else if (strValue == "p1")
					{
						p1 = atof(strText.data());
					}
					else if (strValue == "p2")
					{
						p2 = atof(strText.data());
					}
					else if (strValue == "lmd1")
					{
						lmd1 = atof(strText.data());
					}
					else if (strValue == "lmd2")
					{
						lmd2 = atof(strText.data());
					}
					else if (strValue == "lmd3")
					{
						lmd3 = atof(strText.data());
					}
					else if (strValue == "ArmX")
					{
						fArmX = atof(strText.data());
					}
					else if (strValue == "ArmY")
					{
						fArmY = atof(strText.data());
					}
					else if (strValue == "ArmZ")
					{
						fArmZ = atof(strText.data());
					}
					else if (strValue == "Phi")
					{
						fRPhi = atof(strText.data());
					}
					else if (strValue == "Omega")
					{
						fROmg = atof(strText.data());
					}
					else if (strValue == "Kappa")
					{
						fRKap = atof(strText.data());
					}
					paramChildElement = paramChildElement->NextSiblingElement();
				}
			}
		}

		inline void SerializeParm(const char* paramPath,bool bSave)
		{
			// 根据是保存至文件还是读取文件信息分别处理
			if (bSave)
			{
				TiXmlDocument doc;
				TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
				TiXmlElement* rootElement = new TiXmlElement("HDScenePicture");

				doc.LinkEndChild(decl);
				doc.LinkEndChild(rootElement);
									

				// CameraInfo
				TiXmlElement* Camera = new TiXmlElement("CameraInfo");
				rootElement->LinkEndChild(Camera);

				// 写入所有子节点信息
				Serialize(Camera,bSave);

				// 写入完成后，保存参数文件
				doc.SaveFile(paramPath);
			}
			else
			{
				// 从文件中读取参数
				// 配置文件目录
				TiXmlDocument doc(paramPath);
				if (!doc.LoadFile())
				{
					return;
				}	

				TiXmlElement* rootElement = doc.RootElement();
				if (rootElement)
				{
					string strValue;
					string strText;

					//判断是否为HDScenePicture文件;
					strValue = rootElement->Value();
					if (strValue != "HDScenePicture")
					{
						return;
					}
					TiXmlElement* firstElement = rootElement->FirstChildElement();
					TiXmlElement* nextElement = firstElement;
					while (nextElement)
					{
						strValue = nextElement->Value();
						if (strValue == "CameraInfo")
						{
							Serialize(nextElement,false);
						}
						else
						{
							break;
						}
						nextElement = nextElement->NextSiblingElement();
					}
				}
			}
		}
	};
}