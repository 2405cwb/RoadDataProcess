/*! HLZWriter.cpp
********************************************************************************
<PRE>
模块名       : HLSLib
文件名       : HLZWriter.cpp
相关文件     : 
文件实现功能 : 压缩点云文件hlz写入
作者         : 张飞
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/02/04   1.0      张飞			  创建
</PRE>
******************************************************************************/
#include "HLZWriter.h"
#include "HdCoreData.h"
#include "../hdCommon/HdRandomGenerator.h"
#include "HdLevel.h"
#include "HdLevel31.h"
#include "HdBlockset.h"
#include "HdBlockset31.h"
#include "HdBlock.h"
#include "HdBlock31.h"
#include "HdParcel.h"
#include "HdParcel31.h"
#include <time.h>
//#include <unordered_map>
#include <algorithm>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
CHLZWriter::CHLZWriter(void)
	:m_curFileIndex(0),m_pIndexFile(NULL),m_pDataFile(NULL),m_pColorFile(NULL),m_pPropFile(NULL),m_pLogFile(NULL),
	m_nWrittenLen(0), m_pWriteAddr(NULL), m_pMapEndAddr(NULL), m_pMapBegAddr(NULL), m_fileMap(NULL), m_bNewHlz(false)
{
	m_bOnlyUndateData = false;
	m_curDataAddr = 0;
	m_dGridPrecision = 1.0 / 256.0;
}


CHLZWriter::~CHLZWriter(void)
{
	Close();
}

BOOL CHLZWriter::Open( const char* file_name, bool bOnlyUpdata, U32 io_buffer_size,bool bAppend)
{
	Close();

	// 获取文件路径和文件名
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

	char VolName[256]={0};// 磁盘驱动器卷标名称

	DWORD SerailNumber;// 磁盘驱动器卷标序列号
	DWORD MaxCLenth;// 系统允许的最大文件名长度
	DWORD FileSysFlag;// 文件系统标识
	char FileSysName[256]={0};// 文件操作系统名称

	GetVolumeInformation(drive,VolName,255,&SerailNumber,&MaxCLenth,&FileSysFlag,FileSysName,255);

	if (strcmp("FAT32",FileSysName) == 0)
	{
		m_fileFlag = 1;
	}
	else if (strcmp("NTFS",FileSysName) == 0)
	{
		m_fileFlag = 0;
	}

	if (!bOnlyUpdata)
	{
		// 打开数据文件dir+name-dxxxx.hls
		char strDataFile[256] = {0};
		sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
		m_pDataFile = CreateFile(strDataFile, 
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			bAppend ? OPEN_ALWAYS : CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);
		
		if (m_pDataFile == NULL)
		{
			return FALSE;
		}

		LARGE_INTEGER li;
		li.HighPart = 0;
		li.LowPart = 0;

		LARGE_INTEGER li1;
		li1.HighPart = 0;
		li1.LowPart = 0;
		
		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);

		// 打开颜色文件dir+name-cxxxx.hls
		// 打开分类文件dir+name-pxxxx.hls

		// 打开索引文件dir+name.hls
		char strIndexFile[256] = {0};
		sprintf(strIndexFile,"%s\\%s.hlz",m_dir,m_name);
		m_pIndexFile = fopen(strIndexFile/*file_name*/,"w+b");
		if (m_pIndexFile == NULL)
		{
			return FALSE;
		}
		
		m_header.number_of_col = 0;
		m_header.number_of_point_records = 0;

		WriteHeader();	// 空的头文件写入

		if(!bAppend)
		{
			m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0x80000000, NULL);
			if(NULL == m_fileMap)
			{
				return FALSE;
			}

			m_nWrittenLen = 0;
			m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, 0, 0x06400000);
			m_pWriteAddr = m_pMapBegAddr;
			m_pMapEndAddr = m_pWriteAddr + 0x06400000;
		}
	}
	else
	{
		// 打开索引文件dir+name.hls
		char strIndexFile[256] = {0};
		sprintf(strIndexFile,"%s\\%s.hlz",m_dir,m_name);
		m_pIndexFile = fopen(strIndexFile/*file_name*/,"r+b");
		if (m_pIndexFile == NULL)
		{
			return FALSE;
		}

		//读取hlz文件头，判断hlz版本信息
		fread(strIndexFile, sizeof(char), 256, m_pIndexFile);
		m_header.prase(strIndexFile);
		m_bNewHlz = (m_header.version_major == 3 && m_header.version_minor == 2);
		fseek(m_pIndexFile, 0, SEEK_SET);
	}

	return TRUE;
}

void CHLZWriter::WriteHeader()
{
	if (m_pIndexFile == NULL)
	{
		return;
	}

	fseek(m_pIndexFile,0,SEEK_SET);

	// 写入文件头
	size_t headerSize = sizeof(HLZheader);
	char hdBuf[256] = {0};
	m_header.serialize(hdBuf);
	//写入hlz文件头后更新版本标记
	m_bNewHlz = (m_header.version_major == 3 && m_header.version_minor == 2);

	fwrite(hdBuf,sizeof(char),256,m_pIndexFile);
}

// 写入头文件
void CHLZWriter::WriteHeader(const HLZheader& hlzHeader)
{
	if (m_pIndexFile == NULL)
	{
		return;
	}

	fseek(m_pIndexFile,0,SEEK_SET);
    
	char hdBuf[256] = {0};
	hlzHeader.serialize(hdBuf);
	//写入hlz文件头后更新版本标记
	m_bNewHlz = (hlzHeader.version_major == 3 && hlzHeader.version_minor == 2);

	fwrite(hdBuf,sizeof(char),256,m_pIndexFile);

	fclose(m_pIndexFile);
	m_pIndexFile = NULL;
}

//! 写入索引信息
BOOL CHLZWriter::WriteIndex(map<U16,CHdLevel*>& mLevel,F32 offsetX,F32 offsetY)
{
	if (m_pIndexFile == NULL)
	{
		return FALSE;
	}
	// 先抹去原来内容
	fseek(m_pIndexFile, 0, SEEK_END);
	long pos = ftell(m_pIndexFile);
	long size = pos - 256;
	char* cTmp = new char[size];
	memset(cTmp,0,size);
	fseek(m_pIndexFile, 256, SEEK_SET);
	fwrite(cTmp,1,size,m_pIndexFile);
	delete[] cTmp;
	cTmp = NULL;

	// 定位到索引起始位置
	fseek(m_pIndexFile, 256, SEEK_SET);
	int lvlCount = m_header.number_of_level;
	CHdLevel* pLevel = NULL;
	U16 i = 0;
	for (i = 0;i < lvlCount;i++)
	{
		pLevel = mLevel[i];
		fwrite(&(pLevel->m_level),sizeof(HdLevel),1,m_pIndexFile);

		CHdBlockset* pBlockSet = NULL;
		map<I32,CHdBlockset*>::const_iterator itLevel; 
		for (itLevel = pLevel->m_pListBlockset.begin();itLevel != pLevel->m_pListBlockset.end();itLevel++)
		{
			pBlockSet = itLevel->second;
			// 块集包围盒偏移
			pBlockSet->m_blockSet.box.MinEdge.X -= offsetX;
            pBlockSet->m_blockSet.box.MinEdge.Y -= offsetY;
			pBlockSet->m_blockSet.box.MaxEdge.X -= offsetX;
			pBlockSet->m_blockSet.box.MaxEdge.Y -= offsetY;

			if (!m_header.isCompress)
				fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset) - sizeof(U32),1,m_pIndexFile);
			else
				fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset),1,m_pIndexFile);
			
			// 空的块集记录
			if (pBlockSet->m_blockSet.hasSubBlock == 0)
			{
				continue;
			}

			CHdBlock* pBlock = NULL;
			map<I32,CHdBlock*>::const_iterator itBK;
			for (itBK = pBlockSet->m_pListBlock.begin();itBK != pBlockSet->m_pListBlock.end();itBK++)
			{
				pBlock = itBK->second;
				// 块包围盒偏移
				pBlock->m_block.box.MinEdge.X -= offsetX;
				pBlock->m_block.box.MinEdge.Y -= offsetY;
				pBlock->m_block.box.MaxEdge.X -= offsetX;
				pBlock->m_block.box.MaxEdge.Y -= offsetY;
				if (!m_header.isCompress)
					fwrite(&pBlock->m_block,sizeof(HdBlock) - sizeof(U32),1,m_pIndexFile);
				else
					fwrite(&pBlock->m_block,sizeof(HdBlock),1,m_pIndexFile);
				
				// 添加块内部的包				
				if (pBlock->m_block.hasSubParcel == 0)
				{
					continue;
				}
				CHdParcel* pParcel = NULL;
				map<I32,CHdParcel*>::const_iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					pParcel = itPcl->second;
					// 包包围盒偏移
					pParcel->m_parcel.box.MinEdge.X -= offsetX;
					pParcel->m_parcel.box.MinEdge.Y -= offsetY;
					pParcel->m_parcel.box.MaxEdge.X -= offsetX;
					pParcel->m_parcel.box.MaxEdge.Y -= offsetY;
					if(!m_header.isCompress)
						fwrite(&pParcel->m_parcel,sizeof(HdParcel) - sizeof(U32),1,m_pIndexFile);	
					else
						fwrite(&pParcel->m_parcel,sizeof(HdParcel),1,m_pIndexFile);	
				}				
			}//for (iBk = 0;iBk < pBlockSet->m_blockSet.m_numBlock;iBk++)
		}//for (iBs = 0;iBs < bsCount;iBs++)
	}//for (i = 0;i < lvlCount;i++)

	return TRUE;
}

//! 写入索引信息
BOOL CHLZWriter::WriteIndex(map<U16,CHdLevel*>& mLevel)
{
	return !m_bNewHlz ? WriteOldIndex(mLevel) : WriteNewIndex(mLevel);
}

BOOL CHLZWriter::WriteOldIndex(map<U16,CHdLevel*>& mLevel)
{
	if (m_pIndexFile == NULL)
	{
		return FALSE;
	}

	// 先抹去原来内容
	fseek(m_pIndexFile, 0, SEEK_END);
	long pos = ftell(m_pIndexFile);
	long size = pos - 256;
	char* cTmp = new char[size];
	memset(cTmp,0,size);
	fseek(m_pIndexFile, 256, SEEK_SET);
	fwrite(cTmp,1,size,m_pIndexFile);
	delete[] cTmp;
	cTmp = NULL;

	// 定位到索引起始位置
	fseek(m_pIndexFile, 256, SEEK_SET);
	int lvlCount = m_header.number_of_level;
	CHdLevel* pLevel = NULL;
	U16 i = 0;
	for (i = 0;i < lvlCount;i++)
	{
		pLevel = mLevel[i];
		fwrite(&(pLevel->m_level),sizeof(HdLevel),1,m_pIndexFile);

		CHdBlockset* pBlockSet = NULL;
		map<I32,CHdBlockset*>::const_iterator itLevel; 
		for (itLevel = pLevel->m_pListBlockset.begin();itLevel != pLevel->m_pListBlockset.end();itLevel++)
		{
			pBlockSet = itLevel->second;
			if (!m_header.isCompress)
				fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset) - sizeof(U32),1,m_pIndexFile);
			else
				fwrite(&pBlockSet->m_blockSet,sizeof(HdBlockset),1,m_pIndexFile);

			// 空的块集记录
			if (pBlockSet->m_blockSet.hasSubBlock == 0)
			{
				continue;
			}

			CHdBlock* pBlock = NULL;
			map<I32,CHdBlock*>::const_iterator itBK;
			for (itBK = pBlockSet->m_pListBlock.begin();itBK != pBlockSet->m_pListBlock.end();itBK++)
			{
				pBlock = itBK->second;
				if (!m_header.isCompress)
					fwrite(&pBlock->m_block,sizeof(HdBlock) - sizeof(U32),1,m_pIndexFile);
				else
					fwrite(&pBlock->m_block,sizeof(HdBlock),1,m_pIndexFile);

				// 添加块内部的包				
				if (pBlock->m_block.hasSubParcel == 0)
				{
					continue;
				}
				CHdParcel* pParcel = NULL;
				map<I32,CHdParcel*>::const_iterator itPcl;
				for (itPcl = pBlock->m_pListParcel.begin();itPcl != pBlock->m_pListParcel.end();itPcl++)
				{
					pParcel = itPcl->second;
					if(!m_header.isCompress)
						fwrite(&pParcel->m_parcel,sizeof(HdParcel) - sizeof(U32),1,m_pIndexFile);	
					else
						fwrite(&pParcel->m_parcel,sizeof(HdParcel),1,m_pIndexFile);	
				}				
			}//for (iBk = 0;iBk < pBlockSet->m_blockSet.m_numBlock;iBk++)
		}//for (iBs = 0;iBs < bsCount;iBs++)
	}//for (i = 0;i < lvlCount;i++)

	return TRUE;
}

