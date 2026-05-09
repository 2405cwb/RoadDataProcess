/*hdTin.h
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdTin.h
相关文件	: hdTin.cpp	
文件实现功能：Delaunay三角网构建类
作者		：龚书林
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/		1.0			龚书林
2013/02/28	2.0			危迟	   新增坐标点索引属性	
</PRE>
******************************************************************************************************/
#pragma once
#include "LtTriangulate.h"
#include "hdCommon.h"
#include <vector>
using namespace std;

namespace hd
{

	class HDCOMMON_API CHdTin
	{
	public:
		CHdTin(void);
		~CHdTin(void);

		//! 初始化三角网,num为点数
		bool initial(int num, bool convex_hull=false);
		//! 增加点
		void add(float* p ,int index);
		//! 增加点
		void add(float* p);
		//! 结束
		void finish();
		//! 获取tin大小
		int get_size();
		//! 获取三角网中包含点p的三角形 [2015/05/18 危迟]
		TINtriangle* locateTriangleIncludePoint(float* p);
		//! 获取三角网
		TINtriangle* get_triangle(int t);
		//! 导出ply模型
		void save(const char* savepath);

		//！导出obj文件 fengjing
		void saveObj(const char* savepath);

		//! 销毁TIN
		void destroy();

		//! 测试导出ply时增加外接圆判断
		bool judgeCircle(float x1,float x2,float x3,float y1,float y2,float y3,float z1,float z2,float z3);


		//! 获得所有三角网的端点坐标
		vector<float> get_triangle_vertices();
		
		void save2(const char* savepath2);
	};

}