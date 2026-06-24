#include "RoadPosCalculator.h"
#include <cmath>

bool RoadPosCalculator::calcLatToPicCenter(double dCurLon, double dCurLat, double dCurHeight, double dLastLon, double dLastLat, double dLastHeight, double dOffsetX, double dOffsetY, double dOffsetZ, double& returnLon, double& returnLat, double& returnHeight, bool bInverse /*=false*/)
{
	// 转换高斯三度带投影;
	double dCurEast, dCurNorth;
	dCurEast = dCurNorth = 0.0;
	int iNo = (int)(dCurLon / 3.0);
	double dL0 = iNo * 3.0 * PI_M / 180.0;
	Gauss_Btox(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dCurLat * PI_M / 180.0, dCurLon * PI_M / 180.0, dL0, dCurNorth, dCurEast);

	// 转平面坐标;
	double dLastEast, dLastNorth;
	dLastEast = dLastNorth = 0.0;
	Gauss_Btox(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dLastLat * PI_M / 180.0, dLastLon * PI_M / 180.0, dL0, dLastNorth, dLastEast);

	// 由两点构成三维线;
	double dNormalNorth, dNormalEast, dNormalH;
	if (bInverse)
	{
		// 计算第一张照片，dLastLon实际为第二张照片的经纬度信息;
		dNormalEast = dCurEast - dLastEast;
		dNormalNorth = dCurNorth - dLastNorth;
		dNormalH = dCurHeight - dLastHeight;
	}
	else
	{
		// 其他张照片，last为当前上一帧;
		dNormalEast = dLastEast - dCurEast;
		dNormalNorth = dLastNorth - dCurNorth;
		dNormalH = dLastHeight - dCurHeight;
	}

	// 向量单位化;
	double length = dNormalEast * dNormalEast + dNormalNorth * dNormalNorth + dNormalH * dNormalH;
	if (length == 0)
	{
		return false;
	}
	length = 1.0 / sqrt(length);
	dNormalEast = dNormalEast * length;
	dNormalNorth = dNormalNorth * length;
	dNormalH = dNormalH * length;

	// 得到图片中心点坐标;
	double dTmpEast, dTmpNorth, dTmpH;
	dTmpEast = dCurEast + dOffsetY * dNormalEast;
	dTmpNorth = dCurNorth + dOffsetY * dNormalNorth;
	dTmpH = dCurHeight + dOffsetY * dNormalH;
	dTmpH = dTmpH - dOffsetZ;

	// 当存在x方向偏移量时，需要将向量旋转;
	if (dOffsetX != 0.0)
	{
		double dTmpEast_addX, dTmpNorth_addX, dTmpH_addX;
		dTmpEast_addX = dOffsetX * dNormalEast;
		dTmpNorth_addX = dOffsetX * dNormalNorth;
		dTmpH_addX = dTmpH + dOffsetX * dNormalH;

		// 旋转90度;
		double dRotateAngle = 90.0 * PI_M / 180.0;
		double cs = cos(dRotateAngle);
		double sn = sin(dRotateAngle);
		double dtmpX, dtmpY, dtmpX2, dtmpY2;
		dtmpX2 = dTmpEast_addX * cs - dTmpNorth_addX * sn;
		dtmpY2 = dTmpEast_addX * sn + dTmpNorth_addX * cs;
		dtmpX2 += dTmpEast;
		dtmpY2 += dTmpNorth;

		dTmpEast = dtmpX2;
		dTmpNorth = dtmpY2;
		dTmpH = dTmpH_addX;


	}

	// 转换经纬度传出;
	Gauss_xtoB(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dTmpNorth, dTmpEast, dL0, returnLat, returnLon);
	returnLat = returnLat * 180.0 / PI_M;
	returnLon = returnLon * 180.0 / PI_M;

	//平面坐标
	//returnLat = dTmpNorth;
	//returnLon = dTmpEast;

	returnHeight = dTmpH;
	return true;
}

