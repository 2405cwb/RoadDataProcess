#include "HL2Writer.h"
#include "HL2Reader.h"
#include <Windows.h>
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

// 文件大小限制为2G，2<<31常规FILE读写文件的函数都支持2G函数fengjing
#define LIMIT_SUBFILE_SIZE	INT_MAX			//256M	2<<27
namespace hd
{
CHL2Writer::CHL2Writer(void)
	:m_pDataFile(NULL),m_pIndexFile(NULL),m_limitFileSize(LIMIT_SUBFILE_SIZE),
	m_curFileSize(0),m_curFileIndex(0)
{
	memset(m_dir,0,256);
	memset(m_name,0,256);
	m_bOnlyUndateData = false;
}

CHL2Writer::~CHL2Writer(void)
{
	Close();
}

BOOL CHL2Writer::Open(
	const char* file_name,		// 指定保存路径
	bool bOnlyUpdata,
	U32 io_buffer_size)			// IO缓存大小
{
	//// 对该接口加密
	//string strSoftName = "hdVector";
	//char strMsg[256] = {0};
	//if (!CheckLicense(strSoftName.c_str(), strMsg))
	//{
	//	::MessageBox(NULL,"请向武汉汉宁轨道交通技术有限公司申请加密狗！", "提示",MB_OK);
	//	return false;
	//}

	m_bOnlyUndateData = bOnlyUpdata;

	Close();
	char drive[256] = {0};
	char dir[256] = {0};
	char filename[256] = {0};
	char ext[256] = {0};

	char dirAbs[256] = {0};
	char name[256] = {0};
	_splitpath(file_name,drive,dir,filename,ext);
	_makepath(dirAbs,drive,dir,NULL,NULL);

	strcpy(m_dir,dirAbs);
	strcpy(m_name,filename);

	if (!bOnlyUpdata)
	{
		// 打开数据文件dir+name-fxxxx.hls
		char strDataFile[256] = {0};
		sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,m_curFileIndex);
		m_pDataFile = fopen(strDataFile,"w+b");//CreateFile(strDataFile);//

		if (m_pDataFile == NULL)
		{
			return FALSE;
		}

		if (setvbuf(m_pDataFile, NULL, _IOFBF, io_buffer_size) != 0)
		{
			fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", io_buffer_size);
		}
		// 打开索引文件dir+name.hls
		//char strIndexFile[256] = {0};
		//sprintf(strIndexFile,"%s\\%s.hls",m_dir,m_name);
		m_pIndexFile = fopen(file_name,"w+b");
		if (m_pIndexFile == NULL)
		{
			return FALSE;
		}
		if (setvbuf(m_pIndexFile, NULL, _IOFBF, io_buffer_size) != 0)
		{
			fprintf(stderr, "WARNING: setvbuf() failed with buffer size %u\n", io_buffer_size);
		}

		m_header.set_pointformat(HLS2_POINTFORMAT_XYZIRGBP);
		m_header.number_of_col = 0;
		m_header.number_of_point_records = 0;
		m_header.min_x = F32_MAX;
		m_header.min_y = F32_MAX;
		m_header.min_z = F32_MAX;

		m_header.max_x = F32_MIN;
		m_header.max_y = F32_MIN;
		m_header.max_z = F32_MIN;
		WriteHeader();
	}

	return TRUE;
}

void CHL2Writer::WriteHeader()
{
	if (m_pIndexFile == NULL)
	{
		return;
	}
	fseek(m_pIndexFile,0,SEEK_SET);

	// 写入文件头
	size_t headerSize = sizeof(HLSheader);
	char hdBuf[256] = {0};
	m_header.serialize(hdBuf);
	fwrite(hdBuf,1,256,m_pIndexFile);
	fseek(m_pIndexFile,256,SEEK_SET);	
}

void CHL2Writer::SetPointFormat(U8 ptFormat)
{
	m_header.set_pointformat(ptFormat);
}

