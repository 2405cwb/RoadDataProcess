#include "stdafx.h"
#include "hnDBSqlite.h"
#include "../hnCommon/hnRect.h"
#include <QtMath>
#include "hnDBDefine.h"
#include <algorithm>
#include <cmath>
#include <set>
#include "time.h"
#include <io.h>
using namespace std;

//namespace hnDataTable
//{
	hnDBSqlite::hnDBSqlite(const char* strDataSource)
	{
		strcpy(m_strDataSource, strDataSource);
		memset(m_strQuery, 0, SQL_QUERY_LEN);
		m_bOpen = false;	
	}

	hnDBSqlite::~hnDBSqlite()
	{
	}

	// 连接数据库
	bool hnDBSqlite::connectDB(const vector<string>& vecDiseaseTable)
	{
		m_vecDiseaseTable = vecDiseaseTable;
		// 自定义景观使用独立表，旧工程打开时补建，不修改标准参数库。
		if (std::find(m_vecDiseaseTable.begin(), m_vecDiseaseTable.end(), "UserStreetDisease") == m_vecDiseaseTable.end())
			m_vecDiseaseTable.push_back("UserStreetDisease");

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

				// 创建表
				createTable(vecDiseaseTable);
				if (!m_sqliteDB.ExcuteNonQuery("CREATE TABLE IF NOT EXISTS UserStreetDisease(ID int default -1 primary key,Mileage double default 0,RoadStandard varchar(1024),RSurfaceType int default 0,DrawType int default 0,Level int default 0,Length double default 0.0,Width double default 0.0,Area double default 0.0,Depth double default 0.0,PixelLen int default 0,PixelWid int default 0,RealLen double default 0.0,ReaWidth double default 0.0,VecRect blob default null,RectCnt int default 0,Vec3dPoint blob default null,n3dCnt int default 0,GpsTimer blob default null,GpsTimerCnt int default 0,Remark varchar(1024),DiseaseTableName varchar(1024),DiseaseType int default 0,Weight double default 1.0,DisName varchar(1024),AddFile4 varchar(1024),AddFile5 varchar(1024),Dmi double default 0.0,DmiUp double default 0.0,DmiDown double default 0.0,RoadWidth double default 0.0);"))
				{
					m_bOpen = false;
					return false;
				}
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
	int hnDBSqlite::createTable(const vector<string>& vecDiseaseTable)
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
				HN_CREATE_RESULT_TABLE tableCmd;
				vector<string> vecCmds = tableCmd.vecCmds;

				char strCmd[SQL_QUERY_LEN] = { 0 };

				// 创建病害表
				for (int i = 0; i < vecDiseaseTable.size(); i++)
				{
					// Mile,RoadWidth
					memset(strCmd, 0, SQL_QUERY_LEN);
					sprintf(strCmd, "Create Table %s("
						"ID int default -1 primary key,"
						"Mileage double default 0,"
						"RoadStandard varchar(%d),"
						"RSurfaceType int default 0,"
						"DrawType int default 0,"
						"Level int default 0,"
						"Length double default 0.0,"
						"Width double default 0.0,"
						"Area double default 0.0,"
						"Depth double default 0.0,"
						"PixelLen int default 0,"
						"PixelWid int default 0,"
						"RealLen double default 0.0,"
						"ReaWidth double default 0.0,"
						"VecRect blob default null,"
						"RectCnt int default 0,"
						"Vec3dPoint blob default null,"
						"n3dCnt int default 0,"
						"GpsTimer blob default null,"
						"GpsTimerCnt int default 0,"
						"Remark varchar(%d),"
						"DiseaseTableName varchar(%d),"
						"DiseaseType int default 0,"
						"Weight double default 1.0,"
						"DisName varchar(%d),"
						"AddFile4 varchar(%d),"
						"AddFile5 varchar(%d),"
						"Dmi double default 0.0,"
						"DmiUp double default 0.0,"
						"DmiDown double default 0.0,"
						"RoadWidth double default 0.0);",
						vecDiseaseTable[i].c_str(), SQL_ADDFILE_LEN,   SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, 
						SQL_ADDFILE_LEN, SQL_ADDFILE_LEN, SQL_ADDFILE_LEN);

					vecCmds.push_back(strCmd);
				}

				// 遍历所有的建表语句
				for (unsigned int i = 0; i < vecCmds.size(); i++)
				{
					// 执行语句
					string strCmd = vecCmds[i];
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
	bool hnDBSqlite::executeDB(const char* strQuery)
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

	// 统一校验相对里程数据，并在一个事务中完成设置、校桩和打标写入。
	bool hnDBSqlite::saveRelativeMileageData(const hnProjectSetInfo* projectSettings,
		const vector<hnMilePile>& milePiles, const vector<hnMarkInfo>& marks,
		bool replaceMarks, double projectStartMile, double projectEndMile,
		double projectLength, QString* errorMessage)
	{
		const double valueTolerance = 0.001;
		const double trueMileTolerance = 1.0;
		if (!m_sqliteDB.IsOpen() || !std::isfinite(projectStartMile) ||
			!std::isfinite(projectEndMile) || !std::isfinite(projectLength) || projectLength <= 0.0)
		{
			if (errorMessage) *errorMessage = QStringLiteral("数据库未打开或工程里程设置无效。");
			return false;
		}

		const double minTrueMile = qMin(projectStartMile, projectEndMile);
		const double maxTrueMile = qMax(projectStartMile, projectEndMile);
		int startAnchorCount = 0;
		int endAnchorCount = 0;
		std::set<int> pileIds;
		for (const hnMilePile& pile : milePiles)
		{
			if (!std::isfinite(pile.dTrueMile) || !std::isfinite(pile.dEnclMile) ||
				pile.dTrueMile < minTrueMile - trueMileTolerance ||
				pile.dTrueMile > maxTrueMile + trueMileTolerance ||
				pile.dEnclMile < -valueTolerance || pile.dEnclMile > projectLength + valueTolerance ||
				!pileIds.insert(pile.nID).second)
			{
				if (errorMessage) *errorMessage = QStringLiteral("校桩存在越界数值或重复 ID，已拒绝写入数据库。");
				return false;
			}
			if (qAbs(pile.dTrueMile - projectStartMile) <= trueMileTolerance && qAbs(pile.dEnclMile) <= valueTolerance)
				++startAnchorCount;
			if (qAbs(pile.dTrueMile - projectEndMile) <= trueMileTolerance &&
				qAbs(pile.dEnclMile - projectLength) <= valueTolerance)
				++endAnchorCount;
		}
		if (startAnchorCount != 1 || endAnchorCount != 1)
		{
			if (errorMessage) *errorMessage = QStringLiteral("校桩表必须且只能包含一个标准起点和一个标准终点，已拒绝写入数据库。");
			return false;
		}

		if (replaceMarks)
		{
			std::set<int> markIds;
			for (const hnMarkInfo& mark : marks)
			{
				if (!std::isfinite(mark.dTrueMile) || !std::isfinite(mark.dEnclMile) ||
					mark.dTrueMile < minTrueMile - trueMileTolerance ||
					mark.dTrueMile > maxTrueMile + trueMileTolerance ||
					mark.dEnclMile < -valueTolerance || mark.dEnclMile > projectLength + valueTolerance ||
					!markIds.insert(mark.nID).second)
				{
					if (errorMessage) *errorMessage = QStringLiteral("打标存在越界数值或重复 ID，已拒绝写入数据库。");
					return false;
				}
			}
		}

		sqlite3* db = m_sqliteDB.getDb();
		char* sqliteError = nullptr;
		if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &sqliteError) != SQLITE_OK)
		{
			if (errorMessage)
				*errorMessage = sqliteError ? QString::fromLocal8Bit(sqliteError) : QStringLiteral("无法开始数据库事务。");
			if (sqliteError) sqlite3_free(sqliteError);
			return false;
		}

		bool success = true;
		try
		{
			if (projectSettings)
			{
				hnProjectSetInfo settings = *projectSettings;
				success = m_projectSetTable.writeData(settings);
			}
			vector<hnMilePile> piles = milePiles;
			if (success) success = m_milePileTable.clearData() && m_milePileTable.writeData(piles);
			if (success && replaceMarks)
			{
				vector<hnMarkInfo> finalMarks = marks;
				success = m_markerInfoTable.clearData() && m_markerInfoTable.writeData(finalMarks);
			}
		}
		catch (...)
		{
			success = false;
		}

		if (!success || sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &sqliteError) != SQLITE_OK)
		{
			sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
			if (errorMessage)
				*errorMessage = sqliteError ? QString::fromLocal8Bit(sqliteError) :
					QStringLiteral("校桩打标写入失败，数据库事务已经回滚。");
			if (sqliteError) sqlite3_free(sqliteError);
			return false;
		}
		return true;
	}

	// 设置参数
	void hnDBSqlite::setParam()
	{
		// 工程信息设置表
		m_projectSetTable.setDB(m_sqliteDB);

		// 打标信息表
		m_markerInfoTable.setDB(m_sqliteDB);

		// 里程校桩表
		m_milePileTable.setDB(m_sqliteDB);

		// 控制点表
		m_ctrlPointTable.setDB(m_sqliteDB);

		m_mileInfoTable.setDB(m_sqliteDB);

		// 关联数库
		m_diseaseTable.setDB(m_sqliteDB);
		m_diseaseTable.setDiseaseTableName(m_vecDiseaseTable);
	}


	hnRoadDiseaseTable* hnDBSqlite::getDiseaseTable()
	{
		return &m_diseaseTable;
	}

	CtrlPointTable* hnDBSqlite::getCtrlPointTable()
	{
		return &m_ctrlPointTable;
	}

	//是否为空
	bool hnDBSqlite::isOpen()
	{
		if (m_sqliteDB.IsOpen())
		{
			return true;
		}

		return false;
	}

	//获得db路径
	char* hnDBSqlite::getDBPath()
	{
		return m_strDataSource;
	}

//}

bool hnDBSqlite::backupDatabase(const QString& path)
{
    sqlite3* target = NULL;
    const QByteArray name = path.toUtf8();
    if (sqlite3_open(name.constData(), &target) != SQLITE_OK)
    {
        if (target) sqlite3_close(target);
        return false;
    }
    sqlite3_backup* backup = sqlite3_backup_init(target, "main", m_sqliteDB.getDb(), "main");
    if (!backup)
    {
        sqlite3_close(target);
        return false;
    }
    const int step = sqlite3_backup_step(backup, -1);
    const int finish = sqlite3_backup_finish(backup);
    const int close = sqlite3_close(target);
    return step == SQLITE_DONE && finish == SQLITE_OK && close == SQLITE_OK;
}
