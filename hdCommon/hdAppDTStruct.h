/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdAppDTStruct.h
相关文件	: hdCommon.h、hdHdiStruct.h
文件实现功能：定义成果数据库表结构。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/2/27	1.0			杨峰		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdHdiStruct.h"
#include "..\hdCore\hdTime.h"

namespace hd
{
	// 标注符号类型 枚举
	enum ENUM_SYMBOL_TYPE
	{
		E_ST_PIC		= 1,		// 图片类型
		E_ST_FLASH		= 2,		// 动画类型
		E_ST_MODEL		= 3,		// 三维模型类型
		E_ST_FACADE		= 4,		// 面片类型
		E_ST_MUTIFACADE = 5,        // 多面片
		E_ST_VIDEO		= 6,		// 视频类型
		E_ST_TEXT		= 7,	    // 文字类型	
		E_ST_POINT      = 8,        // 点标注
		E_ST_LINE       = 9,		// 线标注
		E_ST_PLANE      = 10,		// 平面标注
		E_ST_MQWB       = 11,		// 门前五包
		E_ST_MUSIC		= 12,		// 音频标注
		E_ST_BILLBOARD  = 15		// 广告牌
	};

	// 标注符号文件类型 枚举
	enum ENUM_SYMBOL_FILE_TYPE
	{
		E_SFT_JPG		= 0,		// jpg类型
		E_SFT_BMP		= 1,		// bmp类型
		E_SFT_PNG		= 2,		// png类型
		E_SFT_TIF		= 3,		// tif类型
		E_SFT_GIF		= 4,		// gif类型
		E_SFT_SWF		= 5,		// swf类型
		E_SFT_A3D		= 6,		// a3d类型
		E_SFT_FLV		= 7,		// FLV类型
		E_SFT_NULL		= 99		// 没有文件
	};

	// 标注符号的类型
	enum ENUM_MARKER_TYPE
	{
		E_MARKER_JIAO=1, // 吉奥的符号
		E_MARKER_SV=2,	// 街景的符号
		E_MARKER_IV=3  //室内的符号
	};
	// 轨迹配置 数据表结构
	struct HD_STREETVIEW_CONFIG
	{
		// 构造函数，赋初值
		HD_STREETVIEW_CONFIG()
		{
			memset(strRouteID, 0, OBJECT_ID_LEN);
			nTileLevel = 0;
			nTileWidth = nTileHeight = 0;
			nRIWidth = 3600;
			nRIHeight = 1800;
		}

		char	strRouteID[OBJECT_ID_LEN];	// 轨迹线唯一标识
		int		nTileLevel;					// 全景切片级别
		int		nTileWidth;					// 切片图像Width
		int		nTileHeight;				// 切片图像Height
		int		nRIWidth;					// 深度图Width
		int		nRIHeight;					// 深度图Height
	};

	// 工程数据表结构
	struct HD_STREETVIEW_PROJECT
	{
		// 构造函数，赋初值
		HD_STREETVIEW_PROJECT()
		{
			memset(strProjectID, 0, OBJECT_ID_LEN);
			memset(strProjectName, 0, NAME_LEN);
			nTileLevel = 0;
			nTileWidth = nTileHeight = 0;
			nRIWidth = 3600;
			nRIHeight = 1800;
			dAngle = 0;
			memset(strCompany, 0, NAME_LEN);
			memset(strProcessorName, 0, OBJECT_ID_LEN);
		}

		char	strProjectID[OBJECT_ID_LEN];	// 工程唯一标识
		char	strProjectName[NAME_LEN];		// 工程名字
		int		nTileLevel;					// 全景切片级别
		int		nTileWidth;					// 切片图像Width
		int		nTileHeight;				// 切片图像Height
		int		nRIWidth;					// 深度图Width
		int		nRIHeight;					// 深度图Height
		char	strCompany[NAME_LEN];				// 公司名字
		char	strProcessorName[OBJECT_ID_LEN];	// 数据处理人员名字
		HDTIME	tGetherTime;			// 数据采集时间
		HDTIME	tProcesseTime;		// 数据处理时间
		double	dAngle;				// 与正北方向夹角

	};

