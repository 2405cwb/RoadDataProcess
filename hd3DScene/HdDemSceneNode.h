/*! HdDemSceneNode.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdDemSceneNode.h
相关文件     : 
文件实现功能 : 在三维场景中绘制栅格形式的 DEM 数据
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
#ifndef HD3DSCENE_HDDEMSCENENODE_H
#define HD3DSCENE_HDDEMSCENENODE_H

#include "IObjectSceneNode.h"

namespace hd
{
	class CHdColorRamp;

	namespace scene
	{
		class CHdRasterDataset;
		class CHdRasterBlock;

		// DEM 节点类
		class HD3DSCENE_API CHdDemSceneNode: public IObjectSceneNode
		{
		public:
			CHdDemSceneNode(ISceneNode* parent, ISceneManager* mgr, irr::s32 id, CHdRasterDataset* pDataset);
			virtual~ CHdDemSceneNode();
			
			//****************************************************************************************//
			//                                    ISceneNode接口                                      //
			//****************************************************************************************//
			virtual void OnRegisterSceneNode();
			virtual void render();
			virtual const core::aabbox3d<irr::f32>& getBoundingBox() const;
			virtual video::SMaterial& getMaterial(irr::u32 i);
			virtual u32 getMaterialCount() const;
			virtual ESCENE_NODE_TYPE getType() const;
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);
			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);
			//****************************************************************************************//

			//! 获取 DEM 数据集
			CHdRasterDataset* GetDemDataset();

			//! 根据视口重新加载数据
			bool ReloadData();

			//! 设置 DEM 的基础高程
			void SetBaseHeight(double dfBaseZ);

			//! 视图的变换模型发生变化时，更新节点的变换模型
			void UpdateTransModel();

			//! 设置渲染 DEM 使用的色带（无有效纹理时使用）

			//! 设置 DEM 的纹理数据集
			void SetTextureDataset(CHdRasterDataset* pDataset);

			//! 获取 DEM 的纹理数据集
			CHdRasterDataset* GetTextureDataset();

			//! 设置纹理大小
			void SetTextureSize(int nSize);

		private:

			//! 根据当前相机位置，计算待读取数据的地理范围（影像的局部坐标系，以影像左下角为原点）
			bool CalcDataEnv(float& fLoadMinX, float& fLoadMaxX, float& fLoadMinY, float& fLoadMaxY);

			//! 根据待读取数据的地理范围，计算出实际读取的金字塔层、像素范围、目标栅格大小，以及地理范围
			bool CalcLoadPixels(CHdRasterDataset* pDataset,       // 待读取的数据集
				                float fLoadMinX,                  // 待读取的地理范围（数据集的局部坐标系）
				                float fLoadMaxX, 
				                float fLoadMinY, 
				                float fLoadMaxY, 
				                int nMaxXSize,                    // 目标栅格大小的上限
				                int nMaxYSize,
				                int& nOvIndex,                    // 实际读取的金字塔层索引
				                int& nSrcXOff,                    // 金字塔的读取像素范围
				                int& nSrcYOff, 
				                int& nSrcXSize, 
				                int& nSrcYSize,
				                int& nDstXSize,                   // 目标栅格的实际大小
				                int& nDstYSize, 
				                float& fRealMinX,                 // 目标栅格的实际地理范围
				                float& fRealMaxX, 
				                float& fRealMinY, 
				                float& fRealMaxY);

			//! 根据顶点的高程值对顶点进行着色
			bool RenderVertex(SMeshBuffer* pMeshBuffer);

			//! 从一个 DEM 栅格中获取指定像素的高程值
			float GetElevation(const CHdRasterBlock& raster, int nPixelX, int nPixelY);

			//! 根据地理范围（DEM 的局部坐标系）重新生成纹理，返回该地理范围对应的纹理坐标范围
			bool UpdateTexture(float fGeoMinX, float fGeoMaxX, float fGeoMinY, float fGeoMaxY, 
				               float& fTMinX, float& fTMaxX, float& fTMinY, float& fTMaxY);

			//! 判断纹理数据集的绘制方式
			bool InitTextureRenderType();
			
		private:

			CHdRasterDataset* m_pDemDataset;               // DEM 数据集

			double m_dfBaseZ;                              // DEM 的基础高程（栅格值加上基础高程等于实际高程）

			CHdRasterDataset* m_pTextureDataset;           // DEM 关联的纹理数据集

			int m_nTextureSize;                            // 纹理图像的大小（长宽相等）

			CHdColorRamp* m_pColorRamp;                    // DEM 按高程渲染时使用的色带          

			SMesh* m_pMesh;                                // 网格对象，用于显示 DEM

			int m_nMeshBufferSize;                         // 网格分块大小（每行、每列的顶点数），分块的目的是便于渲染引擎利用
			                                               // 网格块的包围盒快速过滤掉不在视口范围内的网格块
			 
			int m_nMeshBufferRows;                         // 网格块的行（列）数

			float m_fLoadMinX, m_fLoadMaxX;                // 当前加载的 DEM 区域的地理范围（局部坐标系）
			float m_fLoadMinY, m_fLoadMaxY;  

			float m_fMinZ, m_fMaxZ;                        // DEM 的高程范围（不包含基础高程）

			float m_fNoDataZ;                              // 无效高程值

			int m_nRenderType;                             // 栅格数据渲染方式（1-RGB，2-拉伸，3-颜色表）
			int m_nRenderBandCount;                        // 绘制栅格数据时使用的波段数和波段索引
			int m_pRenderBandIndex[4];   
			double m_dfMinValue, m_dfMaxValue;             // 拉伸使用的最小最大值
		};
	}
}

#endif