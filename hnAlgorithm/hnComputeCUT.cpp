#include "StdAfx.h"
#include "hnComputeCUT.h"
#include <algorithm>
#include <iostream>

hnComputeCUT::hnComputeCUT(void)
{
}


hnComputeCUT::~hnComputeCUT(void)
{
}




  //双车辙，以中间凸起点为界，左右各计算一个车辙深度
float hnComputeCUT::computerut3 (int length ,float ArrayHeight[], float ArrayDistance[],int nStart, int nEnd, float& RD_left, float& RD_right, float& m_k, int flen,
    int MP_idx[], float MP_val[])
{


    RD_left = 0.0f;
    RD_right = 0.0f;

    int type_envelope = -1;
    bool isErase = false;

    m_k = 0.0f;
    float m_b = 0.0f;
	MidianAverageFileter(ArrayHeight, nStart, nEnd, flen, ArrayHeight);
	float* correctH = new float[length]();
	//增加倾斜矫正
	rut_slopCorrect(ArrayHeight, nStart, nEnd, correctH);
	delete[]  correctH;
	correctH = nullptr;
	LSLineFit_New(ArrayHeight, nStart, nEnd, m_k, m_b);

    Height2Distance(ArrayHeight, nStart, nEnd, m_k, m_b);

    isErase = eraseOutliers(ArrayHeight, nStart, nEnd);
    if (isErase)
    {
        m_k = 0.0f;
        m_b = 0.0f;

		LSLineFit_New(ArrayHeight, nStart, nEnd, m_k, m_b);
        Height2Distance(ArrayHeight, nStart, nEnd, m_k, m_b);
        eraseOutliers(ArrayHeight, nStart, nEnd);
    }

	MidianAverageFileter(ArrayHeight, nStart, nEnd, flen, ArrayDistance);
	findMaximumPiont(ArrayHeight, nStart, nEnd, MP_idx, MP_val, type_envelope);
    getRD(ArrayDistance, MP_idx, MP_val, type_envelope, RD_left, RD_right);
	
    return max(RD_left, RD_right);
}

float hnComputeCUT::computerut(float * line, float * gsfilter,int gsfLength ,int lines, int linee, float threshval, int partlen, int pointthr, MyQtCommon::MyPoint * py, float * tline)
{
	float m_k = 0.0f;
	float m_b = 0.0f;

	LSLineFit(line, lines, linee,  m_k,  m_b);
	Height2Distance( line, lines, linee, m_k, m_b);
	eraseOutliers(line, lines, linee);

	MidianAverageFileter(line, lines, linee, gsfLength, tline);

	//leastsquare(tline, lines, linee, ref k, ref b);
	//distanceline(ref tline, lines, linee, k, b);

	for (int i = lines; i < linee; ++i)
	{
		py[i].px = i;
		py[i].py = tline[i];
	}
	return GetRutVal(py, lines, linee, 0.1f);


}

void hnComputeCUT::rut_slopCorrect(float * ArrayHeight, int nStart, int nEnd, float * correctH)
{
	if (nStart - 200>0)
	{
		nStart -= 200;
	}
	float d_nStart = ArrayHeight[nStart];
	float d_nEnd = ArrayHeight[nEnd - 1];
	for (int i = nStart ; i<nEnd; ++i)
	{
		if (d_nStart >d_nEnd)
		{
			correctH[i] = ArrayHeight[i] - d_nEnd;
		}
		else
		{
			correctH[i] = ArrayHeight[i] - d_nStart;
		}
	}
	float C_nStart = correctH[nStart];
	float C_nEnd = correctH[nEnd - 1];
	for (int i = nStart ; i<nEnd; ++i)
	{
		if (d_nStart >d_nEnd)
		{
			float d_Height = ((nEnd - i) * C_nStart) / (nEnd - nStart);
			correctH[i] = correctH[i] - d_Height + d_nEnd;
			ArrayHeight[i] = correctH[i];
		}
		else
		{
			float d_Height = ((i - nStart)*C_nEnd) / (nEnd - nStart);
			correctH[i] = correctH[i] - d_Height + d_nStart;
			ArrayHeight[i] = correctH[i];
		}
	}
}

