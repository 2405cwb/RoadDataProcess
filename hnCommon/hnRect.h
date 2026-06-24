#pragma once
#include "stdafx.h"
#include "hn2dPointWithMile.h"

namespace hnCommon
{
	template <class T>
	class hnRectWithMile
	{
	
	public:
		hnRectWithMile() {  }
		
		hnRectWithMile(T np0, T np1,T np2,T np3)
		{
			p0 = np0;
			p1 = np1;
			p2 = np2;
			p3 = np3;
		}

		hnRectWithMile(const hnRectWithMile<T>& other)
		{
			p0 = other.p0;
			p1 = other.p1;
			p2 = other.p2;
			p3 = other.p3;
		}

		hnRectWithMile<T>& operator=(const hnRectWithMile<T>& other)
		{
			p0 = other.p0;
			p1 = other.p1;
			p2 = other.p2;
			p3 = other.p3;
			return *this;
		}

	public:// 顺时针
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
	typedef hnRectWithMile<hn2dPointWithMileD> hn2dRectD;

	//定义float对象
	typedef hnRectWithMile<hn2dPointWithMileF> hn2dRectF;

	// 定义int对象
	typedef hnRectWithMile<hn2dPointWithMileI> hn2dRectI;

}



