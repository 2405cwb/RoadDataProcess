#include "hnPavementImageRoute.h"
#include <io.h>
#include <algorithm>
#include <Windows.h>
#include <vector>
#include <string>

using namespace std;

#define MAX_PATH          260
#define IMAGE_FOLDER ("Image") 
#define POINTCLOUD_FOLDER ("PointCloud") 
#define IMAGE_INDEX_NAME ("Pavement-cam-1")
#define IMAGE_INDEX_EXT (".idx")
#define SEAPCD_EXT  (".hlz")
#define SCANPOS_EXT  (".lin")
#define SCAN_PREF	("iScan-Pcd-")//iScan
#define SCANPOS_PREF	("iScan-Pos-")//iScan
#define GREY_IMAGE_FOLDER  ("灰度图")
#define GREY_IMAGE_PRE ("GREY")
#define RGB_IMAGE_FOLDER  ("深度图")
#define RGB_IMAGE_PRE ("RGB")
#define ROADLINE_IMAGE_FOLDER ("道路线形图")
#define ROADLINE_IMAGE_PRE ("ROAD")
#define ROAD_IMAGE_FOLDER ("RoadImg")
#define ROAD_IMAGE_SUBFOLDER ("Camera0")

hnPavementImageRoute::hnPavementImageRoute()
{
	m_folder = "";
	m_routeName = "";
	m_vecImageInfo.clear();
}


hnPavementImageRoute::~hnPavementImageRoute()
{
}

bool hnPavementImageRoute::loadScanRoute(const char* path)
{
	// 条件判断;
	if (path == NULL)
		return false;
	clear();

	// 工程路径信息记录;
	m_folder = path;
	if (m_folder == "" || m_folder.find_first_of('\\') == -1)
		return false;
	if (m_folder.substr(m_folder.length() - 1) != "\\")
	{
		m_folder += "\\";
	}

	std::string strImgIndexPath = getImageIndexFilePath();
	if (strImgIndexPath == "")
	{
		return false;
	}

	FILE* pIndexFile = fopen(strImgIndexPath.c_str(), "rt");
	if (!pIndexFile)
	{
		return false;
	}

	// 读取影像索引信息数据;
	bool bSucc = false;
	char strLine[1024];
	int nIndexCount = 0;
	m_vecImageInfo.resize(5000);
	while (!feof(pIndexFile))
	{
		memset(strLine, 0, 1024);
		fgets(strLine, 1024, pIndexFile);

		PAVEMENT_IMAGE_INDEX info;
		bSucc = info.serialize(strLine);
		if (!bSucc)
		{
			continue;
		}

		m_vecImageInfo[nIndexCount] = info;
		nIndexCount++;

		if (nIndexCount >= m_vecImageInfo.size())
		{
			m_vecImageInfo.resize(m_vecImageInfo.size() + 5000);
		}
	}
	fclose(pIndexFile);
	m_vecImageInfo.resize(nIndexCount);

	// 加载路面影像同步文件
	char strImgDir[MAX_PATH];
	sprintf_s(strImgDir,"%s\\%s\\SYN\\trigger.txt",m_folder.data(), ROAD_IMAGE_FOLDER);
	loadRoadImage(strImgDir);

	return true;
}

void hnPavementImageRoute::clear()
{
	m_folder = "";
	m_routeName = "";
	m_vecImageInfo.clear();
}

