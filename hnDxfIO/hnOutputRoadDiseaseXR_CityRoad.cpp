#include "hnOutputRoadDiseaseXR_CityRoad.h"
#include "StdAfx.h"
#include "hnOutDiseaseDxf_CityRoad.h"
hnOutputRoadDiseaseXR_CityRoad::hnOutputRoadDiseaseXR_CityRoad(int divideMile) :m_roadWidthXR(3.75), m_roadDirectionXR(1), m_roadTotal(0)
{
	m_divideMile = divideMile;
}


hnOutputRoadDiseaseXR_CityRoad::~hnOutputRoadDiseaseXR_CityRoad(void)
{
	clearVec();
}
//调试用 输出日志
static void LogTest(vector<hnGridDiseaseInfo> vecGroupDisease)
{
	ofstream f("D://data00.txt");
	for (int i = 0; i < vecGroupDisease.size(); i++)
	{
		f << "vecGroupDisease[i] strName;" << vecGroupDisease[i].strName << endl;
		f << "vecGroupDisease[i] strBegMile;" << vecGroupDisease[i].strBegMile << endl;
		f << "vecGroupDisease[i] strEndMile;" << vecGroupDisease[i].strEndMile << endl;
		f << "vecGroupDisease[i] dBegMileage;" << vecGroupDisease[i].dBegMileage << endl;
		f << "vecGroupDisease[i] dEndMileage;" << vecGroupDisease[i].dEndMileage << endl;
		for (int j = 0; j < vecGroupDisease[i].vecDiseaseInfo.size(); j++)
		{
			f << "vecDiseaseInfo.nDiseaseType;" << vecGroupDisease[i].vecDiseaseInfo[j]->nDiseaseType << endl;
			f << "vecDiseaseInfo.dLength;" << vecGroupDisease[i].vecDiseaseInfo[j]->dLength << endl;
			f << "vecDiseaseInfo.dWidth;" << vecGroupDisease[i].vecDiseaseInfo[j]->dWidth << endl;
			f << "vecDiseaseInfo.dArea;" << vecGroupDisease[i].vecDiseaseInfo[j]->dArea << endl;
			f << "vecDiseaseInfo.dDepth;" << vecGroupDisease[i].vecDiseaseInfo[j]->dDepth << endl;
			f << "vecDiseaseInfo.nLevel;" << vecGroupDisease[i].vecDiseaseInfo[j]->nLevel << endl;
			f << "vecDiseaseInfo.nGeometry;" << vecGroupDisease[i].vecDiseaseInfo[j]->nGeometry << endl;
			f << "vecDiseaseInfo.dBegMileage;" << vecGroupDisease[i].vecDiseaseInfo[j]->dBegMileage << endl;
			f << "vecDiseaseInfo.dEndMileage;" << vecGroupDisease[i].vecDiseaseInfo[j]->dEndMileage << endl;
			for (int k = 0; k < vecGroupDisease[i].vecDiseaseInfo[j]->vecPt.size(); k++)
			{
				f << "vecDiseaseInfo.vecPt;" << vecGroupDisease[i].vecDiseaseInfo[j]->vecPt[k].dx << "," << vecGroupDisease[i].vecDiseaseInfo[j]->vecPt[k].dy << endl;
			}
		}
		f << endl << endl;
	}
	f << endl << endl << endl << endl;

}
bool hnOutputRoadDiseaseXR_CityRoad::outDisease(const char*outDir, vector<Disease_C>& vecDisease, hnGridDiseaseInfo grid, int direction,  void(*processCallback)(float, const char*))
{
	// 导出dxf
	hnOutDiseaseDxf_CityRoad outDxf;
	m_nCenterType = 2; //CenterType: 计算Y坐标--第一车道左侧
	m_nLineType = direction > 0 ? 0 : 1; //LineType：大于0从小到大，小于0从大到小
	outDxf.setRoadWidth(grid.dRoadWidth);
	outDxf.setCenterType(m_nCenterType, m_nLineType);
	outDxf.setRoadTotal(grid.nRoadTotalNum);
	outDxf.setDivideMile(m_divideMile);

	m_roadDirectionXR = direction;
	m_roadWidthXR = grid.dRoadWidth;
	m_dMaxMile = grid.dEndMileage;
	m_dMinMile = grid.dBegMileage;
	m_roadTotal = grid.nRoadTotalNum;
	// 清空数据
	clearData();

	if (processCallback)
	{
		processCallback(0.2, "正在读取数据...");
	}

	// 计算病害的属性信息
	if (!calDiseaseAttr(vecDisease))
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
	LogTest(vecGroupDisease);
	string strDir = outDir;
	//	strDir = strDir.substr(0, strDir.find_last_of("\\"));
	string strOutPath = "";

	if (processCallback)
	{
		processCallback(0.8, "输出病害...");
	}

	// 输出
	
	for (int i = 0; i < vecGroupDisease.size(); i++)
	{
		// 分割病害
		segGroupDisease(vecGroupDisease[i]);
		// 检查病害
		checkDisease(vecGroupDisease[i]);
		//过滤重叠病害
		filterDisease(vecGroupDisease[i]);
		LogTest(vecGroupDisease);
		// 输出病害
		strOutPath = strDir + "\\" + vecGroupDisease[i].strName + ".dxf";
		outDxf.outDiseaseDxf(strOutPath.c_str(), vecGroupDisease[i]);
	}

	if (processCallback)
	{
		processCallback(1.0, "输出完成...");
	}

	return true;
}


