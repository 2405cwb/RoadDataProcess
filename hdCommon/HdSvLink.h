/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdSvRoute
文件名		：HdSvlink.h
相关文件	: HdRoute.h, ScanRoute.h
文件实现功能：连接关系类 	  
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：该类只是街景使用，添加了指针计数器，类似于智能指针
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
[6/11/2014]	1.0			马振明		  创建
</PRE>
******************************************************************************************************/
#pragma once
#include "HDObject.h"
#include "hdSvDBStructDef.h"

namespace hd
{
	class HDCOMMON_API CHdSvLink : public CHDObject
	{
	public:
		CHdSvLink(void);
		~CHdSvLink(void);
		CHdSvLink(const HD_SV_LINK& pLink);
		// 默认构造函数
		CHdSvLink(const CHdSvLink& link);

		// 得到对象的类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return E_HOT_SVLINK; }

		// 设置邻接关系数据
		void SetLink(HD_SV_LINK* pLink)
		{
			// 判断是否为空
			if (NULL == pLink )
			{
				return;
			}

			// 判断是否自我赋值
			if (pLink!=m_pLink)
			{
				m_pLink = pLink;
			}

			m_pCount = new int(1);
		}

		// 设置邻接关系数据
		void SetLink(const HD_SV_LINK& link);

		// 获取指针
		const HD_SV_LINK* const GetLink() const
		{
			return m_pLink;
		}

		// 赋值操作符
		CHdSvLink& operator=(const CHdSvLink&);
		
		// 获取起点影像名称
		char* GetSrcImageName() const
		{
			return m_pLink->strSrcImageName;
		}

		// 获取影像终点名称
		char* GetDstImageName() const
		{
			return m_pLink->strDstImageName;
		}

		// 获取正向名称
		char* GetStrPostiveName() const
		{
			return m_pLink->strPostiveName;
		}

		// 获取反向名称
		char* GetNegativeName() const
		{
			return m_pLink->strNegativeName;
		}

		// 获取坐标
		double* GetBL() const
		{
			return m_pLink->dBL;
		}
		
		// 获取序列ID
		int GetNID() const
		{
			return m_pLink->nID;
		}

		// 获取字符串ID
		char* GetStrID() const
		{
			return m_pLink->strLinkID;
		}

		// 设置字符串ID
		CHdSvLink& SetStrID(const char*);

		// 设置序列ID
		CHdSvLink& SetNID(int nID);


		// 设置起点影像名称
		CHdSvLink& SetSrcImageName(const char*);

		// 设置终点影像名称
		CHdSvLink& SetDstImageName(const char*);

		// 设置正向名称
		CHdSvLink& SetPostiveName(const char*);

		// 设置反向
		CHdSvLink& SetNegativeName(const char*);

		// 设置坐标
		CHdSvLink& SetBL(double dB0,double dL0,double dB1,double dL1);
		
	private:
		HD_SV_LINK* m_pLink;			// 邻接关系数据
		int* m_pCount;					// 指针计数器
	};
}


