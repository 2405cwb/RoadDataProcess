/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdDBTypeDef.h
相关文件	: hdCommon.h
文件实现功能：定义数据库类型。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/2/27	1.0			杨峰		创建
2013/8/15	1.1			刘俊		添加SEGMENT表中路名字段（RoadName）
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include <map>
#include <vector>
#include <string>
#include "hdDTEnumDef.h"
using namespace std;

namespace hd
{
	// 创建本地数据库表结构 结构体
	struct HD_CREATETABLE
	{
		// 构造函数，赋初值
		HD_CREATETABLE(ENUM_DB_TYPE eDbType)
		{
			// 先清空内存
			arrTableCmd.clear();

			// 数据库HD_STREETVIEW建表语句数组
			vector<string> arrSVCmd;
			// 数据库HD_IMAGETILE建表语句数组
			vector<string> arrTileCmd;
			// 数据库HD_RANGEIMAGE建表语句数组
			vector<string> arrRICmd;
			// 数据库HD_SYMBOL建表语句数组
			vector<string> arrSymbolCmd;
			// 数据库iScan-Image建表语句数组
			vector<string> arrImageCmd;
			// 数据库iScan-Para建表语句数组
			vector<string> arrParaCmd;
			// 数据库HD_BILLBOARD建表语句数组
			vector<string> arrBillBoardCmd;
			// 数据库HD_STREETVIEW_BJATTRINFO建表数组
			vector<string> arrBJAttrInfoCmd;
			// 数据库HD_STREETVIEW_WBATTRINFO建表shuz
			vector<string> arrWBAttrInfoCmd;
			// 数据库HD_SLOPEMONITOR_DATAINFO建表
			vector<string> arrSlopeMonitorInfoCmd;

			// 用建表语句填充数组
			char strCmd[SQL_QUERY_LEN];
			


			// 当数据库为access数据库时，含有二进制文件的建表语句
			string strBlob;
			if (eDbType == E_DT_ACCESS || eDbType == E_DT_MSSQL)
			{
				strBlob = MS_BOLB_TYPE;
			}
			else if (eDbType == E_DT_SQLITE)
			{
				strBlob = SQLITE_BOLB_TYPE;
			}

			// HD_STREETVIEW_CONFIG表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"RouteID varchar(%d) primary key,"
				"TileLevel int default 0,"
				"TileWidth int default 0,"
				"TileHeight int default 0,"
				"RIWidth int default 3600,"				
				"RIHeight int default 1800); ",
				CONFIG_TABLE, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_PROJECT表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ProjectID varchar(%d) primary key,"
				"ProjectName varchar(%d) ,"
				"TileLevel int default 0,"
				"TileWidth int default 0,"
				"TileHeight int default 0,"
				"RIWidth int default 3600,"				
				"RIHeight int default 1800, "//,//GatherTime
				"GatherTime datetime ,"
				"CompanyName varchar(%d) ,"
				"DataProcessTime datetime ,"
				"ProcessorName varchar(%d),"
				"Angle float default 0.0);",
				
				PROJECT_TABLE, OBJECT_ID_LEN, NAME_LEN, NAME_LEN, NAME_LEN);
			arrSVCmd.push_back(strCmd);


			// HD_STREETVIEW_LINK表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"LinkID varchar(%d) primary key,"
				"ProjectID varchar(%d) ,"
				"RouteID varchar(%d) ,"
				"SrcImageName varchar(%d) ,"
				"DstImageName varchar(%d) ,"
				"DstName varchar(%d),"
				"Direction int default 1);",				

