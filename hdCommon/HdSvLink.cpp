#include "StdAfx.h"
#include "HdSvLink.h"

namespace hd
{
	CHdSvLink::CHdSvLink(void)
		:m_pLink(new HD_SV_LINK)
		,m_pCount(new int(1))
	{

	}

	// 析构
	CHdSvLink::~CHdSvLink(void)
	{
		// 如果计数器变为0，释放内存
		if (--*m_pCount == 0)
		{
			delete m_pCount;
			delete m_pLink;
			m_pLink = NULL;
			m_pCount = NULL;
		}
	}

	// 自定义构造函数，实现HD_SV_LINK*到CHdSvLink*的隐式转换
	CHdSvLink::CHdSvLink(const HD_SV_LINK& pLink)
	{
		m_pCount =new int(1);
		m_pLink = new HD_SV_LINK();
		SetLink(pLink);
	}

	// 默认构造函数,增加计数器
	CHdSvLink::CHdSvLink(const CHdSvLink& link)
		:m_pCount(link.m_pCount)
		,m_pLink(link.m_pLink)
	{
		++*m_pCount;
	}

	// 设置邻接关系数据
	void CHdSvLink::SetLink(const HD_SV_LINK& link)
	{
		// 避免同值复制
		if (&link != m_pLink)
		{
			strcpy(m_pLink->strLinkID,link.strLinkID);
			strcpy(m_pLink->strSrcImageName,link.strSrcImageName);
			strcpy(m_pLink->strDstImageName,link.strDstImageName);
			strcpy(m_pLink->strPostiveName,link.strPostiveName);
			strcpy(m_pLink->strNegativeName,link.strNegativeName);
			for (int i=0;i<4;i++)
			{
				m_pLink->dBL[i] = link.dBL[i];
			}
		}
	}

	CHdSvLink& CHdSvLink::operator=(const CHdSvLink& link)
	{
		// 先增加计数器
		++*link.m_pCount;

		// 判断当前计数器
		if (--*m_pCount==0)
		{
			delete m_pCount;
			delete m_pLink;
		}

		m_pCount = link.m_pCount;
		m_pLink = link.m_pLink;
		return *this;
	}

	// 设置ID
	CHdSvLink& CHdSvLink::SetStrID(const char* strID)
	{
		strcpy(m_pLink->strLinkID,strID);
		return *this;
	}

	// 设置序列ID
	CHdSvLink& CHdSvLink::SetNID(int nID)
	{
		m_pLink->nID = nID;
		return *this;
	}

	// 设置起点影像名称
	CHdSvLink& CHdSvLink::SetSrcImageName(const char* strImageName)
	{
		strcpy(m_pLink->strSrcImageName,strImageName);
		return *this;
	}

	// 设置终点影像名称
	CHdSvLink& CHdSvLink::SetDstImageName(const char* strImageName)
	{
		strcpy(m_pLink->strDstImageName,strImageName);
		return *this;
	}

	// 设置正向名称
	CHdSvLink& CHdSvLink::SetPostiveName(const char* strName)
	{
		strcpy(m_pLink->strPostiveName,strName);
		return *this;
	}


	// 设置反向名称
	CHdSvLink& CHdSvLink::SetNegativeName(const char* strName)
	{
		strcpy(m_pLink->strNegativeName,strName);
		return *this;
	}

	// 设置坐标
	CHdSvLink& CHdSvLink::SetBL(double dB0,double dL0,double dB1,double dL1)
	{
		m_pLink->dBL[0] = dB0;
		m_pLink->dBL[1] = dL0;
		m_pLink->dBL[2] = dB1;
		m_pLink->dBL[3] = dL1;
		return *this;
	}
}

