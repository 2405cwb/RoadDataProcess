
/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdDataDriver
文件名		：Hdtfw.cpp
相关文件	: 
文件实现功能：解析tfw文件
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/5/14	1.0			马振明		创建
</PRE>
******************************************************************************************************/

#include "StdAfx.h"
#include "ChdTfw.h"
#include <stdio.h>

using namespace hd::scene;

ChdTfw::ChdTfw(void)
{
}


ChdTfw::~ChdTfw(void)
{
}

// 读数据tfw数据,double* dProj的数量一定要是6，也要保证顺序,详细格式参见头文件
bool ChdTfw::Read(const char* strPath,double* dProj)
{
	if (!strPath || !dProj)
	{
		return false;
	}

	FILE* pFile =NULL;

	pFile = fopen(strPath,"rt");
	
	try
	{
		int nCount =0;
		while(!feof(pFile))
		{
			char strBuf[30];		// 保存每一行字符
			memset(strBuf,0,30);
			fgets(strBuf,1024,pFile);
			sscanf(strBuf, "%lf",&dProj[nCount]);
			nCount++;
		}
	}
	catch (...)
	{
		fclose(pFile);
		return false;
	}

	fclose(pFile);
	return true;
}

// 写tfw数据，double* dProj的数量一定要是6，也要保证顺序
bool ChdTfw::Write(const char* strPath,double* dProj)
{
	if (!strPath || !dProj)
	{
		return false;
	}

	FILE* pFile =NULL;
	pFile = fopen(strPath,"w+");
	try
	{
		if (pFile)
		{
			fprintf_s(pFile,"%lf\n%lf\n%lf\n%lf\n%lf\n%lf",dProj[0],dProj[1],dProj[2],dProj[3],dProj[4],dProj[5]);
		}
	}
	catch (...)
	{
		fclose(pFile);
		return false;
	}
	fclose(pFile);

	return true;
}