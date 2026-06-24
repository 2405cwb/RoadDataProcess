/*! @file
********************************************************************************
<PRE>
模块名       : ProjectManager
文件名       : hdSceneScan.h
相关文件     : hdSceneScan.cpp
文件实现功能 : 定义扫描数据类，扫描数据中，包含各种类型的数据，例如扫描点云数据、图片
				数据、内存对象数据等
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/29   1.0      姚立                新增加文件
2012/07/03			  杨峰				  添加靶球对象
2013/03/07   1.1      龚书林			 修改继承
</PRE>
*******************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "..\hdCommon.h"
#include "Hd3LSScanStruct.h"
#include "HdSxPoint3D.h"
#include "HdSxPolyline3D.h"
#include "HdSxPolyline2D.h"
#include "HdLabel.h"
#include "HdSphere.h"
#include "HdPlane.h"
#include "HdKeyboard.h"
#include "HdCtrlPoint.h"
#include "HdOrientPoint.h"
#include "HdFeaturePoint.h"
#include "HdHsTarget.h"
#include "HdPicture.h"
#include "..\BursaWolfModel.h"

namespace hd
{
	// 前置申明点云对象
	class PointCloud;
	class CSeaPointCloud;

	//! hdScene扫描站数据
	class HDCOMMON_API CHdSceneScan : public CHDObject
	{
	public:
		CHdSceneScan();

		CHdSceneScan(_HD_SCAN_SCAN& hdss);

		CHdSceneScan(CHdSceneScan& hdss);

		virtual ~CHdSceneScan();

	public:
		HD_SCAN_SCAN*			pScan;				//扫描原始数据记录
		vector<CHdOrientPoint*> pOtPoints;			//扫描中添加的定向点对象
		vector<CHdSxPoint3D*>	pPoints;			//扫描中添加的点对象
		vector<CHdLabel*>		pLabels;			//扫描中添加的标签对象
		vector<CHdSxPolyline3D*>	pPolylines;		//扫描中添加的线段对象
		vector<CHdSphere*>		pSpheres;			//扫描中添加的靶球对象
		vector<CHdPlane*>			pPlanes;		//扫描中添加的平面对象
		vector<CHdKeyboard*>    pChessborads;		//扫描中添加的棋盘格标靶对象
		vector<CHdFeaturePoint*>	pFeaturePts;	//特征点对象
		vector<CHdHsTarget*>        pHsTargets;     //HS标靶对象
		CBursaWolfModel			transModel;			//本测站当前的转换模型
		vector<string>			m_ModelStationPath;	//本测站打开或导入的模型文件
		vector<HD_MPIC_PARAM>		m_MCamPics;			// 集成相机拍摄照片参数
		//vector<CHdPicture*>			m_MCamPicObjs;		// 集成相机拍摄照片对象
		CHdPicture*					m_MCamPicObj;
		HD_MCAM_INNPARAM		mCamParam;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_SCENE_SCAN; }
		
		void setTransModel(CBursaWolfModel& model, bool bTrans = false);

		virtual void Serialize(TiXmlElement* element, bool bSave){}
		
		void Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave);

		void SerializeMCam(const char* ipcFile,bool bSave = false);

		// 清空MCam文件
		void ClearMCam();

		// 添加模型文件
		void AddModelFile(const char* StrModefile);

		// 删除模型文件
		void DeleteModelFile(int nIndex);

		// 添加图片对象
		inline void pushbackMCamPic(const char* strPic, bool bSphere = 0, double fHoriAngle = 0.0f, double fVertAngle = 0.0f)
		{
			CHdPicture* newPic = new CHdPicture(strPic);
			newPic->m_bSphere = bSphere;
			newPic->m_fHoriAngle = fHoriAngle;
			newPic->m_fVertAngle = fVertAngle;

			//m_MCamPicObjs.push_back(newPic);
		}
	};

	class CHdPcdObject : public CHDObject
	{
	public:
		PointCloud* m_pPcd;
		CHdPcdObject()
			:m_pPcd(NULL){}
		CHdPcdObject(PointCloud* pcd)
			:m_pPcd(pcd){}
		CHdPcdObject(const CHdPcdObject& other)
			:m_pPcd(other.m_pPcd){}
		~CHdPcdObject(){}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_POINTCLOUD; }
	};

    class CHDTriangleObject : public CHDObject
    {
    public:
        PointCloud* m_pPcd;
        int m_ScanIndex;// 记录点云索引
        CHDTriangleObject()
            : m_pPcd(NULL) { m_ScanIndex = -1; }
        CHDTriangleObject(PointCloud* pcd)
            :m_pPcd(pcd) { m_ScanIndex = -1; }
        CHDTriangleObject(const CHDTriangleObject& other)
            :m_pPcd(other.m_pPcd) { m_ScanIndex = -1; }
        ~CHDTriangleObject() {}

        virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_SKETCHTRIANGLE; }
    };

	class CHdSeaPcdObject : public CHDObject
	{
	public:
		CSeaPointCloud* m_pSeaPcd;
		CHdSeaPcdObject()
			:m_pSeaPcd(NULL){}
		CHdSeaPcdObject(CSeaPointCloud* pcd)
			:m_pSeaPcd(pcd){}
		CHdSeaPcdObject(const CHdSeaPcdObject& other)
			:m_pSeaPcd(other.m_pSeaPcd){}
		~CHdSeaPcdObject(){}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_SEADATA; }
	};

}

