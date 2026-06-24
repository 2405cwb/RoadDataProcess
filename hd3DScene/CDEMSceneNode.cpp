/*! @file
********************************************************************************
<PRE>
模块名       : hdApplication
文件名       : CDEMSceneNode.cpp
相关文件     : CDEMSceneNode.h, 
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
#include "StdAfx.h"
#include "CDEMSceneNode.h"
#include "..\hd3DEngine\include\SMesh.h"
#include "..\hd3DEngine\include\CDynamicMeshBuffer.h"
#include "..\hd3DEngine\os.h"
#include "..\hd3DEngine\COpenGLDriver.h"
#include "..\hdPointCloud\point_cloud.h"
#include "hd3DView.h"
#include <irrlicht.h>
#include "CFPSSceneNode.h"

namespace hd
{
	namespace scene
	{
		// 渲染颜色结构
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

		// 构造函数
		CDEMSceneNode::CDEMSceneNode(ISceneNode* parent, ISceneManager* mgr,
			s32 id, s32 maxLOD, E_TERRAIN_PATCH_SIZE patchSize,
			const core::vector3df& position,
			const core::vector3df& rotation,
			const core::vector3df& scale)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id),
			m_DemData(patchSize, maxLOD, position, rotation, scale), /*m_pRenderBuffer(0),*/
			m_nVerticesToRender(0), m_nIndicesToRender(0),m_bOverrideDistanceThreshold(false),
			m_bUseDefaultRotationPivot(true), m_bForceRecalculation(false),	
			OldCameraPosition(core::vector3df(-99999.9f, -99999.9f, -99999.9f)),
			OldCameraRotation(core::vector3df(-99999.9f, -99999.9f, -99999.9f)),
			OldCameraUp(core::vector3df(-99999.9f, -99999.9f, -99999.9f)),
			CameraMovementDelta(10.0f), CameraRotationDelta(1.0f),CameraFOVDelta(0.1f),
			m_colorRampZ(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),m_fValidCount(0),
			m_colorRampCycle(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_bRenderByHeight(true),
			m_nAxis(0),
			m_fCycleStep(5.f),
			m_ZRampIndex(4),
			m_CycleRampIndex(4),
			m_renderStyle(RENDER_DEM_BY_WIREFRAME_Z),
			m_bShowBBox(false),
			m_nLightShiness(10.f)
		{
			// 初始化代码锁
			::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000408 );

			// 初始化材质,线框Z渲染
			// m_material.AmbientColor = RGB(255, 251, 240); // 白色
			// 光照渲染是否打开 实验光照可以达到更好的渲染效果之前将不使用光照进行渲染 [2014/04/17 危迟]
			m_material.Lighting = false;			
			m_material.Thickness = 2;				// 大小 为2 
			m_material.ZWriteEnable = true;			// 渲染buff可读写
			m_material.Wireframe = true;			// 线框渲染
			m_material.PointCloud = false;			// 点云渲染
			m_selColor = RGB(0,0,255);				// 单色渲染初始值

			// 设置成三角网显示
			setAutomaticCulling(irr::scene::EAC_OFF);

			// 默认光照关闭
			m_bLightOn = false;

			// 标记顶点是否需要重新抽稀计算
			m_bIsPcdSimpled = false;
			m_RenderCount = 0;
		}

		// 析构函数
		CDEMSceneNode::~CDEMSceneNode(void)
		{
			// 释放代码锁
			::DeleteCriticalSection(&m_cs);

			// 释放DEM切片
			if (m_DemData.Patches)
			{
				delete [] m_DemData.Patches;
				m_DemData.Patches = NULL;
			}
			
		}

		// 根据视口重新加载数据
		BOOL CDEMSceneNode::ReloadData()
		{
			// 判空
			if(m_pView == NULL || m_modelPCD == NULL)
			{
				return FALSE;
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 计算LOD
			int ret = preRenderLODCalculations();
			if (ret == 0)
			{
				// 不需要重新计算LOD
				return FALSE;
			}
			// 计算顶点索引
			preRenderIndicesCalculations();

			LeaveCriticalSection(&m_cs);
			return 1;
		}

		// 设置渲染方式
		void CDEMSceneNode::SetRenderDemStyle(const ENUM_DEM_RENDERSTYLE& style)
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

			// 设置当前渲染方式 
			m_renderStyle = style;

			// 按高程渲染
			if (style == RENDER_DEM_BY_ENTITY_Z || style == RENDER_DEM_BY_POINTCLOUD_Z || style == RENDER_DEM_BY_WIREFRAME_Z)
			{
				// 统计颜色值
				CalcuCoordRender();
			}
			// 按循环色带渲染
			else if (style == RENDER_DEM_BY_ENTITY_CYCLERAMP || style == RENDER_DEM_BY_POINTCLOUD_CYCLERAMP || style == RENDER_DEM_BY_WIREFRAME_CYCLERAMP)
			{
				// 统计颜色值
				CalcuCycleRampRender();
			}
			// 按单色渲染
			else if (style == RENDER_DEM_BY_ENTITY_ACOLOR || style == RENDER_DEM_BY_POINTCLOUD_ACOLOR || style == RENDER_DEM_BY_WIREFRAME_ACOLOR)
			{
				// 统计颜色值
				CalcuOneColorRender();
			}
		}

		// 计算按坐标z渲染显示颜色
		void CDEMSceneNode::CalcuCoordRender()
		{
			// 获取视图中所选择的色带条。原因是操作按循环色带渲染之后
			// 当前色带条会变动，为了保持一致。
			m_colorRampZ.SetRampColor4f(m_ZRampIndex);

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			float scale = 0.0f;					// 色带中的比例值
			u8 a = 0,r = 0,g = 0,b = 0;			// 颜色值
			float transparent = 1.0f;			// 透明度

			int selStep = 0;					// 选择颜色
			float tempValue = 0.0f;				// 中间变量
			float height = 0.0f;				// 中间变量
												
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

			// 遍历数据，设置每个点的颜色
			for (s32 x = 0; x < modelpcd.size(); x++)
			{
				// 当前点颜色
				SColor& scolor = modelpcd[x].Color;

				// 当前点高程
				f32 h = modelpcd[x].Pos.Z;

				// 计算当前点的颜色
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
				scolor = SColor(a,color.r,color.g,color.b);		
			}
		}


		// 计算按循环色带渲染颜色
		void CDEMSceneNode::CalcuCycleRampRender()
		{
			float scale = 0.0f;					// 色带中的比例值
			u8 a = 0,r = 0,g = 0,b = 0;			// 颜色值
			float transparent = 1.0f;			// 透明度

			int selStep = 0;					// 选择颜色
			float tempValue = 0.0f;				// 中间变量
			float height = 0.0f;				// 按x或y或z
			float tmpHeight = 0.f;				// 中间变量

			// 遍历内存点云,计算每个点渲染颜色
			float minZ = m_DemData.BoundingBox.MinEdge.Z;
			float minX = m_DemData.BoundingBox.MinEdge.X;
			float minY = m_DemData.BoundingBox.MinEdge.Y;

			// 设置循环色带
			m_colorRampCycle.SetRampColor4f(m_CycleRampIndex);

			// 默认按照Z轴去计算
			u32 beginColor = m_colorRampCycle.GetBeginColor();
			u32 endColor = m_colorRampCycle.GetEndColor();
			
			// 遍历数据，设置每个点的颜色
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组
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
					// Z
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
					// X
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
					// Y
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
					selStep = tmpHeight / (m_fCycleStep / 10.f);
					// 求解在该色带中的比例值
					tempValue = tmpHeight - selStep * (m_fCycleStep/10.f);
					scale = tempValue / (m_fCycleStep/10.f);
					// 如果为过渡带， 反减比例
					if (flag)
					{
						scale = 1 - scale;
					}

					// 设置循环带颜色
					m_colorRampCycle.GetColor4ub(scale, a, color.r, color.g, color.b, selStep+ 1);

					// 设置当前点颜色
					scolor = SColor(a,color.r,color.g,color.b);		
				}
			}
		}

		// 计算按单色渲染
		void CDEMSceneNode::CalcuOneColorRender()
		{
			// 遍历数据，设置每个点的颜色
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组
			for (s32 x = 0; x < modelpcd.size(); x++)
			{
				SColor& scolor = modelpcd[x].Color;
				scolor = SColor(255,GetRValue(m_selColor),GetGValue(m_selColor),GetBValue(m_selColor));		
			}
		}

		// 加载DEM数据
		long CDEMSceneNode::SetDemData(CHdModelPointCloud* modelPcd)
		{
			// 判空
			if (!modelPcd)
			{
				return 0;
			}

			// 设置当前DEM数据
			m_modelPCD = modelPcd;
			m_absModel = m_modelPCD->GetModel();


			// 设置渲染model
			if (m_RenderCount == 0)
			{
				// 获取视图的模型参数
				CBursaWolfModel* pViewBursaModel = m_pView->GetTransModel();
				CBursaWolfModel demModel = m_modelPCD->GetModel();

				m_renderModel =(*pViewBursaModel)*m_absModel;

				m_RenderCount++;
			}

			// 统计坐标
			StatCoord(m_modelPCD->m_vecDemData, m_modelPCD->GetRowNum(), m_modelPCD->GetColNum());

			// 统计颜色参数
			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			int selStep = 0;
			float tempValue = 0.0f;
			float h = 0.0f;

			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			// 计算大小 取较小值
			//m_DemData.Size = min(width,height);
			m_DemData.hSize = m_modelPCD->GetColNum();
			m_DemData.wSize = m_modelPCD->GetRowNum();

			// 根据分块大小，获取最大的细节层次数
			switch (m_DemData.PatchSize)
			{
			case ETPS_9:
				if (m_DemData.MaxLOD > 3)
				{
					m_DemData.MaxLOD = 3;
				}
				break;
			case ETPS_17:
				if (m_DemData.MaxLOD > 4)
				{
					m_DemData.MaxLOD = 4;
				}
				break;
			case ETPS_33:
				if (m_DemData.MaxLOD > 5)
				{
					m_DemData.MaxLOD = 5;
				}
				break;
			case ETPS_65:
				if (m_DemData.MaxLOD > 6)
				{
					m_DemData.MaxLOD = 6;
				}
				break;
			case ETPS_129:
				if (m_DemData.MaxLOD > 7)
				{
					m_DemData.MaxLOD = 7;
				}
				break;
			}

			// 获取点的数量
			const u64 numVertices = m_DemData.hSize*m_DemData.wSize/*m_DemData.Size * m_DemData.Size*/;
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

			// 设置总点数
			try
			{
				modelpcd.resize(numVertices);										
			}
			catch(...)
			{
				return -1;
			}
			
			// 行列步长
			const f32 wtdSize = 1.0f/(f32)(m_DemData.wSize - 1);
			const f32 htdSize = 1.0f/(f32)(m_DemData.hSize - 1);

			s32 index = 0; // 顶点计数器
			float fx=0.f;  // 顶点坐标X值
			float fx2=0.f; // 顶点纹理坐标X值

			// tif数据需要统计包围盒
			double xmin = F64_MAX;
			double ymin = F64_MAX;
			double zmin = F64_MAX;
			double xmax = F64_MIN;
			double ymax = F64_MIN;
			double zmax = F64_MIN; 
			core::aabbox3d<f32>	box(xmin, ymin, zmin, xmax, ymax, zmax);

			double dTmpX,dTmpY,dTmpZ;
			dTmpX = dTmpY = dTmpZ = 0.0f;

			// 获取数据 将其写入到meshbuffer中
			for (s32 x = 0; x < m_DemData.wSize; ++x)
			{
				float fy = 0.f;    // 顶点坐标Y值
				float fy2 = 0.f;   // 顶点纹理坐标Y值
				vector<float>& vecPts = m_modelPCD->m_vecDemData.at(x); // 获取一行数据
				for (s32 y = 0; y < m_DemData.hSize; ++y)
				{
					// 获取数据，将其设置于模型数据结构中
					video::S3DVertex2TCoords& vertex = modelpcd[index++];
					vertex.Normal.set(0.0f, 1.0f, 0.0f);
					vertex.Color = SColor(100,255,0,0);

					// 获得相对坐标
					dTmpX = y * m_modelPCD->GetXStepSize();
					dTmpY = x * m_modelPCD->GetXStepSize();
					dTmpZ = vecPts.at(y);

					// 用rendermodel转换获得显示坐标
					m_renderModel.Translate(dTmpX,dTmpY,dTmpZ);
					vertex.Pos.X = dTmpX;
					vertex.Pos.Y = dTmpY;
					vertex.Pos.Z = dTmpZ;

					//vertex.Pos.X = y * m_modelPCD->GetXStepSize();
					//vertex.Pos.Y = x * m_modelPCD->GetYStepSize();
					//vertex.Pos.Z = *(vecPts._Myfirst + y);

					// 统计颜色值
					if (!m_bRenderByHeight)
					{
						vertex.Color = SColor();
					}
					else
					{
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
						vertex.Color = SColor(a,color.r,color.g,color.b);				
					}

					// 导入tif或asc数据需要统计包围盒
					if (m_modelPCD->GetDataPath().find(".tif") != -1 || m_modelPCD->GetDataPath().find(".asc") != -1||
						m_modelPCD->GetDataPath().find(".TIF") != -1 || m_modelPCD->GetDataPath().find(".ASC") != -1)
					{
						// 统计包围盒 确保有效值进行统计
						if (vertex.Pos.Z <= -9999.0f)
						{
							box.addInternalPoint(vertex.Pos.X, vertex.Pos.Y, vertex.Pos.Z);
						}
					}

					// 设置一级纹理和二级纹理坐标值(鬼火图片坐标与点云相反，此处xy对换)
					vertex.TCoords.X = vertex.TCoords2.X = fy2;// 1.f-fx2
					vertex.TCoords.Y = vertex.TCoords2.Y = 1-fx2;// fy2

					++fy;
					fy2 += htdSize;    //meshbuffer的纹理坐标值范围[0,1]
				}
				++fx;
				fx2 += wtdSize;
			}

			// 对于tif与asc数据，设置包围盒
			if (m_modelPCD->GetDataPath().find(".tif") != -1 || m_modelPCD->GetDataPath().find(".asc") != -1 ||
				m_modelPCD->GetDataPath().find(".TIF") != -1 || m_modelPCD->GetDataPath().find(".ASC") != -1)
			{
				box.MinEdge.Z = m_fMinCoord;
				box.MaxEdge.Z = m_fMaxCoord;
				// 设置包围盒
				m_modelPCD->SetBoudingBox(box);
			}
			// 坐标值平滑
			//SmoothTerrain(mb, 1);
			// 清除缓存
			//m_modelPCD->m_vecDemData.clear();

			// 计算每个顶点的法向量
			CalculateNormals();

			//for (u32 i = 0; i < numVertices; ++i)
			//{
			//	modelpcd[i].Pos *= m_DemData.Scale;		 // 设置缩放
			//	modelpcd[i].Pos += m_DemData.Position;   // 设置位置
			//}

			// 计算距离阈值
			CalculateDistanceThresholds();

			// 创建切片
			CreatePatches();

			// 计算切片数据
			CalculatePatchData();

			// 设置DEM数据中心点为默认的旋转中心
			m_DemData.RotationPivot = m_DemData.Center;

			// 旋转数据的顶点坐标
			setRotation(m_DemData.Rotation);

			// 计算顶点索引
			preRenderIndicesCalculations();

			return 1;
		}

		// 计算距离阈值
		void CDEMSceneNode::CalculateDistanceThresholds(bool scalechanged)
		{
			if (!m_bOverrideDistanceThreshold)
			{
				m_DemData.LODDistanceThreshold.set_used(0);

				m_DemData.LODDistanceThreshold.reallocate(m_DemData.MaxLOD);

				const f64 size = m_DemData.PatchSize * m_DemData.PatchSize *
					m_DemData.Scale.X * m_DemData.Scale.Z;
				for (s32 i=0; i<m_DemData.MaxLOD; ++i)
				{
					m_DemData.LODDistanceThreshold.push_back(size * ((i+1+ i / 2) * (i+1+ i / 2)));
				}
			}
		}

		// 平滑地形
		void CDEMSceneNode::SmoothTerrain(IDynamicMeshBuffer* mb, s32 smoothFactor)
		{
			for (s32 run = 0; run < smoothFactor; ++run)
			{

				s32 xd = m_DemData.hSize;

				for (s32 x = 1; x < m_DemData.wSize - 1;++x)
				{
					for (s32 y = 1; y< m_DemData.hSize - 1;++y)
					{
						// 坐标值平滑 根据上下左右四个点的坐标值进行平滑
						mb->getVertexBuffer()[y + xd].Pos.Z = 
							(mb->getVertexBuffer()[y - 1 + xd].Pos.Z +
							mb->getVertexBuffer()[y + 1 + xd].Pos.Z +
							mb->getVertexBuffer()[y + xd - m_DemData.hSize].Pos.Z + 
							mb->getVertexBuffer()[y + xd + m_DemData.hSize].Pos.Z) * 0.25f;

					}
					xd += m_DemData.hSize;
				}
			}
		}

		// 计算顶点法向量
		void CDEMSceneNode::CalculateNormals()
		{
			s32 count;
			core::vector3df a, b, c, t;

			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

			for (s32 x = 0; x < m_DemData.wSize; ++x)
			{	
				for (s32 y = 0; y < m_DemData.hSize; ++y)
				{
					count = 0;
					core::vector3df normal;

					// 左上
					if (x>0 && y>0)
					{
						a = modelpcd[(x-1)*m_DemData.hSize+y-1].Pos;
						b = modelpcd[(x-1)*m_DemData.hSize+y].Pos;
						c = modelpcd[x*m_DemData.hSize+y].Pos;

						// 确保三个点都是有效值 [2014/04/12 危迟]
						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}
	
						normal += t;

						a = modelpcd[(x-1)*m_DemData.hSize+y-1].Pos;
						b = modelpcd[x*m_DemData.hSize+y-1].Pos;
						c = modelpcd[x*m_DemData.hSize+y].Pos;
						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}
						normal += t;

						count += 2;
					}

					// 右上 内部坐标系是左手系 右手系此时应该是top left
					if (x>0 && y<m_DemData.hSize-1)
					{
						a = modelpcd[(x-1)*m_DemData.hSize+y].Pos;
						b = modelpcd[(x-1)*m_DemData.hSize+y+1].Pos;
						c = modelpcd[x*m_DemData.hSize+y+1].Pos;

						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}
						normal += t;

						a = modelpcd[(x-1)*m_DemData.hSize+y].Pos;
						b = modelpcd[x*m_DemData.hSize+y+1].Pos;
						c = modelpcd[x*m_DemData.hSize+y].Pos;

						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}

						normal += t;

						count += 2;
					}

					// 右下
					if (x<m_DemData.wSize-1 && y<m_DemData.hSize-1)
					{
						a = modelpcd[x*m_DemData.hSize+y+1].Pos;
						b = modelpcd[x*m_DemData.hSize+y].Pos;
						c = modelpcd[(x+1)*m_DemData.hSize+y+1].Pos;

						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}

						normal += t;

						a = modelpcd[x*m_DemData.hSize+y+1].Pos;
						b = modelpcd[(x+1)*m_DemData.hSize+y+1].Pos;
						c = modelpcd[(x+1)*m_DemData.hSize+y].Pos;
						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}

						normal += t;

						count += 2;
					}

					// 左下
					if (x<m_DemData.wSize-1 && y>0)
					{
						a = modelpcd[x*m_DemData.hSize+y-1].Pos;
						b = modelpcd[x*m_DemData.hSize+y].Pos;
						c = modelpcd[(x+1)*m_DemData.hSize+y].Pos;

						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}

						normal += t;

						a = modelpcd[x*m_DemData.hSize+y-1].Pos;
						b = modelpcd[(x+1)*m_DemData.hSize+y].Pos;
						c = modelpcd[(x+1)*m_DemData.hSize+y-1].Pos;

						if (a.Z != -9999.0f && b.Z != -9999.0f && c.Z != -9999.0f)
						{
							b -= a;
							c -= a;
							t = b.crossProduct(c);
							t.normalize();

							// 指定方向必须向上
							if (t.Z < 0.0f)
							{
								t.X = -1.0f * t.X;
								t.Y = -1.0f * t.Y;
								t.Z = -1.0f * t.Z;
							}
						}
						normal += t;

						count += 2;
					}

					if (count != 0)
					{
						//// 尝试求取平均值查看效果
						//normal.X /= count;
						//normal.Y /= count;
						//normal.Z /= count;

						// 指定方向必须向上
						if (normal.Z < 0.0f)
						{
							normal.X = -1.0f * normal.X;
							normal.Y = -1.0f * normal.Y;
							normal.Z = -1.0f * normal.Z;
						}

						normal.normalize();
					}
					else
					{
						normal.set(0.0f, 1.0f, 0.0f);
					}

					modelpcd[x * m_DemData.hSize + y].Normal = normal;
				}
			}
		}

		// 创建切片，申请内存存储，只需在导入数据后执行一次 
		void CDEMSceneNode::CreatePatches()
		{
			// 计算X方向上切片个数
			m_DemData.hPatchCount = (m_DemData.hSize - 1) / (m_DemData.CalcPatchSize);

			// 计算Y方向上切片个数
			m_DemData.wPatchCount = (m_DemData.wSize - 1) / (m_DemData.CalcPatchSize);

			// 判空
			if (m_DemData.Patches)
			{
				delete [] m_DemData.Patches;
			}

			// 申请内存
			m_DemData.Patches = new SDEMPatch[m_DemData.wPatchCount * m_DemData.hPatchCount];
		}

		// 计算切片数据 根据缩放平移参数计算实际渲染的数据 
		void CDEMSceneNode::CalculatePatchData()
		{
			float minZ = -9999.f;
			core::aabbox3df box = core::aabbox3df(999999.9f, 999999.9f, 999999.9f, -999999.9f, -999999.9f, -999999.9f);
			
			if (m_modelPCD)
			{
				m_modelPCD->GetBoudingBox(box);
				minZ = box.MinEdge.Z;
			}

			// 重置整个数据的包围盒
			m_DemData.BoundingBox = box/*core::aabbox3df(999999.9f, 999999.9f, 999999.9f, -999999.9f, -999999.9f, -999999.9f)*/;

			for (s32 x = 0; x < m_DemData.wPatchCount; ++x)
			{
				for (s32 y = 0; y < m_DemData.hPatchCount; ++y)
				{
					const s32 index = x * m_DemData.hPatchCount + y;
					m_DemData.Patches[index].CurrentLOD = 0;

					// 对每一切片，重置包围盒
					m_DemData.Patches[index].BoundingBox = core::aabbox3df(999999.9f, 999999.9f, 999999.9f,
						-999999.9f, -999999.9f, -999999.9f);

					vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

					// 读取buffer存储的顶点坐标值，计算包围盒大小
					for (s32 xx = x*(m_DemData.CalcPatchSize); xx <= (x + 1) * m_DemData.CalcPatchSize; ++xx)
					{
						for (s32 yy = y*(m_DemData.CalcPatchSize); yy <= (y + 1) * m_DemData.CalcPatchSize; ++yy)
						{
							core::vector3df tmpPt = modelpcd[xx * m_DemData.hSize + yy].Pos;
							if (tmpPt.Z <= -9999.0f)
							{
								tmpPt.Z = minZ;
							}
							m_DemData.Patches[index].BoundingBox.addInternalPoint(tmpPt);
						}
					}

					// 循环计算整个数据的包围盒
					m_DemData.BoundingBox.addInternalBox(m_DemData.Patches[index].BoundingBox);

					// 获取切片的中心
					m_DemData.Patches[index].Center = m_DemData.Patches[index].BoundingBox.getCenter();

					// 指定当前切片的邻域
					// 上
					if (x > 0)
						m_DemData.Patches[index].Top = &m_DemData.Patches[(x-1) * m_DemData.hPatchCount + y];
					else
						m_DemData.Patches[index].Top = 0;

					// 下
					if (x < m_DemData.wPatchCount - 1)
						m_DemData.Patches[index].Bottom = &m_DemData.Patches[(x+1) * m_DemData.hPatchCount + y];
					else
						m_DemData.Patches[index].Bottom = 0;

					// 左
					if (y > 0)
						m_DemData.Patches[index].Left = &m_DemData.Patches[x * m_DemData.hPatchCount + y - 1];
					else
						m_DemData.Patches[index].Left = 0;

					// 右
					if (y < m_DemData.hPatchCount - 1)
						m_DemData.Patches[index].Right = &m_DemData.Patches[x * m_DemData.hPatchCount + y + 1];
					else
						m_DemData.Patches[index].Right = 0;
				}
			}

			// 获取整个数据的中心
			m_DemData.Center = m_DemData.BoundingBox.getCenter();

			// 如果外部未指定新的旋转中心，更新默认的旋转中心
			if (m_bUseDefaultRotationPivot)
			{
				m_DemData.RotationPivot = m_DemData.Center;
			}
		}

		void CDEMSceneNode::OnRegisterSceneNode()
		{
			if (!IsVisible || !SceneManager->getActiveCamera())
				return;

			// 注册渲染
			SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
			m_bForceRecalculation = false;
		}

		// 渲染之前计算LOD 主要根据当前活动相机位置与数据的位置
		int CDEMSceneNode::preRenderLODCalculations()
		{
			// 获取当前相机
			irr::scene::ICameraSceneNode * camera = SceneManager->getActiveCamera();
			if(!camera)
			{
				return -1;
			}

			// 注册渲染
			SceneManager->registerNodeForRendering(this);

			// 获取相机参数
			const core::vector3df cameraPosition = camera->getAbsolutePosition();
			const core::vector3df cameraRotation = core::line3d<f32>(cameraPosition, camera->getTarget()).getVector().getHorizontalAngle();
			core::vector3df cameraUp = camera->getUpVector();
			cameraUp.normalize();
			const f32 CameraFOV = SceneManager->getActiveCamera()->getFOV();

			// 检测相机参数判断是否需要重新计算LOD
			if (!m_bForceRecalculation)
			{
				if ((fabsf(cameraRotation.X - OldCameraRotation.X) < CameraRotationDelta) &&
					(fabsf(cameraRotation.Y - OldCameraRotation.Y) < CameraRotationDelta))
				{
					if ((fabs(cameraPosition.X - OldCameraPosition.X) < CameraMovementDelta) &&
						(fabs(cameraPosition.Y - OldCameraPosition.Y) < CameraMovementDelta) &&
						(fabs(cameraPosition.Z - OldCameraPosition.Z) < CameraMovementDelta))
					{
						if (fabs(CameraFOV-OldCameraFOV) < CameraFOVDelta &&
							cameraUp.dotProduct(OldCameraUp) > (1.f - (cos(core::DEGTORAD * CameraRotationDelta))))
						{
							return 0;
						}
					}
				}
			}

			// 更新相机参数
			OldCameraPosition = cameraPosition;
			OldCameraRotation = cameraRotation;
			OldCameraUp = cameraUp;
			OldCameraFOV = CameraFOV;

			// 获取视椎体
			const SViewFrustum* frustum = SceneManager->getActiveCamera()->getViewFrustum();

			// 通过每一个切片是否在视锥体范围内及到相机的距离来计算
			const s32 count = m_DemData.wPatchCount * m_DemData.hPatchCount;
			for (s32 j = 0; j < count; ++j)
			{
				// 判断切片是否在视椎体范围内
				if (frustum->getBoundingBox().intersectsWithBox(m_DemData.Patches[j].BoundingBox))
				{
					// 计算相机位置与切片中心的距离
					const f32 distance = (cameraPosition.X - m_DemData.Patches[j].Center.X) * (cameraPosition.X - m_DemData.Patches[j].Center.X) +
						(cameraPosition.Y - m_DemData.Patches[j].Center.Y) * (cameraPosition.Y - m_DemData.Patches[j].Center.Y) +
						(cameraPosition.Z - m_DemData.Patches[j].Center.Z) * (cameraPosition.Z - m_DemData.Patches[j].Center.Z);

					// 判断距离值与LOD距离阈值
					for (s32 i = m_DemData.MaxLOD - 1; i >= 0; --i)
					{
						if (distance >= m_DemData.LODDistanceThreshold[i])
						{
							m_DemData.Patches[j].CurrentLOD = i;
							break;
						}
						//else if (i == 0)
						{
							m_DemData.Patches[j].CurrentLOD = 0;
						}
					}
				}
				else
				{
					m_DemData.Patches[j].CurrentLOD = -1;
				}
			}
			return 1;
		}

		// 计算buffer顶点索引
		void CDEMSceneNode::preRenderIndicesCalculations()
		{
			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();

			// 根据顶点数组size进行判断是否全部显示(100w以下全部显示)
			if (modelpcd.size() < 1000 * 1000)
			{
				// 
				if (!m_bIsPcdSimpled)
				{
					// 由文件构建三角网
					m_modelPCD->BuildTriangleInDemFile();

					vector<u32>& indexBuffer = m_modelPCD->GetRenderIndex();
					m_nIndicesToRender = indexBuffer.size();

#ifdef _DEBUG
					// 将三角形数量显示出来
					CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(SceneManager->getSceneNodeFromId(999));
					if (fpsNode)
					{
						fpsNode->SetTrianCounts(m_nIndicesToRender/3, true);
					}
#endif
					m_bIsPcdSimpled = true;
					return;
				}
				else
				{
#ifdef _DEBUG
					// 将三角形数量显示出来
					CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(SceneManager->getSceneNodeFromId(999));
					if (fpsNode)
					{
						fpsNode->SetTrianCounts(m_nIndicesToRender/3, true);
					}
#endif

					return;
				}
			}

			// 获取当前渲染buffer索引buffer
			vector<u32>& indexBuffer = m_modelPCD->GetRenderIndex();
			indexBuffer.clear();
			indexBuffer.resize(0);
			m_nIndicesToRender = 0;

			s32 index = 0;		// 切片计数器
			// 对所有可见的切片查找索引值
			for (s32 i = 0; i < m_DemData.wPatchCount; ++i)
			{
				for (s32 j = 0; j < m_DemData.hPatchCount; ++j)
				{
					if (m_DemData.Patches[index].CurrentLOD >= 0)
					{
						s32 x = 0;
						s32 z = 0;

						// 基于当前切片LOD层数，计算步长
						const s32 step = 1 << m_DemData.Patches[index].CurrentLOD;

						// 循环遍历切片，查找索引
						while (z < m_DemData.CalcPatchSize)
						{
							// 去除无效点
							// 如果主对角线上的两个点无效，则整个索引无效
							// 如果只有一个副对角线上的点无效，则可以添加一个三角形
							const s32 index11 = getIndex(j, i, index, x, z);
							const s32 index22 = getIndex(j, i, index, x + step, z + step);
							const s32 index21 = getIndex(j, i, index, x + step, z);
							const s32 index12 = getIndex(j, i, index, x, z + step);

							// 过滤掉无效值
							if (modelpcd[index11].Pos.Z <= -9999.f ||
								modelpcd[index21].Pos.Z <= -9999.f ||
								modelpcd[index12].Pos.Z <= -9999.f ||
								modelpcd[index22].Pos.Z <= -9999.f )
							{
								// 水平增加索引位置
								x += step;

								// 超过切片边界，跳出当前切片
								if (x >= m_DemData.CalcPatchSize)
								{
									x = 0;
									z += step;
								}
								continue;
							}

							//if (CalculateTriBox(index12, index21,index22))
							//{
							try
							{
								// 将三角形插入到渲染索引数组
								indexBuffer.push_back(index12);
								indexBuffer.push_back(index11);
								indexBuffer.push_back(index22);
								indexBuffer.push_back(index22);
								indexBuffer.push_back(index11);
								indexBuffer.push_back(index21);
							}
							catch(...)
							{
								return;
							}

								// 更新渲染索引数量
								m_nIndicesToRender+=6;
							//}

							// 测试选中三角形分开渲染 [2014/04/18 危迟]
							/*if (modelpcd[index11].Pos.Z > m_DemData.BoundingBox.getCenter().Z)
							{
								m_nSelRenderTri.push_back(index12);
								m_nSelRenderTri.push_back(index11);
								m_nSelRenderTri.push_back(index22);
								m_nSelRenderTri.push_back(index22);
								m_nSelRenderTri.push_back(index11);
								m_nSelRenderTri.push_back(index21);
								m_nSelCount += 6;
							}
							else
							{
								m_nUnselRenderTri.push_back(index12);
								m_nUnselRenderTri.push_back(index11);
								m_nUnselRenderTri.push_back(index22);
								m_nUnselRenderTri.push_back(index22);
								m_nUnselRenderTri.push_back(index11);
								m_nUnselRenderTri.push_back(index21);
								m_nUnselCount += 6;
							}*/

							// 水平增加索引位置
							x += step;

							// 超过切片边界，跳出当前切片
							if (x >= m_DemData.CalcPatchSize)
							{
								x = 0;
								z += step;
							}
						}
					}
					++index;
				}
			}

			// 尝试每次更新计算三角形顶点法向量
			core::vector3df vec_a, vec_b, vec_c, vec_t;
			u32 index11,index12,index22;

			// 先将法向量单位化
			for (unsigned int i =0;i < modelpcd.size();i++)
			{

				modelpcd[i].Normal = vec_t;
			}

			indexBuffer = m_modelPCD->GetRenderIndex();
			u32 totalCount = m_nIndicesToRender / 3;
			for (u32 i = 0;i < totalCount;i++)
			{
				core::vector3df tmpNormal;

				// 获得三角形顶点索引
				index11 = indexBuffer[i*3];
				index12 = indexBuffer[i*3+1];
				index22 = indexBuffer[i*3+2];

				// 获取顶点坐标
				vec_a = modelpcd[index11].Pos;
				vec_b = modelpcd[index12].Pos;
				vec_c = modelpcd[index22].Pos;

				vec_t = (vec_b - vec_a).crossProduct(vec_c - vec_a);
				vec_t.normalize();

				 modelpcd[index11].Normal += vec_t;
				 modelpcd[index12].Normal += vec_t;
				 modelpcd[index22].Normal += vec_t;
			}

#ifdef _DEBUG
			// 将三角形数量显示出来
			CFPSSceneNode* fpsNode = dynamic_cast<CFPSSceneNode*>(SceneManager->getSceneNodeFromId(999));
			if (fpsNode)
			{
				fpsNode->SetTrianCounts(m_nIndicesToRender/3, true);
			}
#endif
		}

		// 对于dem到tin的数据，通过包围盒过滤狭长的三角形
		bool CDEMSceneNode::CalculateTriBox(int id0, int id1, int id2)
		{
			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();

			// 判断参数的合法性
			if (!m_modelPCD || id0 >= modelpcd.size() || id1 >= modelpcd.size() || id2 >= modelpcd.size())
			{
				return false;
			}
		
			// 三角形的三个顶点
			core::vector3df point0(modelpcd[id0].Pos.X, modelpcd[id0].Pos.Y,modelpcd[id0].Pos.Z);
			core::vector3df point1(modelpcd[id1].Pos.X, modelpcd[id1].Pos.Y,modelpcd[id1].Pos.Z);
			core::vector3df point2(modelpcd[id2].Pos.X, modelpcd[id2].Pos.Y,modelpcd[id2].Pos.Z);

			// 计算三条边长，
			float dis01 = point0.getDistanceFrom(point1);
			float dis12 = point1.getDistanceFrom(point2);
			float dis02 = point0.getDistanceFrom(point2);

			// 边长大于m_MaxTriaSIzeLen的三角形
			if (dis01 > m_MaxTriaSIzeLen || dis12 > m_MaxTriaSIzeLen || dis02 > m_MaxTriaSIzeLen)
			{
				return false;
			}
				
			return true;
		}

		// 在不同LOD上获取索引值
		u32 CDEMSceneNode::getIndex(const s32 PatchX, const s32 PatchY, const s32 PatchIndex, u32 vX, u32 vY) const
		{
			// 上边界
			if (vY == 0)
			{
				if (m_DemData.Patches[PatchIndex].Top &&
					m_DemData.Patches[PatchIndex].CurrentLOD < m_DemData.Patches[PatchIndex].Top->CurrentLOD &&
					(vX % (1 << m_DemData.Patches[PatchIndex].Top->CurrentLOD)) != 0 )
				{
					vX -= vX % (1 << m_DemData.Patches[PatchIndex].Top->CurrentLOD);
				}
			}
			else
				if (vY == (u32)m_DemData.CalcPatchSize)  // 下层边界
				{
					if (m_DemData.Patches[PatchIndex].Bottom &&
						m_DemData.Patches[PatchIndex].CurrentLOD < m_DemData.Patches[PatchIndex].Bottom->CurrentLOD &&
						(vX % (1 << m_DemData.Patches[PatchIndex].Bottom->CurrentLOD)) != 0)
					{
						vX -= vX % (1 << m_DemData.Patches[PatchIndex].Bottom->CurrentLOD);
					}
				}

				// 左边界
				if (vX == 0)
				{
					if (m_DemData.Patches[PatchIndex].Left &&
						m_DemData.Patches[PatchIndex].CurrentLOD < m_DemData.Patches[PatchIndex].Left->CurrentLOD &&
						(vY % (1 << m_DemData.Patches[PatchIndex].Left->CurrentLOD)) != 0)
					{
						vY -= vY % (1 << m_DemData.Patches[PatchIndex].Left->CurrentLOD);
					}
				}
				else
					if (vX == (u32)m_DemData.CalcPatchSize) // 右边界
					{
						if (m_DemData.Patches[PatchIndex].Right &&
							m_DemData.Patches[PatchIndex].CurrentLOD < m_DemData.Patches[PatchIndex].Right->CurrentLOD &&
							(vY % (1 << m_DemData.Patches[PatchIndex].Right->CurrentLOD)) != 0)
						{
							vY -= vY % (1 << m_DemData.Patches[PatchIndex].Right->CurrentLOD);
						}
					}

					if (vY >= (u32)m_DemData.PatchSize)
						vY = m_DemData.CalcPatchSize;

					if (vX >= (u32)m_DemData.PatchSize)
						vX = m_DemData.CalcPatchSize;

					return (vY + ((m_DemData.CalcPatchSize) * PatchY)) * m_DemData.hSize +
						(vX + ((m_DemData.CalcPatchSize) * PatchX));
		}

		// 
		void CDEMSceneNode::ClearAllColors()
		{
			ITexture* pText = m_material.getTexture(0);
			if (pText)
			{
				// 使用纹理坐标时，顶点颜色置为白色（不对纹理坐标造成影响）
				vector<S3DVertex2TCoords>& vertex = m_modelPCD->getVertexBuffer();
				for (unsigned int i = 0;i < vertex.size();i++)
				{
					S3DVertex2TCoords& ptCoord = vertex.at(i);
					ptCoord.Color = SColor(255,255,255,255);
				}
			}
		}

		// 渲染
		void CDEMSceneNode::render()
		{
			// 判空
			if (!IsVisible || !SceneManager->getActiveCamera())
			{
				return;
			}

			// 获取the video driver
			video::IVideoDriver* driver = SceneManager->getVideoDriver();

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
				m_material.GouraudShading = true;

				// 设置镜面光颜色
				m_material.SpecularColor.set(150,150,150,150);

				// 设置镜面反射度，显示立体化
				m_material.Shininess = m_nLightShiness;
			}
			else
			{
				// 将光照关闭
				m_material.Lighting = false;
				m_material.ColorMaterial = ECM_NONE;
				m_material.BackfaceCulling = false;
			}

			// 设置材质
			driver->setMaterial(m_material);

			// 渲染
			driver->drawVertexPrimitiveList(m_modelPCD->getVertexBuffer()._Myfirst(), m_DemData.hSize*m_DemData.wSize, 
				m_modelPCD->GetRenderIndex()._Myfirst(), m_nIndicesToRender/3, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_32BIT);

			// 渲染选中包围盒
			if (m_indexSelBuffer.size() > 0)
			{
				driver->drawVertexPrimitiveList(m_modelPCD->getVertexBuffer()._Myfirst(), m_DemData.hSize*m_DemData.wSize, 
					m_indexSelBuffer._Myfirst(),m_indexSelBuffer.size()/3, EVT_2TCOORDS,scene::EPT_TRIANGLES,EIT_32BIT);
			}

			//// 根据模型点云是否记录了挖填值，进行显示
			//if (m_modelPCD->GetCutFill())
			//{
			//	//vector<core::aabbox3df> vecCutFillBox = m_modelPCD->GetCutFillBoxes();

			//	//// 逐个box进行渲染
			//	//for (unsigned int n = 0;n < vecCutFillBox.size();n++)
			//	//{
			//	//	if (n >= 100)
			//	//	{
			//	//		continue;
			//	//	}

			//	//	core::aabbox3df box = *(vecCutFillBox._Myfirst + n);

			//	//	// 绘制box
			//	//	driver->draw3DBox(box);
			//	//}
			//}

			// 绘制包围盒
			if (m_bShowBBox)
			{
				driver->draw3DBox(m_DemData.BoundingBox);
			}

