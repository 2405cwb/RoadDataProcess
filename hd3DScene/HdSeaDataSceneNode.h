/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : HdSeaDataSceneNode.h
相关文件     : HdSeaDataSceneNode.cpp
文件实现功能 : 海量点云数据显示
作者         : 软件部，蔡红云
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/03/23	  1.0    蔡红云					创建并实现
2015/10/10	  1.1   蔡红云					更新取点接口
</PRE>
*******************************************************************************/
#pragma once
#include "IObjectSceneNode.h"
#include "..\hd3DEngine\include\ESceneNodeTypes.h"
#include "..\hdHlslib\HdCoreData.h"
#include "S3DVertex.h"
#include "..\..\hdCore\hdColor.h"
#include "CScanSceneNode.h"
#include "..\hdPointCloud\SeaPointCloud.h"
#include <windowsx.h>

#include "..\hd3DEngine\include\matrix4.h"

namespace hd
{
	namespace scene
	{
		//视图中选区视锥体
		struct SelectRegionFrustum
		{
			SelectRegionFrustum(const SViewFrustum& sf, bool insd,int slctmd,
				const core::rect<s32> &vrt,HRGN rgn ,int wd,int ht)
				:vctFrustum(sf),bInside(insd),selectMode(slctmd),viewPort(vrt),rgnPoly(rgn),
				width(wd),height(ht)
			{
			}

			~SelectRegionFrustum()
			{
				if (rgnPoly)
				{
					//DeleteObject((HGDIOBJ)(HRGN)(rgnPoly));
					DeleteRgn(rgnPoly);
					rgnPoly = NULL;
				}
			}

			//选区视锥体
			SViewFrustum vctFrustum;
		
			//是否为选择选区内部
			bool bInside;

			//选择模式：新建选择、加选择、减选择
			int selectMode;

			core::rect<s32> viewPort; // 视口
			
			HRGN 			rgnPoly;// 对应的屏幕区域

			// 窗口范围
			int width; 
			int height;

			//! Calculates 2d screen position from a 3d position.
			core::position2d<s32> getScreenCoordinatesFrom3DPosition(
				const core::vector3df & pos3d)
			{

				core::dimension2d<u32> dim(viewPort.getWidth(), viewPort.getHeight());

				dim.Width /= 2;
				dim.Height /= 2;

				core::matrix4 trans = vctFrustum.getTransform ( video::ETS_PROJECTION );
				trans *= vctFrustum.getTransform ( video::ETS_VIEW );;

				f32 transformedPos[4] = { pos3d.X, pos3d.Y, pos3d.Z, 1.0f };

				trans.multiplyWith1x4Matrix(transformedPos);

				if (transformedPos[3] < 0)
					return core::position2d<s32>(-10000,-10000);

				const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :
					core::reciprocal(transformedPos[3]);

				return core::position2d<s32>(
					core::round32(dim.Width * transformedPos[0] * zDiv) + dim.Width,
					dim.Height - core::round32(dim.Height * (transformedPos[1] * zDiv)));
			}

		private:

			SelectRegionFrustum(const SelectRegionFrustum& other );

			const SelectRegionFrustum &operator = (const SelectRegionFrustum& other)
			{
				return *this;
			}

		};

		class HD3DSCENE_API CHdSeaDataSceneNode: public IObjectSceneNode
		{
		public:
			CHdSeaDataSceneNode(irr::scene::ISceneNode* parent,irr::scene::ISceneManager* mgr,s32 id);
			~CHdSeaDataSceneNode(void);


			/************************************************************************/
			/*                   ISceneNode接口实现                                 */
			/************************************************************************/
		public:
			virtual void OnRegisterSceneNode();

			virtual void render();

			virtual const core::aabbox3d<f32>& getBoundingBox() const;

			virtual u32 GetMaterialCount() const;

			virtual video::SMaterial& GetMaterial(u32 i);

			//! Returns type of the scene node
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_HD_SEADATA_POINT; }

			bool IsTrans(){return m_bTrans;}

			void SetTrans(bool bTrans) { m_bTrans = bTrans; }


		protected:

			// 代码锁
			CRITICAL_SECTION m_cs;

			// 代码锁 针对选择操作
			CRITICAL_SECTION m_csSlct;

