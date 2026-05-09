/*! LasMeshSceneNode.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : CScanSceneNode.h
相关文件     : 
文件实现功能 : 实现三维激光点云文件的渲染 
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/18   1.0      龚书林    
2013/02/28   1.01	  危迟
2013/06/23   1.02     龚书林			  优化渲染效率和效果
2013/08/27   1.03     朱旭波              增加选中点区域进行渲染（默认、X、Y、Z、I）
                                          根据系统设置修改选中点默认渲染颜色
2014/07/09   1.04     朱旭波              采用多线程添加场景节点时，为防止出现死锁现象，
                                          在SetPointCloud处加锁，点云render处也加锁（对
										  话框消失时会调用WM_PAINT进行刷新）
</PRE>
*******************************************************************************/
#pragma once
//#ifndef _C_SCAN_SCENE_NODE_H_
//#define _C_SCAN_SCENE_NODE_H_
#include "stdafx.h"
#include "..\hd3Dengine\include\irrlicht.h"
#include "..\hd3Dengine\include\driverChoice.h"
#include "..\hdPointCloud\point_cloud.h"
//#include "..\hdCore\hdColor.h"
#include "..\hdPointCloud\DistanceImg.h"
#include "..\hdCommon\hdTin.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "IObjectSceneNode.h"
#include "ISceneView.h"
#include "..\..\hdCommon\CClassificationMap.h"
#include "..\hdCore\Hd3dBoxEx.h"
#include "..\hdCore\HdobBox3d.h"
#include "..\hdPointCloud\hdFilterManager.h"

using namespace irr;
using namespace irr::video;
using namespace hd;

namespace hd
{	
	namespace scene
	{
		union RenderColor
		{
			RenderColor()
				:c_color(0){}
			struct
			{
				hd::u8 b;
				hd::u8 g;
				hd::u8 r;
				hd::u8 a;	
			};
			hd::u32 c_color;
		};

		class HD3DSCENE_API CScanSceneNode : public IObjectSceneNode
		{
			//属性
		private:
		protected:
			//! 点云数据
			PointCloud*	m_pointCloud;
			// 代码锁
			CRITICAL_SECTION m_cs;
			//! 渲染颜色
			hdVector<hdVector<RenderColor>>	m_renderColors;
			//! 点云中的靶球
			std::vector<PointXYZI> m_smrPts;
			//! 材质,每个SceneNode必须包含此对象
			video::SMaterial	m_material;
			//! 点云块外包范围
			core::aabbox3d<f32> m_box;
			//! 点云指定范围  fengjing
			core::aabbox3d<f32> m_SpecialBox;
			//! 点云显示分类
			ENUM_POINTCLASS m_renderCls;
			//! 点云符号化显示方式
			ENUM_RENDERSTYLE m_renderStyle;
			//! 点云显示方式
			ENUM_SHOWSTYLE   m_showStyle;
			//! 是否显示boundingbox
			BOOL		m_renderBBox;
			//! 是否显示指定范围的box fengjing
			BOOL		m_bRenderSBox;
			//! 是否显示反射强度组成的透明度
			BOOL        m_bShowIntenRender;
			//! 抽样比例
			u32			m_simple;
			//! 移动漫游过程显示的最多点数，用于动态抽稀显示
			u32			m_simpleCount;
			//! 抽样随机数
			u32			m_seed;
			//! 最大反射值的倒数,用乘法计算灰度灰度颜色,加速计算
			float       m_maxIntensity_rcp;
			//  最大反射强度值
			int		m_MaxIntensity;
			//! 最小反射强度值
			int     m_MinIntensity;
			//! 渲染颜色条
			CHdColorRamp   m_colorRampByCol;
			CHdColorRamp   m_colorRampZ;
			CHdColorRamp   m_colorRampCycle;
			//! 渲染透明度
			float		m_transparence;
			//! 默认颜色
			RenderColor	m_defaultClr;
			//! 深度图运算
			CDistanceImg m_distImg;
	
			u32 m_PointsizeBtn;
	
			BOOL m_SubSample;
			//! 高度分布,每个高度范围点云个数相同
			vector<float>  m_heightStep;
			//! X分布
			vector<float>  m_xStep;
			//! Y分布
			vector<float>  m_yStep;
			//! 点云数据是否改变
			bool		m_bPcdChanged;
			//! ISceneView
			//ISceneView* m_pSceneView;

