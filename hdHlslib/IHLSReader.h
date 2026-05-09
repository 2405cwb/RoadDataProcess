/*! IHLSReader.h
********************************************************************************
<PRE>
模块名       : hdHLSlib
文件名       : IHLSReader.h
相关文件     : 
文件实现功能 : 海达数云点文件hls读取
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/06/5   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include ".\inc\hlsDefinition.h"
#include "..\hdHlslib\inc\hl2Definition.h"
#include "..\hdCommon\point_types.h"
#include "..\hdCore\hdBlkArray.h"
#include <io.h>
#include "InclinometerFit.h"
#include <vector>
#include "HlzDefs.h"
using namespace std;

namespace hd
{

	enum ENUM_HD_DATA_TYPE // 点云数据类型 分为HLS--HLZ---2种
	{
		E_HDT_HLS = 0,
		E_HDT_HLZ
	};

	class HLS_API IHLSReader
	{

		// 公共成员变量
	public:
		HLSheader m_header; // hls 头文件
		HLZheader m_hlzHeader;  // hlz 头文件

		// 文件头7参数对应的坐标转换矩阵
		double m_matrix[16];

		ENUM_HD_DATA_TYPE m_dataType; // 数据格式

	protected:

		// 文件路径
		char       m_filepath[1024];

		// 圈索引
		CLoopIndex* m_pLoopIndex;

		// 倾角仪器拟合
		CInclinometerFit m_incFit;

	public:
		IHLSReader(void);
		virtual ~IHLSReader(void);

		// 打开文件读取
		virtual BOOL Open(const char* path) = 0;

		// 判断是否已经打开
		BOOL IsOpen()
		{
			if (m_dataType ==  E_HDT_HLS)
			{
				return m_header.number_of_point_records > 0;
			}
			else
			{
				return m_hlzHeader.number_of_point_records > 0;
			}
		}

		// 关闭
		virtual BOOL Close()
		{
			if (m_pLoopIndex)
			{
				delete[] m_pLoopIndex;
				m_pLoopIndex = NULL;
			}
			return TRUE;
		}

		// 读取文件头
		HLSheader GetHeader()
		{
			if(!IsOpen() && strlen(m_filepath) > 0 && _access(m_filepath, 04) == 0)
			{
				Open(m_filepath);
			}
			return m_header;
		}

		// 读取HLZ文件头
		HLZheader GetHlzHeader()
		{
			if(!IsOpen() && strlen(m_filepath) > 0 && _access(m_filepath, 04) == 0)
			{
				Open(m_filepath);
			}
			return m_hlzHeader;
		}

		// 获取数据文件路径
		char* GetFilePath(){return m_filepath;}

		// 读取一个点
		inline virtual BOOL Read_Point(PointXYZIPRGBA& pt,U64 index) = 0;

		// 读取一圈,将数据拷贝到目标数组
		inline virtual BOOL ReadLoop(
			hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
			I32 loop,								// 圈号
			U32 simple = 1) = 0;					// 抽稀加载间隔		
		// 读取一圈,获取数据指针
		//inline virtual BOOL ReadLoop(
		//	PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
		//	U32& count,				// 读取到的点数
		//	I32 loop){return FALSE;}// 圈号

		// 读取一圈,将数据拷贝到目标数组,包含无效点
		inline virtual BOOL ReadLoopFull(
			hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
			I32 loop,								// 圈号
			U32 simple = 1) = 0;					// 抽稀加载间隔			

		// 更新一圈数据,到数据文件
		inline virtual BOOL UpdateLoop(
			const hdVector<PointXYZIPRGBA>& ptBuf,	// 需要更新的列
			I32 loop) = 0;								// 列序号		

		// 根据输入点,计算全局坐标
		inline virtual void GetCoordinate(
			const F32& x,const F32& y,const F32& z,
			F64& outX,F64& outY,F64& outZ);

		//! 将x,y,z转换为全局坐标
		inline virtual void GetCoordinate(F64& x,F64& y,F64& z);

		  // 全局坐标转换为局部左边；
		bool AntiTranslate(double& x, double& y, double& z) const;
	   
		// 全局坐标转换为局部坐标；
		bool AntiTranslate(float& x, float& y, float& z) const;

		// 获取HLS2_LOOPINDEX
		inline virtual CLoopIndex* GetLoopIndex(void (*loadCallback)(float,const char*) = NULL){return m_pLoopIndex;}

		// 申请索引空间
		inline CLoopIndex* AllocLoopIndex(u32 count);

		// 获取圈数
		inline virtual U32 GetLoopCount(){return m_header.number_of_col;}

		//! 获取每一圈包含点数
		virtual U32 GetCountInLoop(){return m_header.number_of_row;}

		//! 判断是否可以更新
		inline BOOL CanUpdate();

		// 关闭文件映射
		virtual void CloseFile() = 0;

	protected:

		//! 根据7参数计算矩阵
		void ComputeMatrix();

		// 根据极坐标计算直角坐标
		inline void ComputeCoordate(PointXYZIPRGBA& pt)
		{
			if (pt.x == 0.0f)//极坐标pt.x是距离
			{
				pt.x = 0.0f;
				pt.y = 0.0f;
				pt.z = 0.0f;
				pt.intensity = 0;
				return;
			}
			float dist = pt.x;
			float angleV = (f32)(DEG2RAD(pt.z - 90.0f));
			float angleHDeg = pt.y;

			pt.z = (float)(dist * sin(angleV));
			pt.x = (float)(dist * cos(angleV) * cos(DEG2RAD(pt.y)));
			pt.y = (float)(-dist * cos(angleV) * sin(DEG2RAD(pt.y)));
			m_incFit.RectifyPoint(angleHDeg,angleV,pt.x,pt.y,pt.z);

			m_header.max_x = MAX(pt.x, m_header.max_x);
			m_header.min_x = MIN(pt.x, m_header.min_x);
			m_header.max_y = MAX(pt.y, m_header.max_y);
			m_header.min_y = MIN(pt.y, m_header.min_y);
			m_header.max_z = MAX(pt.z, m_header.max_z);
			m_header.min_z = MIN(pt.z, m_header.min_z);
		}

		// 根据极坐标计算直角坐标,返回有效点数
		inline void ComputeCoordate(hdVector<PointXYZIPRGBA>& vecPts)
		{
			for (unsigned int i = 0;i<vecPts.size();i++)
			{
				PointXYZIPRGBA& pt = *(vecPts._Myfirst + i);
				ComputeCoordate(pt);
			}
		}
	};
}

