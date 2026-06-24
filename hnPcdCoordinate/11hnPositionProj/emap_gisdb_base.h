//----------------------------------------------------------------------------
//	emap_gisdb_base.h
//  charlin.luo
//  2011-12-12
//
//
//
//----------------------------------------------------------------------------

#ifndef _EMAP_GISDB_BASE_H_
#define _EMAP_GISDB_BASE_H_

#include <stdio.h>
#include <windows.h>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  文件状态
typedef enum{
	E_FILE_STATE_NORMAL		=	0x00,		//正常状态，未修改
	E_FILE_STATE_EDIT		=	0x01		//已修改
}E_FILE_STATE;

//	文件类型
typedef enum{
	E_FILE_TYPE_UNKNOWN		=	0x00,		//	未定义
	E_FILE_TYPE_VECTOR_ED2	=	0x01,		//	可编辑矢量
	E_FILE_TYPE_VECTOR_EDS  =   0X02,		//  背景矢量数据
	E_FILE_TYPE_ANNOTATION  =   0X03,		//  注记文件数据
	E_FILE_TYPE_RASTER		=	0x04,		//	栅格
	E_FILE_TYPE_NETWROK		=	0x05,		//	网络	
}E_FILE_TYPE;

//	实体空间几何类型
typedef enum{
	E_GEO_TYPE_UNKNOWN	=	0x00,		//	未定义
	E_GEO_TYPE_PNT		=	0x01,		//	点
	E_GEO_TYPE_LIN		=	0x02,		//	线
	E_GEO_TYPE_REG		=	0x03,		//	面

	E_GEO_TYPE_MPNT		=	0x04,		//	多点
	E_GEO_TYPE_MLIN		=	0x05,		//	多线
	E_GEO_TYPE_MREG		=	0x06,		//	多面

	E_GEO_TYPE_ANNO		=	0x07,		//	注记
	E_GEO_TYPE_TILE		=	0x08,		//	瓦片
	E_GEO_TYPE_NETWORK  =	0x09,		//  网络
}E_GEO_TYPE;

//  瓦片数据说明
typedef enum{
	E_TILE_FORMAT_UNKNOWN = 0x00,		//  为定义
	E_TILE_FORMAT_BMP	  = 0x01,       //  BMP格式
	E_TILE_FORMAT_JPEG	  = 0x02,       //  JPRG格式
	E_TILE_FORMAT_PNG	  = 0x03,       //  PNG格式
	E_TILE_FORMAT_TIF	  = 0x04,       //  TIF格式
}E_TILE_FORMAT;

//----------------------------------------------------------------------------
//	坐标类型
typedef enum{
	E_COOR_TYPE_UNKNOWN		=	0x00,		//	未定义
	E_COOR_TYPE_2D			=	0x01,		//	2D坐标
	E_COOR_TYPE_3D			=	0x02,		//	3D坐标
	E_COOR_TYPE_2DINFO		=	0x03,		//	含有附件信息的2D坐标
	E_COOR_TYPE_3DINFO		=	0x04		//	含有附件信息的3D坐标
}E_COOR_TYPE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// 空间参考 宏定义，相关定义参考ArcGIS10 辛后林 2012-5-24

//	地图坐标系类型
typedef enum{
	E_COOR_SYSTEM_TYPE_UNKNOWN	=	0x00,			//	未定义
	E_COOR_SYSTEM_TYPE_GEO		=	0x01,			//	地理坐标系，球面坐标系，GeographicCoordinateSystem
	E_COOR_SYSTEM_TYPE_PRJ		=	0x02,			//	投影坐标系，平面坐标系，ProjectedCoordinateSystem
}E_COOR_SYSTEM_TYPE;

//	坐标单位
typedef enum{
	E_COOR_UNIT_TYPE_UNKNOWN		=	0x00,		//	未定义
	E_COOR_UNIT_TYPE_DEGREE			=	0x01,		//	度
	E_COOR_UNIT_TYPE_RADIAN			=	0x02,		//	弧度
	E_COOR_UNIT_TYPE_METER			=	0x03,		//	米
	E_COOR_UNIT_TYPE_CM				=	0x04,		//	厘米
	E_COOR_UNIT_TYPE_MM				=	0x05		//	毫米
}E_COOR_UNIT_TYPE;

