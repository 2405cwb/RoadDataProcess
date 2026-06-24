#include "stdafx.h"
#include "hnDBSqlite.h"
#include "hnDBDefine.h"
#include <algorithm>
#include "time.h"
#include <io.h>
using namespace std;

//namespace hnDataTable
//{
	hnDBSqlite::hnDBSqlite(const char* strDataSource)
	{
		strcpy(m_strDataSource, strDataSource);
		memset(m_strQuery, 0, SQL_QUERY_LEN);
		m_bOpen = false;	
	}

	hnDBSqlite::~hnDBSqlite()
	{
	}

	// 连接数据库
	bool hnDBSqlite::connectDB(const vector<string>& vecDiseaseTable)
	{
		m_vecDiseaseTable = vecDiseaseTable;

		bool bExist = false;

		try
		{
			if (!m_sqliteDB.IsOpen())
			{
				// 判断数据库是否存在
				if (_access(m_strDataSource, 00) != -1)
				{
					bExist = true;
				}

				// 打开成功则返回true
				m_bOpen = m_sqliteDB.Open(m_strDataSource);

				// 创建表
				createTable(vecDiseaseTable);
			}

		}
		catch (...)
		{
			// 抛出异常
			throw exception(m_sqliteDB.GetLastErrorMsg());
			m_bOpen = false;
		}

		// 设置参数
		if (m_bOpen)
		{
			// 设置参数
			setParam();
		}

		return m_bOpen;
	}

	// 创建数据库表结构
	int hnDBSqlite::createTable(const vector<string>& vecDiseaseTable)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return -1;
		}

		try
		{
			char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句
			sprintf(strTableQuery, "select * from sqlite_master where type='table'");

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

			// 如果没有数据则返回false
			if (dr.RowCount() == 0)
			{
				// 创建表结构体对象
				HN_CREATE_RESULT_TABLE tableCmd;
				vector<string> vecCmds = tableCmd.vecCmds;

				char strCmd[SQL_QUERY_LEN] = { 0 };

				// 创建病害表
				for (int i = 0; i < vecDiseaseTable.size(); i++)
				{
					// Mile,RoadWidth
					memset(strCmd, 0, SQL_QUERY_LEN);
					sprintf(strCmd, "Create Table %s("
						"ID int default -1 primary key,"
						"Mileage double default 0,"
						"RoadStandard varchar(%d),"
						"RSurfaceType int default 0,"
						"DrawType int default 0,"
						"Level int default 0,"
						"Length double default 0.0,"
						"Width double default 0.0,"
						"Area double default 0.0,"
						"Depth double default 0.0,"
						"PixelLen int default 0,"
						"PixelWid int default 0,"
						"RealLen double default 0.0,"
						"ReaWidth double default 0.0,"
						"VecRect blob default null,"
						"RectCnt int default 0,"
						"Vec3dPoint blob default null,"
						"n3dCnt int default 0,"
						"GpsTimer blob default null,"
						"GpsTimerCnt int default 0,"
						"Remark varchar(%d),"
						"DiseaseTableName varchar(%d),"
						"DiseaseType int default 0,"
						"Weight double default 1.0,"
						"DisName varchar(%d),"
						"AddFile4 varchar(%d),"
						"AddFile5 varchar(%d),"
						"Dmi double default 0.0,"
						"DmiUp double default 0.0,"
						"DmiDown double default 0.0,"
						"RoadWidth double default 0.0);",
						vecDiseaseTable[i].c_str(), SQL_ADDFILE_LEN,   SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, 
						SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);

					vecCmds.push_back(strCmd);
				}

				// 遍历所有的建表语句
				for (unsigned int i = 0; i < vecCmds.size(); i++)
				{
					// 执行语句
					string strCmd = vecCmds[i];
					m_sqliteDB.ExcuteNonQuery(strCmd.data());
				}
			}

			// 关闭数据库
			dr.Close();

			return 1;
		}
		catch (...)
		{
			// 抛出异常
			throw exception(m_sqliteDB.GetLastErrorMsg());
			return -1;
		}
		return 1;
	}

	// 执行SQL语句
	bool hnDBSqlite::executeDB(const char* strQuery)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 执行表命令
			return m_sqliteDB.ExcuteNonQuery(strQuery);
		}
		catch (...)
		{
			// 抛出异常
			throw exception(m_sqliteDB.GetLastErrorMsg());
			return false;
		}
	}

	// 设置参数
	void hnDBSqlite::setParam()
	{
		// 工程信息设置表
		m_projectSetTable.setDB(m_sqliteDB);

		// 打标信息表
		m_markerInfoTable.setDB(m_sqliteDB);

		// 里程校桩表
		m_milePileTable.setDB(m_sqliteDB);

		// 控制点表
		m_ctrlPointTable.setDB(m_sqliteDB);

		m_mileInfoTable.setDB(m_sqliteDB);

		// 关联数库
		m_diseaseTable.setDB(m_sqliteDB);
		m_diseaseTable.setDiseaseTableName(m_vecDiseaseTable);
	}


	hnRoadDiseaseTable* hnDBSqlite::getDiseaseTable()
	{
		return &m_diseaseTable;
	}

	CtrlPointTable* hnDBSqlite::getCtrlPointTable()
	{
		return &m_ctrlPointTable;
	}

	//是否为空
	bool hnDBSqlite::isOpen()
	{
		if (m_sqliteDB.IsOpen())
		{
			return true;
		}

		return false;
	}

	//获得db路径
	char* hnDBSqlite::getDBPath()
	{
		return m_strDataSource;
	}

//}
