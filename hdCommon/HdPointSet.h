/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：HdPointSet.h
相关文件	: HdPointSet.cpp
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
#pragma once

#include "HD3DPoint.h"
#include <vector>
#include<string>
namespace hd
{
	class HD_CtrlPoint  //控制点辅助类
	{
		public:
		HD_CtrlPoint(const char* inName, double x,double y,double z)
		{			
			strcpy(m_name,inName);
			m_pPoint.m_x = x;
			m_pPoint.m_y = y;
			m_pPoint.m_z = z;
		}
		       
		~HD_CtrlPoint() 
		{
		}
        
        CHD3DPoint m_pPoint; // 点坐标
		char m_name[64];// 名字

	};

	class HDCOMMON_API CHdPointSet:public CHDObject
	{
	public:
		CHdPointSet();
		virtual ~CHdPointSet();

	public:

		// 返回类型
		ENUM_HDMS_OBJECT_TYPE GetType() const 
		{
			return ESDT_OBJECT_CONTROLPOINTSET;
		}
	
		// 获取节点
		const HD_CtrlPoint* GetPoint(int index) const;

		//  获取节点个数
		size_type GetSize() const;
		
		//--增加节点
		void AddPoint(const HD_CtrlPoint& pt);

		// name--名字 x、y、z绝对坐标
		void AddPoint(const char* name,double x,double y,double z);

		// 打开文件用于初始化控制点集合
		bool Open(const char* filePath);

		// 获取文件名
		std::string GetFileName();
						
	private:

		std::vector<HD_CtrlPoint> m_vecPoint; // 点集合

		std::string m_strPath; // 控制点文件路径
	};
}



