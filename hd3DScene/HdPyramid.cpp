#include "StdAfx.h"
#include "HdPyramid.h"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdal_alg.h"
#include <vector>
using namespace std;
using namespace hd::scene;

CHdPyramid::CHdPyramid(void)
{
}


CHdPyramid::~CHdPyramid(void)
{
}


// 生成金字塔,返回金字塔的级别,
// 传入参数为路径，金字塔每块的标准高和宽
int CHdPyramid::CreatePyramids(const char* strFilePath,int nPyramidH,int nPyramidW)
{
	//设置支持Erdas与ArcGIS格式的字塔文件
	CPLSetConfigOption("USE_RRD","YES");    
	// 设置支持中文路径
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	// 打开数据
	GDALDatasetH hDataset;
	hDataset = GDALOpen(strFilePath, GA_ReadOnly );

	GDALDriverH hDriver = GDALGetDatasetDriver(hDataset);
	const char* pszDriver = GDALGetDriverShortName(hDriver);

	if( hDataset == NULL )
	{
		// 清除数据
		GDALClose(hDataset);
		return -1;
	}

	// 获取图像的基本信息
	int iWidth = GDALGetRasterXSize(hDataset);
	int iHeigh = GDALGetRasterYSize(hDataset);

	// 如果达不到采样大小,不采样
	if (nPyramidW>iWidth || nPyramidH>iHeigh)
	{
		// 清除数据
		GDALClose(hDataset);
		return -1;
	}

	int iPixelNum = iWidth * iHeigh;    // 图像中的总像元个数
	int iTopNum = nPyramidH*nPyramidW;  // 顶层金字塔大小
	int iCurNum = iPixelNum / 4;

	int anLevels[1024] = { 0 };
	 //金字塔级数
	int nLevelCount = 0;               

	//计算金字塔级数，从第二级到顶层
	do    
	{
		anLevels[nLevelCount] = static_cast<int>(pow(2.0, nLevelCount+2));
		nLevelCount ++;
		iCurNum /= 4;
	} while (iCurNum > iTopNum);

	const char *pszResampling = "nearest"; //采样方式

	// 生成金字塔
	if (nLevelCount > 0 &&
		GDALBuildOverviews( hDataset,pszResampling, nLevelCount, anLevels,
		0, NULL, GDALDummyProgress, NULL ) != CE_None )
	{
		// 清除数据
		GDALClose(hDataset);
		return -1;
	}

	// 清除数据
	GDALClose(hDataset);
	return nLevelCount;
}

// 获取金字塔信息
int CHdPyramid::GetPyramidsInfo(const char* strFilePath)
{
	// 设置支持中文路径
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	// 打开数据
	GDALDataset* hDataset = NULL;
	hDataset = (GDALDataset*)GDALOpen(strFilePath, GA_ReadOnly );

	if( hDataset == NULL )
	{
		// 清除数据
		GDALClose(hDataset);
		return -1;
	}

	// 获取金字塔的层级
	int nPyramidCount = hDataset->GetRasterBand(1)->GetOverviewCount();

	vector<int> vectX;
	vector<int> vectY;

	// 获取波段信息
	for (int i=0;i<nPyramidCount;i++)
	{
		GDALRasterBand* pTmpBand = hDataset->GetRasterBand(1)->GetOverview(i);
		vectX.push_back(pTmpBand->GetXSize());
		vectY.push_back(pTmpBand->GetYSize());
	}

	// 清除数据
	GDALClose(hDataset);

	return 1;
}


