/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdAppDTStruct.h
相关文件	: hdCommon.h、hdHdiStruct.h
文件实现功能：定义成果数据库表结构。
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/5/29   1.2			马振明		创建数据库对应的表结构
2014/6/03   1.2			马振明		修改数据库对应的表名称
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "..\hdCore\hdTime.h"
#include "hdHdiStruct.h"

namespace hd
{
	// 影像采集类型
	enum ENUM_IMAGEINFO_TYPE
	{
		E_IMAGE_UNKNOW = 0,		// 未知
		E_IMAGE_ISCAN = 1,		// iScan
		E_IMAGE_PSCAN = 2,		// pScan
		E_IMAGE_ASCAN = 3,		// aScan
		E_IMAGE_LSCAN = 4		// lScan
	};

	//********************************************************************************************
	// 街景1.2版本的数据结构
	//********************************************************************************************
	// 轨迹配置 数据表结构
	struct HD_SV_CONFIG
	{
		// 构造函数，赋初值
		HD_SV_CONFIG()
		{
			memset(strRouteID, 0, OBJECT_ID_LEN);
			nTileLevel = 0;
			nTileWidth = nTileHeight = 0;
			nRIWidth = 3600;
			nRIHeight = 1800;
			memset(strVersion,0,VERSION_LEN);
		}

		char	strRouteID[OBJECT_ID_LEN];	// 轨迹线唯一标识
		int		nTileLevel;					// 全景切片级别
		int		nTileWidth;					// 切片图像Width
		int		nTileHeight;				// 切片图像Height
		int		nRIWidth;					// 深度图Width
		int		nRIHeight;					// 深度图Height
		char	strVersion[VERSION_LEN];	// 版本号
		ENUM_IMAGEINFO_TYPE eImageType;		// 影像采集类型
	};

	//邻接关系数据表结构
	struct HD_SV_LINK
	{
		// 构造函数，赋初值
		HD_SV_LINK()
		{
			memset(strLinkID, 0, OBJECT_ID_LEN_L);
			memset(strSrcImageName, 0, PANO_ID_LEN);
			memset(strDstImageName, 0, PANO_ID_LEN);
			memset(strPostiveName, 0, NAME_LEN);
			memset(strNegativeName, 0, NAME_LEN);
			nID = -1;
		}
		char	strLinkID[OBJECT_ID_LEN_L];		// 邻接关系ID
		char	strSrcImageName[PANO_ID_LEN];	// 当前全景站点名字
		char	strDstImageName[PANO_ID_LEN];	// 目标全景站点名字
		char	strPostiveName[NAME_LEN];		// 正向目的地名字
		char	strNegativeName[NAME_LEN];		// 反向目的地名字
		double	dBL[4];							// 连接关系2个点的经纬度
		int		nID;							// 邻接关系的序列ID
	};

	// 全景索引信息 数据表结构
	struct HD_SV_IMAGEINFO
	{
		// 构造函数，赋初值
		HD_SV_IMAGEINFO()
		{
			memset(strImageID,0,PANO_ID_LEN);
			memset(strImageName,0,PANO_ID_LEN);
			memset(strSegmentID, 0, OBJECT_ID_LEN);
			memset(strRouteID, 0, OBJECT_ID_LEN);
			memset(strMode,0,MODE_LEN);
			memset(strRouteID,0,OBJECT_ID_LEN);
			nSegmentIndex = -1;
			eImageType = E_IMAGE_ISCAN;
			nLinkCount =0;
			nHistoryType=1; //历史轨迹标记,历史轨迹0 最新轨迹1 2014/12/09 lwm
			strcpy_s(strMode,"Day");//默认值为"Day"(避免忘记赋值时将数据库默认值覆盖) lwm 2014/12/18
   dPYaw=0.0f;
   nKey=0;
		}
		
