#include "hnPcdCoordinate.h"
#include "iscandb.hpp"
#include "..\hnCommon\hnCommonFun.h"
#include "../hdGeoPosition/GeoProjection/src/emap_prjtrans_base.h"

using namespace hnCommon;

#define PI_TO_180_DEGREE 57.295779513082320876798154814105
using namespace Eigen;
const double PI64 = 3.1415926535897932384626433832795028841971693993751;


hnPcdCoordinate::hnPcdCoordinate()
{
	loadCallback = NULL;
	m_utc_to_gps_second = 0.0;
	m_ptr_convert_translator = NULL;
}

hnPcdCoordinate::~hnPcdCoordinate()
{

}

//时间转换
bool hnPcdCoordinate::UTCT2GPST(const hnSynInfo& stTime, int& nGpsWeek,
	double& dGpsSeconds, double dGPSSubUTC/* = 18.0*/)
{
	int dayofw(0), dayofy(0), yr(0), ttlday(0), m(0), weekno(0);
	const  int  dinmth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	//  Convert day, month and year to day of year 
	if (stTime.timeData.month == 1)
	{
		dayofy = stTime.timeData.day;
	}
	else
	{
		dayofy = 0;
		for (m = 1; m <= (stTime.timeData.month - 1); m++)
		{
			dayofy += dinmth[m];
			if (m == 2)
			{
				if (stTime.timeData.year % 4 == 0 && stTime.timeData.year % 100 != 0 ||
					stTime.timeData.year % 400 == 0)
				{
					dayofy += 1;
				}
			}
		}
		dayofy += stTime.timeData.day;
	}
	//  Convert day of year and year into week number and day of week 
	ttlday = 360;
	for (yr = 1981; yr <= (stTime.timeData.year - 1); yr++)
	{
		ttlday += 365;
		if (yr % 4 == 0 && yr % 100 != 0 || yr % 400 == 0)
		{
			ttlday += 1;
		}
	}
	ttlday += dayofy;
	weekno = ttlday / 7;
	dayofw = ttlday - 7 * weekno;

	nGpsWeek = weekno;

	//2021.9.13 ly 计算将毫秒和微秒整合之后/1000000.
	dGpsSeconds = dayofw * 86400.0 + stTime.timeData.hour * 3600.0 + stTime.timeData.minute * 60.0 +
		stTime.timeData.second + (stTime.timeData.milliSecond * 1000 + stTime.timeData.wMilliSecond) / 1000000.;

	dGpsSeconds += dGPSSubUTC;
	if (dGpsSeconds > 7 * 24 * 3600)
	{
		dGpsSeconds -= 7 * 24 * 3600;
		nGpsWeek += 1;
	}

	return true;
}


//1.读POS得到经纬度// 加载POS数据至内存
bool hnPcdCoordinate::loadPosData(const char* strPosPath, std::vector<hnPosInfo>& vecInfo)
{
	// 检查文件是否存在
	if (_access(strPosPath, 0) != 0)
	{
		return 0;
	}

	// 中间文件用于读取数据
	char strData[1024];
	memset(strData, 0, 1024);

	// 读取文件
	int file_line_count = 0;
	bool bFindData = false;
	FILE* ptrFile = fopen(strPosPath, "rt");
	while (!feof(ptrFile))
	{
		fgets(strData, 1024, ptrFile);
		file_line_count++;
	}
	fclose(ptrFile);
	ptrFile = fopen(strPosPath, "rt");

	// 读取第一行数据
	fgets(strData, 1024, ptrFile);
	string strLine = strData;
	int nPos = strLine.find_first_of('.');
	if (nPos > 0 && nPos <= 10)
	{
		bFindData = true;
	}

	// 迭代剔除前面的n行数据
	while (!bFindData)
	{
		// 读取一行数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);
		strLine = strData;
		nPos = strLine.find_first_of('.');
		if (nPos > 0 && nPos <= 10)
		{
			bFindData = true;
		}
	}

	// 定义存储数据的vector
	int nPerSize = 50000;
	int nCount = 0;
	vecInfo.resize(nPerSize);

	// 找到后，进行解析
	hnPosInfo infoTmp;
	bool nSize = infoTmp.serialize(strData);
	if (!nSize)
	{
		fclose(ptrFile);
		return 0;
	}

	// 第一条记录也要存储
	vecInfo[nCount] = infoTmp;
	nCount++;

	// 读取获取全部数据
	while (!feof(ptrFile) /*&& m_is_running*/)
	{
		// 读取数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);

		// 解析数据
		hnPosInfo info;
		nSize = info.serialize(strData);
		if (nSize)
		{
			vecInfo[nCount] = info;
			nCount++;

			// 容器逐渐扩大
			if (nCount >= vecInfo.size())
			{
				vecInfo.resize(vecInfo.size() + nPerSize);
			}
		}

		if (loadCallback && nCount % 1000 == 0)
		{
			loadCallback(1.0 * nCount / file_line_count, "读取POS数据...");
		}

	}

	vecInfo.resize(nCount);
	fclose(ptrFile);


	// 将经纬度坐标转换投影坐标;
	convertBlhToNeh(vecInfo);
	if (loadCallback)
	{
		loadCallback(1.0, "POS加载完成");
	}

	return true;
}

