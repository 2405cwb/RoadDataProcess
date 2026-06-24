#pragma once
#include "stdafx.h"
#include "hn3dPointDef.h"
namespace hnCommon
{
	template <class T>
	class hn3dPointWithMile
	{
	public:
		hn3dPointWithMile() { x = 0; y = 0; z = 0.0; bottomEncoderMile = 0; }
		hn3dPointWithMile(T nx, T ny, T nz,T mile)
		{
			x = nx;
			y = ny;
			z = nz;
			bottomEncoderMile = mile;
		}

		hn3dPointWithMile(const hn3dPointWithMile<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			bottomEncoderMile = other.bottomEncoderMile;
		}

		hn3dPointWithMile<T>& operator=(const hn3dPointWithMile<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			bottomEncoderMile = other.bottomEncoderMile;

			return *this;
		}

	public:
		// x坐标
		T x;

		// y坐标
		T y;

		// z坐标
		T z;

		//图片底部编码器里程 3d是0 8 16 24。。。
		T bottomEncoderMile;
	};

	//定义double对象
	typedef hn3dPointWithMile<double> hn3dPointWithMileD;

	//定义float对象
	typedef hn3dPointWithMile<float> hn3dPointWithMileF;

	// 定义int对象
	typedef hn3dPointWithMile<int> hn3dPointWithMileI;

}