//  max long 42 9496 7296
typedef enum{
	E_COOR_PRECESION_UNKNOWN		=	100,	
	E_COOR_PRECESION_DEGREE			=	10000000,
	E_COOR_PRECESION_RADIAN			=	100000000,
	E_COOR_PRECESION_METER			=	100,
	E_COOR_PRECESION_CM				=	10,
	E_COOR_PRECESION_MM				=	1
}E_COOR_UNIT_PRECESION;

//	参考椭球体类型(earthParam字段值)
typedef enum{
	E_EARTH_TYPE_UNKNOWN			=	0,  //  未指定"参考椭球体参数类型"
	E_EARTH_TYPE_Beijing54			=	1,  //" 1:北京54/克拉索夫斯基(1940年)椭球",  //"Krasovsky"
	E_EARTH_TYPE_Krasovsky			=	2,  //" 1:北京54/克拉索夫斯基(1940年)椭球",  //"Krasovsky"
	E_EARTH_TYPE_Xian80				=	3,  //" 2:西安80/1975 年I.U.G.G推荐椭球 ",
	E_EARTH_TYPE_IUGG1975			=	4,  //" 2:西安80/1975 年I.U.G.G推荐椭球 ",
	E_EARTH_TYPE_IUGG1979			=	5,  //" 3:1979 年I.U.G.G推荐椭球 ",
	E_EARTH_TYPE_IUGG1983			=	6,  //" 4:1983 年I.U.G.G推荐椭球",
	E_EARTH_TYPE_IUGG1967			=	7,  //" 6:1967 年I.U.G.G推荐椭球",
	E_EARTH_TYPE_WGS84				=	8,  //" 7:WGS-84 ",
	E_EARTH_TYPE_GRS80				=	9,  //" 8:GRS-80 ",
	E_EARTH_TYPE_WGS72				=	10,  //" 9:WGS-72 ",
	E_EARTH_TYPE_China2000			=	11,  //" 10：国家2000"
	E_EARTH_TYPE_Australia1965		=	12, //" 11:澳大利亚1965年椭球",
}E_EARTH_TYPE;

// 投影类型
typedef enum{
	E_PROJECT_TYPE_UNKNOWN						= 0,	// 未定义
	E_PROJECT_TYPE_UTM							= 1,	//通用横向墨卡托投影坐标系(UTM)",
	E_PROJECT_TYPE_Albers_Conical_EQ_Area		= 2,	//亚尔勃斯等积圆锥投影坐标系",ALBERS CONICAL EQUAL AREA
	E_PROJECT_TYPE_Lambert_Conformal_Conic		= 3,	//兰伯特等角圆锥投影坐标系",LAMBERT CONFORMAL CONIC
	E_PROJECT_TYPE_Mercator						= 4,	//墨卡托(正轴等角圆柱)投影坐标系",MERCATOR
	E_PROJECT_TYPE_Gauss_Kruger					= 5,	//高斯-克吕格(横切椭圆柱等角)投影",GAUSS-KRUGER
	E_PROJECT_TYPE_Polyconic					= 6,	//普通多圆锥投影坐标系",POLYCONIC
	E_PROJECT_TYPE_EQ_Dist_Conic				= 7,	//等距圆锥投影坐标系",EQUIDISTANT  CONIC
	E_PROJECT_TYPE_Transverse_Mecator			= 8,	//横向墨卡托(横切圆柱等角)投影",TRANSVERSE MECATOR
	E_PROJECT_TYPE_StereoGraphic				= 9,	//球面投影(视点在球面)坐标系",STEREOGRAPHIC
	E_PROJECT_TYPE_Lambert_Azimuthal_EQ_Area	= 10,	//兰伯特等积方位投影坐标系",LAMBERT  AZIMUTHAL EQUAL_AREA
	E_PROJECT_TYPE_Azimuthal_EQ_Dist			= 11,	//等距方位投影坐标系",AZIMUTHAL EQUIDISTANT
	E_PROJECT_TYPE_Gnomonic						= 12,	//心射切面(球心)投影坐标系",GNOMONIC
	E_PROJECT_TYPE_Orthographic					= 13,	//正射投影(视点无穷远)坐标系",ORTHOGRAPHIC
	E_PROJECT_TYPE_General_VER_NS_Perspective	= 14,	//通用垂直近距透视(外心)投影",GENERAL VERTICAL NEAR_SIDE PERSPECTIVE
	E_PROJECT_TYPE_Sinusoidal					= 15,	//正弦投影(伪圆柱)坐标系",SINUSOIDAL
	E_PROJECT_TYPE_Equirectangular				= 16,	//等距离切圆柱(方格)投影坐标系",EQUIRECTANGULAR
	E_PROJECT_TYPE_Miller_Cylindrical			= 17,	//米勒圆柱(透视正圆柱)投影坐标系",MILLER CYLINDRICAL
	E_PROJECT_TYPE_V_D_Grinten_I				= 18,	//范德格林顿I投影坐标系",VAN DER  GRINTEN I
	E_PROJECT_TYPE_Oblique_Mercator				= 19,	//斜轴墨卡托投影坐标系",OBLIQUE MERCATOR (HOTINE)
	E_PROJECT_TYPE_Polar_Srereographic			= 20,	//极点球面投影坐标系",POLAR SREREOGRAPHIC
}E_PROJECT_TYPE;

