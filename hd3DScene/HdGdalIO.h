/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdDataDriver
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
// 基本调用方法类似常见的文件调用
// 读取示例
ChdGdalIO gdalRead;
if (!gdalRead.Open("路径"))
{
"读取失败"
}

// 得到基本影像信息
IMAGEEXTINFO* pImageInfo = gdalRead.GetExtInfo();
gdalRead.Close();

// 写影像简单示例
ChdGdalIO gdalwrite;
gdalwrite.Open("G:\\Test\\test.jpg",GF_Write,nRows,nCols,nBandNum);
gdalwrite.WriteImage<BYTE>((BYTE*)m_pBuffer,pRaster->GetBandNum(),0,0,nRows,nCols,NULL);
gdalwrite.Close();

if (m_pBuffer)
{
delete m_pBuffer;
m_pBuffer = NULL;
}
******************************************************************************************************/
#pragma once
#include "stdafx.h"
#include "hdGeoRaster.h"
#include "BaseStruct.h"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "ogr_api.h"

// 屏蔽算术溢出等安全警告
#pragma warning(disable:4056)

namespace hd
{
	namespace scene
	{
	typedef enum 
	{
		GFT_TIFF = 0,
		GFT_JPG = 1,
		GFT_PNG =2,
		GFT_DEFALUT =3					// 默认类型，坐标即像素坐标
	}GDALFileType;

	class COverViewInfo;

	class HD3DSCENE_API ChdGdalIO
	{
	public:
		ChdGdalIO(void);
		~ChdGdalIO(void);

		// 打开图像,默认为读
		bool Open(const char* strFilePath,GDALRWFlag eFlags =GF_Read,int nWriteH =0,int nWriteW=0,int nBandNum=0,GDALDataType eType=GDT_Byte);

		// 获取数据类型
		GDALDataType GetDataType() const
		{
			return m_eDataType;
		}

		// 关闭图像
		void Close();

		// 获取影像的基本信息
		bool GetExtInfo(IMAGEEXTINFOPTR pImageExtInfo) const;

		// 获取影像的基本信息，返回指针，未得到，返回NULL；
		// @确定不再使用时，要手动释放该指针
		IMAGEEXTINFO* GetExtInfo() const;

		// 获取金字塔信息,没有返回false
		bool GetOverViewInfo(vector<OVERVIEWLEVEL*>& vectOVLevels) const;

		// 读取影像,添加缩放比例
		CHdGeoRaster* ReadImage(double dScale =1.0);

		// 根据范围读取影像,添加缩放比例
		CHdGeoRaster* ReadImage(const Chd2DBoundingBoxd& bRect,double dScale =1.0,bool bBand = false);

		// 写坐标信息
		bool WriteProj(double* dProj);

		// 写tfw文件
		bool WriteTfw(double* dProj);

	protected:
		//读取16UInt影像到Byte,添加缩放比例
		CHdGeoRaster* Read16UInttoByte(double dScale =1.0);

		//读取32位影像到Byte,添加缩放比例
		CHdGeoRaster* Read32FtoByte(double dScale =1.0);

		// 读取Byte数据,添加缩放比例
		CHdGeoRaster* ReadByte(double dScale =1.0);

	private:
		GDALDataset* m_pImageDS;			// 读取数据集指针
		string m_strFilePath;				// 设置文件路径
		GDALDataType m_eDataType;			// 设置数据类型
		GDALFileType m_eFileType;			// 设置文件类型

