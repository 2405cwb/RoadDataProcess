#ifndef _HN_3D_POINTXYZ_DEF_
#define _HN_3D_POINTXYZ_DEF_
#include "stdafx.h"

namespace hnCommon
{
	template <class T>
	class hnPointXYZI
	{
	public:
		hnPointXYZI() { x = 0; y = 0; z = 0; intensity = 0.0; }
		hnPointXYZI(T nx, T ny, T nz, float nT)
		{
			x = nx;
			y = ny;
			z = nz;
			intensity = nT;
		}

		hnPointXYZI(const hnPointXYZI<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			intensity = other.intensity;
		}

		hnPointXYZI<T>& operator=(const hnPointXYZI<T>& other)
		{
			x = other.x;
			y = other.y;
			z = other.z;
			intensity = other.intensity;
			return *this;
		}

	public:
		// x坐标
		T x;

		// y坐标
		T y;

		// z坐标
		T z;

		// 强度
		float intensity;
	};

	//定义double对象
	typedef hnPointXYZI<double> hnPointXYZID;

	//定义float对象
	typedef hnPointXYZI<float> hnPointXYZIF;

}



#endif // !_HN_3D_POINTXYZ_DEF_

