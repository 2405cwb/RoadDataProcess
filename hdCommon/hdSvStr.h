/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdSvStr.h
相关文件	: 参考了hdSceneStr.h
文件实现功能：定义HDStreetView中英文版字符串,涉及各dll模块中的消息对话框或进度提示等字符串
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/02/11	1.0			杨峰		创建
</PRE>
******************************************************************************************************/

#pragma once

#include "..\hdCore\tinyxml.h"
#include "hdCommon.h"
#include <string>
using namespace std;

// 0表示中文,1表示英文
HDCOMMON_API int GetSvLanguageID();

// 通用
#define HDSV_IDS_LIC_INVALID (GetSvLanguageID()==0 ?"检测加密失败，请联系武汉海达数云公司进行软件加密解锁!" : "The Licence is Invalid！")
#define HDSV_IDS_GIVEN_LIC_INVALID (GetSvLanguageID()==0 ?"检测加密失败，请联系武汉海达数云公司进行\"%s\"软件的加密解锁!" : "The Licence of %s is Invalid！")
#define HDSV_IDS_FINISH (GetSvLanguageID()==0 ? "完成" : "Finish")
#define HDSV_IDS_PROMPT (GetSvLanguageID()==0 ? "提示" : "Prompt")
#define HDSV_IDS_ERROR (GetSvLanguageID()==0 ? "错误" : "Error")
#define HDSV_IDS_WARN (GetSvLanguageID()==0 ? "警告" : "Warning")
#define HDSV_IDS_PROCESS (GetSvLanguageID()==0 ? "处理" : "Processing")
#define HDSV_IDS_MEMORY_REQUSTFAILED (GetSvLanguageID()==0 ? "内存申请失败!" : "Requset of Memory Failed!")
#define HDSV_IDS_CREATE_FOLDER_FAILED (GetSvLanguageID()==0 ? "创建文件夹失败!" : "The Creation of the Folder Failed!")
#define HDSV_IDS_CREATE_FILE_FAILED (GetSvLanguageID()==0 ? "创建文件失败!" : "The Creation of File Failed!")
#define HDSV_IDS_SYS_LAN_CHINESE (GetSvLanguageID()==0 ? "中文（简体）" : "Chinese（Simplified）")
#define HDSV_IDS_SYS_LAN_ENGLISH (GetSvLanguageID()==0 ? "英语（美国）" : "English（US）")
#define HDSV_IDS_SOFTWARE_NAME (GetSvLanguageID()==0 ? "海达街景数据生产软件" : "Hi-target Street View Data Production Software")
#define HDSV_IDS_STR_NULL (GetSvLanguageID()==0 ? "字符为空" : "the Text is null")

// 主界面
#define HDSV_IDS_SYSTEM_SETTING_UILAN_PROMPT (GetSvLanguageID()==0 ?"必须重启软件才能生效！" : "You must restart the software to change the UI language！")
#define HDSV_IDS_CURRENT_DATABASE_SQLITE (GetSvLanguageID()==0 ? "当前数据库:SQLite" : "Current Database:SQLite")
#define HDSV_IDS_CURRENT_DATABASE_SQLSERVER (GetSvLanguageID()==0 ? "当前数据库:SQL Server" : "Current Database:SQL Server")
#define HDSV_IDS_CURRENT_DATABASE_ORACLE (GetSvLanguageID()==0 ? "当前数据库:Oracle" : "Current Database:Oracle")
#define HDSV_IDS_SET_CENTRALMERIDIAN_FAILED (GetSvLanguageID()==0 ? "中央经线设置失败，请输入0到180范围的值" : "It is failed to set central meridian,please enter the range pf values 0-180")

