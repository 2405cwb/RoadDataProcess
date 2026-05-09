/*!@HdQuadSourceIndex.h
*******************************************************************************************************
<PRE>
模块名		：hdPointCloud
文件名		：HdQuadSourceIndex.h
相关文件	: HdQuadSourceIndex.cpp
文件实现功能：二维TIN三角形索引结构
作者		：研发部 冯晶
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2014/3/19	1.0			冯晶		 创建
</PRE>
******************************************************************************************************/
#pragma once
#include "hdPointCloud.h"
#include "point_cloud.h"
#include "HdModelPointCloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCommon\point_types.h"
#include "..\hdCore\hdBox3d.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "..\hd3DEngine\include\aabbox3d.h"
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <string>
#include <vector>
#include "..\hdCommon\point_types.h"
#include "..\hd3DEngine\include\S3DVertex.h"
#include "..\hd3DEngine\include\SViewFrustum.h"
using namespace std;
using namespace irr;
using namespace irr::video;

namespace hd
{
	// 四叉树节点
	typedef struct _QuadNode
	{	
		core::aabbox3df			BoundingBox;			// 包围盒
		vector<u32>				TrigItems;				// 三角形索引
		_QuadNode*				Children[4];			// 孩子节点
		bool					IsValid;				// 是否有效

		// 构造函数
		_QuadNode()
			: IsValid(false)
		{
			// 初始化
			for (int i = 0; i < 4; i++)
			{
				Children[i] = NULL;
			}
		}
	}QuadNode;

	class HDPOINTCLOUD_API CHdQuadSourceIndex
	{
	public:
		// 构造函数
		CHdQuadSourceIndex(S3DVertex2TCoords* modelPts, u32* indexData,  u32 indexCount);

		// 析构函数
		virtual ~CHdQuadSourceIndex(void);

	private:
		// 禁止拷贝
		CHdQuadSourceIndex(const CHdQuadSourceIndex &Quad)
		{
		}

		// 禁止复制
		const CHdQuadSourceIndex& operator=(const CHdQuadSourceIndex& Quad)
		{
			return *this;
		}
	public:
		// 自适应计算四叉树树高
		void CalcLevels();

		// 构建四叉树空间索引
		void BuildQuadTree(core::aabbox3df Box);

		// 查找四叉树-根据范围查询三角形索引
		void SearchQuadTree(QuadNode* node, const core::aabbox3df& SelBox, vector<u32>& items);

		// 查找四叉树-根据点索引查询经过该点的三角形
		void SearchQuadTree(QuadNode* node, u32 index, vector<u32>& items);

		// 查找四叉树-查询经过指定线的三角形
		void SearchQuadTree(core::line3df selLine, vector<u32>& items);

		// 查找四叉树-根据视锥体范围查询三角形索引
		void SearchQuadTree(QuadNode* node, const irr::scene::SViewFrustum& frustum, vector<u32>& items);

		// 查找相邻三角形，传入指定三角形索引，传出相邻的三角形
		void SearchAdjionTria(QuadNode* node, u32 index0, u32 index1, u32 index2, vector<u32>& items);

		// 查找指定三角形
		bool DeleteQuadTree(QuadNode* node, u32 index0, u32 index1, u32 index2);

		// 删除指定的多个三角形
		bool DeleteQuadTree(vector<u32>& items);

		// 删除指定三角形-根据范围删除
		void DeleteQuadTree(core::aabbox3df SelBox);

		// 删除制定三角形-根据指定点所在三角形
		void DeleteQuadTree(u32 index);

		// 获取根节点
		inline QuadNode*& GetQuardRoot();

	private:
		// 分割范围
		void SplitBox(core::aabbox3df Box, core::aabbox3df& URBox, core::aabbox3df& ULBox, core::aabbox3df& LLBox, core::aabbox3df& LRBox);

		// 创建各个分支
		void CreateQuadBranch(int depth, QuadNode* node);

		// 是否为叶子节点
		bool IsQuadLeaf(QuadNode* node);

		// 根据三角形索引，将三角形插入相应的叶子节点中
		void InsertQuad(int index, core::aabbox3df Box, QuadNode* node);

		// 遍历所有节点
		void TraversalQuadTree();

		// 重新构建四叉树
		void RebulidQuadTree(core::aabbox3df Box);

		// 删除三角形后更新三角形索引
		void UpdateTrianlgeIndex();

		// 删除三角形后更新点
		void UpdatePts();

		// 删除空节点
		void DeleteNullNode(QuadNode* node);

		// 释放树的空间
		void ReleseQuadTree(QuadNode** node);

	protected:
		S3DVertex2TCoords*	m_pts;					// 定点数据
		u32*				m_IndexData;			// 原始三角形数据
		QuadNode*			m_QuardRoot;			// 四叉树根节点
		u32					m_IndexDataCount;		// 三角形数量
		u32					m_NodeCount;			// 四叉树结点数量
		int					m_Depth;				// 四叉树的高度

	public:
		u32				m_PerLeafIndexs;		// 叶子节点的临界点数
	};
}

