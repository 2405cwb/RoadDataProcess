#include "StdAfx.h"
#include "mlsBuilder.h"
#include "..\laslib\HLSReader.h"
#include "mlsHeader.h"

#define LIMIT_DATASIZE	(2 << 30)
#define IO_BUFFERSIZE	(2 << 20)

using namespace hd;

mlsBuilder::mlsBuilder(void)
{
	m_boundBox.MinEdge.set(F64_MAX,F64_MAX,F64_MAX);
	m_boundBox.MaxEdge.set(F64_MIN,F64_MIN,F64_MIN);
	m_totalCount = 0;
	m_maxPtInCube = 0;
	m_zStep = m_xStep = m_yStep = 0.0;
	m_pMdxFile = NULL;
	m_pDataFile = NULL;
	m_curFileSize = 0;
}


mlsBuilder::~mlsBuilder(void)
{
	//关闭分块文件
	/*for (map<cubeNo,cubeInfo,cubeLess>::iterator it = m_pcdStat.begin();
		it != m_pcdStat.end();it++)
	{
		if (it->second.fwFile != NULL)
		{
			fclose(it->second.fwFile);
			it->second.fwFile = NULL;
		}
	}*/

	if (m_pDataFile)
	{
		fclose(m_pDataFile);
		m_pDataFile = NULL;
	}

	if (m_pMdxFile)
	{
		fclose(m_pMdxFile);
		m_pMdxFile = NULL;
	}
}
//! maxPtInCube大于10万,小于200万
void mlsBuilder::setMaxCount( int maxPtInCube )
{
	if (maxPtInCube > 100000 && maxPtInCube < 2000000)
	{
		m_maxPtInCube = maxPtInCube;
	}
}

bool mlsBuilder::calucBoundBox()
{
	if (m_hlsFileList.size() == 0)
	{
		return false;
	}
	m_boundBox.MinEdge.set(F64_MAX,F64_MAX,F64_MAX);
	m_boundBox.MaxEdge.set(F64_MIN,F64_MIN,F64_MIN);
	m_totalCount = 0;

	for (std::vector<string>::const_iterator it = m_hlsFileList.begin();
		it != m_hlsFileList.end();it++)
	{
		hd::HLSreader reader;
		BOOL bRet = reader.open((*it).c_str());
		if(!bRet)continue;

		m_totalCount += reader.m_header.number_of_point_records;
		m_boundBox.MinEdge.X = MIN(m_boundBox.MinEdge.X,reader.m_header.min_x);
		m_boundBox.MinEdge.Y = MIN(m_boundBox.MinEdge.Y,reader.m_header.min_y);
		m_boundBox.MinEdge.Z = MIN(m_boundBox.MinEdge.Z,reader.m_header.min_z);

		m_boundBox.MaxEdge.X = MAX(m_boundBox.MaxEdge.X,reader.m_header.max_x);
		m_boundBox.MaxEdge.Y = MAX(m_boundBox.MaxEdge.Y,reader.m_header.max_y);
		m_boundBox.MaxEdge.Z = MAX(m_boundBox.MaxEdge.Z,reader.m_header.max_z);
	}
	return true;
}

