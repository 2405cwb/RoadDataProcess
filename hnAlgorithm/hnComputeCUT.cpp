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
// 保留原有公开接口，未启用调试导出时不创建任何断面快照。
float hnComputeCUT::computerut3(
	int length,
	float ArrayHeight[],
	float ArrayDistance[],
	int nStart,
	int nEnd,
	float& RD_left,
	float& RD_right,
	float& m_k,
	int flen,
	int MP_idx[],
	float MP_val[])
{
	return computerut3(length, ArrayHeight, ArrayDistance, nStart, nEnd,
		RD_left, RD_right, m_k, flen, MP_idx, MP_val, nullptr);
}

// 按算法真实执行顺序复制处理后的断面，追踪对象为空时与原流程完全一致。
float hnComputeCUT::computerut3(
	int length,
	float ArrayHeight[],
	float ArrayDistance[],
	int nStart,
	int nEnd,
	float& RD_left,
	float& RD_right,
	float& m_k,
	int flen,
	int MP_idx[],
	float MP_val[],
	hnRutProfileTrace* trace)
{
	if (trace != nullptr)
	{
		trace->worldHeight.assign(ArrayHeight, ArrayHeight + length);
	}

	RD_left = 0.0f;
	RD_right = 0.0f;

	int type_envelope = -1;
	bool isErase = false;

	m_k = 0.0f;
	float m_b = 0.0f;

	// ============================================================
	// 2025 兼容算法最终版
	// 1. 不做第一次额外 MidianAverageFileter
	// 2. 不做 rut_slopCorrect
	// 3. 使用全段有效点 LSLineFit
	// 4. eraseOutliers 后如发生剔除，再拟合一次
	// 5. 只保留最后一次 MidianAverageFileter
	// 6. 在最终 ArrayDistance 上寻找 W0~W4 并计算车辙
	// ============================================================

	LSLineFit(
		ArrayHeight,
		nStart,
		nEnd,
		m_k,
		m_b);
	if (trace != nullptr)
	{
		trace->firstFitK = m_k;
		trace->firstFitB = m_b;
	}

	Height2Distance(
		ArrayHeight,
		nStart,
		nEnd,
		m_k,
		m_b);
	if (trace != nullptr)
	{
		trace->firstDetrended.assign(ArrayHeight, ArrayHeight + length);
	}

	isErase = eraseOutliers(
		ArrayHeight,
		nStart,
		nEnd);
	if (trace != nullptr)
	{
		trace->firstOutlierReplaced.assign(ArrayHeight, ArrayHeight + length);
	}

	if (isErase)
	{
		if (trace != nullptr)
		{
			trace->secondStageApplied = true;
		}
		m_k = 0.0f;
		m_b = 0.0f;

		LSLineFit(
			ArrayHeight,
			nStart,
			nEnd,
			m_k,
			m_b);
		if (trace != nullptr)
		{
			trace->secondFitK = m_k;
			trace->secondFitB = m_b;
		}

		Height2Distance(
			ArrayHeight,
			nStart,
			nEnd,
			m_k,
			m_b);
		if (trace != nullptr)
		{
			trace->secondDetrended.assign(ArrayHeight, ArrayHeight + length);
		}

		eraseOutliers(
			ArrayHeight,
			nStart,
			nEnd);
		if (trace != nullptr)
		{
			trace->secondOutlierReplaced.assign(ArrayHeight, ArrayHeight + length);
		}
	}
	else if (trace != nullptr)
	{
		trace->secondFitK = trace->firstFitK;
		trace->secondFitB = trace->firstFitB;
		trace->secondDetrended = trace->firstOutlierReplaced;
		trace->secondOutlierReplaced = trace->firstOutlierReplaced;
	}

	MidianAverageFileter(
		ArrayHeight,
		nStart,
		nEnd,
		flen,
		ArrayDistance);
	if (trace != nullptr)
	{
		// ArrayDistance 在计算区间外仍是调用方的紧凑临时数据，不能当作横向像素输出。
		trace->filteredHeight.assign(length, fInvalide);
		for (int i = nStart; i < nEnd && i < length; ++i)
		{
			trace->filteredHeight[i] = ArrayDistance[i];
		}
	}

	findMaximumPiont(
		ArrayDistance,
		nStart,
		nEnd,
		MP_idx,
		MP_val,
		type_envelope);
	if (trace != nullptr)
	{
		trace->envelopeType = type_envelope;
		for (int i = 0; i < 5; ++i)
		{
			trace->featureIndexes[i] = MP_idx[i];
			trace->featureValues[i] = MP_val[i];
		}
	}

	getRD(
		ArrayDistance,
		MP_idx,
		MP_val,
		type_envelope,
		RD_left,
		RD_right);
	if (trace != nullptr)
	{
		trace->leftRut = RD_left;
		trace->rightRut = RD_right;
	}

	return (std::max)(RD_left, RD_right);
}

