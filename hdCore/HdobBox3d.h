/*! @CHdobBox3d.h
********************************************************************************
<PRE>
模块名       : hdCore
文件名       : HdobBox3d.h
相关文件     : HdobBox3d.cpp
文件实现功能 : 实现三维坐标下obb包围盒，主要实现obb box相交检测功能
检测相交说明 : obbox即方向包围盒，其检测两个方向box相交采用算法思路是基于分离轴理论，
               两个box不相交，则必定存在某一个面将其分隔开，两个box的八个顶点在该分隔面
			   的法向量上的投影（得到相对于原点的距离），两个box的距离范围必定不相交，
			   寻找该分隔面，即寻找该面的法向量，存在十五种可能：
			   1、两个box三个面法向量（3*2个）
			   2、两个box顶点边长所形成的向量 （3*3个）
作者         : 朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/06/04   1.0      朱旭波              创建
</PRE>
*******************************************************************************/

#pragma once
#include "hdCore.h"
#include "hdVector3d.h"
#include <vector>
using namespace std;
using namespace hd;
namespace hd
{
	class HDCORE_API CHdobBox3d
	{
	public:
		// 默认构造
		CHdobBox3d(void);

		// vertex[24]，对应8个角点
		CHdobBox3d(f32* vertex);

		// 析构
		~CHdobBox3d(void);
		public:
		// 定义obb box的八个顶点
		hd::CHdVector3df m_vertex[8];

	public:
		// 内联，计算点在该轴上的投影值（不一定是XYZ轴）
		inline float Dot(CHdVector3df axis,CHdVector3df pt)
		{
			// 求dot
			return (float)axis.dotProduct(pt)/axis.dotProduct(axis);
		}

	public:
		// 判断本box与外部传入box是否相交,返回0表示不相交，返回1表示相交
		int BoxIntersect(CHdobBox3d* box);

		// 设置box的八个顶点坐标,传入参数为8*3的数组，用于外部设置传入
		void SetVertex(f32* vertex);

		//判断点是否在包围盒内
		bool isPointInside(CHdVector3df pt);

		//获取包围盒内的顶点
		void GetBoxVertex(vector<hd::CHdVector3df>& vecVertex);

	private:
		// 获取边界范围值,传入box，分隔面轴线值，通过引用返回该轴线方向最大最小值
		void GetInterval(CHdobBox3d* box,CHdVector3df axis,float &fMin,float &fMax);

		// 取得box的面的法向量,传入参数为顶点的索引值，范围为0-7
		CHdVector3df GetFaceDir(int indeID);

		// 取得边的方向值，传入参数为顶点索引值
		CHdVector3df GetEdgeDir(int indexID);

        //设置参数值
	    void SetValue();
       
	   CHdVector3df m_xAxis;    // 包围盒x轴方向单位矢量
	   CHdVector3df m_yAxis;    // 包围盒y轴方向单位矢量
	   CHdVector3df m_zAxis;    // 包围盒z轴方向单位矢量
	   CHdVector3df m_vecMax;  // 
	   CHdVector3df m_vecMin;

	};
}