#define HDSV_IDS_EXPORT_SHP_DONE (GetSvLanguageID()==0 ? "导出Shp完成" : "Exporting Shp File is Done")
#define HDSV_IDS_EXPORT_SHP_FAILED (GetSvLanguageID()==0 ? "导出Shp失败" : "Exporting Shp File is Failed")
#define HDSV_IDS_IMPORT_POINTCLOUD (GetSvLanguageID()==0 ? "正在导入点云" : "Importing PointCloud")
#define HDSV_IDS_IMPORT_POINTCLOUD_FINISHED (GetSvLanguageID()==0 ? "导入点云完成" : "Importing PointCloud is Done")
#define HDSV_IDS_IMPORT_POINTCLOUD_FAILED (GetSvLanguageID()==0 ? "导入点云失败，内存申请失败" : "Importing Failed,Requset of Memory Failed!")
#define HDSV_IDS_ROUTE_COUNT (GetSvLanguageID()==0 ? "%s(共%d条轨迹)" : "%s (There are number of %d Scans)")
#define HDSV_IDS_SEL_POINTCLOUD (GetSvLanguageID()==0 ? "你选择了多个点云，只加载第一个选择的点云。" : "You choosed multi-PointCloud,Only Can Loading the First PointCloud")
#define HDSV_IDS_SEL_VALID_ROUTE (GetSvLanguageID()==0 ? "所选择轨迹为无效轨迹。" : "Be Selected Route is InValid Route")
#define HDSV_IDS_BLUR_WND (GetSvLanguageID()==0 ? "没有缩略图，无法进行模糊操作，请检查。" : "It Can not Bluring Picture Because There is No Abbreviative Images Existed, Please Check!")
#define HDSV_IDS_NO_FAC (GetSvLanguageID()==0 ? "该工程没有面片。" : "The Current Project has no Facade.")
#define HDSV_IDS_SAME_FILE (GetSvLanguageID()==0 ? "有重名文件，是否覆盖？" : "It Has Exist the Same File, Do You Want to Cover it?")
#define HDSV_IDS_SAME_FILE_NO (GetSvLanguageID()==0 ? "选择目录下有重名文件，请检查！" : "It Has Exist the Same File In Selected Floder, Please Check!")
#define HDSV_IDS_FAC_IMPORT (GetSvLanguageID()==0 ? "面片数据导入(支持fac、dxf文件)" : "Import Facade Data(Support fac、dxf File)")
#define HDSV_IDS_FAC_EXPORT (GetSvLanguageID()==0 ? "面片数据导出(支持fac、dxf文件)" : "Export Facade Data(Support fac、dxf File)")
#define HDSV_IDS_CONN_SUCCESSED (GetSvLanguageID()==0 ? "连接成功。" : "Connecting Successed")
#define HDSV_IDS_CONN_FAILED (GetSvLanguageID()==0 ? "连接失败。" : "Connecting Failed")
#define HDSV_IDS_FASTKEY_NO (GetSvLanguageID()==0 ? "序号" : "No")
#define HDSV_IDS_FASTKEY_KEY (GetSvLanguageID()==0 ? "快捷键" : "Shortcut Keys")
#define HDSV_IDS_FASTKEY_FUNC (GetSvLanguageID()==0 ? "对应功能" : "Function")
#define HDSV_IDS_FASTKEY_DESC (GetSvLanguageID()==0 ? "功能描述" : "Function Description")
#define HDSV_IDS_FASTKEY_NEW (GetSvLanguageID()==0 ? "新建" : "New")
#define HDSV_IDS_FASTKEY_NEW_INFO (GetSvLanguageID()==0 ? "创建新工程" : "New Project")
#define HDSV_IDS_FASTKEY_OPEN (GetSvLanguageID()==0 ? "打开" : "Open")
#define HDSV_IDS_FASTKEY_OPEN_INFO (GetSvLanguageID()==0 ? "打开现有工程" : "Open Cureent Project")
#define HDSV_IDS_FASTKEY_SAVE (GetSvLanguageID()==0 ? "保存" : "Save")
#define HDSV_IDS_FASTKEY_SAVE_INFO (GetSvLanguageID()==0 ? "保存活动工程" : "Save Active Project")
#define HDSV_IDS_FASTKEY_UNDO (GetSvLanguageID()==0 ? "撤销" : "Undo")
#define HDSV_IDS_FASTKEY_UNDO_INFO (GetSvLanguageID()==0 ? "撤销上一次操作" : "Undo the Last Operation")
#define HDSV_IDS_FASTKEY_REDO (GetSvLanguageID()==0 ? "重做" : "Redo")
#define HDSV_IDS_FASTKEY_REDO_INFO (GetSvLanguageID()==0 ? "重复上一次撤销的操作" : "Redo the Previously Undone Action")
#define HDSV_IDS_FASTKEY_BREAK (GetSvLanguageID()==0 ? "打断" : "Break")
#define HDSV_IDS_FASTKEY_BREAK_INFO (GetSvLanguageID()==0 ? "打断轨迹。" : "Break the Route")
#define HDSV_IDS_FASTKEY_ROAM (GetSvLanguageID()==0 ? "漫游" : "Roaming")
#define HDSV_IDS_FASTKEY_ROAM_INFO (GetSvLanguageID()==0 ? "拖动轨迹" : "Dragging the Route")
#define HDSV_IDS_FASTKEY_INVALID (GetSvLanguageID()==0 ? "无效" : "InValid")
#define HDSV_IDS_FASTKEY_RECTSEL (GetSvLanguageID()==0 ? "框选显示轨迹点" : "Rect Select Point")
#define HDSV_IDS_FASTKEY_CONN (GetSvLanguageID()==0 ? "连接" : "Connect Route")
#define HDSV_IDS_FASTKEY_ADDLINK (GetSvLanguageID()==0 ? "添加邻接关系" : "Add Link")
#define HDSV_IDS_FASTKEY_DELLINK (GetSvLanguageID()==0 ? "删除邻接关系" : "Del Link")


#define HDSV_IDS_FASTKEY_INVALID_INFO (GetSvLanguageID()==0 ? "使轨迹片段无效" : "Set the Route Fragment InValid")
#define HDSV_IDS_FASTKEY_RORATE (GetSvLanguageID()==0 ? "三维浏览" : "3D Roaming")
#define HDSV_IDS_FASTKEY_RORATE_INFO (GetSvLanguageID()==0 ? "三维浏览点云视图" : "3D Roaming In PointCloud View")
#define HDSV_IDS_FASTKEY_ADDFAC (GetSvLanguageID()==0 ? "添加面片" : "Add Facade")
#define HDSV_IDS_FASTKEY_ADDFAC_INFO (GetSvLanguageID()==0 ? "在三维视图中画面片" : "Drawing Facade in 3D View")
#define HDSV_IDS_FASTKEY_LOOKDOWN (GetSvLanguageID()==0 ? "俯视图" : "Top View")
#define HDSV_IDS_FASTKEY_LOOKDOWN_INFO (GetSvLanguageID()==0 ? "让三维视图以俯视图显示" : "LookDown in 3D View")
#define HDSV_IDS_FASTKEY_DELSFAC (GetSvLanguageID()==0 ? "删除单个面片" : "Delete Single Facade")
#define HDSV_IDS_FASTKEY_DELSFAC_INFO (GetSvLanguageID()==0 ? "在三维视图中删除单个面片" : "Delete Single Facade in 3D View")
#define HDSV_IDS_FASTKEY_DELMFAC (GetSvLanguageID()==0 ? "删除框选面片" : "Delete the Selected Facade")
#define HDSV_IDS_FASTKEY_DELMFAC_INFO (GetSvLanguageID()==0 ? "在三维视图中删除框选面片" : "Delete the Selected Facade in 3D View")
#define HDSV_IDS_FASTKEY_DELAFAC (GetSvLanguageID()==0 ? "删除可视范围内全部面片" : "Delete All Facade in the Current View")
#define HDSV_IDS_FASTKEY_DELAFAC_INFO (GetSvLanguageID()==0 ? "在三维视图中删除可视范围内全部面片" : "Delete All Facade in the Current 3D View")
#define HDSV_IDS_FASTKEY_NEXT (GetSvLanguageID()==0 ? "下一站" : "Next Station")
#define HDSV_IDS_FASTKEY_NEXT_INFO (GetSvLanguageID()==0 ? "显示下一站全景影像或者模糊影像" : "Show the Next Image or Blur Image")
#define HDSV_IDS_FASTKEY_LAST (GetSvLanguageID()==0 ? "上一站" : "Last Station")
#define HDSV_IDS_FASTKEY_LAST_INFO (GetSvLanguageID()==0 ? "显示上一站全景影像或者模糊影像" : "Show the Last Image or Blur Image")
#define HDSV_IDS_FASTKEY_TAB (GetSvLanguageID()==0 ? "切换" : "Switch")
#define HDSV_IDS_FASTKEY_TAB_INFO (GetSvLanguageID()==0 ? "切换到上一次使用的工具" : "Switch to the Last Tool")
// 修改快捷键描述 2015/05/27 luowenmin
#define HDSV_IDS_FASTKEY_FACADEVIEW_CTRL (GetSvLanguageID()==0 ? "切换画垂直面片" : "Switch to Draw vertical Facade")
#define HDSV_IDS_FASTKEY_FACADEVIEW_CTRL_INFO (GetSvLanguageID()==0 ? "面片视图中，切换画垂直面片" : "Switch to Draw vertical Facade in the Facade View")
//
#define HDSV_IDS_FASTKEY_CHOOSE (GetSvLanguageID()==0 ? "选择" : "Choose")