BOOL CHL2Writer::WriteLoop( const PointXYZIPRGBA* ptBuf,U32 count )
{
	if (m_pDataFile == NULL || m_pIndexFile == NULL 
		|| ptBuf == NULL || count == 0 || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP)
	{
		return FALSE;
	}
	// 遍历获取无效点区段,同时获取有效点
	PointXYZIPRGBA* ptValid = new PointXYZIPRGBA[count];
	HLS2_LOOPSEG* pLoopSeg = new HLS2_LOOPSEG[count];
	U32 validCount = 0;
	I32 segIndex = -1;
	U32 validIndex = 0;
	bool bValid = false;
	U32 statsCount = 0;
	for (U32 i = 0;i < count;i++)
	{
		const PointXYZIPRGBA& pt = ptBuf[i];
		if (!pt.isValid())
		{
			bValid = false;
		}
		else
		{
			if(!bValid)
			{
				if(segIndex >=0)
					pLoopSeg[segIndex].setCount(statsCount);
				segIndex++;
				statsCount = 0;
				pLoopSeg[segIndex].setStart(i);
			}
			
			statsCount++;
			bValid = true;
			ptValid[validIndex] = pt;
			validIndex++;
		}
	}
	if(segIndex >=0)
		pLoopSeg[segIndex].setCount(statsCount);

	segIndex++;
	m_loopIndex.segCount = segIndex;
	m_loopIndex.offset = ftell(m_pDataFile);
	m_loopIndex.count = validIndex;
	m_loopIndex.rowCount = count;
	m_loopIndex.hasRGBP = 1;
	// 判断是否达到文件限定大小
	if (m_loopIndex.offset + sizeof(PointXYZIPRGBA) * m_loopIndex.count > m_limitFileSize)
	{
		CloseDataFile(m_pDataFile);
		m_pDataFile = NULL;
		m_curFileIndex++;
		char strDataFile[256] = {0};
		sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,m_curFileIndex);
		m_pDataFile = fopen(strDataFile,"w+b");
		if (m_pDataFile == NULL)
		{
			delete[] ptValid;
			delete[] pLoopSeg;
			ptValid = NULL;
			pLoopSeg = NULL;
			return FALSE;
		}
		m_loopIndex.fileNo = m_curFileIndex;
		m_loopIndex.offset = 0;
	}
	m_loopIndex.fileNo = m_curFileIndex;
	// 写入有效点
	size_t writeCount = fwrite(ptValid,sizeof(PointXYZIPRGBA),validIndex,m_pDataFile);	
	BOOL bSuccess = (writeCount == validIndex);
	if (bSuccess)
	{
		// 统计坐标范围和强度范围
		U16 minIntensity = U16_MAX;
		U16 maxIntensity = 0;
		U32 i;
		m_loopIndex.ResetExtent();
		for (i = 0;i<validIndex;i++)
		{
			// 获取当前圈范围
			const PointXYZIPRGBA& pt = ptValid[i];
			m_loopIndex.xmin = MIN(m_loopIndex.xmin,pt.x);
			m_loopIndex.ymin = MIN(m_loopIndex.ymin,pt.y);
			m_loopIndex.zmin = MIN(m_loopIndex.zmin,pt.z);
			
			m_loopIndex.xmax = MAX(m_loopIndex.xmax,pt.x);
			m_loopIndex.ymax = MAX(m_loopIndex.ymax,pt.y);
			m_loopIndex.zmax = MAX(m_loopIndex.zmax,pt.z);

			minIntensity = MIN(minIntensity,pt.intensity);
			maxIntensity = MAX(maxIntensity,pt.intensity);
		}
		m_loopIndex.minIntensity = minIntensity;
		m_loopIndex.maxIntensity = maxIntensity;
		// 获取总体范围
		m_header.min_x = MIN(m_header.min_x,m_loopIndex.xmin);
		m_header.min_y = MIN(m_header.min_y,m_loopIndex.ymin);
		m_header.min_z = MIN(m_header.min_z,m_loopIndex.zmin);

		m_header.max_x = MAX(m_header.max_x,m_loopIndex.xmax);
		m_header.max_y = MAX(m_header.max_y,m_loopIndex.ymax);
		m_header.max_z = MAX(m_header.max_z,m_loopIndex.zmax);

		// 写入圈索引
		fwrite(&m_loopIndex,sizeof(HLS2_LOOPINDEX),1,m_pIndexFile);
		// 写入无效点区间段
		fwrite(pLoopSeg,sizeof(HLS2_LOOPSEG),segIndex,m_pIndexFile);
		
		m_header.number_of_point_records += count;
		m_header.number_of_col++;		
	}

	delete[] pLoopSeg;
	pLoopSeg = NULL;

	delete[] ptValid;
	ptValid = NULL;
	return bSuccess;
}

BOOL CHL2Writer::WriteLoop( const PointXYZI_D* ptBuf,U32 count )
{
	if (m_pDataFile == NULL || m_pIndexFile == NULL 
		|| ptBuf == NULL || count == 0 || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP)
	{
		return FALSE;
	}
	PointXYZIPRGBA* ptBufOff = new PointXYZIPRGBA[count];
	for (int i = 0;i < count;i++)
	{
		ptBufOff[i].x = ptBuf[i].x - m_header.offsetX;
		ptBufOff[i].y = ptBuf[i].y - m_header.offsetY;
		ptBufOff[i].z = ptBuf[i].z - m_header.offsetZ;
		ptBufOff[i].intensity = ptBuf[i].intensity;
	}
	WriteLoop(ptBufOff,count);
	delete[] ptBufOff;
	ptBufOff = NULL;
	return TRUE;
}