			// 是否需要进行坐标转换
			bool m_bTrans;
			//渲染次数计数
			int m_RenderCount;
			//! 反射强度渲染加强，将最大最小值缩小
			int m_nMinIntensity;
			int m_nMaxIntensity;
			//! 按列渲染选择列的最大最小值
			int m_nMinCol;
			int m_nMaxCol;
			//! 设置按Z渲染的最大最小值
			float m_fMinHeight;
			float m_fMaxHeight;
			//! 设置按X渲染的最大最小值
			float m_fMinCorX;
			float m_fMaxCorX;
			//! 设置按Y渲染的最大最小值
			float m_fMinCorY;
			float m_fMaxCorY;

			//! 区域渲染对应的XYZ最大最小值
			float m_fAreaMinHeight;
			float m_fAreaMaxHeight;
			float m_fAreaMinX;
			float m_fAreaMaxX;
			float m_fAreaMinY;
			float m_fAreaMaxY;
			float m_fAreaMinCoord;
			float m_fAreaMaxCoord;

			//! 按坐标渲染的变量最大\最小\步长
			float m_fMinCoord;
			float m_fMaxCoord;
			float m_fStep;
			//! 特征阈值 (0-255)
			int m_nFeatureThreshold;
			//! 真实的特征阈值 
			float m_dFeatureThreshold;
			//! 特征类型 0-强度 1-高程 2-投影特征
			int m_nFeatureTypeIndex;
			//! 记录当前内存点云,每圈是否在视图内
			std::vector<u8> m_vecInView;
			//! 记录当前内存点云,在视图内点个数
			u64	m_countInView;
		    //! 循环渲染步长
			float m_fCycleStep;
			//! 循环渲染基准坐标轴
			hd::u32 m_nAxis;
			//! 记录区域渲染的点个数，避免二次渲染同样的数据点云
			u32  m_nCurSelCount;
			//! 由于通用x、y、z步长，需要标记0是否进行过区域渲染，再次切换至全部按高程等渲染时，需要重新统计计算
			bool m_bAreaRender;
			//! 区域渲染点云符号化显示方式
			ENUM_RENDERSTYLE m_renderAreaStyle;
			// 按类别进行渲染时。存储类别颜色
			COLORREF m_sClassColor[256];
			// 记录系统配置的选中点颜色
			COLORREF m_selPtColor;
			// 记录渲染最大最小距离值
			float m_fMinDist;
			float m_fMaxDist;
			// 记录视图中CScanSceneNode节点个数
			int m_scant;
			// 标记是否需要重新统计坐标
			bool m_isStateCoord;

			// 点云的亮度值拉伸倍数 1~255 默认255 渲染设置可以对其进行设置 fengjing 2014/07/16
			int m_ColoraSetting;

			// 是否在浏览模式下锁定当前视图内的点云[zf 2014/6/7]
			bool m_bLockMemoryPts;

            // 旋转平移是否刷新点云[张阳 2016/04/13]
            bool m_bIsRender;

            // 旋转中心
            // core::vector3df m_rotCenter;

            // 旋转角度
            // core::vector3df m_rotDegrees;

		public:	
			// 按类别进行进行渲染时，类别对应的颜色索引值
			unsigned int m_nColorIndex[256];
			//// 设定渲染的类别的索引
			//bool m_bRenderClassIndex[256];
			//// 是否为渲染设置
			//bool m_bIsRenderSetting;
			//! 统计点的坐标分布区段m_heightStep,m_xStep,m_xStep,使得每个区段的点个数大致相同
			void StatCoord();
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
			// ！设置特定包围盒的范围
			/*void SetBoundbox(BOOL IsShowBox,core::aabbox3d<f32> box);*/

            void SetRender(bool isRender) { m_bIsRender = isRender; }

			//! 根据Map视图范围更新3D视图
			void UpdateExtent(const hd::CHd3dBoxEx extent);

			//! 根据box范围更新3D视图  [zhangfei 2014/3/11]
			void UpdateExtent(const core::aabbox3d<f32> box);

			// 设置点云的亮度值拉伸倍数 fengjing
			void SetColorASetting(int colorA) { m_ColoraSetting = colorA; }

			// 获取点云的亮度值拉伸倍数 fengjing
			int GetColorASetting() { return  m_ColoraSetting; }

			//! 重新统计点的反射强度值 fengjing 移植 20140904
			void ReStatIntensity();

            //// 获取旋转中心
            //core::vector3df GetRotateCenter() { return m_rotCenter; }

            //// 设置旋转中心
            //void SetRotateCenter(core::vector3df rotCenter) { m_rotCenter = rotCenter; }

            //// 获取旋转中心
            //core::vector3df GetRotateDegrees() { return m_rotDegrees; }

