

#pragma once
#include <vector>
#include <string>
#include <list>
#include <map>
#include <set>
#include "HlzDefs.h"
//#include "..\hdCore\hdString.h"
#include "..\hdCore\hdBox3d.h"
#include "HdLevel.h"
#include "..\hdCommon\point_types.h"
#include "HLZWriter.h"
#include "HlzBuilderDefs.h"

using namespace std;
using namespace hd;

namespace hd
{
	// 前置声明
	class IHLSReader;
	class CHdCoreData;
	class CHLZWriter;

	class HLS_API CHlzBuilder
	{
		//! 私有变量
	private:
		// hlz写入对象CHLZWriter
		CHLZWriter* m_pHlzWrite;

		// 块集临时文件数组
		std::map<U32, BlockSetFileInfo> m_BlocksetFiles; 

		// 第0层对象
		//HdLevel m_level0;

		//点云文件列表
		std::vector<hd::stringc> m_hlsFileList;	

		// 分层集合
		//std::set<CHdLevel*> m_LevelSet;

		// 当前HLS文件读取对象
		//IHLSReader* m_pHlsReader;

		// 分层分块管理对象
		CHdCoreData* m_pCoreData;

		// 整体范围
		CHdBox3dd    m_fullExtent;

		//索引文件路径
		hd::stringc	m_name;			

		// 所有临时文件保存目录
		hd::stringc	m_savePath;	

		//网格大小,X步长
		F32					m_stepX;	

		//网格大小,Y步长
		F32					m_stepY;

		//全局网格坐标x方向起始值    袁亮  20160622
		I32                 m_xNoFrom;
		//全局网格坐标x方向终止值    袁亮  20160622
		I32                 m_xNoTo;
		//全局网格坐标y方向起始值    袁亮  20160622
		I32                 m_yNoFrom;
		//全局网格坐标x方向终止值    袁亮  20160622
		I32                 m_yNoTo;

		//! 坐标统计变量
		CoordStatInfo m_coordStats;

		// hlz头文件的点数，重新统计
		U64 m_ptNum;

		//! 标记转换是否对0级格网均匀抽稀，若抽稀，精度设置为0层精度m，对应子层精度m*2
		bool m_bResampleZero;

		//! 对格网均匀抽稀精度设置，若0层重采样抽稀，则该精度值为0层精度，否则，为1层精度
		double m_dGridPrecision;

		//! 私有成员函数
	private:

		//! 切割输入文件
		bool SplitInputFiles(const char* savePath);

		//! 按标准格网切割Hls成为块集
		bool SplitBlockSetFiles(const char* savePath, hd::stringc& strHlsFile);
		//! 按标准格网切割Hls成为块集
		bool SplitBlockSetFiles_LN(const char* savePath, hd::stringc& strHlsFile,U8 iLevel);

		//! 切割Las到块集文件
		bool SplitBlockSetLas(const char* savePath,hd::stringc& strLasFile);

		//! 对块集进行切割成块
		//输入:待切分的块集
		bool SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);//

		//! 对块切割成包,包成每个包的点云数量不超过64000
		// 对于包内点数大于64000，进行递归条用对包文件进行切割
		bool SplitParcelFiles(
			const CHdBox3df&	blockBox,								// 输入块的空间范围
			const char* strBlockFile,							// 输入待切分的块文件
			const char* savePath,								// 输入切分后的包保存的位置
			std::vector<ParcelFileInfo>& arrayParcels			// 输出切分后的
			);	

		//! 从临时文件中读取一定数量的点到内存
		//U32 readPtsBuffer(FILE*& pFile, std::vector<PointXYZIPRGBA>& vecBuffer);

		//! 根据块集列表
		CHdBox3df GetBlockListBox(std::vector<BlockSetFileInfo>& bsFileList);
		//! 将第0层数据写入到文件
		void writeLevel0Data(CHdLevel* pLevel0);

