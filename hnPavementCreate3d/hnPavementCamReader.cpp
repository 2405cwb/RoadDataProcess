#include "hnPavementCamReader.h"
#include <QDir>

#include <QFileInfo>
#include <QFile>
#include <io.h>
#include <stdlib.h>
#include "snappy-c.h"
#include <filesystem>

#define PAVEMENT_CAM_SYN_LEN 176
#define PAVEMENT_CAM_SYN_HEADER_LEN 5

hnPavementCamReader::hnPavementCamReader(void)
{
	m_ptrCamFile = NULL;
	m_ptrDatFile = NULL;
	m_iCurNo = 0;
	m_strCurDatDirPath = "";
	m_strCamFilePath = "";

	m_ptrRowTimeStamp = NULL;
	m_ptrRowHeight = NULL;
	m_ptrRowIntensity = NULL;

	m_vecCamSynInfo.size();
	m_nTestCount = 0;
	m_bNeedRead = false;
	m_nCurCamIndex = 0;
	m_nCurReadIndex = 0;
	m_cachedSubFrameDatIndex = -1;
	m_subFrameCacheEnabled = false;
	m_ptrRowHeight = new unsigned short*[100];
	m_ptrRowIntensity = new unsigned char*[100];
	m_ptrRowTimeStamp = new unsigned char*[100];
	for (int n = 0;n < 100;n++)
	{
		m_ptrRowHeight[n] = new unsigned short[2560 * 40];
		m_ptrRowIntensity[n] = new unsigned char[2560 * 40];
		m_ptrRowTimeStamp[n] = new unsigned char[16*40];
	}

	m_MatrxX = new double*[960];
	m_MatrxZ = new double*[960];
	for (int i = 0; i < 960; ++i)
	{
		m_MatrxX[i] = new double[2560];
		m_MatrxZ[i] = new double[2560];
		memset(m_MatrxX[i], 0.0f, 2560 * sizeof(double));
		memset(m_MatrxZ[i], 0.0f, 2560 * sizeof(double));
	}

	//m_strOrientData = new char[2560 * 40 * 100 * 2];
	//m_strOrientDataNew = new char[2560 * 40 * 100 * 2];

	m_testData = 0.0;
	m_bJumpRead = false;
	m_jumpLines = 0;
}


hnPavementCamReader::~hnPavementCamReader(void)
{
	//delete[] m_strOrientData;
	//m_strOrientData = NULL;
	//delete[] m_strOrientDataNew;
	//m_strOrientDataNew = NULL;


	for (int n = 0;n < 100;n++)
	{
		unsigned short* ptrHeight = m_ptrRowHeight[n];
		delete[] ptrHeight;
		ptrHeight = NULL;
		unsigned char* ptrIntensity = m_ptrRowIntensity[n];
		delete[] ptrIntensity;
		ptrIntensity = NULL;

		unsigned char* ptrTimeStamp = m_ptrRowTimeStamp[n];
		delete[] ptrTimeStamp;
		ptrTimeStamp = NULL;
	}
	delete[] m_ptrRowHeight;
	delete[] m_ptrRowIntensity;
	m_ptrRowHeight = NULL;
	m_ptrRowIntensity = NULL;

	for (int n = 0;n < 960;n++)
	{
		double* ptrMatrixX = m_MatrxX[n];
		double* ptrMatrixZ = m_MatrxZ[n];
		delete[] ptrMatrixX;
		ptrMatrixX = NULL;

		delete[] ptrMatrixZ;
		ptrMatrixZ = NULL;
	}
	delete[] m_MatrxX;
	delete[] m_MatrxZ;
	m_MatrxX = NULL;
	m_MatrxZ = NULL;
}

bool hnPavementCamReader::Open( const char* path )
{
	m_cachedSubFrameDatIndex = -1;
	m_strCamFilePath = path;

	// 根据当前cam文件名，顺序查找第一个dat所在文件夹路径;
	std::string strDatFilePath = m_strCamFilePath;
	int nPos = strDatFilePath.find_last_of('/');
	std::string strFirstDatDirPath = strDatFilePath.substr(0,nPos+1); // 截取前段文件名;
	strFirstDatDirPath += "Image_0000";
	m_strCurDatDirPath = strFirstDatDirPath;
	m_iCurDirNo = 0;

	// 检查该文件夹是否存在;
	if (_access(m_strCurDatDirPath.data(),0) != 0)
	{
		return false;
	}

	// 查找第一个dat文件;
	m_iCurNo = 0;
	std::string strFirstDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
	if (_access(strFirstDatFilePath.data(),0) != 0)
	{
		return false;
	}

	// 打开文件;
	m_ptrCamFile = fopen(m_strCamFilePath.data(),"rb");
	m_ptrDatFile = fopen(strFirstDatFilePath.data(),"rb");

	// 同步帧文件应先找到第一个同步帧的起始位置;
	int nIdx = 0;
	int nCurIndex = 0;
	unsigned char strLine[10240];
	int nret = fread(strLine,1,sizeof(unsigned char)*10240,m_ptrCamFile);
	if (nret > 0)
	{
		for (int n = 0; n < nret-4;n++)
		{
			if (strLine[n] == 0x0A && strLine[n+1] == 0xFF && strLine[n+2] == 0xFF && strLine[n+3] == 0xFF) //  && strLine[n+3] == 0xFF 
			{
				nCurIndex = n;
				break;

				//nIdx++;
				//if (nIdx >= 43)
				//{
				//	break;
				//}
			}
		}
	}

	fseek(m_ptrCamFile,0,SEEK_END);
	long camLength = ftell(m_ptrCamFile);
	fseek(m_ptrCamFile,nCurIndex,SEEK_SET);

	//fseek(m_ptrDatFile,0,SEEK_SET);
	//fseek(m_ptrDatFile,0,SEEK_END);
	//long datLength = ftell(m_ptrDatFile);






	//fseek(m_ptrDatFile,0,SEEK_SET);

	m_bNeedRead = true;
	m_nCurCamIndex = 0;
	m_nCurReadIndex = 0;
	m_nTestCount = 0;

	// 读取全部同步信息到内存，读取完成后内部关闭cam文件;
	m_vecCamSynInfo.clear();
	synCamData(m_vecCamSynInfo);

	std::string strCheckTime = path;
	strCheckTime += ".report";

	 

	FILE* ptrFiler = fopen(strCheckTime.data(), "r");
	if (ptrFiler)
	{
		fclose(ptrFiler);
	}
	else
	{
		FILE* ptrFile = fopen(strCheckTime.data(), "wt");
		for (unsigned int n = 0; n < m_vecCamSynInfo.size(); n++)
		{
			PAVEMENT_CAM_SYN_INFO info = m_vecCamSynInfo[n];
			fprintf(ptrFile, "%d,%d,%d,%.3lf\n", n, info.curIndex, info.nGpsWeek, info.nGpsSecond + info.nMsecond / 1000.0 + info.nUsecond / 1000000.0);
		}
		fclose(ptrFile);
	}
	// 读取相机标定文件;
	readCamMatrix();

	return true;
}

