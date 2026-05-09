#pragma once

#include <QString>
#include <QStringList>
struct _EXCELGPS_
{
private:
public:
	
	//unable
	_EXCELGPS_()
	{
		_utctime = "";
		_latitude = 0;
		_longitude = 0;
		_elevation = 0;
		_mile = -1;
		_dmi = -1;
	}
	_EXCELGPS_(QString info)
	{
		QStringList split = info.split(" ");

		if (split.size()==6)
		{
			_utctime = split[0];
			_longitude = QString("%1").arg(split[1].toDouble(), 0, 'f', 7).toDouble();
			_latitude = QString("%1").arg(split[2].toDouble(), 0, 'f', 7).toDouble();
			_elevation = QString("%1").arg(split[3].toDouble(), 0, 'f', 2).toDouble();
			_dmi = split[4].toDouble();
			_mile = split[5].toInt();
		}
		 
	}
	//utc时间
	QString _utctime;

	//维度
	double _latitude;

	//经度
	double _longitude;

	//高程
	double _elevation;

	//桩号
	int _mile;

	double _dmi;




};