// 高斯-克吕格投影 分带类型
typedef enum{
	E_ZONE_TYPE_UNKNOWN				= 0,	// 未定义
	E_ZONE_TYPE_3					= 1,	// 3度分带
	E_ZONE_TYPE_6					= 2,	// 6度分带
}E_ZONE_TYPE;

// 转换方法类型 Convert
typedef enum{
	E_CONVERT_TYPE_UNKNOWN				= 0,	// 未定义
	E_CONVERT_TYPE_4					= 1,	// 4参数投影转换
	E_CONVERT_TYPE_7					= 2,	// 7参数投影转换
}E_CONVERT_TYPE;

// 投影坐标系，比例尺类型
typedef enum
{
	E_SCALE_TYPE_UNKNOWN		= 0,	// 未定义
	E_SCALE_TYPE_1000000		= 1,	// 1:100万
	E_SCALE_TYPE_500000			= 2,	// 1:50万
	E_SCALE_TYPE_250000			= 3,	// 1:25万
	E_SCALE_TYPE_200000			= 4,	// 1:20万
	E_SCALE_TYPE_100000			= 5,	// 1:10万
	E_SCALE_TYPE_50000			= 6,	// 1:5万
	E_SCALE_TYPE_25000			= 7,	// 1:2.5万
	E_SCALE_TYPE_10000			= 8,	// 1:1万
	E_SCALE_TYPE_5000			= 9,	// 1:5千
	E_SCALE_TYPE_2000			= 10,	// 1:2千
	E_SCALE_TYPE_1000			= 11,	// 1:1千
	E_SCALE_TYPE_500			= 12,	// 1:500
}E_SCALE_TYPE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// 空间参考系结构体定义
typedef struct _tagSpatialRef
{
	long		coorSystem;			// 坐标系类型，地理坐标系或投影坐标系
	long		coorUnit;			// 坐标单位
	long		earthType;			// 参考椭球体类型
	long		prjType;			// 投影类型
	double		Rc;
	double		Ac;
	double		Lo;					//中心点经度
	double		Bo;					//中心点纬度
	double		NF;					//假原点北坐标
	double		EF;					//假原点东坐标
	double		EC;					//平均东坐标
	double		NC;					//平均北坐标
	double		FE;					//东偏移
	double		FN;					//北偏移
	double		B1;					//第一纬线
	double		B2;					//第二纬线
	double		Bf;
	double		Lf;
	double		Bc;
	double		Lc;					//平均经度
	double		Bp;					//标准纬线
	double		Li;					//最初的经线
	double		Ko;					//尺度缩放比	
	double		Kc;
	double		Kp;
	double		PH;					//投影高
	int			W;					//度 带宽
	int			Add;				//带号
	int			bAdd;				//添加带号
	int			North;				//坐标轴X正向是北向
	int			East;				//坐标轴Y正向是东向
	int			Unused;
}Spatial_Ref_t;

//----------------------------------------------------------------------------

#endif
//----------------------------------------------------------------------------
//	EOF emap_gisdb_base.h



