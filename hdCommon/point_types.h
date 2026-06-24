/*! point_types.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : point_types.h
相关文件     : 
文件实现功能 : 点类型定义
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/31   1.0      龚书林    
2013/09/11	 2.0	  危迟				添加分类选中属性 两级 
										占用intensity第13、14位标记 有效强度范围0~4096
</PRE>
*******************************************************************************/

#ifndef __HD_POINT_TYPE_H_INCLUDED__
#define __HD_POINT_TYPE_H_INCLUDED__

// 设置结构体成员对齐1byte
#pragma  pack(push,1)

#include <math.h>
#include <vector>
#include "..\hdCore\hdDefs.h"

#ifndef SMR_RADIUS
#define SMR_RADIUS  0.0725
#endif

#ifndef CHB_SIDE
#define CHB_SIDE    0.1
#endif	

//#ifndef RAD2DEG
//#define RAD2DEG(x) ((x)*57.29578)
//#endif

namespace hd
{

	typedef enum ENUM_POINTTYPE
	{
		HDPCD_XYZ		= 1,	// XYZ属性
		HDPCD_INTENSITY = 2,	// 强度属性
		HDPCD_RGB       = 4,	// RGB属性
		HDPCD_CLASS		= 8,	// 分类属性
		HDPCD_NORMAL	= 16,	// NORMAL属性
		HDPCD_NORMALXYZ	= 17,	// NORMAL属性
	};
// 包含x,y,z属性的点
struct PointXYZ
{
	hd::f32  x,y,z;
	PointXYZ()
		:x(0.0f),y(0.0f),z(0.0f){}
	PointXYZ(double _dx, double _dy, double _dz)
	{
		x = (hd::f32)_dx;
		y = (hd::f32)_dy;
		z = (hd::f32)_dz;
	}
	ENUM_POINTTYPE getType(){return HDPCD_XYZ;}
};

// 包含x,y,z,intensity属性的点
struct PointXYZI
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;  // 反射强度,第16,15位分别标记选中,删除
	PointXYZI()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0){}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY;}

	bool operator< (const struct PointXYZI& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZI& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

// 点的RGB和属性分类
union PointRGBP
{
	struct
	{
		hd::u8 r;
		hd::u8 g;
		hd::u8 b;
		hd::u8 prop;
	};
	hd::u32 color_p;
};

// 包含x,y,z,r,g,b属性的点
struct PointXYZRGB
{
	hd::f32  x,y,z;	// 坐标
	hd::u8 b;
	hd::u8 g;
	hd::u8 r;
	PointXYZRGB()
		:x(0.0f),y(0.0f),z(0.0f),b(0),g(0),r(0){}//,_unused(0)
	int getType(){return HDPCD_XYZ | HDPCD_RGB;}
};

//hls转hlz中间缓存文件中点数据格式（18字节压缩到10字节） 袁亮  20161111
struct XYZIPRGB
{
	hd::u16 x, y, z;    //所在块集内的格网坐标
	hd::u8  intensity;  //强度线性映射到0-255之间
	hd::u8  prop;       //分类属性   
	hd::u16 color;      //R5G6B5

	XYZIPRGB()
		:x(0), y(0), z(0), intensity(0),  prop(0), color(0)
	{

	}

	static bool hasColor()
	{
		return true;
	}

	inline void setColor(u8 r, u8 g, u8 b)
	{
		color = (u16)((r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3);
	}

	inline u16 getColor() const
	{
		return color;
	}
};

//hls转hlz中间缓存文件中点数据格式2（不存储颜色信息）
struct XYZIP
{
	hd::u16 x, y, z;    //所在块集内的格网坐标
	hd::u8  intensity;  //强度线性映射到0-255之间
	hd::u8  prop;       //分类属性 

	XYZIP()
		:x(0), y(0), z(0), intensity(0), prop(0)
	{

	}

	static bool hasColor()
	{
		return false;
	}

	inline void setColor(u8 r, u8 g, u8 b)
	{
	}

	inline u16 getColor() const 
	{
		return 0;
	}
};

// 包含x,y,z,intensity,normal_x,normal_y,normal_z,curvature属性
struct PointXYZINormal
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	hd::f32 nx,ny,nz;		// 法向量
	hd::f32 curvature;	// 曲率

	PointXYZINormal()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),
		nx(0.0f),ny(0.0f),nz(0.0f),curvature(0.0f){}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_NORMAL;}

	bool operator< (const struct PointXYZINormal& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZINormal& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

struct NormalPointXYZ
{
	hd::f32 x,y,z;
	hd::f32 nx,ny,nz;

	NormalPointXYZ()
		:x(0.0f),y(0.0f),z(0.0f),
		nx(0.0f),ny(0.0f),nz(0.0f){}

	int getType(){return HDPCD_XYZ | HDPCD_NORMAL;}
};
// 包含normal_x,normal_y,normal_z,curvature属性
struct Normal
{
	hd::f32 nx,ny,nz;		// 法向量
	hd::f32 curvature;	// 曲率

	Normal()
		:nx(0.0f),ny(0.0f),nz(0.0f),curvature(0.0f){}

	int getType(){return HDPCD_NORMAL;}
};

struct NormalXYZ
{
	hd::f32 nx,ny,nz;		// 法向量

	NormalXYZ()
		:nx(0.0f),ny(0.0f),nz(0.0f){}

	int getType(){return HDPCD_NORMALXYZ;}
};

// 包含x,y,z,intensity,r,g,b属性的点
//template <typename T = f32>
struct PointXYZIRGB
{
	hd::f32  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	hd::u8 b;
	hd::u8 g;
	hd::u8 r;

	PointXYZIRGB()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),b(0),g(0),r(0){}//,_unused(0)

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_RGB;}

	bool operator< (const struct PointXYZIRGB& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZIRGB& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}
	inline bool isValid()
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f);
	}
	inline bool isValid(int min,int max) 
	{
		return getIntensity() >= min && getIntensity() <= max;
	}

	// 获取反射强度
	inline hd::u16 getIntensity()const{return intensity & 0x3fff;}
	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}
};

