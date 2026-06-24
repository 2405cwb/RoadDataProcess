/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdPointSet.cpp
相关文件	: HdPointSet.h
文件实现功能：点集
作者		：蔡红云
版本		：1.0
-------------------------------------------------------
备注：       用于导入控制点
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/09/28	1.0			蔡红云		创建
</PRE>
******************************************************************************************************/

#include "StdAfx.h"
#include "HdPointSet.h"
#include "..\hdCore\hdString.h"
namespace hd
{

	CHdPointSet::CHdPointSet(void)
	{
		
	}

	CHdPointSet::~CHdPointSet(void)
	{
		m_vecPoint.clear();
	}

	 //获取节点
	const HD_CtrlPoint* CHdPointSet::GetPoint(int index) const
	{
		if (index >=0 && index <=m_vecPoint.size()-1)
		{
			return &m_vecPoint[index];
		}
		return NULL;
	}

	//  获取节点个数
	size_type CHdPointSet::GetSize() const
	{
		return m_vecPoint.size();
	}

	//--增加节点
	void CHdPointSet::AddPoint(const HD_CtrlPoint& pt)
	{		
		m_vecPoint.push_back(pt);
	}

	// --增加节点 名字、坐标
	void CHdPointSet::AddPoint(const char* name,double x,double y,double z)
	{			
		m_vecPoint.push_back(HD_CtrlPoint(name,x,y,z));
	}

	// 打开文件用于初始化控制点集合
	bool CHdPointSet::Open(const char* filePath)
	{
		FILE* pFile = fopen(filePath,"rt");

		if (!pFile) // 打开失败返回
		{
			return false;
		}

		const int maxLine = 512;// 一行字符数

		char line[maxLine]; //一行数据

		// 坐标
		double xyz[3] ={0.0};
		
		std::vector<stringc> vecstring;

		char strName[64];

		while (fgets(line, sizeof(char) * maxLine, pFile))
		{
			if(strstr(line,",")) // 以逗号分割
			{
				vecstring.clear();

				// 分割字符、解析字段 
				stringc stringtmp(line);
				stringtmp.split(vecstring,",");

				//字段不为4个返回
				if(vecstring.size()!=4)
				{
					continue;
				}

				// 获取坐标值
				xyz[0] = atof(vecstring[0].c_str());
				xyz[1] = atof(vecstring[1].c_str());
				xyz[2] = atof(vecstring[2].c_str());

				// 增加节点
				AddPoint(vecstring[0].c_str(),xyz[0],xyz[1],xyz[2]);
				
			}
			else // 以空格分割
			{				 
				sscanf(line,"%s%lf%lf%lf",strName,&xyz[0],&xyz[1],&xyz[2]);

				// 增加节点
				AddPoint(strName,xyz[0],xyz[1],xyz[2]);
			}

		}

		// 关闭文件
		fclose(pFile);

		// 更新文件名
		m_strPath = std::string(filePath);

		return true;
	}

	// 获取文件名
	std::string CHdPointSet::GetFileName()
	{
		char dirive[256] = {0};
		char dir[256] = {0};
		char fileName[256]   = {0};
		char ext[256]    = {0};

		// 分割文件
		::_splitpath(m_strPath.c_str(),dirive,dir,fileName,ext);

		return std::string(fileName);
				
	}
}
