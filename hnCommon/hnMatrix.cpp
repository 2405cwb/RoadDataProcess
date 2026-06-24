#include "StdAfx.h"
#include "hnMatrix.h"
#include <math.h>
#include <vector>
#include <memory>
#include "assert.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace std;

namespace hn
{

/**
* 基本构造函数
*/
hdMatrix::hdMatrix(void)
{
	m_numRows = 1;
	m_numColumns = 1;
	m_eps = 0.0;
	m_elements = NULL;
	Init(m_numRows, m_numColumns);
}
/**
* 指定行列构造函数
* 
* @param nRows - 指定的矩阵行数
* @param nCols - 指定的矩阵列数
*/
hdMatrix::hdMatrix(int nRows, int nCols)
{	
	m_eps = 0.0;
	m_elements = NULL;
	Init(nRows, nCols);
}
/**
* 指定值构造函数
* 
* @param nRows - 指定的矩阵行数
* @param nCols - 指定的矩阵列数
* @param value - 一维数组，长度为nRows*nCols，存储矩阵各元素的值
*/
hdMatrix::hdMatrix(int nRows, int nCols, double* val)
{	
	m_eps = 0.0;
	m_elements = NULL;
	Init(nRows, nCols);
	SetData(val, nRows*nCols);
}

/**
* 拷贝构造函数
* 
* @param other - 源矩阵
*/
hdMatrix::hdMatrix(const hdMatrix &other)
{	
	m_numColumns = other.m_numColumns;
	m_numRows = other.m_numRows;
	m_eps = 0.0;
	m_elements = NULL;
	Init(m_numRows, m_numColumns);
	SetData(other.m_elements, m_numRows*m_numColumns);
}

hdMatrix::~hdMatrix(void)
{
	if (m_elements)
	{
		delete []m_elements;
		m_elements = NULL;
	}
}
/**
* 初始化函数
* 
* @param nRows - 指定的矩阵行数
* @param nCols - 指定的矩阵列数
* @return bool, 成功返回true, 否则返回false
*/
bool hdMatrix::Init(int nRows, int nCols)
{
	m_numRows = nRows;
	m_numColumns = nCols;
	int nSize = nCols*nRows;
	if (nSize < 0)
		return false;

	if (m_elements)
	{
		delete[] m_elements;
		m_elements = NULL;
	}
	// 分配内存
	m_elements = new double[nSize];
	memset(m_elements, 0, nSize*sizeof(double));
	return true;
}
/**
* 设置指定元素的值
* 
* @param nRow - 元素的行
* @param nCol - 元素的列
* @param value - 指定元素的值
* @return bool 型，说明设置是否成功
*/
bool hdMatrix::SetElement(int nRow, int nCol, double val)
{
	if (nCol < 0 || nCol >= m_numColumns || nRow < 0 || nRow >= m_numRows)
		return false;						// array bounds error

	m_elements[nCol + nRow * m_numColumns] = val;

	return true;
}

/**
* 设置矩阵各元素的值
* 
* @param value - 一维数组，长度为numColumns*numRows，存储
*	              矩阵各元素的值
*/
void hdMatrix::SetData(double* val ,int count)
{
	memcpy(m_elements,val,sizeof(double) * count);
}

/**
* 实现矩阵的加法
* 
* @param other - 与指定矩阵相加的矩阵
* @return Matrix型，指定矩阵与other相加之和
* @如果矩阵的行/列数不匹配，则会抛出异常
*/
hdMatrix& hdMatrix::Add(const hdMatrix& other) 
{
	// 首先检查行列数是否相等
	if (m_numColumns != other.m_numColumns ||
		m_numRows != other.m_numRows)
		throw ("矩阵的行/列数不匹配。");
	
	// 矩阵加法
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)			
			SetElement(i,j,(*this)(i,j) + other(i, j));
	}

	return *this ;
}

void hdMatrix::Add( const hdMatrix& other, hdMatrix& result )
{
	// 首先检查行列数是否相等
	if (m_numColumns != other.m_numColumns ||
		m_numRows != other.m_numRows)
		throw ("矩阵的行/列数不匹配。");

	// 初始化结果矩阵
	result.Init(m_numRows, m_numColumns);

	// 矩阵加法
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)
			result.SetElement(i, j, (*this)(i, j) + other(i, j));
	}	
}

/**
* 实现矩阵的减法
* 
* @param other - 与指定矩阵相减的矩阵
* @return Matrix型，指定矩阵与other相减之差
* @如果矩阵的行/列数不匹配，则会抛出异常
*/
hdMatrix& hdMatrix::Subtract(const hdMatrix& other) 
{
	if (m_numColumns != other.m_numColumns ||
		m_numRows != other.m_numRows)
		throw ("矩阵的行/列数不匹配。");	

	// 进行减法操作
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)			
			SetElement(i,j,(*this)(i,j) - other(i, j));
	}

	return *this;
}

void hdMatrix::Subtract( const hdMatrix& other, hdMatrix& result )
{
	// 首先检查行列数是否相等
	if (m_numColumns != other.m_numColumns ||
		m_numRows != other.m_numRows)
		throw ("矩阵的行/列数不匹配。");

	// 初始化结果矩阵
	result.Init(m_numRows, m_numColumns);

	// 矩阵加法
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)
			result.SetElement(i, j, (*this)(i, j) - other(i, j));
	}	
}

