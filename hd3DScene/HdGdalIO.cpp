/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdImageRender
文件名		：HdRasterImage.h
相关文件	: GDAL,HdImageBuffer.h
文件实现功能：基于GDAL进行解析图像
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/4/14	1.0			马振明		创建
</PRE>
******************************************************************************************************/
#include "stdafx.h"
#include "HdGdalIO.h"
#include <io.h>
#include "ChdTfw.h"
#include "OverViewInfo.h"
#include <exception>
#include "gdal.h"

using namespace hd::scene;

// 初始化静态成员变量
int ChdGdalIO::m_sRegisterCount = 0;

ChdGdalIO::ChdGdalIO(void)
	:m_pImageDS(NULL)
	,m_eDataType(GDT_Unknown)
{
}


ChdGdalIO::~ChdGdalIO(void)
{
	// 关闭
	if (m_pImageDS)
	{
		GDALClose(m_pImageDS);
		m_pImageDS = NULL;
	}
}

// 打开图像,默认为读
bool ChdGdalIO::Open(const char* strFilePath,GDALRWFlag eFlags,int nWriteH,int nWriteW,int nBandNum,GDALDataType eType)
{
	// 如果为空，返回空
	if (NULL == strFilePath)
	{
		return false;
	}

	m_strFilePath = strFilePath;

	// 获取文件类型
	int nLen = m_strFilePath.find_last_of(".");
	string strExt = m_strFilePath.substr(nLen+1,m_strFilePath.size());

	if (strExt == "tif" || strExt == "TIF")
	{
		m_eFileType = GFT_TIFF;
	}
	else if (strExt == "jpg" || strExt == "JPG")
	{
		m_eFileType = GFT_JPG;
	}
	else
	{
		m_eFileType = GFT_DEFALUT;
	}

	// 通过文件名判断保存类型
	Close();

	// 判断数据类型,是tiff，还是其他
	try
	{
		// 如果是读
		if (eFlags == GF_Read)
		{
			m_pImageDS = (GDALDataset*)GDALOpen(strFilePath,GA_ReadOnly);

			// 如果打开失败，关闭，并返回flase
			if (m_pImageDS && m_pImageDS->GetRasterCount()>0)
			{
				m_eDataType = m_pImageDS->GetRasterBand(1)->GetRasterDataType();
				return true;
			}
			else
			{
				Close();
				return false;
			}
		}
		// 设置为写模式
		else if (eFlags == GF_Write)
		{
			string strGType = "";
			// 获取写入的类型
			if (strExt == "tif" || strExt == "TIF")
			{
				strGType = "GTiff";
			}
			else if (strExt == "jpg" || strExt == "JPG")
			{
				strGType = "JPEG";
			}
			else if (strExt == "png" || strExt == "PNG")
			{
				strGType = "PNG";
			}

			// 设置驱动
			GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(strGType.c_str());  
			char** papszOptions = NULL;
			m_pImageDS = poDriver->Create(strFilePath,nWriteW,nWriteH,nBandNum,eType,papszOptions);
			
			if (!m_pImageDS)
			{
				Close();
				return false;
			}
			else
			{
 				m_eDataType = eType;
				return true;
			}
		}
	}
	catch (...)
	{
		Close();
		return false;
	}

	return false;
}

// 关闭数据集
void ChdGdalIO::Close()
{
	if (m_pImageDS)
	{
		// 释放内存
		GDALClose(m_pImageDS);
		m_pImageDS = NULL;
	}
}

