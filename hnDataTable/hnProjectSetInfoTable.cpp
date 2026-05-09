#include "stdafx.h"
#include "hnProjectSetInfoTable.h"
#include "time.h"
#include "hnDBDefine.h"

//namespace hnDataTable
//{
	hnProjectSetInfoTable::hnProjectSetInfoTable()
		:hnDBTable()
	{
	}


	hnProjectSetInfoTable::~hnProjectSetInfoTable()
	{
	}

	// 读取病害设置信息
	bool hnProjectSetInfoTable::readData(vector<hnProjectSetInfo>& vecData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s", SETTING_INFO_TABLE);

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

				vecData[i].dBegMile = dr.GetFloatValue("BegMile");
				vecData[i].dEndMile = dr.GetFloatValue("EndMile");
				vecData[i].dBegEnclMile = dr.GetFloatValue("BegEnclMile");
				vecData[i].dEndEnclMile = dr.GetFloatValue("EndEnclMile");

				vecData[i].nLineType = dr.GetIntValue("LineType");
				vecData[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");

				vecData[i].dLength = dr.GetFloatValue("Length");
				vecData[i].nWorkType = dr.GetIntValue("WorkType");
				vecData[i].nImageExtent = dr.GetIntValue("ImageExtent");
				vecData[i].nFrequency = dr.GetIntValue("Frequency");
				vecData[i].dWheelPerimeter = dr.GetFloatValue("WheelPerimeter");
				vecData[i].dRoadWidth = dr.GetFloatValue("Width");

				if (dr.GetStringValue("Province") != NULL)
				{
					strcpy(vecData[i].strProvince, dr.GetStringValue("Province"));
				}

				if (dr.GetStringValue("City") != NULL)
				{
					strcpy(vecData[i].strCity, dr.GetStringValue("City"));
				}

				if (dr.GetStringValue("County") != NULL)
				{
					strcpy(vecData[i].strCounty, dr.GetStringValue("County"));
				}

				if (dr.GetStringValue("RoadName") != NULL)
				{
					strcpy(vecData[i].strRoadName, dr.GetStringValue("RoadName"));
				}

				if (dr.GetStringValue("Date") != NULL)
				{
					strcpy(vecData[i].strDate, dr.GetStringValue("Date"));
				}

				if (dr.GetStringValue("Timer") != NULL)
				{
					strcpy(vecData[i].strTimer, dr.GetStringValue("Timer"));
				}

				if (dr.GetStringValue("RoadLevel") != NULL)
				{
					strcpy(vecData[i].strRoadLevel, dr.GetStringValue("RoadLevel"));
				}

				if (dr.GetStringValue("Surveyor") != NULL)
				{
					strcpy(vecData[i].strSurveyor, dr.GetStringValue("Surveyor"));
				}

				if (dr.GetStringValue("Weather") != NULL)
				{
					strcpy(vecData[i].strWeather, dr.GetStringValue("Weather"));
				}

				if (dr.GetStringValue("RoadType") != NULL)
				{
					strcpy(vecData[i].strRoadStandard, dr.GetStringValue("RoadType"));
				}

				if (dr.GetStringValue("RoadNO") != NULL)
				{
					strcpy(vecData[i].strRoadNO, dr.GetStringValue("RoadNO"));
				}

				if (dr.GetStringValue("Number") != NULL)
				{
					strcpy(vecData[i].strNumber, dr.GetStringValue("Number"));
				}

				if (dr.GetStringValue("Model") != NULL)
				{
					strcpy(vecData[i].strModel, dr.GetStringValue("Model"));
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

	// 读取工程设置信息
	bool hnProjectSetInfoTable::readData(hnProjectSetInfo& retData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s", SETTING_INFO_TABLE);

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
			if (dr.RowCount() <= 0)
			{
				return false;
			}

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				retData.nID = dr.GetIntValue("ID");

				retData.dBegMile = dr.GetFloatValue("BegMile");
				retData.dEndMile = dr.GetFloatValue("EndMile");
				retData.dBegEnclMile = dr.GetFloatValue("BegEnclMile");
				retData.dEndEnclMile = dr.GetFloatValue("EndEnclMile");

				retData.nLineType = dr.GetIntValue("LineType");
				retData.nRSurfaceType = dr.GetIntValue("RSurfaceType");

				retData.dLength = dr.GetFloatValue("Length");
				retData.nWorkType = dr.GetIntValue("WorkType");
				retData.nImageExtent = dr.GetIntValue("ImageExtent");
				retData.nFrequency = dr.GetIntValue("Frequency");
				retData.dWheelPerimeter = dr.GetFloatValue("WheelPerimeter");
				retData.nDrawType = dr.GetFloatValue("DrawType");
				retData.dRoadWidth = dr.GetFloatValue("Width");
				
				retData.dRadioX = dr.GetFloatValue("RadioX");
				retData.dRadioY = dr.GetFloatValue("RadioY");
				retData.dRoadLength = dr.GetFloatValue("RoadLength");
				retData.picPixelX = dr.GetFloatValue("PicPixelX");
				retData.picPixelY = dr.GetFloatValue("PicPixelY");


				if (dr.GetStringValue("Province") != NULL)
				{
					strcpy(retData.strProvince, dr.GetStringValue("Province"));
				}

				if (dr.GetStringValue("City") != NULL)
				{
					strcpy(retData.strCity, dr.GetStringValue("City"));
				}

				if (dr.GetStringValue("County") != NULL)
				{
					strcpy(retData.strCounty, dr.GetStringValue("County"));
				}

				if (dr.GetStringValue("RoadName") != NULL)
				{
					strcpy(retData.strRoadName, dr.GetStringValue("RoadName"));
				}

				if (dr.GetStringValue("Date") != NULL)
				{
					strcpy(retData.strDate, dr.GetStringValue("Date"));
				}

				if (dr.GetStringValue("Timer") != NULL)
				{
					strcpy(retData.strTimer, dr.GetStringValue("Timer"));
				}

				if (dr.GetStringValue("RoadLevel") != NULL)
				{
					strcpy(retData.strRoadLevel, dr.GetStringValue("RoadLevel"));
				}

				if (dr.GetStringValue("Surveyor") != NULL)
				{
					strcpy(retData.strSurveyor, dr.GetStringValue("Surveyor"));
				}

				if (dr.GetStringValue("Weather") != NULL)
				{
					strcpy(retData.strWeather, dr.GetStringValue("Weather"));
				}

				if (dr.GetStringValue("RoadType") != NULL)
				{
					strcpy(retData.strRoadStandard, dr.GetStringValue("RoadType"));
				}

				if (dr.GetStringValue("RoadNO") != NULL)
				{
					strcpy(retData.strRoadNO, dr.GetStringValue("RoadNO"));
				}

				if (dr.GetStringValue("Number") != NULL)
				{
					strcpy(retData.strNumber, dr.GetStringValue("Number"));
				}

				if (dr.GetStringValue("Model") != NULL)
				{
					strcpy(retData.strModel, dr.GetStringValue("Model"));
				}


				if (dr.GetStringValue("AddFile2") != NULL)
				{
					strcpy(retData.strAddFile2, dr.GetStringValue("AddFile2"));
				}

				if (dr.GetStringValue("AddFile3") != NULL)
				{
					strcpy(retData.strAddFile3, dr.GetStringValue("AddFile3"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(retData.strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(retData.strAddFile5, dr.GetStringValue("AddFile5"));
				}

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(retData.strRemark, dr.GetStringValue("Remark"));
				}

				i++;

				break;
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

	// 写入病害设置信息
	bool hnProjectSetInfoTable::writeData(vector<hnProjectSetInfo>& vecData)
	{
		for (int i = 0; i < vecData.size(); i++)
		{
			writeData(vecData[i]);
		}

		return true;
	}

	// 写入病害设置信息
	bool hnProjectSetInfoTable::writeData(hnProjectSetInfo& inData)
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
			sprintf(strTableQuery, "select * from %s where ID = %d", SETTING_INFO_TABLE, inData.nID);

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

			// 不存在则添加
			if (dr.RowCount() == 0)
			{
				
				// 添加CPIIIsql语句
				memset(strTableQuery, 0, SQL_QUERY_LEN);
				sprintf(strTableQuery, "insert into %s(ID, BegMile, EndMile, BegEnclMile, EndEnclMile, LineType, DrawType, RSurfaceType,Length,WorkType,ImageExtent,Frequency, WheelPerimeter,"
					"Province,City,County,RoadName,Date,Timer,RoadLevel,Surveyor,Weather,RoadType, RoadNO,Number, Model,Width,AddFile2,AddFile3,AddFile4,AddFile5,Remark,RadioX,RadioY,RoadLength,PicPixelX,PicPixelY) "
					"values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?,?,?,?,?, ?, ?, ?, ?, ?, ?)", SETTING_INFO_TABLE);

				// sql命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.nID);
				cmd.BindParam(2, inData.dBegMile);
				cmd.BindParam(3, inData.dEndMile);
				cmd.BindParam(4, inData.dBegEnclMile);
				cmd.BindParam(5, inData.dEndEnclMile);
				cmd.BindParam(7, inData.nLineType);
				cmd.BindParam(6, inData.nDrawType);
				cmd.BindParam(8, inData.nRSurfaceType);
				cmd.BindParam(9, inData.dLength);
				cmd.BindParam(10, inData.nWorkType);
				cmd.BindParam(11, inData.nImageExtent);
				cmd.BindParam(12, inData.nFrequency);
				cmd.BindParam(13, inData.dWheelPerimeter);
				cmd.BindParam(14, inData.strProvince);
				cmd.BindParam(15, inData.strCity);
				cmd.BindParam(16, inData.strCounty);
				cmd.BindParam(17, inData.strRoadName);
				cmd.BindParam(18, inData.strDate);
				cmd.BindParam(19, inData.strTimer);
				cmd.BindParam(20, inData.strRoadLevel);
				cmd.BindParam(21, inData.strSurveyor);
				cmd.BindParam(22, inData.strWeather);
				cmd.BindParam(23, inData.strRoadStandard);
				cmd.BindParam(24, inData.strRoadNO);
				cmd.BindParam(25, inData.strNumber);
				cmd.BindParam(26, inData.strModel);
				cmd.BindParam(27, inData.dRoadWidth);
				cmd.BindParam(28, inData.strAddFile2);
				cmd.BindParam(29, inData.strAddFile3);
				cmd.BindParam(30, inData.strAddFile4);
				cmd.BindParam(31, inData.strAddFile5);
				cmd.BindParam(32, inData.strRemark);
				cmd.BindParam(33, inData.dRadioX);
				cmd.BindParam(34, inData.dRadioY);
				cmd.BindParam(35, inData.dRoadLength);
				cmd.BindParam(36, inData.picPixelX);
				cmd.BindParam(37, inData.picPixelY);

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
					"BegMile=?, EndMile=?, BegEnclMile=?, EndEnclMile=?, LineType=?, DrawType=?, RSurfaceType=?,Length=?,WorkType=?,ImageExtent=?,Frequency=?,WheelPerimeter=?,"
					"Province=?,City=?,County=?,RoadName=?,Date=?,Timer=?,RoadLevel=?, Surveyor=?,Weather=?,RoadType=?,RoadNO=?,Number=?,Model=?,Width=?,"
					"AddFile2=?,AddFile3=?,AddFile4=?,AddFile5=?,Remark=?,RadioX=?,RadioY=?,RoadLength=?,PicPixelX=?,PicPixelY=? where ID = %d", SETTING_INFO_TABLE, inData.nID);

				// cmd命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.dBegMile);
				cmd.BindParam(2, inData.dEndMile);
				cmd.BindParam(3, inData.dBegEnclMile);
				cmd.BindParam(4, inData.dEndEnclMile);
				cmd.BindParam(5, inData.nLineType);
				cmd.BindParam(6, inData.nDrawType);
				cmd.BindParam(7, inData.nRSurfaceType);
				cmd.BindParam(8, inData.dLength);
				cmd.BindParam(9, inData.nWorkType);
				cmd.BindParam(10, inData.nImageExtent);
				cmd.BindParam(11, inData.nFrequency);
				cmd.BindParam(12, inData.dWheelPerimeter);
				cmd.BindParam(13, inData.strProvince);
				cmd.BindParam(14, inData.strCity);
				cmd.BindParam(15, inData.strCounty);
				cmd.BindParam(16, inData.strRoadName);
				cmd.BindParam(17, inData.strDate);
				cmd.BindParam(18, inData.strTimer);
				cmd.BindParam(19, inData.strRoadLevel);
				cmd.BindParam(20, inData.strSurveyor);
				cmd.BindParam(21, inData.strWeather);
				cmd.BindParam(22, inData.strRoadStandard);
				cmd.BindParam(23, inData.strRoadNO);
				cmd.BindParam(24, inData.strNumber);
				cmd.BindParam(25, inData.strModel);
				cmd.BindParam(26, inData.dRoadWidth);
				cmd.BindParam(27, inData.strAddFile2);
				cmd.BindParam(28, inData.strAddFile3);
				cmd.BindParam(29, inData.strAddFile4);
				cmd.BindParam(30, inData.strAddFile5);
				cmd.BindParam(31, inData.strRemark);
				cmd.BindParam(32, inData.dRadioX);
				cmd.BindParam(33, inData.dRadioY);
				cmd.BindParam(34, inData.dRoadLength);
				cmd.BindParam(35, inData.picPixelX);
				cmd.BindParam(36, inData.picPixelY);

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

	bool hnProjectSetInfoTable::clearData()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}
		char strQuery[SQL_QUERY_LEN];
		memset(strQuery, 0, SQL_QUERY_LEN);

		sprintf(strQuery, "delete from %s", SETTING_INFO_TABLE);

		// 执行sql语句
		executeDB(strQuery);

		return true;
	}

	// 获取最大id
	int hnProjectSetInfoTable::getMaxID()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}

		// 查询表语句
		sprintf(m_strQuery, "select max(ID) from %s", SETTING_INFO_TABLE);

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