/***  
* 遥感影像重采样 
* @param pszSrcFile        输入文件的路径  
* @param pszOutFile        写入的结果图像的路径  
* @param eResample         采样模式，有五种，具体参见GDALResampleAlg定义，默认为双线性内插  
                            GRA_NearestNeighbour=0      最近邻法，算法简单并能保持原光谱信息不变；缺点是几何精度差，灰度不连续，边缘会出现锯齿状     
                            GRA_Bilinear=1              双线性法，计算简单，图像灰度具有连续性且采样精度比较精确；缺点是会丧失细节； 
                            GRA_Cubic=2                 三次卷积法，计算量大，图像灰度具有连续性且采样精度高； 
                            GRA_CubicSpline=3           三次样条法，灰度连续性和采样精度最佳； 
                            GRA_Lanczos=4               分块兰索斯法，由匈牙利数学家、物理学家兰索斯法创立，实验发现效果和双线性接近； 
* @param fResX             X转换采样比，默认大小为1.0，大于1图像变大，小于1表示图像缩小。数值上等于采样后图像的宽度和采样前图像宽度的比  
* @param fResY             Y转换采样比，默认大小为1.0，大于1图像变大，小于1表示图像缩小。数值上等于采样后图像的高度和采样前图像高度的比 
* @retrieve     0   成功 
* @retrieve     -1  打开源文件失败 
* @retrieve     -2  创建新文件失败 
* @retrieve     -3  处理过程中出错 
*/    
int CHdPyramid::ResampleGDAL(const char* pszSrcFile, const char* pszOutFile, float fResX, float fResY,GDALResampleAlg eResample)    
{    
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");     
    GDALDataset *pDSrc = (GDALDataset *)GDALOpen(pszSrcFile, GA_ReadOnly);    
    if (pDSrc == NULL)    
    {    
        return -1;    
    }    
  
    GDALDriver *pDriver = GetGDALDriverManager()->GetDriverByName("GTiff");    
    if (pDriver == NULL)    
    {    
        GDALClose((GDALDatasetH) pDSrc);    
        return -2;    
    }    
    int width=pDSrc->GetRasterXSize();    
    int height=pDSrc->GetRasterYSize();  
    int nBandCount = pDSrc->GetRasterCount();     
    GDALDataType dataType = pDSrc->GetRasterBand(1)->GetRasterDataType();    
  
    char *pszSrcWKT = NULL;  
    pszSrcWKT = const_cast<char *>(pDSrc->GetProjectionRef());  
   
    double dGeoTrans[6] = {0};    
    int nNewWidth=width,nNewHeight=height;  
    pDSrc->GetGeoTransform(dGeoTrans);   
  
    bool bNoGeoRef = false;  
    double dOldGeoTrans0 = dGeoTrans[0]; 

    //如果没有投影，人为设置一个    
    if(strlen(pszSrcWKT)<=0)  
    {  
        dGeoTrans[0]=1.0;  
        pDSrc->SetGeoTransform(dGeoTrans);  

        bNoGeoRef = true;  
    }     
  
    dGeoTrans[1] = dGeoTrans[1] / fResX;  
    dGeoTrans[5] = dGeoTrans[5] / fResY;  
    nNewWidth = static_cast<int>(nNewWidth*fResX+0.5);  
    nNewHeight = static_cast<int>(nNewHeight*fResY+0.5);  
  
    //创建结果数据集  
    GDALDataset *pDDst = pDriver->Create(pszOutFile, nNewWidth, nNewHeight, nBandCount, dataType, NULL);    
    if (pDDst == NULL)    
    {     
        GDALClose((GDALDatasetH) pDSrc);    
        return -2;    
    }    
      
    pDDst->SetProjection(pszSrcWKT);    
    pDDst->SetGeoTransform(dGeoTrans);       
  
  
    void *hTransformArg = NULL;  
    hTransformArg = GDALCreateGenImgProjTransformer2((GDALDatasetH) pDSrc, (GDALDatasetH) pDDst, NULL);   
  
    if (hTransformArg == NULL)    
    {    
        GDALClose((GDALDatasetH) pDSrc);    
        GDALClose((GDALDatasetH) pDDst);    
        return -3;    
    }  
   
    GDALWarpOptions *psWo = GDALCreateWarpOptions();    
  
    psWo->papszWarpOptions = CSLDuplicate(NULL);    
    psWo->eWorkingDataType = dataType;    
    psWo->eResampleAlg = eResample;    
  
    psWo->hSrcDS = (GDALDatasetH) pDSrc;    
    psWo->hDstDS = (GDALDatasetH) pDDst;    
  
    psWo->pfnTransformer = GDALGenImgProjTransform;    
    psWo->pTransformerArg = hTransformArg;  
  
  
    psWo->nBandCount = nBandCount;    
    psWo->panSrcBands = (int *) CPLMalloc(nBandCount*sizeof(int));    
    psWo->panDstBands = (int *) CPLMalloc(nBandCount*sizeof(int));    
    for (int i=0; i<nBandCount; i++)    
    {    
        psWo->panSrcBands[i] = i+1;    
        psWo->panDstBands[i] = i+1;   
    }     
   
    GDALWarpOperation oWo;    
    if (oWo.Initialize(psWo) != CE_None)    
    {    
        GDALClose((GDALDatasetH) pDSrc);    
        GDALClose((GDALDatasetH) pDDst);    
        return -3;    
    }      
  
    oWo.ChunkAndWarpImage(0, 0, nNewWidth, nNewHeight);   
  
    GDALDestroyGenImgProjTransformer(hTransformArg);    
    GDALDestroyWarpOptions( psWo );       
    if(bNoGeoRef)   
    {  
        dGeoTrans[0]=dOldGeoTrans0;  
        pDDst->SetGeoTransform(dGeoTrans);       
    }   

    GDALFlushCache( pDDst );  
    GDALClose((GDALDatasetH) pDSrc);    
    GDALClose((GDALDatasetH) pDDst);    
   
    return 0;    
}   

