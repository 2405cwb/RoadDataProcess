#include "StdAfx.h"
#include "HDPanoSymbol.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
namespace hd
{
	CHDPanoSymbol::CHDPanoSymbol(void)
	{
		memset(m_strName, 0, NAME_LEN);
		memset(m_strUrl, 0, MAX_PATH);
		m_nType = 0;
	}

	CHDPanoSymbol::CHDPanoSymbol(char* strName, int nType, char* strUrl)
		:m_nType(nType)
	{
		strcpy(m_strName, strName);
		strcpy(m_strUrl, strUrl);
	}

	CHDPanoSymbol::~CHDPanoSymbol(void)
	{
	}

	const char* CHDPanoSymbol::GetName() const
	{
		return m_strName;
	}

	int CHDPanoSymbol::GetSymbolType() const
	{
		return m_nType;
	}

	const char* CHDPanoSymbol::GetUrl() const
	{
		return m_strUrl;
	}

	void CHDPanoSymbol::SetName(const char* strName)
	{
		strcpy(m_strName, strName);
	}

	void CHDPanoSymbol::SetSymbolType(int nType)
	{
		m_nType = nType;
	}

	void CHDPanoSymbol::SetUrl(const char* strUrl)
	{
		strcpy(m_strUrl, strUrl);
	}
}