// hnPointCloud.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "hnPointCloud.h"
#include "..\hnCommon\hnCommonDef.h"
#include "..\hnCommon\hnCompare.h"
#include "..\hnAlgorithm\hnPlaneFit.h"
#include <algorithm>
using namespace std;

namespace hnPtCloud
{
	hnPointCloud::hnPointCloud()
	{

	}

	hnPointCloud::~hnPointCloud()
	{

	}

	// 根据左上角、左下角以及右上角、点云接口获取深度信息
	bool hnPointCloud::getDiseaseDepth(hn3dPointD ptLU, hn3dPointD ptLD, hn3dPointD ptRU, vector<CSeaPointCloud*> vecPcd, double& dDepth)
	{
		// 计算中心
		hn3dPointD ptCenter;
		ptCenter.x = (ptLU.x + ptLD.x) / 2.0;
		ptCenter.y = (ptLU.y + ptLD.y) / 2.0;
		ptCenter.z = 0.0;

		// 方向向量
		hn3dPointD ptNormal;
		double dDist = sqrt((ptLU.x - ptRU.x)*(ptLU.x - ptRU.x) + (ptLU.y - ptRU.y)*(ptLU.y - ptRU.y));
		ptNormal.x = (ptRU.x - ptLU.x) / dDist;
		ptNormal.y = (ptRU.y - ptLU.y) / dDist;
		ptNormal.z = 0.0;

		double dMinX = sqrt((ptLU.x + ptLD.x)*(ptLU.x + ptLD.x) + (ptLU.y + ptLD.y)*(ptLU.y + ptLD.y));
		double dMaxX = dMinX / 2.0;
		dMinX = -dMinX / 2.0;

		double dMinY = -dDist - 0.5;
		double dMaxY = -0.5;

	    // 获取宽度
		double dWidth = sqrt((ptLU.x - ptLD.x)*(ptLU.x - ptLD.x) + (ptLU.y - ptLD.y)*(ptLU.y - ptLD.y)) + 1.0;
		ptCenter.x = ptCenter.x + ptNormal.x * dDist / 2.0;
		ptCenter.y = ptCenter.y + ptNormal.y * dDist / 2.0;
		dDist = dDist + 1.0;

		// 获取中心高度
		if (!getCenterZ(vecPcd, ptCenter, ptNormal))
		{
			return false;
		}

		// 点云数据
		vector<CSeaPointCloud*> vecPcds;

		// 判断是否有点云
		if (!getValidPtCloud(ptCenter, vecPcd, vecPcds))
		{
			return false;
		}

		// 创建包围盒
		vector<hn3dPointD> vecBoxs;
		hn3dPointD pOr(0.0, 0.0, 0.0);
		getGMPtBoxCoord(pOr, dWidth / 2.0, dWidth / 2.0, 20.0, dDist, vecBoxs);

		// 创建转换模型
		CBursaWolfModel pTransModel = createTransModel(ptCenter, ptNormal);

		vector<hn3dPointD> vecPts;
		vector<hn3dPointD> vecAllPt;

		// 包围盒
		CHdBox3dd pSelBox;
		getBox(vecBoxs, pSelBox);

		// 点云数据
		CSeaPointCloud* pPcd = NULL;

		// 点云转换模型
		CBursaWolfModel pPcdModel;

		// 点云包围盒
		CHdBox3df pPcdBox;

		m_vecPt.clear();

		// 遍历所有点云
		for (int i = 0; i < vecPcds.size(); i++)
		{
			pPcd = vecPcds[i];
			if (!pPcd)
			{
				continue;
			}

			vecPts.clear();

			// 获取点云包围盒
			pPcdModel = pPcd->GetModel();

			// 裁切点云的包围盒
			getRelativeBox(pTransModel, pPcdModel, vecBoxs, pPcdBox);

			if (!getPtCloud(pPcd, pPcdBox, pSelBox, pTransModel, vecPts, 1))
			{
				continue;
			}

			vecAllPt.insert(vecAllPt.end(), vecPts.begin(), vecPts.end());
		}

		if (vecPts.size() <= 0)
		{
			return false;
		}

		// 计算深度
		////////////////////////////
		//FILE* pf = fopen("D:\\123.xyz", "w");
		//if (pf)
		//{
		//	for (int j = 0; j < m_vecPt.size(); j++)
		//	{
		//		fprintf(pf, "%lf,%lf,%lf,255,0,0\n", m_vecPt[j].x, m_vecPt[j].y, m_vecPt[j].z);
		//	}
		//	
		//	fclose(pf);
		//}
		/////////////////////////////////////

		// 过滤噪点
		vector<hn3dPointD> vecOriPt = vecPts;
		filterRoadPt(vecPts);
		if (vecPts.size() <= 0)
		{
			dDepth = 0.0;
			return false;
		}

		vecOriPt = vecPts;

		///////////////////
		//pf = fopen("D:\\123_filter.xyz", "w");
		//if (pf)
		//{
		//	for (int i = 0; i < outPts.size(); i++)
		//	{
		//		fprintf(pf, "%lf,%lf,%lf,0,0,255\n", outPts[i].m_x, outPts[i].m_y, outPts[i].m_z);
		//	}

		//	fclose(pf);
		//}
		/////////////////////////////////

		//拟合平面
		hnPlaneFit planeFit;
		planeFit.LSPlaneFit(vecPts);
		double dA, dB, dC, dD;
		dA = dB = dC = dD = 0.0;
		planeFit.GetVector(dA, dB, dC);
		dD = planeFit.getDValue();

		double dValue = dA*dA + dB*dB + dC*dC;
		if (dValue <= 0)
		{
			dDepth = 0.0;
			return false;
		}
		//////////////////////

		////// 获取面上投影点
		//double dx,dy,dz;
		//dx = dy = dz = 0.0;
		//pf = fopen("D:\\123_Project.xyz", "w");
		//if (pf)
		//{
		//	for (int i = 0; i < outPts.size(); i++)
		//	{
		//		dx = (dB*dB + dC*dC)*outPts[i].m_x -dA*(dB* outPts[i].m_y + dC*outPts[i].m_z + dD);
		//		dx = dx / dValue;

		//		dy = (dA*dA + dC*dC)*outPts[i].m_y - dB*(dA*outPts[i].m_x + dC*outPts[i].m_z + dD);
		//		dy = dy / dValue;

		//		dz = (dA*dA + dB*dB)*outPts[i].m_z - dC*(dA*outPts[i].m_x + dB*outPts[i].m_y + dD);
		//		dz = dz / dValue;

		//		fprintf(pf, "%lf,%lf,%lf,0,255,0\n", dx, dy, dz);
		//	}


		//	fclose(pf);
		//}



		/////////////////////////////////
		//// 先按照Y排序，分段过滤
		//sort(vecPts.begin(), vecPts.end(), compareYBy3dPoint);
		//double dBegY = vecPts[0].y;
		//double dEndY = vecPts[vecPts.size() - 1].y;
		//int nGridCnt = (int)((dEndY - dBegY) / 0.5) + 1;
		//vector<vector<double>> vecGridPts;
		//vecGridPts.resize(nGridCnt);

		//// 分段
		//int nIndex = 0;
		//double dHeight = 0.0;
		//for (int i = 0; i < vecPts.size(); i++)
		//{
		//	nIndex = (vecPts[i].y - dBegY) / 0.5;

		//	dHeight = (vecPts[i].x*dA + vecPts[i].y * dB + vecPts[i].z * dC + dD) / sqrt(dA*dA + dB*dB + dC*dC);

		//	if (dHeight > 0.2 || dHeight < -0.2)
		//	{
		//		continue;
		//	}

		//	vecGridPts[nIndex].push_back(dHeight);
		//}

		//// 确定深度
		//dDepth = -1000.0;
		//for (int i = 0; i < vecGridPts.size(); i++)
		//{
		//	if (vecGridPts[i].size() <= 0)
		//	{
		//		continue;
		//	}

		//	// 排序
		//	sort(vecGridPts[i].begin(), vecGridPts[i].end());
		//	//dHeight = abs(vecGridPts[i][vecGridPts[i].size()*0.03] - vecGridPts[i][vecGridPts[i].size()*0.97]);
		//	dHeight = abs(vecGridPts[i][vecGridPts[i].size()*0.03]);

		//	if (dHeight > dDepth)
		//	{
		//		dDepth = dHeight;
		//	}

		//	dHeight = abs(vecGridPts[i][vecGridPts[i].size()*0.97]);

		//	if (dHeight > dDepth)
		//	{
		//		dDepth = dHeight;
		//	}
		//}

		// 获取病害范围内的点云
		vector<double> vecHeight;
		double dHeight = 0.0;
		vector<hn3dPointD> vecOut;
		dWidth = (dWidth - 0.7)/2.0;
		dDist = (dDist - 0.7) / 2.0;
		for (int i = 0; i < vecOriPt.size(); i++)
		{
			//if (vecOriPt[i].x <= dWidth && vecOriPt[i].x >= -dWidth && vecOriPt[i].y <= dDist && vecOriPt[i].y >= -dDist)
			if (vecOriPt[i].x <= dMaxX && vecOriPt[i].x >= dMinX && vecOriPt[i].y <= dMaxY && vecOriPt[i].y >= dMinY)
			{
				dHeight = (vecOriPt[i].x*dA + vecOriPt[i].y * dB + vecOriPt[i].z * dC + dD) / sqrt(dA*dA + dB*dB + dC*dC);

				if (dHeight > 0.2 || dHeight < -0.2)
				{
					continue;
				}

				dHeight =  abs(dHeight);

				vecHeight.push_back(dHeight);

				//vecOut.push_back(vecOriPt[i]);
			}
		}

		if (vecHeight.size() <= 0)
		{
			dDepth = -1;
			return false;
		} 
		// 排序
		sort(vecHeight.begin(), vecHeight.end());

		//dDepth = abs(vecHeight[vecHeight.size()*0.1] - vecHeight[vecHeight.size()*0.9]);
		dDepth = abs(vecHeight[vecHeight.size()*0.9]);

		//pf = fopen("D:\\124.xyz", "w");
		//if (pf)
		//{
		//	for (int j = 0; j < vecOut.size(); j++)
		//	{
		//		fprintf(pf, "%lf,%lf,%lf,0,255,0\n", vecOut[j].x, vecOut[j].y, vecOut[j].z);
		//	}

		//	fclose(pf);
		//}

		// 

		//vector<double> vecHeight;
		//vecHeight.resize(outPts.size());
		//int nCount = 0;
		//double dHeight = 0.0;
		//for (int i = 0; i < outPts.size(); i++)
		//{
		//	dHeight = (outPts[i].m_x*dA + outPts[i].m_y * dB + outPts[i].m_z * dC + dD) / sqrt(dA*dA + dB*dB + dC*dC);

		//	if (dHeight > 0.2 || dHeight < - 0.2)
		//	{
		//		continue;
		//	}

		//	vecHeight[nCount] = dHeight;
		//	++nCount;
		//}

		//vecHeight.resize(nCount);

		//// 排序
		//sort(vecHeight.begin(), vecHeight.end());
		//dDepth = abs(vecHeight[vecHeight.size()*0.01] - vecHeight[vecHeight.size()*0.99]);
		return true;
	}

