/*! @file
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : CHdTinSceneNode.h
相关文件     : CHdTinSceneNode.cpp, 
文件实现功能 : 不规则三角网TIN渲染
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/10/18   1.0      危迟                 创建
2013/03/05   1.1      冯晶                 重构
2014/11/05   1.2      朱旭波               增加光照（demsn相同）
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "hd3DSceneDefine.h"
#include "..\hdCommon\hdTin.h"
#include "..\hdPointCloud\point_cloud.h"
#include "..\hdPointCloud\HdTINPointCloud.h"

using namespace hd;
using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CHdTinSceneNode : public IObjectSceneNode
		{
		public:
			// 构造
			CHdTinSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);

			// 析构
			virtual ~CHdTinSceneNode(void);

			//**************************ISceneNode接口***************************************//
		public:

			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual ESCENE_NODE_TYPE getType() const {return ESNT_HD_TIN;}

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual u32 getMaterialCount() const;

			virtual video::SMaterial& getMaterial(u32 i);

			//**************************自身接口***************************************//
		public:
			// 根据视口重新加载数据
			BOOL ReloadData();

			// 加载TIN 数据
			void SetTinData(CHdTINPointCloud* pcd);

			// 获取tin数据
			CHdTINPointCloud* GetTinData() { return m_TINPcd;}

			// 统计点云->Tin坐标 
			void StatCoord(PointCloud* pcd);

			// 统计DEM->Tin坐标 
			void StatCoord(CHdTINPointCloud* pcd);

			// 设置是否显示包围盒子
			void SetShowBoundingBox(bool bShow) { m_bShowBoundBox = bShow; }

			// 设置是否显示包围盒子
			BOOL GetShowBoxState() { return m_bShowBoundBox; }

			// 设置模型渲染风格
			void SetModelRenderStyle(ENUM_DEM_RENDERSTYLE style);

			// 设置材质
			void SetMaterial(SMaterial mat);

			// 获取渲染方式
			ENUM_DEM_RENDERSTYLE GetModelRenderStyle() const { return m_renderStyle;}

			// 设置循环渲染坐标轴
			void SetCycleInfo(int axis, int curRamp, float step) { m_nAxis = axis; m_CycleRampIndex = curRamp; m_fCycleStep = step;}

			// 获取循环渲染步长
			float GetCycleStep() { return m_fCycleStep;  }

			// 获取循环渲染坐标轴
			int GetCycleAxis(){ return m_nAxis;}

			// 获取循环色带索引
			int GetCycleRampIndex() {return m_CycleRampIndex;}

			// 设置高程渲染色带
			void SetZRampIndex(int cur) { m_ZRampIndex = cur; }

			// 获取高程渲染色带
			int GetZRampIndex() { return m_ZRampIndex;}

			// 设置单色渲染颜色
			void SetSelColor(COLORREF selcolor) { m_selColor = selcolor;}

			// 设置单色渲染颜色
			COLORREF GetSelColor() const { return m_selColor;}

			// 根据屏幕坐标获取点云坐标 
			// 函数返回最近的距离值 
			int Get3DPosFromScrPos(
				core::vector3df& ptPoint,	  // 返回的相对坐标值
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly        // 是否查找未显示的点
				);

			// 根据三维线获取相交点坐标 返回距离直线起点最近的坐标值
			int Get3DPosFrom3DLine(
				core::vector3df& ptPoint,	 // 返回的显示坐标值		   
				core::line3df& line			 // 传入的三维相交直线
				);

			// 根据三维线获取相交点坐标 返回距离直线起点最近的坐标值
			int Get3DPosFrom3DLine(
				core::vector3df& ptPoint,	 // 返回的显示坐标值		   
				core::line3df& line,		 // 传入的三维相交直线
				core::aabbox3df& Selbox		 // 限制的查询盒子
				);

			// 获取增加顶点
			int InterpolationFromScrPos( core::vector3df& ptPoint, /* 返回的相对坐标值*/ 
				f64& x,f64& y,f64& z,		/* 返回的绝对坐标值*/
				int srcX,int srcY,			/* 屏幕坐标 */
				int tol,					/* 屏幕查找范围*/
				bool bFindVisibleOnly		/* 是否查找未显示的点*/ );

			// 删除原有三角形，插值新三角形
			int InterpolatinFrom3DLine( core::vector3df& ptPoint, core::line3df& line);

			// 设置选择区域三角形
			vector<u32>& GetIndexSelBuffer()	{ return m_indexSelBuffer; }

			// 获取当前渲染的三角形个数
			vector<u32>& GetCurRenderIndices()	{ return m_CurRenderIndices; }

			void setCacuIndices(BOOL iscaculate) { m_bIsCacuIndices = iscaculate;}

			// 动态统计当前范围内的渲染索引
			void preRenderIndicesCalculations();

			// 外部设置光照是否显示
			void SetLightOn(bool bOn){m_bLightOn = bOn;}

			// 外部获得光照是否显示
			bool IsLightOn(){return m_bLightOn;}

			// 设置镜面光反射度
			void SetLightShiness(f32 LightShiness) { m_nLightShiness = LightShiness; }

			// 获取镜面光反射度
			f32 GetlightShiness() { return m_nLightShiness; }

			// 清除所有颜色
			void ClearAllColors();
		
		private:
			// 计算按坐标z渲染显示颜色
			void CalcuCoordRender();

			// 计算按循环色带渲染颜色
			void CalcuCycleRampRender();

			// 计算按单色渲染
			void CalcuOneColorRender();


			// 动态统计当前范围内相交的断面线 [2014/04/21 危迟]
			void preRenderProfileLineCalculations();

			// 查询在包围盒子内部的三角形
			bool GetTriangleFromSelBox(core::vector3df& aabbox,vector<u32> triIndices);
		private:
			// 代码锁
			CRITICAL_SECTION		m_cs;

			// 是否显示包围盒
			BOOL					m_bShowBoundBox;

			// 是否需要重新计算三角形索引，主要控制选择后重新计算点云索引
			BOOL					m_bIsCacuIndices;

			// 当前渲染三角形索引
			vector<u32>				m_CurRenderIndices; 

			// 当前选中的三角形
			vector<u32>				m_indexSelBuffer;

			// 断面线结点列表
			vector<core::vector3df> m_ProfilePts;

			// 渲染的索引数
			u32						m_nIndicesToRender;

			// 按坐标渲染的变量最大\最小\步长
			float					m_fMinCoord;
			float					m_fMaxCoord;
			float					m_fStep;

			// 渲染色带
			CHdColorRamp			m_colorRampZ;

			// 高程色带编号
			int						m_ZRampIndex;

			// 色带循环
			CHdColorRamp			m_colorRampCycle;

			// 色带循环方向，Z--0，X--1，Y--2
			int						m_nAxis;

			// 循环渲染步长
			float					m_fCycleStep;

			// 循环渲染色带编号
			int						m_CycleRampIndex;

			// 单色渲染
			COLORREF				m_selColor;

			// 包围盒
			core::aabbox3df			m_box;

			// 模型数据
			CHdTINPointCloud*		m_TINPcd;

			// 材质,每个SceneNode必须包含此对象
			video::SMaterial		m_material;

			// 模型显示方式
			ENUM_DEM_RENDERSTYLE	m_renderStyle;

			// 控制光照是否显示
			bool m_bLightOn;

			// 镜面光反射度
			f32 m_nLightShiness;

			// 记录渲染次数
			int m_RenderCount;
		};
	}
}


