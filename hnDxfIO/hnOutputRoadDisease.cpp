#include "StdAfx.h"
#include "hnOutputRoadDisease.h"

// 解析字符
bool analysisStr(char* strSource, hnDiseaseInfoBase* roadDisease)
{
	const char *d = ",";
	char *p;
	p = strtok(strSource, d);

	vector<double> vecValue;

	int nCnt = 0;
	while (p)
	{
		if (nCnt == 0)
		{
			// 病害类型
			roadDisease->nDiseaseType = atoi(p);
		}
		else if (nCnt == 1)
		{
			// 病害程度
			roadDisease->nLevel = atoi(p);
		}
		else if (nCnt == 2)
		{
			// 病害长度
			roadDisease->dLength = atof(p);
		}
		else if (nCnt == 3)
		{
			// 病害宽度
			roadDisease->dWidth = atof(p);
		}
		else if (nCnt == 4)
		{
			// 病害面积
			roadDisease->dArea = atof(p);
		}
		else if (nCnt == 5)
		{
			// 病害深度
			roadDisease->dDepth = atof(p);
		}
		else if (nCnt == 6)
		{
			// 病害几何形态
			roadDisease->nGeometry = atoi(p);
		}
		else
		{
			// 病害顶点
			vecValue.push_back(atof(p));
		}
		
		p = strtok(NULL, d);

		++nCnt;
	}

	if (vecValue.size() <= 0)
	{
		return false;
	}

	if (vecValue.size() % 2 != 0)
	{
		return false;
	}

	int nPtCnt= vecValue.size() / 2;
	roadDisease->vecPt.resize(nPtCnt);
	
	for (int i = 0; i < nPtCnt; i++)
	{
		roadDisease->vecPt[i].dx = vecValue[i*2];
		roadDisease->vecPt[i].dy = vecValue[i*2 + 1];
	}

	return true;
}

// 排序
bool sortMileagePile(hnMileagePile begMilPile, hnMileagePile endMilPile)
{
	return begMilPile.dMileage < endMilPile.dMileage;
}

hnOutputRoadDisease::hnOutputRoadDisease(int divideMile):m_dBegMileage(0.0),m_nMileageDir(0),m_nCenterType(0)
{
	m_divideMile = divideMile;
	// 最大最小里程
	m_dMaxMile = -100000000.0;
	m_dMinMile = 100000000.0;
	m_nLineType = 0;
}


hnOutputRoadDisease::~hnOutputRoadDisease(void)
{
}

bool hnOutputRoadDisease::outDisease(const char* strDisease, const char* strCenter, 
	const char* strMileagePile, const char* outDir,void (*processCallback)(float, const char*)/* = NULL*/)
{
	// 导出dxf
	hnOutDiseaseDxf outDxf;
	outDxf.setCenterType(m_nCenterType, m_nLineType);

	// 清空数据
	clearData();

	if (processCallback)
	{
		processCallback(0.2, "正在读取数据...");
	}

	// 读取数据
	if (!readDisease(strDisease, strCenter, strMileagePile))
	{
		return false;
	}

	if (m_vecCenter.size() <= 0 || m_vecDiseaseInfo.size() <= 0)
	{
		return false;
	}

	if (processCallback)
	{
		processCallback(0.4, "计算病害的属性信息...");
	}

	// 计算病害的属性信息
	if (!calDiseaseAttr())
	{
		return false;
	}

	vector<hnGridDiseaseInfo> vecGroupDisease;

	if (processCallback)
	{
		processCallback(0.6, "划分病害...");
	}

	// 划分病害
	divideDiseaseGroup(vecGroupDisease);

	if (vecGroupDisease.size() <= 0)
	{
		return false;
	}

	string strDir = outDir;
	strDir = strDir.substr(0, strDir.find_last_of("\\"));
	string strOutPath = "";

	if (processCallback)
	{
		processCallback(0.8, "输出病害...");
	}
	int cuurenDisCount = 0; 
	
	// 输出
	for (int i = 0; i < vecGroupDisease.size(); i++)
	{
		//if (vecGroupDisease[i].dBegMileage != 2744000)
		//{
		//	continue;
		//}

		// 分割病害
		segGroupDisease(vecGroupDisease[i]);

		// 检查病害
		checkDisease(vecGroupDisease[i]);

		// 输出病害
		strOutPath = strDir + "\\" + vecGroupDisease[i].strName + ".dxf";
		int temp = 0;
		outDxf.outDiseaseDxf(strOutPath.c_str() ,vecGroupDisease[i]);
	}

	for(int i=0;i<vecGroupDisease.size();i++)
	{
		for(int j=0;j<vecGroupDisease[i].vecDiseaseInfo.size();j++)
		{
			if(vecGroupDisease[i].vecDiseaseInfo[j] != NULL)
			{
				delete vecGroupDisease[i].vecDiseaseInfo[j];
				vecGroupDisease[i].vecDiseaseInfo[i] = NULL;
			}
		}
	}
	if (processCallback)
	{
		processCallback(1.0, "输出完成...");
	}

	return true;
}

// 设置里程信息
void hnOutputRoadDisease::setMileageInfo(double dBegMil, int nMilDir, int nCenterType, int nLineType)
{
	m_dBegMileage = dBegMil;
	m_nMileageDir = nMilDir;
	m_nCenterType = nCenterType;
	m_nLineType = nLineType;
}

