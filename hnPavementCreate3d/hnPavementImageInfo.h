#ifndef HN3DPAVEMENT_IMAGE_INFO_H
#define HN3DPAVEMENT_IMAGE_INFO_H
#include <stdio.h>
#include <string>
#include <QString>
#include <vector>
#include "..\hnCommon\hn2dPointDef.h"
#include "..\hnCommon\hnCommonDef.h"
using namespace hnCommon;
using namespace std;

struct PAVEMENT_IMAGE_INDEX
{
	PAVEMENT_IMAGE_INDEX()
	{
		memset(strImgName, 0, 128);
		dStartTime = dEndTime = 0.0;

		nImgWidth = 4000;
		nImgHeight = 2560;

		dStartDmi = dEndDmi = 0.0;
		minX = minY = -1000000000.0;
		maxX = maxY = 1000000000.0;
	}

	bool serialize(const char* strData)
	{
		int nSize = sscanf_s(strData, "%s	%lf	%lf	%d	%d	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf	%lf\n",
			strImgName, 128, &dStartTime, &dEndTime, &nImgWidth, &nImgHeight, &dStartDmi, &dEndDmi,
			&upLeftPt.x, &upLeftPt.y, &upRightPt.x, &upRightPt.y,
			&downLeftPt.x, &downLeftPt.y, &downRightPt.x, &downRightPt.y);
		if (nSize < 15)
		{
			return false;
		}

		minX = maxX = upLeftPt.x;
		minY = maxY = upLeftPt.y;

		// minX
		if (upRightPt.x < minX)
		{
			minX = upRightPt.x;
		}
		if (downLeftPt.x < minX)
		{
			minX = downLeftPt.x;
		}
		if (downRightPt.x < minX)
		{
			minX = downRightPt.x;
		}

		// minY
		if (upRightPt.y < minY)
		{
			minY = upRightPt.y;
		}
		if (downLeftPt.y < minY)
		{
			minY = downLeftPt.y;
		}
		if (downRightPt.y < minY)
		{
			minY = downRightPt.y;
		}

		// maxX
		if (upRightPt.x > maxX)
		{
			maxX = upRightPt.x;
		}
		if (downLeftPt.x > maxX)
		{
			maxX = downLeftPt.x;
		}
		if (downRightPt.x > maxX)
		{
			maxX = downRightPt.x;
		}

		// maxY
		if (upRightPt.y > maxY)
		{
			maxY = upRightPt.y;
		}
		if (downLeftPt.y > maxY)
		{
			maxY = downLeftPt.y;
		}
		if (downRightPt.y > maxY)
		{
			maxY = downRightPt.y;
		}

		return true;
	}

	void reserialize(char** strOutput, int count)
	{
		int nSize = sprintf_s(*strOutput, count, "%s	%.6lf	%.6lf	%d	%d	%.3lf	%.3lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf\n",
			strImgName, dStartTime, dEndTime, nImgWidth, nImgHeight, dStartDmi, dEndDmi, 
			upLeftPt.x, upLeftPt.y, upRightPt.x, upRightPt.y, downLeftPt.x, downLeftPt.y, downRightPt.x, downRightPt.y);
	}

	// 灰度影像名;
	char strImgName[128];

	// 影像起始时间(相对于行驶时间),周内秒，后续同步其他模块可能用到;
	double dStartTime;

	// 影像终止时间;
	double dEndTime;

	// 影像长宽;
	int nImgWidth;
	int nImgHeight;

	// 影像代表起始、终止里程，与影像名对应;
	double dStartDmi;
	double dEndDmi;

	// 像素中心线起始平面坐标,其他任意点像素X坐标对应平面坐标为该点到中心线投影距离与比例尺的关系，像素Y坐标为点到直线的投影点与起始点的距离值;
	// 像素四角点坐标,左上;
	hn2dPointD upLeftPt;

	// 右上;
	hn2dPointD upRightPt;

	// 左下;
	hn2dPointD downLeftPt;

	// 右下;
	hn2dPointD downRightPt;

	// 平面坐标范围记录，不写入文件，在读取解析时更新;
	double minX;
	double minY;
	double maxX;
	double maxY;
};

// 路面影像信息
struct ROAD_IMAGE_INDEX
{
	ROAD_IMAGE_INDEX()
	{
		memset(strName1, 0, 128);
		memset(strName2, 0, 128);
		memset(strName3, 0, 128);
		memset(strName4, 0, 128);
		memset(strName5, 0, 128);
		memset(strName6, 0, 128);
		memset(strName7, 0, 128);
		nImageIndex = 0;
		dGpsTimer = 0.0;
	}

