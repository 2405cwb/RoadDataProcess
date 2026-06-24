#include "stdafx.h"
#include "hnDBSqliteRoadInfo.h"
#include "hnDBDefine.h"
#include <algorithm>
#include "time.h"
#include <io.h>
using namespace std;

//namespace hnDataTable
//{
	hnDBSqliteRoadInfo::hnDBSqliteRoadInfo(const char* strDataSource)
	{
		memset(m_strDataSource, 0, SQL_QUERY_LEN);
		strcpy(m_strDataSource, strDataSource);
		memset(m_strQuery, 0, SQL_QUERY_LEN);
		m_bOpen = false;
	
	}

	hnDBSqliteRoadInfo::~hnDBSqliteRoadInfo()
	{
	}

	// 连接数据库
	bool hnDBSqliteRoadInfo::connectDB()
	{
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

				// 
				createTable();
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
	int hnDBSqliteRoadInfo::createTable()
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
				HN_CREATE_SETTINGTABLE tableCmd;
				unsigned int nSize = tableCmd.vecCmds.size();
				// 遍历所有的建表语句
				for (unsigned int i = 0; i < nSize; i++)
				{
					// 执行语句
					string strCmd = tableCmd.vecCmds[i];
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
	bool hnDBSqliteRoadInfo::executeDB(const char* strQuery)
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
	void hnDBSqliteRoadInfo::setParam()
	{
		// 病害设置表
		m_diseaseSetTable.setDB(m_sqliteDB);

		// 道路类型标准参数设置表
		m_roadTypeSetTable.setDB(m_sqliteDB);

	}


	//是否为空
	bool hnDBSqliteRoadInfo::isOpen()
	{
		if (m_sqliteDB.IsOpen())
		{
			return true;
		}

		return false;
	}

	//获得db路径
	char* hnDBSqliteRoadInfo::getDBPath()
	{
		return m_strDataSource;
	}

//}