// 计算病害属性
bool hnOutputRoadDiseaseXR_CityRoad::calDiseaseAttr(vector<Disease_C>& vecDisease)
{
	int diseaseNum = vecDisease.size();
	//Excel数据Disease_C转换dxf数据hnRoadDiseaseInfo
	clearVec();

	int disType = getDiseaseType(vecDisease);
	if (disType == -1)
	{
		//MessageBox(NULL,L"道路类型无法分类",NULL,MB_OK);
		int  userUesult = MessageBox(NULL, L"无法仅通过病害区分道路类型，请选择是否为沥青路面?", L"道路类型选择", MB_YESNO | MB_TOPMOST);
		if (userUesult == 6)
		{
			disType = LIQING_ROAD_TYPE;
		}
		else if (userUesult == 7)
		{
			disType = SHUINI_ROAD_TYPE;
		}

		//	return false;
	}
	for (int i = 0; i < diseaseNum; i++)
	{
		double dist2Edge = vecDisease[i].distToCenter - vecDisease[i].rectWidth / 2; //距离边缘算法

		int roadNum;
		if (m_roadDirectionXR > 0)
		{
			try
			{
				//roadNum = string2Num(vecDisease[i].roadNum) - 1;
				roadNum = vecDisease[i].roadNum - 1;
			}
			catch (...)
			{
				roadNum = 0;
			}
			dist2Edge += roadNum*m_roadWidthXR;
		}
		else if (m_roadDirectionXR < 0)//大里程到小里程，则到车道边的距离反向
		{
			try
			{
				//roadNum = m_roadTotal - string2Num(vecDisease[i].roadNum);
				roadNum = m_roadTotal - vecDisease[i].roadNum;
			}
			catch (...)
			{
				roadNum = 0;
			}
			dist2Edge = m_roadWidthXR - dist2Edge - vecDisease[i].rectWidth;
			dist2Edge += roadNum*m_roadWidthXR;
		}

		int nDiseaseType = getDiseaseIndex(vecDisease[i].diseaseType, disType, getDegree(vecDisease[i].diseaseDegree));
		if (nDiseaseType == -1)
		{
			MessageBox(NULL, L"病害类型无法分类,请保证选取范围内路面类型一致！", NULL, MB_OK);
			return false;
		}
		if (disType == LIQING_ROAD_TYPE)//沥青病害
		{
			hnLiqingDiseaseInfo_CityRoad *info = new hnLiqingDiseaseInfo_CityRoad;
			info->nDiseaseType = nDiseaseType;
			info->dLength = vecDisease[i].rectHeight;
			info->dWidth = vecDisease[i].rectWidth;
			info->dArea = vecDisease[i].diseaseArea;
			info->nLevel = getDegree(vecDisease[i].diseaseDegree);
			info->nGeometry = 1;//此处需要确认需求
			info->dBegMileage = vecDisease[i].beginTrueMile;
			info->dEndMileage = vecDisease[i].beginTrueMile + vecDisease[i].rectHeight;
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile, dist2Edge));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile + vecDisease[i].rectHeight, dist2Edge));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile + vecDisease[i].rectHeight, dist2Edge + vecDisease[i].rectWidth));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile, dist2Edge + vecDisease[i].rectWidth));
			info->nType = vecDisease[i].beginTrueMile / 20 % 5;
			m_vecDiseaseInfo.push_back(info);
		}
		else if (disType == SHUINI_ROAD_TYPE)//水泥病害
		{
			hnShuiNiDiseaseInfo_CityRoad *info = new hnShuiNiDiseaseInfo_CityRoad;
			info->nDiseaseType = nDiseaseType;
			info->dLength = vecDisease[i].rectHeight;
			info->dWidth = vecDisease[i].rectWidth;
			info->dArea = vecDisease[i].diseaseArea;
			info->nLevel = getDegree(vecDisease[i].diseaseDegree);
			info->nGeometry = 1;//此处需要确认需求
			info->dBegMileage = vecDisease[i].beginTrueMile;
			info->dEndMileage = vecDisease[i].beginTrueMile + vecDisease[i].rectHeight;
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile, dist2Edge));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile + vecDisease[i].rectHeight, dist2Edge));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile + vecDisease[i].rectHeight, dist2Edge + vecDisease[i].rectWidth));
			info->vecPt.push_back(hn2dPt((double)vecDisease[i].beginTrueMile, dist2Edge + vecDisease[i].rectWidth));
			info->nType = vecDisease[i].beginTrueMile / 20 % 5;
			m_vecDiseaseInfo.push_back(info);
		}
	}
	return true;
}
int hnOutputRoadDiseaseXR_CityRoad::getDiseaseIndex(string strDiseaseName, int disType, int degree)
{
	if (disType == LIQING_ROAD_TYPE)
	{
		if (strDiseaseName == "线裂") return		hnLiqingDiseaseInfo_CityRoad::XL_DISEASE_TYPE;
		if (strDiseaseName == "网裂") return		hnLiqingDiseaseInfo_CityRoad::WL_DISEASE_TYPE;
		if (strDiseaseName == "龟裂") return    hnLiqingDiseaseInfo_CityRoad::JL_DISEASE_TYPE;
		if (strDiseaseName == "拥包") return		hnLiqingDiseaseInfo_CityRoad::YB_DISEASE_TYPE;
		if (strDiseaseName == "车辙") return		hnLiqingDiseaseInfo_CityRoad::CZ_DISEASE_TYPE;
		if (strDiseaseName == "翻浆") return		hnLiqingDiseaseInfo_CityRoad::FJ_DISEASE_TYPE;
		if (strDiseaseName == "剥落") return		hnLiqingDiseaseInfo_CityRoad::BL_DISEASE_TYPE;
		if (strDiseaseName == "坑槽") return		hnLiqingDiseaseInfo_CityRoad::KC_DISEASE_TYPE;
		if (strDiseaseName == "啃边") return		hnLiqingDiseaseInfo_CityRoad::KB_DISEASE_TYPE;
		if (strDiseaseName == "泛油") return		hnLiqingDiseaseInfo_CityRoad::FY_DISEASE_TYPE;
		if (strDiseaseName == "路框差") return	hnLiqingDiseaseInfo_CityRoad::LKC_DISEASE_TYPE;
		if (strDiseaseName == "沉陷") return		hnLiqingDiseaseInfo_CityRoad::CX_DISEASE_TYPE;
		if (strDiseaseName == "唧浆") return		hnLiqingDiseaseInfo_CityRoad::JJ_DISEASE_TYPE;
		
	}
	if (disType == SHUINI_ROAD_TYPE) 
	{
		if (strDiseaseName == "线裂") return             hnShuiNiDiseaseInfo_CityRoad::XL_SN_DISEASE_TYPE;
		if (strDiseaseName == "板角断裂") return        hnShuiNiDiseaseInfo_CityRoad::BJDL_DISEASE_TYPE;
		if (strDiseaseName == "边角裂缝") return        hnShuiNiDiseaseInfo_CityRoad::BJLF_SN_DISEASE_TYPE;
		if (strDiseaseName == "交叉裂缝和破碎板") return hnShuiNiDiseaseInfo_CityRoad::JCLFHPSB_SN_DISEASE_TYPE;
		if (strDiseaseName == "接缝料损坏") return		hnShuiNiDiseaseInfo_CityRoad::JFLSH_DISEASE_TYPE;
		if (strDiseaseName == "边角剥落") return			hnShuiNiDiseaseInfo_CityRoad::BJBL_DISEASE_TYPE;
		if (strDiseaseName == "坑洞") return				hnShuiNiDiseaseInfo_CityRoad::KD_DISEASE_TYPE;
		if (strDiseaseName == "表面裂纹") return			hnShuiNiDiseaseInfo_CityRoad::BMLW_SN_DISEASE_TYPE;
		if (strDiseaseName == "层状剥落") return			hnShuiNiDiseaseInfo_CityRoad::CZBL_SN_DISEASE_TYPE;
		if (strDiseaseName == "错台") return				hnShuiNiDiseaseInfo_CityRoad::CT_DISEASE_TYPE;
		if (strDiseaseName == "拱胀") return				hnShuiNiDiseaseInfo_CityRoad::GZ_SN_DISEASE_TYPE;
		if (strDiseaseName == "路框差") return		    hnShuiNiDiseaseInfo_CityRoad::LKC_SN_DISEASE_TYPE;
		if (strDiseaseName == "沉陷") return				hnShuiNiDiseaseInfo_CityRoad::CX_SN_DISEASE_TYPE;
		if (strDiseaseName == "唧浆") return				hnShuiNiDiseaseInfo_CityRoad::JJ_SN_DISEASE_TYPE;
	}
	return -1;
}
int hnOutputRoadDiseaseXR_CityRoad::getDiseaseType(vector<Disease_C>& vecDisease)
{
	for (int i = 0; i < vecDisease.size(); i++)
	{
		string strDiseaseName = vecDisease[i].diseaseType;
		//沥青特有病害
		
		if (strDiseaseName == "网裂") return LIQING_ROAD_TYPE;
		if (strDiseaseName == "龟裂") return		LIQING_ROAD_TYPE;
		if (strDiseaseName == "拥包") return		LIQING_ROAD_TYPE;
		if (strDiseaseName == "车辙") return		LIQING_ROAD_TYPE;
		
		if (strDiseaseName == "翻浆") return LIQING_ROAD_TYPE;
		if (strDiseaseName == "剥落") return LIQING_ROAD_TYPE;
		if (strDiseaseName == "坑槽") return		LIQING_ROAD_TYPE;
		if (strDiseaseName == "啃边") return		LIQING_ROAD_TYPE;
		if (strDiseaseName == "泛油") return		LIQING_ROAD_TYPE;
		//水泥特有病害
		if (strDiseaseName == "板角断裂") return  SHUINI_ROAD_TYPE;
		if (strDiseaseName == "边角裂缝") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "交叉裂缝和破碎板") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "接缝料损坏") return  SHUINI_ROAD_TYPE;
		if (strDiseaseName == "边角剥落")return SHUINI_ROAD_TYPE;
		if (strDiseaseName == "坑洞") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "表面裂纹") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "层状剥落") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "错台") return		 SHUINI_ROAD_TYPE;
		if (strDiseaseName == "拱胀") return		 SHUINI_ROAD_TYPE;
	}
	return -1;
}
int hnOutputRoadDiseaseXR_CityRoad::getDegree(string strDegree)
{
	if (strDegree == "重") return 1;
	if (strDegree == "条状") return 0;
	if (strDegree == "块状") return 1;
	if (strDegree == "轻") return 0;
	if (strDegree == "中") return 0;
	return -1;
}
void hnOutputRoadDiseaseXR_CityRoad::clearVec()
{
	for (int i = 0; i < m_vecDiseaseInfo.size(); i++)
	{
		delete m_vecDiseaseInfo[i];
	}
	m_vecDiseaseInfo.clear();
}
void hnOutputRoadDiseaseXR_CityRoad::setRoadWidth(double roadWidth)
{

	m_roadWidthXR = roadWidth;
}
void hnOutputRoadDiseaseXR_CityRoad::setRoadDirection(int roadDirection)
{

	m_roadDirectionXR = roadDirection;
}
void hnOutputRoadDiseaseXR_CityRoad::setRoadTotalNum(int roadTotalNum)
{


}
int hnOutputRoadDiseaseXR_CityRoad::string2Num(string str)
{
	string strCHINESE[9] = { "一","二","三","四","五","六","七","八","九" };
	string strNUMBER = "123456789";

	for (int i = 0; i < 9; i++)
	{
		if (str.find(strCHINESE[i]) != string::npos || str.find(strNUMBER[i]) != string::npos)
		{
			return i + 1;
		}
	}

	return 1;
}