bool hnPavementCamReader::synCamData(std::vector<PAVEMENT_CAM_SYN_INFO>& vecCamSynInfo)
{
	if (!m_ptrCamFile)
	{
		return false; 
	}

	vecCamSynInfo.clear();
	vecCamSynInfo.resize(5000);
	int iIndex = 0;
	PAVEMENT_CAM_SYN_INFO lastCamSynInfo;
	PAVEMENT_CAM_SYN_INFO camSynInfo;
	unsigned char frame_buff[PAVEMENT_CAM_SYN_LEN];

	// 将全部数据读入内存;
	long curFilePos = ftell(m_ptrCamFile);
	fseek(m_ptrCamFile,0,SEEK_END);
	long camLength = ftell(m_ptrCamFile);
	fseek(m_ptrCamFile,0,SEEK_SET);
	fseek(m_ptrCamFile,curFilePos,SEEK_SET);
	unsigned char* strLine = new unsigned char[camLength+1];
	int nreadCount = fread(strLine, 1, camLength, m_ptrCamFile);

	// 分段进行解析;
	for (int n = 0; n < camLength-4;n += 1)
	{
		// 查找帧头;
		if ( strLine[n] == 0x0A && strLine[n+1] == 0xFF && strLine[n+2] == 0xFF && strLine[n+3] == 0xFF) //  && strLine[n+3] == 0xFF 
		{
			// 确定当前位置;
			int nNextFrameIndex = 0;

			// 查找下一个帧头，比较长度;
			for (int m = n+4;m < camLength - 4;m++)
			{
				// 确定下一帧;
				if ( strLine[m] == 0x0A && strLine[m+1] == 0xFF && strLine[m+2] == 0xFF && strLine[m+3] == 0xFF)
				{
					// 跳出当前循环;
					nNextFrameIndex = m;
					break;
				}
			}

			// 未找到下一帧数据，则指定为到达文件末尾;
			if (nNextFrameIndex <= 0)
			{
				nNextFrameIndex = camLength - 1;
			}

			// 确定帧长度;
			int nFrameLength = nNextFrameIndex - n;

			// 比较帧长度，若当前帧长度不是标准长度，则认为是无效帧（数据丢失帧等）;
			if (nFrameLength != PAVEMENT_CAM_SYN_LEN && nFrameLength != (PAVEMENT_CAM_SYN_LEN - 1))
			{
				// 跳转至下一帧有效数据位置处;
				n = n + nFrameLength - 1;
				continue;
			}

			// 到达文件末尾，直接跳出循环解析;
			if (n + nFrameLength > camLength)
			{
				break;
			}

			// 为有效帧，则拷贝数据至指定内存块;
			memcpy(frame_buff,strLine+n,nFrameLength);
			n = n + nFrameLength - 1;

			// 帧序号;
			camSynInfo.curIndex = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN - 1]);

			camSynInfo.nGpsWeek = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 0] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 1];
			camSynInfo.nGpsSecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 2] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 3] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 4];
			camSynInfo.nMsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 5] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 6];
			camSynInfo.nUsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 7] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 8];
			camSynInfo.nDmiValue = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 9] << 24) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 10] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 11] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 12];
			camSynInfo.vecRowUsecond.resize(39);
			int timediff = 0;
			for (int k = 0; k < 39; ++k)
			{
				timediff += (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 13 + k * 4] << 24) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 14 + k * 4] << 16)
					| (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 15 + k * 4] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 16 + k * 4];
				camSynInfo.vecRowUsecond[k] = timediff;
			}

			// 比较当前帧与上一帧的变化量;
			if (lastCamSynInfo.curIndex > 0)
			{
				// 确定是否存在丢失情况，予以插值处理;
				if (abs(camSynInfo.curIndex - lastCamSynInfo.curIndex) > 1 && abs(camSynInfo.curIndex - lastCamSynInfo.curIndex) < 255)
				{
					// 计算时间偏差量;
					double curGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;
					double lastGpsTime = lastCamSynInfo.nGpsSecond + lastCamSynInfo.nMsecond / 1000.0 + lastCamSynInfo.nUsecond / 1000000.0;
					double deltGpsTime = curGpsTime - lastGpsTime;

					// 根据编码器记录信息进行插值?;
					int nInsertValue = abs(camSynInfo.curIndex - lastCamSynInfo.curIndex) - 1;
					int timeStep = deltGpsTime * 1000000/ (nInsertValue + 1);
					int dmiStep = (camSynInfo.nDmiValue - lastCamSynInfo.nDmiValue) / (nInsertValue + 1);
					for (int iIndex = 0;iIndex < nInsertValue;iIndex++)
					{
						PAVEMENT_CAM_SYN_INFO curCam;
						curCam.curIndex = lastCamSynInfo.curIndex + iIndex + 1;
						if (curCam.curIndex > 255)
						{
							curCam.curIndex -= 255;
						}
						curCam.nGpsWeek = lastCamSynInfo.nGpsWeek;
						curCam.nGpsSecond = lastCamSynInfo.nGpsSecond;
						curCam.nDmiValue = lastCamSynInfo.nDmiValue + dmiStep * (iIndex + 1);
						curCam.nMsecond = lastCamSynInfo.nMsecond;
						curCam.nUsecond = lastCamSynInfo.nUsecond + timeStep * (iIndex + 1);

						curCam.vecRowUsecond.resize(39);
						int timediff = 0;
						for (int k = 0; k < 39; ++k)
						{
							timediff += (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 13 + k * 4] << 24) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 14 + k * 4] << 16)
								| (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 15 + k * 4] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 16 + k * 4];
							curCam.vecRowUsecond[k] = timediff;
						}

						// 中间插入值;
						vecCamSynInfo[iIndex] = curCam;
						iIndex++;
						if (iIndex >= vecCamSynInfo.size())
						{
							vecCamSynInfo.resize(vecCamSynInfo.size() + 5000);
						}
					}
				}
			}

			// 记录上一帧信息;
			lastCamSynInfo = camSynInfo;
			vecCamSynInfo[iIndex] = camSynInfo;
			iIndex++;
			if (iIndex >= vecCamSynInfo.size())
			{
				vecCamSynInfo.resize(vecCamSynInfo.size() + 5000);
			}
		}
	}

	vecCamSynInfo.resize(iIndex);

	if (m_ptrCamFile)
	{
		fclose(m_ptrCamFile);
		m_ptrCamFile = NULL;
	}

	delete[] strLine;
	strLine = NULL;

	return true;



	//// 统计时间范围，只需要读取cam文件即可;
	//vecCamSynInfo.clear();
	//vecCamSynInfo.resize(5000);
	//int iIndex = 0;
	//PAVEMENT_CAM_SYN_INFO camSynInfo;
	//unsigned char frame_buff[PAVEMENT_CAM_SYN_LEN];
	//while (!feof(m_ptrCamFile))
	//{
	//	int nreadCount = fread(frame_buff, 1, PAVEMENT_CAM_SYN_LEN, m_ptrCamFile);
	//	if (nreadCount < PAVEMENT_CAM_SYN_LEN)
	//	{
	//		// 认为读到文件尾，读取完成;
	//		continue;
	//	}

	//	// 解析读取到的同步帧数据至camSynInfo对象;
	//	if (frame_buff[0] != 0xFF || frame_buff[1] != 0xFF || frame_buff[2] != 0xFF) // || frame_buff[3] != 0xFF 
	//	{
	//		int test = 0;
	//	}
	//	camSynInfo.nGpsWeek = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 0] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 1];
	//	camSynInfo.nGpsSecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 2] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 3] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 4];
	//	camSynInfo.nMsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 5] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 6];
	//	camSynInfo.nUsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 7] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 8];
	//	camSynInfo.nDmiValue = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 9] << 24) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 10] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 11] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 12];
	//	camSynInfo.vecRowUsecond.resize(39);
	//	int timediff = 0;
	//	for (int k = 0; k < 39; ++k)
	//	{
	//		timediff += (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 13 + k * 4] << 24) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 14 + k * 4] << 16)
	//			| (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 15 + k * 4] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN + 16 + k * 4];
	//		camSynInfo.vecRowUsecond[k] = timediff;
	//	}

	//	vecCamSynInfo[iIndex] = camSynInfo;
	//	iIndex++;
	//	if (iIndex >= vecCamSynInfo.size())
	//	{
	//		vecCamSynInfo.resize(vecCamSynInfo.size() + 5000);
	//	}
	//}
	//vecCamSynInfo.resize(iIndex);

	//if (m_ptrCamFile)
	//{
	//	fclose(m_ptrCamFile);
	//	m_ptrCamFile = NULL;
	//}
	//return true;
}

std::string hnPavementCamReader::getDatPathByNum(const char* strCurDatDir, int iNo)
{
	std::string dir_path = strCurDatDir;
	dir_path += "/";
	int nPos = 0;
	QString strResultFile = "";

	// 判断文件夹存在;
	QString strPathDir = QString::fromLocal8Bit(dir_path.data());
	QDir dir(strPathDir);
	if (!dir.exists())
	{
		return strResultFile.toLocal8Bit().data();
	}

	// 需要检索的Dat编号;
	QString strCpExtName = "";
	strCpExtName = strCpExtName.sprintf("_%03d.dat",iNo);
	strCpExtName = strCpExtName.toLower();

	// 设置过滤条件，文件后缀过滤，按时间排序;
	QStringList filters;
	filters << QString("*.dat") << QString("*.DAT");
	//filters << strCpExtName;
	dir.setFilter(QDir::Files);
	dir.setSorting(QDir::Time);
	dir.setNameFilters(filters);

	// 遍历列表;
	//QFileInfoList file_list = dir.entryInfoList(dir.filter());
	QStringList files = dir.entryList();
	int i = 0;
	bool is_file = false;

	// 遍历;
	while (i < files.size())
	{
		//QFileInfo file_info = file_list.at(i);
		QString strFileName = files.at(i);
		//if (file_info.fileName() == "." || file_info.fileName() == ".." )
		//{
		//	// 系统文件夹过滤;
		//	i++;
		//	continue;
		//}

		// 若为文件;
		//is_file = file_info.isFile();
		QString strFileInfo = strPathDir + strFileName;
		QFileInfo fileInfo(strFileInfo);
		is_file = fileInfo.isFile();
		if (is_file)
		{
			QString strFileExist = fileInfo.filePath();
			int fileLength = strFileExist.length();

			// 检查文件名称;
			QString strExtDat = strFileExist.right(8);
			strExtDat = strExtDat.toLower();

			// 判断名称;
			if ( strExtDat.compare(strCpExtName) == 0 )
			{
				strResultFile = fileInfo.filePath();
				break;
			}
			else
			{
				// 系统文件夹过滤;
				i++;
				continue;
			}

		}

		i++;
	}// while (i < file_list.size())

	files.clear();

	return strResultFile.toLocal8Bit().data();
}

bool hnPavementCamReader::Close()
{
	// 关闭当前文件;
	if (m_ptrCamFile)
	{
		fclose(m_ptrCamFile);
		m_ptrCamFile = NULL;
	}
	if (m_ptrDatFile)
	{
		fclose(m_ptrDatFile);
		m_ptrDatFile = NULL;
	}

	return true;
}

