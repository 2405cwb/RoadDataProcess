//#include "StdAfx.h"
#include "hnRoadGeoDetect.h"
#include <random>
#include <iostream>
#include <time.h>
#include <set>
#include <cassert>
#include <limits.h>

///
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/opencv.hpp>
//#include <opencv2\xfeatures2d.hpp>
using namespace cv;
///

using namespace std;


namespace hn
{

	hnRoadGeoDetect::hnRoadGeoDetect()
	{
	}


	hnRoadGeoDetect::~hnRoadGeoDetect()
	{
	}

	bool hnRoadGeoDetect::calcLine(std::vector<hnPoint3d>& polyline, double &slope,double &error,int nMethod)
	{
		int status = 0;
		if (nMethod == 0)
		{
			int data_size = polyline.size();
			int k = 50;				//最大迭代次数
			int n = 2;				//适用于模型的最少数据个数
			double t = 0.01;		//用于决定数据是否适应于模型的阀值
			int d = data_size*0.5;	//判定模型是否适用于数据集的数据数目 
									//3.初始化输出量
			linearModel best_model;			//最佳线性模型
			set<int> best_consensus_set;	//记录一致点索引的set
			double best_error;				//最小残差
											//4.运行RANSAC			
			status = ransac(polyline, n, k, t, d, best_model, best_consensus_set, best_error);
			cout << best_model.A << " " << best_model.B << " " << best_model.C <<" " <<error <<endl;
			m_lineFitA = best_model.A;
			m_lineFitB = best_model.B;
			m_lineFitC = best_model.C;
			slope = abs(best_model.A / best_model.B);
		}

		return status;
	}

	bool hnRoadGeoDetect::calcCurv(hnPoint3d & lastPoint, hnPoint3d & curPoint, hnPoint3d & nextPoint, double & curvature)
	{
		/*double omega = abs(atan((double)(curPoint.y - lastPoint.y) / (curPoint.x - lastPoint.x)) - abs(atan((double)(nextPoint.y - curPoint.y) / (nextPoint.x - curPoint.x))));
		double p0p1 = sqrt((curPoint.x - lastPoint.x)*(curPoint.x - lastPoint.x) + (curPoint.y - lastPoint.y)*(curPoint.y - lastPoint.y));
		double p1p2 = sqrt((curPoint.x - nextPoint.x)*(curPoint.x - nextPoint.x) + (curPoint.y - nextPoint.y)*(curPoint.y - nextPoint.y));
		curvature = 2 * omega*3.14159265 / 180. / (p0p1 + p1p2);*/

		//if (lastPoint.x == curPoint.x == nextPoint.x)
		//{
		//	curvature = 0.0;
		//}

		double dis1,dis2,dis3;
		dis1 = dis2 = dis3 = 0.0;

		double cosA, sinA, dis;
		cosA = sinA = dis = 0.0;

		dis1 = sqrt((lastPoint.x - curPoint.x)*(lastPoint.x - curPoint.x) + 
			(lastPoint.y - curPoint.y)*(lastPoint.y - curPoint.y));

		dis2 = sqrt((lastPoint.x - nextPoint.x)*(lastPoint.x - nextPoint.x) + 
			(lastPoint.y - nextPoint.y)*(lastPoint.y - nextPoint.y));

		dis3 = sqrt((curPoint.x - nextPoint.x)*(curPoint.x - nextPoint.x) + 
			(curPoint.y - nextPoint.y)*(curPoint.y - nextPoint.y));

		dis = dis1*dis1 + dis3*dis3 - dis2*dis2;

		if (2.0*dis1*dis3 == 0.0)
		{
			curvature = 0.0;
		}
		else
		{
			cosA = dis/ (2.0*dis1*dis3);
			sinA = sqrt(1-cosA*cosA);
			if (sinA == 0.0)
			{
				curvature = 0.0;
			}
			else
			{
				curvature = 0.5*dis2/ sinA;
			}
		}

		return true;
	}


	double calcY(double a, double b, double c,double x)
	{
		return -(c + a*x) / b;
	}