/**
* 实现矩阵的数乘
* 
* @param value - 与指定矩阵相乘的实数
* @return hdMatrix型，指定矩阵与value相乘之积
*/
hdMatrix& hdMatrix::Multiply(const double val) 
{	

	// 进行数乘
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)			
			SetElement(i, j, (*this)(i, j) * val);
	}

	return *this ;
}


void hdMatrix::Multiply( const double val, hdMatrix& result )
{
	// 进行数乘
	result.Init(m_numRows, m_numColumns);
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j <  m_numColumns; ++j)
			result.SetElement(i, j, (*this)(i, j) * val) ;
	}

}

/**
* 实现矩阵的乘法
* 
* @param other - 与指定矩阵相乘的矩阵
* @return Matrix型，指定矩阵与other相乘之积
* @如果矩阵的行/列数不匹配，则会抛出异常
*/
void hdMatrix::Multiply(const hdMatrix& other, hdMatrix& result) 
{
	// 首先检查行列数是否符合要求
	if (m_numColumns != other.m_numRows )
	{
		char* err = "矩阵的行/列数不匹配。";
		throw err;
	}

	// construct the object we are going to return
	result.Init(m_numRows, other.m_numColumns);

	// 矩阵乘法，即
	//
	// [A][B][C]   [G][H]     [A*G + B*I + C*K][A*H + B*J + C*L]
	// [D][E][F] * [I][J] =   [D*G + E*I + F*K][D*H + E*J + F*L]
	//             [K][L]
	//
	double	value ;
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j < other.m_numColumns ; ++j)
		{
			value = 0.0 ;
			for (int k = 0 ; k < m_numColumns ; ++k)
			{
				value += (*this)(i, k) * other(k, j) ;
			}
			result.SetElement(i, j, value) ;
			
		}
	}

}
hdMatrix& hdMatrix::Multiply(const hdMatrix& other) 
{
	// 首先检查行列数是否符合要求
	if (m_numColumns != other.m_numRows )
	{
		char* err = "矩阵的行/列数不匹配。";
		throw err;
	}

	// ruct the object we are going to return
	hdMatrix tmp = *this;
	Init(m_numRows, other.m_numColumns);
	//hdMatrix result(m_numRows, other.m_numColumns);

	// 矩阵乘法，即
	//
	// [A][B][C]   [G][H]     [A*G + B*I + C*K][A*H + B*J + C*L]
	// [D][E][F] * [I][J] =   [D*G + E*I + F*K][D*H + E*J + F*L]
	//             [K][L]
	//
	double	value ;
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j < other.m_numColumns ; ++j)
		{
			value = 0.0 ;
			for (int k = 0 ; k < m_numColumns ; ++k)
			{
				value += tmp(i, k) * other(k, j) ;
			}
			SetElement(i, j, value) ;
			
		}
	}
	return *this;
}

/**
* 矩阵的转置
* 
* @return Matrix型，指定矩阵转置矩阵
*/
void hdMatrix::Transpose(hdMatrix& result) 
{
	// 构造目标矩阵
	//hdMatrix* matrix_trans = new hdMatrix(m_numColumns, m_numRows);
	result.Init(m_numColumns,m_numRows);

	// 转置各元素
	for (int i = 0 ; i < m_numRows ; ++i)
	{
		for (int j = 0 ; j < m_numColumns ; ++j)
		{
			//double ele = (*this)(i, j);
			//matrix_trans->SetElement(j, i, ele);
			result.SetElement(j,i,(*this)(i, j));
		}
	}
	//return matrix_trans;
}

/**
* 实矩阵求逆的全选主元高斯－约当法
* 
* @return bool型，求逆是否成功
*/
bool hdMatrix::InvertGaussJordan(hdMatrix& result)
{
	int i,j,k,l,u,v;
	double d = 0, p = 0;

	result = *this;//先拷贝一份
	//result.Init(m_numRows,m_numColumns);
	// 分配内存
	//int* pnRow = new int[m_numColumns];
	//int* pnCol = new int[m_numColumns];

	int pnRow[20] = {0};// = new int[m_numColumns];
	int pnCol[20] = {0};// = new int[m_numColumns];

	//vector<int> pnRow(m_numColumns);
	//vector<int> pnCol(m_numColumns);
	// 消元
	for (k=0; k<=m_numColumns-1; k++)
	{ 
		d=0.0;
		for (i=k; i<=m_numColumns-1; i++)
		{
			for (j=k; j<=m_numColumns-1; j++)
			{ 
				l=i*m_numColumns+j; 
				p=fabs(result.m_elements[l]);
				if (p>d) 
				{ 
					d=p; 
					pnRow[k]=i; 
					pnCol[k]=j;
				}
			}
		}

		// 失败
		if (d == 0.0)
		{
			//delete[] pnRow;
			//delete[] pnCol;
			return false;
		}

		if (pnRow[k] != k)
		{
			for (j=0; j<=m_numColumns-1; j++)
			{ 
				u=k*m_numColumns+j; 
				v=pnRow[k]*m_numColumns+j;
				p=result.m_elements[u]; 

				result.m_elements[u]=result.m_elements[v]; 
				result.m_elements[v]=p;
			}
		}

		if (pnCol[k] != k)
		{
			for (i=0; i<=m_numColumns-1; i++)
			{ 
				u=i*m_numColumns+k; 
				v=i*m_numColumns+pnCol[k];
				p=result.m_elements[u]; 
				result.m_elements[u]=result.m_elements[v]; 
				result.m_elements[v]=p;
			}
		}

		l=k*m_numColumns+k;
		result.m_elements[l]=1.0/result.m_elements[l];
		for (j=0; j<=m_numColumns-1; j++)
		{
			if (j != k)
			{ 
				u=k*m_numColumns+j; 
				result.m_elements[u]=result.m_elements[u]*result.m_elements[l];
			}
		}

		for (i=0; i<=m_numColumns-1; i++)
		{
			if (i!=k)
			{
				for (j=0; j<=m_numColumns-1; j++)
				{
					if (j!=k)
					{ 
						u=i*m_numColumns+j;
						result.m_elements[u]=result.m_elements[u]-result.m_elements[i*m_numColumns+k]*result.m_elements[k*m_numColumns+j];
					}
				}
			}
		}

		for (i=0; i<=m_numColumns-1; i++)
		{
			if (i!=k)
			{ 
				u=i*m_numColumns+k; 
				result.m_elements[u]=-result.m_elements[u]*result.m_elements[l];
			}
		}
	}

	// 调整恢复行列次序
	for (k=m_numColumns-1; k>=0; k--)
	{ 
		if (pnCol[k]!=k)
		{
			for (j=0; j<=m_numColumns-1; j++)
			{ 
				u=k*m_numColumns+j; 
				v=pnCol[k]*m_numColumns+j;
				p=result.m_elements[u]; 
				result.m_elements[u]=result.m_elements[v]; 
				result.m_elements[v]=p;
			}
		}

		if (pnRow[k]!=k)
		{
			for (i=0; i<=m_numColumns-1; i++)
			{ 
				u=i*m_numColumns+k; 
				v=i*m_numColumns+pnRow[k];
				p=result.m_elements[u]; 
				result.m_elements[u]=result.m_elements[v]; 
				result.m_elements[v]=p;
			}
		}
	}

	//delete[] pnRow;
	//delete[] pnCol;
	// 成功返回
	return true;
}