bool mlsBuilder::normalSplit(const char* savePath)
{
	if (m_hlsFileList.size() == 0)
	{
		return false;
	}
	if (!(m_maxPtInCube > 100000 && m_maxPtInCube < 2000000))
	{
		return false;
	}

	double cubeCount = m_totalCount * 4 / m_maxPtInCube;
	vector3dd extent = m_boundBox.getExtent();
	
	int xCut = 0;
	int yCut = 0;
	int zCut = 0;
	int statCount = 1;
	m_xStep = extent.X;
	m_yStep = extent.Y;
	m_zStep = extent.Z;

	//string path = savePath;
	char filePath[MAX_PATH] = {0};

	while(statCount < cubeCount)
	{
		if (m_xStep > m_yStep && m_xStep > m_zStep)
		{
			m_xStep /= 2.0;
		}
		else if (m_yStep > m_xStep && m_yStep > m_zStep)
		{
			m_yStep /= 2.0;
		}
		else if (m_zStep > (m_xStep) && m_zStep > (m_yStep))
		{
			m_zStep /= 2.0;
		}

		statCount = (statCount << 1);
	}
	//求step倒数,便于后续使用乘法运行,提高效率
	double xStepRcp = 1.0/m_xStep;
	double yStepRcp = 1.0/m_yStep;
	double zStepRcp = 1.0/m_zStep;

	//int maxInCube;		//格网内最多的点数

	cubeNo id;
	for (std::vector<string>::const_iterator it = m_hlsFileList.begin();
		it != m_hlsFileList.end();it++)
	{
		hd::HLSreader reader;
		BOOL bRet = reader.open((*it).c_str());
		if(!bRet)continue;

		for (U32 n = 0;n < reader.m_npoints;n++)
		{
			// 从文件读取坐标,n是文件中的点序号
			bRet = reader.read_fast(n);
			if(!bRet)
				break;
			if (reader.m_point.intensity == 0)
			{
				continue;
			}

			//计算所在格网
			id.xNo = (reader.m_point.x - m_boundBox.MinEdge.X) * xStepRcp;
			id.yNo = (reader.m_point.y - m_boundBox.MinEdge.Y) * yStepRcp;
			id.zNo = (reader.m_point.z - m_boundBox.MinEdge.Z) * zStepRcp;
			
			cubeInfo& cbInfo = m_pcdStat[id];
			cbInfo.ptcount++;
			if (cbInfo.fwFile == NULL)
			{
				sprintf(filePath,"%s\\%d_%d_%d",savePath,id.xNo,id.yNo,id.zNo);
				cbInfo.fwFile = fopen(filePath,"w+b");
				setvbuf(cbInfo.fwFile, NULL, _IOFBF, IO_BUFFERSIZE);
			}
			fwrite(&reader.m_point.x,sizeof(float),1,cbInfo.fwFile);
			fwrite(&reader.m_point.y,sizeof(float),1,cbInfo.fwFile);
			fwrite(&reader.m_point.z,sizeof(float),1,cbInfo.fwFile);
			fwrite(&reader.m_point.intensity,sizeof(U16),1,cbInfo.fwFile);
		}
	}

	//保存分块文件
	for (map<cubeNo,cubeInfo,cubeLess>::iterator it = m_pcdStat.begin();
		it != m_pcdStat.end();it++)
	{
		if (it->second.fwFile != NULL)
		{
			fflush(it->second.fwFile);
		}
	}
	return true;
}

/*
构建索引思路:
1.先使用标准大小cube去切分数据
2.再遍历标准cube,如果cube中的点数小于m_maxPtInCube,
  则找相邻的cube,如果和相邻的cube点数总和仍然小于m_maxPtInCube,则合并这个相邻的cube。

  合并的顺序是先找Z方向,再找XY方向;
  最终合并后的cube必须是个规则的cube;
3.如果cube中点数大于m_maxPtInCube,则切分当前cube。
*/
bool mlsBuilder::buildMls( const char* savePath )
{
	mlsBlockIndex block;
	m_savePath = savePath;
	m_mdxPath = m_savePath + "\\pcd.mdx";
	if (calucBoundBox() == false)
	{
		return false;
	}
	if (normalSplit(savePath) == false)
	{
		return false;
	}
	
	if (m_pMdxFile != NULL)
	{
		fclose(m_pMdxFile);
		m_pMdxFile = NULL;
	}
	m_pMdxFile = fopen(m_mdxPath.c_str(),"w+b");
	setvbuf(m_pMdxFile, NULL, _IOFBF, IO_BUFFERSIZE);

	m_header.creationDay = 1;
	m_header.creationYear = 2012;

	m_header.minX = m_boundBox.MinEdge.X;
	m_header.minY = m_boundBox.MinEdge.Y;
	m_header.minZ = m_boundBox.MinEdge.Z;

	m_header.maxX = m_boundBox.MaxEdge.X;
	m_header.maxY = m_boundBox.MaxEdge.Y;
	m_header.maxZ = m_boundBox.MaxEdge.Z;
	
	fwrite(&m_header,sizeof(mlsHeader),1,m_pMdxFile);
	fseek(m_pMdxFile,256,SEEK_SET);

	//打开数据文件,准备写入
	m_curFileNo = 0;
	m_curFileName = "HDSY0.mls";
	string dataFile = m_savePath + "\\" + m_curFileName;
	m_pDataFile = fopen(dataFile.c_str(),"w+b");
	setvbuf(m_pDataFile, NULL, _IOFBF, IO_BUFFERSIZE);

	m_curCount = 0;
	char filename[64] = {0};
	map<cubeNo,cubeInfo,cubeLess>::iterator it = m_pcdStat.begin();
	for (it = m_pcdStat.begin();it != m_pcdStat.end();it++)
	{
		if (!it->second.isBuild)
		{						
			// 当前块超出限定个数,则切割
			if (it->second.ptcount > m_maxPtInCube)
			{
				sprintf(filename,"%d_%d_%d",it->first.xNo,it->first.yNo,it->first.zNo);
				splitCube(it,filename,m_xStep,m_yStep,m_zStep);
				it->second.isBuild = true;
			}
			else
			{
				if (it->second.ptcount + m_curCount < m_maxPtInCube)
				{
					m_curList.push_back(it->first);
					m_curCount += it->second.ptcount;

					it->second.isBuild = true;
				}
				else	//小块合并饱满,合并文件
				{
					// 合并聚合文件
					Aggregate();

					m_curList.clear();
					m_curList.push_back(it->first);
					m_curCount = it->second.ptcount;

					it->second.isBuild = true;
				}
			}
			
		}
	}

	// 合并聚合文件
	if (m_curList.size() > 0)
	{
		Aggregate();
		m_curList.clear();
	}

	fseek(m_pMdxFile,0,SEEK_SET);
	fwrite(&m_header,sizeof(mlsHeader),1,m_pMdxFile);

	return true;
}

