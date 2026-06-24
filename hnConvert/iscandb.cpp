#include "iscandb.hpp"
#include <iostream>
#include <stdlib.h>

using namespace std;


IScanDB::IScanDB()
{
	
}

IScanDB::~IScanDB() 
{
	
}


int IScanDB::callback1(void *data, int argc, char **argv, char**azColName)
{
	int i;

	vector<QString>vecStrVal;
	for (int i = 0; i < argc; i++)
	{
		vecStrVal.push_back(argv[i]);
	}

	hdHiScanLidarParatest.dx = vecStrVal[3].toDouble();
	hdHiScanLidarParatest.dy = vecStrVal[4].toDouble();
	hdHiScanLidarParatest.dz = vecStrVal[5].toDouble();
	hdHiScanLidarParatest.dyaw = vecStrVal[6].toDouble();
	hdHiScanLidarParatest.dpitch = vecStrVal[7].toDouble();
	hdHiScanLidarParatest.droll = vecStrVal[8].toDouble();

	return 0;
}


//打开数据库
bool IScanDB::ReadISCANVal(const char* strFlie)
{
	// 有效性检查
	if (strFlie == NULL)
	{
		return false;
	}

	sqlite3* m_db;
	// 打开数据库

	// 解决中文路径问题，Ansi -> UTF8
	char* pPath = NULL;
	WCHAR* wcPath;

	// Ansi->Unicode
	wcPath = MbcsToUnicode(strFlie);

	// Unicode->Utf8
	pPath = UnicodeToUtf8(wcPath);

	int nRet = sqlite3_open(pPath, &m_db);
	
	// 释放内存
	free(wcPath);
	free(pPath);

	if (nRet != SQLITE_OK)
	{
		return false;
	}

	char* errMsg = 0;

	string strSQL = "select * from HD_iScanLidar where iScanLidarNo = 1";

	const char* data = "Callback function called";
	int res = sqlite3_exec(m_db, strSQL.c_str(), callback1, (void *)data, &errMsg);

	if (res != SQLITE_OK)
	{
		//设备0编号读取方法
		strSQL = "select * from HD_ISCANPANO where iScanPanoNo = 1";

		data = "Callback function called";
		res = sqlite3_exec(m_db, strSQL.c_str(), callback1, (void *)data, &errMsg);
		if (res != SQLITE_OK)
		{
			return false;
		}
		
	}

	int res1 = sqlite3_close(m_db);
	
	/*if (m_db)
	{
		delete m_db;
		m_db = NULL;
	}*/
	return true;
}

//获得数据库的值
void IScanDB::GetISCANVal(hdHiScanLidarPara &_hdHiScanLidarPara)
{
	_hdHiScanLidarPara = hdHiScanLidarParatest;
	//_hdHiScanLidarPara = hdHiScanLidarParatest;
}

char* IScanDB::UnicodeToUtf8(const WCHAR* szWideFilename)
{
	int nByte;
	char* szFilename;
	nByte = WideCharToMultiByte(CP_UTF8, 0, szWideFilename, -1, 0, 0, 0, 0);
	szFilename = (char *)malloc(nByte);
	if (szFilename == NULL)
	{
		return 0;
	}
	nByte = WideCharToMultiByte(CP_UTF8, 0, szWideFilename, -1, szFilename, nByte, 0, 0);
	if (nByte == NULL)
	{
		free(szFilename);
		szFilename = NULL;
	}
	return szFilename;
}

WCHAR* IScanDB::MbcsToUnicode(const char* szFilename)
{
	int nByte;
	WCHAR* szMbcsFilename;
	int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
	nByte = MultiByteToWideChar(codepage, 0, szFilename, -1, NULL, 0) * sizeof(WCHAR);
	szMbcsFilename = (WCHAR *)malloc(nByte * sizeof(szMbcsFilename[0]));
	if (szMbcsFilename == NULL)
	{
		return NULL;
	}
	nByte = MultiByteToWideChar(codepage, 0, szFilename, -1, szMbcsFilename, nByte);
	if (nByte == 0)
	{
		free(szMbcsFilename);
		szMbcsFilename = NULL;
	}
	return szMbcsFilename;
}
