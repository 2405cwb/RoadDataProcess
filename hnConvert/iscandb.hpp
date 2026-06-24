#pragma once
#include <QObject>
#include <QString>
#include "..\3rd\SQLite\include\sqlite3.h"
#include "hnDataCombineStructInfo.h"
#include <windows.h>
#include "hnconvert_global.h"

static hdHiScanLidarPara hdHiScanLidarParatest;

class HNCONVERT_EXPORT IScanDB
{

public:
	IScanDB();
	
	~IScanDB();

	
	// 读取数据库值
	bool ReadISCANVal(const char* strFlie);

	//获得数据库的值
	void GetISCANVal(hdHiScanLidarPara &_hdHiScanLidarPara);
private:
	//回调函数
	static int callback1(void *data, int argc, char **argv, char**azColName);

	hdHiScanLidarPara m_hdHiScanLidarPara;

	char* UnicodeToUtf8(const WCHAR* szWideFilename);
	// ansic->uncoide
	WCHAR* MbcsToUnicode(const char* szFilename);
};