		static int m_sRegisterCount;		// 注册次数计数器
		// 模板方法
	public:
		// 读取数据,获取所有值,返数据指针，以及行列号，宽高值
		template<class T>
		T* ReadFullImage(int& nWidth,int& nHeight,int& nBandNum,double* dProj,T& dMax,T& dMin,double dScale =1.0)
		{
			// 准备读取
			if (NULL == m_pImageDS )
			{
				return NULL;
			}

			// 防止dScale过大或过小
			if (dScale<=0 && dScale>5)
			{
				dScale =1.0;
			}

			// 读取坐标信息
			m_pImageDS->GetGeoTransform(dProj);

			// 获取高度
			int nSrcWidth = m_pImageDS->GetRasterXSize();
			int nSrcHeight = m_pImageDS->GetRasterYSize();
			nBandNum = m_pImageDS->GetRasterCount();

			// 计算读取后的大小
			nHeight = int(nSrcHeight*dScale+0.5);
			nWidth = int(nSrcWidth*dScale+0.5);

			// 记录值的临时对象
			T* pfTemp = new T[nBandNum*nHeight*nWidth];

			if (sizeof(T)==1)
			{
				// 记录波段最大值最小值
				dMax = 0;
				dMin = 255;
			}
			else
			{
				// 记录波段最大值最小值
				dMax = FLT_MIN;
				dMin = FLT_MAX;
			}

			double dTmpScale = 1.0/dScale;
			// 波段数据
			for (int b = 1;b<=nBandNum;++b)
			{
				// 获取波段
				GDALRasterBand* pBand = m_pImageDS->GetRasterBand(b);

				// 通过波段读取
				if (pBand)
				{
					// 按行读取数据
					for (int i = 0;i < nHeight;++i)
					{
						// 一次读取一行数据
						T* pPixelBuf = new T[nWidth];
						pBand->RasterIO(GF_Read,0,i*dTmpScale,nSrcWidth,1,pPixelBuf,nWidth,1,m_eDataType,0,0);

						//读取数据
						for (int j=0;j<nWidth;++j)
						{
							if (pPixelBuf[j]>-9999.0)
							{
								//判断最大值
								if (pPixelBuf[j] >= dMax)
								{
									dMax = pPixelBuf[j];
								}

								// 判断最小值
								if (pPixelBuf[j] <= dMin)
								{
									dMin =pPixelBuf[j];	
								}
							}
							else
							{
								pPixelBuf[j]=0;
							}

							// 赋值
							pfTemp[(b-1)*nWidth*nHeight+(i*nWidth)+j] = pPixelBuf[j];
						}

						// 释放内存
						if (pPixelBuf)
						{
							delete []pPixelBuf;
							pPixelBuf = NULL;
						}
					}
				}
			}

			return pfTemp;	
		}