	void hnRoadGeoDetect::calcRansac()
	{
	//	//1.读入数据
	//	//int data_size;		//输入第一行表示数据大小
	//	//cin >> data_size;
	//	//vector<hnPoint2d> Points(data_size);
	//	//for (int i = 0; i < data_size; i++) {
	//	//	cin >> Points[i].x >> Points[i].y;
	//	//}
	//	//测试用
	//	vector<hnPoint3d> Points;
	//	ifstream infile;
	//	infile.open("F:\\20210316\\3droad\\1.xyz");

	//	if (!infile.is_open())
	//	{
	//		return ;
	//	}

	//	std::string strTemp = "";

	//	while (getline(infile, strTemp))
	//	{
	//		//vecTxt.push_back(str2qstr(strTemp));
	//		double x = atof(strTemp.substr(0,strTemp.find(',')).c_str());
	//		double y = atof(strTemp.substr(strTemp.find_last_of(',')+1).c_str());
	//		hnPoint3d pt = hnPoint3d(x, y, 0.0);
	//		Points.push_back(pt);
	//	}
	//	infile.close();
	//	//vector<hnPoint2d> Points{ hnPoint2d(3, 4), hnPoint2d(6, 8), hnPoint2d(9, 12), hnPoint2d(15, 20), hnPoint2d(10,10)};
	//	int data_size = Points.size();
	//	//2.设置输入量
	//	int k = 50;				//最大迭代次数
	//	int n = 2;				//适用于模型的最少数据个数
	//	double t = 0.002;		//用于决定数据是否适应于模型的阀值
	//	int d = data_size*0.5;	//判定模型是否适用于数据集的数据数目 
	//							//3.初始化输出量
	//	linearModel best_model;			//最佳线性模型
	//	set<int> best_consensus_set;	//记录一致点索引的set
	//	double best_error;				//最小残差
	//									//4.运行RANSAC			
	//	int status = ransac(Points, n, k, t, d, best_model, best_consensus_set, best_error);
	//	//5.输出
	////	cout << best_model.A << " " << best_model.B << " " << best_model.C << endl;
	//	namedWindow("aaa", 0);
	//	Mat canvas = Mat::zeros(100,100,CV_8UC3);
	//	double xmin = Points[0].x, xmax = Points[0].x;
	//	for (int i = 0; i < Points.size(); i++)
	//	{
	//		line(canvas, Point(Points[i].x, Points[i].y), Point(Points[i].x, Points[i].y), Scalar(255, 255, 255));
	//		if (Points[i].x > xmax) xmax = Points[i].x;
	//		if (Points[i].x < xmin) xmin = Points[i].x;
	//	}
	//	line(canvas, Point(xmin, calcY(best_model.A, best_model.B, best_model.C, xmin)), Point(xmax, calcY(best_model.A, best_model.B, best_model.C, xmax)), Scalar(0, 0, 255));
	//	double x1 = xmin, x2 = calcY(best_model.A, best_model.B, best_model.C, xmin);
	//	double y1 = xmax, y2 = calcY(best_model.A, best_model.B, best_model.C, xmax);
	//	double slope = abs(best_model.A / best_model.B);
	//	imshow("aaa", canvas);
	//	waitKey(0);
	//	//return 0;
	}

	void hnRoadGeoDetect::calcRansac(std::vector<hnPoint3d>& Points)
	{
		//测试用
		int data_size = Points.size();
		//2.设置输入量
		int k = 50;				//最大迭代次数
		int n = 2;				//适用于模型的最少数据个数
		double t = 0.01;		//用于决定数据是否适应于模型的阀值
		int d = data_size*0.5;	//判定模型是否适用于数据集的数据数目 
								//3.初始化输出量
		linearModel best_model;			//最佳线性模型
		set<int> best_consensus_set;	//记录一致点索引的set
		double best_error;				//最小残差
										//4.运行RANSAC			
		int status = ransac(Points, n, k, t, d, best_model, best_consensus_set, best_error);
		////5.输出
		////	cout << best_model.A << " " << best_model.B << " " << best_model.C << endl;
		//namedWindow("aaa", 0);
		//Mat canvas = Mat::zeros(100, 100, CV_8UC3);
		//double xmin = Points[0].x, xmax = Points[0].x;
		//for (int i = 0; i < Points.size(); i++)
		//{
		//	line(canvas, Point(Points[i].x, Points[i].y), Point(Points[i].x, Points[i].y), Scalar(255, 255, 255));
		//	if (Points[i].x > xmax) xmax = Points[i].x;
		//	if (Points[i].x < xmin) xmin = Points[i].x;
		//}
		//line(canvas, Point(xmin, calcY(best_model.A, best_model.B, best_model.C, xmin)), Point(xmax, calcY(best_model.A, best_model.B, best_model.C, xmax)), Scalar(0, 0, 255));
		//imshow("aaa", canvas);
		//waitKey(0);
	}

