#pragma once

#include "..\hdCommon\mydefs.hpp"
#include <stdlib.h>
#include <string.h>

typedef struct mlsHeader
{
	mlsHeader()
	{
		size_t size = sizeof(mlsHeader);
		memset(this,0,size);
		strcpy(fileSignature,"MLSF");
	}
	char	fileSignature[4];
	U32		prjGUID1;
	U16		prjGUID2;
	U16		prjGUID3;
	char	prjGUID4[8];

	U8		versionMajor;
	U8		versionMinor;
	char	sysIdentifier[32];
	char	genSoftware[32];

	U16		creationDay;
	U16		creationYear;
	U16		updateDay;
	U16		updateYear;

	U16		headerSize;

	F64		minX;
	F64		maxX;
	F64		minY;
	F64		maxY;
	F64		minZ;
	F64		maxZ;

	U32		blockNum;
};

typedef struct mlsBlockIndex
{
	mlsBlockIndex()
	{
		size_t size = sizeof(mlsBlockIndex);
		memset(this,0,size);
		hasIntensity = 1;
		pointRecLength = 14;
	}
	U32		dataOffset;				//当前块所在文件位置
	U32		numOfPoint;				//块内点个数
	char	dataFile[64];			//数据文件名称

	U8		hasIntensity:1;			//是否包含强度属性
	U8		hasRGB:1;				//是否包含RGB属性
	U8		hasClassification:1;	//是否包含分类属性
	U8		pointRecLength:5;		//点记录长度

	F64		minX;					//范围
	F64		maxX;
	F64		minY;
	F64		maxY;
	F64		minZ;
	F64		maxZ;
};