#include "hnZfsCombine.h"
#include <io.h>
#include "hdMath/hdMath.h"
#include "hdGeoProj/hdGeoProj.h"

#include <qlogging.h>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "hnCombineTimeWriter.h"
#include "..\include\hdPtCloudDriver\hdHlzStreamBuilder.h"
//#include "hdHiScanRoute\hdParamSQLiteDB.h"
#include "hdHiScanRoute\hdHiScanRouteDefines.h"
#include "hnParamSQLiteDB.h"
#include "hnProjectSetting.h"

using namespace hn;

hnZfsCombine::hnZfsCombine(void)
{
	m_ptr_convert_translator = NULL;
	m_nPreIndex = -1;
	m_laser_topos_x = 0.0;
	m_laser_topos_y = 0.0;
	m_laser_topos_z = 0.0;
	m_laser_topos_heading = 0.0;
	m_laser_topos_pitch = 0.0;
	m_laser_topos_roll = 0.0;
	m_use_ctrl_pt = false;
	m_utc_to_gps_second = 0.0;

	loadCallback = NULL;
	wloadCallback = NULL;
	m_project_setting = NULL;
	m_ptr_coord_trans_system = NULL;
	m_is_running = true;
}


hnZfsCombine::~hnZfsCombine(void)
{
	if (m_ptr_convert_translator)
	{
		DestroyIHdPJTranslator(m_ptr_convert_translator);
		m_ptr_convert_translator = NULL;
	}

	// 内存释放;
	if (m_ptr_coord_trans_system)
	{
		delete m_ptr_coord_trans_system;
		m_ptr_coord_trans_system = NULL;
	}
}

