#ifndef _HN_2D_POINT_DEF_
#define _HN_2D_POINT_DEF_
#include "stdafx.h"

namespace hnCommon
{
	template <class T>
	class hn2dPoint
	{
	public:
		hn2dPoint() { x = 0; y = 0; }
		hn2dPoint(T nx, T ny)
		{
			x = nx;
			y = ny;
		}

		hn2dPoint(const hn2dPoint<T>& other)
		{
			x = other.x;
			y = other.y;
		}

		hn2dPoint<T>& operator=(const hn2dPoint<T>& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}

	public:
		// x坐标
		T x;

		// y坐标
		T y;
	};

	//定义double对象
	typedef hn2dPoint<double> hn2dPointD;

	//定义float对象
	typedef hn2dPoint<float> hn2dPointF;

	// 定义int对象
	typedef hn2dPoint<int> hn2dPointI;

}



#endif // !_HN_2D_POINT_DEF