// 删除数据
void hnOutputRoadDisease::clearData()
{
	for(int i=0;i<m_vecDiseaseInfo.size();i++)
	{
		delete m_vecDiseaseInfo[i];
	}
	m_vecDiseaseInfo.clear();
	m_vecCenter.clear();
	m_vecMileagePile.clear();
}

// 读取病害
bool hnOutputRoadDisease::readDisease(const char* strDisease, const char* strCenter, const char* strMileagePile)
{
	char strTempPath[1000] = {0};
	FILE* pf = NULL;

	char str[30000] = {0};
	int ncnt = 0;

	/*string str1 = "I:\\3D高清路面数据\\iScan-00000000-20191128151830";
	string str2 = "I:\\3D高清路面数据\\iScan-00000000-20191128151830\\CenterLine.csv";
	string str3 = "I:\\3D高清路面数据\\iScan-00000000-20191128151830\\MilePile.csv";*/

	string str1 = strDisease;
	string str2 = strCenter;
	string str3 = strMileagePile;

	for (int i = 0; i < 10000; i++)
	{
		memset(strTempPath, 0, 1000);

		sprintf(strTempPath, "%s\\Disease_%d.csv", str1.c_str(), i);

		// 读取病害信息
		pf = fopen(strTempPath, "r");
		if (!pf)
		{
			break;
		}

		// 将文件指针放在初始位置
		fseek(pf, 0, SEEK_SET);
		hnLiqingDiseaseInfo *roadDiseaseInfo = new hnLiqingDiseaseInfo;

		int nCount = 0;

		// 读取第一行数据
		while (!feof(pf))
		{
			memset(str, 0, 30000);
			fgets(str, 30000, pf);

			// 解析字符
			if (!analysisStr(str, roadDiseaseInfo))
			{
				continue;
			}

			if (roadDiseaseInfo->vecPt.size() < 2)
			{
				continue;
			}

			m_vecDiseaseInfo.push_back(roadDiseaseInfo);
			nCount++;
		}

		fclose(pf);

		if (nCount == 0)
		{
			break;
		}
	}

	if (m_vecDiseaseInfo.size() <= 0)
	{
		return false;
	}

	double dDist = 0.0;

	hn2dPt ptBeg, ptEnd;

	// 清除重复点
	for (int i = 0; i < m_vecDiseaseInfo.size(); i++)
	{
		for (int j = 0; j < m_vecDiseaseInfo[i]->vecPt.size() - 1; j++)
		{
			ptBeg = m_vecDiseaseInfo[i]->vecPt[j];
			ptEnd = m_vecDiseaseInfo[i]->vecPt[j + 1];
			dDist = sqrt((ptBeg.dx - ptEnd.dx)*(ptBeg.dx - ptEnd.dx) +
				(ptBeg.dy - ptEnd.dy)*(ptBeg.dy - ptEnd.dy));

			if (dDist < 0.0001)
			{
				m_vecDiseaseInfo[i]->vecPt.erase(m_vecDiseaseInfo[i]->vecPt.begin() + j);
				--j;
			}
		}

		if (m_vecDiseaseInfo[i]->vecPt.size() <= 1)
		{
			delete m_vecDiseaseInfo[i];
			m_vecDiseaseInfo.erase(m_vecDiseaseInfo.begin() + i);
			--i;
		}
	}

	// 读取中心点
	pf = fopen(str2.c_str(), "r");
	const char *d = ",";
	char *p = NULL;
	int nCnt = 0;
	hn2dPt pt;
	if (pf)
	{
		fseek(pf, 0, SEEK_SET);
		while (!feof(pf))
		{
			memset(str, 0, 30000);
			fgets(str, 30000, pf);
			p = strtok(str, d);

			nCnt = 0;
			
			while (p)
			{
				if (nCnt == 0)
				{
					pt.dx = atof(p);
				}
				else
				{
					pt.dy = atof(p);
				}
				p = strtok(NULL, d);

				++nCnt;
			}

			if (nCnt == 0)
			{
				continue;
			}

			m_vecCenter.push_back(pt);
		}

		fclose(pf);
	}

	if (m_vecCenter.size() <= 0)
	{
		return false;
	}

	// 读取里程桩
	pf = fopen(str3.c_str(), "r");
	hnMileagePile mileagePile;
	if (pf)
	{
		fseek(pf, 0, SEEK_SET);
		while (!feof(pf))
		{
			memset(str, 0, 30000);
			fgets(str, 30000, pf);
			p = strtok(str, d);

			nCnt = 0;

			while (p)
			{
				if (nCnt == 0)
				{
					mileagePile.dMileage = atof(p);
				}
				else
				{
					mileagePile.dRealMileage = atof(p);
				}
				p = strtok(NULL, d);

				++nCnt;
			}

			if (nCnt == 0)
			{
				continue;
			}

			m_vecMileagePile.push_back(mileagePile);
		}

		fclose(pf);
	}

	return true;
}

