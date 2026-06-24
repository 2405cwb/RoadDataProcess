#pragma once
#include "..\hdHlslib\inc\hl2Definition.h"
#include "..\hdHlslib\inc\hlsDefinition.h"
#include "..\hdCommon\point_types.h"
#include "IHLSReader.h"
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <istream>
#include <fstream>
using namespace std;
using namespace hd;

namespace hd
{
class HLS_API CHL2Reader
	//:public IHLSReader
{
	// 公共成员变量
public:
	HLSheader m_header;
private:

	// 数据所在路径
	char  m_dir[256];
	// 数据文件名称,不包含扩展名
	char  m_name[256];
	
	// windows文件句柄
	HANDLE m_pDataFile;
	// 内存映射句柄
	HANDLE m_fileMap;
	// 内存映射数据区起始位置
	char*  m_mapAddress;	
	// 代码锁
	CRITICAL_SECTION m_cs;
	// 当前圈序号
	I32 m_loopIndex;
	// 当前数据文件序号
	U16 m_curFileIndex;
	// 圈索引
	CLoopIndex* m_pLoopIndex;

	// 文件头7参数对应的坐标转换矩阵
	double m_matrix[16];

	// 上次读取的列
	vector<PointXYZIPRGBA> m_pPtBuf;
public:
	// 文件头信息
	//HLSheader m_header;
public:
	CHL2Reader(void);
	~CHL2Reader(void);
	// 打开文件读取
	virtual BOOL Open(const char* path);
	// 读取一个点
	inline virtual BOOL Read_Point(PointXYZIPRGBA& pt,U64 index);
	// 读取一圈,将数据拷贝到目标数组
	inline virtual BOOL ReadLoop(
		PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
		U32& count,				// 读取到的点数
		I32 loop,				// 圈号
		bool bCopyData = true);	
	// 读取一圈,将数据拷贝到目标数组
	inline virtual BOOL ReadLoopTest(
		PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
		U32& count,				// 读取到的点数
		I32 loop);				// 圈号

	// 读取一圈,将数据拷贝到目标数组,包含无效点
	inline virtual BOOL ReadLoopFull(
		PointXYZIPRGBA*& ptBuf,		// 外部传入的数组,外部管理指针	
		U32& count,					// 读取到的点数
		I32 loop);					// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针

	// 关闭
	BOOL virtual Close();
	// 获取HLS2_LOOPINDEX
	inline CLoopIndex* GetLoopIndex(){return m_pLoopIndex;}
	// 关闭文件映射
	inline virtual void CloseFile();
	//! 获取当前点全局坐标
	inline virtual void GetCoordinate(double& x,double& y,double& z);

protected:
	// 读取一圈的数据
	inline BOOL ReadBuffer(void*& pBuf,const HLS2_LOOPINDEX& loopIndex,bool bCopyData);
	// 打开文件映射
	inline BOOL OpenFile(const char* filePath);
	//! 关闭内存映射
	inline void CloseMapdata();
	//! 根据7参数计算矩阵
	void computeMatrix();
};
}
