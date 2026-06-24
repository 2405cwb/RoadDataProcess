#pragma once
#include<QString>
#include "HnProjectEnums.h"
#include "../hnCommon/hnRoadTypeDef.h"
// 里程桩数据
typedef   struct _HN_MILE_
{
	_HN_MILE_()
	{
		nID = 0;
		picturePath = "";
		leftStreetPicPath="";
		rightStreetPicPath="";
		nDMi = 0;
		dEnclMile = 0.0;
		dGpsTimer = 0.0;
		dTrueMile = 0.0;
		roadGrad = -1;
		roadWidth = -1;  
	}
	
	operator QString()const
	{ 
		QString message;
		int roadType1 =static_cast<int>(roadType);
		message = QStringLiteral("_真实里程桩号:") + QString::number(dTrueMile) +QStringLiteral("\n") +
			QStringLiteral("编码脉冲值")+QString::number(nDMi)+QStringLiteral("\n")+
			QStringLiteral("_编码器里程") + QString::number(dEnclMile) + QStringLiteral("\n") +
			QStringLiteral("_gps时间") + QString::number(dGpsTimer) + QStringLiteral("\n") +
			QStringLiteral("_绘制模式") + QString::number(drawType) + QStringLiteral("\n") +
			QStringLiteral("_路面标准:") + HnProjectEnums::roadTypeEnumToQString(roadStandard)+QStringLiteral("\n") +
			 QStringLiteral("_道路材质:") + QString::number(roadType1)+QStringLiteral("\n") +
			QStringLiteral("_道路等级:") + roadGradStr+QStringLiteral("\n")+
			QStringLiteral("_路面单元") + roadUnitStr + QStringLiteral("\n") +
			QStringLiteral("_路面宽度") + QString::number(roadWidth) + QStringLiteral("\n") ;
		return message;
	}
	int GradStrToGrad(QString gradStr)
	{
		if (gradStr.contains(QStringLiteral( "高速"))|| gradStr.contains(QStringLiteral("快速路")))
		{
			return 0;
		}
		if (gradStr.contains(QStringLiteral("一级"))||gradStr.contains(QStringLiteral("主干路")))
		{
			return 1;
		}
		if (gradStr.contains(QStringLiteral("二级")) || gradStr.contains(QStringLiteral("次干路")))
		{
			return 2;
		}
		if (gradStr.contains(QStringLiteral("三级")) || gradStr.contains(QStringLiteral("支路")))
		{
			return 3;
		}
		if (gradStr.contains(QStringLiteral("四级")))
		{
			return 4;
		}
	}
	bool  operator <(const _HN_MILE_& other)const {
		return dEnclMile < other.dEnclMile;
	}
	_HN_MILE_(const _HN_MILE_& other)
	{
		this->nID = other.nID;
		this->nDMi = other.nDMi;
		this->dEnclMile = other.dEnclMile;
		this->dGpsTimer = other.dGpsTimer;
		this->dTrueMile = other.dTrueMile;
		this->picturePath = QString(other.picturePath);
		this->leftStreetPicPath = QString(other.leftStreetPicPath);
		this->rightStreetPicPath = QString(other.rightStreetPicPath);
		this->drawType = other.drawType;
		this->roadWidth = other.roadWidth;
		this->roadType = other.roadType;
		this->roadGrad = other.roadGrad;
		this->roadStandard = other.roadStandard;
		this->roadUnitStr = QString(other.roadUnitStr);
		this->roadGradStr = QString(other.roadGradStr);
	}
	int nID;
	// 编码脉冲值
	long long nDMi;
	 
	
	// 编码器里程
	double dEnclMile;

	// gps时间
	double dGpsTimer;

	// 唯一桩号
	double dTrueMile;

	// 道路图像完整绝对路径
	QString picturePath;

	//左景观完整绝对路径
	QString leftStreetPicPath;

	//右景观完整绝对路径
	QString rightStreetPicPath;

	//绘制模式
	//0 人工模式 1 自动化模式
	hnCommon::ROAD_WORK_TYPE drawType;

	//路面材质 
	//0沥青 1水泥  2砂石
	hnCommon::ROAD_SURFACE_TYPE roadType;

	//路面规范
	//等级公路2018  低等级农村路
	HnProjectEnums::StandardParmTypeEnum roadStandard;

	//路面单元 
	//进路口  出路口   
	QString roadUnitStr;

	//路面等级 
	QString  roadGradStr;
	/// 第i到i+1个里程区间内的公路等级，0-高速、一级，1-二三四级 或者 0-快速路，1-主干路次干路，2-支路
	int roadGrad;
	
	//道路宽度
	double roadWidth;
}hnMile;

