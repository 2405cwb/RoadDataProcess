/*! @hdMongoDBTableFieldNameDef.h
*******************************************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdMongoDBTableFieldNameDef.h
相关文件     : 
文件实现功能 : 定义MongoDB对应表格的字段名
作者         : 张阳
版本         : 1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日 期        版本     修改人              修改内容
2015/10/13   1.0      张阳                创建
</PRE>
******************************************************************************************************/

#pragma once

namespace hd
{
	// HD_ISCANINFO 
	#define ISCANINFO_DEVICENO		("DEVICENO")		// 设备编号
	#define ISCANINFO_ISCANNAME		("ISCANNAME")		// 工程名称
	#define ISCANINFO_CONFIG		("ISCANINFO")		// IScan-Route.config
	#define ISCANINFO_POSNAME		("POSNAME")			// POS文件名
	#define ISCANINFO_HDINAME		("HDINAME")			// HDI文件名
	#define ISCANINFO_CAMERASYN		("CAMERASYN")		// 相机SYN文件名
	#define ISCANINFO_LIN1			("ISCANLIN1")		// 1号扫描仪Lin文件
	#define ISCANINFO_SCANNERSYN1	("ISCANSYN1")		// 1号扫描仪Syn文件
	#define ISCANINFO_LIN2			("ISCANLIN2")		// 2号扫描仪Lin文件
	#define ISCANINFO_SCANNERSYN2	("ISCANSYN2")		// 2号扫描仪Syn文件
	#define ISCANINFO_LIN3			("ISCANLIN3")		// 3号扫描仪Lin文件
	#define ISCANINFO_SCANNERSYN3	("ISCANSYN3")		// 3号扫描仪Syn文件
	#define ISCANINFO_PARA			("ISCANPARA")		// iScan-Para.db
	#define ISCANINFO_CARNO			("CARNO")			// 车牌号
	#define ISCANINFO_REMAKE		("REMAKE")			// 备注
    #define ISCANINFO_PCD_HEADER_1  ("iScanPcdHead1")   // 扫描头1对应的头文件
    #define ISCANINFO_PCD_HEADER_2  ("iScanPcdHead2")   // 扫描头2对应的头文件
    #define ISCANINFO_PCD_HEADER_3  ("iScanPcdHead3")   // 扫描头2对应的头文件

    // HD_HDIPOINT
    #define HDI_POINT_IMAGE_ID      ("ImageID")         // 全景站点ID
    #define HDI_POINT_IMAGE_NAME    ("ImageName")       // 全景站点名称
    #define HDI_POINT_ROUTE_ID      ("RouteID")         // 轨迹ID
    #define HDI_POINT_CAMERA_NO     ("CameraNo")        // 相机号
    #define HDI_POINT_GATHER_TIME   ("GatherTime")      // 采集时间
    #define HDI_POINT_X             ("X")               // X坐标
    #define HDI_POINT_Y             ("Y")               // Y坐标
    #define HDI_POINT_Z             ("Z")               // Z坐标
    #define HDI_POINT_B             ("B")               // 纬度
    #define HDI_POINT_L             ("L")               // 经度
    #define HDI_POINT_YAW           ("Yaw")             // 相机航向角
    #define HDI_POINT_PITCH         ("Pitch")           // 相机俯仰角
    #define HDI_POINT_ROLL          ("Roll")            // 相机翻滚角
    #define HDI_POINT_SHAPE_BL      ("ShapeBL")         // 轨迹点
    #define HDI_POINT_PRJ_MEMO      ("PrjMemo")         // 工程备注

    // HD_HDILINE
    #define HDI_LINE_HDI_ID         ("HDIID")           // 轨迹标识符
    #define HDI_LINE_PRI_NAME       ("PrjName")         // 工程名称
    #define HDI_LINE_DEVICE_NO      ("DeviceNo")        // 设备标识
    #define HDI_LINE_CAMERA         ("Camera")          // 相机号
    #define HDI_LINE_START_TIME     ("StartTime")       // 采集起始时间
    #define HDI_LINE_END_TIME       ("EndTime")         // 采集结束时间
    #define HDI_LINE_PT_NUM         ("PtNum")           // 轨迹点数目
    #define HDI_LINE_X_MIN          ("XMIN")            // X轴坐标最小值
    #define HDI_LINE_Y_MIN          ("YMIN")            // Y轴坐标最小值
    #define HDI_LINE_Z_MIN          ("ZMIN")            // Z轴坐标最小值
    #define HDI_LINE_B_MIN          ("BMIN")            // 经度最小值
    #define HDI_LINE_L_MIN          ("LMIN")            // 纬度最小值
    #define HDI_LINE_X_MAX          ("XMAX")            // X轴坐标最大值
    #define HDI_LINE_Y_MAX          ("YMAX")            // Y轴坐标最大值
    #define HDI_LINE_Z_MAX          ("ZMAX")            // Z轴坐标最大值
    #define HDI_LINE_B_MAX          ("BMAX")            // 经度最大值
    #define HDI_LINE_L_MAX          ("LMAX")            // 纬度最大值
    #define HDI_LINE_SHAPE_PRJ      ("ShapePrj")        // 投影坐标几何形状
    #define HDI_LINE_SHAPE_BL       ("ShapeBL")         // 经纬度坐标几何形状

