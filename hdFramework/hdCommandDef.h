#ifndef _HD_COMMAND_DEF_H_
#define _HD_COMMAND_DEF_H_

/************************************************************************/
/* 用最后一个十六进制值标志此命令属于哪个视图：
1，表示3D视图可用，
2，表示平面视图可用，
4，表示快速视图可用，  
3，表示3D和平面均可用,
5，表示3D和快速视图均可用,
6，表示平面视图和快速视图可用
7，表示任何3D\平面\快速视图可用
8，表示俯视图工具
9，表示剖面视图工具*/
/************************************************************************/

#define COMMAND_3D						0x00000100
#define COMMAND_PLANAR					0x00000200
#define COMMAND_QUICK					0x00000400

/************************************************************************/
/* 仅3D视图可用的命令                                                   */
/************************************************************************/
#define COMMAND_3D_LOOKUP				0x00000101		//向上看
#define COMMAND_3D_LOOKFORWARD			0x00000102		//向前看
#define COMMAND_3D_LOOKRIGHT			0x00000103		//向右看
#define COMMAND_3D_VIEWTOP				0x00000104		//俯视图
#define COMMAND_3D_VIEWRIGHT			0x00000105		//右视图
#define COMMAND_3D_VIEWBACK				0x00000106		//左视图
#define COMMAND_3D_ISOLEFT				0x00000107		//左45度图
#define COMMAND_3D_ISORIGHT				0x00000108		//右45度图
#define COMMAND_3D_MOVECAMERA			0x00000109		
#define COMMAND_3D_FLY					0x0000010A
#define COMMAND_3D_POLYSELECT			0x0000010B
#define COMMAND_3D_CAMERA				0x0000010C		//3D浏览工具
#define COMMAND_3D_CAMERA2				0x0000010D		//3D浏览工具
#define COMMAND_3D_PANO_SELCTRLPT		0x0000010E 		//全景与点云配准同名点选取
#define COMMAND_3D_RECTSELECT			0x0000010F      //矩形框选择
#define COMMAND_3D_ROTATECENTRESEL		0x00000110      //设置旋转中心
#define COMMAND_3D_MLS_SELECTCTRL		0x00000111		//MLS与全景配准，同名点选取
#define COMMAND_3D_VIEWRECTIN			0x00000112		//三维视图下 拉框放大
#define COMMAND_3D_VIEWRECTOUT			0x00000113		//三维视图下，拉框缩小
#define COMMAND_3D_PICKCENTRIOD			0x00000114		//拾取质心
#define COMMAND_3D_SVCAMERA				0x00000115		//StreetView所用的三维浏览工具
#define COMMAND_3D_SVCONTOURLINE		0x00000116		//StreetView所用的轮廓线工具
#define COMMAND_3D_SVFACADEEDITOR		0x00000118		//StreetView所用的面片删除工具
#define COMMAND_3D_ISCAN_CAMREA			0x00000117		//iScan三维视图浏览工具
#define COMMAND_3D_ORTHO_CAMERA			0x00000119		//正交三维视图浏览工具
#define COMMAND_3D_MEASURE_AREA         0x0000011A      //量测平面面积
#define COMMAND_3D_DOM_CAMERA           0x0000011B      // dom与点云叠加俯视图浏览工具
#define COMMAND_3D_RECTSELECTPOINT           0x0000011C      // 对矩形框内的点云精确查询
#define COMMAND_3D_ANYRECTSELECT      0x0000011D		//任意矩形框选择
#define COMMAND_3D_EXPRECTPOINT       0x0000011E		//矩形框选择点云导出

