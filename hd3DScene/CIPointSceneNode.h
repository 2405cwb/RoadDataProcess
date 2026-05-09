/*! @file
********************************************************************************
<PRE>
模块名       : HD3DScene
文件名       : CIPointSceneNode.h
相关文件     : CIPointSceneNode.cpp
文件实现功能 : 实现三维点的绘制，绘制至CAD点位置，当用户移动操作后，CPointSceneNode
               点移除，通过本sn标记显示用户绘制位置，随视图关闭析构
			   与Iobject对象无关 其内部没有封装IObject对象
作者         : 朱旭波
版本         : 软件部，朱旭波
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/08/22	 1.0	  朱旭波			   创建
2013/08/29   1.1      朱旭波			   删除模型计算方法，外部进行模型转换，
                                           根据系统设置显示绘制点颜色
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
		class HD3DSCENE_API CIPointSceneNode : public IObjectSceneNode
		{
		public:
			CIPointSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
			virtual ~CIPointSceneNode(void);

			//*************************ISceneNode 基类接口*******************************//

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual video::SMaterial& getMaterial(u32 i);

			virtual u32 getMaterialCount() const;

			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			virtual ESCENE_NODE_TYPE getType() const { return ESNT_IPOINT; }

			virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

			//*****************************自身对象接口***************************************//
			// 更新包围盒
			void UpdateBoundingBox();

			// 添加结点
			void AddCoordinate(core::vector3df coord);

			// 结点总数
			int GetVertexCount() { return m_vecCoords.size();}

			// 获取结点坐标
			bool GetVertexCoord(int i,core::vector3df& coord);

			// 清除所有结点
			void ClearAllVertex();

		private:
			// 坐标序列
			core::array<core::vector3df> m_vecCoords;    
			// 材质
			video::SMaterial		m_Material;
			// 包围盒
			core::aabbox3d<f32>		m_BBox;
		};
	}
}