// 包含x,y,z,intensity,r,g,b,property属性的点
//template <typename T = f32>
struct PointXYZIPRGBA
{
	hd::f32  x,y,z;			// 坐标
	hd::u16 intensity;		// 反射强度,第16,15位分别标记选中,删除
	union
	{
		struct
		{
			hd::u8 r;
			hd::u8 g;
			hd::u8 b;
			hd::u8 prop;	// 属性分类
		};
		hd::u32 c_color;
	};

	//hd::u8 select;			// 选择状态,第4bit是1的话就是选中,第3bit是1的话就是删除
	PointXYZIPRGBA()
		:x(0.0f),y(0.0f),z(0.0f),intensity(0),prop(1),b(0),g(0),r(0){}
	~PointXYZIPRGBA()
	{

	}

	int getType(){return HDPCD_XYZ | HDPCD_INTENSITY | HDPCD_RGB | HDPCD_CLASS;}

	bool operator< (const struct PointXYZIPRGBA& rhs) const
	{
		return getIntensity()<rhs.getIntensity();
	}

	bool operator> (const struct PointXYZIPRGBA& rhs) const
	{
		return getIntensity()>rhs.getIntensity();
	}

	bool operator== (const struct PointXYZIPRGBA& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z &&
			getIntensity() == rhs.getIntensity() && prop == rhs.prop &&
			r == rhs.r && g == rhs.g && b == rhs.b;
	}

	//有效点判断  [2012/07/24 危迟]  
	inline bool isValid()const 
	{
		return !(x == 0.0f && y == 0.0f && z == 0.0f) && !isDeleted();
	}
	inline bool isValid(int min,int max) const
	{
		return getIntensity() >= min && getIntensity() <= max && !isDeleted();
	}

