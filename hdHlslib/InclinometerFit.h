#pragma once
#include <Windows.h>
#include <stdio.h>
#include ".\inc\mydefs.hpp"

namespace hd
{
	//class CHdLandMarkTransform;
// 倾角仪拟合
class HLS_API CInclinometerFit
{
private:
	// 多项式指数
	int m_n;
	// roll角多项式参数
	double* m_pRollPara;
	// pitch角多项式参数
	double* m_pPitchPara;
	// 倾角仪侧翻误差
	double m_errorRoll;
	// 倾角仪俯仰误差
	double m_errorPitch;
	// 旋转矩阵
	double m_rotateMat[9];
	//CHdLandMarkTransform* m_pRotTrans;
public:
	CInclinometerFit(void);
	~CInclinometerFit(void);

	inline void Clear();

	inline void SetError(double errRoll,double errPitch);
	
	// 是否有倾角仪数据
	inline BOOL HasInc(){return m_pPitchPara != NULL && m_pRollPara != NULL && m_n >= 3;}
	// 设置多项式系数
	void SetPolyN(int n);
	// 加载采样点,并计算拟合参数
	BOOL FitBySimpleData(const char* path);
	// 计算拟合多项式参数
	BOOL GetFittingPara(
		int		n,				// 指多项式指数,系数为指数加1
		double *pObsArgument,	// 为观测值向量
		double *pRealArgument,	// 真实值向量
		int		num,			// 观测值个数
		double	offset,			// 偏移因子（防止数据泄露）
		double *pPara);			// 系数向量

	// 曲面3次方拟合,z=a*pow(x,2) + b*x*y + c*pow(y,2) + d*x + e*y + f;
	BOOL GetSurfaceFitPara(double *pSampleX,double *pSampleY,double *pSampleZ,int numSample, double *pPara);
	// 求直线与曲面交点
	BOOL GetIntersectOfLine(double* pPara,double ptX1,double ptY1,double ptZ1,
										  double ptX2,double ptY2,double ptZ2,
										  double& outX,double& outY,double& outZ);
	// 根据倾角仪观察数据,计算补偿值

	// 根据观测值,计算拟合值
	inline BOOL GetFitValue(double obs,double& roll,double& pitch);
	// 根据水平角度,旋转roll和pitch和误差值,旋转xyz
	void RectifyPoint(double obs,float angleZ,float& x,float& y,float& z);

};

}