//1.读POS得到经纬度// 加载POS数据至内存
bool hnPcdCoordinate::loadPosDataTest(const char* strPosPath, std::vector<hnPosInfo>& vecInfo, double dFirGpTime)
{
	// 检查文件是否存在
	if (_access(strPosPath, 0) != 0)
	{
		return 0;
	}

	// 中间文件用于读取数据
	char strData[1024];
	memset(strData, 0, 1024);

	// 读取文件
	int file_line_count = 0;
	bool bFindData = false;
	FILE* ptrFile = fopen(strPosPath, "rt");
	while (!feof(ptrFile))
	{
		fgets(strData, 1024, ptrFile);
		file_line_count++;
	}
	fclose(ptrFile);
	ptrFile = fopen(strPosPath, "rt");

	// 读取第一行数据
	fgets(strData, 1024, ptrFile);
	string strLine = strData;
	int nPos = strLine.find_first_of('.');
	if (nPos > 0 && nPos <= 10)
	{
		bFindData = true;
	}

	// 迭代剔除前面的n行数据
	while (!bFindData)
	{
		// 读取一行数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);
		strLine = strData;
		nPos = strLine.find_first_of('.');
		if (nPos > 0 && nPos <= 10)
		{
			bFindData = true;
		}
	}

	// 定义存储数据的vector
	int nPerSize = 200000;
	int nCount = 0;
	vecInfo.resize(nPerSize);

	// 找到后，进行解析
	hnPosInfo infoTmp;
	bool nSize = infoTmp.serialize(strData);
	if (!nSize)
	{
		fclose(ptrFile);
		return 0;
	}

	// 第一条记录也要存储
	vecInfo[nCount] = infoTmp;
	nCount++;

	// 读取获取全部数据
	while (!feof(ptrFile) /*&& m_is_running*/)
	{
		// 读取数据
		memset(strData, 0, 1024);
		fgets(strData, 1024, ptrFile);

		// 解析数据
		hnPosInfo info;
		nSize = info.serialize(strData);
		if (nSize)
		{
			vecInfo[nCount] = info;
			nCount++;

			// 容器逐渐扩大
			if (nCount >= vecInfo.size())
			{
				vecInfo.resize(vecInfo.size() + nPerSize);
			}

			if (loadCallback && nCount % 1000 == 0)
			{
				loadCallback(1.0 * nCount / file_line_count, "读取POS数据...");
			}
		}
	}

	vecInfo.resize(nCount);
	fclose(ptrFile);

	////////////////////////////////////////////////////////////////////////// 新增
	if (vecInfo.size() > 0)
	{
		int day = (int)(abs(vecInfo[0].dGpsSecond - dFirGpTime) / 24.0 / 3600.0 + 0.5);

		if (dFirGpTime > vecInfo[0].dGpsSecond)
		{
			for (int i = 0; i < vecInfo.size(); i++)
			{
				vecInfo[i].dGpsSecond = vecInfo[i].dGpsSecond + day * 24 * 3600;
			}
		}
		else
		{
			for (int i = 0; i < vecInfo.size(); i++)
			{
				vecInfo[i].dGpsSecond = vecInfo[i].dGpsSecond - day * 24 * 3600;
			}
		}
	}

	// 将经纬度坐标转换投影坐标;
	convertBlhToNeh(vecInfo);

	if (loadCallback)
	{
		loadCallback(1.0, "POS加载完成");
	}

	return true;
}

