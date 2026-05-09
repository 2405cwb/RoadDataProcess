/*!@file
***************************************************************************
<PRE>
模块名		：平面拟合   
文件名		：hnPlaneFit.cpp
相关文件	: hnPlaneFit.h
文件实现功能：通过选择的点画出一个平面
作者		：李夏亮
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：5
日期		版本		修改人		修改内容
2021/11/24	1.0			李夏亮		创建
</PRE>
******************************************************************************************************/

#include "stdafx.h"
#include "hnPlaneFit.h"

hnPlaneFit::hnPlaneFit()
{
	// 标准偏差
	m_standerDeviation = 0.0;             

	// 成员变量初始化
	for (int index = 0; index < 9 ;index++)
	{
		if (index < 3)
		{
			m_catercorner_value[index] = 0.0;
			m_secondCater[index] = 0.0;
		}

		m_Householder[index] = 0.0;
	}

	// 特征矩阵初始化
	for (int row = 0; row < 3; row ++)
	{
		for(int line = 0 ;line < 3 ;line++)
		{
			m_chMatrix[row][line] = 0.0;
			m_realMatrix[row*3+line] = 0.0;
		}

	}

	// 初始化平面方程的参数
	m_vcPlane.x = 0;
	m_vcPlane.y = 0;
	m_vcPlane.z = 0;
	m_length = 0;
}

hnPlaneFit::~hnPlaneFit(void)
{

}

// 初始化
void hnPlaneFit::Initialize(const vector<hn3dPointD>& colPoints)
{
	// 得到所有数据的长度
	m_length= colPoints.size();

	// 判断是否有数据
	if (0 == m_length)
	{
		return ;
	}

	// 得到所有点的一个平均值，其x y z坐标保存在下面三个临时变量中
	double dSum_X = 0;
	double dSum_Y = 0;
	double dSum_Z = 0;

	// 求和
	int index = 0;
	for (index = 0; index < m_length; index++)
	{
		dSum_X += colPoints[index].x;
		dSum_Y += colPoints[index].y;
		dSum_Z += colPoints[index].z;
	}

	// 获得平均点
	m_avPoint.x = dSum_X/m_length;
	m_avPoint.y = dSum_Y/m_length;
	m_avPoint.z = dSum_Z/m_length;
	//m_avPoint.z = 0;
	// 求特征矩阵
	for (index = 0; index < m_length; index++)
	{
		m_chMatrix[0][0] += ( (colPoints[index].x - m_avPoint.x) * (colPoints[index].x - m_avPoint.x) );
		m_chMatrix[0][1] += ( (colPoints[index].y - m_avPoint.y) * (colPoints[index].x - m_avPoint.x) );
		m_chMatrix[0][2] += ( (colPoints[index].z - m_avPoint.z) * (colPoints[index].x - m_avPoint.x) );
		m_chMatrix[1][1] += ( (colPoints[index].y - m_avPoint.y) * (colPoints[index].y - m_avPoint.y) );
		m_chMatrix[1][2] += ( (colPoints[index].y - m_avPoint.y) * (colPoints[index].z - m_avPoint.z) );
		m_chMatrix[2][2] += ( (colPoints[index].z - m_avPoint.z) * (colPoints[index].z - m_avPoint.z) );

	}

	// 通过时对称矩阵的性质 对矩阵中相关的元素直接赋值;
	m_chMatrix[1][0] =m_chMatrix[0][1];
	m_chMatrix[2][0] =m_chMatrix[0][2];
	m_chMatrix[2][1] =m_chMatrix[1][2];

	// 将3*3的矩阵转化为一维数组
	for (int row = 0; row < 3; row ++)
	{
		for(int line = 0 ;line < 3 ;line++)
		{
			m_realMatrix[ row*3 + line] = m_chMatrix[row][line];
		}
	}

}

