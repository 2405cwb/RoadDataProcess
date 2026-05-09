/*! @file
********************************************************************************
<PRE>
模块名       : ProjectManager
文件名       : hdSceneIscan.h
相关文件     : hdSceneIscan.cpp
文件实现功能 : 定义iScan数据节点包含的子节点信息，包括点信息、测量线、标注、
               GPS控制点等，该类对应的参数信息写入iScan-Route.config
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/02/14   1.0      朱旭波               创建
</PRE>
*******************************************************************************/

#pragma once
#pragma  warning(disable:4251)
#include "..\HDObject.h"
#include "..\hdCommon.h"
#include "HdSxPoint3D.h"
#include "HdSxPolyline3D.h"
#include "HdLabel.h"
#include "HdGPSPoint.h"
#include "HdPicture.h"

using namespace std;
namespace hd
{
	class HDCOMMON_API CHdSceneIscan :
		public CHDObject
	{
	public:
		CHdSceneIscan(void);

		CHdSceneIscan(CHdSceneIscan& hdss);

		virtual ~CHdSceneIscan(void);

	public:
		vector<CHdSxPoint3D*>	   pVecPoints;		// 工程中添加的点对象
		vector<CHdLabel*>		   pVecLabels;		// 工程中添加的标签对象
		vector<CHdSxPolyline3D*>   pVecPolylines;	// 工程中添加的线段对象
		vector<CHdGPSPoint*>       pVecGPSPts;      // 工程中添加的GPS控制点
		vector<CHdPicture*>	       pVecPictures;	// 记录测站中所有照片对象   fengjing
	public:

		// 重载函数
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_SCENE_ISCAN; }

		//virtual void Serialize(TiXmlElement* element, bool bSave){}

		// 设置图片的张数
		void SetPictureSize(int index);

		// 增加图片 fengjing
		void AddPicture(int index,					// 为-1时，表示在数组最后添加一张图片，否则插入有效索引处
						const char* strPicPath);	// 图片路径

		// 减少图片 fengjing
		void DeletePicture(int index);

		// 序列化参数，bSave = true时从内存中保存至文件，为false时读取至内存
		void Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave);
	};

	//// 原配置文件的参数信息，采用结构体方式存放，方便序列化读写
	//typedef struct _HD_SCANROUTE_CONFIG
	//{
	//	string strRouteName;
	//	string strDeviceNo;
	//	int    nScanCount;
	//	int    nCameraCount;
	//	string strVersion;
	//};
}


