#pragma once
#include "stdafx.h"

namespace hnCommon
{
	template <class T>
	class hn2dPointWithMile
	{
	public:
		hn2dPointWithMile() {
			x = 0; y = 0; m_dmi = 0; m_time
				= 0;
		}
		hn2dPointWithMile(T nx, T ny, T dmi,double time)
		{
			x = nx;
			y = ny;
			m_dmi = dmi;
			m_time = time;
		}

		hn2dPointWithMile(const hn2dPointWithMile<T>& other)
		{
			x = other.x;
			y = other.y;
			m_dmi= other.m_dmi;
			m_time = other.m_time;
		}

		hn2dPointWithMile<T>& operator=(const hn2dPointWithMile<T>& other)
		{
			x = other.x;
			y = other.y;
			m_dmi = other.m_dmi;
			m_time = other.m_time;
			return *this;
		}

	public:
		// x坐标
		T x;

		// y坐标
		T y;

		//
		T m_dmi;
		
		//时间
		double m_time;
	};

	//定义double对象
	typedef hn2dPointWithMile<double> hn2dPointWithMileD;

	//定义float对象
	typedef hn2dPointWithMile<float> hn2dPointWithMileF;

	// 定义int对象
	typedef hn2dPointWithMile<int> hn2dPointWithMileI;

}