            //// 设置旋转中心
            //void SetRotateDegrees(core::vector3df rotDegrees) { m_rotDegrees = rotDegrees; }
		private:
		protected:
		
			//! 渲染指定点
			virtual void RenderPoint(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color,int selPoint = 0);
			//! 渲染指定点 为满足实时渲染要求 加快速度 不同特征点渲染函数分别实现 减少每个点的判断操作
			void RenderPointByIntensityFeature(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color);
			void RenderPointByHeightFeature(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color);
			void RenderPointByProjFeature(const PointXYZIPRGBA& pt,int feature, float boundingBoxHeight,const RenderColor& color);
			void RenderPointByAngleFeature(const PointXYZIPRGBA& pt,float feature,float boundingBoxHeight,const RenderColor& color);
			
			//! 计算按强度渲染颜色
			virtual void CalcuIntensityRender();

			
			//! 计算按坐标轴渲染颜色
			virtual void CalcuCoordRender();
			//! 计算按照列渲染颜色
			void CalcuColRender();
			
			//! 计算按照距离渲染颜色
			void CalcuDistRender();

			//! 计算根据轨迹线范围渲染颜色
			void CalcuDistToCenterRender();
			
			//! 计算按照循环色带渲染颜色
			void CalcuCycleRampRender();

			//! 计算渲染颜色
			void CalcuRenderColor();

			//! 计算当前视图范围内坐标范围
			void CalcuCoordRenderView(int renderSimpleCol,int renderSimpleRow);
		
			//! 统计点的反射强度值分布
			void StatIntensity();
		
			//! 统计视图范围内点个数,vecInVview记录每圈是否在范围内,返回视图内点数
			virtual u64 GetCountInView();

			//! 精确统计所有在视图范围内的点，而不是根据上述的由圈相交获得，返回视图内点数
			u64 GetTotalCountInView();

			//! 获得视锥的obb box
			void GetObbBoxByView(CHdobBox3d& viewObbox);

			//! 通过两个obb box相交判断点云圈是否与视锥相交的方式统计视图内总有效点数
			u64 GetCountInViewByObbBox();

			//! 判断一个点是否在视图范围内，返回true表示在其中
			bool isPtInViewBox(const PointXYZIPRGBA& pt);
			
			//! 统计局部坐标点到零点的距离分布
			void StatDistance();

			//! 统计局部坐标点到每圈扫描圈中心的距离分布（仍使用变量m_fMinDist,m_fMaxDist）
			void StatDistanceToLoopCenter();

			//! 统计选中区域点的坐标范围
			void StatAreaCoord();
			//! 计算按区域渲染X、Y、Z颜色
			void CalcuAreaCoordRender();



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
			virtual ESCENE_NODE_TYPE getType() const { return ESNT_SCAN_POINT; }


		/************************************************************************/
		/*                   CScanSceneNode接口实现                          */
		/************************************************************************/
		public:
			//CLasVisualization();
			CScanSceneNode(irr::scene::ISceneNode* parent,irr::scene::ISceneManager* mgr,s32 id);
			~CScanSceneNode(void);
			//! 根据视口重新加载数据
			BOOL ReloadData();
			
			//! 设置点云数据
			BOOL SetPointCloud(PointCloud* pcd);

			//! 设置点云数据
			BOOL SetPointCloud(PointCloud* pcd,CBursaWolfModel model);
			
			//! 获取点云数据
			PointCloud* GetPointCloud(){return m_pointCloud;}
			
			//! 设置点云文件显示类型
			void SetRenderClass(const ENUM_POINTCLASS& renderCls);
			
			//! 获取点云文件显示类型
			ENUM_POINTCLASS GetRenderClass() const;
			
			//! 设置点云渲染方式
			virtual void SetRenderStyle(const ENUM_RENDERSTYLE& style);
			
			//! 获取点云渲染方式
			ENUM_RENDERSTYLE GetRenderStyle() const;
			
			//! 设置显示方式
			void SetShowStyle(const ENUM_SHOWSTYLE eStyle);
			
			//! 获取显示方式
			ENUM_SHOWSTYLE GetShowStyle() const;
			
			//! 设置点显示大小
			void SetPointSize(f32 size);
			
			//! 获取点显示大小
			f32 GetPointSize() const;
			
			//! 获取点个数
			u32 GetPointCount() const;
			
			//! 获取透明度
			f32 GetTransparence() const;
			
			//! 设置抽样比例
			void SetSimple(u32 simple);
			
			//! 获取抽样比例
			u32 GetSimple() const;
			
