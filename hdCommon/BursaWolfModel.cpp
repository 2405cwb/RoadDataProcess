#include "StdAfx.h"
#include "BursaWolfModel.h"
#include "..\hdCore\Matrix.h"
#include "..\hdCore\hdMath.h"
#include "..\hdCore\hdMatrix.h"
#include "..\hdCore\hdMatrix4.h"
#include "..\hdCore\hdVector2d.h"
#include "point_types.h"
#include "eigen.h"

#include <string>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif
using namespace std;
using namespace hd;
using namespace Eigen;

namespace hd
{

CBursaWolfModel::CBursaWolfModel(void)
{
	getIdentity();
	//memset(m_dGravityCenter, 0, 3*sizeof(double));
}

CBursaWolfModel::CBursaWolfModel(const CBursaWolfModel& model)
{
	*this = model;
}


CBursaWolfModel::~CBursaWolfModel(void)
{
	
}

void CBursaWolfModel::getIdentity()
{
	m_fScale = 1.0f;
	m_fError = 0.0f;		
	memset(m_fOffset, 0, 3*sizeof(double));
	memset(m_fAngle, 0, 3*sizeof(double));

	memset(m_fRotateMatrix, 0, 9*sizeof(double));
	m_fRotateMatrix[0] = 1.0f;
	m_fRotateMatrix[4] = 1.0f;
	m_fRotateMatrix[8] = 1.0f;

	identity_matrix((double*)m_matrix);
}