// 裁切,传入裁剪区域的左上点坐标，以及右下点坐标,以及缩放的系数
int CHdPyramid::ClipGDAL(const char* pszSrcFile, const char* pszOutFile,double dLX,double dUY,double dRX,double dDY,double dScale)
{
	//读入文件
	GDALDataset *poDataset;
	// 设置支持中文路径
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

	//open a dataset
	poDataset =(GDALDataset *)GDALOpen(pszSrcFile,GA_ReadOnly);

	if(poDataset == NULL)
	{
		// 清除数据
		GDALClose(poDataset);
		return -1;
	}

	return ClipGDAL(poDataset,pszOutFile,dLX,dUY,dRX,dDY,dScale);
}

// 数据裁切,传入裁剪区域的左上点坐标，以及右下点坐标,以及缩放的系数
int CHdPyramid::ClipGDAL(GDALDataset * poDataset, const char* pszOutFile,double dLX,double dUY,double dRX,double dDY,double dScale)
{
	if (!poDataset)
	{
		return -1;
	}
	// 设置支持中文路径
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

	// 设置驱动
	GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");   

	//设置相关参数为实现裁切做准备
	char **papsOptions = NULL;

	// 获取波段个数
	int bandCount = GDALGetRasterCount(poDataset);
	int nWidth = GDALGetRasterXSize(poDataset);
	int nHeight = GDALGetRasterYSize(poDataset);

	// 设置值
	int nStartX(0),nEndX(0),nStartY(0),nEndY(0),nXSize(0),nYSize(0);
	// 获取坐标信息
	double adfGeoTransform[6];
	memset(adfGeoTransform,0,6);
	// 获取坐标信息
	poDataset->GetGeoTransform(adfGeoTransform);

	// 如果是像素坐标
	if ((dLX<nWidth && dRX<=nWidth) &&((dUY<nHeight)&&(dDY<=nHeight)))
	{
		nStartX = dLX;
		nEndX = dRX;
		nStartY = dUY;
		nEndY = dDY;

		adfGeoTransform[0] +=nStartX*adfGeoTransform[1];
		adfGeoTransform[3] -=nStartY*adfGeoTransform[5];
	}

	// 如果输入范围过大
	else if ((dRX-dLX)>nWidth || (dUY-dDY)>nHeight)
	{
		nStartX = 0;
		nEndX = nWidth;
		nStartY = 0;
		nEndY = nHeight;

		adfGeoTransform[0] +=nStartX*adfGeoTransform[1];
		adfGeoTransform[3] -=nStartY*adfGeoTransform[5];
	}
	else	// 地理坐标
	{
		// 设置值
		int nStartX(0),nEndX(0),nStartY(0),nEndY(0),nXSize(0),nYSize(0);

		// 判断是否超界
		if (dLX-adfGeoTransform[0]<=0)
		{
			dLX = adfGeoTransform[0];
		}

		if (dUY-adfGeoTransform[3]>=0)
		{
			dUY = adfGeoTransform[3];
		}

		// 计算起点的XY
		nStartX = (dLX-adfGeoTransform[0])/adfGeoTransform[1];
		nStartY = (dUY - adfGeoTransform[3])/adfGeoTransform[5];

		// 计算终点的XY
		nEndX = (dRX-adfGeoTransform[0])/adfGeoTransform[1];
		nEndY = (dDY - adfGeoTransform[3])/adfGeoTransform[5];

		// 重新设置裁剪区域的坐标
		adfGeoTransform[0] = dLX;
		adfGeoTransform[3] = dUY;	
	}

	// 设置分分辨率
	adfGeoTransform[1]/=dScale;
	adfGeoTransform[5]/=dScale;

	// 重新判断起始点大小
	if (nStartX>nEndX)
	{
		int nTmp = nEndX;
		nEndX = nStartX;
		nStartX = nTmp;
	}

	if (nStartY>nEndY)
	{
		int nTmp = nEndY;
		nEndY = nStartY;
		nStartY = nTmp;
	}

	// 计算所需宽高
	nXSize = (nEndX- nStartX)*dScale;
	nYSize = (nEndY- nStartY)*dScale;

	//生成一个用于存放数据的缓存空间
	GDALDataset *poDstDS = NULL;

	void* pRaster = NULL;
	GDALDataType eDataType = GDT_Byte;

	// 判断波段类型,如果是8位的Byte类型
	if (GDT_Byte == poDataset->GetRasterBand(1)->GetRasterDataType())
	{
		pRaster = new GByte[nXSize*nYSize*bandCount];
		poDstDS = poDriver->Create(pszOutFile,nXSize,nYSize,bandCount,GDT_Byte,papsOptions);
		eDataType = GDT_Byte;
	}

	// 如果是float32
	else if(GDT_Float32 == poDataset->GetRasterBand(1)->GetRasterDataType())
	{
		pRaster = new float[nXSize*nYSize*bandCount];
		poDstDS = poDriver->Create(pszOutFile,nXSize,nYSize,bandCount,GDT_Float32,papsOptions);
		eDataType = GDT_Float32;
	}
	// 如果是float64
	else if(GDT_Float64 == poDataset->GetRasterBand(1)->GetRasterDataType())
	{
		pRaster = new double[nXSize*nYSize*bandCount];
		poDstDS = poDriver->Create(pszOutFile,nXSize,nYSize,bandCount,GDT_Float64,papsOptions);
		eDataType = GDT_Float64;
	}

	const char *pszSRS_WKT = poDataset->GetProjectionRef();
	poDstDS->SetProjection(pszSRS_WKT);

	// 设置坐标信息
	poDstDS->SetGeoTransform(adfGeoTransform);

	//进行裁切工作，读取原始的数据，写入目标文件
	poDataset->RasterIO(GF_Read,nStartX,nStartY,nEndX-nStartX,nEndY-nStartY,pRaster,
		nXSize,nYSize,eDataType,bandCount,NULL,0,0,0);

	// 重写数据
	poDstDS->RasterIO(GF_Write,0,0,nXSize,nYSize,pRaster,
		nXSize,nYSize,eDataType,bandCount,NULL,0,0,0);

	// 清除数据
	GDALClose(poDataset);
	GDALClose(poDstDS);

	// 释放内存
	if (pRaster)
	{
		delete []pRaster;
		pRaster =NULL;
	}
	return 1;
}