// 计算病害属性
bool hnOutputRoadDisease::calDiseaseAttr()
{
	// 获取车辆行驶的里程方向
	double nDir = 1.0;

	if (m_vecMileagePile.size() > 1)
	{
		// 里程桩排序
		sort(m_vecMileagePile.begin(), m_vecMileagePile.end(),sortMileagePile);

		if (m_vecMileagePile[0].dRealMileage > m_vecMileagePile[1].dRealMileage)
		{
			nDir = -1.0;
		}
	}
	else
	{
		if (m_nMileageDir == 1) // 小里程
		{
			nDir = -1.0;
		}
	}

	if (m_vecMileagePile.size() < 2)
	{
		return false;
	}

	m_dMinMile = m_vecMileagePile[0].dRealMileage;
	m_dMaxMile = m_vecMileagePile[m_vecMileagePile.size() -1].dRealMileage;

	if (m_dMinMile > m_dMaxMile)
	{
		double dTemp = m_dMinMile;
		m_dMinMile = m_dMaxMile;
		m_dMaxMile = dTemp;
	}

	// 
	double dDist, dMileage;
	dDist = dMileage = 0.0;

	double dMaxMileage, dMinMileage;
	dMaxMileage = -10000000.0;
	dMinMileage = 10000000.0;

	for (int i = 0; i < m_vecDiseaseInfo.size(); i++)
	{
		dMaxMileage = -10000000.0;
		dMinMileage = 10000000.0;

		// 计算每个点的里程和距离
		for (int j = 0; j < m_vecDiseaseInfo[i]->vecPt.size(); j++)
		{
			// 计算里程和相对距离
			if (!getMileageAndDist(m_vecDiseaseInfo[i]->vecPt[j], dDist, dMileage))
			{
				// 删除该点
				m_vecDiseaseInfo[i]->vecPt.erase(m_vecDiseaseInfo[i]->vecPt.begin() + j);
				--j;
				continue;
			}

			dDist = nDir*dDist;

			//if (abs(dDist) > 6)
			//{
			//	int a = 0;
			//	a++;
			//}

			// 相对里程桩绝对里程
			getRealMileage(dMileage);

			m_vecDiseaseInfo[i]->vecPt[j].dx = dMileage;
			m_vecDiseaseInfo[i]->vecPt[j].dy = dDist;

			if (dMileage < dMinMileage)
			{
				dMinMileage = dMileage;
			}

			if (dMileage > dMaxMileage)
			{
				dMaxMileage = dMileage;
			}
		}

		m_vecDiseaseInfo[i]->dBegMileage = dMinMileage;
		m_vecDiseaseInfo[i]->dEndMileage = dMaxMileage;

		if (m_vecDiseaseInfo[i]->nGeometry == 1)
		{
			// 转换面得顺序
			convertPlane(m_vecDiseaseInfo[i]->vecPt);
		}

		if (m_vecDiseaseInfo[i]->vecPt.size() <= 1)
		{
			delete m_vecDiseaseInfo[i];
			m_vecDiseaseInfo.erase(m_vecDiseaseInfo.begin() + i);
			--i;
		}
	}

	return true;
}

// 变换面顶点顺序
void hnOutputRoadDisease::convertPlane(vector<hn2dPt>& vecPt)
{
	// 点位0-1表示横向  点位1-2表示高度
	vector<hn2dPt> vecTemp;

	// 先判断横向和竖向
	if (abs(vecPt[0].dx - vecPt[1].dx) < abs(vecPt[2].dx - vecPt[1].dx))
	{
		// 点0和1为竖向
		vecTemp.push_back(vecPt[1]);
		vecTemp.push_back(vecPt[2]);
		vecTemp.push_back(vecPt[3]);
		vecTemp.push_back(vecPt[0]);

		vecPt = vecTemp;
		vecTemp.clear();
	}

	// 先判断横向
	if (vecPt[0].dx > vecPt[1].dx)
	{
		vecTemp.push_back(vecPt[1]);
		vecTemp.push_back(vecPt[0]);
		vecTemp.push_back(vecPt[3]);
		vecTemp.push_back(vecPt[2]);
	}
	else
	{
		vecTemp = vecPt;
	}

	// 再判断纵向
	if (vecTemp[1].dy > vecTemp[2].dy)
	{
		vecPt.clear();
		vecPt.push_back(vecTemp[3]);
		vecPt.push_back(vecTemp[2]);
		vecPt.push_back(vecTemp[1]);
		vecPt.push_back(vecTemp[0]);
	}
	else
	{
		vecPt = vecTemp;
	}
}

