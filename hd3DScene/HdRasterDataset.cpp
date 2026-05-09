#include "stdafx.h"
#include "HdRasterDataset.h"
#include "gdal_priv.h"

using namespace hd::scene;
using namespace std;

CHdRasterDataset::CHdRasterDataset()
{
	m_pGDALDataset = NULL;
	m_pSR = NULL;
	m_pGeoEnv = NULL;
	m_nReferenceCount = 0;
}
CHdRasterDataset::~CHdRasterDataset()
{
	GDALClose(m_pGDALDataset);
	delete m_pSR;
	delete m_pGeoEnv;
}
const CHdSpatialReference* CHdRasterDataset::GetSpatialReference()const
{
	if (!m_pGDALDataset)
	{
		return NULL;
	}
	return m_pSR;
}
const CHdGeoEnvelope* CHdRasterDataset::GetGeoEnvelope() const
{
	if (!m_pGDALDataset)
	{
		return NULL;
	}
	return m_pGeoEnv;
}
CHdRasterDataset* CHdRasterDataset::Open(const char* pcFileName, bool bOnlyRead)
{
	CHdRasterDataset* pHdDataset = NULL;
	GDALAccess gAccess = GA_ReadOnly;
	if (bOnlyRead = false)
	{
		gAccess = GA_Update;
	}
	GDALDataset* pDataset = (GDALDataset*)GDALOpen(pcFileName, gAccess);
	if (pDataset)
	{
		pHdDataset = new CHdRasterDataset;
		pHdDataset->m_pGDALDataset = pDataset;
		pHdDataset->m_strFullName = pcFileName;

		double pdfGeoTransform[6];
		pDataset->GetGeoTransform(pdfGeoTransform);

		double dfMinX, dfMaxX, dfMinY, dfMaxY;
		unsigned int nWidth = pDataset->GetRasterXSize();
		unsigned int nHeight = pDataset->GetRasterYSize();

		dfMinX = pdfGeoTransform[0];
		dfMaxX = pdfGeoTransform[0] + pdfGeoTransform[1] * nWidth + pdfGeoTransform[2] * nHeight;
		
		if (pdfGeoTransform[5] < 0)
		{
			dfMaxY = pdfGeoTransform[3];
			dfMinY = pdfGeoTransform[3] + pdfGeoTransform[4] * nWidth + pdfGeoTransform[5] * nHeight;
		}
		else
		{
			dfMinY = pdfGeoTransform[3];
			dfMaxY = pdfGeoTransform[3] + pdfGeoTransform[4] * nWidth + pdfGeoTransform[5] * nHeight;
		}
		pHdDataset->m_pGeoEnv = new CHdGeoEnvelope(dfMinX, dfMaxX, dfMinY, dfMaxY);

		pHdDataset->m_nReferenceCount = 0;
	}

	return pHdDataset;
}
void CHdRasterDataset::Close(CHdRasterDataset* pHdDataset)
{
	delete pHdDataset;
}
const char* CHdRasterDataset::GetDatasetFullName() const
{
	return m_strFullName.data();
}
int CHdRasterDataset::Reference()
{
	m_nReferenceCount ++;
	return m_nReferenceCount;
}
int CHdRasterDataset::Dereference()
{
	m_nReferenceCount --;
	return m_nReferenceCount;
}
unsigned int CHdRasterDataset::GetRasterWidth() const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	return m_pGDALDataset->GetRasterXSize();
}
unsigned int CHdRasterDataset::GetRasterHeight() const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	return m_pGDALDataset->GetRasterYSize();
}
unsigned int CHdRasterDataset::GetBandCount() const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	return m_pGDALDataset->GetRasterCount();
}
EHdRasterDataType CHdRasterDataset::GetBandDataType(int nBand) const
{
	if (!m_pGDALDataset)
	{
		return EHD_DT_Unknown;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return EHD_DT_Unknown;
	}
	GDALDataType gDataType = pBand->GetRasterDataType();
	if (gDataType <= 7)
	{
		return (EHdRasterDataType)gDataType;
	}
	else
	{
		return EHD_DT_Unknown;
	}
}
EHdColorInterp CHdRasterDataset::GetBandColorInterp(int nBand) const
{
	if (!m_pGDALDataset)
	{
		return EHD_CI_Unknown;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return EHD_CI_Unknown;
	}
	GDALColorInterp gColorInterp = pBand->GetColorInterpretation();
	if (gColorInterp <= 6)
	{
		return (EHdColorInterp)gColorInterp;
	}
	else
	{
		return EHD_CI_Unknown;
	}
}
bool CHdRasterDataset::GetNoDataValue(int nBand, double& dfNoDataValue) const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return 0;
	}
	int bSuccessed = 0;
	dfNoDataValue = pBand->GetNoDataValue(&bSuccessed);
	return bSuccessed > 0;
}
bool CHdRasterDataset::GetMinMaxValue(int nBand, double& dfMinValue, double& dfMaxValue) const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return 0;
	}
	double pdfMinMaxValue[2];
	CPLErr err = pBand->ComputeRasterMinMax(2, pdfMinMaxValue);
	dfMinValue = pdfMinMaxValue[0];
	dfMaxValue = pdfMinMaxValue[1];

	return err == CE_None;
}
CHdColorTable* CHdRasterDataset::GetColorTable() const
{
	return NULL;
}
int CHdRasterDataset::GetOverviewCount(int nBand) const
{
	if (!m_pGDALDataset)
	{
		return 0;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return 0;
	}
	return pBand->GetOverviewCount();
}
bool CHdRasterDataset::GetOverviewSize(int nBand, int nOverviewIndex, int& nWidth, int& nHeight)
{
	if (!m_pGDALDataset)
	{
		return false;
	}
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(nBand);
	if (!pBand)
	{
		return false;
	}
	GDALRasterBand* pOvBand = pBand->GetOverview(nOverviewIndex);
	if (!pOvBand)
	{
		return false;
	}
	nWidth = pOvBand->GetXSize();
	nHeight = pOvBand->GetYSize();
	return true;
}
bool CHdRasterDataset::BuildOverview(HdProgressFunc pProgressFunc)
{
	if (!m_pGDALDataset)
	{
		return false;
	}

	unsigned int nWidth = m_pGDALDataset->GetRasterXSize();
	unsigned int nHeight = m_pGDALDataset->GetRasterYSize();

	// 采样方式，如果存在颜色表，则使用最临近采样，否则使用平均值
	string strResample = "AVERAGE";
	if (GetColorTable())
	{
		strResample = "NEAREST";
	}

	// 金字塔级别
	int nOverviewCount = log((double)nWidth / 256) / log(2.0);
	if (nOverviewCount <= 0)
	{
		return false;
	}
	int* pnOverview = new int[nOverviewCount];
	pnOverview[0] = 2;
	for (int i = 1; i < nOverviewCount; i ++)
	{
		pnOverview[i] = pnOverview[i - 1] * 2;
	}

	// 为所有波段创建金字塔
	m_pGDALDataset->BuildOverviews(strResample.data(), nOverviewCount, pnOverview, 0, NULL, pProgressFunc, NULL);

	delete []pnOverview;

	return true;
}
void CHdRasterDataset::GetGeoTransform(double* pdfTransform) const
{
	if (!m_pGDALDataset)
	{
		return ;
	}
	m_pGDALDataset->GetGeoTransform(pdfTransform);
	//pdfTransform[5] = - abs(pdfTransform[5]);
}
void CHdRasterDataset::SetGeoTransform(double* pdfTransform)
{
	if (!m_pGDALDataset)
	{
		return ;
	}
	m_pGDALDataset->SetGeoTransform(pdfTransform);
}
void CHdRasterDataset::PixelToGeo(double& dfX, double& dfY)
{
	if (!m_pGDALDataset)
	{
		return;
	}
	double pdfGeoTransform[6];
	m_pGDALDataset->GetGeoTransform(pdfGeoTransform);
	double dfTempX = pdfGeoTransform[0] + pdfGeoTransform[1] * dfX + pdfGeoTransform[2] * dfY;
	double dfTempY = pdfGeoTransform[3] + pdfGeoTransform[4] * dfX + pdfGeoTransform[5] * dfY;
	dfX = dfTempX;
	dfY = dfTempY;
}
void CHdRasterDataset::GeoToPixel(double& dfX, double& dfY)
{
	if (!m_pGDALDataset)
	{
		return;
	}
	double pdfGeoTransform[6], pdfInvGeoTransform[6];
	m_pGDALDataset->GetGeoTransform(pdfGeoTransform);
	GDALInvGeoTransform(pdfGeoTransform, pdfInvGeoTransform);
	double dfTempX = pdfInvGeoTransform[0] + pdfInvGeoTransform[1] * dfX + pdfInvGeoTransform[2] * dfY;
	double dfTempY = pdfInvGeoTransform[3] + pdfInvGeoTransform[4] * dfX + pdfInvGeoTransform[5] * dfY;
	dfX = dfTempX;
	dfY = dfTempY;
}
bool CHdRasterDataset::Read(int nXOff, int nYOff, int nXSize, int nYSize, int nBandCount, int* pBandIndex,
	CHdRasterBlock* pRaster) const
{
	if (!m_pGDALDataset)
	{
		return false;
	}
	double pdfGeoTransform[6];
	GetGeoTransform(pdfGeoTransform);
	unsigned char* pData = pRaster->GetRasterData();
	EHdRasterDataType eDataType = pRaster->GetDataType();
	int nDataTypeSize = GetDataTypeSize(eDataType);
	int nLineSpace = nDataTypeSize * pRaster->GetRasterWidth();
	if (pdfGeoTransform[5] > 0)
	{
		pData += nLineSpace * (pRaster->GetRasterHeight() - 1);
		nLineSpace = - nLineSpace;
	}
	CPLErr err = m_pGDALDataset->RasterIO(GF_Read, nXOff, nYOff, nXSize, nYSize, pData, pRaster->GetRasterWidth(), 
		pRaster->GetRasterHeight(), (GDALDataType)eDataType, nBandCount, pBandIndex, 0, nLineSpace, 0);
	if (err == CE_None)
	{
		return true;
	}
	return false;
}
bool CHdRasterDataset::Read(int nOverviewIndex, int nXOff, int nYOff, int nBandCount, int* pBandIndex,
	CHdRasterBlock* pRaster) const
{
	if (!m_pGDALDataset || nBandCount > m_pGDALDataset->GetRasterCount() || !pRaster)
	{
		return false;
	}
	if (pBandIndex == NULL)
	{
		pBandIndex = new int[nBandCount];
		for (int i = 0; i < nBandCount; i ++)
		{
			pBandIndex[i] = i + 1;
		}
	}

	double pdfGeoTransform[6];
	GetGeoTransform(pdfGeoTransform);
	unsigned char* pData = pRaster->GetRasterData();
	EHdRasterDataType eDataType = pRaster->GetDataType();
	int nDataTypeSize = GetDataTypeSize(eDataType);
	int nLineSpace = nDataTypeSize * pRaster->GetRasterWidth();
	if (pdfGeoTransform[5] > 0)
	{
		pData += nLineSpace * (pRaster->GetRasterHeight() - 1);
		nLineSpace = - nLineSpace;
	}
	int nRasterWidth = pRaster->GetRasterWidth();
	int nRasterHeight = pRaster->GetRasterHeight();
	int nBandSize = nDataTypeSize * nRasterWidth * nRasterHeight;

	// 从第一个波段中获取指定金字塔层的大小
	GDALRasterBand* pBand = m_pGDALDataset->GetRasterBand(pBandIndex[0]);
	if (!pBand || !pBand->GetOverview(nOverviewIndex))
	{
		return false;
	}
	int nOvWidth = pBand->GetOverview(nOverviewIndex)->GetXSize();
	int nOvHeight = pBand->GetOverview(nOverviewIndex)->GetYSize();

	// 依次从每个波段的对应金字塔层中读取数据
	for (int i = 0; i < nBandCount; i ++)
	{
		pBand = m_pGDALDataset->GetRasterBand(pBandIndex[i]);
		if (!pBand || !pBand->GetOverview(nOverviewIndex))
		{
			continue;
		}

		// 获取金字塔波段，判断大小是否与第一个波段的一致，通常是一致的
		GDALRasterBand* pOvBand = pBand->GetOverview(nOverviewIndex);
		if (pOvBand->GetXSize() != nOvWidth || pOvBand->GetYSize() != nOvHeight)
		{
			continue;
		}

		// 读取数据
		pOvBand->RasterIO(GF_Read, nXOff, nYOff, nRasterWidth, nRasterHeight, pData + nBandSize * i, 
			nRasterWidth, nRasterHeight, (GDALDataType)pRaster->GetDataType(), 0, nLineSpace);

		pRaster->SetBandColorInterp(i + 1, (EHdColorInterp)pBand->GetColorInterpretation());
	}

	return true;
}
int CHdRasterDataset::GetDataTypeSize(EHdRasterDataType eDataType)
{
	int nSize = 0;
	switch(eDataType)
	{
	case EHD_DT_Byte:
		nSize = 1;
		break;
	case EHD_DT_UInt16:
	case EHD_DT_Int16:
		nSize = 2;
		break;
	case EHD_DT_UInt32:
	case EHD_DT_Int32:
	case EHD_DT_Float32:
		nSize = 4;
		break;
	case EHD_DT_Float64:
		nSize = 8;
		break;
	default:break;
	}
	return nSize;
}