void hnOutputRoadDiseaseXR_CityRoad::filterDisease(hnGridDiseaseInfo& gridDisease)
{
	if (gridDisease.vecDiseaseInfo.size()== 0 )
	{
		return;
	}
	//两两比对，后期可优化
	for (int i = 0; i < gridDisease.vecDiseaseInfo.size() - 1; i++)
	{
		for (int j = i + 1; j < gridDisease.vecDiseaseInfo.size(); j++)
		{
			//矩形框有重叠区间
			if (isRectOverlap(gridDisease.vecDiseaseInfo[i]->vecPt, gridDisease.vecDiseaseInfo[j]->vecPt))
			{
				//龟裂优先保留
				if (gridDisease.vecDiseaseInfo[i]->getDiseaseType() == PROJECTROADTYPE::LIQING_ROAD_TYPE && gridDisease.vecDiseaseInfo[i]->nDiseaseType == hnLiqingDiseaseInfo::LIQINGDISEASETYPE::JL_DISEASE_TYPE)
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo);
				}
				//龟裂优先保留
				else if (gridDisease.vecDiseaseInfo[j]->getDiseaseType() == PROJECTROADTYPE::LIQING_ROAD_TYPE && gridDisease.vecDiseaseInfo[j]->nDiseaseType == hnLiqingDiseaseInfo::LIQINGDISEASETYPE::JL_DISEASE_TYPE)
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo);
				}
				//线性病害优先过滤
				else if (isLineDisease(gridDisease.vecDiseaseInfo[i]) && !isLineDisease(gridDisease.vecDiseaseInfo[j]))
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo);
				}
				//线性病害优先过滤
				else if (!isLineDisease(gridDisease.vecDiseaseInfo[i]) && isLineDisease(gridDisease.vecDiseaseInfo[j]))
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo);
				}
				//面积小的病害优先过滤
				else if (gridDisease.vecDiseaseInfo[i]->dArea > gridDisease.vecDiseaseInfo[j]->dArea)
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo);
				}
				//面积小的病害优先过滤
				else if (gridDisease.vecDiseaseInfo[i]->dArea <= gridDisease.vecDiseaseInfo[j]->dArea)
				{
					cutRectOverlap(gridDisease.vecDiseaseInfo[i], gridDisease.vecDiseaseInfo[j], gridDisease.vecDiseaseInfo);
				}
			}
		}
	}
}