 CBursaWolfModel& CBursaWolfModel::operator=(const CBursaWolfModel& model)
{
	m_fScale = model.m_fScale;
	m_fError = model.m_fError;		
	memcpy(m_fOffset, model.m_fOffset, 3*sizeof(double));
	//memcpy(m_dGravityCenter, model.m_dGravityCenter, 3*sizeof(double));
	memcpy(m_fAngle, model.m_fAngle, 3*sizeof(double));
	memcpy(m_fRotateMatrix, model.m_fRotateMatrix, 9*sizeof(double));

	memcpy(m_matrix,model.m_matrix,sizeof(double) * 16);
	return *this;
}

bool CBursaWolfModel::operator==(const CBursaWolfModel& model) const
{
	if ((equals(m_fScale,model.m_fScale))
		&&(equals(m_fAngle[0],model.m_fAngle[0]))&&(equals(m_fAngle[1],model.m_fAngle[1]))&&(equals(m_fAngle[2],model.m_fAngle[2]))
		&&(equals(m_fOffset[0],model.m_fOffset[0]))&&(equals(m_fOffset[1],model.m_fOffset[1]))&&(equals(m_fOffset[2],model.m_fOffset[2]))
		)
	{
		return true;
	}
	return false;
}

bool CBursaWolfModel::IsIdentity() const 
{
	if ((m_fScale == 1.0f) && (m_fError == 0.0f)	
		&&(m_fAngle[0] == 0.0f)&&(m_fAngle[1] == 0.0f)&&(m_fAngle[2] == 0.0f)
		&&(m_fOffset[0] == 0.0f)&&(m_fOffset[1] == 0.0f)&&(m_fOffset[2] == 0.0f)
		&&(m_fRotateMatrix[0] == 1.0f)&&(m_fRotateMatrix[1] == 0.0f)&&(m_fRotateMatrix[2] == 0.0f)
		&&(m_fRotateMatrix[3] == 0.0f)&&(m_fRotateMatrix[4] == 1.0f)&&(m_fRotateMatrix[5] == 0.0f)
		&&(m_fRotateMatrix[6] == 0.0f)&&(m_fRotateMatrix[7] == 0.0f)&&(m_fRotateMatrix[8] == 1.0f)
		&&(m_matrix[0][0] == 1.0f)&&(m_matrix[1][1] == 1.0f)&&(m_matrix[2][2] == 1.0f)&&(m_matrix[3][3] == 1.0f)
		&&(m_matrix[0][1] == 0.0f)&&(m_matrix[1][0] == 0.0f)&&(m_matrix[2][0] == 0.0f)&&(m_matrix[3][0] == 0.0f)
		&&(m_matrix[0][2] == 0.0f)&&(m_matrix[1][2] == 0.0f)&&(m_matrix[2][1] == 0.0f)&&(m_matrix[3][1] == 0.0f)
		&&(m_matrix[0][3] == 0.0f)&&(m_matrix[1][3] == 0.0f)&&(m_matrix[2][3] == 0.0f)&&(m_matrix[3][2] == 0.0f)
		)
	{
		return true;
	}

	return false;
}

CBursaWolfModel CBursaWolfModel::operator*(const CBursaWolfModel& model) const 
{
	CBursaWolfModel newModel = *this;

	mult((double*)this->m_matrix,(double*)model.m_matrix,(double*)newModel.m_matrix,4,4,4);
	// 缩放系数
	newModel.m_fScale = this->m_fScale*model.m_fScale;
	// 将矩阵转换为6参数
	newModel.matrix2Parameter();
	return newModel;

	//如果没有任何转换，则直接赋值
	bool bCurIdentity = IsIdentity();
	bool bModelIdentity = model.IsIdentity();
	if (bCurIdentity || bModelIdentity)
	{
		newModel = bModelIdentity?*this:model;
	}
	else
	{
		//等效的缩放参数
		newModel.m_fScale = m_fScale*model.m_fScale;
		//等效的旋转矩阵
		mult(m_fRotateMatrix, model.m_fRotateMatrix, newModel.m_fRotateMatrix, 3, 3, 3);

		//等效的平移量
		/*mult(m_fRotateMatrix, model.m_fOffset, newModel.m_fOffset, 3, 3, 1);
		newModel.m_fOffset[0] *= m_fScale;
		newModel.m_fOffset[1] *= m_fScale;
		newModel.m_fOffset[2] *= m_fScale;
		newModel.m_fOffset[0] += m_fOffset[0];
		newModel.m_fOffset[1] += m_fOffset[1];
		newModel.m_fOffset[2] += m_fOffset[2];*/
		newModel.m_fOffset[0] = model.m_fOffset[0];
		newModel.m_fOffset[1] = model.m_fOffset[1];
		newModel.m_fOffset[2] = model.m_fOffset[2];
		//
		/*newModel.m_dGravityCenter[0] = model.m_dGravityCenter[0];
		newModel.m_dGravityCenter[1] = model.m_dGravityCenter[1];
		newModel.m_dGravityCenter[2] = model.m_dGravityCenter[2];*/
		Translate(newModel.m_fOffset[0], newModel.m_fOffset[1], newModel.m_fOffset[2]);
		
		//等效旋转角——待计算，在实际坐标转换中，并不会用到此参数
		//TODO: calculate angles
		newModel.m_fAngle[0] = m_fAngle[0] + model.m_fAngle[0];
		newModel.m_fAngle[1] = m_fAngle[1] + model.m_fAngle[1];
		newModel.m_fAngle[2] = m_fAngle[2] + model.m_fAngle[2];

		//转换到[-pi,pi]之间
		for (int i = 0;i< 3;i++)
		{
			if (newModel.m_fAngle[i]> PI64)
			{
				newModel.m_fAngle[i] -= 2*PI64;
			}
			if (newModel.m_fAngle[i]< -PI64)
			{
				newModel.m_fAngle[i] += 2*PI64;
			}
		}
		//累积的误差——待计算，在实际坐标转换中，并不会用到此参数
		//TODO: calculate errors
	}

	return newModel;
}

//bool CBursaWolfModel::CreateNew(const double* pRefX, const double* pRefY, const double* pRefZ, const double* pRegX, const double* pRegY, const double* pRegZ, int n)
//{
//	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (n == 0))
//	{
//		return false;
//	}
//
//	double *RefX  = new double[n];
//	double *RefY = new double[n];
//	double *RefZ = new double[n];
//	double *RegX = new double[n];
//	double *RegY = new double[n];
//	double *RegZ = new double[n];
//
//	double *V = new double[n*3];
//	for (int j= 0;j< n;j++)
//	{
//		RefX[j] = pRefX[j];
//		RefY[j] = pRefY[j];
//		RefZ[j] = pRefZ[j];
//
//		RegX[j] = pRegX[j];
//		RegY[j] = pRegY[j];
//		RegZ[j] = pRegZ[j];
//	}
//
//	/////////////////////////////////////////
//	//测试求解7参数代码
//	//1.坐标重心化
//	double sumX,sumY,sumZ;
//	sumX = sumY = sumZ = 0;
//	double sumx,sumy,sumz;
//	sumx = sumy = sumz = 0;
//	int i = 0;
//	for (i = 0;i< n;i++)
//	{
//		sumx += RegX[i];
//		sumy += RegY[i];
//		sumz += RegZ[i];
//
//		sumX += RefX[i];
//		sumY += RefY[i];
//		sumZ += RefZ[i];
//	}
//
//	//坐标重心
//	double xg = sumx/n;
//	double yg = sumy/n;
//	double zg = sumz/n;
//
//	double Xtpg = sumX/n;
//	double Ytpg = sumY/n;
//	double Ztpg = sumZ/n;
//
//	//m_dGravityCenter[0] = Xtpg;
//	//m_dGravityCenter[1] = Ytpg;
//	//m_dGravityCenter[2] = Ztpg;
//	//重心坐标
//	for (i = 0;i< n;i++)
//	{
//		RegX[i] -= xg;
//		RegY[i] -= yg;
//		RegZ[i] -= zg;
//
//		RefX[i] -= Xtpg;
//		RefY[i] -= Ytpg;
//		RefZ[i] -= Ztpg;
//	}
//
//	int N = 3*n;
//	int rol = N;
//	int col = 7;
//
//	double *L=new double[rol*1];
//	double *A=new double[rol*col];
//	double *x=new double[col*1];
//
//	double *At	=new double[col*rol];
//	double *AtA	=new double[col*col];
//	double *_AtA=new double[col*col];
//	double *AtL	=new double[col*1];
//	double *Vt  =new double[rol*1];
//
//	//计算kappa角初值
//	double scale0 = 0.0f;
//	for (i = 0;i< n;i++)
//	{
//		double deltaX = RefX[(i+1)%n] - RefX[i];
//		double deltaY = RefY[(i+1)%n] - RefY[i];
//		double deltaZ = RefZ[(i+1)%n] - RefZ[i];
//
//		double deltax = RegX[(i+1)%n] - RegX[i];
//		double deltay = RegY[(i+1)%n] - RegY[i];
//		double deltaz = RegZ[(i+1)%n] - RegZ[i];
//
//		m_fAngle[2] = atan((RefY[1] - RefY[0])/(RefX[1] - RefX[0]))-atan((RegY[1] - RegY[0])/(RegX[1] - RegX[0]));
//		m_fAngle[2] = -m_fAngle[2];
//		m_fScale = sqrt(pow(deltaX,2)+ pow(deltaY,2) + pow(deltaZ,2))/sqrt(pow(deltax,2) + pow(deltay,2) + pow(deltaz,2));
//		scale0 += m_fScale;
//		// 	printf("%lf %lf\n",st.kappa,st.lamabda);
//	}
//	m_fScale = 1;
//
//	int iteration;
//	for(iteration = 0;iteration< 100;iteration++)
//	{
//		//2、计算和未知数系数
//		for (i = 0;i< n;i++)
//		{
//			double R[3][3];
//			double phi,omega,kappa;
//
//			phi = m_fAngle[0];omega = m_fAngle[1];kappa = m_fAngle[2];
//
//			ComputeRotateMatrixByAngle(phi,omega,kappa,&R[0][0]);
//
//			double X1 = R[0][0]*RegX[i] + R[0][1]*RegY[i] + R[0][2]*RegZ[i];
//			double Y1 = R[1][0]*RegX[i] + R[1][1]*RegY[i] + R[1][2]*RegZ[i];
//			double Z1 = R[2][0]*RegX[i] + R[2][1]*RegY[i] + R[2][2]*RegZ[i];
//
//			//常数项
//			L[i*3 + 0] = RefX[i] - m_fScale*X1 - m_fOffset[0];
//			L[i*3 + 1] = RefY[i] - m_fScale*Y1 - m_fOffset[1];
//			L[i*3 + 2] = RefZ[i] - m_fScale*Z1 - m_fOffset[2];
//
//			//严密系数
//			A[i*3*col + 0] = X1;
//			A[i*3*col + 1] = -m_fScale*Z1;
//			A[i*3*col + 2] = -m_fScale*Y1*sin(phi);
//			A[i*3*col + 3] = -m_fScale*Y1*cos(phi)*cos(omega)-m_fScale*Z1*sin(omega);
//			A[i*3*col + 4] = 1;
//			A[i*3*col + 5] = 0;
//			A[i*3*col + 6] = 0;
//			
//			A[i*3*col + col + 0] = Y1;
//			A[i*3*col + col + 1] = 0;
//			A[i*3*col + col + 2] = -m_fScale*X1*sin(phi) - m_fScale*Z1*cos(phi);
//			A[i*3*col + col + 3] =  m_fScale*X1*cos(phi)*cos(omega) + m_fScale*Z1*sin(phi)*cos(omega);
//			A[i*3*col + col + 4] = 0;
//			A[i*3*col + col + 5] = 1;
//			A[i*3*col + col + 6] = 0;
//
//			A[i*3*col + 2*col + 0] = Z1;
//			A[i*3*col + 2*col + 1] = m_fScale*X1;
//			A[i*3*col + 2*col + 2] = m_fScale*Y1*cos(phi);
//			A[i*3*col + 2*col + 3] = m_fScale*X1*sin(omega) - m_fScale*Y1*sin(phi)*cos(omega);
//			A[i*3*col + 2*col + 4] = 0;
//			A[i*3*col + 2*col + 5] = 0;
//			A[i*3*col + 2*col + 6] = 1;
//		}
//		//解方程
//		transpose(A,At,rol,col);
//		mult(At,A,AtA,col,rol,col);
//		invers_matrix(AtA,col);
//		mult(At,L,AtL,col,rol,1);
//		mult(AtA,AtL,x,col,col,1);
//	
//		//MatTrs(A,col,col,At);
//		//MatMult(At,A,AtA,col,rol,col);
//		//MatInv(AtA,col);
//		//MatMult(At,L,AtL,col,rol,1);
//		//MatMult(AtA,AtL,x,col,col,1);
//	
//		//if (fabs(x[0])>1)
//		//{
//		//	return false;
//		//}
//
//		//判断是否迭代
//		if(	/* fabs(x[0])< 1e-4*/
//			fabs(x[1])< 1e-4
//			&&fabs(x[2])< 1e-4
//			&&fabs(x[3])< 1e-4
//			&&fabs(x[4])< 1e-4
//			&&fabs(x[5])< 1e-4
//			&&fabs(x[6])< 1e-4)
//		{
//			//计算精度
//			double *Ax = new double[N*1];
//			mult(A,x,Ax,N,col,1);
//			//MatMult(A,x,Ax,N,col,1);
//			MatMinus(Ax,L,V,N,1);
//			transpose(V,Vt,N,1);
//			//MatTrs(V,N,1,Vt);
//			double VtV;
//			mult(Vt,V,&VtV,1,N,1);
//			//MatMult(Vt,V,&VtV,1,N,1);
//			double sigma = sqrt(VtV/(N-col));
//			m_fError = sigma;
//			double *Q = new double[col];
//			for (i = 0;i< col;i++)
//			{
//				Q[i] = sigma*AtA[i*col + i];
//			//	precision[i] = Q[i];
//			//  printf("%lf\n",Q[i]);		//输出精度
//			}
//			delete []Ax;
//			delete []Q;
//			break;
//		}
//		//更新待定参数的新值
//		else
//		{
//			m_fScale = (float)(m_fScale*(1 + x[0]));
//			m_fAngle[0] += x[1];
//			m_fAngle[1] += x[2];
//			m_fAngle[2] += x[3];
//		}
//	}
//
//	if (iteration == 100)
//	{
//		return false;
//	}
//
//	double sumDX,sumDY,sumDZ;
//	sumDX = sumDY = sumDZ=0;
//
//	//归化到[-pi,pi]
//	if (m_fAngle[2]> PI64)
//	{
//		m_fAngle[2] -= 2*PI64;
//	}
//	if (m_fAngle[2]< -PI64)
//	{
//		m_fAngle[2] += 2*PI64;
//	}
//
//	ComputeRotateMatrixByAngle(m_fAngle[0],m_fAngle[1],m_fAngle[2],&m_fRotateMatrix[0]);
//
//	sumDX = sumDY = sumDZ = 0;
//	//将点坐标恢复
//	for (i = 0;i< n;i++)
//	{
//		RegX[i] += xg;
//		RegY[i] += yg;
//		RegZ[i] += zg;
//
//		RefX[i] += Xtpg;
//		RefY[i] += Ytpg;
//		RefZ[i] += Ztpg;
//
//		m_fOffset[0] = RefX[i] - m_fScale*(m_fRotateMatrix[0]*RegX[i] + m_fRotateMatrix[1]*RegY[i] + m_fRotateMatrix[2]*RegZ[i]);
//		m_fOffset[1] = RefY[i] - m_fScale*(m_fRotateMatrix[3]*RegX[i] + m_fRotateMatrix[4]*RegY[i] + m_fRotateMatrix[5]*RegZ[i]);
//		m_fOffset[2] = RefZ[i] - m_fScale*(m_fRotateMatrix[6]*RegX[i] + m_fRotateMatrix[7]*RegY[i] + m_fRotateMatrix[8]*RegZ[i]);
//
//		sumDX += m_fOffset[0];
//		sumDY += m_fOffset[1];
//		sumDZ += m_fOffset[2];
//	}
//	
//	m_fOffset[0] = sumDX/n;
//	m_fOffset[1] = sumDY/n;
//	m_fOffset[2] = sumDZ/n;
//
//	//给模型中的矩阵赋值
//	m_matrix[0][0] = m_fRotateMatrix[0];
//	m_matrix[0][1] = m_fRotateMatrix[1];
//	m_matrix[0][2] = m_fRotateMatrix[2];
//
//	m_matrix[1][0] = m_fRotateMatrix[3];
//	m_matrix[1][1] = m_fRotateMatrix[4];
//	m_matrix[1][2] = m_fRotateMatrix[5];
//
//	m_matrix[2][0] = m_fRotateMatrix[6];
//	m_matrix[2][1] = m_fRotateMatrix[7];
//	m_matrix[2][2] = m_fRotateMatrix[8];
//
//	m_matrix[0][3] = m_fOffset[0];
//	m_matrix[1][3] = m_fOffset[1];
//	m_matrix[2][3] = m_fOffset[2];
//
//	m_matrix[3][0] = 0.0;
//	m_matrix[3][1] = 0.0;
//	m_matrix[3][2] = 0.0;
//	m_matrix[3][3] = 1.0;
//
//	delete[] RefX;
//	delete[] RefY;
//	delete[] RefZ;
//	delete[] RegX;
//	delete[] RegY;
//	delete[] RegZ;
//
//	delete[]  L;
//	delete[]  A;
//	delete[]  x;
//	delete[]  V;
//
//	delete[]  At;
//	delete[]  AtA;
//	delete[]  _AtA;
//	delete[]  AtL;
//	delete[]  Vt;
//	return true;
//}
//
//bool CBursaWolfModel::Create(const float* pRefX, const float* pRefY, const float* pRefZ, const float* pRegX, const float* pRegY, const float* pRegZ, int n)
//{
//	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (n == 0))
//	{
//		return false;
//	}
//	///////////////////////////////
//	int i;
//	double* dx = new double[n];
//	double* dy = new double[n];
//	double* dz = new double[n];
//	double* mx = new double[n];
//	double* my = new double[n];
//	double* mz = new double[n];
//	for (i = 0; i<n; i++)
//	{
//		dx[i] = pRefX[i];
//		dy[i] = pRefY[i];
//		dz[i] = pRefZ[i];
//		mx[i] = pRegX[i];
//		my[i] = pRegY[i];
//		mz[i] = pRegZ[i];
//	}
//
//	//模型点重心
//	double xg,yg,zg,xp,yp,zp,xtp,ytp,ztp;;
//	xg=average(mx,n);
//	yg=average(my,n);
//	zg=average(mz,n);
//	xtp=average(dx,n);
//	ytp=average(dy,n);
//	ztp=average(dz,n);
//	
//	//
//	//m_dGravityCenter[0] = xtp;
//	//m_dGravityCenter[1] = ytp;
//	//m_dGravityCenter[2] = ztp;
//
//	for(i=0;i<n;i++)
//	{
//		dx[i]=dx[i]-xtp;
//		dy[i]=dy[i]-ytp;
//		dz[i]=dz[i]-ztp;
//	}
//	
//	///////////////////初值/////////////////////
//	double phi,kappa,omega;
//	phi=0.0,kappa=0.0,omega=0.0;
//	m_fScale=1.0;
//	double X0,Y0,Z0;
//	X0=mx[0]-dx[0];
//	Y0=my[0]-dy[0];
//	Z0=mz[0]-dz[0];
//
//	double *a = new double[n];
//	double *b = new double[n];
//	double *c = new double[n];
//	double dphi,dkappa,domega,dscale;
//	double *l = new double[3*n];
//	double *A = new double[21*n];
//	double *AT= new double [21*n];
//	double *ATA = new double [7*7];
//	double *B = new double [21*n];//B=(ATA)-1*AT
//	double x[7];
//	double a1,a2,a3,b1,b2,b3,c1,c2,c3;
//	double *X= new double[n];
//	double *Y= new double[n];
//	double *Z= new double[n];
//	double *V = new double [3*n];
//	double *VT = new double [3*n];
//	double VTV[1];
//	
//	//迭代
//	int m=0;
//	while(m<1000)
//	{
//		a1=cos(phi)*cos(kappa)-sin(phi)*sin(omega)*sin(kappa);
//		a2=-cos(phi)*sin(kappa)-sin(phi)*sin(omega)*cos(kappa);
//		a3=-sin(phi)*cos(omega);
//		b1=cos(omega)*sin(kappa);
//		b2=cos(omega)*cos(kappa);
//		b3=-sin(omega);
//		c1=sin(phi)*cos(kappa)+cos(phi)*sin(omega)*sin(kappa);
//		c2=-sin(phi)*sin(kappa)+cos(phi)*sin(omega)*cos(kappa);
//		c3=cos(phi)*cos(omega);
//
//		m_fRotateMatrix[0]=(float)a1;
//		m_fRotateMatrix[1]=(float)a2;
//		m_fRotateMatrix[2]=(float)a3;
//		m_fRotateMatrix[3]=(float)b1;
//		m_fRotateMatrix[4]=(float)b2;
//		m_fRotateMatrix[5]=(float)b3;
//		m_fRotateMatrix[6]=(float)c1;
//		m_fRotateMatrix[7]=(float)c2;
//		m_fRotateMatrix[8]=(float)c3;
//		////////////////模型坐标相似变换//////////////////////////
//		for(i=0;i<n;i++)  
//		{				  
//			a[i]=m_fScale*(a1*mx[i]+a2*my[i]+a3*mz[i])+X0;
//			b[i]=m_fScale*(b1*mx[i]+b2*my[i]+b3*mz[i])+Y0;
//			c[i]=m_fScale*(c1*mx[i]+c2*my[i]+c3*mz[i])+Z0;
//		}
//		xp=average(a,n);
//		yp=average(b,n);
//		zp=average(c,n);
//		
//		/////////////////////坐标重心化/////////////////////////////////
//		for (i=0;i<n;i++)
//		{
//			a[i]=a[i]-xp;
//			b[i]=b[i]-yp;
//			c[i]=c[i]-zp;
//		}
//		///////////////////////////////////////////////////////
//		for( i=0;i<n;i++)
//		{
//			l[3*i]=dx[i]-a[i];
//			l[3*i+1]=dy[i]-b[i];
//			l[3*i+2]=dz[i]-c[i];
//		}
//		
//		for(int t=0;t<n;t++)
//		{
//			A[0+21*t]=1;
//			A[1+21*t]=0;
//			A[2+21*t]=0;
//			A[3+21*t]=a[t];
//			A[4+21*t]=-c[t];
//			A[5+21*t]=0;
//			A[6+21*t]=-b[t];
//			A[7+21*t]=0;
//			A[8+21*t]=1;
//			A[9+21*t]=0;
//			A[10+21*t]=b[t];
//			A[11+21*t]=0;
//			A[12+21*t]=-c[t];
//			A[13+21*t]=a[t];
//			A[14+21*t]=0;
//			A[15+21*t]=0;
//			A[16+21*t]=1;
//			A[17+21*t]=c[t];
//			A[18+21*t]=a[t];
//			A[19+21*t]=b[t];
//			A[20+21*t]=0;
//		}
//		
//		transpose( &A[0],&AT[0],3*n,7); 
//		mult(&AT[0],&A[0],&ATA[0],7,3*n,7);
//		invers_matrix(&ATA[0],7);//ATA-1
//		mult(&ATA[0],&AT[0],&B[0],7,7,3*n);
//		mult(&B[0],&l[0],&x[0],7,3*n,1);
//		//////////////////////////////////////////////////
//		
//		dscale=x[3] , dphi=x[4] , domega=x[5] , dkappa=x[6];
//		m_fScale=(float)(m_fScale*(1+dscale));
//		phi=phi+dphi, omega=omega+domega, kappa=kappa+dkappa;
//		if(fabs(dphi)<10e-10 && fabs(domega)<10e-10 && fabs(dkappa)<10e-10)
//			break;
//		m++;
//	}
//	
//	m_fAngle[0]=(float)phi;
//	m_fAngle[1]=(float)omega;
//	m_fAngle[2]=(float)kappa;
//	
//	////////////////////////精度评定/////////////////////////////////////
//	double XS,YS,ZS;
//	XS=xtp-m_fScale*(a1*xg+a2*yg+a3*zg);
//	YS=ytp-m_fScale*(b1*xg+b2*yg+b3*zg);
//	ZS=ztp-m_fScale*(c1*xg+c2*yg+c3*zg);
//
//	m_fOffset[0]=(float)XS;
//	m_fOffset[1]=(float)YS;
//	m_fOffset[2]=(float)ZS;
//
//	for(i=0;i<n;i++)
//	{
//		X[i] = m_fScale*(a1*mx[i]+a2*my[i]+a3*mz[i])+XS;
//		Y[i] = m_fScale*(b1*mx[i]+b2*my[i]+b3*mz[i])+YS;
//		Z[i] = m_fScale*(c1*mx[i]+c2*my[i]+c3*mz[i])+ZS;
//	}
//
//	// 此处进行了坐标去重心化处理
//	for(i=0;i<n;i++)
//	{
//		V[3*i]=dx[i]+xtp-X[i];
//		V[3*i+1]=dy[i]+ytp-Y[i];
//		V[3*i+2]=dz[i]+ztp-Z[i];
//	}
//
//	transpose(&V[0],&VT[0],3*n,1);
//	mult(&VT[0],&V[0],&VTV[0],1,3*n,1);
//	int q=(3*n-7);
//	m_fError=(float)(sqrt(fabs(VTV[0])/q));
//
//	m_matrix[0][0] = m_fRotateMatrix[0];
//	m_matrix[0][1] = m_fRotateMatrix[1];
//	m_matrix[0][2] = m_fRotateMatrix[2];
//
//	m_matrix[1][0] = m_fRotateMatrix[3];
//	m_matrix[1][1] = m_fRotateMatrix[4];
//	m_matrix[1][2] = m_fRotateMatrix[5];
//
//	m_matrix[2][0] = m_fRotateMatrix[6];
//	m_matrix[2][1] = m_fRotateMatrix[7];
//	m_matrix[2][2] = m_fRotateMatrix[8];
//
//	m_matrix[0][3] = m_fOffset[0];
//	m_matrix[1][3] = m_fOffset[1];
//	m_matrix[2][3] = m_fOffset[2];
//
//	m_matrix[3][0] = 0.0;
//	m_matrix[3][1] = 0.0;
//	m_matrix[3][2] = 0.0;
//	m_matrix[3][3] = 1.0;
//
//	delete [] dx;
//	delete [] dy;
//	delete [] dz;
//	delete [] mx;
//	delete [] my;
//	delete [] mz;
//
//	delete []l ;
//	delete []A ;
//	
//	delete []V;
//	delete []VT;
//	delete []AT;
//	delete []ATA;
//	delete []B;
//	delete []a;
//	delete []b;
//	delete []c;
//	delete []X;
//	delete []Y;
//	delete []Z;
//
//	return true;
//}
//
//bool CBursaWolfModel::Create(const double* pRefX, const double* pRefY, const double* pRefZ, const double* pRegX, const double* pRegY, const double* pRegZ, int n)
//{
//	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (n == 0))
//	{
//		return false;
//	}
//	double *RefX  = new double[n];
//	double *RefY = new double[n];
//	double *RefZ = new double[n];
//	double *RegX = new double[n];
//	double *RegY = new double[n];
//	double *RegZ = new double[n];
//
//	double *V = new double[n*3];
//	for (int j= 0;j< n;j++)
//	{
//		RefX[j] = pRefX[j];
//		RefY[j] = pRefY[j];
//		RefZ[j] = pRefZ[j];
//
//		RegX[j] = pRegX[j];
//		RegY[j] = pRegY[j];
//		RegZ[j] = pRegZ[j];
//	}
//
//	/////////////////////////////////////////
//	//测试求解7参数代码
//	//1.坐标重心化
//	double sumX,sumY,sumZ;
//	sumX = sumY = sumZ = 0;
//	double sumx,sumy,sumz;
//	sumx = sumy = sumz = 0;
//	int i = 0;
//	for (i = 0;i< n;i++)
//	{
//		sumx += RegX[i];
//		sumy += RegY[i];
//		sumz += RegZ[i];
//
//		sumX += RefX[i];
//		sumY += RefY[i];
//		sumZ += RefZ[i];
//	}
//
//	//坐标重心
//	double xg = sumx/n;
//	double yg = sumy/n;
//	double zg = sumz/n;
//
//	double Xtpg = sumX/n;
//	double Ytpg = sumY/n;
//	double Ztpg = sumZ/n;
//
//	//m_dGravityCenter[0] = Xtpg;
//	//m_dGravityCenter[1] = Ytpg;
//	//m_dGravityCenter[2] = Ztpg;
//	//重心坐标
//	for (i = 0;i< n;i++)
//	{
//		RegX[i] -= xg;
//		RegY[i] -= yg;
//		RegZ[i] -= zg;
//
//		RefX[i] -= Xtpg;
//		RefY[i] -= Ytpg;
//		RefZ[i] -= Ztpg;
//	}
//
//	int N = 3*n;
//	int rol = N;
//	int col = 7;
//
//	double *L=new double[rol*1];
//	double *A=new double[rol*col];
//	double *x=new double[col*1];
//
//	double *At	=new double[col*rol];
//	double *AtA	=new double[col*col];
//	double *_AtA=new double[col*col];
//	double *AtL	=new double[col*1];
//	double *Vt  =new double[rol*1];
//
//	//计算kappa角初值
//	double scale0 = 0.0f;
//	for (i = 0;i< n;i++)
//	{
//		double deltaX = RefX[(i+1)%n] - RefX[i];
//		double deltaY = RefY[(i+1)%n] - RefY[i];
//		double deltaZ = RefZ[(i+1)%n] - RefZ[i];
//
//		double deltax = RegX[(i+1)%n] - RegX[i];
//		double deltay = RegY[(i+1)%n] - RegY[i];
//		double deltaz = RegZ[(i+1)%n] - RegZ[i];
//
//		m_fAngle[2] = atan((RefY[1] - RefY[0])/(RefX[1] - RefX[0]))-atan((RegY[1] - RegY[0])/(RegX[1] - RegX[0]));
//		m_fAngle[2] = -m_fAngle[2];
//		m_fScale = sqrt(pow(deltaX,2)+ pow(deltaY,2) + pow(deltaZ,2))/sqrt(pow(deltax,2) + pow(deltay,2) + pow(deltaz,2));
//		scale0 += m_fScale;
//		// 	printf("%lf %lf\n",st.kappa,st.lamabda);
//	}
//	m_fScale = 1;
//
//	int iteration;
//	for(iteration = 0;iteration< 100;iteration++)
//	{
//		//2、计算和未知数系数
//		for (i = 0;i< n;i++)
//		{
//			double R[3][3];
//			double phi,omega,kappa;
//
//			phi = m_fAngle[0];omega = m_fAngle[1];kappa = m_fAngle[2];
//
//			ComputeRotateMatrixByAngle(phi,omega,kappa,&R[0][0]);
//
//			double X1 = R[0][0]*RegX[i] + R[0][1]*RegY[i] + R[0][2]*RegZ[i];
//			double Y1 = R[1][0]*RegX[i] + R[1][1]*RegY[i] + R[1][2]*RegZ[i];
//			double Z1 = R[2][0]*RegX[i] + R[2][1]*RegY[i] + R[2][2]*RegZ[i];
//
//			//常数项
//			L[i*3 + 0] = RefX[i] - m_fScale*X1 - m_fOffset[0];
//			L[i*3 + 1] = RefY[i] - m_fScale*Y1 - m_fOffset[1];
//			L[i*3 + 2] = RefZ[i] - m_fScale*Z1 - m_fOffset[2];
//
//			//严密系数
//			A[i*3*col + 0] = X1;
//			A[i*3*col + 1] = -m_fScale*Z1;
//			A[i*3*col + 2] = -m_fScale*Y1*sin(phi);
//			A[i*3*col + 3] = -m_fScale*Y1*cos(phi)*cos(omega)-m_fScale*Z1*sin(omega);
//			A[i*3*col + 4] = 1;
//			A[i*3*col + 5] = 0;
//			A[i*3*col + 6] = 0;
//
//			A[i*3*col + col + 0] = Y1;
//			A[i*3*col + col + 1] = 0;
//			A[i*3*col + col + 2] = -m_fScale*X1*sin(phi) - m_fScale*Z1*cos(phi);
//			A[i*3*col + col + 3] =  m_fScale*X1*cos(phi)*cos(omega) + m_fScale*Z1*sin(phi)*cos(omega);
//			A[i*3*col + col + 4] = 0;
//			A[i*3*col + col + 5] = 1;
//			A[i*3*col + col + 6] = 0;
//
//			A[i*3*col + 2*col + 0] = Z1;
//			A[i*3*col + 2*col + 1] = m_fScale*X1;
//			A[i*3*col + 2*col + 2] = m_fScale*Y1*cos(phi);
//			A[i*3*col + 2*col + 3] = m_fScale*X1*sin(omega) - m_fScale*Y1*sin(phi)*cos(omega);
//			A[i*3*col + 2*col + 4] = 0;
//			A[i*3*col + 2*col + 5] = 0;
//			A[i*3*col + 2*col + 6] = 1;
//		}
//		//解方程
//		transpose(A,At,rol,col);
//		mult(At,A,AtA,col,rol,col);
//		invers_matrix(AtA,col);
//		mult(At,L,AtL,col,rol,1);
//		mult(AtA,AtL,x,col,col,1);
//
//		//MatTrs(A,col,col,At);
//		//MatMult(At,A,AtA,col,rol,col);
//		//MatInv(AtA,col);
//		//MatMult(At,L,AtL,col,rol,1);
//		//MatMult(AtA,AtL,x,col,col,1);
//
//		//if (fabs(x[0])>1)
//		//{
//		//	return false;
//		//}
//
//		//判断是否迭代
//		if(	/* fabs(x[0])< 1e-4*/
//			fabs(x[1])< 1e-4
//			&&fabs(x[2])< 1e-4
//			&&fabs(x[3])< 1e-4
//			&&fabs(x[4])< 1e-4
//			&&fabs(x[5])< 1e-4
//			&&fabs(x[6])< 1e-4)
//		{
//			//计算精度
//			double *Ax = new double[N*1];
//			mult(A,x,Ax,N,col,1);
//			//MatMult(A,x,Ax,N,col,1);
//			MatMinus(Ax,L,V,N,1);
//			transpose(V,Vt,N,1);
//			//MatTrs(V,N,1,Vt);
//			double VtV;
//			mult(Vt,V,&VtV,1,N,1);
//			//MatMult(Vt,V,&VtV,1,N,1);
//			double sigma = sqrt(VtV/(N-col));
//
//			double *Q = new double[col];
//			for (i = 0;i< col;i++)
//			{
//				Q[i] = sigma*AtA[i*col + i];
//				//	precision[i] = Q[i];
//				//  printf("%lf\n",Q[i]);		//输出精度
//			}
//			delete []Ax;
//			delete []Q;
//			break;
//		}
//		//更新待定参数的新值
//		else
//		{
//			m_fScale = /*(float)*/(m_fScale*(1 + x[0]));
//			m_fAngle[0] += x[1];
//			m_fAngle[1] += x[2];
//			m_fAngle[2] += x[3];
//		}
//	}
//
//	//	printf("循环次数:%d\n",iteration);
//	if (iteration == 100)
//	{
//		return false;
//	}
//
//	double sumDX,sumDY,sumDZ;
//	sumDX = sumDY = sumDZ=0;
//
//	//归化到[-pi,pi]
//	if (m_fAngle[2]> PI64)
//	{
//		m_fAngle[2] -= 2*PI64;
//	}
//	if (m_fAngle[2]< -PI64)
//	{
//		m_fAngle[2] += 2*PI64;
//	}
//
//	ComputeRotateMatrixByAngle(m_fAngle[0],m_fAngle[1],m_fAngle[2],&m_fRotateMatrix[0]);
//
//	sumDX = sumDY = sumDZ = 0;
//	//将点坐标恢复
//	for (i = 0;i< n;i++)
//	{
//		RegX[i] += xg;
//		RegY[i] += yg;
//		RegZ[i] += zg;
//
//		RefX[i] += Xtpg;
//		RefY[i] += Ytpg;
//		RefZ[i] += Ztpg;
//
//		m_fOffset[0] = RefX[i] - m_fScale*(m_fRotateMatrix[0]*RegX[i] + m_fRotateMatrix[1]*RegY[i] + m_fRotateMatrix[2]*RegZ[i]);
//		m_fOffset[1] = RefY[i] - m_fScale*(m_fRotateMatrix[3]*RegX[i] + m_fRotateMatrix[4]*RegY[i] + m_fRotateMatrix[5]*RegZ[i]);
//		m_fOffset[2] = RefZ[i] - m_fScale*(m_fRotateMatrix[6]*RegX[i] + m_fRotateMatrix[7]*RegY[i] + m_fRotateMatrix[8]*RegZ[i]);
//
//		sumDX += m_fOffset[0];
//		sumDY += m_fOffset[1];
//		sumDZ += m_fOffset[2];
//	}
//
//	m_fOffset[0] = sumDX/n;
//	m_fOffset[1] = sumDY/n;
//	m_fOffset[2] = sumDZ/n;
//
//	m_matrix[0][0] = m_fRotateMatrix[0];
//	m_matrix[0][1] = m_fRotateMatrix[1];
//	m_matrix[0][2] = m_fRotateMatrix[2];
//
//	m_matrix[1][0] = m_fRotateMatrix[3];
//	m_matrix[1][1] = m_fRotateMatrix[4];
//	m_matrix[1][2] = m_fRotateMatrix[5];
//
//	m_matrix[2][0] = m_fRotateMatrix[6];
//	m_matrix[2][1] = m_fRotateMatrix[7];
//	m_matrix[2][2] = m_fRotateMatrix[8];
//
//	m_matrix[0][3] = m_fOffset[0];
//	m_matrix[1][3] = m_fOffset[1];
//	m_matrix[2][3] = m_fOffset[2];
//
//	m_matrix[3][0] = 0.0;
//	m_matrix[3][1] = 0.0;
//	m_matrix[3][2] = 0.0;
//	m_matrix[3][3] = 1.0;
//
//
//	delete[] RefX;
//	delete[] RefY;
//	delete[] RefZ;
//	delete[] RegX;
//	delete[] RegY;
//	delete[] RegZ;
//
//	delete[]  L;
//	delete[]  A;
//	delete[]  x;
//	delete[]  V;
//
//	delete[]  At;
//	delete[]  AtA;
//	delete[]  _AtA;
//	delete[]  AtL;
//	delete[]  Vt;
//
//	return true;
//}

bool CBursaWolfModel::Create( const double* pRefX, /* 目标控制点X */ 
	const double* pRefY, /* 目标控制点Y */ 
	const double* pRefZ, /* 目标控制点Z */ 
	const double* pRegX, /* 源控制点X */ 
	const double* pRegY, /* 源控制点Y */ 
	const double* pRegZ, /* 源控制点Z */ 
	int N_PTS,				/* 控制点数 */ 
	int transType )		// 变换类型 = HD_LANDMARK_RIGIDBODY
{
	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (N_PTS == 0)
		|| (transType != HD_LANDMARK_RIGIDBODY && transType != HD_LANDMARK_SIMILARITY && transType != HD_LANDMARK_AFFINE && transType != HD_LANDMARK_XYZYAW))
	{
		return false;
	}