	//邻接关系数据表结构
	struct HD_STREETVIEW_LINK
	{
		// 构造函数，赋初值
		HD_STREETVIEW_LINK()
		{
			memset(strLinkID, 0, OBJECT_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);
			memset(strRouteID, 0, PANO_ID_LEN);
			memset(strSrcImageName, 0, PANO_ID_LEN);
			memset(strDstImageName, 0, PANO_ID_LEN);
			memset(strDstName, 0, NAME_LEN);
			nDirection = 1;
			dsX = 0.0;
			dsY = 0.0;
			deX = 0.0;
			deY = 0.0;
		}
		char	strLinkID[OBJECT_ID_LEN]; // 邻接关系ID
		char	strProjectID[OBJECT_ID_LEN]; // 工程ID
		char	strRouteID[PANO_ID_LEN];	// 轨迹线唯一标识
		char	strSrcImageName[PANO_ID_LEN]; // 当前全景站点名字
		char	strDstImageName[PANO_ID_LEN]; // 目标全景站点名字
		char	strDstName[NAME_LEN]; // 		目的地名字
		int		nDirection;					// 方向，1为正向，0为反向。
		double	dsX;						// 开始节点的x
		double	dsY;						// 开始节点的y
		double	deX;						// 结束节点的y
		double	deY;						// 结束节点的y

	};

	// 全景索引信息 数据表结构
	struct HD_STREETVIEW_IMAGEINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_IMAGEINFO()
		{
			memset(strSegmentID, 0, OBJECT_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);
			nSegmentIndex = -1;
		}

