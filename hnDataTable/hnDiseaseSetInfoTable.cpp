#include "stdafx.h"
#include "hnDiseaseSetInfoTable.h"
#include "time.h"
#include "hnDBDefine.h"

//namespace hnDataTable
//{
	hnDiseaseSetInfoTable::hnDiseaseSetInfoTable()
		:hnDBTable()
	{
	}


	hnDiseaseSetInfoTable::~hnDiseaseSetInfoTable()
	{
	}

	// 读取病害设置信息
	bool hnDiseaseSetInfoTable::readData(vector<hnDiseaseSetInfo>& vecData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s ORDER BY DiseaseTypeName", DISEASE_SETTING_INFO_TABLE);

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

				vecData[i].nDiseaseIndex = dr.GetIntValue("DiseaseIndex");
				vecData[i].nDiseaseType = dr.GetIntValue("DiseaseType");
				vecData[i].nRoadSurfaceType = dr.GetIntValue("RoadSurfaceType");
				vecData[i].nDrawType = dr.GetIntValue("DrawType");
				vecData[i].nLevel = dr.GetIntValue("Level");
				vecData[i].nShowState = dr.GetIntValue("ShowState");

				vecData[i].fWidget = dr.GetFloatValue("Widget");
				vecData[i].fEffectWid = dr.GetFloatValue("EffectWid");
				vecData[i].fValidLen = dr.GetFloatValue("ValidLen");
				vecData[i].fValidArea = dr.GetFloatValue("ValidArea");
				vecData[i].fEffectType = dr.GetIntValue("EffectType");

				vecData[i].nAreaFormula = dr.GetIntValue("AreaFormula");
				//vecData[i].nShortcutKey = dr.GetIntValue("ShortcutKey");
				vecData[i].nDWKF = dr.GetFloatValue("DWKF");

				vecData[i].dEffectMeasure = dr.GetFloatValue("EffectMeasure");

				if (dr.GetStringValue("ShortcutKey") != NULL)
				{
					strcpy(vecData[i].nShortcutKey, dr.GetStringValue("ShortcutKey"));
				}

			
				if (dr.GetStringValue("DiseaseName") != NULL)
				{
					strcpy(vecData[i].strDiseaseName, dr.GetStringValue("DiseaseName"));
				}

				if (dr.GetStringValue("DisFullName") != NULL)
				{
					strcpy(vecData[i].strDisFullName, dr.GetStringValue("DisFullName"));
				}


				if (dr.GetStringValue("DiseaseTypeName") != NULL)
				{
					strcpy(vecData[i].strDiseaseTypeName, dr.GetStringValue("DiseaseTypeName"));
				}

				if (dr.GetStringValue("DBTableNam") != NULL)
				{
					strcpy(vecData[i].strDBTableName, dr.GetStringValue("DBTableNam"));
				}

				if (dr.GetStringValue("RoadType") != NULL)
				{
					strcpy(vecData[i].strRoadType, dr.GetStringValue("RoadType"));
				}

				if (dr.GetStringValue("SHMD") != NULL)
				{
					strcpy(vecData[i].strSHMD, dr.GetStringValue("SHMD"));
				}

				if (dr.GetStringValue("DXKF") != NULL)
				{
					strcpy(vecData[i].strDXKF, dr.GetStringValue("DXKF"));
				}

				if (dr.GetStringValue("Describe") != NULL)
				{
					strcpy(vecData[i].strDescribe, dr.GetStringValue("Describe"));
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

	// 写入病害设置信息
	bool hnDiseaseSetInfoTable::writeData(vector<hnDiseaseSetInfo>& vecData)
	{
		for (int i = 0; i < vecData.size(); i++)
		{
			writeData(vecData[i]);
		}

		return true;
	}

	// 写入病害设置信息
	bool hnDiseaseSetInfoTable::writeData(hnDiseaseSetInfo& inData)
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
			sprintf(strTableQuery, "select * from %s where ID = %d", DISEASE_SETTING_INFO_TABLE, inData.nID);

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

			// 不存在则添加
			if (dr.RowCount() == 0)
			{
				// 添加CPIIIsql语句
				memset(strTableQuery, 0, SQL_QUERY_LEN);
				sprintf(strTableQuery, "insert into %s(ID, DiseaseIndex,DisFullName, DiseaseType, RoadSurfaceType,DrawType, Level,ShowState,"
					"Widget,EffectType,EffectWid,ValidLen,ValidArea,AreaFormula,ShortcutKey,DWKF,EffectMeasure,DiseaseName,DiseaseTypeName,DBTableNam,RoadType,"
					"SHMD,DXKF,Describe,AddFile1,AddFile2,AddFile3,AddFile4,AddFile5,Remark) "
					"values (?, ?, ?, ?,?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", DISEASE_SETTING_INFO_TABLE);

				// sql命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.nID);
				cmd.BindParam(2, inData.nDiseaseIndex);
				cmd.BindParam(3, inData.strDisFullName);
				cmd.BindParam(4, inData.nDiseaseType);
				cmd.BindParam(5, inData.nRoadSurfaceType);
				cmd.BindParam(6, inData.nDrawType);
				cmd.BindParam(7, inData.nLevel);
				cmd.BindParam(8, inData.nShowState);
				cmd.BindParam(9, inData.fWidget);
				cmd.BindParam(10, inData.fEffectType); //+
				cmd.BindParam(11, inData.fEffectWid);
				cmd.BindParam(12, inData.fValidLen);
				cmd.BindParam(13, inData.fValidArea);
				cmd.BindParam(14, inData.nAreaFormula);
				cmd.BindParam(15, inData.nShortcutKey);
				cmd.BindParam(16, inData.nDWKF);
				cmd.BindParam(17, inData.dEffectMeasure);
				cmd.BindParam(18, inData.strDiseaseName);
				cmd.BindParam(19, inData.strDiseaseTypeName);
				cmd.BindParam(20, inData.strDBTableName);
				cmd.BindParam(21, inData.strRoadType);
				cmd.BindParam(22, inData.strSHMD);
				cmd.BindParam(23, inData.strDXKF);
				cmd.BindParam(24, inData.strDescribe);
				cmd.BindParam(25, inData.strAddFile1);
				cmd.BindParam(26, inData.strAddFile2);
				cmd.BindParam(27, inData.strAddFile3);
				cmd.BindParam(28, inData.strAddFile4);
				cmd.BindParam(29, inData.strAddFile5);
				cmd.BindParam(30, inData.strRemark);

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
					"DiseaseIndex=?,DisFullName=?,DiseaseType=?, RoadSurfaceType=?, DrawType=?, Level=?,ShowState=?,Widget=?,EffectType=?,EffectWid=?,"
					"ValidLen=?,ValidArea=?,AreaFormula=?,ShortcutKey=?,DWKF=?,EffectMeasure=?,DiseaseName=?, DiseaseTypeName=?,DBTableNam=?,RoadType=?,"
					"SHMD=?,DXKF = ?, Describe=?,AddFile1=? ,AddFile2=?,AddFile3=?,AddFile4=?,AddFile5=?,Remark=? where ID = %d", DISEASE_SETTING_INFO_TABLE, inData.nID);

				// cmd命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				
				cmd.BindParam(1, inData.nDiseaseIndex);
				cmd.BindParam(2, inData.strDisFullName); //+
				cmd.BindParam(3, inData.nDiseaseType);
				cmd.BindParam(4, inData.nRoadSurfaceType);
				cmd.BindParam(5, inData.nDrawType);
				cmd.BindParam(6, inData.nLevel);
				cmd.BindParam(7, inData.nShowState);
				cmd.BindParam(8, inData.fWidget);
				cmd.BindParam(9, inData.fEffectType); //+
				cmd.BindParam(10, inData.fEffectWid);
				cmd.BindParam(11, inData.fValidLen);
				cmd.BindParam(12, inData.fValidArea);
				cmd.BindParam(13, inData.nAreaFormula);
				cmd.BindParam(14, inData.nShortcutKey);
				cmd.BindParam(15, inData.nDWKF);
				cmd.BindParam(16, inData.dEffectMeasure);
				cmd.BindParam(17, inData.strDiseaseName);
				cmd.BindParam(18, inData.strDiseaseTypeName);
				cmd.BindParam(19, inData.strDBTableName);
				cmd.BindParam(20, inData.strRoadType);
				cmd.BindParam(21, inData.strSHMD);
				cmd.BindParam(22, inData.strDXKF);
				cmd.BindParam(23, inData.strDescribe);
				cmd.BindParam(24, inData.strAddFile1);
				cmd.BindParam(25, inData.strAddFile2);
				cmd.BindParam(26, inData.strAddFile3);
				cmd.BindParam(27, inData.strAddFile4);
				cmd.BindParam(28, inData.strAddFile5);
				cmd.BindParam(29, inData.strRemark);

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

	// 获取最大id
	int hnDiseaseSetInfoTable::getMaxID()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}

		// 查询表语句
		sprintf(m_strQuery, "select max(ID) from %s", DISEASE_SETTING_INFO_TABLE);

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
