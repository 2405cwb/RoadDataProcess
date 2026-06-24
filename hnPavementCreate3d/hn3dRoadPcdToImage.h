#ifndef HN3DPAVEMENT_TO_IMAGE_H
#define HN3DPAVEMENT_TO_IMAGE_H
#include <string>
#include <vector>
#include "Eigen\Dense"
#include "hnPavementImageInfo.h"
#include <QObject> 
#include "hnProjectSetting.h"
#include "hnpavementcreate3d_global.h"
#include "..\hd3DEngine\include\line3d.h"
//#include "..\hnConvert\hnDataCombineStructInfo.h"

class hnPavementCamReader;
class IHdPJTranslator;

class HNPAVEMENTCREATE3D_EXPORT hn3dRoadPcdToImage : public QObject
{
	Q_OBJECT
public:
	hn3dRoadPcdToImage(QObject* parent = 0);
	~hn3dRoadPcdToImage();

	//void testConvertImg(const char* strOld, const char* strNew);

	// 设置数据库信息;
	void setDbPath(const char* strDbPath);

	// 设置灰度图像存储位置;
	void setGreyImageDirPath(const char* strGreyImgDirPath);

	// 设置索引文件存储位置;
	void setIndexFilePath(const char* strIndexFilePath);

	// 设置深度图像存储位置;
	void setDepthImageDirPath(const char* strDepthImgDirPath);

	// 设置road图像存储路径;
	void setRoadImageDirPath(const char* strRoadImgDirPath);

	// 设置传入路面相机同步文件信息，用于读取点云数据;
	void set3dPavementCamFilePath(const char* strCamPavementFilePath);

	// 传入POS数据;
	void setPosData(std::vector<POS_STRUCT_INFO>& vecPosInfo);

	// 设置需要创建影像类别;
	void setNeedCreateGrey(bool bNeed);
	void setNeedCreateRgb(bool bNeed);
	void setNeedCreateRoad(bool bNeed);

	// 创建灰度影像,传入影像长宽及像素对应比例尺，无点坐标处采用四临域插值，影像像素长度应为40的整数倍;
	bool createGreyImage(float widthScale,float heightScale, int width = 4000, int height = 2560);
	void setSystemSetting(hn::hnProjectSetting* setting);

	// 写入索引信息文件;
	bool createIndexFile(QString strSaveIndexPath);

	// 设置文件读指针;
	void setPavementReader(hnPavementCamReader* pavementReader);

	// 设置分段读取索引信息生成灰度图和深度图;
	bool createSplitImgs(int startIndex,int endIndex);

	// 设置终止;
	void stopWork();

	//1cm内的色阶数，色阶数越高，适用病害等级越小
	int num_colorsIn1Centimeter;

signals:
	// 信号标记，主要用于进度处理;
	void progress(float p,QString msg);

public:
	// 更新POS构建的矩阵;
	void updatePosMatrix(POS_STRUCT_INFO& posInfo);

	//5通过插值计算获取点时间对应的POS点位置、姿态信息，采用线性插值计算;根据时间查询距离该点时间最近的记录值索引;
	bool linearInsertPos(double gpsTime, std::vector<POS_STRUCT_INFO>& vecInfo, POS_STRUCT_INFO& insertResult, int& nearestIndex);

	// 计算绝对坐标;
	void calcuCoord(POS_STRUCT_INFO& posInfo, POINT_STRUCT_XYZIT_INFO& point, double& dx, double& dy, double& dz);


	

private:
	// 更新进度条;
	void updateProgress(float p,QString msg);

	// 创建灰度影像;
	bool getGreyImage(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult, int Para_color);

	// 创建灰度影像-new,根据路面实际投影拟合线后计算点到线的距离构建深度图;
	bool getGreyImageNewDepth(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult);

	// 接口分封，生成灰度影像;
	bool mkGreyImage(int* imgIntensity, double startDmiValue, double endDmiValue );

	// 接口分封，生成深度影像;
	bool mkRgbImage(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert);
	bool mkRgbImage2(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert, int Para_color);

	// 新封装，每帧（行）数据拟合直线，计算点到直线的距离，构建生成深度图;
	bool mkRgbImageFit(int* imgIntensity, double* imgDepth, double startDmiValue, double endDmiValue, double updateWidthScaleInvert);

	// 用于大范围渲染显示范围沉降，构建生成深度图;
	bool mkRgbImageNewFit(std::vector<irr::core::line3dd>& vecFitLines, hnPoint3d* ptrPoint3d, int* imgIntensity, double startDmiValue, double endDmiValue,
		POINT_STRUCT_XYZIT_INFO& ptZeroStart,POINT_STRUCT_XYZIT_INFO& ptZeroEnd, double updateWidthScaleInvert);

	// 根据时间范围更新POS容器;
	bool updatePosByTimeRange(std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range,std::vector<POS_STRUCT_INFO>& vec_pos_info,std::vector<POS_STRUCT_INFO>& vecSubPosInfo);

	// 获取相机数据在零点位置的投影轨迹线，用于作为拟合基准线;
	void mkPosFitLine(std::vector<POS_STRUCT_INFO>& vec_pos_info, std::vector<hnPoint3d>& vec_result_ptline);

	// 根据POS数据获取零点位置的投影轨迹线，并进行抛物线拟合进行POS数据拆解;
	void mkLaserRouteLine( std::vector<POS_STRUCT_INFO>& vec_pos_info );

