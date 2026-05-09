/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CRuotePointSceneNode.h
相关文件     : CRuotePointSceneNode.cpp
文件实现功能 : 实现轨迹点的显示
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/04/03   1.0      危迟               新增加内容
2013/09/05   1.1      危迟				 添加是否绘制索引号接口
</PRE>
*******************************************************************************/
#pragma once
#include "..\..\hd3DScene\IObjectSceneNode.h"
#include "..\..\hdCommon\HDRoutePoint.h"

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CRoutePointSceneNode : public IObjectSceneNode
		{
		public:
			CRoutePointSceneNode(CHdSxPoint3D point, int index,ISceneNode* parent, ISceneManager* mgr, s32 id,bool bVaRoute = false,float fRadius = 0.5f);
			~CRoutePointSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_ROUTEPOINT; }

			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

		public:

			//*********************************自身对象接口********************************//
			//void SetMesh(IMesh* mesh) { m_Mesh = mesh; }
			// 获取网格
			IMesh* GetMesh(int i) { return m_Mesh; }

			// 获取索引号
			int GetIndex() { return m_nIndex; }

			// 设置是否绘制索引号
			void SetDrawID(bool bDraw) { m_bDrawID = bDraw; }

            // 得到是否绘制索引号
            bool GetDrawID() { return m_bDrawID; }

			// 获取是否是va工程索引，后续处理
			bool isVaRoute(){return m_bVaRoute;}

			// 设置dom比例尺
			void SetDomScale(hd::s32 domScale) {m_domScale = domScale;}

			// 获得dom比例尺
			hd::s32 GetDomScale() {return m_domScale;}

			// 获取所在工程名
			string GetPrjName(){return m_strPrjName;}

			// 设置所在工程名
			void SetPrjName(const string& strname){m_strPrjName = strname;}

		    // 重置包围盒 
			void RecalculateBBox();

		private:
			// 轨迹点列表
			int			m_nIndex;
			// mesh
			IMesh*			m_Mesh;
			// 外接盒
			core::aabbox3d<f32>		m_BBox;

			// 是否绘制索引号
			bool m_bDrawID;

			// 标记是否是va工程，默认为false表示是iScan工程
			bool m_bVaRoute;

			// va工程记录该tfw对于dom的比例尺
			hd::s32 m_domScale;

			// 一个视图内共存多个工程时，对轨迹球所在工程进行标记
			string m_strPrjName;

			// 点坐标
			CHdSxPoint3D m_point;
		};
	}
}