//最小二乘法拟合直线
void hnComputeCUT::LSLineFit(float* ArrayHeight, int nStart, int nEnd, float& m_k, float& m_b)
{
    float sumX2 = 0.0f;
    float sumX = 0.0f;
    float sumXY = 0.0f;
    float sumY = 0.0f;

    for (int i = nStart; i < nEnd; i++)
    {
		if (std::abs(ArrayHeight[i])>= fInvalide)
		{
			continue;
		}
        sumX2 += i * i;
        sumX += i;
        sumXY += i * ArrayHeight[i];
        sumY += ArrayHeight[i];
    }

    //计算斜率和截距
    int num_point = nEnd - nStart;
    float tempDenominator = num_point * sumX2 - sumX * sumX;

    if (tempDenominator != 0)
    {
        m_k = (num_point * sumXY - sumX * sumY) / tempDenominator;
        m_b = (sumX2 * sumY - sumX * sumXY) / tempDenominator;
    }
    else
    {
        m_k = 1.0f;
        m_b = 0.0f;
    }
}

void hnComputeCUT::LSLineFit_New(float* ArrayHeight, int nStart, int nEnd, float& m_k, float& m_b)
{
 //计算起点附近的平均值 
	float startAvgHeight = 0.0f;
	float startAvgX = 0.0f;
	int startPointCount =(std::min)(5, nEnd - nStart + 1);
	 
	int pointCnt = 0; 
	for ( int i = nStart ; i<nStart + startPointCount  ; i ++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCnt++;
		startAvgHeight += ArrayHeight[i];
		startAvgX += i;
	}
	startAvgHeight /= pointCnt;
	startAvgX /= pointCnt;
	pointCnt = 0; 
	//计算终点附近平均值
	float endAvgHeight = 0.0f;
	float endAvgX = 0.0f;
	int endPointCount =(std::min)(5, nEnd - nStart + 1);
	int endStartIndex = (std::max)(nStart, nEnd - endPointCount + 1);
 
	for (int i = endStartIndex; i <=nEnd; i++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCnt++;
		endAvgHeight += ArrayHeight[i];
		endAvgX += i;
	}
	endAvgHeight /= pointCnt;
	endAvgX /= pointCnt;

	//两点确定一条直线
	float  deltaX = endAvgX - startAvgX;

	if (std::abs(deltaX) >0)
	{
		m_k = (endAvgHeight - startAvgHeight) / deltaX;
		m_b = startAvgHeight - m_k  * startAvgX;
	}
	else
	{
		m_k = 0;
		m_b =( startAvgHeight+endAvgHeight) /2.0f;
	}


}

//拟合直线，对高度进行转换
void hnComputeCUT::Height2Distance(float* Arrayheight, int numstart, int numend, float m_k, float m_b)
{
    for (int i = numstart; i < numend; i++)
	{
		if (std::abs(Arrayheight[i]) >= fInvalide)
		{
			continue;
		}
        //计算点到线的距离
        Arrayheight[i] = Arrayheight[i] - (m_k * i + m_b);
    }
}

//通过拉依达法则去除异常值
bool hnComputeCUT::eraseOutliers(float* ArrayHeight, int nStart, int nEnd)
{
    bool isErase = false;

    float mean = 0.0f, stdNew = 0.0f;
    mean_std(ArrayHeight, nStart, nEnd, mean, stdNew);
    float stdThresh = 3.34f * stdNew + 2.0f;
    for (int i = nStart; i < nEnd; i++)
    {
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

        if (abs(ArrayHeight[i] - mean) > stdThresh)
        {
            ArrayHeight[i] = mean;
            isErase = true;
        }
    }

    return isErase;
}

 //中值滤波
void hnComputeCUT::MidianAverageFileter(float* x, int ns, int ne, int flen, float* y)
{
    int i = 0, j = 0, hflen = 0, ti = 0, cnt = 0;
    float minval = 10000, maxval = -10000, sum = 0.0f;
    if ((ne - ns) > (flen - 1) * 2)
    {
        hflen = (flen - 1) / 2;
        for (i = ns; i < ne; ++i)
        {
            minval = 10000;
            maxval = -10000;
            sum = 0.0f;
            cnt = 0;
            for (j = -hflen; j <= hflen; ++j)
            {
                ti = i + j;
				if (std::abs(x[i+j])>= fInvalide)
				{
					continue;
				}
                if (ti < ns || ti >= ne)
                    continue;

                if (minval >= x[i + j])
                {
                    minval = x[i + j];
                }
                if (maxval < x[i + j])
                {
                    maxval = x[i + j];
                }
                sum += x[i + j];
                cnt++;
            }
            if (cnt > 2)
            {
                y[i] = (sum - minval - maxval) / (cnt - 2);
            }
            else
            {
                y[i] = x[i];
            }
        }
    }
}