	int i,j;
	// 将控制点转换为double
	double *RefX  = new double[N_PTS];
	double *RefY = new double[N_PTS];
	double *RefZ = new double[N_PTS];
	double *RegX = new double[N_PTS];
	double *RegY = new double[N_PTS];
	double *RegZ = new double[N_PTS];

	for (int j= 0;j< N_PTS;j++)
	{
		RefX[j] = pRefX[j];
		RefY[j] = pRefY[j];
		RefZ[j] = pRefZ[j];

		RegX[j] = pRegX[j];
		RegY[j] = pRegY[j];
		RegZ[j] = pRegZ[j];
	}
	// 计算控制点的中心
	double source_centroid[3]={0,0,0};
	double target_centroid[3]={0,0,0};
	for(i=0;i<N_PTS;i++)
	{
		source_centroid[0] += RegX[i];
		source_centroid[1] += RegY[i];
		source_centroid[2] += RegZ[i];
		
		target_centroid[0] += RefX[i];
		target_centroid[1] += RefY[i];
		target_centroid[2] += RefZ[i];
	}
	source_centroid[0] /= N_PTS;
	source_centroid[1] /= N_PTS;
	source_centroid[2] /= N_PTS;
	target_centroid[0] /= N_PTS;
	target_centroid[1] /= N_PTS;
	target_centroid[2] /= N_PTS;

