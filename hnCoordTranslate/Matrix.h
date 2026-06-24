/*! @file
********************************************************************************
<PRE>
模块名       : 
文件名       : Matrix.h 
相关文件     : Matrix.cpp
文件实现功能 : 头文件：声明矩阵运算的函数
作者         : 程鹏  
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本   修改人                 修改内容 

</PRE>
********************************************************************************

* 版权所有(c) 2012, 海达数云技术有限公司, 保留所有权利

*******************************************************************************/

//////////////////////////////////////////////////////////////////////
// Matrix.h: interface for the CMatrix class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MATRIX_H_)
#define _MATRIX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "memory.h"
//#define max(a,b) ((a)>(b)?(a):(b))

namespace hd
{

//class AFX_EXT_CLASS CMatrix 
class  CMatrix
{
public:
	CMatrix(int row=1, int col=1);
	// copy single pointer array in to the Matrix
	CMatrix(int row, int col, double array[]);
	CMatrix(const CMatrix &init);
	virtual ~CMatrix();
	// copy a single pointer array in to the CMatrix
	const CMatrix &copyArray( int r, int c, const double *array );  

public:
	inline double *GetBuffer() const { return ptr;}
	CMatrix &DeleteBuffer();

public:
	//get the sum of all elements
	double getMatrixSum() const;

	// Get the size of the CMatrix
	inline int getSize() const { return  rows * cols; }
	// Get the number of rows
	inline int rowsCount() const { return rows; }
	// Get the number of coloumns
	inline int colsCount() const { return cols; }
	
	// Return true if the CMatrix is a square CMatrix
	// false otherwise
	inline bool isSquare() const { return ( cols == rows ); }

	// return the max value of the CMatrix	
	double getMaxVal() const;
	// return the max value of the cth col.
	double getMaxColVal( const int c ) const; 
	// return the max value of the rth row.	
    double getMaxRowVal( const int r ) const; 
	
	// return the min value of the CMatrix		
	double getMinVal() const; 
	// return the min value of the cth col   
	double getMinColVal( const int c ) const; 
	// return the min value of the rth row   
	double getMinRowVal( const int r ) const; 
	
	void getRow(CMatrix& mtxR, const int r ) const;
	void getCol(CMatrix& mtxC,  const int c ) const;
	// return the mean of the values of the CMatrix	  
	double getMean() const;	
	double getAbsMean() const;	
	// return the mean of the values of the cth col.  
	double getColMean( const int c ) const; 
	double getColAbsMean( const int c ) const; 
	// return the mean of the values of the rth row	  
	double getRowMean( const int r ) const; 
	double getRowAbsMean( const int r ) const; 

	// find the value in the CMatrix and return as a nx2 CMatrix
	const CMatrix findVal( const double v ) const;				 
	// return the number of value in the CMatrix	   
	int getValCount( const double v ) const;	

// operator overleading.........
public:
	// assign CMatrix's
	const CMatrix &operator=( const CMatrix & );
	// I icin
	const CMatrix &operator=( const char );				

	// Overloaded comparison operators 
	// compare equal
	bool operator==( const CMatrix &right ) const;	
	// Determine if two CMatrix's are equal and
	// return true, otherwise return false (uses operator==).
	bool operator!=( const CMatrix &right ) const					 
	{ return ! ( *this == right ); }

	// Overloaded subscript operators
	// subscript operator
	double &operator()( int, int );				
	// subscript operator
	const double &operator()( int, int ) const;		

	// Overloaded math operators
	// add two Matrices
	CMatrix operator+( const CMatrix & );
	// add a value to a CMatrix	
	CMatrix operator+( const double );	
	// add two Matrices modify this											 
	void operator+=( const CMatrix & );
	// add a value to a CMatrix modify this									 
	void operator+=( const double );  

	// substract one CMatrix from an other 
	CMatrix operator-( const CMatrix & );
	// substract a value from a CMatrix	   
	CMatrix operator-( const double );	
    // substract one CMatrix from an other modify this CMatrix				  
	void operator-=( const CMatrix & ); 										  
	// substract a value from a CMatrix modify this CMatrix					  
	void operator-=( const double );  

	// multiply a CMatrix with an other	   
	CMatrix operator*( const CMatrix & );
	// multiply a CMatrix with a value	
	CMatrix operator*( const double );										 
	// multiply a CMatrix with an other modify this CMatrix
	void operator*=( const CMatrix & );
	// multiply a CMatrix with a value modify this CMatrix					   
	void operator*=( const double );

	// divide the CMatrix by a value;	   
	CMatrix operator/( const double );	
	// divide the CMatrix by a value modify the this CMatrix					   
	void operator/=( const double );

	// take a power of the CMatrix
	CMatrix operator^( const int );		

// methods of the CMatrix class
public:
	// convert CMatrix to identity CMatrix of the same size	  
	const CMatrix &convToIdentity(); 
	// convert CMatrix to inverse diagonal	
	const CMatrix &convToInvDiagonal(); 