//计算数组的平均值和方差
void hnComputeCUT::mean_std(float* ArrayHeight, int nStart, int nEnd, float& mean, float& std_new)
{
	double sum = 0;
    mean = 0.0f; 
	int pointCount = 0;
    for (int i = nStart; i < nEnd; i++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCount++;
		sum += ArrayHeight[i];
       // mean += ArrayHeight[i];
    }
    //mean = mean / (nEnd - nStart);
	mean = (float)(sum / pointCount);
	sum = 0;
    std_new = 0.0f;
    for (int i = nStart; i < nEnd; i++)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		sum += (ArrayHeight[i] - mean) * (ArrayHeight[i] - mean);
       // std_new += (ArrayHeight[i] - mean) * (ArrayHeight[i] - mean);
    }
	std_new = (float)sqrt(sum / pointCount);
    //std_new = (float)sqrt(std_new / (nEnd - nStart));
}

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
void hnComputeCUT::findMaximumPiont(float* ArrayHeight, int num_start, int num_end, int* MP_idx, float* MP_val,
	int& type_envelope)
{
    MP_val[2] = -10000.0f;
    //for (int i = (num_start + num_end) / 2 - 300; i < (num_start + num_end) / 2 + 300; i++)
    //int tsidx = (num_start + num_end) / 3;
    //int teidx = (num_start + num_end) * 2 / 3;
    int meidx = (num_start + num_end) / 2;
    int tsidx = meidx - 300;
    int teidx = meidx + 300;
	if(tsidx <= 0)
	{
		tsidx = 0;
	}

    for (int i = tsidx; i < meidx; ++i)
    {
		if (std::abs(ArrayHeight[i]) >=fInvalide)
		{
			continue;
		}
        if (ArrayHeight[i] >= MP_val[2])
        {
            MP_val[2] = ArrayHeight[i];
            MP_idx[2] = i;
        }
    }
    for (int i = meidx; i < teidx; ++i)
    {
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
        if (ArrayHeight[i] > MP_val[2])
        {
            MP_val[2] = ArrayHeight[i];
            MP_idx[2] = i;
        }
    }

    Average_near(ArrayHeight, num_start, num_end, MP_idx[2], MP_val[2]);

    //寻找左侧凹点，W1
    findPit(ArrayHeight, num_start + 200, MP_idx[2], MP_idx[1], MP_val[1], 0); 

    //寻找左侧端点，W0
    findedge(ArrayHeight, num_start, MP_idx[1] , MP_idx[0], MP_val[0], 0);

    //寻找右侧凹点，W3
    findPit(ArrayHeight, MP_idx[2], num_end -300, MP_idx[3], MP_val[3], 1);

    //寻找右侧端点，W4
    findedge(ArrayHeight, MP_idx[3] , num_end, MP_idx[4], MP_val[4], 1);

    //判断包络线类型，凸W：type_envelope =1 凹W：type_envelope =0
    double k = (MP_val[4] - MP_val[0]) / (MP_idx[4] - MP_idx[0]);
    double b = MP_val[0] - k * MP_idx[0];
    double dis = MP_val[2] - (k * MP_idx[2] + b);

    //中间凸点在端点连线下方，凹W
    if (dis <= 0)
    {
        type_envelope = 0;
    }
    //中间凸点在端点连线上方，凸W
    else
    {
        type_envelope = 1;
    }
}

//计算左右车辙深度
//VecHeights:为路面高程数据
//RP_idx:凸起点序号      RP_val：凸起点值
//type_envelope:0或者1，分别对应端点连线或端点中间凸起点折线作为包络线
//RD_left:左车辙值             RD_right：右车辙值
void hnComputeCUT::getRD(float* ArrayHeight, int* MP_idx, float* MP_val, int type_envelope,
	float& RD_left, float& RD_right)
{
    //根据包络线类型计算左右车辙
    if (type_envelope == 0)
    {
        //端点连线包络线
        //左车辙
        calculateRD(ArrayHeight, MP_idx, MP_val, 0, 4, 1, RD_left);
        //右车辙
        calculateRD(ArrayHeight, MP_idx, MP_val, 0, 4, 3, RD_right);
    }
    else if (type_envelope == 1)
    {
        //端点凸点折线包络线
        //左车辙
        calculateRD(ArrayHeight, MP_idx, MP_val, 0, 2, 1, RD_left);
        //右车辙
        calculateRD(ArrayHeight, MP_idx, MP_val, 2, 4, 3, RD_right);
    }
}

