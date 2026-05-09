
#include "HLSReader.h"
#include "bytestreamin.hpp"
#include "bytestreamin_file.hpp"
#include "bytestreamin_istream.hpp"
#include "hdLandMarkTransform.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define hls_open                    _open
#define hls_close(fd)               _close(fd)
#define hls_lseek(fd,offset,origin) _lseek(fd,offset,origin)
#endif

//#define MAPFILE_SIZE	67108864		//文件映射大小(64 * 1024 * 1024)

namespace hd
{

HLSreader::HLSreader()
{
  m_npoints = 0;
  m_pcount = 0; 
  m_mapPos = 0;
  m_fileSize = 0;
  m_mapSize = 0;

  m_hFile = NULL;
  m_fileMap = NULL;
  m_mapData = NULL;
  m_mapAddress = NULL;

  memset(m_matrix,0,sizeof(double) * 16);
}
  
HLSreader::~HLSreader()
{
  if (m_hFile != NULL) close();

  m_fileSize = 0;
  m_npoints = 0;
  m_pcount = 0; 
  m_mapPos = 0;

  memset(m_matrix,0,sizeof(double) * 16);
}

BOOL HLSreader::open(const char* file_name,U32 io_buffer_size)
{
	close();
	if (file_name == 0)
	{
		fprintf(stderr,"ERROR: fine name pointer is zero\n");
		return FALSE;
	}
	strcpy(m_filepath,file_name);

	//m_pfile = hls_open(file_name,O_RDONLY);//fopen(file_name, "rb");
	m_hFile = CreateFile(file_name, 
		GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);
	
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "ERROR: cannot open file '%s'\n", file_name);
		return FALSE;
	}
	
	DWORD sizeHigh = 0;
	m_fileSize = GetFileSize(m_hFile,&sizeHigh);
	if (sizeHigh != 0)
	{
		m_fileSize = ((U64)sizeHigh << 32) | m_fileSize;
	}

	char strAnglesFile[MAX_PATH] = {0};
	sprintf(strAnglesFile,"%s.angle",file_name);
	m_incFit.FitBySimpleData(strAnglesFile);
	return open();
}

BOOL HLSreader::open()
{
	m_fileMap = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (m_fileMap == NULL)
	{
		fprintf(stderr, "ERROR: cannot create file mapping\n");
		return FALSE;
	}

	// 读取解析文件头
	char  tmp[256] = {0};
	DWORD numRead;
	ReadFile(m_hFile,tmp,256,&numRead,NULL);

	m_header.clean();
	m_header.prase(tmp);

	// 解决旧的数据没有7参数
	if (m_header.scale == 0.0)
	{
		m_header.scale = 1.0;
	}
	
	m_npoints = m_header.number_of_point_records;
	m_pcount = 0;

	/*m_ratateMatrix.makeIdentity();
	irr::core::vector3df center;
	center.set(m_header.centerX,m_header.centerY,m_header.centerZ);
	m_ratateMatrix.setRotationCenter(center,irr::core::vector3df());
	irr::core::vector3df rotate;
	rotate.set(m_header.roll,5.0f * m_header.pitch,0.0f);
	m_ratateMatrix.setRotationDegrees(rotate);

	rotate.set(-m_header.roll,-5.0f * m_header.pitch,0.0f);
	m_ratateMatrix2.setRotationDegrees(rotate);*/
	
	m_lastReadPos = m_header.header_size;
	SetFilePointer(m_hFile,m_lastReadPos,NULL,FILE_BEGIN);

	computeMatrix();
	return TRUE;
}

BOOL HLSreader::map_data(U64 pos)
{
	if (pos >=0 && pos < m_fileSize)
	{
		if (m_mapAddress)
		{
			UnmapViewOfFile (m_mapAddress);
			m_mapAddress = NULL;
		}

		m_mapPos = pos;
		DWORD mapSize = pos + m_mapSize <= m_fileSize? m_mapSize:m_fileSize - pos;
		/*if (pos + m_mapSize > m_fileSize)
		{
			m_mapSize = m_fileSize - pos;
		}*/
		
		m_mapAddress = static_cast<char*>(MapViewOfFile (m_fileMap, FILE_MAP_READ, m_mapPos >> 32,
			m_mapPos & 0xffffffff,
			mapSize));//50000 * m_header.point_data_record_length
		
		m_mapSize = mapSize;
		if (m_mapAddress == NULL)
		{
			fprintf(stderr, "ERROR: cannot map file\n");
			CloseHandle (m_fileMap);
			m_fileMap = NULL;
			return FALSE;
		}
		m_mapData = m_mapAddress;
		return TRUE;
	}	

	return FALSE;
}

