#pragma once
#include <QString>
#include "hnqtcommon_global.h"

class HNQTCOMMON_EXPORT SynTrigInfo {
public:
	int _FrameIndex;
	QString _trigdate;
	QString _trigtime;
	double _trigdmi;

	SynTrigInfo() {}

	SynTrigInfo(const QString& frameIndex, const QString& dataDate, const QString& trigtime, const QString& trigdmi);

	SynTrigInfo(const SynTrigInfo& prev, const SynTrigInfo& next, double dmi);

	QString toString() const {
		return _trigtime + " " + QString::number( _trigdmi);
	}
};
