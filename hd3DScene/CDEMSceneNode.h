/*! @file
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : CDEMSceneNode.h
相关文件     : CDEMSceneNode.cpp, 
文件实现功能 : DEM渲染 
			   基于LOD加快速度渲染
作者         : 危迟
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/10/18   1.0      危迟                 创建
2014/02/21   1.1      冯晶                 重构
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "..\hdPointCloud\HdModelPointCloud.h"
using namespace hd;
using namespace irr;

namespace hd
{
	namespace scene
	{
		class HD3DSCENE_API CDEMSceneNode : public IObjectSceneNode
		{
		public:
			// 构造
			CDEMSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
				s32 maxLOD = 4, E_TERRAIN_PATCH_SIZE patchSize = ETPS_17,
				const core::vector3df& position = core::vector3df(0.0f, 0.0f, 0.0f),
				const core::vector3df& rotation = core::vector3df(0.0f, 0.0f, 0.0f),
				const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f));

			// 析构
			virtual ~CDEMSceneNode(void);

			//**************************ISceneNode接口***************************************//
		public:
			// 注册渲染
			virtual void OnRegisterSceneNode();

			// 渲染
			virtual void render();

			// 获取类型
			virtual ESCENE_NODE_TYPE getType() const {return ESNT_HD_DEM;}

			// 获取外包围盒
			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			// 返回分块的包围盒
			virtual const core::aabbox3d<f32>& getBoundingBox(s32 patchX, s32 patchY) const;

			// 获取纹理数量
			virtual u32 getMaterialCount() const;

			// 获取纹理
			virtual video::SMaterial& getMaterial(u32 i);

		public:

			// 根据视口重新加载数据
			BOOL ReloadData();

			// 设置相机移动变化值，从而重新计算索引值 默认为10.0f
			void SetCameraMovementDelta(f32 delta)
			{
				CameraMovementDelta = delta;
			}

			// 设置相机旋转变化，从而重新计算索引值 默认为1.0f
			void SetCameraRotationDelta(f32 delta)
			{
				CameraRotationDelta = delta;
			}

			// 通过包围盒过滤狭长的三角形
			bool CalculateTriBox(int id0, int id1, int id2);

			// 设置hls生成obj过滤三角形阈值
			void SetMaxTriaSIzeLen(float maxTriaSIzeLen) { m_MaxTriaSIzeLen = maxTriaSIzeLen; }

			// 设置细节层次对应的距离
			bool OverrideLODDistance( s32 LOD, f64 newDistance );

			// 加载DEM数据 返回0表示传进来的值为空，返回-1表示申请内存失败
			long SetDemData(CHdModelPointCloud* modelPcd);

			// 获取DEM数据
			CHdModelPointCloud* GetDemData() { return m_modelPCD; }

			// 设置高程缩放系数
			void SetHeightZoom(float i);

			// 获取是否按照高程渲染
			bool IsRenderByHeight() { return m_bRenderByHeight; }

			// 设置是否按照高程渲染
			void SetRenderByHeight(bool bRender) { m_bRenderByHeight = bRender; }

			// 获取mesh
			//IMesh* GetDEMMesh() { return m_Mesh;}

			// 设置材质
			void SetMaterial(SMaterial mat);

			// 设置渲染方式
			void SetRenderDemStyle(const ENUM_DEM_RENDERSTYLE& style);

			// 获取渲染方式
			ENUM_DEM_RENDERSTYLE GetRenderDemStyle() const { return m_renderStyle;}

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

			// 设置是否显示包围盒子
			void SetShowBoundingBox(bool bShow) { m_bShowBBox = bShow; }

			// 设置是否显示包围盒子
			bool GetShowBoxState() { return m_bShowBBox; }

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

			// 设置选择区域三角形
			vector<u32>& GetIndexSelBuffer()	{ return m_indexSelBuffer; }

			// 外部设置光照是否显示
			void SetLightOn(bool bOn){m_bLightOn = bOn;}

			// 外部获得光照是否显示
			bool IsLightOn(){return m_bLightOn;}

			// 根据屏幕坐标获得其与三角形交点坐标及相交的三角形,返回有效像素距离值
			int Get3DPosAndTrianglFromScrPos(
				core::vector3df& ptPoint,	  // 返回的相对坐标值
				core::triangle3df& triangle,	  // 返回选中三角形的相对坐标值
				int srcX,int srcY
				);

			// 清除颜色 fengjing
			void ClearAllColors();

			// 设置镜面光反射度
			void SetLightShiness(f32 LightShiness) { m_nLightShiness = LightShiness; }

			// 获取镜面光反射度，该值越小，立体感越好
			f32 GetlightShiness() { return m_nLightShiness; }

		private:
		
			// tif数据统计数据高度信息，计算顶点按照高程渲染的颜色
			void StatCoord(vector<vector<float>>& vecHeight,int width, int height);

			// 非tif数据统计高度信息
			void StatCoord();

			// 计算按坐标z渲染显示颜色
			void CalcuCoordRender();

			// 计算按循环色带渲染颜色
			void CalcuCycleRampRender();

			// 计算按单色渲染
			void CalcuOneColorRender();
			
			// 计算距离阈值
			void CalculateDistanceThresholds(bool scalechanged = false);
			
			// 创建分块
			void CreatePatches();
			
			// 计算分块中的数据
			void CalculatePatchData();
			
			// 渲染LOD计算， 返回值为-1表示相机为空，0-表示不需要重新计算， 1-表示正常返回
			virtual int preRenderLODCalculations();
			
			// 渲染索引的计算
			virtual void preRenderIndicesCalculations();
			
			// 获取索引值 
			u32 getIndex(const s32 PatchX, const s32 PatchY, const s32 PatchIndex, u32 vX, u32 vY) const;

			// 平滑地形
			void SmoothTerrain(IDynamicMeshBuffer* mb, s32 smoothFactor);

			//// 计算平滑后的顶点法向量
			//void CalculateNormals(IDynamicMeshBuffer* mb);

			// 计算顶点法向量
			void CalculateNormals();

		protected:
			// Dem切片数据结构
			struct SDEMPatch
			{
				SDEMPatch()
					: CurrentLOD(-1), Top(0), Bottom(0), Right(0), Left(0)
				{
				}

				s32							CurrentLOD;
				core::aabbox3df				BoundingBox;
				core::vector3df				Center;
				SDEMPatch*			Top;
				SDEMPatch*			Bottom;
				SDEMPatch*			Right;
				SDEMPatch*			Left;
			};

			// Dem数据
			struct SDEMData
			{
				SDEMData()
					: PatchSize(0), CalcPatchSize(0),
					MaxLOD(0),
					BoundingBox(core::aabbox3df( 99999.9f, 99999.9f, 99999.9f, -99999.9f, -99999.9f, -99999.9f)),
					Patches(0),hSize(0),wSize(0),hPatchCount(0),wPatchCount(0)
				{
				}

				SDEMData(s32 patchSize, s32 maxLOD, const core::vector3df& position, const core::vector3df& rotation, const core::vector3df& scale)
					: Position(position), Rotation(rotation), Scale(scale),
					PatchSize(patchSize), CalcPatchSize(patchSize-1),
					MaxLOD(maxLOD),
					BoundingBox(core::aabbox3df( 99999.9f, 99999.9f, 99999.9f, -99999.9f, -99999.9f, -99999.9f)),
					Patches(0),hSize(0),wSize(0),hPatchCount(0),wPatchCount(0)
				{
				}

				//s32		Size;
				s32		hSize;						// 宽度
				s32     wSize;						// 高度
				core::vector3df	Position;			// 偏移量
				core::vector3df	Rotation;			// 旋转量
				core::vector3df RotationPivot;		// 旋转中心
				core::vector3df	Scale;				// 缩放比例
				core::vector3df Center;				// 数据中心
				s32		PatchSize;					// 切片尺寸
				s32		CalcPatchSize;				// 实际计算切片尺寸
				s32     wPatchCount;				// x方向上切片数量
				s32		hPatchCount;				// y方向上切片数量
				s32		MaxLOD;						// 最大的LOD数
				core::aabbox3df	BoundingBox;		// 包围盒子
				core::array<f64> LODDistanceThreshold;	// LOD距离阈值
				SDEMPatch*		Patches;			// 切片数据
			};

			// 代码锁
			CRITICAL_SECTION m_cs;

			// 渲染色带
			CHdColorRamp	m_colorRampZ;

			// 高程色带编号
			int				m_ZRampIndex;

			// 色带循环
			CHdColorRamp	m_colorRampCycle;

			// 色带循环方向，Z--0，X--1，Y--2
			int				m_nAxis;

			// 循环渲染步长
			float			m_fCycleStep;

			// 循环渲染色带编号
			int				m_CycleRampIndex;

			// 单色渲染
			COLORREF		m_selColor;

			// DEM数据结构
			SDEMData m_DemData;

			// 渲染的顶点数
			u32 m_nVerticesToRender;

			// 渲染的索引数,三角形的个数
			u32 m_nIndicesToRender;

			// 选择区域三角形索引数组
			vector<u32> m_indexSelBuffer;

			// 是否使用默认的旋转中心 以DEM包围盒中心
			bool m_bUseDefaultRotationPivot;

			// 是否强制重新计算
			bool m_bForceRecalculation;

			// 是否重新计算距离阈值
			bool m_bOverrideDistanceThreshold;

			// 是否按照高程渲染
			bool m_bRenderByHeight;

			// 上一次相机的参数
			core::vector3df	OldCameraPosition;
			core::vector3df	OldCameraRotation;
			core::vector3df	OldCameraUp;
			f32				OldCameraFOV;

			// 相机的偏移量
			f32 CameraMovementDelta;
			f32 CameraRotationDelta;
			f32 CameraFOVDelta;

			// 按坐标渲染的变量最大\最小\步长
			float m_fMinCoord;
			float m_fMaxCoord;
			float m_fStep;

			// 有效数据点数
			u32 m_fValidCount;

			// 模型数据
			CHdModelPointCloud* m_modelPCD;

			// 材质,每个SceneNode必须包含此对象
			video::SMaterial	m_material;

			// 模型显示方式
			ENUM_DEM_RENDERSTYLE m_renderStyle;

			// 是否显示包围盒
			bool m_bShowBBox;
			
			// 最大三角形边长：过滤掉大于该边长的三角形
			float				m_MaxTriaSIzeLen;

			// 控制光照是否显示
			bool m_bLightOn;

			// 镜面光反射度
			f32 m_nLightShiness;

			// 控制三角形顶点在100W一下全部渲染
			bool m_bIsPcdSimpled;

			//渲染次数计数
			int m_RenderCount;

			//  测试选中三角形与非选中三角形分开渲染 [2014/04/18 危迟]
			//vector<u32> m_nUnselRenderTri;	// 非选中三角形列表
			//
			//u32 m_nSelCount;
			//
			//vector<u32> m_nSelRenderTri;	// 选中三角形列表
			//
			//u32 m_nUnselCount;
		};

	}
}

