/*! HdImage.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdImage.h
相关文件     : 
文件实现功能 : 定义图像类
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
#ifndef HD3DSCENE_HDIMAGE_H
#define HD3DSCENE_HDIMAGE_H

#include "stdafx.h"

namespace hd
{
	namespace scene
	{
		// 颜色格式
		enum EHdColorFormat
		{
			EHD_CF_R8G8B8 = 0,
			EHD_CF_A8R8G8B8
		};

		// 一个简单的图像类
		class HD3DSCENE_API CHdImage
		{
		public:
			CHdImage();
			CHdImage(unsigned int nWidth, unsigned int nHeight, EHdColorFormat eColorFormat);
			CHdImage(unsigned int nWidth, unsigned int nHeight, EHdColorFormat eColorFormat, unsigned char* pData, bool bOwnData);
			virtual~ CHdImage();

		public:
			CHdImage(const CHdImage&);
			CHdImage& operator= (const CHdImage&);

		public:
			unsigned int GetWidth() const;
			unsigned int GetHeight() const;
			EHdColorFormat GetColorFormat() const;
			unsigned char* GetImageData() const;
			unsigned char* GetScanLineData(unsigned int nScanLine) const;
			unsigned int GetPixelSpace() const;
			unsigned int GetLineSpace() const;

		private:
			unsigned int m_nWidth;
			unsigned int m_nHeight;
			unsigned char* m_pData;
			EHdColorFormat m_eColorFormat;
			bool m_bOwnData;
		};

	}
}

#endif