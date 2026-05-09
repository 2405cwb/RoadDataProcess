#include "hnMileTable.h"
#include "hnDBDefine.h"
#include "QVector"

hnMileTable::hnMileTable()
{
}


hnMileTable::~hnMileTable()
{
}

bool hnMileTable::readData(QVector<hnMile>& vecData, char* strQuery /*= NULL*/)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}

	try
	{
		// 查询表语句
		sprintf(m_strQuery, "select * from %s", MILE_INFO_TABLE);

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
			// 将值赋给变量--ID
			vecData[i].nID = dr.GetIntValue("ID");

	 
			vecData[i].dEnclMile = dr.GetFloatValue("EnclMile");
			vecData[i].dTrueMile = dr.GetFloatValue("TrueMile");
			vecData[i].dGpsTimer = dr.GetFloatValue("GpsTimer");

			if (dr.GetStringValue("picturePath") != NULL)
			{
		 	const	char *  temp = dr.GetStringValue("picturePath");
				 vecData[i].picturePath=QString::fromLocal8Bit( dr.GetStringValue("picturePath"));
			}

			if (dr.GetStringValue("leftStreetPicPath") != NULL)
			{
				vecData[i].leftStreetPicPath = QString::fromLocal8Bit(dr.GetStringValue("leftStreetPicPath")); 
			}

			if (dr.GetStringValue("rightStreetPicPath") != NULL)
			{
				 
				vecData[i].rightStreetPicPath = QString::fromLocal8Bit(dr.GetStringValue("rightStreetPicPath")); 
			}

			vecData[i].drawType = (hnCommon::ROAD_WORK_TYPE)dr.GetIntValue("drawType");
			vecData[i].roadType =(hnCommon::ROAD_SURFACE_TYPE) dr.GetIntValue("roadType");
			vecData[i].roadStandard =(HnProjectEnums::StandardParmTypeEnum) dr.GetIntValue("roadStandard");

			if (dr.GetStringValue("roadUnitStr") != NULL)
			{
				vecData[i].roadUnitStr = QString::fromLocal8Bit(dr.GetStringValue("roadUnitStr")); 
			}

			if (dr.GetStringValue("roadGradStr") != NULL)
			{
				vecData[i].roadGradStr = QString::fromLocal8Bit(dr.GetStringValue("roadGradStr"));
			}
			vecData[i].roadGrad = dr.GetIntValue("roadGrad");
			vecData[i].roadWidth = dr.GetFloatValue("roadWidth");
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

bool hnMileTable::writeData(QVector<hnMile>& vecData)
{
	for (int i = 0; i < vecData.size(); i++)
	{
		writeData(vecData[i]);
	}

	return true;
}

bool hnMileTable::writeData_Simple(QVector<hnMile>& vecData)
{
	clearData(); 
	//判断数据库是否连接
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}
		char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
		memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句 
													//开启事务
		try
		{
			sqlite3_exec(m_sqliteDB.getDb(), "begin;", 0, 0, 0);
			//其他值
			sqlite3_stmt *stmt = NULL;
			int res = -1;
			// 添加CPIIIsql语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "insert into %s(ID, EnclMile, GpsTimer, TrueMile, picturePath,leftStreetPicPath,rightStreetPicPath,drawType,roadType,roadStandard,roadUnitStr,"
				"roadGradStr,roadGrad,roadWidth) "
				"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?,?,?)", MILE_INFO_TABLE);
			res = sqlite3_prepare_v2(m_sqliteDB.getDb(), strTableQuery, strlen(strTableQuery), &stmt, 0);
			for (int i = 0; i < vecData.size(); ++i)
			{
				res = sqlite3_reset(stmt);
				// SQLITE_STATIC->传递给该字符串的指针将有效,直到执行查询为止
				// 绑定参数
				sqlite3_bind_int(stmt, 1,vecData[i].nID);
				sqlite3_bind_double(stmt,2,vecData[i].dEnclMile);
				sqlite3_bind_double(stmt,3,vecData[i].dGpsTimer);
				sqlite3_bind_double(stmt, 4,vecData[i].dTrueMile); 


				QByteArray utf8Data1   = vecData[i].picturePath.toLocal8Bit();  
				auto sss1 = utf8Data1.toStdString();
				auto cstr1 = sss1.c_str();
				sqlite3_bind_text(stmt,5, cstr1, -1,SQLITE_STATIC);

				 
				QByteArray utf8Data2 = vecData[i].leftStreetPicPath.toLocal8Bit();
				auto sss2 = utf8Data2.toStdString();
				auto cstr2 = sss2.c_str(); 
				sqlite3_bind_text(stmt,6, cstr2, -1, SQLITE_STATIC);

				QByteArray utf8Data3= vecData[i].rightStreetPicPath.toLocal8Bit();
				auto sss3 = utf8Data3.toStdString();
				auto cstr3 = sss3.c_str();
				sqlite3_bind_text(stmt,7, cstr3, -1, SQLITE_STATIC);

				sqlite3_bind_int(stmt, 8,vecData[i].drawType);
				sqlite3_bind_int(stmt, 9,vecData[i].roadType);
				sqlite3_bind_int(stmt, 10, vecData[i].roadStandard);

				QByteArray utf8Data4 = vecData[i].roadUnitStr.toLocal8Bit();
				auto sss4 = utf8Data4.toStdString();
				auto cstr4= sss4.c_str();  
				sqlite3_bind_text (stmt,11, cstr4, -1, SQLITE_STATIC);

				QByteArray utf8Data5 = vecData[i].roadGradStr.toLocal8Bit();
				auto sss5 = utf8Data5.toStdString();
				auto cstr5= sss5.c_str(); 
			 
				sqlite3_bind_text(stmt,12, cstr5, -1, SQLITE_STATIC);
				sqlite3_bind_int(stmt,13, vecData[i].roadGrad);
				sqlite3_bind_double(stmt,14, vecData[i].roadWidth);
				//3.遍历select执行的返回结果
				res = sqlite3_step(stmt);
			 
			}
			sqlite3_finalize(stmt);

			//提交事务
			sqlite3_exec(m_sqliteDB.getDb(), "commit;", 0, 0, 0);
		}
		catch (...)
		{
			//提交事务存在问题 则事务回滚 事务已经提交则无法回滚
			sqlite3_exec(m_sqliteDB.getDb(), "rollback;", 0, 0, 0);
		} 
		

	return true;
}

