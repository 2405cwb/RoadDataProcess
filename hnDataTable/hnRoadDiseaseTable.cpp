#include "stdafx.h"
#include "hnRoadDiseaseTable.h"
#include "time.h"
#include <QDebug>
#include <QProgressDialog>
#include <QSize>
#include <QApplication>
#include <QMessageBox>
#include <algorithm>
#include "../hnQtCommon/MyCommonMethods.h" 
//namespace hnDataTable
//{
hnRoadDiseaseTable::hnRoadDiseaseTable()
	:hnDBTable()
{
}


hnRoadDiseaseTable::~hnRoadDiseaseTable()
{
}

void hnRoadDiseaseTable::debugIsDBOpen()
{
	qDebug() << "[hnRoadDiseaseTable]"
		<<   ( m_sqliteDB.IsOpen() == true ? "true" : "false");
	 
}

void * hnRoadDiseaseTable::debugDbPtr()
{
	return m_sqliteDB.getDb();
}

// 设置病害表名称列表
void hnRoadDiseaseTable::setDiseaseTableName(vector<string>& vecDiseaseTableName)
{
	m_vecDiseaseTableName = vecDiseaseTableName;
}

bool hnRoadDiseaseTable::readRoadDiseaseData_Service(const hnProjectSetInfo& setInfo , const QVector<hnMile>& miles, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo, double roadYDistance)
{
	int line = setInfo.nLineType;
	roadYDistance = qRound(roadYDistance * 100) / 100.0;//保留两位小数
														// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	if (miles.size() < 1)
	{
		return false;
	}
	QVector<hnMile> Qmiles;
	double firstStartMile = miles.first().dTrueMile; 
	for (auto n : miles)
	{ 
		Qmiles.push_back(n);
	}

	qSort(Qmiles.begin(), Qmiles.end());
	double dmiStart = Qmiles.first().dEnclMile - roadYDistance;  //下界
	double dmiEnd = Qmiles.last().dEnclMile + roadYDistance;   //上界
	
	try
	{

		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		vector<hnRoadDiseaseInfo> vecTemp;
		
		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
#if 0
		
			if ("龟裂" == m_vecDiseaseTableName.at(j))
			{
				QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
				qDebug() << myStr;
			}
#endif

			//病害上界 DmiUp (大里程) ， DmiDown 病害下界  (小里程)
			//QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
			QString standard = QString::fromLocal8Bit(setInfo.strRoadStandard);
			// 查询表语句

			QString   strQuery = QString("select * from %3 where"
				"((DmiDown<=%1 AND DmiUp>=%2) OR"
				"(DmiDown<=%1 AND DmiUp>=%1) OR"
				"(DmiDown>=%1 AND DmiUp<=%2) OR"
				"(DmiDown<=%2 AND DmiUp>=%2)) AND DiseaseType=0 AND RoadWidth =%4 "
				"AND RoadStandard ='%5' AND DrawType=%6 AND AddFile4!='2' AND AddFile4!='3'")
				.arg(dmiStart)
				.arg(dmiEnd)
				.arg(QString::fromStdString(m_vecDiseaseTableName.at(j)))
				.arg(QString::number(setInfo.dRoadWidth))
				.arg(standard).arg(setInfo.nDrawType);
			auto str0 = MyCommonMethods::QstringToChar(strQuery);
		 
			qsnprintf(m_strQuery, sizeof(m_strQuery), "%s",str0.c_str());
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 病害数据
			vecTemp.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecTemp[i].nID = dr.GetIntValue("ID");
				vecTemp[i].dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(vecTemp[i].strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				vecTemp[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecTemp[i].nDrawType = dr.GetIntValue("DrawType");
				vecTemp[i].nLevel = dr.GetIntValue("Level");
				vecTemp[i].dLength = dr.GetFloatValue("Length");
				vecTemp[i].dWidth = dr.GetFloatValue("Width");
				vecTemp[i].dArea = dr.GetFloatValue("Area");
				vecTemp[i].dDepth = dr.GetFloatValue("Depth");
				vecTemp[i].nPixelLen = dr.GetIntValue("PixelLen");
				vecTemp[i].nPixelWid = dr.GetIntValue("PixelWid");
				vecTemp[i].dRealLen = dr.GetFloatValue("RealLen");
				vecTemp[i].dReaWidth = dr.GetFloatValue("ReaWidth");


				vecTemp[i].vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(vecTemp[i].vec2dRect._Myfirst(), pBuffer, nBlobLen);


				vecTemp[i].vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(vecTemp[i].vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				vecTemp[i].vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(vecTemp[i].vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecTemp[i].strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(vecTemp[i].strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}



				vecTemp[i].ndiseaseType = dr.GetIntValue("DiseaseType");

				vecTemp[i].diseaseWeight = dr.GetFloatValue("Weight");

				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(vecTemp[i].strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecTemp[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecTemp[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				vecTemp[i].dRoadWidth = dr.GetFloatValue("RoadWidth");
			 
				vecTemp[i].dDmi = dr.GetFloatValue("Dmi");
				vecTemp[i].dDmiEnd = dr.GetFloatValue("DmiUp");
				vecTemp[i].dDmiStart = dr.GetFloatValue("DmiDown");
				i++;
			}

			dr.Close();

			allDatas.insert(allDatas.end(), vecTemp.begin(), vecTemp.end());
		}

		vecData.clear();
		FilterOutDisrase(allDatas, vecData, markinfo, setInfo,false);
	 
	}
	catch (...)
	{
		// 抛出异常
		throw exception(m_sqliteDB.GetLastErrorMsg());
		return false;
	}

	return true;
}

bool hnRoadDiseaseTable::readRoadDiseaseData_Service(const hnProjectSetInfo& setInfo, double dmiStart, double dmiEnd, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo, double roadYDistance)
{
	int line = setInfo.nLineType;
	roadYDistance = qRound(roadYDistance * 100) / 100.0;//保留两位小数
														// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	  
	 
   dmiStart = dmiStart - roadYDistance;  //下界
  dmiEnd = dmiEnd + roadYDistance;   //上界

	try
	{

		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		vector<hnRoadDiseaseInfo> vecTemp;

		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
#if 0

			if ("龟裂" == m_vecDiseaseTableName.at(j))
			{
				QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
				qDebug() << myStr;
			}
#endif

			//病害上界 DmiUp (大里程) ， DmiDown 病害下界  (小里程)
			//QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
			QString standard = QString::fromLocal8Bit(setInfo.strRoadStandard);
			// 查询表语句

			QString   strQuery = QString("select * from %3 where"
				"((DmiDown<=%1 AND DmiUp>=%2) OR"
				"(DmiDown<=%1 AND DmiUp>=%1) OR"
				"(DmiDown>=%1 AND DmiUp<=%2) OR"
				"(DmiDown<=%2 AND DmiUp>=%2)) AND DiseaseType=0 AND RoadWidth =%4 "
				"AND RoadStandard ='%5' AND DrawType=%6 AND AddFile4!='2' AND AddFile4!='3'")
				.arg(dmiStart)
				.arg(dmiEnd)
				.arg(QString::fromStdString(m_vecDiseaseTableName.at(j)))
				.arg(QString::number(setInfo.dRoadWidth))
				.arg(standard).arg(setInfo.nDrawType);
			auto str0 = MyCommonMethods::QstringToChar(strQuery);

			qsnprintf(m_strQuery, sizeof(m_strQuery), "%s", str0.c_str());
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 病害数据
			vecTemp.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecTemp[i].nID = dr.GetIntValue("ID");
				vecTemp[i].dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(vecTemp[i].strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				vecTemp[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecTemp[i].nDrawType = dr.GetIntValue("DrawType");
				vecTemp[i].nLevel = dr.GetIntValue("Level");
				vecTemp[i].dLength = dr.GetFloatValue("Length");
				vecTemp[i].dWidth = dr.GetFloatValue("Width");
				vecTemp[i].dArea = dr.GetFloatValue("Area");
				vecTemp[i].dDepth = dr.GetFloatValue("Depth");
				vecTemp[i].nPixelLen = dr.GetIntValue("PixelLen");
				vecTemp[i].nPixelWid = dr.GetIntValue("PixelWid");
				vecTemp[i].dRealLen = dr.GetFloatValue("RealLen");
				vecTemp[i].dReaWidth = dr.GetFloatValue("ReaWidth");


				vecTemp[i].vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(vecTemp[i].vec2dRect._Myfirst(), pBuffer, nBlobLen);


				vecTemp[i].vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(vecTemp[i].vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				vecTemp[i].vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(vecTemp[i].vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecTemp[i].strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(vecTemp[i].strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}



				vecTemp[i].ndiseaseType = dr.GetIntValue("DiseaseType");

				vecTemp[i].diseaseWeight = dr.GetFloatValue("Weight");

				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(vecTemp[i].strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecTemp[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecTemp[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				vecTemp[i].dRoadWidth = dr.GetFloatValue("RoadWidth");

				vecTemp[i].dDmi = dr.GetFloatValue("Dmi");
				vecTemp[i].dDmiEnd = dr.GetFloatValue("DmiUp");
				vecTemp[i].dDmiStart = dr.GetFloatValue("DmiDown");
				i++;
			}

			dr.Close();

			allDatas.insert(allDatas.end(), vecTemp.begin(), vecTemp.end());
		}

		vecData.clear();
		FilterOutDisrase(allDatas, vecData, markinfo, setInfo, false);

	}
	catch (...)
	{
		// 抛出异常
		throw exception(m_sqliteDB.GetLastErrorMsg());
		return false;
	}

	return true;
}

bool hnRoadDiseaseTable::read3dRoadDiseaseData_Service(const QString& standard, double dmiStart, double dmiEnd, vector<hnRoadDiseaseInfo>&vecData)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	} 
	try
	{
		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;
		int i = 0;
		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;
		vector<hnRoadDiseaseInfo> vecTemp;
		double temp;
		if (dmiStart>dmiEnd)
		{
			temp = dmiStart;
			dmiStart = dmiEnd;
			dmiEnd = temp;
		}

		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
			//病害上界 DmiUp (大里程) ， DmiDown 病害下界  (小里程)
			// 查询表语句
			QString   strQuery = QString("select * from %3 where"
				"(DmiDown<=%1 AND DmiUp>=%2) OR"
				"(DmiDown<=%1 AND DmiUp>=%1) OR"
				"(DmiDown>=%1 AND DmiUp<=%2) OR"
				"(DmiDown<=%2 AND DmiUp>=%2) AND RoadStandard ='%4' AND AddFile4!='2' AND AddFile4!='3'")
				.arg(dmiStart).
				arg(dmiEnd).
				arg(QString::fromStdString(m_vecDiseaseTableName.at(j)))
				.arg(standard);
			//	sprintf(m_strQuery, strQuery.toLocal8Bit().data());
			auto str0 = MyCommonMethods::QstringToChar(strQuery);
			 
			qsnprintf(m_strQuery, sizeof(m_strQuery), "%s", str0.c_str());
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 病害数据
			vecTemp.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecTemp[i].nID = dr.GetIntValue("ID");
				vecTemp[i].dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(vecTemp[i].strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				vecTemp[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecTemp[i].nDrawType = dr.GetIntValue("DrawType");
				vecTemp[i].nLevel = dr.GetIntValue("Level");
				vecTemp[i].dLength = dr.GetFloatValue("Length");
				vecTemp[i].dWidth = dr.GetFloatValue("Width");
				vecTemp[i].dArea = dr.GetFloatValue("Area");
				vecTemp[i].dDepth = dr.GetFloatValue("Depth");
				vecTemp[i].nPixelLen = dr.GetIntValue("PixelLen");
				vecTemp[i].nPixelWid = dr.GetIntValue("PixelWid");
				vecTemp[i].dRealLen = dr.GetFloatValue("RealLen");
				vecTemp[i].dReaWidth = dr.GetFloatValue("ReaWidth");


				vecTemp[i].vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(vecTemp[i].vec2dRect._Myfirst(), pBuffer, nBlobLen);


				vecTemp[i].vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(vecTemp[i].vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				vecTemp[i].vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(vecTemp[i].vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecTemp[i].strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(vecTemp[i].strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}
				vecTemp[i].ndiseaseType = dr.GetIntValue("DiseaseType");
				vecTemp[i].diseaseWeight = dr.GetFloatValue("Weight");
				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(vecTemp[i].strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecTemp[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecTemp[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				vecTemp[i].dRoadWidth = dr.GetFloatValue("RoadWidth");
				vecTemp[i].dDmi = dr.GetFloatValue("Dmi");
				vecTemp[i].dDmiEnd = dr.GetFloatValue("DmiUp");
				vecTemp[i].dDmiStart = dr.GetFloatValue("DmiDown");

				i++;
			}

			dr.Close();

			allDatas.insert(allDatas.end(), vecTemp.begin(), vecTemp.end());
		}
		//根据打标情况过滤病害


		vecData.clear();
		for (int i = 0; i < allDatas.size(); ++i)
		{
			vecData.push_back(allDatas.at(i));
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

bool hnRoadDiseaseTable::readStreetData_Service(const QString& standard, const QVector<hnMile>& miles, QVector<hnRoadDiseaseInfo>&vecData, int line, double roadYDistance)
{
	roadYDistance = qRound(roadYDistance * 100) / 100.0;//保留两位小数
														// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	if (miles.size() < 1)
	{
		return false;
	}

	QVector<hnMile> Qmiles;

	for (auto n : miles)
	{
		Qmiles.push_back(n);
	}
	qSort(Qmiles.begin(), Qmiles.end());
	double sMile = Qmiles.first().dEnclMile;
	double eMile = Qmiles.last().dEnclMile + roadYDistance;
	 
	try
	{

		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		vector<hnRoadDiseaseInfo> vecTemp;
		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
			auto utf8data = standard.toLocal8Bit();
			auto ss3 = utf8data.toStdString();
			auto standardStr = ss3.c_str();
			 
			// 查询表语句
			sprintf(m_strQuery, "select * from %s where Dmi >= %f AND Dmi <= %f AND (DiseaseType=1 OR DiseaseType=2 OR DiseaseType=3) AND RoadStandard='%s' AND AddFile4!='2' AND AddFile4!='3'", m_vecDiseaseTableName[j].c_str(), sMile, eMile, standardStr);

			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 病害数据
			vecTemp.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecTemp[i].nID = dr.GetIntValue("ID");
				vecTemp[i].dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(vecTemp[i].strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				vecTemp[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecTemp[i].nDrawType = dr.GetIntValue("DrawType");
				vecTemp[i].nLevel = dr.GetIntValue("Level");
				vecTemp[i].dLength = dr.GetFloatValue("Length");
				vecTemp[i].dWidth = dr.GetFloatValue("Width");
				vecTemp[i].dArea = dr.GetFloatValue("Area");
				vecTemp[i].dDepth = dr.GetFloatValue("Depth");
				vecTemp[i].nPixelLen = dr.GetIntValue("PixelLen");
				vecTemp[i].nPixelWid = dr.GetIntValue("PixelWid");
				vecTemp[i].dRealLen = dr.GetFloatValue("RealLen");
				vecTemp[i].dReaWidth = dr.GetFloatValue("ReaWidth");


				vecTemp[i].vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(vecTemp[i].vec2dRect._Myfirst(), pBuffer, nBlobLen);


				vecTemp[i].vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(vecTemp[i].vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				vecTemp[i].vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(vecTemp[i].vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecTemp[i].strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(vecTemp[i].strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}



				vecTemp[i].ndiseaseType = dr.GetIntValue("DiseaseType");

				vecTemp[i].diseaseWeight = dr.GetFloatValue("Weight");

				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(vecTemp[i].strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecTemp[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecTemp[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				vecTemp[i].dRoadWidth = dr.GetFloatValue("RoadWidth");
				 
				vecTemp[i].dDmi = dr.GetFloatValue("Dmi");
				vecTemp[i].dDmiEnd = dr.GetFloatValue("DmiUp");
				vecTemp[i].dDmiStart = dr.GetFloatValue("DmiDown");
				i++;
			}

			dr.Close();

			allDatas.insert(allDatas.end(), vecTemp.begin(), vecTemp.end());
		}
		vecData = QVector<hnRoadDiseaseInfo>::fromStdVector(allDatas);
	//	handelMiles(miles, allDatas, vecData, roadYDistance, i,true);
	}
	catch (...)
	{
		// 抛出异常
		throw exception(m_sqliteDB.GetLastErrorMsg());
		return false;
	}

	return true;
}

  
void hnRoadDiseaseTable::readDesignDiseases_Service(const QString& standard, double beginEncoderMile, double endEncoderMile, QVector<hnRoadDiseaseInfo>&result)
{
	 
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return;
	}
	try
	{
		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		//遍历所有表，读取数据
		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
			// 查询表语句
			QString   strQuery = QString("select * from %3 where"
				"((DmiDown <= %1 AND DmiUp >= %2) OR"
				"(DmiDown <= %1 AND DmiUp >= %1) OR"
				"(DmiDown >= %1 AND DmiUp <= %2) OR"
				"(DmiDown <= %2 AND DmiUp >= %2))"
				"AND"
				"(DrawType = 2 OR DrawType = 3)"	//设计模式病害
				"AND"
				"(DiseaseType = 0) AND RoadStandard ='%4' AND AddFile4!='2' AND AddFile4!='3'")				//普通病害，区别于景观病害
				.arg(beginEncoderMile).arg(endEncoderMile).arg(QString::fromStdString(m_vecDiseaseTableName.at(j))).arg(standard);

			string str = MyCommonMethods::QstringToChar(strQuery);

			qsnprintf(m_strQuery, sizeof(m_strQuery), "%s", str.c_str());
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			hnRoadDiseaseInfo disease;
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				disease.nID = dr.GetIntValue("ID");
				disease.dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(disease.strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				disease.nRSurfaceType = dr.GetIntValue("RSurfaceType");
				disease.nDrawType = dr.GetIntValue("DrawType");
				disease.nLevel = dr.GetIntValue("Level");
				disease.dLength = dr.GetFloatValue("Length");
				disease.dWidth = dr.GetFloatValue("Width");
				disease.dArea = dr.GetFloatValue("Area");
				disease.dDepth = dr.GetFloatValue("Depth");
				disease.nPixelLen = dr.GetIntValue("PixelLen");
				disease.nPixelWid = dr.GetIntValue("PixelWid");
				disease.dRealLen = dr.GetFloatValue("RealLen");
				disease.dReaWidth = dr.GetFloatValue("ReaWidth");


				disease.vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(disease.vec2dRect._Myfirst(), pBuffer, nBlobLen);


				disease.vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(disease.vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				disease.vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(disease.vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(disease.strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(disease.strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}
				disease.ndiseaseType = dr.GetIntValue("DiseaseType");
				disease.diseaseWeight = dr.GetFloatValue("Weight");
				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(disease.strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(disease.strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(disease.strAddFile5, dr.GetStringValue("AddFile5"));
				}

				disease.dRoadWidth = dr.GetFloatValue("RoadWidth");
				disease.dDmi = dr.GetFloatValue("Dmi");
				disease.dDmiEnd = dr.GetFloatValue("DmiUp");
				disease.dDmiStart = dr.GetFloatValue("DmiDown");

				result.append(disease);
				i++;
			}
			dr.Close();
		}
	}
	catch (...)
	{
		// 抛出异常
		throw exception(m_sqliteDB.GetLastErrorMsg());
		return ;
	}
}
 

bool hnRoadDiseaseTable::deleteDiseases_Service(vector<hnRoadDiseaseInfo> &vecData)
{
	for each (hnRoadDiseaseInfo dis in vecData)
	{
		deleteDisease_Service(dis);
	}
	return true;
}

bool hnRoadDiseaseTable::deleteDisease_Service( hnRoadDiseaseInfo &inData)
{
	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);
	setDeleteDisease(inData);
	// 判断数据库是否连接
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	//判断病害是否异常

	if (inData.dRoadWidth <= 0/* || strlen( inData.strRoadStandard)==0*/)
	{
		return false;
	}


	try
	{
		char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
		memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句
		sprintf(strTableQuery, "select * from %s where ID ='%d'", inData.strDiseaseTableName, inData.nID);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);
		{
			// 已存在该数据则更新该记录
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "update %s set "
				"Mileage=?, RoadStandard=?,RSurfaceType=?, DrawType=?, Level=?, Length=?,Width=?,Area=?,Depth=?,PixelLen=?,PixelWid=?,"
				"RealLen=?,ReaWidth=?,VecRect=?,RectCnt=?,Vec3dPoint=?,n3dCnt=?,GpsTimer=?,GpsTimerCnt=?,Remark=?,DiseaseTableName=?, DiseaseType=? ,"
				"Weight=?,DisName=?,AddFile4=?,AddFile5=?,Dmi=?,DmiUp=?,DmiDown=?,RoadWidth=? where ID = %d", inData.strDiseaseTableName, inData.nID);

			// cmd命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			//cmd.BindParam(1, inData.nID);
			cmd.BindParam(1, inData.dMileage);
			cmd.BindParam(2, inData.strRoadStandard);
			cmd.BindParam(3, inData.nRSurfaceType);
			cmd.BindParam(4, inData.nDrawType);
			cmd.BindParam(5, inData.nLevel);
			cmd.BindParam(6, inData.dLength);
			cmd.BindParam(7, inData.dWidth);
			cmd.BindParam(8, inData.dArea);
			cmd.BindParam(9, inData.dDepth);
			cmd.BindParam(10, inData.nPixelLen);
			cmd.BindParam(11, inData.nPixelWid);
			cmd.BindParam(12, inData.dRealLen);
			cmd.BindParam(13, inData.dReaWidth);


			BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* inData.vec2dRect.size()];
			memcpy(byBounary1, inData.vec2dRect._Myfirst(), sizeof(hn2dRectI) * inData.vec2dRect.size());
			cmd.BindParam(14, byBounary1, sizeof(hn2dRectI) * inData.vec2dRect.size());
			int geshu = (int)inData.vec2dRect.size();
			cmd.BindParam(15, geshu);


			BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* inData.vec3dRect.size()];
			memcpy(byBounary3, inData.vec3dRect._Myfirst(), sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(16, byBounary3, sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(17, (int)inData.vec3dRect.size());


			BYTE* byBounary2 = new BYTE[sizeof(double)* inData.vecGpsTimer.size()];
			memcpy(byBounary2, inData.vecGpsTimer._Myfirst(), sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(18, byBounary2, sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(19, (int)inData.vecGpsTimer.size());

			cmd.BindParam(20, inData.strRemark);
			cmd.BindParam(21, inData.strDiseaseTableName);
			cmd.BindParam(22, inData.ndiseaseType);
			cmd.BindParam(23, inData.diseaseWeight);
			cmd.BindParam(24, inData.strDisName);
			cmd.BindParam(25, inData.strAddFile4);
			cmd.BindParam(26, inData.strAddFile5);
			cmd.BindParam(27, inData.dDmi);
			cmd.BindParam(28, inData.dDmiEnd);
			cmd.BindParam(29, inData.dDmiStart);
			cmd.BindParam(30, inData.dRoadWidth);
			// 执行sql语句
			if (!m_sqliteDB.ExcuteNonQuery(&cmd))
			{
				if (byBounary1)
				{
					delete[] byBounary1;
					byBounary1 = NULL;
				}

				if (byBounary2)
				{
					delete[] byBounary2;
					byBounary2 = NULL;
				}

				// 清空cmd
				cmd.Clear();
				return false;
			}

			if (byBounary1)
			{
				delete[] byBounary1;
				byBounary1 = NULL;
			}

			if (byBounary2)
			{
				delete[] byBounary2;
				byBounary2 = NULL;
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

bool hnRoadDiseaseTable::deleteAllDisease_Service()
{
	////进度条
	QProgressDialog progressDialog;
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QString::fromLocal8Bit("正在进行清空病害等操作!"));
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(nullptr);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.setModal(true);
	progressDialog.setMaximum(m_vecDiseaseTableName.size());
	progressDialog.show();

	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	for (auto nameStr:m_vecDiseaseTableName)
	{
		const char * name = nameStr.c_str();
		sprintf(strQuery, "delete from %s", name);

		// 执行sql语句
		executeDB(strQuery);

		//更新进度条
		progressValue++;

		progressDialog.setValue(progressValue);
		QApplication::processEvents();
	}
	 
	 
	QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("清空病害完成!"),
		QString::fromLocal8Bit("确定"));

	return true;
}

bool hnRoadDiseaseTable::deleteAllDisease_Service(vector<string> allDiseaseTables)
{
	////进度条
	QProgressDialog progressDialog;
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QString::fromLocal8Bit("正在清空病害"));
	progressDialog.setAutoClose(true);
	progressDialog.setCancelButton(nullptr);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.setModal(true);
	progressDialog.setMaximum(allDiseaseTables.size());
	progressDialog.show();

	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	for (auto nameStr : allDiseaseTables)
	{
		const char * name = nameStr.c_str();
		sprintf(strQuery, "delete from %s", name);

		// 执行sql语句
		executeDB(strQuery);

		//更新进度条
		progressValue++;

		progressDialog.setValue(progressValue);
		QApplication::processEvents();
	}


	QMessageBox::information(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("清空病害完成!"),
		QString::fromLocal8Bit("确定"));

	return true;
}

bool hnRoadDiseaseTable::readAllDiseases_Service(const hnProjectSetInfo& setInfo, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo)
{
	int line = setInfo.nLineType;

	if (!m_sqliteDB.IsOpen())
	{
		return false;
	} 
	double dmiStart = setInfo.dStartDmi;  //下界  0
	double dmiEnd = setInfo.dEndEnclMile;   //上界

	try
	{

		vector<hnRoadDiseaseInfo> allDatas;
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		vector<hnRoadDiseaseInfo> vecTemp;

		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
#if 0

			if ("龟裂" == m_vecDiseaseTableName.at(j))
			{
				QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
				qDebug() << myStr;
			}
#endif

			//病害上界 DmiUp (大里程) ， DmiDown 病害下界  (小里程)
			//QString myStr = QString::fromLocal8Bit(m_vecDiseaseTableName.at(j).c_str());
			QString standard = QString::fromLocal8Bit(setInfo.strRoadStandard);
			// 查询表语句

			QString   strQuery = QString("select * from %3 where"
				"((DmiDown<=%1 AND DmiUp>=%2) OR"
				"(DmiDown<=%1 AND DmiUp>=%1) OR"
				"(DmiDown>=%1 AND DmiUp<=%2) OR"
				"(DmiDown<=%2 AND DmiUp>=%2))  AND RoadWidth =%4 "
				"AND RoadStandard ='%5' AND DrawType=%6 AND AddFile4!='2' AND AddFile4!='3'")
				.arg(dmiStart)
				.arg(dmiEnd)
				.arg(QString::fromStdString(m_vecDiseaseTableName.at(j)))
				.arg(QString::number(setInfo.dRoadWidth))
				.arg(standard).arg(setInfo.nDrawType);
			auto str0 = MyCommonMethods::QstringToChar(strQuery);

			qsnprintf(m_strQuery, sizeof(m_strQuery), "%s", str0.c_str());
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();

			i = 0;

			// 病害数据
			vecTemp.resize(nCount);

			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;

			// 遍历查询出的数据
			while (dr.Read() && (i < nCount))
			{
				// 将值赋给变量--ID
				vecTemp[i].nID = dr.GetIntValue("ID");
				vecTemp[i].dMileage = dr.GetFloatValue("Mileage");
				if (dr.GetStringValue("RoadStandard") != NULL)
				{
					strcpy(vecTemp[i].strRoadStandard, dr.GetStringValue("RoadStandard"));
				}
				vecTemp[i].nRSurfaceType = dr.GetIntValue("RSurfaceType");
				vecTemp[i].nDrawType = dr.GetIntValue("DrawType");
				vecTemp[i].nLevel = dr.GetIntValue("Level");
				vecTemp[i].dLength = dr.GetFloatValue("Length");
				vecTemp[i].dWidth = dr.GetFloatValue("Width");
				vecTemp[i].dArea = dr.GetFloatValue("Area");
				vecTemp[i].dDepth = dr.GetFloatValue("Depth");
				vecTemp[i].nPixelLen = dr.GetIntValue("PixelLen");
				vecTemp[i].nPixelWid = dr.GetIntValue("PixelWid");
				vecTemp[i].dRealLen = dr.GetFloatValue("RealLen");
				vecTemp[i].dReaWidth = dr.GetFloatValue("ReaWidth");


				vecTemp[i].vec2dRect.resize(dr.GetIntValue("RectCnt"));
				pBuffer = dr.GetBlobValue("VecRect", nBlobLen);
				memcpy(vecTemp[i].vec2dRect._Myfirst(), pBuffer, nBlobLen);


				vecTemp[i].vec3dRect.resize(dr.GetIntValue("n3dCnt"));
				pBuffer = dr.GetBlobValue("Vec3dPoint", nBlobLen);
				memcpy(vecTemp[i].vec3dRect._Myfirst(), pBuffer, nBlobLen);

				// 起始终止gps时间
				vecTemp[i].vecGpsTimer.resize(dr.GetIntValue("GpsTimerCnt"));
				pBuffer = dr.GetBlobValue("GpsTimer", nBlobLen);
				memcpy(vecTemp[i].vecGpsTimer._Myfirst(), pBuffer, nBlobLen);

				if (dr.GetStringValue("Remark") != NULL)
				{
					strcpy(vecTemp[i].strRemark, dr.GetStringValue("Remark"));
				}
				if (dr.GetStringValue("DiseaseTableName") != NULL)
				{
					strcpy(vecTemp[i].strDiseaseTableName, dr.GetStringValue("DiseaseTableName"));
				}



				vecTemp[i].ndiseaseType = dr.GetIntValue("DiseaseType");

				vecTemp[i].diseaseWeight = dr.GetFloatValue("Weight");

				if (dr.GetStringValue("DisName") != NULL)
				{
					strcpy(vecTemp[i].strDisName, dr.GetStringValue("DisName"));
				}

				if (dr.GetStringValue("AddFile4") != NULL)
				{
					strcpy(vecTemp[i].strAddFile4, dr.GetStringValue("AddFile4"));
				}

				if (dr.GetStringValue("AddFile5") != NULL)
				{
					strcpy(vecTemp[i].strAddFile5, dr.GetStringValue("AddFile5"));
				}

				vecTemp[i].dRoadWidth = dr.GetFloatValue("RoadWidth");

				vecTemp[i].dDmi = dr.GetFloatValue("Dmi");
				vecTemp[i].dDmiEnd = dr.GetFloatValue("DmiUp");
				vecTemp[i].dDmiStart = dr.GetFloatValue("DmiDown");
				i++;
			}

			dr.Close();

			allDatas.insert(allDatas.end(), vecTemp.begin(), vecTemp.end());
		}

		vecData.clear();
		FilterOutDisrase(allDatas, vecData, markinfo, setInfo, false);

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
int hnRoadDiseaseTable::getMaxID()
{
	return 0;
}

// 获取最大id
int hnRoadDiseaseTable::getMaxID(const string& strTableName)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return 1;
	}

	// 查询表语句
	sprintf(m_strQuery, "select max(ID) from %s", strTableName.c_str());

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


bool hnRoadDiseaseTable::writeDatas_Service(const hnProjectSetInfo& projectConfig, vector<hnRoadDiseaseInfo>& vecData)
{
	for (int i = 0; i < vecData.size(); i++)
	{
		writeSingleDatas_Service(projectConfig, vecData[i]);
	}

	return true;
}


bool hnRoadDiseaseTable::writeDataAffairs_Service(const char* strTableName, bool bWrite, vector<hnRoadDiseaseInfo>& vecData, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle)/* = NULL*/)
{ 
	// 判断数据库是否连接
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}

	char strTableQuery[SQL_QUERY_LEN];
	memset(strTableQuery, 0, SQL_QUERY_LEN);

	try
	{
		//开启事务
		sqlite3_exec(m_sqliteDB.getDb(), "begin;", 0, 0, 0);

		//其他值
		sqlite3_stmt *stmt = NULL;
		int res = -1;

		//写入
		if (bWrite)
		{
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "insert into %s(ID, Mileage,RoadStandard, RSurfaceType, DrawType,Level,Length,Width,Area,Depth,PixelLen,"
				"PixelWid,RealLen,ReaWidth,VecRect,RectCnt,Vec3dPoint,n3dCnt,GpsTimer,GpsTimerCnt,Remark,DiseaseTableName, DiseaseType,Weight,DisName,"
				"AddFile4,AddFile5,Dmi,DmiUp,DmiDown,RoadWidth) values (?, ?, ?, ?, ?, ?,?, ?, ?, ?,?,?, ?,?, ?, ?, ?, ?, ?,?, ?,?,?, ?,?, ?, ?,?, ?, ?, ?)", strTableName);

			res = sqlite3_prepare_v2(m_sqliteDB.getDb(), strTableQuery, strlen(strTableQuery), &stmt, 0);

			//速度会有任何提升
			for (int i = 0; i < vecData.size(); i++)
			{
				setAddDisease(vecData[i]);
				//重置stmt
				res = sqlite3_reset(stmt);

				// SQLITE_STATIC->传递给该字符串的指针将有效,直到执行查询为止
				res = sqlite3_bind_int(stmt, 1, vecData[i].nID);
				res = sqlite3_bind_double(stmt, 2, vecData[i].dMileage);
				res = sqlite3_bind_text(stmt, 3, vecData[i].strRoadStandard, -1, SQLITE_STATIC);
				res = sqlite3_bind_int(stmt, 4, vecData[i].nRSurfaceType);
				res = sqlite3_bind_int(stmt, 5, vecData[i].nDrawType);
				res = sqlite3_bind_int(stmt, 6, vecData[i].nLevel);

				res = sqlite3_bind_double(stmt, 7, vecData[i].dLength);
				res = sqlite3_bind_double(stmt, 8, vecData[i].dWidth);
				res = sqlite3_bind_double(stmt, 9, vecData[i].dArea);
				res = sqlite3_bind_double(stmt, 10, vecData[i].dDepth);

				res = sqlite3_bind_int(stmt, 11, vecData[i].nPixelLen);
				res = sqlite3_bind_int(stmt, 12, vecData[i].nPixelWid);

				res = sqlite3_bind_double(stmt, 13, vecData[i].dRealLen);
				res = sqlite3_bind_double(stmt, 14, vecData[i].dReaWidth);

				BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* vecData[i].vec2dRect.size()];
				memcpy(byBounary1, vecData[i].vec2dRect._Myfirst(), sizeof(hn2dRectI) * vecData[i].vec2dRect.size());
				res = sqlite3_bind_blob(stmt, 15, byBounary1, sizeof(hn2dRectI) * vecData[i].vec2dRect.size(), NULL);
				res = sqlite3_bind_int(stmt, 16, vecData[i].vec2dRect.size());

				BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* vecData[i].vec3dRect.size()];
				memcpy(byBounary3, vecData[i].vec3dRect._Myfirst(), sizeof(hn3dRectI) * vecData[i].vec3dRect.size());
				res = sqlite3_bind_blob(stmt, 17, byBounary3, sizeof(hn3dRectI) * vecData[i].vec3dRect.size(), NULL);
				res = sqlite3_bind_int(stmt, 18, vecData[i].vec3dRect.size());

				BYTE* byBounary2 = new BYTE[sizeof(double)* vecData[i].vecGpsTimer.size()];
				memcpy(byBounary2, vecData[i].vecGpsTimer._Myfirst(), sizeof(double) * vecData[i].vecGpsTimer.size());
				res = sqlite3_bind_blob(stmt, 19, byBounary2, sizeof(double) * vecData[i].vecGpsTimer.size(), NULL);
				res = sqlite3_bind_int(stmt, 20, vecData[i].vecGpsTimer.size());

				res = sqlite3_bind_text(stmt, 21, vecData[i].strRemark, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 22, vecData[i].strDiseaseTableName, -1, SQLITE_STATIC);
				 
				res = sqlite3_bind_int(stmt, 23, vecData[i].ndiseaseType);
				res = sqlite3_bind_double(stmt, 24, vecData[i].diseaseWeight);
				res = sqlite3_bind_text(stmt, 25, vecData[i].strDisName, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 26, vecData[i].strAddFile4, -1, SQLITE_STATIC);
				res = sqlite3_bind_text(stmt, 27, vecData[i].strAddFile5, -1, SQLITE_STATIC);
				res = sqlite3_bind_double(stmt, 28, vecData[i].dDmi);
				res = sqlite3_bind_double(stmt, 29, vecData[i].dDmiEnd);
				res = sqlite3_bind_double(stmt, 30, vecData[i].dDmiStart);
				res = sqlite3_bind_double(stmt, 31, vecData[i].dRoadWidth);
				//3.遍历select执行的返回结果
				res = sqlite3_step(stmt);

				if (pProgress && i%vecData.size() == 0)
				{
					pProgress(0.9999, "导入数据库", false);
				}

				if (byBounary1)
				{
					delete byBounary1;
					byBounary1 = NULL;
				}

				if (byBounary2)
				{
					delete byBounary2;
					byBounary2 = NULL;
				}
				if (byBounary3)
				{
					delete byBounary3;
					byBounary3 = NULL;
				}
			}


		}
		else//更新
		{ 
			sprintf(strTableQuery, "update %s set "
				"Mileage=?,RoadStandard=?, RSurfaceType=?, DrawType=?, Level=?,Length=?,Width=?,Area=?,Depth=?,PixelLen=?,PixelWid=?,"
				"RealLen=?,ReaWidth=?,VecRect=?,RectCnt=?,Vec3dPoint=?,n3dCnt=?,GpsTimer=?,GpsTimerCnt=?,Remark=?,DiseaseTableName=?, DiseaseType=? ,"
				"Weight=?,DisName=?,AddFile4=?,AddFile5=?,Dmi=?,DmiUp=?,DmiDown=?,RoadWidth=? where ID = ?", strTableName);

			res = sqlite3_prepare_v2(m_sqliteDB.getDb(), strTableQuery, strlen(strTableQuery), &stmt, 0);
			 
				//速度会有任何提升
				for (int i = 0; i < vecData.size(); i++)
				{
					setAddDisease(vecData[i]);
					//重置stmt
					res = sqlite3_reset(stmt);

					// SQLITE_STATIC->传递给该字符串的指针将有效,直到执行查询为止
					res = sqlite3_bind_int(stmt, 1, vecData[i].nID);
					res = sqlite3_bind_double(stmt, 2, vecData[i].dMileage);
					res = sqlite3_bind_text(stmt, 3, vecData[i].strRoadStandard, -1, SQLITE_STATIC);
					res = sqlite3_bind_int(stmt, 4, vecData[i].nRSurfaceType);
					res = sqlite3_bind_int(stmt, 5, vecData[i].nDrawType);
					res = sqlite3_bind_int(stmt, 6, vecData[i].nLevel);

					res = sqlite3_bind_double(stmt, 7, vecData[i].dLength);
					res = sqlite3_bind_double(stmt, 8, vecData[i].dWidth);
					res = sqlite3_bind_double(stmt, 9, vecData[i].dArea);
					res = sqlite3_bind_double(stmt, 10, vecData[i].dDepth);

					res = sqlite3_bind_int(stmt, 11, vecData[i].nPixelLen);
					res = sqlite3_bind_int(stmt, 12, vecData[i].nPixelWid);

					res = sqlite3_bind_double(stmt, 13, vecData[i].dRealLen);
					res = sqlite3_bind_double(stmt, 14, vecData[i].dReaWidth);

					BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* vecData[i].vec2dRect.size()];
					memcpy(byBounary1, vecData[i].vec2dRect._Myfirst(), sizeof(hn2dRectI) * vecData[i].vec2dRect.size());
					res = sqlite3_bind_blob(stmt, 15, byBounary1, sizeof(hn2dRectI) * vecData[i].vec2dRect.size(), NULL);
					res = sqlite3_bind_int(stmt, 16, vecData[i].vec2dRect.size());

					BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* vecData[i].vec3dRect.size()];
					memcpy(byBounary3, vecData[i].vec3dRect._Myfirst(), sizeof(hn3dRectI) * vecData[i].vec3dRect.size());
					res = sqlite3_bind_blob(stmt, 17, byBounary3, sizeof(hn3dRectI) * vecData[i].vec3dRect.size(), NULL);
					res = sqlite3_bind_int(stmt, 18, vecData[i].vec3dRect.size());

					BYTE* byBounary2 = new BYTE[sizeof(double)* vecData[i].vecGpsTimer.size()];
					memcpy(byBounary2, vecData[i].vecGpsTimer._Myfirst(), sizeof(double) * vecData[i].vecGpsTimer.size());
					res = sqlite3_bind_blob(stmt, 19, byBounary2, sizeof(double) * vecData[i].vecGpsTimer.size(), NULL);
					res = sqlite3_bind_int(stmt, 20, vecData[i].vecGpsTimer.size());

					res = sqlite3_bind_text(stmt, 21, vecData[i].strRemark, -1, SQLITE_STATIC);
					res = sqlite3_bind_text(stmt, 22, vecData[i].strDiseaseTableName, -1, SQLITE_STATIC);
					res = sqlite3_bind_int(stmt, 23, vecData[i].ndiseaseType);
					res = sqlite3_bind_double(stmt, 24, vecData[i].diseaseWeight);
					res = sqlite3_bind_text(stmt, 25, vecData[i].strDisName, -1, SQLITE_STATIC);
					res = sqlite3_bind_text(stmt, 26, vecData[i].strAddFile4, -1, SQLITE_STATIC);
					res = sqlite3_bind_text(stmt, 27, vecData[i].strAddFile5, -1, SQLITE_STATIC);
					res = sqlite3_bind_double(stmt, 28, vecData[i].dDmi);
					res = sqlite3_bind_double(stmt, 29, vecData[i].dDmiEnd);
					res = sqlite3_bind_double(stmt, 30, vecData[i].dDmiStart);
					res = sqlite3_bind_double(stmt, 31, vecData[i].dRoadWidth);


					//3.遍历select执行的返回结果
					res = sqlite3_step(stmt);

					if (pProgress && i%vecData.size() == 0)
					{
						pProgress(0.9999, "导入数据库", false);
					}

					if (byBounary1)
					{
						delete byBounary1;
						byBounary1 = NULL;
					}

					if (byBounary2)
					{
						delete byBounary2;
						byBounary2 = NULL;
					}
					if (byBounary3)
					{
						delete byBounary3;
						byBounary3 = NULL;
					}
				}
			
		}

		//
		sqlite3_finalize(stmt);

		//提交事务
		sqlite3_exec(m_sqliteDB.getDb(), "commit;", 0, 0, 0);
	}
	catch (...)
	{
		if (pProgress)
		{
			pProgress(1.0, "更新数据库", false);
		}

		//提交事务存在问题 则事务回滚 事务已经提交则无法回滚
		sqlite3_exec(m_sqliteDB.getDb(), "rollback;", 0, 0, 0);
	}

	if (pProgress)
	{
		pProgress(1.0, "更新数据库", false);
	}

	return true;
}


bool hnRoadDiseaseTable::writeSingleDatas_Service(const hnProjectSetInfo& projectConfig, hnRoadDiseaseInfo& inData)
{ 
	setAddDisease(inData);
	// 判断数据库是否连接
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	} 
	//判断病害是否异常

	if (inData.dRoadWidth <=0/* || strlen( inData.strRoadStandard)==0*/)
	{
		return false;
	}
	//判断病害是否处于用户允许区间
	 
	/*if (len*inData.dMileage>= projectConfig.dBegMile*len || len*inData.dMileage <=projectConfig.dEndMile * len )
	{
		return false;
	}*/


	try
	{
		char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
		memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句
		sprintf(strTableQuery, "select * from %s where ID ='%d'", inData.strDiseaseTableName, inData.nID);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery);

		// 不存在则添加
		if (dr.RowCount() == 0)
		{
			// 添加CPIIIsql语句
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "insert into %s(ID, Mileage,RoadStandard, RSurfaceType, DrawType,Level,Length,Width,Area,Depth,PixelLen,"
				"PixelWid,RealLen,ReaWidth,VecRect,RectCnt,Vec3dPoint,n3dCnt,GpsTimer,GpsTimerCnt,Remark,DiseaseTableName, DiseaseType,Weight,DisName,"
				"AddFile4,AddFile5,Dmi,DmiUp,DmiDown,RoadWidth) values (?,  ?, ?, ?,?, ?, ?,?, ?, ?, ?,?, ?,?, ?, ?, ?, ?,?, ?, ?,?,?,?, ?, ?, ?, ?,?,?,?)", inData.strDiseaseTableName);

			// sql命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			cmd.BindParam(1, inData.nID);
			cmd.BindParam(2, inData.dMileage);
			cmd.BindParam(3, inData.strRoadStandard);
			cmd.BindParam(4, inData.nRSurfaceType);
			cmd.BindParam(5, inData.nDrawType);
			cmd.BindParam(6, inData.nLevel);
			cmd.BindParam(7, inData.dLength);
			cmd.BindParam(8, inData.dWidth);
			cmd.BindParam(9, inData.dArea);
			cmd.BindParam(10, inData.dDepth);
			cmd.BindParam(11, inData.nPixelLen);
			cmd.BindParam(12, inData.nPixelWid);
			cmd.BindParam(13, inData.dRealLen);
			cmd.BindParam(14, inData.dReaWidth);


			BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* inData.vec2dRect.size()];
			memcpy(byBounary1, inData.vec2dRect._Myfirst(), sizeof(hn2dRectI) * inData.vec2dRect.size());
			cmd.BindParam(15, byBounary1, sizeof(hn2dRectI) * inData.vec2dRect.size());
			cmd.BindParam(16, (int)inData.vec2dRect.size());


			BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* inData.vec3dRect.size()];
			memcpy(byBounary3, inData.vec3dRect._Myfirst(), sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(17, byBounary3, sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(18, (int)inData.vec3dRect.size());


			BYTE* byBounary2 = new BYTE[sizeof(double)* inData.vecGpsTimer.size()];
			memcpy(byBounary2, inData.vecGpsTimer._Myfirst(), sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(19, byBounary2, sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(20, (int)inData.vecGpsTimer.size());

			cmd.BindParam(21, inData.strRemark);
			cmd.BindParam(22, inData.strDiseaseTableName);
			cmd.BindParam(23, inData.ndiseaseType);
			cmd.BindParam(24, inData.diseaseWeight);
			cmd.BindParam(25, inData.strDisName);
			cmd.BindParam(26, inData.strAddFile4);
			cmd.BindParam(27, inData.strAddFile5);
			cmd.BindParam(28, inData.dDmi);
			cmd.BindParam(29, inData.dDmiEnd);
			cmd.BindParam(30, inData.dDmiStart);
			cmd.BindParam(31, inData.dRoadWidth);




			// 执行sql语句
			if (!m_sqliteDB.ExcuteNonQuery(&cmd))
			{
				if (byBounary1)
				{
					delete[] byBounary1;
					byBounary1 = NULL;
				}

				if (byBounary2)
				{
					delete[] byBounary2;
					byBounary2 = NULL;
				}
				if (byBounary3)
				{
					delete[] byBounary3;
					byBounary3 = NULL;
				}
				// 清空cmd
				cmd.Clear();
				return false;
			}

			if (byBounary1)
			{
				delete[] byBounary1;
				byBounary1 = NULL;
			}

			if (byBounary2)
			{
				delete[] byBounary2;
				byBounary2 = NULL;
			}
			if (byBounary3)
			{
				delete[] byBounary3;
				byBounary3 = NULL;
			}
			// 清空cmd
			cmd.Clear();
		}
		else
		{
			// 已存在该数据则更新该记录
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "update %s set "
				"Mileage=?, RoadStandard=?,RSurfaceType=?, DrawType=?, Level=?, Length=?,Width=?,Area=?,Depth=?,PixelLen=?,PixelWid=?,"
				"RealLen=?,ReaWidth=?,VecRect=?,RectCnt=?,Vec3dPoint=?,n3dCnt=?,GpsTimer=?,GpsTimerCnt=?,Remark=?,DiseaseTableName=?, DiseaseType=? ,"
				"Weight=?,DisName=?,AddFile4=?,AddFile5=?,Dmi=?,DmiUp=?,DmiDown=?,RoadWidth=? where ID = %d", inData.strDiseaseTableName, inData.nID);

			// cmd命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			//cmd.BindParam(1, inData.nID);
			cmd.BindParam(1, inData.dMileage);
			cmd.BindParam(2, inData.strRoadStandard);
			cmd.BindParam(3, inData.nRSurfaceType);
			cmd.BindParam(4, inData.nDrawType);
			cmd.BindParam(5, inData.nLevel);
			cmd.BindParam(6, inData.dLength);
			cmd.BindParam(7, inData.dWidth);
			cmd.BindParam(8, inData.dArea);
			cmd.BindParam(9, inData.dDepth);
			cmd.BindParam(10, inData.nPixelLen);
			cmd.BindParam(11, inData.nPixelWid);
			cmd.BindParam(12, inData.dRealLen);
			cmd.BindParam(13, inData.dReaWidth);


			BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* inData.vec2dRect.size()];
			memcpy(byBounary1, inData.vec2dRect._Myfirst(), sizeof(hn2dRectI) * inData.vec2dRect.size());
			cmd.BindParam(14, byBounary1, sizeof(hn2dRectI) * inData.vec2dRect.size());
			int geshu = (int)inData.vec2dRect.size();
			cmd.BindParam(15, geshu);


			BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* inData.vec3dRect.size()];
			memcpy(byBounary3, inData.vec3dRect._Myfirst(), sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(16, byBounary3, sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(17, (int)inData.vec3dRect.size());


			BYTE* byBounary2 = new BYTE[sizeof(double)* inData.vecGpsTimer.size()];
			memcpy(byBounary2, inData.vecGpsTimer._Myfirst(), sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(18, byBounary2, sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(19, (int)inData.vecGpsTimer.size());

			cmd.BindParam(20, inData.strRemark);
			cmd.BindParam(21, inData.strDiseaseTableName);
			cmd.BindParam(22, inData.ndiseaseType);
			cmd.BindParam(23, inData.diseaseWeight);
			cmd.BindParam(24, inData.strDisName);
			cmd.BindParam(25, inData.strAddFile4);
			cmd.BindParam(26, inData.strAddFile5);
			cmd.BindParam(27, inData.dDmi);
			cmd.BindParam(28, inData.dDmiEnd);
			cmd.BindParam(29, inData.dDmiStart);
			cmd.BindParam(30, inData.dRoadWidth);
			// 执行sql语句
			if (!m_sqliteDB.ExcuteNonQuery(&cmd))
			{
				if (byBounary1)
				{
					delete[] byBounary1;
					byBounary1 = NULL;
				}

				if (byBounary2)
				{
					delete[] byBounary2;
					byBounary2 = NULL;
				}

				// 清空cmd
				cmd.Clear();
				return false;
			}

			if (byBounary1)
			{
				delete[] byBounary1;
				byBounary1 = NULL;
			}

			if (byBounary2)
			{
				delete[] byBounary2;
				byBounary2 = NULL;
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

bool hnRoadDiseaseTable::updaetSingleDataInfo_Service(hnRoadDiseaseInfo& inData)
{
	// 判断数据库是否连接
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	//判断病害是否异常

	if (inData.dRoadWidth <= 0/* || strlen( inData.strRoadStandard)==0*/)
	{
		return false;
	}
	//判断病害是否处于用户允许区间

	/*if (len*inData.dMileage>= projectConfig.dBegMile*len || len*inData.dMileage <=projectConfig.dEndMile * len )
	{
	return false;
	}*/


	try
	{
		char strTableQuery[SQL_QUERY_LEN];			// 得到查询语句
		memset(strTableQuery, 0, SQL_QUERY_LEN);	// 初始化建表语句
		sprintf(strTableQuery, "select * from %s where ID ='%d'", inData.strDiseaseTableName, inData.nID);

		// 查询
		hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(strTableQuery); 
		{
			// 已存在该数据则更新该记录
			memset(strTableQuery, 0, SQL_QUERY_LEN);
			sprintf(strTableQuery, "update %s set "
				"Mileage=?, RoadStandard=?,RSurfaceType=?, DrawType=?, Level=?, Length=?,Width=?,Area=?,Depth=?,PixelLen=?,PixelWid=?,"
				"RealLen=?,ReaWidth=?,VecRect=?,RectCnt=?,Vec3dPoint=?,n3dCnt=?,GpsTimer=?,GpsTimerCnt=?,Remark=?,DiseaseTableName=?, DiseaseType=? ,"
				"Weight=?,DisName=?,AddFile4=?,AddFile5=?,Dmi=?,DmiUp=?,DmiDown=?,RoadWidth=? where ID = %d", inData.strDiseaseTableName, inData.nID);

			// cmd命令
			hnSQLiteCommand cmd(&m_sqliteDB, strTableQuery);

			// 绑定参数
			//cmd.BindParam(1, inData.nID);
			cmd.BindParam(1, inData.dMileage);
			cmd.BindParam(2, inData.strRoadStandard);
			cmd.BindParam(3, inData.nRSurfaceType);
			cmd.BindParam(4, inData.nDrawType);
			cmd.BindParam(5, inData.nLevel);
			cmd.BindParam(6, inData.dLength);
			cmd.BindParam(7, inData.dWidth);
			cmd.BindParam(8, inData.dArea);
			cmd.BindParam(9, inData.dDepth);
			cmd.BindParam(10, inData.nPixelLen);
			cmd.BindParam(11, inData.nPixelWid);
			cmd.BindParam(12, inData.dRealLen);
			cmd.BindParam(13, inData.dReaWidth);


			BYTE* byBounary1 = new BYTE[sizeof(hn2dRectI)* inData.vec2dRect.size()];
			memcpy(byBounary1, inData.vec2dRect._Myfirst(), sizeof(hn2dRectI) * inData.vec2dRect.size());
			cmd.BindParam(14, byBounary1, sizeof(hn2dRectI) * inData.vec2dRect.size());
			int geshu = (int)inData.vec2dRect.size();
			cmd.BindParam(15, geshu);


			BYTE* byBounary3 = new BYTE[sizeof(hn3dRectI)* inData.vec3dRect.size()];
			memcpy(byBounary3, inData.vec3dRect._Myfirst(), sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(16, byBounary3, sizeof(hn3dRectI) * inData.vec3dRect.size());
			cmd.BindParam(17, (int)inData.vec3dRect.size());


			BYTE* byBounary2 = new BYTE[sizeof(double)* inData.vecGpsTimer.size()];
			memcpy(byBounary2, inData.vecGpsTimer._Myfirst(), sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(18, byBounary2, sizeof(double) * inData.vecGpsTimer.size());
			cmd.BindParam(19, (int)inData.vecGpsTimer.size());

			cmd.BindParam(20, inData.strRemark);
			cmd.BindParam(21, inData.strDiseaseTableName);
			cmd.BindParam(22, inData.ndiseaseType);
			cmd.BindParam(23, inData.diseaseWeight);
			cmd.BindParam(24, inData.strDisName);
			cmd.BindParam(25, inData.strAddFile4);
			cmd.BindParam(26, inData.strAddFile5);
			cmd.BindParam(27, inData.dDmi);
			cmd.BindParam(28, inData.dDmiEnd);
			cmd.BindParam(29, inData.dDmiStart);
			cmd.BindParam(30, inData.dRoadWidth);
			// 执行sql语句
			if (!m_sqliteDB.ExcuteNonQuery(&cmd))
			{
				if (byBounary1)
				{
					delete[] byBounary1;
					byBounary1 = NULL;
				}

				if (byBounary2)
				{
					delete[] byBounary2;
					byBounary2 = NULL;
				}

				// 清空cmd
				cmd.Clear();
				return false;
			}

			if (byBounary1)
			{
				delete[] byBounary1;
				byBounary1 = NULL;
			}

			if (byBounary2)
			{
				delete[] byBounary2;
				byBounary2 = NULL;
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

bool hnRoadDiseaseTable::mergeDatas_Service(const hnProjectSetInfo& projectConfig, QVector<hnRoadDiseaseInfo>& newDiseases, QVector<hnRoadDiseaseInfo>& oldDiseases)
{
	//获取新增病害
	QVector<hnRoadDiseaseInfo> realNewDiseases;
 
	bool ok = true;
	for (  auto& item : newDiseases)
	{
		if (std::find(oldDiseases.constBegin(),oldDiseases.constEnd(),item) == oldDiseases.constEnd())
		{
			item.nID = getMaxID(item.strDiseaseTableName); 
			ok =  writeSingleDatas_Service(projectConfig,item);
		}

	}

	return	ok;

	

}

//删除表数据
bool hnRoadDiseaseTable::deleteFormData_Service(const char* strTableName)
{
	char strQuery[SQL_QUERY_LEN];
	memset(strQuery, 0, SQL_QUERY_LEN);

	sprintf(strQuery, "delete from %s", strTableName);

	// 执行sql语句
	executeDB(strQuery);

	return true;
}

bool hnRoadDiseaseTable::checkDiseaseExist(QString standard, int diseaseType)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	try
	{
		// 获取数据的结果
		int nCount = 0;

		int i = 0;

		// 二进制长度
		long nBlobLen = 0;
		BYTE* pBuffer = NULL;

		int allCount = 0;
		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
			QByteArray sss = standard.toLocal8Bit(); 
			auto sss1 = sss.toStdString();
			auto sss2 = sss1.c_str();
			// 查询表语句
		/*	sprintf(m_strQuery, "select * from %s where DrawType=%d and DiseaseType=0 Or RoadStandard = '%s'", m_vecDiseaseTableName[j].c_str(), diseaseType
			, sss2);*/
			sprintf(m_strQuery, "select * from %s where DrawType=%d and DiseaseType=0'", m_vecDiseaseTableName[j].c_str(), diseaseType
				);
			// 查询
			hnSQLiteDataReader dr = m_sqliteDB.ExcuteQuery(m_strQuery);

			// 获取数据的结果
			nCount = dr.RowCount();
			allCount = allCount + nCount;

			i = 0;


			// 二进制长度
			long nBlobLen = 0;
			BYTE* pBuffer = NULL;


			//dr.Close();

		}
		if (allCount > 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	catch (...)
	{

	}
}
 
bool hnRoadDiseaseTable::deleteAllTargetDrawTypeDisease_Service( QString standard,int drawType)
{
	// 判断数据库是否连接成功
	if (!m_sqliteDB.IsOpen())
	{
		return false;
	}
	try
	{ 
		for (int j = 0; j < m_vecDiseaseTableName.size(); j++)
		{
			QByteArray sss = standard.toLocal8Bit();
			auto sss1 = sss.toStdString();
			auto sss2 = sss1.c_str();
			// 查询表语句
			sprintf(m_strQuery, "DELETE FROM %s WHERE DrawType=%d AND DiseaseType = 0 Or RoadStandard = '%s'", m_vecDiseaseTableName[j].c_str(),drawType
			,sss2);

			// 执行sql语句
			executeDB(m_strQuery); 
		}
		return true;
	}
	catch (...)
	{
		return false;
	}

}

void hnRoadDiseaseTable::FilterOutDisrase(const vector<hnRoadDiseaseInfo>& allDisease, QVector<hnRoadDiseaseInfo>&returnDisease, const QVector<hnMarkInfo>& markinfo, const hnProjectSetInfo& setInfo, bool isLine)
{
	QVector<hnMarkInfo> validMarkinfos;
	hnMarkInfo startMarkInfo;
	startMarkInfo.dEnclMile = 0;
	startMarkInfo.nType = 0; //路面材质 
	strcpy(startMarkInfo.strMark, setInfo.getRSurfaceType().c_str());
	validMarkinfos.push_back(startMarkInfo);
	for (auto mark : markinfo)
	{

		if (mark.nType == 0 || mark.nType == 2 || mark.nType == 3)
		{
			validMarkinfos.push_back(mark);
		}
	}

	auto& mark = markinfo;
	for (int i = 0; i < allDisease.size(); ++i)
	{
		auto & dis = allDisease.at(i);

		bool isValid = true;
		//根据markinfo进行过滤
		for (auto mark : validMarkinfos)
		{


			if (dis.dMileage >= mark.dEnclMile)
				//if (dis.dDmi *line >= mark.dTrueMile *line)
			{
				if (mark.nType == 0)
				{
					int surfaceType = 0;
					QString surStr = QString::fromLocal8Bit(mark.strMark);
					if (surStr.contains(QStringLiteral("沥青")))
					{
						surfaceType = 0;
					}
					else if (surStr.contains(QStringLiteral("水泥")))
					{
						surfaceType = 1;
					}
					else
					{
						surfaceType = 2;
					}

					if (dis.nRSurfaceType != surfaceType)
					{
						isValid = false;

					}
					else
					{
						isValid = true;
					}
				}
			}
		}
		/*if (dis.dRoadWidth != setInfo.dRoadWidth)
		{
			isValid = false;
		}*/
		if (!isLine)
		{
			if (dis.dDmiStart == dis.dDmiEnd)
			{
				isValid = false;
			}
		}
		
		if (isValid)
		{
			returnDisease.push_back(dis);
		}


	}
	std::sort(returnDisease.begin(), returnDisease.end());
}

void hnRoadDiseaseTable::setDeleteDisease( hnRoadDiseaseInfo& dis)
{
	QString diseaseAtt = QString::fromLocal8Bit(dis.strAddFile4);
	if (diseaseAtt.isEmpty() || diseaseAtt == "0")
	{
		strcpy(dis.strAddFile4, "2");
	}
	else if (diseaseAtt == "1")
	{
		strcpy(dis.strAddFile4, "3");
	}
	QDateTime currentDataTime = QDateTime::currentDateTime();
	QString time = currentDataTime.toString();
	auto s1 = time.toLocal8Bit();
	auto s2 = s1.toStdString();
	strcpy(dis.strAddFile5, s2.c_str());
}

void hnRoadDiseaseTable::setAddDisease(hnRoadDiseaseInfo & dis)
{
	QString diseaseAtt = QString::fromLocal8Bit(dis.strAddFile4);
	if (diseaseAtt.isEmpty() )
	{
		strcpy(dis.strAddFile4, "0");
	}
	//如果是人工修改了病害 就全部调整为人工病害
	if (diseaseAtt == "2"|| diseaseAtt == "3"|| diseaseAtt =="1")
	{
		strcpy(dis.strAddFile4, "0");
	}
	 
	QDateTime currentDataTime = QDateTime::currentDateTime();
	QString time = currentDataTime.toString();
	auto s1 = time.toLocal8Bit();
	auto s2 = s1.toStdString();
	strcpy(dis.strAddFile5, s2.c_str());
}

 