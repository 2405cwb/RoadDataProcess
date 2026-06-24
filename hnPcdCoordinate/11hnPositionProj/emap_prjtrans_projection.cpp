//---------------------------------------------------------------------------
//	 emap_prjtrans_projection.c
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "emap_prjtrans_api.h"
#include "emap_prjtran_cassinisoldner.h"

int SIGN(double a)
{
	int result = 0;
	if (a > 0)
		result = 1;
	else if (a < 0)
		result = -1;

	return result;
}

double arcsin(double a)
{
	return asin(a);
}
double arctan(double a)
{
	return atan(a);
}
double ln(double a)
{
	return log(a);
}

//求非线性方程一个实根的埃特金迭代法 
double datknf2(double a0, double a1, double x)
{
	double y = PI / 2 - 2 * atan(a0 * pow((1 - a1 * sin(x)) / (1 + a1 * sin(x)), a1 / 2));
	return y;
}

int DatKn2( double &x, double eps, int js, double a0, double a1)
{
	int flag, l;
	double u, v, x0;

	//extern double datknf2(double a0,double a1,double x);
	l = 0; x0 = x; flag = 0;
	while ((flag == 0) && (l != js))
	{
		l = l + 1;
		u = datknf2(a0, a1, x0); v = datknf2(a0, a1, u);
		if (fabs(u - v) < eps) { x0 = v; flag = 1; }
		else x0 = v - (v - u) * (v - u) / (v - 2.0 * u + x0);
	}
	x = x0; l = js - l;
	return (l);
}

double ddhrtf(double a0, double a1, double x)
{
	double z;
	z = tan(PI / 4 + x / 2) * pow((1 - a0 * sin(x)) / (1 + a0 * sin(x)), a0 / 2) - a1;
	return z;
}

//求非线性方程实根的对分法 
int Ddhrt(double a0, double a1, double a, double b, double h, double eps,
		  double x[], int m)
{
	//extern double ddhrtf(double a0,double a1,double x);
	int n, js;
	double z, y, z1, y1, z0, y0;
	n = 0; z = a; y = ddhrtf(a0, a1, z);
	while ((z <= b + h / 2.0) && (n != m))
	{
		if (fabs(y) < eps)
		{
			n = n + 1; x[n - 1] = z;
			z = z + h / 2.0; y = ddhrtf(a0, a1, z);
		}
		else
		{
			z1 = z + h; y1 = ddhrtf(a0, a1, z1);
			if (fabs(y1) < eps)
			{
				n = n + 1; x[n - 1] = z1;
				z = z1 + h / 2.0; y = ddhrtf(a0, a1, z);
			}
			else if (y * y1 > 0.0)
			{ y = y1; z = z1; }
			else
			{
				js = 0;
				while (js == 0)
				{
					if (fabs(z1 - z) < eps)
					{
						n = n + 1; x[n - 1] = (z1 + z) / 2.0;
						z = z1 + h / 2.0; y = ddhrtf(a0, a1, z);
						js = 1;
					}
					else
					{
						z0 = (z1 + z) / 2.0; y0 = ddhrtf(a0, a1, z0);
						if (fabs(y0) < eps)
						{
							x[n] = z0; n = n + 1; js = 1;
							z = z0 + h / 2.0; y = ddhrtf(a0, a1, z);
						}
						else if ((y * y0) < 0.0)
						{ z1 = z0; y1 = y0; }
						else { z = z0; y = y0; }
					}
				}
			}
		}
	}
	return (n);
}

//将经纬度通过高斯投影成为平面坐标 (弧度)
void Gauss_Btox(double a,double f,double h,double Bm, double B, double L,double L0,double &m_x,double &m_y)
{
	double b,e1,e2,l,t,m0,n2,N;
	double A0,A2,A4,A6,A8,X0;
	double da,dN,dB,dH;

	b=a*(1-1/f);
	//	e1=(a*a-b*b)/(a*a);
	e2=(a*a-b*b)/(a*a);
	e2=1-pow((1-(1.0/f)),2);

	double B0=0;
	//B0=30.0*PI/180;
	da =h * (1. - e2*pow(sin(B),2)) / sqrt(1. - e2);

	a+=da;
	double W = 1 - e2 * sin(B) *sin(B);
	double M = a * (1 - e2) / sqrt(pow(1 - e2 * sin(B) * sin(B), 3));
	double Ni = a / W;
	dB = (e2 * sin(B) * cos(B)) / (M + h) / W;

	dN=h*sqrt((1-e2*pow(sin(B),2))/(1-e2));
	N=a/sqrt(1-e2*pow(sin(B),2));

	//N+=dN;
	B+=dB;


	A0=1+3/4.0*e2+45/64.0*pow(e2,2)+350/512.0*pow(e2,3)+11025/16384.0*pow(e2,4);
	A2=(-1/2.0)*(3/4.0*e2+60/64.0*pow(e2,2)+525/512.0*pow(e2,3)+17640/16384.0*pow(e2,4));
	A4=1/4.0*(15/64.0*pow(e2,2)+210/512.0*pow(e2,3)+8820/16384.0*pow(e2,4));
	A6=(-1/6.0)*(35/512.0*pow(e2,3)+2520/16384.0*pow(e2,4));
	A8=(1/8.0)*315/16384.0*pow(e2,4);
	l=L-L0;
	t=tan(B);
	m0=l*cos(B);

	n2=e2/(1-e2)*pow(cos(B),2);

	X0=a*(1-e2)*(A0*B+A2*sin(2*B)+A4*sin(4*B)+A6*sin(6*B)+A8*sin(8*B));
	/*
	double C0,C1,C2,C3,Bp;
	C0=A0*a*(1-e2);
	C1=2*(A2*a*(1-e2))+4*A4*a*(1-e2)+6*A6*a*(1-e2);
	C2=-1.0*(8*A4*a*(1-e2)+32*A6*a*(1-e2));
	C3=32*A6*a*(1-e2);
	Bp=3/4.0*e2+15/16.0*pow(e2,2)+525/512.0*pow(e2,3)+2205/2048.0*pow(e2,4)+72765/65536.0*pow(e2,5);
	X0=C0*Bp+cos(B)*(C1*sin(B)+C2*pow(sin(B),3)+C3*pow(sin(B),5));
	*/
	double M0=a*(1-e2)*(A0*Bm+A2*sin(2*Bm)+A4*sin(4*Bm)+A6*sin(6*Bm)+A8*sin(8*Bm));

	m_x=X0-M0+0.5*N*t*pow(m0,2.0)+1.0/24.0*(5-pow(t,2)+9*n2+4*pow(n2,2.0))*N*t*pow(m0,4.0)
		+1.0/720.0*(61.0-58.0*pow(t,2.0)+pow(t,4.0))*N*t*pow(m0,6.0);
	m_y=N*m0+1.0/6.0*(1.0-pow(t,2.0)+n2)*N*pow(m0,3.0)
		+1.0/120.0*(5.0-18*pow(t,2.0)+pow(t,4.0)+14.0*n2-58.0*n2*pow(t,2.0))*N*pow(m0,5.0);
}

