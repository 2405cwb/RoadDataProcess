#ifndef __HD_DISTANCEIMG_H_INCLUDED__
#define __HD_DISTANCEIMG_H_INCLUDED__

#include "..\hdHLSlib\inc\mydefs.hpp"

namespace hd
{
	//生成彩色深度图
	class CDistanceImg
	{
	private:
		float m_dmin;		//所有点到原点的最小距离
		float m_dmax;		//所有点到原点的最大距离
	public:
		CDistanceImg(){}
		CDistanceImg(float dmin,float dmax)
			:m_dmin(dmin),m_dmax(dmax){}

		~CDistanceImg(void){}
	public:
		inline void setMinMax(float dmin,float dmax)
		{
			m_dmin = dmin;
			m_dmax = dmax;
		}
	private:
		inline void GetRGB4IntDist(int dist, U32 &r,U32 &g,U32 &b)
		{
				b = dist & 0xff;
				g = (dist >> 8 ) & 0xff;
				r = (dist >> 16) & 0xff;
		}
		inline void GetIntDist4RGB(int &dist, U32 r,U32 g,U32 b)
		{
			dist = (r << 16) | (g << 8) | b;//r*256*256 + g*256 + b;
		}
		inline float Normalize(float d)
		{
			return (d - m_dmin)/(m_dmax - m_dmin);
		}
		inline float InvNormalize(float d)
		{
			return d*(m_dmax - m_dmin) + m_dmin;
		}

	public:
		//! 根据距离返回r,g,b颜色值,r,g,b颜色范围:0-255
		inline void GetRGBByDist(float distance,U32& r,U32& g,U32& b)
		{
			float nx = Normalize(distance)*16777215;
			int ix = (int)nx;
			GetRGB4IntDist( ix, r, g, b);
		}
		//! 根据距离返回r,g,b颜色值,r,g,b颜色范围:0.0f-1.0f
		inline void GetRGB3FByDist(float distance,F32& r,F32& g,F32& b)
		{
			float nx = Normalize(distance)*16777215;
			int ix = (int)nx;
			U32 ir,ig,ib;
			GetRGB4IntDist( ix, ir, ig, ib);
			r = ir * ColorRCP;
			g = ig * ColorRCP;
			b = ib * ColorRCP;
		}
	};
}

#endif