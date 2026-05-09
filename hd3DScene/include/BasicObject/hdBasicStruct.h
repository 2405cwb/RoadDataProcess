/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdBasicObject
文件名		：BasicStruct.h
相关文件	: 
文件实现功能：定义基础的结构体，如2D点，3D点
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/22	1.0			马振明		  移植
</PRE>
******************************************************************************************************/

#pragma once

// 2D点结构体
template <class T>
struct POINT2D
{
	T	x;
	T	y;
};

// 3D点结构体
template <class T>
struct POINT3D
{
	T	x;
	T	y;
	T	z;
};

// 定义基本模板点对象
typedef POINT2D<int>	Point2di;
typedef POINT2D<float>	Point2df;
typedef POINT2D<double>	Point2dd;

typedef POINT3D<int>	Point3di;
typedef POINT3D<float>	Point3df;
typedef POINT3D<double>	Point3dd;