bool mlsBuilder::aggregateCube( const cubeNo& cNo )
{
	cubeNo nextNo = cNo;
	U32 minSubCount = MIN(m_subCountX,MIN(m_subCountY,m_subCountZ));
	if (m_subCountX == minSubCount &&
		m_subCountY == minSubCount &&
		m_subCountZ == minSubCount)
	{
		nextNo.zNo++;
		m_subCountZ++;
	}
	else if (m_subCountX == minSubCount &&
			 m_subCountZ > minSubCount &&
			 m_subCountY >= minSubCount)
	{
		nextNo.xNo++;
		m_subCountX++;
	}

	map<cubeNo,cubeInfo,cubeLess>::const_iterator findIt = m_pcdStat.find(nextNo);
	if (findIt != m_pcdStat.end() && 
		findIt->second.ptcount + m_curCount < m_maxPtInCube)
	{
		m_curList.push_back(findIt->first);
		m_curCount += findIt->second.ptcount;
		
		return aggregateCube(findIt->first);
	}
	return true;
}

bool mlsBuilder::splitCube(map<cubeNo,cubeInfo,cubeLess>::iterator& it,const char* path,double cubeStepX,double cubeStepY,double cubeStepZ )
{
	U32 subNumX = 1;
	U32 subNumY = 1;
	U32 subNumZ = 1;

	double cubeCount = it->second.ptcount * 4 / m_maxPtInCube;
	vector3dd extent = m_boundBox.getExtent();

	int xCut = 0;
	int yCut = 0;
	int zCut = 0;
	int statCount = 1;
	double xStep = cubeStepX;
	double yStep = cubeStepY;
	double zStep = cubeStepZ;

	char filePath[MAX_PATH] = {0};

	while(statCount < cubeCount)
	{
		if (xStep > yStep && xStep > zStep)
		{
			xStep /= 2.0;
		}
		else if (yStep > xStep && yStep > zStep)
		{
			yStep /= 2.0;
		}
		else if (zStep > (xStep) && zStep > (yStep))
		{
			zStep /= 2.0;
		}

		statCount = (statCount << 1);
	}

	double xStepRcp = 1.0/xStep;
	double yStepRcp = 1.0/yStep;
	double zStepRcp = 1.0/zStep;

	std::map<cubeNo,cubeInfo,cubeLess> subPcdStat;
	FILE* pDataFile = it->second.fwFile;
	fseek(pDataFile,0,SEEK_SET);

	struct PointStru
	{
		float x;
		float y;
		float z;
		U16	  intensity;
	};
	PointStru pt;
	cubeNo id;
	FILE* pFile = NULL;
	while(!feof(pDataFile))
	{
		fread(&pt,14,1,pDataFile);
		// m_boundBox应该使用当前块的坐下角
		id.xNo = (pt.x - m_boundBox.MinEdge.X) * xStepRcp;
		id.yNo = (pt.y - m_boundBox.MinEdge.Y) * yStepRcp;
		id.zNo = (pt.z - m_boundBox.MinEdge.Z) * zStepRcp;

		cubeInfo& cbInfo = subPcdStat[id];
		cbInfo.ptcount++;
		pFile = cbInfo.fwFile;
		if (pFile == NULL)
		{
			sprintf(filePath,"%s\\%s-%d_%d_%d",m_savePath.c_str(),path,id.xNo,id.yNo,id.zNo);
			pFile = fopen(filePath,"w+b"); 
			setvbuf(pFile, NULL, _IOFBF, IO_BUFFERSIZE);
			cbInfo.fwFile = pFile;
		}
		if (pFile)
		{
			fwrite(&pt,14,1,pFile);
		}
		else
		{
			throw new std::exception("文件打开失败");
		}
	}
	// 强制保存
	for (map<cubeNo,cubeInfo,cubeLess>::iterator subIt = subPcdStat.begin();
		subIt != subPcdStat.end();subIt++)
	{
		fflush(subIt->second.fwFile);
	}

	char filename[64] = {0};
	
	for (map<cubeNo,cubeInfo,cubeLess>::iterator subIt = subPcdStat.begin();
		subIt != subPcdStat.end();subIt++)
	{
		sprintf(filename,"%s-%d_%d_%d",path,subIt->first.xNo,subIt->first.yNo,subIt->first.zNo);
		FILE* pSubFile = subIt->second.fwFile;
		if (subIt->second.ptcount > m_maxPtInCube)
		{
			// 继续切分
			splitCube(subIt,filename,xStep,yStep,zStep);			
		}
		else
		{
			// 获取块索引信息
			mlsBlockIndex blockIdx;
			strcpy(blockIdx.dataFile,m_curFileName.c_str());
			blockIdx.dataOffset = ftell(m_pDataFile);
			blockIdx.hasIntensity = true;
			blockIdx.numOfPoint = subIt->second.ptcount;
			blockIdx.minX = m_boundBox.MinEdge.X + (xStep * subIt->first.xNo);
			blockIdx.minY = m_boundBox.MinEdge.Y + (yStep * subIt->first.yNo);
			blockIdx.minZ = m_boundBox.MinEdge.Z + (zStep * subIt->first.zNo);

			blockIdx.maxX = m_boundBox.MinEdge.X + xStep * (subIt->first.xNo + 1);
			blockIdx.maxY = m_boundBox.MinEdge.Y + yStep * (subIt->first.yNo + 1);
			blockIdx.maxZ = m_boundBox.MinEdge.Z + zStep * (subIt->first.zNo + 1);

			// 读取块
			size_t size = ftell(pSubFile);
			fseek(pSubFile,0,SEEK_SET);
			char* cBuf = (char*)malloc(size);
			fread(cBuf,1,size,pSubFile);
			// 写入块
			if (size + blockIdx.dataOffset > LIMIT_DATASIZE)
			{
				m_curFileNo++;
				fclose(m_pDataFile);
				char tmp[MAX_PATH] = {0};
				sprintf(tmp,"%s%d.mls","HDSY",m_curFileNo);
				m_curFileName = tmp;
				sprintf(tmp,"%s\\%s",m_savePath.c_str(),m_curFileName.c_str());
				m_pDataFile = fopen(tmp,"w+b");
				setvbuf(m_pDataFile, NULL, _IOFBF, IO_BUFFERSIZE);

				blockIdx.dataOffset = 0;
				strcpy(blockIdx.dataFile,m_curFileName.c_str());
			}
			fwrite(cBuf,1,size,m_pDataFile);
			free(cBuf);

			//关闭文件
			fclose(pSubFile);

			sprintf(filePath,"%s\\%s",m_savePath.c_str(),filename);
			remove(filePath);

			// 写入块索引
			fwrite(&blockIdx,sizeof(mlsBlockIndex),1,m_pMdxFile);
			m_header.blockNum++;
		}

	}

	//删除文件
	sprintf(filePath,"%s\\%s",m_savePath.c_str(),path);
	fclose(it->second.fwFile);
	remove(filePath);

	return false;
}

