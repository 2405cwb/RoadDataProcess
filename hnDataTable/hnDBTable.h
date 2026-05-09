#pragma once
#include "stdafx.h"
#include "hnSQLite.h"
#include <vector>
#include "..\hnCommon\hnCommonDef.h"
#include "hndatatable_global.h"
#include "hnSQLite.h"
using namespace std;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnDBTable
	{
	public:
		hnDBTable();
		virtual ~hnDBTable();

	public:
		// 设置数据库;
		void setDB(const hnSQLite& sqliteDB);

		// 设置数据库版本信息;
		void setDBVersion(const int nVersion);

		// 获取最大id;
		virtual int getMaxID() = 0;

		//备注字段分割 记录二维数据id
		bool getImageCrackID(const char* strID, vector<int>& vecImageID);

		// 设置二维病害数据
		bool setImageCrackID(vector<int>& vecImageID, char* strID, int nErase = 0);

	protected:

		// 执行SQL语句;
		bool executeDB(const char* strQuery);

		// 判断第一个字母是否为“s”或“S”;
		bool isValidtyQueryString(const char* strQuery);
		// 判断第一个字母是否为“W”或“w”;
		bool isValidtyQueryWString(const char* strQuery);
		// 解析字符;
		bool AnalysisStr(char* strSource, std::vector<double>& vecValue);

	protected:
		// sqlite数据库对象;
		hnSQLite m_sqliteDB;

		// 版本信息;
		int m_nDbVersion;

		// 数据库查询字符串;
		char m_strQuery[SQL_QUERY_LEN];
	};
//}

