#pragma once

namespace hd
{
    #define PANO_ID_LEN				64		// 全景站点ID长度
    #define OBJECT_ID_LEN			64		// 对象ID长度
    #define OBJECT_ID_LEN_L			128		// 长对象长度
    #define MARK_ID_LEN_L           50      // 长MarkID
    #define NAME_LEN				100		// 线路、符号等的名称
    #define	DEVICE_NO_LEN			10		// 设备标识长度（设备型号+设备编号）
    #define REMARK_LEN				128		// 备注长度
    #define SENSE_NO_LEN			2		// 传感器编号长度
    #define TILE_ID_LEN				64		// 切片文件ID长度
    #define SQL_QUERY_LEN			1500	// SQL查询语句（长度不够可随时增加）
    #define SQL_QUERY_LEN_L			10000	// SQL查询语句（长度不够可随时增加）
    #define SCAN_ROUTE_ID			128		// 轨迹ID长度
    #define OWNER_LEN				255		// 广告牌权属字段长度
    #define ADDRESS_LEN				255		// 标注地址长度
    #define PHONENUM_LEN			255		// 电话号码长度	
    #define VERSION_LEN				10		// 版本编号
    #define MODE_LEN				10		// 采集模式编号
    #define CARNUM_LEN              5       // 车牌号长度 2014/15/05 lwm
    #define DATAPATH_LEN  500 //数据存放路径长度 [2015/06/17 luowenmin]
    // 定义部件长度
    #define BJ_ID_LEN               25      // 部件ID长度
    #define BJ_CLASSID_LEN          15
    #define BJ_NAME_LEN             100     // 名字长度
    #define BJ_STATE_LEN            4       // 状态长度
    #define BJ_POS_LEN              254     // 位置长度
    #define BJ_NOTE_LEN             1000    // 备注长度

    #define WB_SHORT_LEN			10		// 五包短ID长度
    #define WB_ID_LEN				20		// 五包ID长度
    #define WB_NAME_LEN				50		// 五包Name长度
    #define WB_OTHER_NAME_LEN       100		// 五包其他Name长度
    #define WB_NORMOL_LEN			200		// 五包常用长度
    #define WB_YY                   2000    // 五包未签原因
    #define LIN_LEN					250	    // 行长度
    // 数据库、数据库表宏定义
    #define SV_DB				("HD_STREETVIEW")				// 街景数据库
    #define TILE_DB				("HD_IMAGETILE")				// 全景切片数据库
    #define RI_DB				("HD_RANGEIMAGE")				// 深度图数据库
    #define SYMBOL_DB			("HD_SYMBOL")					// 符号数据库
    #define	IMAGE_DB			("iScan-Image")					// 全景图片数据库
    #define PARA_DB				("iScan-Para")					// 参数数据库
    #define BILLBOARD_DB		("HD_BILLBOARD")				// 广告牌标注数据库
    #define SLOPMONITOR_DB		("HD_SLOPMONITOR")     // 边坡监测数据库

    #define IMAGE_TABLE			("HD_IMAGEDATA")				// 影像表
    #define ISCANPARA_TABLE		("HD_ISCANPARA")				// 参数表
    #define ISCANCAMERA_TABLE	("HD_ISCANPANO")				// 全景参数表
    #define ISCANLIDAR_TABLE	("HD_ISCANLIDAR")				// 传感器参数表
    #define ISCAN_MZ_IMAGEPARA_TABLE ("HD_ISCAN_IMAGE_PARA")    // MZ单镜头内参参数表

    #define PROJECT_TABLE		("HD_STREETVIEW_PROJECT")		//工程表
    #define LINK_TABLE			("HD_STREETVIEW_LINK")			//邻接关系表
    #define CONFIG_TABLE		("HD_STREETVIEW_CONFIG")		// 配置表
    #define IMAGEINFO_TABLE		("HD_STREETVIEW_IMAGEINFO")		// 全景站点表
    #define HISTORYLINK_TABLE   ("HD_STREETVIEW_HISTORYLINK")   // 历史轨迹关联信息表 2014/12/22 lwm
    #define IMAGEINFO_TABLE_TMP	("HD_STREETVIEW_IMAGEINFO_TMP")	// 全景站点表
    #define TILEINFO_TABLE		("HD_STREETVIEW_TILEINFO")		// 切片表
    #define RIINFO_TABLE		("HD_STREETVIEW_RANGEIMAGEINFO")// 深度图表
    #define KEYPANOINFO_TABLE ("HD_STREETVIEW_KEYPANOINFO") // 关键站点信息表 [2015/06/16 luowenmin]
    #define KEYPANOIMAGE_TABLE ("HD_STREETVIEW_KEYPANOIMAGE") // 关键站点图片数据表  [2015/06/16 luowenmin] 

