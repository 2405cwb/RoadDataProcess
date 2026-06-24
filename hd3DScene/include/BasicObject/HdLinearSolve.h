#pragma once
#include "hdBasicObject.h"
#include <vector>
using namespace std;

class Matrix
{
	//外部接口
public:  

	//构造函数
	Matrix(int dims=2)
	{
		index=dims;                    //保护数据赋值
		MatrixA=new double [index*index];//动态内存分配
	}

	//析构函数
	virtual ~Matrix()
	{
		//内存释放
		if (MatrixA)
		{
			delete []MatrixA;    
			MatrixA = NULL;
		}

	}

	// 设置矩阵
	void setMatrix(double *rmatr)
	{
		for (int i=0;i<index*index;i++)     
		{
			*(MatrixA+i)=rmatr[i];//矩阵成员赋初值
		}
	}

	//保护数据成员
protected:    

	int index;						//矩阵维数  
	double *MatrixA;				//矩阵存放数组首地址

};

//公有派生类Linequ定义
class CHdLinearSolve:public Matrix    
{
	//外部接口
public:      

	// 构造函数
	CHdLinearSolve(int dims=2)
		: Matrix(dims)
	{
		//使用参数调用基类构造函数
		sums=new double[dims];   //动态分配内存
		solu=new double[dims];
	}

	// 析构函数
	virtual ~CHdLinearSolve()  
	{
		//系统默认调用基类析构函数
		if (sums)
		{
			delete []sums;
			sums = NULL;
		}

		//释放内存
		if (solu)
		{
			delete []solu;
			solu = NULL;
		}	
	}

	//方程赋值
	void setLinequ(double *a,double *b)
	{
		//调用基类函数
		setMatrix(a);		

		for (int i=0;i<index;i++)
		{
			sums[i]=b[i];
		}
	}

	// 获取方程的解
	void GetSolu(double* solutions)
	{
		if (!solutions)
		{
			return;
		}

		for (int i=0;i<index;i++)
		{
			solutions[i]=solu[i];
		}
	}

	//全主元高斯消去法求解方程
	int Solve()
	{
		int *js = new int[index];
		int l = 1,k,i,j,is,p,q;
		double d,t;

		for(k=0;k<=index-2;k++) //消去过程
		{
			d=0.0;
			for(i=k;i<index-1;i++)
				for(j=k;j<index-1;j++)
				{
					t=fabs(MatrixA[i*index+j]);
					if(t>d)
					{
						d=t;
						js[k]=j;
						is=i;
					}
				}

				if(d+1.0==1.0)
					l=0;
				else
				{
					if(js[k]!=k)
						for(i=0;i<=index-1;i++)
						{
							p=i*index+k;
							q=i*index+js[k];
							t=MatrixA[p];
							MatrixA[p]=MatrixA[q];
							MatrixA[q]=t;
						}

						if(is!=k)
						{
							for(j=k;j<=index-1;j++)
							{
								p=k*index+j;
								q=is*index+j;
								t=MatrixA[p];
								MatrixA[p]=MatrixA[q];
								MatrixA[q]=t;
							}
							t=sums[k];
							sums[k]=sums[is];
							sums[is]=t;
						}
				}
				if(l==0)
				{
					delete [] js;
					js = NULL;
					return(0);
				}

				d=MatrixA[k*index+k];
				for(j=k+1;j<=index-1;j++)
				{
					p=k*index+j;
					MatrixA[p]=MatrixA[p]/d;
				}
				sums[k]=sums[k]/d;
				for(i=k+1;i<=index-1;i++)
				{
					for (j=k+1;j<=index-1;j++)
					{
						p=i*index+j;
						MatrixA[p]=MatrixA[p]-MatrixA[i*index+k]*MatrixA[k*index+j];
					}
					sums[i]=sums[i]-MatrixA[i*index+k]*sums[k];
				}
		}

		d=MatrixA[(index-1)*index+index-1];
		if(fabs(d)+1.0==1.0)
		{
			delete [] js;
			js = NULL;
			return (0);
		}

		solu[index-1]=sums[index-1]/d;    //回代过程
		for(i=index-2;i>=0;i--)
		{
			t=0.0;
			for(j=i+1;j<=index-1;j++)
				t=t+MatrixA[i*index+j]*solu[j];
			solu[i]=sums[i]-t;
		}
		js[index-1]=index-1;
		for(k=index-1;k>=0;k--)
		{
			if(js[k]!=k)
			{
				t=solu[k];
				solu[k]=solu[js[k]];
				solu[js[k]]=t;
			}
		}
		delete [] js;
		js = NULL;
		return(1);
	}

private:
	double *sums;    //方程右端项
	double *solu;    //方程的解
};
