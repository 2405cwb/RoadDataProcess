#include "StdAfx.h"
#include "DirectLinearTrans.h"
#include "..\hdCore\hdMatrix.h"
#include "..\hdHlslib\inc\mydefs.hpp"
CDirectLinearTrans::CDirectLinearTrans(void)
{
	for (int i=0;i<11;i++)
	{
		memset(M,0,sizeof(double));
	}
	m_k1 = 0.0;
	m_bMComputed = false;
}


CDirectLinearTrans::~CDirectLinearTrans(void)
{
}

void CDirectLinearTrans::ComputeMMatrix( vector<PanoControlPoint> &controlPoints,int imgWidth, int imgHeight )
{
	//
	m_imgWidth = imgWidth;
	m_imgHeight = imgHeight;
	int nCtrlCount = controlPoints.size();
	hdMatrix A(2 * nCtrlCount, 11);
	hdMatrix L(2 * nCtrlCount, 1);
	hdMatrix X(11, 1);
	hdMatrix AT(11, 2 * nCtrlCount);
	hdMatrix ATA(11, 11);
	hdMatrix ATA_INVERSE(11, 11);
	hdMatrix ATL(11, 1);

	//赋值	
	for (int i = 0; i < nCtrlCount; i++)
	{
		A(i * 2, 0 ) = controlPoints.at(i).X;
		A(i * 2, 1 ) = controlPoints.at(i).Y;
		A(i * 2, 2 ) = controlPoints.at(i).Z;
		A(i * 2, 3 ) = 1;
		A(i * 2, 4 ) = 0;
		A(i * 2, 5 ) = 0;
		A(i * 2, 6 ) = 0;
		A(i * 2, 7 ) = 0;
		A(i * 2, 8 ) = controlPoints.at(i).imageX * controlPoints.at(i).X;
		A(i * 2, 9 ) = controlPoints.at(i).imageX * controlPoints.at(i).Y;
		A(i * 2, 10) = controlPoints.at(i).imageX * controlPoints.at(i).Z;
		L(i * 2, 0 ) = -controlPoints.at(i).imageX ;

		A(i * 2 + 1, 0 ) = 0;
		A(i * 2 + 1, 1 ) = 0;
		A(i * 2 + 1, 2 ) = 0;
		A(i * 2 + 1, 3 ) = 0;
		A(i * 2 + 1, 4 ) = controlPoints.at(i).X;
		A(i * 2 + 1, 5 ) = controlPoints.at(i).Y;
		A(i * 2 + 1, 6 ) = controlPoints.at(i).Z;
		A(i * 2 + 1, 7 ) = 1;
		A(i * 2 + 1, 8 ) = controlPoints.at(i).imageY * controlPoints.at(i).X;
		A(i * 2 + 1, 9 ) = controlPoints.at(i).imageY * controlPoints.at(i).Y;
		A(i * 2 + 1, 10) = controlPoints.at(i).imageY * controlPoints.at(i).Z;
		L(i * 2 + 1, 0 ) = -controlPoints.at(i).imageY;
	}
	//赋值完毕
	//A:2*nCtrlCount行，11列
	//AT:11行，2*nCtrlCount列
	//ATA：11行，11列
	//L：2*nCtrlCount行，1列
	//ATL：11行，1列
	A.Transpose(AT);

	//ATA = AT * A;
	AT.Multiply(A, ATA);

	//ATL = AT * L;
	AT.Multiply(L, ATL);
	
	ATA.InvertGaussJordan(ATA_INVERSE);

	//X = ATA_INVERSE * ATL;
	ATA_INVERSE.Multiply(ATL, X);
	/***************************************/
	//第二次迭代		
	hdMatrix A_2(2 * nCtrlCount, 12);
	hdMatrix L_2(2 * nCtrlCount, 1);
	hdMatrix X_2(12, 1);
	hdMatrix DX_2(12, 1);
	hdMatrix AT_2(12, 2 * nCtrlCount);
	hdMatrix ATA_2(12, 12);
	hdMatrix ATA_INVERSE_2(12, 12);
	hdMatrix ATL_2(12, 1);
	
	for (int i = 0; i < 11; i++)
	{
		X_2(i, 0) = X(i, 0);
	}
	X_2(11, 0) = 0;

	int nTime = 0;//迭代次数
	double halfWidth = m_imgWidth / 2.0;
	double halfHeight = m_imgHeight / 2.0;
	double* Ai = new double[nCtrlCount];
	double* xx = new double[nCtrlCount];
	double* yy = new double[nCtrlCount];

	for (int i = 0; i < 8; i++)
	{
		DX_2(i, 1) = 1;
	}

	while (fabs(DX_2(0, 1)) > 0.00001 || fabs(DX_2(1, 1)) > 0.00001 || fabs(DX_2(2, 1)) > 0.00001
		|| fabs(DX_2(3, 1)) > 0.00001 || fabs(DX_2(4, 1)) > 0.00001 || fabs(DX_2(5, 1)) > 0.00001
		|| fabs(DX_2(6, 1)) > 0.00001 || fabs(DX_2(7, 1)) > 0.00001)
	{
		//再赋值
		for (int i = 0; i < nCtrlCount; i++)
		{
			Ai[i] = X_2(8, 0) * controlPoints.at(i).X + X_2(9, 0) * controlPoints.at(i).Y + X_2(10, 0) * controlPoints.at(i).Z + 1.0;
			double r = sqrt((controlPoints.at(i).imageX - halfWidth) * (controlPoints.at(i).imageX - halfWidth) + (controlPoints.at(i).imageY - halfHeight) * (controlPoints.at(i).imageY - halfHeight));
			double X0, Y0, Z0;
			X0 = X_2(0, 0) * controlPoints.at(i).X + X_2(1, 0) * controlPoints.at(i).Y + X_2(2 , 0) * controlPoints.at(i).Z + X_2(3, 0);
			Y0 = X_2(4, 0) * controlPoints.at(i).X + X_2(5, 0) * controlPoints.at(i).Y + X_2(6 , 0) * controlPoints.at(i).Z + X_2(7, 0);
			Z0 = X_2(8, 0) * controlPoints.at(i).X + X_2(9, 0) * controlPoints.at(i).Y + X_2(10, 0) * controlPoints.at(i).Z + 1.0;

			xx[i] = -X0 / Z0;
			yy[i] = -Y0 / Z0;

			xx[i] -= (xx[i] - halfWidth) * r * r * X_2(11, 0);
			yy[i] -= (xx[i] - halfHeight) * r * r * X_2(11, 0);

			A_2(i * 2 + 0, 0 ) = controlPoints.at(i).X / Ai[i];
			A_2(i * 2 + 0, 1 ) = controlPoints.at(i).Y / Ai[i];
			A_2(i * 2 + 0, 2 ) = controlPoints.at(i).Z / Ai[i];
			A_2(i * 2 + 0, 3 ) = 1.0 / Ai[i];
			A_2(i * 2 + 0, 4 ) = 0;
			A_2(i * 2 + 0, 5 ) = 0;
			A_2(i * 2 + 0, 6 ) = 0;
			A_2(i * 2 + 0, 7 ) = 0;
			A_2(i * 2 + 0, 8 ) = controlPoints.at(i).imageX * controlPoints.at(i).X / Ai[i];
			A_2(i * 2 + 0, 9 ) = controlPoints.at(i).imageX * controlPoints.at(i).Y / Ai[i];
			A_2(i * 2 + 0, 10) = controlPoints.at(i).imageX * controlPoints.at(i).Z / Ai[i];
			A_2(i * 2 + 0, 11) = (controlPoints.at(i).imageX - halfWidth) * r * r;
			L_2(i * 2 + 0, 0 ) = -(controlPoints.at(i).imageX - xx[i]) / Ai[i];

			A_2(i * 2 + 1, 0 ) = 0;
			A_2(i * 2 + 1, 1 ) = 0;
			A_2(i * 2 + 1, 2 ) = 0;
			A_2(i * 2 + 1, 3 ) = 0;
			A_2(i * 2 + 1, 4 ) = controlPoints.at(i).X / Ai[i];
			A_2(i * 2 + 1, 5 ) = controlPoints.at(i).Y / Ai[i];
			A_2(i * 2 + 1, 6 ) = controlPoints.at(i).Z / Ai[i];
			A_2(i * 2 + 1, 7 ) = 1.0 / Ai[i];
			A_2(i * 2 + 1, 8 ) = controlPoints.at(i).imageY * controlPoints.at(i).X / Ai[i];
			A_2(i * 2 + 1, 9 ) = controlPoints.at(i).imageY * controlPoints.at(i).Y / Ai[i];
			A_2(i * 2 + 1, 10) = controlPoints.at(i).imageY * controlPoints.at(i).Z / Ai[i];
			A_2(i * 2 + 1, 11) = (controlPoints.at(i).imageY - halfHeight) * r * r;
			L_2(i * 2 + 1, 0 ) = -(controlPoints.at(i).imageY - yy[i]) / Ai[i];
		}
		//赋值完毕
		A_2.Transpose(AT_2);
		//ATA_2 = AT_2 * A_2;
		AT_2.Multiply(A_2, ATA_2);
		//ATL_2 = AT_2 * L_2;
		AT_2.Multiply(L_2, ATL_2);
		
		ATA_2.InvertGaussJordan(ATA_INVERSE_2);

		//DX_2 = ATA_INVERSE_2 * ATL_2;
		ATA_INVERSE_2.Multiply(ATL_2, DX_2);

		X_2(0, 0 ) += DX_2(0, 0 );
		X_2(1, 0 ) += DX_2(1, 0 );
		X_2(2, 0 ) += DX_2(2, 0 );
		X_2(3, 0 ) += DX_2(3, 0 );
		X_2(4, 0 ) += DX_2(4, 0 );
		X_2(5, 0 ) += DX_2(5, 0 );
		X_2(6, 0 ) += DX_2(6, 0 );
		X_2(7, 0 ) += DX_2(7, 0 );
		X_2(8, 0 ) += DX_2(8, 0 );
		X_2(9, 0 ) += DX_2(9, 0 );
		X_2(10, 0) += DX_2(10, 0);
		X_2(11, 0) += DX_2(11, 0);

		nTime++;
		if (nTime > 500)          //次数超限
			break;
	}
	M[0 ] = X_2(0 , 0);
	M[1 ] = X_2(1 , 0);
	M[2 ] = X_2(2 , 0);
	M[3 ] = X_2(3 , 0);
	M[4 ] = X_2(4 , 0);
	M[5 ] = X_2(5 , 0);
	M[6 ] = X_2(6 , 0);
	M[7 ] = X_2(7 , 0);
	M[8 ] = X_2(8 , 0);
	M[9 ] = X_2(9 , 0);
	M[10] = X_2(10, 0);
	M[11] = 1;
	m_k1 = X_2(11, 0);

	m_bMComputed = true;

	delete []Ai;
	delete []xx;
	delete []yy;

	//计算方位元素
	ComputeOrientation();
}	 


