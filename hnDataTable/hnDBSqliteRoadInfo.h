#ifndef _HN_DB_SQLITE_ROADINFO_H_
#define _HN_DB_SQLITE_ROADINFO_H_
#include <vector>
#include "hnDataTable.h"
#include "hnSQLite.h"
#include "..\hnCommon\hnCommonDef.h"
#include "hndatatable_global.h"
#include "hnDiseaseSetInfoTable.h"
#include "hnRoadTypeSetInfoTable.h"
using namespace std;

//namespace hnDataTable
//{
	class HNDATATABLE_EXPORT hnDBSqliteRoadInfo
	{
	public:
		hnDBSqliteRoadInfo(const char* strDataSource);
		~hnDBSqliteRoadInfo();

	public:
		// 连接数据库
		bool connectDB();

		// 创建数据库表结构
		int createTable();

		// 执行SQL语句
		bool executeDB(const char* strQuery);

		//是否打开
		bool isOpen();

		//获得db路径
		char* getDBPath();
	private:
		// 设置参数
		void setParam();

	private:
		// 数据库查询字符串
		char m_strQuery[SQL_QUERY_LEN];

		// 数据库本地（远程）数据源
		char m_strDataSource[SQL_QUERY_LEN];

		// sqlite数据库对象
		hnSQLite	m_sqliteDB;	      

		// 是否打开数据库
		bool m_bOpen;

	public:
		// 病害设置表
		hnDiseaseSetInfoTable m_diseaseSetTable;

		// 道路类型标准参数设置表
		hnRoadTypeSetInfoTable m_roadTypeSetTable;
	};
//}

#endif
