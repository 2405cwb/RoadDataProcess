/*! HLSreader.h
********************************************************************************
<PRE>
模块名       : LASLib
文件名       : HLSreader.h
相关文件     : 
文件实现功能 : 海达数云点文件hls读取
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/30   1.0      龚书林    

日 期        版本     修改人              修改内容
2012/06/06   1.01     龚书林			  采用内存映射读取

日 期        版本     修改人              修改内容
2012/07/06   1.02     龚书林			  采用分段内存映射,解决大文件读取问题
</PRE>
*******************************************************************************/

#ifndef HLS_READER_HPP
#define HLS_READER_HPP

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif
#include <vector>
#include ".\inc\hlsDefinition.h"
#include "InclinometerFit.h"
//#include "..\hd3DEngine\include\matrix4.h"
#include "..\hdCommon\point_types.h"
using namespace std;
class ByteStreamIn;

typedef struct PointAngle
{
	float pitch;
	float roll;
	PointAngle()
		:pitch(0.0f),roll(0.0f){}
}POINTANGLE;

namespace hd
{
class HLS_API HLSreader
{
	friend class PointCloud;
private:
	// 倾角仪校正对象
	CInclinometerFit m_incFit;
	//char* m_pPtsBuf;
// 公共成员
public:
  HLSheader m_header;
  HLSpoint  m_point;
  // 总点数
  I64 m_npoints;
  // 当前读取的点序号
  I64 m_pcount;

// 公开函数
public:
  HLSreader();
  virtual ~HLSreader();
  //! 读取下一个点,保存在变量m_point
  BOOL read_point();
  //! 使用内存映射读取点
  BOOL read_fast();
  //! 读取指定序号的点,从内存映射中读取
  BOOL read_fast(int index);
  //! 读取一圈数据  读取坐标为全局坐标
  inline BOOL ReadLoop(
	  PointXYZI_D*& ptBuf,		// 外部传入的数组,外部管理指针	
	  U32& count,				// 读取到的点数
	  I32 loop,					// 圈号
	  bool bCopyData = true);	// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针

  //! 读取一圈数据  读取坐标为局部坐标
  inline BOOL ReadLoop(
	  PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
	  U32& count,				// 读取到的点数
	  I32 loop,					// 圈号
	  bool bCopyData = true);	// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针

  //! 读取一个点,采用非内存映射读取
  BOOL read_point(int index);
  //! 读取下一个点,返回x,y,z
  BOOL read_point(float& x,float& y,float& z);

  //! 获取点云范围,open结束后再调用
  inline F64 get_min_x() const { return m_header.min_x; };
  inline F64 get_min_y() const { return m_header.min_y; };
  inline F64 get_min_z() const { return m_header.min_z; };
  inline F64 get_max_x() const { return m_header.max_x; };
  inline F64 get_max_y() const { return m_header.max_y; };
  inline F64 get_max_z() const { return m_header.max_z; };
  
  //! 获取当前点本地坐标
  inline F32 get_x() const { return m_point.x; };
  inline F32 get_y() const { return m_point.y; };
  inline F32 get_z() const { return m_point.z; };
  
  //! 获取当前点全局坐标
  inline void getCoordinate(double& x,double& y,double& z);
  //! 打开点云文件
  BOOL open(const char* file_name, U32 io_buffer_size=65536);
  //! 关闭文件流，用完后必须调用close
  void close();
// 保护成员函数
protected:
	BOOL open();
	BOOL read_point(char* buf);
	BOOL map_data(U64 pos);
	//! 关闭内存映射
	void closeMapdata();
	//! 根据7参数计算矩阵
	void computeMatrix();
// 私有成员
private:
	// 文件路径
	char   m_filepath[1024];	
	HANDLE m_hFile;
	// 内存映射句柄
	HANDLE m_fileMap;
	// 内存映射数据
	char*  m_mapData;
	// 内存映射数据区起始位置
	char*  m_mapAddress;	
	// 当前映射的文件偏移量
	U64    m_mapPos;
	// 文件大小
	U64    m_fileSize;
	// 每次映射大小
	U32	   m_mapSize;
	// 上次读取位置
	U32	   m_lastReadPos;
	// 文件头7参数对应的坐标转换矩阵
	double m_matrix[16];
};


}
#endif