    #define FACADE_TABLE		("HD_STREETVIEW_FACADEINFO")	// 面片表
    #define IMAGEFACADE_TABLE	("HD_STREETVIEW_IMAGEFACADE")	// 面片测站表
    #define MARKER_TABLE		("HD_STREETVIEW_MARKERINFO")	// 标注表
    #define IMAGEMARKER_TABLE	("HD_STREETVIEW_IMAGEMARKER")	// 标注测站表

    #define SYMBOL_TABLE		("HD_STREETVIEW_SYMBOLINFO")	// 符号表
    #define BJATTRINFO_TABLE	("HD_STREETVIEW_BJATTRINFO")	// 部件	
    #define WBATTRINFO_TABLE	("HD_STREETVIEW_WBATTRINFO")	// 五包
    #define MARKERATT_TABLE		("HD_STREETVIEW_MARKERATT")		// 标注属性表
    #define ROADADDRESS_TABLE	("HD_STREETVIEW_ROADADDRESS")	// 路名路址表

    #define ROUTE_TABLE			("HD_STREETVIEW_ROUTE")			// 轨迹表
    #define NODE_TABLE			("HD_STREETVIEW_NODE")			// node表
    #define SEGMENT_TABLE		("HD_STREETVIEW_SEGMENT")		// 轨迹片段表

    #define CARINFO_TABLE		("HD_STREETVIEW_CARINFO")		// 采集车信息表
    #define HISTORYLINK_TABLE	("HD_STREETVIEW_HISTORYLINK")	// 历史连接关系信息表
    #define FALGCONFIG_TABLE    ("HD_STREETVIEW_FLAGCONFIG")	// 标志说明表

    #define BILLBOARD_TABLE		("HD_BILLBOARDDATA")			      // 广告牌标注数据表

    #define IMAGE_BLUR_DB		("HD_IMAGE_BLUR")					// 全景模糊数据库名称
    #define IB_IMAGE_TABLE     ("HD_IMAGE_INFO")					// 全景模糊站点信息
    #define IB_IMAGEBLUR_TABLE		("HD_IMAGE_BLUR")				// 全景模糊关联表
    #define IB_BLUR_TABLE		("HD_BLUR_INFO")					// 全景模糊信息
    #define IB_BLURRES_TABLE	("HD_BLUR_RESOURCE")				// 全景模糊资源信息表
    //#define SLOPMONITOR_TABLE		("HD_SLOPMONITORDATA")			// 边坡监测数据表

    #define MS_BOLB_TYPE		("image")						// MS SQL、ACCESS的二进制字段类型
    #define SQLITE_BOLB_TYPE	("blob")						// SQLite二进制字段类型
    #define MS_ACCESS_SLOPE_MONITOR_TABLE   ("HD_SLOPEMONITOR_DATAINFO") // 滑坡监测access数据库

    //MongoDB 数据库表名
	#define HD_ISCANINFO_TABLE				("HD_ISCANINFO_TABLE")				// 总体结构表
    #define HDI_POINT_TABLE					("HD_HDIPOINT")                     // 全景轨迹点数据表
    #define HDI_LINE_TABLE					("HD_HDILINE")                      // 全景轨迹线数据表
	#define POINT_CLOUD_HEADER_TABLE		("HD_POINTCLOUD_HEADER")			// 点云头文件数据表
    #define POINT_CLOUD_TABLE				("HD_POINTCLOUD")                   // 点云数据存放表
    #define POS_TABLE						("HD_POS")                          // 轨迹属性表
	#define HD_TEST                         ("HD_TEST")                         //测试表
}