//将平面坐标通过高斯投影成为经纬度
void Gauss_xtoB(double a,double f, double h,double Bm,double x, double y,double L0,double &m_B,double &m_L)//弧度
{
	/*
	double e2,l,n2,t,N,V;
	double A0,B0,Bf,K0,K2,K4,K6;
	double da;

	e2=1-pow((1-(1.0/f)),2);

	da =h * (1. - e2*pow(sin(Bm),2)) / sqrt(1. - e2);
	a+=da;

	K0=0.5*(3/4.0*e2+45/64.0*pow(e2,2)+350/512.0*pow(e2,3)+11025/16384.0*pow(e2,4));  
	K2=(-1/3.0)*(63/64.0*pow(e2,2)+1108/512.0*pow(e2,3)+58239/16384.0*pow(e2,4));
	K4=(1/3.0)*(604/512.0*pow(e2,3)+68484/16384.0*pow(e2,4));
	K6=(-1/3.0)*(26328/16384.0)*pow(e2,4);

	M0=a*(M1*Bm-M2*sin(2*Bm)+M3*sin(4*Bm)-M4*sin(6*Bm));

	A0=1+3/4.0*e2+45/64.0*pow(e2,2)+350/512.0*pow(e2,3)+11025/16384.0*pow(e2,4);
	B0=x/(A0*a*(1-e2));
	Bf=B0+sin(2*B0)*(K0+pow(sin(B0),2)*(K2+pow(sin(B0),2)*(K4+K6*pow(sin(B0),2))));
	t=tan(Bf);
	n2=e2/(1-e2)*pow(cos(Bf),2);
	N=a/sqrt(1-e2*pow(sin(Bf),2));
	V=sqrt(1+n2);
	m_B=Bf-0.5*pow(V,2)*t*pow(y/N,2)
	+1/24.0*(5+3*pow(t,2)+n2-9*pow(n2,2))*pow(V,2)*t*pow(y/N,4)
	-1/720.0*(61+90*pow(t,2)+45*pow(n2,2))*pow(V,2)*t*pow(y/N,6);
	l=1/cos(Bf)*y/N-1/6.0*(1+2*pow(t,2)+n2)*(1/cos(Bf))*pow(y/N,3)
	+1/120.0*(5+28*pow(t,2)+24*pow(t,4)+6*n2+8*n2*pow(t,2))*(1/cos(Bf))*pow(y/N,5);
	m_L=l+L0;
	*/

	double da,e2,ep2;
	e2=1-pow((1-(1.0/f)),2);
	ep2=e2/(1-e2);
	double v1,p1,w1,e1,u1;
	double T1,C1,D;

	da =h * (1. - e2*pow(sin(Bm),2)) / sqrt(1. - e2);
	a+=da;

	double M,M0,M1,M2,M3,M4;
	M1=1-e2/4-pow(e2,2)*3/64-pow(e2,3)*5/256;
	M2=e2*3/8+pow(e2,2)*3/32+pow(e2,3)*45/1024;
	M3=pow(e2,2)*15/256+pow(e2,3)*45/1024;
	M4=pow(e2,3)*35/3072;

	M0=a*(M1*Bm-M2*sin(2*Bm)+M3*sin(4*Bm)-M4*sin(6*Bm));

	M=M0+x;
	u1=M/(a*(1-e2/4-3*pow(e2,2)/64-5*pow(e2,3)/256));
	e1=(1-sqrt(1-e2))/(1+sqrt(1-e2));

	w1=u1+(e1*3/2-pow(e1,3)*27/32)*sin(2*u1);
	w1+=(pow(e1,2)*21/16-pow(e1,4)*55/32)*sin(4*u1);
	w1+=(pow(e1,3)*151/96)*sin(6*u1);
	w1+=(pow(e1,4)*1097/512)*sin(8*u1);

	p1=a*(1-e2)/pow(1-e2*pow(sin(w1),2),3.0/2.0);
	v1=a/sqrt(1-e2*pow(sin(w1),2));
	T1=pow(tan(w1),2);
	C1=ep2*pow(cos(w1),2);
	D=y/v1;

	m_B=pow(D,2)/2-(5+3*T1+10*C1-4*pow(C1,2)-9*ep2)*pow(D,4)/24
		+(61+90*T1+298*C1+45*pow(T1,2)-252*ep2-3*pow(C1,2))*pow(D,6)/720;
	m_B=w1-v1*tan(w1)/p1*m_B;

	m_L=D-(1+2*T1+C1)*pow(D,3)/6
		+(5-2*C1+28*T1-3*pow(C1,2)+8*ep2+24*pow(T1,2))*pow(D,5)/120;
	m_L=L0+m_L/cos(w1);

}


/// <summary>莫卡托投影正算
/// 莫卡托投影正算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bc">投影平均纬度(弧度)</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="m_x">平面坐标X(m)</param>
/// <param name="m_y">平面坐标Y(m)</param>

void Mercator_Btox(double a, double f, double h, double Bc,
				   double B, double L, double B0, double L0,
				   double &m_x,  double &m_y)
{
	//原有的

	double de, daa, dq, dq0, C;
	double da, e2;

	e2 = 1 - pow((1 - (1.0 / f)), 2);
	da = h * (1.0 - e2 * pow(sin(Bc), 2)) / sqrt(1.0 - e2);
	a += da;

	de = sqrt(1 - pow((1 - (1.0 / f)), 2));
	daa = a * cos(B0) / sqrt(1.0 - pow(de * sin(B0), 2.0));
	dq0 = tan(PI / 4 + B0 / 2) * pow((1 - de * sin(B0)) / (1 + de * sin(B0)), de / 2);
	dq0 = log10(dq0) / log10(LOGE);
	C = -daa * dq0;
	dq = tan(PI / 4 + B / 2) * pow((1 - de * sin(B)) / (1 + de * sin(B)), de / 2);
	dq = log10(dq) / log10(LOGE);
	if (fabs(B0) < EPS)
		m_x = daa * dq + C;
	else
		m_x = daa * dq;
	m_y = daa * (L - L0);

}

/// <summary>UTM投影正算
/// UTM投影正算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bc">投影平均纬度(弧度)</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="m_x">平面坐标X(m)</param>
/// <param name="m_y">平面坐标Y(m)</param>

