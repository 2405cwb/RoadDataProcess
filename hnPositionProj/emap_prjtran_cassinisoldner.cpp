//---------------------------------------------------------------------------
//	 emap_prjtrans_cassinisoldner.c
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "emap_prjtran_cassinisoldner.h"

//
double QuarterPi = PI / 4.0;
double HalfPi = PI / 2.0;

double MajorAxis;
double ESq;
double E;
double EHalf;

//
double NaturalOrigin_Lat;
double NaturalOrigin_Lon;
double MLineCoef1;
double MLineCoef2;
double MLineCoef3;
double MLineCoef4;
double MLineCoef1Major;
double LatLineCoef1;
double LatLineCoef2;
double LatLineCoef3;
double LatLineCoef4;
double MOrigin;
double MOriginYOffset;
double OneMinusESq;
double ESecSq;
double OneMinusESqSqrt;


double GetM(double B)
{

	double M = MajorAxis * ((1 - ESq / 4 - 3 * ESq * ESq / 64 - 5 * ESq * ESq * ESq / 256) * B
		- (3 * ESq / 8 + 3 * ESq * ESq / 32 + 45 * ESq * ESq * ESq / 1024) *sin(2 * B)
		+ (15 * ESq * ESq / 256 + 45 * ESq * ESq * ESq / 1024) *sin(4 * B)
		- (35 * ESq * ESq * ESq / 3072) *sin(6 * B));
	return M;
}

void CassiniSoldner(double Origin_Lat, double Origin_Lon, double inF, double a)
{
	double E1 = 1.0 - pow((1.0 - (1.0 / inF)), 2);//鲍用的公式
	double b = a * ((inF - 1.0) / inF);
	double E2 = (a * a - b * b) / (b * b);
	//E1 = 0.006785146;
	//
	ESq = E1;
	ESecSq = E2;
	E = ESq * ESq;
	EHalf = E / 2.0;
	MajorAxis = a;
	//
	NaturalOrigin_Lat = Origin_Lat;
	NaturalOrigin_Lon = Origin_Lon;

	double e4 = ESq * ESq;
	double e6 = e4 * ESq;


	MLineCoef1 = 1.0 - (ESq / 4.0) - (3.0 * e4 / 64.0) - (5.0 * e6 / 256.0);
	MLineCoef2 = (3.0* ESq / 8.0) - (3.0 * e4 / 32.0) - (45.0 * e6 / 1024.0);
	MLineCoef3 = (15.0 * e4 / 256.0) + (45.0 * e6 / 1024.0);
	MLineCoef4 = (35.0* e6 / 3072.0);//EPSG是3072

	MLineCoef1Major = MLineCoef1 * MajorAxis;


	MOrigin = GetM(Origin_Lat);

	//
	//MOriginYOffset = MOrigin - falseProjectedOffset.Y1;
	OneMinusESq = (1.0 - ESq);

	OneMinusESqSqrt = sqrt(OneMinusESq);
	double ep = (1.0 - OneMinusESqSqrt) / (1.0 + OneMinusESqSqrt);
	double ep2 = ep * ep;
	double ep3 = ep2 * ep;
	double ep4 = ep3 * ep;
	LatLineCoef1 = (3.0 / 2.0 * ep) - (27.0 / 32.0 * ep3);
	LatLineCoef2 = (21.0 / 16.0 * ep2) - (55.0 / 32.0 * ep4);
	LatLineCoef3 = (151.0 / 96.0 * ep3) - (1097.0 / 512.0 * ep4);
	LatLineCoef4 = (1097 / 512 * ep4);
}

//
void TransformValue(double B, double L,  double *North,  double *East)
{
	double one6th = 0.16666666666666666666;      //C1
	double one120th = 0.00833333333333333333;    //C2
	double one24th = 0.04166666666666666666;     //C3

	//
	double v = sin(B);
	v = MajorAxis / sqrt(1.0 - (ESq * v * v));
	double c = cos(B);
	double a = (L - NaturalOrigin_Lon) * c;
	c = ESecSq * c * c;
	double tanLat = tan(B);
	double t = tanLat * tanLat;
	double a2 = a * a;
	*East = v * a * (1.0 - t * a2 * (one6th - (8.0 - t + 8.0 * c) * a2 * one120th));

	double M = GetM(B);
	*North = M- MOrigin+ v * tanLat * a2 * (0.5 + (5.0 - t + 6.0 * c) * a2 * one24th);
}
void ReverseValue(double North, double East,  double *B, double *L)
{

	double one24th = 0.04166666666666666666;     //C3
	//
	double e4 = ESq * ESq;
	double e6 = e4 * ESq;
	//
	double M1 = MOrigin + North;
	double u1 = M1 / (MajorAxis * (1.0 - ESq / 4.0 - 3.0 * e4 / 64.0 - 5.0 * e6 / 256.0));
	double e1 = (1.0 - OneMinusESqSqrt) / (1.0 + OneMinusESqSqrt);

	double B1 = u1 + LatLineCoef1 *sin(2.0 * u1)
		+ LatLineCoef2 *sin(4.0 * u1)
		+ LatLineCoef3 *sin(6.0 * u1)
		+ LatLineCoef4 *sin(8.0 * u1);
	double v1 = sin(B1);
	v1 = MajorAxis / sqrt(1.0 - (ESq * v1 * v1));

	double p1 = sin(B1);
	p1 = MajorAxis * (1.0 - ESq) / pow(1.0 - (ESq * p1 * p1), 1.5);
	double T1 = tan(B1) * tan(B1);
	//double D = East / v1;
	double lesq = sin(B1);
	lesq = 1.0 - (lesq * lesq * ESq);

	double D = East * sqrt(lesq) / MajorAxis;
	double D2 = D * D;
	double s = (1.0 + (3.0 * T1)) * D2;
	//B1 = B1 - (v1 * tan(B1) / p1) * (D2 * (1.0 / 2.0 - s / 24.0));
	*B = B1 - (v1 * tan(B1) / p1) * D2 * (0.5 - s * one24th);
	*L = NaturalOrigin_Lon + (D - T1 * D2 * D * (1.0 / 3.0 + s / 15.0)) / cos(B1);
	//
}