bool RoadPosCalculator::calcLatToPicPos(bool showGps, double dCurPicLon, double dCurPicLat, double dCurPicH, double dLastPicLon, double dLastPicLat, double dLastPicH, int picX, int picY, int picWidth, int picHeight, double& returnLon, double& returnLat, double& returnHeight, int equip /*= 0*/, bool bInverse/*=false*/, double dWidth/*=2.0*/, double dHeight/*=3.75*/)
{
	// 转换高斯三度带投影;
	double dCurEast, dCurNorth;
	dCurEast = dCurNorth = 0.0;
	int iNo = (int)(dCurPicLon / 3.0);
	double dL0 = iNo * 3.0 * PI_M / 180.0;
	Gauss_Btox(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dCurPicLat * PI_M / 180.0, dCurPicLon * PI_M / 180.0, dL0, dCurNorth, dCurEast);

	// 转平面坐标;
	double dLastEast, dLastNorth;
	dLastEast = dLastNorth = 0.0;
	Gauss_Btox(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dLastPicLat * PI_M / 180.0, dLastPicLon * PI_M / 180.0, dL0, dLastNorth, dLastEast);

	// 由两点构成三维线;
	double dNormalNorth, dNormalEast, dNormalH;
	if (bInverse)
	{
		// 计算第一张照片，dLastLon实际为第二张照片的经纬度信息;
		dNormalEast = dCurEast - dLastEast;
		dNormalNorth = dCurNorth - dLastNorth;
		dNormalH = dCurPicH - dLastPicH;
	}
	else
	{
		// 其他张照片，last为当前上一帧;
		dNormalEast = dLastEast - dCurEast;
		dNormalNorth = dLastNorth - dCurNorth;
		dNormalH = dLastPicH - dCurPicH;
	}

	// 向量单位化;
	double length = dNormalEast * dNormalEast + dNormalNorth * dNormalNorth + dNormalH * dNormalH;
	if (length == 0)
	{
		return false;
	}
	length = 1.0 / sqrt(length);
	dNormalEast = dNormalEast * length;
	dNormalNorth = dNormalNorth * length;
	dNormalH = dNormalH * length;

	// 根据像素比例换算;
	float dHeightOffset = 1.0 * (picHeight / 2.0 - picY) * dHeight / picHeight;
	float dWidthOffset = 1.0 * (picWidth / 2.0 - picX) * dWidth / picWidth;

	// 得到width方向上的坐标;
	double dTmpEast, dTmpNorth, dTmpH;
	dTmpEast = dCurEast + dHeightOffset * dNormalEast;
	dTmpNorth = dCurNorth + dHeightOffset * dNormalNorth;
	dTmpH = dCurPicH + dHeightOffset * dNormalH;

	double dRotateAngle = 90.0;
	if (picX == picWidth / 2)
	{
		// 转换经纬度传出;
		Gauss_xtoB(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dTmpNorth, dTmpEast, dL0, returnLat, returnLon);
		returnLat = returnLat * 180.0 / PI_M;
		returnLon = returnLon * 180.0 / PI_M;
		returnHeight = dTmpH;
		return true;
	}
	else if (picX < picWidth / 2)
	{
		dRotateAngle = 90.0;
	}
	else
	{
		dRotateAngle = -90.0;
	}

	// 旋转90度;
	double dTmpEast1, dTmpNorth1, dTmpH1;
	dTmpEast1 = dTmpEast + fabs(dWidthOffset) * dNormalEast;
	dTmpNorth1 = dTmpNorth + fabs(dWidthOffset) * dNormalNorth;
	dTmpH1 = dTmpH + fabs(dWidthOffset) * dNormalH;

	dRotateAngle = dRotateAngle * PI_M / 180.0;
	double cs = cos(dRotateAngle);
	double sn = sin(dRotateAngle);

	dTmpEast1 -= dTmpEast;
	dTmpNorth1 -= dTmpNorth;

	double dRotatedE = dTmpEast1 * cs - dTmpNorth1 * sn;
	double dRotatedN = dTmpEast1 * sn + dTmpNorth1 * cs;
	dRotatedE += dTmpEast;
	dRotatedN += dTmpNorth;

	// 转换经纬度传出;


	if (showGps)
	{
		Gauss_xtoB(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dRotatedN, dRotatedE, dL0, returnLat, returnLon);
		returnLat = returnLat * 180.0 / PI_M;
		returnLon = returnLon * 180.0 / PI_M;
		returnHeight = dTmpH;
	}
	else
	{
		Gauss_xtoB(EARTH_WGS84_EA, EARTH_WGS84_EF, 0.0, 0.0, dRotatedN, dRotatedE, dL0, returnLat, returnLon);
		returnLat = returnLat * 180.0 / PI_M;
		returnLon = returnLon * 180.0 / PI_M;
		returnHeight = dTmpH;
		//平面坐标
		returnLat = dRotatedN;
		returnLon = dRotatedE + 500000;
		returnHeight = dTmpH;
	} 
	//std::string strPath = "J:\\ROAD_DATA\\gps\\gps数据\\gps数据\\_动态数据_上行_02_湖北省_武汉市_汉阳区_20240306_154533\\检核点-001.txt";
	//FILE* ptrF = fopen(strPath.data(), "at+");
	//fprintf_s(ptrF, "%.10lf,%.10lf,%.3lf\n",
	//	returnLon, returnLat, returnHeight);
	//fprintf_s(ptrF, "%.4lf,%.4lf,%.4lf\n",
	//	dRotatedE, dRotatedN, returnHeight);
	//fclose(ptrF);

	return true;
}

