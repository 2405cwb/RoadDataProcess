#include "hnPcdCoordinate.h"
#include "iscandb.hpp"

#define  PI 3.1415926
hnPcdCoordinate::hnPcdCoordinate()
{
	m_utc_to_gps_second = 0.0;
	m_nPreIndex = -1;
}

hnPcdCoordinate::~hnPcdCoordinate()
{

}

 

bool hnPcdCoordinate::loadPosData(const char* strPosPath, std::vector<POS_STRUCT_INFO>& vecInfo ,bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) /*= NULL*/, bool cancel )
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
	POS_STRUCT_INFO infoTmp;
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
		POS_STRUCT_INFO info;
		nSize = info.serialize(strData);
		if (nSize)
		{
			vecInfo[nCount] = info;
			nCount++;

			if (pProgress)
			{
				float startOffset = 0.0f;
				float weight = 0.7f;
				float fProgress = startOffset + (static_cast<float>(nCount) / file_line_count) *weight;

				if (fProgress >= 0.4)
				{
					fProgress = 0.4;
				} 
				pProgress(fProgress, "加载POS", false);
			}

			// 容器逐渐扩大
			if (nCount >= vecInfo.size())
			{
				vecInfo.resize(vecInfo.size() + nPerSize);
			}
		}

	}

	vecInfo.resize(nCount);
	fclose(ptrFile);


	// 将经纬度坐标转换投影坐标;
	convertBlhToNeh(vecInfo);

	return true;
}

//1.读POS得到经纬度// 加载POS数据至内存
bool hnPcdCoordinate::loadPosDataTest(const char* strPosPath, std::vector<POS_STRUCT_INFO>& vecInfo, double dFirGpTime)
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
	//FILE* ptrFile = fopen(strPosPath, "rt");
	//while (!feof(ptrFile))
	//{
	//	fgets(strData, 1024, ptrFile);
	//	file_line_count++;
	//}
	//fclose(ptrFile);
	FILE* ptrFile = fopen(strPosPath, "rt");

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
	POS_STRUCT_INFO infoTmp;
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
		POS_STRUCT_INFO info;
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

	return true;
}

//2.经纬度转绝对坐标XYZ// 将vecInfo中的经纬度坐标转换投影坐标;
void hnPcdCoordinate::convertBlhToNeh(std::vector<POS_STRUCT_INFO>& vecInfo)
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

	// 数据转换对象;
	IHdPJTranslator* ptr_pj_translator = NULL;
	CreateIHdPJTranslator(&ptr_pj_translator);

	ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	int ncount = vecInfo.size();
	double* degree_lat = new double[ncount];
	double* degree_long = new double[ncount];
	double* east = new double[ncount];
	double* north = new double[ncount];
	double* height = new double[ncount];

	double tmp_north = 0.0;
	double tmp_east = 0.0;
	double tmp_height = 0.0;

	double temp_north = 0.0;
	double temp_east = 0.0;
	double temp_height = 0.0;

	POS_STRUCT_INFO pos_info;

	for (int i = 0; i < vecInfo.size(); i++)
	{
		pos_info = vecInfo[i];

		// 经纬度转投影坐标
		tmp_north = pos_info.dLatitude;
		tmp_east = pos_info.dLongitude;
		tmp_height = pos_info.dHeight;

		temp_north = 0.0;
		temp_east = 0.0;
		temp_height = 0.0;

		ptr_pj_translator->Translator(pos_info.dLongitude, pos_info.dLatitude, pos_info.dHeight, &temp_east, &temp_north, &temp_height);

		pos_info.dNorthCoord = temp_north;
		pos_info.dEastCoord = temp_east;
		pos_info.dHeight = temp_height;

		vecInfo[i] = pos_info;
	}

	delete[] degree_lat;
	degree_lat = NULL;
	delete[] degree_long;
	degree_long = NULL;

	delete[] north;
	north = NULL;
	delete[] east;
	east = NULL;

	DestroyIHdPJTranslator(ptr_pj_translator);

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
	QString str_temp_path = QString::fromLocal8Bit(str_iscan_para_path);
	QString str_config_dir_path = str_temp_path.left(str_temp_path.lastIndexOf('/'));
	str_config_dir_path += "/iScan-Route.config";
	QFileInfo file_info;
	if (file_info.exists(str_config_dir_path))
	{
		QFile file(str_config_dir_path);
		if (!file.open(QFile::ReadOnly | QFile::Text))
		{
			return false;
		}

		// 一次读取全部;
		QTextStream in(&file);
		QString file_data = in.readAll();
		file.close();

		// 解析字符串;
		QStringList qstr_list;
		qstr_list = file_data.split(' ');
		for (int nn = 0; nn < qstr_list.size(); nn++)
		{
			QString str = qstr_list[nn];
			if (str.startsWith("LeapSec=")) // 以LeapSec开头
			{
				int time_leap = 0;
				sscanf_s(str.toLocal8Bit().data(), "LeapSec=\"%d\"", &time_leap);
				m_utc_to_gps_second = 0.0 - time_leap;
				break;
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

//4.获得拟合点云的帧数中xyz和对应的时间t
//void hnPcdCoordinate::GetPtsCoordinateAndGpsTime(vector<double>&vecGpsTime, vector<PointXYZIRGB>&vecPts)
//{
//	m_vecGpsTime = vecGpsTime;
//	m_vecPts = vecPts;
//}

//5通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;根据时间查询距离该点时间最近的记录值索引;
bool hnPcdCoordinate::linearInsertPos(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo, POS_STRUCT_INFO& insertResult, int& nearestIndex)
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
	POS_STRUCT_INFO& curInfo = vecInfo[nearestIndex];
	POS_STRUCT_INFO& nextInfo = vecInfo[nearestIndex + 1];

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
int hnPcdCoordinate::findIndexByGpsTime(double gpsTime, vector<POS_STRUCT_INFO>& vecInfo)
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

	if (bFind)
	{
		m_nPreIndex = curIndex;
	}

	return bFind ? curIndex : -1;
}

//6计算点的绝对三维坐标，通过激光到pos，pos到绝对坐标系进行矩阵运算，坐标值传入传出；
void hnPcdCoordinate::calcuCoord(Eigen::Matrix<double, 4, 4> &PtsToPos, POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz)
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

void hnPcdCoordinate::dTo2d(Eigen::Matrix<double, 4, 4> &PtsToPos, POS_STRUCT_INFO& posInfo, double& dx, double& dy, double& dz)
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

//Pos位置找帧
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











