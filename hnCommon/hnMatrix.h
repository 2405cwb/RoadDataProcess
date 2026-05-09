#pragma once
//#include "hdCore.h"
#include <math.h>
#include <stdio.h>
#include "stdafx.h"

using namespace std;
namespace hn
{
	double HNCOMMOMAPI average(double a[], int n);

	void HNCOMMOMAPI identity_matrix(double m[16]);

	void HNCOMMOMAPI MatAdd(const double *m1,const double *m2,double *result,int rol,int col);

	void HNCOMMOMAPI MatMinus(const double *m1,const double *m2,double *result,int rol,int col);

	void HNCOMMOMAPI mult(const double *m1, const double *m2, double *result, int i_1, int j_12, int j_2);

	void HNCOMMOMAPI mult(const float *m1, const float *m2, float *result, int i_1, int j_12, int j_2);

	int HNCOMMOMAPI invers_matrix(double *m1, int n);

	int HNCOMMOMAPI invers_matrix(float *m1, int n);

	int HNCOMMOMAPI MatInv(double *m1,int n);

	void HNCOMMOMAPI transpose(double *m1, double *m2, int m, int n);

	void HNCOMMOMAPI transpose(float *m1, float *m2,int m,int n);

	void HNCOMMOMAPI MatTrs(double *srcMatrix,int rol,int col,double *dstMatrix);

	void HNCOMMOMAPI MatMult(const double *m1,const double *m2, double *result,int i_1,int j_12,int j_2);

	void HNCOMMOMAPI MatOut(char *fileName,double *mat,int rol,int col);

	//! 根据旋转角度,得到旋转矩阵R.角度单位是弧度
	void HNCOMMOMAPI GetRotateMatByAngle(double phi,double omega,double kappa,double *R);
	//! 根据4*4矩阵获取旋转角度
	//void HDCORE_API getRotationDegrees(double* mat4,double& rotX,double& rotY,double& rotZ);

	// 计算特征值函数
	 int HNCOMMOMAPI matrixEejcb(
		float a[],		//a[]: in 待计算特征值的对称矩阵			
		int n,			//n:   in 对称矩阵的维度
		float v[],		//v[]: in out 计算得到的特征值位于其对角线上
		float eps,		//eps: 误差阈值
		int jt);		//jt:  迭代次数，该函数没有处理该参数，传入大于1的值即可

	// 计算特征值函数
	 int HNCOMMOMAPI matrixEejcb(
		double a[],		//a[]: in 待计算特征值的对称矩阵			
		int n,			//n:   in 对称矩阵的维度
		double v[],		//v[]: in out 计算得到的特征值位于其对角线上
		double eps,		//eps: 误差阈值
		int jt);		//jt:  迭代次数，该函数没有处理该参数，传入大于1的值即可

	class HNCOMMOMAPI hdMatrix
	{
	public:
		hdMatrix(void);
		hdMatrix(int nRows, int nCols);
		hdMatrix(int nRows, int nCols, double* value);
		hdMatrix(const hdMatrix &other);
		~hdMatrix(void);
	private:
		int	m_numColumns;			// 矩阵列数
		int	m_numRows;			// 矩阵行数
		double m_eps;			// 缺省精度
		double* m_elements;	// 矩阵数据缓冲区
	public:
		inline int Columns() {return this->m_numColumns;}
		inline int Rows() {return this->m_numRows;}
		inline double getEps() {return this->m_eps;}
		inline void setEps(double newEps) {this->m_eps = newEps;}
		inline double operator()(int row, int col) const{ return m_elements[col + row * m_numColumns];}//矩阵行列号 从0，0开始
		inline double& operator()(int row, int col) { return m_elements[col + row * m_numColumns];}//矩阵行列号 从0，0开始

		//! 注意：加减乘的操作符会改变原始矩阵的内容，慎用
		inline hdMatrix& operator +(const hdMatrix& m1) { return this->Add(m1);}	
		inline hdMatrix& operator -(const hdMatrix& m1) { return this->Subtract(m1);	}	
		inline hdMatrix& operator *(const hdMatrix& m1)	{ return this->Multiply(m1);}
		inline hdMatrix& operator *(double val)	{ return this->Multiply(val);}
		//! 重载赋值操作符
		hdMatrix& operator=(const hdMatrix& other)
		{
			m_numColumns = other.m_numColumns;
			m_numRows = other.m_numRows;
			m_eps = other.m_eps;
			int nSize = m_numColumns*m_numRows;
			if (m_elements != NULL)
			{
				delete[] m_elements;
				m_elements = NULL;
			}
			m_elements = new double[nSize];
			memcpy(m_elements, other.m_elements, nSize*sizeof(double));
			return *this;
		}

	public:
		bool Init(int nRows, int nCols);
		bool SetElement(int nRow, int nCol, double val);
		void SetData(double* val ,int count);

		/* 加减乘： 会改变原矩阵值 */
		hdMatrix& Add(const hdMatrix& other);
		hdMatrix& Subtract(const hdMatrix& other);
		hdMatrix& Multiply(const hdMatrix& other);
		hdMatrix& Multiply(const double val);

		/* 加减乘： 不改变原矩阵值，运算结果为result */
		void Add(const hdMatrix& other, hdMatrix& result);
		void Subtract(const hdMatrix& other, hdMatrix& result);
		void Multiply(const hdMatrix& other, hdMatrix& result);
		void Multiply(const double val, hdMatrix& result);

		void Transpose(hdMatrix& result);//转置矩阵
		bool Invert(hdMatrix& result);//逆矩阵 初等变换

		bool InvertGaussJordan(hdMatrix& result);//逆矩阵 高斯约当法
		double Minor(int i, int j);// 实矩阵的余子式 Minor Mij
		double DetGauss(); // 求行列式值的全选主元高斯消去法
		double Trail();// 实矩阵的迹

		// 求实对称矩阵特征值与特征向量的雅可比法
		bool JacobiEigenv(double dblEigenValue[], hdMatrix& mtxEigenVector, int nMaxIt = 60, double eps = 0.000001);
		// 求实对称矩阵特征值与特征向量的雅可比过关法
		bool JacobiEigenv2(double dblEigenValue[], hdMatrix& mtxEigenVector, double eps = 0.000001);


		bool MakeUnitMatrix(int nSize);	 // 将方阵初始化为单位矩阵	
		bool MakeUnitMatrix();	         // 将方阵初始化为单位矩阵	
		void Zero();//置为零矩阵
		bool isZero();//判断是否为零矩阵

		bool LeastSquare(hdMatrix *A, hdMatrix *L);//最小二乘解方程
		bool LeastSquare( hdMatrix *A, hdMatrix *L, hdMatrix *P );//最小二乘解方程 P为权矩阵
	};
}
