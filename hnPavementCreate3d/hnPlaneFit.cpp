#include "hnPlaneFit.h"


bool Class_FitPlane::LSPlaneFit(vector<hnPoint3d> &vecpoints,vector<double> &m_PlaneParameters)
{
	m_PlaneParameters.clear();
	int count=vecpoints.size();
	if (count<3)
	{
		return false;
	}
	double meanX=0,meanY=0,meanZ=0;
	double meanXX=0,meanYY=0,meanZZ=0;
	double meanXY=0,meanXZ=0,meanYZ=0;
	for (int i=0;i<count;i++)
	{
		meanX+=vecpoints[i].x;
		meanY+=vecpoints[i].y;
		meanZ+=vecpoints[i].z;

		meanXX+=vecpoints[i].x*vecpoints[i].x;
		meanYY+=vecpoints[i].y*vecpoints[i].y;
		meanZZ+=vecpoints[i].z*vecpoints[i].z;

		meanXY+=vecpoints[i].x*vecpoints[i].y;
		meanXZ+=vecpoints[i].x*vecpoints[i].z;
		meanYZ+=vecpoints[i].y*vecpoints[i].z;
	}
	meanX/=count;
	meanY/=count;
	meanZ/=count;
	meanXX/=count;
	meanYY/=count;
	meanZZ/=count;
	meanXY/=count;
	meanXZ/=count;
	meanYZ/=count;

	Matrix3d eMat;
	eMat(0,0)=meanXX-meanX*meanX;eMat(0,1)=meanXY-meanX*meanY;eMat(0,2)=meanXZ-meanX*meanZ;
	eMat(1,0)=meanXY-meanX*meanY;eMat(1,1)=meanYY-meanY*meanY;eMat(1,2)=meanYZ-meanY*meanZ;
	eMat(2,0)=meanXZ-meanX*meanZ;eMat(2,1)=meanYZ-meanZ*meanY;eMat(2,2)=meanZZ-meanZ*meanZ;
	EigenSolver<Matrix3d> solver(eMat);
	Matrix3d EValues=solver.pseudoEigenvalueMatrix();
	Matrix3d EVector=solver.pseudoEigenvectors();


	double v1=EValues(0,0);
	double v2=EValues(1,1);
	double v3=EValues(2,2);
	int minNumber=0;

	if((abs(v2)<=abs(v1))&&(abs(v2)<=abs(v3)))
	{
		minNumber=1;
	}
	if((abs(v3)<=abs(v2))&&(abs(v3)<=abs(v1)))
	{
		minNumber=2;
	}
	double A=EVector(0,minNumber);
	double B=EVector(1,minNumber);
	double C=EVector(2,minNumber);
	double D=-(A*meanX+B*meanY+C*meanZ);
		
	if(C<0)
	{
		A *= -1.0;
		B *= -1.0;
		C *= -1.0;
		D *= -1.0;

	}

	m_PlaneParameters.push_back(A);
	m_PlaneParameters.push_back(B);
	m_PlaneParameters.push_back(C);
	m_PlaneParameters.push_back(D);
	return true;

}

void Class_FitPlane::LSPlaneFit_Denoise(vector<hnPoint3d> &vecpoints,vector<double> &m_PlaneParameters)
{
	if (vecpoints.size()<3)
	{
		return;
	} 

	LSPlaneFit(vecpoints,m_PlaneParameters);
	int nIndex;
		// 第一次去除里拟合平面距离大于0.1的点
	for (nIndex = 0;nIndex < vecpoints.size();)
	{
		if (vecpoints.size() == 3)
		{
			break;
		}

		// 得到点到平面的距离
		double dDisance = abs(vecpoints[nIndex].x * m_PlaneParameters[0] + vecpoints[nIndex].y * m_PlaneParameters[1] 
		+ vecpoints[nIndex].z * m_PlaneParameters[2] + m_PlaneParameters[3])/sqrt(m_PlaneParameters[0] * m_PlaneParameters[0] 
		+ m_PlaneParameters[1] * m_PlaneParameters[1] + m_PlaneParameters[2] * m_PlaneParameters[2]);

		// 删除距离平面较远的点
		if (dDisance > 0.1)
		{
			vecpoints.erase(vecpoints.begin() + nIndex);
		}
		else
		{
			nIndex++;
		}
	}

	int nBaseNumberPt = vecpoints.size();//得到最初平面的数据量的大小


	LSPlaneFit(vecpoints,m_PlaneParameters);
	//while(1)
	//{
	//	// 拟合平面
	//	LSPlaneFit(vecpoints,m_PlaneParameters);
	//	double MaxDistance = 0.0;                   // 点到平面的最大距离
	//	int    MaxIndex = 0;   
	//	
	//	if (vecpoints.size() == 3)
	//	{
	//		break;
	//	}

	//	// 离拟合平面最大距离点的索引位置
	//	//遍历所有剩下来的点
	//	for (nIndex = 0;nIndex < vecpoints.size();nIndex++)
	//	{
	//		// 得到点到平面的距离
	//		double dDisance = abs(vecpoints[nIndex].x * m_PlaneParameters[0] + vecpoints[nIndex].y * m_PlaneParameters[1] 
	//		+ vecpoints[nIndex].z * m_PlaneParameters[2] + m_PlaneParameters[3])/sqrt(m_PlaneParameters[0] * m_PlaneParameters[0] 
	//		+ m_PlaneParameters[1] * m_PlaneParameters[1] + m_PlaneParameters[2] * m_PlaneParameters[2]);
	//		// 找到离拟合平面最大距离的点 和点的索引
	//		if (dDisance > MaxDistance)
	//		{
	//			MaxDistance = dDisance;
	//			MaxIndex = nIndex;
	//		}
	//	}


	//	// 如果所有的距离都小于0.05 退出循环
	//	if (MaxDistance < 0.05)
	//	{
	//		break;
	//	}
	//	else
	//	{
	//		vecpoints.erase(vecpoints.begin() + MaxIndex);// 删除最大距离点
	//	}

	//	int nNowLength  = vecpoints.size();               // 得到当前数据的数目

	//	// 如果噪声的数量大于10%的所有数量，退出循环
	//	if (nNowLength/nBaseNumberPt < 0.9)
	//	{
	//		break;
	//	}
	//}
}