	// 获取点云数据--点云包围盒以及选择包围盒
	bool hnPointCloud::getPtCloud(CSeaPointCloud* pPtCloud, CHdBox3df pPcdBox, CHdBox3dd pSelBox, CBursaWolfModel pTransModel, vector<hn3dPointD>& vecOutPt, int nType)
	{
		// hlz读取对象
		CHdCoreData* hlzReader = NULL;

		CHdLevel* plevel = NULL;

		CHdListAreaNoInf ShowList;

		//点云个数
		size_t nPtIndex = 0;

		// 点云包围盒
		CBursaWolfModel pPcdModel = pPtCloud->GetModel();

		// 构建hlz读取对象
		hlzReader = dynamic_cast<CHdCoreData*>(pPtCloud->m_hlzReader);

		if (!hlzReader)
		{
			return false;
		}

		// 获得1层数据--抽希效率高
		plevel = hlzReader->GetLevelRec(1);

		if (!plevel)
		{
			return false;
		}

		// 获取与包围盒相交的点云对象
		hlzReader->GetIntersectAreas(plevel, pPcdBox, &ShowList);

		//分配内存
		vecOutPt.clear();
		vecOutPt.resize(INITALLOCMEMORY);

		//点云个数
		nPtIndex = 0;

		//临时点
		hn3dPointD pPoint3d;

		hn3dPointD ptTemp;

		for (size_t k = 0; k < ShowList.size(); k++)
		{
			// 获取数据
			hlzReader->LoadSpecParcel(ShowList.at(k));
			CHdParcelBase *& pHdParcelBase = *(ShowList.data() + k);

			if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
			{
				continue;
			}

			// 获取数据
			for (int i = 0; i < pHdParcelBase->GetPtCount(); i++)
			{
				HlzPoint * pt = (pHdParcelBase->m_pHlzPoint + i);

				if (!pt->isValid())
				{
					continue;
				}

				// 点云的点
				pPoint3d.x = pt->x;
				pPoint3d.y = pt->y;
				pPoint3d.z = pt->z;

				// 转换点云
				pPcdModel.Translate(pPoint3d.x, pPoint3d.y, pPoint3d.z);

				ptTemp = pPoint3d;

				// 转换成自定义坐标
				pTransModel.AntiTranslate(pPoint3d.x, pPoint3d.y, pPoint3d.z);

				// 在包围盒范围内
				if (isPointInsideBox(pSelBox, pPoint3d))
				{
					if (0 == nType)
					{
						pPoint3d.y = 0.0;
					}

				//	m_vecPt.push_back(ptTemp);
					push_DataVector(vecOutPt, nPtIndex, pPoint3d);
				}
			}

			// 取消load
			hlzReader->UnLoadSpecParcel(pHdParcelBase);
		}

		//清除多余点云
		vecOutPt.erase(vecOutPt.begin() + nPtIndex, vecOutPt.end());

		return true;
	}