	// 获取反射强度 占用第13、14位标记分类选中后 真实强度值只有12位 0-4096
	inline hd::u16 getIntensity() const
	{
		return (intensity & 0x0fff);
	}

	//判断点是否选中
	inline bool isSelected()const
	{
		return (intensity & 0x8000) == 0x8000;	// 选中点标记
	}
	//判断点是否删除
	inline bool isDeleted()const
	{
		return (intensity & 0x4000) == 0x4000;	// 删除点标记
	}
	// 设置选中
	inline void setSelected()
	{
		intensity = (intensity | 0x8000);
	}
	// 设置反选
	inline void setXorSelected()
	{
		intensity = (intensity ^ 0x8000);
	}
	// 设置取消选中
	inline void setUnSelected()
	{
		intensity = (intensity & 0x7fff);
	}
	// 设置删除
	inline void setDeleted()
	{
		intensity = (intensity | 0x4000);
	}

	// 设置分类选中属性一级
	inline void setClassifySelectedF()
	{
		intensity = (intensity | 0x2000);
	}

	inline void setUnClassifySelectedF()
	{
		intensity = (intensity & 0x1fff);
	}
	// 获取是否分类选中属性一级
	inline bool isClassifySelectedF() const
	{
		return (intensity & 0x2000) == 0x2000;	
		
	}

	// 设置不分类选中属性一级
	inline void setDeClassifySelectedF()
	{
		intensity = (intensity & 0xDfff);
	}

	// 设置分类选中属性二级
	inline void setClassifySelectedS()
	{
		intensity = (intensity | 0x1000);
	}
	inline void setUnClassifySelectedS()
	{
		intensity = (intensity & 0x0fff);
	}
	// 获取是否分类选中属性二级
	inline bool isClassifySelectedS() const
	{
		return (intensity & 0x1000) == 0x1000;	

	}

	// 设置不分类选中属性二级
	inline void setDeClassifySelectedS()
	{
		intensity = (intensity & 0xEfff);
	}

	////判断点是否选中
	//inline bool isSelected()const
	//{
	//	return (select & 0x8) == 0x8;	// 选中点标记
	//}
	////判断点是否删除
	//inline bool isDeleted()const
	//{
	//	return (select & 0x4) == 0x4;	// 删除点标记
	//}
};

// 根据x排序点云函数
inline bool lessByX(const PointXYZIPRGBA& pt1,const PointXYZIPRGBA& pt2)
{
	return pt1.x < pt2.x;
}

// 根据y排序点云函数
inline bool lessByY(const PointXYZIPRGBA& pt1,const PointXYZIPRGBA& pt2)
{
	return pt1.y < pt2.y;
}

//  根据z排序点云函数
inline bool lessByZ(const PointXYZIPRGBA& pt1,const PointXYZIPRGBA& pt2)
{
	return pt1.z < pt2.z;
}

