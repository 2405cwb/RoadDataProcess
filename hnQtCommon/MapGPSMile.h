#pragma once
#include "GPSInfo.h"
#include "hnqtcommon_global.h" 
class HNQTCOMMON_EXPORT MapGPSMile
{
public:
	GPSInfo _gpsinfo;
	// Àï³Ì
	float _dmi;
	// ×®ºÅ
	int _mile;
	MapGPSMile();

	MapGPSMile(QString txt);

	QString time;

	operator QString() const {

		return QString("%1 %2 %3")
			.arg(_gpsinfo)
			.arg(_dmi)
			.arg(_mile);
	}
};