	// 获取绝对坐标包围盒
	void hnPointCloud::getRelativeBox(CBursaWolfModel pTransModel, CBursaWolfModel pcdModle, vector<hn3dPointD> vecBoxs,
		CHdBox3df& pcdBox)
	{
		// 包围盒坐标
		double dminX, dminY, dminZ, dmaxX, dmaxY, dmaxZ;
		dminX = dminY = dminZ = MINDATA;
		dmaxX = dmaxY = dmaxZ = MAXDATA;

		// 点坐标
		double dx, dy, dz;
		dx = dy = dz = 0.0;

		//FILE* pf = fopen("D:\\box.txt", "w");

		// 遍历所有顶点
		for (int i = 0; i < vecBoxs.size(); i++)
		{
			dx = vecBoxs[i].x;
			dy = vecBoxs[i].y;
			dz = vecBoxs[i].z;

			// 将坐标转换成84坐标
			pTransModel.Translate(dx, dy, dz);

			//if (pf)
			//{
			//	fprintf(pf, "%lf,%lf,%lf,255,0,0\n", dx, dy, dz);
			//}

			// 转换成点云相对坐标
			pcdModle.AntiTranslate(dx, dy, dz);

			if (dx > dmaxX)
			{
				dmaxX = dx;
			}

			if (dx < dminX)
			{
				dminX = dx;
			}

			if (dy > dmaxY)
			{
				dmaxY = dy;
			}

			if (dy < dminY)
			{
				dminY = dy;
			}

			if (dz > dmaxZ)
			{
				dmaxZ = dz;
			}

			if (dz < dminZ)
			{
				dminZ = dz;
			}
		}

		//fclose(pf);

		pcdBox.MinEdge.set(dminX, dminY, dminZ);
		pcdBox.MaxEdge.set(dmaxX, dmaxY, dmaxZ);
	}

