#include "MapGPSMile.h"

MapGPSMile::MapGPSMile()
{
	this->_dmi = 0;
	this->_mile = 0;
}

MapGPSMile::MapGPSMile(QString txt)
{
 QStringList infos = 	txt.split(' ');
 if (infos.size()>5)
 {
	 GPSInfo info;
	 info._utctime = QDateTime::fromString(infos[0]);
	 info._longitude = infos[1].toDouble();
	 info._latitude = infos[2].toDouble();
	 info._elevation = infos[3].toDouble();
	 _gpsinfo = info;
	 _dmi = infos[4].toFloat();
	 _mile = infos[5].toInt();
 }


}
