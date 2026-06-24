/*! HdRasterRenderer.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdRasterRenderer.h
相关文件     : 
文件实现功能 : 将不同颜色格式和数据类型的栅格数据渲染到位图中
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
#ifndef HD3DSCENE_HDRASTERRENDERER_H
#define HD3DSCENE_HDRASTERRENDERER_H

namespace hd
{
	namespace scene
	{
		class CHdImage;
		class CHdRasterBlock;
		class CHdColorTable;
		/*
		enum EHdRasterRendererType
		{
			EHD_RRT_RGB,
			EHD_RRT_Stretch,
			EHD_RRT_ColorTable
		};
		*/
		// 栅格数据渲染类
		class HD3DSCENE_API CHdRasterRenderer
		{
		public:
			CHdRasterRenderer();
			virtual~ CHdRasterRenderer();

			//virtual EHdRasterRendererType GetRendererType() const;

			//virtual bool Render(const CHdRasterBlock* pRaster, CHdImage* pImage);

			// 以 RGB 三个波段合成方式来绘制
			// 默认 pRaster 中的波段顺序为 BGR 或 BGRA，与图像颜色排列顺序一致
			// 如果栅格数据类型为 unsigned char，直接复制到图像中，否则需要根据最小最大值进行拉伸
			bool RenderByRGB(const CHdRasterBlock* pRaster, 
				             double* pdfMinValue, double* pdfMaxValue, 
				             CHdImage* pImage, 
							 int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize);

			// 对单波段拉伸绘制
			bool RenderByStretch(const CHdRasterBlock* pRaster, 
				                 double dfMinValue, double dfMaxValue, 
								 CHdImage* pImage,
								 int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize);

			// 根据颜色表来绘制
			bool RenderByColorMap(const CHdRasterBlock* pRaster, 
				                  CHdColorTable* pColorTable, 
								  CHdImage* pImage,
								  int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize);
		};
		/*
		class HD3DSCENE_API CHdRasterRGBRenderer: public CHdRasterRenderer
		{
		public:
			CHdRasterRGBRenderer();
			virtual~ CHdRasterRGBRenderer();
		};

		class HD3DSCENE_API CHdRasterStretchRenderer: public CHdRasterRenderer
		{
		public:
			CHdRasterStretchRenderer();
			virtual~ CHdRasterStretchRenderer();
		};

		class HD3DSCENE_API CHdRasterColorMapRenderer: public CHdRasterRenderer
		{
		public:
			CHdRasterColorMapRenderer();
			virtual~ CHdRasterColorMapRenderer();
		};
		*/
	}
}

#endif