#pragma once
 
namespace MyQtCommon
{
	typedef  struct _HN_POINT_
	{
		_HN_POINT_()
		{
			px = 0;
			py = 0;
		}
		_HN_POINT_(const _HN_POINT_& val)
		{
			px = val.px;
			py = val.py;
		}
		_HN_POINT_(int valx, float valy)
		{
			px = valx;
			py = valy;
		}
		
		int px;
		float py;
	}MyPoint;
}