#define COMMAND_3D_SCAN_ADJUST_OFFSET          0x00000120      // 三维视图下点云位置调整平移工具
#define COMMAND_3D_SCAN_ADJUST_ROTATE		   0x00000121	   // 三维视图下点云位置调整旋转工具
#define COMMAND_3D_SVROUTE_POINT_SEL			0x00000122		// 用于三维轨迹点选择
#define COMMAND_3D_BOX_SELECT_POINT             0x00000123      // 用于三维视图下对三维包围盒控制
#define COMMAND_3D_ANY_PANO_SELCTRLPT		    0x00000124 		// 任意位置全景与点云配准同名点选取
#define COMMAND_3D_BUILDINGEXTRACT			    0x00000125		// 建筑物提取工具
#define COMMAND_3D_IVPOLYLINE					0x00000126      // 区域实景所用的多段线工具
#define COMMAND_3D_IVPOINT						0x00000127		// 用于绘制三维点
#define COMMAND_SELCTRLPT_IN3DVIEW				0x00000128		// 在三维视图中选择拼接控制点
#define COMMAND_3D_CIRCLE_SELECT                0x00000129      // 圆形选择点云
#define COMMAND_3D_FPSCAMERA					0x00000150		// 在三维视图第一人称视角浏览
#define COMMAND_3D_SELECTCAMPOS					0x00000151		// 在三维视图中选择观察点位置
#define COMMAND_3D_ARBITRARYSLICE				0x00000152		// 在三维模型视图中任意选择剖面
#define COMMAND_3D_SVFACADESELETE				0x00000153		// 在StreetView中面片中用来选择面片
//#define COMMAND_3D_IVHEXAHEDRON					0x00000128		//InterView所用的正六面轮廓线工具liujun
#define COMMAND_3D_CIRCLEMESUREPT              	0x00000154    // PtVectorGIS中圆形区域测点工具
#define COMMAND_3D_SELECT_POLYLINE              0x00000155    // 使用矩形拉框选择其相交的polyLine删除
#define COMMAND_ONLY_3D_SELECT_POLYLINE              0x00000156    // 使用矩形拉框选择其相交的polyLine（公共库下，多边形选择）
#define COMMAND_POINT_NODE                       0x00000157      // 全景点云粗配匹配工具
#define COMMAND_3D_ZOOMWND_SELECTPT					0x00000158   // 点云放大镜窗口选择点云工具
#define COMMAND_SAME_POINT                       0x00000159     // 点云精配选点工具
#define COMMAND_2LINE_INSECT_POINT                       0x00000160    // 2条直线确定一个交点
#define COMMAND_3D_VIEW_BOX_SELECT_POINT             0x00000161      // 用于View版三维视图下对三维包围盒控制
#define COMMAND_3D_SEAVIEW_ROUTEPOINT_PICK        0x00000162         //用于View版轨迹点选择定位工具
#define COMMAND_3D_EXTRACT_TYPICAL_LINE           0x00000163         //用于辅助视图提取道路横断面特征线
#define COMMAND_3D_SELECT_POINT                   0x00000164    // 使用矩形拉框选择其相交的3DSceneNode删除
#define COMMAND_3D_POLYSELECTEXPORT				  0x00000168      //矩形框选择导出点云

#define COMMAND_3D_REG_PANO_SELECT_POINT          0X00000169     // 全景与 HLZ 点云配准选点工具

/********************************草图编辑********************************/
#define COMMAND_SHETCH_SCAN_EDIT                0x00000165      // 草图编辑工具, 草图编辑
#define COMMAND_TOOL_DRAFT_LINK                 0x00000166      // 草图视图下邻接关系编辑工具
#define COMMAND_TOOL_DRAFT_DELETE               0x00000167      // 草图工具邻接关系删除
/************************************************************************/

/********************************总览视图********************************/
#define COMMAND_OVERALL_VIEW_EDIT               0x00000170      // 总览视图编辑工具
/************************************************************************/

