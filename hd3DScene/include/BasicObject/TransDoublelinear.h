/*! HdTranslate.h
********************************************************************************
<PRE>
模块名       : HdBLEncryptTraslate
文件名       : HdTranslate.h
相关文件     : HdTranslate.cpp
文件实现功能 : 双线性变换
			// x = a1 * x1 + b1 * y1 + c1 * x1 * y1 + d1
			// y = a2 * x1 + b2 * y1 + c2 * x1 * y1 + d2
			// 四对坐标，对应八个方程，解得8个未知数a1,a2，b1，b2，c1，c2，d1，d2
			// 8个未知数成为转换系数

作者         : 研发部 冯晶
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2015/03/09   1.0        冯晶                 新建
</PRE>
*******************************************************************************/

#pragma once
#include "hdBasicObject.h"

class BASICOBJECT_API CTransDoubleLinear
{
public:
	// 构造函数
	CTransDoubleLinear(void);

	// 析构函数
	~CTransDoubleLinear(void);

public:
	// 源坐标系转换成目标坐标系
	bool Translate(double dx_in, double dy_in,   // 源坐标
				   double& dx_out, double& dy_out);// 目标坐标

	// 设置控制点对
	bool SetParms(double* PX,double* PY,		 // 源坐标系控制点
				  double* dPX,double* dPY);		 // 目标坐标系控制点

	// 返回8个转换系数,
	double* GetTransParms() { return m_solArray;}

private:
	// 线性方程的系数矩阵,源坐标系的4个控制点
	double m_a[64];

	// 线性方程的常数矩阵,目标坐标系的4个控制点
	double m_b[8];

	// 8个未知系数
	double m_solArray[8];
};