#define HDSV_IDS_FASTKEY_SHIFT (GetSvLanguageID()==0 ? "全景视图中，多选面片" : "Choose more Facades in the Image View")

// 增加新的快捷键描述 2015/03/10 lwm
#define HDSV_IDS_FASTKEY_PANO_FACADE_DELETE (GetSvLanguageID()==0 ? "删除面片" : "Delete facade")
#define HDSV_IDS_FASTKEY_EDIT_PANO_FACADE_DELETE (GetSvLanguageID()==0 ? "全景删除面片" : "Delete Facade in the Image View")
//
// 轨迹视图虚拟面片划线确定 2015/05/27 luowenmin
#define HDSV_IDS_FASTKEY_ROUTEVIEW_VIRTUAL_FACADE_LINE_CONFIRM (GetSvLanguageID()==0 ? "虚拟面片线确定" : "Virtual-facade line confirm")
#define HDSV_IDS_FASTKEY_EDIT_ROUTEVIEW_VIRTUAL_FACADE_LINE_CONFIRM (GetSvLanguageID()==0 ? "轨迹视图虚拟面片划线确定" : "Virtual-facade line confirm in the Route View")
//
#define HDSV_IDS_FASTKEY_MERGE (GetSvLanguageID()==0 ? "合并" : "Merge")
#define HDSV_IDS_FASTKEY_EDIT (GetSvLanguageID()==0 ? "全景合并面片" : "Merge Facade in the Image View")
#define HDSV_IDS_PRJNAME_ERROR (GetSvLanguageID()==0 ? "工程名超过长度(40字符)" : "Project Name Exceed Normal Length(40 Chars)")
#define HDSV_IDS_SAVE_WORKSPACE (GetSvLanguageID()==0 ? "正在保存……" : "Is Auto Save……")
#define HDSV_IDS_SAVE_WORKSPACE_SUCCESED (GetSvLanguageID()==0 ? "保存成功" : "Auto Save Succesed")
#define HDSV_IDS_TRANSLATE_ED2_SUCCESED (GetSvLanguageID()==0 ? "保存成功" : "Auto Save Succesed")
#define HDSV_IDS_TRANSLATE_ED2_FAILED (GetSvLanguageID()==0 ? "转换失败" : "Auto Translate Failed")
#define HDSV_IDS_AUTO_CHECK_ROUTE (GetSvLanguageID()==0 ? "检测轨迹完整性" : "Checking the route……")
#define HDSV_IDS_AUTO_CHECK_DONE (GetSvLanguageID()==0 ? "检查结束" : "Check end")
#define HDSV_IDS_CONNECT_INTERNET_FAIL (GetSvLanguageID()==0 ? "联网失败" : "Connect Internet Failed")
#define HDSV_IDS_IMPORT_SUCCESED (GetSvLanguageID()==0 ? "导入成功" : "Import Succesed")
#define HDSV_IDS_IMPORT_FAILD (GetSvLanguageID()==0 ? "导入失败" : "Import Faild")
#define HDSV_IDS_IS_UNVALID (GetSvLanguageID()==0 ? "内容不符合规范" : "the Content is not valid")
// 2015/05/25 luowenmin
#define HDSV_IDS_CARNUM_IS_UNVALID (GetSvLanguageID()==0 ? "车牌号'%s'长度不为5" : "the length of car num '%s' is not equal to 5")
//

// 轨迹编辑
#define HDSV_IDS_ROUTE_SHOW_PT (GetSvLanguageID()==0 ? "轨迹数大于10,确定显示轨迹点？" : "Route Number larger than 10,?Is sure show route point?")
#define HDSV_IDS_STR_TOO_LONG (GetSvLanguageID()==0 ? "字符长度过长" : "the str is too long")
#define HDSV_IDS_AUTO_GETROUTENAME (GetSvLanguageID()==0 ? "在线获取路名" : "Getting Road Name from Internet……")
#define HDSV_IDS_AUTO_DONE (GetSvLanguageID()==0 ? "操作完成" : "Get Done")
#define HDSV_IDS_GET_ROADNAME_FAILED (GetSvLanguageID()==0 ? "获取失败" : "Get roadname failed")
#define	HDSV_IDS_PROCESS_LOAD (GetLanguageID()==0 ?"正在加载 " : "Loading ")
#define	HDSV_IDS_PROCESS_CREATE (GetLanguageID()==0 ?"正在创建 " : "Creating ") 
#define HDSV_IDS_ADD_OBJECT (GetLanguageID() == 0 ? "正在添加点云场景对象 " : "The Scenenode of Pointcloud is Adding ")
#define	HDSV_IDS_ROUTE_DATA_INFO (GetLanguageID()==0 ?" 轨迹数据信息" : " the information of route")
#define	HDSV_IDS_FILE_DATA_INFO (GetLanguageID()==0 ?" 文件数据信息" : " the information of the file") 
// 地图
#define HDSV_IDS_CURRENT_MAP_BAIDU (GetSvLanguageID()==0 ? "当前为百度地图" : "Current Map is BAIDU")
#define HDSV_IDS_CURRENT_MAP_GOOGLE (GetSvLanguageID()==0 ? "当前为谷歌地图" : "Current Map is GOOGLE")
#define HDSV_IDS_MAP_NULL (GetSvLanguageID()==0 ? "没有地图数据" : "Map is Null")
#define HDSV_IDS_HAD_MAP (GetSvLanguageID()==0 ? "已存在地图,请先移除" : "Had Map,Pelease remove it")
#define HDSV_IDS_REMOVE_NULL (GetSvLanguageID()==0 ? "确定移除地图?" : "Is Reomve Map?")

