#pragma once
#include "hdPointCloud.h"
#include "..\hdCommon\point_types.h"
#include "..\hdHlslib\inc\hlsDefinition.h"
#include "..\mlsBuilder\mlsReader.h"

#include <vector>

using namespace std;

namespace hd
{

class HDPOINTCLOUD_API mls_pointcloud
{
private:
	// 点数组,从物理文件读取到内存中的点,可能抽稀读取,用于显示
	std::vector<PointXYZI>  m_pts;
	// 灰度最大值
	U32		m_maxIntensity;
	// 灰度最小值
	U32		m_minIntensity;
	// 数据源解析读取对象
	CMlsReader m_reader;

private:
	//! 禁止拷贝
	mls_pointcloud(const mls_pointcloud &pcd)
	{
	}
	//! 禁止复制
	const mls_pointcloud& operator=(const mls_pointcloud& other)
	{
		return *this;
	}
	//! 获取当前内存点
	void get_point(PointXYZI& pt);
public:
	// 点云文件头
	HLSheader m_header;
	// 内存中抽稀后的文件头,点数和行列数≤文件中的相应值
	HLSheader m_simpleHeader;

	mls_pointcloud():m_maxIntensity(0),m_minIntensity(0){}

	~mls_pointcloud()
	{
		clear();
	}

	inline PointXYZI& operator[](U32 index)
	{ 
		return (*(m_pts._Myfirst + index));
	}

	inline const PointXYZI& operator[](U32 index) const
	{ 
		return (*(m_pts._Myfirst + index));
	}
	
	//! 获取内存中点个数,如果抽稀加载则小于countInFile()
	inline U32 count() const {return m_pts.size();}
	
	//! 清理
	inline void clear() 
	{
		m_maxIntensity = 0;
		m_minIntensity = 0;
		m_pts.clear();
		m_header.clean();
	}

	//! 将hls点云文件一次性加载到内存
	BOOL loadMlsFile(const char* hlsFile,void (*loadCallback)(float,const char*) = NULL);
	//! 获取最大灰度值
	inline U32 getMaxIntensity() const {return m_maxIntensity;}
	//! 获取最小灰度值
	inline U32 getMinIntensity() const { return m_minIntensity; }
};

}