// 获取坐标信息,注意传入的要是double 6的数组
// 数组大小为6，存储顺序为minx,miny,maxx,maxy,fcellX,fcelly
bool ChdGdalIO::GetExtInfo(IMAGEEXTINFOPTR pImageExtInfo) const
{
	if (!m_pImageDS || !pImageExtInfo) 
	{
		return false;
	}
	else
	{
		// 影像路径
		pImageExtInfo->strImagePath = m_strFilePath;

		int nLen =m_strFilePath.find_last_of("\\");
		string strImageName = m_strFilePath.substr(nLen+1,m_strFilePath.size());

		// 赋值名称
		pImageExtInfo->strImageName = strImageName.substr(0,strImageName.find_last_of("."));

		// 赋值后缀
		pImageExtInfo->strExtName = strImageName.substr(strImageName.find_last_of(".")+1,strImageName.size());

		// 宽高
		pImageExtInfo->nWidth = m_pImageDS->GetRasterXSize();
		pImageExtInfo->nHeight = m_pImageDS->GetRasterYSize();
		pImageExtInfo->nBand = m_pImageDS->GetRasterCount();
		
		double dProjTrans[6];
		memset(dProjTrans,0,6);
		// 读取坐标信息
		m_pImageDS->GetGeoTransform(dProjTrans);

		// 如果是Tiff类型
		if (GFT_TIFF == m_eFileType)
		{
			// 如果从tif中读取不到，用tfw中的为准
			// 注意tfw存储的左上角的起始点像素中心的坐标，所以要求出其范围，需要进行外扩半个范围
			// 判断同样路径下是否存在tfw文件，如果存在，读出其坐标
			string strTfw = m_strFilePath.substr(0,m_strFilePath.find_first_of("."));
			strTfw += ".tfw";

			double dProjTransTfw[6];
			if (_access(strTfw.c_str(),00)==0)
			{
				ChdTfw tfw;
				tfw.Read(strTfw.c_str(),dProjTransTfw);

				// 判断tfw记录的坐标是否和tiff中读取的是否相等
				// 不等按tfw读取，tfw默认为左上角像素中心坐标
				if (dProjTrans[0]!=dProjTransTfw[4] 
					&& dProjTrans[3] != dProjTransTfw[5])
				{
					dProjTrans[0] = dProjTransTfw[4]-dProjTransTfw[0]/2;
					dProjTrans[1] = dProjTransTfw[0];

					dProjTrans[2] = dProjTransTfw[1];
					dProjTrans[3] = dProjTransTfw[5]-dProjTransTfw[3]/2;
					dProjTrans[4] = dProjTransTfw[2];
					dProjTrans[5] = dProjTransTfw[3];
				}
			}

			// 计算X的最小值
			pImageExtInfo->dLeft = dProjTrans[0];

			// 计算X的最大值
			pImageExtInfo->dRight = dProjTrans[0]+pImageExtInfo->nWidth*dProjTrans[1];

			// 分辨率
			pImageExtInfo->fScale = abs(dProjTrans[1]);
			// 判断Y的最小值
			if (dProjTrans[5]>=0)
			{
				pImageExtInfo->dBottom = dProjTrans[3];
				// 计算Y的最大值
				pImageExtInfo->dTop = dProjTrans[3]+dProjTrans[5]*m_pImageDS->GetRasterYSize();
			}
			else
			{
				pImageExtInfo->dBottom = dProjTrans[3]+dProjTrans[5]*m_pImageDS->GetRasterYSize();
				// 计算Y的最大值
				pImageExtInfo->dTop = dProjTrans[3];
			}
		}
		else
		{
			// 计算X的最小值
			pImageExtInfo->dLeft = dProjTrans[0];

			// 计算X的最大值
			pImageExtInfo->dRight = pImageExtInfo->nWidth;

			// 分辨率
			pImageExtInfo->fScale = dProjTrans[1];

			pImageExtInfo->dBottom = 0;

			pImageExtInfo->dTop = pImageExtInfo->nHeight;
		}

		return true;
	}
}

