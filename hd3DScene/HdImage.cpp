#include "stdafx.h"
#include "HdImage.h"

using namespace hd::scene;

CHdImage::CHdImage()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_pData = NULL;
	m_bOwnData = true;
}
CHdImage::CHdImage(unsigned int nWidth, unsigned int nHeight, EHdColorFormat eColorFormat)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_eColorFormat = eColorFormat;
	m_pData = NULL;
	m_bOwnData = true;
	if (eColorFormat == EHD_CF_A8R8G8B8)
	{
		m_pData = new unsigned char[nWidth * nHeight * 4];
	}
}
CHdImage::CHdImage(unsigned int nWidth, unsigned int nHeight, EHdColorFormat eColorFormat, unsigned char* pData, bool bOwnData)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_eColorFormat = eColorFormat;
	m_pData = pData;
	m_bOwnData = bOwnData;
}
CHdImage::CHdImage(const CHdImage& anotherImage)
{
	m_nWidth = anotherImage.GetWidth();
	m_nHeight = anotherImage.GetHeight();
	m_eColorFormat = anotherImage.GetColorFormat();
	unsigned int nSize = anotherImage.GetLineSpace() * m_nHeight;
	m_pData = new unsigned char[nSize];
	memcpy(m_pData, anotherImage.GetImageData(), nSize);
	m_bOwnData = true;
}
CHdImage& CHdImage::operator=(const CHdImage& anotherImage)
{
	m_nWidth = anotherImage.GetWidth();
	m_nHeight = anotherImage.GetHeight();
	m_eColorFormat = anotherImage.GetColorFormat();
	unsigned int nSize = anotherImage.GetLineSpace() * m_nHeight;
	m_pData = new unsigned char[nSize];
	memcpy(m_pData, anotherImage.GetImageData(), nSize);
	m_bOwnData = true;

	return *this;
}
CHdImage::~CHdImage()
{
	if (m_bOwnData)
	{
		delete []m_pData;
		m_pData = NULL;
	}
}
unsigned int CHdImage::GetWidth() const
{
	return m_nWidth;
}
unsigned int CHdImage::GetHeight() const
{
	return m_nHeight;
}
EHdColorFormat CHdImage::GetColorFormat() const
{
	return m_eColorFormat;
}
unsigned char* CHdImage::GetImageData() const
{
	return m_pData;
}
unsigned char* CHdImage::GetScanLineData(unsigned int nScanLine) const
{
	return m_pData + GetLineSpace() * nScanLine;
}
unsigned int CHdImage::GetPixelSpace() const
{
	unsigned int nPixelSpace = 0;
	if (m_eColorFormat == EHD_CF_R8G8B8)
	{
		nPixelSpace = 3;
	}
	else if (m_eColorFormat == EHD_CF_A8R8G8B8)
	{
		nPixelSpace = 4;
	}
	return nPixelSpace;
}
unsigned int CHdImage::GetLineSpace() const
{
	unsigned int nLineSpace = 0;
	if (m_eColorFormat == EHD_CF_R8G8B8)
	{
		nLineSpace = (m_nWidth * 3 + 3) / 4 * 4;
	}
	else if (m_eColorFormat == EHD_CF_A8R8G8B8)
	{
		nLineSpace = m_nWidth * 4;
	}
	return nLineSpace;
}

