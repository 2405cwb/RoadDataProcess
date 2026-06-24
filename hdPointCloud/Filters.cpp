#include "StdAfx.h"
#include "Filters.h"

#include "..\hdCore\hdMatrix.h"
#include "..\hdCommon\hdkdtree.hpp"
#include "..\hdCommon\hdHdiStruct.h"
#include <time.h>
#include "..\hdCommon\hdSceneStr.h"
#include "..\hdHlslib\HL2Writer.h"
#include "..\hdHlslib\HLSReadOpener.h"
#include "..\..\hdPointCloud\hdSysSetting.h"
//using namespace hd::app;
using namespace hd; 
using namespace hdkdtree;

typedef KDTreeSingleIndexAdaptor<
	L2_Simple_Adaptor<float, PointCloud > ,
	PointCloud,
	3 /* dim */
> my_kd_tree_t;

namespace hd
{

	CFilters::CFilters(void)
	{
	}


	CFilters::~CFilters(void)
	{
	}

	void CFilters::OutlierFilter( PointCloud& ptCloud, bool select, int index, int neighbour,float disThreshold,float assignThreshold,void (*processCallback)(float,const char*))
	{
		int *delpointID  = new int [(u32)(ptCloud.count())];
		memset(delpointID,-1, sizeof(int) * ((size_t)(ptCloud.count())));
		//全屏处理
		int col = ptCloud.m_simpleHeader.number_of_col;
		int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum = (int)(ptCloud.m_simpleHeader.number_of_point_records);
		int fIndex  = 0;int nIndex = 0;
		int fineptnum = 0,allptnum = 0;

		double *Martrix = new double[3*3];
		//double *IMartrix = new double[3*3];
		double *DesArr = new double[3];
		double * sol = new double[3];
		memset(Martrix,0,sizeof(double)*9);
		memset(DesArr,0,sizeof(double)*3);
		memset(sol,0,sizeof(double)*3);
		double *DisArr = new double [neighbour * neighbour];
		int *DisArrID = new int [neighbour * neighbour];
		memset(DisArr,-1,sizeof(double)*neighbour * neighbour);
		memset(DisArrID,-1,sizeof(int)*neighbour * neighbour);
		int *ngbr = new int[neighbour * neighbour];
		memset(ngbr,-1,sizeof(int)*neighbour*neighbour);
		int i = 0,j = 0, k = 0;
		int p = 0,q = 0;
		int m = 0,n = 0;
		int ptNumNgbr = 0;
		int count = 0;
		//for ( i = 0;i < col;i++)
		//{
		//	for ( j = 0;j < row;j++)
		//	{
		u32 loopCount = ptCloud.getLoopCount();
		for (u32 i = 0;i < loopCount;i++)
		{
			hdVector<PointXYZIPRGBA>& pts = ptCloud.getLoop(n);
			for (unsigned int j = 0;j < pts.size();j++)
			{
				count++;
				PointXYZIPRGBA& ptf = *(pts._Myfirst + j);
				//当前点云的索引号
				fIndex = (i * (int)pts.size() + j);
				//PointXYZIPRGBA& ptf = ptCloud[fIndex];
				if (!ptf.isValid())
				{
					continue;
				}
				///////////////////通过平面拟合来判定/////////////////////////////////
				//首先计算邻域内得点的个数 至少大于3 且三点不能共线
				for ( p = 0; p < neighbour;p++)
				{
					for ( q = 0; q< neighbour;q++)
					{
						if (neighbour % 2 == 1)
						{
							//注意越界问题
							if (i - (neighbour - 1)/2 + p >= 0 && i - (neighbour - 1)/2 + p < col && 
								(j - (neighbour - 1)/2 + q) >= 0 && (j - (neighbour - 1)/2 + q) < row)
							{
								nIndex = (i - (neighbour - 1)/2 + p)*row + (j - (neighbour - 1)/2 + q);
							}
							else
								continue;

						}
						else
							if (i - (neighbour - 1)/2 + p >= 0 && i - (neighbour - 1)/2 + p < col && 
								(j - (neighbour - 1)/2 + q) >= 0 && (j - (neighbour - 1)/2 + q) < row)
							{
								nIndex = (i - neighbour/2 + p)*row + (j - neighbour/2 + q);
							}
							else
								continue;

						if (nIndex < 0 )
						{
							continue;
						}
						PointXYZIPRGBA& ptn = ptCloud[nIndex];
						//判定点的有效性
						if (ptn.isValid())
						{
							ptNumNgbr++;
							ngbr[ p*neighbour + q ] = nIndex;
						}
					}
				}
				//进行平面拟合
				if (ptNumNgbr < 3)  
				{
					continue; //如果点数小于3,无法进行拟合,进入下一次循环
				}
				//判定三个点是否共面,通过计算两条直线的夹角是否为180度
				if (ptNumNgbr == 3)
				{
					PointXYZIPRGBA& pt1 = ptCloud[ngbr[0]],pt2 = ptCloud[ngbr[1]],pt3 = ptCloud[ngbr[2]];
					float costheta = ((pt1.x - pt2.x)*(pt3.x - pt2.x) + (pt1.y - pt2.y)*(pt3.y - pt2.y) + (pt1.z - pt2.z)*(pt3.z - pt2.z))
						/(sqrt((pt1.x - pt2.x)*(pt1.x - pt2.x) + (pt1.y - pt2.y)*(pt1.y - pt2.y) + (pt1.z - pt2.z)*(pt1.z - pt2.z))
						*(sqrt((pt3.x - pt2.x)*(pt3.x - pt2.x) + (pt3.y - pt2.y)*(pt3.y - pt2.y) + (pt3.z - pt2.z)*(pt3.z - pt2.z))));
					if ( fabs(costheta + 1) < 0.0000001 )
					{
						continue;
					}
				}
				for( p = 0;p < neighbour*neighbour;p++ )
				{
					int t = ngbr[p];

					if (ngbr[p] > 0)
					{
						PointXYZIPRGBA& pt = ptCloud[ngbr[p]];
						Martrix[0] += pt.x * pt.x;
						Martrix[1] += pt.x * pt.y;
						Martrix[2] += pt.x;
						Martrix[3] = Martrix[1];
						Martrix[4] += pt.y * pt.y;
						Martrix[5] += pt.y;
						Martrix[6] = Martrix[2];
						Martrix[7] = Martrix[5];
						Martrix[8] += 1;

						DesArr[0] += pt.x * pt.z;
						DesArr[1] += pt.y * pt.z;
						DesArr[1] += pt.z;
					}	
				}
				//系数矩阵求逆
				invers_matrix(Martrix,3);

				mult(Martrix,DesArr,sol,3,3,1);

				//计算每个点到平面的距离,进行排序
				for ( p = 0;p <neighbour*neighbour;p++)
				{
					if (ngbr[p] > 0)
					{
						PointXYZIPRGBA& pt = ptCloud[ngbr[p]];
						double dis = fabs(sol[0]*(pt.x) + sol[1]*(pt.y) + sol[2] - (pt.z));
						dis = dis/sqrt(sol[0]*sol[0] + sol[1]*sol[1] + 1*1);
						//将距离超过阈值的点号存储
						if (dis > disThreshold)
						{
							DisArrID[m] = ngbr[p];
							DisArr[m] = dis;
							m++;
						}
					}
				}
				//计算超过点所占得比例是否超过分配阈值
				if ( m <= (int)(assignThreshold * ptNumNgbr) + 1)
				{
					//如果小于分配阈值
					//将所有超过距离阈值的点都删除
					for ( p = 0; p< m; p++)
					{
						delpointID[DisArrID[p]] = 1;
					}
				}
				if ( m >(int)(assignThreshold * ptNumNgbr) + 1)
				{
					//如果大于分配阈值，则将距离较远的几个点删除,四舍五入？
					int num = (int)(ptNumNgbr*assignThreshold) + 1;
					double disTemp;int idTemp;
					//将距离进行排序,冒泡法进行排序，大的距离值放在前面
					for ( p = 0;p< m;p++)
					{
						for ( q = p + 1;q < m; q++)
						{
							if (DisArr[p] > 0 && DisArr[q] > 0)
							{
								if (DisArr[p] < DisArr [q])
								{
									disTemp = DisArr[p];
									DisArr[p] = DisArr[q];
									DisArr[q] = disTemp;
									//对应的ID也要排放顺序
									idTemp = DisArrID[p];
									DisArrID[p] = DisArrID[q];
									DisArrID[q] = idTemp;
								}
							}
						}
					}
					for ( p = 0; p< num; p++)
					{
						delpointID[DisArrID[p]] = 1;
					}
				}
				//将申请的内存赋初值
				memset(Martrix,0,sizeof(double)*9);
				memset(DesArr,0,sizeof(double)*3);
				memset(sol,0,sizeof(double)*3);
				memset(DisArr,-1,sizeof(double)*neighbour * neighbour);
				memset(DisArrID,-1,sizeof(int)*neighbour * neighbour);
				memset(ngbr,-1,sizeof(int)*neighbour * neighbour);
				ptNumNgbr = 0;
				m = 0;

				///////////////////////////////////////////////////////////////////////////////////////////
				if (processCallback && (fIndex % 10000) == 0)
				{
					processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
		}
		//将过滤后的点赋为无效点
		for (unsigned int i = 0;i < ptCloud.count(); i++)
		{
			if (delpointID[i] == 1)
			{
				ptCloud[i].x = 0.0f;
				ptCloud[i].y = 0.0f;
				ptCloud[i].z = 0.0f;
				ptCloud[i].intensity = 0;
			}
		}
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		delete []ngbr;
		delete []Martrix;
		delete []DesArr;
		delete []sol;
		delete []DisArr;
		delete []DisArrID;
		delete []delpointID;

	}
	//离群过滤原理
	//计算当前点到扫描中心的距离，以及计算出当前点邻域内的点到扫描中心的距离，之后计算距离差值，
	//计算出这些小于阈值的距离差值的个数，计算出个小于个数所占的比例，如果小于比例阈值，则视为噪点
	//该方法仅对于规则点云适用
	//两种调用方式：1.在工作区右键菜单下，为全部处理
	//			    2.在使用选择工具进行选择操作后，右键菜单，处理点云内部选中的点
	//
	int CFilters::OutlierFilterFaro( PointCloud& ptCloud, bool select, int neighbour,float disThreshold,float assignThreshold,void(*processCallback)(float,const char*))
	{
		int col = ptCloud.m_simpleHeader.number_of_col;
		int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum = int(ptCloud.m_simpleHeader.number_of_point_records);

		int fIndex  = 0;int nIndex = 0;
		int fineptnum = 0,allptnum = 0;
		int i = 0,j = 0;
		int p = 0,q = 0;
		int count = 0;
		assignThreshold = assignThreshold/100;

		//判定是否为规则点云
		//规则点云
		{
			for ( i = 0;i < col;i++)
			{
				for ( j = 0;j < row;j++)
				{

					//当前点云的索引号
					fIndex = (i * row + j);
					if (fIndex >= ptCloud.count())
					{
						continue;
					}

					PointXYZIPRGBA& pt = ptCloud[fIndex];
					if (!pt.isValid())
					{
						continue;
					}
					//选择模式，需要判定当前点是否被选中
					if (select)
					{
						if (!pt.isSelected())
						{
							continue;
						}
					}
					///////////////////////////////////////////////////////////////////
					///////////////////通过距离来判定/////////////////////////////////
					///////////////////////////////////////////////////////////////////
					float dist1 = std::sqrt(pt.x * pt.x 
						+ pt.y * pt.y 
						+ pt.z * pt.z);
					for (int p = 0; p < neighbour;p++)
					{
						for (int q = 0; q< neighbour;q++)
						{
							if (neighbour % 2 == 1)
							{
								//注意越界问题
								if (i - (neighbour - 1)/2 + p >= 0 && i - (neighbour - 1)/2 + p < col && 
									(j - (neighbour - 1)/2 + q) >= 0 && (j - (neighbour - 1)/2 + q) < row)
								{
									nIndex = (i - (neighbour - 1)/2 + p)*row + (j - (neighbour - 1)/2 + q);
								}
								else
									continue;
							}
							else
								if (i - (neighbour - 1)/2 + p >= 0 && i - (neighbour - 1)/2 + p < col && 
									(j - (neighbour - 1)/2 + q) >= 0 && (j - (neighbour - 1)/2 + q) < row)
								{
									nIndex = (i - neighbour/2 + p)*row + (j - neighbour/2 + q);
								}
								else
									continue;

							//验证点的有效性
							if (nIndex < 0 || nIndex >= ptCloud.count())
							{
								continue;
							}

							const PointXYZIPRGBA& ptn = ptCloud[nIndex];
							//判定点的有效性
							//if (ptCloud[nIndex].isValid())
							//{
							//	allptnum++;
							//}
							//计算距离
							float dist2 =  std::sqrt(ptn.x * ptn.x + ptn.y * ptn.y + ptn.z * ptn.z);

							float deltaDis = abs(dist1 - dist2);

							if (deltaDis < disThreshold)
							{
								fineptnum++;
							}
						}
					}
					//判定是否小于分配阈值
					if ((float)fineptnum/(neighbour*neighbour) < assignThreshold )
					{
						//pt.select = 4;//(pt.select | 0x4);
						//pt.setDeleted();
						pt.setSelected();
						count++;
					}
					else
					{
						// 如果点被选中 则将不满足滤波条件的点设置为不选中
						if (pt.isSelected())
						{	
							pt.setUnSelected();
						}
					}
					fineptnum = 0;
					//allptnum = 0;
					///////////////////////////////////////////////////////////////////////////////////////////
					if (processCallback && (fIndex % 10000) == 0)
					{
						processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
					}
				}
			}
		}
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}

	double CFilters::GetDistToPos(PointCloud& ptCloud, PointXYZIPRGBA& pt, int nRow)
	{
		double dDist = 0.0;

		// 转换到大地坐标
		double M[16];
		ptCloud.m_header.computeMatrix(M);

		double x = pt.x;
		double y = pt.y;
		double z = pt.z;
		hdHomogeneousTransformPoint(M,x,y,z);

		if (nRow < 0 || nRow >= m_vecIScanPos.size())
		{
			return dDist;
		}

		// 获得对应圈的POS位置
		double dScale = (double)nRow / m_vecIScanPos.size();		
		int scanPtIndex = (int)(dScale * m_vecIScanPos.size());
		const HD_SCANHDIINFO& scanPos = m_vecIScanPos[scanPtIndex];
		//const HD_SCANHDIINFO& scanPos = *(m_vecIScanPos._Myfirst + nRow);

		double cx = scanPos.dX;
		double cy = scanPos.dY;
		double cz = scanPos.dZ;
		irr::core::vector3df pp((float)(x - cx),(float)(y - cy),(float)(z - cz));

		// 返回距离
		dDist = sqrt(pp.X*pp.X + pp.Y*pp.Y + pp.Z*pp.Z);

		return dDist;
	}

	double CFilters::GetAngleToPos(PointCloud& ptCloud, PointXYZIPRGBA& pt, int nRow)
	{
		double dAngle = -1.0f;

		// 转换到大地坐标
		double M[16];
		ptCloud.m_header.computeMatrix(M);

		double x = pt.x;
		double y = pt.y;
		double z = pt.z;
		hdHomogeneousTransformPoint(M,x,y,z);

		if (nRow < 0 || nRow >= m_vecIScanPos.size())
		{
			return dAngle;
		}

		// 获得对应圈的POS位置
		double dScale = (double)nRow / m_vecIScanPos.size();		
		int scanPtIndex = (int)(dScale * m_vecIScanPos.size());
		const HD_SCANHDIINFO& scanPos = m_vecIScanPos[scanPtIndex];
		//const HD_SCANHDIINFO& scanPos = *(m_vecIScanPos._Myfirst + nRow);

		double cx = scanPos.dX;
		double cy = scanPos.dY;
		double cz = scanPos.dZ;
		irr::core::vector3df pp((float)(x - cx),(float)(y - cy),(float)(z - cz));

		// 返回角度
		hd::CHdVector3df lineVec = hd::CHdVector3df((float)(x - cx),(float)(y - cy),(float)(z - cz));
		hd::CHdVector3df ZVec =  hd::CHdVector3df(0.f,0.f,1.f);

		return lineVec.getAngleTo(ZVec);
	}

	bool CFilters::IsX330CricleNoise(PointXYZIPRGBA& pt, PointCloud& ptCloud, u32 nLoop)
	{
		// 到扫描仪中心四个距离的点判断为噪点
		double dDist2pos = GetDistToPos(ptCloud, pt, nLoop);

		// 去除扫描头附近的噪声 [zfei]
		if (dDist2pos < 1.f)
		{
			return true;
		}

		if (dDist2pos > 76.f && dDist2pos < 77.f)
		{
			return true;
		}
		else if (dDist2pos > 153.f && dDist2pos < 154.f)
		{
			return true;
		}
		else if (dDist2pos > 230.f && dDist2pos < 231.f)
		{
			return true;
		}
		else if (dDist2pos > 306.f && dDist2pos < 307.f)
		{
			return true;
		}
		else if (dDist2pos > 382.f && dDist2pos < 383.f)
		//else if (dDist2pos > 330.f)
		{
			return true;
		}

		return false;
	}

	int CFilters::X330NoiseAnalysis(PointCloud& ptCloud, u32 nLoopSt, hdBlkArray<PointXYZIPRGBA>& ptBuf, double dPosDistIngore,	int nFieldSize,	double dOutlierDistThre, int nOutlierRatio)
	{
		// 过滤参数设置
		int nNeighbour = (nFieldSize - 1)/2;					// 临域大小:7*7(2*nNeighbour + 1 = 7)
		double dDistThre = dOutlierDistThre;					// 距离阈值
		double dRatioThre = (double)nOutlierRatio/100;			// 比例阈值

		//内存中点云的总圈数
		u32 blkCount = ptBuf.blockCount();	

		//*********************将距离POS的距离分成50段，每段为10米,用于过滤离群操作之后未去除的不分点*********************//
		std::vector<std::vector<PointXYZIPRGBA*> > mapDist2Pos;
		mapDist2Pos.resize(50);
		int nValidCount = 0;
		//************************************************************************//

		for (int i = 0; i<blkCount; i++)
		{
			hdVector<PointXYZIPRGBA>& loopPts = ptBuf.getBlock(i);
			u32 nLoopCount = loopPts.size();
			for (u32 j = 0; j < nLoopCount; j++)
			{
				// 利用离群思想判断目标点是否为噪点
				PointXYZIPRGBA& pt = *(loopPts._Myfirst + j);
				if (!pt.isValid())
				{
					continue;
				}

				// 先根据噪点圆盖的半径标记点云[2014/5/23]
				if (IsX330CricleNoise(pt, ptCloud, i + nLoopSt))
				{
					pt.x = 0.0f;
					pt.y = 0.0f;
					pt.z = 0.0f;
					continue;
				}

				//******************离群思想标记噪点***************************//
				int nOutlierPtsNum = 0;				// 临域内离群的个数
				int nNearVaildPtsNum = 0;			// 临域内有效地点数

#pragma region 统计临域点

				// 目标点到POS的距离
				double dDistOrig = GetDistToPos(ptCloud,pt,i + nLoopSt);

				// 如果当前点到POS的距离小于40，则不过滤
				if (dDistOrig < dPosDistIngore/*40.f*/)
				{
					continue;	
				}

				for (int p = -nNeighbour; p <= nNeighbour; p++)
				{
					for (int q = -nNeighbour; q <= nNeighbour; q++)
					{
						// 防止越界
						if ((i + p) < 0 || (j + q) < 0 || (i + p) >= blkCount || (j + q) >= nLoopCount) 
						{
							continue;
						}

						// 自身不统计
						if (p == 0 && q == 0)
						{
							continue;
						}

						// 临域的点序号
						hdVector<PointXYZIPRGBA>& nearLoopPts = ptBuf.getBlock(i + p);

						// 目标的临域点
						if ((j + q) < 0 || (j + q) >= nearLoopPts.size())
						{
							continue;
						}
						PointXYZIPRGBA& ptNear = nearLoopPts[j + q];
						if (!ptNear.isValid())
						{
							continue;
						}

						// 临域点到POS的距离
						double distNear = GetDistToPos(ptCloud,ptNear, nLoopSt + i + p);

						// 临域点和目标点
						double deltaDist = fabs(distNear - dDistOrig);

						if (deltaDist > dDistThre)
						{
							// 临域内离群点数
							nOutlierPtsNum++;
						}

						nNearVaildPtsNum++;
					}
				}
#pragma endregion 统计临域点

				if ((nNearVaildPtsNum < 5) || ((double)nOutlierPtsNum/nNearVaildPtsNum > dRatioThre) )
				{
					pt.x = 0.0f;
					pt.y = 0.0f;
					pt.z = 0.0f;
				}
				else
				{
					// 角度小于60°范围内的点才统计，防止地面上的点（距离POS又比较远）参与了统计[zfei 2014/7/9]
					double dAngle = GetAngleToPos(ptCloud, pt, i + nLoopSt);
					if (dAngle < 0.f || dAngle > 60.f)
					{
						continue;
					}

					// 统计过滤之后的点云分布
					int nMapPos = (int)floor(dDistOrig/10.0);
					nMapPos = clamp(nMapPos, 0, 49);
					std::vector<PointXYZIPRGBA*>& pts = mapDist2Pos[nMapPos];
					pts.push_back(&pt);

					nValidCount++;
				}
				
				//**********************************************************//
			}
		}

		//*****将到Pos的距离比例小于总有效点数1%(改为小于100个点)区段内的点置为无效点*******//
		for (int i = 0; i < mapDist2Pos.size(); i++)
		{
			std::vector<PointXYZIPRGBA*>& pts = mapDist2Pos[i];
			int nSize = pts.size();
			if (nSize > 0 && nSize < /*(nValidCount * 0.1)*/100)
			{
				for (std::vector<PointXYZIPRGBA*>::iterator it = pts.begin(); it != pts.end(); it++)
				{
					(*it)->x = 0.0f;
					(*it)->y = 0.0f;
					(*it)->z = 0.0f;
					//(*it)->setSelected();
				}
			}
		}
		//**********************************************************//
		

		return 0;
	}

	bool CFilters::LoadInLinFile(string& linPath)
	{
		// 读取lin文件中每一圈pos位置
		FILE* fp = fopen(linPath.c_str(),"r");
		if (!fp)
			return false;

		char str[1024] = {0};
		unsigned int loopCount = 0;
		fgets(str,1024,fp);		//第一圈跳过

		if (m_vecIScanPos.size() > 0)
		{ 
			m_vecIScanPos.clear();
		}
		int bufSize = 10000;
		m_vecIScanPos.resize(bufSize);

		while(!feof(fp))
		{
			memset(str,0,1024);
			fgets(str,1024,fp);
			HD_SCANHDIINFO& hdiInfo = m_vecIScanPos[loopCount];
			if (hdiInfo.Serialize(str))
			{
				loopCount++;

				if (loopCount >= m_vecIScanPos.size())
				{
					m_vecIScanPos.resize(m_vecIScanPos.size() + bufSize);
				}
			}
		}
		fclose(fp);
		m_vecIScanPos.resize(loopCount);

		return true;
	}

	int CFilters::OutlierAndDistFilterFaro(PointCloud& ptCloud, string& linPath, double dPosDistIngore,	int nFieldSize,	double dOutlierDistThre, int nOutlierRatio, int nProcPcdIndex, void (*processCallback)(float,const char*))
	{
		if (!ptCloud.isNormalPointCloud())
		{
			// 不需要一定是规则点云[2014/6/27]
			//return -1;
		}

		// 读取Lin文件
		LoadInLinFile(linPath);

		// 将过滤后的点云写到另一个文件中
		CHL2Writer hlsWrite;
		hlsWrite.m_header = ptCloud.m_header;
		hlsWrite.m_header.set_pointformat(HLS2_POINTFORMAT_XYZIRGBP);
		string strNewHls = ptCloud.GetPointCloudPath();
		if (_access(strNewHls.data(),04) != 0)
		{
			return -1;
		}
		int nPos = strNewHls.find_last_of('.');
		strNewHls.replace(nPos,11,"-Filter.hls");	
		if (hlsWrite.Open(strNewHls.data()) == FALSE)
		{
			fprintf(stderr, "ERROR: Could not open hlsPath ");
			return -2;
		}

		// 从文件中读取要处理的点云
		IHLSReader* hlsReader = NULL;
		CHLSReadOpener hlsOpener;
		hlsReader = hlsOpener.Open(ptCloud.GetPointCloudPath().data());
		if(hlsReader == NULL)
		{
			fprintf(stderr, "ERROR: Could not open hlsPath ");
			return -2;
		}
		int loopCount = hlsReader->GetLoopCount();
		hlsReader->GetLoopIndex();

		// 从文件一次读取50圈
		u32 nLoopBuf = 50;	

		// 50圈的条带缓存
		hdBlkArray<PointXYZIPRGBA> stripBuffer;

		int rowCount = ptCloud.m_header.number_of_row;
		int colCount = ptCloud.m_header.number_of_col;
		for (u32 nLoopSt = 0; nLoopSt < colCount; nLoopSt+=nLoopBuf)
		{
			stripBuffer.clear();
			stripBuffer.setBlockCount(nLoopBuf);

			int nRealLoopCount = 0;
			for (u32 i=0; i < nLoopBuf; i++)
			{
				hdVector<PointXYZIPRGBA>& loopPts = stripBuffer.getBlock(i);

				// 从文件中读取正圈点云，必须包含无效点
				if (nLoopSt + i < colCount)
				{
					hlsReader->ReadLoopFull(loopPts,nLoopSt + i);
					nRealLoopCount++;
				}
			}

			// 实际的圈数，解决数据的最后可能没有50圈的问题
			stripBuffer.setBlockCount(nRealLoopCount);

			// 对点云进行噪点分析
			X330NoiseAnalysis(ptCloud, nLoopSt, stripBuffer, dPosDistIngore, nFieldSize, dOutlierDistThre, nOutlierRatio);

			// 将处理结果写入新的文件
			for (int k = 0; k<nRealLoopCount; k++)
			{
				hdVector<PointXYZIPRGBA>& loopPts = stripBuffer.getBlock(k);
				hlsWrite.WriteLoop(loopPts._Myfirst, loopPts.size());
			}

			// 进度和剩余时间
			char strProcess[1024];
			//double dLeftColRatio = floor((double)(colCount - nLoopSt)/colCount);
			int nLeftTimeSecs = (int)ceil(((colCount - nLoopSt)*SECONDSPRELOOP)/1000);
			int nLeftTimeMins = nLeftTimeSecs/60;
			nLeftTimeSecs = nLeftTimeSecs%60;
			sprintf(strProcess,"正在过滤第%d个点云...%d分 %d秒", nProcPcdIndex, nLeftTimeMins, nLeftTimeSecs);
			if (processCallback && (nLoopSt % 100) == 0)
			{
				processCallback((float)nLoopSt/colCount,strProcess);
			}
		}
		hlsWrite.Close();
		hlsReader->Close();
		if (hlsReader)
		{
			delete hlsReader;
			hlsReader = NULL;
		}

		if (processCallback)
		{
			processCallback(0.0, /*msg*/HDSCENE_IDS_FINISH);
		}
		
		return 0;
	}


	//统计过滤 原理
	//搜索当前点最临近的m个点，计算这些点到当前点的距离，之后计算这些距离值的平均值，从而用于代表当前点的密度参数
	//之后对距离平均值进行统计分析，求出这些平均值的均值与方差
	//之后基于统计规律，保留在均值加减n*stddev内的点，去除掉阈值范围之外的点，从而达到过滤的效果
	//该方法对于规则点云与不规则点云均适用
	//两种调用方式：1.在工作区右键菜单下，为全部处理
	//			    2.在使用选择工具进行选择操作后，右键菜单，处理点云内部选中的点
	//领域m越大，分布曲线更平缓，m越小，分布曲线越陡峭
	//因子n越大，去除的噪点越少，保留的点越多，反之
	int CFilters::OutlierFilterStatistic( PointCloud& ptCloud, bool select, int neighbour,double Threshold,void(*processCallback)(float,const char*))
	{
		//全屏处理
		//该方法的邻域不能通过行列号来获取，需要计算在三维空间上的邻域来计算
		//使用kdtree来获取点邻域
		if(processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FILTER_REF_CREATING);
		}

		bool bBuildIdxSuccess = false;
		//选择点 只对选择集构建索引
		if (select)
		{
			bBuildIdxSuccess = ptCloud.buildIndex(false);
		}
		else
		{
			bBuildIdxSuccess = ptCloud.buildIndex(true);
		}

		if (!bBuildIdxSuccess)
		{
			return 0;
		}
		//int col = ptCloud.m_simpleHeader.number_of_col;
		//int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum;
		if (select)
		{
			//获取选择集个数
			pointNum = ptCloud.getSelectCount();
		}
		else
		{
			pointNum = (int)ptCloud.count();
		}
		//= ptCloud.count();//ptCloud.m_simpleHeader.number_of_point_records;
		double *disatance = new double [pointNum];
		memset(disatance,-1,sizeof(double)*(pointNum));
		int fIndex  = 0;int nIndex = 0;
		std::vector<size_t>   ret_index(neighbour);
		std::vector<float> out_dist_sqr(neighbour);
		int i = 0,j = 0;
		int p = 0;
		int count = 0;
		double meanDis = 0.0;
		double stddev = 0.0;
		float queryPt[3]; 

		for ( i = 0;i < pointNum;i++)
		{
			if (select)
			{
				PointXYZIPRGBA& pt = ptCloud.getSelectionPoint(i);	

				if (!pt.isValid())
				{
					continue;
				}
				// 查询当前点云的邻域
				queryPt[0] = pt.x;
				queryPt[1] = pt.y;
				queryPt[2] = pt.z;
			}
			else
			{
				PointXYZIPRGBA& pt = ptCloud[i];

				if (!pt.isValid())
				{
					continue;
				}
				// 查询当前点云的邻域
				queryPt[0] = pt.x;
				queryPt[1] = pt.y;
				queryPt[2] = pt.z;
			}

			//点云的ID和距离
			ptCloud.knnSearch(queryPt,neighbour,&ret_index[0],&out_dist_sqr[0]);
			//除去当前选中的点
			for (p = 1;p<neighbour;p++)
			{
				nIndex = ret_index[p];
				meanDis += out_dist_sqr[p];
			}
			// 求均值
			meanDis /= (neighbour - 1);
			disatance[i] = meanDis;
			count++;
			//将下次要用的变量值进行再次初始化
			meanDis = 0;

			///////////////////////////////////////////////////////////////////////////////////////////
			if (processCallback && (i % 10000) == 0)
			{
				processCallback(i/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
			}
		}

		double sum = 0,sq_sum = 0;
		// 求整体点云的均值与方差
		for (i = 0;i <pointNum;i++)
		{
			if (disatance[i] > 0)
			{
				sum += disatance[i];
				sq_sum += disatance[i] * disatance[i];
			}
			//	stddev += (out_dist_sqr[p] - meanDis)*(out_dist_sqr[p] - meanDis);
		}
		meanDis = sum /count;
		stddev = (double)(sq_sum - sum * sum / count) / (count - 1);
		stddev = sqrt(stddev);
		count = 0;
		//判定是否小于阈值
		for (p = 0;p <pointNum;p++)
		{
			if (disatance[p] > 0)
			{
				if (disatance[p] < meanDis - Threshold*stddev || disatance[p] > meanDis + Threshold*stddev)
				{
					if (select)
					{
						PointXYZIPRGBA& pt = ptCloud.getSelectionPoint(p);
						//pt.select = 4;//(pt.select | 0x4);
						pt.setSelected();
						count++;
					}
					else
					{
						PointXYZIPRGBA& pt = ptCloud[p];
						//pt.select = 4;//(pt.select | 0x4);
						pt.setSelected();
						count++;
					}
				}
				else
				{
					// 对于选中点过滤，需要将不需要过滤的点去除选中 fengjing
					if (select)
					{
						PointXYZIPRGBA& pt = ptCloud.getSelectionPoint(p);
						if (pt.isSelected())
						{
							pt.setUnSelected();
						}
					}
					else
					{
						PointXYZIPRGBA& pt = ptCloud[p];
						if (pt.isSelected())
						{
							pt.setUnSelected();
						}
					}
				}
			}
		}
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		delete []disatance;
		disatance = NULL;
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}

	//该方法对规则点云与不规则点云均适用
	int CFilters::DistanceFilter(PointCloud& ptCloud, bool select, float minDis,float maxDis,void (*processCallback)(float,const char*))
	{
		if (minDis >= maxDis)
		{
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FILTER_RIGHT_THRESHOLD);
			}
			return 0;
		}
		int col = ptCloud.m_simpleHeader.number_of_col;
		int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum;
		if (select)
		{
			pointNum = ptCloud.getSelectCount();
		}
		else
		{
			pointNum = (int)(ptCloud.m_simpleHeader.number_of_point_records);
		}
		int fIndex  = 0;
		int i = 0,j = 0, p = 0;
		int count = 0;
		float cx = (float)ptCloud.m_header.offsetX;
		float cy = (float)ptCloud.m_header.offsetY;
		float cz = (float)ptCloud.m_header.offsetZ;
		float dis = 0.0f;

		//判定是否为规则点云
		if (!ptCloud.isNormalPointCloud())
		{
			//不规则点云
			int count = 0;
			//构建索引
			bool bBuildIdxSuccess = false;
			//选择点 只对选择集构建索引
			if (select)
			{
				bBuildIdxSuccess = ptCloud.buildIndex(false);
			}
			else
			{
				bBuildIdxSuccess = ptCloud.buildIndex(true);
			}

			if (!bBuildIdxSuccess)
			{
				return 0;
			}

			for (int i = 0;i< pointNum;i++)
			{
				if (select)
				{
					PointXYZIPRGBA& pt = ptCloud.getSelectionPoint(i);
					if (!pt.isValid() || !pt.isSelected())
					{
						continue;
					}

					//邻域内的点数
					dis = std::sqrt(pt.x * pt.x + pt.y * pt.y +pt.z * pt.z);
					if (dis < minDis || dis > maxDis)
					{
						pt.setSelected();
						count++;
					}
					else
					{
						// 对于选中点过滤，要考虑将不需要过滤的点设置为非选中状态
						if (pt.isSelected())
						{
							pt.setUnSelected();
						}
					}
				}
				else
				{
					PointXYZIPRGBA& pt = ptCloud[i];

					if (!pt.isValid())
					{
						continue;
					}
					dis = std::sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
					if (dis < minDis || dis > maxDis)
					{
						pt.setSelected();
						count++;
					}
					else
					{
						// 对于选中点过滤，要考虑将不需要过滤的点设置为非选中状态
						if (pt.isSelected())
						{
							pt.setUnSelected();
						}
					}
				}

				///////////////////////////////////////////////////////////////////////////////////////////
				if (processCallback && (fIndex % 10000) == 0)
				{
					processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
		}
		else   //规则点云 对于规则点云，全部处理与部分处理的取点方式均为[]运算符
			//点云中的选择集数组存储的是点的ID，对于规则点云，是采用行列号顺序来进行
			// 遍历，通过选择集来取点需要进行转换，为避免转换，采用[]取点
		{
			//for ( i = 0;i < col;i++)
			//{
			//	for ( j = 0;j < row;j++)
			//	{
			//当前点云的索引号
			u32 loopCount = ptCloud.getLoopCount();
			for (u32 n = 0;n < loopCount;n++)
			{
				hdVector<PointXYZIPRGBA>& pts = ptCloud.getLoop(n);
				for (unsigned int i = 0;i < pts.size();i++)
				{
					PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if (!pt.isValid())
					{
						continue;
					}
					if (select)
					{
						if (!pt.isSelected())
						{
							continue;
						}
					}

					//邻域内的点数
					dis = std::sqrt(pt.x * pt.x + pt.y * pt.y + pt.z * pt.z);
					if (dis < minDis || dis > maxDis)
					{
						pt.setSelected();
						count++;
					}
					else
					{
						//在选中点过滤时 对于不需要过滤的点要取消选中
						if (pt.isSelected())
						{
							pt.setUnSelected();
						}
					}
					///////////////////////////////////////////////////////////////////////////////////////////
					if (processCallback && (i % 10000) == 0)
					{
						processCallback(i / (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
					}
				}
			}
			//fIndex = (i * row + j);
			//PointXYZIPRGBA& pt = ptCloud[fIndex];

			//	}
			//}
		}
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}

	//
	int CFilters:: RadiusFilter( PointCloud& ptCloud, bool select, float radius,int NumThreshold,void (*processCallback)(float,const char*))
	{
		//全屏处理
		long t1,t2,t3;
		t1 = clock();
		processCallback(0.0,HDSCENE_IDS_FILTER_REF_CREATING);
		//my_kd_tree_t   idx(3 /*dim*/, ptCloud, KDTreeSingleIndexAdaptorParams(10 /* max leaf */) );
		//idx.buildIndex();
		bool bBuildIdxSuccess = false;
		//选择点 只对选择集构建索引
		if (select)
		{
			bBuildIdxSuccess = ptCloud.buildIndex(false);
		}
		else
		{
			bBuildIdxSuccess = ptCloud.buildIndex(true);
		}

		if (!bBuildIdxSuccess)
		{
			return 0;
		}

		t2 = clock();
		int col = ptCloud.m_simpleHeader.number_of_col;
		int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum;
		if (select)
		{
			pointNum = ptCloud.getSelectCount();
		}
		else
		{
			pointNum = (int)ptCloud.count();
		}

		int fIndex  = 0;
		int i = 0,j = 0, p = 0;
		int count = 0;
		float queryPt[3]; 
		std::vector<std::pair<size_t,float> >   ret_matches;
		//hdkdtree::SearchParams params;
		//params.sorted = false;
		//判定是否为规则点云
		if (!ptCloud.isNormalPointCloud())
		{
			//不规则点云
			int count = 0;
			//构建索引
			//ptCloud.buildIndex();
			for (int i = 0;i< pointNum;i++)
			{
				if (select)
				{
					PointXYZIPRGBA& pt = ptCloud.getSelectionPoint(i);
					if (!pt.isValid())
					{
						continue;
					}
					// 查询当前点云的邻域
					queryPt[0] = pt.x;
					queryPt[1] = pt.y;
					queryPt[2] = pt.z;

					//邻域内的点数
					p = ptCloud.radiusSearch(queryPt,radius,ret_matches,false);
					if (p < NumThreshold)
					{
						//pt.select = 4;//(pt.select | 0x4);
						pt.setSelected();
						count++;
					}
				}
				else
				{
					PointXYZIPRGBA& pt = ptCloud[i];
					if (!pt.isValid())
					{
						continue;
					}
					// 查询当前点云的邻域
					queryPt[0] = pt.x;
					queryPt[1] = pt.y;
					queryPt[2] = pt.z;

					//邻域内的点数
					p = ptCloud.radiusSearch(queryPt,radius,ret_matches,false);
					if (p < NumThreshold)
					{
						//pt.select = 4;//(pt.select | 0x4);
						pt.setSelected();
						count++;
					}
				}
				///////////////////////////////////////////////////////////////////////////////////////////
				if (processCallback && (i % 10000) == 0)
				{
					processCallback(i/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
				}
			}

		}
		else   //规则点云
		{
			for ( i = 0;i < col;i++)
			{
				for ( j = 0;j < row;j++)
				{
					//当前点云的索引号
					fIndex = (i * row + j);
					PointXYZIPRGBA& pt = ptCloud[fIndex];
					if (!pt.isValid())
					{
						continue;
					}
					if (!pt.isSelected())
					{
						continue;
					}
					// 查询当前点云的邻域
					queryPt[0] = pt.x;
					queryPt[1] = pt.y;
					queryPt[2] = pt.z;
					//邻域内的点数
					p = ptCloud.radiusSearch(queryPt,radius,ret_matches,false);
					if (p < NumThreshold)
					{
						//pt.select = 4;//(pt.select | 0x4);
						pt.setSelected();
						count++;
					}
					t3 = clock();
					///////////////////////////////////////////////////////////////////////////////////////////
					if (processCallback && (fIndex % 10000) == 0)
					{
						processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
					}
				}
			}
		}

		t1 = t2 - t1;
		t2 = t3 - t2;
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}


	//平滑过滤：
	//计算当前点邻域中得点到该点的距离，如果距离值在给定阈值范围之内，则计算平均值，然后将平均值赋给邻域内的点
	//也即把当前点内邻域内的点进行平滑
	//该方法对于规则点云与不规则点云均适用
	int CFilters::SmoothFiler( PointCloud& ptCloud, bool select, int neighbour,float disThreshold,void (*processCallback)(float,const char*))
	{
		//全屏处理
		int col = ptCloud.m_simpleHeader.number_of_col;
		int row = ptCloud.m_simpleHeader.number_of_row;
		int pointNum;
		if (select)
		{
			pointNum = ptCloud.getSelectCount();
		}
		else
		{
			pointNum = (int)(ptCloud.m_simpleHeader.number_of_point_records);
		}

		int fIndex  = 0;int nIndex = 0;
		int fineptnum = 0,allptnum = 0;
		double *DisArr = new double [neighbour * neighbour];
		int *DisArrID = new int [neighbour * neighbour];
		memset(DisArr,-1,sizeof(double)*neighbour * neighbour);	memset(DisArrID,-1,sizeof(int)*neighbour * neighbour);
		int i = 0,j = 0;
		int p = 0,q = 0;
		int count = 0,m = 0;
		PointXYZIPRGBA pt;

		//判定是否为规则点云
		if (!ptCloud.isNormalPointCloud())
		{
			neighbour = neighbour*neighbour;
			//不规则点云
			std::vector<size_t>   ret_index(neighbour);
			std::vector<float> out_dist_sqr(neighbour);
			float queryPt[3]; 
			//构建索引
			bool bBuildIdxSuccess = false;
			//选择点 只对选择集构建索引
			if (select)
			{
				bBuildIdxSuccess = ptCloud.buildIndex(false);
			}
			else
			{
				bBuildIdxSuccess = ptCloud.buildIndex(true);
			}

			if (!bBuildIdxSuccess)
			{
				return 0;
			}
			//ptCloud.buildIndex();
			for (i = 0;i< pointNum;i++)
			{
				if (select)
				{
					PointXYZIPRGBA& ptf = ptCloud.getSelectionPoint(i);
					if (!ptf.isValid())
					{
						continue;
					}

					// 查询当前点云的邻域
					queryPt[0] = pt.x;
					queryPt[1] = pt.y;
					queryPt[2] = pt.z;
				}
				else
				{
					PointXYZIPRGBA& ptf = ptCloud[i];
					if (!ptf.isValid())
					{
						continue;
					}
					// 查询当前点云的邻域
					queryPt[0] = pt.x;
					queryPt[1] = pt.y;
					queryPt[2] = pt.z;
				}
				float Dist = 0.0f;int m = 0;
				//const PointXYZIPRGBA& ptf = ptCloud[i];

				ptCloud.knnSearch(queryPt,neighbour,&ret_index[0],&out_dist_sqr[0]);
				//邻域内的点数
				for (j = 0;j < neighbour;j++)
				{
					if (out_dist_sqr[j] < disThreshold)
					{
						DisArrID[fineptnum] = nIndex;
						fineptnum++;
					}
				}
				//求坐标的均值
				for (p = 0;p <fineptnum;p++)
				{
					PointXYZIPRGBA& ptd = ptCloud[DisArrID[p]];

					pt.x += ptd.x;
					pt.y += ptd.y;
					pt.z += ptd.z;
					// 平滑处理只作用与坐标即可-zhubo.13.12.27
					//pt.intensity += ptd.intensity;
					//pt.r += ptd.r;
					//pt.g += ptd.g;
					//pt.b += ptd.b;
				}
				for (p = 0;p <fineptnum;p++)
				{
					//对内存中的点进行修改，改变内存中的点坐标值，达到平滑效果
					PointXYZIPRGBA& ptd = ptCloud[DisArrID[p]];
					ptd.x = pt.x/fineptnum;
					ptd.y =  pt.y/fineptnum;
					ptd.z = pt.z/fineptnum;
					//ptd.intensity = pt.intensity/fineptnum;
					//ptd.r = pt.r/fineptnum;
					//ptd.g = pt.g/fineptnum;
					//ptd.b = pt.b/fineptnum;
				}
				//将下次要用的变量值进行再次初始化
				allptnum = 0;fineptnum =0;Dist = 0;
				memset(DisArr,-1,sizeof(double)*neighbour * neighbour);
				memset(DisArrID,-1,sizeof(int)*neighbour * neighbour);

				pt.x = pt.y = pt.z = pt.intensity = pt.r = pt.g = pt.b = 0;
				///////////////////////////////////////////////////////////////////////////////////////////
				if (processCallback && (fIndex % 10000) == 0)
				{
					processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
		}
		else   //规则点云
		{
			for ( i = 0;i < col;i++)
			{
				for ( j = 0;j < row;j++)
				{
					//当前点云的索引号
					fIndex = (i * row + j);

					//计算邻域中的点到当前的点的距离
					PointXYZIPRGBA& ptf = ptCloud[fIndex];

					if (!ptf.isValid())
					{
						continue;
					}
					if (select)
					{
						if (!ptf.isSelected())
						{
							continue;
						}
					}

					PointXYZIPRGBA pt;
					float Dist = 0.0f;int m = 0;
					int p,q;
					for (p = 0; p < neighbour;p++)
					{
						for (q = 0; q< neighbour;q++)
						{
							if (neighbour % 2 == 1)
							{
								//注意越界问题
								if (i - (neighbour - 1)/2 + p >= 0 && i - (neighbour - 1)/2 + p < col && 
									(j - (neighbour - 1)/2 + q) >= 0 && (j - (neighbour - 1)/2 + q) < row)
								{
									nIndex = (i - (neighbour - 1)/2 + p)*row + (j - (neighbour - 1)/2 + q);
								}
								else
									continue;
							}
							else
								if (i - (neighbour /*- 1*/)/2 + p >= 0 && i - (neighbour/* - 1*/)/2 + p < col && 
									(j - (neighbour/* - 1*/)/2 + q) >= 0 && (j - (neighbour/* - 1*/)/2 + q) < row)
								{
									nIndex = (i - neighbour/2 + p)*row + (j - neighbour/2 + q);
								}
								else
									continue;
							PointXYZIPRGBA& ptn = ptCloud[nIndex];
							//判定点的有效性
							if (ptn.isValid())
							{
								allptnum++;
								count++;
							}

							float dist =  std::sqrt((ptn.x -ptf.x)* (ptn.x - ptf.x)
								+ (ptn.y - ptf.y) * (ptn.y - ptf.y) 
								+ (ptn.z - ptf.z)* (ptn.z - ptf.z));

							if (dist < disThreshold)
							{
								DisArrID[fineptnum] = nIndex;
								fineptnum++;
							}
						}
					}
					//求坐标的均值
					for (p = 0;p <fineptnum;p++)
					{
						PointXYZIPRGBA& ptd = ptCloud[DisArrID[p]];
						pt.x += ptd.x;
						pt.y += ptd.y;
						pt.z += ptd.z;
						//pt.intensity += ptd.intensity;
						//pt.r += ptd.r;
						//pt.g += ptd.g;
						//pt.b += ptd.b;
					}
					for (p = 0;p <fineptnum;p++)
					{
						//对内存中的点进行修改，改变内存中的点坐标值，达到平滑效果
						PointXYZIPRGBA& ptd = ptCloud[DisArrID[p]];
						ptd.x = pt.x/fineptnum;
						ptd.y =  pt.y/fineptnum;
						ptd.z = pt.z/fineptnum;
						//ptd.intensity = pt.intensity/fineptnum;
						//ptd.r = pt.r/fineptnum;
						//ptd.g = pt.g/fineptnum;
						//ptd.b = pt.b/fineptnum;
					}
					//将下次要用的变量值进行再次初始化
					allptnum = 0;fineptnum =0;Dist = 0;
					memset(DisArr,-1,sizeof(double)*neighbour * neighbour);
					memset(DisArrID,-1,sizeof(int)*neighbour * neighbour);

					pt.x = pt.y = pt.z = pt.intensity = pt.r = pt.g = pt.b = 0;
					///////////////////////////////////////////////////////////////////////////////////////////
					if (processCallback && (fIndex % 10000) == 0)
					{
						processCallback(fIndex/ (float)pointNum,HDSCENE_IDS_FILTER_FILTERING);
					}
				}
			}
		}

		delete []DisArr;
		delete []DisArrID;
		//ptCloud.setSelectCount();
		ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}

	int CFilters::OutlierFilterStatisticIScan( const char* hlsPath,int neighbour,double Threshold,void (*processCallback)(float,const char*) )
	{
		// 加载点云
		PointCloud pcd;
		pcd.loadHlsFileHeader(hlsPath);

		// 车载点云分段
		int ptNum = (int)(pcd.m_header.number_of_point_records);

		if (ptNum == 0)
		{
			return 0;
		}
		// 每一段点数控制在100w点之内
		int segCount = 0;
		segCount = (int)((float)ptNum/(1000000.0f)) + 1;

		// 分段处理
		float startScale = 0.f;
		float endScale = 1.f;
		int count = 0;
		// 步长
		float range = endScale/(float)segCount;
		for (int i = 0; i< segCount;i++)
		{
			startScale = startScale + range*i;
			endScale = startScale + range*(i + 1);
			if (startScale >= endScale)
			{
				continue;
			}
			pcd.loadHlsByScale(startScale,endScale,processCallback);


			//加载数据后，进行处理
			count += OutlierFilterStatistic(pcd,false,neighbour,Threshold,processCallback);

			//string outputPath(hlsPath);
			//outputPath.substr(0, outputPath.find_last_of('.'));

			//char ID[10];
			//sprintf(ID, "%d", i);
			//string partName(ID);
			//outputPath += partName+ ".hls";
			////保存点云到文件
			//PointCloud2Hls(pcd,outputPath.c_str(),true,true,1,1,F32_MIN,F32_MAX,processCallback);
		}

		return count;
	}

	int CFilters::AngleFilter( PointCloud& ptCloud,bool select,string& linPath,float angleThreld,bool bDelete,void (*processCallback)(float,const char*) )
	{
		//by zhangfei
		return AngleFilter(ptCloud,linPath,angleThreld,processCallback);

		int count = 0;
		//linPath = "E:\\iScan-Pcd-1.lin";
		FILE* fp = fopen(linPath.c_str(),"r");

		if (!fp || !ptCloud.isNormalPointCloud())
		{
			return -1;
		}
		char str[1024] = {0};
		float Time = 0;
		int Year = 0,Month = 0, Day = 0,Hour = 0,Minute = 0,Second = 0,MiniSecond = 0;
		double cx = 0.0f,cy = 0.0f,cz = 0.0f;
		double yaw = 0.0f,pitch = 0.0f,roll = 0.0f;
		// 读取lin文件中每一圈pos位置
		int loopCount = 0;
		//第一圈跳过
		fgets(str,1024,fp);
		vector<HD_SCANHDIINFO> vecScanHdi;
		int bufSize = 10000;
		vecScanHdi.resize(bufSize);

		while(!feof(fp))
		{
			memset(str,0,1024);
			fgets(str,1024,fp);
			HD_SCANHDIINFO& hdiInfo = vecScanHdi[loopCount];
			if (hdiInfo.Serialize(str))
			{
				loopCount++;

				if (loopCount >= vecScanHdi.size())
				{
					vecScanHdi.resize(vecScanHdi.size() + bufSize);
				}
			}
		}

		fclose(fp);
		vecScanHdi.resize(loopCount);

		// 圈文件与点云文件不一致,不满足算法要求，返回
		if (loopCount != ptCloud.m_header.number_of_col)
		{
			return -2;
		}

		double x = 0,y = 0,z = 0;
		int i = 0;

		double M[16];
		ptCloud.m_header.computeMatrix(M);

		hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
		if (pts.hasAttr())
		{
			pts.delAttrs();
		}
		pts.addAttrs<float>();
		// 内存中点云的总圈数
		int ptloopCount = ptCloud.getLoopCount();

		for (unsigned int n = 0;n<ptloopCount;n++)
		{
			hdVector<PointXYZIPRGBA>& loopts = ptCloud.getLoop(n);
			float* featureList = ptCloud.getBlockAttr<float>(n);
			// 通过内存中的圈号获取其在文件中对应的圈号
			int m = pts.getLoopIdx(n);
			if (m < 0 || m >= loopCount)
			{
				continue;
			}
			const HD_SCANHDIINFO& scanPos = vecScanHdi[m];
			cx = scanPos.dX;// PosX[m];
			cy = scanPos.dY;// PosY[m];
			cz = scanPos.dZ;// PosZ[m];

			irr::core::vector3df vecZ(0,0,1);

			for (int i = 0;i < loopts.size();i++)
			{
				PointXYZIPRGBA& pt = *(loopts._Myfirst + i);	
				if (!pt.isValid())
				{
					continue;
				}
				x = pt.x;
				y = pt.y;
				z = pt.z;
				hdHomogeneousTransformPoint(M,x,y,z);

				// 计算每圈中的点的角度 单位为度
				irr::core::vector3df pp((float)(x - cx), (float)(y - cy), (float)(z - cz));

				float angle = pp.getAngleTo(vecZ);

				// 将计算的角度值写入点的属性中
				featureList[i] = angle;

				if (angle < angleThreld)
				{
					if (bDelete)
					{
						pt.setDeleted();
						//pt.
					}
					else
					{
						pt.setSelected();
					}

					count++;
				}
				if (processCallback && (i % 10000) == 0)
				{
					processCallback((float)i /  ptCloud.m_simpleHeader.number_of_point_records  ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
		}
		if (!bDelete)
		{
			ptCloud.setSelectCount();
		}
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return count;
	}

	int CFilters::AngleFilter( PointCloud& ptCloud,float angleThreld,bool bDelete,void (*processCallback)(float,const char*) )
	{
		int count = (int)ptCloud.count();
		u32 loopCount = ptCloud.getLoopCount();

		hdBlkArray<PointXYZIPRGBA>& mpts = ptCloud.getPoints();

		if (!mpts.hasAttr())
		{
			return -1;
		}

		for (u32 n = 0;n<loopCount;n++)
		{
			hdVector<PointXYZIPRGBA>& pts = ptCloud.getLoop(n);
			float* cs = ptCloud.getBlockAttr<float>(n);
			for (int i = 0;i < pts.size();i++)
			{
				PointXYZIPRGBA& pt = *(pts._Myfirst + i);		

				float angle = cs[i];

				if (angle < angleThreld)
				{
					if (bDelete)
					{
						pt.setDeleted();
					}
					else
						pt.setSelected();
				}
				else
				{
					if (pt.isSelected())
					{
						pt.setUnSelected();
					}
				}
				if (processCallback && (i % 10000) == 0)
				{
					processCallback((float)i / count ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
		}
		ptCloud.setSelectCount();
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return 0;
	}

	int CFilters::AngleFilter( PointCloud& ptCloud,float angleThreld,void (*processCallback)(float,const char*) )
	{
		u32 loopCount = ptCloud.getLoopCount();
		struct StatsDistAngle
		{
			StatsDistAngle()
				:dist(0.0),angle(0.0){}
			double dist;	// 相邻点距离
			double angle;	// 相邻点高度角
			double angleH;	// 天顶角
		};
		for (u32 n = 0;n<loopCount;n++)
		{
			hdVector<PointXYZIPRGBA>& pts = ptCloud.getLoop(n);
			u32 size = pts.size();
			if (size == 0)
			{
				continue;
			}
			hdVector<StatsDistAngle> aryDist;
			aryDist.resize(pts.size());

			double dx,dy,dz;
			double length;
			double height;
			u32 lowPtIndex = 0;
			f32 minDist = F32_MAX;//相邻2点距离
			PointXYZIPRGBA ptNext;
			for (u32 i = 0;i < size;i++)
			{
				PointXYZIPRGBA& pt = *(pts._Myfirst + i);		
				if (!pt.isValid())
				{
					continue;
				}

				// 查下一个有效点
				for (u32 n = i + 1;n < size;)
				{
					PointXYZIPRGBA& ptNextRef = *(pts._Myfirst + n);	
					if (ptNextRef.isValid())
					{
						ptNext = ptNextRef;
						break;
					}
					n++;
					if (n >= size)
					{
						n = 0;
					}
				}

				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);
				dx = pt.x - ptNext.x;
				dy = pt.y - ptNext.y;
				dz = pt.z - ptNext.z;
				distAngle.dist = sqrt(dx * dx + dy * dy + dz * dz);

				height = fabs(pt.z - ptNext.z);	//高度
				length = sqrt(dx * dx + dy * dy);// 平面长度
				distAngle.angle = atan(height / length);

				if (distAngle.dist < minDist)
				{
					minDist = (float)distAngle.dist;
					lowPtIndex = i;
				}

				if (length < 0.00000001)
				{
					distAngle.angle = 90.0;
				}
				else
				{
					distAngle.angle = RAD2DEG(atan(height / length));
				}
			}

			// 计算距离均差
			double stdMean = 0.0;
			u32 validCount = 0;
			for (u32 i = 1;i < pts.size() -1;i++)
			{
				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);
				if (distAngle.dist > 0.0)
				{
					stdMean+=distAngle.dist;
					validCount++;
				}
			}
			stdMean /= validCount;

			// 根据距离均差和角度因素判断噪点
			const PointXYZIPRGBA& lowPt = *(pts._Myfirst + lowPtIndex);

			for (u32 i = 0;i < size;i++)
			{
				PointXYZIPRGBA& pt = *(pts._Myfirst + i);		
				if (!pt.isValid())
				{
					continue;
				}

				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);

				dx = lowPt.x - pt.x;
				dy = lowPt.y - pt.y;
				length = sqrt(dx * dx + dy * dy);
				height = fabs(lowPt.z - pt.z);
				distAngle.angleH = RAD2DEG(atan(height / length));

				if (distAngle.dist > stdMean * 2.0 &&
					//!(distAngle.angle >= 0.0 && distAngle.angle < 30.0) &&
					//!(distAngle.angle > 75 && distAngle.angle < 105.0) &&
					distAngle.angleH > 90 - angleThreld && distAngle.angleH < 90 + angleThreld)//&& distAngle.angle < 89.0 && distAngle.angle > angleThreld
				{
					pt.setSelected();
				}
				//lowPt.setSelected();

			}

			if (processCallback && (n % 10) == 0)
			{
				processCallback((float)n / loopCount ,HDSCENE_IDS_FILTER_FILTERING);
			}
		}

		ptCloud.setSelectCount();
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return 1;
	}

	//zhangfei
	int CFilters::AngleFilter(PointCloud& ptCloud,string& linPath,float angleThreld,void (*processCallback)(float,const char*))
	{
		//定义角度统计结构体
		struct StatsDistAngle
		{
			StatsDistAngle()
				:dist(0.0),angle(0.0){}
			double dist;	// 相邻点距离
			double angle;	// 相邻点高度角
			double angleH;	// 天顶角
		};

		

		FILE* fp = fopen(linPath.c_str(),"r");
		bool bHasLin = (fp != NULL);
		vector<HD_SCANHDIINFO> vecScanHdi;
		int bufSize = 10000;
		vecScanHdi.resize(bufSize);

		int loopCount = 0;
		//有Lin文件情况下，读取Pos文件，获取Pos点的位置
		if(bHasLin)
		{
			char str[1024] = {0};
			//第一行跳过
			fgets(str,1024,fp);
			// 读取lin文件中每一圈pos位置
			while(!feof(fp))
			{
				memset(str,0,1024);
				fgets(str,1024,fp);
				HD_SCANHDIINFO& hdiInfo = vecScanHdi[loopCount];
				if (hdiInfo.Serialize(str))
				{
					loopCount++;

					if (loopCount >= vecScanHdi.size())
					{
						vecScanHdi.resize(vecScanHdi.size() + bufSize);
					}
				}
			}
			fclose(fp);
		}
		vecScanHdi.resize(loopCount);

		if (!ptCloud.isNormalPointCloud() && loopCount <=0 )
		{
			// 考虑iScan-M不是规则点云[zfei 2014/6/25]
			return -3;
		}

		double M[16];
		ptCloud.m_header.computeMatrix(M);
		hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
		int ptloopCount = ptCloud.getLoopCount();	//内存中点云的总圈数
		long nSltPntCount = 0; //test

		for (unsigned int n = 0;n < ptloopCount;n++)
		{
			// 			if (n == ptloopCount/2)
			// 			{
			// 				return -4;
			// 			}
			hdVector<PointXYZIPRGBA>& loopts = ptCloud.getLoop(n);
			hdVector<StatsDistAngle> aryDist;
			aryDist.resize(loopts.size());
			// 通过内存中的圈号获取其在文件中对应的圈号
			int m = pts.getLoopIdx(n);
			if (bHasLin)
			{
				if ( m < 0 || m >= loopCount)
				{
					continue;
				}
			}

			irr::core::vector3df vecZ(0,0,1);
			PointXYZIPRGBA ptNext;
			u32 lowPtIndex = 0;
			f32 minDist = F32_MAX;	//相邻2点距离
			for (int i = 0;i < loopts.size();i++)
			{
				PointXYZIPRGBA& pt = *(loopts._Myfirst + i);	
				if (!pt.isValid() /*|| pt.isSelected()*/)
				{
					continue;
				}

				// 查下一个有效点
				for (u32 n = i + 1;n < loopts.size();)
				{
					PointXYZIPRGBA& ptNextRef = *(loopts._Myfirst + n);	
					if (!ptNextRef.isValid()/* || ptNextRef.isSelected()*/)
					{
						n++;
						continue;
					}
					else
					{
						ptNext = ptNextRef;
						break;
					}
					n++;
					if (n >= loopts.size())
					{
						n = 0;
					}
				}

				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);
				double dx = pt.x - ptNext.x;
				double dy = pt.y - ptNext.y;
				double dz = pt.z - ptNext.z;
				distAngle.dist = sqrt(dx * dx + dy * dy + dz * dz);

				double height = fabs(pt.z - ptNext.z);	// 高度
				double length = sqrt(dx * dx + dy * dy);// 平面长度
				//distAngle.angle = atan(height / length);

				if (distAngle.dist < minDist)
				{
					minDist = (float)distAngle.dist;
					lowPtIndex = i;
				}

				if (length < 0.00000001)
				{
					distAngle.angle = 90.0;
				}
				else
				{
					distAngle.angle = RAD2DEG(atan(height / length));
				}
			}

			// 计算距离均差
			double stdMean = 0.0;
			u32 validCount = 0;
			for (u32 i = 1;i < loopts.size() -1;i++)
			{
				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);

				if (distAngle.dist > 0.0)
				{
					stdMean+=distAngle.dist;
					validCount++;
				}
			}
			if (validCount > 0)
			{
				stdMean /= validCount;
			}

			// 根据距离均差和角度因素判断噪点
			const PointXYZIPRGBA& lowPt = *(loopts._Myfirst + lowPtIndex);
			PointXYZIPRGBA angleCalPt;
			if (bHasLin)
			{
				const HD_SCANHDIINFO& hdiInfo = vecScanHdi[m];
				angleCalPt.x = (float)hdiInfo.dX;//PosX[m];
				angleCalPt.y = (float)hdiInfo.dY;//PosY[m];
				angleCalPt.z = (float)hdiInfo.dZ;//PosZ[m];
			}
			else
			{
				double x = lowPt.x;
				double y = lowPt.y;
				double z = lowPt.z;
				hdHomogeneousTransformPoint(M,x,y,z);
				angleCalPt.x = (float)x;
				angleCalPt.y = (float)y;
				angleCalPt.z = (float)z;
			}

			for (u32 i = 0;i < loopts.size();i++)
			{
				PointXYZIPRGBA& pt = *(loopts._Myfirst + i);	
				// 对于已经选中的点要去除选中 fengjing
				if (!pt.isValid() /*|| pt.isSelected()*/)
				{
					//nSltPntCount++;
					continue;
				}

				StatsDistAngle& distAngle = *(aryDist._Myfirst + i);
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;
				hdHomogeneousTransformPoint(M,x,y,z);

				double dx = angleCalPt.x - x;
				double dy = angleCalPt.y - y;
				double length = sqrt(dx * dx + dy * dy);
				double height = z - angleCalPt.z;
				//double height = bHasLin ? (z - angleCalPt.z) : fabs(z - angleCalPt.z);	//有Lin文件时，需要排除地面点
				distAngle.angleH = RAD2DEG(atan(height / length));

				if (distAngle.dist > stdMean * 2.0 &&
					distAngle.angleH > 90 - angleThreld && distAngle.angleH < 90 )
				{
					pt.setSelected();
					//nSltPntCount++;
				}
				else
				{
					// 对于不需要过滤的点，如果被选中要取消选中 fengjing
					if (pt.isSelected())
					{
						pt.setUnSelected();
					}
				}
			}

			if (processCallback && (n % 10) == 0)
			{
				processCallback((float)n / ptloopCount ,HDSCENE_IDS_FILTER_FILTERING);
			}
		}
		//nSltPntCount = nSltPntCount;
		ptCloud.setSelectCount();
		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}
		return 1;
	}

	int CFilters::FilterPtCloudByHight(
		PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
		double height,									// 高度
		bool   bHigh,									// 是否过滤高于,true过滤高于height,false过滤低于height
		void (*processCallback)(float,const char*))	// 进度回调
	{
		double M[16];
		ptCloud.m_header.computeMatrix(M);
		hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
		int blkCount = pts.blockCount();	//内存中点云的总圈数

		double x,y,z;
		for (unsigned int n = 0;n < blkCount;n++)
		{
			hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);

			for (unsigned int m = 0;m < loopts.size();m++)
			{
				PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

				if (!pt.isValid())
				{
					continue;
				}

				hdPtSelectMode smode = (hdPtSelectMode)CHdSysSetting::getSysSetting()->selectionSetting.selectMode;
				if(smode == SELECT_MIUS && !pt.isSelected())
				{
					continue;
				}
				//pt.setUnSelected();

				x = pt.x;
				y = pt.y;
				z = pt.z;
				hdHomogeneousTransformPoint(M,x,y,z);

				// 根据高度关系,判读是否选中
				bool scrInside = false;
				if ((bHigh && z >  height) ||
					(!bHigh && z < height))
				{
					scrInside = true;
				}

				if (smode == SELECT_ADD)
				{	

					if (scrInside)
					{
						pt.setSelected();
					}
					
				}					
				else if(smode == SELECT_NEW)
				{
					if (scrInside)
					{
						pt.setSelected();
					}
					else
					{
						pt.setUnSelected();
					}
				}
				else if(smode == SELECT_MIUS)
				{
					if (scrInside)
					{
						pt.setUnSelected();
					}
				}
			}

			// 报告进度
			if (processCallback && (n % 10) == 0)
			{
				processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
			}
		}

		ptCloud.setSelectCount();

		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}

		return 1;
	}

	// 介于高度进行过滤
	int CFilters::FilterPtCloudByHight(
		PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
		double minheight,								// 高度下限
		double maxheight,								// 高度上限
		void (*processCallback)(float,const char*))	// 进度回调
	{
		double M[16];
		ptCloud.m_header.computeMatrix(M);
		hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
		int blkCount = pts.blockCount();	//内存中点云的总圈数

		double x,y,z;
		for (unsigned int n = 0;n < blkCount;n++)
		{
			hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);

			for (unsigned int m = 0;m < loopts.size();m++)
			{
				PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

				if (!pt.isValid())
				{
					continue;
				}

				hdPtSelectMode smode = (hdPtSelectMode)CHdSysSetting::getSysSetting()->selectionSetting.selectMode;
				if(smode == SELECT_MIUS && !pt.isSelected())
				{
					continue;
				}
				//pt.setUnSelected();

				x = pt.x;
				y = pt.y;
				z = pt.z;
				hdHomogeneousTransformPoint(M,x,y,z);

				// 根据高度关系,判读是否选中
				bool scrInside = false;
				if ( z >=minheight && z <= maxheight)
				{
					scrInside = true;
				}

				if (smode == SELECT_ADD)
				{					
					if (scrInside)
					{
						pt.setSelected();
					}
				}					
				else if(smode == SELECT_NEW)
				{
					if (scrInside)
					{
						pt.setSelected();
					}
					else
					{
						pt.setUnSelected();
					}
				}
				else if(smode == SELECT_MIUS)
				{
					if (scrInside)
					{
						pt.setUnSelected();
					}
				}
			}

			// 报告进度
			if (processCallback && (n % 10) == 0)
			{
				processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
			}
		}

		ptCloud.setSelectCount();

		//ptCloud.SetPointCloudChanged(true);
		if (processCallback)
		{
			processCallback(0.0,HDSCENE_IDS_FINISH);
		}

		return 1;
	}

	//! 根据扫描头轨迹点高度过滤点云,过滤后选中.gsl-2013/9/14
	int CFilters::ScanPosFilter(
		PointCloud& ptCloud,			// 需要处理的点云,不能是分段加载的点云
		const string& linPath,			// 扫描头轨迹点文件
		double height,					// 离扫描头高度,正表示上方,负表示下方
		bool   bHigh,					// 是否过滤高于,true过滤高于轨迹点,false过滤低于
		void (*processCallback)(float,const char*))// 进度回调
	{
		//  内存不足进行异常捕捉 [2014.3.4] 蔡红云 
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}
			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			int blkCount = pts.blockCount();	//内存中点云的总圈数

			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);
				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;//(double)n / (double)blkCount;
				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];

				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid())
					{
						continue;
					}
					pt.setUnSelected();
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					// 根据高度关系,判读是否选中
					if ((bHigh && z > (scanPos.dZ + height)) ||
						(!bHigh && z < (scanPos.dZ + height)))
					{
						pt.setSelected();
					}
				}
				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}
			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);
			return 0;
		}
	}

	//! 根据扫描头轨迹点高度过滤点云,过滤后选中.rxs-2016/12/7
	int CFilters::ScanPosFilter(
		PointCloud& ptCloud,			// 需要处理的点云,不能是分段加载的点云
		const string& linPath,			// 扫描头轨迹点文件
		double height,					// 离扫描头高度,正表示上方,负表示下方
		bool   bHigh,					// 是否过滤高于,true过滤高于轨迹点,false过滤低于
		bool   bAddFilter,				// 是否叠加过滤
		void (*processCallback)(float,const char*))// 进度回调
	{
		//  内存不足进行异常捕捉 [2014.3.4] 蔡红云 
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}

			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			int blkCount = pts.blockCount();	//内存中点云的总圈数
			bool bSelete = ptCloud.getSelectCount();
			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);
				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;//(double)n / (double)blkCount;
				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];
				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid())
					{
						continue;
					}
					if (bSelete)
					{
						if (!bAddFilter)
						{
							pt.setUnSelected();
						}
						else if (!pt.isSelected())
						{
							continue;
						}
					}
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					// 根据高度关系,判读是否选中
					if ((bHigh && z > (scanPos.dZ + height)) ||
						(!bHigh && z < (scanPos.dZ + height)))
					{
						pt.setSelected();
					}
					else
					{
						pt.setUnSelected();
					}
				}
				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}
			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);
			return 0;
		}
	}

	//! 根据扫描头轨迹点高度过滤点云,过滤后选中.gsl-2013/9/14
	int CFilters::ScanPosFilter(
		PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
		const string& linPath,							// 扫描头轨迹点文件
		double minHeight,								// 离扫描头底下
		double maxHeight,								// 离扫描头上方
		void (*processCallback)(float,const char*))	// 进度回调
	{
		//  内存不足进行异常捕捉 [2014.3.4] 蔡红云 
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}

			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			int blkCount = pts.blockCount();	//内存中点云的总圈数
			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				//double dScale = (double)n / (double)blkCount;
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);
				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;
				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];

				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid())
					{
						continue;
					}
					pt.setUnSelected();
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					// 根据高度关系,判读是否选中
					if (z > (scanPos.dZ + minHeight) &&
						z < (scanPos.dZ + maxHeight))
					{
						pt.setSelected();
					}
				}
				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}
			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);
			return 0;
		}
	}

	//! 根据扫描头轨迹点高度过滤点云,过滤后选中.rxs-2016/12/7
	int CFilters::ScanPosFilter(
		PointCloud& ptCloud,							// 需要处理的点云,不能是分段加载的点云
		const string& linPath,							// 扫描头轨迹点文件
		double minHeight,								// 离扫描头底下
		double maxHeight,								// 离扫描头上方
		bool bAddFilter,								// 是否叠加过滤
		void (*processCallback)(float,const char*))	// 进度回调
	{
		//  内存不足进行异常捕捉 [2014.3.4] 蔡红云 
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}

			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			int blkCount = pts.blockCount();	//内存中点云的总圈数
			bool bSelete = ptCloud.getSelectCount();
			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				//double dScale = (double)n / (double)blkCount;
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);
				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;

				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];

				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid())
					{
						continue;
					}
					if (bSelete)
					{
						if (!bAddFilter)
						{
							pt.setUnSelected();
						}
						else if (!pt.isSelected())
						{
							continue;
						}
					}
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					// 根据高度关系,判读是否选中
					if (z > (scanPos.dZ + minHeight) &&
						z < (scanPos.dZ + maxHeight))
					{
						pt.setSelected();
					}
					else
					{
						pt.setUnSelected();
					}
				}
				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}
			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);
			return 0;
		}
	}

	int CFilters::ScanPosDistFilter( PointCloud& ptCloud,const string& linPath,double distance, bool bLess, void (*processCallback)(float,const char*) )
	{
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}

			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			//int nTest = ptCloud.getLoopCount();
			int blkCount = pts.blockCount();	//内存中点云的总圈数

			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);

				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;//n / blkCount;
				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];

				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid()/* || pt.isSelected()*/)
					{
						continue;
					}
					pt.setUnSelected();
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					irr::core::vector3df vecDist((float)(scanPos.dX - x), (float)(scanPos.dY - y), (float)(scanPos.dZ - z));
					double disTmp = sqrt(vecDist.X * vecDist.X + vecDist.Y * vecDist.Y + vecDist.Z * vecDist.Z);
					// 根据距离关系,判读是否选中
					if (bLess)

					{
						if (disTmp < distance)
						{
							pt.setSelected();
						}
					}
					else
					{
						if (disTmp > distance)
						{
							pt.setSelected();
						}
					}
				}

				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}

			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);

			return 0;
		}
	}
	
	//! 根据扫描头轨迹点距离过滤点云,过滤后选中.rxs-2016/12/7
	int CFilters::ScanPosDistFilter( PointCloud& ptCloud,const string& linPath,double distance, bool bLess, bool bAddFilter, void (*processCallback)(float,const char*) )
	{
		try
		{
			if (!ptCloud.isNormalPointCloud())
			{
				//return -3;
			}

			// 定义扫描轨迹内存数组
			std::vector<HD_SCANHDIINFO> vecScanHdi;
			HD_SCANHDIINFO::Serialize(linPath.c_str(),vecScanHdi);
			if (vecScanHdi.size() == 0)
			{
				return -2;
			}
			int loopCount = vecScanHdi.size();
			double M[16];
			ptCloud.m_header.computeMatrix(M);
			hdBlkArray<PointXYZIPRGBA>& pts = ptCloud.getPoints();
			//int nTest = ptCloud.getLoopCount();
			int blkCount = pts.blockCount();	//内存中点云的总圈数
			bool bSelete = ptCloud.getSelectCount();
			double x,y,z;
			for (unsigned int n = 0;n < blkCount;n++)
			{
				hdVector<PointXYZIPRGBA>& loopts = pts.getBlock(n);
				// 计算对应的轨迹点
				hd::u32 uFileLoop = ptCloud.getLoopIndex(n);
				
				double dScale = (double)uFileLoop / ptCloud.m_header.number_of_col;//n / blkCount;
				int scanPtIndex = (int)(dScale * loopCount);
				const HD_SCANHDIINFO& scanPos = vecScanHdi[scanPtIndex];

				for (unsigned int m = 0;m < loopts.size();m++)
				{
					PointXYZIPRGBA& pt = *(loopts._Myfirst + m);

					if (!pt.isValid()/* || pt.isSelected()*/)
					{
						continue;
					}

					if (bSelete)
					{
						if (!bAddFilter)
						{
							pt.setUnSelected();
						}
						else if (!pt.isSelected())
						{
							continue;
						}
					}
					x = pt.x;
					y = pt.y;
					z = pt.z;
					hdHomogeneousTransformPoint(M,x,y,z);

					irr::core::vector3df vecDist((float)(scanPos.dX - x), (float)(scanPos.dY - y), (float)(scanPos.dZ - z));
					double disTmp = sqrt(vecDist.X * vecDist.X + vecDist.Y * vecDist.Y + vecDist.Z * vecDist.Z);
					// 根据距离关系,判读是否选中
					if ((bLess && (disTmp < distance)) ||
						(!bLess && (disTmp > distance)))
					{
						pt.setSelected();
					}
					else
					{
						pt.setUnSelected();
					}
				}

				// 报告进度
				if (processCallback && (n % 10) == 0)
				{
					processCallback((float)n / blkCount ,HDSCENE_IDS_FILTER_FILTERING);
				}
			}
			ptCloud.setSelectCount();
			//ptCloud.SetPointCloudChanged(true);
			if (processCallback)
			{
				processCallback(0.0,HDSCENE_IDS_FINISH);
			}

			return 1;
		}
		catch (...)
		{
			::MessageBox(NULL, HDSCENE_IDS_MEMORY_REQUSTFAILED, HDSCENE_IDS_ERROR, MB_OK);

			return 0;
		}
	}

}