std::string hnPavementImageRoute::getImageIndexFilePath()
{
	char strImgIndex[MAX_PATH];
	sprintf(strImgIndex, "%s%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, IMAGE_INDEX_NAME, IMAGE_INDEX_EXT);
	if (_access(strImgIndex, 0) == -1)
		return "";
	else
		return strImgIndex;
}

std::string hnPavementImageRoute::getHlzPath(int scanno /*= 1*/)
{
	char strPcdIndex[MAX_PATH];
	sprintf(strPcdIndex, "%s%s\\%d\\%s%d%s", m_folder.c_str(), POINTCLOUD_FOLDER,scanno, SCAN_PREF, scanno, SEAPCD_EXT);
	if (_access(strPcdIndex, 0) == -1)
		return "";
	else
		return strPcdIndex;
}

std::string hnPavementImageRoute::getLinPath(int scanno /*= 1*/)
{
	char strLinIndex[MAX_PATH];
	sprintf(strLinIndex, "%s%s\\%d\\%s%d%s", m_folder.c_str(), POINTCLOUD_FOLDER, scanno, SCANPOS_PREF, scanno, SCANPOS_EXT);
	if (_access(strLinIndex, 0) == -1)
		return "";
	else
		return strLinIndex;
}

std::string hnPavementImageRoute::getGreyImagePath(int iIndex)
{
	// 条件判断;
	std::string strImgPath = "";
	if (iIndex < 0 || iIndex > m_vecImageInfo.size())
	{
		return "";
	}

	// 构建影像路径名称;
	PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];
	char str[MAX_PATH];
	sprintf(str, "%s%s\\%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, GREY_IMAGE_FOLDER, GREY_IMAGE_PRE, indexInfo.strImgName);
	if (_access(str, 0) == -1)
	{
		string strName = indexInfo.strImgName;
		if (strName.find_last_of(".") != -1)
		{
			strName = strName.substr(0, strName.find_last_of(".")) + ".jpg";
			sprintf(str, "%s%s\\%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, GREY_IMAGE_FOLDER, GREY_IMAGE_PRE, strName);
			if (_access(str, 0) == -1)
			{
				return "";
			}

			return str;
		}

		return "";
	}
	else
	{
		return str;
	}
}

std::string hnPavementImageRoute::getRgbImagePath(int iIndex)
{
	// 条件判断;
	if (iIndex < 0 || iIndex > m_vecImageInfo.size())
	{
		return "";
	}

	// 构建影像路径名称;
	PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];
	char str[MAX_PATH];
	sprintf(str, "%s%s\\%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, RGB_IMAGE_FOLDER, RGB_IMAGE_PRE, indexInfo.strImgName);
	if (_access(str, 0) == -1)
		return "";
	else
		return str;
}

// 获取影像索引值对应的道路线形图
std::string hnPavementImageRoute::getRoadLineImagePath(int iIndex)
{
	// 条件判断;
	if (iIndex < 0 || iIndex > m_vecImageInfo.size())
	{
		return "";
	}

	// 构建影像路径名称;
	PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];
	char str[MAX_PATH];
	sprintf(str, "%s%s\\%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, ROADLINE_IMAGE_FOLDER, ROADLINE_IMAGE_PRE, indexInfo.strImgName);
	if (_access(str, 0) == -1)
	{
		string strName = indexInfo.strImgName;
		if (strName.find_last_of(".") != -1)
		{
			strName = strName.substr(0, strName.find_last_of(".")) + ".jpg";
			sprintf(str, "%s%s\\%s\\%s%s", m_folder.c_str(), IMAGE_FOLDER, ROADLINE_IMAGE_FOLDER, ROADLINE_IMAGE_PRE, strName);
			if (_access(str, 0) == -1)
			{
				return "";
			}

			return str;
		}

		return "";
	}
	else
	{
		return str;
	}
}

int hnPavementImageRoute::getImageIndexCount()
{
	return (int)m_vecImageInfo.size();
}

bool hnPavementImageRoute::convert2dCoordTo3dCoord(int iIndex, int row, int col, double& resultX, double& resultY)
{
	// 条件判断;
	bool bSucc = false;
	if (iIndex < 0 || iIndex > m_vecImageInfo.size())
	{
		return false;
	}

	// 获取当前帧索引信息;
	PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];

	// 由于生成图片时将图像反转，此处需要调整;
	int curRow = row;
	int curCol = col;

	// 图像像素坐标转换三维坐标尺度因素;
	double widthDist = sqrt( (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) );
	double heightDist = sqrt( (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) * (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) + (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) * (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) );
	double updateWidthDist = widthDist * curRow / indexInfo.nImgWidth;
	double updateHeightDist = heightDist * curCol / indexInfo.nImgHeight;

	// 根据垂线斜率计算角度;
	double dx = indexInfo.upRightPt.x - indexInfo.upLeftPt.x;
	double dy = indexInfo.upRightPt.y - indexInfo.upLeftPt.y;
	double angleK = atan2( indexInfo.downRightPt.y - indexInfo.upRightPt.y, indexInfo.downRightPt.x - indexInfo.upRightPt.x ); // 为列向斜率,其行向斜率应垂直于此;
	double angleKInvert = atan2(dy, dx); // 为行向斜率;

	// 坐标原点在左上角处;
	double tempX = updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
	double tempY = updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
	resultX = indexInfo.upLeftPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
	resultY = indexInfo.upLeftPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);

	return true;
}