// 深度图
#define HDSV_IDS_DISTIMG_CREATING (GetSvLanguageID()==0 ?"正在生成深度图..." : "Creating the Range Image")
#define HDSV_IDS_DISTIMG_CREATEFAILED (GetSvLanguageID()==0 ?"深度图生成失败!" : "Create Range Image Failed")
#define HDSV_IDS_DISTIMG_CALC_ANGLE (GetSvLanguageID()==0 ?"正在计算角度并划分网格..." : "Calculating the Angle and Plot Gird")

// hdImageProc && hdImageBlur
#define HDSV_IDS_IMAGEPROC_DETECT_FACE_CAR (GetSvLanguageID()==0 ?"正在自动检测人脸、车牌" : "Auto Detecting Face and CarNo")
#define HDSV_IDS_IMAGEPROC_SCAN_STATION	(GetSvLanguageID()==0 ?"正在检测轨迹%s的第%d个站点" : "Detecting Scans %s the Number of %d ImageStation")
#define HDSV_IDS_IMAGEPROC_FINISHED	(GetSvLanguageID()==0 ?"全景自动模糊完成" : "Image Blur is Finished")
#define HDSV_IDS_IMAGEPROC_SEQUENCE (GetSvLanguageID()==0 ?"请在全景切片之前进行全景模糊操作！" : "Image Tile is After Image Blur,Please Check is Running PanoTile.exe")
#define HDSV_IDS_IMAGEPROC_PROCESSING (GetSvLanguageID()==0 ?"正在执行：%d" : "Processing ：%d")
#define HDSV_IDS_IMAGEPROC_NO_SCAN_CHOOSED (GetSvLanguageID()==0 ?"没有选择轨迹。" : "No Scan be Choosed。")
#define HDSV_IDS_IMAGEPROC_INPUTFILE_INVALID (GetSvLanguageID()==0 ?"输入文件无效，请检查!" : "Input File is Invalid,Please Check!")
#define HDSV_IDS_LOAD_MAP_ED2 (GetSvLanguageID()==0 ?"加载地图失败!" : "Load ed2 Failed!")

// hdSvWorkspace
#define HDSV_IDS_WS_NEW_PROJECT (GetSvLanguageID()==0 ?"正在新建工程" : "Creating Project")
#define HDSV_IDS_WS_NEW_PROJECT_FAILED (GetSvLanguageID()==0 ?"新建工程失败" : "Create Project Failed")
#define HDSV_IDS_WS_NEW_PROJECT_SUCCESSED (GetSvLanguageID()==0 ?"新建工程成功" : "Create Project Successed")
#define HDSV_IDS_WS_OPEN_PROJECT (GetSvLanguageID()==0 ?"正在打开工程" : "Opening Project")
#define HDSV_IDS_WS_OPEN_DATABASE_FAILED (GetSvLanguageID()==0 ?"数据库打开失败，请检查是否存在数据库！" : "Open DataBase Failed,Please Check DataBase is Existed!")
#define HDSV_IDS_WS_OPEN_PROJECT_SUCCESSED (GetSvLanguageID()==0 ?"打开工程成功" : "Open Project Successed")
#define HDSV_IDS_WS_OPEN_PROJECT_FAILED (GetSvLanguageID()==0 ?"打开工程失败" : "Open Project Failed")
#define HDSV_IDS_WS_OPEN_HSV_FAILED (GetSvLanguageID()==0 ?"读取hsv失败" : "Open hsv Failed")
#define HDSV_IDS_WS_OPEN_ROUTE_FAILED (GetSvLanguageID()==0 ?"读取轨迹失败" : "Get Route Failed")
#define HDSV_IDS_WS_OPEN_HSV_VERSION1 (GetSvLanguageID()==0 ?"当前hsv为1.0版本，请用升级工具进行升级" : "the hsv version is 1.0, Please use UpdateTool update it")
#define HDSV_IDS_WS_OPEN_HSV_VERSION2 (GetSvLanguageID()==0 ?"当前hsv为1.2版本，请用升级工具进行升级" : "the hsv version is 1.2, Please use UpdateTool update it")
// 2015/03/27 轨迹批量导入 lwm
#define HDSV_IDS_WS_IMPORT_ROUTES_FINISHED (GetSvLanguageID()==0 ?"轨迹导入完成" : "Routes importing")


