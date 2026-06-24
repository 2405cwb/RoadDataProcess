#ifndef HN3DPAVEMENT_IMAGE_ROUTE_H
#define HN3DPAVEMENT_IMAGE_ROUTE_H
#include "hnPavementImageInfo.h"
#include <string>
#include <vector>
#include "..\hnCommon\hn2dPointDef.h"

using namespace hnCommon;

class hnPavementImageRoute
{
public:
	hnPavementImageRoute();
	~hnPavementImageRoute();

	// 加载工程;
	bool loadScanRoute(const char* path);

	// 清理;
	void clear();

	// 获取固定的影像索引文件路径;
	std::string getImageIndexFilePath();

	// 获取对应扫描头点云数据路径;
	std::string getHlzPath(int scanno = 1);

	// 获取对应扫描头LIN文件路径;
	std::string getLinPath(int scanno = 1);

	// 获取影像索引值对应的灰度影像路径;
	std::string getGreyImagePath(int iIndex);

	// 获取影像索引值对应的深度影像路径;
	std::string getRgbImagePath(int iIndex);

	// 获取影像索引值对应的道路线形图
	std::string getRoadLineImagePath(int iIndex);

	// 获取影像索引总帧数;
	int getImageIndexCount();

	// 坐标转换，根据影像索引及影像像素坐标转换三维平面坐标（无高程信息）;
	bool convert2dCoordTo3dCoord(int iIndex, int row, int col, double& resultX, double& resultY);

	// 坐标转换，根据影像名称及影像像素坐标转换三维平面坐标（无高程信息）;
	bool convert2dCoordTo3dCoord(const char* strImageNam, int row, int col, int& nHdi,double& resultX, double& resultY);

	// 坐标转换，根据三维平面坐标进行投影转换至二维像素坐标及其所在影像索引值;
	bool convert3dCoordTo2dCoord(double dCoordX, double dCoordY, int& iIndex, int& row, int& col);

	// 根据灰度图像名查找对应的索引ID,未查找到返回-1;
	int getIndexByGreyName(const char* strGreyImgName);

	// 根据深度图像名查找对应的索引ID，未查找到返回-1;
	int getIndexByRgbName(const char* strRgbImgName);

	// 获取轨迹点信息
	bool getHdiInfo(int iIndex, PAVEMENT_IMAGE_INDEX& hdi);

	std::string findImg(std::string strDir, const char* strPre);

	std::string getRoadImagePathByIndex( int iIndex, int iSubIndex );

	bool convert3dCoordTo2dCoordExt(double dCoordX, double dCoordY, int iIndex, int& row, int& col);

private:
	// 计算点到线段的投影距离并判断该点在线段的左侧还是右侧;
	int getClosetPointDistToLine(hn2dPointD& point, hn2dPointD& startPt, hn2dPointD& endPt, double& distToLine, double& distToStart);

	// 加载路面影像信息
	bool loadRoadImage(const char* strSynFile);

	// 获取当前灰度图对应的路面影像数据
	bool getRoadImageByHdi(double dBegGps, double dEndGps, std::vector<int>& vecRoadIndex);

private:
	//! 轨迹工程所在目录;
	std::string m_folder;

	//! 轨迹名称;
	std::string m_routeName;

	//! 灰度图影像信息
	std::vector<PAVEMENT_IMAGE_INDEX> m_vecImageInfo;

	//! 路面影像信息
	std::vector<ROAD_IMAGE_INDEX> m_vecRoadImageInfo;
};

#endif