// 坐标转换，根据影像名称及影像像素坐标转换三维平面坐标（无高程信息）;
bool hnPavementImageRoute::convert2dCoordTo3dCoord(const char* strImageNam, int row, int col,  int& nHdi, double& resultX, double& resultY)
{
	int iIndex = -1;

	// 条件判断;
	bool bSucc = false;
	string strName = "";
	for (int i = 0; i < m_vecImageInfo.size(); i++)
	{
		strName = m_vecImageInfo[i].strImgName;
		strName = strName.substr(0, strName.find_last_of("."));
		if (strName != strImageNam)
		{
			continue;
		}

		iIndex = i;
		break;
	}

	if (iIndex < 0 || iIndex > m_vecImageInfo.size())
	{
		return false;
	}

	nHdi = iIndex;

	// 获取当前帧索引信息;
	PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];

	// 由于生成图片时将图像反转，此处需要调整;
	int curRow = row;
	int curCol = col;

	// 图像像素坐标转换三维坐标尺度因素;
	double widthDist = sqrt( (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) );
	double heightDist = sqrt( (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) * (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) + (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) * (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) );
	double updateWidthDist = widthDist * curRow / indexInfo.nImgWidth;
	double updateHeightDist = heightDist * curCol / indexInfo.nImgHeight;

	// 根据垂线斜率计算角度;
	double dx = indexInfo.upRightPt.x - indexInfo.upLeftPt.x;
	double dy = indexInfo.upRightPt.y - indexInfo.upLeftPt.y;
	double angleK = atan2( indexInfo.downRightPt.y - indexInfo.upRightPt.y, indexInfo.downRightPt.x - indexInfo.upRightPt.x ); // 为列向斜率,其行向斜率应垂直于此;
	double angleKInvert = atan2(dy, dx); // 为行向斜率;

	// 坐标原点在左上角处;
	double tempX = updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
	double tempY = updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
	resultX = indexInfo.upLeftPt.x + updateWidthDist * cos(angleKInvert) + updateHeightDist * cos(angleK);
	resultY = indexInfo.upLeftPt.y + updateWidthDist * sin(angleKInvert) + updateHeightDist * sin(angleK);
}