void hnPavementCamReader::startRead()
{
	// 关闭当前文件;
	if (m_ptrCamFile)
	{
		fclose(m_ptrCamFile);
		m_ptrCamFile = NULL;
	}
	if (m_ptrDatFile)
	{
		fclose(m_ptrDatFile);
		m_ptrDatFile = NULL;
	}

	// 重新打开文件;
	Open(m_strCamFilePath.data());
}

// 实际实现为一次获取40行数据;
bool hnPavementCamReader::getLinePoints( int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	bool bRet = false;
	if (m_bNeedRead)
	{
		// 增加时间判断，若当前帧不在时间过滤范围内，则跳过;
		PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[m_nCurCamIndex];
		if (m_nCurCamIndex > 0 && m_nCurCamIndex < (m_vecCamSynInfo.size() - 100)) // 添加过滤条件;
		{
			PAVEMENT_CAM_SYN_INFO& camSynInfoNext = m_vecCamSynInfo[m_nCurCamIndex + 100];

			// 时间信息判断;
			double tempGpsTime = 0.0;
			tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

			double tempGpsTimeNext = 0.0;
			tempGpsTimeNext = camSynInfoNext.nGpsSecond + camSynInfoNext.nMsecond / 1000.0 + camSynInfoNext.nUsecond / 1000000.0;
			bool isPointIn = isTimeInVector(tempGpsTime,m_vec_combine_time_range);
			bool isNextIn = isTimeInVector(tempGpsTimeNext,m_vec_combine_time_range);
			bool isPtOrIn = isTimeOrInVector(tempGpsTime, tempGpsTimeNext,m_vec_combine_time_range);

			if (m_nCurCamIndex >= 9300 && m_nCurCamIndex <= 9400)
			{
				int test = 0;
			}

			// 若两个点都不在过滤时间范围内,且该段不包含时间过滤段，表示不需要读取该文件;
			if (!isPointIn && !isNextIn && !isPtOrIn)
			{
				// 不需要读取该文件;
				if (m_iCurNo != 999)
				{
					m_iCurNo += 1;
				}
				else
				{
					// 当前文件夹1000个文件已读完，应更新读取文件夹;
					m_iCurNo = 0;
					m_iCurDirNo += 1;

					std::string strDatDir = m_strCurDatDirPath;
					int nPos = strDatDir.find_last_of('/');
					std::string strDirDir = strDatDir.substr(0,nPos+1); // 截取前段文件名;
					char strDirTemp[1024];
					sprintf_s(strDirTemp,"%s/Image_%04d",strDirDir.data(),m_iCurDirNo);
					m_strCurDatDirPath = strDirTemp;
				}

				m_bJumpRead = true;
			}
			else
			{
				//bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
				bRet = readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
				if (!bRet)
				{
					return false;
				}

				m_bJumpRead = false;
			}
		}
		else
		{
			//bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
			bRet = readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
			if (!bRet)
			{
				return false;
			}

			m_bJumpRead = false;
		}
		
		m_bNeedRead = false;
	}

	// 条件判断读取溢出情况;
	if (m_nCurCamIndex >= m_vecCamSynInfo.size())
	{
		return false;
	}

	// 条件判断是否需要跳过该100条记录;
	if (m_bJumpRead)
	{
		return_pt_count = 0;
		m_nCurReadIndex++;
		m_nCurCamIndex++;

		if (m_nCurReadIndex >= 100)
		{
			m_nCurReadIndex = 0;
			m_bNeedRead = true;
		}

		return true;
	}


	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[m_nCurCamIndex];
	unsigned short* ptrRowHeight = m_ptrRowHeight[m_nCurReadIndex];
	unsigned char* ptrRowIntensity = m_ptrRowIntensity[m_nCurReadIndex];
	unsigned char* ptrRowTimeStamp = m_ptrRowTimeStamp[m_nCurReadIndex];
	if (!ptrRowHeight || !ptrRowIntensity)
	{
		return false;
	}
	// 一个PAVEMENT_CAM_SYN_INFO共对应2560*40;
	if (m_nCurReadIndex >= 0)
	{
		serializeDatData(camSynInfo,ptrRowHeight,ptrRowIntensity,ptrRowTimeStamp,pionts,return_pt_count);
	}

	
	//m_nCurSubReadIndex += 1;
	//if (m_nCurSubReadIndex >= 10) 
	//{
	//	// 即一帧数据读取完成，此时下一次读取应从文件中读取下一帧数据，即调用readNextCamSynData;
	//	m_nCurSubReadIndex = 0;
	//	m_nCurReadIndex += 1;
	//	m_nTestCount++;
	//}

	m_nCurReadIndex++;
	m_nCurCamIndex++;
	
	if (m_nCurReadIndex >= 100)
	{
		m_nCurReadIndex = 0;
		m_bNeedRead = true;
	}

	//return false;

	return true;
}

int hnPavementCamReader::GetScanLines()
{
	return m_vecCamSynInfo.size();
}

bool hnPavementCamReader::getScanTimeRange( double& start_gps_time,double& end_gps_time )
{
	// 根据内存记录的同步参数计算时间;
	double tempTime = 0.0;
	start_gps_time = 1000000000000.0;
	end_gps_time = -1;
	int nCount = 0;
	for (unsigned int n = 0;n < m_vecCamSynInfo.size();n++)
	{
		PAVEMENT_CAM_SYN_INFO camSynInfo = m_vecCamSynInfo[n];

		// 统计时间范围;
		tempTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0 - m_offset_time;
		if (tempTime < start_gps_time)
		{
			start_gps_time = tempTime;
		}
		if (tempTime > end_gps_time)
		{
			end_gps_time = tempTime;
		}
	}
	return true;
}

void hnPavementCamReader::getLinesIndex( std::vector<int>& vec_result_index,void (*processCallback)(float,const char*) /*= NULL*/ )
{
	vec_result_index.clear();
	for (int iFrame = 0;iFrame < m_vecCamSynInfo.size();iFrame++)
	{
		PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[iFrame];
		vec_result_index.push_back(iFrame);
	}

	return;

	// 重新打开文件;
	Close();
	Open(m_strCamFilePath.data());

	// 统计时间范围，只需要读取cam文件即可;
	vec_result_index.clear();
	int iIndex = 0;
	PAVEMENT_CAM_SYN_INFO camSynInfo;
	unsigned char frame_buff[PAVEMENT_CAM_SYN_LEN];
	while (!feof(m_ptrCamFile))
	{
		int nreadCount = fread(frame_buff,1,PAVEMENT_CAM_SYN_LEN,m_ptrCamFile);
		if (nreadCount < PAVEMENT_CAM_SYN_LEN)
		{
			// 认为读到文件尾，读取完成;
			continue;
		}

		// 解析读取到的同步帧数据至camSynInfo对象;
		if ( frame_buff[0] != 0xFF || frame_buff[1] != 0xFF || frame_buff[2] != 0xFF ) // || frame_buff[3] != 0xFF 
		{
			int test = 0;
		}
		camSynInfo.nGpsWeek = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+0] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+1];
		camSynInfo.nGpsSecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+2] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+3] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+4];
		camSynInfo.nMsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+5] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+6];
		camSynInfo.nUsecond = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+7] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+8];
		camSynInfo.nDmiValue = (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+9] << 24) |  (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+10] << 16) | (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+11] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+12];
		camSynInfo.vecRowUsecond.resize(39);
		int timediff = 0;
		for(int k=0; k<39; ++k)
		{
			timediff += (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+13+k*4] << 24)  |  (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+14+k*4] << 16) 
				| (int)(frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+15+k*4] << 8) | frame_buff[PAVEMENT_CAM_SYN_HEADER_LEN+16+k*4];
			camSynInfo.vecRowUsecond[k] = timediff;
		}

		vec_result_index.push_back(iIndex);
		iIndex++;
		//for (int n = 0;n < 10;n++)
		//{
		//	// 一帧对应一条外部记录;
		//	vec_result_index.push_back(iIndex);
		//	iIndex++;
		//}
	}

	return;
}

bool hnPavementCamReader::getFrameDmiValue(int iFrame, double& curDmiValue)
{
	if (iFrame < 0 || iFrame >= m_vecCamSynInfo.size())
	{
		return false;
	}

	// 此处应注意,直接按系数计算里程值可能由于编码器频率不正确造成比例系数不正确;
	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[iFrame];
	curDmiValue = camSynInfo.nDmiValue * 0.001;
	return true;
}

bool hnPavementCamReader::getFrameTime(int iFrame, double& curTime)
{
	if (iFrame < 0 || iFrame >= m_vecCamSynInfo.size())
	{
		return false;
	}

	// 此处应注意,直接按系数计算里程值可能由于编码器频率不正确造成比例系数不正确;
	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[iFrame];
	curTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;
	return true;
}