#ifdef _DEBUG
			// 渲染选中三角形
			//driver->drawVertexPrimitiveList(m_modelPCD->getVertexBuffer()._Myfirst, m_DemData.hSize*m_DemData.wSize, m_nSelRenderTri._Myfirst, m_nSelCount/3, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_32BIT);

			vector<S3DVertex2TCoords>		pts;
			pts.resize(4);

			video::S3DVertex2TCoords& vertex1 = pts[0];
			vertex1.Normal.set(0.0f, 1.0f, 0.0f);
			vertex1.Color = SColor(100,255,255,0);
			vertex1.Pos = m_DemData.BoundingBox.MinEdge;
			vertex1.Pos.Z = m_DemData.BoundingBox.getCenter().Z;

			video::S3DVertex2TCoords& vertex2 = pts[1];
			vertex2.Normal.set(0.0f, 1.0f, 0.0f);
			vertex2.Color = SColor(100,255,255,0);
			vertex2.Pos = m_DemData.BoundingBox.MinEdge;
			vertex2.Pos.Y = m_DemData.BoundingBox.MaxEdge.Y;
			vertex2.Pos.Z = m_DemData.BoundingBox.getCenter().Z;

			video::S3DVertex2TCoords& vertex3 = pts[2];
			vertex3.Normal.set(0.0f, 1.0f, 0.0f);
			vertex3.Color = SColor(100,255,255,0);
			vertex3.Pos = m_DemData.BoundingBox.MinEdge;
			vertex3.Pos.X = m_DemData.BoundingBox.MaxEdge.X;
			vertex3.Pos.Y = m_DemData.BoundingBox.MaxEdge.Y;
			vertex3.Pos.Z = m_DemData.BoundingBox.getCenter().Z;

			video::S3DVertex2TCoords& vertex4 = pts[3];
			vertex4.Normal.set(0.0f, 1.0f, 0.0f);
			vertex4.Color = SColor(100,255,255,0);
			vertex4.Pos = m_DemData.BoundingBox.MinEdge;
			vertex4.Pos.X = m_DemData.BoundingBox.MaxEdge.X;
			vertex4.Pos.Z = m_DemData.BoundingBox.getCenter().Z;

			vector<u32>				CurRenderIndices; 
			for (u32 i = 0; i< 4;i++)
			{
				CurRenderIndices.push_back(i);
			}
			
			//m_material.MaterialType = EMT_TRANSPARENT_VERTEX_ALPHA;
			// 设置纹理
			
			m_material.Wireframe = false;
			m_material.PointCloud = false;

			//driver->setMaterial(m_material);

			// 测试绘制非选中的三角形
			//driver->drawVertexPrimitiveList(m_modelPCD->getVertexBuffer()._Myfirst, m_DemData.hSize*m_DemData.wSize, m_nUnselRenderTri._Myfirst, m_nUnselCount/3, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_32BIT);

			// 测试绘制四边形
			//driver->drawVertexPrimitiveList(pts._Myfirst,4,CurRenderIndices._Myfirst,1,EVT_2TCOORDS,EPT_QUADS,EIT_32BIT);