void hdMatrix::Zero()
{
	memset(m_elements,0,m_numColumns*m_numRows*sizeof(double));	
}

bool hdMatrix::isZero()
{
	bool retVal = true;
	for (int i = 0;i<m_numRows*m_numColumns;i++)
	{
		if (fabs(m_elements[i] - 0.0) > 0.000000001)
		{
			retVal = false;
			break;
		}
	}
	return retVal;
}

double hdMatrix::Minor( int i, int j )//下标从0，0开始
{
	double tmp;
	hdMatrix A(this->m_numRows - 1,this->m_numColumns - 1);
	int a = 0;
	for (int m = 0;m<this->m_numRows;m++)
	{
		for (int n = 0;n<this->m_numColumns;n++)
		{
			if (m==i) continue;
			if (n==j) continue;
			A.m_elements[a] = (*this)(m, n);
			a++;
		}
	}
	tmp = A.DetGauss();
	return tmp;
}

//////////////////////////////////////////////////////////////////////
// 求行列式值的全选主元高斯消去法
//
// 参数：无
//
// 返回值：double型，行列式的值
//////////////////////////////////////////////////////////////////////
double hdMatrix::DetGauss()
{ 
	int i,j,k,is,js,l,u,v;
	double f,det,q,d;

	// 初值
	f=1.0; 
	det=1.0;

	// 消元
	for (k=0; k<=m_numColumns-2; k++)
	{ 
		q=0.0;
		for (i=k; i<=m_numColumns-1; i++)
		{
			for (j=k; j<=m_numColumns-1; j++)
			{ 
				l=i*m_numColumns+j; 
				d=fabs(m_elements[l]);
				if (d>q) 
				{ 
					q=d; 
					is=i; 
					js=j;
				}
			}
		}

		if (q == 0.0)
		{ 
			det=0.0; 
			return(det);
		}

		if (is!=k)
		{ 
			f=-f;
			for (j=k; j<=m_numColumns-1; j++)
			{ 
				u=k*m_numColumns+j; 
				v=is*m_numColumns+j;
				d=m_elements[u]; 
				m_elements[u]=m_elements[v]; 
				m_elements[v]=d;
			}
		}

		if (js!=k)
		{ 
			f=-f;
			for (i=k; i<=m_numColumns-1; i++)
			{
				u=i*m_numColumns+js; 
				v=i*m_numColumns+k;
				d=m_elements[u]; 
				m_elements[u]=m_elements[v]; 
				m_elements[v]=d;
			}
		}

		l=k*m_numColumns+k;
		det=det*m_elements[l];
		for (i=k+1; i<=m_numColumns-1; i++)
		{ 
			d=m_elements[i*m_numColumns+k]/m_elements[l];
			for (j=k+1; j<=m_numColumns-1; j++)
			{ 
				u=i*m_numColumns+j;
				m_elements[u]=m_elements[u]-d*m_elements[k*m_numColumns+j];
			}
		}
	}

	// 求值
	det=f*det*m_elements[m_numColumns*m_numColumns-1];

	return(det);
}

//////////////////////////////////////////////////////////////////////
// 将方阵初始化为单位矩阵
//
// 参数：
// 1. int nSize - 方阵行列数
//
// 返回值：bool 型，初始化是否成功
//////////////////////////////////////////////////////////////////////
bool hdMatrix::MakeUnitMatrix(int nSize)
{
	if (! Init(nSize, nSize))
		return false;

	for (int i=0; i<nSize; ++i)
		SetElement(i, i, 1);

	return true;
}
bool hdMatrix::MakeUnitMatrix()
{
	if (this->Rows() != this->Columns())
	{
		return false;
	}
	int nSize = this->Rows();
	return this->MakeUnitMatrix(nSize);
}