void UTM_Btox(double a, double f, double h, double Bc,
			  double B, double L, double L0,
			  double &m_x,  double &m_y)
{

	double e2, ep2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	ep2 = e2 / (1 - e2);
	double C, T, A;
	C = ep2 * pow(cos(B), 2);
	T = pow(tan(B), 2);
	A = (L - L0) * cos(B);
	double M, M0, M1, M2, M3, M4;
	M1 = 1 - e2 / 4 - pow(e2, 2) * 3 / 64 - pow(e2, 3) * 5 / 256;
	M2 = e2 * 3 / 8 + pow(e2, 2) * 3 / 32 + pow(e2, 3) * 45 / 1024;
	M3 = pow(e2, 2) * 15 / 256 + pow(e2, 3) * 45 / 1024;
	M4 = pow(e2, 3) * 35 / 3072;

	M = a * (M1 * B - M2 * sin(2 * B) + M3 * sin(4 * B) - M4 * sin(6 * B));
	M0 = a * (M1 * Bc - M2 * sin(2 * Bc) + M3 * sin(4 * Bc) - M4 * sin(6 * Bc));

	double v;
	v = a / sqrt(1 - e2 * pow(sin(B), 2));

	m_y = v * (A + (1 - T + C) * pow(A, 3) / 6 + (5 - 18 * T + pow(T, 2) + 72 * C - 58 * ep2) * pow(A, 5) / 120);
	m_x = pow(A, 2) / 2 +
		(5 - T + 9 * C + 4 * pow(C, 2)) * pow(A, 4) / 24 +
		(61 - 58 * T + pow(T, 2) + 600 * C - 330 * ep2) * pow(A, 6) / 720;
	m_x = M - M0 + v * tan(B) * m_x;

}


/// <summary>UTM南半球投影正算
///UTM南半球投影正算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bc">投影平均纬度(弧度)</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="m_x">平面坐标X(m)</param>
/// <param name="m_y">平面坐标Y(m)</param>
void UTM_South_Btox(double a, double f, double h, double Bc,
					double B, double L, double L0,  double &m_x, double &m_y)
{
	double e2, ep2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	ep2 = e2 / (1 - e2);
	double C, T, A;
	C = ep2 * pow(cos(B), 2);
	T = pow(tan(B), 2);
	A = (L - L0) * cos(B);
	double M, M0, M1, M2, M3, M4;
	M1 = 1 - e2 / 4 - pow(e2, 2) * 3 / 64 - pow(e2, 3) * 5 / 256;
	M2 = e2 * 3 / 8 + pow(e2, 2) * 3 / 32 + pow(e2, 3) * 45 / 1024;
	M3 = pow(e2, 2) * 15 / 256 + pow(e2, 3) * 45 / 1024;
	M4 = pow(e2, 3) * 35 / 3072;

	M = a * (M1 * B - M2 * sin(2 * B) + M3 * sin(4 * B) - M4 * sin(6 * B));
	M0 = a * (M1 * Bc - M2 * sin(2 * Bc) + M3 * sin(4 * Bc) - M4 * sin(6 * Bc));

	double v;
	v = a / sqrt(1 - e2 * pow(sin(B), 2));

	m_y = -(v * (A + (1 - T + C) * pow(A, 3) / 6 + (5 - 18 * T + pow(T, 2) + 72 * C - 58 * ep2) * pow(A, 5) / 120));
	m_x = pow(A, 2) / 2 +
		(5 - T + 9 * C + 4 * pow(C, 2)) * pow(A, 4) / 24 +
		(61 - 58 * T + pow(T, 2) + 600 * C - 330 * ep2) * pow(A, 6) / 720;
	m_x = -(M - M0 + v * tan(B) * m_x);
}

/// <summary>兰伯托切圆锥投影
/// 兰伯托切圆锥投影
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="k0">系数</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
void Lambert_CC1SP_Btox(double a, double f, double B0, double L0,
						double k0, double B, double L,  double &x, double &y)
{
	double m0, t0, t;
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);
	m0 = cos(B0) / sqrt(1 - e2 * pow(sin(B0), 2));
	t0 = tan(PI / 4 - B0 / 2) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);
	t = tan(PI / 4 - B / 2) / pow((1 - e * sin(B)) / (1 + e * sin(B)), e / 2);

	double n = sin(B0);
	double F = m0 / (n * pow(t0, n));
	double r = a * F * pow(t, n) * k0;
	double r0 = 0;
	if (fabs(B0-PI/2)>0.000000000001)
	{
		r0 =a * F * pow(t0, n) * k0;
	}
	r0 = a * F * pow(t0, n) * k0;

	double sita = n * (L - L0);
	x = r0 - r * cos(sita);
	y = r * sin(sita);
}

/// <summary>兰伯托割圆锥投影
/// 兰伯托割圆锥投影
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="B1">第一纬线(弧度)</param>
/// <param name="B2">第二纬线(弧度)</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>

void Lambert_CC2SP_Btox(double a, double f, double B0, double L0,
						double B1, double B2,
						double B, double L, double &x,  double &y)
{
	double mb1, mb2, tb1, tb2, tb0, t;
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);
	mb1 = cos(B1) / sqrt(1 - e2 * pow(sin(B1), 2));
	mb2 = cos(B2) / sqrt(1 - e2 * pow(sin(B2), 2));
	tb1 = tan(PI / 4 - B1 / 2) / pow((1 - e * sin(B1)) / (1 + e * sin(B1)), e / 2);
	tb2 = tan(PI / 4 - B2 / 2) / pow((1 - e * sin(B2)) / (1 + e * sin(B2)), e / 2);
	tb0 = tan(PI / 4 - B0 / 2) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);
	t = tan(PI / 4 - B / 2) / pow((1 - e * sin(B)) / (1 + e * sin(B)), e / 2);

	double n = log10(mb1 / mb2) / log10(tb1 / tb2);
	double F = mb1 / (n * pow(tb1, n));
	double r = a * F * pow(t, n);
	double r0 = 0;
	if (fabs(B0-PI/2)>0.000000000001)
	{
		r0 =a * F * pow(tb0, n);
	}
	double sita = n * (L - L0);

	x = r0 - r * cos(sita);
	y = r * sin(sita);

}


/// <summary>倾斜赤平投影 
///倾斜赤平投影 
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="k0">系数</param>
/// <param name="B">纬度(弧度)</param>
/// <param name="L">经度(弧度)</param>
/// <param name="m_x">平面坐标X(m)</param>
/// <param name="m_y">平面坐标Y(m)</param>

void Stereo_Oblique_Btox(double a, double f, double B0, double L0,
						 double k0, double B, double L,  double &m_x, double &m_y)
{
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);

	double R, n, C, p0, v0;
	p0 = a * (1 - e2) / pow(1 - e2 * pow(sin(B0), 2), 3.0 / 2.0);
	v0 = a / sqrt(1 - e2 * pow(sin(B0), 2));
	R = sqrt(p0 * v0);
	n = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));

	double S1, S2, w1, w2, sinX0;
	S1 = (1 + sin(B0)) / (1 - sin(B0));
	S2 = (1 - e * sin(B0)) / (1 + e * sin(B0));
	w1 = pow(S1 * pow(S2, e), n);

	sinX0 = (w1 - 1) / (w1 + 1);
	C = (n + sin(B0)) * (1 - sinX0) / ((n - sin(B0)) * (1 + sinX0));
	w2 = C * w1;

	double X0, A0;
	X0 = asin((w2 - 1) / (w2 + 1));
	A0 = L0;

	double SS1, SS2, w;
	SS1 = (1 + sin(B)) / (1 - sin(B));
	SS2 = (1 - e * sin(B)) / (1 + e * sin(B));
	w = C * pow(SS1 * pow(SS2, e), n);
	double X, A, M;
	A = n * (L - A0) + A0;
	X = asin((w - 1) / (w + 1));
	M = 1 + sin(X) * sin(X0) + cos(X) * cos(X0) * cos(A - A0);

	m_x = 2 * R * (sin(X) * cos(X0) - cos(X) * sin(X0) * cos(A - A0)) / M;
	m_y = 2 * R * cos(X) * sin(A - A0) / M;

}