		// 读取指定行列的数据，然后读到指定高和宽的buffer中
		// 类似一种缩放读取，buffer由外部申请，可按像素块的方式读取或单独读取某一个波段
		// 与其他接口区别的地方，该nSrcRow，nSrcCol，nDstRow，nDstCol有可能超过图像的边界
		// 对超过边界的部分填充为黑色
		// 添加波段索引
		template<class T>
		bool ReadImage(T* pBuffer,int nSrcRow,int nSrcCol,int nDstRow,int nDstCol,int nHeight,int nWidth,int nBandIndex =-1,bool bBand = true)
		{
			if (!pBuffer || !m_pImageDS)
			{
				return false;
			}
			else
			{
				// 读取的高和宽
				int nReadHeight = nDstRow - nSrcRow;
				int nReadWidth = nDstCol - nSrcCol;

				// 原始影像高宽
				int nSrcHeight = m_pImageDS->GetRasterYSize();
				int nSrcWidth = m_pImageDS->GetRasterXSize();

				// 判断是否无效
				if (nSrcCol>=nSrcWidth
					|| nSrcRow>=nSrcHeight
					|| nDstCol<=0
					|| nDstRow<=0)
				{
					return false;
				}

				// 计算缩放率
				float fScaleX = (float)(1.0*nHeight/nReadHeight);
				float fScaleY = (float)(1.0*nWidth/nReadWidth);
				int nBandNum= m_pImageDS->GetRasterCount();

				// 定义读取行时的宽
				int nStartCol =0;
				int nReadCol=0;
				if (nSrcCol>=0 && nDstCol<=nSrcWidth)
				{
					nStartCol = nSrcCol;
					nReadCol = nDstCol - nSrcCol;
				}
				else if (nSrcCol>=0 && nDstCol>nSrcWidth)
				{
					nStartCol = nSrcCol;
					nReadCol = nSrcWidth - nSrcCol;
				}
				else if(nSrcCol<=0 && nDstCol<=nSrcWidth)
				{
					nStartCol = 0;
					nReadCol = nDstCol;
				}
				else if(nSrcCol<=0 && nDstCol>nSrcWidth)
				{
					nStartCol = 0;
					nReadCol = nSrcWidth;
				}

				// Height
				int nStartRow =0;
				int nReadRow=0;
				if (nSrcRow>=0 && nDstRow<=nSrcHeight)
				{
					nStartRow = nSrcRow;
					nReadRow = nDstRow - nSrcRow;
				}
				else if (nSrcRow>=0 && nDstRow>nSrcHeight)
				{
					nStartRow = nSrcRow;
					nReadRow = nSrcHeight - nSrcRow;
				}
				else if(nSrcRow<=0 && nDstRow<=nSrcHeight)
				{
					nStartRow = 0;
					nReadRow = nDstRow;
				}
				else if(nSrcRow<=0 && nDstRow>nSrcHeight)
				{
					nStartRow = 0;
					nReadRow = nSrcHeight;
				}
				// calc the start index
				int nScaleStartH(0),nScaleEndH(0);
				int nScaleStartW(0),nScaleEndW(0);

				// 在buffer中开始的行列索引
				int nBufStartH(0),nBufStartW(0);
				int nBufEndH(nHeight),nBufEndW(nWidth);

				nScaleStartH = (-nSrcRow>0)?(int(-nSrcRow*fScaleY+0.5)):(nSrcRow*fScaleY+0.5);
				if (nSrcRow<0)
				{
					nScaleEndH = (nDstRow>nSrcHeight)?((nSrcHeight-nSrcRow)*fScaleY+0.5):((nDstRow-nSrcRow)*fScaleY+0.5);
					nBufStartH = nScaleStartH;
				}
				else
				{
					nScaleEndH = (nDstRow>nSrcHeight)?((nSrcHeight)*fScaleY+0.5):((nDstRow)*fScaleY+0.5);
				}

				if (nDstRow>nSrcHeight)
				{
					nBufEndH = nScaleEndH;
				}

				nScaleStartW = (-nSrcCol>0)?(int(-nSrcCol*fScaleX+0.5)):(nSrcCol*fScaleX+0.5);
				if (nSrcCol<0)
				{
					nScaleEndW = (nDstCol>nSrcWidth)?((nSrcWidth-nSrcCol)*fScaleX+0.5):((nDstCol-nSrcCol)*fScaleX+0.5);
					nBufStartW = nScaleStartW;
				}
				else
				{
					nScaleEndW = (nDstCol>nSrcWidth)?((nSrcWidth)*fScaleX+0.5):((nDstCol)*fScaleX+0.5);
				}

				if (nDstCol>nSrcWidth)
				{
					nBufEndW = nSrcWidth;
				}

				int nTmpBufW = nScaleEndW - nScaleStartW;
				int nTmpBufH = nScaleEndH - nScaleStartH;
				// 先将范围内的数据读取出来，然后再缩放或拉伸
				// 临时buffer对象,缩放读取全图

				try
				{
					for (int b=1;b<=nBandNum;b++)
					{
						// 如果波段不等于-1，只读取某个波段
						if (nBandIndex!=-1 
							&& nBandIndex+1!=b)
						{
							continue;
						}

						// 获取波段
						GDALRasterBand* pBand = m_pImageDS->GetRasterBand(b);

						// 一次读取数据
						T* pPixelBuf = new T[nTmpBufW*nTmpBufH];
						memset(pPixelBuf,0,nTmpBufW*nTmpBufH);
						CPLErr pError = pBand->RasterIO(GF_Read,nStartCol,nStartRow,nReadCol,nReadRow,pPixelBuf,nTmpBufW,nTmpBufH,m_eDataType,0,0);

						// 判断是否读取成功
						if (pError != CE_None)
						{
							// 释放内存
							// delete buffer
							if (pPixelBuf)
							{
								delete []pPixelBuf;
								pPixelBuf = NULL;
							}
							return false;
						}

						// 遍历读取
						for (int nH=0;nH<nHeight;nH++)
						{
							if (nH<nBufStartH|| nH>nBufEndH)
							{
								continue;
							}

							for (int nw=0;nw<nWidth;nw++)
							{
								if (nw<nBufStartW || nw>=nBufEndW)
								{
									continue;
								}

								// 按波段赋值
								if (bBand)
								{
									if (nBandIndex==-1)
									{
										pBuffer[(b-1)*nWidth*nHeight+nH*nWidth+nw] = pPixelBuf[(nH-nBufStartH)*nTmpBufW+(nw-nBufStartW)];
									}
									else
									{
										pBuffer[nH*nWidth+nw] = pPixelBuf[(nH-nBufStartH)*nTmpBufW+(nw-nBufStartW)];
									}
								}
								// 按像素赋值
								else
								{
									// 赋值
									if (nBandIndex==-1)
									{
										pBuffer[nBandNum*(nH*nWidth+nw)+(nBandNum-b)] = pPixelBuf[(nH-nBufStartH)*nTmpBufW+(nw-nBufStartW)];
									}
									else
									{
										pBuffer[nH*nWidth+nw] = pPixelBuf[(nH-nBufStartH)*nTmpBufW+(nw-nBufStartW)];
									}
								}
							}
						}

						// delete buffer
						if (pPixelBuf)
						{
							delete []pPixelBuf;
							pPixelBuf = NULL;
						}
					}

					return true;
				}
				catch(...)
				{
					return false;
				}

				return false;
			}
		}

