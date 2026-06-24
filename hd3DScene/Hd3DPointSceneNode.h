/*! Hd3DPointSceneNode.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : Hd3DPointSceneNode.h
相关文件     : Hd3DPointSceneNode.cpp
文件实现功能 : 3D视图中点场景结点显示，不实时生成点节点，采用移动点的方式，防止
			  频繁实时生成释放点导致程序崩溃	   
作者         : 研发部 冯晶
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/04/29    1.0     冯晶			   创建并实现三维视图点场景结点显示
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "..\hdCommon\sceneData\HdSxPoint3D.h"
#include "stdafx.h"
using namespace hd;
using namespace fm;

//点显示的类型
enum EHD_POINT_STYLE
{
	EPS_CROSS		= 0,	//显示十字
	EPS_POINT,				//显示点
	EPS_CIRCLE,				//显示一个填充的圆形
	EPS_SQUARE,				//显示填充的正方形
	EPS_CROSSANDCIRCLE,		//显示十字和圆形
	EPS_COUNT
};

namespace hd
{
	namespace scene
	{

		class HD3DSCENE_API CHdPointSceneNode3D :
			public IObjectSceneNode
		{
		public:
			// 构造
			CHdPointSceneNode3D(CHdSxPoint3D* point, EHD_POINT_STYLE style, ISceneNode* parent, ISceneManager* mgr, int id);

			// 析构
			~CHdPointSceneNode3D();

			// 预处理
			virtual void OnRegisterSceneNode();

			// 渲染
			virtual void render();

			// 获取包围盒
			virtual const core::aabbox3d<float>& getBoundingBox() const;

			// 由于CPolyline对象会在外部被更新，所以需要有接口去更新SceneNode的范围
			void UpdateBoundingBox();

			// 设置显示样式
			void setStyle(EHD_POINT_STYLE style) { m_Style = style; }

			// 得到显示样式
			EHD_POINT_STYLE getStyle() { return m_Style; }

			// 获取材质
			virtual video::SMaterial& getMaterial(unsigned int i);

			// 场景结点使用材质数量
			virtual unsigned int getMaterialCount() const;

			// 设置场景结点属性
			virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

			// 读场景结点属性
			virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

			// 返回场景结点类型
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_3DPOINT; }

			// 更新节点
			void UpdatePoint(CHdSxPoint3D *point) { m_pPoint = point;}

			// 更新场景
			void UpdateManeger(ISceneNode* parent, ISceneManager* mgr) { Parent = parent; SceneManager = mgr;}

		private:
			CHdSxPoint3D*		        m_pPoint;			// 三维点
			video::SMaterial		    m_Material;			// 材质
			core::aabbox3d<float>		m_BBox;				// 包围盒
			EHD_POINT_STYLE			    m_Style;			// 点的样式
		};
	}
}