// 获取影像的基本信息，返回指针，未得到，返回NULL；@确定使用完之后要手动释放该指针
IMAGEEXTINFO* ChdGdalIO::GetExtInfo() const
{
	if (!m_pImageDS) 
	{
		return NULL;
	}
	else
	{
		IMAGEEXTINFO* pImageExtInfo = NULL;
		try
		{
			pImageExtInfo = new IMAGEEXTINFO();
			bool bResult = GetExtInfo(pImageExtInfo);

			// 如果读取失败，释放内存
			if (!bResult)
			{
				delete pImageExtInfo;
				pImageExtInfo =NULL;

				return NULL;
			}

			return pImageExtInfo;
		}
		catch (...)
		{
			// 释放
			if (pImageExtInfo)
			{
				delete pImageExtInfo;
				pImageExtInfo =NULL;

				return NULL;
			}
			
		}

		return NULL;
	}
}

// 获取金字塔信息,没有返回Null
bool ChdGdalIO::GetOverViewInfo(vector<OVERVIEWLEVEL*>& vectOVLevels) const
{
	// 判断是否有效
	if (!m_pImageDS)
	{
		return false;
	}

	// 获取金字塔的层级
	int nPyramidCount = m_pImageDS->GetRasterBand(1)->GetOverviewCount();

	// 如果没有金字塔，返回Null
	if (nPyramidCount==0)
	{
		return false;
	}

	// 获取原始高和宽
	int nSrcY =  m_pImageDS->GetRasterYSize();
	int nSrcX =  m_pImageDS->GetRasterXSize();

	// 获取波段信息
	for (int i=0;i<nPyramidCount;i++)
	{
		GDALRasterBand* pTmpBand = m_pImageDS->GetRasterBand(1)->GetOverview(i);

		OVERVIEWLEVEL* pLevel = new OVERVIEWLEVEL;
		// 级别号加1,0级即原始级别
		pLevel->nLevelIndex = i+1;
		pLevel->nRow = pTmpBand->GetYSize();
		pLevel->nCol = pTmpBand->GetXSize();

		// 按高求,其与原始图像的比例
		pLevel->fScaleY = static_cast<float>(pLevel->nRow)/nSrcY;
		pLevel->fScaleX = static_cast<float>(pLevel->nCol)/nSrcX;

		// 添加
		vectOVLevels.push_back(pLevel);
	}

	return true;
}


// 写坐标信息
bool ChdGdalIO::WriteProj(double* dProj)
{
	// 判断数据的格式类型,只有在Tiff格式时才写
	if ( GFT_TIFF == m_eFileType)
	{
		// 如果坐标信息不为空
		if (dProj && m_pImageDS)
		{
			const char *pszSRS_WKT = m_pImageDS->GetProjectionRef();
			m_pImageDS->SetProjection(pszSRS_WKT);

			// 设置坐标信息
			m_pImageDS->SetGeoTransform(dProj);

			return true;
		}
		else
		{
			return false;
		}
	}
	
	return false;
}

// 写tfw文件,传入的是按tiff中写的格式
bool ChdGdalIO::WriteTfw(double* dProjTrans)
{
	if (!dProjTrans)
	{
		return false;
	}

	// 判断数据的格式类型,只有在Tiff格式时才写
	if ( GFT_TIFF == m_eFileType)
	{
		// 判断同样路径下是否存在tfw文件，如果存在，读出其坐标
		string strTfw = m_strFilePath.substr(0,m_strFilePath.find_last_of("."));
		strTfw += ".tfw";

		// 赋值,tfw存储的是左上角像素中心点的坐标
		double dProjTransTfw[6];
		dProjTransTfw[4] = dProjTrans[0]+dProjTrans[1]/2;
		dProjTransTfw[0] = dProjTrans[1];

		dProjTransTfw[1] = dProjTrans[2];
		dProjTransTfw[5] = dProjTrans[3]+dProjTrans[5]/2;
		dProjTransTfw[2] = dProjTrans[4];
		dProjTransTfw[3] = dProjTrans[5];

		// 写文件
		ChdTfw tfw;
		return tfw.Write(strTfw.c_str(),dProjTransTfw);
	}
	else
	{
		return false;
	}
}

