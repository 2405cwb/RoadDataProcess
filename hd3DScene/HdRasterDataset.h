/*! HdRasterDataset.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdRasterDataset.h
相关文件     : 
文件实现功能 : 实现栅格数据集类，主要包括读写栅格文件的地理信息和栅格数据
作者         : 朱立雄
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/10/17   1.0      朱立雄                创建
</PRE>
*******************************************************************************/
#ifndef HD3DSCENE_HDRASTERDATASET_H
#define HD3DSCENE_HDRASTERDATASET_H

#include "stdafx.h"
#include "HdGeoDataset.h"
#include <vector>

class GDALDataset;

namespace hd
{
	namespace scene
	{
		class CHdRasterBlock;
		class CHdColorTable;

		// 栅格数据类型
		enum EHdRasterDataType
		{
			EHD_DT_Unknown = 0,
			EHD_DT_Byte,
			EHD_DT_UInt16,
			EHD_DT_Int16,
			EHD_DT_UInt32,
			EHD_DT_Int32,
			EHD_DT_Float32,
			EHD_DT_Float64
		};

		// 波段的颜色类型
		enum EHdColorInterp
		{
			EHD_CI_Unknown = 0,
			EHD_CI_Gray,
			EHD_CI_PaletteIndex,
			EHD_CI_Red,
			EHD_CI_Green,
			EHD_CI_Blue,
			EHD_CI_Alpha
		};

		// 栅格插值方式
		enum EHdResampleType
		{
			EHD_RT_NearestNeighbour = 0,
			EHD_RT_Bilinear,
			EHD_RT_Cubic
		};

		//! 定义一个进度回调函数指针（与 GDAL 的进度函数保持一致）
		typedef int (__stdcall *HdProgressFunc)(double dfComplete, const char *pszMessage, void *pProgressArg);

		// 栅格数据集
		class HD3DSCENE_API CHdRasterDataset: public CHdGeoDataset
		{
		public:
			CHdRasterDataset();
			virtual~ CHdRasterDataset();
		private:
			CHdRasterDataset(const CHdRasterDataset&);
			CHdRasterDataset& operator= (const CHdRasterDataset&);

		public:
			virtual const CHdSpatialReference* GetSpatialReference() const;
			virtual const CHdGeoEnvelope* GetGeoEnvelope() const;

			static CHdRasterDataset* Open(const char* pcFileName, bool bOnlyRead = true);
			static void Close(CHdRasterDataset* pHdDataset);

			const char* GetDatasetFullName() const;

			int Reference();
			int Dereference();

			unsigned int GetRasterWidth() const;
			unsigned int GetRasterHeight() const;
			unsigned int GetBandCount() const;
			EHdRasterDataType GetBandDataType(int nBand) const;
			EHdColorInterp GetBandColorInterp(int nBand) const; 
			bool GetNoDataValue(int nBand, double& dfNoDataValue) const;
			bool GetMinMaxValue(int nBand, double& dfMinValue, double& dfMaxValue) const;
			CHdColorTable* GetColorTable() const;

			int GetOverviewCount(int nBand) const;
			bool GetOverviewSize(int nBand, int nOverviewIndex, int& nWidth, int& nHeight);
			bool BuildOverview(HdProgressFunc pProgressFunc = NULL);

			void GetGeoTransform(double* pdfTransform) const;
			void SetGeoTransform(double* pdfTransform);

			void PixelToGeo(double& dfX, double& dfY);
			void GeoToPixel(double& dfX, double& dfY);

			bool Read(int nXOff, int nYOff, int nXSize, int nYSize, int nBandCount, int* pBandIndex, CHdRasterBlock* pRaster) const;

			bool Read(int nOverviewIndex, int nXOff, int nYOff, int nBandCount, int* pBandIndex, CHdRasterBlock* pRaster) const;

			static int GetDataTypeSize(EHdRasterDataType eDataType);

		private:
			GDALDataset* m_pGDALDataset;
			std::string m_strFullName;
			CHdSpatialReference* m_pSR;
			CHdGeoEnvelope* m_pGeoEnv;
			int m_nReferenceCount;
		};

		// 内存中的栅格数据块，以波段为主序
		class HD3DSCENE_API CHdRasterBlock
		{
		public:
			CHdRasterBlock();
			CHdRasterBlock(unsigned int nWidth, unsigned int nHeight, unsigned int nBandCount, EHdRasterDataType eDataType);
			virtual~ CHdRasterBlock();
		private:
			CHdRasterBlock(const CHdRasterBlock&);
			CHdRasterBlock& operator= (const CHdRasterBlock&);

		public:
			unsigned int GetRasterWidth() const;
			unsigned int GetRasterHeight() const;
			unsigned int GetBandCount() const;
			EHdRasterDataType GetDataType() const;
			EHdColorInterp GetBandColorInterp(int nBand) const; 
			double GetNoDataValue() const;
			bool GetMinMaxValue(int nBand, double& dfMinValue, double& dfMaxValue) const;
			unsigned char* GetRasterData() const;

			bool Create(unsigned int nWidth, unsigned int nHeight, unsigned int nBandCount, EHdRasterDataType eDataType);
			void SetBandColorInterp(int nBand, EHdColorInterp eColorInterp);

		private:
			unsigned int m_nWidth;
			unsigned int m_nHeight;
			unsigned int m_nBandCount;
			EHdRasterDataType m_eDataType;
			std::vector< EHdColorInterp > m_vColorInterp;
	        double m_dfNoDataValue;
			unsigned char* m_pData;
		};

		// 颜色表
		class HD3DSCENE_API CHdColorTable
		{
		public:
			CHdColorTable();
			CHdColorTable(unsigned int nColorCount);
			virtual~ CHdColorTable();

		private:
			CHdColorTable(const CHdColorTable&);
			CHdColorTable& operator= (const CHdColorTable&);

		public:

			void CreateTable(unsigned int nColorCount);

			void SetColor(unsigned int nIndex, unsigned char a, unsigned char r, unsigned char g, unsigned char b);
			void GetColor(unsigned int nIndex, unsigned char& a, unsigned char& r, unsigned char& g, unsigned char& b) const;

		private:
			unsigned int m_nColorCount;
			unsigned char* m_pColorArray;
		};
		/*
		// 仿射变换
		class HD3DSCENE_API CHdAffineTransform
		{
		public:
			CHdAffineTransform();
			CHdAffineTransform(double* pdfParam);
			virtual~ CHdAffineTransform();

			void SetTransformParameter(double* pdfParam);
			void GetTransformParameter(double* pdfParam);

			void Transform(double& x, double& y);
			void AntiTransform(double& x, double& y);

		private:
			double pdfParam[6];
		};
		*/
	}
}
#endif