// 获取指定帧的时间
bool hnPavementCamReader::getGpsTime(int nPcdFrame, double& dGpsTimer)
{
	int nFrame = nPcdFrame / 40;
	int nSumFrame = nPcdFrame - nFrame * 40;

	if (nFrame < 0 || nFrame >= m_vecCamSynInfo.size())
	{
		return false;
	}

	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[nFrame];
	if (nSumFrame<camSynInfo.vecRowUsecond.size())
	{
		dGpsTimer = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.vecRowUsecond[nSumFrame] / 1000000.0;

	}
	else
	{
		dGpsTimer = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.vecRowUsecond[camSynInfo.vecRowUsecond.size()-1] / 1000000.0;

	}
}

bool hnPavementCamReader::readNextCamSynData(unsigned short** ptrRowHeight, unsigned char** ptrRowIntensity, unsigned char** ptrRowTimeStamp)
{
	// 读取数据;
	if (!m_ptrDatFile)
	{
		return false;
	}

	// 读取DAT数据，共读取40个触发记录（对应上述一帧同步数据）;
	if (feof(m_ptrDatFile)) // 上一次读取已读到文件尾，则打开新的文件;
	{
		int ret = fclose(m_ptrDatFile);
		m_ptrDatFile = NULL;
		if (m_iCurNo != 999) // 不为最后一个文件，则直接在本文件夹中查找
		{
			m_iCurNo += 1;
			std::string strCurDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
			if (_access(strCurDatFilePath.data(),0) != 0) // 文件不存在，返回;
			{
				return false;
			}

			m_ptrDatFile = fopen(strCurDatFilePath.data(),"rb");
			if (!m_ptrDatFile)
			{
				return false;
			}
		}
		else
		{
			// 当前文件夹1000个文件已读完，应更新读取文件夹;
			m_iCurNo = 0;
			m_iCurDirNo += 1;
			std::string strDatDir = m_strCurDatDirPath;
			int nPos = strDatDir.find_last_of('/');
			std::string strDirDir = strDatDir.substr(0,nPos+1); // 截取前段文件名;
			char strDirTemp[1024];
			sprintf_s(strDirTemp,"%s/Image_%04d",strDirDir.data(),m_iCurDirNo);
			m_strCurDatDirPath = strDirTemp;

			std::string strCurDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
			if (_access(strCurDatFilePath.data(),0) != 0) // 文件不存在，返回;
			{
				return false;
			}

			m_ptrDatFile = fopen(strCurDatFilePath.data(),"rb");
			if (!m_ptrDatFile)
			{
				return false;
			}
		}
	}

	// 打开文件后读取数据,先读取高程数据;
	int nreadCount = 0;
	for (int n = 0;n < 100;n++)
	{
		unsigned short* ptrHeight = ptrRowHeight[n];
		memset(ptrHeight,0,sizeof(unsigned short) * 2560*40);

		nreadCount = fread(ptrHeight,1,sizeof(unsigned short) * 2560*40,m_ptrDatFile);
		if (nreadCount < 2560*40)
		{
			return false;
		}
	}

	// 中间字段信息共计4096字节跳过;
	//unsigned char* jumpStamps = new unsigned char[4120];
	unsigned char jumpStamps[4120];
	nreadCount = fread(jumpStamps,1,sizeof(unsigned char) * 4096,m_ptrDatFile);

	// 读取强度信息数据;
	for (int n = 0;n < 100;n++)
	{
		unsigned char* ptrIntensity = ptrRowIntensity[n];
		memset(ptrIntensity,0,sizeof(unsigned char) * 2560*40);

		nreadCount = fread(ptrIntensity,1,sizeof(unsigned char) * 2560*40,m_ptrDatFile);
		if (nreadCount < 2560*40)
		{
			return false;
		}
	}

	// 跳过4120字节;
	nreadCount = fread(jumpStamps,1,sizeof(unsigned char) * 4120,m_ptrDatFile);

	// 读取时间戳数据;
	for (int n = 0;n < 100;n++)
	{
		unsigned char* ptrTimeStamp = ptrRowTimeStamp[n];
		memset(ptrTimeStamp,0,sizeof(unsigned char) * 16*40);

		nreadCount = fread(ptrTimeStamp,1,sizeof(unsigned char)*16*40,m_ptrDatFile);
		if (nreadCount < 16*40)
		{
			return false;
		}
	}

	// 上述一次读取了 2560 * 4000行数据，认为文件读完，移动文件指针至末尾;
	int testCount = 0;
	unsigned char data;
	fseek(m_ptrDatFile,0,SEEK_END);
	while (!feof(m_ptrDatFile))
	{
		nreadCount = fread(&data,1,sizeof(unsigned char),m_ptrDatFile);
		testCount++;
	}

	//delete[] jumpStamps;
	//jumpStamps = NULL;

	return true;
}

void hnPavementCamReader::serializeDatData( PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,
	unsigned char* ptrRowTimeStamp,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count ,int usePtCount)
{
	if (!ptrRowHeight || !ptrRowIntensity || !ptrRowTimeStamp)
	{
		return;
	}

	// 计算当前帧第零行的时间值;
	double gpsTime = 0.0;
	double tempGpsTime = 0.0;
	tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

	// 计算单点坐标;
	bool isPointIn = false;
	static int lastRow = 0;
	double tempAdd = 0.0;
	double tempAddTime = 0.0;
	int curCol = 0;
	int curRow = 0;
	unsigned short uheight = 0;
	unsigned char uIntensity = 0;
	int nPtCount = 0;

	//// 计算当前40行的时间差值信息;
	//std::vector<unsigned long long> vecRowTimeStamps;
	//for (int n = 0;n < 40;n++)
	//{
	//	unsigned long long timeStamps = 0;
	//	for (int r = 0;r < 8;r++)
	//	{
	//		timeStamps = timeStamps * 256 + ptrRowTimeStamp[n * 16 + r];
	//	}

	//	vecRowTimeStamps.push_back(timeStamps);
	//}

	pionts.resize(usePtCount);
	POINT_STRUCT_XYZIT_INFO pointInfo;
	POINT_STRUCT_XYZIT_INFO tmppointInfo;
	for (int n = 0;n < usePtCount;n++)
	{
		// 获取到当前行;
		curRow = n / 2560;
		curCol = n % 2560;

		// 获取原始的强度信息和高程信息;
		uheight = ptrRowHeight[n];
		uIntensity = ptrRowIntensity[n];

		// 时间计算;
		if (curRow > 0)
		{
			tempAddTime = camSynInfo.vecRowUsecond[curRow-1] * 0.000001;
		}
		else
		{
			tempAddTime = 0.0;
		}

		// 计算获得GPS时间值;
		gpsTime = tempGpsTime + tempAddTime - m_offset_time;
		lastRow = curRow;

		//if (gpsTime > 286626.995 && gpsTime <= 286628.995)
		//{
		//	int test = 0;
		//}

		isPointIn = isTimeInVector(gpsTime,m_vec_combine_time_range);
		if (!isPointIn ) // || uIntensity < 12
		{
			continue;
		}

		if (m_jumpLines > 0)
		{
			// 用于外部抽稀，抽取5,10帧、15, 20帧、25 30帧、35 40帧;
			int tempV1 = curRow % m_jumpLines;// 为抽取帧数的整数倍为有效数据值;
			if ( tempV1 != 0 )
			{
				continue;
			}
		}

		// 计算坐标信息;
		pointInfo.timeSecond = gpsTime;
		//pointInfo.x = (1280 - curCol) * 0.0015;
		//pointInfo.y = 0.0;
		//pointInfo.z = uheight * 0.0005 - 1.95;
		pointInfo.x = m_MatrxX[uheight][curCol] * 0.001;
		pointInfo.y = 0.0;
		pointInfo.z = 0.0 - m_MatrxZ[uheight][curCol] * 0.001;

		pointInfo.intensity = uIntensity;
		//if (!pointInfo.isValid())
		//{
		//	continue;
		//}

		pionts[nPtCount] = pointInfo;
		nPtCount++;
	}

	return_pt_count = nPtCount;
}

bool hnPavementCamReader::isTimeInVector( double gps_time,std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range )
{
	// 查找确定该点在是否在容器时间范围内;
	bool is_time_in = false;

	// 若无时间范围，则默认全部在范围内;
	if (vec_combine_time_range.size() <= 0)
	{
		return true;
	}

	double time_in = 0.0;
	double upper_time = 0.0;
	double lower_time = 0.0;
	for (unsigned int n = 0;n < vec_combine_time_range.size();n++)
	{
		COMBINE_TIME_RANGE time_range = vec_combine_time_range[n];
		upper_time = time_range.end_gps_second;
		lower_time = time_range.start_gps_second;
		if (gps_time >= lower_time && gps_time <= upper_time)
		{
			is_time_in = true;
			break;
		}
	}

	return is_time_in;
}

