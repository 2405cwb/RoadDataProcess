#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include <math.h>
namespace hd
{

	// 定义二维点
	struct HDCOMMON_API HD_2DPOINT
	{
		double X;
		double Y;

		HD_2DPOINT()
		{
			X = 0;
			Y = 0;
		}

		HD_2DPOINT(double x, double y)
		{
			X = x;
			Y = y;
		}

		HD_2DPOINT& operator=(const HD_2DPOINT& pt)
		{
			X = pt.X;
			Y = pt.Y;
			return *this;
		}
	};

	// 定义三维点
	struct HDCOMMON_API HD_3DPOINT
	{
		double X;
		double Y;
		double Z;

		HD_3DPOINT()
		{
			X = 0;
			Y = 0;
			Z = 0;
		}

		HD_3DPOINT(double x, double y, double z)
		{
			X = x;
			Y = y;
			Z = z;
		}

		HD_3DPOINT& operator=(const HD_3DPOINT& pt)
		{
			X = pt.X;
			Y = pt.Y;
			Z = pt.Z;
			return *this;
		}

		bool operator ==(const HD_3DPOINT& pt)
		{
			// 相等
			return ((this->X == pt.X) && (this->Y == pt.Y) && (this->Z == pt.Z));
		}

		bool operator !=(const HD_3DPOINT& pt)
	   {
			// 相等
			if ((fabs(this->X - pt.X) < 0.00000001)  && (fabs(this->Y - pt.Y) < 0.00000001) && (fabs(this->Z - pt.Z) < 0.00000001))
			{
				return false;
			}
			return true;
	   }
	   void operator *(const double d)
    	{
	         this->X *= d;
	         this->Y *= d;
	         this->Z *= d;
       }
	};

	// 定义带法向量的点
	struct HDCOMMON_API HD_3DNORMALPOINT
	{
		HD_3DPOINT pos;
		HD_3DPOINT normal;
	};

	// 定义三维多边形
	struct HDCOMMON_API HD_3DPOINT_ARRAY
	{
		int nVertCount;
		HD_3DPOINT* pVertexs;

		HD_3DPOINT_ARRAY()
		{
			nVertCount = 0;
			pVertexs = NULL;
		}

		HD_3DPOINT_ARRAY(int n)
		{
			pVertexs = new HD_3DPOINT[n];
			nVertCount = n;
		}

		~HD_3DPOINT_ARRAY()
		{
			if(pVertexs)
			{
				delete[] pVertexs;
				pVertexs = NULL;
			}
		}
	};

	struct HDCOMMON_API HD_OBJECT_ARRAY
	{
		int nObjectCount;
		char* pObjectID;	// 对象ID，每OBJECT_ID_LEN个char表示一个对象ID

		HD_OBJECT_ARRAY()
		{
			nObjectCount = 0;
			pObjectID = NULL;
		}

		//构造函数
		HD_OBJECT_ARRAY(int _nCount)
		{
			nObjectCount = _nCount;
			pObjectID = new char[nObjectCount*OBJECT_ID_LEN];
		}

		//析构函数
		~HD_OBJECT_ARRAY()
		{
			if (pObjectID)
			{
				delete [] pObjectID;
				pObjectID = NULL;
			}
		}
	};
}