//2.经纬度转绝对坐标XYZ// 将vecInfo中的经纬度坐标转换投影坐标;
void hnPcdCoordinate::convertBlhToNeh(std::vector<hnPosInfo>& vecInfo, bool isProj)
{
	// 条件判断
	if (vecInfo.size() <= 0)
	{
		return;
	}

	// 设置原始数据属性;
	Spatial_Ref_t m_src_param;
	Spatial_Ref_t m_dst_param;
	memset(&m_src_param, 0, sizeof(m_src_param));
	m_src_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_src_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_src_param.earthType = E_EARTH_TYPE_WGS84;

	// 设置目标数据属性;
	memset(&m_dst_param, 0, sizeof(m_dst_param));
	m_dst_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dst_param.coorUnit = E_COOR_UNIT_TYPE_METER;
	m_dst_param.earthType = E_EARTH_TYPE_WGS84;

	m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
	m_dst_param.W = 3;

	// 中央经线等设置;
	int longtidute_num = (vecInfo[0].dLongitude + 1.5) / 3;
	double centre_longtitude = longtidute_num * 3.0;
	m_dst_param.Lo = centre_longtitude * PI / 180.0;
	m_dst_param.Ko = 1.0;
	m_dst_param.FE = 500000;
	m_dst_param.Bc = 0.0;
	m_dst_param.PH = 0.0;

	if (m_ptr_convert_translator == NULL)
	{
		CreateIHdPJTranslator(&m_ptr_convert_translator);
		m_ptr_convert_translator->SetSrcSpatialRef(&m_src_param);
		m_ptr_convert_translator->SetDstSpatialRef(&m_dst_param);
	}

	// 直接经纬度转换空间直角坐标系;
	convertBLtoProjXYDirect(m_ptr_convert_translator, vecInfo);
	if (isProj)// 若要求转换至投影坐标系;
	{
		double out_x, out_y, out_z;
		for (int i = 0; i < vecInfo.size(); i++)
		{
			POS_STRUCT_INFO& pos_info = vecInfo[i];

			// 空间直角坐标投影转换至投影坐标系（包含四参数、七参数转换等）;
			double dEast, dNorth, dH;
			m_ptr_convert_translator->TranslatorXYZByParam(pos_info.dEastCoord, pos_info.dNorthCoord, pos_info.dHeight, &dEast, &dNorth, &dH);

			//double tempL,tempB,tempH;
			//m_ptr_convert_translator->TransLators_XYZToBL(pos_info.dEastCoord,pos_info.dNorthCoord,pos_info.dHeight, &tempB,&tempL,&tempH);
			//tempL = tempL * PI_TO_180_DEGREE;
			//tempB = tempB * PI_TO_180_DEGREE;

			pos_info.dEastCoord = dEast;
			pos_info.dNorthCoord = dNorth;
			pos_info.dHeight = dH;
		}

		//convertXYZtoProjXYZ(m_ptr_convert_translator,vecInfo);
	}

	//// 数据转换对象;
	//IHdPJTranslator* ptr_pj_translator = NULL;
	//CreateIHdPJTranslator(&ptr_pj_translator);

	//ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	//ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	//int ncount = vecInfo.size();
	//double* degree_lat = new double[ncount];
	//double* degree_long = new double[ncount];
	//double* east = new double[ncount];
	//double* north = new double[ncount];
	//double* height = new double[ncount];

	//double tmp_north = 0.0;
	//double tmp_east = 0.0;
	//double tmp_height = 0.0;

	//double temp_north = 0.0;
	//double temp_east = 0.0;
	//double temp_height = 0.0;

	//hnPosInfo pos_info;

	//for (int i = 0; i < vecInfo.size(); i++)
	//{
	//	pos_info = vecInfo[i];

	//	// 经纬度转投影坐标
	//	tmp_north = pos_info.dLatitude;
	//	tmp_east = pos_info.dLongitude;
	//	tmp_height = pos_info.dHeight;

	//	temp_north = 0.0;
	//	temp_east = 0.0;
	//	temp_height = 0.0;

	//	ptr_pj_translator->Translator(pos_info.dLongitude, pos_info.dLatitude, pos_info.dHeight, &temp_east, &temp_north, &temp_height);

	//	pos_info.dNorthCoord = temp_north;
	//	pos_info.dEastCoord = temp_east;
	//	pos_info.dHeight = temp_height;

	//	vecInfo[i] = pos_info;
	//}

	//delete[] degree_lat;
	//degree_lat = NULL;
	//delete[] degree_long;
	//degree_long = NULL;

	//delete[] north;
	//north = NULL;
	//delete[] east;
	//east = NULL;

	//DestroyIHdPJTranslator(ptr_pj_translator);

	return;
}

//3.读config获得延迟时间18s 并将延迟传出去  用于帧和POS的时间差   设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵; 
bool hnPcdCoordinate::setiScanParaPath(int scan_no, const char* str_iscan_para_path, Eigen::Matrix<double, 4, 4> &PtsToPos)
{
	// 检查文件存在;
	if (_access(str_iscan_para_path, 0) != 0)
	{
		return false;
	}

	// 获取同目录下的config文件;
	string str_temp_path = str_iscan_para_path;
	string str_config_dir_path = str_temp_path.substr(0, str_temp_path.find_last_of('/'));
	str_config_dir_path += "/iScan-Route.config";

	if (_access(str_config_dir_path.c_str(), 0) != 0)
	{
		FILE* pFile = fopen(str_config_dir_path.c_str(), "r");
		if (!pFile)
		{
			return false;
		}

		// 将文件指针放在初始位置
		fseek(pFile, 0, SEEK_SET);
		char str[1024] = { 0 };

		// 索引
		int nIndex = 0;

		vector<string> vecList;
		string strHeader = "";

		// 读取第一行数据
		while (!feof(pFile))
		{
			fgets(str, 1024, pFile);
			
			// 解析字符串;
			vecList.clear();
			splitString(str, " ", vecList);
			for (int nn = 0; nn < vecList.size(); nn++)
			{
				string str = vecList[nn];
				if (str.length() <= 8)
				{
					continue;
				}

				strHeader = str.substr(0, 9);
				if (strHeader == "LeapSec")
				{
					int time_leap = 0;
					sscanf_s(str.data(), "LeapSec=\"%d\"", &time_leap);
					m_utc_to_gps_second = 0.0 - time_leap;
					break;
				}
			}
		}

		
	}

	// 读取ISCAN参数;
	IScanDB iScan;
	if (!iScan.ReadISCANVal(str_iscan_para_path))
	{
		return false;
	}

	//获取参数
	hdHiScanLidarPara iscan_lidar_para;
	iScan.GetISCANVal(iscan_lidar_para);

	// 传入内部;
	setScanPara(iscan_lidar_para.dx, iscan_lidar_para.dy, iscan_lidar_para.dz,
		iscan_lidar_para.dyaw, iscan_lidar_para.dpitch, iscan_lidar_para.droll, PtsToPos);
	return true;
}