	//m_dGravityCenter[0] = target_centroid[0];
	//m_dGravityCenter[1] = target_centroid[1];
	//m_dGravityCenter[2] = target_centroid[2];
	// -- if only one point, stop right here
	
	if (N_PTS == 1)
	{
		getIdentity();
		return false;
	}

	//double Matrix[4][4] = {0.0};
	memset(m_matrix,0,sizeof(double) * 16);

	// -- build the 3x3 matrix M --
	double M[3][3];
	double AAT[3][3];
	for(i=0;i<3;i++) 
	{
		AAT[i][0] = M[i][0]=0.0F; // fill M with zeros
		AAT[i][1] = M[i][1]=0.0F; 
		AAT[i][2] = M[i][2]=0.0F; 
	}

	double a[3],b[3];
	double sa=0.0F,sb=0.0F;
	int pt;
	for(pt=0;pt<N_PTS;pt++)
	{
		// get the origin-centred point (a) in the source set
		//this->SourceLandmarks->GetPoint(pt,a);
		a[0] = RegX[pt];
		a[1] = RegY[pt];
		a[2] = RegZ[pt];

		a[0] -= source_centroid[0];
		a[1] -= source_centroid[1];
		a[2] -= source_centroid[2];
		// get the origin-centred point (b) in the target set
		//this->TargetLandmarks->GetPoint(pt,b);
		b[0] = RefX[pt];
		b[1] = RefY[pt];
		b[2] = RefZ[pt];

		b[0] -= target_centroid[0];
		b[1] -= target_centroid[1];
		b[2] -= target_centroid[2];
		// accumulate the products a*T(b) into the matrix M
		for(i=0;i<3;i++) 
		{
			M[i][0] += a[i]*b[0];
			M[i][1] += a[i]*b[1];
			M[i][2] += a[i]*b[2];

			// for the affine transform, compute ((a.a^t)^-1 . a.b^t)^t.
			// a.b^t is already in M.  here we put a.a^t in AAT.
			if (transType == HD_LANDMARK_AFFINE)
			{
				AAT[i][0] += a[i]*a[0];
				AAT[i][1] += a[i]*a[1];
				AAT[i][2] += a[i]*a[2];
			}
		}
		// accumulate scale factors (if desired)
		sa += a[0]*a[0]+a[1]*a[1]+a[2]*a[2];
		sb += b[0]*b[0]+b[1]*b[1]+b[2]*b[2];
	}

	if(transType == HD_LANDMARK_AFFINE)
	{
		// AAT = (a.a^t)^-1
		//vtkMath::Invert3x3(AAT,AAT);
		invers_matrix((double*)AAT,3);

		// M = (a.a^t)^-1 . a.b^t
		//vtkMath::Multiply3x3(AAT,M,M);
		mult((double*)AAT,(double*)M,(double*)M,3,3,3);

		// this->m_matrix = M^t
		for(i=0;i<3;++i) 
		{
			for(j=0;j<3;++j)
			{
				m_matrix[i][j] = M[j][i];
			}
		}
	}
	else
	{
		// compute required scaling factor (if desired)
		double scale =  static_cast<double>(sqrt(sb/sa));

		// -- build the 4x4 matrix N --

		double Ndata[4][4];
		double *N[4];
		for(i=0;i<4;i++)
		{
			N[i] = Ndata[i];
			N[i][0]=0.0F; // fill N with zeros
			N[i][1]=0.0F;
			N[i][2]=0.0F;
			N[i][3]=0.0F;
		}
		// on-diagonal elements
		N[0][0] = M[0][0]+M[1][1]+M[2][2];
		N[1][1] = M[0][0]-M[1][1]-M[2][2];
		N[2][2] = -M[0][0]+M[1][1]-M[2][2];
		N[3][3] = -M[0][0]-M[1][1]+M[2][2];
		// off-diagonal elements
		N[0][1] = N[1][0] = M[1][2]-M[2][1];
		N[0][2] = N[2][0] = M[2][0]-M[0][2];
		N[0][3] = N[3][0] = M[0][1]-M[1][0];

		N[1][2] = N[2][1] = M[0][1]+M[1][0];
		N[1][3] = N[3][1] = M[2][0]+M[0][2];
		N[2][3] = N[3][2] = M[1][2]+M[2][1];

		// -- eigen-decompose N (is symmetric) --

		double eigenvectorData[4][4];
		double *eigenvectors[4],eigenvalues[4];

		eigenvectors[0] = eigenvectorData[0];
		eigenvectors[1] = eigenvectorData[1];
		eigenvectors[2] = eigenvectorData[2];
		eigenvectors[3] = eigenvectorData[3];

		JacobiN(N,4,eigenvalues,eigenvectors);

		// the eigenvector with the largest eigenvalue is the quaternion we want
		// (they are sorted in decreasing order for us by JacobiN)
		double w,x,y,z;

		// first: if points are collinear, choose the quaternion that 
		// results in the smallest rotation.
		if (eigenvalues[0] == eigenvalues[1] || N_PTS == 2)
		{
			double s0[3],t0[3],s1[3],t1[3];
			s0[0] = RegX[0];
			s0[1] = RegY[0];
			s0[2] = RegZ[0];

			t0[0] = RefX[0];
			t0[1] = RefY[0];
			t0[2] = RefZ[0];

			//this->SourceLandmarks->GetPoint(0,s0);
			//this->TargetLandmarks->GetPoint(0,t0);

			s1[0] = RegX[1];
			s1[1] = RegY[1];
			s1[2] = RegZ[1];

			t1[0] = RefX[1];
			t1[1] = RefY[1];
			t1[2] = RefZ[1];

			//this->SourceLandmarks->GetPoint(1,s1);
			//this->TargetLandmarks->GetPoint(1,t1);

			double ds[3],dt[3];
			double rs = 0, rt = 0;
			for (i = 0; i < 3; i++)
			{
				ds[i] = s1[i] - s0[i];      // vector between points
				rs += ds[i]*ds[i];
				dt[i] = t1[i] - t0[i];
				rt += dt[i]*dt[i];
			}

			// normalize the two vectors
			rs = sqrt(rs);
			ds[0] /= rs; ds[1] /= rs; ds[2] /= rs; 
			rt = sqrt(rt);
			dt[0] /= rt; dt[1] /= rt; dt[2] /= rt; 

			// take dot & cross product
			w = ds[0]*dt[0] + ds[1]*dt[1] + ds[2]*dt[2];
			x = ds[1]*dt[2] - ds[2]*dt[1];
			y = ds[2]*dt[0] - ds[0]*dt[2];
			z = ds[0]*dt[1] - ds[1]*dt[0];

			double r = sqrt(x*x + y*y + z*z);
			double theta = atan2(r,w);

			// construct quaternion
			w = cos(theta/2);
			if (r != 0)
			{
				r = sin(theta/2)/r;
				x = x*r;
				y = y*r;
				z = z*r;
			}
			else // rotation by 180 degrees: special case
			{
				// rotate around a vector perpendicular to ds
				Perpendiculars(ds,dt,0,0);
				r = sin(theta/2);
				x = dt[0]*r;
				y = dt[1]*r;
				z = dt[2]*r;
			}
		}
		else // points are not collinear
		{
			w = eigenvectors[0][0];
			x = eigenvectors[1][0];
			y = eigenvectors[2][0];
			z = eigenvectors[3][0];
		}

		// convert quaternion to a rotation matrix

		double ww = w*w;
		double wx = w*x;
		double wy = w*y;
		double wz = w*z;

		double xx = x*x;
		double yy = y*y;
		double zz = z*z;

		double xy = x*y;
		double xz = x*z;
		double yz = y*z;

		m_matrix[0][0] = ww + xx - yy - zz; 
		m_matrix[1][0] = 2.0*(wz + xy);
		m_matrix[2][0] = 2.0*(-wy + xz);

		m_matrix[0][1] = 2.0*(-wz + xy);  
		m_matrix[1][1] = ww - xx + yy - zz;
		m_matrix[2][1] = 2.0*(wx + yz);

		m_matrix[0][2] = 2.0*(wy + xz);
		m_matrix[1][2] = 2.0*(-wx + yz);
		m_matrix[2][2] = ww - xx - yy + zz;

		if (transType != HD_LANDMARK_RIGIDBODY)
		{ // add in the scale factor (if desired)
			for(i=0;i<3;i++) 
			{
				m_matrix[i][0] *= scale;
				m_matrix[i][1] *= scale;
				m_matrix[i][2] *= scale;
			}
			m_fScale = scale;
		}

	}//if (eigenvalues[0] == eigenvalues[1] || N_PTS == 2)

