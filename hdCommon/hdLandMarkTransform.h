/*! hdLandMarkTransform.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdLandMarkTransform.h
相关文件     : 
文件实现功能 : 海达数云根据控制点计算坐标转换
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/04/26   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include "hdCommon.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCore\hdMatrix.h"
#include "..\hdCore\hdMatrix4.h"
#include "..\hdCore\hdMath.h"

namespace hd
{
/*
根据控制点计算4*4矩阵,并获取7参数
*/
class CHdLandMarkTransform
{
public:
	CHdLandMarkTransform(void)
	{
		//identity_matrix(m_matrix);
		m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0;
		m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[4] =
		m_matrix[6] = m_matrix[7] = m_matrix[8] = m_matrix[9] =
		m_matrix[11] = m_matrix[12] = m_matrix[13] = m_matrix[14] = 0.0;
	}
	~CHdLandMarkTransform(void)
	{

	}

	//! 获取4*4矩阵
	inline void getMatrix(double *m)
	{
		memcpy(m,m_matrix,sizeof(double) * 16);
	}
	//! 获取平移3参数
	inline void getOffset(double& offX,double& offY,double& offZ)
	{
		offX = m_matrix[3];
		offY = m_matrix[7];
		offZ = m_matrix[11];
	}

	inline double getError()const{return m_fError;}
	//! 获取旋转3参数,角度单位为度
	inline void getRotateDegree(double& rotX,double& rotY,double& rotZ)
	{
		matrix4d irrMat;
		irrMat.setM((double*)m_matrix);
		// 计算得到的矩阵和irr矩阵是转置关系
		irrMat.makeInverse();
		CHdVector3d<double> rot = irrMat.getRotationDegrees();
		rotX = rot.X;
		rotY = rot.Y;
		rotZ = rot.Z;
	}
	//! 根据旋转角度设置旋转矩阵
	//inline void setRotateDegree(double rotX,double rotY,double rotZ)
	//{
	//	//double phi,double omega,double kappa

	//	double a1,a2,a3,b1,b2,b3,c1,c2,c3;

	//	a1 = cos(rotX)*cos(rotZ) - sin(rotX)*sin(rotY)*sin(rotZ);
	//	a2 = -cos(rotX)*sin(rotZ) - sin(rotX)*sin(rotY)*cos(rotZ);
	//	a3 = -sin(rotX)*cos(rotY);
	//	b1 = cos(rotY)*sin(rotZ);
	//	b2 = cos(rotY)*cos(rotZ);
	//	b3 = -sin(rotY);
	//	c1 = sin(rotX)*cos(rotZ) + cos(rotX)*sin(rotY)*sin(rotZ);                                                                  
	//	c2 = -sin(rotX)*sin(rotZ) + cos(rotX)*sin(rotY)*cos(rotZ);
	//	c3 = cos(rotX)*cos(rotY);

	//	m_matrix[0] = a1; m_matrix[1] = a2; m_matrix[2] = a3;
	//	m_matrix[3] = b1; m_matrix[4] = b2; m_matrix[5] = b3;
	//	m_matrix[6] = c1; m_matrix[7] = c2; m_matrix[8] = c3;

	//}
	//! 转换float坐标
	inline void transformPoint(float& x,float& y,float& z)
	{
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;
		transformPoint(tmpX,tmpY,tmpZ);
		x = (float)tmpX;
		y = (float)tmpY;
		z = (float)tmpZ;
	}
	//! 转换double坐标
	inline void transformPoint(double& x,double& y,double& z)
	{
		double tmpX = x;
		double tmpY = y;
		double tmpZ = z;

		x = m_matrix[0]*tmpX + m_matrix[1]*tmpY + m_matrix[2]*tmpZ + m_matrix[3];
		y = m_matrix[4]*tmpX + m_matrix[5]*tmpY + m_matrix[6]*tmpZ + m_matrix[7];
		z = m_matrix[8]*tmpX + m_matrix[9]*tmpY + m_matrix[10]*tmpZ + m_matrix[11];
		double w = m_matrix[12]*tmpX + m_matrix[13]*tmpY + m_matrix[14]*tmpZ + m_matrix[15];

		double f = 1.0/w;
		x = static_cast<double>(x*f);
		y = static_cast<double>(y*f);
		z = static_cast<double>(z*f);
	}

