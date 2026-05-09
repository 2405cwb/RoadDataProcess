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
#pragma once
#include "stdafx.h"
#include "sqlite3.h"
#include "hndatatable_global.h"
// 数据类型定义
enum ENUM_SQLITE_DATATYPE
{
	E_SD_INTEGER = SQLITE_INTEGER,	// 整形
	E_SD_FLOAT  = SQLITE_FLOAT,		// 浮点型
	E_SD_TEXT  = SQLITE_TEXT,		// 字符型
	E_SD_BLOB = SQLITE_BLOB,		// 二进制
	E_SD_NULL= SQLITE_NULL			// 无效
};

//namespace hnDataTable
//{
	class hnSQLite;

	class HNDATATABLE_EXPORT hnSQLiteDataReader
	{
	public:
		hnSQLiteDataReader(sqlite3_stmt* pStmt, int nRows);
		~hnSQLiteDataReader();
	public:
		// 读取一行数据
		bool Read();

		// 关闭Reader，读取结束后调用
		void Close();

		// 总的列数
		int ColumnCount(void);

		// 总的行数
		int RowCount(void);

		// 获取某列的名称 
		const char* GetName(int nCol);

		// 获取某列的数据类型
		ENUM_SQLITE_DATATYPE GetDataType(int nCol);

		// 获取某列的值(字符串)
		const char* GetStringValue(int nCol);

		// 获取某列的值(字符串)
		const char* GetStringValue(const char* strFiledName);

		// 获取某列的值(整形)
		int GetIntValue(int nCol);

		// 获取某列的值(整形)
		int GetIntValue(const char* strFiledName);

		// 获取某列的值(长整形)
		long GetInt64Value(int nCol);

		// 获取某列的值(长整形)
		long GetInt64Value(const char* strFiledName);

		// 获取某列的值(浮点形)
		double GetFloatValue(int nCol);

		// 获取某列的值(浮点形)
		double GetFloatValue(const char* strFiledName);

		// 获取某列的值(二进制数据)
		BYTE* GetBlobValue(int nCol, long& nLen);

		// 获取某列的值(二进制数据)
		BYTE* GetBlobValue(const char* strFiledName, long& nLen);

	private:
		// 获取指定列名的列索引
		int GetFiledIndex(const char* strFiledName);
	private:
		sqlite3_stmt* m_pStmt;
		int	m_nRows;			// 当前查询到的行数
	};

	class HNDATATABLE_EXPORT hnSQLiteCommand
	{
	public:
		hnSQLiteCommand(hnSQLite* pSqlite);
		hnSQLiteCommand(hnSQLite* pSqlite, const char* strSql);
		~hnSQLiteCommand();
	public:
		// 设置命令
		bool SetCommandText(const char* strSql);

		// 绑定参数（nindex为要绑定参数的序号，从1开始）
		bool BindParam(int nindex, const char* szValue);
		bool BindParam(int nindex, const int nValue);
		bool BindParam(int index, const int long long nValue);
		bool BindParam(int nindex, const double dValue);
		bool BindParam(int nindex, const unsigned char* blobValue, int nLen);

		// 执行命令
		bool Excute();

		// 清除命令（命令不再使用时需调用该接口清除）
		void Clear();
	private:
		hnSQLite* m_pSqlite;
		sqlite3_stmt* m_pStmt;
	};

	class HNDATATABLE_EXPORT hnSQLite
	{
	public:
		hnSQLite(void);
		~hnSQLite(void);
	public:
		// 打开数据库
		bool Open(const char* strFlie);

		// 数据库是否已打开
		bool IsOpen();

		// 关闭数据库
		void Close();

		// 执行非查询操作（更新或删除）
		bool ExcuteNonQuery(const char* strSql);
		bool ExcuteNonQuery(hnSQLiteCommand* pCmd);
		bool ExcuteExec(const char* strSql);

		// 查询
		hnSQLiteDataReader ExcuteQuery(const char* strSql);

		// 开始事务
		bool BeginTransaction();

		// 提交事务
		bool CommitTransaction();

		// 回滚事务
		bool RollbackTransaction();

		// 获取上一条错误信息
		const char* GetLastErrorMsg();
		// unicode->utf8
		char* UnicodeToUtf8(const WCHAR* szWideFilename);
		// ansic->uncoide
		WCHAR* MbcsToUnicode(const char* szFilename);

		sqlite3*getDb();

	public:
		friend class hnSQLiteCommand;
	private:
		sqlite3* m_db;		// 数据库对象
	};
//}