double hdMatrix::Trail()
{
	assert(this->m_numRows == this->m_numColumns);
	double tr = 0.0;
	for (int i=0;i<this->m_numRows;i++)
	{
		tr += (*this)(i, i);
	}
	return tr;
}

//////////////////////////////////////////////////////////////////////
// 求实对称矩阵特征值与特征向量的雅可比法
//
// 参数：
// 1. double dblEigenValue[] - 一维数组，长度为矩阵的阶数，返回时存放特征值
// 2. hdMatrix& mtxEigenVector - 返回时存放特征向量矩阵，其中第i列为与
//    数组dblEigenValue中第j个特征值对应的特征向量
// 3. int nMaxIt - 迭代次数，默认值为60
// 4. double eps - 计算精度，默认值为0.000001
//
// 返回值：bool型，求解是否成功
//////////////////////////////////////////////////////////////////////
bool hdMatrix::JacobiEigenv(double dblEigenValue[], hdMatrix& mtxEigenVector, int nMaxIt /*= 60*/, double eps /*= 0.000001*/)
{ 
	int i,j,p,q,u,w,t,s,l;
	double fm,cn,sn,omega,x,y,d;

	if (! mtxEigenVector.Init(m_numColumns, m_numColumns))
		return false;

	l=1;
	for (i=0; i<=m_numColumns-1; i++)
	{ 
		mtxEigenVector.m_elements[i*m_numColumns+i]=1.0;
		for (j=0; j<=m_numColumns-1; j++)
			if (i!=j) 
				mtxEigenVector.m_elements[i*m_numColumns+j]=0.0;
	}

	while (true)
	{ 
		fm=0.0;
		for (i=1; i<=m_numColumns-1; i++)
		{
			for (j=0; j<=i-1; j++)
			{ 
				d=fabs(m_elements[i*m_numColumns+j]);
				if ((i!=j)&&(d>fm))
				{ 
					fm=d; 
					p=i; 
					q=j;
				}
			}
		}

		if (fm<eps)
		{
			for (i=0; i<m_numColumns; ++i)
				dblEigenValue[i] = (*this)(i,i);
			return true;
		}

		if (l>nMaxIt)  
			return false;

		l=l+1;
		u=p*m_numColumns+q; 
		w=p*m_numColumns+p; 
		t=q*m_numColumns+p; 
		s=q*m_numColumns+q;
		x=-m_elements[u]; 
		y=(m_elements[s]-m_elements[w])/2.0;
		omega=x/sqrt(x*x+y*y);

		if (y<0.0) 
			omega=-omega;

		sn=1.0+sqrt(1.0-omega*omega);
		sn=omega/sqrt(2.0*sn);
		cn=sqrt(1.0-sn*sn);
		fm=m_elements[w];
		m_elements[w]=fm*cn*cn+m_elements[s]*sn*sn+m_elements[u]*omega;
		m_elements[s]=fm*sn*sn+m_elements[s]*cn*cn-m_elements[u]*omega;
		m_elements[u]=0.0; 
		m_elements[t]=0.0;
		for (j=0; j<=m_numColumns-1; j++)
		{
			if ((j!=p)&&(j!=q))
			{ 
				u=p*m_numColumns+j; w=q*m_numColumns+j;
				fm=m_elements[u];
				m_elements[u]=fm*cn+m_elements[w]*sn;
				m_elements[w]=-fm*sn+m_elements[w]*cn;
			}
		}

		for (i=0; i<=m_numColumns-1; i++)
		{
			if ((i!=p)&&(i!=q))
			{ 
				u=i*m_numColumns+p; 
				w=i*m_numColumns+q;
				fm=m_elements[u];
				m_elements[u]=fm*cn+m_elements[w]*sn;
				m_elements[w]=-fm*sn+m_elements[w]*cn;
			}
		}

		for (i=0; i<=m_numColumns-1; i++)
		{ 
			u=i*m_numColumns+p; 
			w=i*m_numColumns+q;
			fm=mtxEigenVector.m_elements[u];
			mtxEigenVector.m_elements[u]=fm*cn+mtxEigenVector.m_elements[w]*sn;
			mtxEigenVector.m_elements[w]=-fm*sn+mtxEigenVector.m_elements[w]*cn;
		}
	}

	for (i=0; i<m_numColumns; ++i)
		dblEigenValue[i] = (*this)(i,i);

	return true;
}