	// the translation is given by the difference in the transformed source
	// centroid and the target centroid
	double sx, sy, sz;

	sx =  static_cast<double>(m_matrix[0][0] * source_centroid[0] +
		m_matrix[0][1] * source_centroid[1] +
		m_matrix[0][2] * source_centroid[2]);
	sy =  static_cast<double>(m_matrix[1][0] * source_centroid[0] +
		m_matrix[1][1] * source_centroid[1] +
		m_matrix[1][2] * source_centroid[2]);
	sz =  static_cast<double>(m_matrix[2][0] * source_centroid[0] +
		m_matrix[2][1] * source_centroid[1] +
		m_matrix[2][2] * source_centroid[2]);

	m_matrix[0][3] = target_centroid[0] - sx;
	m_matrix[1][3] = target_centroid[1] - sy;
	m_matrix[2][3] = target_centroid[2] - sz;

	// fill the bottom row of the 4x4 matrix
	m_matrix[3][0] = 0.0;
	m_matrix[3][1] = 0.0;
	m_matrix[3][2] = 0.0;
	m_matrix[3][3] = 1.0;

	// 将矩阵转换为6参数
	matrix2Parameter();

	// 计算误差
	m_fError = 0.0;
	for(pt=0;pt<N_PTS;pt++)
	{
		// get the origin-centred point (a) in the source set
		//this->SourceLandmarks->GetPoint(pt,a);
		a[0] = RegX[pt];
		a[1] = RegY[pt];
		a[2] = RegZ[pt];
		
		b[0] = RefX[pt];
		b[1] = RefY[pt];
		b[2] = RefZ[pt];

		vtkHomogeneousTransformPoint(m_matrix,a[0],a[1],a[2]);
		//Translate(a[0],a[1],a[2]);
		m_fError += ((a[0] - b[0]) * (a[0] - b[0]) +
			(a[1] - b[1]) * (a[1] - b[1]) + 
			(a[2] - b[2]) * (a[2] - b[2]));
	}
	m_fError /= N_PTS;
	m_fError = sqrt(m_fError);
	

	delete[] RefX;
	delete[] RefY;
	delete[] RefZ;
	delete[] RegX;
	delete[] RegY;
	delete[] RegZ;
	return true;
}

bool CBursaWolfModel::Create( const float* pRefX, 
	const float* pRefY, 
	const float* pRefZ, 
	const float* pRegX, 
	const float* pRegY, 
	const float* pRegZ, 
	int N_PTS,			
	int transType )		
{
	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (N_PTS == 0)
		|| (transType != HD_LANDMARK_RIGIDBODY && transType != HD_LANDMARK_SIMILARITY && transType != HD_LANDMARK_AFFINE && transType != HD_LANDMARK_XYZYAW))
	{
		return false;
	}

	// 将控制点转换为float
	double *RefX  = new double[N_PTS];
	double *RefY = new double[N_PTS];
	double *RefZ = new double[N_PTS];
	double *RegX = new double[N_PTS];
	double *RegY = new double[N_PTS];
	double *RegZ = new double[N_PTS];

	for (int j= 0;j< N_PTS;j++)
	{
		RefX[j] = pRefX[j];
		RefY[j] = pRefY[j];
		RefZ[j] = pRefZ[j];

		RegX[j] = pRegX[j];
		RegY[j] = pRegY[j];
		RegZ[j] = pRegZ[j];
	}
	bool bRet = Create(RefX, RefY, RefZ, RegX, RegY, RegZ, N_PTS, transType);

	delete[] RefX;
	delete[] RefY;
	delete[] RefZ;
	delete[] RegX;
	delete[] RegY;
	delete[] RegZ;

	return bRet;
}

void CBursaWolfModel::Angle2RotateMatrix()
{
	ComputeRotateMatrixByAngle(m_fAngle[0],m_fAngle[1],m_fAngle[2],&m_fRotateMatrix[0]);
}

void CBursaWolfModel::Parameter2matrix()
{
	m_matrix[0][0] = m_fRotateMatrix[0];
	m_matrix[0][1] = m_fRotateMatrix[1];
	m_matrix[0][2] = m_fRotateMatrix[2];

	m_matrix[1][0] = m_fRotateMatrix[3];
	m_matrix[1][1] = m_fRotateMatrix[4];
	m_matrix[1][2] = m_fRotateMatrix[5];

	m_matrix[2][0] = m_fRotateMatrix[6];
	m_matrix[2][1] = m_fRotateMatrix[7];
	m_matrix[2][2] = m_fRotateMatrix[8];

	m_matrix[0][3] = m_fOffset[0];
	m_matrix[1][3] = m_fOffset[1];
	m_matrix[2][3] = m_fOffset[2];

	m_matrix[3][0] = 0.0;
	m_matrix[3][1] = 0.0;
	m_matrix[3][2] = 0.0;
	m_matrix[3][3] = 1.0;

}

void CBursaWolfModel::ComputeAngleByRotateMatrix()
{
	double a1 = m_fRotateMatrix[0];
	double a3 = m_fRotateMatrix[2];
	double c3 = m_fRotateMatrix[8];
	double b1 = m_fRotateMatrix[3];
	double b2 = m_fRotateMatrix[4];
	double b3 = m_fRotateMatrix[5];

	//double omega = asin(-b3);
	double sinOmega = -b3;
	double cosOmega = sqrt(a3*a3 + c3*c3);
	double omega = atan2(sinOmega, cosOmega);	// atan2的范围是(-PI,PI]

	double phi = atan2(-a3, c3);	

	double kappa = atan2(b1, b2);
	
	m_fAngle[0] = phi;
	m_fAngle[1] = omega;
	m_fAngle[2] = kappa;
	
	//int debug = 0;
}

void CBursaWolfModel::ComputeRotateMatrixByAngle(double phi,double omega,double kappa,double *R)
{
	double a1,a2,a3,b1,b2,b3,c1,c2,c3;

	a1 = cos(phi)*cos(kappa) - sin(phi)*sin(omega)*sin(kappa);
	a2 = -cos(phi)*sin(kappa) - sin(phi)*sin(omega)*cos(kappa);
	a3 = -sin(phi)*cos(omega);
	b1 = cos(omega)*sin(kappa);
	b2 = cos(omega)*cos(kappa);
	b3 = -sin(omega);
	c1 = sin(phi)*cos(kappa) + cos(phi)*sin(omega)*sin(kappa);                                                                  
	c2 = -sin(phi)*sin(kappa) + cos(phi)*sin(omega)*cos(kappa);
	c3 = cos(phi)*cos(omega);

	// 按照相似变换
	R[0] = a1 * m_fScale;
	R[1] = a2 * m_fScale; 
	R[2] = a3 * m_fScale;
	R[3] = b1 * m_fScale; 
	R[4] = b2 * m_fScale; 
	R[5] = b3 * m_fScale;
	R[6] = c1 * m_fScale; 
	R[7] = c2 * m_fScale; 
	R[8] = c3 * m_fScale;

	// phi-omega-kapp系统与鬼火内部计算旋转矩阵的映射关系 [2013/04/19 危迟] 
	/*	
	core::matrix4 mat;
	vector3df rotVec(-phi,omega,kappa);
	mat[0] = R[4] = b2 = cos(omega)*cos(kappa) ;
	mat[1] = R[3] = b1 = cos(omega)*sin(kappa);
	mat[2] = R[5] = b3 = -sin(omega);

	mat[4] = R[1] = a2 = -sin(phi)*sin(omega)*cos(kappa)-cos(phi)*sin(kappa);
	mat[5] = R[0] = a1 = -sin(phi)*sin(omega)*sin(kappa)+cos(phi)*cos(kappa);
	mat[6] = R[2] = a3 = -sin(phi)*cos(omega);

	mat[8] = R[7] = c2 = cos(phi)*sin(omega)*cos(kappa) - sin(phi)*sin(kappa);
	mat[9] = R[6] = c1 = cos(phi)*sin(omega)*sin(kappa) + sin(phi)*cos(kappa);
	mat[10] = R[8]= c3 = cos(phi)*cos(omega);
	*/
}

void CBursaWolfModel::matrix2Parameter()
{
	m_fRotateMatrix[0] = m_matrix[0][0];
	m_fRotateMatrix[1] = m_matrix[0][1];
	m_fRotateMatrix[2] = m_matrix[0][2];

	m_fRotateMatrix[3] = m_matrix[1][0];
	m_fRotateMatrix[4] = m_matrix[1][1];
	m_fRotateMatrix[5] = m_matrix[1][2];

	m_fRotateMatrix[6] = m_matrix[2][0];
	m_fRotateMatrix[7] = m_matrix[2][1];
	m_fRotateMatrix[8] = m_matrix[2][2];

	m_fOffset[0] = m_matrix[0][3];
	m_fOffset[1] = m_matrix[1][3];
	m_fOffset[2] = m_matrix[2][3];

	// 旋转矩阵直接计算m_fAngle[0],m_fAngle[1],m_fAngle[2]
	ComputeAngleByRotateMatrix();
}

/************************************************************************/
/* X                 (X')   X0                                          */
/* Y  =  scale * R * (Y') + Y0   = M * [x,y,z,1]T                       */
/* Z                 (Z')   Z0                                          */
/************************************************************************/
//
bool CBursaWolfModel::Translate(float& x, float& y, float& z) const 
{
	double tmpX = x;
	double tmpY = y;
	double tmpZ = z;

	x = (float)(m_matrix[0][0]*tmpX + m_matrix[0][1]*tmpY + m_matrix[0][2]*tmpZ + m_matrix[0][3]);
	y = (float)(m_matrix[1][0]*tmpX + m_matrix[1][1]*tmpY + m_matrix[1][2]*tmpZ + m_matrix[1][3]);
	z = (float)(m_matrix[2][0]*tmpX + m_matrix[2][1]*tmpY + m_matrix[2][2]*tmpZ + m_matrix[2][3]);
	double w = m_matrix[3][0]*tmpX + m_matrix[3][1]*tmpY + m_matrix[3][2]*tmpZ + m_matrix[3][3];

	double f = 1.0/w;
	x = static_cast<float>(x*f);
	y = static_cast<float>(y*f);
	z = static_cast<float>(z*f);

	return true;
}

bool CBursaWolfModel::TranslateOri(float& x, float& y, float& z) const 
{
	float tempX = x;
	float tempY = y;
	float tempZ = z;

	x = (float)(m_fScale * (m_fRotateMatrix[0]*tempX + m_fRotateMatrix[1]*tempY + m_fRotateMatrix[2]*tempZ) + m_fOffset[0]);
	y = (float)(m_fScale * (m_fRotateMatrix[3]*tempX + m_fRotateMatrix[4]*tempY + m_fRotateMatrix[5]*tempZ) + m_fOffset[1]);
	z = (float)(m_fScale * (m_fRotateMatrix[6]*tempX + m_fRotateMatrix[7]*tempY + m_fRotateMatrix[8]*tempZ) + m_fOffset[2]);
	
	return true;
}