void mlsBuilder::writeAggregate(const std::vector<cubeNo>& aggList)
{
	// 写入上次聚合的块索引和块数据
	if (aggList.size() == 0)
	{
		return;
	}
	mlsBlockIndex blockIdx;
	strcpy(blockIdx.dataFile,m_curFileName.c_str());
	blockIdx.dataOffset = ftell(m_pDataFile);
	blockIdx.hasIntensity = true;

	blockIdx.minX = m_boundBox.MinEdge.X + aggList[0].xNo * m_xStep;
	blockIdx.minY = m_boundBox.MinEdge.Y + aggList[0].yNo * m_yStep;
	blockIdx.minZ = m_boundBox.MinEdge.Z + aggList[0].zNo * m_zStep;

	blockIdx.maxX = m_boundBox.MinEdge.X + aggList[0].xNo * m_xStep;
	blockIdx.maxY = m_boundBox.MinEdge.Y + aggList[0].yNo * m_yStep;
	blockIdx.maxZ = m_boundBox.MinEdge.Z + aggList[0].zNo * m_zStep;

	for (std::vector<cubeNo>::const_iterator itagg = aggList.begin();
		itagg != aggList.end();itagg++)
	{
		blockIdx.numOfPoint += m_pcdStat[*itagg].ptcount;

		blockIdx.minX = MIN(blockIdx.minX,itagg->xNo * m_xStep);
		blockIdx.minY = MIN(blockIdx.minY,itagg->yNo * m_yStep);
		blockIdx.minZ = MIN(blockIdx.minZ,itagg->zNo * m_zStep);

		blockIdx.maxX = MAX(blockIdx.maxX,(itagg->xNo + 1) * m_xStep);
		blockIdx.maxY = MAX(blockIdx.maxY,(itagg->yNo + 1) * m_yStep);
		blockIdx.maxZ = MAX(blockIdx.maxZ,(itagg->zNo + 1) * m_zStep);
	}
		
	if (blockIdx.dataOffset + blockIdx.numOfPoint * blockIdx.pointRecLength > LIMIT_DATASIZE)
	{
		m_curFileNo++;
		fclose(m_pDataFile);
		char tmp[MAX_PATH] = {0};
		sprintf(tmp,"%s%d.mls","HDSY",m_curFileNo);
		m_curFileName = tmp;
		sprintf(tmp,"%s\\%s","HDSY",m_savePath.c_str(),m_curFileName.c_str());
		m_pDataFile = fopen(tmp,"w+b");
		setvbuf(m_pDataFile, NULL, _IOFBF, IO_BUFFERSIZE);

		blockIdx.dataOffset = 0;
		strcpy(blockIdx.dataFile,m_curFileName.c_str());
	}

	// 写入块索引
	fwrite(&blockIdx,sizeof(mlsBlockIndex),1,m_pMdxFile);
	m_header.blockNum++;
	// 写入数据文件
	for (std::vector<cubeNo>::const_iterator itagg = aggList.begin();
		itagg != aggList.end();itagg++)
	{
		FILE* pCubeFile = m_pcdStat[*itagg].fwFile;
		long size = ftell(pCubeFile);
		fseek(pCubeFile,0,SEEK_SET);
		char* cBuff = (char*)malloc(size);
		fread(cBuff,1,size,pCubeFile);

		fwrite(cBuff,1,size,m_pDataFile);

		free(cBuff);
		fclose(pCubeFile);

		//删除文件
		char filePath[MAX_PATH] = {0};
		sprintf(filePath,"%s\\%d_%d_%d",m_savePath.c_str(),itagg->xNo,itagg->yNo,itagg->zNo);
		remove(filePath);
	}

}

void mlsBuilder::Aggregate()
{
	if(m_curList.size() == 0)
		return;

	std::vector<cubeNo> aggList;
	cubeNo lastNo;
	for (std::vector<cubeNo>::iterator it = m_curList.begin();
		it != m_curList.end();it++)
	{
		if (aggList.size() == 0)
		{
			lastNo = *it;
			aggList.push_back(lastNo);
			continue;
		}

		if (aggList.size() > 0 && 
			(((*it).xNo == lastNo.xNo && (*it).yNo == lastNo.yNo && (*it).zNo - lastNo.zNo == 1) ||
			((*it).xNo == lastNo.xNo && (*it).yNo - lastNo.yNo == 1 && (*it).zNo == lastNo.zNo) ||
			((*it).xNo - lastNo.xNo == 1 && (*it).yNo == lastNo.yNo && (*it).zNo == lastNo.zNo)))//相邻的两个cube合并
		{
			lastNo = *it;
			aggList.push_back(lastNo);
		}
		else
		{
			writeAggregate(aggList);
			aggList.clear();

			lastNo = *it;
			aggList.push_back(lastNo);
		}
	}

	if (aggList.size() >0)
	{
		writeAggregate(aggList);
		aggList.clear();
	}
}
