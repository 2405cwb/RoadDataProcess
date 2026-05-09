#include "StdAfx.h"
#include "Hd3dBoxEx.h"

namespace hd
{
	CHd3dBoxEx::CHd3dBoxEx(void)
	{
	
		recalculateBoundingBox();
	}

	CHd3dBoxEx::CHd3dBoxEx(const CHd3dBoxEx& other)
	{
		
	}

	CHd3dBoxEx::CHd3dBoxEx(hd::CHdVector3df FLU,hd::CHdVector3df FLD,
		hd::CHdVector3df FRU,hd::CHdVector3df FRD,
		hd::CHdVector3df NLU,hd::CHdVector3df NLD,
		hd::CHdVector3df NRU,hd::CHdVector3df NRD)
	{
		FarLeftUp = FLU;
		FarLeftDown = FLD;
		FarRightUp = FRU;
		FarRightDown = FRD;
		NearLeftUp = NLU;
		NearLeftDown = NLD;
		NearRightUp = NRU;
		NearRightDown = NRD;

		recalculateBoundingBox();
	}

	hd::CHdVector3df CHd3dBoxEx::getFarLeftUp() const
	{
		return FarLeftUp;
	}

	hd::CHdVector3df CHd3dBoxEx::getFarLeftDown() const
	{

		return FarLeftDown;
	}

	hd::CHdVector3df CHd3dBoxEx::getFarRightUp() const
	{
		return FarRightUp;
	}

	hd::CHdVector3df CHd3dBoxEx::getFarRightDown() const
	{
		return FarRightDown;
	}

	hd::CHdVector3df CHd3dBoxEx::getNearLeftUp() const
	{
		return NearLeftUp;
	}

	hd::CHdVector3df CHd3dBoxEx::getNearLeftDown() const
	{
		return NearLeftDown;
	}

	hd::CHdVector3df CHd3dBoxEx::getNearRightUp() const
	{
		return NearRightUp;
	}

	hd::CHdVector3df CHd3dBoxEx::getNearRightDown() const
	{
		return NearRightDown;
	}

	// 获取顶平面
	hd::plane3df CHd3dBoxEx::getTopPlane() const
	{
		hd::plane3df top(NearLeftUp,NearRightUp,FarLeftUp);
		return top;
	}
	// 获取底平面
	hd::plane3df CHd3dBoxEx::getBottomPlane() const
	{
		hd::plane3df bottom(NearLeftDown,NearRightDown,FarLeftDown);
		return bottom;
	}
	// 获取近平面
	hd::plane3df CHd3dBoxEx::getNearPlane() const
	{
		hd::plane3df nearPlane(NearLeftDown,NearRightDown,NearLeftUp);
		return nearPlane;
	}
	// 获取远平面
	hd::plane3df CHd3dBoxEx::getFarPlane() const
	{
		hd::plane3df farPlane(FarLeftDown,FarRightDown,FarLeftUp);
		return farPlane;
	}
	// 获取左平面
	hd::plane3df CHd3dBoxEx::getLeftPlane() const
	{
		hd::plane3df left(NearLeftDown,NearLeftUp,FarLeftDown);
		return left;
	}
	// 获取右平面
	hd::plane3df CHd3dBoxEx::getRightPlane() const
	{
		hd::plane3df right(NearRightDown,NearRightUp,FarRightDown);
		return right;
	}

	hd::CHdVector3df CHd3dBoxEx::getCenter() const
	{
		hd::CHdVector3df center;
		center = (NearLeftDown + FarRightUp)/2.f;
		return center;
	}

	bool CHd3dBoxEx::IsCubeIn(hd::CHdBox3df cube)
	{
		hd::plane3df planes[E_PLANE_COUNT];
		planes[E_TOP_PLANE] = getTopPlane();
		planes[E_BOTTOM_PLANE] = getBottomPlane();
		planes[E_LEFT_PLANE] = getLeftPlane();
		planes[E_RIGHT_PLANE] = getRightPlane();
		planes[E_NEAR_PLANE] = getNearPlane();
		planes[E_FAR_PLANE] = getFarPlane();
		int i;
		for (i = 0; i < E_PLANE_COUNT; i++)
		{
			const hd::plane3df& p = planes[i];
			if(cube.classifyPlaneRelation(p) == hd::ISREL3D_FRONT)
				return false;
		}
		return true;
	}

	bool CHd3dBoxEx::IsPointIn(const hd::CHdVector3df point)
	{
		hd::plane3df planes[E_PLANE_COUNT];
		planes[E_TOP_PLANE] = getTopPlane();
		planes[E_BOTTOM_PLANE] = getBottomPlane();
		planes[E_LEFT_PLANE] = getLeftPlane();
		planes[E_RIGHT_PLANE] = getRightPlane();
		planes[E_NEAR_PLANE] = getNearPlane();
		planes[E_FAR_PLANE] = getFarPlane();
		int i;
		for (i = 0; i < E_PLANE_COUNT; i++)
		{
			const hd::plane3df& p = planes[i];
			if(p.classifyPointRelation(point) == hd::ISREL3D_FRONT)
				return false;
		}
		return true;

	}

	CHd3dBoxEx::~CHd3dBoxEx(void)
	{
	}
}