bool CBursaWolfModel::Translate(double& x, double& y, double& z) const 
{
	double tmpX = x;
	double tmpY = y;
	double tmpZ = z;

	x = m_matrix[0][0]*tmpX + m_matrix[0][1]*tmpY + m_matrix[0][2]*tmpZ + m_matrix[0][3];
	y = m_matrix[1][0]*tmpX + m_matrix[1][1]*tmpY + m_matrix[1][2]*tmpZ + m_matrix[1][3];
	z = m_matrix[2][0]*tmpX + m_matrix[2][1]*tmpY + m_matrix[2][2]*tmpZ + m_matrix[2][3];
	double w = m_matrix[3][0]*tmpX + m_matrix[3][1]*tmpY + m_matrix[3][2]*tmpZ + m_matrix[3][3];

	double f = 1.0/w;
	x = static_cast<double>(x*f);
	y = static_cast<double>(y*f);
	z = static_cast<double>(z*f);

	return true;
}

void CBursaWolfModel::TranslateExtentW(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax) const
{
	double centerX = (xmin + xmax)/2.f;
	double centerY = (ymin + ymax)/2.f;
	double centerZ = (zmin + zmax)/2.f;

	Translate(centerX,centerY,centerZ);

	double extentX = fabs(xmax - xmin)*m_fScale;
	double extentY = fabs(ymax - ymin)*m_fScale;
	double extentZ = fabs(zmax - zmin)*m_fScale;

	xmin = centerX - extentX/2.f;
	xmax = centerX + extentX/2.f;
	ymin = centerY - extentY/2.f;
	ymax = centerY + extentY/2.f;
	zmin = centerZ - extentZ/2.f;
	zmax = centerZ + extentZ/2.f;

}

bool CBursaWolfModel::TranslateExtent(double& min_x,double& min_y,double& min_z,double& max_x,double& max_y,double& max_z) const
{
	// 底平面4个点
	double x1 = min_x,y1 = min_y,z1 = min_z;
	double x2 = max_x,y2 = min_y,z2 = min_z;
	double x3 = max_x,y3 = max_y,z3 = min_z;
	double x4 = min_x,y4 = max_y,z4 = min_z;
	// 顶平面4个点
	double x5 = min_x,y5 = min_y,z5 = max_z;
	double x6 = max_x,y6 = min_y,z6 = max_z;
	double x7 = max_x,y7 = max_y,z7 = max_z;
	double x8 = min_x,y8 = max_y,z8 = max_z;

	Translate(x1,y1,z1);
	Translate(x2,y2,z2);
	Translate(x3,y3,z3);
	Translate(x4,y4,z4);

	Translate(x5,y5,z5);
	Translate(x6,y6,z6);
	Translate(x7,y7,z7);
	Translate(x8,y8,z8);

	min_x = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
	min_y = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));
	min_z = MIN(MIN(MIN(MIN(z1,z2),z3),z4),MIN(MIN(MIN(z5,z6),z7),z8));

	max_x = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
	max_y = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));
	max_z = MAX(MAX(MAX(MAX(z1,z2),z3),z4),MAX(MAX(MAX(z5,z6),z7),z8));

	return true;
}

bool CBursaWolfModel::TranslateOri(double& x, double& y, double& z) const 
{
	double tempX = x;
	double tempY = y;
	double tempZ = z;

	x = m_fScale * (m_fRotateMatrix[0]*tempX + m_fRotateMatrix[1]*tempY + m_fRotateMatrix[2]*tempZ) + m_fOffset[0];
	y = m_fScale * (m_fRotateMatrix[3]*tempX + m_fRotateMatrix[4]*tempY + m_fRotateMatrix[5]*tempZ) + m_fOffset[1];
	z = m_fScale * (m_fRotateMatrix[6]*tempX + m_fRotateMatrix[7]*tempY + m_fRotateMatrix[8]*tempZ) + m_fOffset[2];
	
	return true;
}

/************************************************************************/
/* X'         (X - X0)                                                  */
/* Y'  =  R * (Y - Y0) * 1/scale                                        */
/* Z'         (Z - Z0)                                                  */
/************************************************************************/
bool CBursaWolfModel::AntiTranslate(float& x, float& y, float& z) const
{
	float tempX = x;
	float tempY = y;
	float tempZ = z;

	double antiR[9];
	memcpy(antiR, m_fRotateMatrix, 9*sizeof(double));
	invers_matrix(antiR, 3);

	double x1 = (tempX - m_fOffset[0])/m_fScale;
	double y1 = (tempY - m_fOffset[1])/m_fScale;
	double z1 = (tempZ - m_fOffset[2])/m_fScale;

	x = (float)(antiR[0]*x1 + antiR[1]*y1 + antiR[2]*z1);
	y = (float)(antiR[3]*x1 + antiR[4]*y1 + antiR[5]*z1);
	z = (float)(antiR[6]*x1 + antiR[7]*y1 + antiR[8]*z1);

	return true;
}

bool CBursaWolfModel::AntiTranslate(double& x, double& y, double& z) const
{
	double tempX = x;
	double tempY = y;
	double tempZ = z;

	double antiR[9];
	memcpy(antiR, m_fRotateMatrix, 9*sizeof(double));
	invers_matrix(antiR, 3);

	double x1 = (tempX - m_fOffset[0])/m_fScale;
	double y1 = (tempY - m_fOffset[1])/m_fScale;
	double z1 = (tempZ - m_fOffset[2])/m_fScale;

	x = (antiR[0]*x1 + antiR[1]*y1 + antiR[2]*z1);
	y = (antiR[3]*x1 + antiR[4]*y1 + antiR[5]*z1);
	z = (antiR[6]*x1 + antiR[7]*y1 + antiR[8]*z1);

	return true;
}

CBursaWolfModel CBursaWolfModel::getAntiModel()  const
{
	CBursaWolfModel model;

	model.m_fScale = 1/m_fScale;

	model.m_fAngle[0] = -m_fAngle[0];
	model.m_fAngle[1] = -m_fAngle[1];
	model.m_fAngle[2] = -m_fAngle[2];

	model.m_fError = m_fError;

	memcpy(model.m_fRotateMatrix, m_fRotateMatrix, 9*sizeof(double));
	invers_matrix(model.m_fRotateMatrix, 3);

	double* antiR = model.m_fRotateMatrix;
	model.m_fOffset[0] = (antiR[0]*m_fOffset[0] + antiR[1]*m_fOffset[1] + antiR[2]*m_fOffset[2]);
	model.m_fOffset[1] = (antiR[3]*m_fOffset[0] + antiR[4]*m_fOffset[1] + antiR[5]*m_fOffset[2]);
	model.m_fOffset[2] = (antiR[6]*m_fOffset[0] + antiR[7]*m_fOffset[1] + antiR[8]*m_fOffset[2]);
	model.m_fOffset[0] *= -model.m_fScale;
	model.m_fOffset[1] *= -model.m_fScale;
	model.m_fOffset[2] *= -model.m_fScale;

	//更新matrix的值
	model.m_matrix[0][0] = antiR[0];
	model.m_matrix[0][1] = antiR[1];
	model.m_matrix[0][2] = antiR[2];

	model.m_matrix[1][0] = antiR[3];
	model.m_matrix[1][1] = antiR[4];
	model.m_matrix[1][2] = antiR[5];

	model.m_matrix[2][0] = antiR[6];
	model.m_matrix[2][1] = antiR[7];
	model.m_matrix[2][2] = antiR[8];

	model.m_matrix[0][3] = model.m_fOffset[0];
	model.m_matrix[1][3] = model.m_fOffset[1];
	model.m_matrix[2][3] = model.m_fOffset[2];

	model.m_matrix[3][0] = 0.0;
	model.m_matrix[3][1] = 0.0;
	model.m_matrix[3][2] = 0.0;
	model.m_matrix[3][3] = 1.0;
	return model;
}

void CBursaWolfModel::Serialize(TiXmlElement* element, bool bSave)
{
	if (bSave)
	{
		char strTemp[128];
		TiXmlText* xmlText = NULL;
		TiXmlElement* xmlElement = NULL;

		TiXmlElement* modelElement = new TiXmlElement("TransModel");
		element->LinkEndChild(modelElement);

		xmlElement = new TiXmlElement("Scale");
		modelElement->LinkEndChild(xmlElement);
		sprintf_s(strTemp,128, "%lf", m_fScale);
		xmlText = new TiXmlText(strTemp);
		xmlElement->LinkEndChild(xmlText);

		xmlElement = new TiXmlElement("Offset");
		modelElement->LinkEndChild(xmlElement);
		sprintf_s(strTemp,128, "%lf,%lf,%lf", m_fOffset[0], m_fOffset[1], m_fOffset[2]);
		xmlText = new TiXmlText(strTemp);
		xmlElement->LinkEndChild(xmlText);

		//xmlElement = new TiXmlElement("GravityCenter");
		//modelElement->LinkEndChild(xmlElement);
		//sprintf_s(strTemp,128, "%lf,%lf,%lf",m_dGravityCenter[0], m_dGravityCenter[1],m_dGravityCenter[2]);
		//xmlText = new TiXmlText(strTemp);
		//xmlElement->LinkEndChild(xmlText);

		xmlElement = new TiXmlElement("Angle");
		modelElement->LinkEndChild(xmlElement);
		sprintf_s(strTemp,128, "%lf,%lf,%lf", m_fAngle[0], m_fAngle[1], m_fAngle[2]);
		xmlText = new TiXmlText(strTemp);
		xmlElement->LinkEndChild(xmlText);

		xmlElement = new TiXmlElement("RotateMatrix");
		modelElement->LinkEndChild(xmlElement);
		sprintf_s(strTemp,128, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", m_fRotateMatrix[0], m_fRotateMatrix[1], m_fRotateMatrix[2],
													m_fRotateMatrix[3], m_fRotateMatrix[4], m_fRotateMatrix[5],
													m_fRotateMatrix[6], m_fRotateMatrix[7], m_fRotateMatrix[8]);
		xmlText = new TiXmlText(strTemp);
		xmlElement->LinkEndChild(xmlText);

		xmlElement = new TiXmlElement("Error");
		modelElement->LinkEndChild(xmlElement);
		sprintf_s(strTemp,128, "%lf", m_fError);
		xmlText = new TiXmlText(strTemp);
		xmlElement->LinkEndChild(xmlText);
	}
	else
	{
		string strValue;
		string strText;
		TiXmlElement* nextElement = element->FirstChildElement();
		while(nextElement)
		{
			strValue = nextElement->Value();
			strText = nextElement->GetText();
			if (strValue == "Scale")
			{
				sscanf_s(strText.data(), "%lf", &m_fScale);
			}
			else if (strValue == "Offset")
			{
				sscanf_s(strText.data(), "%lf,%lf,%lf", &m_fOffset[0], &m_fOffset[1], &m_fOffset[2]);
			}
		/*	else if (strValue == "GravityCenter")
			{
				sscanf_s(strText.data(), "%lf,%lf,%lf", &m_dGravityCenter[0], &m_dGravityCenter[1], &m_dGravityCenter[2]);
			}*/
			else if (strValue == "Angle")
			{
				sscanf_s(strText.data(), "%lf,%lf,%lf", &m_fAngle[0], &m_fAngle[1], &m_fAngle[2]);
			}
			else if (strValue == "RotateMatrix")
			{
				sscanf_s(strText.data(), "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &m_fRotateMatrix[0], &m_fRotateMatrix[1], &m_fRotateMatrix[2],
																	&m_fRotateMatrix[3], &m_fRotateMatrix[4], &m_fRotateMatrix[5],
																	&m_fRotateMatrix[6], &m_fRotateMatrix[7], &m_fRotateMatrix[8]);
			}
			else if (strValue == "Error")
			{
				sscanf_s(strText.data(), "%lf", &m_fError);
			}

			nextElement = nextElement->NextSiblingElement();
		}

		m_matrix[0][0] = m_fRotateMatrix[0];
		m_matrix[0][1] = m_fRotateMatrix[1];
		m_matrix[0][2] = m_fRotateMatrix[2];

		m_matrix[1][0] = m_fRotateMatrix[3];
		m_matrix[1][1] = m_fRotateMatrix[4];
		m_matrix[1][2] = m_fRotateMatrix[5];

		m_matrix[2][0] = m_fRotateMatrix[6];
		m_matrix[2][1] = m_fRotateMatrix[7];
		m_matrix[2][2] = m_fRotateMatrix[8];

		m_matrix[0][3] = m_fOffset[0];
		m_matrix[1][3] = m_fOffset[1];
		m_matrix[2][3] = m_fOffset[2];

		m_matrix[3][0] = 0.0;
		m_matrix[3][1] = 0.0;
		m_matrix[3][2] = 0.0;
		m_matrix[3][3] = 1.0;
	}
}

