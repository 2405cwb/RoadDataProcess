#include "stdafx.h"
#include "HdSearchSphere.h"
#include "..\hdCore\hdMatrix.h"
#include "point_cloud.h"
#include "..\hdCommon\hdSceneStr.h"

using namespace hd;

class CHdPointWithIndex
{
public:
	CHdPointWithIndex(){}
	CHdPointWithIndex(int nLoopIndex, int nPointIndex, float fX, float fY, float fZ)
	{
		m_nLoopIndex = nLoopIndex;  
		m_nPointIndex = nPointIndex;
		m_fX = fX;
		m_fY = fY;
		m_fZ = fZ;
	}
	~CHdPointWithIndex(){}
public:
	int m_nLoopIndex;
	int m_nPointIndex;
	float m_fX;
	float m_fY;
	float m_fZ;
};

CHdSearchSphere::CHdSearchSphere()
{
	m_dfRadius = 0.05;
	m_dfUpDistance = 30.0;
	m_dfUpHeight = 2.0;
	m_nDownIntensity = 0;
}
CHdSearchSphere::~CHdSearchSphere()
{

}
void CHdSearchSphere::SetSphereRadius(double dfRadius)
{
	m_dfRadius = dfRadius;
}
//bool CHdSearchSphere::SearchSingleSphere(PointCloud* pPointCloud, int nLoopIndex, int nPointIndex, PointSphere& sphere)
//{
//	return true;
//}
bool CHdSearchSphere::SearchAllSpheres(PointCloud* pPointCloud, vector< PointSphere >& vSphere,
	ProcessCallbackFunc pProgressFunc)
{
	if (!pPointCloud)
	{
		return false;
	}
	IHLSReader* pReader = pPointCloud->GetHlsReader();
	if (!pReader)
	{
		return false;
	}
	pReader->GetLoopIndex();
	int nLoopCount = pReader->GetLoopCount();
	if (nLoopCount == 0)
	{
		return false;
	}

	// 获取点云文件名，用于进度提示
	string strPCName = pReader->GetFilePath();
	strPCName = strPCName.substr(strPCName.find_last_of("\\/") + 1);

	// 更新进度条的间隔和当前进度百分比（界面刷新效率太低，需要减少刷新次数）
	int nUpdateInterval = max(1, nLoopCount / 100);   

	// 保存靶球的所有可能位置
	vector< CHdPointWithIndex > vPointWithIndex;

	// 存放二维点及其对应的三维点索引的容器，放在循环外面可以避免反复分配释放内存
	vector< CHdVector2df > vPoint2d;
	vector< int > vIndex;
	vector< CHdVector2df > vPoint2dSimplify;  // 抽稀后的点
	vector< int > vIndexSimplify;

	// 处理每一圈点
	for (int iLoop = 0; iLoop < nLoopCount; iLoop ++)
	{
		hd::hdVector< PointXYZIPRGBA > vLoopPoint;
		pReader->ReadLoopFull(vLoopPoint, iLoop);
		int nPointCount = vLoopPoint.size();
		if (nPointCount == 0)
		{
			continue;
		}

		// 获取满足距离、高度和强度约束的点，将三维坐标转为二维坐标
		vPoint2d.clear();
		vIndex.clear();
		for (int iPoint = 0; iPoint < nPointCount; iPoint ++)
		{
			if (!vLoopPoint[iPoint].isValid())
			{
				continue;
			}
			float fX = vLoopPoint[iPoint].x;
			float fY = vLoopPoint[iPoint].y;
			float fZ = vLoopPoint[iPoint].z;
			unsigned short nIntensity = vLoopPoint[iPoint].getIntensity();
			
			CHdVector2df point2d;
			point2d.X = sqrt(fX * fX + fY * fY);
			point2d.Y = fZ;

			if (point2d.X > m_dfUpDistance || point2d.Y > m_dfUpHeight /*|| nIntensity < m_nDownIntensity*/)
			{
				continue;
			}

			vPoint2d.push_back(point2d);
			vIndex.push_back(iPoint);
		}
		int nCount2d = vPoint2d.size();
		if (nCount2d <= 100)
		{
			continue;
		}

		// 对二维点根据相邻点的距离进行抽稀
		vPoint2dSimplify.clear();
		vIndexSimplify.clear();
		for (int i = 0; i < nCount2d - 10; )
		{
			vPoint2dSimplify.push_back(vPoint2d[i]);
			vIndexSimplify.push_back(vIndex[i]);

			int nInterval = 1;
			for (int j = i + 1; j < i + 10; j ++)
			{
				float fdX = vPoint2d[i].X - vPoint2d[j].X;
				float fdY = vPoint2d[i].Y - vPoint2d[j].Y;
				float fDis = sqrt(fdX * fdX + fdY * fdY);
				if (fDis < 0.015)
				{
					nInterval ++;
				}
			}
			i += nInterval;
		}

		// 遍历二维点，判断是否存在圆
		int nCount2dSimplify = vPoint2dSimplify.size();
		int nCurrentIndex = 1;
		bool bInverseClock = (vPoint2dSimplify[0].Y - vPoint2dSimplify[1].Y) * (vPoint2dSimplify[2].X - vPoint2dSimplify[1].X) +
			(vPoint2dSimplify[1].X - vPoint2dSimplify[0].X) * (vPoint2dSimplify[2].Y - vPoint2dSimplify[1].Y) > 0;
		while (nCurrentIndex < nCount2dSimplify - 1)
		{
			// 计算旋转方向与第一个点相同的若干个连续点
			int nBeginIndex = nCurrentIndex;
			int nEndIndex = -1;
			for (int iPoint = nCurrentIndex + 1; iPoint < nCount2dSimplify - 1; iPoint ++)
			{
				bool bInverseClock2 = (vPoint2dSimplify[iPoint - 1].Y - vPoint2dSimplify[iPoint].Y)
					* (vPoint2dSimplify[iPoint + 1].X - vPoint2dSimplify[iPoint].X)
					+ (vPoint2dSimplify[iPoint].X - vPoint2dSimplify[iPoint - 1].X) 
					* (vPoint2dSimplify[iPoint + 1].Y - vPoint2dSimplify[iPoint].Y) > 0;
				if (abs(vPoint2dSimplify[iPoint].X - vPoint2dSimplify[iPoint - 1].X) > 0.04 ||
					bInverseClock2 != bInverseClock)
				{
					nEndIndex = iPoint - 1;
					nCurrentIndex = iPoint;
					bInverseClock = bInverseClock2;

					break;
				}
			}
			if (nEndIndex < 0)
			{
				nEndIndex = nCount2dSimplify - 1;
				nCurrentIndex = nCount2dSimplify;
			}

			// 点数不能过少
			if (nEndIndex - nBeginIndex + 1 < 5)
			{
				continue;
			}
			
			// 计算中间索引的点与首尾点连线中点的距离，判断是否大于一定阈值
			int nMidIndex = (nBeginIndex + nEndIndex) / 2;
    		float fX1 = vPoint2dSimplify[nBeginIndex].X;  float fY1 = vPoint2dSimplify[nBeginIndex].Y;
			float fX2 = vPoint2dSimplify[nEndIndex].X;    float fY2 = vPoint2dSimplify[nEndIndex].Y;
			float fX3 = vPoint2dSimplify[nMidIndex].X;    float fY3 = vPoint2dSimplify[nMidIndex].Y;
			float fX4 = (fX1 + fX2) / 2;
			float fY4 = (fY1 + fY2) / 2;
			if ((fX3 - fX4) * fX4 + (fY3 - fY4) * fY4 > 0)//  中间索引点必须比首尾连线中点离原点更近
			{
				continue;
			}
			float fDis = sqrt((fX3 - fX4) * (fX3 - fX4) + (fY3 - fY4) * (fY3 - fY4));
			if (fDis < m_dfRadius / 2)
			{
				continue;
			}

			// 判断是否能拟合出一个合适的圆
			vector< CHdVector2df > vPoint2dTemp(vPoint2dSimplify.begin() + nMidIndex - 2, vPoint2dSimplify.begin() + nMidIndex + 3);
			double dfCenterX, dfCenterY, dfRadius, dfMaxError;
			if (!FitCircle(vPoint2dTemp, dfCenterX, dfCenterY, dfRadius, dfMaxError)
				|| dfRadius < m_dfRadius - 0.02 || dfRadius > m_dfRadius + 0.01 || dfMaxError > 0.005)
			{
				continue;
			}

			int nPointIndex = vIndexSimplify[nMidIndex];
			vPointWithIndex.push_back(CHdPointWithIndex(iLoop, nPointIndex, vLoopPoint[nPointIndex].x,
				vLoopPoint[nPointIndex].y, vLoopPoint[nPointIndex].z));
		}

		if (pProgressFunc)
		{
			if (iLoop % nUpdateInterval == 0)
			{
				string strMsg = strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_SPHERE;
				pProgressFunc(((double)iLoop) / nLoopCount * 0.95, strMsg.data());
			}
		}
	}
	if (vPointWithIndex.empty())
	{
		return false;
	}
	
	// 去掉距离相近的种子点
	if (vPointWithIndex.size() > 1)
	{
		for (unsigned int i = 0; i < vPointWithIndex.size() - 1; i ++)
		{
			for (unsigned int j = i + 1; j < vPointWithIndex.size(); )
			{
				float fdX = vPointWithIndex[i].m_fX - vPointWithIndex[j].m_fX;
				float fdY = vPointWithIndex[i].m_fY - vPointWithIndex[j].m_fY;
				float fdZ = vPointWithIndex[i].m_fZ - vPointWithIndex[j].m_fZ;
				float fDis = sqrt(fdX * fdX + fdY * fdY + fdZ * fdZ);
				if (fDis < 0.05)
				{
					vPointWithIndex.erase(vPointWithIndex.begin() + j);
				}
				else
				{
					j ++;
				}
			}
		}
	}
	
	// 在每个种子点的附近寻找一个球
	int nFeedCount = vPointWithIndex.size();
	nUpdateInterval = max(1, nFeedCount / 50);
	for (unsigned int i = 0; i < nFeedCount; i ++)
	{
		int nLoopIndex = vPointWithIndex[i].m_nLoopIndex;
		int nPointIndex = vPointWithIndex[i].m_nPointIndex;
		
		// 根据种子点的索引，获取附近的点，这里可以根据水平和垂直方向的扫描角度间隔来计算出更加合适的圈号与点号范围
		vector< CHdVector3df > vPoint3d;
		int nMinLoopIndex = max(0, nLoopIndex - 20);
		int nMaxLoopIndex = min(nLoopCount - 1, nLoopIndex + 20);
		for (int iLoop = nMinLoopIndex; iLoop <= nMaxLoopIndex; iLoop ++)
		{
			hdVector< PointXYZIPRGBA > vLoopPoint;
			pReader->ReadLoopFull(vLoopPoint, iLoop);
			int nPointCount = vLoopPoint.size();
			if (nPointCount == 0)
			{
				continue;
			}

			int nMinPointIndex = max(0, nPointIndex - 100);
			int nMaxPointIndex = min(nPointCount - 1, nPointIndex + 100);
			for (int iPoint = nMinPointIndex; iPoint <= nMaxPointIndex; iPoint ++)
			{
				if (vLoopPoint[iPoint].isValid())
				{
					vPoint3d.push_back(CHdVector3df(vLoopPoint[iPoint].x, vLoopPoint[iPoint].y, vLoopPoint[iPoint].z));
				}
			}
		}

		// 拟合球
		PointSphere sphereFitted;
		CHdVector3df pointSeed(vPointWithIndex[i].m_fX, vPointWithIndex[i].m_fY, vPointWithIndex[i].m_fZ);
		if (SearchSphere(vPoint3d, pointSeed, m_dfRadius, sphereFitted))
		{
			vSphere.push_back(sphereFitted);
		}

		if (pProgressFunc)
		{
			if (i % nUpdateInterval == 0)
			{
				string strMsg = strPCName + " " + HDSCENE_IDS_REGISTER_FINDING_SPHERE;
				pProgressFunc(((double)i) / nFeedCount * 0.05 + 0.95, strMsg.data());
			}
		}
	}

	// 对于重复找到的球，保留拟合点数多的一个
	if (vSphere.size() > 1)
	{
		for (unsigned int i = 0; i < vSphere.size() - 1; i ++)
		{
			for (unsigned int j = i + 1; j < vSphere.size(); j ++)
			{
				float fdX = vSphere[i].x - vSphere[j].x;
				float fdY = vSphere[i].y - vSphere[j].y;
				float fdZ = vSphere[i].z - vSphere[j].z;
				float fDis = sqrt(fdX * fdX + fdY * fdY + fdZ * fdZ);
				if (fDis <= m_dfRadius * 2)
				{
					if (vSphere[i].fitPtCount > vSphere[j].fitPtCount)
					{
						vSphere.erase(vSphere.begin() + j);
						j --;
					}
					else
					{
						vSphere.erase(vSphere.begin() + i);
						i --;
						break;
					}
				}
			}
		}
	}
	
	return true;
}