BOOL CHL2Writer::WriteLoop( const PointXYZIPRGBA* ptBuf,U32 count,CLoopIndex* pLoopIdx )
{
	if (ptBuf == NULL || count == 0 || count != pLoopIdx->m_loopIdx.count)
	{
		return FALSE;
	}

	// 通过该圈索引值确定该圈所在数据文件
	U16 fileNo = pLoopIdx->m_loopIdx.fileNo;
	char strDataFile[256] = {0};

	// 文件未打开，则打开对应数据文件
	if (!m_pDataFile)
	{
		// 打开失败返回
		sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,fileNo);
		m_pDataFile = fopen(strDataFile,"r+b");
		if (m_pDataFile == NULL)
		{
			return FALSE;
		}

		// 记录当前打开的数据文件编号
		m_curFileIndex = fileNo;
	}
	else
	{
		// 文件已打开，要判断当前打开的数据文件是否为该圈要写入的数据文件，若不是，需更新
		if (m_curFileIndex != fileNo)
		{
			CloseDataFile(m_pDataFile);
			m_pDataFile = NULL;

			// 打开失败返回
			sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,fileNo);
			m_pDataFile = fopen(strDataFile,"r+b");
			if (m_pDataFile == NULL)
			{
				return FALSE;
			}

			m_curFileIndex = fileNo;
		}
	}

	// 首先定位到文件头(ab+打开文件时指针定位在文件末尾)
	fseek(m_pDataFile,0,SEEK_SET);

	// 当前数据文件已打开，根据指针偏移定位过去，写入一圈数据
	fseek(m_pDataFile,(long)pLoopIdx->m_loopIdx.offset,SEEK_SET);

	// 写入所有数据（已删除的数据作为无效点写入）
	size_t writeCount = fwrite(ptBuf,sizeof(PointXYZIPRGBA),count,m_pDataFile);
	if (writeCount == count)
	{
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CHL2Writer::WriteMesh( const PointXYZIPRGBA* ptBuf,U32 count,F32 xmin,F32 ymin,F32 zmin,F32 xmax,F32 ymax,F32 zmax )
{
	if (m_pDataFile == NULL || m_pIndexFile == NULL 
		|| ptBuf == NULL || count == 0 || m_header.get_pointformat() != HLS2_POINTFORMAT_XYZIRGBP)
	{
		return FALSE;
	}

	m_loopIndex.segCount = 1;
	m_loopIndex.offset = ftell(m_pDataFile);
	m_loopIndex.count = count;
	m_loopIndex.rowCount = count;
	if (m_loopIndex.offset + sizeof(PointXYZIPRGBA) * m_loopIndex.count > m_limitFileSize)
	{
		CloseDataFile(m_pDataFile);
		m_pDataFile = NULL;
		m_curFileIndex++;
		char strDataFile[256] = {0};
		sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,m_curFileIndex);
		m_pDataFile = fopen(strDataFile,"w+b");
		if (m_pDataFile == NULL)
		{
			return FALSE;
		}
		m_loopIndex.fileNo = m_curFileIndex;
		m_loopIndex.offset = 0;
	}
	m_loopIndex.fileNo = m_curFileIndex;
	m_loopIndex.hasRGBP = 1;
	// 写入有效点
	size_t writeCount = fwrite(ptBuf,sizeof(PointXYZIPRGBA),count,m_pDataFile);	
	BOOL bSuccess = (writeCount == count);
	if (bSuccess)
	{
		// 统计范围
		m_loopIndex.ResetExtent();
		m_loopIndex.xmin = xmin;
		m_loopIndex.ymin = ymin;
		m_loopIndex.zmin = zmin;

		m_loopIndex.xmax = xmax;
		m_loopIndex.ymax = ymax;
		m_loopIndex.zmax = zmax;

		// 获取总体范围
		m_header.min_x = MIN(m_header.min_x,m_loopIndex.xmin);
		m_header.min_y = MIN(m_header.min_y,m_loopIndex.ymin);
		m_header.min_z = MIN(m_header.min_z,m_loopIndex.zmin);

		m_header.max_x = MAX(m_header.max_x,m_loopIndex.xmax);
		m_header.max_y = MAX(m_header.max_y,m_loopIndex.ymax);
		m_header.max_z = MAX(m_header.max_z,m_loopIndex.zmax);
		// 统计强度范围
		U16 minIntensity = U16_MAX;
		U16 maxIntensity = 0;
		for (U32 i = 0;i<count;i++)
		{
			const PointXYZIPRGBA& pt = ptBuf[i];
			minIntensity = MIN(minIntensity,pt.intensity);
			maxIntensity = MAX(maxIntensity,pt.intensity);
		}
		m_loopIndex.minIntensity = minIntensity;
		m_loopIndex.maxIntensity = maxIntensity;

		// 写入圈索引
		fwrite(&m_loopIndex,sizeof(HLS2_LOOPINDEX),1,m_pIndexFile);
		// 写入区间段
		HLS2_LOOPSEG loopSeg[1];
		loopSeg[0].setStart(0);
		loopSeg[0].setCount(count);
		fwrite(loopSeg,sizeof(HLS2_LOOPSEG),1,m_pIndexFile);

		m_header.number_of_point_records += count;
		m_header.number_of_col++;		
	}

	return bSuccess;
}


