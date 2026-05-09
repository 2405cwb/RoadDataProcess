#pragma once
#include <vector>
#include "..\hnConvert\hnDataCombineStructInfo.h"
# include "..\hnPavementCreate3d\hn3dRoadPcdToImage.h"
#include "..\hnPcdCoordinate\hnPcdCoordinate.h"
using namespace std;

typedef struct _HN_ROAD_GEO_PARAM
{
	_HN_ROAD_GEO_PARAM()
	{
		dMileage = 0.0;
		dYaw = 0.0;
		dHAngle = 0.0;
		dVAngle = 0.0;
		dC = 0.0;
	}

	// 里程
	double dMileage;

	// 航向角
	double dYaw;
	
	//断面中心点
	hnPoint3d pt;  

	//横坡
	double dHAngle;

	// 纵坡
	double dVAngle;

	// 曲率
	double dC;

}hnRoadGeoParam;

// POS数据结构
typedef struct HN_POS_STRUCT_INFO
{
	HN_POS_STRUCT_INFO()
	{
		dGpsSecond = dEastCoord = dNorthCoord = dHeight = dHeading = dPitch = dRoll = dLatitude = dLongitude = 0.0;
		fVEast = fVNorth = fVUp = 0.0f;
		nQ = 0;
		fUnknow1 = 0.0f;
		fUnknow2 = 0.0f;
		fUnknow3 = 0.0f;
		fUnknow4 = 0.0f;
		fUnknow5 = 0.0f;
		fUnknow6 = 0.0f;
	}

	bool serialize(const char* strData)
	{
		//int nSize = sscanf_s(strData,"%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f	%d\n",
		//	&dGpsSecond,&dEastCoord,&dNorthCoord,&dHeight,&dHeading,&dPitch,&dRoll,&dLatitude,&dLongitude,
		//	&fVEast,&fVNorth,&fVUp,&nQ);
		//return nSize >= 13? true:false;
		int nSize = sscanf_s(strData, "%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f	%f	%f	%f	%f	%f	%f	%d\n",
			&dGpsSecond, &dLatitude, &dLongitude, &dHeight, &dHeading, &dPitch, &dRoll,
			&fVEast, &fVNorth, &fVUp,
			&fUnknow1, &fUnknow2, &fUnknow3, &fUnknow4, &fUnknow5, &fUnknow6, &nQ);
		if (nSize < 17)
		{
			//2021.9.7 是否为13个值
			int n = 0;
			double d1, d2;
			nSize = sscanf_s(strData, "%lf %lf %lf	%lf %lf %lf %lf %lf %lf %f %f %f %d\n",
				&dGpsSecond, &d1, &d2, &dHeight, &dHeading, &dPitch, &dRoll,
				&dLatitude, &dLongitude, &fUnknow3, &fUnknow4, &fUnknow5, &n);

			if (nSize != 13)
			{
				nSize = sscanf_s(strData, "%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f\n",
					&dGpsSecond, &dLatitude, &dLongitude, &dHeight, &dHeading, &dPitch, &dRoll,
					&fVEast, &fVNorth, &fVUp);
			}

		}

		//dHeading += 4.5245076696;

		//return nSize >= 17? true : false;

		//int nSize = sscanf_s(strData,"%lf	%lf	%lf	%lf	%lf	%lf	%lf	%f	%f	%f\n",
		//	&dGpsSecond,&dLatitude,&dLongitude,&dHeight,&dHeading,&dPitch,&dRoll,
		//	&fVEast,&fVNorth,&fVUp);
		return nSize >= 10 ? true : false;

		//return nSize;
	}

	void reserialize(char** strOutput, int count)
	{
		//sprintf_s(*strOutput,count,"%.3lf	%.3lf	%.3lf	%.3lf	%.10lf	%.10lf	%.10lf	%.10lf	%.10lf	%.3f	%.3f	%.3f	%d\n",
		//	dGpsSecond,dEastCoord,dNorthCoord,dHeight,dHeading,dPitch,dRoll,dLatitude,dLongitude,fVEast,fVNorth,fVUp,nQ);

		int nSize = sprintf_s(*strOutput, count, "%.3lf	%.12lf	%.12lf	%.6lf	%.6lf	%.6lf	%.6lf	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%.3f	%d\n",
			dGpsSecond, dLatitude, dLongitude, dHeight, dHeading, dPitch, dRoll,
			fVEast, fVNorth, fVUp,
			fUnknow1, fUnknow2, fUnknow3, fUnknow4, fUnknow5, fUnknow6, nQ);
	}

	double dGpsSecond; // gps time
	double dEastCoord; // 东向坐标,L
	double dNorthCoord;// 北向坐标,B
	double dHeight;    // 高程
	double dHeading;   // 航向角
	double dPitch;     // 俯仰角
	double dRoll;      // 横滚
	double dLatitude;  // 纬度
	double dLongitude; // 经度
	float fVEast;      // 东向速度
	float fVNorth;     // 北向速度
	float fVUp;        // 天顶速度
	float fUnknow1;
	float fUnknow2;
	float fUnknow3;
	float fUnknow4;
	float fUnknow5;
	float fUnknow6;

	int nQ;            // 质量
}hnPosInfo_0;