// 获得所有的标准偏差
void hnPlaneFit::GetStanderDiv()
{
	// 求所有点的到平面的距离

	double dTemp_distance = 0.0;
	double dSumDistance = 0.0;                            // 所有点的总距离
	double dAveDistance = 0.0;                            // 所有点的平均距离
	for (int index = 0; index < m_length; index++)
	{
		// 求点到平面的距离
		dTemp_distance = m_vcPlane.x * (m_arrPts[index].x - m_avPoint.x) + m_vcPlane.y * (m_arrPts[index].y - m_avPoint.y) + m_vcPlane.z * (m_arrPts[index].z - m_avPoint.z);

		if (dTemp_distance < 0)
		{
			dTemp_distance = -dTemp_distance;
		}

		// 保存每个点到平面的距离
		m_disEvPoint.push_back(dTemp_distance);
		// 求总距离 累加
		dSumDistance += dTemp_distance;
	}

	// 求平均距离
	dAveDistance = dSumDistance/m_length;



	// 所有点的标准偏差之和
	double allstanderDevidation = 0.0;    
	for (int index = 0; index < m_length; index++)
	{
		allstanderDevidation += (m_disEvPoint[index] - dAveDistance) * (m_disEvPoint[index] - dAveDistance);
	}
	// 求标准方差
	m_standerDeviation = sqrt(allstanderDevidation/(m_length - 1));

}

// 得到特征矩阵的特征值及特征向量
void hnPlaneFit::GetCharaValueAndVector(double fAccuracy,int nMaxRepeat)
{
	// 通过QR分解法，计算特征矩阵的特征值和特征向量
	Eastrq(m_realMatrix,3,m_Householder,m_catercorner_value,m_secondCater);
	Aebstq(3,m_catercorner_value,m_secondCater,m_Householder,fAccuracy,nMaxRepeat);

	//取最小特征值及特征向量
	double dMin_characticsValue ;
	dMin_characticsValue = m_catercorner_value[0];
	dMin_characticsValue = min(dMin_characticsValue,m_catercorner_value[1]);
	dMin_characticsValue = min(dMin_characticsValue,m_catercorner_value[2]);

	// 通过最小的特征值得到对应的特征向量

	// 最小值对应的第一个，特征向量对应第一列;
	if (dMin_characticsValue == m_catercorner_value[0])
	{
		m_vcPlane.x = m_Householder[0];
		m_vcPlane.y = m_Householder[3];
		m_vcPlane.z = m_Householder[6];
	}
	// 最小值对应的第二个，特征向量对应第二列;
	else if (dMin_characticsValue == m_catercorner_value[1])
	{
		m_vcPlane.x = m_Householder[1];
		m_vcPlane.y = m_Householder[4];
		m_vcPlane.z = m_Householder[7];
	}
	// 最小值对应的第三个，特征向量对应第三列;
	else if (dMin_characticsValue == m_catercorner_value[2])
	{
		m_vcPlane.x = m_Householder[2];
		m_vcPlane.y = m_Householder[5];
		m_vcPlane.z = m_Householder[8];
	}
}

// 判断多有点的距离是否小于两倍的标准偏差，从而得到是否要进一步的去排斥噪声
bool hnPlaneFit::PlanfitIsOK()
{
	// 标识量
	bool bIsHavePoint = false;
	// 循环检测噪声点
	for (int index = 0; index < m_arrPts.size() ; index++)
	{	
		if (m_disEvPoint[index] > 2*m_standerDeviation )
		{
			// 去除噪声点到平面的距离
			m_disEvPoint.erase(m_disEvPoint.begin() + index );
			// 取出噪声点
			m_arrPts.erase(m_arrPts.begin() + index);
			bIsHavePoint = true;
		}
	}
	// 有噪声点 需要进一步的取出噪声
	if (bIsHavePoint)
	{
		return false;
	}
	// 没有噪声点 正常返回
	return true;
}

// 最小二乘法得到平面的法向量
// cInputInFo:点集合
// dOutput_a dOutput_b dOutput_c ：法向量
// 不排除噪声
void hnPlaneFit::LeastSquares(vector<hn3dPointD>& cInputInFo,double& dOutput_a,double& dOutput_b,double& dOutput_c,bool bIsDNoise)
{
	if (bIsDNoise)
	{
		//排除噪声拟合
		LSPlaneFit(cInputInFo);
	}
	else
	{
		//不排除噪声的拟合
		LeastSquares(cInputInFo);
	}

	//法向量赋值
	dOutput_a = m_vcPlane.x;
	dOutput_b = m_vcPlane.y;
	dOutput_c = m_vcPlane.z;

}