// 读取影像
CHdGeoRaster* ChdGdalIO::ReadImage(double dScale)
{
	// 如果为空，返回空
	if (NULL == m_pImageDS || GDT_Unknown == m_eDataType)
	{
		return NULL;
	}

	// 防止dScale过大或过小
	if (dScale<=0 && dScale>5)
	{
		dScale =1.0;
	}

	CHdGeoRaster* pImageBuffer = NULL;

	// 如果波段类型是GDT_BYTE
	if (GDT_Byte == m_eDataType)
	{
		pImageBuffer = ReadByte(dScale);
	}

	// 如果是Float32类型
	else if (GDT_Float32 == m_eDataType)
	{
		pImageBuffer = Read32FtoByte(dScale);	
	}

	// 如果是Sixteen bit unsigned integer类型
	else if (GDT_UInt16 == m_eDataType)
	{
		pImageBuffer = Read16UInttoByte(dScale);	
	}
	// 其他类型暂时不支持
	else
	{
		return NULL;
	}

	// 如果为空
	if (NULL == pImageBuffer)
	{
		// 释放内存
		return NULL;
	}

	double dProjTrans[6];
	memset(dProjTrans,0,6);

	// 读取坐标信息
	m_pImageDS->GetGeoTransform(dProjTrans);

	// 如果不是Tiff类型
	if (m_eFileType != GFT_TIFF)
	{
		dProjTrans[3] = m_pImageDS->GetRasterYSize();
		dProjTrans[5] = -dProjTrans[5];
	}

	// 定义Y值方向的偏移量与左下角的Y值
	double dCellY(0),dDownY(0);

	int nWidh = m_pImageDS->GetRasterXSize();

	// y值方向的像素分辨率
	dCellY = fabs(dProjTrans[5]);

	// 设置大小
	pImageBuffer->SetCellSize((float)(dProjTrans[1]/dScale),(float)(dCellY/dScale));

	// 计算左上角的地理坐标
	pImageBuffer->SetLT(dProjTrans[0],dProjTrans[3]);

	// 如果Y轴分辨率为正,重新设置左上角
	if (dProjTrans[5]>0)
	{
		pImageBuffer->SetLT(dProjTrans[0],dProjTrans[3]+m_pImageDS->GetRasterYSize()*dCellY);
	}

	pImageBuffer->UpdateBoundingBox();

	// 解析名称
	const char* pName = NULL;
	pName = strrchr(m_strFilePath.c_str(),'\\');

	string strName = pName;

	// 截取\ 
	strName = strName.substr(1,strName.length());
	// 截取后缀
	strName = strName.substr(0,strName.length()-4);
	
	pImageBuffer->SetName(strName.c_str());
	return pImageBuffer;
}