	//3-2按iscan旋转角度方式Z-X-Y构建旋转矩阵;
	void computeMatrixByIScanAngle(double X, double Y, double Z, double Yaw, double Pitch, double Roll, double *R);

	//5-1 根据时间查询距离该点时间最近的记录值索引;
	int findIndexByGpsTime(double gpsTime, std::vector<POS_STRUCT_INFO>& vecInfo);

	// 计算点到线段的投影距离并判断该点在线段的左侧还是右侧;
	int getClosetPointDistToLine(POINT_STRUCT_XYZIT_INFO& point, POINT_STRUCT_XYZIT_INFO& startPt, POINT_STRUCT_XYZIT_INFO& endPt, double& distToLine, double& distToStart);

	// 求已知到直线距离的坐标点;
	int getPointByDistToLine(POINT_STRUCT_XYZIT_INFO& point, POINT_STRUCT_XYZIT_INFO& startPt, POINT_STRUCT_XYZIT_INFO& endPt, double distToLine,POINT_STRUCT_XYZIT_INFO& resultPt);

	// 测试强度校正;
	void updateIntensity( int* imgIntensity );

	// 检查该时间是否在容器内;
	bool isTimeInVector(double gps_time,std::vector<COMBINE_TIME_RANGE>& vec_combine_time_range);

	// POS当地水平参考坐标系到WGS84空间直角坐标系转换矩阵 3* 3；
	void computeMatrixByBL( double X,double Y,double Z,double longtitude,double latitude,double *R);

	bool calcIndexData(hnPavementCamReader* ptrCamReader, int iStartFrame, int iEndFrame, PAVEMENT_IMAGE_INDEX& indexResult);

	// 对多段线进行化简;
	bool SimplifyPolyline(std::vector<hnPoint3d>& polyline, double dfDisThreshold);

	// 对多段线进行平滑;
	bool SmoothPolyline(std::vector<hnPoint3d>& polyline);

	// 点到直线距离;
	double DistancePointToLine(double x1, double y1, double x2, double y2, double x, double y);

	// 点到多线段集的最优投影点;
	void PointInPlines(POINT_STRUCT_XYZIT_INFO& point, std::vector<irr::core::line3dd>& vec_fit_laser_ptline, POINT_STRUCT_XYZIT_INFO& resultPt);

	// z=a*(x^2+y^2)+b*sqrt(x^2+y^2)+c//空间曲线;
	void polynomial3D_fitting(std::vector<double>& x, std::vector<double>& y, std::vector<double>& z, double &a, double &b, double &c);

	//y=kx+b//平面直线;
	void line_fitting(std::vector<double>& x, std::vector<double>& y, double &k, double &b);

	// 统计强度范围，对灰度信息进行拉升，增亮图像;
	void enhanceIntensity(int* ptrIntensity);
private:
	// 存储DB文件路径;
	std::string m_strDbPath;

	// 存储灰度影像数据文件路径;
	std::string m_strGreyImageDirPath;

	// 存储深度影像数据文件路径;
	std::string m_strDepthImageDirPath;

	// 存储道路线形图数据文件夹路径;
	std::string m_strRoadImageDirPath;

	// 路面点云文件路径;
	std::string m_strPavementCamPath;

	//// POS文件路径;
	//std::string m_strPosPath;

	// 存储索引文件路径;
	std::string m_strIndexFilePath;

	// 灰度影像长宽信息等;
	float m_widthScale;
	float m_heightScale;
	int m_imgWidth;
	int m_imgHeight;

	// 外部控制需要生成灰度图/深度图;
	bool m_bNeedCreateGrey;
	bool m_bNeedCreateRgb;
	bool m_bNeedCreateRoad;

	// 内存记录POS信息;
	std::vector<POS_STRUCT_INFO> m_vecPosInfo;

	// 内存记录POS信息;
	std::vector<POS_STRUCT_INFO> m_vecSubPosInfo;

	// 矩阵记录原始激光数据到POS的旋转矩阵（来自于DB）;
	Eigen::Matrix<double, 4, 4> m_mat_laser_to_pos;

	// 矩阵记录POS到WGS84的旋转矩阵;
	Eigen::Matrix<double, 4, 4> m_mat_pos_to_world;

	// 进度条使用，无实际意义;
	int m_nSubIndex;
	int m_nTotalIndex;

	// 融合设置相关参数;
	hn::hnProjectSetting* m_project_setting;
	IHdPJTranslator* m_ptr_convert_translator;

	// 原始数据文件读指针;
	hnPavementCamReader* m_pavement_reader;

	// 分段处理时设置时间过滤信息，用于减少POS查询遍历耗时;
	std::vector<COMBINE_TIME_RANGE> m_vec_combine_time_range;

	// 设置中断;
	bool m_is_running;

	// 规整化POS轨迹，用于平面插值记录;
	std::vector<irr::core::line3dd> m_vec_fit_laser_ptline;

	// 记录平均高度值，作为基准;
	double m_averHeight;

	// 中间变量申请内存，避免频繁new耗时;
	hnPoint3d* m_ptrCoord3d;

	//// 拟合用;
	//double* m_imgDepthFit;

	// 用于记录影像对应像素;
	int* m_imgIntensity;
	double* m_imgDepth;
	double* m_DistPo2Pl; // 生成深度图记录点到拟合平面的距离;

	// 空间曲线参数;
	double m_a_3d;
	double m_b_3d;
	double m_c_3d;
	double m_k_line;
	double m_b_line;

	
};

#endif // HN3DPAVEMENT_TO_IMAGE_H