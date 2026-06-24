/*! @file
********************************************************************************
<PRE>
模块名       : hdCore
文件名       : Hd3dBoxEx.h
相关文件     : Hd3dBoxEx.cpp
文件实现功能 : 实现三维坐标下任意包围盒类
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/08/19   1.0      危迟              新增加内容
</PRE>
*******************************************************************************/
#pragma once
#include "hdPlane3d.h"
#include "hdBox3d.h"
#include "hdCore.h"

#include <vector>
using namespace std;


namespace hd
{
	enum ENUM_HD_BOX_PLANES
	{
		E_FAR_PLANE = 0,

		E_NEAR_PLANE,

		E_LEFT_PLANE,
		
		E_RIGHT_PLANE,
		
		E_BOTTOM_PLANE,
		
		E_TOP_PLANE,

		E_PLANE_COUNT
	};

	class HDCORE_API CHd3dBoxEx
	{
		// 属性
	public:
		//hd::plane3d<f32> planes[E_PLANE_COUNT];

		hd::CHdBox3d<f32> boundingBox;

		// 8个顶点
		hd::CHdVector3df FarLeftUp,FarLeftDown,FarRightUp,FarRightDown;
		hd::CHdVector3df NearLeftUp,NearLeftDown,NearRightUp,NearRightDown;

		// 方法
	public:
		CHd3dBoxEx(void);

		CHd3dBoxEx(hd::CHdVector3df FLU,hd::CHdVector3df FLD,
					hd::CHdVector3df FRU,hd::CHdVector3df FRD,
					hd::CHdVector3df NLU,hd::CHdVector3df NLD,
					hd::CHdVector3df NRU,hd::CHdVector3df NRD);

		CHd3dBoxEx(const CHd3dBoxEx& other);

		CHd3dBoxEx& operator=(const CHd3dBoxEx& other) 
		{
			FarLeftUp = other.FarLeftUp;
			FarLeftDown = other.FarLeftDown;
			FarRightUp = other.FarRightUp;
			FarRightDown = other.FarRightDown;
			NearLeftUp = other.NearLeftUp;
			NearLeftDown = other.NearLeftDown;
			NearRightUp = other.NearRightUp;
			NearRightDown = other.NearRightDown;
			
			boundingBox.MinEdge = other.boundingBox.MinEdge;
			boundingBox.MaxEdge = other.boundingBox.MaxEdge;
			return *this;
		}

		inline void recalculateBoundingBox()
		{
			hd::f32 xmin,ymin,zmin;
			hd::f32 xmax,ymax,zmax;
			xmin = ymin = zmin = F32_MAX;
			xmax = ymax = zmax = F32_MIN;
			
			vector<hd::CHdVector3df> CornerArray;

			CornerArray.push_back(FarLeftUp);
			CornerArray.push_back(FarLeftDown);
			CornerArray.push_back(FarRightUp);
			CornerArray.push_back(FarRightDown);
			CornerArray.push_back(NearLeftUp);
			CornerArray.push_back(NearLeftDown);
			CornerArray.push_back(NearRightUp);
			CornerArray.push_back(NearRightDown);

			for (int i = 0; i< 8;i++)
			{
				if (CornerArray[i].X < xmin)
				{
					xmin = CornerArray[i].X;
				}
				if (CornerArray[i].X > xmax)
				{
					xmax = CornerArray[i].X;
				}
				if (CornerArray[i].Y < ymin)
				{
					ymin = CornerArray[i].Y;
				}
				if (CornerArray[i].Y > ymax)
				{
					ymax = CornerArray[i].Y;
				}
				if (CornerArray[i].Z < zmin)
				{
					zmin = CornerArray[i].Z;
				}
				if (CornerArray[i].Z > zmax)
				{
					zmax = CornerArray[i].Z;
				}
			}

			boundingBox.MinEdge.X = xmin;
			boundingBox.MinEdge.Y = ymin;
			boundingBox.MinEdge.Z = zmin;

			boundingBox.MaxEdge.X = xmax;
			boundingBox.MaxEdge.Y = ymax;
			boundingBox.MaxEdge.Z = zmax;
		}

		hd::CHdVector3df getFarLeftUp() const;

		hd::CHdVector3df getFarLeftDown() const;
	
		hd::CHdVector3df getFarRightUp() const;
	
		hd::CHdVector3df getFarRightDown() const;
		
		hd::CHdVector3df getNearLeftUp() const;
	
		hd::CHdVector3df getNearLeftDown() const;
	
		hd::CHdVector3df getNearRightUp() const;
	
		hd::CHdVector3df getNearRightDown() const;

		hd::CHdVector3df getCenter() const;
		// 获取顶平面
		hd::plane3df getTopPlane() const;
		// 获取底平面
		hd::plane3df getBottomPlane() const;
		// 获取近平面
		hd::plane3df getNearPlane() const;
		// 获取远平面
		hd::plane3df getFarPlane() const;
		// 获取左平面
		hd::plane3df getLeftPlane() const;
		// 获取右平面
		hd::plane3df getRightPlane() const;
		// 判断是否与平行轴盒子相交
		bool IsCubeIn(hd::CHdBox3df cube);
		// 判断点是否在盒子内
		bool IsPointIn(const hd::CHdVector3df point);

		~CHd3dBoxEx(void);
	};
}