#endif
		}

		// 获取包围盒
		const core::aabbox3d<f32>& CDEMSceneNode::getBoundingBox() const
		{
			/*core::aabbox3df box = m_DemData.BoundingBox;
			if (m_modelPCD)
			{
			m_modelPCD->GetBoudingBox(box);
			}*/
			return m_DemData.BoundingBox;
		}

		// 获取指定切片的范围
		const core::aabbox3d<f32>& CDEMSceneNode::getBoundingBox(s32 patchX, s32 patchY) const
		{
			return m_DemData.Patches[patchX * m_DemData.hPatchCount + patchY].BoundingBox;
		}

		// 获取当前材质
		video::SMaterial& CDEMSceneNode::getMaterial(u32 i)
		{
			return m_material;
		}

		// 获取当前材质数量
		u32 CDEMSceneNode::getMaterialCount() const
		{
			return 1;
		}

		// 设置细节层次对应的距离
		bool CDEMSceneNode::OverrideLODDistance(s32 LOD, f64 newDistance)
		{
			m_bOverrideDistanceThreshold = true;

			if (LOD < 0 || LOD > m_DemData.MaxLOD - 1)
			{
				return false;
			}

			m_DemData.LODDistanceThreshold[LOD] = newDistance * newDistance;

			return true;
		}

		// 统计数据高度信息，计算顶点按照高程渲染的颜色
		void CDEMSceneNode::StatCoord(vector<vector<float>>& vecHeight,int width, int height)
		{
			float maxHeight = F32_MIN;
			float minHeight = F32_MAX;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;
			float h = 0.0f;

			// 统计得到最大的高程值和最小的高程值
			for (s32 x = 0; x < width; ++x)
			{
				vector<float>& vecPts = vecHeight.at(x);
				for (s32 y = 0; y < height; ++y)
				{
					h = vecPts.at(y);
					// 无效值为-9999.0f [2013/12/22 危迟]
					if (h <= -9999.f)
					{
						continue;
					}
					if (h > maxHeight)
					{
						maxHeight = h;
					}
					if (h < minHeight)
					{
						minHeight = h;
					}
					m_fValidCount++;
				}
			}

			//统计高度分布，映射至0-999
			memset(mapHeight,0,sizeof(int)*1000);

			float boxHeight = maxHeight - minHeight;
			float boxHeightRcp = 1000.0f / (boxHeight);

			for (s32 x = 0; x < width; ++x)
			{
				vector<float>& vecPts = vecHeight.at(x);
				for (s32 y = 0; y < height; ++y)
				{
					h = vecPts.at(y);

					if (h != 0.f)
					{
						selNum = (int)((h - minHeight) * boxHeightRcp);
						selNum = clamp(selNum,0,999);
						mapHeight[selNum]++;
					}
				}
			}				

			//调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
			selNum = 0;
			for (int j = 999;j >= 0;j--)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(m_fValidCount * 0.03))
				{
					selMax = j;
					break;
				}
			}

			selNum = 0;
			for (int j = 0;j <= 999;j++)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(m_fValidCount * 0.03))
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

		// 设置高程缩放系数
		void CDEMSceneNode::SetHeightZoom( float i )
		{
			//int numVertices = m_pRenderBuffer->getVertexCount();

			//core::vector3df scale(1,1,i);

			//for (u32 i = 0; i < numVertices; ++i)
			//{
			//	/*core::vector3df& coord = m_pRenderBuffer->getVertexBuffer()[i].Pos;	  
			//	coord.Z = m_DemData.BoundingBox.MinEdge.Z + i*(coord.Z - m_DemData.BoundingBox.MinEdge.Z);*/
			//	m_pRenderBuffer->getVertexBuffer()[i].Pos *= scale;
			//}

			//m_pRenderBuffer->setDirty(EBT_VERTEX);
		}

		// 设置材质
		void CDEMSceneNode::SetMaterial( SMaterial mat )
		{
			m_material = mat;
		}

		// 根据屏幕坐标获取点云坐标 
		// 函数返回最近的距离值 
		int CDEMSceneNode::Get3DPosFromScrPos( core::vector3df& ptPoint, /* 返回的相对坐标值*/ 
														f64& x,f64& y,f64& z, /* 返回的绝对坐标值*/ 
														int srcX,int srcY, /* 屏幕坐标 */ 
														int tol, /* 屏幕查找范围*/ 
														bool bFindVisibleOnly /* 是否查找未显示的点*/ )
		{

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

			// 根据显示坐标计算相对坐标 fengjing 
			// 在dem模型视图中，相对坐标，显示坐标暂时保持一致,根据显示坐标计算全局坐标fengjing
			x = ptPoint.X;
			y = ptPoint.Y;
			z = ptPoint.Z;
			m_modelPCD->GetGlobalCoord(x, y);

			return 1;
		}

		// 根据三维线获取相交点坐标 返回距离直线起点最近的坐标值
		int CDEMSceneNode::Get3DPosFrom3DLine( core::vector3df& ptPoint, /* 返回的显示坐标值 */ 
											   core::line3df& line		/* 传入的三维相交直线 */ )
		{
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);

			if (!p3DView)
			{
				return -1;
			}

			// 锁定代码块
			//EnterCriticalSection(&m_cs);

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

			s32 index = 0;
			int count = 0;

			// 构建三维直线
			irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();
			ICameraSceneNode* camera = smgr->getActiveCamera();

			double minDis = F32_MAX;

			// 对所有可见的切片进行查询
			for (s32 i = 0; i < m_DemData.wPatchCount; ++i)
			{
				for (s32 j = 0; j < m_DemData.hPatchCount; ++j)
				{
					if (m_DemData.Patches[index].CurrentLOD >= 0)
					{
						s32 x = 0;
						s32 z = 0;

						// 基于当前切片LOD层数，计算步长
						const s32 step = 1 ;

						// 循环遍历切片，查找索引
						while (z < m_DemData.CalcPatchSize)
						{
							// 去除无效点
							// 如果主对角线上的两个点无效，则整个索引无效
							// 如果只有一个副对角线上的点无效，则可以添加一个三角形
							const s32 index11 = getIndex(j, i, index, x, z);
							const s32 index22 = getIndex(j, i, index, x + step, z + step);
							const s32 index21 = getIndex(j, i, index, x + step, z);
							const s32 index12 = getIndex(j, i, index, x, z + step);

							if (modelpcd[index11].Pos.Z <= -9999.f ||
								modelpcd[index21].Pos.Z <= -9999.f ||
								modelpcd[index12].Pos.Z <= -9999.f ||
								modelpcd[index22].Pos.Z <= -9999.f )
							{
								// 水平增加索引位置
								x += step;

								// 超过切片边界，跳出当前切片
								if (x >= m_DemData.CalcPatchSize)
								{
									x = 0;
									z += step;
								}
								count ++;
								continue;
							}

							// 获取三角形 上三角及下三角
							core::triangle3df upTri;
							core::triangle3df downTri;

							core::vector3df vertex11,vertex12,vertex21,vertex22;
							vertex11 = modelpcd[index11].Pos;
							vertex12 = modelpcd[index12].Pos;
							vertex21 = modelpcd[index21].Pos;
							vertex22 = modelpcd[index22].Pos;

							upTri.set(vertex12,vertex11,vertex22);
							downTri.set(vertex22,vertex11,vertex21);

							// 计算三角形与直线的交点
							core::vector3df outIntersectionA;
							core::vector3df outIntersectionB;
							if (upTri.getIntersectionWithLine(line.start,line.getVector(),outIntersectionA))
							{
								double tmpdis = outIntersectionA.getDistanceFrom(camera->getPosition());

								if (tmpdis < minDis)
								{
									minDis = tmpdis;
									ptPoint = outIntersectionA;
								}
							}
							else if (downTri.getIntersectionWithLine(line.start,line.getVector(),outIntersectionB))
							{
								double tmpdis = outIntersectionB.getDistanceFrom(camera->getPosition());

								if (tmpdis < minDis)
								{
									minDis = tmpdis;
									ptPoint = outIntersectionB;
								}
							}
							// 水平增加索引位置
							x += step;

							// 超过切片边界，跳出当前切片
							if (x >= m_DemData.CalcPatchSize)
							{
								x = 0;
								z += step;
							}
						}
					}
					++index;
				}
			}

			//LeaveCriticalSection(&m_cs);

			return minDis;
		}


		// 根据屏幕坐标获得相交三角形及相交顶点
		int CDEMSceneNode::Get3DPosAndTrianglFromScrPos( core::vector3df& ptPoint, 
			core::triangle3df& triangle, 
			int srcX,int srcY )
		{
			CHd3DView* p3DView = dynamic_cast<CHd3DView*>(m_pView);
			if (!p3DView)
			{
				return -1;
			}

			// 构建三维直线
			irr::scene::ISceneManager* smgr = p3DView->GetSceneManager();

			// 鼠标和POS的射线
			core::line3df line = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(core::vector2di(srcX, srcY));

			// 获取顶点数组
			vector<S3DVertex2TCoords>& modelpcd = m_modelPCD->getVertexBuffer();	// 获取顶点数组

			s32 index = 0;
			int count = 0;

			// 构建三维直线
			ICameraSceneNode* camera = smgr->getActiveCamera();

			double minDis = F32_MAX;

			// 对所有可见的切片进行查询
			for (s32 i = 0; i < m_DemData.wPatchCount; ++i)
			{
				for (s32 j = 0; j < m_DemData.hPatchCount; ++j)
				{
					if (m_DemData.Patches[index].CurrentLOD >= 0)
					{
						s32 x = 0;
						s32 z = 0;

						// 基于当前切片LOD层数，计算步长
						const s32 step = 1 ;

						// 循环遍历切片，查找索引
						while (z < m_DemData.CalcPatchSize)
						{
							// 去除无效点
							// 如果主对角线上的两个点无效，则整个索引无效
							// 如果只有一个副对角线上的点无效，则可以添加一个三角形
							const s32 index11 = getIndex(j, i, index, x, z);
							const s32 index22 = getIndex(j, i, index, x + step, z + step);
							const s32 index21 = getIndex(j, i, index, x + step, z);
							const s32 index12 = getIndex(j, i, index, x, z + step);

							if (modelpcd[index11].Pos.Z <= -9999.f ||
								modelpcd[index21].Pos.Z <= -9999.f ||
								modelpcd[index12].Pos.Z <= -9999.f ||
								modelpcd[index22].Pos.Z <= -9999.f )
							{
								// 水平增加索引位置
								x += step;

								// 超过切片边界，跳出当前切片
								if (x >= m_DemData.CalcPatchSize)
								{
									x = 0;
									z += step;
								}
								count ++;
								continue;
							}

							// 获取三角形 上三角及下三角
							core::triangle3df upTri;
							core::triangle3df downTri;

							core::vector3df vertex11,vertex12,vertex21,vertex22;
							vertex11 = modelpcd[index11].Pos;
							vertex12 = modelpcd[index12].Pos;
							vertex21 = modelpcd[index21].Pos;
							vertex22 = modelpcd[index22].Pos;

							upTri.set(vertex12,vertex11,vertex22);
							downTri.set(vertex22,vertex11,vertex21);

							// 计算三角形与直线的交点
							core::vector3df outIntersectionA;
							core::vector3df outIntersectionB;
							if (upTri.getIntersectionWithLine(line.start,line.getVector(),outIntersectionA))
							{
								double tmpdis = outIntersectionA.getDistanceFrom(camera->getPosition());

								if (tmpdis < minDis)
								{
									minDis = tmpdis;
									ptPoint = outIntersectionA;

									// 记录相交三角形，传出
									triangle = upTri;
								}
							}
							else if (downTri.getIntersectionWithLine(line.start,line.getVector(),outIntersectionB))
							{
								double tmpdis = outIntersectionB.getDistanceFrom(camera->getPosition());

								if (tmpdis < minDis)
								{
									minDis = tmpdis;
									ptPoint = outIntersectionB;

									// 记录相交三角形，传出
									triangle = downTri;
								}
							}
							// 水平增加索引位置
							x += step;

							// 超过切片边界，跳出当前切片
							if (x >= m_DemData.CalcPatchSize)
							{
								x = 0;
								z += step;
							}
						}
					}
					++index;
				}
			}

			if (minDis >= F32_MAX)
			{
				return U32_MAX;
			}
			return minDis;
		}

	}
}