bool CHdSearchSphere::SearchSphere(vector< CHdVector3df >& vPoint3d, CHdVector3df pointSeed, double dfRadius, PointSphere& sphere)
{
	// 获取距离种子点在一定范围内的点
	float fUpDisToSeed_2 = dfRadius * 2 * dfRadius * 2;
	vector< CHdVector3df > vPoint3dNear;
	unsigned int nPointCount = vPoint3d.size();
	for (int i = 0; i < nPointCount; i ++)
	{
		float fdX = vPoint3d[i].X - pointSeed.X;
		float fdY = vPoint3d[i].Y - pointSeed.Y;
		float fdZ = vPoint3d[i].Z - pointSeed.Z;
		float fDis_2 = fdX * fdX + fdY * fdY + fdZ * fdZ;
		if (fDis_2 < fUpDisToSeed_2)
		{
			vPoint3dNear.push_back(vPoint3d[i]);
		}
	}
	if (vPoint3dNear.size() < 15)
	{
		return false;
	}

	// 采用RANSAC方法寻找满足要求的球
	vector< CHdVector3dd > vSphereCenter;
	vector< int > vInnerCount;
	double dfRadius1 = dfRadius - 0.005;
	double dfRadius2 = dfRadius + 0.005;
	int nTimes = 500;
	for (int iTime = 0; iTime < nTimes; iTime ++)
	{
		// 随机产生四个点用于计算球的参数
		vector< CHdVector3df > vPointTemp;
		for (int i = 0; i < 4; i ++)
		{
			int nIndex = rand() % vPoint3dNear.size();
			vPointTemp.push_back(vPoint3dNear[nIndex]);
		}

		// 计算四个点之间的最小距离和最大距离，最小距离过小以及最大距离过大时无效
		float fMinDis = F32_MAX;
		float fMaxDis = F32_MIN;
		for (int i = 0; i < 3; i ++)
		{
			for (int j = i + 1; j < 4; j ++)
			{
				float fdX = vPointTemp[i].X - vPointTemp[j].X;
				float fdY = vPointTemp[i].Y - vPointTemp[j].Y;
				float fdZ = vPointTemp[i].Z - vPointTemp[j].Z;
				float fDis = sqrt(fdX * fdX + fdY * fdY + fdZ * fdZ);
				fMinDis = min(fMinDis, fDis);
				fMaxDis = max(fMaxDis, fDis);
			}
		}
		if (fMinDis < 0.03 || fMaxDis > dfRadius * 2)
		{
			continue;
		}

		// 采用线性方法计算球的参数
		CHdVector3dd sphereCenter;
		double dfFittedRadius = 0;
		if (!FitSphere(vPointTemp, sphereCenter, dfFittedRadius)
			|| abs(dfFittedRadius - dfRadius) > 0.005)
		{
			continue;
		}
		double dfCenterX = sphereCenter.X;
		double dfCenterY = sphereCenter.Y;
		double dfCenterZ = sphereCenter.Z;

		// 计算内点数和内点占总点数之比，判断拟合出的球是否有效
		// 总点数是位于球相对于坐标系原点的角度空间，且与原点的距离大于一定值的所有点
		// 内点则还要求位于球面上且在原点与球心之间
		int nTotalCount = 0;
		int nInnerCount = 0;
		double dfCenterToOrigin = sqrt(dfCenterX * dfCenterX + dfCenterY * dfCenterY + dfCenterZ * dfCenterZ);// 球心到原点的距离
		double dfUpAngleWithCenter = asin(dfRadius / dfCenterToOrigin);  // 球面上的点与球心和原点构成的最大角度
		double dfDownDisToOrigin = dfCenterToOrigin - dfRadius - 0.05;   // 统计总点数时使用的最小距离
		for (int i = 0; i < nPointCount; i ++)
		{
			double dfX = vPoint3d[i].X;
			double dfY = vPoint3d[i].Y;
			double dfZ = vPoint3d[i].Z;
			double dfDisToOrigin = sqrt(dfX * dfX + dfY * dfY + dfZ * dfZ);

			// 点与原点的距离大于阈值（目的是排除挡在球前面的物体，增强对遮挡情况的处理能力）
			if (dfDisToOrigin > dfDownDisToOrigin)
			{
				// 点与原点和球心构成的夹角
				double dfAngle = acos((dfX * dfCenterX + dfY * dfCenterY + dfZ * dfCenterZ) / dfDisToOrigin / dfCenterToOrigin);
				if (dfAngle < dfUpAngleWithCenter * 0.95)
				{
					nTotalCount ++;

					// 点是否在原点与球心之间
					if ((dfX - dfCenterX) * dfCenterX + (dfY - dfCenterY) * dfCenterY + (dfZ - dfCenterZ) * dfCenterZ < 0)
					{
						// 点到球心的距离
						double dfDisToCenter = sqrt((dfX - dfCenterX) * (dfX - dfCenterX) + (dfY - dfCenterY) * (dfY - dfCenterY) +
							(dfZ - dfCenterZ) * (dfZ - dfCenterZ));
						if (dfDisToCenter < dfRadius2 && dfDisToCenter > dfRadius1)
						{
							nInnerCount ++;
						}
					}
				}
			}
		}
		if (nInnerCount > 20 && nInnerCount > nTotalCount * 0.95)
		{
			vSphereCenter.push_back(CHdVector3dd(dfCenterX, dfCenterY,dfCenterZ));
			vInnerCount.push_back(nInnerCount);
		}
	}
	if (vSphereCenter.empty())
	{
		return false;
	}

	// 选择内点数最多的拟合结果
	int nMaxInnerCount = 0;
	int nBestIndex = 0;
	for (int i = 0; i < vInnerCount.size(); i ++)
	{
		if (vInnerCount[i] > nMaxInnerCount)
		{
			nMaxInnerCount = vInnerCount[i];
			nBestIndex = i;
		}
	}
	double dfCenterX = vSphereCenter[nBestIndex].X;
	double dfCenterY = vSphereCenter[nBestIndex].Y;
	double dfCenterZ = vSphereCenter[nBestIndex].Z;
	
	// 根据球心的初值，计算位于球上的点
	vector< CHdVector3df > vPointOnSphere;
	double dfCenterToOrigin = sqrt(dfCenterX * dfCenterX + dfCenterY * dfCenterY + dfCenterZ * dfCenterZ);
	double dfUpAngleWithCenter = asin(dfRadius / dfCenterToOrigin);
	for (int i = 0; i < nPointCount; i ++)
	{
		double dfX = vPoint3d[i].X;
		double dfY = vPoint3d[i].Y;
		double dfZ = vPoint3d[i].Z;
		double dfDisToOrigin = sqrt(dfX * dfX + dfY * dfY + dfZ * dfZ);
		double dfAngle = acos((dfX * dfCenterX + dfY * dfCenterY + dfZ * dfCenterZ) / dfDisToOrigin / dfCenterToOrigin);
		double dfDisToCenter = sqrt((dfX - dfCenterX) * (dfX - dfCenterX) + (dfY - dfCenterY) * (dfY - dfCenterY) +
			(dfZ - dfCenterZ) * (dfZ - dfCenterZ));
		if (dfAngle < dfUpAngleWithCenter * 0.95 && dfDisToCenter < dfRadius2 && dfDisToCenter > dfRadius1)
		{
			vPointOnSphere.push_back(vPoint3d[i]);
		}
	}

	// 采用半径已知的拟合方法重新进行拟合
	CHdVector3dd pointCenter(dfCenterX, dfCenterY, dfCenterZ);
	if (!FitFixedSphere(vPointOnSphere, dfRadius, pointCenter))
	{
		return false;
	}
	dfCenterX = pointCenter.X;
	dfCenterY = pointCenter.Y;
	dfCenterZ = pointCenter.Z;

	// 计算内点数和标准差
	int nInnerCount = 0;
	double dfStdDev = 0;
	for (int i = 0; i < nPointCount; i ++)
	{
		double dfX = vPoint3d[i].X;
		double dfY = vPoint3d[i].Y;
		double dfZ = vPoint3d[i].Z;
		double dfDisToCenter = sqrt((dfX - dfCenterX) * (dfX - dfCenterX) + (dfY - dfCenterY) * (dfY - dfCenterY) +
			(dfZ - dfCenterZ) * (dfZ - dfCenterZ));
		if (dfDisToCenter < dfRadius2 && dfDisToCenter > dfRadius1)
		{
			nInnerCount ++;

			dfStdDev += (dfDisToCenter - dfRadius) * (dfDisToCenter - dfRadius);
		}
	}
	dfStdDev = sqrt(dfStdDev / nInnerCount);

	sphere.x = dfCenterX;
	sphere.y = dfCenterY;
	sphere.z = dfCenterZ;
	sphere.fitPtCount = nInnerCount;
	sphere.stdDev = dfStdDev;

	return true;
}