// 划分病害
bool hnOutputRoadDisease::divideDiseaseGroup(vector<hnGridDiseaseInfo>& vecGrid)
{
	 int nBegMile = m_dMinMile / 1000.0;
	 double dBeg = m_dMinMile - nBegMile*1000.0;
     dBeg = nBegMile*1000.0 + (int)(dBeg / m_divideMile)*m_divideMile;

	 int nEndMile = m_dMaxMile / 1000.0;
	 double dEnd = m_dMaxMile - nEndMile*1000.0;
	 dEnd = nEndMile*1000.0 + ((int)(dEnd / m_divideMile+1))*m_divideMile;

	 char strName[1000] = {0};

	 // 划分
	 hnGridDiseaseInfo gridDisease;
	 int nGridCnt = (dEnd - dBeg) / m_divideMile;

	 for (int i = 0; i < nGridCnt; i ++)
	 {
		 gridDisease.vecDiseaseInfo.clear();
		 gridDisease.dBegMileage = dBeg + i*m_divideMile;
		 gridDisease.dEndMileage = dBeg + (i+1)*m_divideMile;

		 // 起始位置
		 memset(strName, 0, 1000);
		 nBegMile = gridDisease.dBegMileage / 1000.0;
		 sprintf(strName, "K%03d+%03d", nBegMile, (int)(gridDisease.dBegMileage - nBegMile*1000.0));
		 gridDisease.strBegMile = strName;

		 // 终止位置
		 memset(strName, 0, 1000);
		 nEndMile = gridDisease.dEndMileage / 1000.0;
		 sprintf(strName, "K%03d+%03d", nEndMile, (int)(gridDisease.dEndMileage - nEndMile*1000.0));
		 gridDisease.strEndMile = strName;

		 gridDisease.strName = gridDisease.strBegMile + "-" + gridDisease.strEndMile;

		 // 划分病害
		 for (int j = 0; j < m_vecDiseaseInfo.size(); j++)
		 {
			 if ((m_vecDiseaseInfo[j]->dBegMileage >= gridDisease.dBegMileage &&
				 m_vecDiseaseInfo[j]->dBegMileage <= gridDisease.dEndMileage) ||
				 (m_vecDiseaseInfo[j]->dEndMileage >= gridDisease.dBegMileage &&
				 m_vecDiseaseInfo[j]->dEndMileage <= gridDisease.dEndMileage) )
			 {	
				gridDisease.vecDiseaseInfo.push_back(m_vecDiseaseInfo[j]->clone());
				 continue;
			 }

			 if ((gridDisease.dBegMileage >= m_vecDiseaseInfo[j]->dBegMileage &&
				 gridDisease.dBegMileage <= m_vecDiseaseInfo[j]->dEndMileage) ||
				 (gridDisease.dEndMileage >= m_vecDiseaseInfo[j]->dBegMileage &&
				 gridDisease.dEndMileage <= m_vecDiseaseInfo[j]->dEndMileage))
			 {
				/* if(m_vecDiseaseInfo[j]->getDiseaseType() == LIQING_ROAD_TYPE)
				 {
					 hnLiqingDiseaseInfo *info = new hnLiqingDiseaseInfo;
					 memcpy(info,m_vecDiseaseInfo[j],sizeof(hnLiqingDiseaseInfo));
					 gridDisease.vecDiseaseInfo.push_back(info);
				 }
				 else if(m_vecDiseaseInfo[j]->getDiseaseType() == SHUINI_ROAD_TYPE)
				 {
					 hnShuiNiDiseaseInfo *info = new hnShuiNiDiseaseInfo;
					 memcpy(info,m_vecDiseaseInfo[j],sizeof(hnShuiNiDiseaseInfo));
					 gridDisease.vecDiseaseInfo.push_back(info);
				 }*/
				 gridDisease.vecDiseaseInfo.push_back(m_vecDiseaseInfo[j]->clone());
			 }
		 }

		 vecGrid.push_back(gridDisease);
	 }

	return true;
}

// 分割病害
bool hnOutputRoadDisease::segGroupDisease(hnGridDiseaseInfo& gridDisease)
{
	// 分割跨500m里程病害
	for (int i = 0; i < gridDisease.vecDiseaseInfo.size(); i++)
	{
		if (gridDisease.vecDiseaseInfo[i]->vecPt.size() < 2)
		{
			continue;
		}

		// 分割病害
		segDisease(gridDisease.dBegMileage, gridDisease.dEndMileage, gridDisease.vecDiseaseInfo[i]);
	}
	
	vector<hnDiseaseInfoBase*> vecAllDisease;

	double dBegMile, dEndMile;

	hnDiseaseInfoBase *roadTempInfo;
	double roadLength = m_divideMile / 5.0;
	for (int i = 0; i < 5; i++)
	{
		dBegMile = gridDisease.dBegMileage + i*roadLength;
		dEndMile = gridDisease.dBegMileage + (i+1)*roadLength;

		for (int j = 0; j < gridDisease.vecDiseaseInfo.size(); j++)
		{
			if ((gridDisease.vecDiseaseInfo[j]->dBegMileage > dBegMile &&
				gridDisease.vecDiseaseInfo[j]->dBegMileage < dEndMile) ||
				(gridDisease.vecDiseaseInfo[j]->dEndMileage > dBegMile &&
				gridDisease.vecDiseaseInfo[j]->dEndMileage < dEndMile) )
			{
				roadTempInfo = gridDisease.vecDiseaseInfo[j]->clone();
			
				// 分割病害
				if (!segDisease(dBegMile, dEndMile, roadTempInfo))
				{
					continue;
				}

				vecAllDisease.push_back(roadTempInfo);
				continue;
			}

			if ((dBegMile > gridDisease.vecDiseaseInfo[j]->dBegMileage &&
				dBegMile < gridDisease.vecDiseaseInfo[j]->dEndMileage) ||
				(dEndMile > gridDisease.vecDiseaseInfo[j]->dBegMileage &&
				dEndMile < gridDisease.vecDiseaseInfo[j]->dEndMileage))
			{
				roadTempInfo = gridDisease.vecDiseaseInfo[j]->clone();

				// 分割病害
				if (!segDisease(dBegMile, dEndMile, roadTempInfo))
				{
					continue;
				}

				vecAllDisease.push_back(roadTempInfo);
			}
		}
	}

	for(int i=0;i<gridDisease.vecDiseaseInfo.size();i++)
	{
		delete gridDisease.vecDiseaseInfo[i];
		gridDisease.vecDiseaseInfo[i] = NULL;
	}
	gridDisease.vecDiseaseInfo = vecAllDisease;

	return true;
}

