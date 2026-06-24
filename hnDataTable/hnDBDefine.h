#ifndef _HN_DB_DEFINE_H_
#define _HN_DB_DEFINE_H_
#include "stdafx.h"
#include <string>
#include <vector>
#include "..\hnCommon\hnCommonDef.h"

using namespace std;

//namespace hnDataTable
//{
	// 不同规范道路参数表定义
	#define DISEASE_SETTING_INFO_TABLE	    ("DISEASE_SETTING_INFO")     // 病害设置参数表
	#define ROAD_TYPE_SETTING_INFO_TABLE    ("ROAD_TYPE_SETTING_INFO")	 // 道路类型设置参数表

	// 成果数据库表定义
	#define SETTING_INFO_TABLE              ("SETTING_INFO")			// 设置信息表
	#define MARK_INFO_TABLE                 ("MARK_INFO")			    // 打标信息表
	#define MILEAGE_PILE_TABLE              ("MILEAGE_PILE")	        // 里程信息表
    #define CTRL_POINT_TABLE				("CTRL_POINT_TABLE")		// 控制点表
    #define  MILE_INFO_TABLE  ("MILE_INFO_TABLE") //桩号表

	// 创建本地不同规范道路参数表结构 结构体
	struct HN_CREATE_SETTINGTABLE
	{
		HN_CREATE_SETTINGTABLE()
		{
			// 清空数据
			vecCmds.clear();

			// 病害设置参数表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default 0 primary key,"
				"DiseaseIndex int default 0,"
				"DiseaseType int default 0,"
				"RoadSurfaceType int default 0,"
				"DrawType int default 0,"
				"Level int default 0,"
				"ShowState int default 0,"
				"Widget float default 0.0,"
				"EffectType float default 0.0,"
				"EffectWid float default 0.0,"
				"ValidLen float default 0.0,"
				"ValidArea float default 0.0,"
				"AreaFormula int default 0,"
				"ShortcutKey varchar(%d),"
				"DWKF int default 0,"
				"EffectMeasure float default 0.0,"
				"DiseaseName varchar(%d),"
				"DiseaseTypeName varchar(%d),"
				"DBTableNam varchar(%d),"
				"RoadType varchar(%d),"
				"SHMD varchar(%d),"
				"DXKF varchar(%d),"
				"DisFullName varchar(%d),"
				"Describe varchar(%d),"
				"AddFile1 varchar(%d),"
				"AddFile2 varchar(%d),"
				"AddFile3 varchar(%d),"
				"AddFile4 varchar(%d),"
				"AddFile5 varchar(%d),"
				"Remark varchar(%d));",
				DISEASE_SETTING_INFO_TABLE, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,
				SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);
			vecCmds.push_back(strCmd);

			// 道路类型设置参数表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default -1 primary key,"
				"RSurfaceType int default 0,"
				"DrawType int default 0,"
				"RoadLevel varchar(%d),"
				"RutThreslodUp int default 0,"
				"RutThreslodDown int default 0,"
				"RutThreslod float default 0.0,"
				"RutIndex int default 0,"
				"RealV varchar(%d),"
				"AmendPara varchar(%d),"
				"RQI_a0 float default 0.0,"
				"RQI_a1 float default 0.0,"
				"RQI_w1 float default 0.0,"
				"RQI_w2 float default 0.0,"
				"PCI_a0 float default 0.0,"
				"PCI_a1 float default 0.0,"
				"PQI_WPCI float default 0.0,"
				"PQI_WRQI float default 0.0,"
				"PQI_WRDI float default 0.0,"
				"PQI_WPBI float default 0.0,"
				"PQI_WPWI float default 0.0,"
				"RDI_a float default 0.0,"
				"RDI_b float default 0.0,"
				"RDI_RDa float default 0.0,"
				"RDI_RDb float default 0.0,"
				"RDI_a0 float default 0.0,"
				"RDI_a1 float default 0.0,"
				"PWI_a0 float default 0.0,"
				"PWI_a1 float default 0.0,"
				"MQI_WSCI float default 0.0,"
				"MQI_WPQI float default 0.0,"
				"MQI_WBCI float default 0.0,"
				"MQI_WTCI float default 0.0,"
				"RoadFullName varchar(%d),"
				"SN_wi varchar(%d),"
				"LQ_wi varchar(%d),"
				"RQILevel varchar(%d),"
				"RDILevel varchar(%d),"
				"PWILevel varchar(%d),"
				"MTDLevel varchar(%d),"
				"IRILevel varchar(%d),"
				"PCILevel varchar(%d),"
				"PQILevel varchar(%d),"
				"PBILevel varchar(%d),"
				"MQILevel varchar(%d),"
				"PBI_KFBZ varchar(%d),"
				"PBI_KF varchar(%d),"
				"RoadType varchar(%d),"
				"AddFile1 varchar(%d),"
				"AddFile2 varchar(%d),"
				"AddFile3 varchar(%d),"
				"AddFile4 varchar(%d),"
				"AddFile5 varchar(%d),"
				"Remark varchar(%d));",
				ROAD_TYPE_SETTING_INFO_TABLE, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN,
				SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,
				SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);
			vecCmds.push_back(strCmd);

		}

		// 成果数据库建表语句
		vector<string> vecCmds;

		// 用建表语句填充数组
		char strCmd[SQL_QUERY_LEN];
	};

	// 创建本地成果数据库表结构 结构体
	struct HN_CREATE_RESULT_TABLE
	{
		HN_CREATE_RESULT_TABLE()
		{
			// 清空数据
			vecCmds.clear();

			// 设置信息表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default 0 primary key,"
				"BegMile double default 0.0,"
				"EndMile double default 0.0,"
				"BegEnclMile double default 0.0,"
				"EndEnclMile double default 0.0,"
				"LineType int default 0,"
				"DrawType int default 0,"		
				"RSurfaceType int default 0,"
				"Length double default 0.0,"
				"WorkType int default 0,"
				"ImageExtent int default 0,"
				"Frequency int default 0,"
				"WheelPerimeter double default 0.0,"
				"Province varchar(%d),"
				"City varchar(%d),"
				"County varchar(%d),"
				"RoadName varchar(%d),"
				"Date varchar(%d),"
				"Timer varchar(%d),"
				"RoadLevel varchar(%d),"
				"Surveyor varchar(%d),"
				"Weather varchar(%d),"
				"RoadType varchar(%d),"
				"RoadNO varchar(%d),"
				"Number varchar(%d),"
				"Model varchar(%d),"
				"Width double default 3.75,"
				"AddFile2 varchar(%d),"
				"AddFile3 varchar(%d),"
				"AddFile4 varchar(%d),"
				"AddFile5 varchar(%d),"
				"Remark varchar(%d),"
				"RadioX double default 0.0,"
				"RadioY double default 0.0,"
				"RoadLength double default 0.0,"
				"PicPixelX int default 0.0,"
				"PicPixelY int default 0.0);",
				SETTING_INFO_TABLE, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN,
				SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN, SQL_NAME_LEN,SQL_ADDFILE_LEN,  SQL_ADDFILE_LEN,SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,
				SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);
			vecCmds.push_back(strCmd);

			// 打标信息表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default 0 primary key,"
				"Type int default0,"
				"EnclMile double default 0.0,"
				"TrueMile double default 0.0,"
				"GpsTimer double default 0.0,"
				"Mark varchar(%d),"
				"AddFile1 varchar(%d),"
				"AddFile2 varchar(%d),"
				"AddFile3 varchar(%d),"
				"AddFile4 varchar(%d),"
				"AddFile5 varchar(%d),"
				"Remark varchar(%d));",
				MARK_INFO_TABLE, SQL_NAME_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, 
				SQL_ADDFILE_LEN);
			vecCmds.push_back(strCmd);

			// 里程信息表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default -1 primary key,"
				"DMi int64 default 0,"
				"EnclMile double default 0.0,"
				"TrueMile double default 0.0,"
				"GpsTimer double default 0.0,"
				"AddFile1 varchar(%d),"
				"AddFile2 varchar(%d),"
				"AddFile3 varchar(%d),"
				"AddFile4 varchar(%d),"
				"AddFile5 varchar(%d),"
				"Remark varchar(%d));",
				MILEAGE_PILE_TABLE, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,
				SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);
			vecCmds.push_back(strCmd);

			// 控制点表
			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default -1 primary key,"
				"KzdName varchar(%d),"
				"GpsTimer double default 0.0,"
				"Mileage double default 0,"
				"ImageName varchar(%d),"
				"LocX int64 default 0,"
				"LocY int64 default 0,"
				"X double default 0.0,"
				"Y double default 0.0,"
				"Z double default 0.0,"
				"Remarks varchar(%d));",
				CTRL_POINT_TABLE, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN,SQL_ADDFILE_LEN);
			 //桩号表

			memset(strCmd, 0, SQL_QUERY_LEN);
			sprintf(strCmd, "Create Table %s("
				"ID int default -1 primary key,"
				"EnclMile double default 0.0,"
				"GpsTimer double default 0.0,"
				"TrueMile double default 0.0,"
				"picturePath varchar(%d),"
				"leftStreetPicPath varchar(%d),"
				"rightStreetPicPath varchar(%d),"
				"drawType int default 0,"
				"roadType int default 0,"
				"roadStandard int default 0,"
				"roadUnitStr varchar(%d),"
				"roadGradStr varchar(%d)," 
				"roadGrad int default 0,"
				"roadWidth double default 0.0);",
				MILE_INFO_TABLE, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN); 
			vecCmds.push_back(strCmd);
			createCtrlPointTableCmd = strCmd;
		}
		// 成果数据库建表语句
		vector<string> vecCmds;

		// 创建控制点表的语句
		std::string createCtrlPointTableCmd;

		// 用建表语句填充数组
		char strCmd[SQL_QUERY_LEN];
	};
//}

#endif