// 根据范围读取影像,添加缩放比例
CHdGeoRaster* ChdGdalIO::ReadImage(const Chd2DBoundingBoxd& bRect,double dScale,bool bBand)
{
	// 如果为空，返回空
	if (NULL == m_pImageDS || GDT_Unknown == m_eDataType)
	{
		return NULL;
	}

	// 防止dScale过大或过小
	if (dScale<=0 && dScale>5)
	{
		dScale =1.0;
	}

	// 判断能读取的范围，传入的范围有可能越界
	double dProjTrans[6];
	memset(dProjTrans,0,6);

	int nHeight =m_pImageDS->GetRasterYSize();
	int nWidth = m_pImageDS->GetRasterXSize();

	// 读取坐标信息
	m_pImageDS->GetGeoTransform(dProjTrans);


	// 如果不是Tiff类型
	if (m_eFileType != GFT_TIFF)
	{
		dProjTrans[3] = m_pImageDS->GetRasterYSize();
		dProjTrans[5] = -dProjTrans[5];
	}

	double dMinx(0),dMaxx(0),dMiny(0),dMaxy(0);

	// 求范围
	dMinx = MAX(bRect.GetMinX(),dProjTrans[0]);
	dMaxx = MIN(bRect.GetMaxX(),dProjTrans[0]+nWidth*dProjTrans[1]);

	dMiny = MAX(bRect.GetMinY(), dProjTrans[3]+nHeight*dProjTrans[5]);
	dMaxy = MIN(bRect.GetMaxY(),dProjTrans[3]);


	float fCellX(0),fCellY(0);
	fCellX = dProjTrans[1];

	// 计算分辨率
	if (dProjTrans[5]>=0)
	{
		fCellY = dProjTrans[5];
	}
	else
	{
		fCellY = -dProjTrans[5];
	}

	// 读取的宽高，用于构造raster
	int nReadHeight(0);
	int nReadWidth(0);
	int nBandNum(0);
	// 新建一个GeoRaster
	CHdGeoRaster* pImageBuffer = new CHdGeoRaster();

	double dLeft(0),dTop(0);

	// 判断数据类型
	if (GDT_Byte == m_eDataType)
	{
		BYTE* pBuffer = ReadImageByWorld<BYTE>(dMinx,dMaxx,dMiny,dMaxy,nReadHeight,nReadWidth,nBandNum,dLeft,dTop,dScale,bBand);

		// 创建
		if(!pImageBuffer->Creat(pBuffer,nReadHeight,nReadWidth,nBandNum,nBandNum,E_PT_BYTE))
		{
			delete pImageBuffer;
			pImageBuffer = NULL;
			return NULL;
		}
	}
	else if (GDT_Float32 == m_eDataType)
	{
		float* pBuffer = ReadImageByWorld<float>(dMinx,dMaxx,dMiny,dMaxy,nReadHeight,nReadWidth,nBandNum,dLeft,dTop,dScale,false);

		// 创建
		if(!pImageBuffer->Creat(pBuffer,nHeight,nWidth,nBandNum,nBandNum*sizeof(float),E_PT_FLOAT))
		{
			delete pImageBuffer;
			pImageBuffer = NULL;
			return NULL;
		}
	}
	else
	{
		return NULL;
	}

	// 赋值属性
	if (pImageBuffer)
	{
		// 设置为按像素块排列
		pImageBuffer->SetBufferBand(false);

		// 设置大小
		pImageBuffer->SetCellSize(fCellX/dScale,fCellY/dScale);

		// 计算左上角的地理坐标
		pImageBuffer->SetLT(dLeft,dTop);

		pImageBuffer->UpdateBoundingBox();

		// 解析名称
		const char* pName = NULL;
		pName = strrchr(m_strFilePath.c_str(),'\\');

		string strName = pName;

		// 截取\ 
		strName = strName.substr(1,strName.length());
		// 截取后缀
		strName = strName.substr(0,strName.length()-4);

		pImageBuffer->SetName(strName.c_str());
	}
	
	return pImageBuffer;
}

//读取16UInt影像到Byte,添加缩放比例
CHdGeoRaster* ChdGdalIO::Read16UInttoByte(double dScale)
{
	// 防止dScale过大或过小
	if (dScale<=0 && dScale>5)
	{
		dScale =1.0;
	}

	UINT16* pFolat = NULL;

	int nBandNum(0),nHeight(0),nWidth(0);
	UINT16 dMax(-USHRT_MAX),dMin(USHRT_MAX);

	double dProj[6];

	// 读取数据
	pFolat = ReadFullImage<UINT16>(nWidth,nHeight,nBandNum,dProj,dMax,dMin,dScale);

	if (!pFolat)
	{
		return NULL;
	}

	// 申请内存，存储Byte
	CHdGeoRaster* pImageBuffer = new CHdGeoRaster();
	bool bResult = pImageBuffer->Creat(pFolat,nHeight,nWidth,nBandNum,nBandNum*sizeof(UINT16),E_PT_UNINT);
	pImageBuffer->m_fMax = (float)dMax;
	pImageBuffer->m_fMin = (float)dMin;
	// 如果创建失败
	if (!bResult)
	{
		delete pImageBuffer;
		pImageBuffer = NULL;
		return NULL;
	}

	return pImageBuffer;	
}