	linearModel::linearModel() {};
	linearModel::~linearModel() {};

		//使用两个点对直线进行初始估计
	void linearModel::Update(vector<hnPoint3d> &data, set<int> &maybe_inliers) {
			assert(maybe_inliers.size() == 2);		//初始化的点不为2个，报错
													//根据索引读取数据
			vector<int> points(maybe_inliers.begin(), maybe_inliers.end());
			hnPoint3d pts1 = data[points[0]];
			hnPoint3d pts2 = data[points[1]];
			//根据两个点计算直线参数（得到其中一组解，可以任意比例缩放）
			double delta_x = pts2.x - pts1.x;
			double delta_y = pts2.y - pts1.y;
			A = delta_y;
			B = -delta_x;
			C = -delta_y*pts2.x + delta_x*pts2.y;
		}

		//返回点到直线的距离
	double linearModel::computeError(hnPoint3d point) {
			double numerator = abs(A*point.x + B*point.y + C);
			double denominator = sqrt(A*A + B*B);
			return numerator / denominator;
		}

		//根据一致点的集合对直线进行重新估计
	double linearModel::Estimate(vector<hnPoint3d> &data, set<int> &consensus_set) {
		assert(consensus_set.size() >= 2);
		//求均值 means
		double mX, mY;
		mX = mY = 0;
	//	for (auto &index : consensus_set) 
		for(int index=0;index<consensus_set.size();index++)
		{
			mX += data[index].x;
			mY += data[index].y;
		}
		mX /= consensus_set.size();
		mY /= consensus_set.size();

		//求二次项的和 sum
		double sXX, sYY, sXY;
		sXX = sYY = sXY = 0;
		//for (auto &index : consensus_set) 
		for(int index=0;index<consensus_set.size();index++)
		{
			hnPoint3d point;
			point = data[index];
			sXX += (point.x - mX)*(point.x - mX);
			sYY += (point.y - mY)*(point.y - mY);
			sXY += (point.x - mX)*(point.y - mY);
		}
		/*
		//解法1：求y=kx+b的最小二乘估计，然后再转换成一般形式
		//参考 https://blog.csdn.net/hookie1990/article/details/91406309
		bool isVertical = sXY == 0 && sXX < sYY;
		bool isHorizontal = sXY == 0 && sXX > sYY;
		bool isIndeterminate = sXY == 0 && sXX == sYY;
		double k = NAN;
		double b = NAN;
		if (isVertical)
		{
		A = 1;
		B = 0;
		C = mX;
		}
		else if (isHorizontal)
		{
		A = 0;
		B = 1;
		C = mY;
		}
		else if (isIndeterminate)
		{
		A = NAN;
		B = NAN;
		C = NAN;
		}
		else
		{
		k = (sYY - sXX + sqrt((sYY - sXX) * (sYY - sXX) + 4.0 * sXY * sXY)) / (2.0 * sXY);	//斜率
		b = mY - k * mX;															//截距
		//正则化项，使得A^2+B^2 = 1;
		double normFactor = 1 / sqrt(1 + k*k);
		A = normFactor * k;
		B = -normFactor;
		C = normFactor*b;
		}
		//返回残差
		if (isIndeterminate){
		return NAN;
		}
		double error = A*A*sXX + 2 * A*B*sXY + B*B*sYY;
		error /= consensus_set.size();
		return error;
		*/
		//解法2：
		if (sXX == 0) {
			A = 1;
			B = 0;
			C = -mX;
		}
		else {
			A = sXY / sXX;
			B = -1;
			C = mY - A*mX;
			//归一化令A^2+B^2 = 1;
			double normFactor = sqrt(A*A + B*B);
			A /= normFactor;
			B /= normFactor;
			C /= normFactor;
		}
		double error = A*A*sXX + 2 * A*B*sXY + B*B*sYY;
		error /= consensus_set.size();    //求平均误差
		return error;

	}


