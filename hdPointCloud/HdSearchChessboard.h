/*! HdSearchChessboard.h
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdSearchChessboard.h
相关文件     : 
文件实现功能 : 自动查找地面测站点云中的棋盘
作者         : 朱立雄
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人           修改内容   
2016/08/8    1.0      朱立雄            创建
</PRE>
*******************************************************************************/
#ifndef HDPOINTCLOUD_HDSEARCHCHESSBOARD_H
#define HDPOINTCLOUD_HDSEARCHCHESSBOARD_H

#include "hdPointCloud.h"
#include "..\..\hdCommon\point_types.h"
#include <vector>

namespace hd
{
	class PointCloud;

	// 自动查找点云中的所有棋盘
	class HDPOINTCLOUD_API CHdSearchChessboard
	{
	public:
		CHdSearchChessboard();
		virtual~ CHdSearchChessboard();

	private:
		CHdSearchChessboard(const CHdSearchChessboard&);
		CHdSearchChessboard& operator= (const CHdSearchChessboard&);

	public:

		// 在点云中的某个点附近查找一个棋盘
		//bool SearchSingleChessboard(PointCloud* pPointCloud, int nLoopIndex, int nPointIndex, PointChessboard& chessboard);

		// 查找点云中的所有棋盘
		bool SearchAllChessboards(PointCloud* pPointCloud, std::vector< PointChessboard >& vChessboard, 
			                      ProcessCallbackFunc pProgressFunc = NULL);

	private:

		// 统计点云的每一圈点的水平角度
		// fBeginProgress, fEndProgress: 执行该函数之前的总进度和执行之后的进度
		bool StatisticsAngle(float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc = NULL);

		// 指定角度范围和图像大小，根据点云数据计算灰度图像和深度图像的像素值，同时得到有效性掩膜
		bool CreateImage(double dfHorizontalStart, double dfHorizontalEnd, double dfVerticalStart, double dfVerticalEnd,
			             unsigned short* pGrayImageData, float* pDepthImageData, unsigned char* pValidityMask, 
						 unsigned int nImageWidth, unsigned int nImageHeight, 
						 float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc = NULL);

		// 将强度值拉伸到 0 - 255 之间，采用类似于直方图均衡化的方法
		bool StretchGray(unsigned short* pGrayImageData, unsigned char* pValidityMask, unsigned int nImageWidth, unsigned int nImageHeight,
			             float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc = NULL);

		// 根据图像角度范围、灰度图和深度图，搜索所有的棋盘
		bool SearchChessboards(double dfHorizontalStart, double dfHorizontalEnd, double dfVerticalStart, double dfVerticalEnd,
			                   unsigned short* pGrayImageData, float* pDepthImageData, unsigned char* pValidityMask, 
			                   unsigned int nImageWidth, unsigned int nImageHeight, 
							   std::vector< PointChessboard >& vChessboard,
							   float fBeginProgress, float fEndProgress, ProcessCallbackFunc pProgressFunc = NULL);

		// 根据深度图，计算棋盘特征提取所采用的最佳窗口大小，深度越大，窗口越小
		bool CalcWindowSize(float* pDepthImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
			                  unsigned int nImageHeight, int nPixelX, int nPixelY, int& nWindowSize);

		// 根据深度图和窗口大小，计算一个窗口内的三维点的共面性，共面则返回 true
		// （nPixelX, nPixelY）是以图像左上角像素的左上顶点作为原点的坐标系下的坐标
		bool IsPlanar(float* pDepthImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
			          unsigned int nImageHeight, int nPixelX, int nPixelY, int nWindowSize);

		// 根据灰度图和窗口大小，计算一个窗口内的像素灰度值分布特征与棋盘特征的相似度
		// （nPixelX, nPixelY）是以图像左上角像素的左上顶点作为原点的坐标系下的坐标
		float CalcSimilarity(unsigned short* pGrayImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
			                 unsigned int nImageHeight, int nPixelX, int nPixelY, int nWindowSize);

		// 根据图像的角度范围以及深度图，从图像坐标计算三维坐标
		// （nPixelX, nPixelY）是以图像左上角像素的左上顶点作为原点的坐标系下的坐标
		bool Get3dCoordinate(double dfHorizontalStart, double dfHorizontalEnd, double dfVerticalStart, double dfVerticalEnd,
			                 float* pDepthImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
			                 unsigned int nImageHeight, int nPixelX, int nPixelY, float& fX, float& fY, float& fZ);

		// 计算一圈点的平均水平角度
		double CalcLoopAngle(unsigned int nLoopIndex);

		// 计算一圈中的每个点的水平和垂直角度，灰度值和深度
		void CalcPointAngle(unsigned int nLoopIndex, std::vector< double >& vdfHorizontalAngle,
			                std::vector< double >& vdfVerticalAngle, std::vector< unsigned short >& vnIntensity,
							std::vector< float >& vfDepth);

		// 计算一个窗口内的像素值的均值和标准差，窗口内无效像素较多时返回 false
		bool StatisticsPixels(unsigned short* pGrayImageData, unsigned char* pValidityMask, unsigned int nImageWidth, 
		                  	  unsigned int nImageHeight, int nMinRow, int nMaxRow, int nMinCol, int nMaxCol,
							  int& nMean, int& nStdDev);

	private:

		PointCloud* m_pPointCloud;

		float m_fMinChessboardSize;                              // 棋盘中的黑白方块的最小尺寸

		double m_dfHorizontalStart;                              // 灰度图的水平和垂直方向角度范围
		double m_dfHorizontalEnd;
		double m_dfVerticalStart;
		double m_dfVerticalEnd;

		double m_dfHorizontalInterval;                           // 灰度图的水平和垂直方向角度分辨率
		double m_dfVerticalInterval;

		std::vector< double > m_vdfHorizontalAngle;              // 每圈点的水平角度

		std::string  m_strPCName;                                // 点云文件名
	};

}
#endif