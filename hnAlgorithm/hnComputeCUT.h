#pragma once
#include "stdafx.h"
#include <vector>
#include "..\hnQtCommon\MyPoint.h"
using namespace std;


class HNALGORITHM_API hnComputeCUT
{
public:
	hnComputeCUT(void);
	~hnComputeCUT(void);

public:

    //双车辙，以中间凸起点为界，左右各计算一个车辙深度
    float computerut3(int length ,float ArrayHeight[], float ArrayDistance[], int nStart, int nEnd, float& RD_left, float& RD_right, float& m_k, int flen,
    int MP_idx[], float  MP_val[]);
	float computerut(float* line, float* gsfilter,  int gsfLength , int lines, int linee,
		float threshval, int partlen, int pointthr, MyQtCommon::MyPoint* py, float* tline);
private:

	float fInvalide = 2200;

	//倾斜矫正
	void rut_slopCorrect(float * ArrayHeight,int nStart,int nEnd,float * correctH);

	 //最小二乘法拟合直线
     void LSLineFit(float* ArrayHeight, int nStart, int nEnd, float& m_k, float& m_b);

	 //最小二乘法拟合直线
	 void LSLineFit_New(float* ArrayHeight, int nStart, int nEnd, float& m_k, float& m_b);

	 //拟合直线，对高度进行转换
     void Height2Distance(float* Arrayheight, int numstart, int numend, float m_k, float m_b);

	//通过拉依达法则去除异常值
    bool eraseOutliers(float* ArrayHeight, int nStart, int nEnd);

	//中值滤波
    void MidianAverageFileter(float* x, int ns, int ne, int flen, float* y);

	//计算数组的平均值和方差
    void mean_std(float* ArrayHeight, int nStart, int nEnd, float& mean, float& std_new);

	/***********************************************************************************/
	/*       w0 \     w2 /\     w4 /                                                   */
	/* 			 \      /  \      /                                                    */
	/*			  \    /    \    /                                                     */
	/*			   \  /      \  /                                                      */
	/*			 w1 \/     w3 \/               车辙W模型，算法中各凹凸点示意           */
	/***********************************************************************************/
	//寻找车辙的中间凸起点，通过高程数据[500,1500]内的数据的极大值确定,并确定包络线类型
	//VecHeights:为路面高程数据
	//num_start:计算数据的左端点序号，小于500  num_end:计算数据的右端点序号，大于1500
	//RP_idx:凸起点序号      RP_val：凸起点值
	//type_envelope:0或者1，分别对应端点连线或端点中间凸起点折线作为包络线
    void findMaximumPiont(float* ArrayHeight, int num_start, int num_end, int* MP_idx, float* MP_val, int& type_envelope);

	//计算左右车辙深度
	//VecHeights:为路面高程数据
	//RP_idx:凸起点序号      RP_val：凸起点值
	//type_envelope:0或者1，分别对应端点连线或端点中间凸起点折线作为包络线
	//RD_left:左车辙值             RD_right：右车辙值
	void getRD(float* ArrayHeight, int* MP_idx, float* MP_val, int type_envelope,
		float& RD_left, float& RD_right);

	//寻找[idx_start,idx_end]内的极小点作为凹点，记录凹点的序号和对应的值
   void findPit(float* ArrayHeight, int idx_start, int idx_end, int& Pit_idx, float& Pit_val, int side);

   //寻找车辙的左右端点
   void findedge(float* ArrayHeight, int idx_start, int idx_end, int& edge_idx, float& edge_val, int side);

   //根据包络线和凹点数据计算车辙深度
   void calculateRD(float* ArrayHeight, int* MP_idx, float* MP_val, int MPleft_idx, int MPright_idx,
	   int MPpoint_idx, float& RD);

   void Average_near(float* ArrayHeight, int sidx, int eidx, int idx_center, float& value_average);


   float GetRutVal(MyQtCommon::MyPoint*  Pt, int lines, int linee, float thresh);


   void GetMiPoint(MyQtCommon::MyPoint* Pt, int ns, int ne, vector<MyQtCommon::MyPoint>& MiPt, float thresh);
   float GetTrigH_ac(MyQtCommon::MyPoint pta, MyQtCommon::MyPoint ptb, MyQtCommon::MyPoint ptc);
};