		// 读取指定行列的数据,像素坐标，参数bBand代表读取数据时是按波段读取还是按像素块读取
		// 如果按像素块读取，波段顺序与原始图像相反
		template<class T>
		T* ReadImage(int nSrcRow,int nSrcCol,int nDstRow,int nDstCol,int& nHeight,int& nWidth,int& nBandNum,double dScale =1.0,bool bBand=true)
		{
			// 判断是否合理区域
			if (NULL == m_pImageDS || GDT_Unknown == m_eDataType 
				|| nDstCol<0 || nDstRow<0
				|| nSrcRow>nDstRow || nSrcCol>nDstCol)
			{
				return NULL;
			}

			// 防止dScale过大或过小
			if (dScale<=0 && dScale>5)
			{
				dScale =1.0;
			}

			// 获取高度
			int nSrcWidth = m_pImageDS->GetRasterXSize();
			int nSrcHeight = m_pImageDS->GetRasterYSize();

			// 防止越界
			if (nSrcCol<0)
			{
				nSrcCol =0;
			}

			if (nSrcRow<0)
			{
				nSrcRow = 0;
			}

			// 防止越界
			if (nDstCol>nSrcWidth)
			{
				nDstCol = nSrcWidth;
			}

			if (nDstRow>nSrcHeight)
			{
				nDstRow = nSrcHeight;
			}

			// 判断是否超界
			if (nDstRow>nSrcHeight)
			{
				nDstRow = nSrcHeight;
			}

			if (nDstCol>nSrcWidth)
			{
				nDstCol = nSrcWidth;
			}

			// 计算读取后的大小
			nHeight = int((nDstRow-nSrcRow)*dScale+0.5);
			nWidth = int((nDstCol-nSrcCol)*dScale+0.5);

			nBandNum = m_pImageDS->GetRasterCount();
			// 记录值的临时对象
			T* pfTemp = new T[nBandNum*nHeight*nWidth];
			memset(pfTemp,0,nBandNum*nHeight*nWidth);
			double dTmpScale = 1.0/dScale;

			// 波段数据
			for (int b = 1;b<=nBandNum;++b)
			{
				// 获取波段
				GDALRasterBand* pBand = m_pImageDS->GetRasterBand(b);

				// 通过波段读取
				if (pBand)
				{
					// 按行读取数据
					for (int i = 0;i < nHeight;++i)
					{
						// 一次读取一行数据
						T* pPixelBuf = new T[nWidth];
						memset(pPixelBuf,0,nWidth);
						CPLErr pError = pBand->RasterIO(GF_Read,nSrcCol,nSrcRow+i*dTmpScale,nDstCol-nSrcCol,1,pPixelBuf,nWidth,1,m_eDataType,0,0);

						//读取数据
						for (int j=0;j<nWidth;++j)
						{
							// 按波段读取
							if (bBand)
							{
								// 赋值
								pfTemp[(b-1)*nWidth*nHeight+(i*nWidth)+j] = pPixelBuf[j];
							}

							else
							{
								// 赋值,波段要倒着读
								pfTemp[nBandNum*(i*nWidth+j)+(nBandNum-b)] = pPixelBuf[j];
							}
						}

						// 释放内存
						if (pPixelBuf)
						{
							delete []pPixelBuf;
							pPixelBuf = NULL;
						}
					}
				}
			}

			return pfTemp;	
		}

		// 读取指定地理范围的数据,像素坐标
		template<class T>
		T* ReadImage(const Chd2DBoundingBoxd& bRect,int& nHeight,int& nWidth,int& nBandNum,double& dLeft,double &dTop,double dScale = 1.0,bool bBand=true)
		{
			return ReadImageByWorld(bRect.GetMinX(),bRect.GetMaxX(),bRect.GetMinY(),bRect.GetMaxY(),nHeight,nWidth,nBandNum,dLeft,dTop,dScale,bBand);
		}

