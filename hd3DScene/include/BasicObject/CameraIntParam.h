/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：CameraIntParam.h
相关文件	: 
文件实现功能：相机内参类
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/28	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once
#include "hdBasicObject.h"


class BASICOBJECT_API CCameraIntParam :
	public CHdBasicObject
{
public:
	CCameraIntParam(void);
	virtual ~CCameraIntParam(void);

	// 设置相机参数
	void SetCamera(double dcx,double dcy,double dfx,double dfy);

	// 设置畸变参数
	void SetDistCoeffs(const double* dDisCoeffs);

	// 获取相机参数
	const double* GetCamera() const
	{
		return m_dCamera;
	}

	//// 获取相机参数，可左值修改
	//double* GetCamera()
	//{
	//	return m_dCamera;
	//}

	// 获取畸变参数
	const double* GetDistCoeff() const
	{
		return m_dDistCoeffs;
	}

	//// 获取畸变参数，可左值修改
	//double* GetDistCoeff()
	//{
	//	return m_dDistCoeffs;
	//}
private:

	// 相机参数，顺序为cx,cy,fx,fy;
	// 分别为像主点在成像仪中心x，y方向上的偏移量
	// 焦距fx，fy;长度，某些成像仪的是矩形，而非正方形的
	double m_dCamera[4];

	// 畸变参数,最多有8个值，一般默认为4个或5个
	// 顺序参考opencv，为k1,k2,p1,p2[,k3[,k4,k5,k6]],
	// 其中k为透镜畸变，(透镜引起的变形)对于畸变较大的，可取到k3,甚至k6
	// p1,p2为径向畸变（透镜不完全平行图像平面导致）
	double m_dDistCoeffs[8];					

};