BOOL CHL2Writer::Close()
{
	if (m_pDataFile)
	{
		CloseDataFile(m_pDataFile);
		m_pDataFile = NULL;
	}

	if (m_pIndexFile)
	{	
		WriteHeader();
		// 关闭索引文件
		fclose(m_pIndexFile);
		m_pIndexFile = NULL;
	}

	if (!m_bOnlyUndateData)
	{
		if (m_header.number_of_point_records == 0)
		{
			// 删除文件
			char strDataFile[256] = {0};
			int i = 0;
			while (i < 100)
			{
				sprintf(strDataFile,"%s\\%s-%04d.hls",m_dir,m_name,i);
				remove(strDataFile);
				i++;
			}
			sprintf(strDataFile,"%s\\%s.hls",m_dir,m_name);
			remove(strDataFile);
		}
	}

	m_pDataFile = (NULL);
	m_pIndexFile = (NULL);
	m_limitFileSize = (LIMIT_SUBFILE_SIZE);
	m_curFileSize = (0);
	m_curFileIndex = (0);
	
	memset(m_dir,0,256);
	memset(m_name,0,256);

	return TRUE;
}

//! 关闭文件之前检查文件大小是否为dwAllocationGranularity的整数倍
void CHL2Writer::CloseDataFile(FILE* pFile)
{
	SYSTEM_INFO sinf;
	GetSystemInfo(&sinf);
	fseek(pFile,0,SEEK_END);
	long pos = ftell(pFile);
	if (pos % sinf.dwAllocationGranularity != 0)
	{
		long size = (pos / sinf.dwAllocationGranularity + 1) * sinf.dwAllocationGranularity;
		fseek(pFile,size-1,SEEK_SET);
		char c[1] = {0};
		fwrite(c,1,1,pFile);
	}
	fclose(pFile);
}

void CHL2Writer::WriteScanParamsXml(_HD_SCAN_PARAM& ScanParam)
{
	char strInfo[512];

	char paramXmlPath[1024];
	strcpy(paramXmlPath, m_dir);
	strcat(paramXmlPath, m_name);
	strcat(paramXmlPath, "_Params.xml");

	TiXmlDocument doc;
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "gbk", "" );
	TiXmlElement* rootElement = new TiXmlElement("ConfigParams");
	doc.LinkEndChild(decl);
	doc.LinkEndChild(rootElement);

	TiXmlText* xmlText = NULL;
	TiXmlElement* xmlElement = NULL;
	// 行方向起始角
	xmlElement = new TiXmlElement("rowStartAngle");
	rootElement->LinkEndChild(xmlElement);
	
	sprintf_s(strInfo,"%lf", ScanParam.fRowStartAngle);
	//const char* strStAngle = "90";
	xmlText = new TiXmlText(strInfo);
	xmlElement->LinkEndChild(xmlText);
	xmlElement = new TiXmlElement("rowEndAngle");
	rootElement->LinkEndChild(xmlElement);
	sprintf_s(strInfo,"%lf", ScanParam.fRowEndAngle);
	//const char* strStAngle = "-65";		
	xmlText = new TiXmlText(strInfo);
	xmlElement->LinkEndChild(xmlText);

	// 列方向起始角
	xmlElement = new TiXmlElement("colStartAngle");
	rootElement->LinkEndChild(xmlElement);
	sprintf_s(strInfo,"%lf", ScanParam.fColStartAngle);
	xmlText = new TiXmlText(strInfo);
	xmlElement->LinkEndChild(xmlText);

	xmlElement = new TiXmlElement("colEndAngle");
	rootElement->LinkEndChild(xmlElement);
	sprintf_s(strInfo,"%lf", ScanParam.fColEndAngle);
	xmlText = new TiXmlText(strInfo);
	xmlElement->LinkEndChild(xmlText);

	doc.SaveFile(paramXmlPath);

	int nTest = 0;
}

}