/// <summary>墨卡托投影反算
/// 墨卡托投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bc">投影平均纬度(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央经线(弧度)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>

void Mercator_xtoB(double a, double f, double h, double Bc,
				   double x, double y, double B0, double L0,
				   double &m_B,  double &m_L)
{

	//下面的算法也可以

	double de, daa, dq, dq0, C;
	double  B[2];// = new double[2];
	double da, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	da = h * (1.0 - e2 * pow(sin(Bc), 2)) / sqrt(1.0 - e2);
	a += da;

	de = sqrt(1 - pow((1 - (1.0 / f)), 2));
	daa = a * cos(B0) / sqrt(1.0 - pow(de * sin(B0), 2.0));
	m_L = L0 + y / daa;
	dq0 = tan(PI / 4 + B0 / 2) * pow((1 - de * sin(B0)) / (1 + de * sin(B0)), de / 2);
	dq0 = log10(dq0) / log10(LOGE);
	if (fabs(B0) < EPS)
		C = -daa * dq0;
	else
		C = 0;
	dq = (x - C) / daa;
	dq = pow(10, dq * log10(LOGE));

	Ddhrt(de, dq, 0, PI / 2, 0.2, EPS, B, 1);
	m_B = B[0];


}

/// <summary>通用横轴墨卡托投影反算
/// 通用横轴墨卡托投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bm">投影平均纬度(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>

void UTM_xtoB(double a, double f, double h, double Bc,
			  double x, double y, double L0,  double &m_B,  double &m_L)
{
	double e2, ep2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	ep2 = e2 / (1 - e2);
	double v1, p1, w1, e1, u1;
	double T1, C1, D;

	double M, M0, M1, M2, M3, M4;
	M1 = 1 - e2 / 4 - pow(e2, 2) * 3 / 64 - pow(e2, 3) * 5 / 256;
	M2 = e2 * 3 / 8 + pow(e2, 2) * 3 / 32 + pow(e2, 3) * 45 / 1024;
	M3 = pow(e2, 2) * 15 / 256 + pow(e2, 3) * 45 / 1024;
	M4 = pow(e2, 3) * 35 / 3072;

	M0 = a * (M1 * Bc - M2 * sin(2 * Bc) + M3 * sin(4 * Bc) - M4 * sin(6 * Bc));

	M = M0 + x;
	u1 = M / (a * (1 - e2 / 4 - 3 * pow(e2, 2) / 64 - 5 * pow(e2, 3) / 256));
	e1 = (1 - sqrt(1 - e2)) / (1 + sqrt(1 - e2));

	w1 = u1 + (e1 * 3 / 2 - pow(e1, 3) * 27 / 32) * sin(2 * u1);
	w1 += (pow(e1, 2) * 21 / 16 - pow(e1, 4) * 55 / 32) * sin(4 * u1);
	w1 += (pow(e1, 3) * 151 / 96) * sin(6 * u1);
	w1 += (pow(e1, 4) * 1097 / 512) * sin(8 * u1);

	p1 = a * (1 - e2) / pow(1 - e2 * pow(sin(w1), 2), 3.0 / 2.0);
	v1 = a / sqrt(1 - e2 * pow(sin(w1), 2));
	T1 = pow(tan(w1), 2);
	C1 = ep2 * pow(cos(w1), 2);
	D = y / v1;

	m_B = pow(D, 2) / 2 - (5 + 3 * T1 + 10 * C1 - 4 * pow(C1, 2) - 9 * ep2) * pow(D, 4) / 24
		+ (61 + 90 * T1 + 298 * C1 + 45 * pow(T1, 2) - 252 * ep2 - 3 * pow(C1, 2)) * pow(D, 6) / 720;
	m_B = w1 - v1 * tan(w1) / p1 * m_B;

	m_L = D - (1 + 2 * T1 + C1) * pow(D, 3) / 6
		+ (5 - 2 * C1 + 28 * T1 - 3 * pow(C1, 2) + 8 * ep2 + 24 * pow(T1, 2)) * pow(D, 5) / 120;
	m_L = L0 + m_L / cos(w1);

}

/// <summary>南向通用横轴墨卡托投影反算
/// 南向通用横轴墨卡托投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="h">投影平面高(m)</param>
/// <param name="Bm">投影平均纬度(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
/// <param name="L0">中央子午线(弧度)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>
void UTM_South_xtoB(double a, double f, double h, double Bc,
					double x, double y, double L0,
					double &m_B,  double &m_L)
{
	double e2, ep2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	ep2 = e2 / (1 - e2);
	double v1, p1, w1, e1, u1;
	double T1, C1, D;

	double M, M0, M1, M2, M3, M4;
	M1 = 1 - e2 / 4 - pow(e2, 2) * 3 / 64 - pow(e2, 3) * 5 / 256;
	M2 = e2 * 3 / 8 + pow(e2, 2) * 3 / 32 + pow(e2, 3) * 45 / 1024;
	M3 = pow(e2, 2) * 15 / 256 + pow(e2, 3) * 45 / 1024;
	M4 = pow(e2, 3) * 35 / 3072;

	M0 = a * (M1 * Bc - M2 * sin(2 * Bc) + M3 * sin(4 * Bc) - M4 * sin(6 * Bc));

	M = M0 - x;
	u1 = M / (a * (1 - e2 / 4 - 3 * pow(e2, 2) / 64 - 5 * pow(e2, 3) / 256));
	e1 = (1 - sqrt(1 - e2)) / (1 + sqrt(1 - e2));

	w1 = u1 + (e1 * 3 / 2 - pow(e1, 3) * 27 / 32) * sin(2 * u1);
	w1 += (pow(e1, 2) * 21 / 16 - pow(e1, 4) * 55 / 32) * sin(4 * u1);
	w1 += (pow(e1, 3) * 151 / 96) * sin(6 * u1);
	w1 += (pow(e1, 4) * 1097 / 512) * sin(8 * u1);

	p1 = a * (1 - e2) / pow(1 - e2 * pow(sin(w1), 2), 3.0 / 2.0);
	v1 = a / sqrt(1 - e2 * pow(sin(w1), 2));
	T1 = pow(tan(w1), 2);
	C1 = ep2 * pow(cos(w1), 2);
	D = -y / v1;

	m_B = pow(D, 2) / 2 - (5 + 3 * T1 + 10 * C1 - 4 * pow(C1, 2) - 9 * ep2) * pow(D, 4) / 24
		+ (61 + 90 * T1 + 298 * C1 + 45 * pow(T1, 2) - 252 * ep2 - 3 * pow(C1, 2)) * pow(D, 6) / 720;
	m_B = w1 - v1 * tan(w1) / p1 * m_B;

	m_L = D - (1 + 2 * T1 + C1) * pow(D, 3) / 6
		+ (5 - 2 * C1 + 28 * T1 - 3 * pow(C1, 2) + 8 * ep2 + 24 * pow(T1, 2)) * pow(D, 5) / 120;
	m_L = L0 + m_L / cos(w1);
}