void RoadPosCalculator::Gauss_Btox(double a, double f, double h, double Bm, double B, double L, double L0, double& m_x, double& m_y)
{
	double b, e1, e2, l, t, m0, n2, N;
	double A0, A2, A4, A6, A8, X0;
	double da, dN, dB, dH;

	b = a * (1 - 1 / f);
	e2 = (a * a - b * b) / (a * a);

	// 更换计算方法;
	da = h * (1.0) / sqrt(1.0 - e2);

	a += da;
	double W = 1 - e2 * sin(B) * sin(B);
	double M = a * (1 - e2) / sqrt(std::pow(1 - e2 * sin(B) * sin(B), 3));
	double Ni = a / W;
	dB = (e2 * sin(B) * cos(B)) / (M + h) / W;

	dN = h * sqrt((1 - e2 * std::pow(sin(B), 2)) / (1 - e2));
	N = a / sqrt(1 - e2 * std::pow(sin(B), 2));

	//N+=dN;
	B += dB;


	A0 = 1 + 3 / 4.0 * e2 + 45 / 64.0 * std::pow(e2, 2) + 350 / 512.0 * std::pow(e2, 3) + 11025 / 16384.0 * std::pow(e2, 4);
	A2 = (-1 / 2.0) * (3 / 4.0 * e2 + 60 / 64.0 * std::pow(e2, 2) + 525 / 512.0 * std::pow(e2, 3) + 17640 / 16384.0 * std::pow(e2, 4));
	A4 = 1 / 4.0 * (15 / 64.0 * std::pow(e2, 2) + 210 / 512.0 * std::pow(e2, 3) + 8820 / 16384.0 * std::pow(e2, 4));
	A6 = (-1 / 6.0) * (35 / 512.0 * std::pow(e2, 3) + 2520 / 16384.0 * std::pow(e2, 4));
	A8 = (1 / 8.0) * 315 / 16384.0 * std::pow(e2, 4);
	l = L - L0;
	t = std::tan(B);
	m0 = l *std:: cos(B);

	n2 = e2 / (1 - e2) * std::pow(cos(B), 2);

	X0 = a * (1 - e2) * (A0 * B + A2 * sin(2 * B) + A4 * sin(4 * B) + A6 * sin(6 * B) + A8 * sin(8 * B));

	double M0 = a * (1 - e2) * (A0 * Bm + A2 * sin(2 * Bm) + A4 * sin(4 * Bm) + A6 * sin(6 * Bm) + A8 * sin(8 * Bm));

	m_x = X0 - M0 + 0.5 * N * t * std::pow(m0, 2.0) + 1.0 / 24.0 * (5 - std::pow(t, 2) + 9 * n2 + 4 * std::pow(n2, 2.0)) * N * t * std::pow(m0, 4.0)
		+ 1.0 * (61.0 - 58.0 * std::pow(t, 2.0) + std::pow(t, 4.0)) * N * t * std::pow(m0, 6.0) / 720.0;
	m_y = N * m0 + 1.0 / 6.0 * (1.0 - std::pow(t, 2.0) + n2) * N * std::pow(m0, 3.0)
		+ 1.0 / 120.0 * (5.0 - 18 * std::pow(t, 2.0) + std::pow(t, 4.0) + 14.0 * n2 - 58.0 * n2 * std::pow(t, 2.0)) * N * std::pow(m0, 5.0);
}

