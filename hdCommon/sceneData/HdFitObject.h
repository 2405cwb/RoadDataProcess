/*! HdFitObject.h
********************************************************************************
<PRE>
模块名       : hdWorkspace
文件名       : HdFitObject.h
相关文件     :

文件实现功能 : 定义内存对象基类（拟合对象）
作者         : 杨峰
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/7/4     1.0      杨峰    
2013/03/07   1.1      龚书林
</PRE>
*******************************************************************************/
#pragma once
#include "HdFileData.h"
#include "..\..\hdCore\tinyxml.h"

namespace hd
{
	// 拟合对象的状态
	enum EHD_FIT_OBJECT_STATE
	{
		EFOS_OK,			//正常状态
		EFOS_POK,			//
		EFOS_NOK			//错误状态
	};

	class CHdFitObject :
		public CHDObject
	{
	public:
		CHdFitObject(void){};

	public:
		//得到拟合对象状态
		virtual EHD_FIT_OBJECT_STATE GetState() const = 0;

		// 将数据从XML文件中解析到内存，或者将数据从内存保存到XML文件中
		//param element，	数据保存在此元素的子元素下
		//param bSave,		指示序列化的方向，是保存到xml文件，还是从文件中解析到内存。当为真值时，保存到xml文件
		virtual void Serialize(TiXmlElement* element, bool bSave) = 0;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_OBJECT_FIT; }
	};
}