		// 读取指定地理范围的数据,像素坐标
		template<class T>
		T* ReadImageByWorld(double dMinx,double dMaxx,double dMiny,double dMaxy,int& nHeight,int& nWidth,int& nBandNum,double& dLeft,double &dTop,double dScale = 1.0,bool bBand=true)
		{
			// 判断是否合理区域
			if (NULL == m_pImageDS || GDT_Unknown == m_eDataType )
			{
				return NULL;
			}

			int nSrcRow(0),nSrcCol(0),nDstRow(0),nDstCol(0);

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

			// 根据范围，求行列值
			nSrcCol = int((dMinx-dProjTrans[0])/ dProjTrans[1]+0.5);
			nDstCol = int((dMaxx-dProjTrans[0])/ dProjTrans[1]+0.5);

			nSrcRow = int(-(dProjTrans[3]-dMaxy)/ dProjTrans[5]+0.5);
			nDstRow = int(-(dProjTrans[3]-dMiny)/ dProjTrans[5]+0.5);

			// 计算左上角的X坐标
			if (nSrcCol>=0)
			{
				dLeft = dProjTrans[0]+nSrcCol*dProjTrans[1];
			}
			else
			{
				dLeft = dProjTrans[0];
			}

			// 计算左上角的Y坐标
			if (nSrcRow>=0)
			{
				dTop = dProjTrans[3]+nSrcRow*dProjTrans[5];
			}
			else
			{
				dTop = dProjTrans[3];
			}

			return ReadImage<T>(nSrcRow,nSrcCol,nDstRow,nDstCol,nHeight,nWidth,nBandNum,dScale,bBand);
		}

		// 转换波段数据到块数据，由于GDAL读取的数据是按波段读取
		// 有时需要转换为按块，比如，b,g,r、
		// pDstBuffer的内存由外部申请，此处不负责申请内存
		template<class T>
		bool ConvertBandToBlock(T* pSrcBuffer,T* pDstBuffer,int nHeight,int nWidth,int nBandNum)
		{
			// 判断指针是否有效
			if (!pSrcBuffer || !pDstBuffer)
			{
				return false;
			}

			int nBufferSize = sizeof(T);

			try
			{
				// gdal读取是按波段读取，opencv是按b,g,r，所以需要转换
				// 将波段转换为b,g,r
				for (int nh = 0;nh < nHeight;nh++)
				{
					for(int nw =0;nw <nWidth;nw++)
					{
						for (int b=0;b<nBandNum;b++)
						{
							memcpy(pDstBuffer+nBandNum*(nh*nWidth+nw)+b,pSrcBuffer+(nBandNum-b-1)*nHeight*nWidth+nh*nWidth+nw,nBufferSize);
						}
					}
				}

				return true;
			}
			catch(...)
			{
				return false;
			}
			return false;
		}

		// 转换块数据到波段数据，由于GDAL读写的数据是按波段读取
		// 有时需要将b,g,r块存储的转为b,g,r
		// pDstBuffer的内存由外部申请，此处不负责申请内存
		template<class T>
		bool ConvertBlockToBand(T* pSrcBuffer,T* pDstBuffer,int nHeight,int nWidth,int nBandNum)
		{
			// 判断指针是否有效
			if (!pSrcBuffer || !pDstBuffer)
			{
				return false;
			}

			int nBufferSize = sizeof(T);

			try
			{
				// gdal读取是按波段读取，opencv是按b,g,r，所以需要转换
				// 将波段转换为b,g,r
				for (int nh = 0;nh < nHeight;nh++)
				{
					for(int nw =0;nw <nWidth;nw++)
					{
						for (int b=0;b<nBandNum;b++)
						{
							memcpy(pSrcBuffer+(nBandNum-b-1)*nHeight*nWidth+nh*nWidth+nw,pDstBuffer+nBandNum*(nh*nWidth+nw)+b,nBufferSize);
						}
					}
				}

				return true;
			}
			catch(...)
			{
				return false;
			}
			return false;
		}