bool hnPavementImageRoute::convert3dCoordTo2dCoord(double dCoordX, double dCoordY, int& iIndex, int& row, int& col)
{
	// 先查找当前帧影像;
	iIndex = -1;
	bool bFind = false;
	int isIn = 0;
	hn2dPointD curPt;
	curPt.x = dCoordX;
	curPt.y = dCoordY;
	std::vector<hn2dPointD> vecPoly;
	double minX, minY, maxX, maxY;
	for (unsigned int n = 0;n < m_vecImageInfo.size();n++)
	{
		// 获取当前帧索引信息;
		PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[n];

		// 在矩形框内，参与计算判断;
		if ((indexInfo.minX <= curPt.x &&
			indexInfo.minY <= curPt.y &&
			indexInfo.maxX >= curPt.x &&
			indexInfo.maxY >= curPt.y))
		{
			// 找到当前帧;
			bFind = false;
			iIndex = n;

			// 获取当前帧索引信息;
			PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];

			// 中线起止点坐标;
			hn2dPointD ptZeroStart, ptZeroEnd;
			double colAddDelt, rowAddDelt;
			colAddDelt = rowAddDelt = 0.0;
			ptZeroStart.x = (indexInfo.upLeftPt.x + indexInfo.downLeftPt.x) / 2.0;
			ptZeroStart.y = (indexInfo.upLeftPt.y + indexInfo.downLeftPt.y) / 2.0;

			ptZeroEnd.x = (indexInfo.upRightPt.x + indexInfo.downRightPt.x) / 2.0;
			ptZeroEnd.y = (indexInfo.upRightPt.y + indexInfo.downRightPt.y) / 2.0;

			// 计算比例尺;
			double widthDist = sqrt((indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y));
			double heightDist = sqrt((indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) * (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) + (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) * (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y));
			double updateWidthScaleInvert = indexInfo.nImgWidth / widthDist;
			double updateHeightScaleInvert = indexInfo.nImgHeight / heightDist;

			// 计算没每个点到线段的投影值;
			getClosetPointDistToLine(curPt, indexInfo.upLeftPt, indexInfo.upRightPt, colAddDelt, rowAddDelt);

			// 计算在图像中的行列值，此处计算应会在拐弯处存在较明显的形变，应检查;
			int curCol = (int)(colAddDelt * updateHeightScaleInvert);
			//if (bOnRight > 0)
			//{
			//	// 零点在右侧，应该向左移动colAddDelt距离，即向其反方向移动;
			//	curCol = curCol + indexInfo.nImgHeight / 2;
			//}
			//else if (bOnRight <= 0)
			//{
			//	curCol = indexInfo.nImgHeight / 2 - curCol;
			//}

			int curRow = (int)(rowAddDelt * updateWidthScaleInvert);

			// 返回图像像素坐标;
			row = curRow;
			col = curCol;

			// 由于采用矩形框，会存在多余点在框内，需要判断是否在图像范围内;
			if (row >= 0 && row < indexInfo.nImgWidth && col >= 0 && col < indexInfo.nImgHeight)
			{
				// 在范围内，认为找到点，否则继续在下一帧中寻找;
				bFind = true;
				break;
			}
		}
	}

	return bFind;
}

int hnPavementImageRoute::getIndexByGreyName(const char* strGreyImgName)
{
	// 统一字符串小写;
	std::string strGreyName = strGreyImgName;
	std::transform(strGreyName.begin(), strGreyName.end(), strGreyName.begin(), tolower);

	// 遍历查找;
	int iIndex = -1;
	bool bFind = false;
	std::string strTemp = "";
	char str[MAX_PATH];
	for (unsigned int n = 0; n < m_vecImageInfo.size();n++)
	{
		PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[n];
		
		// 字符串比较;
		memset(str, 0, MAX_PATH);
		sprintf(str, "%s%s", GREY_IMAGE_PRE, indexInfo.strImgName);
		strTemp = str;
		std::transform(strTemp.begin(), strTemp.end(), strTemp.begin(), tolower);
		if (strcmp(strTemp.data(), strGreyName.data()) == 0)
		{
			bFind = true;
			iIndex = n;
			break;
		}
	}

	return iIndex;
}

int hnPavementImageRoute::getIndexByRgbName(const char* strRgbImgName)
{
	// 统一字符串小写;
	std::string strRgbName = strRgbImgName;
	std::transform(strRgbName.begin(), strRgbName.end(), strRgbName.begin(), tolower);

	// 遍历查找;
	int iIndex = -1;
	bool bFind = false;
	std::string strTemp = "";
	char str[MAX_PATH];
	for (unsigned int n = 0; n < m_vecImageInfo.size(); n++)
	{
		PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[n];

		// 字符串比较;
		memset(str, 0, MAX_PATH);
		sprintf(str, "%s%s", RGB_IMAGE_PRE, indexInfo.strImgName);
		strTemp = str;
		std::transform(strTemp.begin(), strTemp.end(), strTemp.begin(), tolower);
		if (strcmp(strTemp.data(), strRgbName.data()) == 0)
		{
			bFind = true;
			iIndex = n;
			break;
		}
	}

	return iIndex;
}

