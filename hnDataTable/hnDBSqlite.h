#ifndef _HN_DB_SQLITE_H_
#define _HN_DB_SQLITE_H_
#include <vector>
#include "hnDataTable.h"
#include "hnSQLite.h"
#include "..\hnCommon\hnCommonDef.h"
#include <vector>
#include <string>
#include "hndatatable_global.h"
#include "hnProjectSetInfoTable.h"
#include "hnMarkerInfoTable.h"
#include "hnMilePileTable.h"
#include "hnRoadDiseaseTable.h"
#include "CtrlPointTable.h"
#include "hnMileTable.h"
using namespace std;

class HNDATATABLE_EXPORT hnDBSqlite
{
public:
	hnDBSqlite(const char* strDataSource);
	~hnDBSqlite();

public:
	// 连接数据库
	bool connectDB(const vector<string>& vecDiseaseTable);

	// 创建数据库表结构
	int createTable(const vector<string>& vecDiseaseTable);

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
	hnSQLite m_sqliteDB;	      

	// 是否打开数据库
	bool m_bOpen;

	// 当前病害表名
	vector<string> m_vecDiseaseTable;

public:
	// 工程信息设置表
	hnProjectSetInfoTable m_projectSetTable;

	// 打标信息表
	hnMarkerInfoTable m_markerInfoTable;

	// 里程校桩表
	hnMilePileTable m_milePileTable;

	// 病害数据库
	hnRoadDiseaseTable m_diseaseTable;

	// 控制点表
	CtrlPointTable m_ctrlPointTable;

	//桩号表
	hnMileTable m_mileInfoTable;
};


#endif