//3-1设置扫描的年月日信息,读取ISACN参数  构造按iscan旋转角度方式Z-X-Y构建旋转矩阵;
void hnPcdCoordinate::setScanPara(double laser_to_pos_x, double laser_to_pos_y, double laser_to_pos_z,
	double laser_to_pos_heading, double laser_to_pos_pitch, double laser_to_pos_roll, Eigen::Matrix<double, 4, 4> &PtsToPos)
{
	m_laser_topos_x = laser_to_pos_x;
	m_laser_topos_y = laser_to_pos_y;
	m_laser_topos_z = laser_to_pos_z;
	m_laser_topos_heading = laser_to_pos_heading;
	m_laser_topos_pitch = laser_to_pos_pitch;
	m_laser_topos_roll = laser_to_pos_roll;

	char str[1024];
	sprintf(str, "%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf\n", m_laser_topos_x, m_laser_topos_y, m_laser_topos_z, m_laser_topos_heading, m_laser_topos_pitch, m_laser_topos_roll);
	qErrnoWarning("%s", str);

	// 激光到pos构建的矩阵;
	double arry_pos_temp[16];

	// 度转弧度
	double tmpHeading = m_laser_topos_heading * PI / 180.0;
	double tmpPitch = m_laser_topos_pitch * PI / 180.0;
	double tmpRoll = m_laser_topos_roll * PI / 180.0;
	computeMatrixByIScanAngle(m_laser_topos_x, m_laser_topos_y, m_laser_topos_z,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);
	PtsToPos(0, 0) = arry_pos_temp[0];  PtsToPos(0, 1) = arry_pos_temp[1]; PtsToPos(0, 2) = arry_pos_temp[2]; PtsToPos(0, 3) = arry_pos_temp[3];
	PtsToPos(1, 0) = arry_pos_temp[4];  PtsToPos(1, 1) = arry_pos_temp[5]; PtsToPos(1, 2) = arry_pos_temp[6]; PtsToPos(1, 3) = arry_pos_temp[7];
	PtsToPos(2, 0) = arry_pos_temp[8];  PtsToPos(2, 1) = arry_pos_temp[9]; PtsToPos(2, 2) = arry_pos_temp[10]; PtsToPos(2, 3) = arry_pos_temp[11];
	PtsToPos(3, 0) = arry_pos_temp[12];  PtsToPos(3, 1) = arry_pos_temp[13]; PtsToPos(3, 2) = arry_pos_temp[14]; PtsToPos(3, 3) = arry_pos_temp[15];
}

//3-2按iscan旋转角度方式Z-X-Y构建旋转矩阵;
void hnPcdCoordinate::computeMatrixByIScanAngle(double X, double Y, double Z, double Yaw, double Pitch, double Roll, double *R)
{
	double a1, a2, a3, b1, b2, b3, c1, c2, c3;

	a1 = cos(Roll)*cos(Yaw) + sin(Roll)*sin(Yaw)*sin(Pitch);
	a2 = -cos(Roll)*sin(Yaw) + sin(Roll)*cos(Yaw)*sin(Pitch);
	a3 = -sin(Roll)*cos(Pitch);
	b1 = sin(Yaw)*cos(Pitch);
	b2 = cos(Yaw)*cos(Pitch);
	b3 = sin(Pitch);
	c1 = sin(Roll)*cos(Yaw) - cos(Roll)*sin(Yaw)*sin(Pitch);
	c2 = -sin(Roll)*sin(Yaw) - cos(Roll)*cos(Yaw)*sin(Pitch);
	c3 = cos(Roll)*cos(Pitch);

	double fScale = 1.0;
	R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;
}

////4.获得拟合点云的帧数中xyz和对应的时间t
//void hnPcdCoordinate::GetPtsCoordinateAndGpsTime(vector<double>&vecGpsTime, vector<PointXYZIRGB>&vecPts)
//{
//	m_vecGpsTime = vecGpsTime;
//	m_vecPts = vecPts;
//}

//5通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;根据时间查询距离该点时间最近的记录值索引;
bool hnPcdCoordinate::linearInsertPos(double gpsTime, vector<hnPosInfo>& vecInfo, hnPosInfo& insertResult, int& nearestIndex)
{
	// 条件判断，获取时间上距离最近点，返回为较低点
	//int nearestIndex = -1;
	//if (m_nPreIndex < 0)
	{
		nearestIndex = findIndexByGpsTime(gpsTime, vecInfo);
	}
	//else
	//{
	//	// 时间上应是连续向下的
	//	if (m_nPreIndex > 0)
	//	{
	//		// 检查上一个点的pos时间与该点时间，可以根据POS频率直接推算出来所在POS位置，尽量避免二分查找过程;
	//		POS_STRUCT_INFO& prePos = vecInfo[m_nPreIndex];
	//		POS_STRUCT_INFO& nextPos = vecInfo[m_nPreIndex + 1];
	//		double time_size = nextPos.dGpsSecond - prePos.dGpsSecond;
	//		double tmpTime = gpsTime - prePos.dGpsSecond;
	//		int timeAddCount = (int)(tmpTime / time_size);
	//		nearestIndex = m_nPreIndex + timeAddCount;
	//		if ( nearestIndex < vecInfo.size())
	//		{
	//			POS_STRUCT_INFO& curPos = vecInfo[nearestIndex];
	//			if (gpsTime >= curPos.dGpsSecond && gpsTime <= (curPos.dGpsSecond+time_size))
	//			{
	//				// 更新记录m_nPreIndex时间信息索引
	//				m_nPreIndex = nearestIndex;
	//			}
	//			else
	//			{
	//				// 计算处理不正确，则重新进行二分查找;
	//				nearestIndex = findIndexByGpsTime(gpsTime,vecInfo);
	//			}
	//		}
	//	}
	//}

	if (nearestIndex < 0 || nearestIndex >= vecInfo.size() - 1)
	{
		return false;
	}

	// 获取点值
	hnPosInfo& curInfo = vecInfo[nearestIndex];
	hnPosInfo& nextInfo = vecInfo[nearestIndex + 1];

	// 进行插值比例值计算
	double fscale = (gpsTime - curInfo.dGpsSecond) / (nextInfo.dGpsSecond - curInfo.dGpsSecond);
	insertResult.dEastCoord = curInfo.dEastCoord + (nextInfo.dEastCoord - curInfo.dEastCoord) * fscale;
	insertResult.dNorthCoord = curInfo.dNorthCoord + (nextInfo.dNorthCoord - curInfo.dNorthCoord) * fscale;
	insertResult.dHeight = curInfo.dHeight + (nextInfo.dHeight - curInfo.dHeight) * fscale;
	insertResult.dHeading = curInfo.dHeading + (nextInfo.dHeading - curInfo.dHeading) * fscale;
	insertResult.dPitch = curInfo.dPitch + (nextInfo.dPitch - curInfo.dPitch) * fscale;
	insertResult.dRoll = curInfo.dRoll + (nextInfo.dRoll - curInfo.dRoll) * fscale;
	insertResult.dLatitude = curInfo.dLatitude + (nextInfo.dLatitude - curInfo.dLatitude) * fscale;
	insertResult.dLongitude = curInfo.dLongitude + (nextInfo.dLongitude - curInfo.dLongitude) * fscale;

	//// 测试代码;
	//double tmpx = 0.0;
	//double tmpy = 0.0;
	//// 数据转换对象;
	//IHdPJTranslator* ptr_pj_translator = NULL;	
	//CreateIHdPJTranslator(&ptr_pj_translator);

	//ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	//ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	//ptr_pj_translator->Translator(insertResult.dLongitude,insertResult.dLatitude,insertResult.dHeight,&insertResult.dEastCoord,&insertResult.dNorthCoord,&tmpy);

	//DestroyIHdPJTranslator(ptr_pj_translator);


	return true;
}