float hnComputeCUT::computerut(float* line, float* gsfilter, int gsfLength, int lines, int linee, float threshval, int partlen, int pointthr, MyQtCommon::MyPoint* py, float* tline)
{
	float m_k = 0.0f;
	float m_b = 0.0f;

	LSLineFit(line, lines, linee, m_k, m_b);
	Height2Distance(line, lines, linee, m_k, m_b);
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

void hnComputeCUT::rut_slopCorrect(
	float* ArrayHeight,
	int nStart,
	int nEnd,
	float* correctH)
{
	if (ArrayHeight == nullptr || correctH == nullptr)
	{
		return;
	}

	if (nEnd - nStart < 2)
	{
		return;
	}

	// 保持你原来的逻辑：
	// 如果左边还有200个点，就向左扩200点
	if (nStart - 200 > 0)
	{
		nStart -= 200;
	}

	// 注意：
	// 整套算法使用 [nStart, nEnd)
	// 所以真正最后一个点是 nEnd - 1
	int lastIndex = nEnd - 1;

	int span = lastIndex - nStart;

	if (span <= 0)
	{
		return;
	}

	float d_nStart = ArrayHeight[nStart];
	float d_nEnd = ArrayHeight[lastIndex];

	// 如果左右参考端点本身就是无效数据，
	// 这一帧不要做倾斜矫正
	if (std::abs(d_nStart) >= fInvalide ||
		std::abs(d_nEnd) >= fInvalide)
	{
		return;
	}


	// =====================================
	// 先转换成相对于较低端点的高度
	// =====================================

	for (int i = nStart; i <= lastIndex; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			correctH[i] = ArrayHeight[i];
			continue;
		}

		if (d_nStart > d_nEnd)
		{
			correctH[i] =
				ArrayHeight[i] - d_nEnd;
		}
		else
		{
			correctH[i] =
				ArrayHeight[i] - d_nStart;
		}
	}


	float C_nStart = correctH[nStart];
	float C_nEnd = correctH[lastIndex];


	// =====================================
	// 线性去除左右端点之间的高差
	// =====================================

	for (int i = nStart; i <= lastIndex; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		if (d_nStart > d_nEnd)
		{
			float d_Height =
				static_cast<float>(lastIndex - i)
				* C_nStart
				/ span;

			correctH[i] =
				correctH[i]
				- d_Height
				+ d_nEnd;
		}
		else
		{
			float d_Height =
				static_cast<float>(i - nStart)
				* C_nEnd
				/ span;

			correctH[i] =
				correctH[i]
				- d_Height
				+ d_nStart;
		}

		ArrayHeight[i] = correctH[i];
	}
}
//最小二乘法拟合直线
void hnComputeCUT::LSLineFit(
	float* ArrayHeight,
	int nStart,
	int nEnd,
	float& m_k,
	float& m_b)
{
	m_k = 0.0f;
	m_b = 0.0f;

	if (ArrayHeight == nullptr ||
		nEnd <= nStart)
	{
		return;
	}

	double sumX2 = 0.0;
	double sumX = 0.0;
	double sumXY = 0.0;
	double sumY = 0.0;

	int validCount = 0;

	for (int i = nStart; i < nEnd; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		double x = static_cast<double>(i);
		double y = static_cast<double>(ArrayHeight[i]);

		sumX2 += x * x;
		sumX += x;
		sumXY += x * y;
		sumY += y;

		++validCount;
	}

	if (validCount < 2)
	{
		return;
	}

	double denominator =
		validCount * sumX2 -
		sumX * sumX;

	if (std::abs(denominator) < 1e-12)
	{
		return;
	}

	m_k =
		static_cast<float>(
			(validCount * sumXY -
				sumX * sumY)
			/
			denominator);

	m_b =
		static_cast<float>(
			(sumX2 * sumY -
				sumX * sumXY)
			/
			denominator);
}