		// 赋值
		HD_SV_IMAGEINFO& operator=(const HD_SV_IMAGEINFO& other)
		{
			// 防止自我赋值 
			if (&other!=this)
			{
				strcpy(strImageID, other.strImageID);
				strcpy(strImageName, other.strImageName);
				strcpy(strSegmentID,other.strSegmentID);
				strcpy(strRouteID,other.strRouteID);
				nSegmentIndex = other.nSegmentIndex;
				nCameraNo = other.nCameraNo;
				tGatherTime = other.tGatherTime;
				dX = other.dX;
				dY = other.dY;
				dZ = other.dZ;
				dB = other.dB;
				dL = other.dL;
				dYaw = other.dYaw;
				dPitch = other.dPitch;
				dRoll = other.dRoll;
				nLinkCount = other.nLinkCount;
				eImageType = other.eImageType;
				strcpy(strMode,other.strMode);
    dPYaw=other.dPYaw;
			}

			return *this;
		}

		// 设置影像信息,参数为hdi信息
		void SetImageInfo(const HD_HDIINFO& hdiInfo)
		{
			strcpy(strImageID, hdiInfo.strImageName);
			nCameraNo = hdiInfo.nCameraNo;
			tGatherTime = hdiInfo.tGatherTime;
			dX = hdiInfo.dX;
			dY = hdiInfo.dY;
			dZ = hdiInfo.dZ;
			dB = hdiInfo.dB;
			dL = hdiInfo.dL;
			dYaw = hdiInfo.dYaw;
			dPitch = hdiInfo.dPitch;
			dRoll = hdiInfo.dRoll;
		}

		char		strImageID[PANO_ID_LEN];		// 影像ID
		char		strImageName[PANO_ID_LEN];		// 影像名称
		char		strSegmentID[OBJECT_ID_LEN];	// 轨迹片段ID，记录全景点属于哪条轨迹片段
		char		strRouteID[OBJECT_ID_LEN];		// 轨迹id
		int			nSegmentIndex;					// 全景点在轨迹片段的索引值
		int			nCameraNo;						// 相机号
		HDTIME		tGatherTime;					// 采集时间
		double		dX;								// x坐标
		double 		dY;								// y坐标
		double 		dZ;								// z坐标
		double 		dB;								// 纬度
		double 		dL;								// 经度
		double		dYaw;							// 相机航向角
		double		dPitch;							// 相机俯仰角
		double		dRoll;							// 相机翻滚角
		int			nLinkCount;						// 邻接关系数量
		ENUM_IMAGEINFO_TYPE eImageType;				// 影像类型
		char		strMode[MODE_LEN];				// 采集模式
		//int			nHistoryID;						// 连接关系ID 去掉该字段 2014/12/12 lwm
		int			nKey;							// 关键类型，默认为0
		int			nHistoryType;					// 是否属于历史轨迹，默认为1(历史轨迹0，最新轨迹1) 2014/12/09 lwm
  double dPYaw;   // pos坐标系Yaw值
	};

	// 全景切片 数据表结构
	struct HD_SV_TILEINFO
	{
		// 构造函数，赋初值
		HD_SV_TILEINFO()
		{
			memset(strTileID, 0, TILE_ID_LEN);
			pTileData = NULL;
			nSize = 0;
			nYear = 2014;
			nMouth =1;
			nDay =1;
		}

		// 析构函数，清空内存
		~HD_SV_TILEINFO()
		{
			if (pTileData)
			{
				delete []pTileData;
				pTileData = NULL;
			}
		}

		// 赋值运算
		HD_SV_TILEINFO& operator=(const HD_SV_TILEINFO& other)
		{
			// 判断是否自我赋值
			if (&other != this)
			{
				strcpy(strTileID, other.strTileID);
				nSize = other.nSize;

				// 释放内存
				if (pTileData)
				{
					delete []pTileData;
					pTileData = NULL;
				}
				pTileData = new BYTE[nSize];
				memcpy(pTileData, other.pTileData, nSize);
				nYear = other.nYear;
				nMouth = other.nMouth;
				nDay = other.nDay;
			}
		
			return *this;
		}

