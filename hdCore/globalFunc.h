#pragma once
#include <math.h>

template <class T>
T average(T a[], int n)
{
	T aver,s=0;
	int i;

	for(i=0;i<n;i++)
		s=s+a[i];
	aver=s/n;
	return aver;
}

template <class T>
void mult(const T *m1, const T *m2, T *result, int i_1, int j_12, int j_2)
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

template <class T>
int invers_matrix(T *m1, int n)
{
	int *is,*js;
	int i,j,k,l,u,v;
	T temp,max_v;
	is=new int[n];
	js=new int [n];

	for(k=0;k<n;k++)
	{
		max_v=0.0;
		for(i=k;i<n;i++)
			for(j=k;j<n;j++)
			{
				temp=fabs(m1[i*n+j]);
				if(temp>max_v)
				{
					max_v=temp; is[k]=i; js[k]=j;
				}
			}
			if(max_v==0.0)
			{
				delete [] is;
				delete [] js;
				return(0);
			}
			if(is[k]!=k)
				for(j=0;j<n;j++)
				{
					u=k*n+j; v=is[k]*n+j;
					temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
				}
				if(js[k]!=k)
					for(i=0;i<n;i++)
					{
						u=i*n+k; v=i*n+js[k];
						temp=m1[u]; m1[u]=m1[v]; m1[v]=temp;
					}
					l=k*n+k;
					m1[l]=1.0f/m1[l];
					for(j=0;j<n;j++)
						if(j!=k)
						{
							u=k*n+j;
							m1[u]*=m1[l];
						}
						for(i=0;i<n;i++)
							if(i!=k)
								for(j=0;j<n;j++)
									if(j!=k)
									{
										u=i*n+j;
										m1[u]-=m1[i*n+k]*m1[k*n+j];
									}
									for(i=0;i<n;i++)
										if(i!=k)
										{
											u=i*n+k;
											m1[u]*=-m1[l];
										}
	}
	for(k=n-1;k>=0;k--)
	{
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

template <class T>
void transpose(T *m1, T *m2, int m, int n)
{
	int i,j;                                        
	for(i=0;i<m;i++)                                
		for(j=0;j<n;j++)                        
			m2[j*m+i]=m1[i*n+j];
}