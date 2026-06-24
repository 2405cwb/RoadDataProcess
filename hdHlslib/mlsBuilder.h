#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>
#include "irrlicht.h"
#include "..\hdCommon\mydefs.hpp"
#include "..\hdCommon\hlsDefinition.h"
#include "mlsHeader.h"

using namespace std;
using namespace irr;
//using namespace irr::core;

typedef core::vector3d<f64> vector3dd;
typedef core::aabbox3d<f64> aabbox3dd;

typedef struct cubeNo
{
	U32	xNo;
	U32 yNo;
	U32 zNo;

	bool operator< (const struct cubeNo& rhs) const
	{
		return (xNo + yNo + zNo) < (rhs.xNo + rhs.yNo + rhs.zNo);
	}

	bool operator== (const struct cubeNo& rhs)const
	{
		return xNo == rhs.xNo && yNo == rhs.yNo && zNo == rhs.zNo;
	}
};

typedef struct cubeInfo
{
	cubeInfo()
	{
		isBuild = false;
		ptcount = 0;
		fwFile = NULL;
	}
	bool isBuild;
	int	 ptcount;
	aabbox3dd box;
	FILE* fwFile;
};

typedef struct cubeLess
{
	bool operator()(const cubeNo& left,const cubeNo& right)
	{
		return (left.xNo + left.yNo + left.zNo) < (right.xNo + right.yNo + right.zNo);
	}
};

class LAS_API mlsBuilder
{
private:
	string				m_mdxPath;		//索引文件路径
	string				m_savePath;		//保存目录
	FILE*				m_pMdxFile;		//索引文件
	FILE*				m_pDataFile;	//当前数据文件
	string				m_curFileName;
	U32					m_curFileSize;	//当前数据文件已经写入字节数
	U32					m_curFileNo;	//当前文件编号
	mlsHeader			m_header;		//文件头结构体

	std::vector<std::string> m_hlsFileList;	//点云文件列表
	aabbox3dd			m_boundBox;			//所有点云文件外包盒
	//点云分布统计变量,key为格网编号,value为格网内点数
	std::map<cubeNo,cubeInfo,cubeLess>		m_pcdStat;		
	std::vector<cubeNo>	m_curList;		//当前块包含子块列表
	U32					m_curCount;		//当前块包含点数
	U32					m_subCountX;	
	U32					m_subCountY;	
	U32					m_subCountZ;	
	U64					m_totalCount;	//所有点云文件中点数
	U32					m_maxPtInCube;	//格网内最大点数
	F64					m_xStep;		//网格大小,X步长
	F64					m_yStep;		//网格大小,Y步长
	F64					m_zStep;		//网格大小,Z步长
private:
	//! 计算所有点云范围及总点数
	bool calucBoundBox();
	//! 按标准格网切割
	bool normalSplit(const char* savePath);
	//! 写入聚合文件
	void Aggregate();
	void writeAggregate(const std::vector<cubeNo>& aggList);
	//! Cube合并
	bool aggregateCube(const cubeNo& cNo);
	//! Cube切割
	bool splitCube(map<cubeNo,cubeInfo,cubeLess>::iterator& it,const char* path,double cubeStepX,double cubeStepY,double cubeStepZ);
public:
	mlsBuilder(void);
	~mlsBuilder(void);

	//! 设置每个Cube中最大点数
	void setMaxCount(int maxPtInCube);
	//! 添加一个文件hls文件
	void addFile(const std::string& hlsFile)
	{
		m_hlsFileList.push_back(hlsFile);
	}
	//! 添加一系列hls文件
	void addFiles(const vector<std::string>& hlsFiles)
	{
		for (vector<std::string>::const_iterator it = hlsFiles.begin();it != hlsFiles.end();it++)
		{
			m_hlsFileList.push_back(*it);
		}
	}
	//! 清空所有hls文件
	void clearFiles()
	{
		m_hlsFileList.clear();
	}

	//! 开始建立空间索引生成mls文件
	bool buildMls(const char* savePath);
};