bool hnPavementCamReader::isTimeOrInVector(double start_time, double end_time, std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range)
{
	// 查找确定该点在是否在容器时间范围内;
	bool is_time_in = false;

	// 若无时间范围，则默认全部在范围内;
	if (vec_combine_time_range.size() <= 0)
	{
		return true;
	}

	// 首尾段检查;
	double time_in = 0.0;
	double upper_time = 0.0;
	double lower_time = 0.0;
	for (unsigned int n = 0; n < vec_combine_time_range.size(); n++)
	{
		COMBINE_TIME_RANGE time_range = vec_combine_time_range[n];
		upper_time = time_range.end_gps_second;
		lower_time = time_range.start_gps_second;

		// 检查首尾;
		if (start_time <= lower_time && end_time >= upper_time)
		{
			is_time_in = true;
			break;
		}

		//if (gps_time >= lower_time && gps_time <= upper_time)
		//{
		//	is_time_in = true;
		//	break;
		//}
	}

	return is_time_in;
}

bool hnPavementCamReader::readCamMatrix()
{
	std::string strCamDir = m_strCamFilePath;
	int nPos = strCamDir.find_last_of('/');
	strCamDir = strCamDir.substr(0,nPos+1); // 截取文件路径;

	// 获取矩阵存储路径;
	char strMatrixTemp[1024];
	sprintf_s(strMatrixTemp,"%s/Mat_X.cal",strCamDir.data());
	std::string strMatrixXPath = strMatrixTemp;
	sprintf_s(strMatrixTemp,"%s/Mat_Z.cal",strCamDir.data());
	std::string strMatrixZPath = strMatrixTemp;

	// 检查文件是否存在;
	if ( _access(strMatrixXPath.data(),0) != 0 || _access(strMatrixZPath.data(),0) != 0 )
	{
		return false;
	}

	FILE* fp = fopen(strMatrixXPath.data(), "rb");
	if (fp != NULL)
	{
		for (int i = 0; i < 960; ++i)
		{
			fread(m_MatrxX[i], sizeof(double), 2560, fp);
		}
	}
	fclose(fp);

	fp = fopen(strMatrixZPath.data(), "rb");
	if (fp != NULL)
	{
		for (int i = 0; i < 960; ++i)
		{
			fread(m_MatrxZ[i], sizeof(double), 2560, fp);
		}
	}
	fclose(fp);

	//FILE* ptrFile = fopen("E:\\checkMatrix.txt","wt+");
	//for (int i = 0; i < 960;i++)
	//{
	//	for (int j = 0;j < 2560;j++)
	//	{
	//		fprintf_s(ptrFile,"%.4lf,",m_MatrxX[i][j]);
	//	}
	//	fprintf_s(ptrFile,"\n");

	//	for (int j = 0;j < 2560;j++)
	//	{
	//		fprintf_s(ptrFile,"%.4lf,",m_MatrxZ[i][j]);
	//	}
	//	fprintf_s(ptrFile,"\n");
	//	fprintf_s(ptrFile,"\n");
	//}

	//fclose(ptrFile);

	return true;
}

bool hnPavementCamReader::getRoadLinesPoints( int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	bool bRet = false;
	if (m_bNeedRead)
	{
		bRet = readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
		//bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
		if (!bRet)
		{
			return false;
		}

		m_bNeedRead = false;
	}

	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[m_nCurCamIndex];
	//PAVEMENT_CAM_SYN_INFO& camSynInfo = m_curCamSynInfo[m_nCurReadIndex];
	unsigned short* ptrRowHeight = m_ptrRowHeight[m_nCurReadIndex];
	unsigned char* ptrRowIntensity = m_ptrRowIntensity[m_nCurReadIndex];
	unsigned char* ptrRowTimeStamp = m_ptrRowTimeStamp[m_nCurReadIndex];
	if (!ptrRowHeight || !ptrRowIntensity)
	{
		return false;
	}
	// 一个PAVEMENT_CAM_SYN_INFO共对应2560*40;
	if (m_nCurReadIndex >= 0)
	{
		serializeRoadDatData(camSynInfo,ptrRowHeight,ptrRowIntensity,ptrRowTimeStamp,pionts,return_pt_count);
	}

	m_nCurReadIndex++;

	if (m_nCurReadIndex >= 100)
	{
		m_nCurReadIndex = 0;
		m_bNeedRead = true;
	}

	return true;
}

void hnPavementCamReader::serializeRoadDatData( PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,unsigned char* ptrRowTimeStamp,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	if (!ptrRowHeight || !ptrRowIntensity || !ptrRowTimeStamp)
	{
		return;
	}

	// 计算当前帧第零行的时间值;
	double gpsTime = 0.0;
	double tempGpsTime = 0.0;
	tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

	// 计算单点坐标;
	bool isPointIn = false;
	double tempAdd = 0.0;
	double tempAddTime = 0.0;
	int curCol = 0;
	int curRow = 0;
	unsigned short uheight = 0;
	unsigned char uIntensity = 0;
	int nPtCount = 0;

	int nMaxCol = 0;
	int nMinCol = 2560;
	POINT_STRUCT_XYZIT_INFO minInfo;
	POINT_STRUCT_XYZIT_INFO maxInfo;

	pionts.resize(2560*40);
	POINT_STRUCT_XYZIT_INFO pointInfo;
	for (int n = 0;n < 2560*40;n++)
	{
		// 获取到当前行;
		curRow = n / 2560;
		curCol = n % 2560;

		// 获取原始的强度信息和高程信息;
		uheight = ptrRowHeight[n];
		uIntensity = ptrRowIntensity[n];

		// 时间计算;
		if (curRow > 0)
		{
			tempAddTime = camSynInfo.vecRowUsecond[curRow-1] * 0.000001;
		}
		else
		{
			tempAddTime = 0.0;
		}

		// 计算获得GPS时间值;
		gpsTime = tempGpsTime + tempAddTime - m_offset_time;

		isPointIn = isTimeInVector(gpsTime,m_vec_combine_time_range);
		if (!isPointIn)
		{
			continue;
		}

		// 计算坐标信息;
		pointInfo.timeSecond = gpsTime;
		//pointInfo.x = (1280 - curCol) * 0.0015;
		//pointInfo.y = 0.0;
		//pointInfo.z = uheight * 0.0005 - 1.95;
		pointInfo.x = m_MatrxX[uheight][curCol] * 0.001;
		pointInfo.y = 0.0;
		pointInfo.z = 0.0 - m_MatrxZ[uheight][curCol] * 0.001;

		pointInfo.intensity = uIntensity;

		// 无效零点不记录;
		if (!pointInfo.isValid())
		{
			continue;
		}

		if (curRow == 39)
		{
			if (curCol > nMaxCol)
			{
				nMaxCol = curCol;
				maxInfo = pointInfo;
			}

			if (curCol < nMinCol)
			{
				nMinCol = curCol;
				minInfo = pointInfo;
			}
		}

		//if (curRow == 39 && ( curCol <= 35 || curCol >= 2520 ))
		//{
		//	pionts[nPtCount] = pointInfo;
		//	nPtCount++;
		//}
	}

	pionts[0] = minInfo;
	pionts[1] = maxInfo;

	return_pt_count = 2;
}