// hdDBTransTool
#define HDSV_IDS_DBTRANS_CONFIG (GetSvLanguageID()==0 ?"正在转换Config表..." : "Converting Config Table...")
#define HDSV_IDS_DBTRANS_IMAGEINFO (GetSvLanguageID()==0 ?"正在转换ImageInfo表..." : "Converting ImageInfo Table...")
#define HDSV_IDS_DBTRANS_FACADEINFO (GetSvLanguageID()==0 ?"正在转换FacadeInfo表..." : "Converting FacadeInfo Table...")
#define HDSV_IDS_DBTRANS_ROUTE (GetSvLanguageID()==0 ?"正在转换Route表..." : "Converting Route Table...")
#define HDSV_IDS_DBTRANS_NODE (GetSvLanguageID()==0 ?"正在转换Node表..." : "Converting Node Table...")
#define HDSV_IDS_DBTRANS_SEGMENT (GetSvLanguageID()==0 ?"正在转换Segment表..." : "Converting Segment Table...")
#define HDSV_IDS_DBTRANS_TILEINFO (GetSvLanguageID()==0 ?"正在读取TileInfo表..." : "Converting TileInfo Table...")
#define HDSV_IDS_DBTRANS_RANGEIMAGE (GetSvLanguageID()==0 ?"正在转换RangeImage表..." : "Converting RangeImage Table...")
#define HDSV_IDS_DBTRANS_MARKER (GetSvLanguageID()==0 ?"正在转换Marker表..." : "Converting Marker Table...")
#define HDSV_IDS_DBTRANS_SYMBOL (GetSvLanguageID()==0 ?"正在转换Symbol表..." : "Converting Symbol Table...")
#define HDSV_IDS_DBTRANS_ATTRCONFIG (GetSvLanguageID()==0 ?"正在转换AttrConfig表..." : "Converting AttrConfig Table...")
#define HDSV_IDS_DBTRANS_FLAGCONFIG (GetSvLanguageID()==0 ?"正在转换FlagConfig表..." : "Converting FlagConfig Table...")
#define HDSV_IDS_DBTRANS_IMAGEMARKER (GetSvLanguageID()==0 ?"正在转换ImageMarker表..." : "Converting ImageMarker Table...")
#define HDSV_IDS_DBTRANS_MILEAGE (GetSvLanguageID()==0 ?"正在转换Mileage表..." : "Converting Mileage Table...")
#define HDSV_IDS_DBTRANS_RESOURCE (GetSvLanguageID()==0 ?"正在转换Resource表..." : "Converting Resource Table...")
#define HDSV_IDS_DBTRANS_KEYPANOIMAGE (GetSvLanguageID()==0 ?"正在转换KeyPanoImage表..." : "Converting KeyPanoImage Table...")
#define HDSV_IDS_DBTRANS_KEYPANOINFO (GetSvLanguageID()==0 ?"正在转换KeyPanoInfo表..." : "Converting KeyPanoInfo Table...")

// 增加数据上创工具新表的提示信息 2015/01/22 lwm 
#define HDSV_IDS_DBTRANS_IMAGEFACADE (GetSvLanguageID()==0 ?"正在转换ImageFacade表..." : "Converting ImageFacade Table...")
#define HDSV_IDS_DBTRANS_CARINFO (GetSvLanguageID()==0 ?"正在转换CarInfo表..." : "Converting CarInfo Table...")
#define HDSV_IDS_DBTRANS_HISTORYLINK (GetSvLanguageID()==0 ?"正在转换HistoryLink表..." : "Converting HistoryLink Table...")
//

#define HDSV_IDS_DBTRANS_HSV_UNSELECTED (GetSvLanguageID()==0 ?"未选择hsv文件！" : "UnSelected Any HSV File!")
#define HDSV_IDS_DBTRANS_HSV_DBSQLITE (GetSvLanguageID()==0 ?"选择hsv不是SQLite数据库！" : "hsv is not SQLite DB.")
#define HDSV_IDS_DBTRANS_OPEN_HSV_FAILED (GetSvLanguageID()==0 ?"打开hsv失败。" : "Open HSV File Failed。")
#define HDSV_IDS_DBTRANS_SVRIP_EMPTY (GetSvLanguageID()==0 ?"未填写服务器IP！" : "SQL Server DataBase Server IP is Empty!")
#define HDSV_IDS_DBTRANS_DBNAME_EMPTY (GetSvLanguageID()==0 ?"未填写数据库名称！" : "SQL Server DataBase DB Name is Empty!")
#define HDSV_IDS_DBTRANS_USER_EMPTY (GetSvLanguageID()==0 ?"未填写数据库用户名！" : "SQL Server DataBase User Name is Empty!")
#define HDSV_IDS_DBTRANS_PWD_EMPTY (GetSvLanguageID()==0 ?"未填写数据库登陆密码！" : "SQL Server DataBase User Password is Empty!")
#define HDSV_IDS_DBTRANS_CONN_FAILED (GetSvLanguageID()==0 ?"数据库连接不成功!" : "Connecting SQL Server DataBase Failed!")
#define HDSV_IDS_DBTRANS_CONN_SUCCESSED (GetSvLanguageID()==0 ?"数据库连接测试成功！" : "Connecting SQL Server DataBase Successed!")
#define HDSV_IDS_DBTRANS_NAME (GetSvLanguageID()==0 ?"DB到SQL Server工具" : "DB To SQL Server Tools")

// hdAppExportTool
#define HDSV_IDS_EXPORT_PROCESS_SCAN (GetSvLanguageID()==0 ?"正在处理第%d条轨迹" : "Processing the Number of %d ScanRoute")
#define HDSV_IDS_EXPORT_PROCESS_DONE (GetSvLanguageID()==0 ?"处理完毕！" : "Processing Done！")
#define HDSV_IDS_EXPORT_SAME_ITEM (GetSvLanguageID()==0 ?"已存在相同的项！" : "The Same Item is Existed!")
#define HDSV_IDS_EXPORT_FULL_INFO (GetSvLanguageID()==0 ?"设备参数信息不全！" : "Please Check the Device Parameter Information")
#define HDSV_IDS_EXPORT_SAVE_PATH_EMPTY (GetSvLanguageID()==0 ?"保存路径为空" : "The Saving Path is Empty")
#define HDSV_IDS_EXPORT_SAVE_LIST_EMPTY (GetSvLanguageID()==0 ?"工程列表不能为空" : "The Project List is Empty")
#define HDSV_IDS_EXPORT_HIGHTERROE_INFO (GetSvLanguageID()==0 ?"设备参数错误！" : "Please Check the Device Parameter Information")
#define HDSV_IDS_EXPORT_CARE_ERROR (GetSvLanguageID()==0 ?"车牌不合法！" : "Please Check Car Number")
#define HDSV_IDS_EXPORT_FINISH (GetSvLanguageID()==0 ?"完成" : "Finish")