bool hnMileTable::writeData(hnMile& inData)
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
		sprintf(strTableQuery, "select * from %s where ID = %d", MILE_INFO_TABLE, inData.nID);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

		// 不存在则添加
		if (dr.RowCount() == 0)
		{
			// 添加CPIIIsql语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "insert into %s(ID, EnclMile, GpsTimer, TrueMile, picturePath,leftStreetPicPath,rightStreetPicPath,drawType,roadType,roadStandard,roadUnitStr,"
			"roadGradStr,roadGrad,roadWidth) "
				"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?,?,?)", MILE_INFO_TABLE);

			// sql命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			cmd.BindParam(1, inData.nID);
			cmd.BindParam(2, inData.dEnclMile);
			cmd.BindParam(3, inData.dGpsTimer);
			cmd.BindParam(4, inData.dTrueMile);
			QByteArray utf8Data = inData.picturePath.toLocal8Bit();
			const char * cstr = utf8Data.constData();
			cmd.BindParam(5, cstr);
			utf8Data = inData.leftStreetPicPath.toLocal8Bit();
			cmd.BindParam(6, utf8Data.constData());
			utf8Data = inData.rightStreetPicPath.toLocal8Bit();
			cmd.BindParam(7, utf8Data.constData());
			cmd.BindParam(8, inData.drawType);
			cmd.BindParam(9, inData.roadType);
			cmd.BindParam(10, inData.roadStandard);
			utf8Data = inData.roadUnitStr.toLocal8Bit();
			cmd.BindParam(11, utf8Data.constData());
			utf8Data = inData.roadGradStr.toLocal8Bit(); 
			cmd.BindParam(12, utf8Data.constData());
			cmd.BindParam(13, inData.roadGrad);
			cmd.BindParam(14, inData.roadWidth);

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
				"ID=?, EnclMile=?,  GpsTimer=?,TrueMile=?,picturePath=? ,leftStreetPicPath=?,rightStreetPicPath=?,drawType"
				"=?,roadType=?,roadUnitStr=?,roadGradStr=?,roadGrad=?,roadWidth=? where ID = %d", MILE_INFO_TABLE, inData.nID);

			// cmd命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			cmd.BindParam(1, inData.nID);
			cmd.BindParam(2, inData.dEnclMile);
			cmd.BindParam(3, inData.dGpsTimer);
			cmd.BindParam(4, inData.dTrueMile);
			QByteArray utf8Data = inData.picturePath.toLocal8Bit();
			const char * cstr = utf8Data.constData();
			cmd.BindParam(5, cstr);
			utf8Data = inData.leftStreetPicPath.toLocal8Bit();
			cmd.BindParam(6, utf8Data.constData());
			utf8Data = inData.rightStreetPicPath.toLocal8Bit();
			cmd.BindParam(7, utf8Data.constData());
			cmd.BindParam(8, inData.drawType);
			cmd.BindParam(9, inData.roadType);
			cmd.BindParam(10, inData.roadStandard);
			utf8Data = inData.roadUnitStr.toLocal8Bit();

			cmd.BindParam(11, utf8Data.constData());
			utf8Data = inData.roadGradStr.toLocal8Bit();
			cmd.BindParam(12, utf8Data.constData());
			cmd.BindParam(13, inData.roadGrad);
			cmd.BindParam(14, inData.roadWidth);

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

bool hnMileTable::clearData()
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return 1;
	}
	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	sprintf(strQuery, "delete from %s", MILE_INFO_TABLE);

	// 执行sql语句
	executeDB(strQuery);

	return true;
}

// 获取最大id
int hnMileTable::getMaxID()
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return 1;
	}

	// 查询表语句
	sprintf(m_strQuery, "select max(ID) from %s", MILE_INFO_TABLE);

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
