/*!@file
***************************************************************************
<PRE>
模块名		：平面拟合   
文件名		：hnPlaneFit.h
相关文件	: hnPlaneFit.cpp
文件实现功能：通过选择的点画出一个平面
作者		：李夏亮
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2021/11/24	1.0			李夏亮		创建
</PRE>
******************************************************************************************************/

#pragma once
#include <vector>
#include "stdafx.h"
#include "..\hnCommon\hn3dPointDef.h"
using namespace std;
using namespace hnCommon;

class HNALGORITHM_API hnPlaneFit
{
public:
	hnPlaneFit(void);
	~hnPlaneFit(void);

public:

	// 最小二乘法得到平面的法向量
	// cInputInFo:点集合
	// dOutput_a dOutput_b dOutput_c ：法向量
	// bIsDNoise：是否是排除噪声的拟合  是侧为true 否则为false
	// 不排除噪声的拟合
	void LeastSquares(vector<hn3dPointD>& cInputInFo,double& dOutput_a,double& dOutput_b,double& dOutput_c,bool bIsDNoise);

	// 最小二乘法排除噪声的拟合
	void LSPlaneFit(vector<hn3dPointD>& cInputInFo);

	//获得平面表示圆的半径
	double GetExpressRadius();

	//获得法向量
	void GetVector(double& dAxisa,double& dAxisb,double& dAxisc);

	// 获取D值
	double getDValue(){return m_paramterC; }

private:

	// 初始化参数
	void Initialize(const vector<hn3dPointD>& colPoints);

	// 平面拟合的核心算法
	// pts：选择的点的坐标集合
	// fAccuracy: 精度
	// nMaxRepeat : 最大的循环的次数
	void FitPlane(vector<hn3dPointD>& pts,float fAccuracy,int nMaxRepeat);


	// 利用变型QR方法计算实对称三对角矩阵全部特征值及特征向量
	// n-矩阵的阶数
	// b-长度为n的数组，返回时存放三对角阵的主对角线元素
	// c-长度为n的数组，返回时前n-1个元素存放次对角线元素
	// q-长度为n*n的数组，若存放单位矩阵，则返回实对称三对角矩阵的特E征向量组
	// 若存放Householder变换矩阵，则返回实对称矩阵A的特征向量组
	int  Aebstq(int n,double b[],double c[],double q[],double fAccuracy,int nMaxRepeat);


	// 约化对称矩阵为三对角对称矩阵
	// 利用Householder变换将n阶实对称矩阵约化为对称三对角矩阵
	// a-长度为n*n的数组，存放n阶实对称矩阵//n-矩阵的阶数
	// q-长度为n*n的数组，返回时存放Householder变换矩阵
	// b-长度为n的数组，返回时存放三对角阵的主对角线元素
	// c-长度为n的数组，返回时前n-1个元素存放次对角线元素void Aebstq(double a[],int n,double q[],double b[],double c[]);
	void Eastrq(double a[],int n,double q[],double b[],double c[]);

	// 获得标准差
	void GetStanderDiv();

	// 得到特征矩阵的特征值及特征向量
	void GetCharaValueAndVector(double fAccuracy,int nMaxRepeat);

	// 判断是否需要进行下一次循环
	bool PlanfitIsOK();

	//重载拟合
	void LeastSquares(vector<hn3dPointD>& cInputInFo);

	

private:
	vector<hn3dPointD>              m_arrPts;                    //所有计算的点集合
	hn3dPointD                      m_vcPlane;                   //向量及平面方程ax + by +cZ + d = 0的a,b,c 值
	double                        m_paramterC;                 //向量及平面方程ax + by +cZ + d = 0的d 值
	vector<double>                m_disEvPoint;                //集合中每一个点到面的距离
	double      	              m_standerDeviation;          //标准偏差
	double                        m_catercorner_value[3];      //特征矩阵三角化后对角线的元素值
	double                        m_secondCater[3];            //特征矩阵三角化后次对角线的元素值
	double                        m_Householder[9];            //特征向量矩阵
	
	double                        m_chMatrix[3][3];            //特征矩阵 
	int                           m_length ;                   //点的个数
	double                        m_realMatrix[9];             //一维的保存是对称矩阵；
	int                           m_number;                    //去除噪声是 循环的次数

public:
	hn3dPointD                      m_avPoint;                   //平均距离点

};