/// <summary>兰伯托斜轴切圆锥投影反算
/// 兰伯托斜轴切圆锥投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央经线(弧度)</param>
/// <param name="k0">系数(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>
void Lambert_CC1SP_xtoB(double a, double f, double B0, double L0,
						double k0, double x, double y,
						double &m_B,  double &m_L)
{
	double m0, t0;
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);
	m0 = cos(B0) / sqrt(1 - e2 * pow(sin(B0), 2));
	t0 = tan(PI / 4 - B0 / 2) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);

	double n = sin(B0);
	double F = m0 / (n * pow(t0, n));
	//double r0 = a * F * pow(t0, n)*k0;
	double r0 = 0;
	if (fabs(B0 - PI / 2) > 0.000000000001)
	{ r0 =a * F * pow(t0, n)*k0;
	}
	double sita = atan(y / (r0 - x));
	m_L = sita / n + L0;
	double rp = sqrt(pow(y, 2) + pow(r0 - x, 2));
	rp = fabs(rp) * n / fabs(n);
	double tp = pow(rp / (a * F*k0), 1 / n);

	//B=PI/2-2atan(tp*pow((1-e*sinB)/(1+e*sinB),e/2))//
	DatKn2(m_B, 0.00000001, 20, tp, e);
}

/// <summary>兰伯托斜轴割圆锥投影反算
/// 兰伯托斜轴割圆锥投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央经线(弧度)</param>
/// <param name="B1">第一纬线(弧度)</param>
/// <param name="B2">第二纬线(弧度)</param>
/// <param name="x">平面坐标X(m)</param>
/// <param name="y">平面坐标Y(m)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>
void Lambert_CC2SP_xtoB(double a, double f, double B0, double L0,
						double B1, double B2,
						double x, double y,  double &m_B,  double &m_L)
{
	double mb1, mb2, tb1, tb2, tb0;
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);
	mb1 = cos(B1) / sqrt(1 - e2 * pow(sin(B1), 2));
	mb2 = cos(B2) / sqrt(1 - e2 * pow(sin(B2), 2));
	tb1 = tan(PI / 4 - B1 / 2) / pow((1 - e * sin(B1)) / (1 + e * sin(B1)), e / 2);
	tb2 = tan(PI / 4 - B2 / 2) / pow((1 - e * sin(B2)) / (1 + e * sin(B2)), e / 2);
	tb0 = tan(PI / 4 - B0 / 2) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);

	double n = log10(mb1 / mb2) / log10(tb1 / tb2);
	double F = mb1 / (n * pow(tb1, n));
	double r0 = 0;
	if (fabs(B0 - PI / 2) > 0.000000000001)
	{
		r0 = a * F * pow(tb0, n);
	}
	double sita = atan(y / (r0 - x));
	m_L = sita / n + L0;
	double rp = sqrt(pow(y, 2) + pow(r0 - x, 2));
	rp = fabs(rp) * n / fabs(n);
	double tp = pow(rp / (a * F), 1 / n);

	//B=PI/2-2atan(tp*pow((1-e*sinB)/(1+e*sinB),e/2))//
	DatKn2(m_B, 0.00000001, 20, tp, e);

}

/// <summary>Stereo倾斜赤平投影反算
/// Stereo倾斜赤平投影反算
/// </summary>
/// <param name="a">椭球长半轴(m)</param>
/// <param name="f">椭球扁率的倒数</param>
/// <param name="B0">中央纬线(弧度)</param>
/// <param name="L0">中央经线(弧度)</param>
/// <param name="k0">系数(弧度)</param>
/// <param name="m_x">平面坐标X(m)</param>
/// <param name="m_y">平面坐标Y(m)</param>
/// <param name="m_B">纬度(弧度)</param>
/// <param name="m_L">经度(弧度)</param>
void Stereo_Oblique_xtoB(double a, double f, double B0, double L0,
						 double k0, double m_x, double m_y,   double &m_B,  double &m_L)
{
	double e, e2;
	e2 = 1 - pow((1 - (1.0 / f)), 2);
	e = sqrt(e2);

	double R, n, C, p0, v0;
	p0 = a * (1 - e2) / pow(1 - e2 * pow(sin(B0), 2), 3.0 / 2.0);
	v0 = a / sqrt(1 - e2 * pow(sin(B0), 2));
	R = sqrt(p0 * v0);
	n = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));

	double S1, S2, w1, w2, sinX0;
	S1 = (1 + sin(B0)) / (1 - sin(B0));
	S2 = (1 - e * sin(B0)) / (1 + e * sin(B0));
	w1 = pow(S1 * pow(S2, e), n);

	sinX0 = (w1 - 1) / (w1 + 1);
	C = (n + sin(B0)) * (1 - sinX0) / ((n - sin(B0)) * (1 + sinX0));
	w2 = C * w1;

	double X0, A0;
	X0 = asin((w2 - 1) / (w2 + 1));
	A0 = L0;

	//
	double g, h, i, j;
	g = 2 * R * k0 * tan(PI / 4 - X0 / 2);
	h = 4 * R * k0 * tan(X0) + g;
	i = atan(m_y * k0 / (h + m_x * k0));
	j = atan(m_y * k0 / (g - m_x * k0)) - i;

	double X, A;
	A = j + 2 * i + A0;
	X = X0 + 2 * atan((m_x * k0 - m_y * k0 * tan(j / 2)) / (2 * R * k0));

	double b0, b, exp;
	m_L = (A - A0) / n + A0;
	b0 = 0.5 * log10((1 + sin(X)) / (C * (1 - sin(X)))) / log10(LOGE) / n;
	//b=0.5*log10((1+sin(X))/(C*(1-sin(X))))/log10(LOGE)/n;
	m_B = 2 * atan(pow(LOGE, b0)) - PI / 2;
	int nCount = 0;
	do
	{
		b = log10(tan(m_B / 2 + PI / 4) * pow((1 - e * sin(m_B)) / (1 + e * sin(m_B)), e / 2)) / log10(LOGE);
		exp = (b - b0) * cos(m_B) * (1 - e2 * pow(sin(m_B), 2)) / (1 - e2);
		m_B = m_B - exp;
		nCount++;
	} while (fabs(exp) > 1E-8 && nCount < 10);
}