//寻找[idx_start,idx_end]内的极小点作为凹点，记录凹点的序号和对应的值
void hnComputeCUT::findPit(float* ArrayHeight, int idx_start, int idx_end, int& Pit_idx, float& Pit_val, int side)
{
    Pit_val = 10000.0f;
    for (int i = idx_start; i < idx_end; i++)
    {
        if (side == 0)
        {
			if (std::abs(ArrayHeight[i]) >= fInvalide)
			{
				continue;
			}
            if (ArrayHeight[i] <= Pit_val)
            {
                Pit_val = ArrayHeight[i];
                Pit_idx = i;
            }
        }
        else
        {
			if (std::abs(ArrayHeight[i]) >= fInvalide)
			{
				continue;
			}
            if (ArrayHeight[i] < Pit_val)
            {
                Pit_val = ArrayHeight[i];
                Pit_idx = i;
            }
        }
    }

    Average_near(ArrayHeight, idx_start, idx_end, Pit_idx, Pit_val);
}

//寻找车辙的左右端点
void hnComputeCUT::findedge(float* ArrayHeight, int idx_start, int idx_end, int& edge_idx, float& edge_val, int side)
{
    edge_val = -10000.0f;
    for (int i = idx_start; i < idx_end; i++)
    {
        if (side == 1)
        {
			if (std::abs(ArrayHeight[i]) >= fInvalide)
			{
				continue;
			}
            if (ArrayHeight[i] >= edge_val)
            {
                edge_val = ArrayHeight[i];
                edge_idx = i;
            }
        }
        else
        {
			if (std::abs(ArrayHeight[i]) >= fInvalide)
			{
				continue;
			}
            if (ArrayHeight[i] > edge_val)
            {
                edge_val = ArrayHeight[i];
                edge_idx = i;
            }
        }
    }

    Average_near(ArrayHeight, idx_start, idx_end, edge_idx, edge_val);
}

//根据包络线和凹点数据计算车辙深度
void hnComputeCUT::calculateRD(float* ArrayHeight, int* MP_idx, float* MP_val,
	int MPleft_idx, int MPright_idx, int MPpoint_idx, float& RD)
{
	/* if (abs(MP_idx[MPleft_idx] - MP_idx[MPpoint_idx]) < 200 && MPpoint_idx == 1
		 || abs(MP_idx[MPright_idx] - MP_idx[MPpoint_idx]) < 200 && MPpoint_idx == 3)
	 {
		 RD = max(abs(MP_val[MPleft_idx] - MP_val[MPpoint_idx]), max(abs(MP_val[MPpoint_idx] - MP_val[MPright_idx]), abs(MP_val[MPleft_idx] - MP_val[MPright_idx])));
	 }
	 else*/
    {
        if (MP_idx[MPleft_idx] != MP_idx[MPright_idx])
        {
            float k = (MP_val[MPleft_idx] - MP_val[MPright_idx]) / (MP_idx[MPleft_idx] - MP_idx[MPright_idx]);
            float b = MP_val[MPleft_idx] - k * MP_idx[MPleft_idx];

            float tempRD = 0.0f;
            for (int i = MP_idx[MPpoint_idx - 1]; i <= MP_idx[MPpoint_idx + 1]; i++)
            {
				if (std::abs(ArrayHeight[i]) >= fInvalide)
				{
					continue;
				}
                tempRD = k * i + b - ArrayHeight[i];
                if (tempRD > RD)
                {
                    RD = tempRD;
                }
            }
        }
        else
        {
            RD = 0;
        }
    }
}

void hnComputeCUT::Average_near(float* ArrayHeight, int sidx, int eidx, int idx_center, float& value_average)
{
    int idx_left = idx_center - 10;
    int idx_right = idx_center + 10;
    if (idx_left < sidx)
    {
        idx_left = sidx;
    }
    if (idx_right > eidx - 1)
    {
        idx_right = eidx - 1;
    }
	int pointCount = 0; 
    value_average = 0.0f;
    for (int i = idx_left; i <= idx_right; i++)
    {
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}
		pointCount++;
        value_average += ArrayHeight[i];
    }
    value_average = value_average /pointCount;
}