	// 获取包围盒
	void hnPointCloud::getBox(vector<hn3dPointD> vecBoxs, CHdBox3dd& pcdBox)
	{
		// 包围盒坐标
		double dminX, dminY, dminZ, dmaxX, dmaxY, dmaxZ;
		dminX = dminY = dminZ = MINDATA;
		dmaxX = dmaxY = dmaxZ = MAXDATA;

		// 点坐标
		double dx, dy, dz;
		dx = dy = dz = 0.0;

		// 遍历所有顶点
		for (int i = 0; i < vecBoxs.size(); i++)
		{
			dx = vecBoxs[i].x;
			dy = vecBoxs[i].y;
			dz = vecBoxs[i].z;

			if (dx > dmaxX)
			{
				dmaxX = dx;
			}

			if (dx < dminX)
			{
				dminX = dx;
			}

			if (dy > dmaxY)
			{
				dmaxY = dy;
			}

			if (dy < dminY)
			{
				dminY = dy;
			}

			if (dz > dmaxZ)
			{
				dmaxZ = dz;
			}

			if (dz < dminZ)
			{
				dminZ = dz;
			}
		}

		pcdBox.MinEdge.set(dminX, dminY, dminZ);
		pcdBox.MaxEdge.set(dmaxX, dmaxY, dmaxZ);
	}

