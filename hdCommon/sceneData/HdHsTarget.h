/*!HdHsTarget.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdHsTarget.h
相关文件     : HDObject.h 

文件实现功能 : 定义HS小标靶
作者         : 蔡红云
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2017/02/23   1.0      蔡红云				创建
</PRE>
*******************************************************************************/
#pragma once
#include "HdFeaturePoint.h"
namespace hd
{
	class CHdHsTarget: public CHdFeaturePoint
	{
	public:
		CHdHsTarget(void)
		{
			m_strName = "HsTar";
		}
		CHdHsTarget(const char* name):CHdFeaturePoint(name)
		{
			
		}

		~CHdHsTarget(void)
		{

		}

		// 获取类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_HS_TARGET; }

	};
}