			//! 材质,每个SceneNode必须包含此对象
			video::SMaterial	m_material;

			//! 点云块外包范围
			core::aabbox3d<f32> m_box;

			//! 点云管理对象 
			CHdCoreData* m_hlzReader;

			//! 待显示的区域信息
			CHdListAreaNoInf* m_pHdlstArea;

			//! 点云数据是否改变
			bool		m_bPcdChanged;

			//渲染次数计数
			int m_RenderCount;

			// 方法
		public:
			//根据设置的强度拉伸范围，重新计算点的透明度信息   袁亮  20160728
			void  StretchIntenAndCalcAlpha();

			// 首次是加载最顶层数据
			void LoadTopData();

			// 绘制点  **单点绘制**
			void RenderPoint();
			
			// 当鼠标滚动时，屏幕分辨率改变切换层显示
			BOOL ChangeLevlByScale();

			// 根据当前视口内满足一定点数切换层级
			BOOL ChangeLevlByPtNum(bool quickcamer);

			// 根据相机的包围盒统计待显示的数据列表
			BOOL GetAreaListByCameraBox();

			//! 根据视口重新加载数据
			BOOL ReloadData();
						
			// 检测切换到哪一层满足数据量需求
			BOOL DetectStLvl(bool quckcam);

			// 构建显示列表
			BOOL CreateRenderList(CHdParcelBase * pHdParcelBase);

			//  通过坐标获取颜色值
			void GetColorByCoord(u32 rgb ,HlzPoint* pt);

			void CalcuCoordRender(float h,RenderColor& color);

			// 渲染显示列表
			void RenderList();

			// 获取当前显示的层数
			int  GetCurShowLevl();

			// 获取当前内存中的点数
			int GetPointNumInMemory();

			// 统计内存中点数
			void CalPointNumInMemory();

			// 根据盒子的角点判断是否需加载显示
			BOOL CheckLoad(core::aabbox3df box);

			//! 设置点云渲染方式
			virtual void SetRenderStyle(const ENUM_RENDERSTYLE& style);

			//! 获取点云渲染方式
			ENUM_RENDERSTYLE GetRenderStyle() const;

			//! 计算按照循环色带渲染颜色
			void CalcuCycleRampRender(RenderColor&color ,float x,float y,float z);

			// 设置默认渲染颜色
			void SetDefaultColor( const SColorf& clr );

			//! 设置点显示大小
			void SetPointSize(f32 size);

			//! 获取点显示大小
			f32 GetPointSize() const;

			//! 设置海量点云数据
			BOOL SetPointCloud(CSeaPointCloud* pcd);

			//! 设置海量点云数据
			BOOL SetPointCloud(CSeaPointCloud* pcd,CBursaWolfModel model);

			// 获取相机外包围盒
			void GetCameraBox(CHdBox3df & camerabox);

			//! 获取海量点云数据
			CSeaPointCloud* GetPointCloud()
			{
				return m_SeaPointCloud;
			}

			//! 获取当前视图范围点云质心
			BOOL GetViewCenter(float& cx,float& cy,float& cz);

			u32 GetPointSizeBtn() const 
			{ 
				return m_PointsizeBtn; 
			}

			void SetPointSizeBtn(u32 PointsizeBtn) 
			{ 
				m_PointsizeBtn = PointsizeBtn;
			}

			//! 设置循环渲染步长
			void SetCycleStep(float step) 
			{
				m_fCycleStep = step;
			}

			//! 获取循环渲染步长
			float GetCycleStep() 
			{ 
				return m_fCycleStep; 
			}

			//! 设置循环渲染基准坐标轴
			void SetCycleAxis(int axis) 
			{ 
				m_nAxis = axis; 
			}

			//! 获取循环渲染基准坐标轴
			int GetCycleAxis()
			{ 
				return m_nAxis;
			}

			//! 设置是否根据反射强度做透明度渲染
			void ShowIntensityRender(BOOL bShow);

			void ShowBoundingBox( BOOL bShow )
			{
				m_renderBBox = bShow;
			}

			void GetMaxMinZ( float& nMaxZ, float& nMinZ );

			//! 是否绘制BoundingBox
			BOOL IsShowBoundingBox() { return m_renderBBox; }

			// 获取显示列表
			CHdListAreaNoInf* GetHdListAreaRndInf()  {return &m_SeaPointCloud->GetHdListAreaNoInf();} 

