/*! ISceneView.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : ISceneView.h
相关文件     : 
文件实现功能 : 三维视图抽象类,和鬼火设备相关
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2013/03/07   1.0      龚书林    		
2014/02/21	 1.2      龚书林			 增加事件相关接口
</PRE>
*******************************************************************************/

#pragma once
#include "stdafx.h"
#include "..\hdframework\hdview.h"
#include "hd3DSceneDefine.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "hdCamera.h"
#include "..\hd3DEngine\include\SColor.h"
#include "..\hdCommon\point_types.h"
#include "..\hdCommon\sceneData\HdSxPolyline3D.h"
//#include "..\hdPointCloud\hdFilterManager.h"
#include "..\hdPointCloud\hdFilterSelection.h"
#include <vector>


// 前置声明鬼火设备
namespace irr
{
	class IrrlichtDevice;
	namespace scene
	{
		class ISceneManager;
	}
}

namespace hd
{
	// 前置声明CHd3DCamera
	namespace fm
	{
		class CHd3DCamera;
	}
	// 前置声明CHDObject
	class CHDObject;
	class PointCloud;

	namespace scene
	{
		// 前置声明IObjectSceneNode
		class IObjectSceneNode;

		// 引用命名空间
		using namespace std;
		using namespace irr;
		using namespace irr::scene;
		using namespace hd::fm;

		// 相机状态
		struct SCamStatus
		{
			core::vector3df pos;
			core::vector3df target;
			core::vector3df UpVector;

			// 是否正交
			bool IsOrthogonal;

			// 正交投影
			f32 WidthOfViewVolume;  
			f32 HeightOfViewVolume; 
			// 透视投影
			f32 Fovy;	
			f32 Aspect;	
			
			f32 ZNear;	
			f32 ZFar;	

			SCamStatus():pos(),target(),UpVector(),IsOrthogonal(false),WidthOfViewVolume(0.0f),
						HeightOfViewVolume(0.0f),Fovy(0.0f),Aspect(0.0f),ZNear(0.0f),ZFar(0.0f)
			{

			}

			SCamStatus(ICameraSceneNode* cam)
			{
				SetCamera(cam);
			}
			
			SCamStatus& operator= (const struct SCamStatus& rhs)
			{
				pos = rhs.pos;
				target = rhs.target;
				UpVector = rhs.UpVector;
				IsOrthogonal = rhs.IsOrthogonal;
				WidthOfViewVolume = rhs.WidthOfViewVolume;
				HeightOfViewVolume = rhs.HeightOfViewVolume;
				Fovy = rhs.Fovy;
				Aspect = rhs.Aspect;
				ZNear = rhs.ZNear;
				ZFar = rhs.ZFar;
				return *this;
			}

			bool operator== (const struct SCamStatus& rhs) const
			{
				return pos == rhs.pos && target == rhs.target
					&& UpVector == rhs.UpVector && IsOrthogonal == rhs.IsOrthogonal
					&& WidthOfViewVolume == rhs.WidthOfViewVolume
					&& HeightOfViewVolume == rhs.HeightOfViewVolume
					&& Fovy == rhs.Fovy && Aspect == rhs.Aspect
					&& ZNear == rhs.ZNear && ZFar == rhs.ZFar;
			}

			bool operator!= (const struct SCamStatus& rhs) const
			{
				return pos != rhs.pos || target != rhs.target
					|| UpVector != rhs.UpVector || IsOrthogonal != rhs.IsOrthogonal
					|| WidthOfViewVolume != rhs.WidthOfViewVolume
					|| HeightOfViewVolume != rhs.HeightOfViewVolume
					|| Fovy != rhs.Fovy || Aspect != rhs.Aspect
					|| ZNear != rhs.ZNear || ZFar != rhs.ZFar;
			}

