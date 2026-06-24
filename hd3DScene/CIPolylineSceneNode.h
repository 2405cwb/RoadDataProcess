/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CIPolylineSceneNode.h
相关文件     : CIPolylineSceneNode.cpp, 
文件实现功能 : 实现三维多线段的绘制 
			   与Iobject对象无关 其内部没有封装IObject对象
作者         : 危迟
版本         : 软件部，危迟
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/08/20	 1.0	  危迟					创建
2013/08/21   1.1      危迟				添加模型转换计算相关方法
2013/08/29   1.2     朱旭波            根据系统设置颜色修改显示的颜色（非选中）
2013/08/30   1.3      危迟             添加获取是否三维、是否闭合、是否需要转换接口函数
</PRE>
*******************************************************************************/
#pragma once
#include "..\hd3DEngine\include\irrlicht.h"
#include "IObjectSceneNode.h"

using namespace irr;
using namespace hd;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CIPolylineSceneNode : public IObjectSceneNode
		{
		public:
			CIPolylineSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CIPolylineSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_IPOLYLINE; }

			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

			//*****************************自身对象接口***************************************//
			// 更新包围盒
			void UpdateBoundingBox();

			// 设置闭合
			void SetCloseure(bool bClosure) { m_bClosure = bClosure; }

			// 设置是否三维
			void Set3DLine(bool b3D) { m_b3D = b3D; }

			// 获取是否三维线段
			bool Is3DLine() { return m_b3D; }

			// 设置是否需要转换
			void SetTransCoordinate(bool bTrans) { m_bTrans = bTrans; }

			// 获取是否需要转换
			bool IsTransCoordinate() { return m_bTrans;}

			// 获取是否闭合
			bool IsCloseure() { return m_bClosure; }

			// 添加结点
			void AddCoordinate(core::vector3df coord);

			// 结点总数
			int GetVertexCount() { return m_vecCoords.size();}

			// 获取结点坐标
			bool GetVertexCoord(int i,core::vector3df& coord);

			// 清除所有结点
			void ClearAllVertex();
			
			// 设置线的颜色与系统设置中线的颜色保持一致
			void SetSameColor(bool bSame) { m_bSysColor = bSame; }
		private:
			// 坐标序列
			core::array<core::vector3df> m_vecCoords;    
			// 材质
			video::SMaterial		m_Material;
			// 包围盒
			core::aabbox3d<f32>		m_BBox;
			// 是否闭合
			bool					m_bClosure;
			// 是否三维
			bool					m_b3D;
			// 是否需要转换
			bool					m_bTrans;
			// 是否与系统设置中线颜色保持一致
			bool					m_bSysColor;

		};
	}
}


