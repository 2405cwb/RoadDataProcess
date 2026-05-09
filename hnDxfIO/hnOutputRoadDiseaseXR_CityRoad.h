#pragma once
#include "hnoutputroaddisease.h"

class DXFLIB_EXPORT hnOutputRoadDiseaseXR_CityRoad :
	public hnOutputRoadDisease
{
public:
	hnOutputRoadDiseaseXR_CityRoad(int divideMile);
	~hnOutputRoadDiseaseXR_CityRoad(void);
	//主接口
	bool outDisease(const char*outDir, vector<Disease_C>& vecDisease, hnGridDiseaseInfo grid, int direction, void(*processCallback)(float, const char*) = NULL);

	//C#病害转C++病害
	bool calDiseaseAttr(vector<Disease_C>& vecDisease);

	//设置道路宽度
	void setRoadWidth(double roadWidth);
	//设置道路上下行
	void setRoadDirection(int roadDirection);
	//设置道路总数
	void setRoadTotalNum(int roadTotalNum);
private:
	//获取病害Index序号
	int getDiseaseIndex(string strDiseaseName, int disType, int degree);
	//获取道路类型（水泥路还是沥青路）
	int getDiseaseType(vector<Disease_C>& vecDisease);
	//获取病害程度
	int getDegree(string strDegree);
	//C#病害清空内存
	void clearVec();
	//过滤重叠病害
	void filterDisease(hnGridDiseaseInfo& gridDisease);
	//是否是线型病害
	bool isLineDisease(hnDiseaseInfoBase* input);
	//矩形是否相交
	bool isRectOverlap(vector<hn2dPt>& input1, vector<hn2dPt>& input2);
	//切除重叠区间
	bool cutRectOverlap(hnDiseaseInfoBase* cutDisease, hnDiseaseInfoBase* edge, vector<hnDiseaseInfoBase*> &root);
	//1-9以内string类型的汉字、数字转int
	int string2Num(string str);
	//车道宽
	double m_roadWidthXR;
	//车道方向 >0从小到大 <0从大到小
	int m_roadDirectionXR;

	//车道总数
	int m_roadTotal;
};