//倾斜墨卡托投影正算
void Mecator_Oblique_Btox(double a, double f, double B0, double L0, double Kc,double B1, double B2, double B, double L,
						  double &x,  double &y)
{

	double e,e2;
	e2=1-pow((1-(1.0/f)),2);
	e=sqrt(e2);

	//客户提供的参数
	if (a == 6377298.556 && f == 300.8017)// Everest - 1967
	{
		e = 0.081472981; e2 = 0.006637847;
	}

	double MB,MA,t0,MD;
	MB=sqrt(1+e2*pow(cos(B0),4)/(1-e2));
	MA=a*MB*Kc*sqrt(1-e2)/(1-e2*pow(sin(B0),2));
	t0=tan(PI/4.0-B0/2.0)/pow((1-e*sin(B0))/(1+e*sin(B0)),e/2);
	MD=MB*sqrt(1-e2)/(cos(B0)*sqrt(1-e2*pow(sin(B0),2)));

	double MF,MH,MG,MY,MZ;
	if(MD<1)
		MF=MD;
	else
		MF=MD+sqrt(MD*MD-1)*SIGN(B0);

	MH=MF*pow(t0,MB);
	MG=(MF-1/MF)/2;
	MY=asin(sin(B1)/MD);
	MZ=L0-asin(MG*tan(MY))/MB;

	double uc;
	if(MD<1)
		uc=0;
	else
		uc=(MA/MB)*atan(sqrt(MD*MD-1)/cos(B1))*SIGN(B0);

	double Mt,MQ,MS,MT,MV,MU,Mv,Mu;
	Mt=tan(PI/4-B/2)/pow((1-e*sin(B))/(1+e*sin(B)),e/2);
	MQ=MH/pow(Mt,MB);
	MS=(MQ-1/MQ)/2;
	MT=(MQ+1/MQ)/2;
	MV=sin(MB*(L-MZ));
	MU=(-1.0*MV*cos(MY)+MS*sin(MY))/MT;
	Mv=MA*log10((1-MU)/(1+MU))/(2*MB)/log10(LOGE);
	Mu=MA*atan((MS*cos(MY)+MV*sin(MY))/cos(MB*(L-MZ)))/MB-fabs(uc)*SIGN(B0);

	y=Mv*cos(B2)+Mu*sin(B2);
	x=Mu*cos(B2)-Mv*sin(B2);

	x/=Kc;
	y/=Kc;

}

//倾斜墨卡托投影反算
void Mecator_Oblique_xtoB(double a, double f, double B0, double L0, double Kc, double B1, double B2, double x, double y,
						  double &B,  double &L)
{

	double e,e2;
	e2=1-pow((1-(1.0/f)),2);
	e=sqrt(e2);
	//客户提供的参数
	if (a == 6377298.556 && f == 300.8017)// Everest - 1967
	{
		e = 0.081472981; e2 = 0.006637847;
	}

	double MB,MA,t0,MD;
	MB=sqrt(1+e2*pow(cos(B0),4)/(1-e2));
	MA=a*MB*Kc*sqrt(1-e2)/(1-e2*pow(sin(B0),2));
	t0=tan(PI/4.0-B0/2.0)/pow((1-e*sin(B0))/(1+e*sin(B0)),e/2);
	MD=MB*sqrt(1-e2)/(cos(B0)*sqrt(1-e2*pow(sin(B0),2)));

	double MF,MH,MG,MY,MZ;
	if(MD<1)
		MF=MD;
	else
		MF=MD+sqrt(MD*MD-1)*SIGN(B0);

	MH=MF*pow(t0,MB);
	MG=(MF-1/MF)/2;
	MY=asin(sin(B1)/MD);
	MZ=L0-asin(MG*tan(MY))/MB;

	double uc;
	if(MD<1)
		uc=0;
	else
		uc=(MA/MB)*atan(sqrt(MD*MD-1)/cos(B1))*SIGN(B0);

	double Mv,Mu;
	x*=Kc;
	y*=Kc;

	Mv=y*cos(B2)-x*sin(B2);
	Mu=x*cos(B2)+y*sin(B2)+fabs(uc)*SIGN(B0);

	double MQ,MS,MT,MV,MU,Mt,MX;
	MQ=pow(LOGE,-1.0*MB*Mv/MA);
	MS=(MQ-1/MQ)/2;
	MT=(MQ+1/MQ)/2;
	MV=sin(MB*Mu/MA);
	MU=(MV*cos(MY)+MS*sin(MY))/MT;
	Mt=pow(MH/sqrt((1+MU)/(1-MU)),1.0/MB);
	MX=PI/2-2*atan(Mt);

	B=MX+sin(2*MX)*(1.0/2*e2+5.0/24*pow(e2,2)+1.0/12*pow(e2,3)+13.0/360*pow(e2,4))+
		sin(4*MX)*(7.0/48*pow(e2,2)+29.0/240*pow(e2,3)+811.0/11520*pow(e2,4))+
		sin(6*MX)*(7.0/120*pow(e2,3)+81.0/1120*pow(e2,4))+
		sin(8*MX)*(4279.0/161280*pow(e2,4));

	L=MZ-atan((MS*cos(B2)-MV*sin(B2))/cos(MB*Mu/MA))/MB;

}

//Hotine倾斜墨卡托投影正算
void Mecator_HotineOblique_Btox(double a, double f, double B0, double L0, double Kc, double B1, double B2, 
								double B, double L,  double &x,  double &y)
{

	double e,e2;
	e2=1-pow((1-(1.0/f)),2);
	e=sqrt(e2);


	//开始计算
	if (fabs(B1 - PI / 2) < 0.000001) 	//Test if Swiss projection with B1=90?
	{

		e = sqrt(e2);
		double alpha, R, b0, K, S, b, l, lq, bq, Y, X;
		R = a * sqrt(1 - e2) / (1 - e2 * pow(sin(B0), 2));
		alpha = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));
		b0 = arcsin(sin(B0) / alpha);
		K = ln(tan(PI / 4 + b0 / 2)) - alpha * ln(tan(PI / 4 + B0 / 2)) + alpha * e / 2 * ln((1 + e * sin(B0)) / (1 - e * sin(B0)));
		S = alpha * ln(tan(PI / 4 + B / 2)) - alpha * e / 2 * ln((1 + e * sin(B)) / (1 - e * sin(B))) + K;
		b = 2 * (arctan(pow(Math_E, S)) - PI / 4);
		l = alpha * (L - L0);
		lq = arctan(sin(l) / (sin(b0) * tan(b) + cos(b0) * cos(l)));
		bq = arcsin(cos(b0) * sin(b) - sin(b0) * cos(b) * cos(l));
		Y = R * lq;
		X = R / 2 * ln((1 + sin(bq)) / (1 - sin(bq)));
		y = Y;
		x = X;
	}
	else
		// general Snyder formulae
	{


		//
		double MB, MA, t0, MD;
		MB = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));
		MA = a * MB * Kc * sqrt(1 - e2) / (1 - e2 * pow(sin(B0), 2));
		t0 = tan(PI / 4.0 - B0 / 2.0) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);
		MD = MB * sqrt(1 - e2) / (cos(B0) * sqrt(1 - e2 * pow(sin(B0), 2)));

		double MF, MH, MG, MY, MZ;
		if (MD < 1)
			MF = MD;
		else
			MF = MD + sqrt(MD * MD - 1) * SIGN(B0);

		MH = MF * pow(t0, MB);
		MG = (MF - 1 / MF) / 2;
		MY = asin(sin(B1) / MD);
		MZ = L0 - asin(MG * tan(MY)) / MB;

		//double uc;
		//if (MD < 1)
		//    uc = 0;
		//else
		//    uc = (MA / MB) * atan(sqrt(MD * MD - 1) / cos(B1)) * SIGN(B0);


		double Mt, MQ, MS, MT, MV, MU, Mv, Mu;
		Mt = tan(PI / 4 - B / 2) / pow((1 - e * sin(B)) / (1 + e * sin(B)), e / 2);
		MQ = MH / pow(Mt, MB);
		MS = (MQ - 1 / MQ) / 2;
		MT = (MQ + 1 / MQ) / 2;
		MV = sin(MB * (L - MZ));

		MU = (-1.0 * MV * cos(MY) + MS * sin(MY)) / MT;
		//Mv = MA * log10((1 - MU) / (1 + MU)) / (2 * MB) / log10(LOGE);
		//Mv = MA * Math.Log((1.0 - MU) / (1.0 + MU)) / (2.0 * MB);
		Mv = MA * ln((1.0 - MU) / (1.0 + MU)) / (2.0 * MB);
		Mu = (MA / MB) * atan((MS * cos(MY) + MV * sin(MY)) / cos(MB * (L - MZ)));

		y = Mv * cos(B2) + Mu * sin(B2);
		x = Mu * cos(B2) - Mv * sin(B2);

	}
}