	// 向vector中添加数据
	template<typename Type> bool hnPointCloud::push_DataVector(vector<Type>& vecData, size_t& nSize, Type& pData)
	{
		//超过vector容量，则重新分配，并将原来容器数据拷贝到新的容器
		if (nSize >= vecData.size())
		{
			size_t nAllocMemory = nSize + nSize / ALLOCMEMORYSTEP;

			vector<Type> vecNewData;

			try
			{
				vecNewData.resize(nAllocMemory);
			}
			catch (...)
			{
				return false;
			}

			//拷贝数据
			for (size_t i = 0; i < nSize; i++)
			{
				*(vecNewData._Myfirst() + i) = vecData[i];
			}

			vecData = vecNewData;
		}

		*(vecData._Myfirst() + nSize) = pData;

		nSize += 1;

		return true;
	}

	// 判断点云点是否在包围盒内  
	bool hnPointCloud::isPointInsideBox(CHdBox3dd box, hn3dPointD point)
	{
		if (point.x >= box.MinEdge.X - 0.000001 && point.y >= box.MinEdge.Y - 0.000001 && point.z >= box.MinEdge.Z - 0.000001
			&&point.x <= box.MaxEdge.X + 0.000001 && point.y <= box.MaxEdge.Y + 0.000001 && point.z <= box.MaxEdge.Z + 0.000001)
		{
			return true;
		}

		return false;
	}

	// 获取与中心点有交集的点云数据
	bool hnPointCloud::getValidPtCloud(hn3dPointD& pCenterPt, vector<CSeaPointCloud*> vecOriCloud,vector<CSeaPointCloud*>& vecPtCloud)
	{
		int  value =  vecOriCloud.size(); 
		if (value <= 0)
		{
			return false;
		}

		// 包围盒坐标
		double dminX, dminY, dminZ, dmaxX, dmaxY, dmaxZ;
		dminX = dminY = dminZ = dmaxX = dmaxY = dmaxZ = 0.0;

		// 遍历点云
		for (int i = 0; i < vecOriCloud.size(); i++)
		{
			if (!vecOriCloud[i])
			{
				continue;
			}

			// 获取点云的绝对坐标包围盒
			vecOriCloud[i]->m_hlzHeader.getGlobalExtent(dminX, dminY, dminZ, dmaxX, dmaxY, dmaxZ);

			// 判断点是否在包围盒内
			if (pCenterPt.x > dmaxX || pCenterPt.x < dminX || pCenterPt.y > dmaxY || pCenterPt.y < dminY)
			{
				continue;
			}

			vecPtCloud.push_back(vecOriCloud[i]);
		}

		if (vecPtCloud.size() <= 0)
		{
			return false;
		}

		return true;
	}