int hnPavementImageRoute::getClosetPointDistToLine(hn2dPointD& pt, hn2dPointD& startPt, hn2dPointD& endPt, double& distToLine, double& distToStart)
{
	hn2dPointD retVal;
	double dx = startPt.x - endPt.x;
	double dy = startPt.y - endPt.y;

	if (fabs(dx) < 0.00001 && fabs(dy) < 0.00001)
	{
		retVal = startPt;
		distToLine = 0.0;
		distToStart = 0.0;
		return 0;
	}

	// 计算投影点，并判断距离;
	double u = (pt.x - startPt.x)*(startPt.x - endPt.x) +
		(pt.y - startPt.y)*(startPt.y - endPt.y);
	u = u / (dx*dx + dy*dy);
	retVal.x = startPt.x + u*dx;
	retVal.y = startPt.y + u*dy;
	distToLine = sqrt((retVal.x - pt.x)*(retVal.x - pt.x) + (retVal.y - pt.y)*(retVal.y - pt.y));
	distToStart = sqrt((retVal.x - startPt.x)*(retVal.x - startPt.x) + (retVal.y - startPt.y)*(retVal.y - startPt.y));

	// 确定点在左侧还是右侧，-1为左侧，0在线上，1在右侧;
	int returnRt = 0;
	double tmp = (startPt.y - endPt.y) * pt.x + (endPt.x - startPt.x) * pt.y + startPt.x * endPt.y - endPt.x * startPt.y;
	if (tmp > 0.0)
	{
		returnRt = 1;
	}
	else if (tmp == 0.0)
	{
		returnRt = 0;
	}
	else
	{
		returnRt = -1;
	}

	return returnRt;
}

// 获取轨迹点信息
bool hnPavementImageRoute::getHdiInfo(int iIndex, PAVEMENT_IMAGE_INDEX& hdi)
{
	if (iIndex < 0 || iIndex >= m_vecImageInfo.size())
	{
		return false;
	}

	hdi =  m_vecImageInfo[iIndex];

	return true;
}

std::string hnPavementImageRoute::findImg(std::string strDir, const char* strPre)
{
	//WIN32_FIND_DATA findData;
	//HANDLE hError;
	//std::string strName = "";
	//std::string strPreName = "";
	//std::string strResult = "";

	//char FilePathName[MAX_PATH];

	//char FullPathName[MAX_PATH];
	//strcpy(FilePathName,strDir.data());
	//strcat(FilePathName, "*.jpg");
	//hError = FindFirstFile(FilePathName,&findData);
	//if (hError == INVALID_HANDLE_VALUE)
	//{
	//	return "";
	//}

	//if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
	//{
	//	// 仅查询本级文件夹，截取名称进行比较;
	//	strName = findData.cFileName;
	//	strPreName = strName.substr(0, 3);
	//	if (strcmp(strPreName.data(), strPre) == 0)
	//	{
	//		strResult = strDir + strName;
	//		return strResult;
	//	}
	//}


	//// 遍历查询;
	//while (::FindNextFile(hError,&findData))
	//{
	//	if (strcmp(findData.cFileName,".") == 0 || strcmp(findData.cFileName,"..") == 0 )
	//	{
	//		continue;
	//	}

	//	// 仅查询本级文件夹，截取名称进行比较;
	//	strName = findData.cFileName;
	//	strPreName = strName.substr(0,3);
	//	if (strcmp(strPreName.data(),strPre) == 0)
	//	{
	//		strResult = strDir + strName;
	//		break;
	//	}
	//}

	//return strResult;
	return "";
}

std::string hnPavementImageRoute::getRoadImagePathByIndex( int iIndex, int iSubIndex )
{
	if (iIndex < 0 || iIndex >= m_vecImageInfo.size())
	{
		return false;
	}

	// 按照起始终止时间查找对应的路面影像
	std::vector<int> vecSelRoadIndex;
	if (!getRoadImageByHdi(m_vecImageInfo[iIndex].dStartTime, m_vecImageInfo[iIndex].dEndTime, vecSelRoadIndex))
	{
		return false;
	}

	if (iSubIndex < 0 || iSubIndex >= vecSelRoadIndex.size())
	{
		return false;
	}

	iSubIndex = vecSelRoadIndex[iSubIndex];

	// 一张3D路面影像对应4张路面影像数据;
	int iRoadImageIndex = iIndex * 4 + iSubIndex;
	int iCurImgDirIndex = iRoadImageIndex / 1000;

	// 根据影像前缀进行查找;
	char strPreInfo[128];
	char strImgDir[MAX_PATH];
	sprintf_s(strImgDir,"%s\\%s\\%s\\%s_%04d\\",m_folder.data(), ROAD_IMAGE_FOLDER, ROAD_IMAGE_SUBFOLDER, IMAGE_FOLDER, iCurImgDirIndex);
	sprintf_s(strPreInfo,"%03d", iRoadImageIndex-iCurImgDirIndex*1000);

	// 遍历查询;
	std::string strPath = findImg(strImgDir, strPreInfo);
	return strPath;
}