/************************************************************************/
/* 仅平面视图可用的命令                                                 */
/************************************************************************/
#define COMMAND_PLANAR_CAMERA			0x00000201		//平面浏览工具
#define COMMAND_PLANAR_PAN   			0x00000202		//平面浏览漫游
#define COMMAND_PLANAR_FITLINE			0x00000203		//平面视图选择标志线
#define COMMAND_PLANAR_SELRECTIA			0x00000204		//平面视图选择矩形兴趣区
#define COMMAND_PLANAR_DELRECTIA			0x00000205		//平面视图删除矩形兴趣区
#define COMMAND_PLANAR_ELLIPSELRECTIA		0x00000206		//平面视图选择椭圆兴趣区
/************************************************************************/
/* 3D视图和平面视图均可用的命令                                         */
/************************************************************************/
#define COMMAND_PICK_POINT				0x00000301		// 选点工具
#define COMMAND_CROSS_PICK_POINT        0x00000302      // 三面相交选点工具
#define COMMAND_PICK_POINT_ONLY_3D		0x00000303		// 选点工具
#define COMMAND_SELECT_MARKER_ONLY_3D		0x00000304		// 选标注工具
#define COMMAND_PICK_MULTIPOINT          0x00000305	    // 多点选择工具
/************************************************************************/
/* 仅快速视图可用的命令                                                 */
/************************************************************************/
#define COMMAND_QUICK_SEL_CTRLPT		0x00000401		//此工具实际用于RegView, RegView继承自QuickView
#define COMMAND_QUICK_ADD_CTRLPT		0x00000402		//此命令实际用于RegView，RegView继承自QuickView
#define COMMAND_QUICK_POINTCOLOR		0x00000403		//点云着色命令ID
#define COMMAND_QUICK_PANOTRANSLATE		0x00000404		//全景平移旋转命令ID
#define COMMAND_QUICK_CAMERA			0x00000405		//快速浏览工具		
#define COMMAND_QUICK_PANO_MEASURE		0x00000409		//全景测量工具
#define COMMAND_QUICK_PANO_EDITOR		0x00000410		//全景编辑工具
#define COMMAND_QUICK_PANO_PLAYNEXT     0x0000040A      //全景视图连续向前播放
#define COMMAND_QUICK_PANO_MEASURE_FOR_SZ      0x0000040B      //全景视图中基于深度图进行测量，基于深度图进行量测用于PtVectorGIS深圳项目
#define COMMAND_QUICK_PCD_SERV_MEASURE_FOR_SZ  0x0000040C      // 全景视图下基于点云服务进行测量
#define COMMAND_QUICK_PANO_MEASURE_HEIGHT 0x0000040D    //全景测量高差工具    
#define COMMAND_QUICK_PANO_SYMBOLEDITOR 0x00000411		//全景符号编辑工具
#define COMMAND_QUICK_PANO_BOARDDITOR	0x00000412		// 全景广告牌编辑工具
#define COMMAND_QUICK_PANO_SHPSYMBOLEDITOR 0x00000420	//全景SHP符号编辑工具
#define COMMAND_QUICK_MEASURE_POINT		0x00000421		// Ptvector for Arcgis 全景点测量
#define COMMAND_QUICK_MEASURE_WIDETH	0x00000422	    // Ptvector for Arcgis 全景测距
#define COMMAND_MEASURE_HEIGHT          0x00000423      // 测量高差工具
#define COMMAND_PANO_EDITOR_ADDPOINT    0x00000424      // 添加点工具
#define COMMAND_3DVIEW_MEASURE_HEIGHT   0x00000425      //hdscene view版软件测量高差
#define COMMAND_QUICK_SURVEY_TREE       0X00000426      //Ptvector for Arcgis  三维点云视图测量树工具
#define COMMAND_QUICK_DRAW_LINE		    0x00000427		//PtVector for ArcGIS  测线装要素 
#define COMMAND_3DVIEW_EXTRACT_RB		0x00000428		//PtVector for ArcGIS  提取路牌高度 
#define COMMAND_3DVIEW_EXTRACT_PICTURE		0x00000429		//全景视图中拉框截取图片
#define COMMAND_QUICK_REG_SEL_PLANE         0x0000043A          // 此工具实际用于RegView, RegView继承自QuickView, 选择同名面
#define COMMAND_QUICK_REG_SEL_SPHERE        0x0000043B          // 拼接视图选择靶球
#define COMMAND_QUICK_REG_SEL_CHESSBOARD    0x0000043C          // 拼接视图选择标靶
#define COMMAND_TILE_CAMERA             0x00000430      //切片视图浏览工具
#define COMMAND_QUICK_REG_SEL_CTRLPT    0x00000431      // 拼接视图是快速视图时选点工具
#define COMMAND_QUICK_DRAW_POINT		0x00000432		// 基于点云服务进行绘制点
#define COMMAND_QUICK_DRAW_LINETO2D		0x00000433		// 基于点云服务进行绘制线
#define COMMAND_3DVIEW_EXTRACT_PICTURE2		0x00000434		//选中多个要素拉框截取图片

#define  COMMAND_MEASURE_ANGLE 0x00000435 //角度测量工具 
#define  COMMAND_MEASURE_DIHEDRALANGLE 0x00000436 //二面角测量工具 


/************************************************************************/
/* 全景调查工具                                                 */
/************************************************************************/
#define COMMAND_PANO_SURVEY_POLINE 0x00000437   // 全景调查多段线工具
#define COMMAND_PANO_SURVEY_POINT  0x00000438   // 全景调查点工具
#define COMMAND_PANO_QUERY_AREA    0x00000439   // 全景调查查询闭合圈面积工具


/************************************************************************/
/* 仅缩略视图可用的命令                                                 */
/************************************************************************/
#define COMMAND_OVERVIEW_PAN			0x00000406      //缩略视图浏览
#define COMMAND_OVERVIEW_RECT			0x00000407		//缩略视图选择
#define COMMAND_OVERVIEW_PROP			0x00000408		//缩略视图渲染属性设置

