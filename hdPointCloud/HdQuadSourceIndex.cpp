/*!@HdQuadSourceIndex.cpp
*******************************************************************************************************
<PRE>
模块名		：hdPointCloud
文件名		：HdQuadSourceIndex.cpp
相关文件	: HdQuadSourceIndex.h
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
#include "StdAfx.h"
#include "HdQuadSourceIndex.h"
#include "..\hdCommon\hdSceneStr.h"

namespace hd
{
	// 构造
	CHdQuadSourceIndex::CHdQuadSourceIndex(S3DVertex2TCoords* modelPts, u32* indexData, u32 indexCount)
	{
		// 定点数据
		m_pts = modelPts;

		// 三角形索引
		m_IndexData = indexData;

		// 四叉树根节点
		m_QuardRoot = NULL;

		// 节点总数
		m_NodeCount = 0;

		// 三角形索引个数
		m_IndexDataCount = indexCount;

		// 设置叶子节点的三角形数量的临界值
		m_PerLeafIndexs = 300;
	}

	// 析构
	CHdQuadSourceIndex::~CHdQuadSourceIndex(void)
	{
		// 释放树结构
		ReleseQuadTree(&m_QuardRoot);
	}

	// 自适应计算四叉树树高
	void CHdQuadSourceIndex::CalcLevels()
	{
		// 判空
		if (!m_QuardRoot)
		{
			return;
		}

		// 点数
		int count = m_IndexDataCount;	

		// 求划分的投影平面
		double x = m_QuardRoot->BoundingBox.MaxEdge.X - m_QuardRoot->BoundingBox.MinEdge.X;
		double y = m_QuardRoot->BoundingBox.MaxEdge.Y - m_QuardRoot->BoundingBox.MinEdge.Y;
		// double z = m_QuardRoot->BoundingBox.MaxEdge.Z - m_QuardRoot->BoundingBox.MinEdge.Z;

		// 间距		
		int d = 0;	

		// 根据包围盒dx与dy的范围大小计算初始值
		if (x > y)
		{
			d = (int)(x / y);
		}
		else
		{
			d = (int)(y / x);
		}

		// 点数初值
		count /= d;

		// 高度
		m_Depth = 1;

		// 每个叶子节点的三角形不得大于临界值，自适应计算树的高度
		while (count > m_PerLeafIndexs)
		{
			m_Depth++;		// 层数递增
			count /= 4;		// 点数递减
		}
	}

	// 分割范围,分割规则如下
	//	UR(1)	|	LL(3)
	//	--------|-------
	//	UL(0)	|	LR(2)
	void CHdQuadSourceIndex::SplitBox(core::aabbox3df Box, core::aabbox3df& ULBox, core::aabbox3df& URBox, core::aabbox3df& LRBox, core::aabbox3df& LLBox)
	{
		// 求的包围盒子的划分间隔范围
		double dx = (Box.MaxEdge.X - Box.MinEdge.X) / 2;
		double dy = (Box.MaxEdge.Y - Box.MinEdge.Y) / 2;

		ULBox.MaxEdge.X = (float)(Box.MinEdge.X + dx);
		ULBox.MinEdge.X = (float)Box.MinEdge.X;
		ULBox.MaxEdge.Y = (float)(Box.MinEdge.Y + dy);
		ULBox.MinEdge.Y = (float)Box.MinEdge.Y;
		ULBox.MaxEdge.Z = (float)Box.MaxEdge.Z;
		ULBox.MinEdge.Z = (float)Box.MinEdge.Z;

		URBox.MaxEdge.X = (float)Box.MaxEdge.X;
		URBox.MinEdge.X = (float)(Box.MaxEdge.X - dx);
		URBox.MaxEdge.Y = (float)(Box.MinEdge.Y + dy);
		URBox.MinEdge.Y = (float)Box.MinEdge.Y;
		URBox.MaxEdge.Z = (float)Box.MaxEdge.Z;
		URBox.MinEdge.Z = (float)Box.MinEdge.Z;

		LRBox.MaxEdge.X = (float)(Box.MinEdge.X + dx);
		LRBox.MinEdge.X = (float)Box.MinEdge.X;
		LRBox.MaxEdge.Y = (float)Box.MaxEdge.Y;
		LRBox.MinEdge.Y = (float)(Box.MaxEdge.Y - dy);
		LRBox.MaxEdge.Z = (float)Box.MaxEdge.Z;
		LRBox.MinEdge.Z = (float)Box.MinEdge.Z;

		LLBox.MaxEdge.X = (float)Box.MaxEdge.X;
		LLBox.MinEdge.X = (float)(Box.MaxEdge.X - dx);
		LLBox.MaxEdge.Y = (float)Box.MaxEdge.Y;
		LLBox.MinEdge.Y = (float)(Box.MaxEdge.Y - dy);
		LLBox.MaxEdge.Z = (float)Box.MaxEdge.Z;
		LLBox.MinEdge.Z = (float)Box.MinEdge.Z;	
	}

	// 创建各个分支
	void CHdQuadSourceIndex::CreateQuadBranch(int depth, QuadNode* node)
	{
		// 判空
		if (!depth && node)
		{
			return;
		}

		// 定义分割后的四个子包围盒 ULBox, URBox, LRBox, LLBox;
		core::aabbox3df box[4];

		// 分割包围盒
		SplitBox(node->BoundingBox, box[0], box[1], box[2], box[3]);

		for (int i = 0; i < 4; i++)
		{
			// 创建四个节点
			try
			{
				// 新建节点
				node->Children[i] = new QuadNode();
			}
			catch(...)
			{
				// 分配失败
				::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
				return;
			}

			// 设置范围
			node->Children[i]->BoundingBox.MaxEdge.X = box[i].MaxEdge.X;
			node->Children[i]->BoundingBox.MinEdge.X = box[i].MinEdge.X;
			node->Children[i]->BoundingBox.MaxEdge.Y = box[i].MaxEdge.Y;
			node->Children[i]->BoundingBox.MinEdge.Y = box[i].MinEdge.Y;
			node->Children[i]->BoundingBox.MaxEdge.Z = box[i].MaxEdge.Z;
			node->Children[i]->BoundingBox.MinEdge.Z = box[i].MinEdge.Z;

			//递归创建
			CreateQuadBranch(depth - 1, node->Children[i]);
		}
	}

	// 构建四叉树空间索引
	void CHdQuadSourceIndex::BuildQuadTree(core::aabbox3df Box)
	{
		// 判空
		if (!m_IndexData || !m_pts)
		{
			return;
		}

		// 新建根节点
		try
		{
			// 创建根节点
			m_QuardRoot = new QuadNode();
		}
		catch(...)
		{
			// 分配失败
			::MessageBox(NULL,HDSCENE_IDS_LOADMODEL_OUTMEMORY,HDSCENE_IDS_PROMPT,MB_OK);
			return;
		}

		// 设置包围盒
		m_QuardRoot->BoundingBox = Box;

		// 计算树高
		CalcLevels();

		// 创建分支
		CreateQuadBranch(m_Depth, m_QuardRoot);
	
		// 插入节点
		for (u32 i = 0; i < m_IndexDataCount; i += 3)
		{
			// 取三角形的三个顶点
			core::vector3df point0(m_pts[m_IndexData[i]].Pos.X, m_pts[m_IndexData[i]].Pos.Y, m_pts[m_IndexData[i]].Pos.Z);
			core::vector3df point1(m_pts[m_IndexData[i + 1]].Pos.X, m_pts[m_IndexData[i + 1]].Pos.Y, m_pts[m_IndexData[i + 1]].Pos.Z);
			core::vector3df point2(m_pts[m_IndexData[i + 2]].Pos.X, m_pts[m_IndexData[i + 2]].Pos.Y, m_pts[m_IndexData[i + 2]].Pos.Z);
			
			// 获取三角形的范围
			core::aabbox3df Triabox(point0);
			Triabox.addInternalPoint(point1);
			Triabox.addInternalPoint(point2);

			// 将该三角形插入该四叉树中
			InsertQuad(i, Triabox, m_QuardRoot);
		}

		// 删除空节点
		//DeleteNullNode(m_QuardRoot);
	}

	// 查找四叉树-根据范围查询三角形索引
	void CHdQuadSourceIndex::SearchQuadTree(QuadNode* node, const core::aabbox3df& SelBox, vector<u32>& items)
	{
		// 判空，指定范围大于节点范围
		if (!node || !m_IndexData || node->BoundingBox.isFullInside(SelBox))
		{
			return;
		}

		// 指定范围完全包含于当前节点，或相交，而且当前节点为叶子节点
		if ((SelBox.isFullInside(node->BoundingBox) || SelBox.intersectsWithBox(node->BoundingBox)) && IsQuadLeaf(node))
		{ 
			// 将索引赋给搜索结果
			for (u32 i = 0; i < node->TrigItems.size(); i += 3)
			{
				// 取三角形的三个顶点
				core::vector3df point0(m_pts[node->TrigItems[i]].Pos.X, m_pts[node->TrigItems[i]].Pos.Y, m_pts[node->TrigItems[i]].Pos.Z);
				core::vector3df point1(m_pts[node->TrigItems[i + 1]].Pos.X, m_pts[node->TrigItems[i + 1]].Pos.Y, m_pts[node->TrigItems[i + 1]].Pos.Z);
				core::vector3df point2(m_pts[node->TrigItems[i + 2]].Pos.X, m_pts[node->TrigItems[i + 2]].Pos.Y, m_pts[node->TrigItems[i + 2]].Pos.Z);
				
				// 三个点满足有两个点在指定范围，则该三角形在指定范围
				if ((SelBox.isPointTotalInside(point0) && SelBox.isPointTotalInside(point1)) ||
					(SelBox.isPointTotalInside(point1) && SelBox.isPointTotalInside(point2)) ||
					(SelBox.isPointTotalInside(point0) && SelBox.isPointTotalInside(point2)))
				{
					// 存进返回数组
					items.push_back(node->TrigItems[i]);
					items.push_back(node->TrigItems[i + 1]);
					items.push_back(node->TrigItems[i + 2]);
				}		
			}
			return;
		}

		// 遍历子节点
		for (int i = 0; i < 4; i++)
		{
			// 判空
			if (node->Children[i])
			{
				// 递归搜索
				SearchQuadTree(node->Children[i], SelBox, items);
			}
		}
	}

	// 获取根节点
	QuadNode*& CHdQuadSourceIndex::GetQuardRoot()
	{
		return m_QuardRoot;
	}

	// 查找四叉树-根据三角形索引
	void CHdQuadSourceIndex::SearchQuadTree(QuadNode* node, u32 index, vector<u32>& items)
	{
		// 判空，指定范围大于节点范围
		if (!node || !m_IndexData)
		{
			return;
		}

		// 取三角形的三个顶点
		core::vector3df point(m_pts[index].Pos.X, m_pts[index].Pos.Y, m_pts[index].Pos.Z);
		core::aabbox3df TriaBox(point);

		// 当前节点为叶子节点，且当前顶点都在当前子节点中，
		if (IsQuadLeaf(node) && (TriaBox.isFullInside(node->BoundingBox) || TriaBox.intersectsWithBox(node->BoundingBox)))
		{
			// 将索引赋给搜索结果
			for (u32 i = 0; i < node->TrigItems.size(); i += 3)
			{
				// 找到含有索引index的三角形
				if (node->TrigItems[i] == index)
				{
					// 存进返回数组
					items.push_back(node->TrigItems[i]);
					items.push_back(node->TrigItems[i + 1]);
					items.push_back(node->TrigItems[i + 2]);
				}		
			}
			return;
		}

		// 遍历子节点
		for (int i = 0; i < 4; i++)
		{
			// 判空
			if (node->Children[i])
			{
				// 递归搜索
				SearchQuadTree(node->Children[i], index, items);
			}
		}
	}

	// 查找四叉树-查询经过指定线的三角形
	void CHdQuadSourceIndex::SearchQuadTree(core::line3df selLine, vector<u32>& items)
	{

	}

	// 查找四叉树-根据视锥体范围查询三角形索引
	void CHdQuadSourceIndex::SearchQuadTree(QuadNode* node, const irr::scene::SViewFrustum& frustum, vector<u32>& items)
	{
		// 判空，视椎体范围不在节点范围
		if (!node || !frustum.isCubeIn(node->BoundingBox))
		{
			return;
		}
		
		// 指定范围与给定节点相交，而且当前节点为叶子节点
		if (frustum.isCubeIn(node->BoundingBox) && IsQuadLeaf(node) && node->IsValid == true)
		{ 
			// 将索引赋给搜索结果
			for (u32 i = 0; i < node->TrigItems.size(); i += 3)
			{
				// 取三角形的三个顶点		
				core::vector3df point0(m_pts[node->TrigItems[i]].Pos.X, m_pts[node->TrigItems[i]].Pos.Y, m_pts[node->TrigItems[i]].Pos.Z);
				core::vector3df point1(m_pts[node->TrigItems[i + 1]].Pos.X, m_pts[node->TrigItems[i + 1]].Pos.Y, m_pts[node->TrigItems[i + 1]].Pos.Z);
				core::vector3df point2(m_pts[node->TrigItems[i + 2]].Pos.X, m_pts[node->TrigItems[i + 2]].Pos.Y, m_pts[node->TrigItems[i + 2]].Pos.Z);
				// 获取三角形的范围
				core::aabbox3df Triabox(point0);
				Triabox.addInternalPoint(point1);
				Triabox.addInternalPoint(point2);

				// 当前三角形与指定范围相交
				if (frustum.isCubeIn(Triabox))
				{
					// 存进返回数组
					items.push_back(node->TrigItems[i]);
					items.push_back(node->TrigItems[i + 1]);
					items.push_back(node->TrigItems[i + 2]);
				}	
			}
			return;
		}

		// 遍历子节点
		for (int i = 0; i < 4; i++)
		{
			// 判空
			if (node->Children[i])
			{
				// 递归调用
				SearchQuadTree(node->Children[i], frustum, items);
			}
		}
	}

	// 查找相邻三角形，传入指定三角形索引，传出相邻的三角形
	void CHdQuadSourceIndex::SearchAdjionTria(QuadNode* node, u32 index0, u32 index1, u32 index2, vector<u32>& items)
	{
		if (!node)
		{
			return;
		}

		// 将索引赋给搜索结果
		for (u32 i = 0; i < node->TrigItems.size(); i += 3)
		{
			// 找出与指定三角形共一点的三角形
			if (node->TrigItems[i] == index0 || node->TrigItems[i] == index1 || node->TrigItems[i] == index2 ||
				node->TrigItems[i+1] == index0 || node->TrigItems[i+1] == index1 || node->TrigItems[i+1] == index2 ||
				node->TrigItems[i+2] == index0 || node->TrigItems[i+2] == index1 || node->TrigItems[i+2] == index2 )
			{
				// 不包括自己
				if (node->TrigItems[i] != index0 && node->TrigItems[i+1] != index1 && node->TrigItems[i+2] != index2)
				{
					items.push_back(node->TrigItems[i]);
					items.push_back(node->TrigItems[i+1]);
					items.push_back(node->TrigItems[i+2]);
				}
			}
		}
		

		// 遍历子节点
		for (int i = 0; i < 4; i++)
		{
			// 判空
			if (node->Children[i])
			{
				// 递归调用
				SearchAdjionTria(node->Children[i], index0, index1, index2, items);
			}
		}
	}

	// 删除指定的多个三角形
	bool CHdQuadSourceIndex::DeleteQuadTree(vector<u32>& items)
	{
		// 判空
		if (items.size() < 3)
		{
			return false;
		}

		// 是否删除成功
		bool IsDel = false;

		// 遍历三角形
		for (int i = 0; i < items.size(); i += 3)
		{
			// 删除当前三角形
			IsDel = DeleteQuadTree(m_QuardRoot, items[i], items[i + 1], items[i + 2]);

			// 删除失败则
			if (!IsDel)
			{
				return false;
			}
		}
		return true;
	}

	// 查找指定三角形
	bool CHdQuadSourceIndex::DeleteQuadTree(QuadNode* node, u32 index0, u32 index1, u32 index2)
	{
		// 判空，指定范围大于节点范围
		if (!node || !m_IndexData)
		{
			return false;
		}

		// 取三角形的三个顶点
		core::vector3df point0(m_pts[index0].Pos.X, m_pts[index0].Pos.Y, m_pts[index0].Pos.Z);
		core::vector3df point1(m_pts[index1].Pos.X, m_pts[index1].Pos.Y, m_pts[index1].Pos.Z);
		core::vector3df point2(m_pts[index2].Pos.X, m_pts[index2].Pos.Y, m_pts[index2].Pos.Z);
		core::aabbox3df TriaBox(point0);
		TriaBox.addInternalPoint(point1);
		TriaBox.addInternalPoint(point2);

		// 当前节点为叶子节点，且三角形三个顶点都在当前该节点中，则删除
		if (IsQuadLeaf(node) && (TriaBox.isFullInside(node->BoundingBox) || TriaBox.intersectsWithBox(node->BoundingBox))  /*(
			(node->BoundingBox.isPointInside(point0) || node->BoundingBox.isPointInside(point1) || node->BoundingBox.isPointInside(point2))) */)
		{
			// 将索引赋给搜索结果
			for (u32 i = 0; i < node->TrigItems.size(); i += 3)
			{
				// 找到相应三角形进行删除
				if (node->TrigItems[i] == index0 && node->TrigItems[i + 1] == index1 && node->TrigItems[i + 2] == index2)
				{
					// 删除一个点就会少一个三角形
					node->TrigItems.erase(node->TrigItems.begin() + i);
					node->TrigItems.erase(node->TrigItems.begin() + i);
					node->TrigItems.erase(node->TrigItems.begin() + i);
					return true;
				}		
			}
		}

		// 遍历子节点
		for (int i = 0; i < 4; i++)
		{
			// 判空
			if (node->Children[i])
			{
				// 递归搜索
				DeleteQuadTree(node->Children[i], index0, index1, index2);
			}
		}

		return false;
	}

	// 删除指定三角形-根据范围删除
	void CHdQuadSourceIndex::DeleteQuadTree(core::aabbox3df SelBox)
	{

	}

	// 删除制定三角形-根据指定点所在三角形
	void CHdQuadSourceIndex::DeleteQuadTree(u32 index)
	{

	}

	// 是否为叶子节点
	bool CHdQuadSourceIndex::IsQuadLeaf(QuadNode* node)
	{
		// 节点为空
		if (!node)
		{
			return false;
		}

		// 没有孩子节点则为叶子节点
		if (node->Children[0] == NULL && node->Children[1] == NULL && node->Children[2] == NULL && node->Children[3] == NULL)
		{
			return true;
		}

		return false;
	}

	// 根据三角形索引，将三角形插入相应的叶子节点中
	void CHdQuadSourceIndex::InsertQuad(int index, core::aabbox3df Box, QuadNode* node)
	{
		// 判空 当前节点与指定范围相离，返回
		if (!node || (!Box.isFullInside(node->BoundingBox) && !Box.intersectsWithBox(node->BoundingBox)))
		{
			return;
		}

		// 当前节点有效
		node->IsValid = true;

		// 获取三角形范围的中心点
		core::vector3df TriagCenter = Box.getCenter();

		// 根据距离判断三角形所属空间
		double Min_Dis = F64_MAX;					

		// 非叶子节点
		if (!IsQuadLeaf(node))
		{
			// 被哪个子节点完全包含的索引号
			int iIndex = -1;		

			// 遍历四个节点
			int i = 0;
			while (i < 4)
			{
				if (Box.isFullInside(node->Children[i]->BoundingBox))
				{
					// 当前三角形完全包含于该子节点
					iIndex = i;
					break;
				}
				else if (Box.intersectsWithBox(node->Children[i]->BoundingBox))
				{
					// 当前三角形与该子节点相交，求该节点范围中心到三角形中心的距离
					core::vector3df NodeCenter = node->Children[i]->BoundingBox.getCenter();

					// 获取最短距离，并记录对应索引
					double dis = TriagCenter.getDistanceFrom(NodeCenter);
					if (Min_Dis > dis)
					{
						Min_Dis = dis;
						iIndex = i;
					}
				}
				i++;
			}

			// 如果找到最近的子节点
			if (iIndex != -1)
			{
				// 递归插入子节点
				InsertQuad(index, Box, node->Children[iIndex]);
			}
		}

		// 叶子节点，则直接放进去
		else 
		{
			// 将三角形的点索引插入到三角形
			node->TrigItems.push_back(m_IndexData[index]);
			node->TrigItems.push_back(m_IndexData[index + 1]);
			node->TrigItems.push_back(m_IndexData[index + 2]);
		}
	}

	// 删除空节点
	void CHdQuadSourceIndex::DeleteNullNode(QuadNode* node)
	{
		// 判空
		if (!node)
		{
			return;
		}

		// 有效子节点数为0时，删除
		if (node->IsValid && node->Children[0]->IsValid == false &&node->Children[1]->IsValid == false 
			&& node->Children[2]->IsValid == false && node->Children[3]->IsValid == false)
		{
			// 释放死叉树
			ReleseQuadTree(&node);
		}
		else
		{
			// 遍历子节点
			for (int i = 0; i < 4; i++)
			{
				// 判空
				if (node->Children[i])
				{
					// 递归删除空的节点
					DeleteNullNode(node->Children[i]);
				}
			}
		}
	}

	// 遍历所有节点
	void CHdQuadSourceIndex::TraversalQuadTree()
	{

	}

	// 重新构建四叉树
	void CHdQuadSourceIndex::RebulidQuadTree(core::aabbox3df Box)
	{

	}

	// 删除三角形后更新三角形索引
	void CHdQuadSourceIndex::UpdateTrianlgeIndex()
	{

	}

	// 删除三角形后更新点
	void CHdQuadSourceIndex::UpdatePts()
	{

	}

	// 释放树的空间
	void CHdQuadSourceIndex::ReleseQuadTree(QuadNode** node)
	{
		QuadNode* pNode = *node;
		// 判空
		if (pNode == NULL)
		{
			return;
		}
		else
		{
			// 节点不为空
			for (int i  = 0; i < 4; i++)
			{
				// 递归释放
				ReleseQuadTree(&pNode->Children[i]);
			}

			// 删除节点
			if (pNode)
			{
				delete pNode;
				pNode = NULL;
			}
		}

		// 赋空
		pNode = NULL;
	}
}