BOOL HLSreader::read_point( char* buf )
{
	try
	{
		memcpy(&m_point.x,buf,4);
		buf += 4;
		memcpy(&m_point.y,buf,4);
		buf += 4;
		memcpy(&m_point.z,buf,4);
		buf += 4;
		memcpy(&m_point.intensity,buf,2);
		buf += 2;
		// 如果是地面站点云,需要读取点源ID
		if (m_header.point_data_format >= HLS_POINTFORMAT_XYZI
			&& m_header.point_data_format <= HLS_POINTFORMAT_XYZIRGBP)
		{
			memcpy(&m_point.point_source_ID,buf,2);
			buf += 2;
		}

		if (m_header.point_data_format == HLS_POINTFORMAT_RHVI)// 距离角度记录
		{
			if (m_point.intensity > 0 && m_point.x != 0.0f)
			{
				double dIndex = (double)m_pcount * 36.0 /m_npoints;
				int index = dIndex;
				//const PointAngle& ptAngle = *(m_vecAngles._Myfirst + index);
				float dist = m_point.x;
				float angleV = DEG2RAD(m_point.z - 90.0f);	// + 0.23  m_point.z正上方是180度，正下方是0度  - m_header.pitch	 - ptAngle.pitch + 0.743771
				float angleHDeg = m_point.y;

				m_point.z = dist * sin(angleV);
				m_point.x = dist * cos(angleV) * cos(DEG2RAD(m_point.y));// - m_header.roll	 - ptAngle.roll	 - 1.557129
				m_point.y = -dist * cos(angleV) * sin(DEG2RAD(m_point.y));// - m_header.roll	 - ptAngle.roll	 - 1.557129
				m_incFit.RectifyPoint(angleHDeg,angleV,m_point.x,m_point.y,m_point.z);
				/*if (m_pcount > m_header.number_of_point_records / 2 )
				{
					m_ratateMatrix.rotateVect(m_point.x,m_point.y,m_point.z);
				}
				else
					m_ratateMatrix2.rotateVect(m_point.x,m_point.y,m_point.z);*/
				m_header.max_x = MAX(m_point.x, m_header.max_x);
				m_header.min_x = MIN(m_point.x, m_header.min_x);
				m_header.max_y = MAX(m_point.y, m_header.max_y);
				m_header.min_y = MIN(m_point.y, m_header.min_y);
				m_header.max_z = MAX(m_point.z, m_header.max_z);
				m_header.min_z = MIN(m_point.z, m_header.min_z);
			}
			else
			{
				m_point.x = 0.0f;
				m_point.y = 0.0f;
				m_point.z = 0.0f;
				m_point.intensity = 0;
			}
			
			return TRUE;
		}
		
		if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGB ||
			m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
		{
			memcpy(&m_point.rgb[0],buf,2);
			buf += 2;
			memcpy(&m_point.rgb[1],buf,2);
			buf += 2;
			memcpy(&m_point.rgb[2],buf,2);
			buf += 2;

			if (m_header.point_data_format == HLS_POINTFORMAT_XYZIRGBP ||
				m_header.point_data_format == HLS2_POINTFORMAT_XYZIRGBP)
			{
				memcpy(&m_point.classification,buf,1);
				buf += 1;
				memcpy(&m_point.gps_time,buf,8);
				buf += 8;
			}
		}

		return TRUE;
	}
	catch (...)
	{
		return FALSE;
	}
}


BOOL HLSreader::read_fast()
{
	if (m_pcount < m_npoints)
	{
		return read_fast(m_pcount);
	}
	return FALSE;
}

BOOL HLSreader::read_point(int index)
{
	if (index >=0 && index < m_npoints)
	{
		U32 pos = m_header.header_size + index * m_header.point_data_record_length;
		if (pos != m_lastReadPos)
		{
			SetFilePointer(m_hFile,pos,NULL,FILE_BEGIN);			
		}		
		char buf[32];
		DWORD  dwBytesRead = 0;
		if(ReadFile(m_hFile,buf,m_header.point_data_record_length,&dwBytesRead,NULL))
		{
			m_lastReadPos = pos + m_header.point_data_record_length;
			return read_point(buf);
		}
	}
	return FALSE;
}