// 不做处理的平面拟合
void hnPlaneFit::LeastSquares(vector<hn3dPointD>& cInputInFo)
{
	
	Initialize(cInputInFo);
	//得到特征值和特征向量
	GetCharaValueAndVector(0.001,100);
	//计算常量d的值
	m_paramterC = m_vcPlane.x * m_avPoint.x + m_vcPlane.y * m_avPoint.y + m_vcPlane.z * m_avPoint.z;
	m_arrPts = cInputInFo;
}

// 最小二乘法排除噪声的拟合
void hnPlaneFit::LSPlaneFit(vector<hn3dPointD>& cInputInFo)
{
	//得到最初的拟合平面
	LeastSquares(cInputInFo);
	
	if (cInputInFo.size() < 3)
	{
		return;
	}
	size_t nIndex ;

	// 第一次去除里拟合平面距离大于。0.01的点
	for (nIndex = 0;nIndex < cInputInFo.size();)
	{
		if (cInputInFo.size() == 3)
		{
			break;
		}

		// 得到点到平面的距离
		double dDisance = abs(cInputInFo[nIndex].x * m_vcPlane.x + cInputInFo[nIndex].y * m_vcPlane.y +
			cInputInFo[nIndex].z * m_vcPlane.z - m_paramterC);

		// 删除距离平面较远的点
		if (dDisance > 0.01)
		{
			cInputInFo.erase(cInputInFo.begin() + nIndex);
		}
		else
		{
			nIndex++;
		}
	}

	size_t nBaseNumberPt = cInputInFo.size();//得到最初平面的数据量的大小

	while(1)
	{
		// 拟合平面
		LeastSquares(cInputInFo);
		double dMaxDistance = 0.0;                   // 点到平面的最大距离
		int    nMaxIndex = 0;   
		
		if (cInputInFo.size() == 3)
		{
			break;
		}

		// 离拟合平面最大距离点的索引位置
		//遍历所有剩下来的点
		for (nIndex = 0;nIndex < cInputInFo.size();nIndex++)
		{
			// 得到点到平面的距离
			double dDisance = abs(cInputInFo[nIndex].x * m_vcPlane.x+ cInputInFo[nIndex].y * m_vcPlane.y +
				cInputInFo[nIndex].z * m_vcPlane.z);
			// 找到离拟合平面最大距离的点 和点的索引
			if (dDisance > dMaxDistance)
			{
				dMaxDistance = dDisance;
				nMaxIndex = nIndex;
			}
		}


		// 如果所有的距离都小于0.005 退出循环
		if (dMaxDistance < 0.005)
		{
			break;
		}
		else
		{
			cInputInFo.erase(cInputInFo.begin() + nMaxIndex);// 删除最大距离点
		}

		size_t nNowLength  = cInputInFo.size();               // 得到当前数据的数目

		// 如果噪声的数量大于10%的所有数量，退出循环
		if (nNowLength/nBaseNumberPt < 0.9)
		{
			break;
		}
	}
}

// 稳健平面拟合核心算法实现
void hnPlaneFit::FitPlane(vector<hn3dPointD>& pts,float fAccuracy,int nMaxRepeat)
{
	m_arrPts = pts;

	// 初始化
	Initialize(pts);

	// 得到特征值和特征向量
	GetCharaValueAndVector(fAccuracy,nMaxRepeat);

	// 获得标准偏差
	GetStanderDiv();
	bool bIsOK = false;
	int nNumber = 0;

	// 循环去除噪声
	while(1)
	{
		bIsOK =  PlanfitIsOK();
		if (bIsOK)
		{
			break;
		}
		Initialize(pts);
		GetCharaValueAndVector(fAccuracy,nMaxRepeat);
		GetStanderDiv();
		nNumber++;
		// 限制循环次数
		if (nNumber == 20)
		{
			break;
		}
	}
	//计算常量d的值
	m_paramterC = m_vcPlane.x * m_avPoint.x + m_vcPlane.y * m_avPoint.y + m_vcPlane.z * m_avPoint.z;

}