			void SetCamera(ICameraSceneNode* cam)
			{
				if (cam)
				{
					pos = cam->getPosition();
					target = cam->getTarget();
					UpVector = cam->getUpVector();
					IsOrthogonal = cam->isOrthogonal();
					WidthOfViewVolume = cam->getWidthofViewVolume();
					HeightOfViewVolume = cam->getHeightofViewVolume();
					Fovy = cam->getFOV();
					Aspect = cam->getAspectRatio();
					ZNear = cam->getNearValue();
					ZFar = cam->getFarValue();
				}
			}
		};

              
        using namespace irr;
		using namespace irr::video;

		//！获取二维坐标的最大（小）横（纵）轴坐标值 [ 2013-7-17 冯晶]
		static void VecUpdateMinMax2dv(core::position2di &min, core::position2di &max, const core::position2di v)
		{
			if (v.X<min.X) min.X = v.X; else if (v.X>max.X) max.X=v.X;
			if (v.Y<min.Y) min.Y = v.Y; else if (v.Y>max.Y) max.Y=v.Y;
		}

		[event_source(native)]
		class HD3DSCENE_API ISceneView :
			public IHdView
		{
			typedef DWORD (ISceneView::*ReloadThreadProc)(LPVOID);
		private:
			//! 加载
			HANDLE m_reloadEvent;
			HANDLE m_reloadThread;
			////使用Windows线程池负责所有ScanNode的reload工作   袁亮   20160820
			//PTP_POOL  m_reloadPool;
		protected:
			//! 鬼火绘图设备
			irr::IrrlichtDevice* m_irrDevice;

			//! 当前视图SceneNode列表
			//vector<IObjectSceneNode*> m_sceneList;

			//! 当前视图下camera的状态记录列表
			vector<SCamStatus> m_CamVec;

			//! 当前摄像机工具
			CHdCamera* m_camera;
			//！3D浏览工具
			CHd3DCamera* m_p3DCamera;
			//! 是否显示FPS刷新速率
			bool m_bShowFPS;
			//! 是否显示Lod
			bool m_bShowLOD;

			//! 是否显示比例尺图例
			bool m_bShowRadioLegend;

			//! 是否显示坐标系
			bool m_bShowAxis;
			//！ 是否显示指北针
			bool m_bShowComPass;
			//! 是否显示色彩图例
			bool m_bShowCL;
			//! 是否显示渐变背景
			bool m_bShowBackGround;
			//! 是否渲染全部结点
			bool m_bRenderAll;
			//! 是否显示碰撞检测
			bool m_bShowCollision;
			//! 点云是否添加或移除
			bool m_bScanChanged;
			//! 点云点大小
			f32			m_pointSize;
			//! 是否动态抽稀点云
			bool        m_bSimpleRender;
			//! 视图显示坐标转换模型
			CBursaWolfModel m_transModel;
			//!	2013/8/2 蔡红云 旋转中心点
			core::vector3df m_rotCenter;
			//! 视图投影类型
			ENUM_HD_3D_PROJECTION_TYPE m_eProjType;
			//! 显示类型
			ENUM_SHOWSTYLE m_eShowStyle;
			// ！是否显示扫描仪模型
			bool m_bShowScanModel;
			//! 是否显示外包围盒
			bool m_bShowOutBox;
			// ！为了确保按色带循环渲染时，多个点云效果保持一致 蔡红云 2013/11/21
			// 统计按色带循环渲染时，步长、坐标轴、及色带条信息
			// ！循环色带渲染步长
			f32 m_cyclstep;
			// ! 循环色带渲染坐标轴方向
			int m_cyclaxis;
			// ！循环色带渲染色带条信息
			int m_cyclcur;
			// 代码锁
			CRITICAL_SECTION m_cs;
			// 设定渲染的类别的索引 fengjing
			bool m_bRenderClassIndex[256];

			// 渲染设置-类别是否重新设置类别显示状态
			bool m_bIsRenderSetting;

			// 上次相机状态
			SCamStatus m_lastCameraStatus;

			// 标记是否需要进行截屏SN
			bool m_bNeedScreenShot;

			// ! 点云强度拉伸极小阈值
			float m_fMinIntenStre;

			// ! 点云强度拉伸极大阈值
			float m_fMaxIntenStre;

			//! 标记是否只对当前可见点云查看全部等功能
			bool m_bZoomToVisiblePcd;

			// 1cm 对应的像素值
			int m_nLegend;

			//! 标记视图是否为mongo服务器远程的工程还是本地视图数据，由主程序通过获得scanRoute的m_sourceType设置
			bool m_bMongoData;

			// 用于标记点云在隐藏和显示时均能触发加载
			bool m_bCalReLoad;

			// 上次刷新模式
			ESCENE_REFRESH_TYPE m_lastRefresh;

			//! 内置过滤管理器对象
			//ptcloud::hdFilterManager m_filter_manager;
		private:
			// !动态浏览加载数据线程函数
			static DWORD WINAPI ReloadDataThread(LPVOID param);
			//ScanNode回调加载函数，将被Windows线程池中线程调用      袁亮   20160820
			//static VOID CALLBACK ReloadSNCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext);
			//
		protected:
			//! 设置相机状态
			virtual void SetCamera();

		public:// 声明对象事件 gsl-2014/02/21

			// 视图坐标转换模型改变事件
			__event void OnTransModelChanged(CBursaWolfModel* pTransModel);

			// 视图中添加对象事件
			__event void OnAddedSceneNode(IObjectSceneNode* pObjSceneNode);

			// 视图范围改变事件,即相机位置和方向变化
			__event void OnCameraChanged(ICameraSceneNode* pCamera);
		public:
            

			ISceneView(void);
			virtual ~ISceneView(void);

			//! 设置显示坐标转换模型
			void SetTransModel(CBursaWolfModel* pTransModel)
			{
				m_transModel = *pTransModel;
				__raise OnTransModelChanged(pTransModel);
			}
			//! 根据参考点云对象,设置显示坐标转换模型
			void SetTransModel(PointCloud* pPcd);
			//! 获取显示坐标转换模型,获取后调用AntiTranslate接口将绝对坐标转换为显示坐标
			CBursaWolfModel* GetTransModel(){return &m_transModel;}

			//! 清除SceneNode选中状态
			void ClearSelection(ISceneNode* node);

			//! 清除所有IobjectSceneNode选中状态
			void ClearAllSelection();

			//! 初始化鬼火视图
			virtual void InitialView(HWND hwnd);

			//! 获取鬼火设备对象
			IrrlichtDevice* GetIrrDevice(){return m_irrDevice;}

			//! 获取鬼火Scene管理对象
			ISceneManager* GetSceneManager();

			// 标记设置是否只对该视图中可见的点云查看全部、俯视图等功能
			void SetZoomVisiblePcds(bool bOnlyVisiblePcds){m_bZoomToVisiblePcd = bOnlyVisiblePcds;}

			// 外部获得是否只对当前可见的点云查看全部功能
			bool isOnlyZoomToVisiblePcds() const {return m_bZoomToVisiblePcd;}
	
			//! 设置FPS显示状态
			void SetFPSVisiable(bool bshow);

			//! 获取FPS显示状态
			bool IsFPSVisiable() const {return m_bShowFPS;}

			//! 设置LOD显示状态
			void SetLODVisiable(bool bshow);

			//! 设置比例尺图例显示状态
			void SetRadioLegendVisiable(bool bShow);

			//! 获取比例尺图例显示状态
			bool IsRadioLegendVisiable(){return m_bShowRadioLegend;}

			//! 获取LOD显示状态
			bool IsLODVisiable() const {return m_bShowLOD;}
	
			//! 设置坐标系显示状态
			void SetAxisVisiable(bool bShow);

			//! 设置指北针显示状态 2013/11/15 蔡红云 
			void SetComPassVisiable(bool bShow);

			//! 获取坐标系显示状态
			bool IsAxisVisiable() const{return m_bShowAxis;}

			//！获取指北针的显示状态
			bool IsComPassVisiable() const{return m_bShowComPass;}

			//! 设置色彩图例显示状态
			void SetColorLegendVisible(bool bShow); 

			//! 设置点云是否抽稀显示
			void SetPointCloudSimpleRender(bool bSimple) { m_bSimpleRender = bSimple; }

			//! 获取点云是否抽稀显示
			bool IsPointCloudSimpleRender() { return m_bSimpleRender; }

			//! 获取色彩图例显示状态
			bool IsColorLegendVisible() const { return m_bShowCL; }
			//! 设置渐变背景显示状态
			void SetBackGroundVisible(bool bShow);

			//! 获取渐变色背景显示状态
			bool IsBackGroundVisible() const { return m_bShowBackGround;}

			//! 设置碰撞检测显示状态 [2013/06/20 危迟]
			void SetCollisionVisible(bool bShow) { m_bShowCollision = bShow; }

			//! 获取碰撞检测状态 [2013/06/20 危迟]
			bool IsCollisionVisible() const { return m_bShowCollision; }

			//! 获取视图是否刷新全部
			bool IsViewRenderAllNode() const { return m_bRenderAll; }
			
			// !截屏
			void CreateScreenShot();

			// !设置需要截屏
			void SetNeedScreenShot(){m_bNeedScreenShot = true;}

			//! 设置视图刷新全部
			void SetViewRenderAllNode(bool bAll, bool bCreateScreenShot = true);

			//! 设置视图刷新全部，屏蔽调用强制刷新
			void SetViewRenderAllNodeWithoutRefresh(bool bAll, bool bCreateScreenShot = true);

			// 设置投影模式, 正交、透视  
			void SetProjectionType(ENUM_HD_3D_PROJECTION_TYPE projType);

			// ! 获取点云强度拉伸阈值
			void GetIntensityStrethThreshold(float& min,float& max) { min = m_fMinIntenStre; max = m_fMaxIntenStre;}

			// ！ 设置点云强度拉伸阈值
			void SetIntensityStrethThreshold(float min,float max){ m_fMinIntenStre = min; m_fMaxIntenStre = max;}

			// 获取投影模式 
			ENUM_HD_3D_PROJECTION_TYPE GetProjectionType();

			//! 根据CHDObject获取对应ObjectSceneNode
			IObjectSceneNode* GetObjSceneNode(const CHDObject* pHdData);
			IObjectSceneNode* GetObjSceneNode(const CHDObject* pHdData,ISceneNode* pStart);
			//! 获取当前相机浏览工具
			CHdCamera* getCameraTool() 
			{ 
				return m_camera;
			}
			//! 设置当前相机浏览工具
			bool SetCameraTool(CHdCamera* camera);
			//! 获取3D浏览工具
			CHd3DCamera* Get3DCameraTool(){return m_p3DCamera;}
			//! 设置3D浏览工具
			void Set3DCameraTool(CHd3DCamera* pCamera){m_p3DCamera = pCamera;}
			//! 添加数据项到当前视图
			virtual IObjectSceneNode* AddObject(const CHDObject* pHdObject);

			//! 添加数据项到当前视图
			void AddObject(const std::vector<CHDObject*> dataList);

			//! 获取视图中是否包含指定类型场景结点 [2013/07/04 危迟]
			bool IsIncludeSceneNode(ESCENE_NODE_TYPE type);

			//! 获取视图中指定类型场景结点是否可见 [2013/08/05 危迟] 
			bool IsSceneNodeVisible(ESCENE_NODE_TYPE type);

			//! 根据类型设置场景节点可见性
			void SetSceneNodeVisible(ESCENE_NODE_TYPE type,bool bVisible);
			//! 是否包含iScan点云
			bool HasIScanSceneNode();
	
			//! 获取视图中点云数据是否变化
			bool IsScanChanged(){return m_bScanChanged;}
			//! 设置视图中点云数据是否变化,添加、删除、设置CScanSceneNode可见状态后均是改变状态
			void SetScanChanged(bool bChange){m_bScanChanged = bChange;}

			// 设定是否渲染该类别 fengj
			void SetIsRenderClass(int index, bool bRender){  m_bRenderClassIndex[index] = bRender; }

			// 获取是否渲染该类别 fengj
			bool GetIsRenderClass(int index) { return m_bRenderClassIndex[index];}

			// 渲染设置-类别 设置是否重新设置类别显示状态 fengj
			void SetbRenderSetting(bool bRender) { m_bIsRenderSetting = bRender; }

			// 渲染设置-类别 获取是否重新设置类别显示状态 fengj
			bool GetbRenderSetting() {return m_bIsRenderSetting;}

			//! 获取坐标轴图例的中心点
			void GetAxisPoint(int& x,int& y)
			{
				x = (int)(m_width/12.0f);
				y = (int)(m_height-m_width/12.0f);
			}

			// 获取指北针坐标 2013/11/15 蔡红云
			void GetComPassPoint(int& x, int& y)
			{
				x = (int)(m_width - m_width/12.0f);
				y = (int)(m_width/12.0f);
			}

			//! 获取点云点大小
			virtual f32 GetScanPointSize();

			//! 设置点云显示的点大小
			virtual void SetScanPointSize(float fSize);

			//! 设置点云渲染方式
			virtual void SetScanRenderStyle(ENUM_RENDERSTYLE style);
			//! 得到点云的渲染方式
			virtual ENUM_RENDERSTYLE GetScanRenderStyle();
			//! 设置点云显示可见方式
			virtual void SetShowStyle(ENUM_SHOWSTYLE style);
			//! 获取点云可见显示方式
			virtual ENUM_SHOWSTYLE GetShowStyle();
			// 设置点云默认显示颜色 [2013/11/19 危迟]
			virtual void SetPointCloudDefaultColor(SColorf color);
			// 获取点云默认显示颜色 [2013/11/19 危迟]
			virtual SColorf GetPointCloudDefaultColor();

			// 获取测站索引
			virtual int GetScanIndex() { return -1;}
			//! 刷新
			virtual void Refresh();
			//! 部分刷新
			virtual void RefreshPartial(ESCENE_REFRESH_TYPE type);
			//! 重新加载点云数据
			void ReloadData();
			//! 通过发送消息刷新视图 满足即时刷新需求 
			virtual void RefreshViewBySendMessage();

			//! 大小改变消息
			virtual void OnSize(int cx, int cy);

			void ResetTransModel() { m_transModel.getIdentity();}

			// 添加相机状态
			void AddCamStatus(SCamStatus camSts);

			// 获取相机状态个数
			u32 GetCamStatusCount() { return m_CamVec.size(); }
			// 获取最近的相机状态
			SCamStatus GetLatestCameraStatus();

			//! 获取视图中心,如果是地面单测站就是测站原点,其他情况是立方盒中心
			virtual core::vector3df GetCenter(core::aabbox3df& bbox = core::aabbox3df());

			//!计算场景中缩小以及旋转的最小阈值height * width fengjing 2013-7-17
			// 移植函数至视图中，便于其他工具可以从视图中调用 [2013/12/10 危迟]
			int GetScreenRange(unsigned int &height, unsigned int &width);

			//! 获取当前视图点云数据中心
			virtual core::vector3df GetDataCenter(core::aabbox3df& bbox = core::aabbox3df());

			//! 获取视图旋转中心点
			bool GetRotateCenter(core::vector3df& rotCenter,int srcX,int srcY);

			//！2013/8/2 蔡红云 获取视图旋转中心点新
			bool GetRotateCenterNew(core::vector3df& rotCenter,int srcX,int srcY);

			//！2013/8/2 蔡红云 设置视图旋转中心点新
			bool SetRotateCenterNew(core::vector3df& rotCenter,int srcX,int srcY);

			//bool GetAverageIntenSityFrom3DPos( f64& x,f64& y,f64& z, float areaTol = 0.1 );
			//! 根据平面位置获取点云的行列	zhangfei
			PointXYZIPRGBA getImageScaleFrScrPos(
				ESCENE_NODE_TYPE type, 
				float& row, 
				float& col,
				int srcX, 
				int srcY, 
				int tol = 5);

			//! 根据屏幕矩形框获取点 zhangfei
			bool QueryPtsInRect(
				ESCENE_NODE_TYPE type, 
				HRGN Rgn,
				std::vector<PointXYZIPRGBA*>& rectInPts
				);

			//! 根据平面位置获取强度
			bool getIntensityFrScrPos(int& intensity, int scrX, int scrY, int tol);

			//! 根据屏幕位置获取点云，返回该点显示坐标 fengjing
			 bool Get3DPosFromScrPos(
				PointXYZIPRGBA& ptPoint,
				ESCENE_NODE_TYPE type,  // 场景结点类型   
				int srcX, int srcY,		// 屏幕坐标
				int tol = 5);           // 屏幕容差

			 //! 根据屏幕坐标获取相对坐标和从相对到转换模型 [zhangfei 2014/3/24]
             //点云的场景结点在函数内部判断，上层不用关心场景结点的类型[liangjia 2015/6/26]
			  bool Get3DPosAndModelFromScrPos(
				  PointXYZIPRGBA& ptPoint,
				  CBursaWolfModel& transModel,
				  ESCENE_NODE_TYPE type,  // 场景结点类型   
				  int srcX, int srcY,		// 屏幕坐标
				  int tol = 5
				  );

              //点云的场景结点在函数内部判断，上层不用关心场景结点的类型[liangjia 2015/6/26]
              bool Get3DPosAndModelFromScrPos(
                  PointXYZIPRGBA& ptPoint,
                  CBursaWolfModel& transModel,
                  //ESCENE_NODE_TYPE type,  // 场景结点类型   
                  int srcX, int srcY,		// 屏幕坐标
                  int tol = 5
                  );


			  //! 海量点云根据屏幕坐标获取相对坐标和从相对到转换模型 -----chy------
			  bool Get3DPosAndModelFromSeaScrPos(
				  PointXYZIPRGBA& ptPoint,
				  CBursaWolfModel& transModel,
				  ESCENE_NODE_TYPE type,  // 场景结点类型   
				  int srcX, int srcY,		// 屏幕坐标
				  int tol = 5
				  );

			  //! 根据屏幕坐标获取相对坐标和从相对到转换模型
			  bool Get3DSelPosAndModelFromScrPos(
				  PointXYZIPRGBA& ptPoint,
				  CBursaWolfModel& transModel,
				  ESCENE_NODE_TYPE type,  // 场景结点类型   
				  int srcX, int srcY,		// 屏幕坐标
				  int tol = 5,    // 容差
				  bool bSelect = true // 是否只操作选择点
				  );

			  //! 根据屏幕坐标获取相对坐标、其行列比例和从相对到绝对的转换模型 [zhangfei 2014/5/23]
			  bool GetPosScaleAndModelFromScrPos(
				  PointXYZIPRGBA& ptPoint,
				  CBursaWolfModel& transModel,
				  ESCENE_NODE_TYPE type,  // 场景结点类型   
				  int srcX, int srcY,		// 屏幕坐标
				  float& scaleX, float& scaleY,	// 在灰度图中的比例
				  int tol = 5
				  );

			//! 根据平面位置获取点云绝对坐标
			bool Get3DPosFromScrPos(
				f64& x,f64& y,f64& z,	// 返回点云坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 5);			// 屏幕容差
			
			//! 根据平面位置获取显示坐标 [2014/03/04 危迟]
			bool Get3DRenderPosFromScrPos(
				f32& x,f32& y,f32& z,   // 返回显示坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 3);			// 屏幕容差


			//! 根据平面位置获取显示坐标(hlz点云数据) [2015/04/14 李夏亮]
			bool Get3DRenderPosFromScrPosSea(
				f32& x,f32& y,f32& z,   // 返回显示坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 3);           // 屏幕容差

			//! 设置相机的位置的目标，使相机能够看到视图中的所有对象
			virtual void ZoomToFullExtent(){}

			//! 根据传入的坐标值获取以该点为中心一定范围内所有点得平均值作返回-zhangfei
			bool GetAverageIntensityFrom3DPos(
				ESCENE_NODE_TYPE type,
				f64& x,f64& y,f64& z,  // 传入的3D坐标，并返回该范围平均值
				float area = 0.1      // 范围值，距离为米  
				);

			//! 根据传入的坐标值获取以该点为中心一定范围内所有点得平均值作返回-zhubo
			bool GetAveragePosFrom3DPos(
				f64& x,f64& y,f64& z,  // 传入的3D坐标，并返回该范围平均值
				float area = 0.1,      // 范围值，距离为米  
				bool bVisiable = true);// 是否要求点云可见,true点云必须可见

			//! 根据传入的坐标值获取以该点为中心一定范围内所有点海量点云得平均值作返回
			bool GetAveragePosFromSea3DPos(
				f64& x,f64& y,f64& z,  // 传入的3D坐标，并返回该范围平均值
				float area = 0.1,      // 范围值，距离为米  
				bool bVisiable = true);// 是否要求点云可见,true点云必须可见

			// ! 判断点是否显示
			bool IsPointVisible(PointXYZIPRGBA& pt);

			//! 获取显示模式
			void SetViewShowStyle(ENUM_SHOWSTYLE style) { m_eShowStyle = style;}

			//! 设置显示模式
			ENUM_SHOWSTYLE GetViewShowStyle() { return m_eShowStyle; }

			// 设置视图投影类型
			void SetViewProjectionType(ENUM_HD_3D_PROJECTION_TYPE type) { m_eProjType = type; } 

			// 获取视图投影类型
			ENUM_HD_3D_PROJECTION_TYPE GetViewProjectionType() { return m_eProjType; }

			//！2013/8/26 蔡红云 加载扫描仪模型
			void SetScanModelVisible( bool bShow );

			//! 2013/8/26 蔡红云 获取扫描仪模型显示状态
			bool IsScanModelVisiable() const{return m_bShowScanModel;}

			//! 2013/9/30 蔡红云 针对点云生成LS300扫描仪模型节点
			IObjectSceneNode* AddLsModel( PointCloud* pcd);

			//!2014/01/16 梁佳 设置是否根据反射强度做透明度渲染
			void ShowIntensityRender(BOOL bShow);

			//! 窗口消息处理
			virtual void WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

			//! 设置点云渲染方式-zhubo
			virtual void SetScanAreaRenderStyle(ENUM_RENDERSTYLE style);
			//! 得到点云的渲染方式
			virtual ENUM_RENDERSTYLE GetScanAreaRenderStyle();

			//! 根据平面位置获取点云-zhubo
			bool Get3DPosFromScrPosForCatch(
				f64& x,f64& y,f64& z,	// 返回点云坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 5,			// 屏幕容差
				bool bVisible = true, bool bSelected = false);	// 是否要求点云可见,true要求可见\被选中的点不处理

			//! 根据平面位置获取点云中的点及其绝对坐标（朱立雄 2017-3-10）
			bool Get3DPosFromPcd(
				PointXYZIPRGBA& pt,
				f64& x, f64& y, f64& z,        // 绝对坐标
				int srcX, int srcY,            // 屏幕坐标
				int tol = 5,
				bool bVisible = true, bool bSelected = false);

			// 根据平面位置获取海量点云坐标-- -- chy
			bool Get3DPosFromSeaPcd(
				PointXYZIPRGBA&pt,       // 相对点
				f64& x,f64& y,f64& z,	// 返回点云坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 5,	// 屏幕容差
				bool bVisible = true, bool bSelected = false
                );

			// 根据平面位置获取临时点云坐标-----chy
			bool Get3DPosFromTmpPcd(
				PointXYZIPRGBA&outPt,       // 相对点
				int srcX,int srcY,		// 屏幕坐标
				int tol = 5	// 屏幕容差
				);
		
			// 根据平面位置获取临时点云坐标-----chy
			bool Get3DPosFromTmpPcd(
				PointXYZIPRGBA&outPt,       // 相对点
			    f64& x,f64& y,f64& z,	// 返回点云坐标值
				int srcX,int srcY,		// 屏幕坐标
				int tol = 5	// 屏幕容差
				);

			// ！获取场景中显示的扫描仪模型数
			int getScnshowcnt();
			
			// ！设置点云外包围盒显示状态 蔡红云 2013/11/7
			void SetboxShowState(bool showbox){m_bShowOutBox = showbox;}
			
			// ! 获取点云外包盒显示状态 蔡红云 2013/11/7
			bool GetboxShowState(){ return m_bShowOutBox;}
			
			// 获取场景中点云的偏移量的最小值 蔡红云 2013/1117
			void getAllScanSnodeExt(f32& offsetx, f32& offsety, f32& offsetz);
			
			// 统计场景中所有点云的按XYZ渲染坐标范围 蔡红云 2013/11/7
			void statAllScanSndeStatCoord();

			// 统计海量点云按XYZ渲染坐标范围 蔡红云 2015/11/7
			void statAllSeaSndeStatCoord();
			
			// 统计视图中所有点云外包盒的范围xyz最小值  蔡红云 2013/11/13
			void StateAllBoxMin(f32& xmin, f32& ymin, f32& zmin);
			
			// 获取按色带循环渲染时，步长、坐标轴、及色带条信息
			void getCyclInfo(f32& step, int& axis, int& cur) 
			{ 
				step = m_cyclstep; 
				cur = m_cyclcur;
				axis = m_cyclaxis;

			}
			// 设置按色带循环渲染时，步长、坐标轴、及色带条信息
			void setCyclInfo(f32 step, int axis, int cur)
			{
				m_cyclstep = step; 
				m_cyclcur = cur;
				m_cyclaxis = axis;
			}

			// 	根据屏幕坐标获取点云相对坐标及绝对坐标[2014/3/22 蔡红云]
			bool Get2PosFromScrPos(PointXYZIPRGBA& ptPoint, f64& x,f64& y,f64& z,ESCENE_NODE_TYPE type, int srcX, int srcY, int tol);

			// 获得视图中点云的抽稀显示状态：没有点云对象、全部显示还是抽稀显示[zf 2014/6/7]
			// 返回 -1：没有点云对象; 0:全部显示；1：抽稀显示
			int GetPcdRenderState();

			// ! 更新点云渲染 fengjing 移植 20140904
			void UpdateIntensityRender();

			// ****获取比例尺 返回分母 -蔡红云[20150325]****
			float GetDisplayScale();

			// ****地理距离到屏幕像素距离转换****
			int GetScreenDist(f32 geoDist);

			//*** 设置是否大数据量切换层显示，在鼠标滚动时进行---蔡红云[20150330]***
			void SetChangeLevl(bool flag);

			// 初始化m_lend
			void InitLend();

			// 获得视图是mongo服务器端视图还是本地视图
			bool isMongoView(){return m_bMongoData;}

			// 设置是否为mongo视图
			void SetMongoView(bool bMongo){m_bMongoData = bMongo;}

			// 是否加载数据
			bool GetCalReLoad(){ return m_bCalReLoad;}
			void SetCalReLoad(bool bflag){m_bCalReLoad = bflag;}

		   // 添加点云 

		public:
			void ISceneView::OnloadTitlePanoData();

			//! 添加一个多边形过滤器到所有点云结点
			void AddPolygonFilter(hd::ptcloud::E_Select_Mode select_type, irr::core::vector2df* screen_pts, int pt_count);

			//! 添加一个圆形过滤器到所有点云结点
			void AddCircleFilter(hd::ptcloud::E_Select_Mode select_type, irr::core::vector2df center, f32 radius);

			//! 移除所有点云结点的所有过滤器
			void ClearFilters();

			//! 对所有点云节点进行过滤，返回选中点数
			hd::u32 updateForFilter();

			////! 获取过滤管理器
			//ptcloud::hdFilterManager* getFilterManager(){return &m_filter_manager;}
		};

		//! 初始化鬼火绘图设备
		IrrlichtDevice* InitialIrrDevice(HWND hwnd);

	}
}