bool hnPavementImageRoute::convert3dCoordTo2dCoordExt(double dCoordX, double dCoordY, int iIndex, int& row, int& col)
{
	if (iIndex < 0 || iIndex >= m_vecImageInfo.size())
	{
		return false;
	}

	// 先查找当前帧影像;
	iIndex = -1;
	bool bFind = false;
	int isIn = 0;
	hn2dPointD curPt;
	curPt.x = dCoordX;
	curPt.y = dCoordY;
	std::vector<hn2dPointD> vecPoly;
	//for (unsigned int n = 0; n < m_vecImageInfo.size(); n++)
	{
		// 获取当前帧索引信息;
		PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];

		//// 在矩形框内，参与计算判断;
		//if ((indexInfo.minX <= curPt.X &&
		//	indexInfo.minY <= curPt.Y &&
		//	indexInfo.maxX >= curPt.X &&
		//	indexInfo.maxY >= curPt.Y))
		{
			// 获取当前帧索引信息;
			PAVEMENT_IMAGE_INDEX& indexInfo = m_vecImageInfo[iIndex];

			// 中线起止点坐标;
			hn2dPointD ptZeroStart, ptZeroEnd;
			double colAddDelt, rowAddDelt;
			colAddDelt = rowAddDelt = 0.0;
			ptZeroStart.x = (indexInfo.upLeftPt.x + indexInfo.downLeftPt.x) / 2.0;
			ptZeroStart.y = (indexInfo.upLeftPt.y + indexInfo.downLeftPt.y) / 2.0;

			ptZeroEnd.x = (indexInfo.upRightPt.x + indexInfo.downRightPt.x) / 2.0;
			ptZeroEnd.y = (indexInfo.upRightPt.y + indexInfo.downRightPt.y) / 2.0;

			// 计算比例尺;
			double widthDist = sqrt((indexInfo.upLeftPt.x - indexInfo.upRightPt.x) * (indexInfo.upLeftPt.x - indexInfo.upRightPt.x) + (indexInfo.upLeftPt.y - indexInfo.upRightPt.y) * (indexInfo.upLeftPt.y - indexInfo.upRightPt.y));
			double heightDist = sqrt((indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) * (indexInfo.upLeftPt.x - indexInfo.downLeftPt.x) + (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y) * (indexInfo.upLeftPt.y - indexInfo.downLeftPt.y));
			double updateWidthScaleInvert = indexInfo.nImgWidth / widthDist;
			double updateHeightScaleInvert = indexInfo.nImgHeight / heightDist;

			// 计算没每个点到线段的投影值;
			getClosetPointDistToLine(curPt, indexInfo.upLeftPt, indexInfo.upRightPt, colAddDelt, rowAddDelt);

			// 计算在图像中的行列值，此处计算应会在拐弯处存在较明显的形变，应检查;
			int curCol = (int)(colAddDelt * updateHeightScaleInvert);

			int curRow = (int)(rowAddDelt * updateWidthScaleInvert);

			// 返回图像像素坐标;
			row = curRow;
			col = curCol;

			//// 由于采用矩形框，会存在多余点在框内，需要判断是否在图像范围内;
			//if (row >= 0 && row < indexInfo.nImgWidth && col >= 0 && col < indexInfo.nImgHeight)
			//{
			//	// 在范围内，认为找到点，否则继续在下一帧中寻找;
			//	bFind = true;
			//	break;
			//}
		}
	}

	return bFind;
}