// 检查病害
bool hnOutputRoadDisease::checkDisease(hnGridDiseaseInfo& gridDisease)
{
	for (int i = 0; i < gridDisease.vecDiseaseInfo.size(); i++)
	{
		gridDisease.vecDiseaseInfo[i]->dBegMileage = gridDisease.vecDiseaseInfo[i]->dBegMileage -
			gridDisease.dBegMileage;
		gridDisease.vecDiseaseInfo[i]->dEndMileage = gridDisease.vecDiseaseInfo[i]->dEndMileage -
			gridDisease.dBegMileage;

		for (int j = 0; j < gridDisease.vecDiseaseInfo[i]->vecPt.size(); j++)
		{
			gridDisease.vecDiseaseInfo[i]->vecPt[j].dx = gridDisease.vecDiseaseInfo[i]->vecPt[j].dx -
                 gridDisease.dBegMileage;
		}

	}
	return true;

}

// 计算某点到中心线的投影距离以及相对里程
bool hnOutputRoadDisease::getMileageAndDist(hn2dPt pt, double& dDist, double& dMileage)
{
	dDist = 0.0;
	dMileage = 0.0;

	if (m_vecCenter.size() <= 0)
	{
		return false;
	}

	hn2dPt pBegPoint, pEndPoint;

	double dA = 0.0;
	double dB = 0.0;

	vector<int> vecIndex;

	// 遍历线段的所有点
	for (int j = 0; j < m_vecCenter.size() - 1; j++)
	{
		pBegPoint = m_vecCenter[j];
		pEndPoint = m_vecCenter[j + 1];

		if ((pBegPoint.dx == pEndPoint.dx) && (pBegPoint.dy == pEndPoint.dy))
		{
			continue;
		}

		dA = (pEndPoint.dx - pBegPoint.dx) * (pt.dx - pBegPoint.dx) +
			(pEndPoint.dy - pBegPoint.dy) * (pt.dy - pBegPoint.dy);

		dB = (pBegPoint.dx - pEndPoint.dx) * (pt.dx - pEndPoint.dx) +
			(pBegPoint.dy - pEndPoint.dy) * (pt.dy - pEndPoint.dy);


		if (dA * dB < 0)
		{
			continue;
		}

		vecIndex.push_back(j);
	}

	// 计算方向
	double dC;
	dA = dB = dC = 0.0;

	// 方向 0 - 左边 1-右边
	int nDir = 0;

	double dMin = 100000;
	double dTempMileage = 0.0;
	int nIndex = -1;


	if (vecIndex.size() <= 0)
	{
		double dMinDist = 10000000;

		int nMinIndex = -1;

		// 求最小点
		for (int j = 0; j < m_vecCenter.size() - 1; j++)
		{
			pBegPoint = m_vecCenter[j];

			dDist = sqrt((pt.dx - pBegPoint.dx)*(pt.dx - pBegPoint.dx) +
				(pt.dy - pBegPoint.dy)*(pt.dy - pBegPoint.dy));

			if (dDist < dMinDist)
			{
				nMinIndex = j;
				dMinDist = dDist;
			}
		}

		if (nMinIndex == -1)
		{
			return false;
		}

		dDist = dMinDist;

		// 计算里程
		for (int i = 0; i < nMinIndex; i++)
		{
			pBegPoint = m_vecCenter[i];
			pEndPoint = m_vecCenter[i + 1];

			// 两线的距离
			dA += sqrt((pEndPoint.dx - pBegPoint.dx) * (pEndPoint.dx - pBegPoint.dx) +
				(pEndPoint.dy - pBegPoint.dy) * (pEndPoint.dy - pBegPoint.dy));
		}

		dMileage = dA;

		// 计算方向
		dA = dB = dC = 0.0;

		pBegPoint = m_vecCenter[nMinIndex - 1];
		pEndPoint = m_vecCenter[nMinIndex];

		// 求直线的参数
		if (pBegPoint.dx == pEndPoint.dx)
		{
			dA = 1;
			dB = 0;
			dC = -pBegPoint.dx;
		}
		else
		{
			dA = (pEndPoint.dy - pBegPoint.dy) / (pEndPoint.dx - pBegPoint.dx);
			dB = 1;
			//dC = (pLineBegPt.X * pLineEndPt.Y - pLineEndPt.X * pLineBegPt.Y) / (pLineEndPt.X - pLineBegPt.X);
			dC = pBegPoint.dy - dA * pBegPoint.dx;
		}

		dTempMileage = pt.dy - pt.dx * dA - dC;

		if (dA >= 0)
		{
			if (dTempMileage < 0)
			{
				nDir = 1;
			}
			else
			{
				nDir = 0;
			}
		}
		else
		{
			if (dTempMileage >= 0)
			{
				nDir = 1;
			}
			else
			{
				nDir = 0;
			}
		}

		if (nDir == 0)
		{
			dDist = - dDist;
		}

		return true;
	}

	for (int i = 0; i < vecIndex.size(); i++ )
	{
		// 获取垂足以及方向向量
		pBegPoint = m_vecCenter[vecIndex[i]];
		pEndPoint = m_vecCenter[vecIndex[i] + 1];

		// 两线的距离
		dDist = sqrt((pEndPoint.dx - pBegPoint.dx) * (pEndPoint.dx - pBegPoint.dx) +
			(pEndPoint.dy - pBegPoint.dy) * (pEndPoint.dy - pBegPoint.dy));

		dA = (pEndPoint.dx - pBegPoint.dx) * (pt.dx - pBegPoint.dx) +
			(pEndPoint.dy - pBegPoint.dy) * (pt.dy - pBegPoint.dy);

		dB = sqrt((pt.dx - pBegPoint.dx) * (pt.dx - pBegPoint.dx) +
			(pt.dy - pBegPoint.dy) * (pt.dy - pBegPoint.dy));

		dA = dA / dB / dDist;

		dTempMileage = dA * dB;

		dA = sqrt(1 - dA * dA) * dB;

		if (dA > 200)
		{
			continue;
		}

		if (dA < dMin)
		{
			dMin = dA;
			nIndex = vecIndex[i];
		}
	}

	if (nIndex == -1)
	{
		dDist = 0.0;
		dMileage = 0.0;
		return false;
	}

	// 距离
	dDist = dMin;
	dA = 0.0;

	// 计算里程
	for (int i = 0; i < nIndex; i++)
	{
		pBegPoint = m_vecCenter[i];
		pEndPoint = m_vecCenter[i + 1];

		// 两线的距离
		dA += sqrt((pEndPoint.dx - pBegPoint.dx) * (pEndPoint.dx - pBegPoint.dx) +
			(pEndPoint.dy - pBegPoint.dy) * (pEndPoint.dy - pBegPoint.dy));
	}

	dMileage = dA + dTempMileage;

	pBegPoint = m_vecCenter[nIndex];
	pEndPoint = m_vecCenter[nIndex + 1];

	// 求直线的参数
	if (pBegPoint.dx == pEndPoint.dx)
	{
		dA = 1;
		dB = 0;
		dC = -pBegPoint.dx;
	}
	else
	{
		dA = (pEndPoint.dy - pBegPoint.dy) / (pEndPoint.dx - pBegPoint.dx);
		dB = 1;
		//dC = (pLineBegPt.X * pLineEndPt.Y - pLineEndPt.X * pLineBegPt.Y) / (pLineEndPt.X - pLineBegPt.X);
		dC = pBegPoint.dy - dA * pBegPoint.dx;
	}

	dTempMileage = pt.dy - pt.dx * dA - dC;

	if (dA >= 0)
	{
		if (dTempMileage < 0)
		{
			nDir = 1;
		}
		else
		{
			nDir = 0;
		}
	}
	else
	{
		if (dTempMileage >= 0)
		{
			nDir = 1;
		}
		else
		{
			nDir = 0;
		}
	}

	if (nDir == 0)
	{
		dDist = - dDist;
	}

	return true;
}