void hnComputeCUT::LSLineFit_New(
	float* ArrayHeight,
	int nStart,
	int nEnd,
	float& m_k,
	float& m_b)
{
	m_k = 0.0f;
	m_b = 0.0f;

	if (ArrayHeight == nullptr)
	{
		return;
	}

	// 整个算法统一采用 [nStart, nEnd)
	int totalCount = nEnd - nStart;

	if (totalCount <= 0)
	{
		return;
	}

	// ==============================
	// 起点附近最多取 5 个有效点
	// ==============================

	float startAvgHeight = 0.0f;
	float startAvgX = 0.0f;
	int startValidCount = 0;

	int startPointCount =
		(std::min)(5, totalCount);

	for (int i = nStart;
		i < nStart + startPointCount;
		++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		++startValidCount;

		startAvgHeight += ArrayHeight[i];
		startAvgX += static_cast<float>(i);
	}

	if (startValidCount == 0)
	{
		return;
	}

	startAvgHeight /= startValidCount;
	startAvgX /= startValidCount;


	// ==============================
	// 终点附近最多取 5 个有效点
	// ==============================

	float endAvgHeight = 0.0f;
	float endAvgX = 0.0f;
	int endValidCount = 0;

	// 注意：这里也是 min，不是 max
	int endPointCount =
		(std::min)(5, totalCount);

	int endStartIndex =
		(std::max)(nStart, nEnd - endPointCount);

	for (int i = endStartIndex;
		i < nEnd;
		++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		++endValidCount;

		endAvgHeight += ArrayHeight[i];
		endAvgX += static_cast<float>(i);
	}

	if (endValidCount == 0)
	{
		return;
	}

	endAvgHeight /= endValidCount;
	endAvgX /= endValidCount;


	// ==============================
	// 两个平均点确定直线
	// ==============================

	float deltaX =
		endAvgX - startAvgX;

	if (std::abs(deltaX) > 1e-6f)
	{
		m_k =
			(endAvgHeight - startAvgHeight)
			/ deltaX;

		m_b =
			startAvgHeight
			- m_k * startAvgX;
	}
	else
	{
		m_k = 0.0f;

		m_b =
			(startAvgHeight + endAvgHeight)
			/ 2.0f;
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

// 通过拉依达法则去除异常值
bool hnComputeCUT::eraseOutliers(
	float* ArrayHeight,
	int nStart,
	int nEnd)
{
	bool isErase = false;

	if (ArrayHeight == nullptr || nEnd <= nStart)
	{
		return false;
	}

	float mean = 0.0f;
	float stdNew = 0.0f;

	// 计算当前数据的平均值和标准差
	mean_std(
		ArrayHeight,
		nStart,
		nEnd,
		mean,
		stdNew);

	// 原算法的异常值阈值保持不变
	float stdThresh =
		3.34f * stdNew + 2.0f;

	for (int i = nStart; i < nEnd; ++i)
	{
		// 无效点不参与异常值处理
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		if (std::abs(ArrayHeight[i] - mean) > stdThresh)
		{
			ArrayHeight[i] = mean;
			isErase = true;
		}
	}

	return isErase;
}

//通过拉依达法则去除异常值
void hnComputeCUT::mean_std(
	float* ArrayHeight,
	int nStart,
	int nEnd,
	float& mean,
	float& std_new)
{
	mean = 0.0f;
	std_new = 0.0f;

	if (ArrayHeight == nullptr || nEnd <= nStart)
	{
		return;
	}

	double sum = 0.0;
	int pointCount = 0;

	// 计算平均值
	for (int i = nStart; i < nEnd; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		++pointCount;
		sum += ArrayHeight[i];
	}

	if (pointCount == 0)
	{
		return;
	}

	mean =
		static_cast<float>(
			sum / pointCount);


	// 计算标准差
	sum = 0.0;

	for (int i = nStart; i < nEnd; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		double diff =
			ArrayHeight[i] - mean;

		sum += diff * diff;
	}

	std_new =
		static_cast<float>(
			std::sqrt(sum / pointCount));
}

//中值滤波
void hnComputeCUT::MidianAverageFileter(
	float* x,
	int ns,
	int ne,
	int flen,
	float* y)
{
	int i = 0;
	int j = 0;
	int hflen = 0;
	int ti = 0;
	int cnt = 0;

	float minval = 10000.0f;
	float maxval = -10000.0f;
	float sum = 0.0f;

	if (x == nullptr || y == nullptr)
	{
		return;
	}

	if (ne <= ns || flen <= 0)
	{
		return;
	}

	if ((ne - ns) > (flen - 1) * 2)
	{
		hflen = (flen - 1) / 2;

		for (i = ns; i < ne; ++i)
		{
			minval = 10000.0f;
			maxval = -10000.0f;
			sum = 0.0f;
			cnt = 0;

			for (j = -hflen; j <= hflen; ++j)
			{
				ti = i + j;

				// 必须先判断索引范围，再访问数组
				if (ti < ns || ti >= ne)
				{
					continue;
				}

				// 跳过无效高度
				if (std::abs(x[ti]) >= fInvalide)
				{
					continue;
				}

				if (minval >= x[ti])
				{
					minval = x[ti];
				}

				if (maxval < x[ti])
				{
					maxval = x[ti];
				}

				sum += x[ti];
				++cnt;
			}

			if (cnt > 2)
			{
				// 去掉窗口中的最大值和最小值，再求平均
				y[i] = (sum - minval - maxval) / (cnt - 2);
			}
			else
			{
				y[i] = x[i];
			}
		}
	}
	else
	{
		// 数据长度太短，不做滤波，直接复制
		for (i = ns; i < ne; ++i)
		{
			y[i] = x[i];
		}
	}
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
void hnComputeCUT::findMaximumPiont(
	float* ArrayHeight,
	int num_start,
	int num_end,
	int* MP_idx,
	float* MP_val,
	int& type_envelope)
{
	MP_val[2] = -10000.0f;

	int meidx =
		(num_start + num_end) / 2;

	// =====================================
	// W2：恢复 2025 的 ±500
	// =====================================

	int tsidx = meidx - 500;
	int teidx = meidx + 500;

	// 当前数据本身不会越界，
	// 但保留安全限制
	tsidx = (std::max)(num_start, tsidx);
	teidx = (std::min)(num_end, teidx);


	for (int i = tsidx;
		i < meidx;
		++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		if (ArrayHeight[i] >= MP_val[2])
		{
			MP_val[2] = ArrayHeight[i];
			MP_idx[2] = i;
		}
	}


	for (int i = meidx;
		i < teidx;
		++i)
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


	Average_near(
		ArrayHeight,
		num_start,
		num_end,
		MP_idx[2],
		MP_val[2]);


	// =====================================
	// W1
	// 2025：
	// max(num_start+200, W2-1100) ~ W2
	// =====================================

	findPit(
		ArrayHeight,
		(std::max)(
			num_start + 200,
			MP_idx[2] - 1100),
		MP_idx[2],
		MP_idx[1],
		MP_val[1],
		0);


	// =====================================
	// W0
	// 2025：必须和 W1 至少隔 50
	// =====================================

	findedge(
		ArrayHeight,
		num_start,
		MP_idx[1] - 50,
		MP_idx[0],
		MP_val[0],
		0);


	// =====================================
	// W3
	// 注意这里必须是 W2 + 1100
	// 不是 W3 + 1100
	// =====================================

	findPit(
		ArrayHeight,
		MP_idx[2],
		(std::min)(
			num_end - 200,
			MP_idx[2] + 1100),
		MP_idx[3],
		MP_val[3],
		1);


	// =====================================
	// W4
	// 2025：必须和 W3 至少隔 50
	// =====================================

	findedge(
		ArrayHeight,
		MP_idx[3] + 50,
		num_end,
		MP_idx[4],
		MP_val[4],
		1);


	// =====================================
	// 包络类型
	// =====================================

	double k =
		(MP_val[4] - MP_val[0])
		/
		(MP_idx[4] - MP_idx[0]);

	double b =
		MP_val[0]
		- k * MP_idx[0];

	double dis =
		MP_val[2]
		-
		(k * MP_idx[2] + b);


	if (dis <= 0)
	{
		type_envelope = 0;
	}
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
void hnComputeCUT::calculateRD(
	float* ArrayHeight,
	int* MP_idx,
	float* MP_val,
	int MPleft_idx,
	int MPright_idx,
	int MPpoint_idx,
	float& RD)
{
	// ==========================================
	// 恢复2025算法：
	// 坑点距离端点太近时使用特殊计算
	// ==========================================

	bool nearEdge =
		(MPpoint_idx == 1 &&
			std::abs(
				MP_idx[MPleft_idx] -
				MP_idx[MPpoint_idx]) < 200)
		||
		(MPpoint_idx == 3 &&
			std::abs(
				MP_idx[MPright_idx] -
				MP_idx[MPpoint_idx]) < 200);


	if (nearEdge)
	{
		float dis1 =
			std::abs(
				MP_val[MPleft_idx] -
				MP_val[MPpoint_idx]);

		float dis2 =
			std::abs(
				MP_val[MPpoint_idx] -
				MP_val[MPright_idx]);

		float dis3 =
			std::abs(
				MP_val[MPleft_idx] -
				MP_val[MPright_idx]);


		RD =
			(std::max)(
				dis1,
				(std::max)(
					dis2,
					dis3));

		return;
	}
	// ==========================================
	// 正常包络线算法
	// ==========================================

	if (MP_idx[MPleft_idx] ==
		MP_idx[MPright_idx])
	{
		RD = 0.0f;
		return;
	}


	float k =
		(MP_val[MPleft_idx] -
			MP_val[MPright_idx])
		/
		(MP_idx[MPleft_idx] -
			MP_idx[MPright_idx]);

	float b =
		MP_val[MPleft_idx]
		-
		k * MP_idx[MPleft_idx];


	float tempRD = 0.0f;

	for (int i =
		MP_idx[MPpoint_idx - 1];
		i <=
		MP_idx[MPpoint_idx + 1];
		++i)
	{
		if (std::abs(ArrayHeight[i]) >=
			fInvalide)
		{
			continue;
		}

		tempRD =
			k * i +
			b -
			ArrayHeight[i];

		if (tempRD > RD)
		{
			RD = tempRD;
		}
	}
}

void hnComputeCUT::Average_near(
	float* ArrayHeight,
	int sidx,
	int eidx,
	int idx_center,
	float& value_average)
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

	for (int i = idx_left; i <= idx_right; ++i)
	{
		if (std::abs(ArrayHeight[i]) >= fInvalide)
		{
			continue;
		}

		++pointCount;
		value_average += ArrayHeight[i];
	}

	if (pointCount > 0)
	{
		value_average /= pointCount;
	}
	else
	{
		// 附近没有任何有效点
		// 保持为无效状态更合理
		value_average = fInvalide;
	}
}

float hnComputeCUT::GetRutVal(MyQtCommon::MyPoint* Pt, int lines, int linee, float thresh)
{
	std::vector<MyQtCommon::MyPoint>  MiPt;

	float maxrut = 0.0f, tmprut = 0.0f;

	GetMiPoint(Pt, lines, linee - 1, MiPt, thresh);
	std::sort(MiPt.begin(), MiPt.end(), [=](const MyQtCommon::MyPoint& x, const MyQtCommon::MyPoint& y)
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

	std::vector<MyQtCommon::MyPoint> tmpMiPt;
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

	std::sort(tmpMiPt.begin(), tmpMiPt.end(), [=](const MyQtCommon::MyPoint& x, const MyQtCommon::MyPoint& y)
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