//Hotine倾斜墨卡托投影反算
void Mecator_HotineOblique_xtoB(double a, double f, double B0, double L0, double Kc, double B1, double B2, double x, double y,
								double &B, double &L)
{
	double e,e2;
	e2=1-pow((1-(1.0/f)),2);
	e=sqrt(e2);

	if (fabs(B1 - PI / 2) < 0.000001) 	//Test if Swiss projection with B1=90?
	{
		//标准算法
		double alpha, R, b0, K, S, b, l, lq, bq, Y, X;
		R = a * sqrt(1 - e2) / (1 - e2 * pow(sin(B0), 2));
		alpha = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));
		b0 = arcsin(sin(B0) / alpha);
		K = ln(tan(PI / 4 + b0 / 2)) - alpha * ln(tan(PI / 4 + B0 / 2)) + alpha * e / 2 * ln((1 + e * sin(B0)) / (1 - e * sin(B0)));
		//
		Y = y;
		X = x;
		//
		lq = Y / R;
		bq = 2 * (arctan(exp(X / R)) - PI / 4);
		b = arcsin(cos(b0) * sin(bq) + sin(b0) * cos(bq) * cos(lq));
		l = arctan(sin(lq) / (cos(b0) * cos(lq) - sin(b0) * tan(bq)));

		//
		L = L0 + l / alpha;

		double tempB;
		B = b;
		S = 0;
		do
		{
			//
			tempB = B;
			//S = alpha * ln(tan(PI / 4 + tempB / 2)) - alpha * e / 2 * ln((1 + e * sin(tempB)) / (1 - e * sin(tempB))) + K;
			S = (ln(tan(PI / 4 + b / 2)) - K) / alpha + e * ln(tan(PI / 4 + arcsin(e * sin(tempB)) / 2));
			B = 2 * arctan(exp(S)) - PI / 2;
		} while (fabs(B - tempB) > 0.00000000000001);
	}
	else
	{
		double MB, MA, t0, MD;
		MB = sqrt(1 + e2 * pow(cos(B0), 4) / (1 - e2));
		MA = a * MB * Kc * sqrt(1 - e2) / (1 - e2 * pow(sin(B0), 2));
		t0 = tan(PI / 4.0 - B0 / 2.0) / pow((1 - e * sin(B0)) / (1 + e * sin(B0)), e / 2);
		MD = MB * sqrt(1 - e2) / (cos(B0) * sqrt(1 - e2 * pow(sin(B0), 2)));

		double MF, MH, MG, MY, MZ;
		if (MD < 1)
			MF = MD;
		else
			MF = MD + sqrt(MD * MD - 1) * SIGN(B0);

		MH = MF * pow(t0, MB);
		MG = (MF - 1 / MF) / 2;
		MY = asin(sin(B1) / MD);
		MZ = L0 - asin(MG * tan(MY)) / MB;

		double uc;
		if (MD < 1)
			uc = 0;
		else
			uc = (MA / MB) * atan(sqrt(MD * MD - 1) / cos(B1)) * SIGN(B0);

		double Mv, Mu;
		x *= Kc;
		y *= Kc;

		Mv = y * cos(B2) - x * sin(B2);
		Mu = x * cos(B2) + y * sin(B2);

		double MQ, MS, MT, MV, MU, Mt, MX;
		MQ = pow(LOGE, -1.0 * MB * Mv / MA);
		MS = (MQ - 1 / MQ) / 2;
		MT = (MQ + 1 / MQ) / 2;
		MV = sin(MB * Mu / MA);
		MU = (MV * cos(MY) + MS * sin(MY)) / MT;
		Mt = pow(MH / sqrt((1 + MU) / (1 - MU)), 1.0 / MB);
		MX = PI / 2 - 2 * atan(Mt);

		B = MX + sin(2 * MX) * (1.0 / 2 * e2 + 5.0 / 24 * pow(e2, 2) + 1.0 / 12 * pow(e2, 3) + 13.0 / 360 * pow(e2, 4)) +
			sin(4 * MX) * (7.0 / 48 * pow(e2, 2) + 29.0 / 240 * pow(e2, 3) + 811.0 / 11520 * pow(e2, 4)) +
			sin(6 * MX) * (7.0 / 120 * pow(e2, 3) + 81.0 / 1120 * pow(e2, 4)) +
			sin(8 * MX) * (4279.0 / 161280 * pow(e2, 4));

		L = MZ - atan((MS * cos(B2) - MV * sin(B2)) / cos(MB * Mu / MA)) / MB;
	}
}

