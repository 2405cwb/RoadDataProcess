/*! HdDomSceneNode.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdDomSceneNode.h
相关文件     : 
文件实现功能 : 在三维场景中绘制影像
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
#ifndef HD3DSCENE_HDDOMSCENENODE_H
#define HD3DSCENE_HDDOMSCENENODE_H

#include "IObjectSceneNode.h"

namespace hd
{
	namespace scene
	{
		class CHdRasterDataset;
		class SMessBuffer;

		//! 影像节点类
		class HD3DSCENE_API CHdDomSceneNode: public IObjectSceneNode
		{
		public:
			CHdDomSceneNode(ISceneNode* parent, ISceneManager* mgr, irr::s32 id, CHdRasterDataset* pDataset);
			virtual~ CHdDomSceneNode();

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

			//! 获取影像数据集
			CHdRasterDataset* GetDataset();

			//! 根据视口重新加载数据
			bool ReloadData();

			//! 设置影像的基础高程
			void SetBaseHeight(double dfBaseZ);

			//! 设置纹理大小，纹理越大，影像越清晰，但内存占用也越大
			void SetTextureSize(unsigned int nSize);
			
			//! 视图的变换模型发生变化时，更新节点的变换模型
			void UpdateTransModel();

		private:

			//! 初始化栅格渲染类型
			bool InitRenderType();

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

		private:

			CHdRasterDataset* m_pDataset;                  // 影像数据集

			float m_dfBaseZ;                               // 影像基础高程

			SMeshBuffer *m_pMeshBuffer;                    // 用于显示影像的网格

			int m_nTextureSize;                            // 影像纹理的大小

			float m_fLoadMinX, m_fLoadMaxX;                // 当前纹理的地理范围（局部坐标系）
			float m_fLoadMinY, m_fLoadMaxY;  

			int m_nRenderType;                             // 栅格数据渲染方式（1-RGB，2-拉伸，3-颜色表）
			int m_nRenderBandCount;                        // 绘制栅格数据时使用的波段数和波段索引
			int m_pRenderBandIndex[4];            
			double m_dfMinValue, m_dfMaxValue;             // 拉伸使用的最小最大值
		};
	}
}

#endif