// 单点计算  
bool CBursaWolfModel::CreateModel( const double* pRefX, /* 目标控制点X */ 
								   const double* pRefY, /* 目标控制点Y */ 
								   const double* pRefZ, /* 目标控制点Z */
								   const double* pRegX, /* 源控制点X */ 
								   const double* pRegY, /* 源控制点Y */ 
								   const double* pRegZ, /* 源控制点Z */ 
								   int n, /* 控制点数 */ 
								   double& RefGeoX,double& RefGeoY,double& RefGeoZ,
								   double& RegGeoX,double& RegGeoY,double& RegGeoZ )
{
	//
	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (n <= 0))
	{
		return false;
	}

	// 角度转换为旋转矩阵
	Angle2RotateMatrix();
	// 参数转换为矩阵 
	Parameter2matrix();
	return true;
}

// 测站坐标转换为大地坐标——绝对定向
bool CBursaWolfModel::CreateModel( const double* pRefX, 
									const double* pRefY, 
									const double* pRefZ,
									const double* pRegX,
									const double* pRegY,
									const double* pRegZ, 
									int n ,
									bool bAdjust)
{
	if (!pRefX || !pRefY || !pRefZ || !pRegX || !pRegY || !pRegZ || (n <= 0))
	{
		return false;
	}

	// 暂时限制只有两个点进行转换：测站中心点，同名点
	if (n != 2)
	{
		return false;
	}
	int centerId = -1;

	// 偏移量求解
	for (int i = 0; i< n;i++)
	{
		// 测站坐标为（0,0,0）点表示测站中心，其对应的大地坐标就是坐标的平移量
		if (pRegX[i] == 0.0f && pRegY[i] == 0.0f && pRegZ[i] == 0.0f)
		{
			m_fOffset[0] = pRefX[i];
			m_fOffset[1] = pRefY[i];
			m_fOffset[2] = pRefZ[i];
			centerId = i;
			break;
		}
	}

	if (centerId < 0)
	{
		m_fOffset[2] = ((pRefZ[0] - pRegZ[0]) + (pRefZ[1] - pRegZ[1]))/2.f;
	}

	// 使用平差方法求解 chenpeng
	if (bAdjust)
	{
		//二维平面坐标转换
		//仅在XOY平面
		//仅一个旋转角度theta，以及两个偏移量
		//坐标转换方程为X' = CX + del_X
		//C为旋转矩阵
		// C = [ cos(theta),	sin(theta）
		//		-sin(theta),	cos(theta)]
		//利用间接平差求转换参数	V = Bx - l
		//l = L - L0,	L为真实值（全站仪值），	L0为通过观测值经初始值坐标转换得到的转换坐标
		//B为线性化矩阵

		// 包含x,y,z属性的点
		struct PointXYZD
		{
			hd::f64  x,y,z;
			PointXYZD()
				:x(0.0),y(0.0),z(0.0){}
		};

		double theta = 0;
		double del_x = 0;
		double del_y = 0;
		double x,y;
		double a,b;
		int num = 2;
		double *pB = new double [num*2 * 3];
		memset(pB,0,sizeof(double)*num*2*3);
		double *pl = new double [num*2];
		memset(pl,0,sizeof(double)*num*2);
		PointXYZD *pLPos = new PointXYZD [num];
		memset(pLPos,0,sizeof(PointXYZD)*num);

		//double *pBT = new double [num*2 * 3];
		//memset(pBT,0,sizeof(double)*num*2*3);

		int count = 0;
		double* pResult = new double[3];
		memset(pResult,0,sizeof(double)*3);

		while(count < 10)
		{
			//1.求线性化矩阵B,L0

			//角度换弧度
			theta = theta*DEGTORAD64;
			for(int i = 0; i < num; i++)
			{
				x = pRegX[i];	y = pRegY[i];
				a = -x*sin(theta)+y*cos(theta);
				b = -x*cos(theta)-y*sin(theta);
				pB[i*6+0] = a;	pB[i*6+1] = 1; pB[i*6+2] = 0;
				pB[i*6+3] = b;	pB[i*6+4] = 0; pB[i*6+5] = 1; 
				//求L0
				pLPos[i].x =  (f64)(x*cos(theta) + y*sin(theta) + del_x);
				pLPos[i].y = (f64)(-x*sin(theta) + y*cos(theta) + del_y);
				pLPos[i].z = 0;
			}

			//2.求l,l = L - L0
			for(int i = 0; i < num; i++)
			{
				pl[i*2+0] = pRefX[i] - pLPos[i].x;
				pl[i*2+1] = pRefY[i] - pLPos[i].y;
			}

			//3.最小二乘求平差参数
			/*hdMatrix MATRIX_B(num*2,3,pB);
			hdMatrix MATRIX_L(num*2,1,pl);
			hdMatrix MATRIX_B_TEMP = MATRIX_B;
			hdMatrix MATRIX_B_T;
			MATRIX_B_TEMP.Transpose(MATRIX_B_T);

			hdMatrix MATRIX_P;
			MATRIX_B_T.Multiply(MATRIX_B,MATRIX_P);
			hdMatrix MATRIX_Q;
			MATRIX_B_T.Multiply(MATRIX_L,MATRIX_Q);
			hdMatrix MATRIX_PT;
			MATRIX_P.Invert(MATRIX_PT);
			hdMatrix MATRIX_X;
			MATRIX_PT.Multiply(MATRIX_Q,MATRIX_X);*/

			//
			CMatrix MATRIX_B(num*2,3,pB);
			CMatrix MATRIX_L(num*2,1,pl);
			CMatrix MATRIX_B_TEMP = MATRIX_B;
			CMatrix MATRIX_B_T = MATRIX_B_TEMP.getTranspose();
			CMatrix MATRIX_P = MATRIX_B_T * MATRIX_B;
			CMatrix MATRIX_Q = MATRIX_B_T * MATRIX_L;
			CMatrix MATRIX_X = MATRIX_P.getInverse()*MATRIX_Q;

			for(int i=0; i<3; i++)
			{
				pResult[i] = MATRIX_X.ptr[i];
			}

			theta = (theta + pResult[0])*RADTODEG64;
			del_x += pResult[1];
			del_y += pResult[2];

			count++ ;
		}

		// 角度转弧度
		pResult[0] = theta;
		pResult[1] = del_x;
		pResult[2] = del_y;

		m_fOffset[0] = del_x;
		m_fOffset[1] = del_y;
		m_fAngle[2] = -theta*DEGTORAD64;

		delete []pB;
		delete []pl;
		delete []pLPos;
		delete []pResult;

		// 角度转换为旋转矩阵
		Angle2RotateMatrix();

		// 参数转换为矩阵 
		Parameter2matrix();

		// 计算误差
		m_fError = 0.0;
		double aTmp[3];
		double bTmp[3];
		for(int pt=0;pt<n;pt++)
		{
			// get the origin-centred point (a) in the source set
			//this->SourceLandmarks->GetPoint(pt,a);
			aTmp[0] = pRegX[pt];
			aTmp[1] = pRegY[pt];
			aTmp[2] = pRegZ[pt];

			bTmp[0] = pRefX[pt];
			bTmp[1] = pRefY[pt];
			bTmp[2] = pRefZ[pt];

			vtkHomogeneousTransformPoint(m_matrix,aTmp[0],aTmp[1],aTmp[2]);
			//Translate(a[0],a[1],a[2]);
			m_fError += ((aTmp[0] - bTmp[0]) * (aTmp[0] - bTmp[0]) +
				         (aTmp[1] - bTmp[1]) * (aTmp[1] - bTmp[1]) + 
				         (aTmp[2] - bTmp[2]) * (aTmp[2] - bTmp[2]));
		}
		m_fError /= n;
		m_fError = sqrt(m_fError);

		return true;
	}

	// 旋转角度求解
	if (centerId == 0)
	{
		CHdPosition2dd refVec(pRefX[1]- pRefX[0],pRefY[1] - pRefY[0]);
		CHdPosition2dd regVec(pRegX[1]- pRegX[0],pRegY[1] - pRegY[0]);

		//向量点积求解角度值 下面的函数会将结果到[0,Pi/2]
		m_fAngle[2] = (regVec.getAngleWith(refVec)) * DEGTORAD64;
		if (refVec.X*regVec.X + refVec.Y*regVec.Y < 0)
		{
			m_fAngle[2] = PI64 - m_fAngle[2];
		}
		
		//向量叉积求解旋转方向，判断角度正负 
		// 右手系，逆时针方向为正
		CHdVector3df refVec3d((float)(pRefX[1]- pRefX[0]),(float)(pRefY[1] - pRefY[0]),0);
		CHdVector3df regVec3d((float)(pRegX[1]- pRegX[0]),(float)(pRegY[1] - pRegY[0]),0);
		
		// 源坐标系向量叉乘目标坐标系向量
		CHdVector3df crsPro = regVec3d.crossProduct(refVec3d);
		if (crsPro.Z < 0) 
		{
			m_fAngle[2] = 0 - m_fAngle[2];
		}
		
		//归一化到［-Pi,Pi］
		if (m_fAngle[2] < -PI64)
		{
			m_fAngle[2] += 2*PI64;
		}
		else if (m_fAngle[2] > PI64)
		{
			m_fAngle[2] -= 2*PI64;
		}
		//m_fAngle[2] -= PI64/2.f;
	}
	else
	{
		CHdPosition2dd refVec(pRefX[0] - pRefX[1],pRefY[0] - pRefY[1]);
		CHdPosition2dd regVec(pRegX[0] - pRefX[1],pRegY[0] - pRegY[1]);

		//向量点积求解角度值                                                                                                                                                              
		m_fAngle[2] = (regVec.getAngleWith(refVec)) * DEGTORAD64;
		
		//向量叉积求解旋转方向，判断角度正负 
		// 规定右手系，逆时针方向为正
		CHdVector3df refVec3d((float)(pRefX[0]),(float)(pRefY[0]),(float)(pRefZ[0]));
		CHdVector3df regVec3d((float)(pRegX[0]),(float)(pRegY[0]),(float)(pRegZ[0]));
		
		// 源坐标系向量差乘目标坐标系向量
		CHdVector3df crsPro = regVec3d.crossProduct(refVec3d);
		if (crsPro.Z > 0)
		{
			m_fAngle[2] = 0 - m_fAngle[0];
		}
	
		//归一化到［0,2*Pi］
		if (m_fAngle[2] < 0)
		{
			m_fAngle[2] += 2*PI64;
		}
		else if (m_fAngle[2] > 2*PI64)
		{
			m_fAngle[2] -= 2*PI64;
		}
		//m_fAngle[2] -= PI64/2.f;
	}
	return true;

}

// 根据旋转中心、旋转角度、旋转轴向量构建模型参数 [2014/03/23 危迟]
void CBursaWolfModel::BuildModelByRotateVectorAndCenter( double degree, /* 旋转角度 */ 
														 double centerX,double centerY,double centerZ, /* 旋转中心 */ 
														 double vecterX,double vecterY,double vecterZ )
{
	CHdVector3dd dir(vecterX,vecterY,vecterZ);
	dir.normalize();
	degree *= DEGTORAD64;
	double dCos = cos(degree);
	double dSin = sin(degree);

	m_matrix[0][0]  = (double)(dir.X * dir.X + (dir.Y * dir.Y + dir.Z * dir.Z)*dCos);
	m_matrix[1][0]  = (double)((dir.X * dir.Y)*(1 - dCos) + (dir.Z)*dSin);
	m_matrix[2][0]  = (double)((dir.X * dir.Z)*(1 - dCos) - (dir.Y)*dSin);
	m_matrix[3][0]  = (double)(0);
			 
	m_matrix[0][1]  = (double)((dir.X * dir.Y)*(1 - dCos) - (dir.Z)*dSin);
	m_matrix[1][1]  = (double)( dir.Y * dir.Y + (dir.X * dir.X + dir.Z * dir.Z)*dCos);
	m_matrix[2][1]  = (double)((dir.Y * dir.Z)*(1 - dCos) + (dir.X)*dSin);
	m_matrix[3][1]  = (double)(0);
			 
	m_matrix[0][2] = (double)((dir.X * dir.Z)*(1 - dCos) + (dir.Y)* dSin);
	m_matrix[1][2] = (double)((dir.Y * dir.Z)*(1 - dCos) - (dir.X)* dSin);
	m_matrix[2][2] = (double)(dir.Z * dir.Z + (dir.X * dir.X + dir.Y * dir.Y) * dCos);
	m_matrix[3][2] = (double)(0);

	m_matrix[0][3] = (double)((centerX * (dir.Y * dir.Y  + dir.Z * dir.Z) - dir.X*(centerY * dir.Y + centerZ * dir.Z))*(1 - dCos)
		    + (centerY * dir.Z - centerZ * dir.Y) * dSin);
	m_matrix[1][3] = (double)((centerY * (dir.X * dir.X  + dir.Z * dir.Z) - dir.Y*(centerX * dir.X + centerZ * dir.Z))*(1 - dCos)  
		    + (centerZ * dir.X - centerX * dir.Z) * dSin);
	m_matrix[2][3] = (double)((centerZ * (dir.X * dir.X  + dir.Y * dir.Y) - dir.Z*(centerX * dir.X + centerY * dir.Y))*(1 - dCos)  
		    + (centerX * dir.Y - centerY * dir.X) * dSin);
	m_matrix[3][3] = (double)(1);

	matrix2Parameter();

}