/// <summary>
/// Btox 大地坐标转换为平面坐标
/// </summary>
/// <param name="nModel">转换模型</param>
/// <param name="ea">椭球长半轴(m)</param>
/// <param name="ef">椭球扁率的倒数</param>
/// <param name="par">投影参数</param>
/// <param name="dB">纬度(弧度)</param>
/// <param name="dL">经度(弧度)</param>
/// <param name="H">投影点高程(带投影面高投影的时候会修改)</param>
/// <param name="dx">平面坐标X(m)</param>
/// <param name="dy">平面坐标Y(m)</param>
/// <param name="bNorth">北方向</param>
/// <param name="bEast">东方向</param>
void BLtoxy(int nModel, double ea, double ef, ZHDProjPars par,
			double dB, double dL,  double &H, double &dx,  double &dy,
			bool bNorth, bool bEast)
{
	//1.【将L处理到合理范围-2011-10.9】
	if (dL>0)
	{
		//规算到0到360度
		int n = (int)(dL / (2.0*PI));
		dL = dL - n * 2.0 * PI;
	}else
	{
		//规算到0到-360度
		int n = (int)(dL / (-2.0 * PI));
		dL = dL + n * 2.0 * PI;
	}
	//2.
	if (fabs(dL - par.Lo)>PI)
	{
		if (dL>0)
		{
			dL = dL - PI * 2.0;

		}else
		{
			dL = dL + PI * 2.0;

		}
	}

	//
	bool IszoneNumber = false;//是否有带号
	switch (nModel)
	{
	case ZHD_ProjectionEnum_Guas3://高斯3
		//若还没有计算带号,则计算,并推算投影中心
		IszoneNumber = true;//标志有带号
		par.Add = (int)(par.Lo * 180 / PI / 3);//计算带号
		Gauss_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Lo,  dx, dy);
		break;
	case ZHD_ProjectionEnum_Guass6://高斯6
		//若还没有计算带号,则计算,并推算投影中心
		IszoneNumber = true;//标志有带号
		par.Add = (int)(par.Lo * 180 / PI + 3) / 6;//计算带号
		Gauss_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Lo,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Guass_Userdefine://自定义高斯
		Gauss_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Lo,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Mecator://莫卡托投影
		Mercator_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Bo, par.Lo,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_UTM://UTM投影
		UTM_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Lo,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_TM_South:
		UTM_South_Btox(ea, ef, par.PH, par.Bc, dB, dL, par.Lo,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Lambert_1CCP://Lamber投影
		Lambert_CC1SP_Btox(ea, ef, par.Bo, par.Lo, par.Ko, dB, dL,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Lambert_2CCP://TM(south)
		Lambert_CC2SP_Btox(ea, ef, par.Bo, par.Lo, par.B1, par.B2, dB, dL,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Double_Stereographic:
	case ZHD_ProjectionEnum_Oblique_Stereo://Oblique Stereographic
		Stereo_Oblique_Btox(ea, ef, par.Bo, par.Lo, par.Ko, dB, dL,  dx,  dy);
		break; 
		//马来西亚
	case ZHD_ProjectionEnum_Mecator_Oblique:
		Mecator_Oblique_Btox(ea, ef, par.Bo, par.Lo, par.Ko, par.B1, par.B2, dB, dL,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_Mecator_Hotine_Oblique:
		Mecator_HotineOblique_Btox(ea, ef, par.Bo, par.Lo, par.Ko, par.B1, par.B2, dB, dL,  dx,  dy);
		break;
	case ZHD_ProjectionEnum_CassiniSoldner:
		CassiniSoldner(par.Bo, par.Lo, ef, ea);
		TransformValue(dB, dL, &dx, &dy);
		break;
	}

	if (nModel != ZHD_ProjectionEnum_Mecator_Oblique && nModel != ZHD_ProjectionEnum_Mecator_Hotine_Oblique && nModel != ZHD_ProjectionEnum_Lambert_1CCP)
	{
		//
		dx *= par.Ko;
		dy *= par.Ko;
	}

	dx +=  par.FN;
	dy +=  par.FE;

	if (!bNorth)
		dx = -dx;
	if (!bEast)
		dy = -dy;
	//

}

/// <summary>xtoB 平面坐标转换为大地坐标，即投影反算
/// xtoB 平面坐标转换为大地坐标，即投影反算
/// </summary>
/// <param name="nModel">投影反算模型</param>
/// <param name="ea">椭球长半轴(m)</param>
/// <param name="ef">椭球扁率的倒数</param>
/// <param name="pars">投影参数</param>
/// <param name="dx">平面坐标X(m)</param>
/// <param name="dy">平面坐标Y(m)</param>
/// <param name="dB">纬度(弧度)</param>
/// <param name="dL">经度(弧度)</param>
/// <param name="bNorth">北方向</param>
/// <param name="bEast">东方向</param>
void xytoBL(int nModel, double ea, double ef, ZHDProjPars par,
			double dx, double dy, double dh, double &dB,  double &dL,  double &dH,
			bool bNorth, bool bEast)
{
	if (!bNorth)
		dx = -dx;
	if (!bEast)
		dy = -dy;
	//---------
	dx = (dx - par.FN);
	dy = (dy - par.FE);

	//2011-1207 修改
	if (nModel != ZHD_ProjectionEnum_Mecator_Oblique && nModel != ZHD_ProjectionEnum_Mecator_Hotine_Oblique && nModel != ZHD_ProjectionEnum_Lambert_1CCP)
	{
		dx /= par.Ko;
		dy /= par.Ko;
	}

	//dx,dy为平面坐标,根据投影参数求得大地坐标
	switch (nModel)
	{
	case ZHD_ProjectionEnum_Guas3://高斯3
		dH = dh;
		Gauss_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Lo, dB, dL);
		break;
	case ZHD_ProjectionEnum_Guass6://高斯6
		dH = dh;
		Gauss_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Lo, dB, dL);
		break;
	case ZHD_ProjectionEnum_Guass_Userdefine://自定高斯
		dH = dh;
		Gauss_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Lo, dB, dL);
		break;
	case ZHD_ProjectionEnum_Mecator:  //墨卡托
		Mercator_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Bo, par.Lo,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_UTM:  //UTM
		UTM_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Lo,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_TM_South://TM(south)
		UTM_South_xtoB(ea, ef, par.PH, par.Bc, dx, dy, par.Lo,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_Lambert_1CCP://Lambert
		Lambert_CC1SP_xtoB(ea, ef, par.Bo, par.Lo, par.Ko, dx, dy,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_Lambert_2CCP://Lambert
		Lambert_CC2SP_xtoB(ea, ef, par.Bo, par.Lo, par.B1, par.B2, dx, dy,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_Double_Stereographic:
	case ZHD_ProjectionEnum_Oblique_Stereo://Oblique Stereographic
		Stereo_Oblique_xtoB(ea, ef, par.Bo, par.Lo, par.Ko, dx, dy,  dB,  dL);
		break;
		//马来西亚
	case ZHD_ProjectionEnum_Mecator_Oblique:
		Mecator_Oblique_xtoB(ea, ef, par.Bo, par.Lo, par.Ko , par.B1, par.B2, dx, dy,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_Mecator_Hotine_Oblique:
		Mecator_HotineOblique_xtoB(ea, ef, par.Bo, par.Lo, par.Ko, par.B1, par.B2,  dx, dy,  dB,  dL);
		break;
	case ZHD_ProjectionEnum_CassiniSoldner:
		CassiniSoldner(par.Bo, par.Lo, ef, ea);
		ReverseValue( dx, dy, &dB, &dL);
		break;
	}

}

