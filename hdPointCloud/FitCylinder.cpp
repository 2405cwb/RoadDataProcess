#include "StdAfx.h"
#include "FitCylinder.h"
//#include "CMatrix.h"
#include "..\hdCore\hdMatrix.h"
#include "Filters.h"
#include <math.h> 
#include <stdio.h> 
#include "hdSysSetting.h"
#include "computeNormal.h"

namespace hd
{
	CFitCylinder::CFitCylinder(void)
	{
		m_deigenvalue = NULL;
		m_posXYZRH = NULL;

	}
	CFitCylinder::~CFitCylinder(void)
	{
		if (m_deigenvalue)
		{
			delete []m_deigenvalue;
			m_deigenvalue = NULL;
		}
		if (m_posXYZRH)
		{
			delete []m_posXYZRH;
			m_posXYZRH = NULL;
		}
	}

	vector<float> CFitCylinder::fitPlaneInit(vector<NormalPointXYZ>& inPts)
	{
		float averageX = 0.0f,averageY = 0.0f, averageZ = 0.0f;
		float averageD = 0.0f,sumdd = 0.0f;
		float matrix[3][3] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};                      //矩阵A
		float matrixDeigen[3][3] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};                //存放所有特征值特征向量
		float MinDeigenvalue = 0.0f;    //存放最小特征值
		
		if (m_deigenvalue)
		{
			delete []m_deigenvalue;
			m_deigenvalue = NULL;
		}
		m_deigenvalue = new float[4];                                                             //存放最小特征向量(参数a、b、c、d)
		unsigned int i = 0;

		vector<float> ds;		
		ds.clear();

		for(i = 0;i < inPts.size();i++)
		{
			averageX += inPts[i].nx;
			averageY += inPts[i].ny;
			averageZ += inPts[i].nz;
		}

		averageX = averageX / (inPts.size());
		averageY = averageY / (inPts.size());
		averageZ = averageZ / (inPts.size());

		vector<NormalPointXYZ> deltasXYZ(inPts);	                    
		for (i = 0;i < inPts.size();i++)
		{
			deltasXYZ[i].nx = inPts[i].nx - averageX;
			deltasXYZ[i].ny = inPts[i].ny - averageY;
			deltasXYZ[i].nz = inPts[i].nz - averageZ;

		}

		for(i = 0;i < inPts.size();i++)
		{
			matrix[0][0] += deltasXYZ[i].nx * deltasXYZ[i].nx;
			matrix[0][1] += deltasXYZ[i].nx * deltasXYZ[i].ny;
			matrix[0][2] += deltasXYZ[i].nx * deltasXYZ[i].nz;
			matrix[1][1] += deltasXYZ[i].ny * deltasXYZ[i].ny;
			matrix[1][2] += deltasXYZ[i].ny * deltasXYZ[i].nz;
			matrix[2][2] += deltasXYZ[i].nz * deltasXYZ[i].nz;
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
			k = fabs(m_deigenvalue[0] * deltasXYZ[i].nx + m_deigenvalue[1] * deltasXYZ[i].ny + m_deigenvalue[2] * deltasXYZ[i].nz);
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

	vector<NormalPointXYZ> CFitCylinder::fitPlane(vector<NormalPointXYZ>& inPts,vector<NormalPointXYZ>& result,float eps1,int jt1)
	{
		int l;
		l = 1;
		vector<NormalPointXYZ> ptss;

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
			m_ds = fitPlaneInit(result);
			ptss.clear();
			fitPlane(result,ptss,0.05f,100);
		}
		return ptss;
	}

	// 判断法向量是否平行相等
	bool judgeNormal(float *pa,float *pb)
	{
		if (fabs(pa[0]) == fabs(pb[0]) && fabs(pa[1]) == fabs(pb[1]) && fabs(pa[2]) == fabs(pb[2]))
		   return false;
		else if (fabs(fabs(pa[0] * pb[0] + pa[1] * pb[1] + pa[2] * pb[2]) - 1) < 0.001)
			return false;
		//else if (fabs(pa[0]) == fabs(pc[0]) && fabs(pa[1]) == fabs(pc[1]) && fabs(pa[2]) == fabs(pc[2]))
		//	return false;
		//else if (fabs(pb[0]) == fabs(pc[0]) && fabs(pb[1]) == fabs(pc[1]) && fabs(pb[2]) == fabs(pc[2]))
		//	return false;
		else
			return true;

	}

	// 由圆柱面上任意两法向量相乘得到的向量积与其垂直得到轴线向量的概略值
	vector<NormalPointXYZ> cylinderRemove(vector<NormalPointXYZ>& vecPts,vector<NormalPointXYZ>& result)
	{
		int Nums = vecPts.size();
		if (Nums <= 3)
		{
			return result;
		}

		int nTimes = 0,middle = 0,w = 0;
		int *randNum;
		float maxCount = 0.0f;
		float mid = 0.0f;
		randNum = new int[Nums];
		float pa[3] = {0.0f,0.0f,0.0f}; //随机取两向量积作为圆柱法向量进行判断
		float pb[3] = {0.0f,0.0f,0.0f};
		float pc[3] = {0.0f,0.0f,0.0f};
		//float pd[3] = {0.0f,0.0f,0.0f};
		float paraPlane[3] = {0.0f,0.0f,0.0f};
		//Matrix3f A(3, 3),b(3,1),xx(3,1);
		//A.setZero();
		
		while (nTimes < 1000)
		{
			for (int i = 0;i < Nums;i++)
			{
				randNum[i] = i;
			}
			for (int i = 0;i < 2;i++)
			{
				// 需限制范围
				w = rand()%(Nums - i) + i;
				if (w > Nums - 1)
				{
					w = Nums - 1;
				}

				middle = randNum[i];
				randNum[i] = randNum[w];
				randNum[w] = middle;
			}
			int nCounts = 0;
			nTimes++;

			pa[0] = vecPts[randNum[0]].nx; pa[1] = vecPts[randNum[0]].ny; pa[2] = vecPts[randNum[0]].nz;
			pb[0] = vecPts[randNum[1]].nx; pb[1] = vecPts[randNum[1]].ny; pb[2] = vecPts[randNum[1]].nz;

 			if (judgeNormal(pa,pb))
 			{
				pc[0] = pa[1] * pb[2] - pa[2] * pb[1];
				pc[1] = pa[2] * pb[0] - pa[0] * pb[2];
				pc[2] = pa[0] * pb[1] - pa[1] * pb[0];
				mid = sqrt(pc[0] * pc[0] + pc[1] * pc[1] + pc[2] * pc[2]);

				if (mid != 0.0f)
				{
					pc[0] = pc[0] / mid;
					pc[1] = pc[1] / mid;
					pc[2] = pc[2] / mid;
				}

				for (int i = 0;i < Nums;i++)
				{
					mid = fabs(vecPts[i].nx * pc[0] + vecPts[i].ny * pc[1] + vecPts[i].nz * pc[2]);
					if (fabs(vecPts[i].nx * pc[0] + vecPts[i].ny * pc[1] + vecPts[i].nz * pc[2]) < 0.01)
					{
						nCounts++;
					}
				}
				float count = (nCounts * 1.0f)/(Nums * 1.0f);
				if (maxCount < count)
				{
					maxCount = count;
					paraPlane[0] = pc[0];
					paraPlane[1] = pc[1];
					paraPlane[2] = pc[2];
				}
				if (maxCount > 0.8)
					break;
	
			}
		}
		result.clear();
		for (int i = 0;i < Nums;i++)
		{
			if (fabs(vecPts[i].nx * pc[0] + vecPts[i].ny * pc[1] + vecPts[i].nz * pc[2]) < 0.01 )
			{
				result.push_back(vecPts[i]);
			}
		}
		delete []randNum;
		randNum = NULL;

		return result;

	}

	bool judgeLine(float *a,float *b,float *c)
	{
		if (a[0] == b[0] &&a[1] == b[1])
			return false;
		else if(a[0] == c[0] && a[1] == c[1])
			return false;
		else if(b[0] == c[0] && b[1] == c[1])
			return false;
		if ((c[1] - a[1]) * (b[0] - a[0]) - (b[1] - a[1]) * (c[0] - a[0]) != 0.0f)
			return true;
		else
			return false;
	}

	int jugdeVector(int a,int b,int c)
	{
		int maxNum = 0;
		maxNum = max(a,b);
		maxNum = max(maxNum,c);

		if (maxNum == a)
		{
			return 1;
		}
		else if (maxNum == b)
		{
			return 2;
		}
		else
		{
			return 3;
		}
	}

	float calculCylinderHeight(float a,float b,float c,vector<NormalPointXYZ>& vecPts)
	{
		float distHeight = 0.0f,minHeight = F32_MAX,maxHeight = F32_MIN,midHeight = 0.0f;
		float judge = 0.0f;
		for (int i  = 0;i < (int)vecPts.size();i++)
		{
			midHeight = a * vecPts[i].x + b * vecPts[i].y + c * vecPts[i].z;

			minHeight = MIN(minHeight,midHeight);
			maxHeight = MAX(maxHeight,midHeight);
		}
		distHeight = maxHeight - minHeight;
		return abs(distHeight);

	}

	bool CFitCylinder::cylinderFit(PointCloud& ptCloud,void (*loadCallback)(float,const char*))
	{
		int *ExternalCube;

		try
		{
			ptCloud.computeNormal(50,0.0f,NULL,false);
			ExternalCube = new int[40*40*40];  //将法向量组成的高斯球外切单位立方体划分栅格
		}
		catch (...)
		{
			return false;
		}

		memset(ExternalCube,0,sizeof(int)*(40*40*40));

		float step = 0.05f;
		int Num = 0;           
		int row = 0,col = 0,height = 0;

		int maxCube = 0,minCube = 0;
		
		Normal normal;
		PointXYZIPRGBA pts;
		vector<NormalPointXYZ> normalPts;
		NormalPointXYZ pt;
		//Eigen::Vector3f ptss;
		normalPts.clear();
		Num = ptCloud.getSelectCount();
		for (int i = 0;i < Num;i++)
		{
			pts = ptCloud.getSelectionPoint(i);
			normal = ptCloud.getNormal(i);

			pt.nx = normal.nx;
			pt.ny = normal.ny;
			pt.nz = normal.nz;
			pt.x = pts.x;
			pt.y = pts.y;
			pt.z = pts.z;
			normalPts.push_back(pt);

			if (normal.nx < 0.0f)
			{
				row = (int)((1 + normal.nx) / step);
			}
			else
			{
				row = (int)((normal.nx / step) + 19);
			}

			if (normal.ny < 0.0f)
			{
				col = (int)((1 + normal.ny) / step);
			}
			else
			{
                col = (int)((normal.ny / step) + 19);
			}

			if (normal.nz < 0.0f)
			{
				height = (int)((1 + normal.nz) / step);
			}
			else
			{
			    height = (int)((normal.nz / step) + 19);
			}

			// 添加判断，防止越界
			if (height < 0 || height >= 40 || col < 0 || col >= 40 || row < 0 || row >= 40)
			{
				continue;
			}
			
			if (height*40*40+col*40+row < 0 || height*40*40+col*40+row >= 40*40*40)
			{
				continue;
			}

			ExternalCube[height*40*40+col*40+row] = ExternalCube[height*40*40+col*40+row] + 1;
		}

		//进行聚类分析为两类
		int maxNum = 0,minNum = 0;
		int minCol,minRow,minHeight,maxCol,maxRow,maxHeight;
		minCol = 0,minRow = 0,minHeight = 0,maxCol = 0,maxRow = 0,maxHeight = 0; 
		maxCube = 0;
		minCube = 10000;
		for (int i = 0;i < 40*40*40;i++)
		{
			if (ExternalCube[i] != 0)
			{

				if (maxCube < ExternalCube[i])
				{
					maxCube = ExternalCube[i];
					maxNum = i;
				}
				if (minCube > ExternalCube[i])
				{
					minCube = ExternalCube[i];
					minNum = i;
				}
			}
		}

		minHeight = minNum / (40 * 40);
		minCol = (minNum % (40 * 40)) / 40;
		minRow = (minNum % (40 * 40)) % 40;
		maxHeight = maxNum / (40 * 40);
		maxCol = (maxNum % (40 * 40)) / 40;
		maxRow = (maxNum % (40 * 40)) % 40;

		float *minSeed,*maxSeed;  //选择密度值最大、最小的初始种子点
		minSeed = new float[3];
		maxSeed = new float[3];

		if (minRow < 20)
		{
			minSeed[0] = -1 * (1 - 0.05f * minRow);
		}
		else if (minRow > 20)
		{
			minSeed[0] = 0.05f * (minRow - 20);
		}
		else
			minSeed[0] = 0.0f;

		if (minCol < 20)
		{
			minSeed[1] =  -1 * (1 - 0.05f * minCol);
		}
		else if (minCol > 20)
		{
			minSeed[1] = 0.05f * (minCol - 20);
		}
		else
			minSeed[1] = 0.0f;

		if (minHeight < 20)
		{
			minSeed[2] = -1 * (1 - 0.05f * minHeight);
		}
		else if (minHeight > 20)
		{
			minSeed[2] = 0.05f * (minHeight - 20);
		}
		else
			minSeed[2] = 0.0f;


		if (maxRow < 20)
		{
			maxSeed[0] = -1 * (1 - 0.05f * maxRow);
		}
		else if (maxRow > 20)
		{
			maxSeed[0] = 0.05f * (maxRow - 20);
		}
		else
			maxSeed[0] = 0.0f;

		if (maxCol < 20)
		{
			maxSeed[1] = -1 * (1 - 0.05f * maxCol);
		}
		else if (maxCol > 20)
		{
			maxSeed[1] = 0.05f * (maxCol - 20);
		}
		else
			maxSeed[1] = 0.0f;

		if (maxHeight < 20)
		{
			maxSeed[2] = -1 * (1 - 0.05f * maxHeight);
		}
		else if (maxHeight > 20)
		{
			maxSeed[2] = 0.05f * (maxHeight - 20);
		}
		else
			maxSeed[2] = 0.0f;
		
		vector<NormalPointXYZ> bigCircleGaus;
		vector<NormalPointXYZ> noiseCircleGaus;
		int count = 0;
		double dist1 = 0.0;
		double dist2 = 0.0;
		double conver =100.0;
		int noiseNum,circleNum;
		float *minSeedIter,*maxSeedIter;
		minSeedIter = new float[3];
		maxSeedIter = new float[3];

		while (conver > 0.0001)
		{
			noiseCircleGaus.clear();
			bigCircleGaus.clear();

			for (int i = 0;i < Num;i++)
			{
				dist1 = fabs(normalPts[i].nx - minSeed[0]) + fabs(normalPts[i].ny - minSeed[1]) + fabs(normalPts[i].nz - minSeed[2]);
				dist2 = fabs(normalPts[i].nx - maxSeed[0]) + fabs(normalPts[i].ny - maxSeed[1]) + fabs(normalPts[i].nz - maxSeed[2]);

				if (dist1 <= dist2)
				{
					noiseCircleGaus.push_back(normalPts[i]);
				}
				else
					bigCircleGaus.push_back(normalPts[i]);
			}

			noiseNum = noiseCircleGaus.size();
			circleNum = bigCircleGaus.size();
			memset(minSeedIter,0,sizeof(float)*3);
			memset(maxSeedIter,0,sizeof(float)*3);

			for (int i = 0;i < noiseNum;i++)
			{
				minSeedIter[0] += noiseCircleGaus[i].nx;
				minSeedIter[1] += noiseCircleGaus[i].ny;
				minSeedIter[2] += noiseCircleGaus[i].nz;
			}
			for (int i = 0;i < circleNum;i++)
			{
				maxSeedIter[0] += bigCircleGaus[i].nx;
				maxSeedIter[1] += bigCircleGaus[i].ny;
				maxSeedIter[2] += bigCircleGaus[i].nz;
			}
			for (int i = 0;i < 3;i++)
			{
				maxSeedIter[i] = maxSeedIter[i] / circleNum;
				minSeedIter[i] = minSeedIter[i] / noiseNum;
			}

			dist1 = sqrt((minSeedIter[0] - minSeed[0]) * (minSeedIter[0] - minSeed[0]) + (minSeedIter[1] - minSeed[1]) * (minSeedIter[1] - minSeed[1]) + (minSeedIter[2] - minSeed[2]) * (minSeedIter[2] - minSeed[2]));
			dist2 = sqrt((maxSeedIter[0] - maxSeed[0]) * (maxSeedIter[0] - maxSeed[0]) + (maxSeedIter[1] - maxSeed[1]) * (maxSeedIter[1] - maxSeed[1]) + (maxSeedIter[2] - maxSeed[2]) * (maxSeedIter[2] - maxSeed[2]));
			conver = max(dist1,dist2);

			for (int i = 0;i < 3;i++)
			{
				minSeed[i] = minSeedIter[i];
				maxSeed[i] = maxSeedIter[i];
			}

			count++;
			if (count > 1000)
				break;
		}

		// 试验统计normalPts符合条件的法向量个数
		int numBigY = 0;
		int numBigZ = 0;
		int numBigX = 0;
		for (int i = 0;i < (int)normalPts.size();i++)
		{
			if (fabs(normalPts[i].nx) > 0.8)
			{
				numBigX++;
			}
			if (fabs(normalPts[i].ny) > 0.8)
			{
				numBigY++;
			}
			if (fabs(normalPts[i].nz) > 0.8)
			{
				numBigZ++;
			}
		}

		int mainDirection = jugdeVector(numBigX,numBigY,numBigZ);
		vector<NormalPointXYZ> mainDirecPts;
		mainDirecPts.clear();
		if (mainDirection == 1)
		{
			for (int i = 0;i < (int)normalPts.size();i++)
			{
				if (fabs(normalPts[i].nx) > 0.8)
				{
					mainDirecPts.push_back(normalPts[i]);
				}
			}
		}

		if (mainDirection == 2)
		{
			for (int i = 0;i < (int)normalPts.size();i++)
			{
				if (fabs(normalPts[i].ny) > 0.8)
				{
					mainDirecPts.push_back(normalPts[i]);
				}
			}
		}
		
		if (mainDirection == 3)
		{
			for (int i = 0;i < (int)normalPts.size();i++)
			{
				if (fabs(normalPts[i].nz) > 0.8)
				{
					mainDirecPts.push_back(normalPts[i]);
				}
			}
		}

		//随机取大圆中法向量得其向量积
		vector<NormalPointXYZ> resultPts;
//		vector<NormalPointXYZ> middlePts;
//		middlePts = cylinderRemove( mainDirecPts,middlePts);
		m_ds = fitPlaneInit(mainDirecPts);
		resultPts = fitPlane(mainDirecPts,resultPts,0.05f,100);

		float a0 = m_deigenvalue[0];
		float a1 = m_deigenvalue[1];
		float a2 = m_deigenvalue[2];
		float a3 = m_deigenvalue[3];

		int resultNum;
		resultNum = resultPts.size();

		// 结果点数少于4个 不满足拟合条件 返回
		// 解决没有点，后续代码执行崩溃问题 [2013/12/14 危迟]
		if (resultNum < 4)
		{
			delete []ExternalCube;
			delete []minSeed;
			delete []maxSeed;
			delete []minSeedIter;
			delete []maxSeedIter;
			return false;
		}

		vector<NormalPointXYZ> rotatePts;
		//圆柱点投影到过原点法向量为轴线向量的平面
		rotatePts.clear();
		float mid =0.0f;

		for (int i = 0;i < resultNum;i++)
		{
			mid = -1 * (a0 * resultPts[i].x + a1 * resultPts[i].y + a2 * resultPts[i].z);
			pt.x = resultPts[i].x + a0 * mid;
			pt.y = resultPts[i].y + a1 * mid;
			pt.z = resultPts[i].z + a2 * mid;
			pt.nx = resultPts[i].nx;
			pt.ny = resultPts[i].ny;
			pt.nz = resultPts[i].nz;
			rotatePts.push_back(pt);
		}

		//任取两点构成向量方向为X轴方向
		float u[3] = {0.0f,0.0f,0.0f};//新坐标系X方向
		float v[3] = {0.0f,0.0f,0.0f};//Y轴方向
		float w[3] = {0.0f,0.0f,0.0f};//Z轴方向
		
		u[0] = rotatePts[1].x - rotatePts[0].x;
		u[1] = rotatePts[1].y - rotatePts[0].y;
		u[2] = rotatePts[1].z - rotatePts[0].z;
		mid = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
		u[0] = u[0]/mid;
		u[1] = u[1]/mid;
		u[2] = u[2]/mid;

		w[0] = a0;
		w[1] = a1;
		w[2] = a2;

		v[0] = u[1] * w[2] - u[2] * w[1];
		v[1] = u[2] * w[0] - u[0] * w[2];
		v[2] = u[0] * w[1] - u[1] * w[0];
		mid = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] = v[0]/mid;
		v[1] = v[1]/mid;
		v[2] = v[2]/mid;

		float *R;
		R = new float[4*4];
		memset(R,0,sizeof(float)*16);
		R[0] = u[0];R[1] = u[1];R[2] = u[2];R[4] = 0.0;
		R[4] = v[0];R[5] = v[1];R[6] = v[2];R[7] = 0.0;
		R[8] = w[0];R[9] = w[1];R[10] = w[2];R[11] = 0.0;
		R[12] =0.0;R[13] = 0.0;R[14] = 0.0;R[15] = 1.0;

		float midRotate[4] = {0.0f,0.0f,0.0f,0.0f};  //原坐标系下坐标
		float cordinate[4] = {0.0f,0.0f,0.0f,0.0f};  //新坐标系下坐标

		vector<NormalPointXYZ> planePts;
		planePts.clear();
		for (int i = 0;i < (int)rotatePts.size();i++)
		{
			pt.x = rotatePts[i].x;
			pt.y = rotatePts[i].y;
			pt.z = rotatePts[i].z;
			pt.nx = rotatePts[i].nx;
			pt.ny = rotatePts[i].ny;
			pt.nz = rotatePts[i].nz;

			midRotate[0] = pt.x;
			midRotate[1] = pt.y;
			midRotate[2] = pt.z;
			midRotate[3] = 1.0;
			mult(midRotate,R,cordinate,1,4,4);
			pt.x = cordinate[0];
			pt.y = cordinate[1];
			pt.z = cordinate[2];
			
			planePts.push_back(pt);
		}
		//在平面内拟合圆得到定位点的二维坐标点
		resultNum = planePts.size();
		float *XX;
		XX = new float[3];

		int *randNum;
		int w0,middle;
		int nTimes = 0;
		int nCount = 0;
		float count1 = 0.0f;
		float pa[2] = {0.0f,0.0f};
		float pb[2] = {0.0f,0.0f};
		float pc[2] = {0.0f,0.0f};
		float pab[2] = {0.0f,0.0f};
		float pbc[2] = {0.0f,0.0f};
		float k1 = 0.0f;
		float k2 = 0.0f;
		float x[2] = {0.0f,0.0f};
		float maxCount = 0.0f;

		if (m_posXYZRH)
		{
			delete []m_posXYZRH;
			m_posXYZRH = NULL;
		}
		m_posXYZRH = new float[5];

		w0 = 0;
		middle = 0;
		randNum = new int[resultNum];
		while(nTimes < 1000)
		{
			for (int i = 0;i < resultNum;i++)
			{
				randNum[i] = i;
			}
			for (int i = 0;i < 3;i++)
			{
				w0 = rand()%(resultNum - i) + i;
				if (w0 > resultNum -1)
				{
					w0 = resultNum - 1;
				}

				middle = randNum[i];
				randNum[i] = randNum[w0];
				randNum[w0] = middle;
			}
			nTimes++;
			nCount = 0;
			pa[0] = planePts[randNum[0]].x;pa[1] = planePts[randNum[0]].y;
			pb[0] = planePts[randNum[1]].x;pb[1] = planePts[randNum[1]].y;
			pc[0] = planePts[randNum[2]].x;pc[1] = planePts[randNum[2]].y;

			//不在一条直线上的三点构成一个圆
			if (judgeLine(pa,pb,pc))
			{
				pab[0] = (pa[0] + pb[0]) * 0.5f; pab[1] = (pa[1] + pb[1]) * 0.5f;
				pbc[0] = (pb[0] + pc[0]) * 0.5f; pbc[1] = (pb[1] + pc[1]) * 0.5f;
				k1 = (pb[0] - pa[0])/(pa[1] - pb[1]);
				k2 = (pc[0] - pb[0])/(pb[1] - pc[1]);
				x[0] = (k1 * pab[0] - k2 * pbc[0] + pbc[1] - pab[1])/(k1 - k2);
				x[1] = k1 *(x[0] - pab[0]) + pab[1];
				float r = 0.0f;
				r = sqrt((x[0] -pa[0]) * (x[0] -pa[0]) + (x[1] -pa[1]) * (x[1] -pa[1]));

				for (int i = 0;i < resultNum;i++)
				{
					if (fabs(sqrt((planePts[i].x - x[0]) * (planePts[i].x - x[0]) + (planePts[i].y - x[1]) * (planePts[i].y - x[1])) - r) < 0.001)
					{
						nCount++;
					}
				}

				count1 = (nCount * 1.0f) / (resultNum * 1.0f);

				if (maxCount < count1)
				{
					maxCount = count1;
					XX[0] = x[0];
					XX[1] = x[1];
					XX[2] = r;
				}
				if (maxCount > 0.8)
					break;
			}

		}

		float circlePara[4] = {0.0f,0.0f,0.0f,0.0f};//平面圆心坐标及缩放参数
		circlePara[0] = XX[0];
		circlePara[1] = XX[1];
		circlePara[2] = 0;
		circlePara[3] = 1.0;

		invers_matrix(R,4);                         //圆心坐标转换回原坐标系
		mult(circlePara,R,cordinate,1,4,4);

		m_posXYZRH[0] = cordinate[0];
		m_posXYZRH[1] = cordinate[1];
		m_posXYZRH[2] = cordinate[2];
		m_posXYZRH[3] = XX[2];
		m_posXYZRH[4] = calculCylinderHeight(a0,a1,a2,normalPts);                        //垂直高度先设定为为2m

		delete []ExternalCube;
		delete []minSeed;
		delete []maxSeed;
		delete []minSeedIter;
		delete []maxSeedIter;
		delete []XX;
		delete []R;
		delete []randNum;

		return true;
	}
}