//////////////////////////////////////////////////////////////////////
// 求实对称矩阵特征值与特征向量的雅可比过关法
//
// 参数：
// 1. double dblEigenValue[] - 一维数组，长度为矩阵的阶数，返回时存放特征值
// 2. hdMatrix& mtxEigenVector - 返回时存放特征向量矩阵，其中第i列为与
//    数组dblEigenValue中第j个特征值对应的特征向量
// 3. double eps - 计算精度，默认值为0.000001
//
// 返回值：bool型，求解是否成功
//////////////////////////////////////////////////////////////////////
bool hdMatrix::JacobiEigenv2(double dblEigenValue[], hdMatrix& mtxEigenVector, double eps /*= 0.000001*/)
{ 
	int i,j,p,q,u,w,t,s;
	double ff,fm,cn,sn,omega,x,y,d;

	if (! mtxEigenVector.Init(m_numColumns, m_numColumns))
		return false;

	for (i=0; i<=m_numColumns-1; i++)
	{ 
		mtxEigenVector.m_elements[i*m_numColumns+i]=1.0;
		for (j=0; j<=m_numColumns-1; j++)
			if (i!=j) 
				mtxEigenVector.m_elements[i*m_numColumns+j]=0.0;
	}

	ff=0.0;
	for (i=1; i<=m_numColumns-1; i++)
	{
		for (j=0; j<=i-1; j++)
		{ 
			d=m_elements[i*m_numColumns+j]; 
			ff=ff+d*d; 
		}
	}

	ff=sqrt(2.0*ff);

Loop_0:

	ff=ff/(1.0*m_numColumns);

Loop_1:

	for (i=1; i<=m_numColumns-1; i++)
	{
		for (j=0; j<=i-1; j++)
		{ 
			d=fabs(m_elements[i*m_numColumns+j]);
			if (d>ff)
			{ 
				p=i; 
				q=j;
				goto Loop_2;
			}
		}
	}

	if (ff<eps) 
	{
		for (i=0; i<m_numColumns; ++i)
			dblEigenValue[i] = (*this)(i,i);
		return true;
	}

	goto Loop_0;

Loop_2: 

	u=p*m_numColumns+q; 
	w=p*m_numColumns+p; 
	t=q*m_numColumns+p; 
	s=q*m_numColumns+q;
	x=-m_elements[u]; 
	y=(m_elements[s]-m_elements[w])/2.0;
	omega=x/sqrt(x*x+y*y);
	if (y<0.0) 
		omega=-omega;

	sn=1.0+sqrt(1.0-omega*omega);
	sn=omega/sqrt(2.0*sn);
	cn=sqrt(1.0-sn*sn);
	fm=m_elements[w];
	m_elements[w]=fm*cn*cn+m_elements[s]*sn*sn+m_elements[u]*omega;
	m_elements[s]=fm*sn*sn+m_elements[s]*cn*cn-m_elements[u]*omega;
	m_elements[u]=0.0; m_elements[t]=0.0;

	for (j=0; j<=m_numColumns-1; j++)
	{
		if ((j!=p)&&(j!=q))
		{ 
			u=p*m_numColumns+j; 
			w=q*m_numColumns+j;
			fm=m_elements[u];
			m_elements[u]=fm*cn+m_elements[w]*sn;
			m_elements[w]=-fm*sn+m_elements[w]*cn;
		}
	}

	for (i=0; i<=m_numColumns-1; i++)
	{
		if ((i!=p)&&(i!=q))
		{ 
			u=i*m_numColumns+p; 
			w=i*m_numColumns+q;
			fm=m_elements[u];
			m_elements[u]=fm*cn+m_elements[w]*sn;
			m_elements[w]=-fm*sn+m_elements[w]*cn;
		}
	}

	for (i=0; i<=m_numColumns-1; i++)
	{ 
		u=i*m_numColumns+p; 
		w=i*m_numColumns+q;
		fm=mtxEigenVector.m_elements[u];
		mtxEigenVector.m_elements[u]=fm*cn+mtxEigenVector.m_elements[w]*sn;
		mtxEigenVector.m_elements[w]=-fm*sn+mtxEigenVector.m_elements[w]*cn;
	}

	goto Loop_1;
}

bool hdMatrix::LeastSquare( hdMatrix *A, hdMatrix *L )
{
	bool retVal = false;
	hdMatrix P(A->Rows(),A->Rows());
	P.MakeUnitMatrix();

	retVal = LeastSquare( A, L, &P );
	return retVal;

}

bool hdMatrix::LeastSquare( hdMatrix *A, hdMatrix *L, hdMatrix *P )
{
	bool retVal = false;
	if (A->Rows() != L->Rows() || 
		A->Columns() != this->Rows() || 
		this->Columns() != 1 ||
		P->Rows() != P->Columns() ||
		P->Rows() != A->Rows())
	{
		return retVal;
	}
	int nA = A->Rows();//A的行数，即方程的个数
	int nH = this->Rows();//未知数个数，A的列数
	hdMatrix At(nH, nA);
	hdMatrix AtP(nH, nA);
	hdMatrix AtPA(nH, nH);
	hdMatrix AtPA_inv(nH, nH);
	hdMatrix AtPL(nH, 1);

	A->Transpose(At);
	At.Multiply(*P, AtP);	//AtP = At * P;
	AtP.Multiply(*A, AtPA);	//AtPA = AtP * A;
	AtP.Multiply(*L, AtPL);  //AtPL = AtP * L;
	retVal = AtPA.InvertGaussJordan(AtPA_inv);
	AtPA_inv.Multiply(AtPL, *this);	//H = ATA_INVERSE * ATL;

	return retVal;

}

bool hdMatrix::Invert( hdMatrix& result )//矩阵求逆
{
	if (this->Columns() != this->Rows())
	{
		return false;
	}
	int n = this->Columns();
	double *a = new double[n*n];
	for (int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
			*(a+i*n+j) = (*this)(i,j);
	}
	int i,j,k;
	for(k=0;k<n;k++)
	{
		for(i=0;i<n;i++)
		{
			if (*(a+k*n+k) == 0)
			{
				//cerr<<"求逆矩阵错误！\n分母为零！"<<endl;
				return false;
			}
			if(i!=k)
				*(a+i*n+k)=-*(a+i*n+k)/(*(a+k*n+k));
		}
		*(a+k*n+k)=1/(*(a+k*n+k));
		for(i=0;i<n;i++)
		{
			if(i!=k)
			{
				for(j=0;j<n;j++)
				{
					if(j!=k)
						*(a+i*n+j)+=*(a+k*n+j)* *(a+i*n+k);
				}
			}
		}
		for(j=0;j<n;j++)
		{
			if(j!=k)
				*(a+k*n+j)*=*(a+k*n+k);
		}
	}

	for (int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
			result(i,j) = *(a+i*n+j);
	}

	delete []a;
	return true;

}