bool hnOutputRoadDiseaseXR_CityRoad::isLineDisease(hnDiseaseInfoBase* input)
{
	if (input->getDiseaseType() == PROJECTROADTYPE::LIQING_ROAD_TYPE)//沥青病害
	{
		if (/*input->nDiseaseType ==    hnLiqingDiseaseInfo_CityRoad::LIQINGDISEASETYPE::CZ_DISEASE_TYPE
			||*/ input->nDiseaseType == hnLiqingDiseaseInfo_CityRoad::LIQINGDISEASETYPE::XL_DISEASE_TYPE)
		{
			return true;
		}
	}
	if (input->getDiseaseType() == PROJECTROADTYPE::SHUINI_ROAD_TYPE)//水泥病害
	{
		if (input->nDiseaseType ==    hnShuiNiDiseaseInfo_CityRoad::SHUINIISEASETYPE::XL_SN_DISEASE_TYPE)
		{
			return true;
		}
	}
	return false;
}

bool hnOutputRoadDiseaseXR_CityRoad::isRectOverlap(vector<hn2dPt>& input1, vector<hn2dPt>& input2)
{
	double minX = max(input1[0].dx, input2[0].dx);
	double minY = max(input1[0].dy, input2[0].dy);
	double maxX = min(input1[2].dx, input2[2].dx);
	double maxY = min(input1[2].dy, input2[2].dy);
	if (minX < maxX && minY < maxY)
		return true;

	return false;
}

