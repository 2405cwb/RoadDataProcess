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
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "HdTinSceneNode.h"
#include "..\hdCore\hdMath.h"
#include "..\hdCommon\hdSceneStr.h"
#include "CFPSSceneNode.h"
#include "hd3DView.h"
#include <time.h>

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

		// 构造
		CHdTinSceneNode::CHdTinSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			: IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id),
			m_nIndicesToRender(0),m_colorRampZ(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_colorRampCycle(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_nAxis(0),
			m_fCycleStep(5.f),
			m_ZRampIndex(4),
			m_CycleRampIndex(4),
			m_renderStyle(RENDER_DEM_BY_WIREFRAME_Z),
			m_bShowBoundBox(FALSE),
			m_bIsCacuIndices(FALSE),
			m_nLightShiness(30.0f)
		{
			// 初始化TIN数据结构
			m_TINPcd = NULL;

			// 初始化代码锁
			::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000409 );

			m_material.Lighting = false;			// 光照
			m_material.Thickness = 2;				// 大小 为2 
			m_material.ZWriteEnable = true;			// 渲染buff可读写
			m_material.Wireframe = true;			// 线框渲染
			m_material.PointCloud = false;			// 点云渲染
			m_material.BackfaceCulling = false;
			m_material.FrontfaceCulling = false;
			m_material.ColorMaterial = ECM_DIFFUSE_AND_AMBIENT;

			m_selColor = RGB(0,0,255);			    // 单色渲染 	

			setAutomaticCulling(irr::scene::EAC_OFF);

			// 构造时关闭光照
			m_bLightOn = false;
		}

		// 析构
		CHdTinSceneNode::~CHdTinSceneNode(void)
		{
			// 释放代码锁
			::DeleteCriticalSection(&m_cs);
		}

		// 根据视口重新加载数据
		BOOL CHdTinSceneNode::ReloadData()
		{
			// 判空
			if(m_pView == NULL || m_TINPcd == NULL)
			{
				return FALSE;
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 计算顶点索引
			preRenderIndicesCalculations();

			LeaveCriticalSection(&m_cs);
			return 1;
		}

		void CHdTinSceneNode::OnRegisterSceneNode()
		{
			if (!IsVisible || !SceneManager->getActiveCamera())
				return;

			// 注册渲染
			SceneManager->registerNodeForRendering(this);

			// 渲染之前实时统计渲染的三角形
		    // preRenderIndicesCalculations();

			ISceneNode::OnRegisterSceneNode();
		}

		void CHdTinSceneNode::ClearAllColors()
		{
			ITexture* pText = m_material.getTexture(0);
			if (pText)
			{
				// 使用纹理坐标时，顶点颜色置为白色（不对纹理坐标造成影响）
				vector<S3DVertex2TCoords>& vertex = m_TINPcd->getVertexBuffer();
				for (unsigned int i = 0;i < vertex.size();i++)
				{
					S3DVertex2TCoords& ptCoord = vertex.at(i);
					ptCoord.Color = SColor(255,255,255,255);
				}
			}
		}

		// 渲染
		void CHdTinSceneNode::render()
		{
			// 判空
			if (!IsVisible || !SceneManager || !SceneManager->getActiveCamera() || !m_TINPcd)
			{
				return;
			}

			// 获取the video driver
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if (!driver)
			{
				return;
			}

			// 设置transformation
			driver->setTransform (video::ETS_WORLD, core::IdentityMatrix);

			// 根据渲染方式对材质参数进行设置
			if (m_renderStyle == RENDER_DEM_BY_ENTITY_ACOLOR ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_CYCLERAMP ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_Z)
			{
				// 实体
				m_material.Wireframe = false;
				m_material.PointCloud = false;
			}
			else if (m_renderStyle == RENDER_DEM_BY_WIREFRAME_ACOLOR ||
				m_renderStyle == RENDER_DEM_BY_WIREFRAME_CYCLERAMP ||
				m_renderStyle == RENDER_DEM_BY_WIREFRAME_Z)
			{
				// 线框
				m_material.Wireframe = true;
				m_material.PointCloud = false;
			}
			else
			{
				// 点云
				m_material.Wireframe = false;
				m_material.PointCloud = true;
			}

			if (m_bLightOn)
			{
				// 将光照打开,包含直射光、环境光
				m_material.Lighting = true;
				m_material.ColorMaterial = ECM_DIFFUSE_AND_AMBIENT;
				m_material.BackfaceCulling = false;

				// 单位化法向量
				m_material.NormalizeNormals = true;

				// 添加光照阴影，显示立体效果
				m_material.NormalizeNormals = true;
				m_material.GouraudShading = true;

				// 设置镜面光颜色，可增加显示的亮度
				m_material.SpecularColor.set(150,150,150,150);

				// 设置镜面反射度，显示立体化
				m_material.Shininess = m_nLightShiness;
			}
			else
			{
				// 将光照关闭
				m_material.Lighting = false;
				m_material.ColorMaterial = ECM_DIFFUSE_AND_AMBIENT;
				m_material.BackfaceCulling = false;
			}

			// 设置纹理
			driver->setMaterial(m_material);

			// 渲染
			driver->drawVertexPrimitiveList(m_TINPcd->getVertexBuffer()._Myfirst(), (u32)(m_TINPcd->count()), m_CurRenderIndices._Myfirst(), m_CurRenderIndices.size()/3, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_32BIT);

			// 绘制包围盒
			if (m_bShowBoundBox)
			{
				driver->draw3DBox(m_box);
			}
#ifdef _DEBUG

			// 渲染选中包围盒
			if (m_indexSelBuffer.size() > 0)
			{
				driver->drawVertexPrimitiveList(m_TINPcd->getVertexBuffer()._Myfirst(), (u32)m_TINPcd->count(), m_indexSelBuffer._Myfirst(),m_indexSelBuffer.size()/3, EVT_2TCOORDS,scene::EPT_TRIANGLES,EIT_32BIT);
			}

			//// 渲染选中三角形
			////driver->drawVertexPrimitiveList(m_modelPCD->getVertexBuffer()._Myfirst, m_DemData.hSize*m_DemData.wSize, m_nSelRenderTri._Myfirst, m_nSelCount/3, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_32BIT);
			//
			//// 绘制包围盒
			//if (1)
			//{
			//	driver->draw3DBox(m_box);
			//}

			//vector<S3DVertex2TCoords>	pts;
			//pts.resize(4);

			//video::S3DVertex2TCoords& vertex1 = pts[0];
			//vertex1.Normal.set(0.0f, 1.0f, 0.0f);
			//vertex1.Color = SColor(100,255,255,0);
			//vertex1.Pos = m_box.MinEdge;
			//vertex1.Pos.Z = m_box.getCenter().Z;

			//video::S3DVertex2TCoords& vertex2 = pts[1];
			//vertex2.Normal.set(0.0f, 1.0f, 0.0f);
			//vertex2.Color = SColor(100,255,255,0);
			//vertex2.Pos = m_box.MinEdge;
			//vertex2.Pos.Y = m_box.MaxEdge.Y;
			//vertex2.Pos.Z = m_box.getCenter().Z;

			//video::S3DVertex2TCoords& vertex3 = pts[2];
			//vertex3.Normal.set(0.0f, 1.0f, 0.0f);
			//vertex3.Color = SColor(100,255,255,0);
			//vertex3.Pos = m_box.MinEdge;
			//vertex3.Pos.X = m_box.MaxEdge.X;
			//vertex3.Pos.Y = m_box.MaxEdge.Y;
			//vertex3.Pos.Z = m_box.getCenter().Z;

			//video::S3DVertex2TCoords& vertex4 = pts[3];
			//vertex4.Normal.set(0.0f, 1.0f, 0.0f);
			//vertex4.Color = SColor(100,255,255,0);
			//vertex4.Pos = m_box.MinEdge;
			//vertex4.Pos.X = m_box.MaxEdge.X;
			//vertex4.Pos.Z = m_box.getCenter().Z;

			//vector<u32>	CurRenderIndices; 
			//for (u32 i = 0; i< 4;i++)
			//{
			//	CurRenderIndices.push_back(i);
			//}

			////m_material.MaterialType = EMT_TRANSPARENT_VERTEX_ALPHA;
			//// 设置材质
			////driver->setMaterial(m_material);
			//m_material.Wireframe = false;
			//m_material.PointCloud = false;
			////driver->setMaterial(m_material);
			//// 测试绘制四边形
			////driver->drawVertexPrimitiveList(pts._Myfirst,4,CurRenderIndices._Myfirst,1,EVT_2TCOORDS,EPT_QUADS,EIT_32BIT);

			// 绘制断面线
			/*	int lineCount = m_ProfilePts.size()/2;

			for (int i = 0; i< lineCount;i++)
			{
			driver->draw3DLine(m_ProfilePts[i*2],m_ProfilePts[i*2 + 1]);
			}*/
#endif
		}

		// 加载TIN 数据
		void CHdTinSceneNode::SetTinData(CHdTINPointCloud* pcd)
		{
			// 判空
			if (!pcd)
			{
				return;
			}

			// 指向内存中的tin
			m_TINPcd = pcd;

			// 获取点云的数量
			const u64 count = pcd->count();

			// 获取模型的包围盒
			m_box = pcd->getBoundingBox();

			// 统计坐标信息，为顶点颜色赋值
			StatCoord(pcd);

			// 统计颜色参数
			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			int selStep = 0;
			float tempValue = 0.0f;
			float h = 0.0f;

			// 默认按Z值渲染色带的起止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组;

			// 遍历点云 统计颜色
			for (u32 i = 0;i < count;i++)
			{
				video::S3DVertex2TCoords& vertex = modelpcd[i];	

				// 存储点的颜色值
				h = vertex.Pos.Z;

				RenderColor color;

				if ( h < m_fMinCoord)
				{
					color.r = ((beginColor>>16) & 0xff);
					color.g = ((beginColor>>8) & 0xff);
					color.b = (beginColor & 0xff);
				}
				else if (h > m_fMaxCoord)
				{
					color.r = ((endColor>>16) & 0xff);
					color.g = ((endColor>>8) & 0xff);
					color.b = (endColor & 0xff);
				}
				else
				{
					selStep = (int)((h - m_fMinCoord) / m_fStep);

					tempValue = h - m_fMinCoord - selStep * m_fStep;
					scale = tempValue / m_fStep;
					m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
				}
				vertex.Color = SColor(255,color.r,color.g,color.b);
			}

			// 当前渲染三角形
			m_CurRenderIndices = m_TINPcd->GetRenderIndex();

			// 渲染三角形定点索引总数
			m_nIndicesToRender = m_CurRenderIndices.size();

			// 计数索引
			// preRenderIndicesCalculations();

			//preRenderProfileLineCalculations();;
		}

		// 统计坐标值
		void CHdTinSceneNode::StatCoord(PointCloud* pcd)
		{
			//float maxHeight = F32_MIN;
			//float minHeight = F32_MAX;
			//int selMax = 0;
			//int selMin = 0;
			//int mapHeight[1000];
			//int selNum = 0;
			//float h = 0.0f;
			//float height = 0.0f;

			//unsigned long long count = pcd->count();
			//U32 validCount = pcd->getValidCount();
			//
			//maxHeight = pcd->m_simpleHeader.max_z;
			//minHeight = pcd->m_simpleHeader.min_z;

			////统计高度分布，映射至0-999
			//memset(mapHeight,0,sizeof(int)*1000);

			//float boxHeight = maxHeight - minHeight;
			//float boxHeightRcp = 1000.0f / (boxHeight);

			//for (s32 i = 0; i < count; ++i)
			//{
			//	const PointXYZIPRGBA& pt = (*pcd)[i];
			//	if (pt.isValid())
			//	{
			//		height = pt.z;
			//		selNum = (int)((height - minHeight) * boxHeightRcp);
			//		selNum = clamp(selNum,0,999);
			//		mapHeight[selNum]++;
			//	}
			//}				

			////调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
			//selNum = 0;
			//for (int j = 999;j >= 0;j--)
			//{
			//	selNum += mapHeight[j];
			//	if (selNum >= (int)(validCount * 0.03))
			//	{
			//		selMax = j;
			//		break;
			//	}
			//}

			//selNum = 0;
			//for (int j = 0;j <= 999;j++)
			//{
			//	selNum += mapHeight[j];
			//	if (selNum >= (int)(validCount * 0.03))
			//	{
			//		selMin = j;
			//		break;
			//	}
			//}

			////重新设置新的最大最小高程值
			//m_fMinCoord = minHeight + selMin * (boxHeight) / 1000.0f;
			//m_fMaxCoord = minHeight + selMax * (boxHeight) / 1000.0f;

			//m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;
		}

		// 统计坐标值
		void CHdTinSceneNode::StatCoord(CHdTINPointCloud* pcd)
		{
			float maxHeight = F32_MIN;
			float minHeight = F32_MAX;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;
			float h = 0.0f;
			float height = 0.0f;

			// 有效点个数
			unsigned long long count = pcd->count();
			//U32 validCount = pcd->getValidCount();

			maxHeight = m_box.MaxEdge.Z;
			minHeight = m_box.MinEdge.Z;

			//统计高度分布，映射至0-999
			memset(mapHeight,0,sizeof(int)*1000);

			float boxHeight = maxHeight - minHeight;
			float boxHeightRcp = 1000.0f / (boxHeight);

			vector<S3DVertex2TCoords>& modelPCD = pcd->getVertexBuffer();		// 获取点云
			for (s32 i = 0; i < count; ++i)
			{
				const S3DVertex2TCoords& pt = modelPCD[i];

				height = pt.Pos.Z;
				selNum = (int)((height - minHeight) * boxHeightRcp);
				selNum = clamp(selNum,0,999);
				mapHeight[selNum]++;

			}				

			//调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
			selNum = 0;
			for (int j = 999;j >= 0;j--)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(count * 0.03))
				{
					selMax = j;
					break;
				}
			}

			selNum = 0;
			for (int j = 0;j <= 999;j++)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(count * 0.03))
				{
					selMin = j;
					break;
				}
			}

			//重新设置新的最大最小高程值
			m_fMinCoord = minHeight + selMin * (boxHeight) / 1000.0f;
			m_fMaxCoord = minHeight + selMax * (boxHeight) / 1000.0f;

			m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;
		}

		// 获取包围盒
		const core::aabbox3d<f32>& CHdTinSceneNode::getBoundingBox() const
		{
			return m_box;
		}

		// 获取材质
		video::SMaterial& CHdTinSceneNode::getMaterial(u32 i)
		{
			return m_material;
		}

		// 设置材质
		void CHdTinSceneNode::SetMaterial(SMaterial mat)
		{
			m_material = mat;
		}

		// 获取材质数量
		u32 CHdTinSceneNode::getMaterialCount() const
		{
			return 1;
		}

		// 设置渲染方式
		void CHdTinSceneNode::SetModelRenderStyle(ENUM_DEM_RENDERSTYLE style )
		{
			// 判空
			if (m_renderStyle == style && ( style != RENDER_DEM_BY_ENTITY_ACOLOR &&
				style != RENDER_DEM_BY_ENTITY_CYCLERAMP && style != RENDER_DEM_BY_ENTITY_Z &&
				style != RENDER_DEM_BY_POINTCLOUD_ACOLOR && style != RENDER_DEM_BY_POINTCLOUD_CYCLERAMP &&
				style != RENDER_DEM_BY_POINTCLOUD_Z && style != RENDER_DEM_BY_WIREFRAME_ACOLOR &&
				style != RENDER_DEM_BY_WIREFRAME_CYCLERAMP && style != RENDER_DEM_BY_WIREFRAME_Z))
			{
				return;
			}

			// 设置渲染方式
			m_renderStyle = style;

			// 按Z值渲染
			if (style == RENDER_DEM_BY_ENTITY_Z || style == RENDER_DEM_BY_POINTCLOUD_Z || style == RENDER_DEM_BY_WIREFRAME_Z)
			{
				// 统计颜色
				CalcuCoordRender();
			}
			// 按循环色带渲染
			else if (style == RENDER_DEM_BY_ENTITY_CYCLERAMP || style == RENDER_DEM_BY_POINTCLOUD_CYCLERAMP || style == RENDER_DEM_BY_WIREFRAME_CYCLERAMP)
			{
				// 统计颜色
				CalcuCycleRampRender();
			}
			// 按单色渲染
			else if (style == RENDER_DEM_BY_ENTITY_ACOLOR || style == RENDER_DEM_BY_POINTCLOUD_ACOLOR || style == RENDER_DEM_BY_WIREFRAME_ACOLOR)
			{
				// 统计颜色
				CalcuOneColorRender();
			}
		}

		// 计算按坐标z渲染显示颜色
		void CHdTinSceneNode::CalcuCoordRender()
		{
			// 获取视图中所选择的色带条。原因是操作按循环色带渲染之后
			// 当前色带条会变动，为了保持一致。
			m_colorRampZ.SetRampColor4f(m_ZRampIndex);

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			float scale = 0.0f;				// 缩放
			u8 a = 0,r = 0,g = 0,b = 0;		// 颜色值
			float transparent = 1.0f;		// 透明度

			int selStep = 0;				// 步长
			float tempValue = 0.0f;			// 临时值
			float height = 0.0f;			// 按x或y或z

			// 遍历数据，设置每个点的颜色
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组
			for (s32 x = 0; x < modelpcd.size(); x++)
			{
				// 当前点
				S3DVertex2TCoords& pts = modelpcd[x];

				// 当前点高成
				f32 h = modelpcd[x].Pos.Z;

				// 中间颜色变量
				RenderColor color;

				// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
				if ( h < m_fMinCoord)
				{
					color.r = ((beginColor>>16) & 0xff);
					color.g = ((beginColor>>8) & 0xff);
					color.b = (beginColor & 0xff);
				}

				// 对于大于最大值的点则按照终止颜色渲染
				else if (h > m_fMaxCoord)
				{
					color.r = ((endColor>>16) & 0xff);
					color.g = ((endColor>>8) & 0xff);
					color.b = (endColor & 0xff);
				}

				// 其他在统计范围根据范围以及间隔进行计算
				else
				{
					selStep = (int)((h - m_fMinCoord) / m_fStep);

					tempValue = h - m_fMinCoord - selStep * m_fStep;
					scale = tempValue / m_fStep;
					m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
				}

				// 设置当前点颜色
				pts.Color = SColor(a,color.r,color.g,color.b);	
			}
		}

		// 计算按循环色带渲染颜色
		void CHdTinSceneNode::CalcuCycleRampRender()
		{
			float scale = 0.0f;				// 色带中的比例值
			u8 a = 0,r = 0,g = 0,b = 0;		// 颜色值
			float transparent = 1.0f;		// 透明度

			int selStep = 0;				// 选择颜色
			float tempValue = 0.0f;			// 中间变量
			float height = 0.0f;			// 按x或y或z
			float tmpHeight = 0.f;			// 中间变量

			// 遍历内存点云,计算每个点渲染颜色
			float minZ = m_box.MinEdge.Z;
			float minX = m_box.MinEdge.X;
			float minY = m_box.MinEdge.Y;

			// 设置循环色带
			m_colorRampCycle.SetRampColor4f(m_CycleRampIndex);

			// 默认按照Z轴去计算
			u32 beginColor = m_colorRampCycle.GetBeginColor();
			u32 endColor = m_colorRampCycle.GetEndColor();

			// 遍历数据，设置每个点的颜色
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组

			for (s32 x = 0; x < modelpcd.size(); x++)
			{
				// 为了求取过渡带的正确色彩
				// 标记过渡带，如步长为3，那么过渡带为高差为3.0、6.0等3的整数倍点
				bool flag = false;

				// 当前点颜色
				SColor& scolor = modelpcd[x].Color;

				// 计算后当前点颜色
				RenderColor color;

				// 按Z方向高差
				if (m_nAxis == 0)
				{
					// z
					height = modelpcd[x].Pos.Z;

					// 求解余数
					tmpHeight =  fmodf(height - minZ, m_fCycleStep);
					float tmp = (height - minZ) / m_fCycleStep;

					if ((tmp >= 1 && (int)(tmp+1) % 2 == 0) )
					{
						// 如果 高差为步长的整数倍，那么进行标记
						float tmphgt = height - minZ;
						if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
						{
							flag = true;
						}
						tmpHeight = m_fCycleStep - tmpHeight ;
					}
				}
				// 按X方向高差
				else if (m_nAxis == 1)
				{
					// x
					height = modelpcd[x].Pos.X;

					// 求解余数
					tmpHeight =  fmodf(height - minX,m_fCycleStep);
					float tmp = (height - minX) / m_fCycleStep;

					if (tmp >= 1 && (int)tmp % 2 == 0)
					{
						// 如果 高差为步长的整数倍，那么进行标记
						float tmphgt = height - minX;
						if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
						{
							flag = true;
						}
						tmpHeight = m_fCycleStep - tmpHeight ;
					}
				}
				// 按Y方向高差
				else if (m_nAxis == 2)
				{
					// y
					height = modelpcd[x].Pos.Y;

					// 求解余数
					tmpHeight =  fmodf(height - minY, m_fCycleStep);
					float tmp = (height - minY) / m_fCycleStep;

					if (tmp >= 1 && (int)tmp % 2 == 0)
					{
						// 如果 高差为步长的整数倍，那么进行标记
						float tmphgt = height - minY;
						if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
						{
							flag = true;
						}
						tmpHeight = m_fCycleStep - tmpHeight;
					}
				}

				{
					// 根据余数求解在色带十个颜色中的哪一个颜色
					selStep = (int)(tmpHeight / (m_fCycleStep / 10.f));

					// 求解在该色带中的比例值
					tempValue = tmpHeight - selStep * (m_fCycleStep/10.f);
					scale = tempValue / (m_fCycleStep/10.f);

					// 如果为过渡带， 反减比例
					if (flag)
					{
						scale = 1 - scale;
					}

					// 设置颜色值
					m_colorRampCycle.GetColor4ub(scale, a, color.r, color.g, color.b, selStep+ 1);
					scolor = SColor(a,color.r,color.g,color.b);		
				}
			}
		}

		// 动态统计当前范围内的渲染索引
		void CHdTinSceneNode::CalcuOneColorRender()
		{
			// 遍历数据，设置每个点的颜色
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组
			for (s32 x = 0; x < modelpcd.size(); x++)
			{
				SColor& scolor = modelpcd[x].Color;
				f32 h = modelpcd[x].Pos.Z;	
				scolor = SColor(255,GetRValue(m_selColor),GetGValue(m_selColor),GetBValue(m_selColor));		
			}
		}

		// 动态统计当前范围内的渲染索引
		void CHdTinSceneNode::preRenderIndicesCalculations()
		{
			// 判空
			if (!m_TINPcd || !SceneManager)
			{
				return;
			}

			irr::scene::SViewFrustum rgnFrustum;									// 记录当前视椎体
			static irr::scene::SViewFrustum oldFrustum = rgnFrustum;				// 记录上一次视椎体

			// 获取当前视椎体范围
			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			rgnFrustum = *pViewFrustum;

			// 视椎体范围改变或进行选中后需要重新计算
			if (oldFrustum.getBoundingBox() == rgnFrustum.getBoundingBox() && m_bIsCacuIndices == FALSE)
			{
				// 如果此次视椎体范围没有改变,则不需要进行搜索
				return;
			}
			else
			{
				// 改变则更新上一次范围
				oldFrustum = rgnFrustum;
			}

			//// 清除渲染索引
			//m_CurRenderIndices.clear();
			////long t1,t2,t3;
			////t1 = clock();
			//// 获取是椎体范围内的三角形
			//m_TINPcd->SearchQuadIndex(rgnFrustum, m_CurRenderIndices);
			////t2 = clock();
			////t3 = t2 - t1;
			//// 渲染三角形定点索引总数
			//m_nIndicesToRender = m_CurRenderIndices.size();

			// 当前视椎体范围内三角形索引
			vector<u32>	CurRenderIndices; 

			// 获取是椎体范围内的三角形
			m_TINPcd->SearchQuadIndex(rgnFrustum, CurRenderIndices);

			// 实际渲染三角形定点索引总数
			m_CurRenderIndices.clear();
			m_CurRenderIndices.resize(CurRenderIndices.size());

			// 求抽稀的间隔参数
			int nStep = (int)floor(CurRenderIndices.size() / 3000000 + 0.5);
			m_nIndicesToRender = 0;

			// 对于大于1000000的三角形进行抽稀
			if (nStep > 1)
			{
				// 遍历视野范围的点，获取实时渲染的三角形个数
				for (int i = 0; i < CurRenderIndices.size();i += 3 * nStep)
				{
					m_CurRenderIndices[m_nIndicesToRender] = CurRenderIndices[i];
					m_CurRenderIndices[m_nIndicesToRender + 1] = CurRenderIndices[i + 1];								 
					m_CurRenderIndices[m_nIndicesToRender + 2] = CurRenderIndices[i + 2];		
					m_nIndicesToRender += 3;
				}
				// 循环最后多加了3
				m_nIndicesToRender -= 3;

				// 重新设置大小
				m_CurRenderIndices.resize(m_nIndicesToRender);
			}
			// 对于小于1000000的三角形不进行抽稀少
			else
			{
				m_CurRenderIndices = CurRenderIndices;

				// 渲染三角形索引数
				m_nIndicesToRender = m_CurRenderIndices.size();
			}

			// 将三角形数量显示出来
			CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(SceneManager->getSceneNodeFromId(999));
			if (fpsNode)
			{
				fpsNode->SetTrianCounts(m_nIndicesToRender/3, true);
			}

			//preRenderProfileLineCalculations();
		}

		// 动态统计当前范围内相交的断面线
		void CHdTinSceneNode::preRenderProfileLineCalculations()
		{
			// 断面线结点坐标列表清空
			m_ProfilePts.clear();
			m_ProfilePts.resize(0);

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();

			// 当前渲染三角形索引
			vector<u32>& triIndices = m_TINPcd->GetRenderIndex(); 

			// 计算等高线
			float heightZ = m_box.MaxEdge.Z - m_box.MinEdge.Z;

			int n = (int)floor(heightZ/1.f + 0.5);

			for (int j = 0; j< n;j++)
			{
				// 测试平面
				core::vector3df vertex1 = m_box.MinEdge;
				vertex1.Z = m_box.MinEdge.Z + j*1.f;

				if (vertex1.Z < 0)
				{
					continue;
				}

				core::vector3df vertex2 = m_box.MinEdge;
				vertex2.Y = m_box.MaxEdge.Y;
				vertex2.Z = m_box.MinEdge.Z + j*1.f;

				core::vector3df vertex3;
				vertex3.X = m_box.MaxEdge.X;
				vertex3.Y = m_box.MaxEdge.Y;
				vertex3.Z = m_box.MinEdge.Z + j*1.f;

				core::plane3df slicePlane(vertex1,vertex2,vertex3);

				for (int i = 0; i< triIndices.size()/3;i ++)
				{
					int indexA = triIndices[i*3];
					int indexB = triIndices[i*3 + 1];
					int indexC = triIndices[i*3 + 2];
					core::vector3df pointA,pointB,pointC;

					pointA = modelpcd[indexA].Pos;
					pointB = modelpcd[indexB].Pos;
					pointC = modelpcd[indexC].Pos;

					core::triangle3df tri(pointA,pointB,pointC);

					core::plane3df triPlane(pointA,pointB,pointC);

					core::vector3df outlineVect,outlinepoint;

					// 判断三维平面是否与空间三角形相交 
					// 1.首先判断三角形所在的平面与平面是否相交 如果相交 则进行下一步 否则判断为不相交
					// 2.其次获取平面与平面的交线，然后判断交线与三角形是否相交 如果相交 则判断为相交，进入下一步 如果不相交 则判断为不相交
					// 3.求解平面与平面的交线与三角形的每条线段的交点 即可获取平面与三角形的交线

					if (slicePlane.getIntersectionWithPlane(triPlane,outlinepoint,outlineVect))
					{
						core::vector3df outIntersectionStart;core::vector3df outIntersectionEnd;int nInterCount;

						if (tri.GetIntersectionWithCoPlananrLine(outlinepoint,outlineVect,outIntersectionStart,outIntersectionEnd,nInterCount))
						{
							m_ProfilePts.push_back(outIntersectionStart);
							m_ProfilePts.push_back(outIntersectionEnd);
						}
					}
				}
			}

		}

		// 根据屏幕坐标获取相对坐标 
		int CHdTinSceneNode::Get3DPosFromScrPos( core::vector3df& ptPoint, /* 返回的相对坐标值*/ 
			f64& x,f64& y,f64& z,		/* 返回的绝对坐标值*/
			int srcX,int srcY,		/* 屏幕坐标 */
			int tol,					/* 屏幕查找范围*/
			bool bFindVisibleOnly		/* 是否查找未显示的点*/ )
		{
			// 判空
			if (!m_TINPcd)
			{
				return -1;
			}

			// 获取当前视图
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 实体模型渲染模式下 使用碰撞检测即可获取坐标
			if (m_renderStyle == RENDER_DEM_BY_ENTITY_ACOLOR ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_CYCLERAMP ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_Z)
			{
				// 碰撞检测
				ISceneCollisionManager* pCln = p3DView->GetSceneManager()->getSceneCollisionManager();

				double dx = 0.0,dy = 0.0,dz = 0.0;

				// 获取显示坐标
				if(!pCln->get3DPositionFromScreenPos((u32)srcX,(u32)srcY,ptPoint.X,ptPoint.Y,ptPoint.Z))
				{
					return -1;
				}
			}
			// 需要计算直线与最细层次三角网的交点
			else
			{
				// 构建三维直线
				irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();

				// 鼠标和POS的射线
				core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(srcX, srcY));

				return Get3DPosFrom3DLine(ptPoint,line);
			}

			// 根据显示坐标计算相对坐标,
			// 在tin模型视图中，相对坐标，显示坐标暂时保持一致,根据显示坐标计算全局坐标fengjing
			x = ptPoint.X;
			y = ptPoint.Y;
			z = ptPoint.Z;
			m_TINPcd->GetGlobalCoord(x, y, z);

			return 1;
		}

		int CHdTinSceneNode::Get3DPosFrom3DLine( core::vector3df& ptPoint, /* ?氐南允咀曛?*/ core::line3df& line /* 传入的三维相?直线 */ )
		{
			// 判空
			if (!m_TINPcd || m_nIndicesToRender <= 0)
			{
				return -1;
			}

			// 获取当前视图
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组

			s32 index = 0;
			int count = 0;

			// 构建三维直线
			irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			double minDis = F32_MAX;

			// 对所有可见的三角形进行查询
			for (u32 i = 0; i< m_nIndicesToRender/3 - 1;i++)
			{
				int indexA = m_CurRenderIndices[i*3];
				int indexB = m_CurRenderIndices[i*3 + 1];
				int indexC = m_CurRenderIndices[i*3 + 2];

				// 获取三角形的三个顶点
				core::vector3df pointA,pointB,pointC;

				pointA = modelpcd[indexA].Pos;
				pointB = modelpcd[indexB].Pos;
				pointC = modelpcd[indexC].Pos;

				// 构建三角形
				core::triangle3df tri(pointA,pointB,pointC);

				// 计算三角形与直线的交点
				core::vector3df outIntersection;

				if (tri.getIntersectionWithLine(line.start,line.getVector(),outIntersection))
				{
					double tmpdis = outIntersection.getDistanceFrom(camera->getPosition());

					if (tmpdis < minDis)
					{
						minDis = tmpdis;
						ptPoint = outIntersection;
					}
				}
			}
			LeaveCriticalSection(&m_cs);

			return (int)minDis;
		}

		int CHdTinSceneNode::Get3DPosFrom3DLine( core::vector3df& ptPoint, /* ?氐南允咀曛?*/ core::line3df& line, /* 传入的三维相?直线 */ core::aabbox3df& Selbox /* 限制的查询盒子 */ )
		{
			// 判空
			if (!m_TINPcd)
			{
				return -1;
			}

			// 获取当前视图
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组

			s32 index = 0;
			int count = 0;

			irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			double minDis = F32_MAX;

			// 当前视椎体范围内三角形索引
			vector<u32>	CurRenderIndices; 

			// 获取是椎体范围内的三角形
			m_TINPcd->SearchQuadIndex(Selbox, CurRenderIndices);

			// 对所有可见的三角形进行查询
			for (int i = 0; i< CurRenderIndices.size()/3;i++)
			{
				int indexA = CurRenderIndices[i*3];
				int indexB = CurRenderIndices[i*3 + 1];
				int indexC = CurRenderIndices[i*3 + 2];

				// 获取三角形的三个顶点
				core::vector3df pointA,pointB,pointC;

				pointA = modelpcd[indexA].Pos;
				pointB = modelpcd[indexB].Pos;
				pointC = modelpcd[indexC].Pos;

				// 构建三角形
				core::triangle3df tri(pointA,pointB,pointC);

				// 计算三角形与直线的交点
				core::vector3df outIntersection;

				if (tri.getIntersectionWithLine(line.start,line.getVector(),outIntersection))
				{
					double tmpdis = outIntersection.getDistanceFrom(camera->getPosition());

					if (tmpdis < minDis)
					{
						minDis = tmpdis;
						ptPoint = outIntersection;
					}
				}
			}
			LeaveCriticalSection(&m_cs);

			return (int)minDis;
		}

		bool CHdTinSceneNode::GetTriangleFromSelBox( core::vector3df& aabbox,vector<u32> triIndices )
		{
			if (!m_TINPcd)
			{
				return false;
			}
			// 获取是椎体范围内的三角形
			m_TINPcd->SearchQuadIndex(aabbox, triIndices);

			return true;
		}

		// 根据屏幕坐标获取相对坐标 
		int CHdTinSceneNode::InterpolationFromScrPos( core::vector3df& ptPoint, /* 返回的相对坐标值*/ 
			f64& x,f64& y,f64& z,		/* 返回的绝对坐标值*/
			int srcX,int srcY,			/* 屏幕坐标 */
			int tol,					/* 屏幕查找范围*/
			bool bFindVisibleOnly		/* 是否查找未显示的点*/ )
		{
			// 判空
			if (!m_TINPcd)
			{
				return -1;
			}

			// 获取当前视图
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 实体模型渲染模式下 使用碰撞检测即可获取坐标
			if (m_renderStyle == RENDER_DEM_BY_ENTITY_ACOLOR ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_CYCLERAMP ||
				m_renderStyle == RENDER_DEM_BY_ENTITY_Z)
			{
				// 碰撞检测
				ISceneCollisionManager* pCln = p3DView->GetSceneManager()->getSceneCollisionManager();

				double dx = 0.0,dy = 0.0,dz = 0.0;

				// 获取显示坐标
				if(!pCln->get3DPositionFromScreenPos((u32)srcX,(u32)srcY,ptPoint.X,ptPoint.Y,ptPoint.Z))
				{
					return -1;
				}
			}
			// 需要计算直线与最细层次三角网的交点
			else
			{
				// 构建三维直线
				irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();

				// 鼠标和POS的射线
				core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(srcX, srcY));

				return InterpolatinFrom3DLine(ptPoint,line);
			}

			// 根据显示坐标计算相对坐标,
			// 在tin模型视图中，相对坐标，显示坐标暂时保持一致,根据显示坐标计算全局坐标fengjing
			x = ptPoint.X;
			y = ptPoint.Y;
			z = ptPoint.Z;
			m_TINPcd->GetGlobalCoord(x, y, z);

			return 1;
		}

		int CHdTinSceneNode::InterpolatinFrom3DLine( core::vector3df& ptPoint, /* ?氐南允咀曛?*/ core::line3df& line /* 传入的三维相?直线 */)
		{
			// 判空
			if (!m_TINPcd)
			{
				return -1;
			}

			// 获取当前视图
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_TINPcd->getVertexBuffer();	// 获取顶点数组

			s32 index = 0;
			int count = 0;

			irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			double minDis = F32_MAX;

			// 记录最小距离的三角形起始位置
			int MinIndex = -1;

			// 获取三角形的三个顶点
			core::vector3df pointA,pointB,pointC;

			vector<u32>& triIndices  = m_TINPcd->GetRenderIndex();

			// 对所有可见的三角形进行查询
			for (int i = 0; i< triIndices.size();i+=3)
			{
				int indexA = triIndices[i];
				int indexB = triIndices[i + 1];
				int indexC = triIndices[i + 2];

				pointA = modelpcd[indexA].Pos;
				pointB = modelpcd[indexB].Pos;
				pointC = modelpcd[indexC].Pos;

				// 构建三角形
				core::triangle3df tri(pointA,pointB,pointC);

				// 计算三角形与直线的交点
				core::vector3df outIntersection;

				if (tri.getIntersectionWithLine(line.start,line.getVector(),outIntersection))
				{
					double tmpdis = outIntersection.getDistanceFrom(camera->getPosition());

					if (tmpdis < minDis)
					{
						minDis = tmpdis;
						MinIndex = i;

						ptPoint = outIntersection;
					}
				}
			}

			if (minDis < F32_MAX && MinIndex != -1)
			{
				// 记录当前三角形
				int index1 = triIndices[MinIndex];
				int index2 = triIndices[MinIndex + 1];
				int index3 = triIndices[MinIndex + 2];

				// 对于所选点正好是三角形的顶点，不做处理
				if ((abs(ptPoint.X - modelpcd[index1].Pos.X) < 0.01 && abs(ptPoint.Y - modelpcd[index1].Pos.Y) < 0.01 && abs(ptPoint.Z - modelpcd[index1].Pos.Z) < 0.01) ||
					(abs(ptPoint.X - modelpcd[index2].Pos.X) < 0.01 && abs(ptPoint.Y - modelpcd[index2].Pos.Y) < 0.01 && abs(ptPoint.Z - modelpcd[index2].Pos.Z) < 0.01) ||
					(abs(ptPoint.X - modelpcd[index3].Pos.X) < 0.01 && abs(ptPoint.Y - modelpcd[index3].Pos.Y) < 0.01 && abs(ptPoint.Z - modelpcd[index3].Pos.Z) < 0.01))
				{
					return -1;
				}

				// 将当前新点添加到新的三角形
				video::S3DVertex2TCoords vertex;
				vertex.Pos.X = ptPoint.X;
				vertex.Pos.Y = ptPoint.Y;
				vertex.Pos.Z = ptPoint.Z;
				vertex.Normal = (modelpcd[index1].Normal + modelpcd[index2].Normal + modelpcd[index3].Normal) /  3;
				vertex.Color = SColor(255,255,0,0);
				modelpcd.push_back(vertex);
				int lastpt =  modelpcd.size() - 1;

				modelpcd[index1].Color = SColor(255,255,0,0);
				modelpcd[index2].Color = SColor(255,0,255,0);
				modelpcd[index3].Color = SColor(255,0,0,255);

				// 删除当前三角形
				//triIndices.erase(triIndices.begin() + MinIndex);
				//triIndices.erase(triIndices.begin() + MinIndex);
				//triIndices.erase(triIndices.begin() + MinIndex);

				// 增加新的三角形
				triIndices.push_back(index1);
				triIndices.push_back(index2);
				triIndices.push_back(lastpt);
				
				triIndices.push_back(index2);
				triIndices.push_back(index3);
				triIndices.push_back(lastpt);

				triIndices.push_back(index3);
				triIndices.push_back(index1);
				triIndices.push_back(lastpt);

				// 重新创建索引
				m_TINPcd->CreateQuadIndex();

				// 当前视椎体范围内三角形索引
				vector<u32>	CurRenderIndices; 

				// 获取当前视椎体范围
				const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();

				// 获取是椎体范围内的三角形
				m_TINPcd->SearchQuadIndex(*pViewFrustum, CurRenderIndices);

				// 实际渲染三角形定点索引总数
				m_CurRenderIndices.clear();
				m_CurRenderIndices.resize(CurRenderIndices.size());

				// 求抽稀的间隔参数
				int nStep = (int)floor(CurRenderIndices.size() / 3000000 + 0.5);
				m_nIndicesToRender = 0;

				// 对于大于1000000的三角形进行抽稀
				if (nStep > 1)
				{
					// 遍历视野范围的点，获取实时渲染的三角形个数
					for (int i = 0; i < CurRenderIndices.size();i += 3 * nStep)
					{
						m_CurRenderIndices[m_nIndicesToRender] = CurRenderIndices[i];
						m_CurRenderIndices[m_nIndicesToRender + 1] = CurRenderIndices[i + 1];								 
						m_CurRenderIndices[m_nIndicesToRender + 2] = CurRenderIndices[i + 2];		
						m_nIndicesToRender += 3;
					}
					// 循环最后多加了3
					m_nIndicesToRender -= 3;

					// 重新设置大小
					m_CurRenderIndices.resize(m_nIndicesToRender);
				}
				// 对于小于1000000的三角形不进行抽稀少
				else
				{
					m_CurRenderIndices = CurRenderIndices;

					// 渲染三角形索引数
					m_nIndicesToRender = m_CurRenderIndices.size();
				}

				// 将三角形数量显示出来
				CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(SceneManager->getSceneNodeFromId(999));
				if (fpsNode)
				{
					fpsNode->SetTrianCounts(m_nIndicesToRender/3, true);
				}
				p3DView->Refresh();
			}

			LeaveCriticalSection(&m_cs);
			return (int)minDis;
		}
	}
}