// 加载POS数据至内存
bool hnZfsCombine::loadPosData( const char* strPosPath,std::vector<POS_STRUCT_INFO>& vecInfo)
{
	// 检查文件是否存在
	if (_access(strPosPath,0) != 0)
	{
		return 0;
	}

	// 中间文件用于读取数据
	char strData[1024];
	memset(strData,0,1024);

	// 读取文件
	int file_line_count = 0;
	bool bFindData = false;
	FILE* ptrFile = fopen(strPosPath,"rt");
	while (!feof(ptrFile))
	{
		fgets(strData,1024,ptrFile);
		file_line_count++;
	}
	fclose(ptrFile);
	ptrFile = fopen(strPosPath,"rt");

	// 读取第一行数据
	fgets(strData,1024,ptrFile);
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
		memset(strData,0,1024);
		fgets(strData,1024,ptrFile);
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
	while (!feof(ptrFile) && m_is_running)
	{
		// 读取数据
		memset(strData,0,1024);
		fgets(strData,1024,ptrFile);

		// 解析数据
		POS_STRUCT_INFO info;
		nSize = info.serialize(strData);
		if ( nSize )
		{
			vecInfo[nCount] = info;
			nCount++;

			// 容器逐渐扩大
			if (nCount >= vecInfo.size())
			{
				vecInfo.resize(vecInfo.size() + nPerSize);
			}
		}

		if (loadCallback && nCount % 5000 == 0)
		{
			loadCallback(nCount * 1.0 / file_line_count,"读取POS数据...");
		}

		if (m_use_thread && nCount % 5000 == 0)
		{
			setProgress(nCount * 1.0 / file_line_count,"读取POS数据...");
		}
	}

	vecInfo.resize(nCount);
	fclose(ptrFile);

	// 将经纬度坐标转换投影坐标;
	convertBlhToNeh(vecInfo);

	if (loadCallback)
	{
		loadCallback(1.0,"POS加载完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"POS加载完成");
	}

	return true;
}

// 根据时间查询距离该点时间最近的记录值索引;
int hnZfsCombine::findIndexByGpsTime( double gpsTime,vector<POS_STRUCT_INFO>& vecInfo )
{
	bool bFind = false;
	int curIndex = 0;
	int lowIndex = 0;
	int highIndex = vecInfo.size()-1;
	while (highIndex >= lowIndex)
	{
		int m = (highIndex + lowIndex)/2;
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

// 通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;
bool hnZfsCombine::linearInsertPos(double gpsTime,vector<POS_STRUCT_INFO>& vecInfo,POS_STRUCT_INFO& insertResult,int& nearestIndex)
{
	// 条件判断，获取时间上距离最近点，返回为较低点
	//int nearestIndex = -1;
	//if (m_nPreIndex < 0)
	{
		nearestIndex = findIndexByGpsTime(gpsTime,vecInfo);
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

	if (nearestIndex < 0 || nearestIndex >= vecInfo.size()-1)
	{
		return false;
	}

	// 获取点值
	POS_STRUCT_INFO& curInfo = vecInfo[nearestIndex];
	POS_STRUCT_INFO& nextInfo = vecInfo[nearestIndex+1];

	// 进行插值比例值计算
	double fscale = (gpsTime - curInfo.dGpsSecond)/(nextInfo.dGpsSecond - curInfo.dGpsSecond);
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

using namespace Eigen;
void hnZfsCombine::calcuCoord( POS_STRUCT_INFO& posInfo,double& dx,double& dy,double& dz )
{
	// pos to world
	double arry_pos_temp[16] = {0};

	// 度转弧度
	double tmpHeading = posInfo.dHeading * PI64 / 180.0;
	double tmpPitch = posInfo.dPitch * PI64 / 180.0;
	double tmpRoll = posInfo.dRoll * PI64 / 180.0;
	computeMatrixByIScanAngle(posInfo.dEastCoord,posInfo.dNorthCoord,posInfo.dHeight,
		tmpHeading,tmpPitch,tmpRoll,arry_pos_temp);

	// 矩阵信息赋值
	MatrixXd mat_pos_to_world(4,4);
	mat_pos_to_world(0,0) = arry_pos_temp[0];  mat_pos_to_world(0,1) = arry_pos_temp[1]; mat_pos_to_world(0,2) = arry_pos_temp[2]; mat_pos_to_world(0,3) = arry_pos_temp[3];
	mat_pos_to_world(1,0) = arry_pos_temp[4];  mat_pos_to_world(1,1) = arry_pos_temp[5]; mat_pos_to_world(1,2) = arry_pos_temp[6]; mat_pos_to_world(1,3) = arry_pos_temp[7];
	mat_pos_to_world(2,0) = arry_pos_temp[8];  mat_pos_to_world(2,1) = arry_pos_temp[9]; mat_pos_to_world(2,2) = arry_pos_temp[10]; mat_pos_to_world(2,3) = arry_pos_temp[11];
	mat_pos_to_world(3,0) = arry_pos_temp[12];  mat_pos_to_world(3,1) = arry_pos_temp[13]; mat_pos_to_world(3,2) = arry_pos_temp[14]; mat_pos_to_world(3,3) = arry_pos_temp[15];

	// 点坐标构建矩阵
	
	MatrixXd mat_point_pos(4,1);
	mat_point_pos(0,0) = dx;
	mat_point_pos(1,0) = dy;
	mat_point_pos(2,0) = dz;
	mat_point_pos(3,0) = 1.0;

	// 结果值记录
	MatrixXd mat_point_result(4,1);
	mat_point_result = mat_pos_to_world * m_mat_laser_to_pos * mat_point_pos;
	dx = mat_point_result(0,0);
	dy = mat_point_result(1,0);
	dz = mat_point_result(2,0);

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
// 按iscan旋转角度方式Z-X-Y构建旋转矩阵;
void hnZfsCombine::computeMatrixByIScanAngle(double X,double Y,double Z, double Yaw,double Pitch,double Roll,double *R )
{
	double a1,a2,a3,b1,b2,b3,c1,c2,c3;

	a1 = cos(Roll)*cos(Yaw)+sin(Roll)*sin(Yaw)*sin(Pitch);
	a2 = -cos(Roll)*sin(Yaw)+sin(Roll)*cos(Yaw)*sin(Pitch);
	a3 = -sin(Roll)*cos(Pitch);
	b1 = sin(Yaw)*cos(Pitch);
	b2 = cos(Yaw)*cos(Pitch);
	b3 = sin(Pitch);
	c1 = sin(Roll)*cos(Yaw)-cos(Roll)*sin(Yaw)*sin(Pitch);
	c2 = -sin(Roll)*sin(Yaw)-cos(Roll)*cos(Yaw)*sin(Pitch);
	c3 = cos(Roll)*cos(Pitch);

	double fScale = 1.0;
	R[0] = a1 * fScale;  R[1] = b1 * fScale;  R[2] = c1 * fScale;  R[3] = X;
	R[4] = a2 * fScale;  R[5] = b2 * fScale;  R[6] = c2 * fScale;  R[7] = Y;
	R[8] = a3 * fScale;  R[9] = b3 * fScale;  R[10] = c3 * fScale; R[11] = Z;
	R[12] = 0.0;		 R[13] = 0.0;         R[14] = 0.0;         R[15] = 1.0;
	

	//R[0] = a1 * fScale;
	//R[1] = b1 * fScale; 
	//R[2] = c1 * fScale;
	//R[3] = a2 * fScale; 
	//R[4] = b2 * fScale; 
	//R[5] = c2 * fScale;
	//R[6] = a3 * fScale; 
	//R[7] = b3 * fScale; 
	//R[8] = c3 * fScale;
}

bool hnZfsCombine::dataCombine( const char* strZfsPath,const char* strPosPath,const char* strHlsPath ,const char* strTimePath)
{
	//COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//for (int n = 0;n < 1000000;n++)
	//{
	//	double longtitude = 96.139304750398;
	//	double latitude = 40.598647639025;
	//	double east = 0.0;
	//	double north = 0.0;

	//	// 未设置椭球体，目前仅WGS84椭球体采用该计算;
	//	hdGeographicCoordinateSystem* geographic_system = new hdGeographicCoordinateSystem;
	//	hdProjectedCoordinateSystem* projected_coord_system = new hdProjectedCoordinateSystem;
	//	geographic_system->m_datum_type = WGS_1984;

	//	//projected_coord_system->setGeogCoordSystem(*geographic_system);
	//	projected_coord_system->setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//	projected_coord_system->setFalseEasting(combine_coord_project_info.east_offset);
	//	projected_coord_system->setFalseNorthing(0.0);
	//	projected_coord_system->m_mean_latitude = combine_coord_project_info.average_latitude;
	//	projected_coord_system->m_proj_height = combine_coord_project_info.project_height;

	//	projected_coord_system->GaussProject_BL2NE(latitude,longtitude,north,east);

	//	delete geographic_system;
	//	delete projected_coord_system;
	//}
	//return true;

	// 获取导出配置:
	m_is_running = true;
	ENUM_COMBINE_EXPORT_PCD_TYPE export_pcd_type = m_project_setting->getExportFilesInfo().export_pcd_type;
	if (export_pcd_type == E_COMBINE_EXPORT_PCD_HLS)
	{
		return dataCombineHls(strZfsPath,strPosPath,strHlsPath,strTimePath);
	}
	else if (export_pcd_type == E_COMBINE_EXPORT_PCD_HLZ)
	{
		return dataCombineHlz(strZfsPath,strPosPath,strHlsPath,strTimePath);
	}
	return true;


	// 检查文件是否存在
	bool bSuc = false;
	if (_access(strZfsPath,0) != 0 )
	{
		return false;
	}

	if (_access(strPosPath,0) != 0)
	{
		return false;
	}

	// 加载POS
	std::vector<POS_STRUCT_INFO> vecPosInfo;
	bSuc = loadPosData(strPosPath,vecPosInfo);
	if (!bSuc)
	{
		return false;
	}

	if (loadCallback)
	{
		loadCallback(0.0,"打开ZFS文件");
	}

	if (m_use_thread)
	{
		setProgress(0.0,"打开ZFS文件");
	}

	// 打开zfs文件
	hnZfsReader zfs_reader;
	bSuc = zfs_reader.OpenZfsFile(strZfsPath);
	if (!bSuc)
	{
		return false;
	}

	if (loadCallback)
	{
		loadCallback(1.0,"打开ZFS文件完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"打开ZFS文件完成");
	}

	// 设置外部传入参数
	zfs_reader.setScanTime(m_scan_date_info);
	zfs_reader.setUtcToGpsAbs(m_utc_to_gps_second);

	// HLS头文件信息
	hdWriterHLS hls_writer;
	hdHeaderHLS hls_header;
	hls_header.setPointFormat(HLS2_POINTFORMAT_XYZIRGBP);
	hls_header.m_project_ID_GUID_data_1 = 1;
	hls_header.m_project_ID_GUID_data_2 = 2;
	hls_header.m_project_ID_GUID_data_3 = 3;
	hls_header.m_file_source_id = 0;
	hls_header.m_version_major = 2;	// Hls2.0
	hls_header.m_version_minor = 0;
	strcpy(hls_header.m_system_identifier,"HD 3LS ZFS");
	strcpy(hls_header.m_generating_software,"HD-3LS-SCAN");

	// 判断是否需要导出时间文件及LIN文件;
	bool need_lin = m_project_setting->getExportFilesInfo().need_export_lin_file;
	bool need_time = m_project_setting->getExportFilesInfo().need_export_time_file;

	// lin文件
	if (need_lin)
	{
		std::string str_lin_path = strHlsPath;
		int npos = str_lin_path.find_last_of('/');
		if (npos < 0)
		{
			npos = str_lin_path.find_last_of('\\');
		}
		str_lin_path = str_lin_path.substr(0,npos+1);

		std::string std_pcd_name = strHlsPath;
		std_pcd_name = std_pcd_name.substr(npos+1);
		int nlpos = std_pcd_name.find_last_of('.');
		std_pcd_name = std_pcd_name.substr(0,nlpos+1);

		int posname_p = std_pcd_name.find_last_of('P');
		std::string str_lin_name = std_pcd_name.replace(posname_p,3,"Pos");

		str_lin_path += str_lin_name;
		str_lin_path += "lin";

		// 获取扫描时间信息;
		double scan_start_time,scan_end_time;
		scan_start_time = scan_end_time = 0.0;
		zfs_reader.getScanTimeRange(scan_start_time,scan_end_time);

		exportLinFile(scan_start_time,scan_end_time,str_lin_path.data(),vecPosInfo);
	}


	// 记录更新时间
	time_t timer;
	time(&timer);
	tm* t_tm = localtime(&timer);
	hls_header.m_file_creation_day = t_tm->tm_yday + 1;
	hls_header.m_file_creation_year = t_tm->tm_year + 1900;

	// 记录头文件偏移量
	double gps_time_offset = 0.0;
	bool bFirst = true;
	double header_offset_x = 0.0;
	double header_offset_y = 0.0;
	double header_offset_z = 0.0;

	// 一次申请内存
	hdPointXYZIPRGBA* ptrLoopData = new hdPointXYZIPRGBA[50000];
	memset(ptrLoopData,0,sizeof(hdPointXYZIPRGBA) * 50000);

	// 时间存储信息文件内存申请
	COMBINE_TIME_POINT_STRUCT* ptrTimeLoopData = NULL;
	ptrTimeLoopData = new COMBINE_TIME_POINT_STRUCT[50000];
	memset(ptrTimeLoopData,0,sizeof(COMBINE_TIME_POINT_STRUCT) * 50000);

	// 逐圈读取数据
	int nLoopCount = zfs_reader.GetScanLines();

	// 若使用控制点即进行POS纠偏处理，则考虑只需要提取POS对应时间点一定范围内数据即可;
	COMBINE_TIME_DELAY_SET_STRUCT time_delay_info = m_project_setting->getTimeDelayInfo();
	COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT use_ctrl_param_set = m_project_setting->getUseCtrlRadiusExportInfo();
	if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
	{
		// 获取距离最近POS时间点
		std::vector<COMBINE_TIME_RANGE> vec_combine_time_range;
		calcuCtrlPosTime(m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius,vecPosInfo,vec_combine_time_range);

		zfs_reader.setTimeRange(vec_combine_time_range);
	}
	else
	{
		// 若使用时间进行过滤;
		COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
		if (use_time_export_set.is_use_time_range_export)
		{
			m_vec_combine_time_range.clear();
			for (unsigned int n = 0;n < use_time_export_set.vec_combine_time_range.size();n++)
			{
				COMBINE_TIME_RANGE info = use_time_export_set.vec_combine_time_range[n];
				if (info.start_gps_second >= info.end_gps_second)
				{
					continue;
				}

				m_vec_combine_time_range.push_back(info);
			}

			//m_vec_combine_time_range = use_time_export_set.vec_combine_time_range;
		}

		zfs_reader.setTimeRange(m_vec_combine_time_range);
	}

	// 头文件信息设置;
	int nPointCount = 0;
	hls_header.m_number_of_col = nLoopCount;

	// 打开写出文件失败返回;
	if (!hls_writer.open(strHlsPath,&hls_header))
	{
		return false;
	}

	// 打开写入时间信息文件;
	hnCombineTimeWriter timeWriter;
	if (need_time)
	{
		bSuc = timeWriter.Open(strTimePath);
		if (!bSuc)
		{
			return false;
		}
	}

	// 相关参数定义;
	int have_read_pts_in_loop = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> loop_points;
	loop_points.resize(5000);
	int have_write_loop_count = 0;
	zfs_reader.startRead();
	for (unsigned int nn = 0;nn < nLoopCount;nn++)
	{
		int n = nn;

		// 逐圈获取数据
		have_read_pts_in_loop = 0;
		bSuc = zfs_reader.getLinePoints(n,loop_points,have_read_pts_in_loop);
		if (!bSuc || have_read_pts_in_loop == 0)
		{
			if ( loadCallback && nn % 500 == 0 )
			{
				loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
			}
			if (m_use_thread && nn % 500 == 0 )
			{
				setProgress(nn * 1.0 / nLoopCount,"融合解算...");
			}

			continue;
		}

		memset(ptrLoopData,0,sizeof(hdPointXYZIPRGBA) * 50000);
		memset(ptrTimeLoopData,0,sizeof(COMBINE_TIME_POINT_STRUCT) * 50000);


		// 每个点进行坐标转换
		int loopWriteCount = 0;
		for (unsigned int m = 0;m < have_read_pts_in_loop;m++)
		{
			POINT_STRUCT_XYZIT_INFO& point_info = loop_points[m];
			point_info.timeSecond += time_delay_info.pcd_time_delay * 0.001;

			// 首先根据时间信息进行插值计算，获取该时刻POS位置姿态;
			POS_STRUCT_INFO curPos;
			int nearest_pos_index = -1;
			bSuc = linearInsertPos(point_info.timeSecond,vecPosInfo,curPos,nearest_pos_index);
			if (!bSuc)
			{
				continue;
			}

			// 矩阵运算，获取绝对坐标;
			double dx,dy,dz;
			dx = point_info.x;
			dy = point_info.y;
			dz = point_info.z;
			calcuCoord(curPos,dx,dy,dz);

			// 若使用控制点范围过滤;
			if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
			{
				// 判断点是否在控制点范围内;
				bool is_point_in = isPointInCtrlRadius(dx,dy,dz,m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius);
				if (!is_point_in)
				{
					continue;
				}
			}

			// 以pos第一个点作为系统偏移量计算，防止坐标过大，精度丢失;
			if (bFirst)
			{
				gps_time_offset = point_info.timeSecond;
				header_offset_x = dx;
				header_offset_y = dy;
				header_offset_z = dz;
				bFirst = false;
			}

			// 跳过0点得了
			float tmpX,tmpY,tmpZ;
			tmpX = dx - header_offset_x;
			tmpY = dy - header_offset_y;
			tmpZ = dz - header_offset_z;
			if ((tmpX == 0.0f && tmpY == 0.0f && tmpZ == 0.0f))
			{
				continue;
			}

			// 记录时间信息文件;
			if (need_time)
			{
				ptrTimeLoopData[loopWriteCount].x = point_info.x;
				ptrTimeLoopData[loopWriteCount].y = point_info.y;
				ptrTimeLoopData[loopWriteCount].z = point_info.z;
				ptrTimeLoopData[loopWriteCount].time = point_info.timeSecond - gps_time_offset;
			}

			// 记录坐标，此处计算即为相对坐标;
			ptrLoopData[loopWriteCount].x = dx - header_offset_x;
			ptrLoopData[loopWriteCount].y = dy - header_offset_y;
			ptrLoopData[loopWriteCount].z = dz - header_offset_z;
			ptrLoopData[loopWriteCount].intensity = point_info.intensity;
			loopWriteCount++;
		}

		nPointCount += loopWriteCount;
		have_write_loop_count++;

		if ( loadCallback && nn % 500 == 0 )
		{
			loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
		}

		if (m_use_thread && nn % 500 == 0)
		{
			setProgress(nn * 1.0 / nLoopCount,"融合解算...");
		}

		// 一圈点云写入
		if (loopWriteCount > 0)
		{
			hls_writer.writeLoop(ptrLoopData,loopWriteCount);
			if (need_time)
			{
				timeWriter.WriteLoop(ptrTimeLoopData,loopWriteCount);
			}
			
		}
	}

	// 头文件信息记录
	hls_writer.m_hls_header.m_number_of_col = have_write_loop_count;
	hls_writer.m_hls_header.m_offset_x = header_offset_x;
	hls_writer.m_hls_header.m_offset_y = header_offset_y;
	hls_writer.m_hls_header.m_offset_z = header_offset_z;
	hls_writer.m_hls_header.m_number_of_point_records = nPointCount;

	hls_writer.close();
	zfs_reader.CloseZfsFile();

	// 写入头文件更新
	if (need_time)
	{
		timeWriter.m_header.time_offset = gps_time_offset;
		timeWriter.m_header.loopCount = have_write_loop_count;
		timeWriter.m_header.pointRecordCount = nPointCount;
		timeWriter.Close();
	}

	if (loadCallback)
	{
		loadCallback(1.0,"处理完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"处理完成");
	}

	// 内存释放
	if (ptrLoopData)
	{
		delete []ptrLoopData;
		ptrLoopData = NULL;
	}

	// 内存释放
	if (ptrTimeLoopData)
	{
		delete[] ptrTimeLoopData;
		ptrTimeLoopData = NULL;
	}
	return true;
}

void hnZfsCombine::setScanDate( int year,int month,int day )
{
	m_scan_date_info.year = year;
	m_scan_date_info.month = month;
	m_scan_date_info.day = day;
}

// 设置扫描的年月日信息;
void hnZfsCombine::setScanPara( double laser_to_pos_x,double laser_to_pos_y,double laser_to_pos_z, 
	double laser_to_pos_heading,double laser_to_pos_pitch,double laser_to_pos_roll )
{
	m_laser_topos_x = laser_to_pos_x;
	m_laser_topos_y = laser_to_pos_y;
	m_laser_topos_z = laser_to_pos_z;
	m_laser_topos_heading = laser_to_pos_heading;
	m_laser_topos_pitch = laser_to_pos_pitch;
	m_laser_topos_roll = laser_to_pos_roll;

	char str[1024];
	sprintf(str,"%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf\n",m_laser_topos_x,m_laser_topos_y,m_laser_topos_z,m_laser_topos_heading,m_laser_topos_pitch,m_laser_topos_roll);
	//qDebug() << QString::fromLocal8Bit(str)<<"\n";
	//print(str);
	qErrnoWarning("%s",str);

	// 激光到pos构建的矩阵;
	double arry_pos_temp[16];

	// 度转弧度
	double tmpHeading = m_laser_topos_heading * PI64 / 180.0;
	double tmpPitch = m_laser_topos_pitch * PI64 / 180.0;
	double tmpRoll = m_laser_topos_roll * PI64 / 180.0;
	computeMatrixByIScanAngle(m_laser_topos_x,m_laser_topos_y,m_laser_topos_z,
		tmpHeading,tmpPitch,tmpRoll,arry_pos_temp);
	m_mat_laser_to_pos(0,0) = arry_pos_temp[0];  m_mat_laser_to_pos(0,1) = arry_pos_temp[1]; m_mat_laser_to_pos(0,2) = arry_pos_temp[2]; m_mat_laser_to_pos(0,3) = arry_pos_temp[3];
	m_mat_laser_to_pos(1,0) = arry_pos_temp[4];  m_mat_laser_to_pos(1,1) = arry_pos_temp[5]; m_mat_laser_to_pos(1,2) = arry_pos_temp[6]; m_mat_laser_to_pos(1,3) = arry_pos_temp[7];
	m_mat_laser_to_pos(2,0) = arry_pos_temp[8];  m_mat_laser_to_pos(2,1) = arry_pos_temp[9]; m_mat_laser_to_pos(2,2) = arry_pos_temp[10]; m_mat_laser_to_pos(2,3) = arry_pos_temp[11];
	m_mat_laser_to_pos(3,0) = arry_pos_temp[12];  m_mat_laser_to_pos(3,1) = arry_pos_temp[13]; m_mat_laser_to_pos(3,2) = arry_pos_temp[14]; m_mat_laser_to_pos(3,3) = arry_pos_temp[15];
}

bool hnZfsCombine::setCtrlPointPath( const char* str_ctrl_point_path )
{
	// 检查文件存在;
	if (_access(str_ctrl_point_path,0) != 0)
	{
		return false;
	}

	// 读取加载控制点文件;
	char str_read_line[1024];
	
	// 打开文件读取
	FILE* ptr_file = fopen(str_ctrl_point_path,"rt");
	if (!ptr_file)
	{
		return false;
	}

	// 循环读取
	bool file_has_data = false;
	m_vec_ctrl_points.clear();
	while (!feof(ptr_file))
	{
		// 读取一行数据
		memset(str_read_line,0,1024);
		fgets(str_read_line,1024,ptr_file);

		COMBINE_CTRL_POINT_STRUCT_INFO ctrl_point_info;
		bool read_succ = ctrl_point_info.serialize(str_read_line);
		if (read_succ)
		{
			file_has_data = true;
			m_vec_ctrl_points.push_back(ctrl_point_info);
		}
	}

	fclose(ptr_file);
	m_use_ctrl_pt = file_has_data;
	return file_has_data;
}

void hnZfsCombine::calcuCtrlPosTime( std::vector<COMBINE_CTRL_POINT_STRUCT_INFO> vec_ctrl_points, double dist_tol,
	std::vector<POS_STRUCT_INFO>& vec_pos_info,std::vector<COMBINE_TIME_RANGE>& result_time_info )
{
	// 条件判断
	if (vec_ctrl_points.size() < 0 || vec_pos_info.size() < 0)
	{
		return;
	}

	result_time_info.clear();
	//std::vector<POS_STRUCT_INFO> vec_cal_pos_info;
	//for (unsigned int n = 0;n < vec_pos_info.size();n++)
	//{
	//	POS_STRUCT_INFO info = vec_pos_info[n];
	//	vec_cal_pos_info.push_back(info);
	//}

	//// 存在坐标转换;
	//COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//COMBINE_COORD_CONVERT_SET_STRUCT convert_info = m_project_setting->getCoordConvertInfo();
	//int use_convert = convert_info.use_coord_convert;
	//if (m_ptr_convert_translator && use_convert )
	//{
	//	double dx,dy,dz;
	//	for (unsigned int n = 0;n < vec_cal_pos_info.size();n++)
	//	{
	//		dx = dy = dz = 0.0;
	//		POS_STRUCT_INFO& info = vec_cal_pos_info[n];

	//		double temp_x = info.dNorthCoord;
	//		double temp_y = info.dEastCoord;
	//		double temp_z = info.dHeight;
	//		int convert_model = convert_info.use_coord_convert_model;
	//		if (convert_model == 0)
	//		{
	//			// 四参数加高程拟合转换，输入北东高坐标系，返回东北高坐标系;
	//			m_ptr_convert_translator->TranslatorByParam(temp_x,temp_y,temp_z,&dx,&dy,&dz);

	//			// 高程拟合计算;
	//			double ddH = 0.0;
	//			m_ptr_convert_translator->HFixCalus(convert_info.height_fit_n0,convert_info.height_fit_e0,&ddH);
	//			dz += ddH;
	//		}
	//		else if (convert_model == 1)
	//		{
	//			temp_x = info.dLongitude;
	//			temp_y = info.dLatitude;
	//			temp_z = info.dHeight;

	//			// 未设置椭球体，目前仅WGS84椭球体采用该计算;
	//			hdProjectedCoordinateSystem projected_coord_system;
	//			//projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
	//			projected_coord_system.setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//			projected_coord_system.setFalseEasting(combine_coord_project_info.east_offset);
	//			projected_coord_system.setFalseNorthing(0.0);
	//			projected_coord_system.m_mean_latitude = combine_coord_project_info.average_latitude;
	//			projected_coord_system.m_proj_height = combine_coord_project_info.project_height;

	//			// 直接进行七参数转换，获得经纬度;
	//			m_ptr_convert_translator->TranslatorByParam(temp_x,temp_y,temp_z,&dx,&dy,&dz);

	//			temp_x = dy * 180.0 / PI64;
	//			temp_y = dx * 180.0 / PI64;

	//			// 经纬度转换为投影坐标;
	//			projected_coord_system.GaussProject_BL2NE(temp_y,temp_x);
	//			dx = temp_x;
	//			dy = temp_y;
	//		}

	//		info.dEastCoord = dx;
	//		info.dNorthCoord = dy;
	//		info.dHeight = dz;
	//	}
	//}

	// 遍历处理计算

	COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
	for (unsigned int n = 0;n < vec_ctrl_points.size();n++)
	{
		int pos_index = -1;
		double cal_pos_time = 0.0;
		double dist = F32_MAX;
		COMBINE_CTRL_POINT_STRUCT_INFO& ctrl_point_info = vec_ctrl_points[n];
		for (unsigned int m = 0;m < vec_pos_info.size();m++)
		{
			POS_STRUCT_INFO& pos_info = vec_pos_info[m];
			bool is_time_in = false;
			if (use_time_export_set.is_use_time_range_export)
			{
				// 不在时间范围内的则过滤;
				is_time_in = getCtrlTimeInTime(pos_info.dGpsSecond,use_time_export_set.vec_combine_time_range);
				if (!is_time_in)
				{
					continue;
				}
			}

			double temp_dist = pow(ctrl_point_info.ctrl_coord_x - pos_info.dEastCoord,2.0) + 
				pow(ctrl_point_info.ctrl_coord_y - pos_info.dNorthCoord,2.0);
			if (temp_dist < dist)
			{
				// 更新记录时间信息
				pos_index = m;
				dist = temp_dist;
				cal_pos_time = pos_info.dGpsSecond;
			}
		}

		// 判断距离，确保距离值正常，暂定为十米内认为正常;
		if (pos_index >= 0 && dist < 100.0 * 100.0)
		{
			// 根据给定距离容差范围确定时间段;
			double temp_tol = 0.0;
			COMBINE_TIME_RANGE time_range;
			POS_STRUCT_INFO& cur_pos = vec_pos_info[pos_index];
			time_range.start_gps_second = cur_pos.dGpsSecond;
			time_range.end_gps_second = cur_pos.dGpsSecond;

			if (pos_index == 0)
			{
				// 若当前索引为第一个点，则必定存在较大时间静止，可过滤掉;
				for (int i = pos_index;i < vec_pos_info.size();i++)
				{
					POS_STRUCT_INFO& pos_info = vec_pos_info[i];
					double temp_dist = pow(cur_pos.dEastCoord - pos_info.dEastCoord,2.0) + 
						pow(cur_pos.dNorthCoord - pos_info.dNorthCoord,2.0);

					// 当前点与前一个点距无差别，视为静止采集，起始时间前推;
					if (abs(temp_tol - temp_dist) > 0.0001)
					{
						time_range.start_gps_second = pos_info.dGpsSecond;
						break;
					}
					else
					{
						// 记录前一个点
						temp_tol = temp_dist;
					}
				}
			}
			else
			{
				for (int i = pos_index;i >= 0;i--)
				{
					POS_STRUCT_INFO& pos_info = vec_pos_info[i];
					double temp_dist = pow(cur_pos.dEastCoord - pos_info.dEastCoord,2.0) + 
						pow(cur_pos.dNorthCoord - pos_info.dNorthCoord,2.0);
					if (temp_dist > dist_tol * dist_tol)
					{
						break;
					}
					else
					{
						time_range.start_gps_second = pos_info.dGpsSecond;
					}
				}
			}


			// 向下找终止点
			temp_tol = 0.0;
			if (pos_index == (vec_pos_info.size()-1))
			{
				for (int i = pos_index;i >= 0;i--)
				{
					POS_STRUCT_INFO& pos_info = vec_pos_info[i];
					double temp_dist = pow(cur_pos.dEastCoord - pos_info.dEastCoord,2.0) + 
						pow(cur_pos.dNorthCoord - pos_info.dNorthCoord,2.0);

					if (abs(temp_tol - temp_dist) > 0.0001)
					{
						time_range.end_gps_second = pos_info.dGpsSecond;
						break;
					}
					else
					{
						// 记录前一个点
						temp_tol = temp_dist;
					}
				}
			}

			for (int i = pos_index;i < vec_pos_info.size();i++)
			{
				POS_STRUCT_INFO& pos_info = vec_pos_info[i];
				double temp_dist = pow(cur_pos.dEastCoord - pos_info.dEastCoord,2.0) + 
					pow(cur_pos.dNorthCoord - pos_info.dNorthCoord,2.0);
				if (temp_dist > dist_tol * dist_tol)
				{
					break;
				}
				else
				{
					time_range.end_gps_second = pos_info.dGpsSecond;
				}
			}

			// 时间记录
			result_time_info.push_back(time_range);
		}
	}
}

// 将vecInfo中的经纬度坐标转换投影坐标;
void hnZfsCombine::convertBlhToNeh( std::vector<POS_STRUCT_INFO>& vecInfo )
{
	// 条件判断
	if (vecInfo.size() <= 0 || !m_ptr_coord_trans_system)
	{
		return;
	}

	//// 坐标转换:
	//hdSpatialReferenceH source_cs = m_ptr_coord_trans_system->getSourceCS();
	//if (!source_cs)
	//{
	//	return;
	//}
	//hdProjectedCoordinateSystem* ptr_source_coord = static_cast<hdProjectedCoordinateSystem*>(source_cs);
	//if (!ptr_source_coord)
	//{
	//	return;
	//}

	// 数据转换对象;
	IHdPJTranslator* ptr_pj_translator = NULL;	
	CreateIHdPJTranslator(&ptr_pj_translator);

	//Spatial_Ref_t dst_param = m_dst_param;
	//dst_param.Lo = 96.0 * PI64 / 180.0;
	//dst_param.PH = 0.0;
	ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	//// 遍历处理；
	//COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//COMBINE_COORD_CONVERT_SET_STRUCT convert_set_info = m_project_setting->getCoordConvertInfo();
	//if (convert_set_info.use_coord_convert)
	//{
	//	for (unsigned int n = 0;n < vecInfo.size();n++)
	//	{
	//		// 未运行则跳出;
	//		if (!m_is_running)
	//		{
	//			break;
	//		}

	//		POS_STRUCT_INFO& pos_info = vecInfo[n];

	//		// 经纬度转投影坐标
	//		double tmp_north = pos_info.dLatitude;
	//		double tmp_east = pos_info.dLongitude;
	//		double tmp_height = pos_info.dHeight;

	//		//// 直接进行七参数转换;
	//		//double temp_x = tmp_east;
	//		//double temp_y = tmp_north;
	//		//double temp_z = pos_info.dHeight;
	//		//double dx,dy,dz;
	//		//dx = dy = dz = 0.0;

	//		//ptr_pj_translator->TranslatorByParam(temp_x,temp_y,temp_z,&dx,&dy,&dz);


	//		//tmp_east = dy * 180.0 / PI64;
	//		//tmp_north = dx * 180.0 / PI64;
	//		//pos_info.dHeight = dz;

	//		//// 未设置椭球体，目前仅WGS84椭球体采用该计算;
	//		//hdProjectedCoordinateSystem projected_coord_system;
	//		////projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
	//		//projected_coord_system.setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//		//projected_coord_system.setFalseEasting(combine_coord_project_info.east_offset);
	//		//projected_coord_system.setFalseNorthing(0.0);
	//		//projected_coord_system.m_mean_latitude = combine_coord_project_info.average_latitude;
	//		//projected_coord_system.m_proj_height = combine_coord_project_info.project_height;

	//		//projected_coord_system.GaussProject_BL2NE(tmp_north,tmp_east);
	//		//pos_info.dNorthCoord = tmp_north;
	//		//pos_info.dEastCoord = tmp_east;
	//		double temp_north = 0.0;
	//		double temp_east = 0.0;
	//		double temp_height = 0.0;

	//		// 构建设置坐标转换参数;

	//		//if (convert_set_info.use_coord_convert)
	//		{
	//			m_ptr_convert_translator->Translator(pos_info.dLongitude,pos_info.dLatitude,pos_info.dHeight,&temp_east,&temp_north,&temp_height);
	//		}
	//		//else
	//		//{
	//		//	ptr_pj_translator->Translator(pos_info.dLongitude,pos_info.dLatitude,pos_info.dHeight,&temp_east,&temp_north,&temp_height);
	//		//}

	//		pos_info.dNorthCoord = temp_north;
	//		pos_info.dEastCoord = temp_east;
	//		pos_info.dHeight = temp_height;

	//		if (loadCallback && n % 5000 == 0)
	//		{
	//			loadCallback(n * 1.0 / vecInfo.size(),"POS投影坐标转换...");
	//		}

	//		if (m_use_thread && n % 5000 == 0)
	//		{
	//			setProgress(n * 1.0 / vecInfo.size(),"POS投影坐标转换...");
	//		}
	//	}
	//}
	//else
	{
		int ncount = vecInfo.size();
		double* degree_lat = new double[ncount];
		double* degree_long = new double[ncount];
		double* east = new double[ncount];
		double* north = new double[ncount];

		for (int i = 0;i < vecInfo.size();i++)
		{
			POS_STRUCT_INFO& pos_info = vecInfo[i];

			// 经纬度转投影坐标;
			degree_lat[i] = pos_info.dLatitude;
			degree_long[i] = pos_info.dLongitude;

			if (loadCallback && i % 5000 == 0)
			{
				loadCallback(i * 1.0 / vecInfo.size(),"POS转换...");
			}

			if (m_use_thread && i % 5000 == 0)
			{
				setProgress(i * 1.0 / vecInfo.size(),"POS转换...");
			}
		}

		
		ptr_pj_translator->TransLators_BLToNE(ncount,degree_lat,degree_long,north,east);

		for (int i = 0;i < vecInfo.size();i++)
		{
			POS_STRUCT_INFO& pos_info = vecInfo[i];

			// 经纬度转投影坐标;
			pos_info.dNorthCoord = north[i];
			pos_info.dEastCoord = east[i];

			if (loadCallback && i % 5000 == 0)
			{
				loadCallback(i * 1.0 / vecInfo.size(),"POS记录...");
			}

			if (m_use_thread && i % 5000 == 0)
			{
				setProgress(i * 1.0 / vecInfo.size(),"POS记录...");
			}
		}

		//// 数据转换对象;
		//IHdPJTranslator* ptr_pj_translator1 = NULL;	
		//CreateIHdPJTranslator(&ptr_pj_translator1);

		//ptr_pj_translator1->SetSrcSpatialRef(&m_src_param);
		//ptr_pj_translator1->SetDstSpatialRef(&m_dst_param);

		////for (int i = 0;i < ncount;i++)
		////{
		////	north[i] += 0.0032;
		////}

		//double* east1 = new double[ncount];
		//double* north1 = new double[ncount];
		//ptr_pj_translator1->TransLators_NEToBL(ncount,north,east,degree_lat,degree_long);


		//IHdPJTranslator* ptr_pj_translator2 = NULL;	
		//CreateIHdPJTranslator(&ptr_pj_translator2);

		//ptr_pj_translator2->SetSrcSpatialRef(&m_src_param);
		//ptr_pj_translator2->SetDstSpatialRef(&m_dst_param);
		//FILE* ptr_test = fopen("D:\\testcal2.csv","wt");
		//ptr_pj_translator2->TransLators_BLToNE(ncount,degree_lat,degree_long,north1,east1);
		//for (int i = 0;i < ncount;i++)
		//{
		//	double tempn = north[i] - north1[i];
		//	double tempe = east[i] - east1[i];

		//	fprintf_s(ptr_test,"%.4lf,%.4lf\n",tempn,tempe);
		//}
		//fclose(ptr_test);


		delete[] degree_lat;
		degree_lat = NULL;
		delete[] degree_long;
		degree_long = NULL;

		delete[] north;
		north = NULL;
		delete[] east;
		east = NULL;
	}

	DestroyIHdPJTranslator(ptr_pj_translator);
	return;


	//// 数据转换对象;
	//IHdPJTranslator* ptr_pj_translator = NULL;	
	//CreateIHdPJTranslator(&ptr_pj_translator);

	//ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	//ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	////// 定义两个临时变量;
	////double dZ_IN = 0;
	////double dZ_OUT = 0;

	////// 中间变量定义；
	////double tmp_centre_l = 0.0;
	////int zone_num = 0;

	////// 根据第一个点计算带号;
	////zone_num = (vecInfo[0].dLongitude + 1.5)/3; 
	////tmp_centre_l = zone_num * 3.0;

	////// 设置投影坐标转换系统；
	////hdProjectedCoordinateSystem projected_coord_system("gauss_wgs84",115.75);

	//// 遍历处理；
	//for (unsigned int n = 0;n < vecInfo.size();n++)
	//{
	//	POS_STRUCT_INFO& pos_info = vecInfo[n];
	//	double tmp_height = 0.0;
	//	double tmp_north = 0.0;
	//	double tmp_east = 0.0;

	//	ptr_pj_translator->Translator(pos_info.dLongitude,pos_info.dLatitude,pos_info.dHeight,&pos_info.dEastCoord,&pos_info.dNorthCoord,&tmp_height);

	//	//// 经纬度转投影坐标
	//	//projected_coord_system.GaussProject_BL2NE(pos_info.dLatitude,pos_info.dLongitude,pos_info.dNorthCoord,pos_info.dEastCoord);

	//	//double aa = tmp_north - pos_info.dNorthCoord;
	//	//double bb = tmp_east - pos_info.dEastCoord;
	//	//double cc = tmp_east - pos_info.dHeight;
	//	//int dd = 0;
	//}

	//DestroyIHdPJTranslator(ptr_pj_translator);

	//return;
}

void hnZfsCombine::ctrlPointCal( const char* str_pos_path,const char* str_ctrl_file,const char* str_ctrl_result )
{
	// 条件判断;
	if (_access(str_pos_path,0) != 0 || _access(str_ctrl_file,0) != 0)
	{
		return;
	}

	std::vector<POS_STRUCT_INFO> vec_pos_info;
	loadPosData(str_pos_path,vec_pos_info);

	FILE* ptr_save_file = fopen(str_ctrl_result,"wt");

	float laser_x,laser_y,laser_z;
	laser_x = laser_y = laser_z = 0.0f;
	double gps_time = 0.0;
	double ctrl_global_x,ctrl_global_y,ctrl_global_z;
	ctrl_global_x = ctrl_global_y = ctrl_global_z = 0.0;

	double exist_ctrl_x,exist_ctrl_y,exist_ctrl_z;
	exist_ctrl_x = exist_ctrl_y = exist_ctrl_z = 0.0;

	char ctrl_name[128];
	FILE* ptr_file = fopen(str_ctrl_file,"rt");
	char str_line[1024];
	while (!feof(ptr_file))
	{
		int i = 0;
		ctrl_global_x = ctrl_global_y = ctrl_global_z = 0.0;
		memset(ctrl_name,0,128);
		memset(str_line,0,1024);
		fgets(str_line,1024,ptr_file);

		//FILE* ptr_file = fopen("D:\\select_ctrl_point.txt","at+");
		//fprintf_s(ptr_file,"%.6lf,%s,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.4f,%.4f,%.4f\n",m_cur_select_point.time.time,
		//	str_ctrl_name.data(),ctrl_x,ctrl_y,ctrl_z,
		//	m_cur_select_point.global_x,m_cur_select_point.global_y,m_cur_select_point.global_z,
		//	m_cur_select_point.time.x,m_cur_select_point.time.y,m_cur_select_point.time.z);

		//int ctrl_named = 0;
		//int scans = sscanf_s(str_line,"%lf,%d,%lf,%lf,%lf,%lf,%lf,%lf,%f,%f,%f\n",
		//	&gps_time,&ctrl_named,&exist_ctrl_x,&exist_ctrl_y,&exist_ctrl_z,
		//	&ctrl_global_x,&ctrl_global_y,&ctrl_global_z,
		//	&laser_x,&laser_y,&laser_z);

		int ctrl_named = 0;
		int scans = sscanf_s(str_line,"%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%f,%f,%f\n",
		&ctrl_named,&gps_time,&exist_ctrl_x,&exist_ctrl_y,&exist_ctrl_z,
			&ctrl_global_x,&ctrl_global_y,&ctrl_global_z,
			&laser_x,&laser_y,&laser_z);


		//int scans = sscanf_s(str_line,"%d,%lf,%lf,%lf,%lf,%f,%f,%f\n",
		//	&i,&ctrl_global_x,&ctrl_global_y,&ctrl_global_z,
		//	&gps_time,&laser_x,&laser_y,&laser_z);
		if (scans >= 11)
		{
			double tmpX,tmpY,tmpZ;
			tmpX = ctrl_global_x;
			tmpY = ctrl_global_y;
			tmpZ = ctrl_global_z;

			// 查找最近点线性插值;
			POS_STRUCT_INFO cur_pos_point;
			int nearest_pos_index = -1;
			bool bret = linearInsertPos(gps_time,vec_pos_info,cur_pos_point,nearest_pos_index);
			if (!bret)
			{
				int t =0;
			}

			fprintf_s(ptr_save_file,"%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.4f,%.4f,%.4f\n",
				exist_ctrl_x,exist_ctrl_y,exist_ctrl_z,ctrl_global_x,ctrl_global_y,ctrl_global_z,
				cur_pos_point.dEastCoord,cur_pos_point.dNorthCoord,cur_pos_point.dHeight,
				cur_pos_point.dHeading,cur_pos_point.dPitch,cur_pos_point.dRoll,
				laser_x,laser_y,laser_z);
		}
	}
	fclose(ptr_file);

	fclose(ptr_save_file);
}

int hnZfsCombine::convertHlsToHlz( const char* str_hls_path,const char* str_hlz_path )
{
	// 打开HLS点云;
	hdReaderHLS hls_reader;
	hd::HBOOL is_succ = hls_reader.open(str_hls_path);
	if (!is_succ)
	{
		return -1;
	}

	// 将多字节转换宽字节;
	int nlen = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,str_hlz_path,-1,NULL,0);
	if (nlen == 0)
	{
		return -1;
	}
	wchar_t* wc = new wchar_t[nlen];
	MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,str_hlz_path,-1,wc,nlen);

	// 打开HLZ流文件;
	//CallbackWrapper* ptr_progress = new CallbackWrapper(wloadCallback);
	hdHlzStreamBuilder hlz_stream_builder;
	bool is_suc = hlz_stream_builder.open(wc,false,false,NULL);
	if (!is_suc)
	{
		return -1;
	}
	hlz_stream_builder.setSimpleLevel0(false,0.0025);

	int per_size = 500000;
	double global_x,global_y,global_z;
	int per_write_count = 0;
	hdPointXYZIPRGBAD* ptr_write_data = new hdPointXYZIPRGBAD[per_size];

	// 获取总圈数;
	hdLoopIndex* ptr_loop_index = hls_reader.getLoopIndex(NULL);
	int file_loop_count = hls_reader.getLoopCount();
	for (int iloop = 0;iloop < file_loop_count;iloop++)
	{
		hdPtArray<hdPointXYZIPRGBA> arry_point;
		HBOOL read_suc = hls_reader.readLoopFull(arry_point,iloop);
		for (int ipt = 0;ipt < arry_point.size();ipt++)
		{
			hdPointXYZIPRGBA& point = arry_point[ipt];

			global_x = point.x;
			global_y = point.y;
			global_z = point.z;
			hls_reader.getGlobalCoordinate(global_x,global_y,global_z);
			

			ptr_write_data[per_write_count].x = global_x;
			ptr_write_data[per_write_count].y = global_y;
			ptr_write_data[per_write_count].z = global_z;
			ptr_write_data[per_write_count].intensity = point.intensity;
			ptr_write_data[per_write_count].r = point.r;
			ptr_write_data[per_write_count].g = point.g;
			ptr_write_data[per_write_count].b = point.b;
			ptr_write_data[per_write_count].prop = point.prop;
			per_write_count++;

			if (per_write_count >= per_size)
			{
				hlz_stream_builder.writePts(ptr_write_data,per_write_count);
				per_write_count = 0;
			}
		}

		// 设置读取进度条
		if (loadCallback && iloop % 10 == 0)
		{
			loadCallback(iloop * 1.0 / file_loop_count,"读取HLS文件...");
		}

		if (m_use_thread && iloop % 10 == 0)
		{
			setProgress(iloop * 1.0 / file_loop_count,"读取HLS文件...");
		}
	}

	// 最后剩余部分写入
	if (per_write_count > 0)
	{
		hlz_stream_builder.writePts(ptr_write_data,per_write_count);
		per_write_count = 0;
	}

	// 写入完成;
	hlz_stream_builder.flush();
	hlz_stream_builder.close();

	hls_reader.close();

	if (m_use_thread )
	{
		setProgress(1.0,"转换完成");
	}

	if (ptr_write_data)
	{
		delete[] ptr_write_data;
		ptr_write_data = NULL;
	}

	if (wc)
	{
		delete[] wc;
		wc = NULL;
	}
	return 1;
}

struct LIN_POS_INFO
{
	//0000001	519357.673	2018	07	07	00	15	57	672	583735.748	4575872.068	30.000	259.3483	-0.0577	0.0342
	LIN_POS_INFO()
	{
		memset(str,0,128);
		gpsTime = 0.0;
		year = month = hour = day = minute = second = millisecond = 0;
		eastCoord = northCoord = height = heading = pitch = roll = 0.0;
	}

	bool serialize(const char* strData)
	{
		int nSize = sscanf_s(strData,"%s	%lf	%d	%d	%d	%d	%d	%d	%d	%lf	%lf	%lf	%lf	%lf	%lf\n",
			str,128,&gpsTime,&year,&month,&day,&hour,&minute,&second,&millisecond,
			&eastCoord,&northCoord,&height,&heading,&pitch,&roll);
		return nSize >= 15? true : false;

		//return nSize;
	}

	char str[128];
	double gpsTime;
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	int millisecond;
	double eastCoord;
	double northCoord;
	double height;
	double heading;
	double pitch;
	double roll;
};

void hnZfsCombine::testCalLinPos( const char* str_pos_path ,const char* str_lin_path )
{
	// 加载LIN文件
	std::vector<LIN_POS_INFO> vecLinInfo;
	FILE* ptr_file = fopen(str_lin_path,"rt");
	char str_line[1024];
	memset(str_line,0,1024);
	fgets(str_line,1024,ptr_file);
	while (!feof(ptr_file))
	{
		memset(str_line,0,1024);
		fgets(str_line,1024,ptr_file);

		LIN_POS_INFO info;
		bool succ = info.serialize(str_line);
		if (succ)
		{
			vecLinInfo.push_back(info);
		}
	}
	fclose(ptr_file);


	// 加载POS文件;
	FILE* ptr_save = fopen("d:\\lin_check.csv","wt");
	std::vector<POS_STRUCT_INFO> vecPosInfo;
	loadPosData(str_pos_path,vecPosInfo);

	for (unsigned int n = 0;n < vecLinInfo.size();n++)
	{
		LIN_POS_INFO lin_info = vecLinInfo[n];

		DATE_TIME_INFO date_time;
		date_time.year = lin_info.year;
		date_time.month = lin_info.month;
		date_time.day = lin_info.day;
		date_time.hour = lin_info.hour;
		date_time.minute = lin_info.minute;
		date_time.second = lin_info.second;
		date_time.milliseconds = lin_info.millisecond * 1000.0;

		int gpsWeek;
		double gpsSecond;
		UTCT2GPST(date_time,gpsWeek,gpsSecond,0.0);

		POS_STRUCT_INFO posInfo;
		int nearest_pos_index = -1;
		bool bSuc = linearInsertPos(gpsSecond,vecPosInfo,posInfo,nearest_pos_index);

		convertBlhToNehN(posInfo);

		double dd_east = posInfo.dEastCoord - lin_info.eastCoord;
		double dd_north = posInfo.dNorthCoord - lin_info.northCoord;
		double dd_h = posInfo.dHeight - lin_info.height;
		double dd_heading = posInfo.dHeading - lin_info.heading;
		double dd_pitch = posInfo.dPitch - lin_info.pitch;
		double dd_roll = posInfo.dRoll - lin_info.roll;

		fprintf_s(ptr_save,"%.4lf,%.4lf,%.4lf,%.4lf,%.4lf,%.4lf\n",
			dd_east,dd_north,dd_h,dd_heading,dd_pitch,dd_roll);
	}
	fclose(ptr_save);
	return;


	double gps_time1 = 519357.673;
	double gps_time2 = 519369.454;

	POS_STRUCT_INFO posInfo;
	int nearest_pos_index = -1;
	bool bSuc = linearInsertPos(gps_time1,vecPosInfo,posInfo,nearest_pos_index);

	// pos to world
	double arry_pos_temp[16] = {0};

	// 度转弧度
	double tmpHeading = posInfo.dHeading * PI64 / 180.0;
	double tmpPitch = posInfo.dPitch * PI64 / 180.0;
	double tmpRoll = posInfo.dRoll * PI64 / 180.0;
	computeMatrixByIScanAngle(posInfo.dEastCoord,posInfo.dNorthCoord,posInfo.dHeight,
		tmpHeading,tmpPitch,tmpRoll,arry_pos_temp);

	// 矩阵信息赋值
	MatrixXd mat_pos_to_world(4,4);
	mat_pos_to_world(0,0) = arry_pos_temp[0];  mat_pos_to_world(0,1) = arry_pos_temp[1]; mat_pos_to_world(0,2) = arry_pos_temp[2]; mat_pos_to_world(0,3) = arry_pos_temp[3];
	mat_pos_to_world(1,0) = arry_pos_temp[4];  mat_pos_to_world(1,1) = arry_pos_temp[5]; mat_pos_to_world(1,2) = arry_pos_temp[6]; mat_pos_to_world(1,3) = arry_pos_temp[7];
	mat_pos_to_world(2,0) = arry_pos_temp[8];  mat_pos_to_world(2,1) = arry_pos_temp[9]; mat_pos_to_world(2,2) = arry_pos_temp[10]; mat_pos_to_world(2,3) = arry_pos_temp[11];
	mat_pos_to_world(3,0) = arry_pos_temp[12];  mat_pos_to_world(3,1) = arry_pos_temp[13]; mat_pos_to_world(3,2) = arry_pos_temp[14]; mat_pos_to_world(3,3) = arry_pos_temp[15];

	// 激光到pos构建的矩阵
	// 度转弧度
	tmpHeading = m_laser_topos_heading * PI64 / 180.0;
	tmpPitch = m_laser_topos_pitch * PI64 / 180.0;
	tmpRoll = m_laser_topos_roll * PI64 / 180.0;
	computeMatrixByIScanAngle(m_laser_topos_x,m_laser_topos_y,m_laser_topos_z,
		tmpHeading,tmpPitch,tmpRoll,arry_pos_temp);
	MatrixXd mat_laser_to_pos(4,4);
	mat_laser_to_pos(0,0) = arry_pos_temp[0];  mat_laser_to_pos(0,1) = arry_pos_temp[1]; mat_laser_to_pos(0,2) = arry_pos_temp[2]; mat_laser_to_pos(0,3) = arry_pos_temp[3];
	mat_laser_to_pos(1,0) = arry_pos_temp[4];  mat_laser_to_pos(1,1) = arry_pos_temp[5]; mat_laser_to_pos(1,2) = arry_pos_temp[6]; mat_laser_to_pos(1,3) = arry_pos_temp[7];
	mat_laser_to_pos(2,0) = arry_pos_temp[8];  mat_laser_to_pos(2,1) = arry_pos_temp[9]; mat_laser_to_pos(2,2) = arry_pos_temp[10]; mat_laser_to_pos(2,3) = arry_pos_temp[11];
	mat_laser_to_pos(3,0) = arry_pos_temp[12];  mat_laser_to_pos(3,1) = arry_pos_temp[13]; mat_laser_to_pos(3,2) = arry_pos_temp[14]; mat_laser_to_pos(3,3) = arry_pos_temp[15];

	// 计算旋转矩阵;
	MatrixXd mat_laser_to_wgs(4,4);
	mat_laser_to_wgs = mat_pos_to_world * mat_laser_to_pos;

	//// 结果值记录
	//MatrixXd mat_point_result(4,1);
	//mat_point_result = mat_pos_to_world * mat_laser_to_pos * mat_point_pos;

	double fRotateMatrix[9];
	fRotateMatrix[0] = mat_laser_to_wgs(0,0);   fRotateMatrix[1] = mat_laser_to_wgs(0,1);   fRotateMatrix[2] = mat_laser_to_wgs(0,2);
	fRotateMatrix[3] = mat_laser_to_wgs(1,0);   fRotateMatrix[4] = mat_laser_to_wgs(1,1);   fRotateMatrix[5] = mat_laser_to_wgs(1,2);
	fRotateMatrix[6] = mat_laser_to_wgs(2,0);   fRotateMatrix[7] = mat_laser_to_wgs(2,1);   fRotateMatrix[8] = mat_laser_to_wgs(2,2);

	double a3 = fRotateMatrix[6];
	double c3 = fRotateMatrix[8];
	double b1 = fRotateMatrix[1];
	double b2 = fRotateMatrix[4];
	double b3 = fRotateMatrix[7];

	double sinPitch = b3;
	double cosPitch = sqrt(b1*b1 + b2*b2);

	double Pitch = atan2(sinPitch, cosPitch);	

	double Roll = atan2(-a3, c3);	

	double Yaw = atan2(b1, b2);

	double yaw_angle = Yaw * 180.0 / PI64;
	double pitch_angle = Pitch * 180.0 / PI64;
	double roll_angle = Roll * 180.0 / PI64;


	int test = 0;
}

void hnZfsCombine::convertBlhToNehN( POS_STRUCT_INFO& posInfo )
{
	// 中间变量定义；
	double tmp_centre_l = 0.0;
	int zone_num = 0;

	// 根据第一个点计算带号;
	zone_num = (posInfo.dLongitude + 1.5)/3; 
	tmp_centre_l = zone_num * 3.0;

	// 设置投影坐标转换系统；
	hdProjectedCoordinateSystem projected_coord_system("gauss_wgs84",tmp_centre_l);


	// 经纬度转投影坐标
	projected_coord_system.GaussProject_BL2NE(posInfo.dLatitude,posInfo.dLongitude,posInfo.dNorthCoord,posInfo.dEastCoord);
}

bool hnZfsCombine::UTCT2GPST( const DATE_TIME_INFO& stTime,int& nGpsWeek,double& dGpsSeconds,double dGPSSubUTC/*= 0.*/ )
{
	int dayofw(0),dayofy(0), yr(0), ttlday(0), m(0), weekno(0);
	const  int  dinmth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	//  Convert day, month and year to day of year 
	if (stTime.month == 1)
	{
		dayofy = stTime.day;
	}
	else
	{
		dayofy = 0;
		for (m=1; m<=(stTime.month-1); m++)
		{
			dayofy += dinmth[m];
			if ( m==2 )
			{
				if (stTime.year % 4 == 0 && stTime.year % 100 != 0 || stTime.year % 400 == 0) 
				{
					dayofy += 1;
				}
			}
		}
		dayofy += stTime.day;
	}
	//  Convert day of year and year into week number and day of week 
	ttlday = 360;
	for (yr=1981; yr<=(stTime.year-1); yr++)
	{
		ttlday  += 365;
		if (yr % 4 == 0 && yr % 100 != 0 || yr % 400 ==0)
		{
			ttlday  += 1;
		}
	}
	ttlday += dayofy;
	weekno  = ttlday/7;
	dayofw  = ttlday - 7 * weekno;

	nGpsWeek    = weekno; 
	dGpsSeconds = dayofw * 86400.0 + stTime.hour * 3600.0 + stTime.minute * 60.0 + stTime.second +stTime.milliseconds/1000000.;

	dGpsSeconds += dGPSSubUTC;
	if(dGpsSeconds > 7*24*3600)
	{
		dGpsSeconds -= 7*24*3600;
		nGpsWeek    += 1;
	}

	return TRUE;
}

// 设置传入iScan-para.db路径，与setScanPara互通;
using namespace hd::scanroute;
bool hnZfsCombine::setiScanParaPath(int scan_no, const char* str_iscan_para_path )
{
	// 检查文件存在;
	if (_access(str_iscan_para_path,0) != 0)
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
		for (int nn = 0;nn < qstr_list.size();nn++)
		{
			QString str = qstr_list[nn];
			if (str.startsWith("LeapSec=")) // 以LeapSec开头
			{
				int time_leap  = 0;
				sscanf_s(str.toLocal8Bit().data(),"LeapSec=\"%d\"",&time_leap);
				m_utc_to_gps_second = 0.0 - time_leap;
				break;
			}
		}
	}

	// 读取参数;
	hnParamSQLiteDB iscan_para_db;
	HBOOL is_success = iscan_para_db.open(str_iscan_para_path);
	if (!is_success)
	{
		return false;
	}

	// 读取参数信息
	hdHiScanLidarPara iscan_lidar_para;
	is_success = iscan_para_db.getLidarPara(scan_no,iscan_lidar_para);
	iscan_para_db.close();
	if (!is_success)
	{
		return false;
	}

	// 传入内部;
	setScanPara(iscan_lidar_para.dx,iscan_lidar_para.dy,iscan_lidar_para.dz,
		iscan_lidar_para.dyaw,iscan_lidar_para.dpitch,iscan_lidar_para.droll);
	return true;
}

void hnZfsCombine::setCombineSetting( hn::hnProjectSetting* project_setting )
{
	m_project_setting = project_setting;

	// 设置hdProjectedCoordinateSystem坐标转换相关参数;
	if (!m_ptr_coord_trans_system)
	{
		// 定义坐标系;
		hdSpatialReferenceH source_coord_system = hdSpatialReferenceCreate(E_SR_PROJ);
		hdSpatialReferenceH dest_coord_system = hdSpatialReferenceCreate(E_SR_PROJ);

		m_ptr_coord_trans_system = new hd::geoproj::hdCoordinateTransformation(source_coord_system,dest_coord_system);
	}
	
	// 坐标信息设置，坐标源;
	hdSpatialReferenceH source_cs = m_ptr_coord_trans_system->getSourceCS();
	if (!source_cs)
	{
		return;
	}
	hdProjectedCoordinateSystem* ptr_source_coord = static_cast<hdProjectedCoordinateSystem*>(source_cs);
	if (!ptr_source_coord)
	{
		return;
	}

	// 目标源;
	hdSpatialReferenceH dest_cs = m_ptr_coord_trans_system->getTargetCS();
	if (!dest_cs)
	{
		return;
	}
	hdProjectedCoordinateSystem* ptr_dest_coord = static_cast<hdProjectedCoordinateSystem*>(dest_cs);
	if (!ptr_dest_coord)
	{
		return;
	}

	// 设置原始数据属性;
	memset(&m_src_param, 0, sizeof(m_src_param));
	m_src_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_src_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_src_param.earthType = E_EARTH_TYPE_WGS84;

	// 设置目标数据属性;
	memset(&m_dst_param, 0, sizeof(m_dst_param));
	m_dst_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dst_param.coorUnit = E_COOR_UNIT_TYPE_METER;
	
	// 目标数据对象目标椭球体;
	hdGeographicCoordinateSystem geographic_coord_system;
	COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	switch (combine_coord_project_info.sphere_target_type)
	{
	case E_COMBINE_PROJECT_SPHERE_BEIJING54:
		{
			m_dst_param.earthType = E_EARTH_TYPE_Beijing54;
			geographic_coord_system.m_datum_type = BEIJING_1954;
			break;
		}

	case E_COMBINE_PROJECT_SPHERE_XIAN80:
		{
			m_dst_param.earthType = E_EARTH_TYPE_Xian80;
			geographic_coord_system.m_datum_type = XIAN_1980;
			break;
		}
	case E_COMBINE_PROJECT_SPHERE_WGS84:
		{
			m_dst_param.earthType = E_EARTH_TYPE_WGS84;
			geographic_coord_system.m_datum_type = WGS_1984;
			break;
		}
	case E_COMBINE_PROJECT_SPHERE_China2000:
		{
			m_dst_param.earthType = E_EARTH_TYPE_China2000;
			geographic_coord_system.m_datum_type = WGS_1984;
			break;
		}
	}

	// 设置目标地理坐标系;
	ptr_dest_coord->setGeogCoordSystem(geographic_coord_system);

	// 坐标投影方法;
	switch (combine_coord_project_info.sphere_zone_type)
	{
	case E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_3:
		{
			m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
			m_dst_param.W = 3;
			ptr_dest_coord->setProjectMethod(GAUSS_KRUGER);
			ptr_source_coord->setProjectMethod(GAUSS_KRUGER);
			break;
		}
	case E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_6:
		{
			m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
			m_dst_param.W = 6;
			ptr_dest_coord->setProjectMethod(GAUSS_KRUGER);
			ptr_source_coord->setProjectMethod(GAUSS_KRUGER);
			break;
		}
	case E_COMBINE_PROJECT_TYPE_TRANSVERSE_MERCATOR:
		{
			m_dst_param.prjType = E_PROJECT_TYPE_Mercator;
			m_dst_param.W = 3;
			ptr_dest_coord->setProjectMethod(TRANSVERSE_MERCATOR);
			ptr_source_coord->setProjectMethod(TRANSVERSE_MERCATOR);
			break;
		}
	}

	// 中央经线等设置;
	m_dst_param.Lo = combine_coord_project_info.centre_longtitude * PI64 / 180.0;
	m_dst_param.Ko = 1.0;
	m_dst_param.FE = combine_coord_project_info.east_offset;
	m_dst_param.Bc = combine_coord_project_info.average_latitude;
	m_dst_param.PH = combine_coord_project_info.project_height;

	// 设置中央经线等;
	ptr_source_coord->setCentralMeridian(combine_coord_project_info.centre_longtitude);
	ptr_source_coord->setFalseEasting(combine_coord_project_info.east_offset);
	ptr_source_coord->setFalseNorthing(0.0);
	ptr_source_coord->m_proj_height = combine_coord_project_info.project_height;
	ptr_source_coord->m_mean_latitude = combine_coord_project_info.average_latitude;

	ptr_dest_coord->setCentralMeridian(combine_coord_project_info.centre_longtitude);
	ptr_dest_coord->setFalseEasting(combine_coord_project_info.east_offset);
	ptr_dest_coord->setFalseNorthing(0.0);
	ptr_dest_coord->m_proj_height = combine_coord_project_info.project_height;
	ptr_dest_coord->m_mean_latitude = combine_coord_project_info.average_latitude;

	// 构建设置坐标转换参数;
	COMBINE_COORD_CONVERT_SET_STRUCT convert_set_info = m_project_setting->getCoordConvertInfo();
	if (convert_set_info.use_coord_convert)
	{
		m_ptr_convert_translator = NULL;
		CreateIHdPJTranslator(&m_ptr_convert_translator);
		m_ptr_convert_translator->SetSrcSpatialRef(&m_src_param);
		m_ptr_convert_translator->SetDstSpatialRef(&m_dst_param);

		// 为四参数转换模型;
		if (convert_set_info.use_coord_convert_model == 0)
		{
			E_PJTFourPar_T pj_four_param;
			pj_four_param.Dx = convert_set_info.four_param_dx;
			pj_four_param.Dy = convert_set_info.four_param_dy;
			pj_four_param.T = convert_set_info.four_param_dr;
			pj_four_param.K = convert_set_info.four_param_k;
			m_ptr_convert_translator->SetFourParam(1,&pj_four_param);

			E_PJHeightFixPar_T pj_height_fit_param;
			pj_height_fit_param.A = convert_set_info.height_fit_a;
			pj_height_fit_param.B = convert_set_info.height_fit_b;
			pj_height_fit_param.C = convert_set_info.height_fit_c;
			pj_height_fit_param.D = convert_set_info.height_fit_d;
			pj_height_fit_param.E = convert_set_info.height_fit_e;
			pj_height_fit_param.F = convert_set_info.height_fit_f;
			m_ptr_convert_translator->SetHeightFitParam(convert_set_info.height_fit_model+1,&pj_height_fit_param);
		}
		else if (convert_set_info.use_coord_convert_model == 1 )
		{
			// 为七参数转换模型;
			E_PJTSevenPar_T pj_seven_param;
			pj_seven_param.DX = convert_set_info.seven_param_dx;
			pj_seven_param.DY = convert_set_info.seven_param_dy;
			pj_seven_param.DZ = convert_set_info.seven_param_dz;
			pj_seven_param.WX = convert_set_info.seven_param_rx * PI64 /(180.0 * 3600.0);
			pj_seven_param.WY = convert_set_info.seven_param_ry * PI64 /(180.0 * 3600.0);
			pj_seven_param.WZ = convert_set_info.seven_param_rz * PI64 /(180.0 * 3600.0);
			pj_seven_param.K = convert_set_info.seven_param_k;

			m_ptr_convert_translator->SetSevenParam(2,&pj_seven_param);
		}
	}
}

bool hnZfsCombine::isPointInCtrlRadius( double point_x,double point_y,double point_z,std::vector<COMBINE_CTRL_POINT_STRUCT_INFO> vec_ctrl_points, double dist_tol )
{
	// 判断点是否在控制点范围内;
	bool is_in = false;
	double pow_dist_tol = dist_tol * dist_tol;
	double temp_dist = 100000.0;
	for ( unsigned int n = 0;n < vec_ctrl_points.size();n++ )
	{
		COMBINE_CTRL_POINT_STRUCT_INFO& ctrl_point_info = vec_ctrl_points[n];

		double temp_dist = pow(ctrl_point_info.ctrl_coord_x - point_x,2.0) + 
			pow(ctrl_point_info.ctrl_coord_y - point_y,2.0) + pow(ctrl_point_info.ctrl_coord_z - point_z,2.0);

		// 判断比较;
		if (temp_dist < pow_dist_tol)
		{
			is_in = true;
			break;
		}
	}

	return is_in;
}

void hnZfsCombine::setUseThread( bool use_thread )
{
	m_use_thread = use_thread;
}

void hnZfsCombine::setProgress( float p,const char* str_msg)
{
	QString str_mgs_t = QString::fromLocal8Bit(str_msg);
	emit progress(p,str_mgs_t);
}

bool hnZfsCombine::exportLinFile( double start_time,double end_time, const char* strLinPath,std::vector<POS_STRUCT_INFO>& vecInfo )
{
	// 打开文件;
	FILE* ptr_file = fopen(strLinPath,"wt");
	if (!ptr_file)
	{
		return false;
	}

	// 写入文件头;
	fprintf_s(ptr_file,"Round	GPSSecond	Year	Month	Day	Hour	Minute	Second	Millisecond	POS-X	POS-Y	POS-Z	POS-Heading	POS-Pitch	POS-Roll\n");

	// 计算输出间隔值;
	bool bSuc = false;
	int export_lin_count = (end_time - start_time + 0.005) / 0.005;
	for (int n = 0;n < export_lin_count;n++)
	{
		if (!m_is_running)
		{
			break;
		}

		double cur_gps_time = start_time + n * 0.005;

		// 首先根据时间信息进行插值计算，获取该时刻POS位置姿态;
		POS_STRUCT_INFO curPos;
		int nearest_pos_index = -1;
		bSuc = linearInsertPos(cur_gps_time,vecInfo,curPos,nearest_pos_index);
		if (!bSuc)
		{
			continue;
		}

		// 矩阵运算，获取绝对坐标;
		double dx,dy,dz;
		dx = 0.0;
		dy = 0.0;
		dz = 0.0;
		calcuCoord(curPos,dx,dy,dz);

		DATE_TIME_INFO date_info;
		date_info.year = m_scan_date_info.year;
		date_info.month = m_scan_date_info.month;
		date_info.day = m_scan_date_info.day;

		// 换算天内秒;
		double temp_in_day_time = 0.0;
		int temp_day = cur_gps_time / (3600*24);
		temp_in_day_time = cur_gps_time - temp_day * 3600*24;

		date_info.hour = (int)(temp_in_day_time / 3600);
		date_info.minute = (int)(temp_in_day_time - date_info.hour * 3600)/60;
		date_info.second = (int)(temp_in_day_time - date_info.hour * 3600.0 - date_info.minute * 60.0);
		date_info.milliseconds = (int)((temp_in_day_time - date_info.hour * 3600.0 - date_info.minute * 60.0 - date_info.second) * 1000);

		// 写入一行信息;
		//0000018	401040.114	2018	07	12	15	24	00	114	500114.601	3106018.801	42.636	-126.5252	0.1385	0.2735
		fprintf_s(ptr_file,"%07d	%.3lf	%04d	%02d	%02d	%02d	%02d	%02d	%02d	%.3lf	%.3lf	%.3lf	%.3lf	%.3lf	%.3lf\n",
			n+1,cur_gps_time,date_info.year,date_info.month,date_info.day,date_info.hour,date_info.minute,date_info.second,date_info.milliseconds,
			dx,dy,dz,curPos.dHeading,curPos.dPitch,curPos.dRoll);

		if (loadCallback && n % 50 == 0)
		{
			loadCallback(n * 1.0 / export_lin_count,"写入LIN文件...");
		}

		if (m_use_thread && n % 50 == 0)
		{
			setProgress(n * 1.0 / export_lin_count,"写入LIN文件...");
		}
	}
	fclose(ptr_file);

	if (loadCallback)
	{
		loadCallback(1.0,"写入LIN文件完成...");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"写入LIN文件完成...");
	}

	return true;
}

bool hnZfsCombine::dataCombineHls( const char* strZfsPath,const char* strPosPath,const char* strHlsPath,const char* strTimePath)
{
	// 检查文件是否存在
	bool bSuc = false;
	if (_access(strZfsPath,0) != 0 )
	{
		return false;
	}

	if (_access(strPosPath,0) != 0)
	{
		return false;
	}

	// 加载POS
	std::vector<POS_STRUCT_INFO> vecPosInfo;
	bSuc = loadPosData(strPosPath,vecPosInfo);
	if (!bSuc || !m_is_running)
	{
		return false;
	}

	if (loadCallback)
	{
		loadCallback(0.0,"正在打开ZFS文件...");
	}

	if (m_use_thread)
	{
		setProgress(0.0,"正在打开ZFS文件...");
	}

	// 打开zfs文件
	hnZfsReader zfs_reader;
	bSuc = zfs_reader.OpenZfsFile(strZfsPath);
	if (!bSuc || !m_is_running)
	{
		if (loadCallback)
		{
			loadCallback(0.0,"打开ZFS文件失败");
		}

		if (m_use_thread)
		{
			setProgress(0.0,"打开ZFS文件失败");
		}

		return false;
	}

	if (loadCallback)
	{
		loadCallback(1.0,"打开ZFS文件完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"打开ZFS文件完成");
	}

	// 设置外部传入参数
	zfs_reader.setScanTime(m_scan_date_info);
	zfs_reader.setUtcToGpsAbs(m_utc_to_gps_second);

	// HLS头文件信息
	hdWriterHLS hls_writer;
	hdHeaderHLS hls_header;
	hls_header.setPointFormat(HLS2_POINTFORMAT_XYZIRGBP);
	hls_header.m_project_ID_GUID_data_1 = 1;
	hls_header.m_project_ID_GUID_data_2 = 2;
	hls_header.m_project_ID_GUID_data_3 = 3;
	hls_header.m_file_source_id = 0;
	hls_header.m_version_major = 2;	// Hls2.0
	hls_header.m_version_minor = 0;
	strcpy(hls_header.m_system_identifier,"HD 3LS ZFS");
	strcpy(hls_header.m_generating_software,"HD-3LS-SCAN");

	// 判断是否需要导出时间文件及LIN文件;
	bool need_lin = m_project_setting->getExportFilesInfo().need_export_lin_file;
	bool need_time = m_project_setting->getExportFilesInfo().need_export_time_file;

	// lin文件
	if (need_lin)
	{
		std::string str_lin_path = strHlsPath;
		int npos = str_lin_path.find_last_of('/');
		if (npos < 0)
		{
			npos = str_lin_path.find_last_of('\\');
		}
		str_lin_path = str_lin_path.substr(0,npos+1);

		std::string std_pcd_name = strHlsPath;
		std_pcd_name = std_pcd_name.substr(npos+1);
		int nlpos = std_pcd_name.find_last_of('.');
		std_pcd_name = std_pcd_name.substr(0,nlpos+1);

		int posname_p = std_pcd_name.find_last_of('P');
		std::string str_lin_name = std_pcd_name.replace(posname_p,3,"Pos");

		str_lin_path += str_lin_name;
		str_lin_path += "lin";

		// 获取扫描时间信息;
		double scan_start_time,scan_end_time;
		scan_start_time = scan_end_time = 0.0;
		zfs_reader.getScanTimeRange(scan_start_time,scan_end_time);

		exportLinFile(scan_start_time,scan_end_time,str_lin_path.data(),vecPosInfo);
	}


	// 记录更新时间
	time_t timer;
	time(&timer);
	tm* t_tm = localtime(&timer);
	hls_header.m_file_creation_day = t_tm->tm_yday + 1;
	hls_header.m_file_creation_year = t_tm->tm_year + 1900;

	// 记录头文件偏移量
	double gps_time_offset = 0.0;
	bool bFirst = true;
	double header_offset_x = 0.0;
	double header_offset_y = 0.0;
	double header_offset_z = 0.0;

	// 一次申请内存
	hdPointXYZIPRGBA* ptrLoopData = new hdPointXYZIPRGBA[50000];
	memset(ptrLoopData,0,sizeof(hdPointXYZIPRGBA) * 50000);

	// 时间存储信息文件内存申请
	COMBINE_TIME_POINT_STRUCT* ptrTimeLoopData = NULL;
	ptrTimeLoopData = new COMBINE_TIME_POINT_STRUCT[50000];
	memset(ptrTimeLoopData,0,sizeof(COMBINE_TIME_POINT_STRUCT) * 50000);

	// 逐圈读取数据
	int nLoopCount = zfs_reader.GetScanLines();

	// 若使用控制点即进行POS纠偏处理，则考虑只需要提取POS对应时间点一定范围内数据即可;
	//std::vector<int> zfs_need_read_scanlines_index;
	COMBINE_TIME_DELAY_SET_STRUCT time_delay_info = m_project_setting->getTimeDelayInfo();
	COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT use_ctrl_param_set = m_project_setting->getUseCtrlRadiusExportInfo();
	if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
	{
		// 获取距离最近POS时间点
		std::vector<COMBINE_TIME_RANGE> vec_combine_time_range;
		calcuCtrlPosTime(m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius * 2,vecPosInfo,vec_combine_time_range);

		for (unsigned int nm = 0;nm < vec_combine_time_range.size();nm++)
		{
			COMBINE_TIME_RANGE& time_range = vec_combine_time_range[nm];
			time_range.start_gps_second += time_delay_info.pcd_time_delay * 0.001;
			time_range.end_gps_second += time_delay_info.pcd_time_delay * 0.001;
		}

		//COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
		//std::vector<COMBINE_TIME_RANGE> vec_combine_time_result;
		//if (use_time_export_set.is_use_time_range_export)
		//{
		//	m_vec_combine_time_range = use_time_export_set.vec_combine_time_range;
		//	getCtrlTimeInTime(vec_combine_time_range,m_vec_combine_time_range,vec_combine_time_result);
		//}
		//else
		//{
		//	vec_combine_time_result = vec_combine_time_range;
		//}
		
		if (vec_combine_time_range.size() <= 0)
		{
			zfs_reader.CloseZfsFile();
			return false;
		}

		zfs_reader.setTimeRange(vec_combine_time_range);
	}
	else
	{
		// 若使用时间进行过滤;
		COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
		if (use_time_export_set.is_use_time_range_export)
		{
			m_vec_combine_time_range = use_time_export_set.vec_combine_time_range;

			for (unsigned int nm = 0;nm < m_vec_combine_time_range.size();nm++)
			{
				COMBINE_TIME_RANGE& time_range = m_vec_combine_time_range[nm];
				time_range.start_gps_second += time_delay_info.pcd_time_delay * 0.001;
				time_range.end_gps_second += time_delay_info.pcd_time_delay * 0.001;
			}
		}

		zfs_reader.setTimeRange(m_vec_combine_time_range);
	}

	// 头文件信息设置;
	int nPointCount = 0;
	hls_header.m_number_of_col = nLoopCount;

	// 打开写出文件失败返回;
	if (!hls_writer.open(strHlsPath,&hls_header) || !m_is_running)
	{
		return false;
	}

	// 打开写入时间信息文件;
	hnCombineTimeWriter timeWriter;
	if (need_time)
	{
		bSuc = timeWriter.Open(strTimePath);
		if (!bSuc)
		{
			return false;
		}
	}

	// 相关参数定义;
	int have_read_pts_in_loop = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> loop_points;
	loop_points.resize(5000);
	int have_write_loop_count = 0;
	zfs_reader.startRead();
	for (unsigned int nn = 0;nn < nLoopCount;nn++)
	{
		int n = nn;
		if (!m_is_running)
		{
			break;
		}

		// 逐圈获取数据
		have_read_pts_in_loop = 0;
		bSuc = zfs_reader.getLinePoints(n,loop_points,have_read_pts_in_loop);
		if (!bSuc || have_read_pts_in_loop == 0 || !m_is_running)
		{
			if ( loadCallback && nn % 500 == 0 )
			{
				loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
			}
			if (m_use_thread && nn % 500 == 0 )
			{
				setProgress(nn * 1.0 / nLoopCount,"融合解算...");
			}

			continue;
		}

		memset(ptrLoopData,0,sizeof(hdPointXYZIPRGBA) * 50000);
		memset(ptrTimeLoopData,0,sizeof(COMBINE_TIME_POINT_STRUCT) * 50000);

		//have_read_pts_in_loop = 0;
		//loop_points.clear();
		//FILE* ptr_file = fopen("F:\\HD099_20180922181447\\DiseaseInfoRing.txt","rt");
		//while (!feof(ptr_file))
		//{
		//	double xx,yy,zz;
		//	POINT_STRUCT_XYZIT_INFO info;
		//	int nret = fscanf_s(ptr_file,"%lf,%lf,%lf,%lf,%d\n",&yy,&xx,&zz,&info.timeSecond,&info.intensity);
		//	if (nret < 4)
		//	{
		//		continue;
		//	}
		//	info.x = 0.0002;
		//	info.x = xx;
		//	info.y = yy;
		//	info.z = zz;
		//	loop_points.push_back(info);
		//	have_read_pts_in_loop++;
		//}
		//fclose(ptr_file);
		//FILE* ptr_file_save =  fopen("F:\\HD099_20180922181447\\DiseaseInfoRing_save.xyz","wt");


		// 每个点进行坐标转换
		int loopWriteCount = 0;
		for (unsigned int m = 0;m < have_read_pts_in_loop;m++)
		{
			// 同海达融合软件保持一致，外部设置延时80毫秒即将每个点的时间减去80毫秒;
			POINT_STRUCT_XYZIT_INFO& point_info = loop_points[m];
			point_info.timeSecond -= time_delay_info.pcd_time_delay * 0.001;

			// 首先根据时间信息进行插值计算，获取该时刻POS位置姿态;
			POS_STRUCT_INFO curPos;
			int nearest_pos_index = -1;
			bSuc = linearInsertPos(point_info.timeSecond,vecPosInfo,curPos,nearest_pos_index);
			if (!bSuc)
			{
				continue;
			}

			// 矩阵运算，获取绝对坐标;
			double dx,dy,dz;
			dx = point_info.x;
			dy = point_info.y;
			dz = point_info.z;
			calcuCoord(curPos,dx,dy,dz);

			//fprintf_s(ptr_file_save,"%.4lf,%.4lf,%.4lf,%d\n",dx,dy,dz,point_info.intensity);

			// 若使用控制点范围过滤;
			if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
			{
				// 判断点是否在控制点范围内;
				bool is_point_in = isPointInCtrlRadius(dx,dy,dz,m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius);
				if (!is_point_in)
				{
					continue;
				}
			}

			// 以pos第一个点作为系统偏移量计算，防止坐标过大，精度丢失;
			if (bFirst)
			{
				gps_time_offset = point_info.timeSecond;
				header_offset_x = dx;
				header_offset_y = dy;
				header_offset_z = dz;
				bFirst = false;
			}

			// 跳过0点得了
			float tmpX,tmpY,tmpZ;
			tmpX = dx - header_offset_x;
			tmpY = dy - header_offset_y;
			tmpZ = dz - header_offset_z;
			if ((tmpX == 0.0f && tmpY == 0.0f && tmpZ == 0.0f))
			{
				continue;
			}

			// 记录时间信息文件;
			if (need_time)
			{
				ptrTimeLoopData[loopWriteCount].x = point_info.x;
				ptrTimeLoopData[loopWriteCount].y = point_info.y;
				ptrTimeLoopData[loopWriteCount].z = point_info.z;
				ptrTimeLoopData[loopWriteCount].time = point_info.timeSecond - gps_time_offset;
			}

			// 记录坐标，此处计算即为相对坐标;
			ptrLoopData[loopWriteCount].x = dx - header_offset_x;
			ptrLoopData[loopWriteCount].y = dy - header_offset_y;
			ptrLoopData[loopWriteCount].z = dz - header_offset_z;
			ptrLoopData[loopWriteCount].intensity = point_info.intensity;
			loopWriteCount++;
		}

		//fclose(ptr_file_save);
		//return true;

		if (loopWriteCount <= 0)
		{
			continue;
		}

		nPointCount += loopWriteCount;
		have_write_loop_count++;

		if ( loadCallback && nn % 500 == 0 )
		{
			loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
		}

		if (m_use_thread && nn % 500 == 0)
		{
			setProgress(nn * 1.0 / nLoopCount,"融合解算...");
		}

		// 一圈点云写入
		if (loopWriteCount > 0)
		{
			hls_writer.writeLoop(ptrLoopData,loopWriteCount);
			if (need_time)
			{
				timeWriter.WriteLoop(ptrTimeLoopData,loopWriteCount);
			}

		}
	}

	// 头文件信息记录
	hls_writer.m_hls_header.m_number_of_col = have_write_loop_count;
	hls_writer.m_hls_header.m_offset_x = header_offset_x;
	hls_writer.m_hls_header.m_offset_y = header_offset_y;
	hls_writer.m_hls_header.m_offset_z = header_offset_z;
	hls_writer.m_hls_header.m_number_of_point_records = nPointCount;

	hls_writer.close();
	zfs_reader.CloseZfsFile();

	// 写入头文件更新
	if (need_time)
	{
		timeWriter.m_header.time_offset = gps_time_offset;
		timeWriter.m_header.loopCount = have_write_loop_count;
		timeWriter.m_header.pointRecordCount = nPointCount;
		timeWriter.Close();
	}

	if (loadCallback)
	{
		loadCallback(1.0,"处理完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"处理完成");
	}

	// 内存释放
	if (ptrLoopData)
	{
		delete []ptrLoopData;
		ptrLoopData = NULL;
	}

	// 内存释放
	if (ptrTimeLoopData)
	{
		delete[] ptrTimeLoopData;
		ptrTimeLoopData = NULL;
	}

	if (!m_is_running)
	{
		return false;
	}
	return true;
}

bool hnZfsCombine::dataCombineHlz( const char* strZfsPath,const char* strPosPath,const char* strHlzPath,const char* strTimePath)
{
	// 检查文件是否存在
	bool bSuc = false;
	if (_access(strZfsPath,0) != 0 )
	{
		return false;
	}

	if (_access(strPosPath,0) != 0)
	{
		return false;
	}

	// 加载POS
	std::vector<POS_STRUCT_INFO> vecPosInfo;
	bSuc = loadPosData(strPosPath,vecPosInfo);
	if (!bSuc)
	{
		return false;
	}

	if (loadCallback)
	{
		loadCallback(0.0,"打开ZFS文件");
	}

	if (m_use_thread)
	{
		setProgress(0.0,"打开ZFS文件");
	}

	// 打开zfs文件
	hnZfsReader zfs_reader;
	bSuc = zfs_reader.OpenZfsFile(strZfsPath);
	if (!bSuc || !m_is_running)
	{
		return false;
	}

	if (loadCallback)
	{
		loadCallback(1.0,"打开ZFS文件完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"打开ZFS文件完成");
	}

	// 设置外部传入参数
	zfs_reader.setScanTime(m_scan_date_info);
	zfs_reader.setUtcToGpsAbs(m_utc_to_gps_second);

	// 判断是否需要导出时间文件及LIN文件;
	bool need_lin = m_project_setting->getExportFilesInfo().need_export_lin_file;

	// lin文件
	if (need_lin)
	{
		std::string str_lin_path = strHlzPath;
		int npos = str_lin_path.find_last_of('/');
		if (npos < 0)
		{
			npos = str_lin_path.find_last_of('\\');
		}
		str_lin_path = str_lin_path.substr(0,npos+1);

		std::string std_pcd_name = strHlzPath;
		std_pcd_name = std_pcd_name.substr(npos+1);
		int nlpos = std_pcd_name.find_last_of('.');
		std_pcd_name = std_pcd_name.substr(0,nlpos+1);

		int posname_p = std_pcd_name.find_last_of('P');
		std::string str_lin_name = std_pcd_name.replace(posname_p,3,"Pos");

		str_lin_path += str_lin_name;
		str_lin_path += "lin";

		// 获取扫描时间信息;
		double scan_start_time,scan_end_time;
		scan_start_time = scan_end_time = 0.0;
		zfs_reader.getScanTimeRange(scan_start_time,scan_end_time);

		exportLinFile(scan_start_time,scan_end_time,str_lin_path.data(),vecPosInfo);
	}

	//// 格式转换;
	//const size_t csize = strlen(strHlzPath) + 1;
	//wchar_t* wc = new wchar_t[csize];
	//mbstowcs(wc,strHlzPath,csize);

	// 将多字节转换宽字节;
	int nlen = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,strHlzPath,-1,NULL,0);
	if (nlen == 0)
	{
		return -1;
	}
	wchar_t* wc = new wchar_t[nlen];
	MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,strHlzPath,-1,wc,nlen);

	// 打开HLZ流文件;
	//CallbackWrapper* ptr_progress = new CallbackWrapper(wloadCallback);
	hdHlzStreamBuilder hlz_stream_builder;
	bool is_suc = hlz_stream_builder.open(wc,false,false,NULL);
	if (!is_suc)
	{
		return -1;
	}
	hlz_stream_builder.setSimpleLevel0(false,0.0025);

	// 申请内存记录;
	int per_size = 500000;
	double global_x,global_y,global_z;
	int per_write_count = 0;
	hdPointXYZIPRGBAD* ptr_write_data = new hdPointXYZIPRGBAD[per_size];
	memset(ptr_write_data,0,sizeof(hdPointXYZIPRGBAD) * 500000);

	//// 记录头文件偏移量
	//double gps_time_offset = 0.0;
	//bool bFirst = true;
	//double header_offset_x = 0.0;
	//double header_offset_y = 0.0;
	//double header_offset_z = 0.0;

	//// 一次申请内存
	//hdPointXYZIPRGBA* ptrLoopData = new hdPointXYZIPRGBA[50000];
	//memset(ptrLoopData,0,sizeof(hdPointXYZIPRGBA) * 50000);

	//// 时间存储信息文件内存申请
	//COMBINE_TIME_POINT_STRUCT* ptrTimeLoopData = NULL;
	//ptrTimeLoopData = new COMBINE_TIME_POINT_STRUCT[50000];
	//memset(ptrTimeLoopData,0,sizeof(COMBINE_TIME_POINT_STRUCT) * 50000);

	// 逐圈读取数据
	int nLoopCount = zfs_reader.GetScanLines();

	// 若使用控制点即进行POS纠偏处理，则考虑只需要提取POS对应时间点一定范围内数据即可;
	//std::vector<int> zfs_need_read_scanlines_index;
	COMBINE_TIME_DELAY_SET_STRUCT time_delay_info = m_project_setting->getTimeDelayInfo();
	COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT use_ctrl_param_set = m_project_setting->getUseCtrlRadiusExportInfo();
	if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
	{
		// 获取距离最近POS时间点
		std::vector<COMBINE_TIME_RANGE> vec_combine_time_range;
		calcuCtrlPosTime(m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius,vecPosInfo,vec_combine_time_range);

		zfs_reader.setTimeRange(vec_combine_time_range);
	}
	else
	{
		// 若使用时间进行过滤;
		COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_export_set = m_project_setting->getUseTimeRangeExportInfo();
		if (use_time_export_set.is_use_time_range_export)
		{
			m_vec_combine_time_range = use_time_export_set.vec_combine_time_range;
		}

		zfs_reader.setTimeRange(m_vec_combine_time_range);
	}

	//// 头文件信息设置;
	//int nPointCount = 0;
	//hls_header.m_number_of_col = nLoopCount;

	//// 打开写出文件失败返回;
	//if (!hls_writer.open(strHlsPath,&hls_header))
	//{
	//	return false;
	//}

	//// 打开写入时间信息文件;
	//hnCombineTimeWriter timeWriter;
	//if (need_time)
	//{
	//	bSuc = timeWriter.Open(strTimePath);
	//	if (!bSuc)
	//	{
	//		return false;
	//	}
	//}

	// 相关参数定义;
	int have_read_pts_in_loop = 0;
	std::vector<POINT_STRUCT_XYZIT_INFO> loop_points;
	loop_points.resize(5000);
	int have_write_loop_count = 0;
	zfs_reader.startRead();
	for (unsigned int nn = 0;nn < nLoopCount;nn++)
	{
		int n = nn;
		if (!m_is_running)
		{
			break;
		}

		// 逐圈获取数据
		have_read_pts_in_loop = 0;
		bSuc = zfs_reader.getLinePoints(n,loop_points,have_read_pts_in_loop);
		if (!bSuc || have_read_pts_in_loop == 0 || !m_is_running)
		{
			if ( loadCallback && nn % 500 == 0 )
			{
				loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
			}
			if (m_use_thread && nn % 500 == 0 )
			{
				setProgress(nn * 1.0 / nLoopCount,"融合解算...");
			}

			continue;
		}

		// 每个点进行坐标转换
		int loopWriteCount = 0;
		for (unsigned int m = 0;m < have_read_pts_in_loop;m++)
		{
			// 同海达保持一致，外部设置延时80毫秒即将当前点云时间减去80毫秒;
			POINT_STRUCT_XYZIT_INFO& point_info = loop_points[m];
			point_info.timeSecond -= time_delay_info.pcd_time_delay * 0.001;

			// 首先根据时间信息进行插值计算，获取该时刻POS位置姿态;
			POS_STRUCT_INFO curPos;
			int nearest_pos_index = -1;
			bSuc = linearInsertPos(point_info.timeSecond,vecPosInfo,curPos,nearest_pos_index);
			if (!bSuc)
			{
				continue;
			}

			// 矩阵运算，获取绝对坐标;
			double dx,dy,dz;
			dx = point_info.x;
			dy = point_info.y;
			dz = point_info.z;
			calcuCoord(curPos,dx,dy,dz);

			// 若使用控制点范围过滤;
			if (use_ctrl_param_set.is_use_ctrl_pts_radius_export)
			{
				// 判断点是否在控制点范围内;
				bool is_point_in = isPointInCtrlRadius(dx,dy,dz,m_vec_ctrl_points,use_ctrl_param_set.use_ctrl_pts_radius);
				if (!is_point_in)
				{
					continue;
				}
			}

			ptr_write_data[per_write_count].x = dx;
			ptr_write_data[per_write_count].y = dy;
			ptr_write_data[per_write_count].z = dz;
			ptr_write_data[per_write_count].intensity = point_info.intensity;
			ptr_write_data[per_write_count].r = 0;
			ptr_write_data[per_write_count].g = 0;
			ptr_write_data[per_write_count].b = 0;
			ptr_write_data[per_write_count].prop = 0;
			per_write_count++;

			if (per_write_count >= per_size)
			{
				hlz_stream_builder.writePts(ptr_write_data,per_write_count);
				per_write_count = 0;
			}
		}

		if ( loadCallback && nn % 500 == 0 )
		{
			loadCallback(nn * 1.0 / nLoopCount,"融合解算...");
		}

		if (m_use_thread && nn % 500 == 0)
		{
			setProgress(nn * 1.0 / nLoopCount,"融合解算...");
		}

	}

	// 最后剩余部分写入
	if (per_write_count > 0)
	{
		hlz_stream_builder.writePts(ptr_write_data,per_write_count);
		per_write_count = 0;
	}

	// 写入完成;
	hlz_stream_builder.flush();
	hlz_stream_builder.close();

	zfs_reader.CloseZfsFile();
	if (loadCallback)
	{
		loadCallback(1.0,"处理完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"处理完成");
	}

	// 内存释放
	if (ptr_write_data)
	{
		delete []ptr_write_data;
		ptr_write_data = NULL;
	}

	if (wc)
	{
		delete[] wc;
		wc = NULL;
	}

	if (!m_is_running)
	{
		return false;
	}
	return true;
}

int hnZfsCombine::calcuPosCorrect( const char* str_old_pos_path,double start_time,double end_time,const char* str_save_pos_path,const char* str_cpt_path )
{
	// 加载POS数据;
	std::vector<POS_STRUCT_INFO> vec_pos_info;
	loadPosData(str_old_pos_path,vec_pos_info);
	if (vec_pos_info.size() < 0)
	{
		return 0;
	}

	if (abs(start_time) < 0.0001 || abs(end_time) < 0.0001)
	{
		start_time = vec_pos_info[0].dGpsSecond;
		end_time = vec_pos_info[vec_pos_info.size()-1].dGpsSecond;
	}


	// 读取cpt文件;
	std::vector<POS_CPT_STRUCT_INFO> vec_cpt_info;
	loadCptFile(str_cpt_path,vec_cpt_info);

	// 控制点改正，从选择的时间段开始;
	std::vector<POS_STRUCT_INFO> vec_result_pos_info;
	vec_result_pos_info = vec_pos_info;

#pragma region 起始段
	// 处理第0段;
	POS_CPT_STRUCT_INFO correct_param_zero = vec_cpt_info[0];
	int cur_pos_zero_start_index = 0;
	int cur_nearest_zero_index = 0;
	POS_STRUCT_INFO cur_pos_zero_start;
	POS_STRUCT_INFO cur_pos_zero;
	bool bret = linearInsertPos(start_time,vec_pos_info,cur_pos_zero_start,cur_pos_zero_start_index);
	bret = linearInsertPos(correct_param_zero.gps_time,vec_pos_info,cur_pos_zero,cur_nearest_zero_index);
	if (bret)
	{
		// 处理第零段，即从起始POS到第一个控制点间的数据;
		double ddelt_height = correct_param_zero.height - cur_pos_zero.dHeight - 0.0;
		double ddelt_x = correct_param_zero.dx - cur_pos_zero.dEastCoord - 0.0;
		double ddelt_y = correct_param_zero.dy - cur_pos_zero.dNorthCoord - 0.0;

		//// 获取线性插值点;
		//int cur_nearest_index = 0;
		//int next_nearest_index = 0;
		//POS_STRUCT_INFO cur_pos;
		//POS_STRUCT_INFO next_pos;
		//bool bret = linearInsertPos(correct_param.gps_time,vec_pos_info,cur_pos,cur_nearest_index);
		//bool bbret = linearInsertPos(next_param.gps_time,vec_pos_info,next_pos,next_nearest_index);
		//if (!bret || !bbret)
		//{
		//	return;
		//}

		// 获取总距离值;
		double dist_cal_total = getPosDist(vec_pos_info,cur_pos_zero_start_index,cur_nearest_zero_index);

		// 将范围内POS按高差反向分配;
		int ncount = 0;
		double cur_dist_cal = 0.0;
		//*(vec_result_pos_info._Myfirst + 0) = vec_pos_info[0];
		for (int ii = cur_pos_zero_start_index+1;ii <= cur_nearest_zero_index;ii++)
		{
			// 反向分配计算dheight=posinfo.height + deltH1 + scale * (deltH2-deltH1);
			POS_STRUCT_INFO pos_info = vec_pos_info[ii];

			double update_x = 0.0;
			double update_y = 0.0;
			double update_height = 0.0;
			double temp_dist = getPosDist(vec_pos_info,ii-1,ii);
			cur_dist_cal += temp_dist;
			if (dist_cal_total > 0.0)
			{
				if (cur_dist_cal / dist_cal_total == 1.0)
				{
					int tes = 0;
				}

				// 不为零，进行分配处理;
				update_height = pos_info.dHeight + cur_dist_cal * ddelt_height / dist_cal_total;
				update_x = pos_info.dEastCoord + cur_dist_cal * ddelt_x / dist_cal_total;
				update_y = pos_info.dNorthCoord + cur_dist_cal * ddelt_y / dist_cal_total;

				pos_info.dEastCoord = update_x;
				pos_info.dNorthCoord = update_y;
				pos_info.dHeight = update_height;
			}

			*(vec_result_pos_info._Myfirst + ii) = pos_info;
			ncount++;

			// 进度显示;
			if (loadCallback && ncount % 20 == 0)
			{
				loadCallback(ncount*1.0/(cur_nearest_zero_index - cur_pos_zero_start_index),"处理数据...");
			}

			if (m_use_thread && ncount % 20 == 0)
			{
				setProgress(ncount*1.0/(cur_nearest_zero_index - cur_pos_zero_start_index),"处理数据...");
			}
		}
	}

#pragma endregion
	// POS高程改正,需要针对起始段和终止段进行处理;
	for (unsigned int n = 0;n < vec_cpt_info.size()-1;n++)
	{
		POS_CPT_STRUCT_INFO correct_param = vec_cpt_info[n];
		POS_CPT_STRUCT_INFO next_param = vec_cpt_info[n+1];

		// 获取线性插值点;
		int cur_nearest_index = 0;
		int next_nearest_index = 0;
		POS_STRUCT_INFO cur_pos;
		POS_STRUCT_INFO next_pos;
		bool bret = linearInsertPos(correct_param.gps_time,vec_pos_info,cur_pos,cur_nearest_index);
		bool bbret = linearInsertPos(next_param.gps_time,vec_pos_info,next_pos,next_nearest_index);
		if (!bret || !bbret)
		{
			continue;
		}

		double next_delta_h = next_param.height - next_pos.dHeight;
		double next_delta_x = next_param.dx - next_pos.dEastCoord;
		double next_delta_y = next_param.dy - next_pos.dNorthCoord;

		double cur_delta_h = correct_param.height - cur_pos.dHeight;
		double cur_delta_x = correct_param.dx - cur_pos.dEastCoord;
		double cur_delta_y = correct_param.dy - cur_pos.dNorthCoord;

		// 高差值
		double ddelt_height = next_delta_h - cur_delta_h;
		double ddelt_x = next_delta_x - cur_delta_x;
		double ddelt_y = next_delta_y - cur_delta_y;

		// 获取总距离值;
		double dist_cal_total = getPosDist(vec_pos_info,cur_nearest_index,next_nearest_index);

		// 将范围内POS按高差反向分配;
		int ncount = 0;
		double cur_dist_cal = 0.0;
		for (int ii = cur_nearest_index+1;ii <= next_nearest_index;ii++)
		{
			// 反向分配计算dheight=posinfo.height + deltH1 + scale * (deltH2-deltH1);
			POS_STRUCT_INFO pos_info = vec_pos_info[ii];
			double update_height = 0.0;
			double update_x = 0.0;
			double update_y = 0.0;
			double temp_dist = getPosDist(vec_pos_info,ii-1,ii);
			cur_dist_cal += temp_dist;
			if (dist_cal_total > 0.0)
			{
				if (cur_dist_cal / dist_cal_total == 1.0)
				{
					int tes = 0;
				}

				// 不为零，进行分配处理;
				update_height = pos_info.dHeight + cur_delta_h + cur_dist_cal * ddelt_height / dist_cal_total;
				update_x = pos_info.dEastCoord + cur_delta_x + cur_dist_cal * ddelt_x / dist_cal_total;
				update_y = pos_info.dNorthCoord + cur_delta_y + cur_dist_cal * ddelt_y / dist_cal_total;

				pos_info.dHeight = update_height;
				pos_info.dEastCoord = update_x;
				pos_info.dNorthCoord = update_y;
			}

			*(vec_result_pos_info._Myfirst + ii) = pos_info;

			// 进度显示;
			if (loadCallback && ncount % 20 == 0)
			{
				loadCallback(ncount*1.0/(next_nearest_index - cur_nearest_index),"处理数据...");
			}

			if (m_use_thread && ncount % 20 == 0)
			{
				setProgress(ncount*1.0/(next_nearest_index - cur_nearest_index),"处理数据...");
			}
		}
	}

#pragma region 终止段

	// 处理最后段;
	POS_CPT_STRUCT_INFO correct_param_last = vec_cpt_info[vec_cpt_info.size()-1];
	int cur_nearest_index_last = 0;
	int next_nearest_end_index = 0;
	POS_STRUCT_INFO cur_pos_last;
	POS_STRUCT_INFO next_pos_last;
	bret = linearInsertPos(correct_param_last.gps_time,vec_pos_info,cur_pos_last,cur_nearest_index_last);
	bret = linearInsertPos(end_time,vec_pos_info,next_pos_last,next_nearest_end_index);
	if (bret)
	{
		//// 处理第零段，即从起始POS到第一个控制点间的数据;
		//POS_CORRECT_PARAM correct_param = vec_cpt_info[vec_cpt_info.size()-1];
		//POS_CORRECT_PARAM next_param;
		//next_param.gps_time = vec_pos_info[vec_pos_info.size()-1].dGpsSecond;
		//next_param.delta_height = 0.0;

		double next_delta_h = 0.0;
		double next_delta_x = 0.0;
		double next_delta_y = 0.0;

		double cur_delta_h = correct_param_last.height - cur_pos_last.dHeight;
		double cur_delta_x = correct_param_last.dx - cur_pos_last.dEastCoord;
		double cur_delta_y = correct_param_last.dy - cur_pos_last.dNorthCoord;

		// 高差值
		double ddelt_height = next_delta_h - cur_delta_h;
		double ddelt_x = next_delta_x - cur_delta_x;
		double ddelt_y = next_delta_y - cur_delta_y;


		// 获取线性插值点;
		int cur_nearest_index = 0;
		int next_nearest_index = 0;
		POS_STRUCT_INFO cur_pos;
		POS_STRUCT_INFO next_pos;
		bool bret = linearInsertPos(correct_param_last.gps_time,vec_pos_info,cur_pos,cur_nearest_index);
		bool bbret = linearInsertPos(end_time,vec_pos_info,next_pos,next_nearest_index);
		if (!bret || !bbret)
		{
			return 0;
		}

		// 获取总距离值;
		double dist_cal_total = getPosDist(vec_pos_info,cur_nearest_index,next_nearest_index);

		// 将范围内POS按高差反向分配;
		int ncount = 0;
		double cur_dist_cal = 0.0;
		for (int ii = cur_nearest_index+1;ii <= next_nearest_index;ii++)
		{
			// 反向分配计算dheight=posinfo.height + deltH1 + scale * (deltH2-deltH1);
			POS_STRUCT_INFO pos_info = vec_pos_info[ii];
			double update_height = 0.0;
			double update_x = 0.0;
			double update_y = 0.0;
			double dist_temp = getPosDist(vec_pos_info,ii-1,ii);
			cur_dist_cal += dist_temp;
			if (dist_cal_total > 0.0)
			{
				if (cur_dist_cal / dist_cal_total == 1.0)
				{
					int tes = 0;
				}

				// 不为零，进行分配处理;
				update_height = pos_info.dHeight + cur_delta_h + cur_dist_cal * ddelt_height / dist_cal_total;
				update_x = pos_info.dEastCoord + cur_delta_x + cur_dist_cal * ddelt_x / dist_cal_total;
				update_y = pos_info.dNorthCoord + cur_delta_y + cur_dist_cal * ddelt_y / dist_cal_total;

				pos_info.dHeight = update_height;
				pos_info.dEastCoord = update_x;
				pos_info.dNorthCoord = update_y;
			}

			*(vec_result_pos_info._Myfirst + ii) = pos_info;

			// 进度显示;
			if (loadCallback && ncount % 20 == 0)
			{
				loadCallback(ncount*1.0/(next_nearest_index - cur_nearest_index),"处理数据...");
			}

			if (m_use_thread && ncount % 20 == 0)
			{
				setProgress(ncount*1.0/(next_nearest_index - cur_nearest_index),"处理数据...");
			}
		}
	}

#pragma endregion

	if (loadCallback )
	{
		loadCallback(0.0,"处理完成");
	}

	if (m_use_thread)
	{
		setProgress(0.0,"处理完成");
	}

	// 写入存储;
	writePosData(str_save_pos_path,vec_result_pos_info);
	return 1;
}

bool sortByTime(POS_CPT_STRUCT_INFO p1,POS_CPT_STRUCT_INFO p2)
{
	if (p1.gps_time < p2.gps_time)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void hnZfsCombine::loadCptFile( const char* str_cpt_file_path,std::vector<POS_CPT_STRUCT_INFO>& vec_cpt_info )
{
	FILE* ptr_file = fopen(str_cpt_file_path,"rt");
	if (!ptr_file)
	{
		return;
	}

	// 去掉三行文件头;
	char str_line[1024];
	fgets(str_line,1024,ptr_file);
	fgets(str_line,1024,ptr_file);
	fgets(str_line,1024,ptr_file);

	// 文件读取解析;
	while (!feof(ptr_file))
	{
		memset(str_line,0,1024);
		fgets(str_line,1024,ptr_file);

		POS_CPT_STRUCT_INFO info;
		bool is_suc = info.serialize(str_line);
		if (is_suc)
		{
			vec_cpt_info.push_back(info);
		}
	}
	fclose(ptr_file);
	std::sort(vec_cpt_info.begin(),vec_cpt_info.end(),sortByTime);

	// 坐标转换:
	hdSpatialReferenceH source_cs = m_ptr_coord_trans_system->getSourceCS();
	if (!source_cs)
	{
		return;
	}
	hdProjectedCoordinateSystem* ptr_source_coord = static_cast<hdProjectedCoordinateSystem*>(source_cs);
	if (!ptr_source_coord)
	{
		return;
	}

	// 数据转换对象;
	IHdPJTranslator* ptr_pj_translator = NULL;	
	CreateIHdPJTranslator(&ptr_pj_translator);

	ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	// 遍历处理；
	COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//for (unsigned int n = 0;n < vecInfo.size();n++)
	//{
	//	POS_STRUCT_INFO& pos_info = vecInfo[n];
	//	double tmp_height = 0.0;
	//	double tmp_north = 0.0;
	//	double tmp_east = 0.0;

	//	// 若选择为非WGS84或者CGCS2000坐标系,则需要进行二次转换;
	//	if (combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_WGS84 
	//		&& combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_China2000)
	//	{
	//		ptr_pj_translator->Translator(pos_info.dLongitude,pos_info.dLatitude,pos_info.dHeight,&pos_info.dEastCoord,&pos_info.dNorthCoord,&tmp_height);
	//	}
	//	else
	//	{
	//		// 经纬度转投影坐标
	//		projected_coord_system.GaussProject_BL2NE(pos_info.dLatitude,pos_info.dLongitude,pos_info.dNorthCoord,pos_info.dEastCoord);
	//	}
	//}
	//DestroyIHdPJTranslator(ptr_pj_translator);
	//return;

	// 数据转换对象;
	//IHdPJTranslator* ptr_pj_translator = NULL;	
	//CreateIHdPJTranslator(&ptr_pj_translator);

	//ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	//ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	// 遍历处理；
	for (unsigned int n = 0;n < vec_cpt_info.size();n++)
	{
		POS_CPT_STRUCT_INFO& info = vec_cpt_info[n];
		double tmp_height = 0.0;
		double tmp_north = 0.0;
		double tmp_east = 0.0;

		// 若选择为非WGS84或者CGCS2000坐标系,则需要进行二次转换;
		if (combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_WGS84 
			&& combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_China2000)
		{
			ptr_pj_translator->Translator(info.longtitude,info.latitude,info.height,&info.dx,&info.dy,&tmp_height);
		}
		else
		{
			//// 经纬度转投影坐标
			//ptr_source_coord->GaussProject_BL2NE(info.latitude,info.longtitude,info.dy,info.dx);
			// 未设置椭球体，目前仅WGS84椭球体采用该计算;
			hdProjectedCoordinateSystem projected_coord_system;
			//projected_coord_system.m_geog_coord_sys.m_datum_type = ;
			//projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
			projected_coord_system.setCentralMeridian(ptr_source_coord->getCentralMeridian());
			projected_coord_system.setFalseEasting(combine_coord_project_info.east_offset);
			projected_coord_system.setFalseNorthing(0.0);
			projected_coord_system.m_mean_latitude = combine_coord_project_info.average_latitude;
			projected_coord_system.m_proj_height = combine_coord_project_info.project_height;

			projected_coord_system.GaussProject_BL2NE(info.latitude,info.longtitude,info.dy,info.dx);
		}


		//// 33.00694657 91.65359815
		//double tmp_long = 91.65359815;
		//double tmp_lat = 33.00694657;
		//double tmp_h1 = 0.0;
		//double tmp_res_x = 0.0;
		//double tmp_res_y = 0.0;
		//ptr_pj_translator->Translator(tmp_long,tmp_lat,tmp_h1,&tmp_res_x,&tmp_res_y,&tmp_h1);
		//double temp = 0.0;

		//// 设置投影坐标转换系统；
		//hdProjectedCoordinateSystem projected_coord_system("gauss_wgs84",92.5);

		//// 经纬度转投影坐标
		//double t2_long = 91.65359815;
		//double t2_lat = 33.00694657;
		//double t2_n = 0.0;
		//double t2_e = 0.0;
		//projected_coord_system.GaussProject_BL2NE(t2_lat,t2_long,t2_n,t2_e);

		//double aa = tmp_res_x - t2_e;
		//double bb = tmp_res_y - t2_n;
		//int dd = 0;

		//fprintf_s(ptr_file1,"%d	%.6lf	%.6lf	%.6lf\n",
		//	info.index,info.dx,info.dy,info.height);
	}
	//fclose(ptr_file1);

	DestroyIHdPJTranslator(ptr_pj_translator);
}

double hnZfsCombine::getPosDist( std::vector<POS_STRUCT_INFO>& vecInfo,int start_index,int end_index )
{
	// 条件判断;
	if (start_index < 0 || end_index >= vecInfo.size())
	{
		return 0.0;
	}

	double dist = 0.0;
	for (int n = start_index;n < end_index;n++)
	{
		POS_STRUCT_INFO cur_info = vecInfo[n];
		POS_STRUCT_INFO next_info = vecInfo[n+1];
		double temp_dist = sqrt(pow(next_info.dEastCoord - cur_info.dEastCoord,2.0) + 
			pow(next_info.dNorthCoord - cur_info.dNorthCoord,2.0));
		dist += temp_dist;
	}

	return dist;
}

void hnZfsCombine::writePosData( const char* strPosSavePath,std::vector<POS_STRUCT_INFO>& vecInfo )
{
	char* str = new char[1024];
	memset(str,0,1024);
	int tempCount = 0;
	FILE* ptrFile = fopen(strPosSavePath,"wt");

	// 数据转换对象;
	IHdPJTranslator* ptr_pj_translator = NULL;	
	CreateIHdPJTranslator(&ptr_pj_translator);

	ptr_pj_translator->SetSrcSpatialRef(&m_src_param);
	ptr_pj_translator->SetDstSpatialRef(&m_dst_param);

	//// 坐标转换:
	//hdSpatialReferenceH source_cs = m_ptr_coord_trans_system->getSourceCS();
	//if (!source_cs)
	//{
	//	return;
	//}
	//hdProjectedCoordinateSystem* ptr_source_coord = static_cast<hdProjectedCoordinateSystem*>(source_cs);
	//if (!ptr_source_coord)
	//{
	//	return;
	//}

	//// 暂时根据椭球体类型采用不同参考系;
	//COMBINE_COORD_PROJECT_SET_STRUCT combine_coord_project_info = m_project_setting->getSphereProjectInfo();
	//for (unsigned int n = 0;n < vecInfo.size();n++)
	//{
	//	// 查看第一个隧道的数据
	//	POS_STRUCT_INFO& info = *(vecInfo._Myfirst + n);

	//	double tmp_height = 0.0;
	//	double tmp_lat = 0.0;
	//	double tmp_long = 0.0;

	//	// 若选择为非WGS84或者CGCS2000坐标系,则需要进行二次转换;
	//	if (combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_WGS84 
	//		&& combine_coord_project_info.sphere_target_type != E_COMBINE_PROJECT_SPHERE_China2000)
	//	{
	//		ptr_pj_translator->TranslatorReverse(info.dEastCoord,info.dNorthCoord,info.dHeight,&tmp_long,&tmp_lat,&tmp_height);
	//	}
	//	else
	//	{
	//		//// 经纬度转投影坐标
	//		//tmp_long = info.dEastCoord;
	//		//tmp_lat = info.dNorthCoord;
	//		//ptr_source_coord->GaussProject_NE2BL(tmp_lat,tmp_long);

	//		// 经纬度转投影坐标
	//		double tmp_north = info.dNorthCoord;
	//		double tmp_east = info.dEastCoord;

	//		// 未设置椭球体，目前仅WGS84椭球体采用该计算;
	//		hdProjectedCoordinateSystem projected_coord_system;
	//		//projected_coord_system.m_geog_coord_sys.m_datum_type = ;
	//		//projected_coord_system.setGeogCoordSystem(ptr_source_coord->getGeogCoordSystem());
	//		projected_coord_system.setCentralMeridian(combine_coord_project_info.centre_longtitude);
	//		projected_coord_system.setFalseEasting(combine_coord_project_info.east_offset);
	//		projected_coord_system.setFalseNorthing(0.0);
	//		projected_coord_system.m_mean_latitude = combine_coord_project_info.average_latitude;
	//		projected_coord_system.m_proj_height = combine_coord_project_info.project_height;

	//		projected_coord_system.GaussProject_NE2BL(tmp_north,tmp_east);
	//		tmp_long = tmp_east;
	//		tmp_lat = tmp_north;
	//	}

	//	info.dLongitude = tmp_long;
	//	info.dLatitude = tmp_lat;

	//	// 只记录一段
	//	memset(str,0,1024);
	//	info.reserialize(&str,1024);
	//	fprintf_s(ptrFile,"%s",str);

	//	if (loadCallback && n % 5000 == 0)
	//	{
	//		loadCallback(n*1.0/vecInfo.size(),"存储数据...");
	//	}

	//	if (m_use_thread && n % 5000 == 0)
	//	{
	//		setProgress(n*1.0/vecInfo.size(),"存储数据...");
	//	}
	//}

	{
		int ncount = vecInfo.size();
		double* degree_lat = new double[ncount];
		double* degree_long = new double[ncount];
		double* east = new double[ncount];
		double* north = new double[ncount];

		for (int i = 0;i < vecInfo.size();i++)
		{
			POS_STRUCT_INFO& pos_info = vecInfo[i];

			// 经纬度转投影坐标;
			east[i] = pos_info.dEastCoord;
			north[i] = pos_info.dNorthCoord;

			if (loadCallback && i % 5000 == 0)
			{
				loadCallback(i * 1.0 / vecInfo.size(),"POS转换...");
			}

			if (m_use_thread && i % 5000 == 0)
			{
				setProgress(i * 1.0 / vecInfo.size(),"POS转换...");
			}
		}


		ptr_pj_translator->TransLators_NEToBL(ncount,north,east,degree_lat,degree_long);

		for (int i = 0;i < vecInfo.size();i++)
		{
			POS_STRUCT_INFO& pos_info = vecInfo[i];

			// 经纬度转投影坐标;
			pos_info.dLatitude = degree_lat[i];
			pos_info.dLongitude = degree_long[i];

			if (loadCallback && i % 5000 == 0)
			{
				loadCallback(i * 1.0 / vecInfo.size(),"POS记录...");
			}

			if (m_use_thread && i % 5000 == 0)
			{
				setProgress(i * 1.0 / vecInfo.size(),"POS记录...");
			}
		}

		delete[] degree_lat;
		degree_lat = NULL;
		delete[] degree_long;
		degree_long = NULL;

		delete[] north;
		north = NULL;
		delete[] east;
		east = NULL;

		for (unsigned int n = 0;n < vecInfo.size();n++)
		{
			POS_STRUCT_INFO& info = vecInfo[n];

			// 只记录一段
			memset(str,0,1024);
			info.reserialize(&str,1024);
			fprintf_s(ptrFile,"%s",str);

			if (loadCallback && n % 5000 == 0)
			{
				loadCallback(n*1.0/vecInfo.size(),"存储数据...");
			}

			if (m_use_thread && n % 5000 == 0)
			{
				setProgress(n*1.0/vecInfo.size(),"存储数据...");
			}
		}
	}

	fclose(ptrFile);

	delete[] str;
	str = NULL;
	DestroyIHdPJTranslator(ptr_pj_translator);

	if (loadCallback)
	{
		loadCallback(1.0,"处理完成");
	}

	if (m_use_thread)
	{
		setProgress(1.0,"处理完成");
	}
}

void hnZfsCombine::stopWork()
{
	m_is_running = false;
}

bool hnZfsCombine::getCtrlTimeInTime( double gps_time,std::vector<COMBINE_TIME_RANGE>& vec_time_in )
{
	//for (unsigned int n = 0;n < vec_combine_time_range.size();n++)
	//{
	//	double gps_time = vec_combine_time_range[n].start_gps_second;
	//	bool is_time_in = false;
	//	for (unsigned int m = 0;m < vec_time_in.size();m++)
	//	{
	//		COMBINE_TIME_RANGE time_range = vec_time_in[m];
	//		double upper_time = time_range.end_gps_second;
	//		double lower_time = time_range.start_gps_second;
	//		if (gps_time >= lower_time && gps_time <= upper_time)
	//		{
	//			is_time_in = true;
	//			break;
	//		}
	//	}

	//	// 符合范围内则记录;
	//	if (is_time_in)
	//	{
	//		vec_time_result.push_back(vec_combine_time_range[n]);
	//	}
	//	
	//}

	bool is_time_in = false;
	for (unsigned int m = 0;m < vec_time_in.size();m++)
	{
		COMBINE_TIME_RANGE time_range = vec_time_in[m];
		double upper_time = time_range.end_gps_second;
		double lower_time = time_range.start_gps_second;
		if (gps_time >= lower_time && gps_time <= upper_time)
		{
			is_time_in = true;
			break;
		}
	}

	return is_time_in;
}