float hnComputeCUT::GetRutVal(MyQtCommon::MyPoint* Pt, int lines, int linee, float thresh)
{
	std::vector<MyQtCommon::MyPoint>  MiPt;
	
	float maxrut = 0.0f, tmprut = 0.0f;

	GetMiPoint(Pt, lines, linee - 1,  MiPt, thresh);
	std::sort(MiPt.begin(), MiPt.end(), [=](const MyQtCommon::MyPoint&x, const MyQtCommon::MyPoint&y)
	{
		return x.px > y.px;
	}
	);
	//// 计算x中间点到x两端点边的高
	//int len = MiPt.Count;
	//for (int i = 0; i < len; ++i)
	//{
	//    for (int j = i + 2; j < len; ++j)
	//    {
	//        tmprut = GetTrigH_ac_b(MiPt, i, j);
	//        if (tmprut > maxrut)
	//        {
	//            maxrut = tmprut;
	//        }
	//    }
	//}
	int len = MiPt.size();
	for (int i = 0; i < len; ++i)
	{
		for (int j = i + 1; j < len; ++j)
		{
			for (int k = j + 1; k < len; ++k)
			{
				if (maxrut == 0)
				{
					//Console.WriteLine();
				}
				tmprut = GetTrigH_ac(MiPt[i], MiPt[j], MiPt[k]);
				if (tmprut > maxrut)
				{
					maxrut = tmprut;
				}
			}
		}
	}
	return maxrut;
}

void hnComputeCUT::GetMiPoint(MyQtCommon::MyPoint* Pt, int ns, int ne, vector<MyQtCommon::MyPoint>& MiPt, float thresh)
{
	if (ne - ns < 127) return;

	//起始点先拉一段直线
	float k = (Pt[ns].py - Pt[ne].py) / (Pt[ns].px - Pt[ne].px);
	float b = Pt[ns].py - Pt[ns].px * k;
	MyQtCommon::MyPoint farminpt(Pt[ns]);
	MyQtCommon::MyPoint farmaxpt(Pt[ns]);
	float farmindis = 100000.0f;
	float farmaxdis = -100000.0f;
	float tmpval = 0.0f;

	std::vector<MyQtCommon::MyPoint> tmpMiPt ;
	MyQtCommon::MyPoint temp(Pt[ns]);
	tmpMiPt.push_back(temp);

	//找直线两侧距离最远的点
	for (int i = ns + 1; i < ne - 1; ++i)
	{
		tmpval = Pt[i].py - (k * Pt[i].px + b);
		if (tmpval < farmindis)
		{
			farminpt.px = Pt[i].px;
			farminpt.py = Pt[i].py;
			farmindis = tmpval;
		}
		if (tmpval > farmaxdis)
		{
			farmaxpt.px = Pt[i].px;
			farmaxpt.py = Pt[i].py;
			farmaxdis = tmpval;
		}
	}

	//全部在直线上方
	if (farmindis > 0 && farmaxdis > 0)
	{
		if (abs(farmaxdis) >= thresh)
		{
			tmpMiPt.push_back(farmaxpt);
		}
	}
	//全部在直线下方
	else if (farmindis < 0 && farmaxdis < 0)
	{
		if (abs(farmindis) >= thresh)
		{
			tmpMiPt.push_back(farminpt);
		}
	}
	else
	{
		//在连线下方距离最远的点
		if (abs(farmindis) >= thresh)
		{
			tmpMiPt.push_back(farminpt);
		}
		//在连线上方距离最远的点
		if (abs(farmaxdis) >= thresh)
		{
			tmpMiPt.push_back(farmaxpt);
		}
	}
	MyQtCommon::MyPoint temp1(Pt[ne]);
	tmpMiPt.push_back(temp1);

	std::sort(tmpMiPt.begin(), tmpMiPt.end(), [=](const MyQtCommon::MyPoint&x, const MyQtCommon::MyPoint&y)
	{
		return x.px > y.px;
	});
	// 极值点的数量，包括端点
	bool iscontain = false;
	int ptlen = tmpMiPt.size();
	for (int i = 0; i < ptlen; ++i)
	{
		iscontain = false;
		for (int j = 0; j < MiPt.size(); ++j)
		{
			if (MiPt[j].px == tmpMiPt[i].px)
			{
				iscontain = true;
				break;
			}
		}
		if (!iscontain)
		{
			MiPt.push_back(tmpMiPt[i]);
		}
	}
	if (ptlen <= 2) return;
	else
	{
		//分线段递归计算
		for (int i = 1; i < ptlen; ++i)
		{
			if (tmpMiPt[i - 1].px < tmpMiPt[i].px)
			{
				GetMiPoint(Pt, tmpMiPt[i - 1].px, tmpMiPt[i].px, MiPt, thresh);
			}
			else
			{
				GetMiPoint(Pt, tmpMiPt[i].px, tmpMiPt[i - 1].px, MiPt, thresh);
			}
		}
	}
}

float hnComputeCUT::GetTrigH_ac(MyQtCommon::MyPoint pta, MyQtCommon::MyPoint ptb, MyQtCommon::MyPoint ptc)
{
	float k = (pta.py - ptc.py) / (pta.px - ptc.px);
	float b = pta.py - pta.px * k;
	return abs(ptb.py - (ptb.px * k + b));
}