	// 分割字符
	bool sectionChar(char* strResouce, const char* strSec, std::vector<string>& vecOutStr)
	{
		// 解析数据
		vecOutStr.clear();

		char *p;
		p = strtok(strResouce, strSec);

		string strOut = "";
		while (p)
		{
			strOut = p;
			vecOutStr.push_back(strOut);
			p = strtok(NULL, strSec);
		}

		if (vecOutStr.size() <= 0)
		{
			return false;
		}

		return true;
	}

	bool serialize(const char* strData, int year, int month, int day)
	{
		// 解析后的字符
		std::vector<string> vecOutStr;

		if (!sectionChar(const_cast<char*>(strData), ",", vecOutStr))
		{
			return false;
		}

		if (vecOutStr.size() != 7)
		{
			return false;
		}

		// 16进制转10进制
		this->nImageIndex = from16To10(vecOutStr[1].c_str());

		// 转gps时间
		this->dGpsTimer = paraTimer(vecOutStr[2].c_str(), year, month, day);

		return true;
	}

	// 16进制转10进制
	int from16To10(const char* strIn)
	{
		std::string strOri = "";
		std::string strNew = "";
		std::string strInS = strIn;
		int nSum = 0;

		int nT = 0;
		int nTemp = 0;
		for (int i = strInS.length() - 1; i >= 0; i--)
		{
			strOri = strInS[i];
			nT = strInS.length() - 1 - i;
			if (strOri == "A")
			{
				nSum += 10 * pow(16.0, nT);
			}
			else if (strOri == "B")
			{
				nSum += 11 * pow(16.0, nT);
			}
			else if (strOri == "C")
			{
				nSum += 12 * pow(16.0, nT);
			}
			else if (strOri == "D")
			{
				nSum += 13 * pow(16.0, nT);
			}
			else if (strOri == "E")
			{
				nSum += 14 * pow(16.0, nT);
			}
			else if (strOri == "F")
			{
				nSum += 15 * pow(16.0, nT);
			}
			else
			{
				nTemp = atoi(strOri.c_str());
				nSum += nTemp*pow(16.0, nT);
			}
		}

		return nSum;
	}

	//时间转换
	bool utcToGPS(const DATE_TIME_INFO& stTime, int& nGpsWeek, double& dGpsSeconds, double dGPSSubUTC/*= 0.*/)
	{
		int dayofw(0), dayofy(0), yr(0), ttlday(0), m(0), weekno(0);
		const  int  dinmth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

		//  Convert day, month and year to day of year 
		if (stTime.month == 1)
		{
			dayofy = stTime.day;
		}
		else
		{
			dayofy = 0;
			for (m = 1; m <= (stTime.month - 1); m++)
			{
				dayofy += dinmth[m];
				if (m == 2)
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
		for (yr = 1981; yr <= (stTime.year - 1); yr++)
		{
			ttlday += 365;
			if (yr % 4 == 0 && yr % 100 != 0 || yr % 400 == 0)
			{
				ttlday += 1;
			}
		}
		ttlday += dayofy;
		weekno = ttlday / 7;
		dayofw = ttlday - 7 * weekno;

		nGpsWeek = weekno;
		dGpsSeconds = dayofw * 86400.0 + stTime.hour * 3600.0 + stTime.minute * 60.0 + stTime.second + stTime.milliseconds / 1000000.;

		dGpsSeconds += dGPSSubUTC;
		if (dGpsSeconds > 7 * 24 * 3600)
		{
			dGpsSeconds -= 7 * 24 * 3600;
			nGpsWeek += 1;
		}

		return true;
	}

	// 解析时间
	double paraTimer(const char* strIn, int year, int month, int day)
	{
		DATE_TIME_INFO nTimer;
		nTimer.year = year;
		nTimer.month = month;
		nTimer.day = day;
		sscanf_s(strIn, "%02u%02u%02u%03u", &nTimer.hour, &nTimer.minute, &nTimer.second, &nTimer.milliseconds);

		// utc 转 gps
		int nGpsWeek = 0.0;
		double dGpsSecond = 0.0;
		utcToGPS(nTimer, nGpsWeek, dGpsSecond, 18.0);

		return dGpsSecond;
	}

	char strName1[128];
	char strName2[128];
	char strName3[128];
	char strName4[128];
	char strName5[128];
	char strName6[128];
	char strName7[128];

	// 编号
	int nImageIndex;

	// gps时间
	double dGpsTimer;
};

#endif
