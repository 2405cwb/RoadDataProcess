/*! IObjectSceneNode.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : IObjectSceneNode.h
相关文件     : CPointSceneNode.h, CPolylineSceneNode.h
文件实现功能 : 对象SceneNode的基类接口
作者         : 软件部，姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/21   1.0      姚立                新增加内容
2013/02/27	 2.0      危迟
</PRE>
*******************************************************************************/
#pragma once
//#ifndef __C_OBJECT_SCENE_NODE_H_INCLUDED__
//#define __C_OBJECT_SCENE_NODE_H_INCLUDED__
#include "stdafx.h"
#include "..\hd3DEngine\include\irrlicht.h"
//#include "..\hdCommon\HDObject.h"
//#include "..\hdFramework\hdView.h"
#include "ISceneView.h"

using namespace irr;
using namespace irr::scene;

namespace hd
{
	class CHDObject;
	class CBursaWolfModel;
	namespace scene
	{
		extern HD3DSCENE_API video::SColor g_selColor;// = video::SColor(100,255,255,0);

		class HD3DSCENE_API IObjectSceneNode : public ISceneNode
		{
		public:
			//constructor
			IObjectSceneNode(const CHDObject* pData,const video::SColor& color, const video::SColor& selectedColor, 
				ISceneNode* parent, ISceneManager* mgr, s32 id)
				:ISceneNode(parent,mgr,id),
				m_pHdData(pData),m_Color(color),m_SelectedColor(selectedColor),m_bSelected(false),m_bDelete(false),m_bEdited(false),m_pView(NULL){}
			virtual ~IObjectSceneNode() 
			{
				if(m_bDelete && m_pHdData)
				{
					delete m_pHdData;
					m_pHdData = NULL;
				}
			}

			//! 设置线的显示颜色;
			void SetColor(const video::SColor & color) { m_Color = color; }

			//! 得到线的显示颜色;
			void GetColor(video::SColor& color) { color = m_Color; }

			//! 设置线的显示颜色;
			void SetSelectedColor(const video::SColor & color) { m_SelectedColor = color; }

			//! 得到线的显示颜色;
			void GetSelectedColor(video::SColor& color) { color = m_SelectedColor; }

			//! 设置当前点是否被选中
			void SetSelected(bool bSelected) { m_bSelected = bSelected; }

			//! 得到当前选中状态
			bool GetSelected() { return m_bSelected; }

			//! 获取数据对象,gsl-2012/7/18
			const CHDObject* GetHdData() {return m_pHdData;}
		
			//! 设置数据项
			void SetHdData(const CHDObject* pData) { m_pHdData = pData; }
			
			//! 设置所在视图
			void SetView(ISceneView* pView) { m_pView = pView; }

			//! 设置变换模型 test
			void SetModel(const CBursaWolfModel& pModel) { m_absModel = pModel; }

			//! 获取变换模型
			CBursaWolfModel& GetModel() { return m_absModel; }

			//! 设置渲染模型
			void SetRenderModel(const CBursaWolfModel& pModel) { m_renderModel = pModel; }

			//! 获取渲染坐标变换模型
			CBursaWolfModel& GetRenderModel() { return m_renderModel; }

			virtual void OnRegisterSceneNode() { m_bDisplayed = false; }

			virtual void render() { m_bDisplayed = true; }

			// 得到当前绘制状态
			bool GetDisplayed() { return m_bDisplayed; }

			// 设置是否释放时删除关联数据
			void SetDropData(bool bDrop){m_bDelete = bDrop;}

			//[add by mzm 2013.10.21] 为编辑符号时用
			// 设置是否编辑过
			void SetEdited(bool bEdited){m_bEdited=bEdited;}
			// 获取当前的编辑状态
			bool GetEdited() {return  m_bEdited;}
			//  [1/22/2014 liujun]
			bool IsDelete(){return m_bDelete;}

		public:
			const CHDObject*			m_pHdData;
			bool						m_bSelected;
			bool                        m_bEdited;       //是否已经编辑过[add by mzm]
			video::SColor				m_Color;
			video::SColor				m_SelectedColor;//当点被选中时显示的颜色
		protected:
			ISceneView*					m_pView;	// SceneNode所在视图
			// 相对坐标转换绝对坐标的转换模型
			CBursaWolfModel				m_absModel;
			// 相对坐标转转至显示坐标的模型 m_renderModel = m_absModel*(m_pView->getModel())
			CBursaWolfModel				m_renderModel;
			bool						m_bDelete;		//标记在析构时，是否删除CHDObject对象
		private:
			bool						m_bDisplayed;	// 当前对象是否已经绘制
		};
	}
}

//#endif