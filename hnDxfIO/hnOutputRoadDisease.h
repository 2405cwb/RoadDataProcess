#pragma once
#include "hnOutDiseaseDxf.h"
#include"hnOutputXR.h"
typedef struct _HN_MILEAGE_PILE_
{
	_HN_MILEAGE_PILE_()
	{
		dMileage = 0.0;
		dRealMileage = 0.0;
	}

	// 相对里程
	double dMileage;

	// 真实里程
	double dRealMileage;
}hnMileagePile;

// 这是一个基类，读取病害并对病害做一些检查和分割等预处理
// 不同的道路等级需要对这个类进行继承拓展
class DXFLIB_EXPORT hnOutputRoadDisease
{
public:
	hnOutputRoadDisease(int divideMile = 500);
	~hnOutputRoadDisease(void);

public:
	bool outDisease(const char* strDisease, const char* strCenter, const char* strMileagePile,
		const char* outDir,void (*processCallback)(float, const char*) = NULL);

	// 设置里程信息
	void setMileageInfo(double dBegMil, int nMilDir, int nCenterType, int nLineType);
	
protected:

	// 删除数据
    void clearData();

	// 读取病害
	bool readDisease(const char* strDisease, const char* strCenter, const char* strMileagePile);

	// 计算病害属性
	bool calDiseaseAttr();

	// 变换面顶点顺序
	void convertPlane(vector<hn2dPt>& vecPt);

	// 划分病害
	bool divideDiseaseGroup(vector<hnGridDiseaseInfo>& vecGrid);

	// 分割病害
	bool segGroupDisease(hnGridDiseaseInfo& gridDisease);

	// 检查病害
	bool checkDisease(hnGridDiseaseInfo& gridDisease);

	// 计算某点到中心线的投影距离以及相对里程
	bool getMileageAndDist(hn2dPt pt, double& dDist, double& dMileage);

	// 里程
	bool getRealMileage(double& dMileage);

	// 点坐标
	void getPoint(hn2dPt ptBeg, hn2dPt ptEnd, hn2dPt& oriPt);

	// 分割病害
	bool segDisease(double dBegMile, double dEndMile, hnDiseaseInfoBase* roadDisease);

protected:
	// 起始里程
	double m_dBegMileage;

	// 里程方向 0- 大里程， 1- 小里程
	int m_nMileageDir;

	// 道路病害信息
	vector<hnDiseaseInfoBase*> m_vecDiseaseInfo;

	// 中心线数据
	vector<hn2dPt> m_vecCenter;

	// 里程桩数据
	vector<hnMileagePile> m_vecMileagePile;

	// 最大最小里程
	double m_dMaxMile;
	double m_dMinMile;

	// 道路
	int m_nCenterType;

	// 上下行 0:从小到大	1:从大到小
	int m_nLineType;

	//导出时切分里程长度
	int m_divideMile;
};

