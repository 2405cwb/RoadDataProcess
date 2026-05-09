#pragma once
#include<QString>
#include "..\hnApplication\hnProEnum.h"
// 里程桩数据
typedef  struct _HN_MILE_
{
	_HN_MILE_()
	{

		nDMi = 0;
		dEnclMile = 0.0;
		dGpsTimer = 0.0;
		dTrueMile = 0.0;
	}


	// 编码脉冲值
	long long nDMi;

	// 编码器里程
	double dEnclMile;

	// gps时间
	double dGpsTimer;

	// 真实里程
	double dTrueMile;

	// 道路图像路径
	QString picturePath;

	QString leftStreetPicPath;

	QString rightStreetPicPath;

	//绘制模式
	//0 大框 1 小框
	int drawType;

	//路面材质 
	//0沥青 1水泥  2砂石
	int roadType;

	//路面规范
	//等级公路2018  低等级农村路
	HnProEnums::StandardParmType roadStandard;

	//路面单元 
	//进路口  出路口   
	QString roadUnit;

	//路面等级 
	QString  roadGrad;

}hnMile;