	// HD_POINTCLOUD_HEADER
	#define PCHEADER_ISCANNAME      ("ISCANNAME")		// 工程名, 索引值
	#define PCHEADER_LIDARNO		("LIDARNO")			// 扫描头编号
	#define PCHEADER_HEADER			("HEADER")			// 点云头文件数据
	#define PCHEADER_XMIN			("XMIN")			// XMIN, 索引值
	#define PCHEADER_YMIN			("YMIN")			// YMIN, 索引值
	#define PCHEADER_ZMIN			("ZMIN")			// ZMIN, 索引值
	#define PCHEADER_XMAX			("XMAX")			// XMAX, 索引值
	#define PCHEADER_YMAX			("YMAX")			// YMAX, 索引值
	#define PCHEADER_ZMAX			("ZMAX")			// ZMAX, 索引值
	#define PCHEADER_MEMO			("MEMO")			// 备注

    // HD_POINTCLOUD
    #define POINT_CLOUD_LEVEL_ID    ("LevelID")         //层级ID, 索引值, 非空
    #define POINT_CLOUD_PARCEL_ID   ("ParcelID")        //切片名称, 索引值, 非空, 唯一
    #define POINT_CLOUD_ZONE        ("Zone")            //投影代号, 索引值, 非空
    #define POINT_CLOUD_PRJ_ID      ("PrjID")           //工程ID, 索引值
    #define POINT_CLOUD_LIDAR_NO    ("LidarID")         //扫描头编号
    #define POINT_CLOUD_POINT_NUM   ("Num")             //点个数
    #define POINT_CLOUD_HAS_DATA    ("HasData")         //是否含有数据
    #define POINT_CLOUD_X_MIN       ("XMIN")            //XMIN, 索引值
    #define POINT_CLOUD_Y_MIN       ("YMIN")            //YMIN, 索引值
    #define POINT_CLOUD_Z_MIN       ("ZMIN")            //ZMIN, 索引值
    #define POINT_CLOUD_X_MAX       ("XMAX")            //XMAX, 索引值
    #define POINT_CLOUD_Y_MAX       ("YMAX")            //YMAX, 索引值
    #define POINT_CLOUD_Z_MAX       ("ZMAX")            //ZMAX, 索引值
    #define POINT_CLOUD_CRD_DATA    ("CrdData")         //坐标数据, Blob
    #define POINT_CLOUD_INT_DATA    ("IntData")         //强度数据, Blob
    #define POINT_CLOUD_CLR_DATA    ("ClrData")         //颜色数据, Blob, 可选
    #define POINT_CLOUD_CLASS_DATA  ("ClassData")       //分类数据, Blob, 可选
    #define POINT_CLOUD_GATHER_TIME ("GatherTime")      //采集时间, 用于区分版本
    #define POINT_CLOUD_PCL_MEMO    ("PclMemo")         //备注信息

    // POS_TABLE
    #define POS_POSID				("POSID")			// POS标识, 索引值
    #define POS_STARTTIME			("STARTTIME")		// 开始时间
    #define POS_ENDTIME				("ENDTIME")			// 终止时间
    #define POS_MEMO				("MEMO")			// 备注

//////////////////////////////////////////////////////////////////////////////////////////////////
	//HD_TEST
	#define TEST_INT				("INT")				// INT, 索引值
	#define TEST_STRING				("STRING")			// STRING
	#define TEST_DOUBLE				("DOUBLE")			// DOUBLE
	#define TEST_FLOAT				("FLOAT")			// FLOAT
	#define TEST_STRING_2           ("STRING_2")        // STRING
	#define TEST_LONG               ("LONG")            // LONG
	#define TEST_INT_2              ("INT_2")           // INT_2 
	#define TEST_STRING_3			("STRING_3")		// STRING
	#define TEST_DOUBLE_2			("DOUBLE_2")		// DOUBLE
	#define TEST_FLOAT_2			("FLOAT_2")			// FLOAT
	#define TEST_BINADATA			("BINADATA")        // BINADATA
}