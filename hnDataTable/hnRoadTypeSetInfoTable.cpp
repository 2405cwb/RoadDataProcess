#include "stdafx.h"
#include "hnRoadTypeSetInfoTable.h"
#include "time.h"
#include "hnDBDefine.h"

//namespace hnDataTable
//{
	hnRoadTypeSetInfoTable::hnRoadTypeSetInfoTable()
		:hnDBTable()
	{
	}


	hnRoadTypeSetInfoTable::~hnRoadTypeSetInfoTable()
	{
	}

	// 读取病害设置信息
	bool hnRoadTypeSetInfoTable::readData(vector<hnRoadTypeSetInfo>& vecData, char* strQuery/* = NULL*/)
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return false;
		}

		try
		{
			// 查询表语句
			sprintf(m_strQuery, "select * from %s ORDER BY RoadLevel", ROAD_TYPE_SETTING_INFO_TABLE);

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

				vecData[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecData[i].nDrawType = dr.GetIntValue("DrawType");
				//vecData[i].nRoadLevel = dr.GetIntValue("RoadLevel");
				

				vecData[i].dRutThreslodUp = dr.GetFloatValue("RutThreslodUp");//+
				vecData[i].dRutThreslodDown = dr.GetFloatValue("RutThreslodDown");//+
				vecData[i].nRutIndex = dr.GetIntValue("RutIndex");
				
				vecData[i].nRutIndex = dr.GetIntValue("RutIndex");

				vecData[i].dRQI_a0 = dr.GetFloatValue("RQI_a0");
				vecData[i].dRQI_a1 = dr.GetFloatValue("RQI_a1");
				vecData[i].dRQI_w1 = dr.GetFloatValue("RQI_w1");//+
				vecData[i].dRQI_w2 = dr.GetFloatValue("RQI_w2");//+
				vecData[i].dPCI_a0 = dr.GetFloatValue("PCI_a0");
				vecData[i].dPCI_a1 = dr.GetFloatValue("PCI_a1");
				vecData[i].dPQI_WPCI = dr.GetFloatValue("PQI_WPCI");
				vecData[i].dPQI_WRQI = dr.GetFloatValue("PQI_WRQI");
				vecData[i].dPQI_WRDI = dr.GetFloatValue("PQI_WRDI");
				vecData[i].dPQI_WPBI = dr.GetFloatValue("PQI_WPBI");
				vecData[i].dPQI_WPWI = dr.GetFloatValue("PQI_WPWI");
				vecData[i].dRDI_a = dr.GetFloatValue("RDI_a");
				vecData[i].dRDI_b = dr.GetFloatValue("RDI_b");
				vecData[i].dRDI_RDa = dr.GetFloatValue("RDI_RDa");
				vecData[i].dRDI_RDb = dr.GetFloatValue("RDI_RDb");
				vecData[i].dRDI_a0 = dr.GetFloatValue("RDI_a0");
				vecData[i].dRDI_a1 = dr.GetFloatValue("RDI_a1");
				vecData[i].dPWI_a0 = dr.GetFloatValue("PWI_a0");
				vecData[i].dPWI_a1 = dr.GetFloatValue("PWI_a1");
				vecData[i].dMQI_WSCI = dr.GetFloatValue("MQI_WSCI");
				vecData[i].dMQI_WPQI = dr.GetFloatValue("MQI_WPQI");
				vecData[i].dMQI_WBCI = dr.GetFloatValue("MQI_WBCI");
				vecData[i].dMQI_WTCI = dr.GetFloatValue("MQI_WTCI");
				if (dr.GetStringValue("RoadLevel") != NULL)
				{
					strcpy(vecData[i].nRoadLevel, dr.GetStringValue("RoadLevel"));

				}
				if (dr.GetStringValue("RealV") != NULL)
				{//+
					strcpy(vecData[i].strRealV, dr.GetStringValue("RealV"));

				}	
				if (dr.GetStringValue("RoadFullName") != NULL)
				{//+
					strcpy(vecData[i].strRoadFullName, dr.GetStringValue("RoadFullName"));

				}
				if (dr.GetStringValue("AmendPara") != NULL)
				{//+
					strcpy(vecData[i].strAmendPara, dr.GetStringValue("AmendPara"));

				}
				if (dr.GetStringValue("SN_wi") != NULL)
				{
					strcpy(vecData[i].strSN_wi, dr.GetStringValue("SN_wi"));
				}

				if (dr.GetStringValue("LQ_wi") != NULL)
				{
					strcpy(vecData[i].strLQ_wi, dr.GetStringValue("LQ_wi"));
				}

				if (dr.GetStringValue("RQILevel") != NULL)
				{
					strcpy(vecData[i].strRQILevel, dr.GetStringValue("RQILevel"));
				}

				if (dr.GetStringValue("RDILevel") != NULL)
				{
					strcpy(vecData[i].strRDILevel, dr.GetStringValue("RDILevel"));
				}

				if (dr.GetStringValue("PWILevel") != NULL)
				{
					strcpy(vecData[i].strPWILevel, dr.GetStringValue("PWILevel"));
				}

				if (dr.GetStringValue("MTDLevel") != NULL)
				{
					strcpy(vecData[i].strMTDLevel, dr.GetStringValue("MTDLevel"));
				}

				if (dr.GetStringValue("IRILevel") != NULL)
				{
					strcpy(vecData[i].strIRILevel, dr.GetStringValue("IRILevel"));
				}

				if (dr.GetStringValue("PCILevel") != NULL)
				{
					strcpy(vecData[i].strPCILevel, dr.GetStringValue("PCILevel"));
				}

				if (dr.GetStringValue("PQILevel") != NULL)
				{
					strcpy(vecData[i].strPQILevel, dr.GetStringValue("PQILevel"));
				}

				if (dr.GetStringValue("PBILevel") != NULL)
				{
					strcpy(vecData[i].strPBILevel, dr.GetStringValue("PBILevel"));
				}

				if (dr.GetStringValue("MQILevel") != NULL)
				{
					strcpy(vecData[i].strMQILevel, dr.GetStringValue("MQILevel"));
				}

				if (dr.GetStringValue("PBI_KFBZ") != NULL)
				{
					strcpy(vecData[i].strPBI_KFBZ, dr.GetStringValue("PBI_KFBZ"));
				}

				if (dr.GetStringValue("PBI_KF") != NULL)
				{
					strcpy(vecData[i].strPBI_KF, dr.GetStringValue("PBI_KF"));
				}

				if (dr.GetStringValue("RoadType") != NULL)
				{
					strcpy(vecData[i].strRoadType, dr.GetStringValue("RoadType"));
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
	bool hnRoadTypeSetInfoTable::writeData(vector<hnRoadTypeSetInfo>& vecData)
	{
		for (int i = 0; i < vecData.size(); i++)
		{
			writeData(vecData[i]);
		}

		return true;
	}

	// 写入病害设置信息
	bool hnRoadTypeSetInfoTable::writeData(hnRoadTypeSetInfo& inData)
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
			sprintf(strTableQuery, "select * from %s where ID = %d", ROAD_TYPE_SETTING_INFO_TABLE, inData.nID);

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

			// 不存在则添加
			if (dr.RowCount() == 0)
			{
				
				// 添加CPIIIsql语句
				memset(strTableQuery, 0, SQL_QUERY_LEN);
				sprintf(strTableQuery, "insert into %s(ID, RSurfaceType,RoadFullName, DrawType, RoadLevel,RutThreslodUp,RutThreslodDown, RutIndex,RealV,AmendPara,RQI_a0,RQI_a1,RQI_w1,RQI_w2,"
					"PCI_a0,PCI_a1,PQI_WPCI,PQI_WRQI,PQI_WRDI,PQI_WPBI,PQI_WPWI,RDI_a,RDI_b,RDI_RDa,RDI_RDb,RDI_a0,RDI_a1,PWI_a0,PWI_a1,MQI_WSCI,"
					"MQI_WPQI,MQI_WBCI,MQI_WTCI,SN_wi,LQ_wi,RQILevel,RDILevel,PWILevel,MTDLevel,IRILevel,PCILevel,PQILevel,PBILevel,MQILevel,"
					"PBI_KFBZ,PBI_KF,RoadType, AddFile1,AddFile2,AddFile3,AddFile4,AddFile5,Remark) "
					"values (?, ?, ?,?,?,?,?,?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", ROAD_TYPE_SETTING_INFO_TABLE);

				// sql命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1 , inData.nID);
				cmd.BindParam(2, inData.nRSurfaceType);
				cmd.BindParam(3, inData.strRoadFullName); //+
				cmd.BindParam(4, inData.nDrawType);
				cmd.BindParam(5, inData.nRoadLevel);
				cmd.BindParam(6, inData.dRutThreslodUp); //+
				cmd.BindParam(7, inData.dRutThreslodDown);//+
				cmd.BindParam(8, inData.nRutIndex);
				cmd.BindParam(9, inData.strRealV);  //+
				cmd.BindParam(10, inData.strAmendPara); //+
				cmd.BindParam(11, inData.dRQI_a0);
				cmd.BindParam(12, inData.dRQI_a1);
				cmd.BindParam(13, inData.dRQI_w1);   //+
				cmd.BindParam(14, inData.dRQI_w2);	//+
				cmd.BindParam(15, inData.dPCI_a0);
				cmd.BindParam(16, inData.dPCI_a1);
				cmd.BindParam(17, inData.dPQI_WPCI);
				cmd.BindParam(18, inData.dPQI_WRQI);
				cmd.BindParam(19, inData.dPQI_WRDI);
				cmd.BindParam(20, inData.dPQI_WPBI);
				cmd.BindParam(21, inData.dPQI_WPWI);
				cmd.BindParam(22, inData.dRDI_a);
				cmd.BindParam(23, inData.dRDI_b);
				cmd.BindParam(24, inData.dRDI_RDa);
				cmd.BindParam(25, inData.dRDI_RDb);
				cmd.BindParam(26, inData.dRDI_a0);
				cmd.BindParam(27, inData.dRDI_a1);
				cmd.BindParam(28, inData.dPWI_a0);
				cmd.BindParam(29, inData.dPWI_a1);
				cmd.BindParam(30, inData.dMQI_WSCI);
				cmd.BindParam(31, inData.dMQI_WPQI);
				cmd.BindParam(32, inData.dMQI_WBCI);
				cmd.BindParam(33, inData.dMQI_WTCI);
				cmd.BindParam(34, inData.strSN_wi);
				cmd.BindParam(35, inData.strLQ_wi);
				cmd.BindParam(36, inData.strRQILevel);
				cmd.BindParam(37, inData.strRDILevel);
				cmd.BindParam(38, inData.strPWILevel);
				cmd.BindParam(39, inData.strMTDLevel);
				cmd.BindParam(40, inData.strIRILevel);
				cmd.BindParam(41, inData.strPCILevel);
				cmd.BindParam(42, inData.strPQILevel);
				cmd.BindParam(43, inData.strPBILevel);
				cmd.BindParam(44, inData.strMQILevel);
				cmd.BindParam(45, inData.strPBI_KFBZ);
				cmd.BindParam(46, inData.strPBI_KF);
				cmd.BindParam(47, inData.strRoadType);
				cmd.BindParam(48, inData.strAddFile1);
				cmd.BindParam(49, inData.strAddFile2);
				cmd.BindParam(50, inData.strAddFile3);
				cmd.BindParam(51, inData.strAddFile4);
				cmd.BindParam(52, inData.strAddFile5);
				cmd.BindParam(53, inData.strRemark);

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
					"RSurfaceType=?, RoadFullName=?,DrawType=?, RoadLevel=?, RutThreslodUp=?,RutThreslodDown=?, RutIndex=?,RealV=?,AmendPara=?,RQI_a0=?,RQI_a1=?,RQI_w1=?,RQI_w2=?,PCI_a0=?,PCI_a1=?,"
					"PQI_WPCI=?,PQI_WRQI=?,PQI_WRDI=?,PQI_WPBI=?,PQI_WPWI=?,RDI_a=?,RDI_b=?, RDI_RDa=?,RDI_RDb=?,RDI_a0=?,RDI_a1=?,PWI_a0=?,"
					"PWI_a1=?,MQI_WSCI = ?,MQI_WPQI=?,MQI_WBCI=?,MQI_WTCI=?,SN_wi=?,LQ_wi=?,RQILevel=?,RDILevel=?,PWILevel=?,MTDLevel=?,IRILevel=?,"
					"PCILevel=?,PQILevel=?,PBILevel=?,MQILevel=?,PBI_KFBZ=?,PBI_KF=?,RoadType=?, AddFile1=? ,AddFile2=?,AddFile3=?,AddFile4=?,AddFile5=?,Remark=? where ID = %d", ROAD_TYPE_SETTING_INFO_TABLE, inData.nID);

				// cmd命令
				hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

				// 绑定参数
				cmd.BindParam(1, inData.nRSurfaceType);
				cmd.BindParam(2, inData.strRoadFullName); //+
				cmd.BindParam(3, inData.nDrawType);
				cmd.BindParam(4, inData.nRoadLevel);
				cmd.BindParam(5, inData.dRutThreslodUp); //+
				cmd.BindParam(6, inData.dRutThreslodDown);//+
				cmd.BindParam(7, inData.nRutIndex);
				cmd.BindParam(8, inData.strRealV);  //+
				cmd.BindParam(9, inData.strAmendPara); //+
				cmd.BindParam(10, inData.dRQI_a0);
				cmd.BindParam(11, inData.dRQI_a1);
				cmd.BindParam(12, inData.dRQI_w1);   //+
				cmd.BindParam(13, inData.dRQI_w2);	//+
				cmd.BindParam(14, inData.dPCI_a0);
				cmd.BindParam(15, inData.dPCI_a1);
				cmd.BindParam(16, inData.dPQI_WPCI);
				cmd.BindParam(17, inData.dPQI_WRQI);
				cmd.BindParam(18, inData.dPQI_WRDI);
				cmd.BindParam(19, inData.dPQI_WPBI);
				cmd.BindParam(20, inData.dPQI_WPWI);
				cmd.BindParam(21, inData.dRDI_a);
				cmd.BindParam(22, inData.dRDI_b);
				cmd.BindParam(23, inData.dRDI_RDa);
				cmd.BindParam(24, inData.dRDI_RDb);
				cmd.BindParam(25, inData.dRDI_a0);
				cmd.BindParam(26, inData.dRDI_a1);
				cmd.BindParam(27, inData.dPWI_a0);
				cmd.BindParam(28, inData.dPWI_a1);
				cmd.BindParam(29, inData.dMQI_WSCI);
				cmd.BindParam(30, inData.dMQI_WPQI);
				cmd.BindParam(31, inData.dMQI_WBCI);
				cmd.BindParam(32, inData.dMQI_WTCI);
				cmd.BindParam(33, inData.strSN_wi);
				cmd.BindParam(34, inData.strLQ_wi);
				cmd.BindParam(35, inData.strRQILevel);
				cmd.BindParam(36, inData.strRDILevel);
				cmd.BindParam(37, inData.strPWILevel);
				cmd.BindParam(38, inData.strMTDLevel);
				cmd.BindParam(39, inData.strIRILevel);
				cmd.BindParam(40, inData.strPCILevel);
				cmd.BindParam(41, inData.strPQILevel);
				cmd.BindParam(42, inData.strPBILevel);
				cmd.BindParam(43, inData.strMQILevel);
				cmd.BindParam(44, inData.strPBI_KFBZ);
				cmd.BindParam(45, inData.strPBI_KF);
				cmd.BindParam(46, inData.strRoadType);
				cmd.BindParam(47, inData.strAddFile1);
				cmd.BindParam(48, inData.strAddFile2);
				cmd.BindParam(49, inData.strAddFile3);
				cmd.BindParam(50, inData.strAddFile4);
				cmd.BindParam(51, inData.strAddFile5);
				cmd.BindParam(52, inData.strRemark);
				
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
	int hnRoadTypeSetInfoTable::getMaxID()
	{
		// 判断数据库是否连接成功
		if (!m_sqliteDB.IsOpen())
		{
			return 1;
		}

		// 查询表语句
		sprintf(m_strQuery, "select max(ID) from %s", ROAD_TYPE_SETTING_INFO_TABLE);

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
