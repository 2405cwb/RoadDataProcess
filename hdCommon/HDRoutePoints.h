/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HDRoutePoints.h
相关文件	: HDObject.h	
文件实现功能：轨迹点对象，从CHDObject继承。
作者		：危迟
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2013/03/19	1.0			危迟		创建
</PRE>
******************************************************************************************************/
#pragma once
#include "HDObject.h"
#include "hdHdiStruct.h"
#include <vector>

using namespace std;

namespace hd
{
	class CHdRoutePoints : public CHDObject
	{
	private:
		vector<HD_HDIINFO>* m_hdiVec;

	public:
		CHdRoutePoints(void)
			:m_hdiVec(NULL) {}
		void SetRoutePoints(vector<HD_HDIINFO>* pHdiVec) { m_hdiVec = pHdiVec; }
		u32 GetPointsCount() const
		{ 
			if (m_hdiVec)
			{
				return m_hdiVec->size();
			}
			else
			{
				return 0;
			}
		}
		void GetPoint(int i,double& X,double& Y,double& Z) const
		{
			if (m_hdiVec == NULL || m_hdiVec->size() <= 0 || i >= (int)m_hdiVec->size())
			{
				return;
			}
			const HD_HDIINFO& hdi = *(m_hdiVec->_Myfirst + i);
			X = hdi.dX;
			Y = hdi.dY;
			Z = hdi.dZ;
		}
		virtual ~CHdRoutePoints(void) {}
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_ROUTEPOINTS; }
	};
}