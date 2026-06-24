#include "StdAfx.h"
#include "FitPlane.h"
#include "Filters.h"
//#include "CMatrix.h"
#include "..\hdCore\hdMatrix.h"
#include "..\hd3DEngine\include\vector2d.h"
#include "math.h " 
#include "stdio.h " 
#include "hdSysSetting.h"

using namespace irr;
//using namespace irr::io;
using namespace irr::core;
namespace hd
{
	CFitPlane::CFitPlane(void)
	{
		m_deigenvalue = NULL;
	}
	CFitPlane::~CFitPlane(void)
	{
		if (m_deigenvalue)
		{
			delete []m_deigenvalue;
			m_deigenvalue = NULL;
		}
	}

vector<float> hd::CFitPlane::FitPlaneInit(vector<PointXYZ>& inPts)
{
	float averageX = 0.0f,averageY = 0.0f, averageZ = 0.0f;
	float averageD = 0.0f,sumdd = 0.0f;
	float matrix[3][3] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};                      //矩阵A
	float matrixDeigen[3][3] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};                //存放所有特征值特征向量
	float MinDeigenvalue = 0.0f;                                                              //存放最小特征值
	m_deigenvalue = new float[4];                                                             //存放最小特征向量(参数a、b、c、d)
	unsigned int i = 0;

	//m_deltas.clear();
	vector<float> ds;		
	ds.clear();

	for(i = 0;i < inPts.size();i++)
	{
		averageX += inPts[i].x;
		averageY += inPts[i].y;
		averageZ += inPts[i].z;
	}

	averageX = averageX / (inPts.size());
	averageY = averageY / (inPts.size());
	averageZ = averageZ / (inPts.size());

	vector<PointXYZ> deltasXYZ(inPts);	                    
	for (i = 0;i < inPts.size();i++)
	{
		deltasXYZ[i].x = inPts[i].x - averageX;
		deltasXYZ[i].y = inPts[i].y - averageY;
		deltasXYZ[i].z = inPts[i].z - averageZ;

	}

	for(i = 0;i < inPts.size();i++)
	{
		matrix[0][0] += deltasXYZ[i].x * deltasXYZ[i].x;
		matrix[0][1] += deltasXYZ[i].x * deltasXYZ[i].y;
		matrix[0][2] += deltasXYZ[i].x * deltasXYZ[i].z;
		matrix[1][1] += deltasXYZ[i].y * deltasXYZ[i].y;
		matrix[1][2] += deltasXYZ[i].y * deltasXYZ[i].z;
		matrix[2][2] += deltasXYZ[i].z * deltasXYZ[i].z;
	}
	matrix[1][0] = matrix[0][1];
	matrix[2][0] = matrix[0][2];
	matrix[2][1] = matrix[1][2];

	matrixEejcb(*matrix,3,*matrixDeigen,0.0001f,100);    
	if (matrix[0][0] != 0&&matrix[1][1] != 0&&matrix[2][2] != 0)                           //得到最小特征值
	{
		MinDeigenvalue = min(fabs(matrix[0][0]),fabs(matrix[1][1]));
		MinDeigenvalue = min(MinDeigenvalue,fabs(matrix[2][2]));        
	}
	else if (matrix[0][0] == 0&&matrix[1][1] != 0&&matrix[2][2] != 0)
	{
		MinDeigenvalue = min(fabs(matrix[1][1]),fabs(matrix[2][2]));
	}
	else if (matrix[1][1] == 0&&matrix[0][0] != 0&&matrix[2][2] != 0)
	{
		MinDeigenvalue = min(fabs(matrix[0][0]),fabs(matrix[2][2]));
	}
	else if (matrix[2][2] == 0&&matrix[0][0] != 0&&matrix[1][1] != 0)
	{
		MinDeigenvalue = min(fabs(matrix[0][0]),fabs(matrix[1][1]));
	}
	else if (matrix[0][0] == 0&&matrix[1][1] == 0&&matrix[2][2] != 0)
	{
		MinDeigenvalue = fabs(matrix[2][2]);
	}
	else if (matrix[0][0] == 0&&matrix[2][2] == 0&&matrix[1][1] != 0)
	{
		MinDeigenvalue = fabs(matrix[1][1]);
	}
	else if (matrix[1][1] == 0&&matrix[2][2] == 0&&matrix[0][0] != 0)
	{
		MinDeigenvalue = fabs(matrix[0][0]);
	}

	if (MinDeigenvalue == fabs(matrix[0][0]))                                  //获得最小特征值的特征向量
	{
		m_deigenvalue[0] = matrixDeigen[0][0];
		m_deigenvalue[1] = matrixDeigen[1][0];
		m_deigenvalue[2] = matrixDeigen[2][0];
	}
	else if (MinDeigenvalue == fabs(matrix[1][1]))
	{
		m_deigenvalue[0] = matrixDeigen[0][1];
		m_deigenvalue[1] = matrixDeigen[1][1];
		m_deigenvalue[2] = matrixDeigen[2][1];
	}
	else if(MinDeigenvalue == fabs(matrix[2][2]))
	{
		m_deigenvalue[0] = matrixDeigen[0][2];
		m_deigenvalue[1] = matrixDeigen[1][2];
		m_deigenvalue[2] = matrixDeigen[2][2];
	}
	m_deigenvalue[3] = m_deigenvalue[0] * averageX + m_deigenvalue[1] * averageY + m_deigenvalue[2] * averageZ;

	for(i = 0;i < inPts.size();i++)
	{
		float k = 0.0f;
		k = fabs(m_deigenvalue[0] * deltasXYZ[i].x + m_deigenvalue[1] * deltasXYZ[i].y + m_deigenvalue[2] * deltasXYZ[i].z);
		ds.push_back(k);
	}
	for (i = 0;i < inPts.size();i++)
	{
		averageD += ds[i];
	}
	averageD = averageD / (inPts.size());

	for (i = 0;i < inPts.size();i++)
	{
		sumdd += (ds[i] - averageD) * (ds[i] - averageD);
	}
	m_sigma = sqrt(sumdd / (inPts.size() - 1));                    //计算阈值标准偏差

	return ds;

}