	//! 根据控制点计算4*4矩阵
	inline bool calMartrix( const double* pTargetX,  /* 目标控制点X */ 
		const double* pTargetY,					/* 目标控制点Y */ 
		const double* pTargetZ,					/* 目标控制点Z */ 
		const double* pSourceX,					/* 源控制点X */ 
		const double* pSourceY,					/* 源控制点Y */ 
		const double* pSourceZ,					/* 源控制点Z */ 
		int N_PTS,								/* 控制点数 */ 
		int transType )							// 变换类型 = HD_LANDMARK_RIGIDBODY
	{
		if (!pTargetX || !pTargetY || !pTargetZ || 
			!pSourceX || !pSourceY || !pSourceZ || (N_PTS == 0)
			|| (transType != HD_LANDMARK_RIGIDBODY && transType != HD_LANDMARK_SIMILARITY && transType != HD_LANDMARK_AFFINE))
		{
			return false;
		}

		if (N_PTS == 1)
		{
			identity_matrix(m_matrix);
			return false;
		}

		int i,j;
		// 将控制点转换为double
		double *tarX  = new double[N_PTS];
		double *tarY = new double[N_PTS];
		double *tarZ = new double[N_PTS];
		double *sorX = new double[N_PTS];
		double *sorY = new double[N_PTS];
		double *sorZ = new double[N_PTS];

		for (int j= 0;j< N_PTS;j++)
		{
			tarX[j] = pTargetX[j];
			tarY[j] = pTargetY[j];
			tarZ[j] = pTargetZ[j];

			sorX[j] = pSourceX[j];
			sorY[j] = pSourceY[j];
			sorZ[j] = pSourceZ[j];
		}
		// 计算控制点的中心
		double source_centroid[3]={0,0,0};
		double target_centroid[3]={0,0,0};
		for(i=0;i<N_PTS;i++)
		{
			source_centroid[0] += sorX[i];
			source_centroid[1] += sorY[i];
			source_centroid[2] += sorZ[i];

			target_centroid[0] += tarX[i];
			target_centroid[1] += tarY[i];
			target_centroid[2] += tarZ[i];
		}
		source_centroid[0] /= N_PTS;
		source_centroid[1] /= N_PTS;
		source_centroid[2] /= N_PTS;
		target_centroid[0] /= N_PTS;
		target_centroid[1] /= N_PTS;
		target_centroid[2] /= N_PTS;
						
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
			a[0] = sorX[pt];
			a[1] = sorY[pt];
			a[2] = sorZ[pt];

			a[0] -= source_centroid[0];
			a[1] -= source_centroid[1];
			a[2] -= source_centroid[2];
			// get the origin-centred point (b) in the target set
			//this->TargetLandmarks->GetPoint(pt,b);
			b[0] = tarX[pt];
			b[1] = tarY[pt];
			b[2] = tarZ[pt];

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
					m_matrix[i * 4 + j] = M[j][i];
				}
			}
		}
		else
		{
			// compute required scaling factor (if desired)
			double scale = (double)sqrt(sb/sa);

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
				s0[0] = sorX[0];
				s0[1] = sorY[0];
				s0[2] = sorZ[0];

				t0[0] = tarX[0];
				t0[1] = tarY[0];
				t0[2] = tarZ[0];

				//this->SourceLandmarks->GetPoint(0,s0);
				//this->TargetLandmarks->GetPoint(0,t0);

				s1[0] = sorX[0];
				s1[1] = sorY[0];
				s1[2] = sorZ[0];

				t1[0] = tarX[0];
				t1[1] = tarY[0];
				t1[2] = tarZ[0];

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

			m_matrix[0] = ww + xx - yy - zz; 
			m_matrix[4] = 2.0*(wz + xy);
			m_matrix[8] = 2.0*(-wy + xz);

			m_matrix[1] = 2.0*(-wz + xy);  
			m_matrix[5] = ww - xx + yy - zz;
			m_matrix[9] = 2.0*(wx + yz);

			m_matrix[2] = 2.0*(wy + xz);
			m_matrix[6] = 2.0*(-wx + yz);
			m_matrix[10] = ww - xx - yy + zz;

			if (transType != HD_LANDMARK_RIGIDBODY)
			{ // add in the scale factor (if desired)
				for(i=0;i<3;i++) 
				{
					m_matrix[i * 4] *= scale;
					m_matrix[i * 4 + 1] *= scale;
					m_matrix[i * 4 + 2] *= scale;
				}
			}
		}//if (eigenvalues[0] == eigenvalues[1] || N_PTS == 2)

		// the translation is given by the difference in the transformed source
		// centroid and the target centroid
		double sx, sy, sz;

		sx =	m_matrix[0] * source_centroid[0] +
				m_matrix[1] * source_centroid[1] +
				m_matrix[2] * source_centroid[2];
		sy =	m_matrix[4] * source_centroid[0] +
				m_matrix[5] * source_centroid[1] +
				m_matrix[6] * source_centroid[2];
		sz =	m_matrix[8] * source_centroid[0] +
				m_matrix[9] * source_centroid[1] +
				m_matrix[10] * source_centroid[2];

		m_matrix[3] = target_centroid[0] - sx;
		m_matrix[7] = target_centroid[1] - sy;
		m_matrix[11] = target_centroid[2] - sz;

		// fill the bottom row of the 4x4 matrix
		m_matrix[12] = 0.0;
		m_matrix[13] = 0.0;
		m_matrix[14] = 0.0;
		m_matrix[15] = 1.0;

		
		// 计算误差
		m_fError = 0.0;
		for(pt=0;pt<N_PTS;pt++)
		{
			// get the origin-centred point (a) in the source set
			//this->SourceLandmarks->GetPoint(pt,a);
			a[0] = sorX[pt];
			a[1] = sorY[pt];
			a[2] = sorZ[pt];

			b[0] = tarX[pt];
			b[1] = tarY[pt];
			b[2] = tarZ[pt];

			hdHomogeneousTransformPoint(m_matrix,a[0],a[1],a[2]);
			//Translate(a[0],a[1],a[2]);
			m_fError += ((a[0] - b[0]) * (a[0] - b[0]) +
				(a[1] - b[1]) * (a[1] - b[1]) + 
				(a[2] - b[2]) * (a[2] - b[2]));
		}
		m_fError /= N_PTS;
		m_fError = sqrt(m_fError);

		delete[] tarX;
		delete[] tarY;
		delete[] tarZ;
		delete[] sorX;
		delete[] sorY;
		delete[] sorZ;
		return true;
	}
private:
	double m_matrix[16];
	double m_fError;
};

}