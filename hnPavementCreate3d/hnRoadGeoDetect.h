#pragma once
//#include "stdafx.h"
#include<vector>
#include<set>
#include "hnDataCombineStructInfo.h"
#include "hnpavementcreate3d_global.h"

namespace hn
{
	class linearModel {
	public:
		//待估计参数
		double A, B, C;
	public:
		linearModel();
		~linearModel();

		double computeError(hnPoint3d point);

		//使用两个点对直线进行初始估计;
		void Update(std::vector<hnPoint3d> &data, std::set<int> &maybe_inliers);

		//根据一致点的集合对直线进行重新估计;
		double Estimate(std::vector<hnPoint3d> &data, std::set<int> &consensus_set);

	};

	class HNPAVEMENTCREATE3D_EXPORT hnRoadGeoDetect
	{
	public:
		hnRoadGeoDetect();
		~hnRoadGeoDetect();

	public:
		bool calcLine(std::vector<hnPoint3d>& polyline,double &slope,double &error,int nMethod = 0);
		bool calcCurv(hnPoint3d& lastPoint, hnPoint3d& curPoint, hnPoint3d& nextPoint, double &curvature);
		bool fitLine(std::vector<hnPoint3d>& ptVec);

		//测试RANSAC算法
		void calcRansac();
		void calcRansac(std::vector<hnPoint3d>& polyline);

		double m_lineFitA;
		double m_lineFitB;
		double m_lineFitC;
	private:

		template<typename T, typename U>
		int ransac(std::vector<T> &data, int n, int k, double t, int d, U &best_model, std::set<int> &best_consensus_set, double &best_error);
	};

}