	// 创建自定义坐标系与84坐标系转换模型
	CBursaWolfModel hnPointCloud::createTransModel(hn3dPointD pCenterPt, hn3dPointD pNormal)
	{
		//绕Z轴旋转角度
		double dYaw = 0.0;
		dYaw = atan2((double)pNormal.y, (double)pNormal.x) * 57.29577951308233;
		if (dYaw < 0)
		{
			dYaw += 360.0;	// 将水平角转换到[0,360)之间
		}

		dYaw = dYaw - 90;
		dYaw = dYaw * hd::DEGTORAD64;

		// 绕Y轴旋转角度计算
		double vertiAngle = atan2((double)pNormal.z,
			sqrt((double)pNormal.x * (double)pNormal.x + (double)pNormal.y * (double)pNormal.y)) * 57.29577951308233;

		// 归算至[0,360]
		if (vertiAngle < 0)
		{
			vertiAngle += 360.0;
		}

		vertiAngle = vertiAngle * hd::DEGTORAD64;

		// 转换矩阵
		CBursaWolfModel pTransModel;

		//计算转换矩阵
		pTransModel.IrrAngle2RotateMatrix(vertiAngle, 0.0, dYaw);

		//设置偏移量
		pTransModel.m_fOffset[0] = pCenterPt.x;
		pTransModel.m_fOffset[1] = pCenterPt.y;
		pTransModel.m_fOffset[2] = pCenterPt.z;
		pTransModel.Parameter2matrix();

		return pTransModel;
	}