// 5-1根据时间查询距离该点时间最近的记录值索引;
int hnPcdCoordinate::findIndexByGpsTime(double gpsTime, vector<hnPosInfo>& vecInfo)
{
	bool bFind = false;
	int curIndex = 0;
	int lowIndex = 0;
	int highIndex = vecInfo.size() - 1;
	while (highIndex >= lowIndex)
	{
		int m = (highIndex + lowIndex) / 2;
		if (gpsTime < vecInfo[m].dGpsSecond)
		{
			// 比该时间节点要小，但是如果大于上一个时间节点，表示位于两者之间
			highIndex = m - 1;
			if (highIndex < 0 || highIndex >= vecInfo.size())
			{
				bFind = false;
				break;
			}

			if (gpsTime >= vecInfo[highIndex].dGpsSecond)
			{
				curIndex = m - 1;
				bFind = true;
				break;
			}
		}
		else if (gpsTime > vecInfo[m].dGpsSecond)
		{
			// 比该时间节点要大，但是比下一个时间节点小，则表示位于该节点段
			lowIndex = m + 1;
			if (lowIndex < 0 || lowIndex >= vecInfo.size())
			{
				bFind = false;
				break;
			}

			if (gpsTime <= vecInfo[lowIndex].dGpsSecond)
			{
				curIndex = m;
				bFind = true;
				break;
			}
		}
		else
		{
			curIndex = m;
			bFind = true;
			break;
		}
	}

	return bFind ? curIndex : -1;
}

