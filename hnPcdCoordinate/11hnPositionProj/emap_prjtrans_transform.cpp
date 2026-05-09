//---------------------------------------------------------------------------
//	 emap_prjtrans_transform.c
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "emap_prjtrans_base.h"
#include "emap_prjtrans_api.h"

double Atn90to90(double Y, double X)
{
	double t = 0;
	if (X == 0)
	{
		if (Y > 0) { t = PI / 2; }
		if (Y < 0) { t = -PI / 2; }
	}
	else
	{
		t = atan(Y / X);
	}
	return t;
}
//-----------------------------------------------
//我们需要计算为东经180-西经180
//-----------------------------------------------
double  Atn180to180(double Y, double X)
{
	double t = 0;
	if (X > 0 && Y > 0) { t = atan(Y / X); }
	if (X > 0 && Y < 0) { t = 2 * PI + atan(Y / X); }
	if (X < 0 && Y > 0) { t = PI + atan(Y / X); }
	if (X < 0 && Y < 0) { t = PI + atan(Y / X); }

	if (X == 0 && Y > 0) { t = PI / 2; }
	if (X == 0 && Y < 0) { t = PI * 3 / 2; }
	if (X > 0 && Y == 0) { t = 0; }
	if (X < 0 && Y == 0) { t = PI; }
	if (X == 0 && Y == 0) { t = 0; }
	//归到东西180
	if (t > PI) { t = -(2 * PI - t); }

	return t;
}


// 矩阵求逆
bool  GetJZ(double *A, int n, double *B, int *JS)
{
	double temp = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int L = 0;
	for (k = 0; k < n; k++)
	{
		double d = 0.0;
		for (i = k; i < n; i++)
		{
			for (j = k; j < n; j++)
				if (fabs (A[i * n + j]) > d)
				{
					d = fabs(A[i * n + j]);
					JS[k] = j;
					L = i;
				}
		}
		//if (d + 1.0 == 1.0)
		//	return false;
		if(d<=0)
			return false;
		if (JS[k] != k)
			for (i = 0; i < n; i++)
			{
				temp = A[i * n + k];
				A[i * n + k] = A[i * n + JS[k]];
				A[i * n + JS[k]] = temp;
			}
			if (L != k)
			{
				for (j = k; j < n; j++)
				{
					temp = A[k * n + j];
					A[k * n + j] = A[L * n + j];
					A[L * n + j] = temp;
				}
				temp = B[k];
				B[k] = B[L];
				B[L] = temp;
			}
			for (j = k + 1; j < n; j++)
				A[k * n + j] = (1.0 * A[k * n + j]) / A[k * n + k];
			B[k] = (B[k] * 1.0) / A[k * n + k];

			for (i = 0; i < n; i++)
			{
				if (i != k)
				{
					for (j = k + 1; j < n; j++)
						A[i * n + j] = A[i * n + j] - A[i * n + k] * A[k * n + j];
					B[i] = B[i] - A[i * n + k] * B[k];
				}
			}
	}
	for (k = n - 1; k > -1; k--)
	{
		if (JS[k] != k)
		{
			temp = B[k];
			B[k] = B[JS[k]];
			B[JS[k]] = temp;
		}
	}
	return true;
}

// <summary>大地坐标转为空间直角坐标(BLH->XYZ)
void BtoX(double a, double f, double B, double L, double H, double *m_X,  double *m_Y,  double *m_Z)
{
	//double N, e2;
	//double M, Rm;

	//e2 = 1 -pow((1 - (1.0 / f)), 2);

	//N = a / (sqrt(1 - e2 *pow(sin(B), 2)));
	//M = a * (1 - e2) / sqrt(pow(1 - e2 *pow(sin(B), 2), 3));
	//Rm = sqrt(M * N);

	//*m_X = (N + H) * cos(B) * cos(L);
	//*m_Y = (N + H) * cos(B) * sin(L);
	//*m_Z = (N * (1 - e2) + H) *sin(B);

	double N, e2;
	double M, Rm;

	e2 = 1 -pow((1 - (1.0 / f)), 2);

	N = a / (sqrt(1 - e2 *pow(sin(B), 2)));
	M = a * (1 - e2) / sqrt(pow(1 - e2 *pow(sin(B), 2), 3));
	Rm = sqrt(M * N);

	*m_X = (N + H) * cos(B) * cos(L);
	*m_Y = (N + H) * cos(B) * sin(L);
	*m_Z = (N * (1 - e2) + H) *sin(B);
}