// 里程
bool hnOutputRoadDisease::getRealMileage(double& dMileage)
{
	if (m_vecMileagePile.size() >=2)
	{
		// 判断里程桩真实里程方向
		bool bBigMileage = true;
		if (m_vecMileagePile[0].dRealMileage > m_vecMileagePile[1].dRealMileage)
		{
			bBigMileage = false;
		}

		double dRealMileage = 0.0;

		// 选择的里程桩索引
		int nPileIndex = 0;

		if (dMileage < m_vecMileagePile[0].dMileage)
		{
			if (bBigMileage)
			{
				dRealMileage = m_vecMileagePile[0].dRealMileage - (m_vecMileagePile[0].dMileage - dMileage);
			}
			else
			{
				dRealMileage = m_vecMileagePile[0].dRealMileage + (m_vecMileagePile[0].dMileage - dMileage);
			}
		}
		else if (dMileage > m_vecMileagePile[m_vecMileagePile.size() - 1].dMileage)
		{
			if (bBigMileage)
			{
				dRealMileage = m_vecMileagePile[m_vecMileagePile.size() - 1].dRealMileage + (dMileage - m_vecMileagePile[m_vecMileagePile.size() - 1].dMileage);
			}
			else
			{
				dRealMileage = m_vecMileagePile[m_vecMileagePile.size() - 1].dRealMileage - (dMileage - m_vecMileagePile[m_vecMileagePile.size() - 1].dMileage);
			}

		}
		else
		{
			for (int i = 0; i < m_vecMileagePile.size() - 1; i++)
			{
				if (dMileage > m_vecMileagePile[i].dMileage &&
					dMileage < m_vecMileagePile[i + 1].dMileage)
				{
					nPileIndex = i;
					break;
				}
			}

			dRealMileage = m_vecMileagePile[nPileIndex].dRealMileage + (m_vecMileagePile[nPileIndex + 1].dRealMileage -
				m_vecMileagePile[nPileIndex].dRealMileage) / (m_vecMileagePile[nPileIndex + 1].dMileage -
				m_vecMileagePile[nPileIndex].dMileage) * (dMileage - m_vecMileagePile[nPileIndex].dMileage);

		}

		dMileage = dRealMileage;
	}
	else if (m_vecMileagePile.size() == 1)
	{
		dMileage = m_vecMileagePile[0].dRealMileage + (dMileage - m_vecMileagePile[0].dMileage);
	}
	else
	{
		if (m_nMileageDir == 0)
		{
			dMileage = dMileage + m_dBegMileage;
		}
		else
		{
			dMileage = m_dBegMileage - dMileage;
		}
	}

	return true;
}