		// 分配内存大小
		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pTileData = new BYTE[nBytes];
				memset(pTileData, 0, nBytes);
				nSize = nBytes;
			}
		}

		char		strTileID[TILE_ID_LEN];	// 切片唯一标识 
		BYTE*		pTileData;				// 切片文件
		long		nSize;					// 文件所占内存大小
		int			nYear;					// 年
		int			nMouth;					// 月
		int			nDay;					// 日
	};

	// 深度图 数据表结构
	struct HD_SV_RANGEIMAGEINFO
	{
		// 构造函数，赋初值
		HD_SV_RANGEIMAGEINFO()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			dMaxDis = dMinDis = 0.0;
			pMapData = NULL;
			nSize = 0;
		}

		// 析构函数，清空内存
		~HD_SV_RANGEIMAGEINFO()
		{
			if (pMapData)
			{
				delete [] pMapData;
				pMapData = NULL;
			}
		}

		// 赋值运算
		HD_SV_RANGEIMAGEINFO& operator=(const HD_SV_RANGEIMAGEINFO& other)
		{			
			// 避免对象的自我赋值
			if (&other != this)
			{
				strcpy(strImageID, other.strImageID);
				dMaxDis = other.dMaxDis;
				dMinDis = other.dMinDis;
				nSize = other.nSize;
				// 释放内存
				if (pMapData)
				{
					delete []pMapData;
					pMapData = NULL;
				}
				pMapData = new BYTE[nSize];
				memcpy(pMapData, other.pMapData, nSize);
			}
			
			return *this;
		}

		// 分配内存大小
		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{				
				pMapData = new BYTE[nBytes];
				memset(pMapData, 0, nBytes);
				nSize = nBytes;
			}
		}

		// 清空内存
		void Clear()
		{
			if (pMapData)
			{
				delete []pMapData;
				pMapData = NULL;
			}
		}

		char	strImageID[PANO_ID_LEN];	// 深度图唯一标识 
		double	dMaxDis;					// 深度图最大距离值
		double	dMinDis;					// 深度图最小距离值
		BYTE*	pMapData;					// 深度图文件
		long	nSize;						// 深度图文件所占内存大小
	};

	// 面片 数据表结构
	struct HD_SV_FACADEINFO
	{
		// 构造函数，赋初值
		HD_SV_FACADEINFO()
		{
			memset(strFacadeID, 0, OBJECT_ID_LEN);
			memset(strRouteID, 0, OBJECT_ID_LEN);
			nType = 1;
			pPoints = NULL;
			nPointSize = 0;
		}

		// 析构函数，清空内存
		~HD_SV_FACADEINFO()
		{
			if (pPoints)
			{
				delete [] pPoints;
				pPoints = NULL;
			}
		}

		// 赋值运算
		HD_SV_FACADEINFO& operator=(const HD_SV_FACADEINFO& other)
		{
			// 判断是否自我赋值
			if (&other != this)
			{
				strcpy(strFacadeID, other.strFacadeID);
				strcpy(strRouteID, other.strRouteID);
				nType = other.nType;

				// 如果指针不为空
				if (pPoints)
				{
					delete []pPoints;
					pPoints = NULL;
				}
				nPointSize = other.nPointSize;
				pPoints = new BYTE[nPointSize];
				memcpy(pPoints, other.pPoints, nPointSize);
			}
			
			return *this;
		}

		// 分配内存大小
		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pPoints = new BYTE[nBytes];
				memset(pPoints, 0, nBytes);
				nPointSize = nBytes;
			}
		}

		// 得到节点个数
		int GetVertexCount() 
		{
			int nVertexCount = -1;
			if (pPoints)
			{
				memcpy(&nVertexCount, pPoints, sizeof(int));
			}
			
			return nVertexCount;
		}

		char	strFacadeID[OBJECT_ID_LEN];		// 面片唯一标识
		char	strRouteID[OBJECT_ID_LEN];		// 轨迹ID
		int		nType;							// 面片类型 1 面片 2 多面片
		BYTE*	pPoints;						// 面片坐标集合
		long	nPointSize;						// 面片内存大小
		double	dCenterX;						// 面片中心点X坐标
		double	dCenterY;						// 面片中心点Y坐标
		double	dCenterZ;						// 面片中心点Z坐标
		double	dNormalX;						// 法向量X
		double	dNormalY;						// 法向量Y
		double	dNormalZ;						// 法向量Z
	};

	// 面片测站关系 数据表结构
	struct HD_SV_IMAGEFACADE
	{
		// 构造函数，赋初值
		HD_SV_IMAGEFACADE()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			memset(strFacadeID, 0, OBJECT_ID_LEN);
		}		

		// 赋值
		HD_SV_IMAGEFACADE& operator=(const HD_SV_IMAGEFACADE& other)
		{
			// 防止自我赋值 
			if (&other!=this)
			{
				strcpy(strImageID, other.strImageID);
				strcpy(strFacadeID, other.strFacadeID);
			
			}

			return *this;
		}

		char	strImageID[PANO_ID_LEN];	// 全景唯一标识
		char	strFacadeID[OBJECT_ID_LEN];	// 面片唯一标识
	};

	// 轨迹线 数据表结构
	struct HD_SV_ROUTE
	{
		// 构造函数，赋初值
		HD_SV_ROUTE()
		{
			memset(strRouteID, 0, OBJECT_ID_LEN);
			memset(strRouteName, 0, NAME_LEN);
			memset(strDeviceNo, 0, DEVICE_NO_LEN);
			nPtNum = 0;
			pShapeXY = NULL;
			pShapeBL = NULL;
			nSize = 0;
			memset(strCarNum,0,DEVICE_NO_LEN);
   memset(strDataPath,0,DATAPATH_LEN);
		}

		// 析构函数，清空内存
		~HD_SV_ROUTE()
		{
			if (pShapeXY)
			{
				delete [] pShapeXY;
				pShapeXY = NULL;
			}

			if (pShapeBL)
			{
				delete [] pShapeBL;
				pShapeBL = NULL;
			}
		}

		// 赋值运算
		HD_SV_ROUTE& operator=(const HD_SV_ROUTE& other)
		{
			// 判断是否自我赋值
			if (&other != this)
			{
				strcpy(strRouteID, other.strRouteID);
				strcpy(strRouteName, other.strRouteName);
				strcpy(strDeviceNo, other.strDeviceNo);
    strcpy_s(strCarNum,other.strCarNum);
    strcpy_s(strDataPath,other.strDataPath);
				nPtNum = other.nPtNum;
				nSize = other.nSize;
				if (pShapeBL)
				{
					delete []pShapeBL;
					pShapeBL =NULL;
				}
				if (pShapeXY)
				{
					delete []pShapeXY;
					pShapeXY = NULL;
				}
				pShapeXY = new BYTE[nSize];
				pShapeBL = new BYTE[nSize];			
				memcpy(pShapeXY, other.pShapeXY, nSize);
				memcpy(pShapeBL, other.pShapeBL, nSize);
			}
			
			return *this;
		}

		// 分配内存大小
		void AllocSize()
		{
			if (nPtNum > 0)
			{
				nSize = nPtNum*sizeof(double)*2;
				pShapeXY = new BYTE[nSize];
				pShapeBL = new BYTE[nSize];
				memset(pShapeXY, 0, nSize);
				memset(pShapeBL, 0, nSize);
			}
		}		

		char	strRouteID[OBJECT_ID_LEN];	// 轨迹标识符
		char	strRouteName[NAME_LEN];		// 轨迹名称
		char	strDeviceNo[DEVICE_NO_LEN];	// 设备编号
		HDTIME	tStartTime;					// 采集起始时间
		HDTIME	tEndTime;					// 采集结束时间
		int		nPtNum;						// 轨迹全景数量
		long	nSize;						// pShape所占内存大小
		BYTE*	pShapeXY;					// XY坐标
		BYTE*	pShapeBL;					// 经纬度
		char	strCarNum[DEVICE_NO_LEN];	// 车牌号
  char strDataPath[DATAPATH_LEN]; // 原始数据存储路径 [2015/06/18 luowenmin]
	};

	// 轨迹片段 数据表结构
	struct HD_SV_SEGMENT
	{
		// 构造函数，赋初值
		HD_SV_SEGMENT()
		{
			memset(strSegmentID, 0, OBJECT_ID_LEN);
			memset(strRouteID, 0, OBJECT_ID_LEN);
			memset(strDeviceNo, 0, DEVICE_NO_LEN);
			memset(strInfo, 0, NAME_LEN);
			nStartNodeIndex = 0;
			nEndNodeIndex = 0;
			nPtNum = 0;
			pShapeXY = NULL;
			pShapeBL = NULL;
			bIsValid = true;
			nHistoryType=1; //历史轨迹标记,（历史轨迹0 最新轨迹1）默认值为1 2014/12/18 2014/12/09 lwm
			memset(strMode,0,MODE_LEN);
			strcpy_s(strMode,"Day");//默认值为"Day"(避免忘记赋值时将数据库默认值覆盖) lwm 2014/12/18
		}

		// 析构函数，清空内存
		~HD_SV_SEGMENT()
		{
			if (pShapeXY)
			{
				delete [] pShapeXY;
				pShapeXY = NULL;

			}
			if (pShapeBL)
			{
				delete [] pShapeBL;
				pShapeBL = NULL;

			}
		}

		// 赋值运算
		HD_SV_SEGMENT& operator=(const HD_SV_SEGMENT& other)
		{
			if (&other != this)
			{
				strcpy(strSegmentID, other.strSegmentID);
				strcpy(strRouteID, other.strRouteID);
				strcpy(strDeviceNo, other.strDeviceNo);
				strcpy(strInfo, other.strInfo);
				tStartTime = other.tStartTime;
				tEndTime = other.tEndTime;
				nStartNodeIndex = other.nStartNodeIndex;
				nEndNodeIndex = other.nEndNodeIndex;
				nPtNum = other.nPtNum;
				nSize = other.nSize;
				if (pShapeBL)
				{
					delete []pShapeBL;
					pShapeBL =NULL;
				}
				if (pShapeXY)
				{
					delete []pShapeXY;
					pShapeXY = NULL;
				}
				pShapeXY = new BYTE[nSize];
				pShapeBL = new BYTE[nSize];			
				memcpy(pShapeXY, other.pShapeXY, nSize);
				memcpy(pShapeBL, other.pShapeBL, nSize);
				bIsValid = other.bIsValid;
				strcpy(strMode,other.strMode);//采集模式 2014/12/09 lwm
			}
			
			return *this;
		}

		// 分配内存大小
		void AllocSize()
		{
			if (nPtNum > 0)
			{
				nSize = nPtNum*sizeof(double)*2;
				pShapeXY = new BYTE[nSize];
				pShapeBL = new BYTE[nSize];
				memset(pShapeXY, 0, nSize);
				memset(pShapeBL, 0, nSize);
			}
		}		

		char	strSegmentID[OBJECT_ID_LEN];	// 片段标识符		
		char	strRouteID[OBJECT_ID_LEN];		// 轨迹标识符
		char	strDeviceNo[DEVICE_NO_LEN];		// 设备编号
		char	strInfo[NAME_LEN];				// 片段名字
		HDTIME	tStartTime;						// 采集起始时间
		HDTIME	tEndTime;						// 采集结束时间
		int		nStartNodeIndex;				// 片段起始点索引
		int		nEndNodeIndex;					// 片段结束点索引
		int		nPtNum;							// 轨迹全景数量
		BYTE*	pShapeXY;						// XY坐标 
		BYTE*	pShapeBL;						// 经纬度 
		long	nSize;							// pShape所占内存大小
		bool	bIsValid;						// 片段是否有效

		int		nHistoryType;				    // 是否属于历史轨迹，默认为1(历史轨迹0，最新轨迹1) 2014/12/09 lwm
		char    strMode[MODE_LEN];				// 采集模式   白天 "Day ",晚上"Night"   

	};


	// 采集车车辆信息 数据表结构 2014/12/05 lwm
	struct HD_SV_CARINFO
	{ 
		HD_SV_CARINFO()
		  :fHeight(2.65f){
			memset(strCarNum, 0, CARNUM_LEN);

		}
		~HD_SV_CARINFO(){

		}

		//赋值运算
		const HD_SV_CARINFO& operator=(const HD_SV_CARINFO& other)
		{
			if (&other!=this)
			{
				strcpy(strCarNum, other.strCarNum);
				fHeight=other.fHeight;
			}

			return *this;
		}

		char strCarNum[CARNUM_LEN];             //车牌号
		float fHeight;                          //车高
	};

	// 最新轨迹历史轨迹站点映射表 2014/12/12 lwm
	struct HD_SV_HISTORYLINK
	{
		HD_SV_HISTORYLINK()
		{
			memset(strImageID,0,PANO_ID_LEN);
			memset(strHistoryLink,0,SQL_QUERY_LEN);
		}

		~HD_SV_HISTORYLINK()
		{

		}

		const HD_SV_HISTORYLINK& operator=(const HD_SV_HISTORYLINK& other)
		{
			if (&other!=this)
			{
				strcpy(strImageID, other.strImageID);
				strcpy(strHistoryLink,other.strHistoryLink);
				//tGatherTime = other.tGatherTime;
			}

			return *this;
		}

		char strImageID[PANO_ID_LEN];		    // 影像ID
		char strHistoryLink[SQL_QUERY_LEN];     // 站点关联的历史轨迹站点

		//HDTIME		tGatherTime;					// 采集时间

	};

 // 轨迹id于轨迹原始采集数据路径映射 [2015/06/17 luowenmin]
 struct HD_SV_ROUTEDATAPATHINFO
 {
     HD_SV_ROUTEDATAPATHINFO()
     {
         memset(strRouteID,0,OBJECT_ID_LEN);
         memset(strRouteDataPath,0,DATAPATH_LEN);
     }

     // 重载小于运算符
     bool operator<(const HD_SV_ROUTEDATAPATHINFO& other) const
     {
         return (strcmp(strRouteID,other.strRouteID)<0);
     }

     // 重载相等操作符
     bool operator==(const HD_SV_ROUTEDATAPATHINFO& other) const
     {
         return (strcmp(strRouteID,other.strRouteID)==0);
     }

     // 重载大于操作符 
     bool operator>(const HD_SV_ROUTEDATAPATHINFO& other) const
     {
         return (strcmp(strRouteID,other.strRouteID)>0);
     }


    		char	strRouteID[OBJECT_ID_LEN];		// 轨迹标识符
      char strRouteDataPath[DATAPATH_LEN]; // 数据存放路径
 };
 //


 // 站点公里桩信息 [2015/09/12 luowenmin]
 struct HD_SV_KMPILEINFO 
 {
     HD_SV_KMPILEINFO()
    :nMile(0)
    ,nMeter(0)
    ,nForward(0)
    {
       memset(strImageID, 0, PANO_ID_LEN);  
    }
     ~HD_SV_KMPILEINFO()
     {

     }
     char	strImageID[PANO_ID_LEN];		// 影像ID
     int nMile;
     int nMeter;
     int nForward;
 };

 //
}