// 根据iScan输出角度yaw,pitch,roll构建旋转矩阵 [2014/07/17 危迟]
// 旋转顺序是Z、X、Y，也即Yaw,Pitch,Roll，但由于构建过程旋转矩阵进行了转置，所以其实际的旋转顺序Y、X、Z
// 坐标系绕自身Y轴顺时针旋转roll角，绕X轴顺时针旋转pitch角，绕Z轴逆时针旋转yaw角 [2014/11/07 危迟]
void CBursaWolfModel::ComputeRotateMatrixByIScanAngle( double Yaw,double Pitch,double Roll,double *R )
{
	double a1,a2,a3,b1,b2,b3,c1,c2,c3;

	a1 = cos(Roll)*cos(Yaw)+sin(Roll)*sin(Yaw)*sin(Pitch);
	a2 = -cos(Roll)*sin(Yaw)+sin(Roll)*cos(Yaw)*sin(Pitch);
	a3 = -sin(Roll)*cos(Pitch);
	b1 = sin(Yaw)*cos(Pitch);
	b2 = cos(Yaw)*cos(Pitch);
	b3 = sin(Pitch);
	c1 = sin(Roll)*cos(Yaw)-cos(Roll)*sin(Yaw)*sin(Pitch);
	c2 = -sin(Roll)*sin(Yaw)-cos(Roll)*cos(Yaw)*sin(Pitch);
	c3 = cos(Roll)*cos(Pitch);

	R[0] = a1 * m_fScale;
	R[1] = b1 * m_fScale; 
	R[2] = c1 * m_fScale;
	R[3] = a2 * m_fScale; 
	R[4] = b2 * m_fScale; 
	R[5] = c2 * m_fScale;
	R[6] = a3 * m_fScale; 
	R[7] = b3 * m_fScale; 
	R[8] = c3 * m_fScale;
}

// 根据旋转矩阵输出iScan中定义yaw,pitch,roll [2014/07/17 危迟]
// 旋转顺序是Z、X、Y，也即Yaw,Pitch,Roll，但由于构建过程旋转矩阵进行了转置，所以其实际的旋转顺序Y、X、Z
// 坐标系绕自身Y轴顺时针旋转roll角，绕X轴顺时针旋转pitch角，绕Z轴逆时针旋转yaw角 [2014/11/07 危迟]
void CBursaWolfModel::ComputeIScanAngleByRotateMatrix( double& Yaw,double& Pitch,double& Roll )
{
	double a3 = m_fRotateMatrix[6];
	double c3 = m_fRotateMatrix[8];
	double b1 = m_fRotateMatrix[1];
	double b2 = m_fRotateMatrix[4];
	double b3 = m_fRotateMatrix[7];

	double sinPitch = b3;
	double cosPitch = sqrt(b1*b1 + b2*b2);

	Pitch = atan2(sinPitch, cosPitch);	

	Roll = atan2(-a3, c3);	

	Yaw = atan2(b1, b2);

}

// 根据iScan角度构建旋转矩阵 [2014/07/17 危迟]
void CBursaWolfModel::IScanAngle2RotateMatrix(double Yaw,double Pitch,double Roll)
{
	ComputeRotateMatrixByIScanAngle(Yaw,Pitch,Roll,&m_fRotateMatrix[0]);
}

// 根据irr角度angleX，angleY，angleZ构建旋转矩阵 [2014/07/19 危迟]
// 旋转顺序为Z、Y、X
// 几何意义为首先绕Z轴逆时针旋转angleZ，再次绕Y轴逆时针旋转angleY，最后绕X轴逆时针旋转angleX [2014/11/07 危迟]
void CBursaWolfModel::ComputeRotateMatrixByIrrAngle( double angleX,double angleY,double angleZ,double *R )
{
	const f64 cr = cos( angleX );
	const f64 sr = sin( angleX );
	const f64 cp = cos( angleY );
	const f64 sp = sin( angleY );
	const f64 cy = cos( angleZ );
	const f64 sy = sin( angleZ );

	R[0] = ( cp*cy );
	R[3] = ( cp*sy );
	R[6] = ( -sp );

	const f64 srsp = sr*sp;
	const f64 crsp = cr*sp;

	R[1] = ( srsp*cy-cr*sy );
	R[4] = ( srsp*sy+cr*cy );
	R[7] = ( sr*cp );

	R[2] = ( crsp*cy+sr*sy );
	R[5] = ( crsp*sy-sr*cy );
	R[8] = ( cr*cp );
}

// 根据旋转矩阵输出Irr角度angleX,angleY,angleZ [2014/07/19 危迟]
// 旋转顺序为Z、Y、X
// 几何意义为首先绕Z轴逆时针旋转angleZ，再次绕Y轴逆时针旋转angleY，最后绕X轴逆时针旋转angleX [2014/11/07 危迟]
void CBursaWolfModel::ComputeIrrAngleByRotateMatrix( double& angleX,double& angleY,double& angleZ )
{
	double a3 = m_fRotateMatrix[7];
	double c3 = m_fRotateMatrix[8];
	double b1 = m_fRotateMatrix[0];
	double b2 = m_fRotateMatrix[3];
	double b3 = m_fRotateMatrix[6];

	double sinAngleY = -b3;
	double cosAngleY = sqrt(b1*b1 + b2*b2);

	angleY = atan2(sinAngleY, cosAngleY);	

	angleX = atan2(a3, c3);	

	angleZ = atan2(b2, b1);
}

// 根据Irr角度构建旋转矩阵 [2014/07/19 危迟]
void CBursaWolfModel::IrrAngle2RotateMatrix( double angleX,double angleY,double angleZ )
{
	ComputeRotateMatrixByIrrAngle(angleX,angleY,angleZ,&m_fRotateMatrix[0]);
}

// 根据两组平面方程先计算三个旋转角，但只能当做非线性最小二乘的角度初值
bool CBursaWolfModel::CreateRotateMatByPlanes(std::vector<NormalPointXYZ>& srcPlanes, std::vector<NormalPointXYZ>& destPlanes)
{
	if ((srcPlanes.size() <= 0) && (srcPlanes.size() != destPlanes.size()))
	{
		return false;
	}
	int nCount = (int)srcPlanes.size();

	// 最小二乘法计算旋转矩阵的9个参数
	double* A = new double[3*nCount*9];
	memset(A,0,27*nCount*sizeof(double));

	double* X = new double[9];		// 旋转矩阵的9个元素
	memset(X, 0, 9*sizeof(double));
	double* L = new double[3*nCount];
	memset(L, 0, 3*nCount*sizeof(nCount));

	for (int i=0; i < nCount; i++)
	{
		NormalPointXYZ& srcNomal = srcPlanes.at(i);
		//if (i%3 == 0)
		int ni = 3*i;
		{
			A[ni*9] = srcNomal.nx;
			A[ni*9 + 1] = srcNomal.ny;
			A[ni*9 + 2] = srcNomal.nz;

			A[(ni+1)*9 + 3] = srcNomal.nx;
			A[(ni+1)*9 + 4] = srcNomal.ny;
			A[(ni+1)*9 + 5] = srcNomal.nz;

			A[(ni+2)*9 + 6] = srcNomal.nx;
			A[(ni+2)*9 + 7] = srcNomal.ny;
			A[(ni+2)*9 + 8] = srcNomal.nz;
		}

		NormalPointXYZ& destNormal = destPlanes.at(i);
		L[i*3] = destNormal.nx;
		L[i*3 + 1] = destNormal.ny;
		L[i*3 + 2] = destNormal.nz;
	}

	double *AT = new double[nCount*27];
	double *ATA = new double[9*9];
	double *ATL = new double[9];

	transpose(A, AT, 3*nCount, 9);
	mult(AT,A, ATA, 9, 3*nCount, 9);
	invers_matrix(ATA, 9);

	mult(AT,L, ATL, 9, 3*nCount, 1);
	mult(ATA,ATL,X, 9, 9, 1);

	// SVD分解得到最接近该矩阵的旋转矩阵
	MatrixXd mQ(3,3);
	for (int i=0;i<3;i++)
	{
		for (int j=0;j<3;j++)
		{
			mQ(i,j) = X[i*3+j];
		}
	}
	//RMatrixSVD(mQ);	//奇异值分解, 得到近似的旋转矩阵
	for (int i=0;i<3;i++)
	{
		for (int j=0;j<3;j++)
		{			
			X[i*3+j] = mQ(i,j);			
		}
	}

	memcpy(m_fRotateMatrix, X, 9*sizeof(double));
	
	//Parameter2matrix();
	ComputeAngleByRotateMatrix();

	return true;
}

// 点P绕X轴旋转，求解旋转后的坐标需要的旋转矩阵R 逆时针为正 [2015/04/15 危迟]
void CBursaWolfModel::CalculateRotateMatrixByRotateXAxis(double omega)
{
	 m_matrix[0][0] = 1;	m_matrix[0][1] = 0;				m_matrix[0][2] = 0;
	 m_matrix[1][0] = 0;	m_matrix[1][1] = cos(omega);	m_matrix[1][2] = -sin(omega);
	 m_matrix[2][0] = 0;	m_matrix[2][1] = sin(omega);	m_matrix[2][2] = cos(omega);
}

// 点P绕Y轴旋转，求解旋转后的坐标需要的旋转矩阵R 顺时针为正 [2015/04/15 危迟]
void CBursaWolfModel::CalculateRotateMatrixByRotateYAxis(double phi)
{
	m_matrix[0][0] = cos(phi);		m_matrix[0][1] = 0;		m_matrix[0][2] = -sin(phi);
	m_matrix[1][0] = 0;				m_matrix[1][1] = 1;		m_matrix[1][2] = 0;
	m_matrix[2][0] = sin(phi);		m_matrix[2][1] = 0;		m_matrix[2][2] = cos(phi);
}

// 点P绕Z轴旋转，求解旋转后的坐标需要的旋转矩阵R 逆时针为正 [2015/04/15 危迟]
void CBursaWolfModel::CalculateRotateMatrixByRotateZAxis(double kappa)
{
	m_matrix[0][0] = cos(kappa);	m_matrix[0][1] = -sin(kappa);	m_matrix[0][2] = 0;
	m_matrix[1][0] = sin(kappa);	m_matrix[1][1] = cos(kappa);	m_matrix[1][2] = 0;
	m_matrix[2][0] = 0;				m_matrix[2][1] = 0;				m_matrix[2][2] = 1;
}

bool CBursaWolfModel::CreateModelByMatrix( double* M )
{
	if (!M)
	{
		return false;
	}

	m_matrix[0][0] = M[0];	 m_matrix[0][1] = M[1];	 m_matrix[0][2] = M[2]; m_matrix[0][3] = M[3];
	m_matrix[1][0] = M[4];	 m_matrix[1][1] = M[5];	 m_matrix[1][2] = M[6];	m_matrix[1][3] = M[7];
	m_matrix[2][0] = M[8];	 m_matrix[2][1] = M[9];  m_matrix[2][2] = M[10];	m_matrix[2][3] = M[11];
	m_matrix[3][0] = M[12];	 m_matrix[3][1] = M[13];  m_matrix[3][2] = M[14];	m_matrix[3][3] = M[15];

	matrix2Parameter();
	return true;
}


}