//using namespace snappy;
bool hnPavementCamReader::readNextCamSynDataNew( unsigned short** ptrRowHeight,unsigned char** ptrRowIntensity,unsigned char** ptrRowTimeStamp )
{
	// 读取数据;
	if (!m_ptrDatFile)
	{
		return false;
	}

	// 读取DAT数据，共读取40个触发记录（对应上述一帧同步数据）;
	if (feof(m_ptrDatFile)) // 上一次读取已读到文件尾，则打开新的文件;
	{
		int ret = fclose(m_ptrDatFile);
		m_ptrDatFile = NULL;
		if (m_iCurNo != 999) // 不为最后一个文件，则直接在本文件夹中查找
		{
			m_iCurNo += 1;
			std::string strCurDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
			if (_access(strCurDatFilePath.data(),0) != 0) // 文件不存在，返回;
			{
				return false;
			}

			//// 测试代码start;
			//FILE* ptrFile = fopen("E:\\checkTst1.txt","at+");
			//fprintf_s(ptrFile,"%d	%d	%d	%s\n",m_iCurDirNo,m_iCurNo,m_nCurCamIndex,strCurDatFilePath.data());
			//fclose(ptrFile);
			//// 测试代码end;

			m_ptrDatFile = fopen(strCurDatFilePath.data(),"rb");
			if (!m_ptrDatFile)
			{
				return false;
			}
		}
		else
		{
			// 当前文件夹1000个文件已读完，应更新读取文件夹;
			m_iCurNo = 0;
			m_iCurDirNo += 1;
			std::string strDatDir = m_strCurDatDirPath;
			int nPos = strDatDir.find_last_of('/');
			std::string strDirDir = strDatDir.substr(0,nPos+1); // 截取前段文件名;
			char strDirTemp[1024];
			sprintf_s(strDirTemp,"%s/Image_%04d",strDirDir.data(),m_iCurDirNo);
			m_strCurDatDirPath = strDirTemp;

			std::string strCurDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
			if (_access(strCurDatFilePath.data(),0) != 0) // 文件不存在，返回;
			{
				return false;
			}

			//// 测试代码start;
			//FILE* ptrFile = fopen("E:\\checkTst1.txt","at+");
			//fprintf_s(ptrFile,"%d	%d	%d	%s\n",m_iCurDirNo,m_iCurNo,m_nCurCamIndex,strCurDatFilePath.data());
			//fclose(ptrFile);
			//// 测试代码end;

			m_ptrDatFile = fopen(strCurDatFilePath.data(),"rb");
			if (!m_ptrDatFile)
			{
				return false;
			}
		}
	}

	// 打开文件后，读取全部二进制数据;
	fseek(m_ptrDatFile,0,SEEK_END);
	size_t camLength = ftell(m_ptrDatFile);
	fseek(m_ptrDatFile,0,SEEK_SET);

	//memset(m_strOrientData,0,2560 * 40 * 100 * 2);
	//memset(m_strOrientDataNew,0,2560 * 40 * 100 * 2);

	char* strOrientData = new char[camLength];
	fread(strOrientData,1,camLength,m_ptrDatFile);
	while (!feof(m_ptrDatFile))
	{
		char st1;
		fread(&st1,1,1,m_ptrDatFile);
	}

	size_t camlengthNew = 0;
	if (snappy_uncompressed_length((const char*)strOrientData,camLength,&camlengthNew) != SNAPPY_OK)
	{
		delete[] strOrientData;
		strOrientData = NULL;
		return false;
	}

	char* strOrientDataNew = new char[camlengthNew];
	if (snappy_uncompress((const char*)strOrientData,camLength,strOrientDataNew,&camlengthNew) != SNAPPY_OK)
	{
		delete[] strOrientData;
		strOrientData = NULL;
		delete[] strOrientDataNew;
		strOrientDataNew = NULL;
		return false;
	}

	// 解析后数据复制;
	for (int n = 0;n < 100;n++)
	{
		unsigned short tmp_gray = 0;
		unsigned short tmp2 = 0;
		unsigned short tmp = 0;
		long pointnum = 2560*40;
		long datalen = sizeof(unsigned short) * pointnum;
		unsigned short* ptrHeight = ptrRowHeight[n];
		memset(ptrHeight,0,datalen);

		unsigned char* ptrIntensity = ptrRowIntensity[n];
		memset(ptrIntensity,0,sizeof(unsigned char) * 2560*40);

		memcpy(ptrHeight,strOrientDataNew + n * datalen,datalen);

		for (int i1=0; i1 < pointnum; ++i1)
		{
			tmp = ptrHeight[i1] & 0x03ff;
			tmp2 = ptrHeight[i1] & 0xfc00;
			tmp2 = tmp2 >> 10;
			tmp2 = tmp2 << 2;
			ptrHeight[i1] = tmp;
			ptrIntensity[i1] = tmp2;
			int test = 0;
		}
	}

	delete[] strOrientData;
	strOrientData = NULL;
	delete[] strOrientDataNew;
	strOrientDataNew = NULL;

	return true;
}

bool hnPavementCamReader::getFullDatPaths( std::vector<QString>& vecDatPaths )
{
	// 查找第一个dat文件;
	vecDatPaths.clear();
	int iCurNo = 0;
	int iCurDirNo = 0;
	std::string strCurDatDirPath = m_strCurDatDirPath;


	// 确定多少个文件;
	int ndatCount = (int)floor(m_vecCamSynInfo.size() / 100 + 0.5);
	for (unsigned int n = 0;n < ndatCount;n++)
	{
		// 文件存在则记录;
		std::string strFirstDatFilePath = getDatPathByNum(strCurDatDirPath.data(),iCurNo);
		if (_access(strFirstDatFilePath.data(),0) == 0)
		{
			vecDatPaths.push_back(QString::fromLocal8Bit(strFirstDatFilePath.data()));
		}

		iCurNo++;

		// 下一次应该找后续文件夹;
		if (iCurNo >= 1000)
		{
			// 当前文件夹1000个文件已读完，应更新读取文件夹;
			iCurNo = 0;
			iCurDirNo += 1;
			std::string strDatDir = strCurDatDirPath;
			int nPos = strDatDir.find_last_of('/');
			std::string strDirDir = strDatDir.substr(0,nPos+1); // 截取前段文件名;
			char strDirTemp[1024];
			sprintf_s(strDirTemp,"%s/Image_%04d",strDirDir.data(),iCurDirNo);
			strCurDatDirPath = strDirTemp;
		}
	}

	return true;
}

void hnPavementCamReader::setSplitDatIndex( int start_index,int end_index )
{
	m_split_start_index = start_index;
	m_split_end_index = end_index;
}

bool hnPavementCamReader::jumpToSplitStartIndex()
{
	// 确定当前img文件夹和img文件夹下哪一帧开始;
	m_iCurDirNo = m_split_start_index / 1000;
	m_iCurNo = m_split_start_index % 1000;
	m_nCurReadIndex = 0;
	m_bNeedRead = true;

	m_nCurCamIndex = m_split_start_index * 100;

	// 根据当前cam文件名，顺序查找第一个dat所在文件夹路径;
	std::string strDatFilePath = m_strCamFilePath;
	int nPos = strDatFilePath.find_last_of('/');
	std::string strFirstDatDirPath = strDatFilePath.substr(0,nPos+1); // 截取前段文件名;

	// 设置当前文件夹路径;
	char strDirTemp[1024];
	sprintf_s(strDirTemp,"%s/Image_%04d",strFirstDatDirPath.data(),m_iCurDirNo);
	m_strCurDatDirPath = strDirTemp;

	// 检查该文件夹是否存在;
	if (_access(m_strCurDatDirPath.data(),0) != 0)
	{
		return false;
	}

	// 查找第一个dat文件;
	std::string strFirstDatFilePath = getDatPathByNum(m_strCurDatDirPath.data(),m_iCurNo);
	if (_access(strFirstDatFilePath.data(),0) != 0)
	{
		return false;
	}

	// 打开文件;
	m_ptrDatFile = fopen(strFirstDatFilePath.data(),"rb");
	return true;
}

void hnPavementCamReader::getSplitDatTime( double& start_gps_time,double& end_gps_time )
{
	// 前后扩展一帧，防止溢出;
	int startCamIndex = m_split_start_index * 100 - 1;
	int endCamIndex = m_split_end_index * 100 + 1;
	start_gps_time = end_gps_time = 0.0;
	if (startCamIndex < 0)
	{
		startCamIndex = 0;
	}
	if (endCamIndex >= m_vecCamSynInfo.size())
	{
		endCamIndex = m_vecCamSynInfo.size() - 1;
	}

	if (startCamIndex < m_vecCamSynInfo.size())
	{
		PAVEMENT_CAM_SYN_INFO& camInfo = m_vecCamSynInfo[startCamIndex];

		// 获取时间;
		double tempGpsTimeNext = 0.0;
		tempGpsTimeNext = camInfo.nGpsSecond - m_offset_time + camInfo.nMsecond / 1000.0 + camInfo.nUsecond / 1000000.0;
		start_gps_time = tempGpsTimeNext - 1.0;
	}

	if (endCamIndex < m_vecCamSynInfo.size())
	{
		PAVEMENT_CAM_SYN_INFO& camInfo = m_vecCamSynInfo[endCamIndex];

		// 获取时间;
		double tempGpsTimeNext = 0.0;
		tempGpsTimeNext = camInfo.nGpsSecond - m_offset_time + camInfo.nMsecond / 1000.0 + camInfo.nUsecond / 1000000.0;
		end_gps_time = tempGpsTimeNext + 1.0;
	}
}

void hnPavementCamReader::getSplitLineIndex( std::vector<int>& vec_result_index,void (*processCallback)(float,const char*) /*= NULL*/ )
{
	int startCamIndex = m_split_start_index * 100;
	int endCamIndex = m_split_end_index * 100;
	vec_result_index.clear();
	for (int iFrame = startCamIndex;iFrame < endCamIndex;iFrame++)
	{
		if (iFrame >= m_vecCamSynInfo.size())
		{
			continue;
		}

		PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[iFrame];
		vec_result_index.push_back(iFrame);
	}

	return;
}

void hnPavementCamReader::setCamSynInfoLoad( std::vector<PAVEMENT_CAM_SYN_INFO>& vecCamSynInfo ,const char* path)
{
	m_vecCamSynInfo = vecCamSynInfo;

	// 记录cam文件路径;
	m_strCamFilePath = path;

	// 读取相机标定文件;
	readCamMatrix();
}

