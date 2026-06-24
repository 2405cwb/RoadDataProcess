#include "stdafx.h"
#include "HdSearchChessboard.h"
#include "point_cloud.h"
#include "..\..\hdCore\hdVector2d.h"
#include "..\..\hdCommon\hdSceneStr.h"

#include "gdal.h"
#include "gdal_priv.h"

using namespace hd;
using namespace std;

CHdSearchChessboard::CHdSearchChessboard()
{
	m_fMinChessboardSize = 0.07;
}
CHdSearchChessboard::~CHdSearchChessboard()
{

}
//bool CHdSearchChessboard::SearchSingleChessboard(PointCloud* pPointCloud, int nLoopIndex, int nPointIndex, 
//	PointChessboard& chessboard)
//{
//	return true;
//}
bool CHdSearchChessboard::SearchAllChessboards(PointCloud* pPointCloud, std::vector< PointChessboard >& vChessboard, 
	ProcessCallbackFunc pProgressFunc)
{
	IHLSReader* pReader = pPointCloud->GetHlsReader();
	if (!pReader)
	{
		return 0;
	}
	pReader->GetLoopIndex();
	int nLoopCount = pReader->GetLoopCount();
	if (nLoopCount == 0)
	{
		return 0;
	}
	m_pPointCloud = pPointCloud;

	// 获取点云文件名，用于进度提示
	string strFileName = pReader->GetFilePath();
	m_strPCName = strFileName.substr(strFileName.find_last_of("\\/") + 1);

	// 统计每圈点的水平角度
	StatisticsAngle(0, 0, pProgressFunc);
	
	// 设置生成图像时需要考虑的水平和垂直方向的角度范围
	m_dfHorizontalStart = 0;
	m_dfHorizontalEnd = 360.0 + 2.0;
	m_dfVerticalStart = 90.0;
	m_dfVerticalEnd = - 45.0;

	// 计算水平和垂直方向的角度分辨率
	double dfDstInterval = max(360.0 / pPointCloud->m_header.number_of_col, 360.0 / 8192);
	m_dfHorizontalInterval = dfDstInterval;
	m_dfVerticalInterval = dfDstInterval;

	// 计算每次处理的图像块的最大宽度和高度，分配内存
	unsigned int nBlockWidth = 1800;
	unsigned int nBlockHeight = (m_dfVerticalStart - m_dfVerticalEnd) / m_dfVerticalInterval;
	unsigned short* pGrayImageData = new unsigned short[nBlockWidth * nBlockHeight];
	float* pDepthImageData = new float[nBlockWidth * nBlockHeight];
	unsigned char* pValidityMask = new unsigned char[nBlockWidth * nBlockHeight];

	// 图像分块数（非准确值，仅用于设置进度）
	int nBlockCount = (360.0 / m_dfHorizontalInterval + nBlockWidth - 1) / nBlockWidth;
	int nCompletedBlockCount = 0;

	// 分块计算灰度图和深度图，并搜索棋盘
	double dfImageHorizontalStart = m_dfHorizontalStart;       // 图像块的水平角度范围
	double dfImageHorizontalEnd = dfImageHorizontalStart + nBlockWidth * m_dfHorizontalInterval;
	while (true)
	{
		// 图像块的实际大小
		unsigned int nImageWidth = (unsigned int)((dfImageHorizontalEnd - dfImageHorizontalStart) / m_dfHorizontalInterval + 0.5);
		unsigned int nImageHeight = nBlockHeight;

		//clock_t t1, t2, t3;
		//t1 = clock();

		// 创建图像
		float fBeginProgress = (double)nCompletedBlockCount / nBlockCount;
		float fEndProgress = fBeginProgress + 0.8 / nBlockCount;
		if (CreateImage(dfImageHorizontalStart, dfImageHorizontalEnd, m_dfVerticalStart, m_dfVerticalEnd, pGrayImageData,
			pDepthImageData, pValidityMask, nImageWidth, nImageHeight, fBeginProgress, fEndProgress, pProgressFunc))
		{
			//t2 = clock();

			// 查找棋盘
			vector< PointChessboard > vChessboardTemp;
			fBeginProgress = fEndProgress;
			fEndProgress = ((double)nCompletedBlockCount + 1) / nBlockCount;
			if (SearchChessboards(dfImageHorizontalStart, dfImageHorizontalEnd, m_dfVerticalStart, m_dfVerticalEnd, pGrayImageData, 
				pDepthImageData, pValidityMask, nImageWidth, nImageHeight, vChessboardTemp, fBeginProgress, fEndProgress, pProgressFunc))
			{
				vChessboard.insert(vChessboard.end(), vChessboardTemp.begin(), vChessboardTemp.end());
			}

			//t3 = clock();
		}

		//clock_t tCreate = t2 - t1;
		//clock_t tSearch = t3 - t2;
		
		// 已结束，则退出
		if (dfImageHorizontalEnd > m_dfHorizontalEnd - 0.1)
		{
			break;
		}

		// 计算下一个图像块的水平角度范围，要重叠 2.0 度
		dfImageHorizontalStart = dfImageHorizontalEnd - 2.0;
		dfImageHorizontalEnd = min(m_dfHorizontalEnd, dfImageHorizontalStart + nBlockWidth * m_dfHorizontalInterval);

		nCompletedBlockCount ++;
	}

	// 去除重复找到的棋盘
	if (vChessboard.size() > 1)
	{
		for (unsigned int i = 0; i < vChessboard.size() - 1; i ++)
		{
			for (unsigned int j = i + 1; j < vChessboard.size(); )
			{
				float fdX = vChessboard[i].x - vChessboard[j].x;
				float fdY = vChessboard[i].y - vChessboard[j].y;
				float fdZ = vChessboard[i].z - vChessboard[j].z;
				float fDis = sqrt(fdX * fdX + fdY * fdY + fdZ * fdZ);
				if (fDis < 0.05)
				{
					vChessboard.erase(vChessboard.begin() + j);
				}
				else
				{
					j ++;
				}
			}
		}
	}

	delete []pGrayImageData;
	delete []pDepthImageData;
	delete []pValidityMask;
	
	return true;
}
bool CHdSearchChessboard::StatisticsAngle(float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc)
{
	IHLSReader* pReader = m_pPointCloud->GetHlsReader();
	unsigned int nLoopCount = pReader->GetLoopCount();

	m_vdfHorizontalAngle.assign(nLoopCount, 0);

	int nLoopInterval = 500;

	// 估计总的迭代次数，用于设置进度
	int nTotalIterCount = nLoopCount / nLoopInterval;
	int nCompletedIterCount = 0;

	unsigned int nStartIndex = 0;
	unsigned int nEndIndex = min(nLoopCount - 1, nStartIndex + nLoopInterval);
	m_vdfHorizontalAngle[nStartIndex] = CalcLoopAngle(nStartIndex);
	m_vdfHorizontalAngle[nEndIndex] = CalcLoopAngle(nEndIndex);
	while(nStartIndex < nLoopCount - 1)
	{
		double dfStartHorizontalAngle = m_vdfHorizontalAngle[nStartIndex];
		double dfEndHorizontalAngle = m_vdfHorizontalAngle[nEndIndex];
		double dfAngleInterval = (dfEndHorizontalAngle - dfStartHorizontalAngle) / (nEndIndex - nStartIndex);
		
		if (nEndIndex - nStartIndex > 1)
		{
			unsigned int nMidIndex = (nStartIndex + nEndIndex) / 2;
			double dfMidHorizontalAngle = dfStartHorizontalAngle + dfAngleInterval * (nMidIndex - nStartIndex);
			double dfMidRealHorizontalAngle = CalcLoopAngle(nMidIndex);
			m_vdfHorizontalAngle[nMidIndex] = dfMidRealHorizontalAngle;

			if (abs(dfMidHorizontalAngle - dfMidRealHorizontalAngle) > 0.1)
			{
				nEndIndex = nMidIndex;
			}
			else
			{
				for (unsigned int i = nStartIndex; i < nEndIndex; i ++)
				{
					m_vdfHorizontalAngle[i] = dfStartHorizontalAngle + dfAngleInterval * (i - nStartIndex);
				}
				nStartIndex = nEndIndex;
				nEndIndex = min(nLoopCount - 1, nStartIndex + nLoopInterval);
				m_vdfHorizontalAngle[nEndIndex] = CalcLoopAngle(nEndIndex);
			}
		}
		else
		{
			nStartIndex = nEndIndex;
			nEndIndex = min(nLoopCount - 1, nStartIndex + nLoopInterval);
			m_vdfHorizontalAngle[nEndIndex] = CalcLoopAngle(nEndIndex);
		}

		// 更新进度
		if (pProgressFunc != NULL && nCompletedIterCount % 20 == 0)
		{
			float fCurrentProgress = fBeginProgress + ((double)nCompletedIterCount) / nTotalIterCount * 
				(fEndProgress - fBeginProgress);
			fCurrentProgress = min(fCurrentProgress, fEndProgress);
			string strMsg = m_strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_CHESSBOARD;
			pProgressFunc(fCurrentProgress, strMsg.data());
		}
		nCompletedIterCount ++;
		nTotalIterCount = nCompletedIterCount + (nLoopCount - nStartIndex) / nLoopInterval;
	}

	return true;
}
bool CHdSearchChessboard::CreateImage(double dfHorizontalStart, double dfHorizontalEnd, double dfVerticalStart, 
	double dfVerticalEnd, unsigned short* pGrayImageData, float* pDepthImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, float fBeginProgress, float fEndProgress, 
	ProcessCallbackFunc pProgressFunc)
{
	IHLSReader* pReader = m_pPointCloud->GetHlsReader();
	unsigned int nLoopCount = pReader->GetLoopCount();

	// 掩模置零，在该函数中用掩模保存每个像素包含的点数
	memset(pValidityMask, 0, nImageWidth * nImageHeight);

	memset(pGrayImageData, 0, nImageWidth * nImageHeight);

	// 水平和垂直方向的角度分辨率
	double dfHorizontalInterval = (dfHorizontalEnd - dfHorizontalStart) / nImageWidth;
	double dfVerticalInterval = (dfVerticalStart - dfVerticalEnd) / nImageHeight;

	// 计算与图像角度范围相交的圈数，用于设置进度
	int nIntersectLoopCount = 0;
	for (unsigned int iLoop = 0; iLoop < nLoopCount; iLoop ++)
	{
		// 判断该圈点的角度范围是否与图像的角度范围相交
		if (m_vdfHorizontalAngle[iLoop] - 0.5 > dfHorizontalEnd || m_vdfHorizontalAngle[iLoop] + 0.5 < dfHorizontalStart)
		{
			continue;
		}
		nIntersectLoopCount ++;
	}
	int nCompletedLoopCount = 0;

	// 读取每圈点，更新灰度图和深度图
	hdVector< PointXYZIPRGBA > vLoopPoint;
	vector< double > vdfHorizontalAngle, vdfVerticalAngle;
	vector< unsigned short > vnIntensity;
	vector< float > vfDepth;
	for (unsigned int iLoop = 0; iLoop < nLoopCount; iLoop ++)
	{
		// 判断该圈点的角度范围是否与图像的角度范围相交
		if (m_vdfHorizontalAngle[iLoop] - 0.1 > dfHorizontalEnd || m_vdfHorizontalAngle[iLoop] + 0.1 < dfHorizontalStart)
		{
			continue;
		}

		CalcPointAngle(iLoop, vdfHorizontalAngle, vdfVerticalAngle, vnIntensity, vfDepth);

		unsigned int nPointCount = vdfHorizontalAngle.size();
		for (unsigned int iPoint = 0; iPoint < nPointCount; iPoint ++)
		{
			double dfHorizontalAngle = vdfHorizontalAngle[iPoint];
			double dfVerticalAngle = vdfVerticalAngle[iPoint];

			int nPixelRow = (dfVerticalStart - dfVerticalAngle) / dfVerticalInterval;
			int nPixelCol = (dfHorizontalAngle - dfHorizontalStart) / dfHorizontalInterval;
			if (nPixelRow >= 0 && nPixelRow < nImageHeight && nPixelCol >= 0 && nPixelCol < nImageWidth)
			{
				int nPixelOff = nPixelRow * nImageWidth + nPixelCol;

				unsigned short nIntensity = vnIntensity[iPoint];
				float fDepth = vfDepth[iPoint];

				int nCount = pValidityMask[nPixelOff];
				if (nCount == 0)
				{
					pGrayImageData[nPixelOff] = nIntensity;
					pDepthImageData[nPixelOff] = fDepth;
					pValidityMask[nPixelOff] ++;
				}
				else if (nCount < 255)
				{
					pGrayImageData[nPixelOff] = (nIntensity + pGrayImageData[nPixelOff] * nCount + (nCount + 1) / 2) / (nCount + 1);
					pDepthImageData[nPixelOff] = (fDepth + pDepthImageData[nPixelOff] * nCount) / (nCount + 1);
					pValidityMask[nPixelOff] ++;
				}
			}

			// 处理水平角度大于 360.0 度的情况
			if (dfHorizontalAngle + 360.0 < dfHorizontalEnd)
			{

			}
		}

		// 更新进度
		if (pProgressFunc != NULL && nCompletedLoopCount % 20 == 0)
		{
			float fCurrentProgress = fBeginProgress + ((double)nCompletedLoopCount) / nIntersectLoopCount * 
				(fEndProgress - fBeginProgress) * 0.95;
			string strMsg = m_strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_CHESSBOARD;
			pProgressFunc(fCurrentProgress, strMsg.data());
		}
		nCompletedLoopCount ++;
	}

	// 利用 3 * 3 窗口中的像素来填充无效像素
	for (unsigned int iRow = 1; iRow < nImageHeight - 1; iRow ++)
	{
		unsigned int nLineOff = iRow * nImageWidth;
		for (unsigned int iCol = 1; iCol < nImageWidth - 1; iCol ++)
		{
			unsigned int nPixelOff = nLineOff + iCol;
			if (pValidityMask[nPixelOff] > 0)
			{
				continue;
			}
			if (pValidityMask[nPixelOff - 1] > 0 && pValidityMask[nPixelOff + 1] > 0)
			{
				pGrayImageData[nPixelOff] = (pGrayImageData[nPixelOff - 1] + pGrayImageData[nPixelOff + 1]) / 2;
				pDepthImageData[nPixelOff] = (pDepthImageData[nPixelOff - 1] + pDepthImageData[nPixelOff + 1]) / 2;
				pValidityMask[nPixelOff] = 1;
			}
			else if (pValidityMask[nPixelOff - nImageWidth] > 0 && pValidityMask[nPixelOff + nImageWidth] > 0)
			{
				pGrayImageData[nPixelOff] = (pGrayImageData[nPixelOff - nImageWidth] + pGrayImageData[nPixelOff + nImageWidth]) / 2;
				pDepthImageData[nPixelOff] = (pDepthImageData[nPixelOff - nImageWidth] + pDepthImageData[nPixelOff + nImageWidth]) / 2;
				pValidityMask[nPixelOff] = 1;
			}
			else if (pValidityMask[nPixelOff - nImageWidth - 1] > 0 && pValidityMask[nPixelOff + nImageWidth + 1] > 0)
			{
				pGrayImageData[nPixelOff] = (pGrayImageData[nPixelOff - nImageWidth - 1] + pGrayImageData[nPixelOff + nImageWidth + 1]) / 2;
				pDepthImageData[nPixelOff] = (pDepthImageData[nPixelOff - nImageWidth - 1] + pDepthImageData[nPixelOff + nImageWidth + 1]) / 2;
				pValidityMask[nPixelOff] = 1;
			}
			else if (pValidityMask[nPixelOff - nImageWidth + 1] > 0 && pValidityMask[nPixelOff + nImageWidth - 1] > 0)
			{
				pGrayImageData[nPixelOff] = (pGrayImageData[nPixelOff - nImageWidth + 1] + pGrayImageData[nPixelOff + nImageWidth - 1]) / 2;
				pDepthImageData[nPixelOff] = (pDepthImageData[nPixelOff - nImageWidth + 1] + pDepthImageData[nPixelOff + nImageWidth - 1]) / 2;
				pValidityMask[nPixelOff] = 1;
			}
		}
	}

	// 强度拉伸
	StretchGray(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, fBeginProgress + (fEndProgress - fBeginProgress) * 0.95,
		fEndProgress);

	// 输出灰度图
	//GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	//if (pDriver)
	//{
	//	GDALDataset* pDataset = pDriver->Create("E:\\gray.tif", nImageWidth, nImageHeight, 1, GDT_Byte, NULL);
	//	if (pDataset)
	//	{
	//		pDataset->RasterIO(GF_Write, 0, 0, nImageWidth, nImageHeight, pGrayImageData, nImageWidth, nImageHeight, GDT_UInt16,
	//			1, NULL, 0, 0, 0);
	//		delete pDataset;
	//	}
	//}

	return true;
}
bool CHdSearchChessboard::StretchGray(unsigned short* pGrayImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, float fBeginProgress, float fEndProgress, 
	ProcessCallbackFunc pProgressFunc)
{
	// 抽样获取强度值
	vector< unsigned short > vnIntensity;
	for (unsigned int iRow = 0; iRow < nImageHeight; iRow += 5)
	{
		unsigned int nLineOff = iRow * nImageWidth;
		for (unsigned int iCol = 0; iCol < nImageWidth; iCol += 5)
		{
			unsigned int nPixelOff = nLineOff + iCol;
			if (pValidityMask[nPixelOff] == 0)
			{
				continue;
			}
			vnIntensity.push_back(pGrayImageData[nPixelOff]);
		}
	}

	// 将强度值划分为 256 个区间，各个区间的强度个数均等，得到 255 个强度分界值
	sort(vnIntensity.begin(), vnIntensity.end());
	vector< unsigned short > vnIntensityBoundary;
	for (int i = 0; i < 255; i ++)
	{
		vnIntensityBoundary.push_back(vnIntensity[(i + 1) * vnIntensity.size() / 256]);
	}

	// 计算变换后的强度值
	for (unsigned int iRow = 0; iRow < nImageHeight; iRow ++)
	{
		unsigned int nLineOff = iRow * nImageWidth;
		for (unsigned int iCol = 0; iCol < nImageWidth; iCol ++)
		{
			unsigned int nPixelOff = nLineOff + iCol;
			if (pValidityMask[nPixelOff] == 0)
			{
				continue;
			}

			unsigned short nIntensity = pGrayImageData[nPixelOff];
			unsigned short nGray = 0;

			// 利用拆半查找法来计算强度值所在的区间，得到灰度值
			if (nIntensity < vnIntensityBoundary[0])
			{
				nGray = 0;
			}
			else if (nIntensity >= vnIntensityBoundary[254])
			{
				nGray = 255;
			}
			else
			{
				int nLowIndex = 0;
				int nHighIndex = 254;
				while (nHighIndex - nLowIndex > 1)
				{
					int nMidIndex = (nLowIndex + nHighIndex) / 2;
					if (nIntensity < vnIntensityBoundary[nMidIndex])
					{
						nHighIndex = nMidIndex;
					}
					else
					{
						nLowIndex = nMidIndex;
					}
				}
				nGray = nLowIndex + 1;
			}

			pGrayImageData[nPixelOff] = nGray;
		}

		// 更新进度
		if (pProgressFunc != NULL && iRow % 400 == 0)
		{
			float fCurrentProgress = fBeginProgress + ((double)iRow) / nImageHeight * (fEndProgress - fBeginProgress);
			string strMsg = m_strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_CHESSBOARD;
			pProgressFunc(fCurrentProgress, strMsg.data());
		}
	}

	return true;
}
bool CHdSearchChessboard::SearchChessboards(double dfHorizontalStart, double dfHorizontalEnd, 
	double dfVerticalStart, double dfVerticalEnd, unsigned short* pGrayImageData, float* pDepthImageData, 
	unsigned char* pValidityMask, unsigned int nImageWidth, unsigned int nImageHeight, 
	vector< PointChessboard >& vChessboard, float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc)
{
	// 找出所有的棋盘
	vector< CHdVector2di > vPixel;
	vector< float > vSimilarity;
	for (unsigned int iPixelY = 0; iPixelY < nImageHeight; iPixelY ++)
	{
		for (unsigned int iPixelX = 0; iPixelX < nImageWidth; iPixelX ++)
		{
			int nWindowSize = 0;
			if (CalcWindowSize(pDepthImageData, pValidityMask, nImageWidth, nImageHeight, iPixelX, iPixelY, nWindowSize))
			{
				float fSimilarity = CalcSimilarity(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, iPixelX, iPixelY, nWindowSize);
				if (fSimilarity > 0.5 )
				{
					if (IsPlanar(pDepthImageData, pValidityMask, nImageWidth, nImageHeight, iPixelX, iPixelY, nWindowSize))
					{
						vPixel.push_back(CHdVector2di(iPixelX, iPixelY));
						vSimilarity.push_back(fSimilarity);
					}
				}
			}
		}

		// 更新进度
		if (pProgressFunc != NULL && iPixelY % 200 == 0)
		{
			float fCurrentProgress = fBeginProgress + ((double)iPixelY) / nImageHeight * (fEndProgress - fBeginProgress);
			string strMsg = m_strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_CHESSBOARD;
			pProgressFunc(fCurrentProgress, strMsg.data());
		}
	}

	// 局部非最大抑制
	if (vPixel.size() > 1)
	{
		for (unsigned int i = 0; i < vPixel.size(); i ++)
		{
			for (unsigned int j = 0; j < vPixel.size(); j ++)
			{
				if (j == i)
				{
					continue;
				}
				if (abs(vPixel[i].X - vPixel[j].X) < 5 && abs(vPixel[i].Y - vPixel[j].Y) < 5)
				{
					if (vSimilarity[j] > vSimilarity[i])
					{
						vPixel.erase(vPixel.begin() + i);
						vSimilarity.erase(vSimilarity.begin() + i);
						i --;
						break;
					}
				}
			}
		}
	}

	// 根据图像坐标计算三维坐标
	unsigned int nChessboardCount = vPixel.size();
	for (unsigned int i = 0; i < nChessboardCount; i ++)
	{
		PointChessboard chessboard;
		Get3dCoordinate(dfHorizontalStart, dfHorizontalEnd, dfVerticalStart, dfVerticalEnd, pDepthImageData, pValidityMask,
			nImageWidth, nImageHeight, vPixel[i].X, vPixel[i].Y, chessboard.x, chessboard.y, chessboard.z);
		vChessboard.push_back(chessboard);
	}

	return true;
}
bool CHdSearchChessboard::CalcWindowSize(float* pDepthImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, int nPixelX, int nPixelY, int& nWindowSize)
{
	if (nPixelX < 1 || nPixelX > nImageWidth - 1 ||
		nPixelY < 1 || nPixelY > nImageHeight - 1)
	{
		return false;
	}

	// 利用 2 * 2 窗口计算中心点的深度
	float fCenterDepth = 0;
	int nValidCount = 0;
	for (unsigned int iRow = nPixelY - 1; iRow < nPixelY + 1; iRow ++)
	{
		unsigned int nLineOff = iRow * nImageWidth;
		for (unsigned int iCol = nPixelX - 1; iCol < nPixelX + 1; iCol ++)
		{
			if (pValidityMask[nLineOff + iCol] == 0)
			{
				continue;
			}

			fCenterDepth += pDepthImageData[nLineOff + iCol];
			nValidCount ++;
		}
	}
	if (nValidCount == 0)
	{
		return false;
	}
	fCenterDepth /= nValidCount;

	// 计算 (m_nMinWindowSize * 2) * (m_nMinWindowSize * 2) 窗口内的像素在水平方向的深度差的中值
	//vector< float > vHorizonDepthDiff;
	//vHorizonDepthDiff.reserve(m_nMinWindowSize * 2);
	//for (unsigned int iRow = nPixelY - m_nMinWindowSize; iRow < nPixelY + m_nMinWindowSize; iRow ++)
	//{
	//	unsigned int nPixelOff1 = iRow * nImageWidth + nPixelX - m_nMinWindowSize;
	//	unsigned int nPixelOff2 = iRow * nImageWidth + nPixelX + m_nMinWindowSize - 1;
	//	vHorizonDepthDiff.push_back(pDepthImageData[nPixelOff1] - pDepthImageData[nPixelOff2]);
	//}
	//sort(vHorizonDepthDiff.begin(), vHorizonDepthDiff.end());
	//float fHorizonDepthDiff = vHorizonDepthDiff[m_nMinWindowSize] / (m_nMinWindowSize * 2 - 1);
	//if (fHorizonDepthDiff < 0)
	//{
	//	fHorizonDepthDiff = - fHorizonDepthDiff;
	//}

	// 计算垂直方向的平均深度差

	// 计算像素坐标 (nPixelX, nPixelY) 处的物体表面法向量与射线在两个方向的夹角余弦值
	//double dfTanHorizonAngle = fHorizonDepthDiff / (fCenterDepth * tan(DEG2RAD(m_dfHorizontalInterval)));
	//double dfCosHorizonAngle = 1 / sqrt(1 + dfTanHorizonAngle * dfTanHorizonAngle);

	// 根据中心点的深度和法向量方向，计算最佳窗口大小
	//int nHorizonSize = (int)(RAD2DEG(atan(m_fMinChessboardSize / 2 /* * dfCosHorizonAngle *// fCenterDepth)) / m_dfHorizontalInterval + 0.5);
	int nHorizonSize = (int)(RAD2DEG(m_fMinChessboardSize / 2 / fCenterDepth) / m_dfHorizontalInterval + 0.5);
	nWindowSize = min(30, max(4, nHorizonSize)); // 限制一下窗口大小

	return true;
}
bool CHdSearchChessboard::IsPlanar(float* pDepthImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, int nPixelX, int nPixelY, int nWindowSize)
{
	float fMaxHorizonDepthDiff = 0;
	float fMaxVerticalDepthDiff = 0;

	for (unsigned int iRow = nPixelY - nWindowSize; iRow < nPixelY + nWindowSize; iRow ++)
	{
		for (unsigned int iCol = nPixelX - nWindowSize; iCol < nPixelX + nWindowSize - 1; iCol ++)
		{
			unsigned int nPixelOff = iRow * nImageWidth + iCol;
			if (pValidityMask[nPixelOff] > 0 && pValidityMask[nPixelOff + 1] > 0)
			{
				fMaxHorizonDepthDiff = max(fMaxHorizonDepthDiff, abs(pDepthImageData[nPixelOff + 1] - pDepthImageData[nPixelOff]));
			}
		}
	}
	for (unsigned int iCol = nPixelX - nWindowSize; iCol < nPixelX + nWindowSize; iCol ++)
	{
		for (unsigned int iRow = nPixelY - nWindowSize; iRow < nPixelY + nWindowSize - 1; iRow ++)
		{
			unsigned int nPixelOff = iRow * nImageWidth + iCol;
			if (pValidityMask[nPixelOff] > 0 && pValidityMask[nPixelOff + nImageWidth] > 0)
			{
				fMaxVerticalDepthDiff = max(fMaxVerticalDepthDiff, abs(pDepthImageData[nPixelOff + nImageWidth] - pDepthImageData[nPixelOff]));
			}
		}
	}

	if (fMaxHorizonDepthDiff > 0.04 || fMaxVerticalDepthDiff > 0.02)
	{
		return false;
	}

	return true;
}
float CHdSearchChessboard::CalcSimilarity(unsigned short* pGrayImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, int nPixelX, int nPixelY, int nWindowSize)
{
	if (nPixelX < nWindowSize || nPixelX > nImageWidth - nWindowSize ||
		nPixelY < nWindowSize || nPixelY > nImageHeight - nWindowSize)
	{
		return 0;
	}
	
	// 分别计算位于中心点左上、左下、右上和右下四个区域内的像素灰度值的均值和标准差，
	// 根据对角区域的相似性和反对角区域的相异性进行过滤，这里需要排除位于区域边缘的像素以提高稳健性
	// 为减小计算量，先用一个小窗口进行初步过滤，然后再用最佳窗口来计算
	
	int nLeftUpAverage, nLeftDownAverage, nRightUpAverage, nRightDownAverage;
	int nLeftUpStdDev, nLeftDownStdDev, nRightUpStdDev, nRightDownStdDev;
	
	int nMinWindowSize = 4;
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nMinWindowSize, nPixelY - 2,
		nPixelX - nMinWindowSize, nPixelX - 2, nLeftUpAverage, nLeftUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nMinWindowSize, nPixelY - 2,
		nPixelX + 1, nPixelX + nMinWindowSize - 1, nRightUpAverage, nRightUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY + 1, nPixelY + nMinWindowSize - 1,
		nPixelX - nMinWindowSize, nPixelX - 2, nLeftDownAverage, nLeftDownStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY + 1, nPixelY + nMinWindowSize - 1,
		nPixelX + 1, nPixelX + nMinWindowSize - 1, nRightDownAverage, nRightDownStdDev))
	{
		return 0;
	}

	if (abs(nLeftUpAverage - nRightDownAverage) > 20 || abs(nLeftDownAverage - nRightUpAverage) > 20 ||
		abs(nLeftUpAverage - nRightUpAverage) < 100 || abs(nLeftDownAverage - nRightDownAverage) < 100)
	{
		return 0;
	}
	
	if (nLeftUpStdDev > 20 || nRightUpStdDev > 20 || nLeftDownStdDev > 20 || nRightDownStdDev > 20)
	{
		return 0;
	}
	
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nWindowSize, nPixelY - 2,
		nPixelX - nWindowSize, nPixelX - 2, nLeftUpAverage, nLeftUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nWindowSize, nPixelY - 2,
		nPixelX + 1, nPixelX + nWindowSize - 1, nRightUpAverage, nRightUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY + 1, nPixelY + nWindowSize - 1,
		nPixelX - nWindowSize, nPixelX - 2, nLeftDownAverage, nLeftDownStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY + 1, nPixelY + nWindowSize - 1,
		nPixelX + 1, nPixelX + nWindowSize - 1, nRightDownAverage, nRightDownStdDev))
	{
		return 0;
	}

	if (abs(nLeftUpAverage - nRightDownAverage) > 20 || abs(nLeftDownAverage - nRightUpAverage) > 20 ||
		abs(nLeftUpAverage - nRightUpAverage) < 100 || abs(nLeftDownAverage - nRightDownAverage) < 100)
	{
		return 0;
	}

	if (nLeftUpStdDev > 20 || nRightUpStdDev > 20 || nLeftDownStdDev > 20 || nRightDownStdDev > 20)
	{
		return 0;
	}
	
	// 计算四个完整区域的均值和标准差，然后据此计算出灰度分布特征与棋盘的相似度
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nWindowSize, nPixelY - 1,
		nPixelX - nWindowSize, nPixelX - 1, nLeftUpAverage, nLeftUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY - nWindowSize, nPixelY - 1,
		nPixelX, nPixelX + nWindowSize - 1, nRightUpAverage, nRightUpStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY, nPixelY + nWindowSize - 1,
		nPixelX - nWindowSize, nPixelX - 1, nLeftDownAverage, nLeftDownStdDev))
	{
		return 0;
	}
	if (!StatisticsPixels(pGrayImageData, pValidityMask, nImageWidth, nImageHeight, nPixelY, nPixelY + nWindowSize - 1,
		nPixelX, nPixelX + nWindowSize - 1, nRightDownAverage, nRightDownStdDev))
	{
		return 0;
	}

	float fSimilarity1 = (((float)min(nLeftUpAverage, nRightDownAverage)) / max(nLeftUpAverage, nRightDownAverage)
		+ ((float)min(nRightUpAverage, nLeftDownAverage)) / max(nRightUpAverage, nLeftDownAverage)) / 2;
	float fSimilarity2 = min(1.0, abs(nLeftUpAverage + nRightDownAverage - nRightUpAverage - nLeftDownAverage) / 400.0);
	float fSimilarity3 = max(0.0, 1 - (nLeftUpStdDev + nLeftDownStdDev + nRightUpStdDev + nRightDownStdDev) / 300.0);

	return fSimilarity1 / 4 + fSimilarity2 / 4 + fSimilarity3 / 2;
}
bool CHdSearchChessboard::Get3dCoordinate(double dfHorizontalStart, double dfHorizontalEnd, double dfVerticalStart, 
	double dfVerticalEnd, float* pDepthImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
	unsigned int nImageHeight, int nPixelX, int nPixelY, float& fX, float& fY, float& fZ)
{
	if (nPixelX < 1 || nPixelX > nImageWidth - 1 ||
		nPixelY < 1 || nPixelY > nImageHeight - 1)
	{
		return false;
	}

	// 利用 2 * 2 窗口计算中心点的深度
	float fCenterDepth = 0;
	int nValidCount = 0;
	for (unsigned int iRow = nPixelY - 1; iRow < nPixelY + 1; iRow ++)
	{
		unsigned int nLineOff = iRow * nImageWidth;
		for (unsigned int iCol = nPixelX - 1; iCol < nPixelX + 1; iCol ++)
		{
			if (pValidityMask[nLineOff + iCol] == 0)
			{
				continue;
			}

			fCenterDepth += pDepthImageData[nLineOff + iCol];
			nValidCount ++;
		}
	}
	if (nValidCount == 0)
	{
		return false;
	}
	fCenterDepth /= nValidCount;

	// 计算中心点的水平和垂直角度
	double dfHorizontalAngle = DEG2RAD(dfHorizontalStart + m_dfHorizontalInterval * nPixelX);
	double dfVerticalAngle = DEG2RAD(dfVerticalStart - m_dfVerticalInterval * nPixelY);

	double dfCosVertical = cos(dfVerticalAngle);
	double dfSinVertical = sin(dfVerticalAngle);

	fX = fCenterDepth * dfCosVertical * cos(dfHorizontalAngle);
	fY = - fCenterDepth * dfCosVertical * sin(dfHorizontalAngle);
	fZ = fCenterDepth * dfSinVertical;

	return true;
}
double CHdSearchChessboard::CalcLoopAngle(unsigned int nLoopIndex)
{
	IHLSReader* pReader = m_pPointCloud->GetHlsReader();

	hd::hdVector< PointXYZIPRGBA > vLoopPoint;
	pReader->ReadLoopFull(vLoopPoint, nLoopIndex);
	int nPointCount = vLoopPoint.size();
	if (nPointCount == 0)
	{
		return 0;
	}

	vector< double > vAngle;

	int nPointInterval = max(1, nPointCount / 100);
	for (int iPoint = 0; iPoint < nPointCount; iPoint += nPointInterval)
	{
		if (vLoopPoint[iPoint].isValid())
		{
			double dfAngle = RAD2DEG(atan2( - vLoopPoint[iPoint].y, vLoopPoint[iPoint].x));
			if (dfAngle < 0)
			{
				dfAngle += 360.0;
			}
			vAngle.push_back(dfAngle);
		}
	}
	if (vAngle.empty())
	{
		return 0;
	}

	sort(vAngle.begin(), vAngle.end());
	double dfLoopAngle = vAngle[vAngle.size() / 2];
	
	return dfLoopAngle;
}
void CHdSearchChessboard::CalcPointAngle(unsigned int nLoopIndex, std::vector< double >& vdfHorizontalAngle, 
	std::vector< double >& vdfVerticalAngle, std::vector< unsigned short >& vnIntensity, std::vector< float >& vfDepth)
{
	IHLSReader* pReader = m_pPointCloud->GetHlsReader();

	hd::hdVector< PointXYZIPRGBA > vLoopPoint;
	pReader->ReadLoopFull(vLoopPoint, nLoopIndex);
	int nPointCount = vLoopPoint.size();
	if (nPointCount == 0)
	{
		return ;
	}

	vdfHorizontalAngle.clear();
	vdfVerticalAngle.clear();
	vnIntensity.clear();
	vfDepth.clear();

	int nPointInterval = max(1, nPointCount / 100);
	for (int iPoint = 0; iPoint < nPointCount; iPoint += nPointInterval)
	{
		int nMinIndex = iPoint;
		int nMaxIndex = min(nPointCount - 1, iPoint + nPointInterval - 1);

		int nFirstIndex(-1), nLastIndex(-1);
		double dfFirstHorizonAngle(0), dfFirstVerticalAngle(0);
		double dfLastHorizonAngle(0), dfLastVerticalAngle(0);
		for (int i = nMinIndex; i <= nMaxIndex; i ++)
		{
			if (vLoopPoint[i].isValid())
			{
				float fX = vLoopPoint[i].x;
				float fY = vLoopPoint[i].y;
				float fZ = vLoopPoint[i].z;

				dfFirstHorizonAngle = RAD2DEG(atan2( - fY, fX));
				dfFirstVerticalAngle = RAD2DEG(atan2(fZ, sqrt(fX * fX + fY * fY)));
				if (dfFirstHorizonAngle < 0)
				{
					dfFirstHorizonAngle += 360.0;
				}
				nFirstIndex = i;
				break;
			}
		}
		if (nFirstIndex < 0)
		{
			continue;
		}

		for (int i = nMaxIndex; i >= nFirstIndex; i --)
		{
			if (vLoopPoint[i].isValid())
			{
				float fX = vLoopPoint[i].x;
				float fY = vLoopPoint[i].y;
				float fZ = vLoopPoint[i].z;

				dfLastHorizonAngle = RAD2DEG(atan2( - fY, fX));
				dfLastVerticalAngle = RAD2DEG(atan2(fZ, sqrt(fX * fX + fY * fY)));
				if (dfLastHorizonAngle < 0)
				{
					dfLastHorizonAngle += 360.0;
				}
				nLastIndex = i;
				break;
			}
		}

		double dfHorizonInterval = (dfLastHorizonAngle - dfFirstHorizonAngle) / (nLastIndex - nFirstIndex);
		double dfVerticalInterval = (dfLastVerticalAngle - dfFirstVerticalAngle) / (nLastIndex - nFirstIndex);
		for (int i = nFirstIndex; i <= nLastIndex; i ++)
		{
			if (vLoopPoint[i].isValid())
			{
				float fX = vLoopPoint[i].x;
				float fY = vLoopPoint[i].y;
				float fZ = vLoopPoint[i].z;

				double dfHorizontalAngle = dfFirstHorizonAngle + (i - nFirstIndex) * dfHorizonInterval;
				double dfVerticalAngle = dfFirstVerticalAngle + (i - nFirstIndex) * dfVerticalInterval;

				unsigned short nIntensity = vLoopPoint[i].getIntensity();

				float fDepth = sqrt(fX * fX + fY * fY + fZ * fZ);

				vdfHorizontalAngle.push_back(dfHorizontalAngle);
				vdfVerticalAngle.push_back(dfVerticalAngle);
				vnIntensity.push_back(nIntensity);
				vfDepth.push_back(fDepth);
			}
		}
	}
	
	return;
}
bool CHdSearchChessboard::StatisticsPixels(unsigned short* pGrayImageData, unsigned char* pValidityMask, 
	unsigned int nImageWidth, unsigned int nImageHeight, int nMinRow, int nMaxRow, int nMinCol, int nMaxCol, 
	int& nMean, int& nStdDev)
{
	nMean = 0;
	nStdDev = 0;
	int nValidCount = 0;
	int nTotalCount = (nMaxRow - nMinRow + 1) * (nMaxCol - nMinCol + 1);

	for (int iRow = nMinRow; iRow <= nMaxRow; iRow ++)
	{
		for (int iCol = nMinCol; iCol <= nMaxCol; iCol ++)
		{
			int nPixelOff = iRow * nImageWidth + iCol;
			if (pValidityMask[nPixelOff] > 0)
			{
				nMean += pGrayImageData[nPixelOff];
				nValidCount ++;
			}
		}
	}
	if (nValidCount < nTotalCount * 0.95)
	{
		return false;
	}
	nMean /= nValidCount;

	for (int iRow = nMinRow; iRow <= nMaxRow; iRow ++)
	{
		for (int iCol = nMinCol; iCol <= nMaxCol; iCol ++)
		{
			int nPixelOff = iRow * nImageWidth + iCol;
			if (pValidityMask[nPixelOff] > 0)
			{
				nStdDev += (pGrayImageData[nPixelOff] - nMean) * (pGrayImageData[nPixelOff] - nMean);
			}
		}
	}
	nStdDev = sqrt((float)nStdDev / nValidCount);
	return true;
}