		char		strSegmentID[OBJECT_ID_LEN];	// 轨迹片段ID，记录全景点属于哪条轨迹片段
		char		strProjectID[OBJECT_ID_LEN];	// 工程id
		int			nSegmentIndex;					// 全景点在轨迹片段的索引值
		HD_HDIINFO	hdiInfo;						// hdi对象
	};

	// 全景切片 数据表结构
	struct HD_STREETVIEW_TILEINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_TILEINFO()
		{
			memset(strTileID, 0, TILE_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);

			pTileData = NULL;
			nSize = 0;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_TILEINFO()
		{
			if (pTileData)
			{
				delete []pTileData;
				pTileData = NULL;
			}
		}

		// 赋值运算
		HD_STREETVIEW_TILEINFO& operator=(const HD_STREETVIEW_TILEINFO& other)
		{
			strcpy(strTileID, other.strTileID);
			strcpy(strProjectID, other.strProjectID);
			nSize = other.nSize;
			pTileData = new BYTE[nSize];
			memcpy(pTileData, other.pTileData, nSize);

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
		char		strProjectID[OBJECT_ID_LEN];	// 工程唯一标识
		BYTE*		pTileData;				// 切片文件
		long		nSize;					// 文件所占内存大小
	};

	// 深度图 数据表结构
	struct HD_STREETVIEW_RANGEIMAGEINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_RANGEIMAGEINFO()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);
			dMaxDis = dMinDis = 0.0;
			pMapData = NULL;
			nSize = 0;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_RANGEIMAGEINFO()
		{
			if (pMapData)
			{
				delete [] pMapData;
				pMapData = NULL;
			}
		}

		// 赋值运算
		HD_STREETVIEW_RANGEIMAGEINFO& operator=(const HD_STREETVIEW_RANGEIMAGEINFO& other)
		{			
			strcpy(strImageID, other.strImageID);
			strcpy(strProjectID, other.strProjectID);
			dMaxDis = other.dMaxDis;
			dMinDis = other.dMinDis;
			nSize = other.nSize;
			pMapData = new BYTE[nSize];
			memcpy(pMapData, other.pMapData, nSize);

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
		char	strProjectID[OBJECT_ID_LEN];	// 工程唯一标识
		double	dMaxDis;					// 深度图最大距离值
		double	dMinDis;					// 深度图最小距离值
		BYTE*	pMapData;					// 深度图文件
		long	nSize;						// 深度图文件所占内存大小
	};

	// 面片 数据表结构
	struct HD_STREETVIEW_FACADEINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_FACADEINFO()
		{
			memset(strFacadeID, 0, OBJECT_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);
			nType = 1;
			pPoints = NULL;
			nPointSize = 0;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_FACADEINFO()
		{
			if (pPoints)
			{
				delete [] pPoints;
				pPoints = NULL;
			}
		}

		// 赋值运算
		HD_STREETVIEW_FACADEINFO& operator=(const HD_STREETVIEW_FACADEINFO& other)
		{
			strcpy(strFacadeID, other.strFacadeID);
			strcpy(strProjectID, other.strProjectID);
			nType = other.nType;
			nPointSize = other.nPointSize;
			pPoints = new BYTE[nPointSize];
			memcpy(pPoints, other.pPoints, nPointSize);

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

		char	strFacadeID[OBJECT_ID_LEN];	// 面片唯一标识
		char	strProjectID[OBJECT_ID_LEN];	// 工程唯一标识
		int		nType;						// 面片类型 1 面片 2 多面片
		BYTE*	pPoints;					// 面片坐标集合
		long	nPointSize;					// 面片内存大小
		double	dCenterX;					// 面片中心点X坐标
		double	dCenterY;					// 面片中心点Y坐标
		double	dNormalX;					// 法向量X
		double	dNormalY;					// 法向量Y
		double	dNormalZ;					// 法向量Z
	};

	// 面片测站关系 数据表结构
	struct HD_STREETVIEW_IMAGEFACADE
	{
		// 构造函数，赋初值
		HD_STREETVIEW_IMAGEFACADE()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			memset(strFacadeID, 0, OBJECT_ID_LEN);
		}		

		char	strImageID[PANO_ID_LEN];	// 全景唯一标识
		char	strFacadeID[OBJECT_ID_LEN];	// 面片唯一标识
	};

	// 标注 数据表结构
	struct HD_STREETVIEW_MARKERINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_MARKERINFO()
		{
			memset(strMarkerID, 0, OBJECT_ID_LEN);
			memset(strProjectID, 0, OBJECT_ID_LEN);
			memset(strName, 0, NAME_LEN);
			memset(strSymbolID, 0, OBJECT_ID_LEN);
			memset(strSouceID,0,OBJECT_ID_LEN);
			dCenterX = dCenterY = dCenterZ = 0.0;
			dNormalX = dNormalY = dNormalZ = 1.0;
			dScaleX = dScaleY = dScaleZ = 1.0;
			bEdited=false;
			pPoints=NULL;
			nSize=0;
			eSymbolType = E_ST_POINT; // 默认是点标注
		}		

		~HD_STREETVIEW_MARKERINFO()
		{
			if (pPoints)
			{
				delete pPoints;
				pPoints=NULL;
			}
		}

		// 赋值运算
		HD_STREETVIEW_MARKERINFO& operator=(const HD_STREETVIEW_MARKERINFO& other)
		{
			strcpy(strMarkerID, other.strMarkerID);
			strcpy(strProjectID, other.strProjectID);
			strcpy(strName, other.strName);
			strcpy(strSymbolID, other.strSymbolID);
			strcpy(strSouceID, other.strSouceID);
			dCenterX = other.dCenterX;
			dCenterY = other.dCenterY;
			dCenterZ = other.dCenterZ;
			dNormalX = other.dNormalX;
			dNormalY = other.dNormalY;
			dNormalZ = other.dNormalZ;
			dScaleX = other.dScaleX;
			dScaleY = other.dScaleY;
			dScaleZ = other.dScaleZ;
			bEdited = other.bEdited;
			eSymbolType = other.eSymbolType;
			nSize = other.nSize;			
			pPoints = new BYTE[nSize];
			memcpy(pPoints, other.pPoints, nSize);

			return *this;
		}

		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pPoints = new BYTE[nBytes];
				memset(pPoints, 0, nBytes);
				nSize=nBytes;
			}
		}		

		char	strMarkerID[OBJECT_ID_LEN];	// 全景标注唯一标识
		char	strProjectID[OBJECT_ID_LEN];// 工程唯一标识
		char	strName[NAME_LEN];			// 全景标注名称
		char	strSymbolID[OBJECT_ID_LEN];	// 标注符号唯一标识
		double	dCenterX;					// 中心点X坐标
		double	dCenterY;					// 中心点Y标
		double	dCenterZ;					// 中心点Z标
		double	dNormalX;					// 法向量X
		double	dNormalY;					// 法向量Y
		double	dNormalZ;					// 法向量Z
		double	dScaleX;					// X方向缩放系数
		double	dScaleY;					// Y方向缩放系数
		double	dScaleZ;					// Z方向缩放系数
		char    strSouceID[OBJECT_ID_LEN];  // 标注ID [add by mzm 2013.10.21]
		bool    bEdited;                    // 是否被编辑过
		BYTE*   pPoints;					// 点串
		long    nSize;                      // 点串大小
		int		eSymbolType;				// 符号类型，不能使用枚举类型变量，和数据库读写之间有问题。
	};

	// 标注测站关系 数据表结构
	struct HD_STREETVIEW_IMAGEMARKER
	{
		// 构造函数，赋初值
		HD_STREETVIEW_IMAGEMARKER()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			memset(strMarkerID, 0, OBJECT_ID_LEN);
		}		

		char	strImageID[PANO_ID_LEN];	// 全景唯一标识
		char	strMarkerID[OBJECT_ID_LEN];	// 标注唯一标识
	};

	// 标注符号 数据表结构
	struct HD_STREETVIEW_SYMBOLINFO
	{
		// 构造函数，赋初值
		HD_STREETVIEW_SYMBOLINFO()
		{
			memset(strSymbolID, 0, OBJECT_ID_LEN);
			memset(strName, 0, NAME_LEN);
			eSymbolType = E_ST_PIC;
			eFileType = E_SFT_JPG;
			pFileData = NULL;
			nSize = 0;
		}

		// 赋值运算
		HD_STREETVIEW_SYMBOLINFO& operator=(const HD_STREETVIEW_SYMBOLINFO& other)
		{
			strcpy(strSymbolID, other.strSymbolID);
			strcpy(strName, other.strName);
			eSymbolType = other.eSymbolType;
			eFileType = other.eFileType;			
			nSize = other.nSize;
			pFileData = new BYTE[nSize];
			memcpy(pFileData, other.pFileData, nSize);

			return *this;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_SYMBOLINFO()
		{
			if (pFileData)
			{
				delete [] pFileData;
				pFileData = NULL;
			}
		}

		// 分配内存大小
		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pFileData = new BYTE[nBytes];
				memset(pFileData, 0, nBytes);
				nSize = nBytes;
			}
		}		

		char					strSymbolID[OBJECT_ID_LEN];	// 符号唯一标识
		char					strName[NAME_LEN];			// 符号名称
		ENUM_SYMBOL_TYPE		eSymbolType;				// 符号类型
		ENUM_SYMBOL_FILE_TYPE	eFileType;					// 文件类型
		BYTE*					pFileData;					// 符号文件
		long					nSize;						// 文件所占内存大小
	};

	// 标注属性 数据表结构
	struct HD_STREETVIEW_MARKERATT
	{
		// 构造函数，赋初值
		HD_STREETVIEW_MARKERATT()
		{
			memset(strMarkerID, 0, OBJECT_ID_LEN);
			memset(strPyName, 0, NAME_LEN);
			memset(strAddress, 0, ADDRESS_LEN);
			memset(strPhoneNum, 0, PHONENUM_LEN);
		}		

		char	strMarkerID[OBJECT_ID_LEN];	// 全景标注唯一标识
		char	strPyName[NAME_LEN];		// 拼音名称
		char	strAddress[ADDRESS_LEN];	// 地址
		char	strPhoneNum[PHONENUM_LEN];	// 电话号码
	};

	// 路名路址 数据表结构
	struct HD_STREETVIEW_ROADADDRESS
	{
		// 构造函数，赋初值
		HD_STREETVIEW_ROADADDRESS()
		{
			memset(strID, 0, OBJECT_ID_LEN);
			memset(strName, 0, NAME_LEN);
			memset(strArea, 0, NAME_LEN);
			nPointCount = 0;
			pShape = NULL;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_ROADADDRESS()
		{
			if (pShape)
			{
				delete [] pShape;
				pShape = NULL;
			}
		}

		// 赋值运算
		HD_STREETVIEW_ROADADDRESS& operator=(const HD_STREETVIEW_ROADADDRESS& other)
		{
			strcpy(strID, other.strID);
			strcpy(strName, other.strName);
			strcpy(strArea, other.strArea);
			nPointCount = other.nPointCount;
			pShape = new BYTE[nPointCount * 2 * sizeof(double)];
			memcpy(pShape, other.pShape, nPointCount * 2 * sizeof(double));

			return *this;
		}

		// 分配内存大小
		void AllocSize()
		{
			if (nPointCount > 0)
			{
				pShape = new BYTE[nPointCount * 2 * sizeof(double)];
				memset(pShape, 0, nPointCount * 2 * sizeof(double));
			}
		}		

		char	strID[OBJECT_ID_LEN];		// 路名路址唯一标识
		char	strName[NAME_LEN];			// 路名名称
		char	strArea[NAME_LEN];			// 路名区域
		int		nPointCount;				// 点数
		BYTE*	pShape;						// 经纬度序列
	};

	// 轨迹线 数据表结构
	struct HD_STREETVIEW_ROUTE
	{
		// 构造函数，赋初值
		HD_STREETVIEW_ROUTE()
		{
			memset(strRouteID, 0, OBJECT_ID_LEN);
			memset(strRouteName, 0, NAME_LEN);
			memset(strDeviceNo, 0, DEVICE_NO_LEN);
			nPtNum = 0;
			pShapeXY = NULL;
			pShapeBL = NULL;
			nSize = 0;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_ROUTE()
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
		HD_STREETVIEW_ROUTE& operator=(const HD_STREETVIEW_ROUTE& other)
		{
			strcpy(strRouteID, other.strRouteID);
			strcpy(strRouteName, other.strRouteName);
			strcpy(strDeviceNo, other.strDeviceNo);
			nPtNum = other.nPtNum;
			nSize = other.nSize;
			pShapeXY = new BYTE[nSize];
			pShapeBL = new BYTE[nSize];			
			memcpy(pShapeXY, other.pShapeXY, nSize);
			memcpy(pShapeBL, other.pShapeBL, nSize);

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
	};

	// Node 数据表结构
	struct HD_STREETVIEW_NODE
	{
		// 构造函数，赋初值
		HD_STREETVIEW_NODE()
		{
			memset(strNodeID, 0, OBJECT_ID_LEN);
			dX = dY = dB = dL = 0.0f;
		}

		HD_STREETVIEW_NODE(const char* _strID, double _dX, double _dY, double _dB, double _dL)
			:dX(_dX), dY(_dY), dB(_dB), dL(_dL)
		{
			memcpy(strNodeID, _strID, OBJECT_ID_LEN);
		}

		char	strNodeID[OBJECT_ID_LEN];	// NODE标识符
		double	dX;							// 点坐标X
		double	dY;							// 点坐标Y
		double	dB;							// 经度
		double	dL;							// 维度
	};

	// 轨迹片段 数据表结构
	struct HD_STREETVIEW_SEGMENT
	{
		// 构造函数，赋初值
		HD_STREETVIEW_SEGMENT()
		{
			memset(strSegmentID, 0, OBJECT_ID_LEN);
			memset(strRouteID, 0, OBJECT_ID_LEN);
			memset(strDeviceNo, 0, DEVICE_NO_LEN);
			memset(strStartNodeID, 0, OBJECT_ID_LEN);
			memset(strEndNodeID, 0, OBJECT_ID_LEN);
			memset(strRoadName, 0, NAME_LEN);
			nPtNum = 0;
			pShapeXY = NULL;
			pShapeBL = NULL;
		}

		// 析构函数，清空内存
		~HD_STREETVIEW_SEGMENT()
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
		HD_STREETVIEW_SEGMENT& operator=(const HD_STREETVIEW_SEGMENT& other)
		{
			strcpy(strSegmentID, other.strSegmentID);
			strcpy(strRouteID, other.strRouteID);
			strcpy(strDeviceNo, other.strDeviceNo);
			strcpy(strStartNodeID, other.strStartNodeID);
			strcpy(strEndNodeID, other.strEndNodeID);
			strcpy(strRoadName, other.strRoadName);
			tStartTime = other.tStartTime;
			tEndTime = other.tEndTime;
			nPtNum = other.nPtNum;
			nSize = other.nSize;
			pShapeXY = new BYTE[nSize];
			pShapeBL = new BYTE[nSize];			
			memcpy(pShapeXY, other.pShapeXY, nSize);
			memcpy(pShapeBL, other.pShapeBL, nSize);

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
		char	strRoadName[NAME_LEN];			// 片段名字
		HDTIME	tStartTime;						// 采集起始时间
		HDTIME	tEndTime;						// 采集结束时间
		char	strStartNodeID[OBJECT_ID_LEN];	// 片段标识符
		char	strEndNodeID[OBJECT_ID_LEN];	// 片段标识符
		int		nPtNum;							// 轨迹全景数量
		BYTE*	pShapeXY;						// XY坐标 
		BYTE*	pShapeBL;						// 经纬度 
		long	nSize;							// pShape所占内存大小
	};

	// 部件 数据表结构 武大吉奥专用
	struct HD_STREETVIEW_BJATTRINFO
	{
		//构造函数
		HD_STREETVIEW_BJATTRINFO()
		{
			memset(strMarkID,0,OBJECT_ID_LEN);
			memset(strPCode,0,BJ_ID_LEN);
			memset(strPName,0,BJ_NAME_LEN);
			memset(strPSID,0,BJ_CLASSID_LEN);
			memset(strPBName,0,BJ_NAME_LEN);
			memset(strPBID,0,BJ_CLASSID_LEN);
			memset(strDepName,0,BJ_NAME_LEN);
			memset(strDepID,0,BJ_NAME_LEN);
			memset(strGridID,0,BJ_NAME_LEN);
			memset(strState,0,BJ_STATE_LEN);
			memset(strIsValid,0,BJ_STATE_LEN);
			memset(strReviewState,0,BJ_STATE_LEN);
			memset(strDataSource,0,BJ_STATE_LEN);
			memset(strRemark,0,BJ_NOTE_LEN);
			memset(strPartPos,0,BJ_POS_LEN);
			memset(strActionType,0,BJ_STATE_LEN);
			memset(strIconType,0,BJ_STATE_LEN);
			memset(strIcon,0,BJ_NOTE_LEN);
			memset(strMaterial,0,BJ_NAME_LEN);
			memset(strRoadName,0,BJ_NAME_LEN);
			memset(strStarName,0,BJ_NAME_LEN);
			memset(strEndName,0,BJ_NAME_LEN);
			memset(strLocation,0,BJ_NAME_LEN);
			memset(strWhatShape,0,BJ_NAME_LEN);
			iPatrNum=0;
			memset(strPartSize,0,BJ_NAME_LEN);
			memset(strMainTain,0,BJ_NAME_LEN);
			iMarkFlag=0;
			memset(strPartPhoto,0,BJ_NOTE_LEN);
			iHavePhtoto=0;
			memset(strSqName,0,BJ_NAME_LEN);
		}

		char strMarkID[OBJECT_ID_LEN];           // 标注ID
		char strPCode[BJ_ID_LEN];				 // 部件ID
		char strPName[BJ_NAME_LEN];              // 部件小类名称
		char strPSID[BJ_CLASSID_LEN];            // 小类ID
		char strPBName[BJ_NAME_LEN];			 // 大类名称
		char strPBID[BJ_CLASSID_LEN];            // 大类ID
		char strDepName[BJ_NAME_LEN];			 // 部件归属部门
		char strDepID[BJ_NAME_LEN];              // 部件归属部门ID
		char strGridID[BJ_ID_LEN];               // 部件所在网格ID
		char strState[BJ_STATE_LEN];             // 部件情况
		char strIsValid[BJ_STATE_LEN];           // 部件状态
		char strReviewState[BJ_STATE_LEN];		 // 审核状态
		char strDataSource[BJ_STATE_LEN];		 // 更新来源
		HDTIME tCreateDate;						 // 创建日期
		char strRemark[BJ_NOTE_LEN];			 // 备注
		char strPartPos[BJ_POS_LEN];             // 位置描述
		char strActionType[BJ_STATE_LEN];		 // 变更类型
		HDTIME tBGRQ;							 // 变更日期
		char strIconType[BJ_STATE_LEN];			 // 附件类型
		char strIcon[BJ_NOTE_LEN];				 // 附件内容
		char strMaterial[BJ_NAME_LEN];			 // 材质
		char strRoadName[BJ_NAME_LEN];			 // 路名
		char strStarName[BJ_NAME_LEN];		     // 街道开始名字
		char strEndName[BJ_NAME_LEN];			 // 街道结束名字
		char strLocation[BJ_NAME_LEN];			 // 位置
		char strWhatShape[BJ_NAME_LEN];			 // 形状
		int  iPatrNum;							 // 数量
		char strPartSize[BJ_NAME_LEN];			 // 尺寸
		char strMainTain[BJ_NAME_LEN];			 // 维护单位
		int  iMarkFlag;							 // 标识位置
		HDTIME tCensus;							 // 普查时间
		char strPartPhoto[BJ_NOTE_LEN];			 // 部件照片
		int iHavePhtoto;						 // 是否有照片
		HDTIME tUpdateTime;						 // 更新照片
		char strSqName[BJ_NAME_LEN];			 // 社区编号
		int iBJtype;							 // 部件类型 
	};

	// 五包 数据表结构 武大吉奥专用
	struct HD_STREETVIEW_WBATTRINFO
	{
		HD_STREETVIEW_WBATTRINFO()
		{
			memset(strMarkerID,0,WB_ID_LEN);
			memset(strObjectID,0,WB_ID_LEN);
			memset(strSSQ,0,WB_OTHER_NAME_LEN);
			memset(strQBM,0,WB_OTHER_NAME_LEN);
			memset(strSSJ,0,WB_OTHER_NAME_LEN);
			memset(strJBM,0,WB_NAME_LEN);
			memset(strSQMC,0,WB_OTHER_NAME_LEN);
			memset(strSQBM,0,WB_NAME_LEN);
			memset(strZRWGMC,0,WB_OTHER_NAME_LEN);
			memset(strZRWG,0,WB_NAME_LEN);
			memset(strWGBM,0,WB_NAME_LEN);
			memset(strWGMC,0,WB_ID_LEN);
			memset(strwbCompany,0,WB_NAME_LEN);
			memset(strwbAddress,0,WB_NAME_LEN);
			memset(strwbPeoson,0,BJ_NOTE_LEN);
			memset(strwbzgr,0,WB_OTHER_NAME_LEN);
			memset(strwbPhone,0,WB_NAME_LEN);
			memset(strwbleft,0,WB_NORMOL_LEN);
			memset(strwbright,0,WB_NORMOL_LEN);
			dwbHWidth=0.0;
			dwbZWidth=0.0;
			dwbArea=0.0;
			memset(strwbglCompany,0,WB_OTHER_NAME_LEN);
			memset(strwbgdPeoson,0,WB_NAME_LEN);
			memset(strJYLB,0,WB_OTHER_NAME_LEN);
			memset(strCZQK,0,WB_OTHER_NAME_LEN);
			memset(strCQQK,0,WB_NAME_LEN);
			memset(strQDQK,0,WB_OTHER_NAME_LEN);
			memset(strWQYY,0,WB_YY);
		}

		char strMarkerID[WB_ID_LEN];
		char strObjectID[WB_ID_LEN];
		char strSSQ[WB_OTHER_NAME_LEN];
		char strQBM[WB_OTHER_NAME_LEN];
		char strSSJ[WB_OTHER_NAME_LEN];
		char strJBM[WB_NAME_LEN];
		char strSQMC[WB_OTHER_NAME_LEN];
		char strSQBM[WB_NAME_LEN];
		char strZRWGMC[WB_OTHER_NAME_LEN];
		char strZRWG[WB_NAME_LEN];
		char strWGBM[WB_NAME_LEN];
		char strWGMC[WB_ID_LEN];
		char strwbCompany[WB_NAME_LEN];
		char strwbAddress[WB_NAME_LEN];
		char strwbPeoson[BJ_NOTE_LEN];
		char strwbzgr[WB_OTHER_NAME_LEN];
		char strwbPhone[WB_NAME_LEN];
		char strwbleft[WB_NORMOL_LEN];
		char strwbright[WB_NORMOL_LEN];
		double dwbHWidth;
		double dwbZWidth;
		double dwbArea;
		char strwbglCompany[WB_OTHER_NAME_LEN];
		char strwbgdPeoson[WB_NAME_LEN];
		HDTIME twbPeosonDatetime;
		HDTIME twbCompanyDatetime;
		char strJYLB[WB_OTHER_NAME_LEN];
		char strCZQK[WB_OTHER_NAME_LEN];
		char strCQQK[WB_NAME_LEN];
		char strQDQK[WB_OTHER_NAME_LEN];
		char strWQYY[WB_YY];
	};

	// 地图偏移量
	struct HN_MAP_OFFSETINFO
	{
		HN_MAP_OFFSETINFO()
		{
			dL = 0.0;
			dB = 0.0;
			dOffset_L = 0.0;
			dOffset_B = 0.0;
		}

		// 经度
		double dL;

		// 纬度
		double dB;

		// 经度偏移量
		double dOffset_L;

		// 纬度偏移量
		double dOffset_B;
	};
}