//! 快速顺序读取
BOOL HLSreader::read_fast( int index )
{
	if (index < m_npoints)
	{
		if(m_header.point_data_format >= 2)
			return read_point(index);
		BOOL bRet = FALSE;
		U64 pos = m_header.header_size + index * m_header.point_data_record_length;
		// 如果读取的点为第一个点或者小于当前映射区,则映射第一块
		if (pos == m_header.header_size || pos < m_mapPos)// 开始映射第一块  || m_mapPos == 0
		{
			SYSTEM_INFO sinf;
			GetSystemInfo(&sinf);
			// 第一次映射的大小必须是dwAllocationGranularity和point_data_record_length的整数倍+header_size
			// 映射起始位置必须是dwAllocationGranularity的整数倍
			m_mapSize = sinf.dwAllocationGranularity * m_header.point_data_record_length * 
				(512 / m_header.point_data_record_length) + m_header.header_size; 
			m_mapPos = 0;
			if(map_data(m_mapPos) == FALSE)
				return FALSE;
		}
		
		// 判断是否在当前映射区域
		while (!(pos >= m_mapPos && pos + m_header.point_data_record_length <= m_mapPos + m_mapSize))
		{
			SYSTEM_INFO sinf;
			GetSystemInfo(&sinf);
			// 修改映射快大小,保证是dwAllocationGranularity和point_data_record_length的整数倍
			// 映射起始位置必须是dwAllocationGranularity的整数倍
			m_mapSize = sinf.dwAllocationGranularity * m_header.point_data_record_length * 
				(512 / m_header.point_data_record_length);
			if(map_data(m_mapPos + m_mapSize) == FALSE)
			{
				// 第一次映射的大小必须是dwAllocationGranularity和point_data_record_length的整数倍+header_size
				m_mapSize = sinf.dwAllocationGranularity * m_header.point_data_record_length * 
					(512 / m_header.point_data_record_length) + m_header.header_size; 
				m_mapPos = 0;
				if(map_data(m_mapPos) == FALSE)
					return FALSE;
			}
		}
			
		m_mapData = m_mapAddress + (pos - m_mapPos);
		bRet = read_point(m_mapData);

		if (bRet)
		{
			m_pcount++;
		}
		return bRet;
	}
	return FALSE;
}

inline BOOL HLSreader::ReadLoop( 
	PointXYZI_D*& ptBuf,		// 外部传入的数组,外部管理指针	
	U32& count,				// 读取到的点数
	I32 loop,				// 圈号
	bool bCopyData)			// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针
{
	return TRUE;
}

//! 读取一圈数据  读取坐标为全局坐标
inline BOOL HLSreader::ReadLoop(
	PointXYZIPRGBA*& ptBuf,	// 外部传入的数组,外部管理指针	
	U32& count,				// 读取到的点数
	I32 loop,				// 圈号
	bool bCopyData)	// 是否拷贝数据到ptBuf,true拷贝数据,false复制指针
{
	return TRUE;
}

BOOL HLSreader::read_point()
{
	return read_fast();
}

BOOL HLSreader::read_point( float& x,float& y,float& z )
{
	BOOL bRet = read_fast(m_pcount);
	if (bRet)
	{
		x = m_point.x;
		y = m_point.y;
		z = m_point.z;
	}
	return bRet;
}

void HLSreader::close()
{
	if (m_hFile != NULL)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	
	closeMapdata();
}

void HLSreader::closeMapdata()
{
	if (m_fileMap)
	{
#if _WIN32
		UnmapViewOfFile (m_mapAddress);
		CloseHandle (m_fileMap);
		m_fileMap = 0;
		m_mapData = 0;
		m_mapPos = 0;
		m_mapSize = 0;
		m_fileSize = 0;
		m_mapAddress = 0;
#endif
	}
}

void HLSreader::getCoordinate( double& x,double& y,double& z )
{
	//computeMatrix();

	// 将本地坐标转换为全局坐标
	x = m_matrix[0]*m_point.x + m_matrix[1]*m_point.y + m_matrix[2]*m_point.z + m_matrix[3];
	y = m_matrix[4]*m_point.x + m_matrix[5]*m_point.y + m_matrix[6]*m_point.z + m_matrix[7];
	z = m_matrix[8]*m_point.x + m_matrix[9]*m_point.y + m_matrix[10]*m_point.z + m_matrix[11];
	double w = m_matrix[12]*m_point.x + m_matrix[13]*m_point.y + m_matrix[14]*m_point.z + m_matrix[15];

	double f = 1.0/w;
	x = static_cast<double>(x*f);
	y = static_cast<double>(y*f);
	z = static_cast<double>(z*f);
}

void HLSreader::computeMatrix()
{
	m_header.computeMatrix(m_matrix);
}




}