bool hnPavementCamReader::getLinePointsWithJump( int idex,int jumpLines,int leftOrRight,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	bool bRet = false;
	if (m_bNeedRead)
	{
		// 增加时间判断，若当前帧不在时间过滤范围内，则跳过;
		PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[m_nCurCamIndex];
		if (m_nCurCamIndex > 0 && m_nCurCamIndex < (m_vecCamSynInfo.size() - 100)) // 添加过滤条件;
		{
			PAVEMENT_CAM_SYN_INFO& camSynInfoNext = m_vecCamSynInfo[m_nCurCamIndex + 100];

			// 时间信息判断;
			double tempGpsTime = 0.0;
			tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

			double tempGpsTimeNext = 0.0;
			tempGpsTimeNext = camSynInfoNext.nGpsSecond + camSynInfoNext.nMsecond / 1000.0 + camSynInfoNext.nUsecond / 1000000.0;
			bool isPointIn = isTimeInVector(tempGpsTime,m_vec_combine_time_range);
			bool isNextIn = isTimeInVector(tempGpsTimeNext,m_vec_combine_time_range);

			// 若两个点都不在过滤时间范围内，表示不需要读取该文件;
			if (!isPointIn && !isNextIn)
			{
				// 不需要读取该文件;
				if (m_iCurNo != 999)
				{
					m_iCurNo += 1;
				}
				else
				{
					// 当前文件夹1000个文件已读完，应更新读取文件夹;
					m_iCurNo = 0;
					m_iCurDirNo += 1;

					std::string strDatDir = m_strCurDatDirPath;
					int nPos = strDatDir.find_last_of('/');
					std::string strDirDir = strDatDir.substr(0,nPos+1); // 截取前段文件名;
					char strDirTemp[1024];
					sprintf_s(strDirTemp,"%s/Image_%04d",strDirDir.data(),m_iCurDirNo);
					m_strCurDatDirPath = strDirTemp;
				}

				m_bJumpRead = true;
			}
			else
			{
				//bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
				bRet = readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
				if (!bRet)
				{
					return false;
				}

				m_bJumpRead = false;
			}
		}
		else
		{
			//bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
			bRet = readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);
			if (!bRet)
			{
				return false;
			}

			m_bJumpRead = false;
		}

		m_bNeedRead = false;
	}

	// 条件判断读取溢出情况;
	if (m_nCurCamIndex >= m_vecCamSynInfo.size())
	{
		return false;
	}

	// 条件判断是否需要跳过该100条记录;
	if (m_bJumpRead)
	{
		return_pt_count = 0;
		//m_nCurReadIndex++;
		//m_nCurCamIndex++;
		m_nCurReadIndex += jumpLines;
		m_nCurCamIndex += jumpLines;

		if (m_nCurReadIndex >= 100)
		{
			m_nCurReadIndex = 0;
			m_bNeedRead = true;
		}

		return true;
	}


	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[m_nCurCamIndex];
	unsigned short* ptrRowHeight = m_ptrRowHeight[m_nCurReadIndex];
	unsigned char* ptrRowIntensity = m_ptrRowIntensity[m_nCurReadIndex];
	unsigned char* ptrRowTimeStamp = m_ptrRowTimeStamp[m_nCurReadIndex];
	if (!ptrRowHeight || !ptrRowIntensity)
	{
		return false;
	}
	// 一个PAVEMENT_CAM_SYN_INFO共对应2560*40，本接口只使用2560个点（即单帧数据）;
	if (m_nCurReadIndex >= 0)
	{
		//serializeDatData(camSynInfo,ptrRowHeight,ptrRowIntensity,ptrRowTimeStamp,pionts,return_pt_count,2560);
		serializeDatDataFrame(camSynInfo,ptrRowHeight,ptrRowIntensity,ptrRowTimeStamp,pionts,return_pt_count,leftOrRight);
	}

	m_nCurReadIndex += jumpLines;
	m_nCurCamIndex += jumpLines;

	if (m_nCurReadIndex >= 100)
	{
		m_nCurReadIndex = 0;
		m_bNeedRead = true;
	}

	return true;
}

bool hnPavementCamReader::getSubFramePoints( int mainFrameIndex,int subFrameIndex,int leftOrRight,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count )
{
	// 条件判断;
	if (mainFrameIndex < 0 || mainFrameIndex >= m_vecCamSynInfo.size())
	{
		return false;
	}

	// 根据帧索引信息，确定是哪一张影像;
	int datImgIndex = mainFrameIndex / 100;
	int datInSubIndex = mainFrameIndex % 100;
	int frameIndex = datInSubIndex * 40 + subFrameIndex;

	// 几何顺序计算只解压当前DAT，不再预解析其中全部4000个断面；
	// 每次请求仅拆出目标断面的2560个点，普通读取模式仍保持原逻辑。
	if (m_subFrameCacheEnabled && m_cachedSubFrameDatIndex != datImgIndex)
	{
		QString strCurDatPath = getDatPathByImgNo(datImgIndex);
		if (strCurDatPath.isEmpty())
		{
			// 记住缺失块，避免同一尾段的后续断面反复扫描目录和输出警告。
			m_cachedSubFrameDatIndex = datImgIndex;
			m_cachedSubFrameRawData.clear();
			return false;
		}
		QFile datFile(strCurDatPath);
		if (!datFile.open(QIODevice::ReadOnly))
		{
			m_cachedSubFrameDatIndex = -1;
			m_cachedSubFrameRawData.clear();
			return false;
		}
		const QByteArray compressed = datFile.readAll();
		size_t uncompressedLength = 0;
		if (snappy_uncompressed_length(compressed.constData(), compressed.size(), &uncompressedLength) != SNAPPY_OK)
		{
			m_cachedSubFrameDatIndex = -1;
			m_cachedSubFrameRawData.clear();
			return false;
		}
		m_cachedSubFrameRawData.resize(uncompressedLength);
		if (snappy_uncompress(compressed.constData(), compressed.size(),
			m_cachedSubFrameRawData.data(), &uncompressedLength) != SNAPPY_OK)
		{
			m_cachedSubFrameDatIndex = -1;
			m_cachedSubFrameRawData.clear();
			return false;
		}
		m_cachedSubFrameRawData.resize(uncompressedLength);
		m_cachedSubFrameDatIndex = datImgIndex;
	}
	else if (!m_subFrameCacheEnabled)
	{
		QString strCurDatPath = getDatPathByImgNo(datImgIndex);
		if (m_ptrDatFile)
		{
			fclose(m_ptrDatFile);
			m_ptrDatFile = NULL;
		}
		m_ptrDatFile = fopen(strCurDatPath.toLocal8Bit().data(),"rb");
		if (!m_ptrDatFile || !readNextCamSynDataNew(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp))
			return false;
	}
	//bool bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);

	// 解析当前帧位置的数据;
	if ((!m_subFrameCacheEnabled && (!m_ptrRowHeight || !m_ptrRowIntensity || !m_ptrRowTimeStamp))
		|| (m_subFrameCacheEnabled && m_cachedSubFrameRawData.empty()))
	{
		return false;
	}

	// 计算当前帧第零行的时间值;
	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[mainFrameIndex];
	unsigned short* ptrRowHeight = m_subFrameCacheEnabled ? NULL : m_ptrRowHeight[datInSubIndex];
	unsigned char* ptrRowIntensity = m_subFrameCacheEnabled ? NULL : m_ptrRowIntensity[datInSubIndex];
	unsigned char* ptrRowTimeStamp = m_subFrameCacheEnabled ? NULL : m_ptrRowTimeStamp[datInSubIndex];
	const unsigned short* packedPoints = m_subFrameCacheEnabled
		? reinterpret_cast<const unsigned short*>(m_cachedSubFrameRawData.data()) : NULL;
	double gpsTime = 0.0;
	double tempGpsTime = 0.0;
	tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

	// 计算单点坐标;
	bool isPointIn = false;
	static int lastRow = 0;
	double tempAdd = 0.0;
	double tempAddTime = 0.0;
	int curCol = 0;
	int curRow = 0;
	unsigned short uheight = 0;
	unsigned char uIntensity = 0;
	int nPtCount = 0;

	pionts.resize(2560);
	POINT_STRUCT_XYZIT_INFO pointInfo;
	POINT_STRUCT_XYZIT_INFO tmppointInfo;
	for (int n = 2560 * subFrameIndex;n < 2560 * (subFrameIndex+1);n++)
	{
		// 获取到当前行;
		curRow = n / 2560;
		curCol = n % 2560;

		// 获取原始的强度信息和高程信息;
		if (m_subFrameCacheEnabled)
		{
			const size_t packedIndex = static_cast<size_t>(frameIndex) * 2560 + curCol;
			if ((packedIndex + 1) * sizeof(unsigned short) > m_cachedSubFrameRawData.size())
				continue;
			const unsigned short packed = packedPoints[packedIndex];
			uheight = packed & 0x03ff;
			uIntensity = static_cast<unsigned char>(((packed & 0xfc00) >> 10) << 2);
		}
		else
		{
			uheight = ptrRowHeight[n];
			uIntensity = ptrRowIntensity[n];
		}

		// 时间计算;
		if (subFrameIndex > 0)
		{
			tempAddTime = camSynInfo.vecRowUsecond[subFrameIndex-1] * 0.000001;
		}
		else
		{
			tempAddTime = 0.0;
		}

		// 计算获得GPS时间值;
		gpsTime = tempGpsTime + tempAddTime - m_offset_time;
		lastRow = curRow;

		isPointIn = isTimeInVector(gpsTime,m_vec_combine_time_range);
		if (!isPointIn || uIntensity < 12) // 
		{
			continue;
		}

		// 计算坐标信息;
		pointInfo.timeSecond = gpsTime;
		pointInfo.x = m_MatrxX[uheight][curCol] * 0.001;
		pointInfo.y = 0.0;
		pointInfo.z = 0.0 - m_MatrxZ[uheight][curCol] * 0.001;

		pointInfo.intensity = uIntensity;
		if (!pointInfo.isValid())
		{
			continue;
		}

		// 判断取值要求,为-1表示取中心点左侧，为1表示取中心点右侧，为0表示全取;
		if (leftOrRight < 0 )
		{
			if ( curCol < 1280 )
			{
				pionts[nPtCount] = pointInfo;
				nPtCount++;
			}
		}
		else if (leftOrRight > 0)
		{
			if ( curCol > 1280 )
			{
				pionts[nPtCount] = pointInfo;
				nPtCount++;
			}
		}
		else
		{
			pionts[nPtCount] = pointInfo;
			nPtCount++;
		}

		//pionts[nPtCount] = pointInfo;
		//nPtCount++;
	}

	return_pt_count = nPtCount;

	return true;
}