CHdRasterBlock::CHdRasterBlock()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_nBandCount = 0;
	m_pData = NULL;
}
CHdRasterBlock::CHdRasterBlock(unsigned int nWidth, unsigned int nHeight, unsigned int nBandCount, EHdRasterDataType eDataType)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nBandCount = nBandCount;
	m_eDataType = eDataType;
	int nDataTypeSize = CHdRasterDataset::GetDataTypeSize(eDataType);
	m_pData = new unsigned char[m_nWidth * m_nHeight * m_nBandCount * nDataTypeSize];
	m_vColorInterp.resize(nBandCount);
}
CHdRasterBlock::~CHdRasterBlock()
{
	delete []m_pData;
}
unsigned int CHdRasterBlock::GetRasterWidth() const
{
	return m_nWidth;
}
unsigned int CHdRasterBlock::GetRasterHeight() const
{
	return m_nHeight;
}
unsigned int CHdRasterBlock::GetBandCount() const
{
	return m_nBandCount;
}
EHdRasterDataType CHdRasterBlock::GetDataType() const
{
	return m_eDataType;
}
EHdColorInterp CHdRasterBlock::GetBandColorInterp(int nBand) const
{
	if (nBand >= 1 && nBand <= m_nBandCount)
	{
		return m_vColorInterp[nBand - 1];
	}
	return EHD_CI_Unknown;
}
double CHdRasterBlock::GetNoDataValue() const
{
	return m_dfNoDataValue;
}
bool CHdRasterBlock::GetMinMaxValue(int nBand, double& dfMinValue, double& dfMaxValue) const
{
	if (nBand < 1 || nBand > m_nBandCount)
	{
		return false;
	}

	int nDataTypeSize = CHdRasterDataset::GetDataTypeSize(m_eDataType);
	unsigned char* pBandData = m_pData + m_nWidth * m_nHeight * (nBand - 1) * nDataTypeSize;
	int nPixelCount = m_nWidth * m_nHeight;
	switch(m_eDataType)
	{
	case EHD_DT_Byte:
		{
			unsigned char nMinValue = pBandData[0];
			unsigned char nMaxValue = pBandData[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData[i] < nMinValue)
				{
					nMinValue = pBandData[i];
				}
				else if (pBandData[i] > nMaxValue)
				{
					nMaxValue = pBandData[i];
				}
			}
			dfMinValue = nMinValue;
			dfMaxValue = nMaxValue;
			break;
		}
	case EHD_DT_UInt16:
		{
			unsigned short* pBandData2 = (unsigned short*)pBandData;
			unsigned short nMinValue = pBandData2[0];
			unsigned short nMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < nMinValue)
				{
					nMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > nMaxValue)
				{
					nMaxValue = pBandData2[i];
				}
			}
			dfMinValue = nMinValue;
			dfMaxValue = nMaxValue;
			break;
		}
	case EHD_DT_Int16:
		{
			short* pBandData2 = (short*)pBandData;
			short nMinValue = pBandData2[0];
			short nMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < nMinValue)
				{
					nMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > nMaxValue)
				{
					nMaxValue = pBandData2[i];
				}
			}
			dfMinValue = nMinValue;
			dfMaxValue = nMaxValue;
			break;
		}
	case EHD_DT_UInt32:
		{
			unsigned int* pBandData2 = (unsigned int*)pBandData;
			unsigned int nMinValue = pBandData2[0];
			unsigned int nMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < nMinValue)
				{
					nMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > nMaxValue)
				{
					nMaxValue = pBandData2[i];
				}
			}
			dfMinValue = nMinValue;
			dfMaxValue = nMaxValue;
			break;
		}
	case EHD_DT_Int32:
		{
			int* pBandData2 = (int*)pBandData;
			int nMinValue = pBandData2[0];
			int nMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < nMinValue)
				{
					nMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > nMaxValue)
				{
					nMaxValue = pBandData2[i];
				}
			}
			dfMinValue = nMinValue;
			dfMaxValue = nMaxValue;
			break;
		}
	case EHD_DT_Float32:
		{
			float* pBandData2 = (float*)pBandData;
			float fMinValue = pBandData2[0];
			float fMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < fMinValue)
				{
					fMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > fMaxValue)
				{
					fMaxValue = pBandData2[i];
				}
			}
			dfMinValue = fMinValue;
			dfMaxValue = fMaxValue;
			break;
		}
	case EHD_DT_Float64:
		{
			double* pBandData2 = (double*)pBandData;
			dfMinValue = pBandData2[0];
			dfMaxValue = pBandData2[0];
			for (int i = 0; i < nPixelCount; i ++)
			{
				if (pBandData2[i] < dfMinValue)
				{
					dfMinValue = pBandData2[i];
				}
				else if (pBandData2[i] > dfMaxValue)
				{
					dfMaxValue = pBandData2[i];
				}
			}
			break;
		}
	default:break;
	}
	return true;
}
unsigned char* CHdRasterBlock::GetRasterData() const
{
	return m_pData;
}
bool CHdRasterBlock::Create(unsigned int nWidth, unsigned int nHeight, unsigned int nBandCount, EHdRasterDataType eDataType)
{
	delete []m_pData;

	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nBandCount = nBandCount;
	m_eDataType = eDataType;
	int nDataTypeSize = CHdRasterDataset::GetDataTypeSize(eDataType);
	m_pData = new unsigned char[m_nWidth * m_nHeight * m_nBandCount * nDataTypeSize];
	m_vColorInterp.resize(nBandCount);

	return true;
}
void CHdRasterBlock::SetBandColorInterp(int nBand, EHdColorInterp eColorInterp)
{
	if (nBand >= 1 && nBand <= m_nBandCount)
	{
		m_vColorInterp[nBand - 1] = eColorInterp;
	}
}


