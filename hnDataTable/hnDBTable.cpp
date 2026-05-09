#include "stdafx.h"
#include <algorithm>
#include "hnDBTable.h"

using namespace std;
//namespace hnDataTable
//{
	hnDBTable::hnDBTable()
		:m_nDbVersion(2)
	{
		memset(m_strQuery, 0, SQL_QUERY_LEN);
	}


	hnDBTable::~hnDBTable()
	{
	}

	// 判断第一个字母是否为“s”或“S”
	bool hnDBTable::isValidtyQueryString(const char* strQuery)
	{
		string strWhere = strQuery;

		// 检测是否以S为有效字开头
		for (unsigned int i = 0; i < strWhere.size(); i++)
		{
			if (strWhere[i] == ' ')
			{
				continue;
			}
			if (strWhere[i] == 's' || strWhere[i] == 'S')
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	// 判断第一个字母是否为“w”或“W”
	bool hnDBTable::isValidtyQueryWString(const char* strQuery)
	{
		string strWhere = strQuery;

		// 检测是否以S为有效字开头
		for (unsigned int i = 0; i < strWhere.size(); i++)
		{
			if (strWhere[i] == ' ')
			{
				continue;
			}
			if (strWhere[i] == 'w' || strWhere[i] == 'W')
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}

	// 设置数据库
	void hnDBTable::setDB(const hnSQLite& sqliteDB)
	{
		m_sqliteDB = sqliteDB;
	}

	// 设置数据库版本信息
	void hnDBTable::setDBVersion(const int nVersion)
	{
		m_nDbVersion = nVersion;
	}

	// 执行SQL语句
	bool hnDBTable::executeDB(const char* strQuery)
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

		return true;
	}

	// 获取最大id
	int hnDBTable::getMaxID()
	{
		return true;
	}

	// 解析字符
	bool hnDBTable::AnalysisStr(char* strSource, std::vector<double>& vecValue)
	{
		const char *d = ",";
		//const char *d = " ";
		char *p;
		char* next_token = NULL;
		p = strtok_s(strSource, d, &next_token);

		while (p)
		{
			vecValue.push_back(atof(p));
			p = strtok_s(NULL, d,&next_token);
		}

		// 使用空格
		if (vecValue.size() <= 1)
		{
			vecValue.clear();
			const char* d_space = " ";
			p = NULL;
			p = strtok_s(strSource, d_space, &next_token);

			while (p)
			{
				vecValue.push_back(atof(p));
				p = strtok_s(NULL, d_space, &next_token);
			}
		}

		// 使用table
		if (vecValue.size() <= 1)
		{
			vecValue.clear();
			const char* d_tab = "	";
			p = NULL;
			p = strtok_s(strSource, d_tab,&next_token);

			while (p)
			{
				vecValue.push_back(atof(p));
				p = strtok_s(NULL, d_tab, &next_token);
			}
		}

		return true;
	}

	//备注字段分割 记录二维数据id
	bool hnDBTable::getImageCrackID(const char* strID, vector<int>& vecImageID)
	{
		vecImageID.clear();

		char* strTempID = const_cast<char*>(strID);

		const char *d = ",";
		char *p;
		p = strtok(strTempID, d);
		int nID = 0;
		while (p)
		{
			nID = atoi(p);

			vecImageID.push_back(nID);

			p = strtok(NULL, d);
		}

		if (vecImageID.size() <= 0)
		{
			return false;
		}

		return true;
	}

	// 设置二维病害数据
	bool hnDBTable::setImageCrackID(vector<int>& vecImageID, char* strID, int nErase/* = 0*/)
	{
		if (vecImageID.size() <= 0)
		{
			return false;
		}

		if (nErase == 1)
		{
			sort(vecImageID.begin(), vecImageID.end());
			for (int i = 0; i < vecImageID.size(); i++)
			{
				if (i == vecImageID.size() - 1)
				{
					continue;
				}

				if (vecImageID[i] == vecImageID[i + 1])
				{
					vecImageID.erase(vecImageID.begin() + i);
					--i;
				}
			}
		}

		string str = "";

		for (int i = 0; i < vecImageID.size(); i++)
		{
			memset(strID, 0, 256);

			if (i == 0)
			{
				sprintf(strID, "%d", vecImageID[i]);
			}
			else
			{
				sprintf(strID, "%s,%d", str.c_str(), vecImageID[i]);
			}

			str = strID;
		}

		strcpy(strID, str.c_str());
	}

//}

