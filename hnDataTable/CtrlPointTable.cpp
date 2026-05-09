#include "CtrlPointTable.h"

CtrlPointTable::CtrlPointTable():
	hnDBTable()
{
	m_tableName = CTRL_POINT_TABLE;
}

int CtrlPointTable::getMaxID()
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return 1;
	}

	// 查询表语句
	sprintf(m_strQuery, "select max(ID) from %s", m_tableName.toLocal8Bit().data());

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

bool CtrlPointTable::readData(vector<hnKZDDataInfo>& vecData, char * strQuery)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}

	try
	{
		// 查询表语句
		sprintf(m_strQuery, "select * from %s", m_tableName.toLocal8Bit().data());

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

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		// 遍历查询出的数据
		while (dr.Read() && (i < nCount))
		{
			// ID
			vecData[i].nID = dr.GetIntValue("ID");
			//控制点名称
			if (dr.GetStringValue("KzdName") != NULL)
			{
				strcpy(vecData[i].strKzdName, dr.GetStringValue("KzdName"));
			}
			//控制点GPS时间
			vecData[i].dGpsTimer = dr.GetFloatValue("GpsTimer");
			//里程
			vecData[i].dMileage = dr.GetFloatValue("Mileage");
			//图像名称
			if (dr.GetStringValue("ImageName") != NULL)
			{
				strcpy(vecData[i].strImageName, dr.GetStringValue("ImageName"));
			}
			//图片x位置
			vecData[i].nLocX = dr.GetInt64Value("LocX");
			//图片y位置
			vecData[i].nLocY = dr.GetInt64Value("LocY");
			// 三维坐标-X
			vecData[i].dX = dr.GetFloatValue("X");
			// 三维坐标-Y
			vecData[i].dY = dr.GetFloatValue("Y");
			// 三维坐标-Z
			vecData[i].dZ = dr.GetFloatValue("Z");
			// 备注
			if (dr.GetStringValue("Remaks") != NULL)
			{
				strcpy(vecData[i].strRemaks, dr.GetStringValue("Remaks"));
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

bool CtrlPointTable::writeData(vector<hnKZDDataInfo>& vecData)
{
	for (int i = 0; i < vecData.size(); i++)
	{
		writeData(vecData[i]);
	}

	return true;
}

bool CtrlPointTable::writeData(hnKZDDataInfo & inData)
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
		sprintf(strTableQuery, "select * from %s where ID = %d", m_tableName.toLocal8Bit().data(), inData.nID);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

		// 不存在则添加
		if (dr.RowCount() == 0)
		{
			// 添加CPIIIsql语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "insert into %s(ID, KzdName, GpsTimer,Mileage, ImageName, LocX, LocY, X, Y, Z, Remarks) "
				"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", m_tableName.toLocal8Bit().data());

			// sql命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			cmd.BindParam(1, inData.nID);
			cmd.BindParam(2, inData.strKzdName);
			cmd.BindParam(3, inData.dGpsTimer);
			cmd.BindParam(4, inData.dMileage);
			cmd.BindParam(5, inData.strImageName);
			cmd.BindParam(6, inData.nLocX);
			cmd.BindParam(7, inData.nLocY);
			cmd.BindParam(8, inData.dX);
			cmd.BindParam(9, inData.dY);
			cmd.BindParam(10, inData.dZ);
			cmd.BindParam(11, inData.strRemaks);

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
				"KzdName=?, GpsTimer=?,Mileage=?, ImageName=?, LocX=?, LocY=?, X=?, Y=?, Z=?, Remarks where ID = %d",
				m_tableName.toLocal8Bit().data(), inData.nID);

			// cmd命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			cmd.BindParam(1, inData.strKzdName);
			cmd.BindParam(2, inData.dGpsTimer);
			cmd.BindParam(3, inData.dMileage);
			cmd.BindParam(4, inData.strImageName);
			cmd.BindParam(5, inData.nLocX);
			cmd.BindParam(6, inData.nLocY);
			cmd.BindParam(7, inData.dX);
			cmd.BindParam(8, inData.dY);
			cmd.BindParam(9, inData.dZ);
			cmd.BindParam(10, inData.strRemaks);

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

bool CtrlPointTable::deleteData(vector<hnKZDDataInfo>& vecData)
{
	bool ok = false;
	for (auto data :vecData)
	{
		ok = this->deleteData(data);
	}
	return ok;
}

bool CtrlPointTable::deleteData(const hnKZDDataInfo & data)
{
	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	sprintf(strQuery, "DELETE FROM %s WHERE ID=%d", m_tableName.toLocal8Bit().data(), data.nID);

	// 执行sql语句
	return executeDB(strQuery);	
}

bool  CtrlPointTable::deleteAllData()
{
	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	sprintf(strQuery, "DELETE FROM %s", m_tableName.toLocal8Bit().data());

	// 执行sql语句
	return executeDB(strQuery);
}