// 根据用户输入GPS时间，计算当前位置处的道路几何状态
class hnCalRoadGeometry
{
public:
	hnCalRoadGeometry();
	~hnCalRoadGeometry();

	// 设置标定参数 dBaseRoll-横向标定参数(单位：度)；dBasePitch-俯仰标定参数(单位：度）,strPos-pos文件的路径
	void setBaseParam(double dBaseRoll, double dBasePitch, const char* strPos);

	//// 根据输入的时间以及采样间距计算出相应位置横坡、纵坡以及曲率半径
	//bool calRoadGeometery(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

	// 根据输入的时间以及采样间距计算出相应位置横坡、纵坡以及曲率半径
	bool calRoadGeometeryNew(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

	// 根据输入的时间以及采样间距计算出相应位置横坡、纵坡以及曲率半径
	bool calRoadGeometeryNew1(vector<hnRoadGeoParam>& vecGeoParam, double dDist, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

private:
	//1.读POS得到经纬度// 加载POS数据至内存
	bool loadPosData(const char* strPosPath, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

	// 二分查找输入gps位置处的pos信息
	bool hnCalRoadGeometry::dichotomyFindNearestLoc(vector<hnPosInfo_0>&vec, double Target, hnRoadGeoParam& outPos);

	//二维点转三维点
	void pt2D23D(hnPoint3d& point3d, double dGpsTime, double dEast, double dNorth, POS_STRUCT_INFO& outPos);


	//高程统计滤波
	QVector<hnPoint3d> filterCloudByScore(const QVector<hnPoint3d>& inputPoints, double kFactor = 2.0);

	//普通最小二乘法 OLS 直线拟合
	bool fitLineOLS(const QVector<hnPoint3d>&points, double &k, double & b);

	//主函数:RANSAC +OLS联合计算横坡
	//points 输入的单边车道点
	//distanceThreshold 局内点判定阈值(米) 建议设置为0.02~0.05
	//numItenratons ：随机采样迭代次数 
	bool calculateLaneCrossSlop(const QVector<hnPoint3d> & points,
		double distanceThreshold,
		int numIterations,
		double & finalK,
		double & finalB,
		double & crossSlopPercent);






	//中值滤波
	void MidianAverageFileter(float* x, int ns, int ne, int flen, float* y);

	//倾斜矫正
	void rut_slopCorrect(float * ArrayHeight, int nStart, int nEnd, float * correctH);

	//最小二乘法拟合直线
	void LSLineFit_New(float* ArrayHeight, float* ArrayHeightX, int nStart, int nEnd, float& m_k, float& m_b);
private:
	// 横向标定参数(单位：度)
	double m_dBaseRoll;

	// 俯仰标定参数(单位：度）
	double m_dBasePitch;

	// pos 信息
	vector<POS_STRUCT_INFO> m_vecPosInfo;

	// pos路径
	char m_strPos[1024];

	// 3d影像生成接口
	hn3dRoadPcdToImage m_creatImage;

	Eigen::Matrix<double, 4, 4>m_PtsToPos;

	// 点云坐标（二维-三维）管理器
	hnPcdCoordinate m_coordinate;

	float fInvalide = 2200;

};

