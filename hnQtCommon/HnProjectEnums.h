#pragma once
#include <QString>
namespace HnProjectEnums
{
	//路面标准枚举
	enum StandardParmTypeEnum
	{
		//空
		None,

		/// <summary>
		/// 1--等级公路2018
		/// </summary>
		DegreeRoad2018,

		/// <summary>
		/// 2--城镇道路
		/// </summary>
		CityRoad,

		/// <summary>
		/// 3-低等级农村公路
		/// </summary>
		RuralRoadlowLevel

	};
	//roadTypeQStringToEnum
	static StandardParmTypeEnum roadTypeQStringToEnum(const QString& roadTypeStr)
	{
		StandardParmTypeEnum enumValue = StandardParmTypeEnum::DegreeRoad2018;
		if (roadTypeStr==QStringLiteral("等级公路2018")||roadTypeStr == QStringLiteral("等级公路 JTG H20-2018"))
		{
			enumValue = StandardParmTypeEnum::DegreeRoad2018;
		}
		else if (roadTypeStr == QStringLiteral("城镇道路"))
		{
			enumValue = StandardParmTypeEnum::CityRoad;
		}
		else if (roadTypeStr == QStringLiteral("低等级农村公路"))
		{
			enumValue = StandardParmTypeEnum::RuralRoadlowLevel;
		}
		else
		{
			qWarning("输入了不存在的模块类型");
		}
		return enumValue;
	}
	static StandardParmTypeEnum roadTypeQStringToEnum(const char* roadTypeChar)
	{
		QString roadTypeStr = QString::fromLocal8Bit(roadTypeChar);
		StandardParmTypeEnum enumValue = StandardParmTypeEnum::DegreeRoad2018;
		if (roadTypeStr == QStringLiteral("等级公路2018"))
		{
			enumValue = StandardParmTypeEnum::DegreeRoad2018;
		}
		else if (roadTypeStr == QStringLiteral("城镇道路"))
		{
			enumValue = StandardParmTypeEnum::CityRoad;
		}
		else if (roadTypeStr == QStringLiteral("低等级农村公路"))
		{
			enumValue = StandardParmTypeEnum::RuralRoadlowLevel;
		}
		else
		{
			qWarning("输入了不存在的模块类型");
		}
		return enumValue;
	}
	static QString roadTypeEnumToQString(const StandardParmTypeEnum& roadTypeEnum)
	{
		QString result;
		switch (roadTypeEnum)
		{
		case StandardParmTypeEnum::DegreeRoad2018:
			result = QString::fromLocal8Bit("等级公路2018");
			
			break;
		case StandardParmTypeEnum::CityRoad:
			result = QString::fromLocal8Bit("城镇道路");
			break;
		case StandardParmTypeEnum::RuralRoadlowLevel:
			result = QString::fromLocal8Bit("低等级农村公路");
			break;
		default:
			result = QString();
			break;
		}
		return result;
	}


	//给报表配置使用
	static QString roadTypeEnumToQString_ForExcel(const StandardParmTypeEnum& roadTypeEnum)
	{
		QString result;
		switch (roadTypeEnum)
		{
		case StandardParmTypeEnum::DegreeRoad2018:
			//result = QString::fromLocal8Bit("等级公路2018");
			result = QString::fromLocal8Bit("等级公路 JTG H20-2018");
			break;
		case StandardParmTypeEnum::CityRoad:
			result = QString::fromLocal8Bit("城镇道路");
			break;
		case StandardParmTypeEnum::RuralRoadlowLevel:
			result = QString::fromLocal8Bit("低等级农村公路");
			break;
		default:
			result = QString();
			break;
		}
		return result;
	}
	//设备名称枚举  IRI 平整度 ,RUT 车辙,ROAD 路面,STREET 景观
	enum EquipMentEnum
	{
		IRI,
		RUT,
		ROAD,
		STREET
	};
}
