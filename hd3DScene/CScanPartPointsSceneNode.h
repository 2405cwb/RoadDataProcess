/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CScanPartPointsSceneNode.h
相关文件     : CScanPartPointsSceneNode.cpp
文件实现功能 :  显示从扫描站中获取的部分点云
作者         : 张飞
版本         : 软件部，张飞
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/03/04	 1.0	 张飞			   创建
2014/09/16	 1.0	 蔡红云		       移植修改
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "IObjectSceneNode.h"
#include "..\hd3DEngine\include\ESceneNodeTypes.h"
#include "..\hdCore\Hd3dBoxEx.h"
#include "CScanSceneNode.h"
#include "..\hd3DEngine\COpenGLExtensionHandler.h"
#include <vector>
#include "..\hdCommon\hnPoint3d.h"
#include "..\hnCommon\hnPointXYZIDef.h"

using namespace hn;
using namespace std;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CScanPartPointsSceneNode : public IObjectSceneNode
		{
		public:
			CScanPartPointsSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CScanPartPointsSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_PART_SCAN_POINT; }

			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

			virtual void RenderPoint();

			//*****************************自身对象接口***************************************//
			// 更新包围盒
			void UpdateBoundingBox();

			// 更新包围盒
			void UpdateBoundingBox(vector<hnCommon::hnPointXYZIF>& pPartPts,bool isFirst = false);

			// 设置显示部分点云，本接口直接更换点云数据，接口已修改;
			bool SetPartPoints(vector<hnCommon::hnPointXYZIF>& pPartPts, bool bIntial);

			// 获得渲染点云，外部调用
			vector<hnCommon::hnPointXYZIF>& GetPartPoints(){return m_pPartPcd;}

			// 点的数量
			int GetPtsCount(){return m_pPartPcd.size();}

			// 设置box的显示状态
			void SetRndrBox(bool flag ) { m_brdrBox = flag;}

			//设置点云大小
			void SetPointSize(float nSize) { m_Material.Thickness = nSize;}

        public:
            void SetAix(int m_nAisStyle){ m_nAxis = m_nAisStyle;}
            void SetCycleStep(float fStep){ m_fStep = fStep;}

		private:

			//! 点云数据
			vector<hnCommon::hnPointXYZIF> m_pPartPcd;

			// 材质
			video::SMaterial		m_Material;
			// 包围盒
			core::aabbox3d<f32>		m_BBox;

			// 颜色条步长
			float m_fStep;

			//! 设置按Z渲染的最大最小值
			float m_fMinHeight;
			float m_fMaxHeight;

			//! 高度分布,每个高度范围点云个数相同
			vector<float>  m_heightStep;

			//! 反射强度渲染统计
			int m_nMinIntensity;
			int m_nMaxIntensity;

			//! 渲染颜色条
			CHdColorRamp   m_colorRamp;

			//! 点云符号化显示方式
			ENUM_RENDERSTYLE m_renderStyle;

			// 按类别进行渲染时。存储类别颜色
			COLORREF m_sClassColor[256];

			// 是否渲染包围盒
			bool m_brdrBox; 

		private:

			// 是否显示透明度
			BOOL      m_bShowIntenRender;

            // 色带循环方向，Z--0，X--1，Y--2
			int	  m_nAxis;

            // 循环色带渲染时色带条
			CHdColorRamp   m_colorRampCycle;

			// 代码锁
			CRITICAL_SECTION m_cs;

			// 顶点坐标
			GLfloat* m_vertices;   

			// 顶点颜色
			GLfloat* m_verColors; 

			// 点个数
			int m_nPtNum;

			// 已添加点数,未达到300W点之前，数据应记录，便于更新范围;
			int m_nAllInNum;

			// 记录当前已添加点的位置;
			int m_nCurPosition;

			// 渲染方式
			int m_rendStyle;

			// 申请内存大小
			int m_nMemery;

		};
	}
}	