				LINK_TABLE, OBJECT_ID_LEN, OBJECT_ID_LEN, OBJECT_ID_LEN, PANO_ID_LEN, PANO_ID_LEN, NAME_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageName varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"CameraNo int default 0,"
				"SegmentID varchar(%d),"
				"SegmentIndex int default -1,"				
				"GatherTime datetime,"
				"X float default 0.0,"
				"Y float default 0.0,"
				"Z float default 0.0,"
				"B float default 0.0,"
				"L float default 0.0,"
				"Yaw float default 0.0,"
				"Pitch float default 0.0,"
				"Roll float default 0.0); ",
				IMAGEINFO_TABLE, PANO_ID_LEN, OBJECT_ID_LEN,OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEFACADE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d),"
				"FacadeID varchar(%d),"
				"primary key(ImageID, FacadeID));",
				IMAGEFACADE_TABLE, PANO_ID_LEN, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_MARKERINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"Name varchar(%d), "
				"SymbolID varchar(%d),"
				"CenterX float default 0.0,"
				"CenterY float default 0.0,"
				"CenterZ float default 0.0,"
				"NormalX float default 0.0,"
				"NormalY float default 0.0,"
				"NormalZ float default 0.0,"
				"ScaleX	float default 1.0,"
				"ScaleY	float default 1.0,"
				"ScaleZ	float default 1.0,"
				"SouceID varchar(%d),"
				"Edited  bit default 0,"
				"Points  %s default null,"
				"SymbolType	int default 1);",
				MARKER_TABLE, MARK_ID_LEN_L, OBJECT_ID_LEN, NAME_LEN, OBJECT_ID_LEN, OBJECT_ID_LEN, strBlob.data());
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEMARKER建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d),"
				"MarkerID varchar(%d),"
				"primary key(ImageID, MarkerID));",
				IMAGEMARKER_TABLE, PANO_ID_LEN, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_NODE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"NodeID varchar(%d) primary key,"
				"X float default 0.0,"
				"Y float default 0.0,"
				"B float default 0.0,"
				"L float default 0.0);",
				NODE_TABLE, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_MARKERATT建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"PyName varchar(%d),"
				"Address varchar(%d),"
				"PhoneNum varchar(%d));",
				MARKERATT_TABLE, OBJECT_ID_LEN, NAME_LEN, ADDRESS_LEN, PHONENUM_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_ISCANPARA表
			//memset(strCmd, 0, SQL_QUERY_LEN);
			//sprintf(strCmd, "Create Table %s("
			//	"iScanNo varchar(%d),"
			//	"ParaTime datetime,"
			//	"MemoInfo varchar(%d),"
			//	"primary key(iScanNo, ParaTime));",
			//	ISCANPARA_TABLE, DEVICE_NO_LEN, REMARK_LEN);
			//arrParaCmd.push_back(strCmd);

			// HD_ISCANPANO_PARA表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"iScanNo varchar(%d),"
				"iScanPanoNo varchar(%d),"
				"ParaTime datetime,"
				"X float default 0.0,"
				"Y float default 0.0,"
				"Z float default 0.0,"
				"Yaw float default 0.0,"
				"Pitch float default 0.0,"
				"Roll float default 0.0,"
				"Width int default 0,"
				"Height int default 0,"
				"HoriStartAngle float default 0.0,"
				"HoriEndAngle float default 0.0,"
				"VertStartAngle float default 0.0,"
				"VertEndAngle float default 0.0,"
				"MemoInfo varchar(%d),"
				"primary key(iScanNo, iScanPanoNo, ParaTime));",
				ISCANCAMERA_TABLE, DEVICE_NO_LEN, SENSE_NO_LEN, REMARK_LEN);
			arrParaCmd.push_back(strCmd);

			// HD_ISCAN_IMAGE_PARA表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"type int default 0,"
				"fx float default 0.0,"
				"fy float default 0.0,"
				"cx float default 0.0,"
				"cy float default 0.0,"
				"k1 float default 0.0,"
				"k2 float default 0.0,"
				"k3 float default 0.0,"
				"k4 float default 0.0,"
				"k5 float default 0.0,"
				"k6 float default 0.0,"
				"p1 float default 0.0,"
				"p2 float default 0.0,"
				"lmd1 float default 0.0,"
				"lmd2 float default 0.0,"
				"lmd3 float default 0.0,"
				"fArmX float default 0.0,"
				"fArmY float default 0.0,"
				"fArmZ float default 0.0,"
				"fRPhi float default 0.0,"
				"fROmg float default 0.0,"
				"fRKap float default 0.0,"
				"ParaTime datetime,"
				"fOffsetX float default 0.0,"
				"fOffsetY float default 0.0,"
				"fOffsetZ float default 0.0,"
				"fAngleX float default 0.0,"
				"fAngleY float default 0.0,"
				"fAngleZ float default 0.0);",
				ISCAN_MZ_IMAGEPARA_TABLE, DEVICE_NO_LEN, SENSE_NO_LEN, REMARK_LEN);
			arrParaCmd.push_back(strCmd);

			
			// HD_ISCANLIDAR_PARA表
			//memset(strCmd, 0, SQL_QUERY_LEN);
			//sprintf(strCmd, "Create Table %s("
			//	"iScanNo varchar(%d),"
			//	"iScanLidarNo varchar(%d),"
			//	"ParaTime datetime,"
			//	"X float default 0.0,"
			//	"Y float default 0.0,"
			//	"Z float default 0.0,"
			//	"Yaw float default 0.0,"
			//	"Pitch float default 0.0,"
			//	"Roll float default 0.0,"
			//	"MemoInfo varchar(%d),"
			//	"primary key(iScanNo, iScanLidarNo, ParaTime));",
			//	ISCANLIDAR_TABLE, DEVICE_NO_LEN, SENSE_NO_LEN, REMARK_LEN);
			//arrParaCmd.push_back(strCmd);


			// HD_STREETVIEW_FACADEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"FacadeID varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"Type int default 1,"
				"Points %s default null,"
				"CenterX float default 0.0,"
				"CenterY float default 0.0,"
				"NormalX float default 0.0,"
				"NormalY float default 0.0,"
				"NormalZ float default 0.0);",
				FACADE_TABLE, OBJECT_ID_LEN,OBJECT_ID_LEN, strBlob.data());
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_ROUTE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"RouteID varchar(%d) primary key,"
				"RouteName varchar(%d),"
				"DeviceNo varchar(%d),"
				"StartTime datetime,"
				"EndTime datetime,"
				"PtNum int default 0,"
				"ShapeXY %s default null,"
				"ShapeBL %s default null);",
				ROUTE_TABLE, OBJECT_ID_LEN, NAME_LEN, DEVICE_NO_LEN, strBlob.data(), strBlob.data());
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_SEGMENT建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"SegmentID varchar(%d) primary key,"
				"RouteID varchar(%d),"
				"DeviceNo varchar(%d),"
				"RoadName varchar(%d),"
				"StartTime datetime,"
				"EndTime datetime,"
				"StartNodeID varchar(%d),"
				"EndNodeID varchar(%d),"
				"PtNum int default 0,"
				"ShapeXY %s default null,"
				"ShapeBL %s default null);",
				SEGMENT_TABLE, OBJECT_ID_LEN, OBJECT_ID_LEN, DEVICE_NO_LEN, NAME_LEN,
				OBJECT_ID_LEN, OBJECT_ID_LEN, strBlob.data(), strBlob.data());
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_TILEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"TileID	 varchar(%d) primary key,"
				"ProjectID varchar(%d) default null,"
				"TileData  %s default null);",
				TILEINFO_TABLE, TILE_ID_LEN, OBJECT_ID_LEN, strBlob.data());
			arrTileCmd.push_back(strCmd);

			// HD_STREETVIEW_RANGEIMAGEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d) primary key,"
				"ProjectID varchar(%d) default null,"
				"MaxDis	float default 0.0,"
				"MinDis	float default 0.0,"
				"MapData %s default null);",
				RIINFO_TABLE, PANO_ID_LEN,OBJECT_ID_LEN, strBlob.data());
			arrRICmd.push_back(strCmd);

			// HD_STREETVIEW_SYMBOLINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"SymbolID varchar(%d) primary key,"
				"Name varchar(%d),"
				"SymbolType	int default 1,"
				"FileType int default 0,"
				"FileData %s default null);",
				SYMBOL_TABLE, OBJECT_ID_LEN, NAME_LEN, strBlob.data());
			arrSymbolCmd.push_back(strCmd);

			// HD_IMAGEDATA建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d)  primary key,"
				"ImageData %s default null,"
				"BlurAreaData %s default null);",
				IMAGE_TABLE, PANO_ID_LEN, strBlob.data(), strBlob.data());
			arrImageCmd.push_back(strCmd);

			// HD_BILLBOARD建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"BillBoardID varchar(%d)  primary key,"
				"PointCount int default 0,"
				"Shape %s default null,"
				"CenterX float default 0.0,"
				"CenterY float default 0.0,"
				"CenterZ float default 0.0,"
				"Owner varchar(%d),"
				"SetTime datetime,"
				"StartTime datetime,"				
				"EndTime datetime);",
				BILLBOARD_TABLE, OBJECT_ID_LEN, strBlob.data(), OWNER_LEN);
			arrBillBoardCmd.push_back(strCmd);

			// HD_STREETVIEW_ROADADDRESS建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID varchar(%d),"
				"Name varchar(%d),"
				"Area varchar(%d),"
				"PointCount int default 0,"
				"Shape %s default null);",
				ROADADDRESS_TABLE, OBJECT_ID_LEN, NAME_LEN, NAME_LEN, strBlob.data());
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_BJATTRINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"PCODE varchar(%d),"
				"PSNAME varchar(%d),"
				"PSID varchar(%d),"
				"PBNAME varchar(%d),"
				"PBID varchar(%d),"
				"DEPNAME varchar(%d),"
				"DEPID varchar(%d),"
				"GRIDID varchar(%d),"
				"STATE varchar(%d),"
				"ISVALID varchar(%d),"
				"REVIEWSTAT varchar(%d),"
				"DATASOURCE varchar(%d),"
				"CREATEDATE datetime,"
				"REMARK varchar(%d),"
				"PARTPOS varchar(%d),"
				"ACTIONTYPE varchar(%d),"
				"BGRQ datetime,"
				"ICONTYPE varchar(%d),"
				"ICON %s default null,"
				"MATERIAL varchar(%d),"
				"ROADNAME varchar(%d),"
				"STARNAME varchar(%d),"
				"ENDNAME varchar(%d),"
				"LOCATION varchar(%d),"
				"WHATSHAPE varchar(%d),"
				"PARTNUM int default 0,"
				"PARTSIZE varchar(%d),"
				"MAINTAIN varchar(%d),"
				"MARKFLAG int default 0,"
				"CENSUS datetime,"
				"PARTPHOTO varchar(%d),"
				"HAVEPHOTO int default 0,"
				"UPDATETIME datetime,"
				"SQNAME varchar(%d),"
				"SYMBOLTYPE int default 0);",
				BJATTRINFO_TABLE, 50, 16, 100, 10, 100, 10, 100, 100, 15,
				2, 2, 2, 2, 1000, 254, 2, 2, strBlob.data(), 100, 100, 100,
				100, 100, 100, 100, 100, 500, 100);
				arrBJAttrInfoCmd.push_back(strCmd);//部件表

			// 五包建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"OBJECTID varchar(%d),"
				"QHNAME varchar(%d),"
				"QHCODE varchar(%d),"
				"JDNAME varchar(%d),"
				"JDCODE varchar(%d),"
				"SQNAME varchar(%d),"
				"SQCODE varchar(%d),"
				"ZRNAME varchar(%d),"
				"ZRCODE varchar(%d),"
				"WBCODE varchar(%d),"
				"WGMC varchar(%d),"
				"wbCompany varchar(%d),"
				"wbAddress varchar(%d),"
				"wbPeoson varchar(%d),"
				"wbzgr varchar(%d),"
				"wbPhone varchar(%d),"
				"wbleft varchar(%d),"
				"wbright varchar(%d),"
				"wbHWidth float default 0.0,"
				"wbZWidth float default 0.0,"
				"wbArea float default 0.0,"
				"WBGLCOM varchar(%d),"
				"WBGDPEO varchar(%d),"
				"WBPDATET datetime,"
				"WBCOMDT datetime,"
				"JYLB varchar(%d),"
				"CZQK varchar(%d),"
				"CQQK varchar(%d),"
				"QDQK varchar(%d),"
				"WQYY varchar(%d));",
				WBATTRINFO_TABLE, 50, 20, 100, 100, 100, 50, 100, 50, 100,
				50, 50, 50, 50, 50, 1000, 100, 50, 200, 200, 100, 50, 100,
				100, 50, 100, 2000);
			arrWBAttrInfoCmd.push_back(strCmd);
			
			// 滑坡监测access数据库HD_SLOPEMONITOR_DATAINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"FID Counter(1,1) primary key,"
				"RouteID varchar(%d),"
				"ParaTime varchar(%d),"
				"PtIndex int default 0,"
				"X double default 0.0,"
				"Y double default 0.0,"
				"Z double default 0.0);",
				MS_ACCESS_SLOPE_MONITOR_TABLE, OBJECT_ID_LEN_L,32,MS_ACCESS_SLOPE_MONITOR_TABLE);
			arrSlopeMonitorInfoCmd.push_back(strCmd);
		
			//"CONSTRAINT[pk_%s] PRIMARY KEY CLUSTERED(\
			//[FID]\
			//ON [PRIMARY])\
			


			// 填充hashtable
			arrTableCmd.insert(pair <string, vector<string>>(SV_DB, arrSVCmd));
			arrTableCmd.insert(pair <string, vector<string>>(TILE_DB, arrTileCmd));
			arrTableCmd.insert(pair <string, vector<string>>(RI_DB, arrRICmd));
			arrTableCmd.insert(pair <string, vector<string>>(SYMBOL_DB, arrSymbolCmd));
			arrTableCmd.insert(pair <string, vector<string>>(IMAGE_DB, arrImageCmd));
			arrTableCmd.insert(pair <string, vector<string>>(PARA_DB, arrParaCmd));
			arrTableCmd.insert(pair <string, vector<string>>(BILLBOARD_DB, arrBillBoardCmd));
			//添加部件表建表语句
			arrTableCmd.insert(pair <string, vector<string>>(BJATTRINFO_TABLE,arrBJAttrInfoCmd));
			arrTableCmd.insert(pair <string, vector<string>>(WBATTRINFO_TABLE,arrWBAttrInfoCmd));

			// 滑坡监测建表语句
			arrTableCmd.insert(pair <string,vector<string>>(SLOPMONITOR_DB,arrSlopeMonitorInfoCmd));
		}

		// 析构函数，清空内存
		~HD_CREATETABLE()
		{
			arrTableCmd.clear();
		}

		// 数据库建表语句集合，key:数据库名，value:对应的数据库建表语句
		map<string, vector<string>> arrTableCmd;
	};

	// 创建SQL SERVER数据库表结构 结构体
	struct HD_CREATETABLE_SQLSERVER
	{
		// 构造函数，赋初值
		HD_CREATETABLE_SQLSERVER()
		{
			// 先清空内存
			arrSVCmd.clear();			

			// 用建表语句填充数组
			char strCmd[SQL_QUERY_LEN];

			// HD_STREETVIEW_CONFIG表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"RouteID varchar(%d) primary key,"
				"TileLevel int default 0,"
				"TileWidth int default 0,"
				"TileHeight int default 0,"
				"RIWidth int default 3600,"				
				"RIHeight int default 1800); ",
				CONFIG_TABLE, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_PROJECT表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ProjectID varchar(%d) primary key,"
				"ProjectName varchar(%d) ,"
				"TileLevel int default 0,"
				"TileWidth int default 0,"
				"TileHeight int default 0,"
				"RIWidth int default 3600,"				
				"RIHeight int default 1800, "//,//GatherTime
				"GatherTime datetime ,"
				"CompanyName varchar(%d) ,"
				"DataProcessTime datetime ,"
				"ProcessorName varchar(%d),"
				"Angle float default 0.0);",

				PROJECT_TABLE, OBJECT_ID_LEN, NAME_LEN, NAME_LEN, NAME_LEN);
			arrSVCmd.push_back(strCmd);


			// HD_STREETVIEW_LINK表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"LinkID varchar(%d) primary key,"
				"ProjectID varchar(%d) ,"
				"RouteID varchar(%d) ,"
				"SrcImageName varchar(%d) ,"
				"DstImageName varchar(%d) ,"
				"DstName varchar(%d),"	
				"Direction int default 1);",

				LINK_TABLE, OBJECT_ID_LEN, OBJECT_ID_LEN, OBJECT_ID_LEN, PANO_ID_LEN, PANO_ID_LEN, NAME_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageName varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"CameraNo int default 0,"
				"SegmentID varchar(%d),"
				"SegmentIndex int default -1,"				
				"GatherTime datetime,"
				"X float default 0.0,"
				"Y float default 0.0,"
				"Z float default 0.0,"
				"B float default 0.0,"
				"L float default 0.0,"
				"Yaw float default 0.0,"
				"Pitch float default 0.0,"
				"Roll float default 0.0); ",
				IMAGEINFO_TABLE, PANO_ID_LEN, OBJECT_ID_LEN,OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEFACADE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d),"
				"FacadeID varchar(%d),"
				"primary key(ImageID, FacadeID));",
				IMAGEFACADE_TABLE, PANO_ID_LEN, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_MARKERINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"Name varchar(%d), "
				"SymbolID varchar(%d),"
				"CenterX float default 0.0,"
				"CenterY float default 0.0,"
				"CenterZ float default 0.0,"
				"NormalX float default 0.0,"
				"NormalY float default 0.0,"
				"NormalZ float default 0.0,"
				"ScaleX	float default 1.0,"
				"ScaleY	float default 1.0,"
				"ScaleZ	float default 1.0,"
				"SouceID varchar(%d),"
				"Edited  bit default 0,"
				"Points  %s default null,"
				"SymbolType	int default 1);",
				MARKER_TABLE, MARK_ID_LEN_L, OBJECT_ID_LEN, NAME_LEN, OBJECT_ID_LEN, OBJECT_ID_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_IMAGEMARKER建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d),"
				"MarkerID varchar(%d),"
				"primary key(ImageID, MarkerID));",
				IMAGEMARKER_TABLE, PANO_ID_LEN, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_NODE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"NodeID varchar(%d) primary key,"
				"X float default 0.0,"
				"Y float default 0.0,"
				"B float default 0.0,"
				"L float default 0.0);",
				NODE_TABLE, OBJECT_ID_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_MARKERATT建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"PyName varchar(%d),"
				"Address varchar(%d),"
				"PhoneNum varchar(%d));",
				MARKERATT_TABLE, OBJECT_ID_LEN, NAME_LEN, ADDRESS_LEN, PHONENUM_LEN);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_FACADEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"FacadeID varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"Type int default 1,"
				"Points %s default null,"
				"CenterX float default 0.0,"
				"CenterY float default 0.0,"
				"NormalX float default 0.0,"
				"NormalY float default 0.0,"
				"NormalZ float default 0.0);",
				FACADE_TABLE, OBJECT_ID_LEN,OBJECT_ID_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_ROUTE建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"RouteID varchar(%d) primary key,"
				"RouteName varchar(%d),"
				"DeviceNo varchar(%d),"
				"StartTime datetime,"
				"EndTime datetime,"
				"PtNum int default 0,"
				"ShapeXY %s default null,"
				"ShapeBL %s default null);",
				ROUTE_TABLE, OBJECT_ID_LEN, NAME_LEN, DEVICE_NO_LEN, MS_BOLB_TYPE, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_SEGMENT建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"SegmentID varchar(%d) primary key,"
				"RouteID varchar(%d),"
				"DeviceNo varchar(%d),"
				"RoadName varchar(%d),"
				"StartTime datetime,"
				"EndTime datetime,"
				"StartNodeID varchar(%d),"
				"EndNodeID varchar(%d),"
				"PtNum int default 0,"
				"ShapeXY %s default null,"
				"ShapeBL %s default null);",
				SEGMENT_TABLE, OBJECT_ID_LEN, OBJECT_ID_LEN, DEVICE_NO_LEN, NAME_LEN,
				OBJECT_ID_LEN, OBJECT_ID_LEN, MS_BOLB_TYPE, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_TILEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"TileID	 varchar(%d) primary key,"
				"ProjectID varchar(%d),"
				"TileData  %s default null);",
				TILEINFO_TABLE, TILE_ID_LEN, OBJECT_ID_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_RANGEIMAGEINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ImageID varchar(%d) primary key,"	
				"ProjectID varchar(%d),"
				"MaxDis	float default 0.0,"
				"MinDis	float default 0.0,"
				"MapData %s default null);",
				RIINFO_TABLE, PANO_ID_LEN, OBJECT_ID_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_SYMBOLINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"SymbolID varchar(%d) primary key,"
				"Name varchar(%d),"
				"SymbolType	int default 1,"
				"FileType int default 0,"
				"FileData %s default null);",
				SYMBOL_TABLE, OBJECT_ID_LEN, NAME_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_ROADADDRESS建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID varchar(%d),"
				"Name varchar(%d),"
				"Area varchar(%d),"
				"PointCount int default 0,"
				"Shape %s default null);",
				ROADADDRESS_TABLE, OBJECT_ID_LEN, NAME_LEN, NAME_LEN, MS_BOLB_TYPE);
			arrSVCmd.push_back(strCmd);

			// HD_STREETVIEW_BJATTRINFO建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"PCODE varchar(%d),"
				"PSNAME varchar(%d),"
				"PSID varchar(%d),"
				"PBNAME varchar(%d),"
				"PBID varchar(%d),"
				"DEPNAME varchar(%d),"
				"DEPID varchar(%d),"
				"GRIDID varchar(%d),"
				"STATE varchar(%d),"
				"ISVALID varchar(%d),"
				"REVIEWSTAT varchar(%d),"
				"DATASOURCE varchar(%d),"
				"CREATEDATE datetime,"
				"REMARK varchar(%d),"
				"PARTPOS varchar(%d),"
				"ACTIONTYPE varchar(%d),"
				"BGRQ datetime,"
				"ICONTYPE varchar(%d),"
				"ICON %s default null,"
				"MATERIAL varchar(%d),"
				"ROADNAME varchar(%d),"
				"STARNAME varchar(%d),"
				"ENDNAME varchar(%d),"
				"LOCATION varchar(%d),"
				"WHATSHAPE varchar(%d),"
				"PARTNUM int default 0,"
				"PARTSIZE varchar(%d),"
				"MAINTAIN varchar(%d),"
				"MARKFLAG int default 0,"
				"CENSUS datetime,"
				"PARTPHOTO varchar(%d),"
				"HAVEPHOTO int default 0,"
				"UPDATETIME datetime,"
				"SQNAME varchar(%d),"
				"SYMBOLTYPE int default 0);",
				BJATTRINFO_TABLE, 50, 16, 100, 10, 100, 10, 100, 100, 15,
				2, 2, 2, 2, 1000, 254, 2, 2, MS_BOLB_TYPE, 100, 100, 100,
				100, 100, 100, 100, 100, 500, 100);
			arrSVCmd.push_back(strCmd);

			// 五包建表语句
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d),"
				"OBJECTID varchar(%d),"
				"QHNAME varchar(%d),"
				"QHCODE varchar(%d),"
				"JDNAME varchar(%d),"
				"JDCODE varchar(%d),"
				"SQNAME varchar(%d),"
				"SQCODE varchar(%d),"
				"ZRNAME varchar(%d),"
				"ZRCODE varchar(%d),"
				"WBCODE varchar(%d),"
				"WGMC varchar(%d),"
				"wbCompany varchar(%d),"
				"wbAddress varchar(%d),"
				"wbPeoson varchar(%d),"
				"wbzgr varchar(%d),"
				"wbPhone varchar(%d),"
				"wbleft varchar(%d),"
				"wbright varchar(%d),"
				"wbHWidth float default 0.0,"
				"wbZWidth float default 0.0,"
				"wbArea float default 0.0,"
				"WBGLCOM varchar(%d),"
				"WBGDPEO varchar(%d),"
				"WBPDATET datetime,"
				"WBCOMDT datetime,"
				"JYLB varchar(%d),"
				"CZQK varchar(%d),"
				"CQQK varchar(%d),"
				"QDQK varchar(%d),"
				"WQYY varchar(%d));",
				WBATTRINFO_TABLE, 50, 20, 100, 100, 100, 50, 100, 50, 100,
				50, 50, 50, 50, 50, 1000, 100, 50, 200, 200, 100, 50, 100,
				100, 50, 100, 2000);
			arrSVCmd.push_back(strCmd);
		}

		// 析构函数，清空内存
		~HD_CREATETABLE_SQLSERVER()
		{
			arrSVCmd.clear();
		}

		// 数据库HD_STREETVIEW建表语句数组
		vector<string> arrSVCmd;
	};	

	struct HD_CREATETABLE_BILLBOARD
	{
		HD_CREATETABLE_BILLBOARD()
		{
			memset(strCmd, 0, 1024);
			sprintf(strCmd, "Create Table %s("
				"MarkerID varchar(%d) primary key,"
				"Company varchar(%d) ,"
				"Address varchar(%d),"
				"Phone varchar(%d),"
				"Website varchar(%d),"
				"Duration varchar(%d),"				
				"Area float default 0.0, "//,//GatherTime
				"Prices float default 0.0,"
				"Width float default 0.0,"
				"Height float default 0.0,"
				"SymbolType float default 15,"
				"Photo image default null ,"
				"State varchar(%d) ,"
				"Location varchar(%d) ,"
				"EditDate datetime,"
				"Remark varchar(%d)"
				");",
				BILLBOARD_TABLE, MARK_ID_LEN_L, MARK_ID_LEN_L, MARK_ID_LEN_L, 16, MARK_ID_LEN_L, MARK_ID_LEN_L,
				2,50,1000);
		}


		char strCmd[1024];
	};
}