			//! 设置抽稀后点数
			void SetSimpleCount(u32 simpleCount){m_simpleCount = simpleCount;}
			
			//! 获取抽稀后点数
			u32 GetSimpleCount() const;

			//! 设置最大最小反射强度
			void SetMaxMinIntensity(int nMaxIntensity, int nMinIntensity);

			//! 得到最大最小反射强度
			void GetMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity);
			
			//! 得到实际最大最小反射强度
			void GetRealMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity);

			// ! 得到渲染拉伸的最大最小值
			void GetRenderMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity);

			//! 获取默认颜色
			SColorf GetDefaultColor() const
			{ 
				return SColorf(m_defaultClr.r / 255.0f,m_defaultClr.g / 255.0f,
								m_defaultClr.b / 255.0f,m_defaultClr.a / 255.0f); 
			}
			
			//! 设置默认颜色
			void SetDefaultColor(const SColorf& clr);

			//! 设置透明度
			void SetTransparence(f32 transparence)
			{
				if(transparence >= 0.0 && transparence <= 1.0)
					m_transparence = transparence;
			}
			
			u32 GetPointSizeBtn() const { return m_PointsizeBtn; }

			void SetPointSizeBtn(u32 PointsizeBtn) { m_PointsizeBtn = PointsizeBtn; }

			BOOL GetSubSample() const { return m_SubSample; }

			void SetSubSample(BOOL SubSample) { m_SubSample = SubSample;} 

			//! 设置按列渲染颜色
			void SetDefaultColorByCol(int cursel);
			
			void SetMaxColForColor(int maxCol){m_nMaxCol = maxCol;}
			void SetMinColForColor(int minCol){m_nMinCol = minCol;}
			
			//! 获得按列渲染颜色
			CHdColorRamp GetDefaultColorByCol() const{return m_colorRampByCol;}
			int GetMaxColForColor(){return m_nMaxCol;}
			int GetMinColForColor(){return m_nMinCol;}
			
			//　设置类别颜色索引
			void SetColorIndex(unsigned int colorArr[],int arrLength);
			
			// 设置类别颜色值 fengjing
			void SetClassColorValue(COLORREF ClassColor[], int arrlength);

			// 根据鼠标选择的范围设置
			void SetDefaultColorByZ(int cursel);
			
			const CHdColorRamp& GetColorRampZ(){return m_colorRampZ;}

			//! 设置boundingbox绘制
			void ShowBoundingBox(BOOL bShow);
			
			//! 是否绘制BoundingBox
			BOOL IsShowBoundingBox() { return m_renderBBox; }

			//! 设置是否根据反射强度做透明度渲染
			void ShowIntensityRender(BOOL bShow);

			//! 是否绘制透明度渲染
			BOOL IsShowIntensityRender(){ return m_bShowIntenRender; }

			bool IsTrans(){return m_bTrans;}

			void SetTrans(bool bTrans) { m_bTrans = bTrans; }

			//! 设置特征阈值 当前版本暂时设定为float型数值
			void SetFeatureThreshold(int thred) { m_nFeatureThreshold = thred; }

			//! 获取特征阈值
			int GetFeatureThreshold() { return m_nFeatureThreshold; }

			//! 设置特征索引
			void SetFeatureIndex(int fIdx) { m_nFeatureTypeIndex = fIdx; }

			//! 获取特征索引
			int GetFeatureIndex() { return m_nFeatureTypeIndex; }
			////! 设置所有的点均在滑动分类时渲染
			//void SetRenderAllOnSlider(bool bAll) { m_bRenderAllSlider = bAll; }

			////! 获取是否所有的点均在滑动分类时渲染
			//bool GetRenderAllOnSlider() { return m_bRenderAllSlider; }
			
			//! 获取渲染的最大最小高程值
			void GetMaxMinZ(float& nMaxZ, float& nMinZ);

			//! 获取当前视图范围点云质心
			BOOL GetViewCenter(float& cx,float& cy,float& cz);

			//! 在车载全景影像定位时,根据与pos的距离查询设置需要显示的点云,xyz和点云显示坐标系一致
			void QueryByHDI(float cx,float cy,float cz,float dist);

			//! 车载影像定位根据lin文件每圈时间在HDI两帧影像间设置需要显示的点云,只显示在startLoop-endLoop之间点云
			void QueryByTime(double scaleStart,double scaleEnd);

			//! 获取旋转中心点,用屏幕中心点射线,求离射线最近点
			float GetRotCenter(float& cx,float& cy,float& cz,int srcX,int srcY);

			//! 设置循环渲染步长
			void SetCycleStep(float step) { m_fCycleStep = step; }

			//! 获取循环渲染步长
			float GetCycleStep() { return m_fCycleStep; }

			//! 设置循环渲染基准坐标轴
			void SetCycleAxis(int axis) { m_nAxis = axis; }

			//! 获取循环渲染基准坐标轴
			int GetCycleAxis() { return m_nAxis; }

			// 设置循环渲染色带起始颜色
			void SetDefaultColorByCycle(int cursel);

			// 获取循环渲染色带
			const CHdColorRamp& GetColorRampCycle() { return m_colorRampCycle; }

			// 判断某一圈是否在视图内
			bool isLoopInView(int n);

			//! 设置选中区域点云渲染方式
			void SetAreaRenderStyle(const ENUM_RENDERSTYLE& style);

			//! 获取点云渲染方式
			ENUM_RENDERSTYLE GetAreaRenderStyle() const;

			//! 按XYZ选择时获取该渲染方式的最大最小值，如按X渲染，获取的值为minX，maxX
			void GetMinMaxValue(float& maxValue, float& minValue);
			
			// 重置渲染次数
			void ResetRenderCount(int n) { m_RenderCount = n; }

			// 设置放大镜窗口显示的点云[zhangfei 2014/3/3]
			//void Set3DZoomWndPcd(PointCloud* pPcd);

			// 根据屏幕坐标获取点云坐标 
			// 函数返回最近的距离值 
			unsigned int Get3DPosFromScrPos(
				PointXYZIPRGBA& ptPoint,	  // 返回的相对坐标值
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly        // 是否查找未显示的点
				);

			unsigned int Get3DPosFromScrPos(
				PointXYZIPRGBA& ptPoint,	  // 返回的相对坐标值
				f64& x,f64& y,f64& z,         // 返回的绝对坐标值
				int srcX,int srcY,			  // 屏幕坐标
				int tol,					  // 屏幕查找范围
				bool bFindVisibleOnly,      // 是否查找未显示的点
				bool bSelectPt // 是否只查找选中的点
				);

			core::matrix4 m_IrrAbsMat;
			bool m_bUseMat;

			// 设置场景节点绝对位置 鬼火世界坐标系
	        void SetAbsoluteTransformation(core::matrix4 mat,bool useMat)
            {
                m_IrrAbsMat = mat;
                m_bUseMat = useMat;
            }

            void RefreshRenderModel(CBursaWolfModel& absModel);

			bool GetCurrentRenderTransformation(core::matrix4& mat)
			{
				if (m_bUseMat)
				{
					mat = m_IrrAbsMat;
				}

				return m_bUseMat;
			}

			core::vector3dd m_AdjustOft;		// 平移调整偏移量
			core::vector3dd m_AdjustRot;		// 旋转调整偏移量

			// 设置测站调整坐标平移量 
			void SetAdjustOffset(double oftX,double oftY,double oftZ) { m_AdjustOft.set(oftX,oftY,oftZ); }

			// 设置测站调整坐标平移量
			void SetAdjustOffset(core::vector3dd& oft) { m_AdjustOft = oft; }

			// 获取测站调整坐标平移量
			void GetAdjustOffset(core::vector3dd& oft) { oft = m_AdjustOft; }

			// 设置测站旋转角度值
			void SetAdjustRotateAngle(double angleX,double angleY,double angleZ) { m_AdjustRot.set(angleX,angleY,angleZ);}

			// 设置测站旋转角度值
			void SetAdjustRotateAngle(core::vector3dd& rotAngle) { m_AdjustRot = rotAngle; }

			// 获取测站旋转角度值
			void GetAdjustRotateAngle(core::vector3dd& rotAngle) { rotAngle = m_AdjustRot; }

			// 获得点云行抽稀的比例(>=1)，没有抽稀时为0[zhangfei 2014/6/7]
			int GetRenderSimpleLevel();

			// 在浏览模式下锁定当前视图中的点云
			void LockCurrentMemoryPcd(bool bLock){m_bLockMemoryPts = bLock;}

			// 获得是否锁定了当前视图内的点云
			bool IsLockCurrentMemoryPcd(){return m_bLockMemoryPts;}

			// 根据点云内存中存在圈更新m_vecInView
			void UpdateVecInView();

			//! 设置过滤类型以刷新显示
			hd::u32 updateForFilter();

			//! 重新渲染数据
			void reRenderStyle();

			};

	}// end of namespace scene
}// end of namespace hd

//#endif