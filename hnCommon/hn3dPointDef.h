#ifndef _HN_3D_POINT_DEF_
#define _HN_3D_POINT_DEF_
#include "stdafx.h"

namespace hnCommon
{
	template <class T>
	class hn3dPoint
	{
	public:
		hn3dPoint() { x = 0; y = 0; z = 0.0; }
		hn3dPoint(T nx, T ny, T nz)
		{
			x = nx;
			y = ny;
			z = nz;
		}

		hn3dPoint(const hn3dPoint<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
		}

		hn3dPoint<T>& operator=(const hn3dPoint<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			return *this;
		}

	public:
		// x坐标
		T x;

		// y坐标
		T y;

		// z坐标
		T z;
	};

	//定义double对象
	typedef hn3dPoint<double> hn3dPointD;

	//定义float对象
	typedef hn3dPoint<float> hn3dPointF;

	// 定义int对象
	typedef hn3dPoint<int> hn3dPointI;

}



#endif // !_HN_2D_POINT_DEF

