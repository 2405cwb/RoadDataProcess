#include "stdafx.h"
#include "HdRasterRenderer.h"
#include "HdRasterDataset.h"
#include "HdImage.h"

using namespace hd::scene;

CHdRasterRenderer::CHdRasterRenderer()
{

}
CHdRasterRenderer::~CHdRasterRenderer()
{

}
bool CHdRasterRenderer::RenderByRGB(const CHdRasterBlock* pRaster, double* pdfMinValue, double* pdfMaxValue, 
	CHdImage* pImage, int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize)
{
	if (!pRaster || pRaster->GetBandCount() < 3)
	{
		return false;
	}
	if (!pImage || pImage->GetColorFormat() != EHD_CF_A8R8G8B8)  // 暂时只考虑这一种图像格式
	{
		return false;
	}
	if (nDstXOff < 0 || nDstYOff < 0 || nDstXSize < 0 || nDstYSize < 0 ||   // 绘制到图像的目标区域不能超出图像范围
		nDstXOff + nDstXSize > pImage->GetWidth() ||
		nDstYOff + nDstYSize > pImage->GetHeight() ||
		nDstXSize != pRaster->GetRasterWidth() ||            // 暂时不考虑绘制时进行缩放
		nDstYSize != pRaster->GetRasterHeight())
	{
		return false;
	}
	if (pRaster->GetDataType() != EHD_DT_Byte)       // RGB数据通常是字节类型
	{
		return false;
	}

	unsigned int nRasterWidth = pRaster->GetRasterWidth();
	unsigned int nRasterHeight = pRaster->GetRasterHeight();
	EHdRasterDataType eDataType = pRaster->GetDataType();
	int nDataTypeSize = CHdRasterDataset::GetDataTypeSize(eDataType);
	unsigned int nBandSize = nRasterWidth * nRasterHeight * nDataTypeSize;
	unsigned char* pImageData = pImage->GetImageData();
	if (pRaster->GetBandCount() == 3)
	{
		switch(pRaster->GetDataType())
		{
		case EHD_DT_Byte:
			{
				unsigned char* pRasterData = pRaster->GetRasterData();
				for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
				{
					unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
					for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
					{
						unsigned int nPixelOff = iRow * nRasterWidth + iCol;
						pScanLineData[(nDstXOff + iCol) * 4] = pRasterData[nPixelOff];
						pScanLineData[(nDstXOff + iCol) * 4 + 1] = pRasterData[nPixelOff + nBandSize];
						pScanLineData[(nDstXOff + iCol) * 4 + 2] = pRasterData[nPixelOff + nBandSize * 2];
						pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
					}
				}
				break;
			}
		case EHD_DT_UInt16:
		case EHD_DT_Int16:
		case EHD_DT_UInt32:
		case EHD_DT_Int32:
		case EHD_DT_Float32:
		case EHD_DT_Float64:
		default:break;
		}
	}
	else 
	{
		switch(pRaster->GetDataType())
		{
		case EHD_DT_Byte:
			{
				unsigned char* pRasterData = pRaster->GetRasterData();
				for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
				{
					unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
					for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
					{
						unsigned int nPixelOff = iRow * nRasterWidth + iCol;
						pScanLineData[(nDstXOff + iCol) * 4]  = pRasterData[nPixelOff];
						pScanLineData[(nDstXOff + iCol) * 4 + 1] = pRasterData[nPixelOff + nBandSize];
						pScanLineData[(nDstXOff + iCol) * 4 + 2] = pRasterData[nPixelOff + nBandSize * 2];
						pScanLineData[(nDstXOff + iCol) * 4 + 3] = pRasterData[nPixelOff + nBandSize * 3];
					}
				}
				break;
			}
		case EHD_DT_UInt16:
		case EHD_DT_Int16:
		case EHD_DT_UInt32:
		case EHD_DT_Int32:
		case EHD_DT_Float32:
		case EHD_DT_Float64:
		default:break;
		}
	}
	return true;
}
bool CHdRasterRenderer::RenderByStretch(const CHdRasterBlock* pRaster, double dfMinValue, double dfMaxValue, 
	CHdImage* pImage, int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize)
{
	if (!pRaster)
	{
		return false;
	}
	if (!pImage || pImage->GetColorFormat() != EHD_CF_A8R8G8B8)  // 暂时只考虑这一种图像格式
	{
		return false;
	}
	if (nDstXOff < 0 || nDstYOff < 0 || nDstXSize < 0 || nDstYSize < 0 ||   // 绘制到图像的目标区域不能超出图像范围
		nDstXOff + nDstXSize > pImage->GetWidth() ||
		nDstYOff + nDstYSize > pImage->GetHeight() ||
		nDstXSize != pRaster->GetRasterWidth() ||            // 暂时不考虑绘制时进行缩放
		nDstYSize != pRaster->GetRasterHeight())
	{
		return false;
	}

	unsigned int nRasterWidth = pRaster->GetRasterWidth();
	unsigned int nRasterHeight = pRaster->GetRasterHeight();
	switch(pRaster->GetDataType())
	{
	case EHD_DT_Byte:
		{
			unsigned char* pRasterData = pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					unsigned char nGray = pRasterData[iRow * nRasterWidth + iCol];
					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_UInt16:
		{
			unsigned short nMinValue = (unsigned short)(dfMinValue + 0.5);
			unsigned short nMaxValue = (unsigned short)(dfMaxValue + 0.5);
			unsigned short* pRasterData = (unsigned short*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					unsigned short nValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (nValue >= nMaxValue)
					{
						nGray = 255;
					}
					else if (nValue > nMinValue)
					{
						nGray = ((nValue - nMinValue) * 255 + (nMaxValue - nMinValue) / 2) / (nMaxValue - nMinValue);
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_Int16:
		{
			short nMinValue = (short)(dfMinValue + 0.5);
			short nMaxValue = (short)(dfMaxValue + 0.5);
			short* pRasterData = (short*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					short nValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (nValue >= nMaxValue)
					{
						nGray = 255;
					}
					else if (nValue > nMinValue)
					{
						nGray = ((nValue - nMinValue) * 255 + (nMaxValue - nMinValue) / 2) / (nMaxValue - nMinValue);
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_UInt32:
		{
			unsigned int nMinValue = (unsigned int)(dfMinValue + 0.5);
			unsigned int nMaxValue = (unsigned int)(dfMaxValue + 0.5);
			unsigned int* pRasterData = (unsigned int*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					unsigned int nValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (nValue >= nMaxValue)
					{
						nGray = 255;
					}
					else if (nValue > nMinValue)
					{
						nGray = ((nValue - nMinValue) * 255 + (nMaxValue - nMinValue) / 2) / (nMaxValue - nMinValue);
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_Int32:
		{
			int nMinValue = (int)(dfMinValue + 0.5);
			int nMaxValue = (int)(dfMaxValue + 0.5);
			int* pRasterData = (int*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					int nValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (nValue >= nMaxValue)
					{
						nGray = 255;
					}
					else if (nValue > nMinValue)
					{
						nGray = ((nValue - nMinValue) * 255 + (nMaxValue - nMinValue) / 2) / (nMaxValue - nMinValue);
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_Float32:
		{
			float fMinValue = dfMinValue;
			float fMaxValue = dfMaxValue;
			float* pRasterData = (float*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					float fValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (fValue >= fMaxValue)
					{
						nGray = 255;
					}
					else if (fValue > fMinValue)
					{
						nGray = (fValue - fMinValue) * 255 / (fMaxValue - fMinValue) + 0.5;
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	case EHD_DT_Float64:
		{
			double* pRasterData = (double*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					double dfValue = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char nGray = 0;
					if (dfValue >= dfMaxValue)
					{
						nGray = 255;
					}
					else if (dfValue > dfMinValue)
					{
						nGray = (dfValue - dfMinValue) * 255 / (dfMaxValue - dfMinValue) + 0.5;
					}

					pScanLineData[(nDstXOff + iCol) * 4] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = nGray;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = 255;
				}
			}
			break;
		}
	default:break;
	}

	return true;
}
bool CHdRasterRenderer::RenderByColorMap(const CHdRasterBlock* pRaster, CHdColorTable* pColorTable, 
	CHdImage* pImage, int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize)
{
	if (!pRaster || !pColorTable)
	{
		return false;
	}
	if (!pImage || pImage->GetColorFormat() != EHD_CF_A8R8G8B8)  // 暂时只考虑这一种图像格式
	{
		return false;
	}
	if (nDstXOff < 0 || nDstYOff < 0 || nDstXSize < 0 || nDstYSize < 0 ||   // 绘制到图像的目标区域不能超出图像范围
		nDstXOff + nDstXSize > pImage->GetWidth() ||
		nDstYOff + nDstYSize > pImage->GetHeight() ||
		nDstXSize != pRaster->GetRasterWidth() ||            // 暂时不考虑绘制时进行缩放
		nDstYSize != pRaster->GetRasterHeight())
	{
		return false;
	}

	EHdRasterDataType eDataType = pRaster->GetDataType();
	if (eDataType != EHD_DT_Byte && eDataType != EHD_DT_UInt16 && eDataType != EHD_DT_Int16)
	{
		return false;
	}

	unsigned int nRasterWidth = pRaster->GetRasterWidth();
	unsigned int nRasterHeight = pRaster->GetRasterHeight();
	switch(pRaster->GetDataType())
	{
	case EHD_DT_Byte:
		{
			unsigned char* pRasterData = pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					unsigned char nColorIndex = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char r, g, b, a;
					pColorTable->GetColor(nColorIndex, a, r, g, b);
					pScanLineData[(nDstXOff + iCol) * 4] = b;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = g;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = r;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = a;
				}
			}
			break;
		}
	case EHD_DT_UInt16:
		{
			unsigned short* pRasterData = (unsigned short*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					unsigned short nColorIndex = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char r, g, b, a;
					pColorTable->GetColor(nColorIndex, a, r, g, b);
					pScanLineData[(nDstXOff + iCol) * 4] = b;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = g;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = r;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = a;
				}
			}
			break;
		}
	case EHD_DT_Int16:
		{
			short* pRasterData = (short*)pRaster->GetRasterData();
			for (unsigned int iRow = 0; iRow < nRasterHeight; iRow ++)
			{
				unsigned char* pScanLineData = pImage->GetScanLineData(iRow + nDstYOff);
				for (unsigned int iCol = 0; iCol < nRasterWidth; iCol ++)
				{
					short nColorIndex = pRasterData[iRow * nRasterWidth + iCol];
					unsigned char r, g, b, a;
					pColorTable->GetColor(nColorIndex, a, r, g, b);
					pScanLineData[(nDstXOff + iCol) * 4] = b;
					pScanLineData[(nDstXOff + iCol) * 4 + 1] = g;
					pScanLineData[(nDstXOff + iCol) * 4 + 2] = r;
					pScanLineData[(nDstXOff + iCol) * 4 + 3]  = a;
				}
			}
			break;
		}
	case EHD_DT_UInt32:
	case EHD_DT_Int32:
	case EHD_DT_Float32:
	case EHD_DT_Float64:
	default:break;
	}

	return false;
}