	// 获取全局坐标的点云数据
	bool hnPointCloud::getCloudGlobalCoord(vector<CSeaPointCloud*> vecPcds, vector<hn3dPointD> vecBoxPts, CBursaWolfModel pTransModel,
		vector<hn3dPointD>& vecPts)
	{
		// 包围盒
		CHdBox3dd pSelBox;
		getBox(vecBoxPts, pSelBox);

		// 点云数据
		CSeaPointCloud* pPcd = NULL;

		// 点云转换模型
		CBursaWolfModel pPcdModel;

		// 点云包围盒
		CHdBox3df pPcdBox;

		// hlz读取对象
		CHdCoreData* hlzReader = NULL;

		CHdLevel* plevel = NULL;

		CHdListAreaNoInf ShowList;

		//点云个数
		size_t nPtIndex = 0;

		//分配内存
		vecPts.clear();
		vecPts.resize(INITALLOCMEMORY);

		//临时点
		hn3dPointD pPoint3d;

		// 全局
		hn3dPointD pGlobalPt;

		// 遍历所有点云
		for (int j = 0; j < vecPcds.size(); j++)
		{
			pPcd = vecPcds[j];
			if (!pPcd)
			{
				continue;
			}

			// 获取点云包围盒
			pPcdModel = pPcd->GetModel();

			// 裁切点云的包围盒
			getRelativeBox(pTransModel, pPcdModel, vecBoxPts, pPcdBox);

			// 点云包围盒
			CBursaWolfModel pPcdModel = pPcd->GetModel();

			// 构建hlz读取对象
			hlzReader = dynamic_cast<CHdCoreData*>(pPcd->m_hlzReader);

			if (!hlzReader)
			{
				continue;
			}

			// 获得0层数据
			plevel = hlzReader->GetLevelRec(0);

			if (!plevel)
			{
				continue;
			}

			// 获取与包围盒相交的点云对象
			hlzReader->GetIntersectAreas(plevel, pPcdBox, &ShowList);

			for (size_t k = 0; k < ShowList.size(); k++)
			{
				// 获取数据
				hlzReader->LoadSpecParcel(ShowList.at(k));
				CHdParcelBase *& pHdParcelBase = *(ShowList.data() + k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				// 获取数据
				for (int i = 0; i < pHdParcelBase->GetPtCount(); i++)
				{
					HlzPoint * pt = (pHdParcelBase->m_pHlzPoint + i);

					if (!pt->isValid())
					{
						continue;
					}

					// 点云的点
					pPoint3d.x = pt->x;
					pPoint3d.y = pt->y;
					pPoint3d.z = pt->z;

					// 转换点云
					pPcdModel.Translate(pPoint3d.x, pPoint3d.y, pPoint3d.z);

					pGlobalPt = pPoint3d;

					// 转换成自定义坐标
					pTransModel.AntiTranslate(pPoint3d.x, pPoint3d.y, pPoint3d.z);

					// 在包围盒范围内
					if (isPointInsideBox(pSelBox, pPoint3d))
					{
						push_DataVector(vecPts, nPtIndex, pGlobalPt);
					}
				}

				// 取消load
				hlzReader->UnLoadSpecParcel(pHdParcelBase);
			}

		}

		//清除多余点云
		vecPts.erase(vecPts.begin() + nPtIndex, vecPts.end());

		return true;
	}

	// 获取Z值
	bool hnPointCloud::getCenterZ(vector<CSeaPointCloud*> vecOriPcd,hn3dPointD& pCenter, hn3dPointD pNormal)
	{
		// 点云数据
		vector<CSeaPointCloud*> vecPcds;

		// 判断是否有点云
		if (!getValidPtCloud(pCenter, vecOriPcd, vecPcds))
		{
			return false;
		}

		// 创建包围盒
		vector<hn3dPointD> vecBoxs;
		hn3dPointD pOri(0.0, 0.0, 0.0);
		getGMPtBoxCoord(pOri, 0.01, 0.01, 1000000.0,
			0.05, vecBoxs);

		// 创建转换模型
		CBursaWolfModel pTransModel = createTransModel(pCenter, pNormal);

		vector<hn3dPointD> vecPts;

		// 获取该里程钢轨纵断面点云数据
		getCloudGlobalCoord(vecPcds, vecBoxs, pTransModel, vecPts);

		if (vecPts.size() <= 0)
		{
			return false;
		}

		sort(vecPts.begin(), vecPts.end(), compareZBy3dPoint);

		double dMidZ = (vecPts[0].z + vecPts[vecPts.size() - 1].z) / 2.0;
		for (int i = 0; i < vecPts.size(); i++)
		{
			if (vecPts[i].z < dMidZ)
			{
				continue;
			}

			vecPts.erase(vecPts.begin() + i);
			i--;
		}
		////////////////////

		pCenter.z = vecPts[(int)(vecPts.size()*0.9)].z;

		return true;
	}

	// 设置当前包围盒
	void hnPointCloud::getGMPtBoxCoord(hn3dPointD pCenter, double dLeft, double dRight, double dHeight,
		double dThickness, vector<hn3dPointD>& vecBox)
	{
		// 厚度
		hn3dPointD pPoint3d;
		vecBox.clear();

		//包围盒顶部0号点
		pPoint3d.x = pCenter.x - dLeft;
		pPoint3d.y = pCenter.y + dThickness / 2;
		pPoint3d.z = pCenter.z - dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部1号点
		pPoint3d.x = pCenter.x + dRight;
		pPoint3d.y = pCenter.y + dThickness / 2;
		pPoint3d.z = pCenter.z - dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部2号点
		pPoint3d.x = pCenter.x + dRight;
		pPoint3d.y = pCenter.y - dThickness / 2;
		pPoint3d.z = pCenter.z - dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部3号点
		pPoint3d.x = pCenter.x - dLeft;
		pPoint3d.y = pCenter.y - dThickness / 2;
		pPoint3d.z = pCenter.z - dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部4号点
		pPoint3d.x = pCenter.x - dLeft;
		pPoint3d.y = pCenter.y + dThickness / 2;
		pPoint3d.z = pCenter.z + dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部5号点
		pPoint3d.x = pCenter.x + dRight;
		pPoint3d.y = pCenter.y + dThickness / 2;
		pPoint3d.z = pCenter.z + dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部6号点
		pPoint3d.x = pCenter.x + dRight;
		pPoint3d.y = pCenter.y - dThickness / 2;
		pPoint3d.z = pCenter.z + dHeight / 2.0;

		vecBox.push_back(pPoint3d);

		//包围盒顶部7号点
		pPoint3d.x = pCenter.x - dLeft;
		pPoint3d.y = pCenter.y - dThickness / 2;
		pPoint3d.z = pCenter.z + dHeight / 2.0;

		vecBox.push_back(pPoint3d);
	}

	// 过滤路面点云
	bool hnPointCloud::filterRoadPt(vector<hn3dPointD>& vecPt)
	{
		if (vecPt.size() <= 0)
		{
			return false;
		}

		int nBeg = 0;
		int nEnd = 0;
		double dDist = 0.0;
		int nSelCnt = 0;

		// 先按照Y排序，分段过滤
		sort(vecPt.begin(), vecPt.end(), compareYBy3dPoint);
		double dBegY = vecPt[0].y;
		double dEndY = vecPt[vecPt.size() - 1].y;
		int nGridCnt = (int)((dEndY - dBegY) / 2.0) + 1;
		vector<vector<hn3dPointD>> vecGridPts;
		vecGridPts.resize(nGridCnt);

		// 分段
		int nIndex = 0;
		for (int i = 0; i < vecPt.size(); i++)
		{
			nIndex = (vecPt[i].y - dBegY) / 2.0;
			vecGridPts[nIndex].push_back(vecPt[i]);
		}

		// 过滤后的点
		vector<hn3dPointD> vecFilterPt;

		// 对每段进行过滤
		for (int j = 0; j < vecGridPts.size(); j++)
		{
			vecPt = vecGridPts[j];
			if (vecPt.size() <= 0)
			{
				continue;
			}

			// 先按X值排序
			sort(vecPt.begin(), vecPt.end(), compareXBy3dPoint);

			// 按半径过滤
			for (int i = 0; i < vecPt.size(); i++)
			{
				nBeg = i - 200;
				nEnd = i + 200;
				if (nBeg < 0)
				{
					nBeg = 0;
				}

				if (nEnd >= vecPt.size())
				{
					nEnd = vecPt.size() - 1;
				}

				nSelCnt = 0;

				for (int j = nBeg; j <= nEnd; j++)
				{
					if (j == i)
					{
						continue;
					}

					dDist = sqrt((vecPt[j].x - vecPt[i].x)*(vecPt[j].x - vecPt[i].x) +
						(vecPt[j].z - vecPt[i].z)*(vecPt[j].z - vecPt[i].z));

					if (dDist < 0.02)
					{
						++nSelCnt;
					}

					if (nSelCnt >= 5)
					{
						break;
					}
				}

				if (nSelCnt < 5)
				{
					vecPt.erase(vecPt.begin() + i);
					--i;
				}

			}

			if (vecPt.size() <= 0)
			{
				continue;
			}

			// 先按Z值排序
			sort(vecPt.begin(), vecPt.end(), compareZBy3dPoint);

			double dMid = 0.0;
			int nCount = 0;
			for (int i = vecPt.size()*0.8; i < vecPt.size()*0.9; i++)
			{
				dMid += vecPt[i].z;
				nCount++;
			}

			dMid = dMid / nCount;

			// 按中值过滤
			for (int i = 0; i < vecPt.size(); i++)
			{
				if (abs(vecPt[i].z - dMid) > 0.05)
				{
					vecPt.erase(vecPt.begin() + i);
					--i;
				}
			}

			if (vecPt.size() <= 0)
			{
				continue;
			}

			vecFilterPt.insert(vecFilterPt.end(), vecPt.begin(), vecPt.end());
		}

		vecPt = vecFilterPt;

		if (vecPt.size() <= 0)
		{
			return false;
		}

		return true;
	}
}