template <class T>
inline bool lessByXYZ_T(const T& pt1,const  T& pt2)
{
	s16 dis_x = pt1.x - pt2.x;
	s16 dis_y = pt1.y - pt2.y;
	s16 dis_z = pt1.z - pt2.z;
	f32 dis = 0.0;
	if (dis_x < (-dis))
	{
		return true;
	}
	else if (dis_x >= (-dis) && dis_x <= dis)
	{
		if (dis_y < (-dis))
		{
			return true;
		}
		else if (dis_y >= (-dis) && dis_y <= dis)
		{
			if (dis_z < 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (dis_y > (-dis))
		{
			return false;
		}
	}
	else if (dis_x > dis)
	{
		return false;
	}
}

//  根据x,y,z的优先级排序点云函数
inline bool lessByXYZ(const PointXYZIPRGBA& pt1,const PointXYZIPRGBA& pt2)
{
	f32 dis_x = pt1.x - pt2.x;
	f32 dis_y = pt1.y - pt2.y;
	f32 dis_z = pt1.z - pt2.z;
	f32 dis = 0.0;
	if (dis_x < (-dis))
	{
		return true;
	}
	else if (dis_x >= (-dis) && dis_x <= dis)
	{
		if (dis_y < (-dis))
		{
			return true;
		}
		else if (dis_y >= (-dis) && dis_y <= dis)
		{
			if (dis_z < 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (dis_y > (-dis))
		{
			return false;
		}
	}
	else if (dis_x > dis)
	{
		return false;
	}
}

//  根据x,y,z的优先级排序点云指针函数
inline bool pLessByXYZ(const PointXYZIPRGBA* pt1,const PointXYZIPRGBA* pt2)
{
	f32 dis_x = pt1->x - pt2->x;
	f32 dis_y = pt1->y - pt2->y;
	f32 dis_z = pt1->z - pt2->z;
	f32 dis = 0.0;
	if (dis_x < (-dis))
	{
		return true;
	}
	else if (dis_x >= (-dis) && dis_x <= dis)
	{
		if (dis_y < (-dis))
		{
			return true;
		}
		else if (dis_y >= (-dis) && dis_y <= dis)
		{
			if (dis_z < 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (dis_y > (-dis))
		{
			return false;
		}
	}
	else if (dis_x > dis)
	{
		return false;
	}
}

template <class T>
inline bool pLessByXYZ_T(const T* pt1, const T* pt2)
{
	s32 dis_x = pt1->x - pt2->x;
	s32 dis_y = pt1->y - pt2->y;
	s32 dis_z = pt1->z - pt2->z;
	s32 dis = 0.0;
	if (dis_x < (-dis))
	{
		return true;
	}
	else if (dis_x >= (-dis) && dis_x <= dis)
	{
		if (dis_y < (-dis))
		{
			return true;
		}
		else if (dis_y >= (-dis) && dis_y <= dis)
		{
			if (dis_z < 0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (dis_y > (-dis))
		{
			return false;
		}
	}
	else if (dis_x > dis)
	{
		return false;
	}
}

// 包含x,y,z,intensity属性的双精度坐标点
struct PointXYZI_D
{
	hd::f64  x,y,z;		// 坐标
	hd::u16 intensity;		// 反射强度
	PointXYZI_D()
		:x(0.0),y(0.0),z(0.0),intensity(0){}

	bool operator< (const struct PointXYZI_D& rhs) const
	{
		return intensity<rhs.intensity;
	}

	bool operator> (const struct PointXYZI_D& rhs) const
	{
		return intensity>rhs.intensity;
	}
	inline bool isValid()
	{
		return !(x == 0.0 && y == 0.0 && z == 0.0);
	}
	inline bool isValid(int min,int max) 
	{
		return intensity >= min && intensity <= max;
	}
};
/*
平面拟合
*/
struct CtlPlaneFun
{
	hd::f32	rowScale,colScale;	// 平面中心行列比例;
	hd::f32	x,y,z;				// 平面中心位置;
	hd::s32	fitPtCount;		// 参与拟合的点数;
	hd::f32	stdDev;				// 拟合标准差;
	std::vector<PointXYZIPRGBA> vecBorderPts;//边界点;

	hd::f32 FunM[4];			//平面方程Ax+By+Cz+D=0

	CtlPlaneFun()
		:x(0.0f),y(0.0f),z(0.0f),rowScale(0.0f),colScale(0.0f),stdDev(0.0f),fitPtCount(0),vecBorderPts(NULL){}

	inline bool IsSamePlane(CtlPlaneFun& other, hd::f32 fAngleTor = 1.f, hd::f32 fDistTor = 0.1f)
	{
		//// 两平面需提前单位化
		//core::vector3df normSelf(FunM[0], FunM[1], FunM[2]);
		//core::vector3df normOther(other.FunM[0], other.FunM[1], other.FunM[2]);
		//if (normSelf.getAngleTo(normOther) < fAngleTor && fabs(FunM[3] - other.FunM[3]) < fDistTor)
		//{
		//	return true;
		//}
		return true;
	}
};
/*
靶球拟合
*/
struct PointSphere
{
	hd::f32	rowScale,colScale;	// 靶球所在点云行列比例
	hd::f32	x,y,z;				// 靶球中心位置
	hd::s32		fitPtCount;		// 参与拟合的点数
	hd::f32	stdDev;				// 半径标准差
	PointSphere()
		:x(0.0f),y(0.0f),z(0.0f),rowScale(0.0f),colScale(0.0f),stdDev(0.0f),fitPtCount(0){}
};
/*
棋盘格标靶点
*/
struct PointChessboard
{
	hd::f32 rowScale,colScale;      //标靶所在的灰度图像素比例
	hd::s32 row,col;				  //标靶中心点所在的灰度图像行列号
	hd::f32 x,y,z;				  //标靶中心点三维坐标
	hd::f32	stdDev;				// 匹配精度——5.26.2014添加by dongdai;
	PointChessboard()
		:x(0.0f),y(0.0f),z(0.0f),rowScale(0.0f),colScale(0.0f),row(0),col(0),stdDev(0.0f) {}
	
 	PointChessboard(const PointChessboard& other)
 		:x(other.x),y(other.y),z(other.z),rowScale(other.rowScale),colScale(other.colScale),row(other.row),col(other.col),stdDev(other.stdDev) {}

	inline hd::f32 GetDistance(PointChessboard& other)
	{
		return sqrt((x-other.x)*(x-other.x) + (y-other.y)*(y-other.y) + (z-other.z)*(z-other.z));
	}
};

/*
点云中的点分类
*/
typedef enum ENUM_POINTCLASS {
	CLASS_ALL = 0,
	CLASS_FIRST,
	CLASS_LAST,
	CLASS_GROUND,
	CLASS_OBJECT,
	CLASS_BUILDING,
	CLASS_VEGETATION,
	CLASS_MASS_POINTS,
	CLASS_WATER,
	CLASS_UNCLASSIFIED,
	CLASS_OVERLAP
} ENUM_POINTCLASS;


// 全景与点云配准控制点
struct PanoControlPoint 
{
public:
	PanoControlPoint()
	{
		memset(this,0,sizeof(PanoControlPoint));
	}
	PanoControlPoint(int imgx,int imgy,f32 px,f32 py,f32 pz)
		:imageX(imgx),imageY(imgy),X(px),Y(py),Z(pz)
	{
	}
	int imageX;
	int imageY;
	hd::f32 X;
	hd::f32 Y;
	hd::f32 Z;
};

// zfs点云的坐标系统角度计算方法[zfei 2014/9/2]
//(弃用，都使用GetPointAngle进行计算，原因是在将zfs转成hls时即将坐标系统转换过来)
inline void GetPointAngleZfs(double x,double y,double z,double& angleX,double& angleZ)
{
	angleZ = atan2(z,sqrt(x*x + y*y)) * 57.29577951308233;		// 竖直角的范围肯定在(-90,90)之间
	
	angleX = atan2(x,y) * 57.29577951308233;
	if (angleX < 0)
	{
		angleX += 360.0;	// 将水平角转换到[0,360)之间
	}
}

//! 获取扫描点的水平角angleX和垂直角angleZ
inline void GetPointAngle(double x,double y,double z,double& angleX,double& angleZ)
{
	angleZ = atan2(z,sqrt(x*x + y*y)) * 57.29577951308233;		// 竖直角的范围肯定在(-90,90)之间
	angleX = atan2(y,x) * 57.29577951308233;
	if (angleX < 0)
	{
		angleX += 360.0;	// 将水平角转换到[0,360)之间
	}
}

}
//恢复默认结构体对齐
#pragma  pack(pop)

#endif