	/**
	* @brief 运行RANSAC算法
	*
	* @param[in]	data	一组观测数据
	* @param[in]	n		适用于模型的最少数据个数
	* @param[in]	k		算法的迭代次数
	* @param[in]	t		用于决定数据是否适应于模型的阀值
	* @param[in]	d		判定模型是否适用于数据集的数据数目 
	* @param[in&out]	model	自定义的待估计模型，为该函数提供Update、computeError和Estimate三个成员函数
	*							运行结束后，模型参数被设置为最佳的估计值
	* @param[out]	best_consensus_set	输出一致点的索引值
	* @param[out]	best_error	输出最小损失函数
	*/
	template<typename T, typename U>
	int hnRoadGeoDetect::ransac(vector<T> &data, int n, int k, double t, int d,
		U &best_model, set<int> &best_consensus_set, double &best_error) {
		//1.初始化
		int  iterations = 0;	//迭代次数
		U maybe_model;			//使用随机选点初始化求得的模型
		U better_model;			//根据符合条件的一致点拟合出的模型

		int isFound = 0;					//算法成功的标志
		set<int> maybe_inliers;				//初始随机选取的点（的索引值）

											//best_error = DBL_MAX;	//初始化为最大值
		best_error = 1.7976931348623158e+308;
		default_random_engine rng(time(NULL));					//随机数生成器
		uniform_int_distribution<int> dist(0, data.size() - 1);	//采用均匀分布

																//2.主循环
		while (iterations < k) {
			//3.随机选点
			maybe_inliers.clear();
			while (1) {
				int index = dist(rng);
				maybe_inliers.insert(index);
				if (maybe_inliers.size() == n) {
					break;
				}
			}
			//4.计算初始值
			maybe_model.Update(data, maybe_inliers);								//自定义函数，更新模型
			set<int> consensus_set(maybe_inliers.begin(), maybe_inliers.end());		//选取模型后，根据误差阈值t选取的内点(的索引值)

																					//5.根据初始模型和阈值t选择内点	
			for (int i = 0; i < data.size(); i++) {
				double error_per_item = maybe_model.computeError(data[i]);
				if (error_per_item < t) {
					consensus_set.insert(i);
				}
			}
			//6.根据全部的内点重新计算模型
			if (consensus_set.size() > d) {
				double this_error = better_model.Estimate(data, consensus_set);		//自定义函数，（最小二乘）更新模型，返回计算出的误差
																					//7.若当前模型更好，则更新输出量
				if (this_error < best_error) {
					best_model = better_model;
					best_consensus_set = consensus_set;
					best_error = this_error;
				}
				isFound = 1;
			}
			++iterations;
		}

		return isFound;
	}

	bool hnRoadGeoDetect::fitLine( std::vector<hnPoint3d>& line )
	{
		double a,b,c;
		a = b = c = 0.0;

		int size = line.size();
		double x_mean = 0;
		double y_mean = 0;
		for (int i = 0; i < size; i++)
		{
			x_mean += line[i].x;
			y_mean += line[i].y;
		}
		x_mean /= size;
		y_mean /= size; //至¨￠此??，ê?计?算?出?了￠? x y 的ì?均¨′值|ì

		double Dxx = 0, Dxy = 0, Dyy = 0;

		for (int i = 0; i < size; i++)
		{
			Dxx += (line[i].x - x_mean) * (line[i].x - x_mean);
			Dxy += (line[i].x - x_mean) * (line[i].y - y_mean);
			Dyy += (line[i].y - y_mean) * (line[i].y - y_mean);
		}
		double lambda = ((Dxx + Dyy) - sqrt((Dxx - Dyy) * (Dxx - Dyy) + 4 * Dxy * Dxy)) / 2.0;
		double den = sqrt(Dxy * Dxy + (lambda - Dxx) * (lambda - Dxx));
		a = Dxy / den;
		b = (lambda - Dxx) / den;
		c = -a * x_mean - b * y_mean;

		m_lineFitA = a;
		m_lineFitB = b;
		m_lineFitC = c;

		return true;
	}

}