// hdDataConvert
#define HDSV_IDS_DC_BROWSE_PATH (GetSvLanguageID()==0 ?"请选择保存转换后iScan数据目录" : "Please Choose the Save Path")
#define HDSV_IDS_DC_CENTERB_EMPTY (GetSvLanguageID()==0 ?"请填写中央子午经线！" : "Central Meridian Longitude is Empty, Please Check!")
#define HDSV_IDS_DC_CENTERB_NOT_NUM (GetSvLanguageID()==0 ?"中央子午经线不是数字，请检查！" : "Central Meridian Longitude is Not Number, Please Check!")
#define HDSV_IDS_DC_FINISHED (GetSvLanguageID()==0 ?"数据转换结束" : "Data Convert Finished")
#define HDSV_IDS_DC_POINTCLOUD_FILE_ERROR (GetSvLanguageID()==0 ?"点云文件错误! " : "PointCloud File Error")
#define HDSV_IDS_DC_PANOINDEX_FILE_ERROR (GetSvLanguageID()==0 ?"全景索引文件错误! " : "PanoIndex File Error")
#define HDSV_IDS_DC_SAVE_PATH_EMPTY (GetSvLanguageID()==0 ?"保存工作区路径为空。" : "The Saving Path is Empty")
#define HDSV_IDS_DC_NO_DATA_ADD (GetSvLanguageID()==0 ?"没有添加数据。 " : "No Data be Added")
#define HDSV_IDS_DC_POINTCLOUD_FILE_EMPTY (GetSvLanguageID()==0 ?"有点云数据文件路径为空 " : "There are PointCloud File Path is Empty")
#define HDSV_IDS_DC_PANOINDEX_FILE_EMPTY (GetSvLanguageID()==0 ?"有全景影像索引文件路径为空 " : "There are PanoIndex File Path is Empty")
#define HDSV_IDS_DC_CONVERTING_POINTCLOUD (GetSvLanguageID()==0 ?"正在转换点云... " : "Converting PointCloud...")
#define HDSV_IDS_DC_CONVERT_POINTCLOUD_FINISHED (GetSvLanguageID()==0 ?"转换点云完成 " : "Convert PointCloud Finished")
#define HDSV_IDS_DC_CONVERTING_PANO (GetSvLanguageID()==0 ?"正在转换全景... " : "Converting Pano...")
#define HDSV_IDS_DC_CONVERT_PANO_FINISHED (GetSvLanguageID()==0 ?"转换全景完成 " : "Convert Pano Finished")
#define HDSV_IDS_DC_POINTCLOUD_FILENAME_EMPTY (GetSvLanguageID()==0 ?"hls文件名为空，请检查! " : "HLS FileName is Empty,Please Check")
#define HDSV_IDS_DC_PANOINDEX_FILENAME_EMPTY (GetSvLanguageID()==0 ?"全景索引文件名为空，请检查! " : "PanoIndex FileName is Empty,Please Check")
#define HDSV_IDS_DC_INDEX_COUNT (GetSvLanguageID()==0 ?"索引个数 " : "Index Count")
#define HDSV_IDS_DC_PANO_COUNT (GetSvLanguageID()==0 ?"  与影像文件个数 " : "With PanoImage Count")
#define HDSV_IDS_DC_NOT_MATCH (GetSvLanguageID()==0 ?"  不匹配。 " : "is Not Match")
#define HDSV_IDS_DC_LIST_NO (GetSvLanguageID()==0 ?"序号 " : "No")
#define HDSV_IDS_DC_LIST_POINTCLOUD_PATH (GetSvLanguageID()==0 ?"点云文件路径 " : "PointCloud File Path")
#define HDSV_IDS_DC_LIST_PANOINDEX_PATH (GetSvLanguageID()==0 ?"全景索引文件路径 " : "PanoIndex File Path")

// hdMergeTool
#define HDSV_IDS_MERGE_CONFIG (GetSvLanguageID()==0 ?"正在合并Config表..." : "Mergeing Config Table...")
#define HDSV_IDS_MERGE_IMAGEINFO (GetSvLanguageID()==0 ?"正在合并ImageInfo表..." : "Mergeing ImageInfo Table...")
#define HDSV_IDS_MERGE_FACADEINFO (GetSvLanguageID()==0 ?"正在合并FacadeInfo表..." : "Mergeing FacadeInfo Table...")
#define HDSV_IDS_MERGE_ROUTE (GetSvLanguageID()==0 ?"正在合并Route表..." : "Mergeing Route Table...")
#define HDSV_IDS_MERGE_NODE (GetSvLanguageID()==0 ?"正在合并Node表..." : "Mergeing Node Table...")
#define HDSV_IDS_MERGE_SEGMENT (GetSvLanguageID()==0 ?"正在合并Segment表..." : "Mergeing Segment Table...")
#define HDSV_IDS_MERGE_TILEINFO (GetSvLanguageID()==0 ?"正在合并TileInfo表..." : "Mergeing TileInfo Table...")
#define HDSV_IDS_MERGE_RANGEIMAGE (GetSvLanguageID()==0 ?"正在合并RangeImage表..." : "Mergeing RangeImage Table...")
#define HDSV_IDS_MERGE_NO_SCAN_DATA (GetSvLanguageID()==0 ?"该工程中没有轨迹数据。" : "There is No Scan Data in Current Project。")
#define HDSV_IDS_MERGE_SAME_SCAN (GetSvLanguageID()==0 ?"hsv中包含相同轨迹。" : "The Same Scan is Existed in Current Project。")
#define HDSV_IDS_MERGE_PRJNAME_EMPTY (GetSvLanguageID()==0 ?"请填写工程名！" : "The Project Name is Empty,Please Check!")
#define HDSV_IDS_MERGE_OUTPATH_EMPTY (GetSvLanguageID()==0 ?"请选择保存目录！" : "The Save Path is Empty,Please Check!")
#define HDSV_IDS_MERGE_NO_HSV_ADD (GetSvLanguageID()==0 ?"至少需要添加2个hsv文件！" : "At Least Two Hsv File Must be Added!")
#define HDSV_IDS_MERGE_DB_UNFULL (GetSvLanguageID()==0 ?"数据库信息不完整！" : "SQL Server Information is Not Enough!")
#define HDSV_IDS_MERGE_CONN_FAILED (GetSvLanguageID()==0 ?"数据库连接不成功!" : "Connecting SQL Server DataBase Failed!")
#define HDSV_IDS_MERGE_CONN_SUCCESSED (GetSvLanguageID()==0 ?"数据库连接测试成功！" : "Connecting SQL Server DataBase Successed!")

