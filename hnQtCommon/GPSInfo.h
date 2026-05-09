#pragma once
#include <QDateTime>
#include <QString>
#include "hnqtcommon_global.h"

class HNQTCOMMON_EXPORT GPSInfo
{

public:
	GPSInfo();

	GPSInfo( const QString& infostr);

	GPSInfo(const GPSInfo& SInfo, const GPSInfo& EInfo, const QDateTime& insertime);
 
public:
	operator QString()const {

		QString strTime = _utctime.time() .toString("HHmmsszzz");
		return QString("%1 %2 %3 %4")
			.arg(_utctime.time(). toString("HHmmsszzz"))
			.arg(_longitude, 0, 'f', 7)
			.arg(_latitude, 0, 'f', 7)
			.arg(_elevation, 0, 'f', 2);
	}
	double _elevation;
	double _latitude;
	double _longitude;
	QDateTime _utctime;
	bool _IsOK;
	 double _GGACorrection;
private:
	bool convertDegreesToDigital(const QString& inval, int sidx, double& outval);
};