vector<PointXYZ> hd::CFitPlane::FitPlane(vector<PointXYZ>& inPts,vector<PointXYZ>& result,float eps1,int jt1)
{
	int l;
	l = 1;
	vector<PointXYZ> ptss;
	
	for(unsigned int i = 0;i < inPts.size();i++)
	{
		if(m_ds[i] <= (2*m_sigma))
		{
			result.push_back(inPts[i]);
		}
	}
	while(l == 1)
	{
		if (m_sigma < eps1)                           
		{
			return inPts;
		}
		if (l > jt1)
		{
			return ptss;
		}
		l = l+1;
		m_ds.clear();
		m_ds = FitPlaneInit(result);
		ptss.clear();
		FitPlane(result,ptss,eps1,jt1);
	}
	return ptss;
}

double GetAngle(core::vector2df sourcePlaneVector, core::vector2df destPlaneVector)
{
	double temp = 0;
	double norm1 = 0;
	double norm2 = 0;

	norm1 = sqrt(sourcePlaneVector.X * sourcePlaneVector.X + sourcePlaneVector.Y * sourcePlaneVector.Y);
	norm2 = sqrt(destPlaneVector.X * destPlaneVector.X + destPlaneVector.Y * destPlaneVector.Y);

	temp = sourcePlaneVector.X * destPlaneVector.X + sourcePlaneVector.Y * destPlaneVector.Y;

	if (norm1 == 0 || norm2 == 0)
	{
		return 0;
	}
	else
	{
		temp = temp / (norm1 * norm2);

		if (temp > 1.0)
		{
			temp = 1.0;
		}
		else if (temp < -1.0)
		{
			temp = -1.0;
		}

		return acos(temp);
	}
}

core::vector2df GetFarthestPointF(core::vector2df aPointF, vector<core::vector2df> aPoints)
{
	core::vector2df farthestPointF;

	double maxLength = 0;
	double tempLength = 0;

	double x = 0;
	double y = 0;

	for(size_t i = 0;i<aPoints.size();i++)//.Drawing.PointF aDestPoint in aPoints)
	{
		core::vector2df& aDestPoint = aPoints[i];
		x = (double)(aDestPoint.X - aPointF.X);
		y = (double)(aDestPoint.Y - aPointF.Y);

		tempLength = sqrt(x * x + y * y);

		if (maxLength < tempLength)
		{
			maxLength = tempLength;
			farthestPointF = aDestPoint;
		}
	}

	return farthestPointF;
}