// hdPanoTile
#define HDSV_IDS_TILE_LOAD_PIC_FAILED (GetSvLanguageID()==0 ?"加载图片失败！" : "Load Image Failed!")
#define HDSV_IDS_TILE_NO_24BIT (GetSvLanguageID()==0 ?"不是24位真彩色图片！" : "The Image is Not 24 Bit!")
#define HDSV_IDS_TILE_PIC_SIZE_ERROR (GetSvLanguageID()==0 ?"图像大小输入错误！" : "Image Size Error")
#define HDSV_IDS_TILE_LOG (GetSvLanguageID()==0 ?"轨迹%s的影像%s生成切片失败\n" : "Image %s in Scan %s Failed")
#define HDSV_IDS_TILE_DB_CONN_FAILED (GetSvLanguageID()==0 ?"连接数据库失败。" : "Connecting DataBase Failed。")
#define HDSV_IDS_TILE_CONN_SUCCESSED (GetSvLanguageID()==0 ?"数据库连接测试成功！" : "Connecting DataBase Successed!")
#define HDSV_IDS_TILE_PROCESSING (GetSvLanguageID()==0 ?"正在处理第%d/%d张图片..." : "Processing %d/%d Image...")
#define HDSV_IDS_TILE_CHECKING (GetSvLanguageID()==0 ?"正在检查第%d/%d张图片..." : "Checking %d/%d Image...")
#define HDSV_IDS_TILE_ACCOMPLISH (GetSvLanguageID()==0 ?"处理完毕" : "Processing Done")
#define HDSV_IDS_TILE_INPUTFILE_INVALID (GetSvLanguageID()==0 ?"输入文件无效，请检查!" : "Input File is Invalid,Please Check!")
#define HDSV_IDS_TILE_CHOOSE_INPUT (GetSvLanguageID()==0 ?"请选择输入目录！" : "Please Choose Input Path!")
#define HDSV_IDS_TILE_CHOOSE_OUTPUT (GetSvLanguageID()==0 ?"请选择输出目录！" : "Please Choose Output Path!")
#define HDSV_IDS_TILE_CHOOSE_FAILED (GetSvLanguageID()==0 ?"选择文件不符标准。" : "Choose File is unvalid")
#define HDSV_IDS_TILE_QUALITYPERCENT_ZERO (GetSvLanguageID()==0 ?"图片质量为0！" : "The Image	Qualitypercent is Zero!")
#define HDSV_IDS_TILE_LEVEL_EMPTY (GetSvLanguageID()==0 ?"未选择任何级别！" : "The Level has Not Chosen!")
#define HDSV_IDS_TILE_STATION_EMPTY (GetSvLanguageID()==0 ?"未选择任何站点！" : "The station has Not Chosen!")
#define HDSV_IDS_TILE_OUTPUT_FOLDER (GetSvLanguageID()==0 ?"输出文件夹" : "Output Folder")
#define HDSV_IDS_TILE_SAVEDB (GetSvLanguageID()==0 ?"保存数据库" : "Save Database")
#define HDSV_IDS_TILE_SAVEPATH (GetSvLanguageID()==0 ?"请选择要输出的目录位置:" : "Please Choose the Output Path")
#define HDSV_IDS_TILE_CURRENT_LEVEL (GetSvLanguageID()==0 ?"%d级 " : "%d level ")
#define HDSV_IDS_TILE_CURRENT_XLEVEL (GetSvLanguageID()==0 ?"X级 " : "X level ")
#define HDSV_IDS_TILE_FAST (GetSvLanguageID()==0 ?"点击频率过快，先歇歇吧！" : "Click Frequency too Fast, have a rest")
#define HDSV_IDS_TILE_DB_UNFULL (GetSvLanguageID()==0 ?"SQL Server数据库信息不完整！" : "SQL Server Information is Not Enough!")
#define HDSV_IDS_TILE_SCAN_BEGIN (GetSvLanguageID()==0 ?"=====轨迹 %s 切片情况=====\n" : "=====The PanoTile Actuality of Scan %s =====\n")
#define HDSV_IDS_TILE_STATION_DONE (GetSvLanguageID()==0 ?"切片已完成!" : "PanoTile has Done!")
#define HDSV_IDS_TILE_STATION_LEFT (GetSvLanguageID()==0 ?"切片未完成!" : "PanoTile has UnDone!")
#define HDSV_IDS_TILE_SCAN_END (GetSvLanguageID()==0 ?"=====轨迹 %s 共%d个站点，已完成%d个，未完成%d个=====\r\n" : "=====Scans %s has %d Station, %d has Done, %d has UnDone=====\r\n")
#define HDSV_IDS_TILE_FILE_SAVEPATH (GetSvLanguageID()==0 ?"文件已保存到%s" : "File is Saved in %s")
#define HDSV_IDS_TILE_TITLE (GetSvLanguageID()==0 ?"海达全景切片生成工具" : "HD PanoTile Tool")
#define HDSV_IDS_TILE_LIST_LEVEL (GetSvLanguageID()==0 ?"级别" : "Level")
#define HDSV_IDS_TILE_MORE_HSV (GetSvLanguageID()==0 ?"选择了多个工程文件,请重新选择" : "Select more than 1 hsv file, select again")
#define HDSV_IDS_TILE_DATAS_LOADING (GetSvLanguageID()==0 ?"正在加载数据..." : "Loading datas...")
#define HDSV_IDS_TILE_DATAS_PATH_INVALID (GetSvLanguageID()==0 ?"所选路径:'%s'无效,请检查!":"The path '%s' is invalid, please check it!")