bool CHdSearchSphere::FitFixedSphere(vector< CHdVector3df >& vPoint3d, double dfRadius, CHdVector3dd& sphereCenter)
{
	unsigned int nCount = vPoint3d.size();
	double dfCenterX = sphereCenter.X;
	double dfCenterY = sphereCenter.Y;
	double dfCenterZ = sphereCenter.Z;
	double dfR_2 = dfRadius * dfRadius;

	double *pA = new double[nCount * 3];
	double *pB = new double[nCount];
	hdMatrix matA(nCount, 3), matB(nCount, 1), matAT(3, nCount), matATA(3, 3), matATB(3, 1), matATAI(3, 3), matResult(3, 1);

	int nIterCount = 0;
	while (true)
	{
		for (unsigned int i = 0; i < nCount; i ++)
		{
			double dfX = vPoint3d[i].X;
			double dfY = vPoint3d[i].Y;
			double dfZ = vPoint3d[i].Z;
			pA[i * 3] = dfCenterX - dfX;
			pA[i * 3 + 1] = dfCenterY - dfY;
			pA[i * 3 + 2] = dfCenterZ - dfZ;
			pB[i] = (dfR_2 - (dfCenterX - dfX) * (dfCenterX - dfX) - (dfCenterY - dfY) * (dfCenterY - dfY) -
				(dfCenterZ - dfZ) * (dfCenterZ - dfZ)) / 2;
		}
		
		matA.SetData(pA, nCount * 3);
		matB.SetData(pB, nCount);
		matA.Transpose(matAT);
		matAT.Multiply(matA, matATA);
		matAT.Multiply(matB, matATB);
		if (!matATA.Invert(matATAI))
		{
			delete []pA;
			delete []pB;
			return false;
		}
		matATAI.Multiply(matATB, matResult);
		double dfCenterXIncrease = matResult(0, 0);
		double dfCenterYIncrease = matResult(1, 0);
		double dfCenterZIncrease = matResult(2, 0);

		if (abs(dfCenterXIncrease) < 0.0001 && abs(dfCenterYIncrease) < 0.0001 && abs(dfCenterZIncrease) < 0.0001)
		{
			break;
		}
		dfCenterX += dfCenterXIncrease;
		dfCenterY += dfCenterYIncrease;
		dfCenterZ += dfCenterZIncrease;

		nIterCount ++;
		if (nIterCount == 20)
		{
			delete []pA;
			delete []pB;
			return false;
		}
	}

	sphereCenter.X = dfCenterX;
	sphereCenter.Y = dfCenterY;
	sphereCenter.Z = dfCenterZ;
	
	delete []pA;
	delete []pB;

	return true;
}
bool CHdSearchSphere::FitSphere(vector< CHdVector3df >& vPoint3d, CHdVector3dd& sphereCenter, double& dfRadius)
{
	double pA[16], pB[4];
	for (int i = 0; i < 16; i ++)
	{
		pA[i] = 0;
	}
	for (int i = 0; i < 4; i ++)
	{
		pB[i] = 0;
	}

	unsigned int nCount = vPoint3d.size();
	for (unsigned int i = 0; i < nCount; i ++)
	{
		double dfX = vPoint3d[i].X;
		double dfY = vPoint3d[i].Y;
		double dfZ = vPoint3d[i].Z;
		double dfX_2 = dfX * dfX;
		double dfY_2 = dfY * dfY;
		double dfZ_2 = dfZ * dfZ;
		double dfS = dfX_2 + dfY_2 + dfZ_2;

		pA[0] += dfX_2;
		pA[1] += dfX * dfY;
		pA[2] += dfX * dfZ;
		pA[3] += dfX;
		pA[5] += dfY_2;
		pA[6] += dfY * dfZ;
		pA[7] += dfY;
		pA[10] += dfZ_2;
		pA[11] += dfZ;
		pB[0] += - dfX * dfS;
		pB[1] += - dfY * dfS;
		pB[2] += - dfZ * dfS;
		pB[3] += - dfS;
	}
	pA[4] = pA[1];
	pA[8] = pA[2];
	pA[9] = pA[6];
	pA[12] = pA[3];
	pA[13] = pA[7];
	pA[14] = pA[11];
	pA[15] = nCount;

	hdMatrix matA(4, 4, pA), matB(4, 1, pB);
	hdMatrix matAI(4, 4);
	if (!matA.Invert(matAI))
	{
		return false;
	}
	hdMatrix matResult(4, 1);
	matAI.Multiply(matB, matResult);

	sphereCenter.X = - matResult(0, 0) / 2;
	sphereCenter.Y = - matResult(1, 0) / 2;
	sphereCenter.Z = - matResult(2, 0) / 2;
	double dfRadius_2 = (matResult(0, 0) * matResult(0, 0) + matResult(1, 0) * matResult(1, 0) + 
		matResult(2, 0) * matResult(2, 0)) / 4 - matResult(3, 0);
	if (dfRadius_2 < 0)
	{
		return false;
	}
	dfRadius = sqrt(dfRadius_2);

	return true;
}
bool CHdSearchSphere::FitCircle(vector< CHdVector2df >& vPoint2d, double& dfCenterX, double& dfCenterY,
	double& dfRadius, double& dfMaxDis)
{
	double pArray1[9], pArray2[3];
	for (int i = 0; i < 8; i ++)
	{
		pArray1[i] = 0;
	}
	for (int i = 0; i < 3; i ++)
	{
		pArray2[i] = 0;
	}

	vector< CHdVector2df >::iterator iterPoint = vPoint2d.begin();
	vector< CHdVector2df >::iterator iterPointEnd = vPoint2d.end();
	while(iterPoint != iterPointEnd)
	{
		double dfX = (*iterPoint).X;
		double dfY = (*iterPoint).Y;
		double dfX_2 = dfX * dfX;
		double dfY_2 = dfY * dfY;
		double dfXY = dfX * dfY;

		pArray1[0] += dfX_2;
		pArray1[1] += dfXY;
		pArray1[2] += dfX;
		pArray1[4] += dfY_2;
		pArray1[5] += dfY;
		pArray2[0] += - dfX * (dfX_2 + dfY_2);
		pArray2[1] += - dfY * (dfX_2 + dfY_2);
		pArray2[2] += - dfX_2 - dfY_2;

		++ iterPoint;
	}
	pArray1[3] = pArray1[1];
	pArray1[6] = pArray1[2];
	pArray1[7] = pArray1[5];
	pArray1[8] = vPoint2d.size();

	// 求解方程组
	hdMatrix mat1(3, 3, pArray1);
	hdMatrix mat2(3, 1, pArray2);
	hdMatrix matInverse(3, 3);
	if (!mat1.Invert(matInverse))
	{
		return false;
	}
	hdMatrix matResult(3, 1);
	matInverse.Multiply(mat2, matResult);

	double a = matResult(0, 0);
	double b = matResult(1, 0);
	double c = matResult(2, 0);
	dfCenterX = - a / 2;
	dfCenterY = - b / 2;
	double dfRadius_2 = dfCenterX * dfCenterX + dfCenterY * dfCenterY - c;
	if (dfRadius_2 < 0)
	{
		return false;
	}
	dfRadius = sqrt(dfRadius_2);
	
	dfMaxDis = 0;
	iterPoint = vPoint2d.begin();
	while (iterPoint != iterPointEnd)
	{
		double dfX = (*iterPoint).X;
		double dfY = (*iterPoint).Y;
		double dfDis = abs(sqrt((dfX - dfCenterX) * (dfX - dfCenterX) + (dfY - dfCenterY) * (dfY - dfCenterY)) - dfRadius);
		dfMaxDis = max(dfMaxDis, dfDis);
		++ iterPoint;
	}

	return true;
}