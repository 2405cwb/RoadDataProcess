/*! CHdGridObject.cpp
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : CHdGridObject.cpp
相关文件     : CHdGridObject.h
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

#include "StdAfx.h"
#include "HdGridObject.h"
#include "ogr_api.h"
#include <algorithm>
namespace hd
{
CHdGridObject::CHdGridObject(void)
{
}


CHdGridObject::~CHdGridObject(void)
{
}

std::string CHdGridObject::GetFileName() const
{
	char driver[256] = {0};
	char dir[256] = {0};
	char fileName[256] = {0};
	char ext[256] = {0};

	//分割文件
	::_splitpath(m_strPath.c_str(),driver,dir,fileName,ext);

	return std::string(fileName);

}

std::string CHdGridObject::GetFileExt() const
{
	char driver[256] = {0};
	char dir[256] = {0};
	char fileName[256] = {0};
	char ext[256] = {0};

	//分割文件
	::_splitpath(m_strPath.c_str(),driver,dir,fileName,ext);
	std::string fileExt(ext);
	std::transform(fileExt.begin(),fileExt.end(),fileExt.begin(),tolower);
	return fileExt;
}

std::string CHdGridObject::GetFilePath() const
{
	return m_strPath;
}

// 设置文件路径
void CHdGridObject::SetFilePath(std::string strFilePath)
{
    m_strPath = strFilePath;
}
}