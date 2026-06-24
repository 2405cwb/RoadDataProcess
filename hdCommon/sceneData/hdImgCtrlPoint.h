/*! HdLabel.h
********************************************************************************
<PRE>
模块名       : hdWorkspace
文件名       : hdImgCtrlPoint.h
相关文件     : HDObject.h 

文件实现功能 : 影像配准控制点
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/08/08   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdobject.h"
#include "..\hdCore\hdMath.h"
#include <vector>

using namespace std;

namespace hd
{
	class HdImgCtrlPoint :
		public CHDObject
	{
	public:
		double m_imageX;	//对应的图片水平方向比例
		double m_imageY;	//对应的图片垂直方向比例

		double m_fX;
		double m_fY;
		double m_fZ;

		double m_fRow;	//对应的点云的行比例
		double m_fCol;	//对应的点云的列比例

		int   m_ctrlptID;  // 控制点 ID

		int   m_nImageID;  // 全景图片 ID（全景与 HLZ 点云配准时使用  朱立雄 2017-1-19）

	public:
		HdImgCtrlPoint(void)
			:m_fX(F64_MIN), m_fY(F64_MIN),m_fZ(F64_MIN),
			m_fRow(-1.0f), m_fCol(-1.0f),m_imageX(-1.0f),m_imageY(-1.0f)
		{ 
			m_ctrlptID = -1;
		}
		virtual ~HdImgCtrlPoint(void){}
			
		bool isValid()
		{
			return m_fRow != -1.0f && m_fCol != -1.0f && m_imageX != -1.0f && m_imageY != -1.0f;
		}
		void setPoint(double imgX,double imgY,double colScale,double rowScale,double x,double y,double z)
		{
			m_imageX = imgX;
			m_imageY = imgY;
			m_fCol = colScale;
			m_fRow = rowScale;
			m_fX = x;
			m_fY = y;
			m_fZ = z;
		}

		bool IsImageCoordValid(){return m_imageX >= 0.0 && m_imageX <= 1.0 && m_imageY >= 0.0 && m_imageY <= 1.0;}

		bool Is3DCoordValid(){return m_fX > -1.0e100 && m_fY > -1.0e100 && m_fZ > -1.0e100;}

		void SetImageCoordinate(double X, double Y){m_imageX = X; m_imageY = Y;}

		void Set3DCoordinate(double X, double Y, double Z){m_fX = X; m_fY = Y; m_fZ = Z;}

		void SetPointID(int nID){m_ctrlptID = nID;}

		void SetImageID(int nImageID){m_nImageID = nImageID;}

		int GetPointID() const{return m_ctrlptID;}

		int GetImageID(){return m_nImageID;}

	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_IMAGE_CTRLPT; }
		
		//! element为ImageCtrlPoints节点
		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[32];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* ctrlPtElement = new TiXmlElement("ImageCtrlPoint");
					
				sprintf_s(strTemp,32, "%.6lf", m_imageX);
				ctrlPtElement->SetAttribute("ImageX",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_imageY);
				ctrlPtElement->SetAttribute("ImageY",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fCol);
				ctrlPtElement->SetAttribute("Col",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fRow);
				ctrlPtElement->SetAttribute("Row",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fX);
				ctrlPtElement->SetAttribute("X",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fY);
				ctrlPtElement->SetAttribute("Y",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fZ);
				ctrlPtElement->SetAttribute("Z",strTemp);

				element->LinkEndChild(ctrlPtElement);
			}
			else
			{
				string strValue;
				m_imageX = (atof(element->Attribute("ImageX")));
				m_imageY = (atof(element->Attribute("ImageY")));
				m_fCol = (atof(element->Attribute("Col")));
				m_fRow = (atof(element->Attribute("Row")));
				m_fX = (atof(element->Attribute("X")));
				m_fY = (atof(element->Attribute("Y")));
				m_fZ = (atof(element->Attribute("Z")));
			}
		}

	};

	// M-Cam相机标定控制点
	class CHdMCamImgCtrlPoint :
		public HdImgCtrlPoint
	{
		// 属性
	public:
		string m_PicName;			// 控制点所在的图片名
		int m_nPicIndex;			// 图片的索引
		double m_ImgX;				// 图片的列号
		double m_ImgY;				// 图片的行号

	public:
		CHdMCamImgCtrlPoint(void)
			:HdImgCtrlPoint()
		{ 
			m_ctrlptID = -1;
			m_nPicIndex = -1;
			m_ImgX = -1.f;
			m_ImgY = -1.f;
			m_PicName = "";
		}
		CHdMCamImgCtrlPoint(int nPicdIndex,double imgX,double imgY)
			:HdImgCtrlPoint()
		{
			m_ctrlptID = -1;
			m_nPicIndex = nPicdIndex;
			m_ImgX = imgX;
			m_ImgY = imgY;
			m_PicName = "";
		}
		virtual ~CHdMCamImgCtrlPoint(void){}

		bool isValid()
		{
			return m_fRow != -1.0f && m_fCol != -1.0f && m_imageX != -1.0f && m_imageY != -1.0f
				&& m_ImgX != -1.f && m_ImgY != -1.f;
		}
		void setPoint(string picName,double imgX,double imgY,double imgXRatio,double imgYRatio,double colScale,double rowScale,double x,double y,double z)
		{
			HdImgCtrlPoint::setPoint(imgXRatio,imgYRatio,colScale,rowScale,x,y,z);
			m_PicName = picName;
			m_ImgX = imgX;
			m_ImgY = imgY;
		}
	public:
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_MCAM_CTRLPT; }

		//! element为ImageCtrlPoints节点
		inline virtual void Serialize(TiXmlElement* element, bool bSave)
		{
			if (bSave)
			{
				char strTemp[32];
				TiXmlText* xmlText = NULL;
				TiXmlElement* xmlElement = NULL;

				TiXmlElement* ctrlPtElement = new TiXmlElement("MCamImageCtrlPoint");

				sprintf_s(strTemp,32, "%.6lf", m_ImgX);
				ctrlPtElement->SetAttribute("ImageX",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_ImgY);
				ctrlPtElement->SetAttribute("ImageY",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_imageX);
				ctrlPtElement->SetAttribute("ImageXRatio",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_imageY);
				ctrlPtElement->SetAttribute("ImageYRatio",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fCol);
				ctrlPtElement->SetAttribute("Col",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fRow);
				ctrlPtElement->SetAttribute("Row",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fX);
				ctrlPtElement->SetAttribute("X",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fY);
				ctrlPtElement->SetAttribute("Y",strTemp);

				sprintf_s(strTemp,32, "%.6lf", m_fZ);
				ctrlPtElement->SetAttribute("Z",strTemp);

				element->LinkEndChild(ctrlPtElement);
			}
			else
			{
				string strValue;
				m_ImgX = (atof(element->Attribute("ImageX")));
				m_ImgY = (atof(element->Attribute("ImageY")));
				m_imageX = (atof(element->Attribute("ImageXRatio")));
				m_imageY = (atof(element->Attribute("ImageYRatio")));
				m_fCol = (atof(element->Attribute("Col")));
				m_fRow = (atof(element->Attribute("Row")));
				m_fX = (atof(element->Attribute("X")));
				m_fY = (atof(element->Attribute("Y")));
				m_fZ = (atof(element->Attribute("Z")));
			}
		}

	};
}