//6计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
void hnPcdCoordinate::calcuCoord2(Eigen::Matrix<double, 4, 4> &PtsToPos, hnPosInfo& posInfo, double& dx, double& dy, double& dz)
{
	// pos to world
	double arry_pos_temp[16] = { 0 };

	// 度转弧度
	double tmpHeading = posInfo.dHeading * PI / 180.0;
	double tmpPitch = posInfo.dPitch * PI / 180.0;
	double tmpRoll = posInfo.dRoll * PI / 180.0;
	computeMatrixByIScanAngle(posInfo.dEastCoord, posInfo.dNorthCoord, posInfo.dHeight,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);

	// 矩阵信息赋值
	Eigen::MatrixXd mat_pos_to_world(4, 4);
	mat_pos_to_world(0, 0) = arry_pos_temp[0];  mat_pos_to_world(0, 1) = arry_pos_temp[1]; mat_pos_to_world(0, 2) = arry_pos_temp[2]; mat_pos_to_world(0, 3) = arry_pos_temp[3];
	mat_pos_to_world(1, 0) = arry_pos_temp[4];  mat_pos_to_world(1, 1) = arry_pos_temp[5]; mat_pos_to_world(1, 2) = arry_pos_temp[6]; mat_pos_to_world(1, 3) = arry_pos_temp[7];
	mat_pos_to_world(2, 0) = arry_pos_temp[8];  mat_pos_to_world(2, 1) = arry_pos_temp[9]; mat_pos_to_world(2, 2) = arry_pos_temp[10]; mat_pos_to_world(2, 3) = arry_pos_temp[11];
	mat_pos_to_world(3, 0) = arry_pos_temp[12];  mat_pos_to_world(3, 1) = arry_pos_temp[13]; mat_pos_to_world(3, 2) = arry_pos_temp[14]; mat_pos_to_world(3, 3) = arry_pos_temp[15];

	// 点坐标构建矩阵


	Eigen::MatrixXd mat_point_pos(4, 1);
	mat_point_pos(0, 0) = dx;
	mat_point_pos(1, 0) = dy;
	mat_point_pos(2, 0) = dz;
	mat_point_pos(3, 0) = 1.0;

	// 结果值记录
	Eigen::MatrixXd mat_point_result(4, 1);
	mat_point_result = mat_pos_to_world * PtsToPos * mat_point_pos;
	dx = mat_point_result(0, 0);
	dy = mat_point_result(1, 0);
	dz = mat_point_result(2, 0);

	//// 存在坐标转换;
	//COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//COMBINE_COORD_CONVERT_SET_STRUCT convert_info = m_project_setting->getCoordConvertInfo();
	//int use_convert = convert_info.use_coord_convert;
	//if (m_ptr_convert_translator && use_convert )
	//{
	//	double temp_x = dx;
	//	double temp_y = dy;
	//	double temp_z = dz;
	//	int convert_model = convert_info.use_coord_convert_model;
	//	if (convert_model == 0)
	//	{
	//		// 四参数加高程拟合转换，输入北东高坐标系，返回东北高坐标系;
	//		m_ptr_convert_translator->TranslatorByParam(temp_x,temp_y,temp_z,&dx,&dy,&dz);

	//		// 高程拟合计算;
	//		double ddH = 0.0;
	//		m_ptr_convert_translator->HFixCalus(convert_info.height_fit_n0,convert_info.height_fit_e0,&ddH);
	//		dz += ddH;
	//	}
	//	else if (convert_model == 1)
	//	{
	//		//temp_x = 511780.2283;
	//		//temp_y = 4496008.5322;
	//		//temp_z = 1231.7256;

	//		// 未设置椭球体，目前仅WGS84椭球体采用该计算;
	//		hdProjectedCoordinateSystem projected_coord_system;
	//		//projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
	//		projected_coord_system.setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//		projected_coord_system.m_proj_height = combine_coord_project_info.project_height;
	//		//projected_coord_system.setCentralMeridian(96.0);
	//		//projected_coord_system.m_proj_height = 0.0;
	//		projected_coord_system.setFalseEasting(combine_coord_project_info.east_offset);
	//		projected_coord_system.setFalseNorthing(0.0);
	//		projected_coord_system.m_mean_latitude = combine_coord_project_info.average_latitude;

	//		projected_coord_system.GaussProject_NE2BL(temp_y,temp_x);

	//		// 直接进行七参数转换，获得经纬度;
	//		m_ptr_convert_translator->TranslatorByParam(temp_x,temp_y,temp_z,&dx,&dy,&dz);

	//		temp_x = dy * 180.0 / PI64;
	//		temp_y = dx * 180.0 / PI64;

	//		//// 经纬度转换为投影坐标;
	//		//hdProjectedCoordinateSystem projected_coord_system1;
	//		////projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
	//		//projected_coord_system1.setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//		//projected_coord_system1.m_proj_height = combine_coord_project_info.project_height;
	//		//projected_coord_system1.setFalseEasting(combine_coord_project_info.east_offset);
	//		//projected_coord_system1.setFalseNorthing(0.0);
	//		//projected_coord_system1.m_mean_latitude = combine_coord_project_info.average_latitude;
	//		projected_coord_system.GaussProject_BL2NE(temp_y,temp_x);
	//		dx = temp_x;
	//		dy = temp_y;

	//		//double test = 0.0;
	//	}
	//}
}

void hnPcdCoordinate::dTo2d(Eigen::Matrix<double, 4, 4> &PtsToPos, hnPosInfo& posInfo, double& dx, double& dy, double& dz)
{
	// pos to world
	double arry_pos_temp[16] = { 0 };

	// 度转弧度
	double tmpHeading = posInfo.dHeading * PI / 180.0;
	double tmpPitch = posInfo.dPitch * PI / 180.0;
	double tmpRoll = posInfo.dRoll * PI / 180.0;
	computeMatrixByIScanAngle(posInfo.dEastCoord, posInfo.dNorthCoord, posInfo.dHeight,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);

	// 矩阵信息赋值
	/*Eigen::MatrixXd mat_pos_to_world(4, 4);*/
	Eigen::Matrix<double, 4, 4>mat_pos_to_world(4, 4);
	mat_pos_to_world(0, 0) = arry_pos_temp[0];  mat_pos_to_world(0, 1) = arry_pos_temp[1]; mat_pos_to_world(0, 2) = arry_pos_temp[2]; mat_pos_to_world(0, 3) = arry_pos_temp[3];
	mat_pos_to_world(1, 0) = arry_pos_temp[4];  mat_pos_to_world(1, 1) = arry_pos_temp[5]; mat_pos_to_world(1, 2) = arry_pos_temp[6]; mat_pos_to_world(1, 3) = arry_pos_temp[7];
	mat_pos_to_world(2, 0) = arry_pos_temp[8];  mat_pos_to_world(2, 1) = arry_pos_temp[9]; mat_pos_to_world(2, 2) = arry_pos_temp[10]; mat_pos_to_world(2, 3) = arry_pos_temp[11];
	mat_pos_to_world(3, 0) = arry_pos_temp[12];  mat_pos_to_world(3, 1) = arry_pos_temp[13]; mat_pos_to_world(3, 2) = arry_pos_temp[14]; mat_pos_to_world(3, 3) = arry_pos_temp[15];

	// 点坐标构建矩阵


	Eigen::MatrixXd mat_point_pos(4, 1);
	mat_point_pos(0, 0) = dx;
	mat_point_pos(1, 0) = dy;
	mat_point_pos(2, 0) = dz;
	mat_point_pos(3, 0) = 1.0;

	// 结果值记录 B=abA  A=b^(-1)a^(-1)B

	Eigen::Matrix<double, 4, 4> mat_pos_to_world_inverse = mat_pos_to_world.inverse();
	Eigen::Matrix<double, 4, 4> PtsToPos_inverse = PtsToPos.inverse();
	Eigen::MatrixXd mat_point_result(4, 1);
	//mat_point_result = mat_pos_to_world * PtsToPos * mat_point_pos;
	/*mat_point_result = mat_pos_to_world.inverse() * PtsToPos.inverse() * mat_point_pos;*/
	mat_point_result = PtsToPos_inverse * mat_pos_to_world_inverse * mat_point_pos;
	dx = mat_point_result(0, 0);
	dy = mat_point_result(1, 0);
	dz = mat_point_result(2, 0);
}