double average(double a[], int n)
{
	double s = 0.0;
	int i;
	for(i=0;i<n;i++)
		s += a[i];
	return s / n;
}

void MatAdd(const double *m1,const double *m2,double *result,int rol,int col)
{
	int i,j;
	for (i=0;i<rol;i++)
	{
		for (j=0;j<col;j++)
		{
			result[i*col+j]=m1[i*col+j]+m2[i*col+j];
		}
	}
}

void MatMinus(const double *m1,const double *m2,double *result,int rol,int col)
{
	int i,j;
	for (i=0;i<rol;i++)
	{
		for (j=0;j<col;j++)
		{
			result[i*col+j]=m1[i*col+j]-m2[i*col+j];
		}
	}
}

void MatTrs(double *srcMatrix,int rol,int col,double *dstMatrix)
{
	//如果dstMatrix没有分配内存，则返回失败;
	if(srcMatrix==NULL||dstMatrix==NULL)
		return;

	for(int i=0;i<rol;i++)
		for(int j=0;j<col;j++)
		{
			dstMatrix[j*rol+i]=srcMatrix[i*col+j];
		}
}

void MatMult(const double *m1,const double *m2, double *result,int i_1,int j_12,int j_2)
{
	int i,j,k;
	for(i=0;i<i_1;i++)
		for(j=0;j<j_2;j++){
			result[i*j_2+j]=0.0;
			for(k=0;k<j_12;k++)
				result[i*j_2+j]+=m1[i*j_12+k]*m2[j+k*j_2];
		}
		return;
}

int MatInv(double *Matrix,int m)
{ 
	int *is,*js;
	int i,j,k,l,u,v;
	double temp,max_v;
	is = new int[m];
	js = new int[m];
	if(is==0||js==0)	return 0;

	for(k=0;k<m;k++)
	{
		max_v = 0.0;
		for(i=k;i<m;i++)
			for(j=k;j<m;j++)
			{
				temp = fabs(Matrix[i*m+j]);
				if( temp>max_v )
				{
					max_v = temp; 
					is[k] = i; 
					js[k] = j;
				}
			}
			if(max_v==0.0)
			{
				delete []is; 
				delete []js;
				//printf("invers is not availble!\m");
			}
			if(is[k]!=k)
				for(j=0;j<m;j++)
				{
					u = k*m+j;
					v = is[k]*m+j;
					temp = Matrix[u]; 
					Matrix[u] = Matrix[v];
					Matrix[v] = temp;
				}
				if(js[k]!=k)
					for(i=0;i<m;i++){
						u=i*m+k; v=i*m+js[k];
						temp=Matrix[u]; Matrix[u]=Matrix[v]; Matrix[v]=temp;
					}
					l=k*m+k;
					Matrix[l]=1.0/Matrix[l];
					for(j=0;j<m;j++)
						if(j!=k){
							u=k*m+j;
							Matrix[u]*=Matrix[l];
						}
						for(i=0;i<m;i++)
							if(i!=k)
								for(j=0;j<m;j++)
									if(j!=k){
										u=i*m+j;
										Matrix[u]-=Matrix[i*m+k]*Matrix[k*m+j];
									}
									for(i=0;i<m;i++)
										if(i!=k){
											u=i*m+k;
											Matrix[u]*=-Matrix[l];
										}
	}
	for(k=m-1;k>=0;k--){
		if(js[k]!=k)
			for(j=0;j<m;j++){
				u=k*m+j; v=js[k]*m+j;
				temp=Matrix[u]; Matrix[u]=Matrix[v]; Matrix[v]=temp;
			}
			if(is[k]!=k)
				for(i=0;i<m;i++){
					u=i*m+k; v=i*m+is[k];
					temp=Matrix[u]; Matrix[u]=Matrix[v]; Matrix[v]=temp;
				}
	}
	delete []is; delete []js;
	return 1;
}

void mult(const double *m1, const double *m2, double *result, int i_1, int j_12, int j_2)
{
	int i,j,k;
	for(i=0;i<i_1;i++)
		for(j=0;j<j_2;j++)
		{
			result[i*j_2+j]=0.0;
			for(k=0;k<j_12;k++)
				result[i*j_2+j]+=m1[i*j_12+k]*m2[j+k*j_2];
		}
}

void mult(const float *m1, const float *m2, float *result, int i_1, int j_12, int j_2)
{
	int i,j,k;
	for(i=0;i<i_1;i++)
		for(j=0;j<j_2;j++)
		{
			result[i*j_2+j]=0.0;
			for(k=0;k<j_12;k++)
				result[i*j_2+j]+=m1[i*j_12+k]*m2[j+k*j_2];
		}
}

