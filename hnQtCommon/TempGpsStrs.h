#pragma once
#include <QList>
#include <QDateTime>
#include "hnqtcommon_global.h"
class HNQTCOMMON_EXPORT TempGpsStrs {
public:
	QList<QString> gpsStrs;
	QDateTime oTime;
	QString ymd;
	QList<QDateTime> NeedSub1sList;

	TempGpsStrs() {}

	TempGpsStrs(const QList<QString>& gpsStrs, const QDateTime& oTime, const QString& ymd)
		: gpsStrs(gpsStrs), oTime(oTime), ymd(ymd) {
		// ≥ı ºªØ NeedSub1sList
	}
};