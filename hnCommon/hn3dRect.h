#pragma once
#include "stdafx.h"
#include "hn3dPointWithMile.h"

namespace hnCommon
{
	template <class T>
	class hn3dRectWithMile
	{

	public:
		hn3dRectWithMile() {  }

		hn3dRectWithMile(T np0, T np1, T np2, T np3)
		{
			p0 = np0;
			p1 = np1;
			p2 = np2;
			p3 = np3;
		}

		hn3dRectWithMile(const hn3dRectWithMile<T>& other)
		{
			p0 = other.p0;
			p1 = other.p1;
			p2 = other.p2;
			p3 = other.p3;
		}

		hn3dRectWithMile<T>& operator=(const hn3dRectWithMile<T>& other)
		{
			p0 = other.p0;
			p1 = other.p1;
			p2 = other.p2;
			p3 = other.p3;
			return *this;
		}

	public:
		// 左上角
		T p0;

		// 右上角
		T p1;
		// 右下角
		T p2;
		// 左下角
		T p3;
	};

	//定义double对象
	typedef hn3dRectWithMile<hn3dPointWithMileD> hn3dRectD;

	//定义float对象
	typedef hn3dRectWithMile<hn3dPointWithMileF> hn3dRectF;

	// 定义int对象
	typedef hn3dRectWithMile<hn3dPointWithMileI> hn3dRectI;

}