// XtoB 空间直角坐标转换为大地坐标(XYZ->BLH)
void XtoB(double a, double f, double X, double Y, double Z, double *m_B, double *m_L, double *m_H)
{
	double e2,N;
	double C1,C2,C3,C4,R,Q,k,m,n;

	e2=1-pow((1-(1.0/f)),2);
	
	R=sqrt(pow(X,2)+pow(Y,2)+pow(Z,2));
	Q=atan(Z/sqrt(pow(X,2)+pow(Y,2)));
	k=a/R;
	m=sin(Q);
	n=cos(Q);
	C1=k*m*n;
	C2=pow(m,2)+2*k*(1-2*pow(m,2));
	C3=9*pow(m,4)+24*k*pow(m,2)*(2-3*pow(m,2))+4*pow(k,2)*(6-35*pow(m,2)+35*pow(m,4));
	C4=5*pow(m,6)+16*k*pow(m,4)*(3-4*pow(m,2))
		+4*pow(k,2)*pow(m,2)*(20-77*pow(m,2)+63*pow(m,4))
		+16*pow(k,3)*(1-12*pow(m,2)+30*pow(m,4)-20*pow(m,6));

	*m_B=Q+C1*e2*(1+0.5*e2*(C2+1/12.0*e2*(C3+1.5*C4*e2)));
	*m_L=atan(Y/X);

	N=a/(sqrt(1-e2*pow(sin(*m_B),2)));
	*m_H = R*n/cos(*m_B)-N;

	if(Y<0 && X<0) *m_L-=PI;
	if(Y>0 && X<0) *m_L+=PI;
}


// xtox 四参数平面转换
void xtox(ZHDFourPar fPar,  double *x,  double *y)
{
	double dx, dy;
	double x1 = *x, y1 = *y;

	dx = fPar.Dx + (x1 * cos(fPar.T) - y1 * sin(fPar.T)) * fPar.K ;
	dy = fPar.Dy + (y1 * cos(fPar.T) + x1 * sin(fPar.T)) * fPar.K ;
	*x = dx;
	*y = dy;
}


// 四参数平面转换反算
void xtox_false(ZHDFourPar fPar, double *x,  double *y)
{
	double k, a, x1, x0, y1, y0;
	x0 = fPar.Dx;
	y0 = fPar.Dy;
	a = fPar.T;
	k = fPar.K;// (1 + fPar.K * 1.0E-06);
	x1 = *x;
	y1 = *y;

	//求反变换
	double A[] ={k*cos(a), (-1)*k*sin(a), k*sin(a), k*cos(a)};
	double B[] ={ x1 - x0, y1 - y0 };
	int	   JS[] ={ 0, 0 };

	//
	bool bOk = GetJZ(A, 2, B, JS);
	if (bOk)
	{
		*x = B[0];
		*y = B[1];
	}
	else
	{
		*x = 999.999;
		*y = 999.999;
	}
}


// 七参数正算
void XtoX_Bursa_Simple__(ZHDSevenPar sPar,double *X,  double *Y,  double *Z)
{
	double x, y, z;
	double X1, Y1, Z1;

	X1 = *X;
	Y1 = *Y;
	Z1 = *Z;
	x = sPar.DX + (1 + sPar.K * 1.0E-06) * X1 + sPar.WZ * Y1 - sPar.WY * Z1;
	y = sPar.DY + (1 + sPar.K * 1.0E-06) * Y1 - sPar.WZ * X1 + sPar.WX * Z1;
	z = sPar.DZ + (1 + sPar.K * 1.0E-06) * Z1 + sPar.WY * X1 - sPar.WX * Y1;
	*X = x;
	*Y = y;
	*Z = z;
}

// 七参数反算
void XtoX_Bursa_Simple_False__(ZHDSevenPar sPar,double *X,  double *Y,  double *Z)
{

	double K = sPar.K * 1.0E-06;
	double DX = sPar.DX;
	double DY = sPar.DY;
	double DZ = sPar.DZ;
	double WX = sPar.WX;
	double WY = sPar.WY;
	double WZ = sPar.WZ;
	double X1 = *X, Y1 = *Y, Z1 = *Z;

	double A[] = { 1 + K, WZ, -WY, -WZ, 1 + K, WX, WY, -WX, 1 + K };
	double B[] = { X1 - DX, Y1 - DY, Z1 - DZ };
	int JS[] = { 0, 0, 0 };
	if (GetJZ(A, 3, B, JS))
	{
		*X = B[0];
		*Y = B[1];
		*Z = B[2];
	}
}

// 通常的高程拟合计算
void HFixCalus2(int Model, ZHDHFixPar par, double x, double y, double *dResult)
{
	*dResult = 0;
	//
	switch (Model)
	{
	case ZHD_HFixEnum_None:
		*dResult = 0;
		break;
	case ZHD_HFixEnum_Constant:
		*dResult = par.A;
		break;
	case ZHD_HFixEnum_Pane:
		*dResult = par.A + par.B * x + par.C * y;
		break;
	case ZHD_HFixEnum_Curve:
		*dResult = par.A + par.B * x + par.C * y + par.D * pow(x, 2) + par.E * pow(y, 2)+ par.F* x * y  ;
		break;
	}
}