bool CDirectLinearTrans::GetPixelByXYZ( PanoControlPoint &point )
{
	bool bRet = false;
	double imageX, imageY;
	bRet = GetPixelByXYZ(point.X,point.Y,point.Z,imageX,imageY);
	point.imageX = (int)hd_round(imageX);
	point.imageY = (int)hd_round(imageY);
	return bRet;
}

bool CDirectLinearTrans::GetPixelByXYZ(double x,double y,double z,double& imageX,double& imageY)
{
	//判断M矩阵是否已经计算
	if (!m_bMComputed)
	{
		return false;
	}
	double X0 = M[0] * x + M[1] * y + M[2 ] * z + M[3];
	double Y0 = M[4] * x + M[5] * y + M[6 ] * z + M[7];
	double Z0 = M[8] * x + M[9] * y + M[10] * z + M[11];

	//判断有点问题，对两套数据的效果正好相反
	if (fabs(Z0) <= 0.000001 )//判断物方点是否会投影到像平面上 2012-7-30 10:48:49
	{
		return false;
	}
	double dImgX = -X0 / Z0;
	double dImgY = -Y0 / Z0;

	double halfWidth = m_imgWidth / 2.0;
	double halfHeight = m_imgHeight / 2.0;

	double r = sqrt((dImgX - halfWidth) * (dImgX - halfWidth) + (dImgY - halfHeight) * (dImgY - halfHeight));
	//imageX = hd_round(dImgX - (dImgX - halfWidth) * r * r * m_k1);
	//imageY = hd_round(dImgY - (dImgY - halfHeight) * r * r * m_k1);
	imageX = dImgX - (dImgX - halfWidth) * r * r * m_k1;
	imageY = dImgY - (dImgY - halfHeight) * r * r * m_k1;

	return(imageX > 0 && imageX < m_imgWidth && imageY > 0 && imageY < m_imgHeight);	
}