// 读取指定帧数据（2560个点）nImageIndex-影像索引 subFrameIndex 当前影像所在帧号;
bool hnPavementCamReader::getFramePoints(int nImageIndex, int nSubFrameIndex, int nPtIndex, hn3dPointD& out3dPt)
{
	// 定位哪一个包 40帧数据1包
	int datInSubIndex = nSubFrameIndex / 40;
	nSubFrameIndex = nSubFrameIndex - datInSubIndex * 40;

	// 条件判断--每张图像100包数据;
	if (datInSubIndex < 0 || datInSubIndex >= 100)
	{
		return false;
	}

	// 获取当前帧的影像路径;
	QString strCurDatPath = getDatPathByImgNo(nImageIndex);

	// 读取该张影像的全部数据;
	if (m_ptrDatFile)
	{
		fclose(m_ptrDatFile);
	}
	m_ptrDatFile = fopen(strCurDatPath.toLocal8Bit().data(), "rb");

	// 获取指定帧数据，首先跳转至指定帧;
	bool bRet = readNextCamSynDataNew(m_ptrRowHeight, m_ptrRowIntensity, m_ptrRowTimeStamp);
	//bool bRet = readNextCamSynData(m_ptrRowHeight,m_ptrRowIntensity,m_ptrRowTimeStamp);

	// 解析当前帧位置的数据;
	if (!m_ptrRowHeight || !m_ptrRowIntensity || !m_ptrRowTimeStamp)
	{
		return false;
	}

	// 计算当前帧第零行的时间值;
	PAVEMENT_CAM_SYN_INFO& camSynInfo = m_vecCamSynInfo[nImageIndex*100 + datInSubIndex];
	unsigned short* ptrRowHeight = m_ptrRowHeight[datInSubIndex];
	unsigned char* ptrRowIntensity = m_ptrRowIntensity[datInSubIndex];
	unsigned char* ptrRowTimeStamp = m_ptrRowTimeStamp[datInSubIndex];
	double gpsTime = 0.0;
	double tempGpsTime = 0.0;
	tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

	// 计算单点坐标;
	bool isPointIn = false;
	static int lastRow = 0;
	double tempAdd = 0.0;
	double tempAddTime = 0.0;
	int curCol = 0;
	int curRow = 0;
	unsigned short uheight = 0;
	unsigned char uIntensity = 0;
	int nPtCount = 0;

	POINT_STRUCT_XYZIT_INFO pointInfo;
	for (int n = 2560 * nSubFrameIndex; n < 2560 * (nSubFrameIndex + 1); n++)
	{
		// 获取到当前行;
		curRow = n / 2560;
		curCol = n % 2560;

		if (curCol != nPtIndex)
		{
			continue;
		}

		// 获取原始的强度信息和高程信息;
		uheight = ptrRowHeight[n];
		uIntensity = ptrRowIntensity[n];

		// 时间计算;
		if (nSubFrameIndex > 0)
		{
			tempAddTime = camSynInfo.vecRowUsecond[nSubFrameIndex - 1] * 0.000001;
		}
		else
		{
			tempAddTime = 0.0;
		}

		// 计算获得GPS时间值;
		gpsTime = tempGpsTime + tempAddTime - m_offset_time;
		lastRow = curRow;

		isPointIn = isTimeInVector(gpsTime, m_vec_combine_time_range);
		if (!isPointIn || uIntensity < 12) // 
		{
			continue;
		}

		// 计算坐标信息;
		pointInfo.timeSecond = gpsTime;
		pointInfo.x = m_MatrxX[uheight][curCol] * 0.001;
		pointInfo.y = 0.0;
		pointInfo.z = 0.0 - m_MatrxZ[uheight][curCol] * 0.001;

		pointInfo.intensity = uIntensity;
		if (!pointInfo.isValid())
		{
			continue;
		}

		out3dPt.x = pointInfo.x;
		out3dPt.y = pointInfo.y;
		out3dPt.z = pointInfo.z;

		return true;

		//pionts[nPtCount] = pointInfo;
		//nPtCount++;
	}

	return false;
}

QString hnPavementCamReader::getDatPathByImgNo( int imgNo )
{
	// 确定当前帧所在的文件夹及文件夹内帧编号;
	QString strResultDatPath = "";
	int imgDirNo = imgNo / 1000;
	int curImgNo = imgNo % 1000;

	// 根据当前cam文件名，顺序查找第一个dat所在文件夹路径;
	std::string strDatFilePath = m_strCamFilePath;
	int nPos = strDatFilePath.find_last_of('/');
	std::string strCamDirPath = strDatFilePath.substr(0,nPos+1); // 截取前段文件名;

	// 获取img文件夹路径;
	std::string strDatDir = strCamDirPath;
	char strDirTemp[1024];
	sprintf_s(strDirTemp,"%s/Image_%04d",strCamDirPath.data(),imgDirNo);
	std::string sstrCurDatDirPath = strDirTemp;

	// 文件存在则记录;
	std::string strFirstDatFilePath = getDatPathByNum(sstrCurDatDirPath.data(),curImgNo);
	if (_access(strFirstDatFilePath.data(),0) == 0)
	{
		strResultDatPath = QString::fromLocal8Bit(strFirstDatFilePath.data());
	}

	return strResultDatPath;
}

void hnPavementCamReader::serializeDatDataFrame( PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,unsigned char* ptrRowTimeStamp, std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count,int leftOrRight )
{
	if (!ptrRowHeight || !ptrRowIntensity || !ptrRowTimeStamp)
	{
		return;
	}

	// 计算当前帧第零行的时间值;
	double gpsTime = 0.0;
	double tempGpsTime = 0.0;
	tempGpsTime = camSynInfo.nGpsSecond + camSynInfo.nMsecond / 1000.0 + camSynInfo.nUsecond / 1000000.0;

	// 计算单点坐标;
	bool isPointIn = false;
	static int lastRow = 0;
	double tempAdd = 0.0;
	double tempAddTime = 0.0;
	int curCol = 0;
	int curRow = 0;
	unsigned short uheight = 0;
	unsigned char uIntensity = 0;
	int nPtCount = 0;

	int usePtCount = 2560;
	pionts.resize(usePtCount);
	POINT_STRUCT_XYZIT_INFO pointInfo;
	POINT_STRUCT_XYZIT_INFO tmppointInfo;
	for (int n = 0;n < usePtCount;n++)
	{
		// 获取到当前行;
		curRow = n / 2560;
		curCol = n % 2560;

		// 获取原始的强度信息和高程信息;
		uheight = ptrRowHeight[n];
		uIntensity = ptrRowIntensity[n];

		// 时间计算;
		if (curRow > 0)
		{
			tempAddTime = camSynInfo.vecRowUsecond[curRow-1] * 0.000001;
		}
		else
		{
			tempAddTime = 0.0;
		}

		// 计算获得GPS时间值;
		gpsTime = tempGpsTime + tempAddTime - m_offset_time;
		lastRow = curRow;

		isPointIn = isTimeInVector(gpsTime,m_vec_combine_time_range);
		if (!isPointIn || uIntensity < 12) // 
		{
			continue;
		}

		// 计算坐标信息;
		pointInfo.timeSecond = gpsTime;
		pointInfo.x = m_MatrxX[uheight][curCol] * 0.001;
		pointInfo.y = 0.0;
		pointInfo.z = 0.0 - m_MatrxZ[uheight][curCol] * 0.001;

		pointInfo.intensity = uIntensity;
		if (!pointInfo.isValid())
		{
			continue;
		}

		// 判断取值要求,为-1表示取中心点左侧，为1表示取中心点右侧，为0表示全取;
		if (leftOrRight < 0 )
		{
			if ( curCol < 1280 )
			{
				pionts[nPtCount] = pointInfo;
				nPtCount++;
			}
		}
		else if (leftOrRight > 0)
		{
			if ( curCol > 1280 )
			{
				pionts[nPtCount] = pointInfo;
				nPtCount++;
			}
		}
		else
		{
			pionts[nPtCount] = pointInfo;
			nPtCount++;
		}

	}

	return_pt_count = nPtCount;
}

void hnPavementCamReader::setFilterJump( int jumpLines )
{
	m_jumpLines = jumpLines;
}