// hdRICreator
#define HDSV_IDS_RI_PROCESSING (GetSvLanguageID()==0 ?"正在处理第%d/%d个站点..." : "Processing %d/%d Station...")
#define HDSV_IDS_RI_LOG (GetSvLanguageID()==0 ?"轨迹%s   第%s 站点 深度图生成失败\n" : "Image %s in Scan %s Failed\n")
#define HDSV_IDS_RI_TITLE (GetSvLanguageID()==0 ?"海达深度图生成工具" : "HD RangeImage Creator Tool")
#define HDSV_IDS_HLZRI_TITLE (GetSvLanguageID()==0 ?"海达深度图生成工具(大数据版)" : "HD RangeImage Creator Tool(Big Data Edition)")
#define HDSV_IDS_RI_DB_CONN_FAILED (GetSvLanguageID()==0 ?"连接数据库失败。" : "Connecting DataBase Failed。")
#define HDSV_IDS_RI_CONN_SUCCESSED (GetSvLanguageID()==0 ?"数据库连接测试成功！" : "Connecting DataBase Successed!")
#define HDSV_IDS_RI_SVRIP_EMPTY (GetSvLanguageID()==0 ?"未填写服务器IP！" : "SQL Server DataBase Server IP is Empty!")
#define HDSV_IDS_RI_DBNAME_EMPTY (GetSvLanguageID()==0 ?"未填写数据库名称！" : "SQL Server DataBase DB Name is Empty!")
#define HDSV_IDS_RI_USER_EMPTY (GetSvLanguageID()==0 ?"未填写数据库用户名！" : "SQL Server DataBase User Name is Empty!")
#define HDSV_IDS_RI_PWD_EMPTY (GetSvLanguageID()==0 ?"未填写数据库登陆密码！" : "SQL Server DataBase User Password is Empty!")
#define HDSV_IDS_RI_HSV_EMPTY (GetSvLanguageID()==0 ?"请选择hsv文件！" : "Please Choose HSV File!")
#define HDSV_IDS_RI_SAVEPATH_EMPTY (GetSvLanguageID()==0 ?"请选择图片保存目录！" : "Please Choose Image Saving Path!")
#define HDSV_IDS_RI_SCANS_EMPTY (GetSvLanguageID()==0 ?"请选择扫描头！" : "Please Choose Scanners!")
#define HDSV_IDS_RI_NO_STATION (GetSvLanguageID()==0 ?"还未选中任何一行测站，请检查！" : "UnSelected any Station!")
#define HDSV_IDS_RI_PNG_PATH_EMPTY (GetSvLanguageID()==0 ?"请选择保存png文件目录" : "Please Choose Png Saving Path")

// 数据库
#define HDSV_IDS_RI_OPEN_DB_CUCCEED (GetSvLanguageID()==0 ?"数据库打开成功" : "Open DataBace Succeed")
#define HDSV_IDS_RI_OPEN_DB_FAILED (GetSvLanguageID()==0 ?"数据库打开失败" : "Open DataBace Failed")

#define HDSV_IDS_DB_MANEGER_SEL_NON (GetSvLanguageID()==0 ?"没有选中轨迹" : "No selected routes")
#define HDSV_IDS_DB_MANEGER_DEL_SEL (GetSvLanguageID()==0 ?"是否删除选中轨迹" : "Are you sure delete selected routes")
#define HDSV_IDS_DB_MANEGER_DEL_COMPLATED (GetSvLanguageID()==0 ?"删除成功" : "Deleted")
#define HDSV_IDS_NONE_FACADE (GetSvLanguageID()==0 ? "没有面片导出？" : "None Facade")


// 数据更新工具
#define HDSV_IDS_DU_TITLE (GetSvLanguageID()==0 ?"数据升级工具" : "Data Update Tool")
#define HDSV_IDS_SQL_INVALID (GetSvLanguageID()==0 ?"不支持的数据库类型" : "Not support DB type")
#define HDSV_IDS_DU_FAILED (GetSvLanguageID()==0 ?"升级失败" : "Udpate Failed")
#define HDSV_IDS_DU_SUCCESSED (GetSvLanguageID()==0 ?"升级完成" : "Udpate Successed")


// 全景模糊工具
#define HDSV_IDS_BLURTOOL_CONVERT_REGDB (GetSvLanguageID()==0 ?"是否转换缩略图的模糊成果？":"Is sure Convert?")
#define HDSV_IDS_BLURTOOL_OPENFAILD (GetSvLanguageID()==0 ?"打开失败！" : "Open Falied")
#define HDSV_IDS_BLURTOOL_NAME (GetSvLanguageID()==0 ?"海达全景模糊工具" : "hdImageBlurTool")
#define HDSV_IDS_BLURTOOL_DATAEMPTY (GetSvLanguageID()==0 ?"找不到数据" : "the Data is empty")
#define HDSV_IDS_BLURTOOL_OPENSUCCESS (GetSvLanguageID()==0 ?"打开成功" : "Open Successed")
#define HDSV_IDS_BLURTOOL_OPENDATA (GetSvLanguageID()==0 ?"正在打开数据" : "Open Data……")
#define HDSV_IDS_BLURTOOL_CONVERTING (GetSvLanguageID()==0 ?"正在转换" : "Convert Data……")
#define HDSV_IDS_BLURTOOL_CONVEROK (GetSvLanguageID()==0 ?"转换成功" : "Convert OK")
#define HDSV_IDS_BLURTOOL_CONVERFAILED (GetSvLanguageID()==0 ?"转换失败" : "Convert Failed")
#define HDSV_IDS_BLURTOOL_CLEARDATA (GetSvLanguageID()==0 ?"清除数据" : "Clear Data")
#define HDSV_IDS_BLURTOOL_LOADSUCCESS (GetSvLanguageID()==0 ?"图像打开成功" : "Load Image Successed")
#define HDSV_IDS_BLURTOOL_NOTFINDPS (GetSvLanguageID()==0 ?"请检查是否安装PhotoShop！" : "Check has install PhotoShop")
#define HDSV_IDS_BLURTOOL_OPRUNVALID (GetSvLanguageID()==0 ?"无效操作！请先选中显示模糊框或显示动态模糊效果" : "Unvalid Opreation")
#define HDSV_IDS_BLURTOOL_OUTPUTIMGSUCCESS (GetSvLanguageID()==0 ?"导出影像成功" : "Output Image OK")
#define HDSV_IDS_BLURTOOL_LESSGAUSSIAN (GetSvLanguageID()==0 ?"模糊区域过小,请扩大区域或减小模糊半径!" : "the Blur region is less gaussian radius, please modify it")
