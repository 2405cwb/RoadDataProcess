#pragma once
#include "stdafx.h"

namespace hn
{
#pragma pack(push, 1)
	typedef struct HN_POINT_3D
	{
		HN_POINT_3D()
		{
			x = 0;
			y = 0;
			z = 0;
		}

		bool isValid()
		{
			if (x == 0.0 && y == 0.0 && z == 0.0)
			{
				return false;
			}

			return true;
		}

		float x;
		float y;
		float z;
	}hnPoint3d;

#pragma pack(pop)
}

