#include "stdafx.h"
#include "hnMarkerInfoTable.h"
#include "time.h"
#include "hnDBDefine.h"

//namespace hnDataTable
//{
	hnMarkerInfoTable::hnMarkerInfoTable()
		:hnDBTable()
	{
	}


	hnMarkerInfoTable::~hnMarkerInfoTable()
	{
	}

	// 读取打标信息
	bool hnMarkerInfoTable::readData(vector<hnMarkInfo>& vecData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s", MARK_INFO_TABLE);

			// 如果strQuery不为空则判断合法性，然后将其连接至查询语句
			if (strQuery != NULL)
			{
				// 检查输入的strQuery第一个有效字段是否以W或w开头
				if (!isValidtyQueryString(strQuery))
				{
					return false;
				}

				strcpy(m_strQuery, strQuery);
			}

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			int nCount = dr.RowCount();

			int i = 0;

			// 桥梁病害数据
			vecData.resize(nCount);

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecData[i].nID = dr.GetIntValue("ID");	

				vecData[i].nType = dr.GetIntValue("Type");
				vecData[i].dEnclMile = dr.GetFloatValue("EnclMile");
				vecData[i].dTrueMile = dr.GetFloatValue("TrueMile");
				vecData[i].dGpsTimer = dr.GetFloatValue("GpsTimer");
			
				if (dr.GetStringValue("Mark") != NULL)
				{
					strcpy(vecData[i].strMark, dr.GetStringValue("Mark"));
				}

				if (dr.GetStringValue("AddFile1") != NULL)
				{
					strcpy(vecData[i].strAddFile1, dr.GetStringValue("AddFile1"));
				}

				if (dr.GetStringValue("AddFile2") != NULL)
				{
					strcpy(vecData[i].strAddFile2, dr.GetStringValue("AddFile2"));
				}

				if (dr.GetStringValue("AddFile3") != NULL)
				{
					strcpy(vecData[i].strAddFile3, dr.GetStringValue("AddFile3"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecData[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecData[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecData[i].strRemark, dr.GetStringValue("Remark"));
				}

				i++;
			}

			dr.Close();
		}
		catch (...)
		{
			// 抛出异常
			throw exception(m_sqliteDB.GetLastErrorMsg());
			return false;
		}

		return true;
	}

	// 写入打标信息
    // 无论独立调用还是加入上层事务，任何 SQL 失败都必须返回 false。
    bool hnMarkerInfoTable::writeData(vector<hnMarkInfo>& vecData, bool(*pProgress)(float, const char*, bool))
    {
        if (!m_sqliteDB.IsOpen()) return false;
        sqlite3* db = m_sqliteDB.getDb();
        const bool ownTransaction = sqlite3_get_autocommit(db) != 0;
        if (ownTransaction && sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK) return false;
        sqlite3_stmt* stmt = nullptr;
        bool success = false;
        try
        {
            const char* sql = "INSERT INTO MARK_INFO(ID,Type,EnclMile,TrueMile,GpsTimer,Mark,"
                "AddFile1,AddFile2,AddFile3,AddFile4,AddFile5,Remark) VALUES(?,?,?,?,?,?,?,?,?,?,?,?)";
            success = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK;
            for (size_t i = 0; success && i < vecData.size(); ++i)
            {
                const hnMarkInfo& mark = vecData[i];
                int status = sqlite3_reset(stmt);
                status |= sqlite3_bind_int(stmt, 1, mark.nID);
                status |= sqlite3_bind_int(stmt, 2, mark.nType);
                status |= sqlite3_bind_double(stmt, 3, mark.dEnclMile);
                status |= sqlite3_bind_double(stmt, 4, mark.dTrueMile);
                status |= sqlite3_bind_double(stmt, 5, mark.dGpsTimer);
                status |= sqlite3_bind_text(stmt, 6, mark.strMark, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 7, mark.strAddFile1, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 8, mark.strAddFile2, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 9, mark.strAddFile3, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 10, mark.strAddFile4, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 11, mark.strAddFile5, -1, SQLITE_TRANSIENT);
                status |= sqlite3_bind_text(stmt, 12, mark.strRemark, -1, SQLITE_TRANSIENT);
                success = status == SQLITE_OK && sqlite3_step(stmt) == SQLITE_DONE;
            }
            if (stmt)
            {
                const int status = sqlite3_finalize(stmt);
                stmt = nullptr;
                if (status != SQLITE_OK) success = false;
            }
            if (success && pProgress) pProgress(0.9999f, "导入数据库", false);
            if (success && ownTransaction) success = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
        }
        catch (...)
        {
            if (stmt) sqlite3_finalize(stmt);
            success = false;
        }
        if (!success && ownTransaction) sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return success;
    }

	bool hnMarkerInfoTable::updateData(vector<hnMarkInfo>& vecData,  bool(*pProgress)(float fVal, const char* qstrName, bool bCancle))
	{
		// 判断数据库是否连接
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}
		char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
		memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句 
		try
		{ 
			//开启事务
			sqlite3_exec(m_sqliteDB.getDb(), "beginUpdate;", 0, 0, 0);
			//其他值
			sqlite3_stmt *stmt = NULL;
			int res = -1;
			// 添加CPIIIsql语句
			memset(strTableQuery, 0, SQL_QUERY_LEN); 
				sprintf(strTableQuery, "update %s set "
					"Type=?, EnclMile=?, TrueMile=?, GpsTimer=?, Mark=?,"
					"AddFile1=? ,AddFile2=?,AddFile3=?,AddFile4=?,AddFile5=?,Remark=? where ID = ?", MARK_INFO_TABLE);
		 
			res = sqlite3_prepare_v2(m_sqliteDB.getDb(), strTableQuery, strlen(strTableQuery), &stmt, 0);
			for (int i = 0; i < vecData.size(); ++i)
			{
				//重置stmt
				res = sqlite3_reset(stmt); 
				// SQLITE_STATIC->传递给该字符串的指针将有效,直到执行查询为止 
				res = sqlite3_bind_int(stmt, 1, vecData[i].nType);
				res = sqlite3_bind_double(stmt, 2, vecData[i].dEnclMile);
				res = sqlite3_bind_double(stmt, 3, vecData[i].dTrueMile);
				res = sqlite3_bind_double(stmt, 4, vecData[i].dGpsTimer);
				res = sqlite3_bind_text(stmt, 5, vecData[i].strMark, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 6, vecData[i].strAddFile1, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 7, vecData[i].strAddFile2, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 8, vecData[i].strAddFile3, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 9, vecData[i].strAddFile4, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 10, vecData[i].strAddFile5, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 11, vecData[i].strRemark, -1, SQLITE_STATIC);
				res = sqlite3_bind_int(stmt, 12, vecData[i].nID);
				//3.遍历select执行的返回结果
				res = sqlite3_step(stmt);
				if (pProgress && i%vecData.size() == 0)
				{
					pProgress(0.9999, "更新数据库", false);
				}
			}
			sqlite3_finalize(stmt);

			sqlite3_exec(m_sqliteDB.getDb(), "commit;", 0, 0, 0);
		}
		catch (...)
		{
			if (pProgress)
			{
				pProgress(1.0, "更新数据库", false);
			}

			sqlite3_exec(m_sqliteDB.getDb(), "rollback;", 0, 0, 0);
		} 
	}

	bool hnMarkerInfoTable::deleteData(const vector<int>& vecIdxData)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}
		std::string sql = "DELETE FROM " + std::string( MARK_INFO_TABLE) + " WHERE ID IN (";
		for (size_t i = 0; i < vecIdxData.size(); ++i)
		{
			sql += std::to_string(vecIdxData[i]);
			if (i!= vecIdxData.size()-1)
			{
				sql += ",";
			}
		}
		sql += ");";

		// 执行sql语句
		executeDB(sql.c_str()); 
		return true;
	}

    bool hnMarkerInfoTable::clearData()
    {
        if (!m_sqliteDB.IsOpen()) return false;
        return sqlite3_exec(m_sqliteDB.getDb(), "DELETE FROM MARK_INFO;", nullptr, nullptr, nullptr) == SQLITE_OK;
    }

	// 获取最大id
	int hnMarkerInfoTable::getMaxID()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}

		// 查询表语句
		sprintf(m_strQuery, "select max(ID) from %s", MARK_INFO_TABLE);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

		if (dr.RowCount() != 1)
		{
			return 1;
		}

		int nID = 1;

		if (dr.Read())
		{
			nID = dr.GetIntValue(0) + 1;
		}

		return nID;
	}

//}
