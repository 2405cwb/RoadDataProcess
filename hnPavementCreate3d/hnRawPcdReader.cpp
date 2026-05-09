#include "hnRawPcdReader.h"


hnRawPcdReader::hnRawPcdReader(void)
{
	m_utc_to_gps_abs = 18.0;
	m_offset_time = 0.0;
	m_vec_combine_time_range.clear();
	m_scan_type = ENUM_RAW_SCAN_ZFS;
	m_need_export_single_line = 0;
	m_export_single_index = 0;
}


hnRawPcdReader::~hnRawPcdReader(void)
{
}

void hnRawPcdReader::setScanTime( DATE_TIME_INFO& date_info )
{
	if (m_scan_date_time.year == 0)
	{
		m_scan_date_time = date_info;
	}
}

void hnRawPcdReader::setUtcToGpsAbs( double second )
{
	m_utc_to_gps_abs = second;
}

void hnRawPcdReader::setOffsetTime( double offsetTime )
{
	m_offset_time = offsetTime;
}

void hnRawPcdReader::setTimeRange( std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range )
{
	m_vec_combine_time_range = vec_combine_time_range;
}

bool hnRawPcdReader::UTCT2GPST( const DATE_TIME_INFO& stTime,int& nGpsWeek,double& dGpsSeconds,double dGPSSubUTC/*= 0.*/ )
{
	int dayofw(0),dayofy(0), yr(0), ttlday(0), m(0), weekno(0);
	const  int  dinmth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	//  Convert day, month and year to day of year 
	if (stTime.month == 1)
	{
		dayofy = stTime.day;
	}
	else
	{
		dayofy = 0;
		for (m=1; m<=(stTime.month-1); m++)
		{
			dayofy += dinmth[m];
			if ( m==2 )
			{
				if (stTime.year % 4 == 0 && stTime.year % 100 != 0 || stTime.year % 400 == 0) 
				{
					dayofy += 1;
				}
			}
		}
		dayofy += stTime.day;
	}
	//  Convert day of year and year into week number and day of week 
	ttlday = 360;
	for (yr=1981; yr<=(stTime.year-1); yr++)
	{
		ttlday  += 365;
		if (yr % 4 == 0 && yr % 100 != 0 || yr % 400 ==0)
		{
			ttlday  += 1;
		}
	}
	ttlday += dayofy;
	weekno  = ttlday/7;
	dayofw  = ttlday - 7 * weekno;

	nGpsWeek    = weekno; 
	dGpsSeconds = dayofw * 86400.0 + stTime.hour * 3600.0 + stTime.minute * 60.0 + stTime.second +stTime.milliseconds/1000000.;

	dGpsSeconds += dGPSSubUTC;
	if(dGpsSeconds > 7*24*3600)
	{
		dGpsSeconds -= 7*24*3600;
		nGpsWeek    += 1;
	}

	return true;
}

void hnRawPcdReader::setScanType( int scanType )
{
	m_scan_type = (ENUM_RAW_SCAN_TYPE)scanType;
}

void hnRawPcdReader::setGpsSynInfo( SYN_UPDATE_STRUCT& synInfo )
{
	m_syn_update_info = synInfo;
}

void hnRawPcdReader::setExportSingleLine( int needSingle,int singleIndex )
{
	m_need_export_single_line = needSingle;
	m_export_single_index = singleIndex;
}

void hnRawPcdReader::getLineIndexTime( int curIndex,double& gpsTime )
{
	gpsTime = -1;
	return;
}

bool hnRawPcdReader::getRoadLinesPoints( int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	return false;
}

void hnRawPcdReader::setUseAngleFilter( int useAngleFilter,double angleStart,double angleEnd )
{
	m_use_angle_filter = useAngleFilter;
	m_angle_filter_start = angleStart;
	m_angle_filter_end = angleEnd;
}