void RoadPosCalculator::Gauss_xtoB(double a, double f, double h, double Bm, double x, double y, double L0, double& m_B, double& m_L)
{
	double da, e2, ep2;
	e2 = 1 - std::pow((1 - (1.0 / f)), 2);
	ep2 = e2 / (1 - e2);
	double v1, p1, w1, e1, u1;
	double T1, C1, D;

	da = h * (1. - e2 * std::pow(sin(Bm), 2)) / sqrt(1. - e2);
	a += da;

	double M, M0, M1, M2, M3, M4;
	M1 = 1 - e2 / 4 - std::pow(e2, 2) * 3 / 64 - std::pow(e2, 3) * 5 / 256;
	M2 = e2 * 3 / 8 + std::pow(e2, 2) * 3 / 32 + std::pow(e2, 3) * 45 / 1024;
	M3 = std::pow(e2, 2) * 15 / 256 + std::pow(e2, 3) * 45 / 1024;
	M4 = std::pow(e2, 3) * 35 / 3072;

	M0 = a * (M1 * Bm - M2 * sin(2 * Bm) + M3 * sin(4 * Bm) - M4 * sin(6 * Bm));

	M = M0 + x;
	u1 = M / (a * (1 - e2 / 4 - 3 * std::pow(e2, 2) / 64 - 5 * std::pow(e2, 3) / 256));
	e1 = (1 - sqrt(1 - e2)) / (1 + sqrt(1 - e2));

	w1 = u1 + (e1 * 3 / 2 - std::pow(e1, 3) * 27 / 32) * sin(2 * u1);
	w1 += (std::pow(e1, 2) * 21 / 16 - std::pow(e1, 4) * 55 / 32) * sin(4 * u1);
	w1 += (std::pow(e1, 3) * 151 / 96) * sin(6 * u1);
	w1 += (std::pow(e1, 4) * 1097 / 512) * sin(8 * u1);

	p1 = a * (1 - e2) / std::pow(1 - e2 * std::pow(sin(w1), 2), 3.0 / 2.0);
	v1 = a / sqrt(1 - e2 * std::pow(sin(w1), 2));
	T1 = std::pow(std::tan(w1), 2);
	C1 = ep2 * std::pow(cos(w1), 2);
	D = y / v1;

	m_B = std::pow(D, 2) / 2 - (5 + 3 * T1 + 10 * C1 - 4 * std::pow(C1, 2) - 9 * ep2) * std::pow(D, 4) / 24
		+ (61 + 90 * T1 + 298 * C1 + 45 * std::pow(T1, 2) - 252 * ep2 - 3 * std::pow(C1, 2)) * std::pow(D, 6) / 720;
	m_B = w1 - v1 * tan(w1) / p1 * m_B;

	m_L = D - (1 + 2 * T1 + C1) * std::pow(D, 3) / 6
		+ (5 - 2 * C1 + 28 * T1 - 3 * std::pow(C1, 2) + 8 * ep2 + 24 * std::pow(T1, 2)) * std::pow(D, 5) / 120;
	m_L = L0 + m_L / cos(w1);
}