		//! 根据上一层数据，将其下一层数据写入到文件,返回当前层的点数
		U64  writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* curLevel);

		//! 写入下一层数据
		U64  writeNextLevel(U8 nLevelNo, CHdLevel* curLevel);

		//! 根据当前层的块集编号确定上一层的块集
		void getPreLevelBlockset(
			U16 xNo,					// 输入当前块集的位置X
			U16 yNo,					// 输入当前块集的位置Y
			CHdLevel* pPreLevel,			// 输出上一层
			std::vector<BlockSetFileInfo>& preBlocksetFiles);	// 当前块集对应的上一层块集

		//! 统计点云强度范围
		void StatIntensity(
			IHLSReader* pHlsReader,			// 输入的hls读取对象
			U16& min,						// 输出的最小强度
			U16& max);						// 输出的最大强度

		//! 根据点的坐标确定其所在的块位置
		// 将块集切分为八块，即XYZ方向各切一刀;编码从左到右,从下到上
		//    6--7
		//  4 --5|
		//  | |  |
		//	| 2--3
		//	|/  /
		//  0--1
		inline void getPtBlockIndex(
			PointXYZIPRGBA& pt,		// 输入的点，真实坐标
			//CHdVector3df& min,		// 输入块集的包围盒左下角
			const CHdVector3df& mid,		// 输入块集的包围盒中心点
			U8& index);				// 输出该点的块位置

		//! 获取块（或者包）的空间范围
		void getBlockBox(
			//const char* blockPath,			// 输入块的路径，从中获取块的序号
			const CHdVector3df& mid,				// 输入所属块集的范围中心
			const CHdVector3df& halfSize,			// 输出该块的空间范围
			CHdBox3df& box,					//输出该块的空间范围
			U16 index);

		//! 获取块集文件夹下的所有块文件
		//U32 getBlockFiles(const char* strBSPath, std::vector<hd::stringc>& vecBlockFiles);


		//! 根据点云统计块集划分方法

		//! 根据总体
	public:
		CHlzBuilder(void);
		~CHlzBuilder(void);

		// 强度最小值
		U16				    m_iMin;

		//! 强度最大值
		U16					m_iMax;

		//! 关闭
		void Close();
		//! 进度统计对象
		void (*processCallback)(float,const char*);

		//! 打开hls1.0/hls2.0
		//IHLSReader* OpenHls(const char* strPath);

		//! 关闭当前hls1.0/hls2.0读取对象
		//void CloseHls();

		//! 添加一个文件hls或者xyz文件
		void addFile(const hd::stringc& hlsFile)
		{
			m_hlsFileList.push_back(hlsFile);
		}

		//! 添加一系列hls或者xyz文件
		void addFiles(const std::vector<hd::stringc>& hlsFiles)
		{
			for (unsigned int i = 0;i < hlsFiles.size();i++)
			{
				m_hlsFileList.push_back(hlsFiles[i]);
			}
		}

		//! 清空所有hls文件
		void clearFiles()
		{
			m_hlsFileList.clear();
		}

		//! 关闭所有文件  2013/11/20 蔡红云 
		void CloseTempFiles();

		//! 删除所有临时文件 2013/11/19 蔡红云 
		BOOL RemoveTempFiles(const char* path);

		//! 删除块集文件
		BOOL RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles);

		//! 开始生成hlz文件
		bool buildHlz(const char* savePath, BOOL isCompress = FALSE);

		//! 外部接口设置，是否抽稀零层及抽稀精度，若为fou，该抽稀精度为1层的抽稀精度1/2
		void SetResampleZero(bool bResample,double dGridPrecision);

		//计算HLS文件中的数据的最大值、最小值
		void calHlsExtnt(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max);

		// 数据复制
		inline void VecCopy3fv(double v[3], const double a[3])
		{
			v[0] = a[0];
			v[1] = a[1];
			v[2] = a[2];
		}

		// 比较最大值最小值
		inline void VecUpdateMinMax3dv(double min[3], double max[3], const double v[3])
		{
			if (v[0]<min[0]) min[0]=v[0]; else if (v[0]>max[0]) max[0]=v[0];
			if (v[1]<min[1]) min[1]=v[1]; else if (v[1]>max[1]) max[1]=v[1];
			if (v[2]<min[2]) min[2]=v[2]; else if (v[2]>max[2]) max[2]=v[2];
		}
	};

}
