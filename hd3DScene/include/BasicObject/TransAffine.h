/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：TransAffine.h
相关文件	: BasicStruct.h
文件实现功能：定义放射变换坐标类
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/23	1.0			马振明		移植实现
</PRE>
******************************************************************************************************/

#pragma once
#include "Trans.h"
#include "hdMatrix.h"
// 仿射变换公式
// X = a1*x+b1*y+c1;
// Y = a2*x+b2*y+c2;
// 转换成矩阵格式
// X	a1 b1 c1	x
// Y  = a2 b2 c2	y
// 1	0  0   1	1
// 可简化为Y = AX
template <class T>
class BASICOBJECT_API CTransAffine :
	public CTrans<T>
{
public:
	CTransAffine(void);
	virtual ~CTransAffine(void);

	// 根据点初始化转换参数
	virtual bool Initialize(POINT2D<T>* pSrcPt, POINT2D<T>* pDestPt, int nPtNum);

	// 转换坐标
	virtual bool Trans(POINT2D<T>& ptInput, POINT2D<T>& ptOutput);

	// 批量转换
	virtual bool Trans(POINT2D<T>* pPtInput, POINT2D<T>* pPtOutput, int nPtNum);

	// 转换坐标
	virtual bool Trans(T srcX,T srcY, T& dstX, T& dstY);

	// 初始化转换参数
	virtual bool UpdateParam(double* dParams);
	
protected:
	hdMatrix	m_transMatrix;					// 转换矩阵
};