bool CDirectLinearTrans::ComputeOrientation()
{
	if (!m_bMComputed)
	{
		return false;
	}
	//////////////////////////////////////////////////////////////////////////
	//内方位元素
	/*
	******************************************
	r3^2 = 1 / (l9^2 + l10^2 + l11^2)
	x0 = -(l1*l9 + l2*l10 + l3*l11)*r3^2
	y0 = -(l5*l9 + l6*l10 + l7*l11)*r3^2
	******************************************
	*/
	double r3_2 = 1/(M[8]*M[8] + M[9]*M[9] + M[10]*M[10]);
	m_intOrt.x0 = -(M[0]*M[8] + M[1]*M[9] + M[2]*M[10]) * r3_2;
	m_intOrt.y0 = -(M[4]*M[8] + M[5]*M[9] + M[6]*M[10]) * r3_2;

	/*
	******************************************
	_A = r3^2(l1^2 + l2^2 + l3^3) - x0^2
	B  = r3^2(l5^2 + l6^2 + l7^3) - y0^2 
	C  = r3^2(l1*l5 + l2*l6 + l3*l7) - x0*y0 
	fx = sqrt((_A*B - C^2)/B)
	fy = sqrt((_A*B - C^2)/_A)
	******************************************
	*/
	double _A = r3_2 * (M[0]*M[0] + M[1]*M[1] + M[2]*M[2]) - m_intOrt.x0*m_intOrt.x0;
	double B  = r3_2 * (M[4]*M[4] + M[5]*M[5] + M[6]*M[6]) - m_intOrt.y0*m_intOrt.y0;
	double C  = r3_2 * (M[0]*M[4] + M[1]*M[5] + M[2]*M[6]) - m_intOrt.x0*m_intOrt.y0;
	m_intOrt.dBeta = asin(C*C/(_A*B));
	m_intOrt.ds = sqrt(_A/B) - 1;
	m_intOrt.fx = sqrt(_A)* cos(m_intOrt.dBeta);
	m_intOrt.fy = m_intOrt.fx/(1 + m_intOrt.ds);

	//////////////////////////////////////////////////////////////////////////
	//外方位元素

	hdMatrix X(3,1);//外方位线元素
	hdMatrix A(3,3);
	hdMatrix L(3,1);
	///*
	///******************************************
	//l1*Xs + l2 *Ys + l3 *Zs = -l4
	//l5*Xs + l6 *Ys + l7 *Zs =  l8
	//l9*Xs + l10*Ys + l11*Zs = -1
	//******************************************
	//*/
	L(0,0) = -M[3];
	L(1,0) = -M[7];
	L(2,0) = -1;

	A(0,0) = M[0];
	A(0,1) = M[1];
	A(0,2) = M[2];
	A(1,0) = M[4];
	A(1,1) = M[5];
	A(1,2) = M[6];
	A(2,0) = M[8];
	A(2,1) = M[9];
	A(2,2) = M[10];

	hdMatrix A_inv(A);
	A.InvertGaussJordan(A_inv);
	A_inv.Multiply(L,X);
	m_extOrt.Xs = X(0,0);
	m_extOrt.Ys = X(1,0);
	m_extOrt.Zs = X(2,0);

	/*
	******************************************
	a3 = l9  / (19^2 + l10^2 + 111^2)^(1/2)
	b3 = l10 / (19^2 + l10^2 + 111^2)^(1/2)
	c3 = l11 / (19^2 + l10^2 + 111^2)^(1/2)	
	l6 = [b2*fx/[(1+ds)*cos(dβ)] - a3*y0]/r3
	l2 = (b1*fx - b2*fx*tan(dβ) - b3*x0)/r3
	tan(phi) = a3/c3
	sin(omega) = -b3
	tan(kappa) = b1/b2
	******************************************
	*/
	double r3 = sqrt(r3_2);
	double a3 = M[8] * r3;
	double b3 = M[9] * r3;
	double c3 = M[10] * r3;
	double b2 = r3 * (M[6] + M[9]*m_intOrt.y0) * (1 + m_intOrt.ds) * cos(m_intOrt.dBeta) / m_intOrt.fx;
	double b1 = (M[1]*r3 + b3*m_intOrt.x0 + b2*m_intOrt.fx*tan(m_intOrt.dBeta)) / m_intOrt.fx;

	m_extOrt.phi = atan2(a3, c3);
	m_extOrt.omega = asin(-b3);
	m_extOrt.kappa = atan2(b1, b2);
	return true;
}