// 加载路面影像信息
bool hnPavementImageRoute::loadRoadImage(const char* strSynFile)
{
	FILE* pf = fopen(strSynFile, "rt");
	if (!pf)
	{
		return false;
	}

	// 读取影像索引信息数据;
	bool bSucc = false;
	char strLine[1024];
	int nIndexCount = 0;
	m_vecRoadImageInfo.resize(5000);

	std::vector<ROAD_IMAGE_INDEX> vecTemp;
	ROAD_IMAGE_INDEX info;
	while (!feof(pf))
	{
		memset(strLine, 0, 1024);
		fgets(strLine, 1024, pf);

		// 临时年月天
		bSucc = info.serialize(strLine, 2021, 9, 18);
		if (!bSucc)
		{
			continue;
		}

		// 
		if (vecTemp.size() <= 0)
		{
			vecTemp.push_back(info);
		}
		else
		{
			if (info.nImageIndex != vecTemp[vecTemp.size() - 1].nImageIndex)
			{
				m_vecRoadImageInfo[nIndexCount].nImageIndex = vecTemp[vecTemp.size() - 1].nImageIndex;
				m_vecRoadImageInfo[nIndexCount].dGpsTimer = vecTemp[vecTemp.size() - 1].dGpsTimer;

				++nIndexCount;

				vecTemp.clear();
				vecTemp.push_back(info);
			}
			else
			{
				vecTemp.push_back(info);
			}
		}

		if (nIndexCount >= m_vecRoadImageInfo.size())
		{
			m_vecRoadImageInfo.resize(m_vecRoadImageInfo.size() + 5000);
		}
	}

	fclose(pf);
	m_vecRoadImageInfo.resize(nIndexCount);

	// 
	double dStartGps = m_vecImageInfo[0].dStartTime;
	
	int m_day = (int)(abs(m_vecRoadImageInfo[0].dGpsTimer - dStartGps) / 24.0 / 3600.0 + 0.5);

	if (dStartGps > m_vecRoadImageInfo[0].dGpsTimer)
	{
		for (int i = 0; i < m_vecRoadImageInfo.size(); i++)
		{
			m_vecRoadImageInfo[i].dGpsTimer = m_vecRoadImageInfo[i].dGpsTimer + m_day * 24 * 3600;
		}
	}
	else
	{
		for (int i = 0; i < m_vecRoadImageInfo.size(); i++)
		{
			m_vecRoadImageInfo[i].dGpsTimer = m_vecRoadImageInfo[i].dGpsTimer - m_day * 24 * 3600;
		}
	}

	////////////////////ceshi
	//FILE* pf1 = fopen("D:\\check.log", "w");
	//if (!pf1)
	//{
	//	return true;
	//}

	//vector<int> vecIndex;
	//for (int i = 0; i < m_vecImageInfo.size(); i++)
	//{
	//	vecIndex.clear();

	//	if (!getRoadImageByHdi(m_vecImageInfo[i].dStartTime, m_vecImageInfo[i].dEndTime, vecIndex))
	//	{
	//		continue;
	//	}

	//	fprintf(pf1, "索引为%d的灰度度对应路面影像ID：", i);

	//	for (int j = 0; j < vecIndex.size(); j++)
	//	{
	//		fprintf(pf1, "%d, ", vecIndex[j]);
	//	}

	//	fprintf(pf1,"\n");
	//}

	//fclose(pf1);
	//////////////////////////////

	return true;
}

// 获取当前灰度图对应的路面影像数据--临时方法
bool hnPavementImageRoute::getRoadImageByHdi(double dBegGps, double dEndGps, std::vector<int>& vecRoadIndex)
{
	if (m_vecRoadImageInfo.size() <= 0)
	{
		return false;
	}

	// 
	for (int i = 0; i < m_vecRoadImageInfo.size() -1; i++)
	{
		// 
		if (m_vecRoadImageInfo[i].dGpsTimer < dBegGps || m_vecRoadImageInfo[i].dGpsTimer > dEndGps)
		{
			continue;
		}

		vecRoadIndex.push_back(m_vecRoadImageInfo[i].nImageIndex);
	}

	if (vecRoadIndex.size() <= 0)
	{
		return false;
	}

	return true;
}