void GetBorderPointFs(vector<core::vector2df>& aPoints,vector<core::vector2df> &aBorderPoints)
{
	if (aPoints.size() < 3)
	{
	}

	//1.获取任意点的最远点

	core::vector2df farthestPointF = GetFarthestPointF(aPoints[0], aPoints);

	//2.获取边界点 --------------------------------------->

	core::vector2df sourcePointF = farthestPointF;
	core::vector2df destPointF = aPoints[0];

	core::vector2df maxAnglePoint;

	while (!farthestPointF.equals(maxAnglePoint))
	{
		core::vector2df sourcePlaneVector = (destPointF - sourcePointF);

		double maxAngle = 0;

		for (size_t i = 0;i<aPoints.size();i++)
		{
			core::vector2df& aDestPoint = aPoints[i];

			core::vector2df destPlaneVector = (aDestPoint - sourcePointF);

			double tempAngle = GetAngle(sourcePlaneVector, destPlaneVector);

			if (maxAngle <= tempAngle)
			{
				maxAngle =tempAngle;
				maxAnglePoint = aDestPoint;
			}
		}

		aBorderPoints.push_back(maxAnglePoint);

		destPointF = sourcePointF;
		sourcePointF = maxAnglePoint;
	} //---------------------------------------< 2.获取边界点 

}


 bool hd::CFitPlane::getPlyPoints(vector<PointXYZ>& vecPts)
 {
 	vector<PointXYZ> resultPts;            //平面上点
 	vector<PointXYZ> plyPts;               //平面边界点
 	m_ds = FitPlaneInit(vecPts);
 	resultPts = FitPlane(vecPts,resultPts,0.03f,100);
	//由平面参数得到平面法向量
	float normalPara[3];
	for (int i = 0;i < 3;i++)
	{
		normalPara[i] = m_deigenvalue[i] / sqrt(m_deigenvalue[0] * m_deigenvalue[0] + m_deigenvalue[1] * m_deigenvalue[1] + m_deigenvalue[2] * m_deigenvalue[2]);
	}
	int nCountAll = (int)vecPts.size();
	int nCounts = (int)resultPts.size();
	double dCountJudge = (nCounts * 1.0) / (nCountAll * 1.0);
	if (dCountJudge < 0.5)
	{
		return false;
	}

	vector<core::vector2df> Pts2d;
	vector<core::vector2df> borderPts;
    core::vector2df pt;
	//xy(0,0,1)、xz(0,1,0)、yz(1,0,0)平面法向量
	float normalXYZ[3][3];
	float angleXYZ[3];
	normalXYZ[0][0] = 0.0f; normalXYZ[0][1] = 0.0f; normalXYZ[0][2] = 1.0f;
	normalXYZ[1][0] = 0.0f; normalXYZ[1][1] = 1.0f; normalXYZ[1][2] = 0.0f;
	normalXYZ[2][0] = 1.0f; normalXYZ[2][1] = 0.0f; normalXYZ[2][2] = 0.0f;

	for (int i = 0;i < 3;i++)
	{
		angleXYZ[i] = acos(normalPara[0] * normalXYZ[i][0] + normalPara[1] * normalXYZ[i][1] + normalPara[2] * normalXYZ[i][2]);
	}
	int Num;
	for (int i =0;i < 3;i++)
	{
		angleXYZ[i] = angleXYZ[i] * (180.0f/3.1415926f);
		if (angleXYZ[i] > 90.0f)
		{
			angleXYZ[i] = 180.0f - angleXYZ[i];
		}
	}

	if (angleXYZ[0] <= angleXYZ[1] && angleXYZ[0] <= angleXYZ[2])
	{
		Num = 0;
	}
	else if (angleXYZ[1] <= angleXYZ[0] && angleXYZ[1] <= angleXYZ[2])
	{
		Num = 1;
	}
	else if (angleXYZ[2] <= angleXYZ[1] && angleXYZ[2] <= angleXYZ[0])
	{
		Num = 2;
	}

	if (Num == 0)         //投影到xy平面
	{
		for (int i = 0;i < nCounts;i++)
		{
			pt.X = resultPts[i].x;
			pt.Y = resultPts[i].y;
			Pts2d.push_back(pt);
		}
	}
	else if (Num == 1)   //投影到xz平面
	{
		for (int i = 0;i < nCounts;i++)
		{
			pt.X = resultPts[i].x;
			pt.Y = resultPts[i].z;
			Pts2d.push_back(pt);
		}
	}
	else	             //投影到yz平面
		for (int i = 0;i < nCounts;i++)
		{
			pt.X = resultPts[i].y;
			pt.Y = resultPts[i].z;
			Pts2d.push_back(pt);
		}
		                //获得平面上边框点
	GetBorderPointFs(Pts2d,borderPts);
	                    //边框点个数
	int nCounts1 = borderPts.size();
	m_plyPts.resize(nCounts1);
	if (Num == 0)
	{
		for (int i = 0;i < nCounts1;i++)
		{
			for (int j = 0;j < nCounts;j++)
			{
				if (borderPts[i].X == resultPts[j].x && borderPts[i].Y == resultPts[j].y)
				{
					m_plyPts[i].x = resultPts[j].x;
					m_plyPts[i].y = resultPts[j].y;
					m_plyPts[i].z = resultPts[j].z;
				}
			}
		}
	}
	else if (Num == 1)
	{
		for (int i = 0;i < nCounts1;i++)
		{
			for (int j = 0;j < nCounts;j++)
			{
				if (borderPts[i].X == resultPts[j].x && borderPts[i].Y == resultPts[j].z)
				{
					m_plyPts[i].x = resultPts[j].x;
					m_plyPts[i].y = resultPts[j].y;
					m_plyPts[i].z = resultPts[j].z;
				}
			}
		}
	}
	else
		for (int i = 0;i < nCounts1;i++)
		{
			for (int j = 0;j < nCounts;j++)
			{
				if (borderPts[i].X == resultPts[j].y && borderPts[i].Y == resultPts[j].z)
				{
					m_plyPts[i].x = resultPts[j].x;
					m_plyPts[i].y = resultPts[j].y;
					m_plyPts[i].z = resultPts[j].z;
				}
			}
		}
		return true;
 }
 
}