// 转换float32位到Byte
CHdGeoRaster* ChdGdalIO::Read32FtoByte(double dScale)
{
	// 防止dScale过大或过小
	if (dScale<=0 && dScale>5)
	{
		dScale =1.0;
	}

	float* pFolat = NULL;

	int nBandNum(0),nHeight(0),nWidth(0);
	float dMax(FLT_MIN),dMin(FLT_MAX);

	double dProj[6];

	// 读取数据
	pFolat = ReadFullImage<float>(nWidth,nHeight,nBandNum,dProj,dMax,dMin,dScale);

	if (!pFolat)
	{
		return NULL;
	}
	
	// 申请内存，存储Byte
	CHdGeoRaster* pImageBuffer = new CHdGeoRaster();
	bool bResult = pImageBuffer->Creat(pFolat,nHeight,nWidth,nBandNum,nBandNum*sizeof(float),E_PT_FLOAT);
	pImageBuffer->m_fMax = (float)dMax;
	pImageBuffer->m_fMin = (float)dMin;
	// 如果创建失败
	if (!bResult)
	{
		delete pImageBuffer;
		pImageBuffer = NULL;
		return NULL;
	}

	return pImageBuffer;	
}

 //读取Byte数据
CHdGeoRaster* ChdGdalIO::ReadByte(double dScale)
{
	// 防止dScale过大或过小
	if (dScale<=0 && dScale>5)
	{
		dScale =1.0;
	}

	// 判断
	if (NULL == m_pImageDS)
	{
		return NULL;
	}

	int nBandNum = m_pImageDS->GetRasterCount();
	int nSrcWidth = m_pImageDS->GetRasterXSize();
	int nSrcHeight = m_pImageDS->GetRasterYSize();

	int nWidth = int(nSrcWidth*dScale+0.5);
	int nHeight = int(nSrcHeight*dScale+0.5);
	
	// 临时buffer对象
	BYTE* pBuffer = new BYTE[nBandNum*nHeight*nWidth];

	// 如果波段类型是GDT_BYTE
	if (GDT_Byte ==  m_eDataType)
	{
		// GDAL默认按波段读取，改成按像素值读取
		// 转换成b,g,r的读取方式
		// 定义波段读取顺序，默认为倒序
		// 考虑到3个波段时是按需要转换成bgr格式，所以进行统一为倒序方式
		int *pBandMap = new int[nBandNum];

		for (int i = 0;i<nBandNum;++i)
		{
			pBandMap[i]=nBandNum -i;
		}
			
		// 读取影像
		if (CE_Failure == m_pImageDS->RasterIO(GF_Read,0,0,nSrcWidth,nSrcHeight,
			pBuffer,nWidth,nHeight,
			GDT_Byte,nBandNum,pBandMap,nBandNum,
			nBandNum*nWidth,1))
		{
		
			delete []pBandMap;
			pBandMap = NULL;

			return NULL;
		}

		// 释放内存
		if (pBandMap)
		{
			delete []pBandMap;
			pBandMap = NULL;
		}

		// 新建一个GeoRaster
		CHdGeoRaster* pImageBuffer = new CHdGeoRaster();
		if(pImageBuffer->Creat(pBuffer,nHeight,nWidth,nBandNum,nBandNum,E_PT_BYTE))
		{
			return pImageBuffer;
		}
		else
		{
			delete pImageBuffer;
			pImageBuffer = NULL;
			return NULL;
		}
	}
	else
	{
		delete []pBuffer;
		pBuffer = NULL;
		return NULL;
	}
}
