/*!@file
*******************************************************************************************************
<PRE>
模块名		：hnDBOperate
文件名		：hnSQLite.h
相关文件	：sqlite3.h
文件实现功能：SQLite数据库操作类。
作者		：李夏亮
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2018/3/19	1.0			李夏亮		创建
</PRE>
******************************************************************************************************/

#include "stdafx.h"
#include "hnSQLite.h"
#include <assert.h>
#include <stdlib.h>


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

//namespace hnDataTable
//{
	hnSQLite::hnSQLite(void)
		:m_db(NULL)
	{
	}

	hnSQLite::~hnSQLite(void)
	{
		// 关闭数据库
		Close();
	}

	bool hnSQLite::Open(const char* strFlie)
	{
		// 有效性检查
		if (strFlie == NULL)
		{
			return false;
		}

		// 解决中文路径问题，Ansi -> UTF8
		char* pPath = NULL;
		WCHAR* wcPath;

		// Ansi->Unicode
		wcPath = MbcsToUnicode(strFlie);

		// Unicode->Utf8
		pPath = UnicodeToUtf8(wcPath);

		// 打开数据库
		int nRet = sqlite3_open(pPath, &m_db);

		// 释放内存
		free(wcPath);
		free(pPath);

		if (nRet != SQLITE_OK)
		{
			// 打开失败时提示错误信息
			MessageBox(NULL, MbcsToUnicode(GetLastErrorMsg()), MbcsToUnicode("err"), MB_OK);
			return false;
		}

		return true;
	}

	bool hnSQLite::IsOpen()
	{
		// 返回当前数据库是否打开状态
		if (m_db)
		{
			return true;
		}

		return false;
	}

	void hnSQLite::Close()
	{
		if (m_db)
		{
			// 当数据库打开，关闭
			sqlite3_close(m_db);
			m_db = NULL;
		}
	}

	bool hnSQLite::ExcuteNonQuery(const char* strSql)
	{
		// 有效性检查
		if (strSql == NULL)
		{
			return false;
		}
		sqlite3_stmt* stmt;
		// 执行查询，准备->下一步->完成
		int errorCode = sqlite3_prepare_v2(m_db, strSql, -1, &stmt, NULL);
		if (errorCode != SQLITE_OK)
		{
			return false;
		}
		// 下一步执行
		sqlite3_step(stmt);
		// 查询结束
		return (sqlite3_finalize(stmt) == SQLITE_OK) ? true : false;
	}

	bool hnSQLite::ExcuteExec(const char* strSql)
	{
		// 有效性检查
		if (strSql == NULL)
		{
			return false;
		}
		char* pErrmsg = NULL;
		if (sqlite3_exec(m_db, strSql, 0, 0, &pErrmsg) != SQLITE_OK)
		{
			sqlite3_free(pErrmsg);
			return false;
		}
		sqlite3_free(pErrmsg);
		return true;
	}

	bool hnSQLite::ExcuteNonQuery(hnSQLiteCommand* pCmd)
	{
		// Command对象的查询执行，如存储过程，SQL函数等
		if (pCmd == NULL)
		{
			int ugs = 1;
			return false;
		}
		return pCmd->Excute();
	}

	// 查询
	hnSQLiteDataReader hnSQLite::ExcuteQuery(const char* strSql)
	{
		// 有效性检查
		if (strSql != NULL)
		{
			sqlite3_stmt* stmt = NULL;
			// 执行查询
			int errorCode = sqlite3_prepare_v2(m_db, strSql, -1, &stmt, NULL);
			if (errorCode== SQLITE_OK)
			{
				int nRows(0);
				int nCols(0);
				char* szError = 0;
				char** paszResults = 0;
				// 查询成功，返回CHdSQLiteDataReader对象，及当前查询到的行数
				if (sqlite3_get_table(m_db, strSql, &paszResults, &nRows, &nCols, &szError) == SQLITE_OK)
				{
					sqlite3_free(szError);
					sqlite3_free_table(paszResults);
					return hnSQLiteDataReader(stmt, nRows);
				}
				else
				{
					if (stmt)
					{
						sqlite3_finalize(stmt);
						stmt = NULL;
					}
				}
			}
		}
		return hnSQLiteDataReader(NULL, 0);
	}

	// 开始事务
	bool hnSQLite::BeginTransaction()
	{
		// 开始数据库事务
		char* pErrmsg = NULL;
		if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", NULL, NULL, &pErrmsg) != SQLITE_OK)
		{
			sqlite3_free(pErrmsg);
			return false;
		}
		sqlite3_free(pErrmsg);
		return true;
	}

	// 提交事务
	bool hnSQLite::CommitTransaction()
	{
		// 提交数据库事务
		char* pErrmsg = NULL;
		if (sqlite3_exec(m_db, "COMMIT TRANSACTION;;", NULL, NULL, &pErrmsg) != SQLITE_OK)
		{
			sqlite3_free(pErrmsg);
			return false;
		}
		sqlite3_free(pErrmsg);
		return true;
	}

	// 回滚事务
	bool hnSQLite::RollbackTransaction()
	{
		// 数据库事务回滚
		char* pErrmsg = NULL;
		if (sqlite3_exec(m_db, "ROLLBACK  TRANSACTION;", NULL, NULL, &pErrmsg) != SQLITE_OK)
		{
			sqlite3_free(pErrmsg);
			return false;
		}
		sqlite3_free(pErrmsg);
		return true;
	}

	// 获取上一条错误信息
	const char* hnSQLite::GetLastErrorMsg()
	{
		// 获取数据库错误信息
		return sqlite3_errmsg(m_db);
	}

	char* hnSQLite::UnicodeToUtf8(const WCHAR* szWideFilename)
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

	WCHAR* hnSQLite::MbcsToUnicode(const char* szFilename)
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

	hnSQLiteDataReader::hnSQLiteDataReader(sqlite3_stmt *pStmt, int nRows)
		:m_pStmt(pStmt), m_nRows(nRows)
	{
	}

	hnSQLiteDataReader::~hnSQLiteDataReader()
	{
		Close();
	}

	// 读取一行数据
	bool hnSQLiteDataReader::Read()
	{
		if (m_pStmt == NULL)
		{
			return false;
		}
		if (sqlite3_step(m_pStmt) != SQLITE_ROW)
		{
			return false;
		}
		return true;
	}

	// 关闭Reader，读取结束后调用
	void hnSQLiteDataReader::Close()
	{
		if (m_pStmt)
		{
			sqlite3_finalize(m_pStmt);
			m_pStmt = NULL;
		}
	}

	// 总的列数
	int hnSQLiteDataReader::ColumnCount(void)
	{
		return sqlite3_column_count(m_pStmt);
	}

	// 总的行数
	int hnSQLiteDataReader::RowCount(void)
	{
		return m_nRows;
	}

	// 获取某列的名称 
	const char* hnSQLiteDataReader::GetName(int nCol)
	{
		return (const char*)sqlite3_column_name(m_pStmt, nCol);
	}

	// 获取某列的数据类型
	ENUM_SQLITE_DATATYPE hnSQLiteDataReader::GetDataType(int nCol)
	{
		return (ENUM_SQLITE_DATATYPE)sqlite3_column_type(m_pStmt, nCol);
	}

	int hnSQLiteDataReader::GetFiledIndex(const char* strFiledName)
	{
		if ((m_pStmt != NULL) && (strFiledName != NULL))
		{
			for (int nField = 0; nField < ColumnCount(); nField++)
			{
				const char* szTemp = sqlite3_column_name(m_pStmt, nField);

				if (strcmp(strFiledName, szTemp) == 0)
				{
					return nField;
				}
			}
		}

		return -1;
	}

	// 获取某列的值(字符串)
	const char* hnSQLiteDataReader::GetStringValue(int nCol)
	{
		return (const char*)sqlite3_column_text(m_pStmt, nCol);
	}

	// 获取某列的值(字符串)
	const char* hnSQLiteDataReader::GetStringValue(const char* strFiledName)
	{
		int nCol = GetFiledIndex(strFiledName);
		if (nCol != -1)
		{
			return GetStringValue(nCol);
		}

		return NULL;
	}

	// 获取某列的值(整形)
	int hnSQLiteDataReader::GetIntValue(int nCol)
	{
		return sqlite3_column_int(m_pStmt, nCol);
	}

	// 获取某列的值(整形)
	int hnSQLiteDataReader::GetIntValue(const char* strFiledName)
	{
		int nCol = GetFiledIndex(strFiledName);
		return GetIntValue(nCol);
	}

	// 获取某列的值(长整形)
	long hnSQLiteDataReader::GetInt64Value(int nCol)
	{
		return (long)sqlite3_column_int64(m_pStmt, nCol);
	}

	// 获取某列的值(长整形)
	long hnSQLiteDataReader::GetInt64Value(const char* strFiledName)
	{
		int nCol = GetFiledIndex(strFiledName);
		return GetInt64Value(nCol);
	}

	// 获取某列的值(浮点形)
	double hnSQLiteDataReader::GetFloatValue(int nCol)
	{
		return sqlite3_column_double(m_pStmt, nCol);
	}

	// 获取某列的值(浮点形)
	double hnSQLiteDataReader::GetFloatValue(const char* strFiledName)
	{
		int nCol = GetFiledIndex(strFiledName);
		return GetFloatValue(nCol);
	}

	// 获取某列的值(二进制数据)
	BYTE* hnSQLiteDataReader::GetBlobValue(int nCol, long& nLen)
	{
		nLen = sqlite3_column_bytes(m_pStmt, nCol);
		return (BYTE*)sqlite3_column_blob(m_pStmt, nCol);
	}

	// 获取某列的值(二进制数据)
	BYTE* hnSQLiteDataReader::GetBlobValue(const char* strFiledName, long& nLen)
	{
		int nCol = GetFiledIndex(strFiledName);
		return GetBlobValue(nCol, nLen);
	}

	hnSQLiteCommand::hnSQLiteCommand(hnSQLite* pSqlite)
		:m_pSqlite(pSqlite), m_pStmt(NULL)
	{
	}

	hnSQLiteCommand::hnSQLiteCommand(hnSQLite* pSqlite, const char* lpSql)
		: m_pSqlite(pSqlite), m_pStmt(NULL)
	{
		SetCommandText(lpSql);
	}

	hnSQLiteCommand::~hnSQLiteCommand()
	{
		Clear();
	}

	bool hnSQLiteCommand::SetCommandText(const char* lpSql)
	{
		if (sqlite3_prepare_v2(m_pSqlite->m_db, lpSql, -1, &m_pStmt, NULL) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::BindParam(int index, const char* szValue)
	{
		if (sqlite3_bind_text(m_pStmt, index, szValue, -1, SQLITE_TRANSIENT) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::BindParam(int index, const int nValue)
	{
		if (sqlite3_bind_int(m_pStmt, index, nValue) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::BindParam(int index, const int long long nValue)
	{
		if (sqlite3_bind_int64(m_pStmt, index, nValue) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::BindParam(int index, const double dValue)
	{
		if (sqlite3_bind_double(m_pStmt, index, dValue) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::BindParam(int index, const unsigned char* blobBuf, int nLen)
	{
		if (sqlite3_bind_blob(m_pStmt, index, blobBuf, nLen, NULL) != SQLITE_OK)
		{
			return false;
		}
		return true;
	}

	bool hnSQLiteCommand::Excute()
	{
		sqlite3_step(m_pStmt);
		//modify
		//return (sqlite3_reset(m_pStmt) == SQLITE_OK) ? true : false;
		if (sqlite3_reset(m_pStmt) == SQLITE_OK)
		{
			return true;
		}
		else
		{
			int uuu = sqlite3_reset(m_pStmt);
			return false;
		}

	}

	void hnSQLiteCommand::Clear()
	{
		if (m_pStmt)
		{
			sqlite3_finalize(m_pStmt);
			m_pStmt = NULL;
		}
	}

	// 获得db
	sqlite3*hnSQLite::getDb()
	{
		return m_db;
	}
//}