		// 写影像,在指定行和列写入数据
		// @注意pData记录的数据为要写入的数据块的内存数据
		template<class T>
		bool WriteImage(T* pData,int nBandNum,int nSrcRow,int nSrcCol,int nDstRow,int nDstCol,double* dProj = NULL,bool bBand=true,int nBandIndex = -1)
		{
			if (!m_pImageDS)
			{
				return false;
			}

			// 如果坐标信息不为空
			if (dProj && m_pImageDS)
			{
				const char *pszSRS_WKT = m_pImageDS->GetProjectionRef();
				m_pImageDS->SetProjection(pszSRS_WKT);

				// 设置坐标信息
				m_pImageDS->SetGeoTransform(dProj);
			}

			if (pData)
			{
				int nWidth = nDstCol - nSrcCol;
				int nHeight = nDstRow - nSrcRow;

				if (nBandIndex==-1)
				{
					// 按像素写
					if (!bBand)
					{
						for (int b=0;b<nBandNum;b++)
						{
							// 按波段的顺序写，该类的读写接口都是这个顺序，所以可以直接按行按列写
							// 获取波段
							GDALRasterBand* pBand = m_pImageDS->GetRasterBand(b+1);

							if (pBand)
							{
								for (int i =0;i< nHeight;++i)
								{
									for (int j=0;j<nWidth;++j)
									{
										// 重写数据,按像素写
										pBand->RasterIO(GF_Write,nSrcCol+j,nSrcRow+i,1,1,pData+nBandNum*(i*nWidth+j)+nBandNum-b-1,1,1,m_eDataType,0,0);
									}
								}	
							}
						}
					}
					else
					{
						for (int b=0;b<nBandNum;b++)
						{
							// 按波段的顺序写，该类的读写接口都是这个顺序，所以可以直接按行按列写
							// 获取波段
							GDALRasterBand* pBand = m_pImageDS->GetRasterBand(b+1);

							if (pBand)
							{
								// 重写数据,按波段写
								pBand->RasterIO(GF_Write,nSrcCol,nSrcRow,nWidth,nHeight,pData+b*nWidth*nHeight,nWidth,nHeight,m_eDataType,0,0);
							}
						}
					}
				}
				// 只写某一个波段
				else
				{
					if (nBandIndex+1<=nBandNum)
					{
						GDALRasterBand* pBand = m_pImageDS->GetRasterBand(nBandIndex+1);
						// 重写数据,按波段写
						CPLErr pError = pBand->RasterIO(GF_Write,nSrcCol,nSrcRow,nWidth,nHeight,pData,nWidth,nHeight,m_eDataType,0,0);
						if (pError!=CE_None)
						{
							return false;
						}
					}
					else
					{
						return false;
					}
				}

				return true;
			}

			return false;
		}

		// 转换像素块，从左上转成左下
		template<class T>
		bool ConvertPixelLTtoLB(T* pBuffer,int nBand,int nHeight,int nWidth)
		{
			if (!pBuffer)
			{
				return false;
			}

			int nPP = sizeof(T);

			// 先定义一个临时的内存
			T* pTmpBuf = new T[nBand*nHeight*nWidth];
			memcpy(pTmpBuf,pBuffer,nBand*nHeight*nWidth*nPP);

			// 直接一行转换
			for (int i=0;i<nHeight;i++)
			{
				memcpy(pBuffer+i*nWidth*nBand,pTmpBuf+(nHeight-i-1)*nWidth*nBand,nBand*nPP*nWidth);
			}

			// 删除内存
			if (pTmpBuf)
			{
				delete []pTmpBuf;
				pTmpBuf = NULL;
			}

			return true;
		}

		// 转换像素块，从左下转成左上
		template<class T>
		bool ConvertPixelLBtoLT(T* pBuffer,int nBand,int nHeight,int nWidth)
		{
			if (!pBuffer)
			{
				return false;
			}

			int nPP = sizeof(T);

			// 先定义一个临时的内存
			T* pTmpBuf = new T[nBand*nHeight*nWidth];
			memcpy(pTmpBuf,pBuffer,nBand*nHeight*nWidth*nPP);

			// 直接一行转换
			for (int i=0;i<nHeight;i++)
			{
				memcpy(pBuffer+(nHeight-1-i)*nWidth*nBand,pTmpBuf+i*nWidth*nBand,nBand*nPP*nWidth);
			}

			// 删除内存
			if (pTmpBuf)
			{
				delete []pTmpBuf;
				pTmpBuf = NULL;
			}
			return true;
		}
	};
	}
}
