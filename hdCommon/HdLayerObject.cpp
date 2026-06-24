#include "StdAfx.h"
#include "HdLayerObject.h"
#include <algorithm>
namespace hd
{
CHdLayerObject::CHdLayerObject(void)
	:m_poDS(NULL),m_pOgrLayer(NULL)
{
	
}


CHdLayerObject::~CHdLayerObject(void)
{
	
}
// 打开图层数据
BOOL CHdLayerObject::Open( const char* path,const char* lyrName)
{
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	CPLSetConfigOption("DXF_ENCODING","UTF-8");
	CPLSetConfigOption("SHP_ENCODING","UTF-8");
	if (m_poDS)
	{
		delete m_poDS;
		m_poDS = NULL;
	}

	m_poDS = (GDALDataset*)GDALOpenEx(path, GDAL_OF_READONLY | GDAL_OF_VECTOR, NULL, NULL, NULL);
	if( m_poDS == NULL )
	{
		return FALSE;
	}

	// 如果给定名称,则按名称打开,否则打开第0个图层
	if (lyrName)
	{
		m_pOgrLayer = m_poDS->GetLayerByName( lyrName );
	}
	else
	{
		m_pOgrLayer = m_poDS->GetLayer(0);
	}

	//更新文件名
	m_strPath=std::string(path);

	return m_pOgrLayer != NULL;

}

std::string CHdLayerObject::GetFileName() const
{
	char driver[256] = {0};
	char dir[256] = {0};
	char fileName[256] = {0};
	char ext[256] = {0};

	//分割文件
	::_splitpath(m_strPath.c_str(),driver,dir,fileName,ext);

	return std::string(fileName);

}

std::string CHdLayerObject::GetFileExt() const
{
	char driver[256] = {0};
	char dir[256] = {0};
	char fileName[256] = {0};
	char ext[256] = {0};

	//分割文件
	::_splitpath(m_strPath.c_str(),driver,dir,fileName,ext);
	std::string fileExt(ext);
	std::transform(fileExt.begin(),fileExt.end(),fileExt.begin(),tolower);
	return fileExt;
}

std::string CHdLayerObject::GetFilePath() const
{
	return m_strPath;
}

// 设置文件路径
void CHdLayerObject::SetFilePath(std::string strFilePath)
{
    m_strPath = strFilePath;
}
}