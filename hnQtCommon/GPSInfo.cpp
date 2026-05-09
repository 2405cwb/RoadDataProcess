#include "GPSInfo.h"

GPSInfo::GPSInfo()  
{
	this->_elevation = 0; 
	this->_GGACorrection = 0;
	this->_latitude = 0;
	this->_longitude = 0;
	this->_utctime =QDateTime();
}
GPSInfo::GPSInfo(const QString& infostr)
{
	int signCount = infostr.count('$');
	if (signCount != 1)
	{
		_IsOK = false;
		return;
	}

	// Ω‚ŒˆGPS–≈œ¢
	QStringList strs = infostr.split(',');
	if (strs[0].contains("GGA") && strs.size() == 15 && infostr.length() <= 90) {
		if (strs[1].length() == 9 && !strs[2].isEmpty() && !strs[4].isEmpty() && !strs[9].isEmpty() && !strs[11].isEmpty()) {
			try {
				int hour = strs[1].mid(0, 2).toInt();
				int minute = strs[1].mid(2, 2).toInt();
				int second = strs[1].mid(4, 2).toInt();
				int msSecond = strs[1].split('.').last().toInt();
				if (msSecond<=100)
				{
					msSecond = msSecond * 10;
				}
				QTime time(hour, minute, second, msSecond); 
				_utctime = QDateTime();
				_utctime.setTime(time);
			}
			catch (...) {
				_IsOK = false;
				return;
			}
			if (!convertDegreesToDigital(strs[2], 2, _latitude)) {
				_IsOK = false;
				return;
			}
			if (!convertDegreesToDigital(strs[4], 3, _longitude)) {
				_IsOK = false;
				return;
			}
			if (_latitude < 3.86 || _latitude > 53.55 || _longitude < 73.66 || _longitude > 135.05) {
				_IsOK = false;
				return;
			}
			try {
				_elevation = strs[9].toDouble();
				_GGACorrection = strs[11].toDouble();
				if (strs[10] != "M") {
					_IsOK = false;
				}
				else {
					_IsOK = true;
				}
			}
			catch (...) {
				_IsOK = false;
			}
		}
		else {
			_IsOK = false;
		}
	}
	else if (strs[0].contains("FPD") && strs.size() > 8) {
		if (!strs[2].isEmpty() && !strs[6].isEmpty() && !strs[7].isEmpty() && !strs[8].isEmpty()) {
			double weeksecond = (strs[2].toDouble() - 18) * 1000;
			double second = fmod(weeksecond, 86400000);
			QTime baseTime = QTime::fromString("000000000", "HHmmsszzz");
			if (!baseTime.isValid())
			{
				return;
			}
			QTime timeDiff = QTime(0, 0).addMSecs(second);
			QDate currentDate = QDate::currentDate();
			_utctime = QDateTime(currentDate, baseTime.addMSecs(second)); 
			_latitude = strs[6].toDouble();
			_longitude = strs[7].toDouble();
			_elevation = strs[8].toDouble() - _GGACorrection;
			_IsOK = true;
		}
	}
	else if (strs[0].contains("GPRMC") && strs.size() > 8) {
		if (strs[1].length() == 9 && !strs[3].isEmpty() && !strs[5].isEmpty() && strs[2] == "A") {
			try {
				int hour = strs[1].mid(0, 2).toInt();
				int minute = strs[1].mid(2, 2).toInt();
				int second = strs[1].mid(4, 2).toInt();
				int msSecond = strs[1].split('.').last().toInt();
				if (msSecond <= 100)
				{
					msSecond = msSecond * 10;
				}
				QTime time(hour, minute,second,msSecond);

				_utctime = QDateTime();
				_utctime.setTime(time); 
			}
			catch (...) {
				_IsOK = false;
				return;
			}
			if (!convertDegreesToDigital(strs[3], 2, _latitude)) {
				_IsOK = false;
				return;
			}
			if (!convertDegreesToDigital(strs[5], 3, _longitude)) {
				_IsOK = false;
				return;
			}
			if (_latitude < 3.86 || _latitude > 53.55 || _longitude < 73.66 || _longitude > 135.05) {
				_IsOK = false;
				return;
			}
			_elevation = 0;
			_IsOK = true;
		}
		else {
			_IsOK = false;
		}
	}
	else {
		_IsOK = false;
	}
}

GPSInfo::GPSInfo(const GPSInfo& SInfo, const GPSInfo& EInfo, const QDateTime& insertime) {

	QString sTimeStr = SInfo._utctime.time().toString("HHmmsszzz");
	QString eTimeStr = EInfo._utctime.time().toString("HHmmsszzz");
	QString iTimeStr = insertime.time().toString("HHmmsszzz");
	qint64 es_date = EInfo._utctime .time(). msecsTo(SInfo._utctime.time());
	qint64 cs_date = insertime.time().msecsTo(SInfo._utctime.time());
	double k = static_cast<double>(cs_date) / es_date;

	_utctime = insertime;
	_latitude = k * (EInfo._latitude - SInfo._latitude) + SInfo._latitude;
	_longitude = k * (EInfo._longitude - SInfo._longitude) + SInfo._longitude;
	_elevation = k * (EInfo._elevation - SInfo._elevation) + SInfo._elevation;
	_IsOK = true;
}



bool GPSInfo::convertDegreesToDigital(const QString& inval, int sidx, double& outval)
{
	bool res = false;
	outval = 0;
	if (inval.isEmpty()) {
		return res;
	}
	try {
		int du = inval.mid(0, sidx).toInt();
		outval = inval.mid(sidx).toDouble() / 60;
		outval += du;
		res = true;
	}
	catch (...) {
	}
	return res;
}