bool hnOutputRoadDiseaseXR_CityRoad::cutRectOverlap(hnDiseaseInfoBase* cutDisease, hnDiseaseInfoBase* edge, vector<hnDiseaseInfoBase*> &root)
{
	double minX = max(cutDisease->vecPt[0].dx, edge->vecPt[0].dx);
	double minY = max(cutDisease->vecPt[0].dy, edge->vecPt[0].dy);
	double maxX = min(cutDisease->vecPt[2].dx, edge->vecPt[2].dx);
	double maxY = min(cutDisease->vecPt[2].dy, edge->vecPt[2].dy);
	double deltaX = maxX - minX;
	double deltaY = maxY - minY;

	//分4种位移情况
	vector<hn2dPt> vecMoveMinX, vecMoveMaxX, vecMoveMinY, vecMoveMaxY;
	//初始化赋值
	for (int i = 0; i < 4; i++)
	{
		vecMoveMinX.push_back(cutDisease->vecPt[i]);
		vecMoveMaxX.push_back(cutDisease->vecPt[i]);
		vecMoveMinY.push_back(cutDisease->vecPt[i]);
		vecMoveMaxY.push_back(cutDisease->vecPt[i]);
	}
	//矩形边界位移
	vecMoveMinX[0].dx += deltaX; vecMoveMinX[3].dx += deltaX;
	vecMoveMaxX[1].dx -= deltaX; vecMoveMaxX[2].dx -= deltaX;
	vecMoveMinY[0].dy += deltaY; vecMoveMinY[1].dy += deltaY;
	vecMoveMaxY[2].dy -= deltaY; vecMoveMaxY[3].dy -= deltaY;
	//边界位移前矩形面积
	double cutDiseaseArea = (cutDisease->vecPt[2].dy - cutDisease->vecPt[0].dy) *(cutDisease->vecPt[2].dx - cutDisease->vecPt[0].dx);
	vector<double>deltaArea; deltaArea.push_back(cutDiseaseArea); deltaArea.push_back(cutDiseaseArea); deltaArea.push_back(cutDiseaseArea); deltaArea.push_back(cutDiseaseArea);
	//边界位移后无重叠矩形，则算作有效位移
	if (!isRectOverlap(vecMoveMinX, edge->vecPt))
	{
		double newDiseseArea = (vecMoveMinX[2].dy - vecMoveMinX[0].dy) *(vecMoveMinX[2].dx - vecMoveMinX[0].dx);
		deltaArea[0] = cutDiseaseArea - newDiseseArea;
	}
	if (!isRectOverlap(vecMoveMaxX, edge->vecPt))
	{
		double newDiseseArea = (vecMoveMaxX[2].dy - vecMoveMaxX[0].dy) *(vecMoveMaxX[2].dx - vecMoveMaxX[0].dx);
		deltaArea[1] = cutDiseaseArea - newDiseseArea;
	}
	if (!isRectOverlap(vecMoveMinY, edge->vecPt))
	{
		double newDiseseArea = (vecMoveMinY[2].dy - vecMoveMinY[0].dy) *(vecMoveMinY[2].dx - vecMoveMinY[0].dx);
		deltaArea[2] = cutDiseaseArea - newDiseseArea;
	}
	if (!isRectOverlap(vecMoveMaxY, edge->vecPt))
	{
		double newDiseseArea = (vecMoveMaxY[2].dy - vecMoveMaxY[0].dy) *(vecMoveMaxY[2].dx - vecMoveMaxY[0].dx);
		deltaArea[3] = cutDiseaseArea - newDiseseArea;
	}
	//比较有效位移的方案里原始矩形面积变化，取变化最小的位移方案
	double minDeltaArea = min(deltaArea[0], min(deltaArea[1], min(deltaArea[2], deltaArea[3])));
	//若4个位移方案都无效，则要把旧矩形一分为二
	if (abs(deltaArea[0] - cutDiseaseArea) < 0.001&& abs(deltaArea[1] - cutDiseaseArea) < 0.001&&abs(deltaArea[2] - cutDiseaseArea) < 0.001&&abs(deltaArea[3] - cutDiseaseArea) < 0.001)
	{
		if (cutDisease->vecPt[0].dy < edge->vecPt[0].dy && cutDisease->vecPt[3].dy > edge->vecPt[3].dy)
		{
			hnDiseaseInfoBase* newDisease = cutDisease->clone();
			cutDisease->vecPt[2].dy = cutDisease->vecPt[3].dy = edge->vecPt[0].dy;
			newDisease->vecPt[0].dy = newDisease->vecPt[1].dy = edge->vecPt[3].dy;
			root.push_back(newDisease);
		}
		else if (cutDisease->vecPt[0].dx < edge->vecPt[0].dx && cutDisease->vecPt[1].dx > edge->vecPt[1].dx)
		{
			hnDiseaseInfoBase* newDisease = cutDisease->clone();
			cutDisease->vecPt[1].dx = cutDisease->vecPt[2].dx = edge->vecPt[0].dx;
			newDisease->vecPt[0].dx = newDisease->vecPt[3].dx = edge->vecPt[1].dx;
			root.push_back(newDisease);
		}
		return true;
	}
	//应用有效位移的方案
	if (deltaArea[0] == minDeltaArea)
	{
		cutDisease->vecPt.assign(vecMoveMinX.begin(), vecMoveMinX.end());
	}
	if (deltaArea[1] == minDeltaArea)
	{
		cutDisease->vecPt.assign(vecMoveMaxX.begin(), vecMoveMaxX.end());
	}
	if (deltaArea[2] == minDeltaArea)
	{
		cutDisease->vecPt.assign(vecMoveMinY.begin(), vecMoveMinY.end());
	}
	if (deltaArea[3] == minDeltaArea)
	{
		cutDisease->vecPt.assign(vecMoveMaxY.begin(), vecMoveMaxY.end());
	}
	return true;
}