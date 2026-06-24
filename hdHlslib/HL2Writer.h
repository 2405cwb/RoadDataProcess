/*! HLSWriter2.h
********************************************************************************
<PRE>
模块名       : LASLib
文件名       : HLSWriter2.h
相关文件     : 
文件实现功能 : 海达数云车载点云点文件hls写入
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/30   1.0      龚书林			  原始版本
</PRE>
*******************************************************************************/

#pragma once
#include ".\inc\hlsDefinition.h"
#include ".\inc\hl2Definition.h"
#include "..\hdCommon\sceneData\hd3LSScanStruct.h"
#include "..\hdCommon\point_types.h"
#include <stdio.h>
#include <istream>
#include <fstream>
#include <vector>
using namespace std;
using namespace hd;
namespace hd
{
class HLS_API CHL2Writer
{
// 私有成员
private:
	// 数据所在路径
	char  m_dir[256];
	// 数据文件名称,不包含扩展名
	char  m_name[256];
	// 当前数据文件指针
	FILE* m_pDataFile;
	// 索引文件指针
	FILE* m_pIndexFile;
	// 数据文件限定大小
	U64	m_limitFileSize;
	// 当前数据文件已经写入大小
	U32 m_curFileSize;
	// 当前数据文件序号
	U16 m_curFileIndex;
	// 圈索引变量
	HLS2_LOOPINDEX m_loopIndex;

	// 增加写入只一圈点云而不对头文件做更新判断
	bool m_bOnlyUndateData;

public:
	// 文件头信息
	HLSheader m_header;

	CHL2Writer(void);
	~CHL2Writer(void);
	// 设置格式
	void SetPointFormat(U8 ptFormat = HLS2_POINTFORMAT_XYZIRGBP);
	// 打开准备写入
	BOOL Open(
		const char* file_name,		// 指定保存路径
		bool bOnlyUpdata = false,   // 打开只更新部分圈点云数据,头文件、圈索引等不做更新
		U32 io_buffer_size=65536);	// IO缓存大小
	// 写入xyzi扫描圈,ptBuf是绝对坐标,写入时内部自动转相对坐标
	BOOL WriteLoop(const PointXYZI_D* ptBuf,U32 count);
	// 写入xyzi扫描圈,ptBuf是相对坐标的
	BOOL WriteLoop(const PointXYZIPRGBA* ptBuf,U32 count);
	// 写入分块
	BOOL WriteMesh(const PointXYZIPRGBA* ptBuf,U32 count,F32 xmin,F32 ymin,F32 zmin,F32 xmax,F32 ymax,F32 zmax);	

/**************************更新部分圈数据，只对对应圈的数据文件进行更新********************************/
	// 调用示例代码见CHdPointCloudExport::PointCloudUpdateHls()
	
	// 更新写入一圈点云（做更新保存使用），根据索引定位写入一圈，将原有效数据段填满，不做压缩
	BOOL WriteLoop( const PointXYZIPRGBA* ptBuf,U32 count,CLoopIndex* pLoopIdx );
/**************************更新部分圈数据，只对对应圈的数据文件进行更新*********************************/

	// 关闭写入,bOnlyUpdata = true时只更新部分圈的点云，头文件未做更新，故也不应该判断头文件信息数据
	BOOL Close();

	// 写入地面站数据对应的附加参数信息（水平和垂直起始角度）[zfei 2014/9/17]
	void WriteScanParamsXml(_HD_SCAN_PARAM& ScanParam);
protected:
	//! 关闭文件之前检查文件大小是否为dwAllocationGranularity的整数倍
	void CloseDataFile(FILE* pFile);
	
	// 更新写入文件头
	void WriteHeader();
};

}