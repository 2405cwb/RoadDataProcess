/*! HdGridObject.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HdGridObject.h
相关文件     : HdGridObject.cpp
文件实现功能 : 栅格数据对象
作者         : 张阳
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/01/18   1.0      张  阳                新建
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "hdCommon.h"
#include "hdobject.h"
#include "..\..\..\3rd\GDAL\Include\ogrsf_frmts.h"
#include "..\..\..\3rd\GDAL\Include\ogr_api.h"
namespace hd
{
class HDCOMMON_API CHdGridObject :
	public CHDObject
{
public:
	CHdGridObject(void);
	~CHdGridObject(void);

	// 获取类型
	virtual ENUM_HDMS_OBJECT_TYPE GetType() const {return ESDT_OBJECT_GRID;}
	
	//获取文件名
	std::string GetFileName() const;

	// 获取文件扩展
	std::string GetFileExt() const;

	// 获取文件路径
	std::string GetFilePath() const;

    // 设置文件路径
    void SetFilePath(std::string strFilePath);
private:
	std::string m_strPath; // 文件路径
};
}