////////////////////////////////////////////////////////////////////////////
/******************************************************
利用变型QR方法计算实对称三对角矩阵全部特征值及特征向量
n-矩阵的阶数
b-长度为n的数组，返回时存放三对角阵的主对角线元素
c-长度为n的数组，返回时前n-1个元素存放次对角线元素
q-长度为n*n的数组，若存放单位矩阵，则返回实对称三对角矩阵的特征向量组
若存放Householder变换矩阵，则返回实对称矩阵A的特征向量组
********************************************************/
////////////////////////////////////////////////////////////////////////////
int hnPlaneFit::Aebstq(int n,double b[],double c[],double q[],double fAccuracy,int nMaxRepeat)
{ 

	//具体算法实现，可以看作是接口

	int i,j,k,m,it,u,v;
	double d,f,h,g,p,r,e,s;
	c[n-1]=0.0;
	d=0.0; 
	f=0.0;
	for (j=0; j<=n-1; j++)
	{ 
		it=0;
		h=fAccuracy*(fabs(b[j])+fabs(c[j]));
		if (h>d)
		{
			d=h;
		}
		m=j;
		while ((m<=n-1)&&(fabs(c[m])>d)) 
		{
			m=m+1;
		}
		if (m!=j)
		{ 
			do
			{ 
				if (it==nMaxRepeat)
				{ 
					printf("fail\n");
					return(-1);
				}
				it=it+1;
				g=b[j];
				p=(b[j+1]-g)/(2.0*c[j]);
				r=sqrt(p*p+1.0);
				if (p>=0.0) 
				{
					b[j]=c[j]/(p+r);
				}
				else 
				{
					b[j]=c[j]/(p-r);
				}
				h=g-b[j];
				for (i=j+1; i<=n-1; i++)
				{
					b[i]=b[i]-h;
				}
				f=f+h; 
				p=b[m]; 
				e=1.0; 
				s=0.0;
				for (i=m-1; i>=j; i--)
				{ 
					g=e*c[i];
					h=e*p;
					if (fabs(p)>=fabs(c[i]))
					{
						e=c[i]/p;
						r=sqrt(e*e+1.0);
						c[i+1]=s*p*r;
						s=e/r; 
						e=1.0/r;
					}
					else
					{ 
						e=p/c[i];
						r=sqrt(e*e+1.0);
						c[i+1]=s*c[i]*r;
						s=1.0/r; 
						e=e/r;
					}
					p=e*b[i]-s*g;
					b[i+1]=h+s*(e*g+s*b[i]);
					for (k=0; k<=n-1; k++)
					{ 
						u=k*n+i+1; 
						v=u-1;
						h=q[u];
						q[u]=s*q[v]+e*h;
						q[v]=e*q[v]-s*h;
					}
				}
				c[j]=s*p;
				b[j]=e*p;
			}
			while (fabs(c[j])>d);
		}
		b[j]=b[j]+f;
	}
	for (i=0; i<=n-1; i++)
	{ 
		k=i; p=b[i];
		if (i+1<=n-1)
		{ 
			j=i+1;
			while ((j<=n-1)&&(b[j]<=p))
			{ 
				k=j;
				p=b[j];
				j=j+1;
			}
		}
		if (k!=i)
		{ 
			b[k]=b[i]; 
			b[i]=p;
			for (j=0; j<=n-1; j++)
			{
				u=j*n+i; 
				v=j*n+k;
				p=q[u];
				q[u]=q[v];
				q[v]=p;
			}
		}
	}
	return(1);
}