	// get the transpose of a CMatrix
	CMatrix &getTranspose();

	// calculate the LUD of the CMatrix
	void LUBKS( int phy_size, int *outvect, double *output );
	// calculate the LUD of the CMatrix
	int LUD( int phy_size, int *outvect, int output );
	// get the inverse of the CMatrix and return the CMatrix.
	CMatrix &getInverse();										 
	// extract a subCMatrix from a CMatrix
	CMatrix getSubCMatrix( const int startRow, const int startCol,
							const int rowSize, const int colSize  ) const;

	CMatrix &deleteRow( const int r );	// Delete a row from the CMatrix		    
	CMatrix &deleteCol( const int c );	// Delete a coloumn from the CMatrix	    
	// inserts the row CMatrix to the given row shifting other rows down
	CMatrix &insertRow( const int r, const CMatrix &rVector );  					
	// inserts the coloumn CMatrix to the given coloumn shifting other coloumns right
	CMatrix &insertCol( const int c, const CMatrix &cVector ); 				
	// interchange rows in the CMatrix ( r1 <-> r2 )
	CMatrix &interchangeRows( const int r1, const int r2 );	 				
	// interchange columns in the CMatrix ( c1 <-> c2 )
	CMatrix &interchangeCols( const int c1, const int c2 );
	
	// add the right CMatrix on top of this
	CMatrix &attachTop( const CMatrix & );					 
	// add the right CMatrix under this
	CMatrix &attachBottom( const CMatrix & );				 
	// add the right CMatrix to Left of this
	CMatrix &attachLeft( const CMatrix & );					 
	// add the right CMatrix to Right of this
	CMatrix &attachRight( const CMatrix & );		

	// Return the number of CMatrix objects instantiated
	// static functions can not be const
	//inline int getCMatrixCount() { return CMatrixCount; }

//ext operator
public:
	void Init(int r, int c);
	// 复矩阵乘法
	void CMul(const CMatrix& AR, 
		const CMatrix& AI, 
		const CMatrix& BR, 
		const CMatrix& BI,
		CMatrix& CR, 
		CMatrix& CI) const;
	// 对称正定矩阵的乔里斯基分解与行列式的求值
	void DetCholesky(double* dblDet);   
	// 求行列式值的全选主元高斯消去法
	double DetGauss();       
	// 获取矩阵的指定行矩阵
	int GetRowVector(int nRow, double* pVector) const;	
	// 获取矩阵的指定列矩阵
	int GetColVector(int nCol, double* pVector) const;	
	// 求广义逆的奇异值分解法
	void GInvertUV(CMatrix& mtxAP, 
		CMatrix& mtxU, 
		CMatrix& mtxV, 
		double eps = 0.000000001);
	// 一般实矩阵的奇异值分解
	bool SplitUV(CMatrix& mtxU, CMatrix& mtxV, double eps = 0.000000001);   
	// 求赫申伯格矩阵全部特征值的QR方法
	void HBergEigenv(double dblU[], double dblV[], int nMaxIt = 60, double eps = 0.000000001);
	// 实矩阵求逆的全选主元高斯－约当法
	void InvertGaussJordan(); 
	// 复矩阵求逆的全选主元高斯－约当法
	void InvertGaussJordan(CMatrix& mtxImag);     
	// 对称正定矩阵的求逆
	void InvertSsgj();   
	// 托伯利兹矩阵求逆的埃兰特方法
	void InvertTrench();    
	// 求实对称矩阵特征值与特征向量的雅可比法
	void JacobiEigenv(double dblEigenValue[], 
		CMatrix& mtxEigenVector, 
		int nMaxIt = 60, 
		double eps = 0.000000001);
	// 求实对称矩阵特征值与特征向量的雅可比过关法
	void JacobiEigenv2(double dblEigenValue[], 
		CMatrix& mtxEigenVector, 
		double eps = 0.000000001);
	// 约化一般实矩阵为赫申伯格矩阵的初等相似变换法
	void MakeHberg();
	// 约化对称矩阵为对称三对角阵的豪斯荷尔德变换法
	void MakeSymTri(CMatrix& mtxQ, CMatrix& mtxT, double dblB[], double dblC[]);
	// 求矩阵秩的全选主元高斯消去法
	int RankGauss();
	// 矩阵的三角分解
	void SplitLU(CMatrix& mtxL, CMatrix& mtxU);
	// 一般实矩阵的QR分解
	void SplitQR(CMatrix& mtxQ);  
	// 实对称三对角阵的全部特征值与特征向量的计算
	void SymTriEigenv(double dblB[], double dblC[], 
		CMatrix& mtxQ, int nMaxIt = 60, double eps = 0.000000001);

public:
	int rows;	// number of rows
	int cols;	// number of coloumns
	double *ptr;		// pointer to the first element of CMatrix
	static int CMatrixCount;	// # of CMatrix's instantiated

private:
	void ppp(double a[], double e[], double s[], double v[], int m, int n);
	void sss(double fg[2], double cs[2]);
};
}
#endif // !defined(_MATRIX_H_)