CHdColorTable::CHdColorTable()
{
	m_nColorCount = 0;
	m_pColorArray = NULL;
}
CHdColorTable::CHdColorTable(unsigned int nColorCount)
{
	m_nColorCount = nColorCount;
	m_pColorArray = new unsigned char[nColorCount * 4];
}
CHdColorTable::~CHdColorTable()
{
	delete []m_pColorArray;
}
void CHdColorTable::CreateTable(unsigned int nColorCount)
{
	if (m_pColorArray)
	{
		delete []m_pColorArray;
		m_nColorCount = NULL;
	}
	m_nColorCount = nColorCount;
	m_pColorArray = new unsigned char[nColorCount * 4];
}
void CHdColorTable::SetColor(unsigned int nIndex, unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
	if (nIndex >= 0 && nIndex < m_nColorCount)
	{
		m_pColorArray[4 * nIndex] = a;
		m_pColorArray[4 * nIndex + 1] = r;
		m_pColorArray[4 * nIndex + 2] = g;
		m_pColorArray[4 * nIndex + 3] = b;
	}
}
void CHdColorTable::GetColor(unsigned int nIndex, unsigned char& a, unsigned char& r, unsigned char& g, unsigned char& b) const
{
	if (nIndex >= 0 && nIndex < m_nColorCount)
	{
		a = m_pColorArray[4 * nIndex];
		r = m_pColorArray[4 * nIndex + 1];
		g = m_pColorArray[4 * nIndex + 2];
		b = m_pColorArray[4 * nIndex + 3];
	}
}