void hnPcdCoordinate::convertBLtoProjXYDirect(IHdPJTranslator* ptr_pj_translator, std::vector<POS_STRUCT_INFO>& vecInfo)
{
	if (!ptr_pj_translator)
	{
		return;
	}

	//int ncount = vecInfo.size();
	//double* degree_lat = new double[ncount];
	//double* degree_long = new double[ncount];
	//double* east = new double[ncount];
	//double* north = new double[ncount];

	double out_x, out_y, out_z;
	for (int i = 0; i < vecInfo.size(); i++)
	{
		POS_STRUCT_INFO& pos_info = vecInfo[i];

		ptr_pj_translator->TransLators_BLToXYZ(pos_info.dLatitude, pos_info.dLongitude, pos_info.dHeight, &out_x, &out_y, &out_z);

		pos_info.dEastCoord = out_x;
		pos_info.dNorthCoord = out_y;
		pos_info.dHeight = out_z;

		////// 经纬度转投影坐标;
		////degree_lat[i] = pos_info.dLatitude;
		////degree_long[i] = pos_info.dLongitude;

		//if (loadCallback && i % 5000 == 0)
		//{
		//	loadCallback(i * 1.0 / vecInfo.size(), "POS转换...");
		//}

		//if (m_use_thread && i % 5000 == 0)
		//{
		//	setProgress(i * 1.0 / vecInfo.size(), "POS转换...");
		//}
	}
}

void hnPcdCoordinate::calcuCoord(Eigen::Matrix<double, 4, 4> &PtsToPos,POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz)
{
	// pos to world
	double arry_pos_temp[16] = { 0 };

	// 度转弧度;
	double tmpHeading = posInfo.dHeading * PI64 / 180.0;
	double tmpPitch = posInfo.dPitch * PI64 / 180.0;
	double tmpRoll = posInfo.dRoll * PI64 / 180.0;

	//获得矩阵的旋转 构建车体坐标系的旋转矩阵  IE解算结果:运载体坐标系下 tmpHeading，tmpPitch，tmpRoll
	computeMatrixByIScanAngle(0.0, 0.0, 0.0,
		tmpHeading, tmpPitch, tmpRoll, arry_pos_temp);

	// 矩阵信息赋值;
	MatrixXd mat_pos_to_world(4, 4);
	mat_pos_to_world(0, 0) = arry_pos_temp[0];  mat_pos_to_world(0, 1) = arry_pos_temp[1]; mat_pos_to_world(0, 2) = arry_pos_temp[2]; mat_pos_to_world(0, 3) = arry_pos_temp[3];
	mat_pos_to_world(1, 0) = arry_pos_temp[4];  mat_pos_to_world(1, 1) = arry_pos_temp[5]; mat_pos_to_world(1, 2) = arry_pos_temp[6]; mat_pos_to_world(1, 3) = arry_pos_temp[7];
	mat_pos_to_world(2, 0) = arry_pos_temp[8];  mat_pos_to_world(2, 1) = arry_pos_temp[9]; mat_pos_to_world(2, 2) = arry_pos_temp[10]; mat_pos_to_world(2, 3) = arry_pos_temp[11];
	mat_pos_to_world(3, 0) = arry_pos_temp[12];  mat_pos_to_world(3, 1) = arry_pos_temp[13]; mat_pos_to_world(3, 2) = arry_pos_temp[14]; mat_pos_to_world(3, 3) = arry_pos_temp[15];

	// 点坐标构建矩阵;
	MatrixXd mat_point_pos(4, 1);
	mat_point_pos(0, 0) = dx;
	mat_point_pos(1, 0) = dy;
	mat_point_pos(2, 0) = dz;
	mat_point_pos(3, 0) = 1.0;

	//扫描仪点 坐标系点只是经过两次旋转矩阵 转换到运载体坐标系下   
	MatrixXd mat_point_result(4, 1);
	mat_point_result = mat_pos_to_world * PtsToPos * mat_point_pos;

	//运载体坐标系下   dx,dy,dz 
	dx = mat_point_result(0, 0);
	dy = mat_point_result(1, 0);
	dz = mat_point_result(2, 0);

	// 空间直角坐标系直接参与计算;  获得矩阵的旋转平移量  坐标原点为地心 从运载体坐标系转换到地心坐标系（ECEF）也就是地心直角坐标系
	computeMatrixByBL(posInfo.dEastCoord, posInfo.dNorthCoord, posInfo.dHeight, posInfo.dLongitude * PI64 / 180.0, posInfo.dLatitude * PI64 / 180.0, arry_pos_temp);
	MatrixXd mat_near_to_world(4, 4);
	mat_near_to_world(0, 0) = arry_pos_temp[0];  mat_near_to_world(0, 1) = arry_pos_temp[1]; mat_near_to_world(0, 2) = arry_pos_temp[2]; mat_near_to_world(0, 3) = arry_pos_temp[3];
	mat_near_to_world(1, 0) = arry_pos_temp[4];  mat_near_to_world(1, 1) = arry_pos_temp[5]; mat_near_to_world(1, 2) = arry_pos_temp[6]; mat_near_to_world(1, 3) = arry_pos_temp[7];
	mat_near_to_world(2, 0) = arry_pos_temp[8];  mat_near_to_world(2, 1) = arry_pos_temp[9]; mat_near_to_world(2, 2) = arry_pos_temp[10]; mat_near_to_world(2, 3) = arry_pos_temp[11];
	mat_near_to_world(3, 0) = arry_pos_temp[12];  mat_near_to_world(3, 1) = arry_pos_temp[13]; mat_near_to_world(3, 2) = arry_pos_temp[14]; mat_near_to_world(3, 3) = arry_pos_temp[15];

	MatrixXd mat_point_result0(4, 1);
	mat_point_result0(0, 0) = mat_point_result(1, 0);
	mat_point_result0(1, 0) = mat_point_result(0, 0);
	mat_point_result0(2, 0) = -1.0 * mat_point_result(2, 0);
	mat_point_result0(3, 0) = 1.0;

	// 新的空间直角坐标系; 获得x,y,z将运载体坐标系下的点mat_point_result0转换为地心坐标系mat_point_result_new
	MatrixXd mat_point_result_new(4, 1);
	mat_point_result_new = mat_near_to_world * mat_point_result0;
	dx = mat_point_result_new(0, 0);
	dy = mat_point_result_new(1, 0);
	dz = mat_point_result_new(2, 0);

	// 空间直角坐标经四参数、七参数等转换经纬度再转换投影坐标;  输入为直角坐标系x,y,z输出为经过经纬度转换再经过投影坐标转换获得东北高坐标系（也就是运载体坐标系）  内部关注下
	double dEast, dNorth, dH;
	m_ptr_convert_translator->TranslatorXYZByParam(dx, dy, dz, &dEast, &dNorth, &dH);

	dx = dEast;
	dy = dNorth;
	dz = dH;
}

