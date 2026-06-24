#ifndef _HN_POINTXYZI_
#define _HN_POINTXYZI_
#include "stdafx.h"

#pragma pack(push, 1)
namespace hnCommon
{
	enum ENUM_POINTTYPE
	{
		HNPCD_XY,				// XY属性
		HNPCD_XYZ,				// XYZ属性
		HNPCD_INTENSITY,		// 强度属性
		HNPCD_RGB,				// RGB属性
	};

	// 包含x,y,z,intensity,r,g,b属性的点
	struct hnPointXYZIRGB
	{
		float x, y, z;		// 笛卡尔坐标
		float intensity;		// 反射强度
		int b;
		int g;
		int r;

		hnPointXYZIRGB()
			:x(0.0f), y(0.0f), z(0.0f), intensity(0.0), b(0), g(0), r(0) {}

		hnPointXYZIRGB(double _dx, double _dy, double _dz)
			:intensity(0.0), b(0), g(0), r(0)
		{
			x = (float)_dx;
			y = (float)_dy;
			z = (float)_dz;
		}

		hnPointXYZIRGB(double _dx, double _dy)
		{
			x = (float)_dx;
			y = (float)_dy;
		}

		int getType() { return HNPCD_XYZ | HNPCD_INTENSITY | HNPCD_RGB; }

		bool operator< (const struct hnPointXYZIRGB& rhs) const
		{
			return getIntensity() < rhs.getIntensity();
		}

		bool operator> (const struct hnPointXYZIRGB& rhs) const
		{
			return getIntensity() > rhs.getIntensity();
		}
		inline bool isValid()
		{
			return !(x == 0.0f && y == 0.0f && z == 0.0f);
		}
		inline bool isValid(int min, int max)
		{
			return getIntensity() >= min && getIntensity() <= max;
		}

		// 获取反射强度
		inline float getIntensity()const { return intensity; }
	};

	//极坐标点 
	typedef struct _HNPOLARCOORDPT_
	{
		_HNPOLARCOORDPT_()
		{
			dist = 0.0;
			angle = 0.0;
			intensity = 0.0;
		}

		_HNPOLARCOORDPT_(double dDist,
			double dAngle, double dintensity)
		{
			dist = dDist;
			angle = dAngle;
			intensity = dintensity;
		}

		// 强度
		double intensity;

		// 距离
		double dist;

		//角度为0-360°
		double angle;

	}hnPolarCoordPt;

}

#pragma pack(pop)

#endif // 