BOOL CHLZWriter::WriteNewIndex(map<U16,CHdLevel*>& mLevel)
{
	if (m_pIndexFile == NULL)
	{
		return FALSE;
	}

	// 先抹去原来内容
	fseek(m_pIndexFile, 0, SEEK_END);
	long pos = ftell(m_pIndexFile);
	long size = pos - 256;
	char* cTmp = new char[size];
	memset(cTmp,0,size);
	fseek(m_pIndexFile, 256, SEEK_SET);
	fwrite(cTmp,1,size,m_pIndexFile);
	delete[] cTmp;
	cTmp = NULL;

	// 定位到索引起始位置
	fseek(m_pIndexFile, 256, SEEK_SET);

	//层数
	int level_count = mLevel.size();
	CHdLevel31* level = NULL;
	u16 i = 0;

	//遍历层记录
	for (i = 0; i < level_count; i++)
	{
		level = static_cast<CHdLevel31*>(mLevel[i]);

		fwrite(&(level->m_level31), sizeof(HdLevel31), 1, m_pIndexFile);

		CHdBlockset31* blockset = NULL;
		for (auto it_blockset = level->m_pListBlockset.begin(); it_blockset !=  level->m_pListBlockset.end(); it_blockset++)
		{
			blockset = static_cast<CHdBlockset31*>(it_blockset->second);
			HdBlockset31& blockset_info = blockset->m_blockSet31;

			// 空的块集记录
			//blockNum = 0说明块集没有分块，这时块集索引中含有数据地址信息
			if (blockset_info.getBlockNum() == 0)
			{
				//1.先写块集索引中定长部分
				u16 fix_len = sizeof(HdBlockset31) - 3 * sizeof(HdAddr);
				fwrite(&blockset_info, fix_len, 1, m_pIndexFile);

				//2.判断是否包含颜色、时间数据，决定写入块集索引的长度
				//2.a 包含颜色时间数据
				if(blockset_info.hasColor() && blockset_info.hasTime())
				{
					fwrite(&blockset_info.addrColor, 2 * sizeof(HdAddr), 1, m_pIndexFile);
				}
				//2.b 仅包含颜色数据
				else if(blockset_info.hasColor())
				{
					fwrite(&blockset_info.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
				}
				//2.c 仅包含时间数据
				else if(blockset_info.hasTime())
				{
					fwrite(&blockset_info.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
				}
				//2.d 不包含颜色、时间数据
				else
				{
				}

				//3.判断数据是否压缩
				if(blockset_info.isCompress())
				{
					fwrite(&blockset_info.attriCmpLen, sizeof(HdAddr), 1, m_pIndexFile);
				}

				//当前块集不包含块，直接处理下一个块集
				continue;
			}
			else
			{
				//blockNum > 0说明块集有分块，这时块集索引中不包含数据地址信息及压缩长度信息
				fwrite(&blockset_info, sizeof(HdBlockset31) - 4 * sizeof(HdAddr), 1, m_pIndexFile);
			}

			CHdBlock31* block = NULL;
			for (auto it_block = blockset->m_pListBlock.begin(); it_block != blockset->m_pListBlock.end(); it_block++)
			{
				block = static_cast<CHdBlock31*>(it_block->second);
				HdBlock31 & block_info = block->m_block31;

				// 添加块内部的包	
				//parcelNum = 0说明块没有分包，这时块索引中含有数据地址信息
				if(block_info.numParcel == 0)
				{
					//1.先写入定长索引部分
					fwrite(&block_info, sizeof(HdBlock31) - 3 * sizeof(HdAddr), 1, m_pIndexFile);

					//2.判断是否包含颜色、时间数据，决定写入块索引的长度
					//2.a 包含颜色时间数据
					if(block_info.hasColor() && block_info.hasTime())
					{
						fwrite(&block_info.addrColor, 2 * sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.b 仅包含颜色数据
					else if(block_info.hasColor())
					{
						fwrite(&block_info.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.c 仅包含时间数据
					else if(block_info.hasTime())
					{
						fwrite(&block_info.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.d 不包含颜色、时间数据
					else
					{
					}

					//3.判断数据是否压缩
					if(block_info.isCompress())
					{
						fwrite(&block_info.attriCmpLen, sizeof(HdAddr), 1, m_pIndexFile);
					}

					//当前块不包含包，直接处理下一个块
					continue;
				}
				else
				{
					//parcelNum > 0说明块有分包，这时块索引中不包含数据地址信息及压缩长度信息
					fwrite(&block_info, sizeof(HdBlock31) - 4 * sizeof(HdAddr), 1, m_pIndexFile);
				}

				CHdParcel31* parcel = NULL;
				for (auto it_parcel = block->m_pListParcel.begin(); it_parcel != block->m_pListParcel.end(); it_parcel++)
				{
					parcel = static_cast<CHdParcel31*>(it_parcel->second);
					HdParcel31& parcel_info = parcel->m_parcel31;

					//1.先写包索引中定长部分
					u16 fix_len = sizeof(HdParcel31) - 3* sizeof(HdAddr);
					fwrite(&parcel_info, fix_len, 1, m_pIndexFile);

					//2.判断是否包含颜色、时间数据，决定写入包索引的长度
					//2.a 包含颜色时间数据
					if(parcel_info.hasColor() && parcel_info.hasTime())
					{
						fwrite(&parcel_info.addrColor, 2 * sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.b 仅包含颜色数据
					else if(parcel_info.hasColor())
					{
						fwrite(&parcel_info.addrColor, sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.c 仅包含时间数据
					else if(parcel_info.hasTime())
					{
						fwrite(&parcel_info.addrTime, sizeof(HdAddr), 1, m_pIndexFile);
					}
					//2.d 不包含颜色、时间数据
					else
					{
					}

					//3.判断数据是否压缩
					if(parcel_info.isCompress())
					{
						fwrite(&parcel_info.attriCmpLen, sizeof(HdAddr), 1, m_pIndexFile);
					}

				}//for (it_parcel = block->m_pListParcel.begin(); it_parcel != block->m_pListParcel.end(); it_parcel++)

			}//for (it_block = blockset->m_list_block.begin(); it_block != blockset->m_list_block.end(); it_block++)

		}//for (it_blockset = level->m_pListBlockset.begin(); it_blockset !=  level->m_pListBlockset.end(); it_blockset++)

	}//for (i = 0; i < level_count; i++)


	return TRUE;
}

BOOL CHLZWriter::WriteLevelInfo(HdLevel& level)
{
	if (m_pIndexFile == NULL || m_pDataFile == NULL)
	{
		return FALSE;
	}

	// 定位文件指针到文件末尾 
	fseek(m_pIndexFile, 0, SEEK_END);		// 留出文件头的位置
	
	// 写入二进制流
	fwrite(&level, sizeof(HdLevel), 1, m_pIndexFile);

	return TRUE;
}

//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteBlockSetData(HdBlockset& blockSet, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
	{
		return FALSE;
	}

	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteData(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
	//if(!WriteDataAtOnce(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
		return FALSE;
		
	blockSet.addrCoord.Prase(addrCoord);

	blockSet.addrIntensity.Prase(addrIntensity);

	blockSet.addrColor.Prase(addrColor);

	// 最后将块集索引信息写入索引文件
	fwrite(&blockSet, sizeof(HdBlockset) - sizeof(U32), 1, m_pIndexFile);
	return TRUE;
}

//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteBlockSetData(HdBlockset& blockSet, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U32 count)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
	{
		return FALSE;
	}

	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteCmpData(ptXYZBuf, ptXYZBufSize ,intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
	//if(!WriteCmpDataAtOnce(ptXYZBuf, ptXYZBufSize ,intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
		return FALSE;

	blockSet.addrCoord.Prase(addrCoord);
	blockSet.addrIntensity.Prase(addrIntensity);
	blockSet.addrColor.Prase(addrColor);
	blockSet.cmpSize = ptXYZBufSize;

	// 最后将块集索引信息写入索引文件
	fwrite(&blockSet, sizeof(HdBlockset), 1, m_pIndexFile);	

	return TRUE;
}

//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteBlockData(HdBlock& block, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count)
{
	if (m_pDataFile == NULL || m_pIndexFile == NULL)
	{
		return FALSE;
	}
	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteData(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
	//if(!WriteDataAtOnce(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
		return FALSE;
		
	block.addrCoord.Prase(addrCoord);

	block.addrIntensity.Prase(addrIntensity);

	block.addrColor.Prase(addrColor);

	// 最后将块的索引信息写入索引文件
	fwrite(&block, sizeof(HdBlock)- sizeof(U32), 1, m_pIndexFile);	

	return TRUE;
}

//! 写入块索引和数据, 针对该块中没有分包的情况，压缩
//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteBlockData(HdBlock& block, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count)
{
	if (m_pDataFile == NULL || m_pIndexFile == NULL)
	{
		return FALSE;
	}
	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteCmpData(ptXYZBuf, ptXYZBufSize, intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
	//if(!WriteCmpDataAtOnce(ptXYZBuf, ptXYZBufSize, intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
		return FALSE;

	block.addrCoord.Prase(addrCoord);
	block.addrIntensity.Prase(addrIntensity);
	block.addrColor.Prase(addrColor);
	block.cmpSize = ptXYZBufSize;

	// 最后将块的索引信息写入索引文件
	fwrite(&block, sizeof(HdBlock), 1, m_pIndexFile);	

	return TRUE;
}

//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteParcelData(HdParcel& parcel, const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
	{
		return FALSE;
	}
	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteData(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
	//if(!WriteDataAtOnce(ptBuf,intenBuf,ptColorBuf,count,addrCoord,addrIntensity,addrColor))
		return FALSE;
		
	parcel.addrCoord.Prase(addrCoord);

	parcel.addrIntensity.Prase(addrIntensity);

	parcel.addrColor.Prase(addrColor);

	// 最后将块集索引信息写入索引文件
	fwrite(&parcel, sizeof(HdParcel)- sizeof(U32), 1, m_pIndexFile);	

	return TRUE;
}

//! 写入包索引和数据，针对压缩的情况
//支持同步写入颜色数据信息   袁亮  20160625
BOOL CHLZWriter::WriteParcelData(HdParcel& parcel, const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE || m_pIndexFile == NULL)
	{
		return FALSE;
	}
	fseek(m_pIndexFile, 0, SEEK_END);

	U64 addrCoord = 0;
	U64 addrIntensity = 0;
	U64 addrColor = 0;
	// 写入包数据
	//同步写入颜色数据     袁亮   20160625
	if(!WriteCmpData(ptXYZBuf, ptXYZBufSize, intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
	//if(!WriteCmpDataAtOnce(ptXYZBuf, ptXYZBufSize, intenBuf, ptColorBuf, count,addrCoord,addrIntensity,addrColor))
		return FALSE;

	parcel.addrCoord.Prase(addrCoord);
	parcel.addrIntensity.Prase(addrIntensity);
	parcel.addrColor.Prase(addrColor);
	parcel.cmpSize = ptXYZBufSize;

	// 最后将块集索引信息写入索引文件
	fwrite(&parcel, sizeof(HdParcel), 1, m_pIndexFile);	

	return TRUE;
}

//! 写入块集、块、包数据
//支持同步写入颜色信息，并返回颜色数据的地址   袁亮    20160625
BOOL CHLZWriter::WriteData(const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count,U64& addrCoord,U64& addrIntensity, U64& addrColor)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	/*LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);*/

	//改用内存映射文件方式写hld文件，提高文件IO效率  袁亮    20160909
	U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
	U32 nLeftSpace = 0x80000000 - m_nWrittenLen;
	addrCoord = addrIntensity = addrColor = curFileAddr + m_nWrittenLen;
	//addrCoord = curFileAddr + (U64)li1.QuadPart;

	U32 nWriteLen = sizeof(HdPointXYZ) * count + sizeof(U8) * count + sizeof(HdPtColor) * count;
	//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
	/*if(m_fileFlag == 1)
	{*/
	U64 curDataAddr = addrCoord + nWriteLen;
	if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
	{
		// 关闭上次文件
		if (m_pDataFile!=INVALID_HANDLE_VALUE)
		{
			// 关闭数据文件
			U32 nWriteLen = m_pWriteAddr - m_pMapBegAddr;
			::FlushViewOfFile(m_pMapBegAddr, nWriteLen);
			::UnmapViewOfFile(m_pMapBegAddr);
			CloseHandle(m_fileMap);
			SetFilePointer(m_pDataFile, m_nWrittenLen, NULL, FILE_BEGIN);
			SetEndOfFile(m_pDataFile);
			CloseHandle(m_pDataFile);
			m_pDataFile = NULL;
		}

		// 打开数据文件dir+name-dxxxx.hls			
		m_curFileIndex++;
		curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;

		char strDataFile[256] = {0};
		sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
		m_pDataFile = CreateFile(strDataFile, 
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			CREATE_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);

		if (m_pDataFile == NULL)
		{
			return FALSE;
		}

		m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0x80000000, NULL);
		if(NULL == m_fileMap)
		{
			return FALSE;
		}

		m_nWrittenLen = 0;
		m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, 0, 0x06400000);
		m_pWriteAddr = m_pMapBegAddr;
		m_pMapEndAddr = m_pWriteAddr + 0x06400000;

		addrCoord = addrIntensity = addrColor = curFileAddr;

		/*SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
		addrCoord = curFileAddr + (U64)li1.QuadPart;*/
	}
	//}

	if(m_pWriteAddr + nWriteLen > m_pMapEndAddr)
	{
		U32 nWriteNum = m_pWriteAddr - m_pMapBegAddr;
		::FlushViewOfFile(m_pMapBegAddr, nWriteNum);
		::UnmapViewOfFile(m_pMapBegAddr);

		SYSTEM_INFO sinf;
		::GetSystemInfo(&sinf);
		DWORD dwAllocGran = sinf.dwAllocationGranularity;

		U32 newMapAddr = m_nWrittenLen / dwAllocGran * dwAllocGran;
		U32 offset = m_nWrittenLen - newMapAddr;
		U32 dataLen = MIN(nLeftSpace, 0x06400000);
		m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, newMapAddr, dataLen+offset);
		m_pWriteAddr = m_pMapBegAddr + offset;
		m_pMapEndAddr = m_pWriteAddr + dataLen;
	}


	if(count > 0 && ptBuf)
	{
		memcpy(m_pWriteAddr, ptBuf, (DWORD)count * sizeof(HdPointXYZ));
		m_pWriteAddr += count * sizeof(HdPointXYZ);
		m_nWrittenLen += count * sizeof(HdPointXYZ);
		addrIntensity = addrCoord + count * sizeof(HdPointXYZ);
	}

	if(count > 0 && intenBuf)
	{
		memcpy(m_pWriteAddr, intenBuf, (DWORD)count);
		m_pWriteAddr += count;
		m_nWrittenLen += count;
		addrColor = addrIntensity + count;
	}

	if(count > 0 && ptColorBuf)
	{
		memcpy(m_pWriteAddr, ptColorBuf, (DWORD)count * sizeof(HdPtColor));
		m_pWriteAddr += count * sizeof(HdPtColor);
		m_nWrittenLen += count * sizeof(HdPtColor);
	}

	// 写入坐标
	/*DWORD dwResult;
	if (count > 0 && ptBuf)
	{
		if(!WriteFile (m_pDataFile, ptBuf, (DWORD)count * sizeof(HdPointXYZ), &dwResult, NULL))
			return FALSE;
	}

	// 强度的首地址	
	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
	addrIntensity = curFileAddr + (U64)li1.QuadPart;
	
	// 写入强度
	if (count > 0 && intenBuf)
	{
		if(!WriteFile (m_pDataFile, intenBuf, (DWORD)count, &dwResult, NULL))
			return FALSE;
	}

	//颜色的首地址及颜色数据       袁亮   20160625
	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
	addrColor = curFileAddr + (U64)li1.QuadPart;

	if(count > 0 && ptColorBuf != NULL)
	{
		if(!WriteFile(m_pDataFile, ptColorBuf, (DWORD)count * sizeof(HdPtColor), &dwResult, NULL))
			return FALSE;
	}*/

	return TRUE;
}

//效率不高，已弃用
////坐标、强度、颜色一次性写入，减少IO开销     袁亮   20160805
////确保三个buf同时为NULL且count = 0，或者三个buf同时非NULL且count != 0
//BOOL CHLZWriter::WriteDataAtOnce(const HdPointXYZ* ptBuf, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count,U64& addrCoord,U64& addrIntensity, U64& addrColor)
//{
//	if (m_pDataFile == INVALID_HANDLE_VALUE)
//	{
//		return FALSE;
//	}
//
//	LARGE_INTEGER li;
//	li.HighPart = 0;
//	li.LowPart = 0;
//
//	LARGE_INTEGER li1;
//	li1.HighPart = 0;
//	li1.LowPart = 0;
//
//	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//	U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//	addrCoord = curFileAddr + (U64)li1.QuadPart;
//
//	//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
//	/*if(m_fileFlag == 1)
//	{*/
//		U64 curDataAddr = addrCoord + sizeof(HdPointXYZ) * count + sizeof(U8) * count + sizeof(HdPtColor) * count;
//		if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
//		{
//			// 关闭上次文件
//			if (m_pDataFile!=INVALID_HANDLE_VALUE)
//			{
//				// 关闭数据文件
//				CloseHandle(m_pDataFile);
//				m_pDataFile = NULL;
//			}
//
//			// 打开数据文件dir+name-dxxxx.hls			
//			m_curFileIndex++;
//			curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//
//			char strDataFile[256] = {0};
//			sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
//			m_pDataFile = CreateFile(strDataFile, 
//				GENERIC_READ | GENERIC_WRITE,
//				FILE_SHARE_READ|FILE_SHARE_WRITE, 
//				NULL,
//				CREATE_ALWAYS, 
//				FILE_ATTRIBUTE_NORMAL, 
//				NULL);
//
//			if (m_pDataFile == NULL)
//			{
//				return FALSE;
//			}
//
//			SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//			addrCoord = curFileAddr + (U64)li1.QuadPart;
//		}
//	//}
//
//	//当前空间实体有包含下层实体，此时仅仅更新空间索引，不写入空间数据
//	if(ptBuf == NULL && intenBuf == NULL && ptColorBuf == NULL)
//	{
//		addrColor = addrIntensity = addrCoord;
//		return TRUE;
//	}
//
//	//三个buf写到一个buf中
//	U64   offset = 0;
//	U64   bufsize = (sizeof(HdPointXYZ) + sizeof(U8) + sizeof(HdPtColor)) * count;
//	char* buf = new char[bufsize];
//	memcpy(buf, ptBuf, sizeof(HdPointXYZ) * count);
//	offset += sizeof(HdPointXYZ) * count;
//	addrIntensity = addrCoord + offset;
//	memcpy(buf+offset, intenBuf, sizeof(U8) * count);
//	offset += sizeof(U8) * count;
//	addrColor = addrCoord + offset;
//	memcpy(buf+offset, ptColorBuf, sizeof(HdPtColor) * count);
//
//	DWORD dwResult;
//	if(!WriteFile(m_pDataFile, buf, bufsize, &dwResult, NULL))
//	{
//		delete[] buf;
//		return FALSE;
//	}
//
//	delete[] buf;
//	return TRUE;
//}

//! 写入块集、块、包数据
//支持同步写入颜色信息，并返回颜色数据的地址   袁亮    20160625
BOOL CHLZWriter::WriteCmpData(const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count,U64& addrCoord,U64& addrIntensity, U64& addrColor)
{
	if (m_pDataFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	/*LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);*/

	//改用内存映射文件方式写hld文件，提高文件IO效率  袁亮    20160909
	U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
	U32 nLeftSpace = 0x80000000 - m_nWrittenLen;
	addrCoord = addrIntensity = addrColor = curFileAddr + m_nWrittenLen;

	U32 nWriteLen = ptXYZBufSize + sizeof(U8) * count + sizeof(HdPtColor) * count;
	//addrCoord = curFileAddr + (U64)li1.QuadPart;

	//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
	/*if(m_fileFlag == 1)
	{*/
		//U64 curDataAddr = addrCoord + sizeof(HdPointXYZ) * count + sizeof(U8) * count + sizeof(HdPtColor) * count;
	    U64 curDataAddr = addrCoord + nWriteLen;
		if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
		{
			// 关闭上次文件
			if (m_pDataFile!=INVALID_HANDLE_VALUE)
			{
				// 关闭数据文件
				U32 nWriteLen = m_pWriteAddr - m_pMapBegAddr;
				::FlushViewOfFile(m_pMapBegAddr, nWriteLen);
				::UnmapViewOfFile(m_pMapBegAddr);
				CloseHandle(m_fileMap);
				SetFilePointer(m_pDataFile, m_nWrittenLen, NULL, FILE_BEGIN);
				SetEndOfFile(m_pDataFile);
				CloseHandle(m_pDataFile);
				m_pDataFile = NULL;
			}

			// 打开数据文件dir+name-dxxxx.hls			
			m_curFileIndex++;
			curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;

			char strDataFile[256] = {0};
			sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
			m_pDataFile = CreateFile(strDataFile, 
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ|FILE_SHARE_WRITE, 
				NULL,
				CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if (m_pDataFile == NULL)
			{
				return FALSE;
			}

			m_fileMap = ::CreateFileMapping(m_pDataFile, NULL, PAGE_READWRITE, 0, 0x80000000, NULL);
			if(NULL == m_fileMap)
			{
				return FALSE;
			}

			m_nWrittenLen = 0;
			m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, 0, 0x06400000);
			m_pWriteAddr = m_pMapBegAddr;
			m_pMapEndAddr = m_pWriteAddr + 0x06400000;

			addrCoord = addrIntensity = addrColor = curFileAddr;

			/*SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
			addrCoord = curFileAddr + (U64)li1.QuadPart;*/
		}
		//}

		if(m_pWriteAddr + nWriteLen > m_pMapEndAddr)
		{
			U32 nWriteNum = m_pWriteAddr - m_pMapBegAddr;
			::FlushViewOfFile(m_pMapBegAddr, nWriteNum);
			::UnmapViewOfFile(m_pMapBegAddr);

			SYSTEM_INFO sinf;
			::GetSystemInfo(&sinf);
			DWORD dwAllocGran = sinf.dwAllocationGranularity;

			U32 newMapAddr = m_nWrittenLen / dwAllocGran * dwAllocGran;
			U32 offset = m_nWrittenLen - newMapAddr;
			U32 dataLen = MIN(nLeftSpace, 0x06400000);
			m_pMapBegAddr = (char*)::MapViewOfFile(m_fileMap, FILE_MAP_WRITE, 0, newMapAddr, dataLen+offset);
			m_pWriteAddr = m_pMapBegAddr + offset;
			m_pMapEndAddr = m_pWriteAddr + dataLen;
		}


		if(count > 0 && ptXYZBuf > 0)
		{
			memcpy(m_pWriteAddr, ptXYZBuf, (DWORD)ptXYZBufSize);
			m_pWriteAddr += ptXYZBufSize;
			m_nWrittenLen += ptXYZBufSize;
			addrIntensity = addrCoord + ptXYZBufSize;
		}

		if(count > 0 && intenBuf)
		{
			memcpy(m_pWriteAddr, intenBuf, (DWORD)count);
			m_pWriteAddr += count;
			m_nWrittenLen += count;
			addrColor = addrIntensity + count;
		}

		if(count > 0 && ptColorBuf)
		{
			memcpy(m_pWriteAddr, ptColorBuf, (DWORD)count * sizeof(HdPtColor));
			m_pWriteAddr += count * sizeof(HdPtColor);
			m_nWrittenLen += count * sizeof(HdPtColor);
		}

		// 写入坐标
		/*DWORD dwResult;
		if (count > 0 && ptXYZBuf)
		{
		// 写入数据
		if(!WriteFile (m_pDataFile, ptXYZBuf, (DWORD)ptXYZBufSize, &dwResult, NULL))
		return FALSE;
		}

		// 强度的首地址	
		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
		addrIntensity = curFileAddr + (U64)li1.QuadPart;

		// 写入强度
		if (count > 0 && intenBuf)
		{
		if(!WriteFile (m_pDataFile, intenBuf, (DWORD)count, &dwResult, NULL))
		return FALSE;
		}

		//颜色的首地址及颜色数据       袁亮   20160625
		SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
		addrColor = curFileAddr + (U64)li1.QuadPart;

		if(count > 0 && ptColorBuf != NULL)
		{
		if(!WriteFile(m_pDataFile, ptColorBuf, (DWORD)count * sizeof(HdPtColor), &dwResult, NULL))
		return FALSE;
		}*/

		return TRUE;
}

//效率不高，已弃用
////坐标、强度、颜色一次性写入，减少IO开销     袁亮   20160805
////确保三个buf同时为NULL且count = 0，或者三个buf同时非NULL且count != 0
//BOOL CHLZWriter::WriteCmpDataAtOnce(const U8* ptXYZBuf, const U32 ptXYZBufSize, const U8* intenBuf, const HdPtColor* ptColorBuf, U64 count,U64& addrCoord,U64& addrIntensity, U64& addrColor)
//{
//	if (m_pDataFile == INVALID_HANDLE_VALUE)
//	{
//		return FALSE;
//	}
//
//	LARGE_INTEGER li;
//	li.HighPart = 0;
//	li.LowPart = 0;
//
//	LARGE_INTEGER li1;
//	li1.HighPart = 0;
//	li1.LowPart = 0;
//
//	SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//	U64 curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//	addrCoord = curFileAddr + (U64)li1.QuadPart;
//
//	//不论文件系统，hlz数据文件都按照2GB大小进行划分    袁亮  20160818
//	/*if(m_fileFlag == 1)
//	{*/
//		U64 curDataAddr = addrCoord + sizeof(HdPointXYZ) * count + sizeof(U8) * count + sizeof(HdPtColor) * count;
//		if(curDataAddr > curFileAddr + 0x80000000)//0x80000000 2GB
//		{
//			// 关闭上次文件
//			if (m_pDataFile!=INVALID_HANDLE_VALUE)
//			{
//				// 关闭数据文件
//				CloseHandle(m_pDataFile);
//				m_pDataFile = NULL;
//			}
//
//			// 打开数据文件dir+name-dxxxx.hls			
//			m_curFileIndex++;
//			curFileAddr = (U64)m_curFileIndex * (U64)0x80000000;
//
//			char strDataFile[256] = {0};
//			sprintf(strDataFile,"%s\\%s-%04d.hld",m_dir,m_name,m_curFileIndex);
//			m_pDataFile = CreateFile(strDataFile, 
//				GENERIC_READ | GENERIC_WRITE,
//				FILE_SHARE_READ|FILE_SHARE_WRITE, 
//				NULL,
//				CREATE_ALWAYS, 
//				FILE_ATTRIBUTE_NORMAL, 
//				NULL);
//
//			if (m_pDataFile == NULL)
//			{
//				return FALSE;
//			}
//
//			SetFilePointerEx(m_pDataFile,li,&li1,FILE_END);
//			addrCoord = curFileAddr + (U64)li1.QuadPart;
//		}
//	//}
//
//	if(ptXYZBuf == NULL && intenBuf == NULL && ptColorBuf == NULL)
//	{
//		addrColor = addrIntensity = addrCoord;
//		return TRUE;
//	}
//
//	//三个buf写到一个buf中
//	U64   offset = 0;
//	U64   bufsize = (sizeof(U8) + sizeof(HdPtColor)) * count + ptXYZBufSize;
//	char* buf = new char[bufsize];
//	memcpy(buf, ptXYZBuf, ptXYZBufSize);
//	offset += ptXYZBufSize;
//	addrIntensity = addrCoord + offset;
//	memcpy(buf+offset, intenBuf, sizeof(U8) * count);
//	offset += sizeof(U8) * count;
//	addrColor = addrCoord + offset;
//	memcpy(buf+offset, ptColorBuf, sizeof(HdPtColor) * count);
//
//	DWORD dwResult;
//	if(!WriteFile(m_pDataFile, buf, bufsize, &dwResult, NULL))
//	{
//		delete[] buf;
//		return FALSE;
//	}
//
//	delete[] buf;
//	return TRUE;
//}

// 云存储中用于根据当前层的数据块集进行数据坐标偏移，满足所有点数据坐标都是针对当前块集原点
U64 CHLZWriter::WriteNextLevelBlockset(const char* savePath, std::vector<BlockSetFileInfo>& preBlocksetFiles, HdBlockset& curBlockset,CHdLevel* curLevel,U32 curBlockSetNo)
{
	// 打开保存文件
	HANDLE pBlckstDataFile = CreateFile(savePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (pBlckstDataFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
	

	I32 simpleSize = 0;
	I32 nCount = 0;
	U32 Index = 0;
	int err;
	// 计算当前块集编号
	U32 nBlockSetY = curBlockSetNo / curLevel->m_level.numBlocksetX;
	U32 nBlockSetX = curBlockSetNo - nBlockSetY * curLevel->m_level.numBlocksetX;
	// 得到当前块集坐标原点作为偏移量
	F32 BlockSetStartX = nBlockSetX * curLevel->m_level.sizeX;
	F32 BlockSetStartY = nBlockSetY * curLevel->m_level.sizeY;

	F32 offsetX = 0.0;
	F32 offsetY = 0.0;
	// 根据当前层信息进行点云坐标偏移，保证当前块集内的点云相对坐标原点为块集角点
	for (U32 idx = 0; idx < preBlocksetFiles.size(); idx++)
	{
		// 从块集文件中一次读取6.4w点，然后进行随机抽取后写入到当前块集文件
 		BlockSetFileInfo& preBsFile = *(preBlocksetFiles._Myfirst() + idx);
		offsetX = preBsFile.box.MinEdge.X - BlockSetStartX;
		offsetY = preBsFile.box.MinEdge.Y - BlockSetStartY;

		HANDLE pDataFile = CreateFile(preBsFile.path.c_str(), 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			continue;
		}
 		
 		std::vector<PointXYZIPRGBA> bufferPts;
 		while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
 		{
 			nCount = bufferPts.size();
			simpleSize = nCount / SIMPLE_SCALE;
 			// 抽取的点数为内存中1/4
 			std::vector<PointXYZIPRGBA> samplePts;
 			samplePts.resize(simpleSize);
			
 			for (I32 i=0; i < simpleSize; i++)
 			{
 				// 随机抽取一个数
 				randomGenerator.drawUniformUnsignedIntRange(Index, 0, nCount-1);
 
 				PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
 				pPt = *(bufferPts._Myfirst() + Index);
				pPt.x += offsetX;
				pPt.y += offsetY;
 			}
 			
 			// 将抽取的点写入			
			DWORD reslt;
			SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
			WriteFile(pBlckstDataFile,samplePts._Myfirst(),sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

			curBlockset.numPoint += simpleSize;
 		}
 
		if (pDataFile!=INVALID_HANDLE_VALUE)
		{
	        CloseHandle(pDataFile);
			pDataFile = NULL;
		}
		// 将块集文件删除，节省磁盘空间
		err = remove(preBsFile.path.c_str());
		if (err == -1)
		{
			perror("无法删除块集文件WriteNextLevelBlockset");
		}
	}
	
	// 更新当前块的空间范围
	curBlockset.box = preBlocksetFiles[0].box;
	for (I32 i = 0; i < preBlocksetFiles.size(); i++)
	{
		BlockSetFileInfo &blckFile = preBlocksetFiles[i];

		//curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
		//curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.X = min(BlockSetStartX, blckFile.box.MinEdge.X);
		curBlockset.box.MinEdge.Y = min(BlockSetStartY, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);

		//curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
		//curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.X = max(BlockSetStartX + curLevel->m_level.sizeX, blckFile.box.MaxEdge.X);
		curBlockset.box.MaxEdge.Y = max(BlockSetStartY + curLevel->m_level.sizeY, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
	}
    if (pBlckstDataFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(pBlckstDataFile);

		pBlckstDataFile = NULL;
	}
	
	return curBlockset.numPoint;
}
U64 CHLZWriter::WriteNextLevelBlockset(const char* savePath, std::vector<BlockSetFileInfo>& preBlocksetFiles, HdBlockset& curBlockset)
{
	// 打开保存文件
	HANDLE pBlckstDataFile = CreateFile(savePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (pBlckstDataFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
	

	I32 simpleSize = 0;
	I32 nCount = 0;
	U32 Index = 0;
	int err;
	for (U32 idx = 0; idx < preBlocksetFiles.size(); idx++)
	{
		// 从块集文件中一次读取6.4w点，然后进行随机抽取后写入到当前块集文件
 		BlockSetFileInfo& preBsFile = *(preBlocksetFiles._Myfirst() + idx);
 		

		HANDLE pDataFile = CreateFile(preBsFile.path.c_str(), 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			continue;
		}
 		
 		std::vector<PointXYZIPRGBA> bufferPts;
 		while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
 		{
 			nCount = bufferPts.size();
			simpleSize = nCount / SIMPLE_SCALE;
 			// 抽取的点数为内存中1/4
 			std::vector<PointXYZIPRGBA> samplePts;
 			samplePts.resize(simpleSize);
			
 			for (I32 i=0; i < simpleSize; i++)
 			{
 				// 随机抽取一个数
 				randomGenerator.drawUniformUnsignedIntRange(Index, 0, nCount-1);
 
 				PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
 				pPt = *(bufferPts._Myfirst() + Index);
 			}
 			
 			// 将抽取的点写入			
			DWORD reslt;
			SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
			WriteFile(pBlckstDataFile,samplePts._Myfirst(),sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

			curBlockset.numPoint += simpleSize;
 		}
 
		if (pDataFile!=INVALID_HANDLE_VALUE)
		{
	        CloseHandle(pDataFile);
			pDataFile = NULL;
		}
		// 将块集文件删除，节省磁盘空间
		err = remove(preBsFile.path.c_str());
		if (err == -1)
		{
			perror("无法删除块集文件WriteNextLevelBlockset");
		}
	}
	
	// 更新当前块的空间范围
	curBlockset.box = preBlocksetFiles[0].box;
	for (I32 i = 1; i < preBlocksetFiles.size(); i++)
	{
		BlockSetFileInfo &blckFile = preBlocksetFiles[i];

		curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
		curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);

		curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
		curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
	}
    if (pBlckstDataFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(pBlckstDataFile);

		pBlckstDataFile = NULL;
	}
	
	return curBlockset.numPoint;
}

BOOL CHLZWriter::Close()
{
	if(m_pMapBegAddr != NULL)
	{
		U32 nWriteLen = m_pWriteAddr - m_pMapBegAddr;
		::FlushViewOfFile(m_pMapBegAddr, nWriteLen);
		::UnmapViewOfFile(m_pMapBegAddr);
		m_pMapBegAddr = NULL;

		if(m_fileMap != NULL)
		{
			CloseHandle(m_fileMap);
			m_fileMap = NULL;
		}

		SetFilePointer(m_pDataFile, m_nWrittenLen, NULL, FILE_BEGIN);
		SetEndOfFile(m_pDataFile);
	}

	if(m_fileMap != NULL)
	{
		CloseHandle(m_fileMap);
		m_fileMap = NULL;
	}

	if (m_pDataFile!=INVALID_HANDLE_VALUE)
	{
		// 关闭数据文件
		CloseHandle(m_pDataFile);
		m_pDataFile = NULL;
	}

	if (m_pColorFile)
	{
		// 关闭颜色数据文件
	}

	if (m_pPropFile)
	{
		// 关闭分类数据文件
	}

	if (m_pIndexFile)
	{	
		// 关闭索引文件
		WriteHeader();
		fclose(m_pIndexFile);
		m_pIndexFile = NULL;
	}

	if (m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	m_pDataFile = (NULL);
	m_pColorFile = (NULL);
	m_pPropFile = (NULL);
	m_pIndexFile = (NULL);

	return TRUE;
}

void CHLZWriter::WriteLogFile(const char* logStr)
{
	// log文件路径
	std::string logPath = m_dir;
	logPath += m_name;
	logPath += "_writelog.txt";

	if (m_pLogFile == NULL)
	{
		m_pLogFile = fopen(logPath.c_str(), "w+t");
	}
	
	if (m_pLogFile)
	{
		fprintf(m_pLogFile, logStr);
	}
}

U32 CHLZWriter::ReadPtsBuffer(HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer)
{
	if (pFile == NULL)
	{
		return 0;
	}

	U64 bufferSize = 128000;		// 固定读取12.8w个点到内存

	LARGE_INTEGER liMove,li1,li2;
	liMove.LowPart = 0;
	liMove.HighPart = 0;

	li1.LowPart = 0;
	li1.HighPart = 0;

	li2.LowPart = 0;
	li2.HighPart = 0;

	SetFilePointerEx(pFile,liMove,&li1,FILE_CURRENT);
	U64 nCur = ((U64)li1.HighPart<<32)|li1.LowPart;

	SetFilePointerEx(pFile,liMove,&li2,FILE_END);
	U64 nEnd = ((U64)li2.HighPart<<32)|li2.LowPart;

	// 判断剩余点数
	U64 nLeftSize = (nEnd - nCur)/(sizeof(PointXYZIPRGBA));
	
	if (nLeftSize < bufferSize)
	{
		bufferSize = nLeftSize;
	}

	SetFilePointerEx(pFile,li1,&liMove,FILE_BEGIN);

	vecBuffer.resize(bufferSize);
	DWORD numRead = 0;
	ReadFile(pFile,vecBuffer._Myfirst(),bufferSize*sizeof(PointXYZIPRGBA),&numRead,NULL);
	
	return vecBuffer.size();
}

//! 从临时文件中读取一定数量的点到内存，外部传入传出读取多少点数，vec由外部一次申请较大内存
U32 CHLZWriter::ReadPtsBuffer( HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer,U64& readCount )
{
	if (pFile == NULL)
	{
		return 0;
	}

	U64 bufferSize = 128000;		// 固定读取12.8w个点到内存

	LARGE_INTEGER liMove,li1,li2;
	liMove.LowPart = 0;
	liMove.HighPart = 0;

	li1.LowPart = 0;
	li1.HighPart = 0;

	li2.LowPart = 0;
	li2.HighPart = 0;

	SetFilePointerEx(pFile,liMove,&li1,FILE_CURRENT);
	U64 nCur = ((U64)li1.HighPart<<32)|li1.LowPart;

	SetFilePointerEx(pFile,liMove,&li2,FILE_END);
	U64 nEnd = ((U64)li2.HighPart<<32)|li2.LowPart;

	// 判断剩余点数
	U64 nLeftSize = (nEnd - nCur)/(sizeof(PointXYZIPRGBA));

	if (nLeftSize < bufferSize)
	{
		bufferSize = nLeftSize;
	}

	SetFilePointerEx(pFile,li1,&liMove,FILE_BEGIN);

	//vecBuffer.resize(bufferSize);
	DWORD numRead = 0;
	ReadFile(pFile,vecBuffer._Myfirst(),bufferSize*sizeof(PointXYZIPRGBA),&numRead,NULL);
	readCount = bufferSize;

	return readCount;
	//return vecBuffer.size();
}

U32 CHLZWriter::ReadAllPts(FILE* &pFile, std::vector<PointXYZIPRGBA>& vecBuffer)
{
	if (pFile == NULL)
	{
		return 0;
	}
	// 判断剩余点数;
	fseek(pFile, 0, SEEK_END);
	U32 nEnd = ftell(pFile);
	U32 bufferSize = nEnd/(sizeof(PointXYZIPRGBA));
	
	try
	{
		vecBuffer.resize(bufferSize);
 		fseek(pFile, 0, SEEK_SET);		// 定位到原来的位置
 		fread(vecBuffer._Myfirst(), sizeof(PointXYZIPRGBA), bufferSize, pFile);
		
		return vecBuffer.size();
	}
	catch (...)
	{
		::MessageBox(NULL,"内存分配出错，请提高内存配置或采用\"临时文件方式\"", NULL, MB_OK);
		return -1;
	}
	
}

U32 CHLZWriter::ReadPtsBySize(HANDLE pFile, std::vector<PointXYZIPRGBA>& vecBuffer, U64 bufferSize)
{
	if (pFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

//	U32 bufferSize = 128000;		// 固定读取12.8w个点到内存

	LARGE_INTEGER liMove,li1,li2;
	liMove.LowPart = 0;
	liMove.HighPart = 0;

	li1.LowPart = 0;
	li1.HighPart = 0;

	li2.LowPart = 0;
	li2.HighPart = 0;

	SetFilePointerEx(pFile,liMove,&li1,FILE_CURRENT);
	U64 nCur = ((U64)li1.HighPart<<32)|li1.LowPart;

	SetFilePointerEx(pFile,liMove,&li2,FILE_END);
	U64 nEnd = ((U64)li2.HighPart<<32)|li2.LowPart;

	// 判断剩余点数
	U64 nLeftSize = (nEnd - nCur)/(sizeof(PointXYZIPRGBA));

	if (nLeftSize < bufferSize)
	{
		bufferSize = nLeftSize;
	}

	SetFilePointerEx(pFile,li1,&liMove,FILE_BEGIN);

	vecBuffer.resize(bufferSize);
	DWORD numRead = 0;
	ReadFile(pFile,vecBuffer._Myfirst(),bufferSize*sizeof(PointXYZIPRGBA),&numRead,NULL);
	
	return vecBuffer.size();
}

// 根据上一层写入本层级blockset临时文件,根据格网大小均匀抽稀
U64 CHLZWriter::WriteNextLevelBlocksetByMid( const char* savePath, std::vector<BlockSetFileInfo>& preBlocksetFiles, HdBlockset& curBlockset, U8 curLevel )
{
	// 暂时固定0层精度为1/256.0，后续每层精度相对缩减2倍，该接口调用精度一层约为0.0078m，点间距9mm-10mm
	double dStepPrecision = pow(3.0,curLevel) * m_dGridPrecision;
	double dAntiStep = 1.0 / dStepPrecision;

	// 打开保存文件
	HANDLE pBlckstDataFile = CreateFile(savePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (pBlckstDataFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);

	I32 simpleSize = 0;
	I32 nCount = 0;
	U32 Index = 0;
	int err;
	for (U32 idx = 0; idx < preBlocksetFiles.size(); idx++)
	{
		// 从块集文件中一次读取6.4w点，然后进行随机抽取后写入到当前块集文件
		BlockSetFileInfo& preBsFile = *(preBlocksetFiles._Myfirst() + idx);

		HANDLE pDataFile = CreateFile(preBsFile.path.c_str(), 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		/*std::vector<PointXYZIPRGBA> bufferPts;
		bufferPts.resize(128000);
		U64 nReadCount = 0;*/

		// 使用vector记录
		std::vector<PosKey> vecPosKeys;
		vecPosKeys.resize(128000);
		//vecPosKeys.resize(bufferPts.size());

		// 记录索引值
		std::vector<int> vecTargetIndex;
		vecTargetIndex.resize(vecPosKeys.size());

		// 采样结果值
		std::vector<PointXYZIPRGBA> samplePts;
		samplePts.resize(vecPosKeys.size());

		// 记录前一层级块集包围盒
		CHdBox3df preBsBox = preBsFile.box;

		//使用内存映射文件读取块中的点数据   袁亮   20160909
		SYSTEM_INFO sinf;
		::GetSystemInfo(&sinf);
		DWORD dwAllocGran = sinf.dwAllocationGranularity;
		//每次映射加载500W个点(5000000*18/1024/1024 = 85.8M)
		DWORD dwDataLen = 5000000 * sizeof(PointXYZIPRGBA);

		DWORD dwFileSizeHigh;
		U64 qwFileSize = ::GetFileSize(pDataFile, &dwFileSizeHigh);
		qwFileSize += (((U64)dwFileSizeHigh) << 32);

		HANDLE hFileMap = ::CreateFileMapping(pDataFile, NULL, PAGE_READONLY, 0, 0, NULL);

		U32 bufferSize = 128000; //按照12.8W个点分批进行网格抽稀工作
		U32 dwOffset = 0, dwMapLen = 0;
		U64 qwFileOffset = 0, qwMapAddr = 0;
		while(qwFileSize > 0)
		{
			if(qwFileSize < dwDataLen)
			{
				dwDataLen = (DWORD)qwFileSize;
			}

			qwMapAddr = qwFileOffset / dwAllocGran * dwAllocGran;
			dwOffset = qwFileOffset - qwMapAddr;
			dwMapLen = dwDataLen + dwOffset;

			DWORD dwError;
			char* pData = (char*)::MapViewOfFile(hFileMap, FILE_MAP_READ, (DWORD)(qwMapAddr >> 32), (DWORD)(qwMapAddr & 0xFFFFFFFF), dwMapLen);
			PointXYZIPRGBA* pPtData = (PointXYZIPRGBA*)(pData + dwOffset);
			U32 nPtNum = dwDataLen / sizeof(PointXYZIPRGBA);
			while(nPtNum > 0)
			{
				double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
				dMinX = dMinY = dMinZ = F32_MAX;
				dMaxX = dMaxY = dMaxZ = F32_MIN;

				// 定义中间变量
				double dCenterX = 0.0;
				double dCenterY = 0.0;
				double dCenterZ = 0.0;

				PointXYZIPRGBA* pTmp = pPtData;
				U32 num = nPtNum > bufferSize ? bufferSize : nPtNum;
				for (U32 i=0; i < num; i++)
				{
					PointXYZIPRGBA& pts = *pTmp++;
					dMinX = MIN(dMinX,pts.x);
					dMinY = MIN(dMinY,pts.y);
					dMinZ = MIN(dMinZ,pts.z);
					dMaxX = MAX(dMaxX,pts.x);
					dMaxY = MAX(dMaxY,pts.y);
					dMaxZ = MAX(dMaxZ,pts.z);
				}

				// 根据范围及精度确定格网个数
				int xRows = ceil((dMaxX - dMinX) * dAntiStep);
				int yRows = ceil((dMaxY - dMinY) * dAntiStep);
				int zRows = ceil((dMaxZ - dMinZ) * dAntiStep);

				// 定义中间变量
				int xStep = 0;
				int yStep = 0;
				int zStep = 0;
				int nTmp = 0;
				simpleSize = 0;

				// 格网划分处理
				pTmp = pPtData;
				for (U32 n = 0;n < num; n++)
				{
					PointXYZIPRGBA& pts = *pTmp++;

					// 计算对应格网索引值
					xStep = ceil((pts.x - dMinX) * dAntiStep);
					yStep = ceil((pts.y - dMinY) * dAntiStep);
					zStep = ceil((pts.z - dMinZ) * dAntiStep);

					double dTmpX,dTmpY,dTmpZ;
					dTmpX = dMinX + (xStep +  0.5) * dStepPrecision - pts.x;
					dTmpY = dMinY + (yStep +  0.5) * dStepPrecision - pts.y;
					dTmpZ = dMinZ + (zStep +  0.5) * dStepPrecision - pts.z;

					// 对应键为
					U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

					// 插入值
					PosKey posKey;
					posKey.nIndex = n;

					// 计算获取距离box中心最近点
					posKey.dDist = pow(dTmpX,2.0) + pow(dTmpY,2.0) + pow(dTmpZ,2.0);
					posKey.gridIndex = mapPin;
					*(vecPosKeys._Myfirst() + n) = posKey;
				}

				// 针对vec进行处理排序
				std::sort(vecPosKeys.begin(),vecPosKeys.begin() + num, SortByGrid);

				// 定义中间变量
				PosKey& pos = *(vecPosKeys._Myfirst() + 0);
				U64 nCurGridPos = pos.gridIndex;
				*(vecTargetIndex._Myfirst() + 0) = pos.nIndex;
				int nCurIndex = 1;

				// 根据之前记录的距离值，grid相同的比较距离，从小到大排序
				for (unsigned int nn = 1;nn < num; nn++)
				{
					PosKey& pos = *(vecPosKeys._Myfirst() + nn);
					if (pos.gridIndex == nCurGridPos)
					{
						// 之前已按距离排序，同一格网距离中心最小为最前索引值
						continue;
					}
					else
					{
						// 处理到下一个格网时，记录
						nCurGridPos = pos.gridIndex;
						*(vecTargetIndex._Myfirst() + nCurIndex) = pos.nIndex;
						nCurIndex++;
					}
				}

				// 抽取的点数赋值
				simpleSize = nCurIndex;

				// 点云更新处理
				for (I32 i=0; i < simpleSize; i++)
				{
					int nTmpIndex = *(vecTargetIndex._Myfirst() + i);
					PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
					pPt = *(pPtData + nTmpIndex);
					//pPt = *(bufferPts._Myfirst() + nTmpIndex);
				}

				// 将抽取的点写入			
				DWORD reslt;
				SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
				WriteFile(pBlckstDataFile,samplePts._Myfirst(),sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

				curBlockset.numPoint += simpleSize;

				pPtData += num;
				nPtNum -= num;
			}

			::UnmapViewOfFile(pData);
			qwFileOffset += dwDataLen;
			qwFileSize -= dwDataLen;
		}

		::CloseHandle(hFileMap);
		::CloseHandle(pDataFile);
		
		//while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
		/*while (ReadPtsBuffer(pDataFile, bufferPts,nReadCount) > 0)
		{
			// 定义单次读取范围box
			double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
			dMinX = dMinY = dMinZ = F32_MAX;
			dMaxX = dMaxY = dMaxZ = F32_MIN;

			// 定义中间变量
			double dCenterX = 0.0;
			double dCenterY = 0.0;
			double dCenterZ = 0.0;

			//// 格网包围盒存在数据不完整情况，仍采用内部统计范围
			//dMinX = preBsBox.MinEdge.X;
			//dMinY = preBsBox.MinEdge.Y;
			//dMinZ = preBsBox.MinEdge.Z;
			//dMaxX = preBsBox.MaxEdge.X;
			//dMaxY = preBsBox.MaxEdge.Y;
			//dMaxZ = preBsBox.MaxEdge.Z;

			// 确定格网范围
			for (unsigned int n = 0;n < nReadCount;n++)
			{
				PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
				dMinX = MIN(dMinX,pts.x);
				dMinY = MIN(dMinY,pts.y);
				dMinZ = MIN(dMinZ,pts.z);
				dMaxX = MAX(dMaxX,pts.x);
				dMaxY = MAX(dMaxY,pts.y);
				dMaxZ = MAX(dMaxZ,pts.z);
			}

			// 根据范围及精度确定格网个数
			int xRows = ceil((dMaxX - dMinX) * dAntiStep);
			int yRows = ceil((dMaxY - dMinY) * dAntiStep);
			int zRows = ceil((dMaxZ - dMinZ) * dAntiStep);

			// 定义中间变量
			int xStep = 0;
			int yStep = 0;
			int zStep = 0;
			int nTmp = 0;
			simpleSize = 0;

			// 格网划分处理
			for (unsigned int n = 0;n < nReadCount;n++)
			{
				PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);

				//// 计算对应格网索引值
				//xStep = floor((pts.x - dMinX) * dAntiStep);
				//yStep = floor((pts.y - dMinY) * dAntiStep);
				//zStep = floor((pts.z - dMinZ) * dAntiStep);

				// 计算对应格网索引值
				xStep = ceil((pts.x - dMinX) * dAntiStep);
				yStep = ceil((pts.y - dMinY) * dAntiStep);
				zStep = ceil((pts.z - dMinZ) * dAntiStep);

				double dTmpX,dTmpY,dTmpZ;
				dTmpX = dMinX + (xStep +  0.5) * dStepPrecision - pts.x;
				dTmpY = dMinY + (yStep +  0.5) * dStepPrecision - pts.y;
				dTmpZ = dMinZ + (zStep +  0.5) * dStepPrecision - pts.z;

				// 对应键为
				U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

				// 插入值
				PosKey posKey;
				posKey.nIndex = n;

				// 计算获取距离box中心最近点
				posKey.dDist = pow(dTmpX,2.0) + pow(dTmpY,2.0) + pow(dTmpZ,2.0);
				posKey.gridIndex = mapPin;
				*(vecPosKeys._Myfirst() + n) = posKey;
			}

			// 针对vec进行处理排序
			std::sort(vecPosKeys.begin(),vecPosKeys.begin() + nReadCount,SortByGrid);

			// 定义中间变量
			PosKey& pos = *(vecPosKeys._Myfirst() + 0);
			U64 nCurGridPos = pos.gridIndex;
			*(vecTargetIndex._Myfirst() + 0) = pos.nIndex;
			int nCurIndex = 1;

			// 根据之前记录的距离值，grid相同的比较距离，从小到大排序
			for (unsigned int nn = 1;nn < nReadCount;nn++)
			{
				PosKey& pos = *(vecPosKeys._Myfirst() + nn);
				if (pos.gridIndex == nCurGridPos)
				{
					// 之前已按距离排序，同一格网距离中心最小为最前索引值
					continue;
				}
				else
				{
					// 处理到下一个格网时，记录
					nCurGridPos = pos.gridIndex;
					*(vecTargetIndex._Myfirst() + nCurIndex) = pos.nIndex;
					nCurIndex++;
				}
			}

			// 抽取的点数赋值
			simpleSize = nCurIndex;

			// 点云更新处理
			for (I32 i=0; i < simpleSize; i++)
			{
				int nTmpIndex = *(vecTargetIndex._Myfirst() + i);
				PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
				pPt = *(bufferPts._Myfirst() + nTmpIndex);
			}

			// 将抽取的点写入			
			DWORD reslt;
			SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
			WriteFile(pBlckstDataFile,samplePts._Myfirst,sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

			curBlockset.numPoint += simpleSize;
		}

		// 文件关闭
		if (pDataFile!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(pDataFile);
			pDataFile = NULL;
		}*/
		
		// 将块集文件删除，节省磁盘空间
		err = remove(preBsFile.path.c_str());
		if (err == -1)
		{
			perror("无法删除块集文件WriteNextLevelBlockset");
		}
	}

	// 更新当前块的空间范围
	curBlockset.box = preBlocksetFiles[0].box;
	for (I32 i = 1; i < preBlocksetFiles.size(); i++)
	{
		BlockSetFileInfo &blckFile = preBlocksetFiles[i];

		curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
		curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);

		curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
		curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
	}
	if (pBlckstDataFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(pBlckstDataFile);

		pBlckstDataFile = NULL;
	}

	return curBlockset.numPoint;
}

//// 根据块集记录的子包分别处理
//U64 CHLZWriter::WriteNextLevelBlocksetByMid2( const char* savePath, 
//	std::vector<BlockSetFileInfo>& preBlocksetFiles, 
//	HdBlockset& curBlockset, U8 curLevel )
//{
//	// 暂时固定0层精度为1/1024m，后续每层精度相对缩减2倍
//	double dStepPrecision = pow(2.0,curLevel) / 256.0;
//
//	// 打开保存文件
//	HANDLE pBlckstDataFile = CreateFile(savePath, 
//		GENERIC_READ | GENERIC_WRITE,
//		FILE_SHARE_READ|FILE_SHARE_WRITE, 
//		NULL,
//		OPEN_ALWAYS, 
//		FILE_ATTRIBUTE_NORMAL, 
//		NULL);
//
//	if (pBlckstDataFile == INVALID_HANDLE_VALUE)
//	{
//		return 0;
//	}
//
//	LARGE_INTEGER li;
//	li.HighPart = 0;
//	li.LowPart = 0;
//
//	LARGE_INTEGER li1;
//	li1.HighPart = 0;
//	li1.LowPart = 0;
//
//	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
//
//	I32 simpleSize = 0;
//	I32 nCount = 0;
//	U32 Index = 0;
//	int err;
//	U64 nTotalCount = 0;
//	for (U32 idx = 0; idx < preBlocksetFiles.size(); idx++)
//	{
//		// 从块集文件中一次读取6.4w点，然后进行随机抽取后写入到当前块集文件
//		BlockSetFileInfo& preBsFile = *(preBlocksetFiles._Myfirst() + idx);
//		//if (preBsFile.numPoint <= 0)
//		//{
//		//	continue;
//		//}
//
//		// 逐级获取
//		if (preBsFile.bHasBlock)
//		{
//			for (U32 blockIdx = 0;blockIdx < preBsFile.vecBlock.size();blockIdx++)
//			{
//				BlockFileInfo& preBlockFile = *(preBsFile.vecBlock._Myfirst() + blockIdx);
//
//				// 可能存在无数据记录
//				if (preBlockFile.path == "")
//				{
//					continue;
//				}
//
//				// 若含有子包记录
//				if (preBlockFile.bHasParcel)
//				{
//					// 获得相关子包
//					for (U32 parcelIdx = 0;parcelIdx < preBlockFile.vecParcel.size();parcelIdx++)
//					{
//						ParcelFileInfo& preParcelFile =  *(preBlockFile.vecParcel._Myfirst() + parcelIdx);
//
//						if (preParcelFile.numPoint <= 0)
//						{
//							continue;
//						}
//
//						// 获得包围盒及文件路径
//						CHdBox3df parcelBox = preParcelFile.box;
//						string strParcelPath = preParcelFile.path.c_str();
//
//						// 均匀抽稀处理
//						U64 count = WriteLevelBlocksetFileByMeanBox(dStepPrecision,strParcelPath.data(),
//							parcelBox,pBlckstDataFile,li,li1);
//						nTotalCount += count;
//					}
//				}
//				else
//				{
//					// 获得包围盒及文件路径
//					CHdBox3df blockBox = preBlockFile.box;
//					string strBlockPath = preBlockFile.path.c_str();
//
//					// 均匀抽稀处理
//					U64 count = WriteLevelBlocksetFileByMeanBox(dStepPrecision,strBlockPath.data(),
//						blockBox,pBlckstDataFile,li,li1);
//					nTotalCount += count;
//				}
//			}
//		}
//		else // 无子块记录，直接以块集读取,可一次读取完成
//		{
//			// 获得包围盒及文件路径
//			CHdBox3df blocksetBox = preBsFile.box;
//			string strBlocksetPath = preBsFile.path.c_str();
//
//			// 均匀抽稀处理
//			U64 count = WriteLevelBlocksetFileByMeanBox(dStepPrecision,strBlocksetPath.data(),
//				blocksetBox,pBlckstDataFile,li,li1);
//			nTotalCount += count;
//		}
//
//
//		//HANDLE pDataFile = CreateFile(preBsFile.path.c_str(), 
//		//	GENERIC_READ,
//		//	FILE_SHARE_READ,
//		//	NULL,
//		//	OPEN_EXISTING,
//		//	FILE_ATTRIBUTE_NORMAL,
//		//	NULL);
//		//if (pDataFile == INVALID_HANDLE_VALUE)
//		//{
//		//	continue;
//		//}
//
//		//std::vector<PointXYZIPRGBA> bufferPts;
//		//while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
//		//{
//		//	// 定义单次读取范围box
//		//	double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
//		//	dMinX = dMinY = dMinZ = F32_MAX;
//		//	dMaxX = dMaxY = dMaxZ = F32_MIN;
//
//		//	// 确定格网范围
//		//	for (unsigned int n = 0;n < bufferPts.size();n++)
//		//	{
//		//		PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
//		//		dMinX = MIN(dMinX,pts.x);
//		//		dMinY = MIN(dMinY,pts.y);
//		//		dMinZ = MIN(dMinZ,pts.z);
//		//		dMaxX = MAX(dMaxX,pts.x);
//		//		dMaxY = MAX(dMaxY,pts.y);
//		//		dMaxZ = MAX(dMaxZ,pts.z);
//		//	}
//
//		//	// 根据范围及精度确定格网个数
//		//	int xRows = ceil((dMaxX - dMinX)/dStepPrecision);
//		//	int yRows = ceil((dMaxY - dMinY)/dStepPrecision);
//		//	int zRows = ceil((dMaxZ - dMinZ)/dStepPrecision);
//
//		//	// 采用map记录，键为对于格网索引值（z * (xRows * yRows) + y * xRows + x）
//		//	map<int,int> mapKeys; // 存储所有multimap的键，便于后续遍历处理，用1标记
//		//	std::multimap<int,int> mapMeshIndex; // 键为格网索引，值为bufferPts索引值，可能存在多个值，采用multimap
//
//		//	// 定义中间变量
//		//	int xStep = 0;
//		//	int yStep = 0;
//		//	int zStep = 0;
//		//	int nTmp = 0;
//		//	simpleSize = 0;
//
//		//	// 格网划分处理
//		//	for (unsigned int n = 0;n < bufferPts.size();n++)
//		//	{
//		//		PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
//
//		//		// 计算对应格网索引值
//		//		xStep = floor((pts.x - dMinX)/dStepPrecision);
//		//		yStep = floor((pts.y - dMinY)/dStepPrecision);
//		//		zStep = floor((pts.z - dMinZ)/dStepPrecision);
//
//		//		// 对应键为
//		//		int mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;
//
//		//		// 插入值
//		//		mapMeshIndex.insert(make_pair(mapPin,n));
//
//		//		// 检查是否已标记存在，若不存在进行标记
//		//		if (mapKeys[mapPin] == 0)
//		//		{
//		//			mapKeys[mapPin] = 1;
//		//			simpleSize++;
//		//		}
//
//		//	}
//
//		//	// 根据键索引确定值
//		//	map<int,int>::const_iterator map_key_it = mapKeys.begin();
//		//	typedef multimap<int,int>::iterator key_value_it_beg;
//		//	typedef multimap<int,int>::iterator key_value_it_end;
//
//		//	// 定义中间变量
//		//	double dCenterX = 0.0;
//		//	double dCenterY = 0.0;
//		//	double dCenterZ = 0.0;
//		//	double dDist = 0.0;
//		//	int nTargIndex = 0;
//		//	int nBufSize = bufferPts.size();
//		//	int nCurTagIndex = 0;
//
//		//	// 抽取的点数赋值
//		//	std::vector<PointXYZIPRGBA> samplePts;
//		//	samplePts.resize(simpleSize);
//
//		//	// 遍历每一个map值
//		//	while (map_key_it != mapKeys.end())
//		//	{
//		//		// 获得键
//		//		if (map_key_it->second == 0)
//		//		{
//		//			++map_key_it;
//		//			continue;
//		//		}
//
//		//		// 值为1，表明存在键
//		//		int nMapPin = map_key_it->first;
//
//		//		// 遍历获取mapMeshIndex对应值集
//		//		key_value_it_beg beg = mapMeshIndex.lower_bound(nMapPin);
//		//		key_value_it_end end = mapMeshIndex.upper_bound(nMapPin);
//
//		//		// 获得box对应x,y,zStep
//		//		zStep = nMapPin / (xRows * yRows);
//		//		nTmp = nMapPin % (xRows * yRows);
//		//		yStep = nTmp / xRows;
//		//		xStep = nTmp % xRows;
//
//		//		// 计算获取距离box中心最近点
//		//		dCenterX = dMinX + (xStep +  0.5) * dStepPrecision;
//		//		dCenterY = dMinY + (yStep +  0.5) * dStepPrecision;
//		//		dCenterZ = dMinZ + (zStep +  0.5) * dStepPrecision;
//
//		//		// 遍历处理前初始化
//		//		nTargIndex = -1;
//		//		dDist = F32_MAX;
//
//		//		// 遍历范围内数据
//		//		while (beg != end)
//		//		{
//		//			int nBufIndex = beg->second;
//
//		//			// 获取坐标
//		//			if (nBufIndex >= 0 && nBufIndex < nBufSize)
//		//			{
//		//				PointXYZIPRGBA& pPt = *(bufferPts._Myfirst() + nBufIndex);
//		//				double dTmp = pow(pPt.x - dCenterX,2.0) + pow(pPt.y - dCenterY,2.0) + pow(pPt.z - dCenterZ,2.0);
//		//				if (dTmp < dDist)
//		//				{
//		//					dDist = dTmp;
//		//					nTargIndex = nBufIndex;
//		//				}
//		//			}
//
//		//			++beg;
//		//		}
//
//		//		if (nTargIndex >= 0)
//		//		{
//		//			// 一个格网计算完成后，更新samplePts采样数据值
//		//			PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + nCurTagIndex);
//		//			pPt = *(bufferPts._Myfirst() + nTargIndex);
//
//		//			++nCurTagIndex;
//		//		}
//
//		//		++map_key_it;
//		//	}
//
//		//	// 将抽取的点写入			
//		//	DWORD reslt;
//		//	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
//		//	WriteFile(pBlckstDataFile,samplePts._Myfirst,sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);
//
//		//	curBlockset.numPoint += simpleSize;
//		//}
//
//		//if (pDataFile!=INVALID_HANDLE_VALUE)
//		//{
//		//	CloseHandle(pDataFile);
//		//	pDataFile = NULL;
//		//}
//
//		// 将块集文件删除，节省磁盘空间
//		err = remove(preBsFile.path.c_str());
//		if (err == -1)
//		{
//			perror("无法删除块集文件WriteNextLevelBlockset");
//		}
//	}
//
//	// 更新当前块的空间范围
//	curBlockset.numPoint += nTotalCount;
//	curBlockset.box = (*preBlocksetFiles._Myfirst).box;
//	for (I32 i = 1; i < preBlocksetFiles.size(); i++)
//	{
//		BlockSetFileInfo &blckFile = (*(preBlocksetFiles._Myfirst() + i));
//
//		curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
//		curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
//		curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);
//
//		curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
//		curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
//		curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
//	}
//	if (pBlckstDataFile!=INVALID_HANDLE_VALUE)
//	{
//		CloseHandle(pBlckstDataFile);
//
//		pBlckstDataFile = NULL;
//	}
//
//	return curBlockset.numPoint;
//}

U64 CHLZWriter::WriteLevelBlocksetFileByMeanBox( 
	double dStepPrecision, /* 为本次层级均匀抽稀格网精度 */ 
	const char* strFilePath, /* 待读取的临时文件，为 ?趾蟮目榧?块/包，可?次读取完成 */ 
	CHdBox3df extentBox, /* 临时文件对应的包围盒，减少二次统计时间 */ 
	HANDLE pBlckstDataFile, /* 待写入的下一层级块集临时文件句柄，内部直接写入*/
	LARGE_INTEGER& li,
	LARGE_INTEGER& li1)
{
	// 打开读取文件
	HANDLE pDataFile = CreateFile(strFilePath, 
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pDataFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	U64 totalPoints = 0;

	// 采用分段读取思路，实际应是一次读取完成，while循环一次
	std::vector<PointXYZIPRGBA> bufferPts;
	while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
	{
		// 定义单次读取范围box
		double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
		dMinX = dMinY = dMinZ = F32_MAX;
		dMaxX = dMaxY = dMaxZ = F32_MIN;

		// 不统计，直接由外部传入包围盒赋值
		dMinX = extentBox.MinEdge.X;
		dMinY = extentBox.MinEdge.Y;
		dMinZ = extentBox.MinEdge.Z;
		dMaxX = extentBox.MaxEdge.X;
		dMaxY = extentBox.MaxEdge.Y;
		dMaxZ = extentBox.MaxEdge.Z;

		//// 确定格网范围
		//for (unsigned int n = 0;n < bufferPts.size();n++)
		//{
		//	PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
		//	dMinX = MIN(dMinX,pts.x);
		//	dMinY = MIN(dMinY,pts.y);
		//	dMinZ = MIN(dMinZ,pts.z);
		//	dMaxX = MAX(dMaxX,pts.x);
		//	dMaxY = MAX(dMaxY,pts.y);
		//	dMaxZ = MAX(dMaxZ,pts.z);
		//}

		// 根据范围及精度确定格网个数
		int xRows = ceil((dMaxX - dMinX)/dStepPrecision);
		int yRows = ceil((dMaxY - dMinY)/dStepPrecision);
		int zRows = ceil((dMaxZ - dMinZ)/(dStepPrecision*2.0));

		// 采用map记录，键为对于格网索引值（z * (xRows * yRows) + y * xRows + x）
		map<int,int> mapKeys; // 存储所有multimap的键，便于后续遍历处理，用1标记
		std::multimap<int,int> mapMeshIndex; // 键为格网索引，值为bufferPts索引值，可能存在多个值，采用multimap

		// 定义中间变量
		int xStep = 0;
		int yStep = 0;
		int zStep = 0;
		int nTmp = 0;
		int simpleSize = 0;

		// 格网划分处理
		for (unsigned int n = 0;n < bufferPts.size();n++)
		{
			PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);

			// 计算对应格网索引值
			xStep = floor((pts.x - dMinX)/dStepPrecision);
			yStep = floor((pts.y - dMinY)/dStepPrecision);
			zStep = floor((pts.z - dMinZ)/(dStepPrecision*2.0));

			// 对应键为
			int mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

			// 插入值
			mapMeshIndex.insert(make_pair(mapPin,n));

			// 检查是否已标记存在，若不存在进行标记
			if (mapKeys[mapPin] == 0)
			{
				mapKeys[mapPin] = 1;
				simpleSize++;
			}

		}

		// 根据键索引确定值
		map<int,int>::const_iterator map_key_it = mapKeys.begin();
		typedef multimap<int,int>::iterator key_value_it_beg;
		typedef multimap<int,int>::iterator key_value_it_end;

		// 定义中间变量
		double dCenterX = 0.0;
		double dCenterY = 0.0;
		double dCenterZ = 0.0;
		double dDist = 0.0;
		int nTargIndex = 0;
		int nBufSize = bufferPts.size();
		int nCurTagIndex = 0;

		// 抽取的点数赋值
		std::vector<PointXYZIPRGBA> samplePts;
		samplePts.resize(simpleSize);

		// 遍历每一个map值
		while (map_key_it != mapKeys.end())
		{
			// 获得键
			if (map_key_it->second == 0)
			{
				++map_key_it;
				continue;
			}

			// 值为1，表明存在键
			int nMapPin = map_key_it->first;

			// 遍历获取mapMeshIndex对应值集
			key_value_it_beg beg = mapMeshIndex.lower_bound(nMapPin);
			key_value_it_end end = mapMeshIndex.upper_bound(nMapPin);

			// 获得box对应x,y,zStep
			zStep = nMapPin / (xRows * yRows);
			nTmp = nMapPin % (xRows * yRows);
			yStep = nTmp / xRows;
			xStep = nTmp % xRows;

			// 计算获取距离box中心最近点
			dCenterX = dMinX + (xStep +  0.5) * dStepPrecision;
			dCenterY = dMinY + (yStep +  0.5) * dStepPrecision;
			dCenterZ = dMinZ + (zStep +  0.5) * dStepPrecision*2.0;

			// 遍历处理前初始化
			nTargIndex = -1;
			dDist = F32_MAX;

			// 遍历范围内数据
			while (beg != end)
			{
				int nBufIndex = beg->second;

				// 获取坐标
				if (nBufIndex >= 0 && nBufIndex < nBufSize)
				{
					PointXYZIPRGBA& pPt = *(bufferPts._Myfirst() + nBufIndex);
					double dTmp = pow(pPt.x - dCenterX,2.0) + pow(pPt.y - dCenterY,2.0) + pow(pPt.z - dCenterZ,2.0);
					if (dTmp < dDist)
					{
						dDist = dTmp;
						nTargIndex = nBufIndex;
					}
				}

				++beg;
			}

			if (nTargIndex >= 0)
			{
				// 一个格网计算完成后，更新samplePts采样数据值
				PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + nCurTagIndex);
				pPt = *(bufferPts._Myfirst() + nTargIndex);

				++nCurTagIndex;
			}

			++map_key_it;
		}

		// 将抽取的点写入			
		DWORD reslt;
		SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
		WriteFile(pBlckstDataFile,samplePts._Myfirst(),sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

		totalPoints += simpleSize;
	}

	// 关闭文件
	if (pDataFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(pDataFile);
		pDataFile = NULL;
	}
	
	// 将处理后的子文件删除，节省磁盘空间
	int err = remove(strFilePath);
	if (err == -1)
	{
		perror("无法删除子集文件WriteNextLevelBlockset");
	}

	return totalPoints;
}


U64 CHLZWriter::WriteNextLevelBlocksetByMidCloud( const char* savePath, 
	std::vector<BlockSetFileInfo>& preBlocksetFiles, 
	HdBlockset& curBlockset, 
	CHdLevel* curLevel, 
	U32 curBlockSetNo, 
	U8 nCurLevel )
{
	// 暂时固定0层精度为1/128m，后续每层精度相对缩减2倍
	double dStepPrecision = pow(3.0,nCurLevel) * m_dGridPrecision;
	double dAntiStep = 1.0 / dStepPrecision;

	//// 定义结构体
	//struct PosKey 
	//{
	//	int gridIndex;
	//	int nIndex;
	//	int nStepX;
	//	int nStepY;
	//	int nStepZ;
	//	double dDist;
	//	PosKey()
	//	{
	//		gridIndex = 0;
	//		nIndex = 0;
	//		nStepZ = nStepX = nStepY = 0;
	//		dDist = 0.0;
	//	}
	//};

	// 打开保存文件
	HANDLE pBlckstDataFile = CreateFile(savePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL,
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if (pBlckstDataFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LARGE_INTEGER li;
	li.HighPart = 0;
	li.LowPart = 0;

	LARGE_INTEGER li1;
	li1.HighPart = 0;
	li1.LowPart = 0;

	SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);

	I32 simpleSize = 0;
	I32 nCount = 0;
	U32 Index = 0;
	int err;
	// 计算当前块集编号
	U32 nBlockSetY = curBlockSetNo / curLevel->m_level.numBlocksetX;
	U32 nBlockSetX = curBlockSetNo - nBlockSetY * curLevel->m_level.numBlocksetX;
	// 得到当前块集坐标原点作为偏移量
	F32 BlockSetStartX = nBlockSetX * curLevel->m_level.sizeX;
	F32 BlockSetStartY = nBlockSetY * curLevel->m_level.sizeY;

	//// 更新当前块的空间范围
	//curBlockset.box = (*preBlocksetFiles._Myfirst).box;
	//for (I32 i = 0; i < preBlocksetFiles.size(); i++)
	//{
	//	BlockSetFileInfo &blckFile = (*(preBlocksetFiles._Myfirst() + i));

	//	//curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
	//	//curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
	//	curBlockset.box.MinEdge.X = min(BlockSetStartX, blckFile.box.MinEdge.X);
	//	curBlockset.box.MinEdge.Y = min(BlockSetStartY, blckFile.box.MinEdge.Y);
	//	curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);

	//	//curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
	//	//curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
	//	curBlockset.box.MaxEdge.X = max(BlockSetStartX + curLevel->m_level.sizeX, blckFile.box.MaxEdge.X);
	//	curBlockset.box.MaxEdge.Y = max(BlockSetStartY + curLevel->m_level.sizeY, blckFile.box.MaxEdge.Y);
	//	curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
	//}

	F32 offsetX = 0.0;
	F32 offsetY = 0.0;
	// 根据当前层信息进行点云坐标偏移，保证当前块集内的点云相对坐标原点为块集角点
	for (U32 idx = 0; idx < preBlocksetFiles.size(); idx++)
	{
		// 从块集文件中一次读取6.4w点，然后进行随机抽取后写入到当前块集文件
		BlockSetFileInfo& preBsFile = *(preBlocksetFiles._Myfirst() + idx);
		offsetX = preBsFile.box.MinEdge.X - BlockSetStartX;
		offsetY = preBsFile.box.MinEdge.Y - BlockSetStartY;

		HANDLE pDataFile = CreateFile(preBsFile.path.c_str(), 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (pDataFile == INVALID_HANDLE_VALUE)
		{
			continue;
		}

		std::vector<PointXYZIPRGBA> bufferPts;
		bufferPts.resize(128000);
		U64 nReadCount = 0;

		// 使用vector记录
		std::vector<PosKey> vecPosKeys;
		vecPosKeys.resize(bufferPts.size());

		// 记录索引值
		std::vector<int> vecTargetIndex;
		vecTargetIndex.resize(vecPosKeys.size());

		// 采样结果值
		std::vector<PointXYZIPRGBA> samplePts;
		samplePts.resize(vecPosKeys.size());

		//while (ReadPtsBuffer(pDataFile, bufferPts) > 0)
		while (ReadPtsBuffer(pDataFile, bufferPts,nReadCount) > 0)
		{
			// 定义单次读取范围box
			double dMinX,dMinY,dMinZ,dMaxX,dMaxY,dMaxZ;
			dMinX = dMinY = dMinZ = F32_MAX;
			dMaxX = dMaxY = dMaxZ = F32_MIN;

			////curBlockset.box;
			//dMinX = curBlockset.box.MinEdge.X;
			//dMinY = curBlockset.box.MinEdge.Y;
			//dMinZ = curBlockset.box.MinEdge.Z;
			//dMaxX = curBlockset.box.MaxEdge.X;
			//dMaxY = curBlockset.box.MaxEdge.Y;
			//dMaxZ = curBlockset.box.MaxEdge.Z;

			// 定义中间变量
			double dCenterX = 0.0;
			double dCenterY = 0.0;
			double dCenterZ = 0.0;

			// 确定格网范围
			for (unsigned int n = 0;n < nReadCount;n++)
			{
				PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);
				pts.x += offsetX;
				pts.y += offsetY;

				dMinX = MIN(dMinX,pts.x);
				dMinY = MIN(dMinY,pts.y);
				dMinZ = MIN(dMinZ,pts.z);
				dMaxX = MAX(dMaxX,pts.x);
				dMaxY = MAX(dMaxY,pts.y);
				dMaxZ = MAX(dMaxZ,pts.z);
			}

			// 根据范围及精度确定格网个数
			int xRows = ceil((dMaxX - dMinX) * dAntiStep);
			int yRows = ceil((dMaxY - dMinY) * dAntiStep);
			int zRows = ceil((dMaxZ - dMinZ) * dAntiStep);

			// 定义中间变量
			int xStep = 0;
			int yStep = 0;
			int zStep = 0;
			int nTmp = 0;
			simpleSize = 0;

			//// 时间统计测试
			//long t1,t2,t3,t4;
			//t1 = clock();

			// 格网划分处理
			for (unsigned int n = 0;n < nReadCount;n++)
			{
				PointXYZIPRGBA& pts = *(bufferPts._Myfirst() + n);

				//// 计算对应格网索引值
				//xStep = floor((pts.x - dMinX) * dAntiStep);
				//yStep = floor((pts.y - dMinY) * dAntiStep);
				//zStep = floor((pts.z - dMinZ) * dAntiStep);

				// 计算对应格网索引值
				xStep = ceil((pts.x - dMinX) * dAntiStep);
				yStep = ceil((pts.y - dMinY) * dAntiStep);
				zStep = ceil((pts.z - dMinZ) * dAntiStep);

				double dTmpX,dTmpY,dTmpZ;
				dTmpX = dMinX + (xStep +  0.5) * dStepPrecision - pts.x;
				dTmpY = dMinY + (yStep +  0.5) * dStepPrecision - pts.y;
				dTmpZ = dMinZ + (zStep +  0.5) * dStepPrecision - pts.z;

				// 对应键为
				U64 mapPin = zStep * (xRows * yRows) + yStep * xRows + xStep;

				// 插入值
				PosKey posKey;
				posKey.nIndex = n;
				//posKey.nStepX = xStep;
				//posKey.nStepY = yStep;
				//posKey.nStepZ = zStep;

				// 计算获取距离box中心最近点
				posKey.dDist = pow(dTmpX,2.0) + pow(dTmpY,2.0) + pow(dTmpZ,2.0);
				posKey.gridIndex = mapPin;
				*(vecPosKeys._Myfirst() + n) = posKey;

				//mapMeshIndex.insert(make_pair(mapPin,posKey));

				//// 检查是否已标记存在，若不存在进行标记
				//if (mapKeys[mapPin] == 0)
				//{
				//	mapKeys[mapPin] = 1;
				//	simpleSize++;
				//}
			}

			//t2 = clock();
			//printf("%s%d\n","box计算耗时 : ",t2-t1);

			// 针对vec进行处理排序
			std::sort(vecPosKeys.begin(),vecPosKeys.begin() + nReadCount,SortByGrid);

			// 定义中间变量
			PosKey& pos = *(vecPosKeys._Myfirst() + 0);
			U64 nCurGridPos = pos.gridIndex;
			*(vecTargetIndex._Myfirst() + 0) = pos.nIndex;
			int nCurIndex = 1;

			// 根据之前记录的距离值，grid相同的比较距离，从小到大排序
			for (unsigned int nn = 1;nn < nReadCount;nn++)
			{
				PosKey& pos = *(vecPosKeys._Myfirst() + nn);
				if (pos.gridIndex == nCurGridPos)
				{
					// 之前已按距离排序，同一格网距离中心最小为最前索引值
					continue;
				}
				else
				{
					// 处理到下一个格网时，记录
					nCurGridPos = pos.gridIndex;
					*(vecTargetIndex._Myfirst() + nCurIndex) = pos.nIndex;
					nCurIndex++;
				}
			}

			// 抽取的点数赋值
			simpleSize = nCurIndex;
			//std::vector<PointXYZIPRGBA> samplePts;
			//samplePts.resize(simpleSize);

			// 点云更新处理
			for (I32 i=0; i < simpleSize; i++)
			{
				int nTmpIndex = *(vecTargetIndex._Myfirst() + i);
				PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
				pPt = *(bufferPts._Myfirst() + nTmpIndex);
			}

			//// 根据键索引确定值
			//map<int,int>::const_iterator map_key_it = mapKeys.begin();

			//// 定义中间变量
			//double dDist = 0.0;
			//int nTargIndex = 0;
			//int nBufSize = bufferPts.size();
			//int nCurTagIndex = 0;

			////t3 = clock();

			//// 遍历每一个map值
			//while (map_key_it != mapKeys.end())
			//{
			//	// 获得键
			//	if (map_key_it->second == 0)
			//	{
			//		++map_key_it;
			//		continue;
			//	}

			//	// 值为1，表明存在键
			//	int nMapPin = map_key_it->first;
			//	//typedef multimap<int,int>::iterator key_value_it_beg;
			//	//typedef multimap<int,int>::iterator key_value_it_end;
			//	typedef unordered_multimap<int,PosKey>::iterator key_value_it_beg;
			//	typedef unordered_multimap<int,PosKey>::iterator key_value_it_end;

			//	// 遍历获取mapMeshIndex对应值集
			//	//key_value_it_beg beg = mapMeshIndex.lower_bound(nMapPin);
			//	//key_value_it_end end = mapMeshIndex.upper_bound(nMapPin);


			//	//// 获得box对应x,y,zStep
			//	//zStep = nMapPin / (xRows * yRows);
			//	//nTmp = nMapPin % (xRows * yRows);
			//	//yStep = nTmp / xRows;
			//	//xStep = nTmp % xRows;

			//	// 计算获取距离box中心最近点
			//	dCenterX = dMinX + (xStep +  0.5) * dStepPrecision;
			//	dCenterY = dMinY + (yStep +  0.5) * dStepPrecision;
			//	dCenterZ = dMinZ + (zStep +  0.5) * dStepPrecision;

			//	// 遍历处理前初始化
			//	nTargIndex = -1;
			//	dDist = F32_MAX;

			//	pair<key_value_it_beg,key_value_it_end> pos = mapMeshIndex.equal_range(nMapPin);

			//	// 遍历范围内数据
			//	//while (beg != end)
			//	while (pos.first != pos.second)
			//	{
			//		//int nBufIndex = beg->second;
			//		PosKey psKey = pos.first->second;
			//		int nBufIndex = psKey.nIndex;

			//		// 获取坐标
			//		if (nBufIndex >= 0 && nBufIndex < nBufSize)
			//		{
			//			PointXYZIPRGBA& pPt = *(bufferPts._Myfirst() + nBufIndex);
			//			double dTmp = pow(pPt.x - dCenterX,2.0) + pow(pPt.y - dCenterY,2.0) + pow(pPt.z - dCenterZ,2.0);
			//			if (dTmp < dDist)
			//			{
			//				dDist = dTmp;
			//				nTargIndex = nBufIndex;
			//			}
			//		}

			//		//++beg;
			//		++pos.first;
			//	}

			//	if (nTargIndex >= 0)
			//	{
			//		// 一个格网计算完成后，更新samplePts采样数据值
			//		PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + nCurTagIndex);
			//		pPt = *(bufferPts._Myfirst() + nTargIndex);

			//		++nCurTagIndex;
			//	}

			//	++map_key_it;
			//}

			//t4 = clock();
			//printf("%s%d\n","键值计算耗时 : ",t4-t3);

			// 将抽取的点写入			
			DWORD reslt;
			SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
			WriteFile(pBlckstDataFile,samplePts._Myfirst(),sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

			curBlockset.numPoint += simpleSize;

			//// 尝试主动释放内存
			//mapMeshIndex.clear();
			//samplePts.clear();
			//mapKeys.clear();

			///////////////
			//nCount = bufferPts.size();
			//simpleSize = nCount / SIMPLE_SCALE;
			//
			//// 抽取的点数为内存中1/4
			//std::vector<PointXYZIPRGBA> samplePts;
			//samplePts.resize(simpleSize);

			//for (I32 i=0; i < simpleSize; i++)
			//{
			//	// 随机抽取一个数
			//	randomGenerator.drawUniformUnsignedIntRange(Index, 0, nCount-1);

			//	PointXYZIPRGBA& pPt = *(samplePts._Myfirst() + i);
			//	pPt = *(bufferPts._Myfirst() + Index);
			//	pPt.x += offsetX;
			//	pPt.y += offsetY;
			//}

			//// 将抽取的点写入			
			//DWORD reslt;
			//SetFilePointerEx(pBlckstDataFile,li,&li1,FILE_END);
			//WriteFile(pBlckstDataFile,samplePts._Myfirst,sizeof(PointXYZIPRGBA)*simpleSize,&reslt,NULL);

			//curBlockset.numPoint += simpleSize;
		}

		if (pDataFile!=INVALID_HANDLE_VALUE)
		{
			CloseHandle(pDataFile);
			pDataFile = NULL;
		}
		// 将块集文件删除，节省磁盘空间
		err = remove(preBsFile.path.c_str());
		if (err == -1)
		{
			perror("无法删除块集文件WriteNextLevelBlockset");
		}
	}

	// 更新当前块的空间范围
	curBlockset.box = preBlocksetFiles[0].box;
	for (I32 i = 0; i < preBlocksetFiles.size(); i++)
	{
		BlockSetFileInfo &blckFile = preBlocksetFiles[i];

		//curBlockset.box.MinEdge.X = min(curBlockset.box.MinEdge.X, blckFile.box.MinEdge.X);
		//curBlockset.box.MinEdge.Y = min(curBlockset.box.MinEdge.Y, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.X = min(BlockSetStartX, blckFile.box.MinEdge.X);
		curBlockset.box.MinEdge.Y = min(BlockSetStartY, blckFile.box.MinEdge.Y);
		curBlockset.box.MinEdge.Z = min(curBlockset.box.MinEdge.Z, blckFile.box.MinEdge.Z);

		//curBlockset.box.MaxEdge.X = max(curBlockset.box.MaxEdge.X, blckFile.box.MaxEdge.X);
		//curBlockset.box.MaxEdge.Y = max(curBlockset.box.MaxEdge.Y, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.X = max(BlockSetStartX + curLevel->m_level.sizeX, blckFile.box.MaxEdge.X);
		curBlockset.box.MaxEdge.Y = max(BlockSetStartY + curLevel->m_level.sizeY, blckFile.box.MaxEdge.Y);
		curBlockset.box.MaxEdge.Z = max(curBlockset.box.MaxEdge.Z, blckFile.box.MaxEdge.Z);
	}

	if (pBlckstDataFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(pBlckstDataFile);

		pBlckstDataFile = NULL;
	}

	return curBlockset.numPoint;
}

// 设置格网精度
void CHLZWriter::SetGridPrecision( double dGridPrecision )
{
	m_dGridPrecision = dGridPrecision;
}

// 根据grid排序
bool SortByGrid( PosKey& pt0,PosKey& pt1 )
{
	bool bRet = pt0.gridIndex < pt1.gridIndex;
	if ( pt0.gridIndex == pt1.gridIndex)
	{
		return pt0.dDist < pt1.dDist;
	}

	return bRet;
}

}