void hnPcdCoordinate::computeMatrixByBL(double X, double Y, double Z, double L, double B, double *R)
{
	double a1, a2, a3, b1, b2, b3, c1, c2, c3;

	a1 = -1.0 * cos(L) * sin(B);
	a2 = -1.0 * sin(L) * sin(B);
	a3 = cos(B);
	b1 = -1.0 * sin(L);
	b2 = cos(L);
	b3 = 0.0;
	c1 = -1.0 * cos(L) * cos(B);
	c2 = -1.0 * sin(L) * cos(B);
	c3 = -1.0 * sin(B);

	//double fScale = 1.0;
	//R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;
	//R[3] = a2 * fScale;  R[4] = b2 * fScale;  R[5] = c2 * fScale;
	//R[6] = a3 * fScale;  R[7] = b3 * fScale;  R[8] = c3 * fScale;

	//double fScale = 1.0;
	//R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	//R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	//R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	//R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;

	double fScale = 1.0;
	R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;
}

//二分法查找最近位置double
bool hnPcdCoordinate::DichotomyFindNearestLoc(vector<double>&vec, double Target, int &nNearestLoc)
{
	if (vec.empty())
	{
		return false;
	}

	int index1 = 0;
	int index2 = vec.size() - 1;
	int mid = 0;

	while (index1 <= index2)
	{
		mid = (index1 + index2) / 2;

		if (fabs(vec[mid] - Target) < 0.00001)
		{
			nNearestLoc = mid;
			return true;
		}

		if (vec[mid] > Target)
		{
			index2 = mid - 1;
		}

		if (vec[mid] < Target)
		{
			index1 = mid + 1;
		}
	}

	if (index2 < 0 || index1 < 0)
	{
		nNearestLoc = 0;
		return true;
	}

	if (index2 >= vec.size() || index1 >= vec.size())
	{
		nNearestLoc = vec.size() - 1;
		return true;
	}

	if (fabs(Target - vec[index2]) < fabs(Target - vec[index1]))
	{
		nNearestLoc = index2;
	}
	else
	{
		nNearestLoc = index1;
	}
	return true;
}



////Pos位置找帧
//void hnPcdCoordinate::frameByPos(double dGpsTime, vector<hnFrameGpsInfo>&vecFrameGpsInfo,
//	int& nOutFrame)
//{
//	int nSize = vecFrameGpsInfo.size();
//	vector<double>vecGpsTime;
//	vecGpsTime.resize(nSize);
//
//	for (int i = 0; i < nSize; i++)
//	{
//		vecGpsTime[i] = vecFrameGpsInfo[i].gpsTime;
//	}
//
//	int NearestLoc = -1;
//	DichotomyFindNearestLoc(vecGpsTime, dGpsTime,
//		NearestLoc);
//
//	nOutFrame = vecFrameGpsInfo[NearestLoc].frame;
//}