			// 获取选中点颜色
			COLORREF GetSelColor(){return m_selPtColor;}

			// 设置选中点颜色
			void SetSelColor(COLORREF selColor){m_selPtColor = selColor;}
					
			// ！获取按Z渲染的最大最小值
			void GetZmaxmin(f32& zmax, f32& zmin){ zmax = m_fMaxHeight; zmin = m_fMinHeight;}
			
			// ！设置按Z渲染的最大最小值
			void SetZmaxmin(f32 zmax, f32 zmin){ m_fMaxHeight = zmax ; m_fMinHeight = zmin;}
			
			// ！获取按X渲染的最大最小值
			void GetXmaxmin(f32& xmax, f32& xmin){ xmax = m_fMaxCorX; xmin = m_fMinCorX;}
			
			// ！设置按X渲染的最大最小值
			void SetXmaxmin(f32 xmax, f32 xmin){ m_fMaxCorX = xmax ; m_fMinCorX = xmin;}
			
			// ！获取按Y渲染的最大最小值
			void GetYmaxmin(f32& ymax, f32& ymin){ ymax = m_fMaxCorY; ymin = m_fMinCorY;}
			
			// ！设置按Y渲染的最大最小值
			void SetYmaxmin(f32 ymax, f32 ymin){ m_fMaxCorY = ymax ; m_fMinCorY = ymin;}

			// 根据屏幕坐标获取点云坐标 
			// 函数返回最近的距离值 
			unsigned int Get3DPosFromScrPos(
				PointXYZIPRGBA& ptPoint,	  // 返回的相对坐标值
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly,        // 是否查找未显示的点
				bool bSelected = false        // 是否过滤掉选中点 
				);

			unsigned int Get3DPosFromScrPos1(
				PointXYZIPRGBA& ptPoint,	  // 返回的相对坐标值
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly,        // 是否查找未显示的点
				bool bSelected = false        // 是否过滤掉选中点 
				);

			// 获取屏幕点附近的点集
			unsigned int Get3DBufferPtFromScrPos(
				vector<core::vector3df>* pts,	  // 返回的相对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly,        // 是否查找未显示的点
				bool bSelected = false        // 是否过滤掉选中点 
				);


			//! 根据传入的坐标值获取以该点为中心一定范围内所有点海量点云得平均值作返回
			bool GetAveragePosFromSea3DPos(
				f64& x,f64& y,f64& z,  // 传入的3D坐标，并返回该范围平均值
				float area = 0.1,      // 范围值，距离为米  
				bool bVisiable = true);// 是否要求点云可见,true点云必须可见

			// 更新选择的点云
			void UpdateSltPoints();
			
			void UpdateSltInfo(SelectRegionFrustum* pSelRegion);

			// 点击清除选中后，把矩形区域置空
			void ClearSltRgn();

			void SetDefaultColorByCycle(int cursel);

			// 获取观察点
			const core::vector3dd & GetEye(){ return m_eye;}

			// 设置观察点
			void SetEye(const core::vector3dd& eye){ m_eye = eye;}

			// 设置切换层级显示
			BOOL SetChangeLvl(BOOL flag){return m_bChangLvl = flag;}

			// 获取按高程渲染色带条
			const CHdColorRamp& GetColorRampZ(){return m_colorRamp;}

			// 根据鼠标选择的范围设置
			void SetDefaultColorByZ(int cursel);

			// 设置透视投影下显示距离
			void SetShowDistInQucikCamera(float dist){m_distance = dist;}

			// 设置3D相机下浏览阈值
			void SetThredIn3DCamera(int thred) { m_LoadThredIn3DCamrera = thred; }

		    // 设置快速相机下浏览阈值
			void SetThredInQuickCamera(int thred) { m_LoadThredInQucikCamera = thred; }

			// 全景视图下根据射线取点测试 inScanPos 全景球位置
			// EndPos 碰撞检测得到的坐标 
			// outX outY outZ 输出坐标
			bool Get3dPointFromPano(core::vector3df inScanPos,core::vector3df EndPos, float& outX,
				float& outY,float& outZ);
			
