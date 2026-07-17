#ifndef HNPAVEMENT_CAM_PCDREADER_H
#define HNPAVEMENT_CAM_PCDREADER_H
#include <vector>
#include "hnrawpcdreader.h"
#include <QString>
#include "..\hnCommon\hn3dPointDef.h"
using namespace hnCommon;

struct PAVEMENT_CAM_SYN_INFO
{
	PAVEMENT_CAM_SYN_INFO()
	{
		curIndex = 0;
		nGpsWeek = nGpsSecond = nMsecond = nUsecond = 0;
	}
	int curIndex;
	int nGpsWeek;
	int nGpsSecond;
	int nMsecond;
	int nUsecond;
	int nDmiValue;

	std::vector<int> vecRowUsecond;
};

class HNPAVEMENTCREATE3D_EXPORT hnPavementCamReader :
	public hnRawPcdReader
{
public:
	hnPavementCamReader(void);
	~hnPavementCamReader(void);

	// 打开文件读取,重载实现;
	virtual bool Open(const char* path);

	// 关闭文件，重载实现;
	virtual bool Close();

	// 开始读取，重载实现;
	virtual void startRead();

	// 读取一帧数据(40*2560)，重载实现;
	virtual bool getLinePoints(int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count);

	// 读取单帧数据（2560个点），根据给定跳转帧数进行跳转（可视为抽稀读取），本接口用于高速路不同车道间高程平差调整;
	bool getLinePointsWithJump(int idex,int jumpLines,int leftOrRight,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count);

	// 读取指定帧数据（2560个点）;
	bool getSubFramePoints(int mainFrameIndex,int subFrameIndex,int leftOrRight,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count);
	// 几何顺序计算专用。默认关闭，避免改变现有随机读取调用方的行为。
	void setSubFrameCacheEnabled(bool enabled) { m_subFrameCacheEnabled = enabled; m_cachedSubFrameDatIndex = -1; m_cachedSubFrameRawData.clear(); }

	// 读取指定点的坐标 nImageIndex-影像索引 subFrameIndex 当前影像内帧号， ;
	bool getFramePoints(int nImageIndex, int nSubFrameIndex, int nPtIndex, hn3dPointD& out3dPt);

	// 根据给定帧获取帧影像数据的文件路径;
	QString getDatPathByImgNo(int imgNo);

	// 获取点云总帧数，重载实现;
	virtual int GetScanLines();

	// 通用接口实现，获取扫描持续时间,重载实现;
	virtual bool getScanTimeRange(double& start_gps_time,double& end_gps_time);

	// 根据设置传入的时间信息获取距离该时间前后一定范围的帧数据索引，重载实现;
	virtual void getLinesIndex(std::vector<int>& vec_result_index,void (*processCallback)(float,const char*) = NULL);

	// 获取当前帧里程信息;
	bool getFrameDmiValue(int iFrame,double& curDmiValue);

	// 获取当前起始帧的时间值;
	bool getFrameTime(int iFrame, double& curTime);

	// 获取指定帧的时间
	bool getGpsTime(int nPcdFrame, double& dGpsTimer);

	// 获取边线数据接口，重载实现，仅支持3d路面数据重载实现;
	virtual bool getRoadLinesPoints( int idx,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count );

	// 获取全部的DAT影像数据路径;
	bool getFullDatPaths(std::vector<QString>& vecDatPaths);

	// 获取同步信息;
	std::vector<PAVEMENT_CAM_SYN_INFO>& getCamSynInfo(){return m_vecCamSynInfo;}

	// 设置同步信息并加载数据标定参数文件;
	void setCamSynInfoLoad(std::vector<PAVEMENT_CAM_SYN_INFO>& vecCamSynInfo,const char* path);

	// 设置分割段采用分割模式处理;
	void setSplitDatIndex(int start_index,int end_index);

	// 获取分割段起始、终止时间;
	void getSplitDatTime(double& start_gps_time,double& end_gps_time);

	// 跳转至分割起始位置处;
	bool jumpToSplitStartIndex();

	// 获取分割段的数据帧信息;
	void getSplitLineIndex(std::vector<int>& vec_result_index,void (*processCallback)(float,const char*) = NULL);

	// 设置跳转读取阈值，支持10,20,30,40,用于抽稀读取;
	void setFilterJump(int jumpLines);
private:
	// 读取同步文件并记录到内存中;
	bool synCamData(std::vector<PAVEMENT_CAM_SYN_INFO>& vecCamSynInfo);

	// 传入文件名后半部分，判断该文件是否存在，若存在则返回该文件绝对路径;
	std::string getDatPathByNum(const char* strCurDatDir,int iNo);

	// 从cam文件中读取100帧同步数据并解析ptrRowHeight,ptrRowIntensity外部申请内存，即一次读取完成一个dat文件;
	bool readNextCamSynData( unsigned short** ptrRowHeight,unsigned char** ptrRowIntensity,unsigned char** ptrRowTimeStamp );

	// 从cam文件中读取全部dat数据;
	bool readNextCamSynDataNew( unsigned short** ptrRowHeight,unsigned char** ptrRowIntensity,unsigned char** ptrRowTimeStamp );

	// 解析dat,默认40*2560个点协议信息;
	void serializeDatData( PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,unsigned char* ptrRowTimeStamp,
		std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count,int usePtCount = 102400);

	// 解析一帧2560个点协议信息,leftOrRight为-1表示取左侧，为0表示全部取，为1表示取右侧;
	void serializeDatDataFrame( PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,unsigned char* ptrRowTimeStamp,
		std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count,int leftOrRight);

	// 解析dat包协议信息，仅输出边缘点数据;
	void serializeRoadDatData(PAVEMENT_CAM_SYN_INFO& camSynInfo, unsigned short* ptrRowHeight,unsigned char* ptrRowIntensity ,unsigned char* ptrRowTimeStamp,std::vector<POINT_STRUCT_XYZIT_INFO>& pionts,int& return_pt_count);

	// 检查该时间是否在容器内;
	bool isTimeInVector(double gps_time,std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range);

	// 首尾传入检查,可能存在给定的时间段范围很小，在某一段内开始，在这一段内结束的情况;
	bool isTimeOrInVector(double start_time, double end_time, std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range);

	// 读取相机标定文件，并存储在内存中;
	bool readCamMatrix();
private:
	// 记录cam文件绝对路径;
	std::string m_strCamFilePath;

	// 记录当前正在使用中的DAT所在文件夹路径;
	std::string m_strCurDatDirPath;
	int m_iCurNo; // 记录当前文件夹读取的哪一个数据文件；
	int m_iCurDirNo; // 记录读取的是哪一个当前文件夹;

	FILE* m_ptrCamFile;
	FILE* m_ptrDatFile;

	//// 申请内存数据对象;
	//char* m_strOrientData;
	//char* m_strOrientDataNew;

	// 原始数据记录指针;
	int m_nTestCount;

	bool m_bJumpRead;
	bool m_bNeedRead;   // 标记是否需要从文件中读取下一个数据文件;
	int m_nCurCamIndex; // 记录当前同步数据获取的索引值;
	int m_nCurReadIndex; // 记录当前读取的100 * 40数据对应到哪一份;
	int m_cachedSubFrameDatIndex; // getSubFramePoints顺序读取时已解析的DAT编号
	bool m_subFrameCacheEnabled;
	std::vector<char> m_cachedSubFrameRawData; // 顺序几何计算仅缓存Snappy解压数据，按需解析断面
	unsigned char** m_ptrRowTimeStamp; // 记录的行时间戳信息;
	unsigned short** m_ptrRowHeight; // 记录的行高程值;
	unsigned char** m_ptrRowIntensity; // 记录的行强度值;
	double m_testData;

	// 标定参数;
	double** m_MatrxX;
	double** m_MatrxZ;

	// 记录全段的同步参数信息;
	std::vector<PAVEMENT_CAM_SYN_INFO> m_vecCamSynInfo;

	// 分割时间段;
	int m_split_start_index;
	int m_split_end_index;

	// jump读取帧数;
	int m_jumpLines;
};

#endif // HNPAVEMENT_CAM_PCDREADER_H