int invers_matrix(double *m1, int n)
{
	int *is,*js;
	int i,j,k,l,u,v;
	double temp,max_v;
	is=new int[n];
	js=new int [n];

	for(k=0;k<n;k++){
		max_v=0.0;
		for(i=k;i<n;i++)
			for(j=k;j<n;j++){
				temp=fabs(m1[i*n+j]);
				if(temp>max_v){
					max_v=temp; is[k]=i; js[k]=j;
				}
			}
			if(max_v==0.0){
				delete [] is;
				delete [] js;
				return(0);
			}
			if(is[k]!=k)
				for(j=0;j<n;j++){
					u=k*n+j; v=is[k]*n+j;
					temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
				}
				if(js[k]!=k)
					for(i=0;i<n;i++){
						u=i*n+k; v=i*n+js[k];
						temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
					}
					l=k*n+k;
					m1[l]=1.0f/m1[l];
					for(j=0;j<n;j++)
						if(j!=k){
							u=k*n+j;
							m1[u]*=m1[l];
						}
						for(i=0;i<n;i++)
							if(i!=k)
								for(j=0;j<n;j++)
									if(j!=k){
										u=i*n+j;
										m1[u]-=m1[i*n+k]*m1[k*n+j];
									}
									for(i=0;i<n;i++)
										if(i!=k){
											u=i*n+k;
											m1[u]*=-m1[l];
										}
	}
	for(k=n-1;k>=0;k--){
		if(js[k]!=k)
			for(j=0;j<n;j++){
				u=k*n+j; v=js[k]*n+j;
				temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
			}
			if(is[k]!=k)
				for(i=0;i<n;i++){
					u=i*n+k; v=i*n+is[k];
					temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
				}
	}

	delete [] is;
	delete [] js;
	return(1);
}

int invers_matrix(float *m1, int n)
{
	int *is,*js;
	int i,j,k,l,u,v;
	float temp,max_v;
	is=new int[n];
	js=new int [n];

	for(k=0;k<n;k++){
		max_v=0.0;
		for(i=k;i<n;i++)
			for(j=k;j<n;j++){
				temp=fabs(m1[i*n+j]);
				if(temp>max_v){
					max_v=temp; is[k]=i; js[k]=j;
				}
			}
			if(max_v==0.0){
				delete [] is;
				delete [] js;
				return(0);
			}
			if(is[k]!=k)
				for(j=0;j<n;j++){
					u=k*n+j; v=is[k]*n+j;
					temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
				}
				if(js[k]!=k)
					for(i=0;i<n;i++){
						u=i*n+k; v=i*n+js[k];
						temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
					}
					l=k*n+k;
					m1[l]=1.0f/m1[l];
					for(j=0;j<n;j++)
						if(j!=k){
							u=k*n+j;
							m1[u]*=m1[l];
						}
						for(i=0;i<n;i++)
							if(i!=k)
								for(j=0;j<n;j++)
									if(j!=k){
										u=i*n+j;
										m1[u]-=m1[i*n+k]*m1[k*n+j];
									}
									for(i=0;i<n;i++)
										if(i!=k){
											u=i*n+k;
											m1[u]*=-m1[l];
										}
	}
	for(k=n-1;k>=0;k--){
		if(js[k]!=k)
			for(j=0;j<n;j++){
				u=k*n+j; v=js[k]*n+j;
				temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
			}
			if(is[k]!=k)
				for(i=0;i<n;i++){
					u=i*n+k; v=i*n+is[k];
					temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
				}
	}

	delete [] is;
	delete [] js;
	return(1);
}

void transpose(double *m1, double *m2, int m, int n)
{
	//指针为空则返回错误
	if(m1==NULL||m2==NULL)
		return;

	int i,j;                                        
	for(i=0;i<m;i++)                                
		for(j=0;j<n;j++)                        
			m2[j*m+i]=m1[i*n+j];            
}

void transpose(float *m1, float *m2, int m, int n)
{
	//指针为空则返回错误
	if(m1==NULL||m2==NULL)
		return;

	int i,j;                                        
	for(i=0;i<m;i++)                                
		for(j=0;j<n;j++)                        
			m2[j*m+i]=m1[i*n+j];            
}