// 点坐标
void hnOutputRoadDisease::getPoint(hn2dPt ptBeg, hn2dPt ptEnd, hn2dPt& oriPt)
{
	double dA,dB,dC;
	dA = dB = dC = 0.0;

	// 求直线的参数
	if (ptBeg.dx == ptEnd.dx)
	{
		dA = 1;
		dB = 0;
		dC = -ptBeg.dx;
	}
	else
	{
		dA = (ptEnd.dy - ptBeg.dy) / (ptEnd.dx - ptBeg.dx);
		dB = -1;
		dC = ptBeg.dy - dA * ptBeg.dx;
	}

	if (dB == 0.0)
	{
		oriPt.dy = (ptBeg.dy + ptEnd.dy) / 2.0;
	}
	else
	{
		oriPt.dy = (-1.0*dA*oriPt.dx - dC) / dB;
	}

}

// 分割病害
bool hnOutputRoadDisease::segDisease(double dBegMile, double dEndMile, 
	hnDiseaseInfoBase* roadDisease)
{
	vector<hn2dPt> vecPt;
	hn2dPt ptOri;

	// 起始点过界 
	if (roadDisease->dBegMileage - dBegMile < -0.000001)
	{
		// 线状病害
		if (roadDisease->nGeometry == 0)
		{
			int nIndex = -1;
			vecPt = roadDisease->vecPt;

			for (int j = 0; j < vecPt.size() - 1; j++)
			{
				if (dBegMile >= vecPt[j].dx &&
					dBegMile <= vecPt[j+1].dx)
				{
					nIndex = j;
					break;
				}
			}

			if (nIndex == -1)
			{
				return false;
			}

			if (nIndex == 0)
			{
				ptOri.dx = dBegMile;
				getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

				ptOri.dx = ptOri.dx + 0.0001;
				vecPt[nIndex] = ptOri;
				roadDisease->vecPt = vecPt;
			}
			else
			{
				ptOri.dx = dBegMile;
				getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

				ptOri.dx = ptOri.dx + 0.0001;
				vecPt[nIndex] = ptOri;
				vecPt.erase(vecPt.begin(), vecPt.begin() + nIndex);
				roadDisease->vecPt = vecPt;
			}

			//// 起始点过界
			//if (nIndex == 0)
			//{
			//	ptOri.dx = dBegMile;
			//	getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

			//	ptOri.dx = ptOri.dx + 0.0001;
			//	vecPt[nIndex + 1] = ptOri;
			//	vecPt.erase(vecPt.begin() + nIndex + 2, vecPt.end());
			//	roadDisease->vecPt = vecPt;
			//}
			//else if (nIndex == vecPt.size() - 1)
			//{
			//	ptOri.dx = dBegMile;
			//	getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

			//	ptOri.dx = ptOri.dx + 0.0001;
			//	vecPt[nIndex + 1] = ptOri;
			//	vecPt.erase(vecPt.begin(), vecPt.begin() + nIndex + 2);
			//	roadDisease->vecPt = vecPt;
			//}

		}
		else // 面状病害
		{
			if (roadDisease->vecPt.size() < 4)
			{
				return false;
			}

			if (roadDisease->vecPt[0].dx < dBegMile)
			{
				ptOri.dx = dBegMile;
				getPoint(roadDisease->vecPt[0], roadDisease->vecPt[1], ptOri);

				roadDisease->vecPt[0].dx = dBegMile + 0.0001;
				roadDisease->vecPt[0].dy = ptOri.dy;
			}

			if (roadDisease->vecPt[3].dx < dBegMile)
			{
				ptOri.dx = dBegMile;
				getPoint(roadDisease->vecPt[2], roadDisease->vecPt[3], ptOri);

				roadDisease->vecPt[3].dx = dBegMile + 0.0001;
				roadDisease->vecPt[3].dy = ptOri.dy;
			}

			double dDist1 = sqrt((roadDisease->vecPt[3].dx - roadDisease->vecPt[2].dx)*
				(roadDisease->vecPt[3].dx - roadDisease->vecPt[2].dx) +(roadDisease->vecPt[3].dy - roadDisease->vecPt[2].dy)*
				(roadDisease->vecPt[3].dy - roadDisease->vecPt[2].dy));

			double dDist2 = sqrt((roadDisease->vecPt[0].dx - roadDisease->vecPt[1].dx)*
				(roadDisease->vecPt[0].dx - roadDisease->vecPt[1].dx) +(roadDisease->vecPt[0].dy - roadDisease->vecPt[1].dy)*
				(roadDisease->vecPt[0].dy - roadDisease->vecPt[1].dy));

			if (abs(dDist1 - dDist2) > 0.000001)
			{
				if (dDist1 > dDist2)
				{
					ptOri.dx = roadDisease->vecPt[2].dx + (roadDisease->vecPt[3].dx - roadDisease->vecPt[2].dx)/dDist1 * dDist2;
					ptOri.dy = roadDisease->vecPt[2].dy + (roadDisease->vecPt[3].dy - roadDisease->vecPt[2].dy)/dDist1 * dDist2;

					roadDisease->vecPt[3] = ptOri;
				}
				else
				{
					ptOri.dx = roadDisease->vecPt[1].dx + (roadDisease->vecPt[0].dx - roadDisease->vecPt[1].dx)/dDist2 * dDist1;
					ptOri.dy = roadDisease->vecPt[1].dy + (roadDisease->vecPt[0].dy - roadDisease->vecPt[1].dy)/dDist2 * dDist1;

					roadDisease->vecPt[0] = ptOri;
				}
			}
		}
	}

	// 终止点过界
	if (roadDisease->dEndMileage - dEndMile > 0.000001)
	{
		// 线状病害
		if (roadDisease->nGeometry == 0)
		{
			int nIndex = -1;
			hn2dPt ptOri;

			vecPt = roadDisease->vecPt;

			for (int j = 0; j < vecPt.size() - 1; j++)
			{
				if (dEndMile >= vecPt[j].dx &&
					dEndMile <= vecPt[j+1].dx)
				{
					nIndex = j;
					break;
				}
			}

			if (nIndex == -1)
			{
				return false;
			}

			//// 起始点过界
			//if (vecPt[0].dx > dEndMile)
			//{
			//	ptOri.dx = dEndMile;
			//	getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

			//	ptOri.dx = ptOri.dx - 0.0001;
			//	vecPt[nIndex + 1] = ptOri;
			//	vecPt.erase(vecPt.begin(), vecPt.begin() + nIndex + 2);
			//	roadDisease->vecPt = vecPt;
			//}

			//// 终止点过界
			//if (vecPt[vecPt.size() - 1].dx > dEndMile)
			//{
			//	ptOri.dx = dEndMile;
			//	getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

			//	ptOri.dx = ptOri.dx - 0.0001;
			//	vecPt[nIndex + 1] = ptOri;
			//	vecPt.erase(vecPt.begin() + nIndex + 2, vecPt.end());
			//	roadDisease->vecPt = vecPt;
			//}

			if (nIndex == vecPt.size() - 2)
			{
				// 终止点过界
				ptOri.dx = dEndMile;
				getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

				ptOri.dx = ptOri.dx - 0.0001;
				vecPt[nIndex + 1] = ptOri;
				roadDisease->vecPt = vecPt;
			}
			else
			{
				// 终止点过界
				ptOri.dx = dEndMile;
				getPoint(vecPt[nIndex], vecPt[nIndex+1], ptOri);

				ptOri.dx = ptOri.dx - 0.0001;
				vecPt[nIndex + 1] = ptOri;
				vecPt.erase(vecPt.begin() + nIndex + 2, vecPt.end());
				roadDisease->vecPt = vecPt;
			}

		}
		else // 面状病害
		{
			if (roadDisease->vecPt.size() < 4)
			{
				return false;
			}

			if (roadDisease->vecPt[1].dx > dEndMile)
			{
			
				ptOri.dx = dEndMile;
				getPoint(roadDisease->vecPt[0], roadDisease->vecPt[1], ptOri);

				roadDisease->vecPt[1].dx = dEndMile - 0.0001;
				roadDisease->vecPt[1].dy = ptOri.dy;

			}

			if (roadDisease->vecPt[2].dx > dEndMile)
			{
				ptOri.dx = dEndMile;
				getPoint(roadDisease->vecPt[2], roadDisease->vecPt[3], ptOri);

				roadDisease->vecPt[2].dx = dEndMile - 0.0001;
				roadDisease->vecPt[2].dy = ptOri.dy;
			}

			double dDist1 = sqrt((roadDisease->vecPt[3].dx - roadDisease->vecPt[2].dx)*
				(roadDisease->vecPt[3].dx - roadDisease->vecPt[2].dx) +(roadDisease->vecPt[3].dy - roadDisease->vecPt[2].dy)*
				(roadDisease->vecPt[3].dy - roadDisease->vecPt[2].dy));

			double dDist2 = sqrt((roadDisease->vecPt[0].dx - roadDisease->vecPt[1].dx)*
				(roadDisease->vecPt[0].dx - roadDisease->vecPt[1].dx) +(roadDisease->vecPt[0].dy - roadDisease->vecPt[1].dy)*
				(roadDisease->vecPt[0].dy - roadDisease->vecPt[1].dy));

			if (abs(dDist1 - dDist2) > 0.000001)
			{
				if (dDist1 > dDist2)
				{
					ptOri.dx = roadDisease->vecPt[3].dx + (roadDisease->vecPt[2].dx - roadDisease->vecPt[3].dx)/dDist1 * dDist2;
					ptOri.dy = roadDisease->vecPt[3].dy + (roadDisease->vecPt[2].dy - roadDisease->vecPt[3].dy)/dDist1 * dDist2;

					roadDisease->vecPt[2] = ptOri;
				}
				else
				{
					ptOri.dx = roadDisease->vecPt[0].dx + (roadDisease->vecPt[1].dx - roadDisease->vecPt[0].dx)/dDist2 * dDist1;
					ptOri.dy = roadDisease->vecPt[0].dy + (roadDisease->vecPt[1].dy - roadDisease->vecPt[0].dy)/dDist2 * dDist1;

					roadDisease->vecPt[1] = ptOri;
				}
			}
		}
	}

	return true;
}