////////////////////////////////////////////////////////////
/**********************************************************
//约化对称矩阵为三对角对称矩阵
//利用Householder变换将n阶实对称矩阵约化为对称三对角矩阵
//a-长度为n*n的数组，存放n阶实对称矩阵//n-矩阵的阶数
//q-长度为n*n的数组，返回时存放Householder变换矩阵
//b-长度为n的数组，返回时存放三对角阵的主对角线元素
//c-长度为n的数组，返回时前n-1个元素存放次对角线元素
*********************************************************/
//////////////////////////////////////////////////////////
void hnPlaneFit::Eastrq(double a[],int n,double q[],double b[],double c[])
{ 

	//具体算法实现，可以看作是接口

	int i,j,k,u,v;
	double h,f,g,h2;
	for (i=0; i<=n-1; i++)
	{
		for (j=0; j<=n-1; j++)
		{ 
			u=i*n+j; q[u]=a[u];
		}
	}
	for (i=n-1; i>=1; i--)
	{
		h=0.0;if (i>1)
		{
			for (k=0; k<=i-1; k++)
			{
				u=i*n+k; h=h+q[u]*q[u];
			}
		}
		if (h+1.0==1.0)
		{ 
			c[i-1]=0.0;
			if (i==1)
			{
				c[i-1]=q[i*n+i-1];

			}
			b[i]=0.0;
		}
		else
		{ 
			c[i-1]=sqrt(h);u=i*n+i-1;
			if (q[u]>0.0)
			{
				c[i-1]=-c[i-1];
			}
			h=h-q[u]*c[i-1];
			q[u]=q[u]-c[i-1];
			f=0.0;
			for (j=0; j<=i-1; j++)
			{ 
				q[j*n+i]=q[i*n+j]/h;
				g=0.0;
				for (k=0; k<=j; k++)
				{
					g=g+q[j*n+k]*q[i*n+k];
				}
				if (j+1<=i-1)
				{
					for (k=j+1; k<=i-1; k++)
					{
						g=g+q[k*n+j]*q[i*n+k];
					}
				}
				c[j-1]=g/h;
				f=f+g*q[j*n+i];
			}
			h2=f/(h+h);
			for (j=0; j<=i-1; j++)
			{ 
				f=q[i*n+j];
				g=c[j-1]-h2*f;
				c[j-1]=g;
				for (k=0; k<=j; k++)
				{ 
					u=j*n+k;
					q[u]=q[u]-f*c[k-1]-g*q[i*n+k];
				}
			}
			b[i]=h;
		}
	}
	b[0]=0.0;
	for (i=0; i<=n-1; i++)
	{ 
		if ((b[i]!=0.0)&&(i-1>=0))
		{
			for (j=0; j<=i-1; j++)
			{ 
				g=0.0;
				for (k=0; k<=i-1; k++)
				{
					g=g+q[i*n+k]*q[k*n+j];
				}
				for (k=0; k<=i-1; k++)
				{ 
					u=k*n+j;q[u]=q[u]-g*q[k*n+i];
				}
			}
		}
		u=i*n+i;
		b[i]=q[u]; 
		q[u]=1.0;
		if (i-1>=0)
		{
			for (j=0; j<=i-1; j++)
			{ 
				q[i*n+j]=0.0;
				q[j*n+i]=0.0;
			}
		}
	}
}


//获得平面表示圆的半径
double hnPlaneFit::GetExpressRadius()
{
	double dMaxDistance = 0.0;

	for (size_t nIndex = 0; nIndex < m_arrPts.size(); nIndex++)
	{
		double dDistance = sqrt(pow(m_arrPts[nIndex].x - m_avPoint.x,2)+pow(m_arrPts[nIndex].y - m_avPoint.y,2)+pow(m_arrPts[nIndex].z - m_avPoint.z,2));
		if (dDistance > dMaxDistance)
		{
			dMaxDistance = dDistance;
		}
	}
	return dMaxDistance/2;
}

//获得法向量
void hnPlaneFit::GetVector(double& dAxisa,double& dAxisb,double& dAxisc)
{
	//法向量赋值
	dAxisa = m_vcPlane.x;
	dAxisb = m_vcPlane.y;
	dAxisc = m_vcPlane.z;
}