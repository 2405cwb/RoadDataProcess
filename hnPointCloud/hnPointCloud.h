#ifndef _HN_POINTCLOUD_H_
#define _HN_POINTCLOUD_H_
#include "stdafx.h"
#include "..\hnCommon\hn3dPointDef.h"
#include "..\hdPointCloud\SeaPointCloud.h"
using namespace hnCommon;
using namespace hd;;

namespace hnPtCloud
{
	class HNPTCLOUDAPI hnPointCloud
	{
	public:
		hnPointCloud();
		~hnPointCloud();

		// 根据左上角、左下角以及右上角、点云接口获取深度信息
		bool getDiseaseDepth(hn3dPointD ptLU, hn3dPointD ptLD, hn3dPointD ptRU, vector<CSeaPointCloud*> vecPcd, double& dDepth);

	private:
		// 获取点云数据--点云包围盒以及选择包围盒
		bool getPtCloud(CSeaPointCloud* pPtCloud, CHdBox3df pPcdBox, CHdBox3dd pSelBox, CBursaWolfModel pTransModel, vector<hn3dPointD>& vecOutPt, int nType = 0);

		// 获取绝对坐标包围盒
		void getRelativeBox(CBursaWolfModel pTransModel, CBursaWolfModel pcdModle, vector<hn3dPointD> vecBoxs,
			CHdBox3df& pcdBox);

		// 获取包围盒
		void getBox(vector<hn3dPointD> vecBoxs, CHdBox3dd& pcdBox);

		// 判断点云点是否在包围盒内  
		bool isPointInsideBox(
			CHdBox3dd box,								// 包围盒
			hn3dPointD point							// 判断的点
		);

		// 向vector中添加数据
		template<typename Type> bool push_DataVector(
			vector<Type>& vecData,
			size_t& nSize,
			Type& pData
		);

		// 获取Z值
		bool getCenterZ(vector<CSeaPointCloud*> vecOriPcd, hn3dPointD& pCenter, hn3dPointD pNormal);

		// 获取与中心点有交集的点云数据
		bool getValidPtCloud(hn3dPointD& pCenterPt, vector<CSeaPointCloud*> vecOriCloud, vector<CSeaPointCloud*>& vecPtCloud);

		// 创建自定义坐标系与84坐标系转换模型
		CBursaWolfModel createTransModel(hn3dPointD pCenterPt, hn3dPointD pNormal);

		// 设置当前包围盒
		void getGMPtBoxCoord(hn3dPointD pCenter, double dLeft, double dRight, double dHeight,
			double dThickness, vector<hn3dPointD>& vecBox);

		// 获取全局坐标的点云数据
		bool getCloudGlobalCoord(vector<CSeaPointCloud*> vecPcds, vector<hn3dPointD> vecBoxPts, CBursaWolfModel pTransModel,
			vector<hn3dPointD>& vecPts);

		// 过滤路面点云
		bool filterRoadPt(vector<hn3dPointD>& vecPt);

		// 测试
	private:
		vector<hn3dPointD> m_vecPt;
	};
}

#endif