			// 测试代码，测试根据传入矩阵由三维坐标计算屏幕坐标
			core::position2d<s32> GetScreenPosFrom3dPosTst(core::matrix4& camMatrix,core::dimension2d<u32> dim,const core::vector3df& pos3d);
			
			// 测试代码，函数返回最近的距离值 
			unsigned int Get3DPosFromScrPosTst(
				//const core::vector3df RelativeRot,
				//const core::vector3df RelativeTrans,
				//const core::vector3df RelativeScale,
				core::matrix4& camMatrix,
				core::dimension2d<u32> dim,
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol						  // 屏幕查找范围
				);

		private:

			// 渲染的列表单元
			struct SRenderUnit
			{
				video::S3DVertex2TCoords* vertices;
				u32 count;
				u32 size;
			};

			// 是否进行坐标转换
			bool m_bTrans;

			// 渲染列表
			core::array<SRenderUnit> m_Renderlist;

			// 渲染色带
			CHdColorRamp			m_colorRamp;

			// 循环色带渲染时色带条
			CHdColorRamp            m_colorRampCycle;

			// 内存中点数
			u64                     m_ptNum; 

			// 点云渲染方式
			ENUM_RENDERSTYLE m_renderStyle;

			// 是否显示透明度
			BOOL        m_bShowIntenRender;

			//! 按坐标渲染的变量最大\最小\步长
			float m_fMinCoord;
			float m_fMaxCoord;
			float m_fStep;

			// 色带循环方向，Z--0，X--1，Y--2
			int				m_nAxis;

			//! 设置按Z渲染的最大最小值
			float m_fMinHeight;
			float m_fMaxHeight;

			//! 设置按X渲染的最大最小值
			float m_fMinCorX;
			float m_fMaxCorX;

			//! 设置按Y渲染的最大最小值
			float m_fMinCorY;
			float m_fMaxCorY;

			// 循环渲染步长
			float m_fCycleStep;

			//! 默认颜色
			RenderColor	m_defaultClr;

			//  是否渲染包围盒
			BOOL		m_renderBBox;

			// 选中点颜色
			COLORREF m_selPtColor;

			// 海量点云对象
			CSeaPointCloud* m_SeaPointCloud;

			u32 m_PointsizeBtn;

			//! 反射强度渲染加强，将最大最小值缩小
			int m_nMinIntensity;
			int m_nMaxIntensity;
			//强度拉伸情况下，当前强度范围   袁亮  20160728
			int m_nCurMinIntensity;
			int m_nCurMaxIntensity;

			// 储存计算所得的透明度信息
			map<U64,u8* >* m_pApha;

      		u32  m_selectCount; // 选择点数

			//保存点云选择的视锥体的列表，用来控制海量点云的选中状态  --liangjia 20150630
			vector<SelectRegionFrustum*> m_vctSelRegion;

			//! 点云显示方式
			ENUM_SHOWSTYLE   m_showStyle;
			
			// 快速相机下进行浏览时的距离阈值
			float m_distance;

			// 3D相机下加载的点云阈值
			int  m_LoadThredIn3DCamrera;

			// 快速相机下加载的点云阈值
			int m_LoadThredInQucikCamera;

			// 是否需要切换层级显示
			BOOL m_bChangLvl;


			// 是否需要时实时过滤
			bool m_bAutoFiltr;

			// 观察点、临时存入节点对象中\用于快速浏览
			core::vector3dd m_eye;

			// 是否锁定内存
			bool m_lockMemory;

			//-------更新点云选择信息----------------------------------

		public:

			// 获取选择的点数,内部实时统计
			u32 getSelectCount();

			// 清除选择
			void SetUnSelect();

			// 选择所有点
			void SelectAll();

			// 反选
			void InvertSelect();

			//! 设置显示方式
			void SetShowStyle(const ENUM_SHOWSTYLE eStyle);

			//! 获取显示方式
			ENUM_SHOWSTYLE GetShowStyle() const;

			// 获取是否要自动过滤
			bool GetAutFilter(){return m_bAutoFiltr;}

			// 设置是否要自动过滤
			void SetAutoFilter(bool altoFilter){m_bAutoFiltr = altoFilter;}

			// 设置是否锁定内存
			void SetLockMemory(bool lockMemory){m_lockMemory = lockMemory;}

			//! 更新选择过滤以刷新显示
			hd::u32 updateForFilter();
			
		};
	}
}