void MatOut(char *fileName,double *mat,int rol,int col)
{
	FILE *fp = NULL;
	fopen_s(&fp,fileName, "w");
	if (!fp)
	{
		return;
	}
	for (int i=0;i<rol;i++)
	{
		for (int j=0;j<col;j++)
		{
			fprintf(fp,"%lf",mat[i*col+j]);
			if (j!=col-1)
			{
				fprintf(fp," ");
			}
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
}

void GetRotateMatByAngle(double phi,double omega,double kappa,double *R)
{
	//R矩阵
	double a1,a2,a3;
	double b1,b2,b3;
	double c1,c2,c3;

	a1 = cos(phi)*cos(kappa) - sin(phi)*sin(omega)*sin(kappa);
	a2 = -cos(phi)*sin(kappa) - sin(phi)*sin(omega)*cos(kappa);
	a3 = -sin(phi)*cos(omega);
	b1 = cos(omega)*sin(kappa);
	b2 = cos(omega)*cos(kappa);
	b3 = -sin(omega);
	c1 = sin(phi)*cos(kappa) + cos(phi)*sin(omega)*sin(kappa);
	c2 = -sin(phi)*sin(kappa) + cos(phi)*sin(omega)*cos(kappa);
	c3 = cos(phi)*cos(omega);

	R[0] = a1;R[1] = a2;R[2] = a3;
	R[3] = b1;R[4] = b2;R[5] = b3;
	R[6] = c1;R[7] = c2;R[8] = c3;
}

void identity_matrix( double m[16] )
{
	m[0] = m[5] = m[10] = m[15] = 1.0;
	m[1] = m[2] = m[3] = m[4] =
	m[6] = m[7] = m[8] = m[9] =
	m[11] = m[12] = m[13] = m[14] = 0.0;
}

// 计算特征值函数
int matrixEejcb(
	float a[],		//a[]: in 待计算特征值的对称矩阵			
	int n,			//n:   in 对称矩阵的维度
	float v[],		//v[]: in out 计算得到的特征值位于其对角线上
	float eps,		//eps: 误差阈值
	int jt)			//jt:  迭代次数，该函数没有处理该参数，传入大于1的值即可
{
	int i,j,p,q,u,w,t,s,l;
	float fm,cn,sn,omega,x,y,d;
	l = 1;
	for (i = 0;i <= n-1;i++)
	{
		v[i*n+i] = 1.0;
		for (j = 0;j <= n-1;j++)
		{
			if (i != j)
			{
				v[i*n+j] = 0.0;
			}
		}
	}
	while (l == 1)
	{
		fm = 0.0;
		for (i = 0;i <= n-1;i++)
		{
			for (j = 0;j <= n-1;j++)
			{
				d = fabs(a[i*n+j]);
				if ((i != j)&&(d > fm))
				{
					fm = d;
					p = i;
					q = j;
				}
			}
		}
		if (fm < eps)
		{
			return(1);
		}
		if (l > jt)
		{
			return(-1);
		}
		l = l + 1;
		u = p * n + q;
		w = p * n + p;
		t = q * n + p;
		s = q * n + q;
		x = -a[u];
		y = (a[s] - a[w]) / 2.0f;
		omega = x / sqrt(x * x + y * y);
		if (y < 0.0)
		{
			omega = -omega;
		}
		sn = 1.0f + sqrt(1.0f - omega*omega);
		sn = omega / sqrt(2.0f * sn);
		cn = sqrt(1.0f - sn * sn);
		fm = a[w];
		a[w] = fm * cn * cn + a[s] * sn * sn + a[u] * omega;
		a[s] = fm * sn * sn + a[s] * cn * cn - a[u] * omega;
		a[u] = 0.0;
		a[t] = 0.0;
		for (j = 0;j <= n-1;j++)
		{
			if ((j != p)&&(j != q))
			{
				u = p * n + j;
				w = q * n + j;
				fm = a[u];
				a[u] = fm * cn + a[w] * sn;
				a[w] = -fm * sn + a[w] * cn;
			}
		}

		for (i = 0;i <= n-1;i++)
		{
			if ((i != p)&&(i != q))
			{
				u = i * n + p;
				w = i * n + q;
				fm = a[u];
				a[u] = fm * cn + a[w] * sn;
				a[w] = -fm * sn + a[w] * cn;
			}
		}
		for (i = 0;i <= n-1;i++)
		{
			u = i * n + p;
			w = i * n + q;
			fm = v[u];
			v[u] = fm * cn + v[w] * cn;
			v[w] = -fm * sn + v[w] * cn;
		}
	}
	return(1);
}

// 计算特征值函数
int matrixEejcb(
	double a[],		//a[]: in 待计算特征值的对称矩阵			
	int n,			//n:   in 对称矩阵的维度
	double v[],		//v[]: in out 计算得到的特征值位于其对角线上
	double eps,		//eps: 误差阈值
	int jt)			//jt:  迭代次数，该函数没有处理该参数，传入大于1的值即可
{
	int i,j,p,q,u,w,t,s,l;
	double fm,cn,sn,omega,x,y,d;
	l = 1;
	for (i = 0;i <= n-1;i++)
	{
		v[i*n+i] = 1.0;
		for (j = 0;j <= n-1;j++)
		{
			if (i != j)
			{
				v[i*n+j] = 0.0;
			}
		}
	}
	while (l == 1)
	{
		fm = 0.0;
		for (i = 0;i <= n-1;i++)
		{
			for (j = 0;j <= n-1;j++)
			{
				d = fabs(a[i*n+j]);
				if ((i != j)&&(d > fm))
				{
					fm = d;
					p = i;
					q = j;
				}
			}
		}
		if (fm < eps)
		{
			return(1);
		}
		if (l > jt)
		{
			return(-1);
		}
		l = l + 1;
		u = p * n + q;
		w = p * n + p;
		t = q * n + p;
		s = q * n + q;
		x = -a[u];
		y = (a[s] - a[w]) / 2.0f;
		omega = x / sqrt(x * x + y * y);
		if (y < 0.0)
		{
			omega = -omega;
		}
		sn = 1.0f + sqrt(1.0f - omega*omega);
		sn = omega / sqrt(2.0f * sn);
		cn = sqrt(1.0f - sn * sn);
		fm = a[w];
		a[w] = fm * cn * cn + a[s] * sn * sn + a[u] * omega;
		a[s] = fm * sn * sn + a[s] * cn * cn - a[u] * omega;
		a[u] = 0.0;
		a[t] = 0.0;
		for (j = 0;j <= n-1;j++)
		{
			if ((j != p)&&(j != q))
			{
				u = p * n + j;
				w = q * n + j;
				fm = a[u];
				a[u] = fm * cn + a[w] * sn;
				a[w] = -fm * sn + a[w] * cn;
			}
		}

		for (i = 0;i <= n-1;i++)
		{
			if ((i != p)&&(i != q))
			{
				u = i * n + p;
				w = i * n + q;
				fm = a[u];
				a[u] = fm * cn + a[w] * sn;
				a[w] = -fm * sn + a[w] * cn;
			}
		}
		for (i = 0;i <= n-1;i++)
		{
			u = i * n + p;
			w = i * n + q;
			fm = v[u];
			v[u] = fm * cn + v[w] * cn;
			v[w] = -fm * sn + v[w] * cn;
		}
	}
	return(1);
}

}