/************************************************************************/
/* 标注添加命令													        */
/************************************************************************/
#define COMMAND_ADD_MARKER_POINT		0x00007004
#define COMMAND_ADD_MARKER_POLYLINE	0x00007005
#define COMMAND_ADD_MARKER_PLANE		0x00007006

/************************************************************************/

/************************************************************************/
/* 标注编辑命令													        */
/************************************************************************/
#define COMMAND_SEL_MARKER		        0x00007007      //选择标注
#define COMMAND_MOVE_MARKER             0x00007008      //移动标注
#define COMMAND_EDIT_MARKER             0x00007009
#define COMMAND_GEOMETRY_EDIT_MARKER    0x00007010
#define COMMAND_EXTRACTLAMP			    0x0000701A     // 提取路灯
#define COMMAND_SKETCH_POLYGON_SELECT	0x0000701B
#define COMMAND_EXTRACTSINGLELAMP       0x0000701C     // 提取单个路灯
#define COMMAND_EXTRACTSINGLETREE	    0x0000701D     // 提取单个行道树
#define COMMAND_EXTRACTBUCKET           0x0000701E     // 提取单个垃圾桶
/************************************************************************/
/* 平面视图和快速视图均可用的命令                                       */
/************************************************************************/
#define COMMAND_MEASURE_POINT			0x00006001
#define COMMAND_ADD_LABEL				0x00006002
#define COMMAND_ADD_SPHERE				0x00006003
#define COMMAND_ADD_KEYBOARD			0x00006004
#define COMMAND_ADD_PLANE				0x00006005		//添加平面
#define COMMAND_ADD_FEATUREPT			0X00006006
#define COMMAND_VIEW_MEASURE_POINT		0x00006007      //View版添加测量线
#define COMMAND_VIEW_ADD_LABEL          0x00006008      //view版添加标注
#define COMMAND_MEASURE_HORI_ANGLE      0x00006009      // 通过拟合平面，量测平面与水平面的夹角
#define COMMAND_ADDRECT_FITPLANE		0x00006010		// 添加拉框拟合平面
#define COMMAND_ADDPOINTS_FITPLANE		0x00006011		// 添加点选拟合平面
#define COMMAND_SELPLANE_FIT_LINE       0x00006012      //选择面拟合线
#define COMMAND_PLANE_SPLITE_LINE       0x00006013      //面分割线或线延长到面
#define COMMAND_DELETE_FIT_LINE         0x00006014      //删除拟合线
#define COMMAND_DELETE_FIT_PLANE        0x00006015      //删除拟合面
#define COMMAND_SELPLANE_FIT_POINT      0x00006016      //三面交点
#define COMMAND_ADDRECT_FITPLINE		0x00006017		// 添加拉框拟合墙线
#define COMMAND_FITLINE_CROSS_POINT     0x00006018      //多线交于一点
#define COMMAND_ADDRECT_FITPOINT		0x00006019		// 添加拉框拟合墙角点
#define COMMAND_ADDSINGLEPOINT_FITPLANE	0x00006020		// 添加单点选拟合平面
#define COMMAND_DRAWPOLYGONLINE			0x00006021		// 添加多边形线

/************************************************************************/
/* Draw2CAD命令													        */
/************************************************************************/
#define COMMAND_DRAW2CAD_POLYLINE		0x00007001
#define COMMAND_DRAW2CAD_POINT			0x00007002
#define COMMAND_DRAW2CAD_POINT_XYZ			0x00007003

/************************************************************************/
/* 俯视图命令													        */
/************************************************************************/
#define COMMAND_VERTICAL_SEL_SLICE		0x00008001
#define COMMAND_VERTICAL_CLASSIFY_ROADMARK		0x00008002
#define COMMAND_VERTICAL_CIRCLE_SELECT_PT    0x00008003


/************************************************************************/
/* 剖面视图分类命令													        */
/************************************************************************/
#define COMMAND_SLICE_CLASSIFY_UPLINE		    0x00009001
#define COMMAND_SLICE_CLASSIFY_BELOWLINE		0x00009002
#define COMMAND_SLICE_CLASSIFY_BETWEENLINE		0x00009003
#define  COMMAND_SLICE_CLASSIFY_BRUSH           0x00009004
#define  COMMAND_SLICE_CLASSIFY_RECTSELECT      0x00009005
#define  COMMAND_SLICE_CLASSIFY_POLYSELECT      0x00009006

/************************************************************************/
/* 模型视图可用命令													        */
/************************************************************************/
#define COMMAND_MODEL_POLYSELECT				0x00009010

/************************************************************************/
/* 插件可用命令													        */
/************************************************************************/
#define COMMAND_PLUGIN_BEGIN		0x00010000

#endif