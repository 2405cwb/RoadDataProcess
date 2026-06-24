#pragma once
#include <vector>
#include <string>
#include <list>
#include <map>
#include "..\hdCore\hdBox3d.h"
#include "..\hdCore\hdString.h"
#include "inc\mydefs.hpp"
#include "inc\hlsDefinition.h"
#include "HL2Writer.h"
#include "..\hdCommon\point_types.h"
#include "..\hdCommon\hdHdiStruct.h"
#include "octree\octree.h"

using namespace std;
using namespace hd;

// 定义临时点的扩展结构
struct PointXYZIPRGBAEX
{
	PointXYZIPRGBA pt;
	int LoopIndex;
	hd::u8 uHlsIdx;
};

// las或hls1.0合并转换到hls2.0
class HLS_API CHLS2Builder
{
private:
	bool					m_bStripByX;
	hd::stringc				m_name;			//索引文件路径
	hd::stringc				m_savePath;		//保存目录

	CHL2Writer				 m_hlsWrite;
	std::vector<hd::stringc> m_hlsFileList;	//点云文件列表
	std::vector<hd::stringc> m_hlsCorrLinFileList;		// hls对应的Lin文件列表
	std::vector<std::vector<HD_SCANHDIINFO> > m_vecHlsPos;	// 解析输入的lin文件
	CHdBox3dd			m_boundBox;			//所有点云文件外包盒

	U32					m_curCount;		//当前块包含点数
	U64					m_totalCount;	//所有点云文件中点数
	U32					m_maxPtInCube;	//格网内最大点数
	F64					m_stepX;			//网格大小,X步长
	F64					m_stepY;			//网格大小,Y步长

	U32					   m_blockCount;// 缓存块数
	vector<PointXYZIPRGBA> m_vecBuf;	// 内存缓存
	U32					   m_bufIndex;	// 内存缓存索引

	/*std::map<U32,FILE*>    m_dicFile;*/
	// 蔡红云 2013/11/19 临时文件字典表，包括文件的序号、文件名称、以及文件指针
	// 这样写字典表的好处就是后续删除临时文件时可进行遍历
	std::map<U32, pair<FILE*, string >> m_dicFile; 

	// 蔡红云 2013/9/10 支持合并LAS文件，用于控制进度条
	U32 m_readPointCnt;

	// 每个点云加载之内存的阈值
	U32 m_nLoadSimple;

	// hls文件合并是否要进行抽稀,默认是不进行抽稀释[2014/4/24 蔡红云]
	BOOL m_bneedcx;

	// 空间格网的大小
	F32 m_3DGridSize;

	// 是否根据到POS中心的距离进行空间合并 [zfei 2014/7/23]
	BOOL m_bSpatialCombine;

public:
	CHLS2Builder(void);
	~CHLS2Builder(void);

	void (*processCallback)(float,const char*);

	//! 计算所有点云范围及总点数
	bool calucBoundBox();
	//! 按标准格网切割Hls
	bool normalSplitHlsFile(const char* savePath, hd::stringc& strHlsFile);
	//! 按标准格网切割Las
	bool normalSplitLasFile(const char* savePath, hd::stringc& strLasFile);
	//! 按标准格网切割
	bool normalSplit(const char* savePath);

	//! 设置每个Cube中最大点数
	void setMaxCount(int maxPtInCube = 1024);

	//! 设置hls加载阈值
	void SetHLSLoadThreShold(int loadNum) { m_nLoadSimple = loadNum;}

	//! 设置合并hls是否要抽稀
	void SetHLSToCX(BOOL cx){ m_bneedcx = cx;}

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

	// 设置空间合并抽稀的格网大小[zfei]
	void Set3DGridSize(F32 fGridSize){m_3DGridSize = fGridSize;}
	
	//! 添加hls文件对应的lin文件 [zfei 2014/7/31]
	void addLinFiles(const std::vector<hd::stringc>& linFiles)
	{
		for (unsigned int i = 0; i < linFiles.size(); i++)
		{
			m_hlsCorrLinFileList.push_back(linFiles[i]);
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
	void RemoveTempFiles(); 

	// 点云到POS中心的距离[zfei 2014/7/23]
	hd::f32 GetDistToPos(PointXYZIPRGBA& pt, hd::u32 nLoopIndex, hd::u32 nPosFileIndex);

	//! 空间局部点合并抽稀 [zfei 2014/7/23]
	void PtsCombineOpti(vector<PointXYZIPRGBAEX>& ptGrid, CHdBox3dd& boundBox);

	//! 开始建立空间索引生成hls文件
	bool buildHls(const char* savePath);

	//! 空间合并抽稀hls文件 [zfei 2014/7/23]
	bool buildHls(const char* saveCombineHlsPath, BOOL bCombine);

	// ！ 蔡红云 2013/10/ 12 统计HLS文件中数据的最小值、最大值
	void calHlsExtnt(hd::stringc& );

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

