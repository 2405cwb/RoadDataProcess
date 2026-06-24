#include "stdafx.h"
#include "hnMilePileTable.h"
#include "time.h"
#include "hnDBDefine.h"

//namespace hnDataTable
//{
	hnMilePileTable::hnMilePileTable()
		:hnDBTable()
	{
	}


	hnMilePileTable::~hnMilePileTable()
	{
	}

	// 读取里程桩信息
	bool hnMilePileTable::readData(vector<hnMilePile>& vecData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s", MILEAGE_PILE_TABLE);

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
			if (nCount<2)
			{
				return false;
			}
			int i = 0;

			// 桥梁病害数据
			vecData.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecData[i].nID = dr.GetIntValue("ID");	

				vecData[i].nDMi = dr.GetInt64Value("DMi");
				vecData[i].dEnclMile = dr.GetFloatValue("EnclMile");
				vecData[i].dTrueMile = dr.GetFloatValue("TrueMile");
				vecData[i].dGpsTimer = dr.GetFloatValue("GpsTimer");

				if (dr.GetStringValue("AddFile1") != NULL)
				{
					strcpy(vecData[i].strAddFile111, dr.GetStringValue("AddFile1"));
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

	// 写入里程桩信息
	bool hnMilePileTable::writeData(vector<hnMilePile>& vecData)
	{
		for (int i = 0; i < vecData.size(); i++)
		{
			writeData(vecData[i]);
		}

		return true;
	}

	// 写入里程桩信息
	bool hnMilePileTable::writeData(hnMilePile& inData)
	{
		// 判断数据库是否连接
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句
			sprintf(strTableQuery, "select * from %s where ID = %d", MILEAGE_PILE_TABLE, inData.nID);

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

			// 不存在则添加
			if (dr.RowCount() == 0)
			{
				// 添加CPIIIsql语句
				memset(strTableQuery, 0, SQL_QUERY_LEN);
				sprintf(strTableQuery, "insert into %s(ID, DMi, EnclMile, TrueMile, GpsTimer,AddFile1,AddFile2,AddFile3,AddFile4,AddFile5,Remark) "
					"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", MILEAGE_PILE_TABLE);

				// sql命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.nID);
				cmd.BindParam(2, inData.nDMi);
				cmd.BindParam(3, inData.dEnclMile);
				cmd.BindParam(4, inData.dTrueMile);
				cmd.BindParam(5, inData.dGpsTimer);
				cmd.BindParam(6, inData.strAddFile111);
				cmd.BindParam(7, inData.strAddFile2);
				cmd.BindParam(8, inData.strAddFile3);
				cmd.BindParam(9, inData.strAddFile4);
				cmd.BindParam(10, inData.strAddFile5);
				cmd.BindParam(11, inData.strRemark);

				// 执行sql语句
				if (!m_sqliteDB.ExcuteNonQuery(&cmd))
				{
					// 清空cmd
					cmd.Clear();
					return false;
				}

				// 清空cmd
				cmd.Clear();
			}
			else
			{
				// 已存在该数据则更新该记录
				memset(strTableQuery, 0, SQL_QUERY_LEN);
				sprintf(strTableQuery, "update %s set "
					"DMi=?, EnclMile=?, TrueMile=?, GpsTimer=?,AddFile1=? ,AddFile2=?,AddFile3=?,AddFile4=?,AddFile5=?,Remark=? where ID = %d", MILEAGE_PILE_TABLE, inData.nID);

				// cmd命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.nDMi);
				cmd.BindParam(2, inData.dEnclMile);
				cmd.BindParam(3, inData.dTrueMile);
				cmd.BindParam(4, inData.dGpsTimer);
				cmd.BindParam(5, inData.strAddFile111);
				cmd.BindParam(6, inData.strAddFile2);
				cmd.BindParam(7, inData.strAddFile3);
				cmd.BindParam(8, inData.strAddFile4);
				cmd.BindParam(9, inData.strAddFile5);
				cmd.BindParam(10, inData.strRemark);

				// 执行sql语句
				if (!m_sqliteDB.ExcuteNonQuery(&cmd))
				{
					// 清空cmd
					cmd.Clear();
					return false;
				}

				// 清空cmd
				cmd.Clear();
			}
		}
		catch (...)
		{
			// 抛出异常
			throw exception(m_sqliteDB.GetLastErrorMsg());
			return false;
		}

		return true;
	}

	bool hnMilePileTable::clearData()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}
		char strQuery[SQL_QUERY_LEN];
		memset(strQuery, 0, SQL_QUERY_LEN);

		sprintf(strQuery, "delete from %s", MILEAGE_PILE_TABLE);

		// 执行sql语句
		executeDB(strQuery);

		return true;

	}

	// 获取最大id
	int hnMilePileTable::getMaxID()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}

		// 查询表语句
		sprintf(m_strQuery, "select max(ID) from %s", MILEAGE_PILE_TABLE);

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
