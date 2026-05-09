/*! @file
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : HdSeaDataSceneNode.cpp
相关文件     : HdSeaDataSceneNode.h
文件实现功能 : 海量点云数据显示
作者         : 软件部，蔡红云
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/03/23	  1.0    蔡红云					创建
2015/10/10	  1.1    蔡红云					更新取点接口
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "HdSeaDataSceneNode.h"
#include "..\hdHlslib\HlzDefs.h"
#include "..\hdCommon\point_types2.h"
#include "..\hd3DEngine\include\irrMath.h"
#include "..\hdHlslib\HdLevel.h"
#include "..\hdHlslib\HdBlock.h"
#include "..\hdHlslib\HdBlockset.h"
#include "..\hdHlslib\HdParcel.h"
#include "..\hd3DEngine\COpenGLExtensionHandler.h"
#include "..\hdCommon\hdSceneStr.h"
#include"..\hdPointCloud\hdSysSetting.h"
#include "..\hdFramework\hdCommandDef.h"
namespace hd

{
	namespace scene
	{

		bool SortPbByDis(CHdParcelBase* pt0,CHdParcelBase* pt1)
		{
			return pt0->m_distance < pt1->m_distance;
		}

		CHdSeaDataSceneNode::CHdSeaDataSceneNode(irr::scene::ISceneNode* parent,irr::scene::ISceneManager* mgr,s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id),m_colorRamp(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_colorRampCycle(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_ptNum(0)
		{
			::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000402 );
			::InitializeCriticalSectionAndSpinCount( &m_csSlct,0x80000403);
			m_material.Wireframe = false;
			m_material.Lighting = false;
			m_material.Thickness = 1; // 大小 为2 
			m_material.ZWriteEnable = true;

			m_material.ZBuffer = ECFN_LESS;

			m_bTrans = false;
			m_colorRamp.SetRampColor4f(4);
			m_renderStyle = RENDER_BY_INTENSITY;
			m_bShowIntenRender = TRUE;
			setAutomaticCulling(irr::scene::EAC_OFF);
			m_fMinCoord = 0.f;
			m_fMaxCoord = 0.f;
			m_fStep = 0.f;
			m_nAxis = 0; // 默认循环色带按Z方向

			m_selPtColor = RGB(0,0,255); // 默认的选择颜色
			m_fMinHeight = 0.f;
			m_fMaxHeight = 0.f;

			m_fMinCorX = 0.f;
			m_fMaxCorX = 0.f;

			m_fMinCorY = 0.f;
			m_fMaxCorY = 0.f;
			m_fCycleStep = 10.f;

			m_renderBBox = FALSE;
			m_bPcdChanged = false;
			m_PointsizeBtn = 2; 
			m_nMinIntensity = 0;
			m_nMaxIntensity = 0;
			m_nCurMinIntensity = 0;
			m_nCurMaxIntensity = 0;
			m_hlzReader = NULL;
			m_SeaPointCloud = NULL;
			m_RenderCount = 0;
			m_showStyle = SHOW_ALL;
			m_selectCount = 0;
			m_pHdlstArea = NULL;
			m_pApha = NULL;
			m_distance = 80.f;
			m_LoadThredIn3DCamrera = 2500000;
			m_LoadThredInQucikCamera = 8000000;
			m_bChangLvl = FALSE;
			m_bAutoFiltr = false;
			m_lockMemory = false;
		}

		CHdSeaDataSceneNode::~CHdSeaDataSceneNode(void)
		{
			::DeleteCriticalSection(&m_cs);
			::DeleteCriticalSection(&m_csSlct);

			// 释放选择信息
			for (int i=0; i<m_vctSelRegion.size(); i++)
			{
				delete m_vctSelRegion[i];
				m_vctSelRegion[i] = NULL;
			}

			m_vctSelRegion.clear();

		}

		// 注册节点
		void CHdSeaDataSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				SceneManager->registerNodeForRendering(this);
			}

			ISceneNode::OnRegisterSceneNode();

		}

		// 根据鼠标选择的范围设置
		void CHdSeaDataSceneNode::SetDefaultColorByZ(int cursel)
		{
			if (cursel == m_colorRamp.GetCurCursel())
			{
				return;
			}

			m_colorRamp.SetRampColor4f(cursel);
		}

		// 渲染
		void CHdSeaDataSceneNode::render()
		{
			//long t1,t2;
			//t1 = clock();


			// 渲染点云
			core::matrix4 mat(AbsoluteTransformation);
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if (!driver)
			{
				return;
			}

			if (!m_pView->IsViewRenderAllNode())
			{
				return;
			}

			driver->setTransform(video::ETS_WORLD,mat);

			// 设置渲染材质
			driver->setMaterial(m_material);

			// 设置渲染状态为3D模式
			driver->setRenderStates3DMode();

			// 蔡红云 2013/11/7 场景中的点云节点是否显示包围盒由视图中盒子显示状态判定
			m_renderBBox = m_pView->GetboxShowState();

			if (m_renderBBox) 
			{
				driver->draw3DBox(m_box, m_bSelected?(video::SColor(255,GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor))):(video::SColor(255,255,255,255)));
			}

			//OutputDebugStringA("Not\n");

			// 锁定代码块

    		EnterCriticalSection(&m_cs);

			// 开始绘制
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 
			glBegin(GL_POINTS);
			RenderPoint();
			glEnd();
			glDisable(GL_BLEND);
			LeaveCriticalSection(&m_cs);			


			//t2 = clock();
			//char strText[128];
			//string strName = m_SeaPointCloud->GetPointCloudPath();
			//strName = strName.substr(strName.find_last_of("\\") + 1);
			//sprintf_s(strText,"%s%s%ld\n",strName.data()," render耗时: ",t2-t1);
			//OutputDebugString(strText);
		}

		// 返回外包围盒
		const core::aabbox3d<f32>& CHdSeaDataSceneNode::getBoundingBox() const
		{
			return m_box;
		}

		u32 CHdSeaDataSceneNode::GetMaterialCount() const
		{
			return 1;
		}

		video::SMaterial& CHdSeaDataSceneNode::GetMaterial(u32 i)
		{
			return m_material;
		}

		//根据设置的强度拉伸范围，重新计算点的透明度信息   袁亮  20160728
		void CHdSeaDataSceneNode::StretchIntenAndCalcAlpha()
		{
			ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);
			if (pView == NULL)
			{
				return;
			}

			float floorRatio = 0.15f;
			float ceilRatio = 0.05f;
			pView->GetIntensityStrethThreshold(floorRatio,ceilRatio);

			// 去除强度大的和强度小之后的强度范围
			m_nCurMinIntensity = (int)(m_nMinIntensity + (m_nMaxIntensity - m_nMinIntensity) * floorRatio);
			m_nCurMaxIntensity = (int)(m_nMaxIntensity - (m_nMaxIntensity - m_nMinIntensity) * ceilRatio);

			//清空之前的透明度数据 
			m_SeaPointCloud->ClearAMry();
			// 遍历列表加载数据
			for (int i = 0; i != m_pHdlstArea->size(); i++)
			{
				// 由点云接口加载包数据
				CHdParcelBase* noInfo = m_pHdlstArea->at(i);

				if (noInfo && noInfo->m_pHlzPoint)
				{
					// 计算透明度信息
					m_SeaPointCloud->CalAphaInfo(noInfo,m_nCurMaxIntensity,m_nCurMinIntensity);
				}
			}

			m_pApha = &m_SeaPointCloud->GetAphaList();
		}

		void CHdSeaDataSceneNode::LoadTopData()
		{
			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 获取层列表
			map<U16,CHdLevel*>& listLevel = m_hlzReader->GetListLevel();

			int index = listLevel.size()-1;

			// 获取最顶层
			CHdLevel* pLevelRec = m_hlzReader->GetLevelRec(index);

			// 为空、返回
			if (!pLevelRec)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			m_hlzReader->SetCurrentLevelByNum(index);

			// ***一层的区域数据、先清空数组**
			m_pHdlstArea->clear();

			m_SeaPointCloud->ClearAMry();

			//CHdListAreaNoInf& Renderlist  = m_HdlstArea.GetRenderList();
			m_hlzReader->GetLevelAreas(pLevelRec,m_pHdlstArea);

			// 如果没有数据则返回
			if (m_pHdlstArea->size()<=0)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			// 初始化基本空间信息
			m_fMinHeight =  m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinZ;
			m_fMaxHeight =  m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxZ;

			m_fMinCorX =  m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinX;
			m_fMaxCorX =  m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxX;

			m_fMinCorY = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinY;
			m_fMaxCorY = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxY;

			m_nMinIntensity = m_SeaPointCloud->m_hlzReader->m_hlzHeader.intensityMin;
			m_nMaxIntensity = m_SeaPointCloud->m_hlzReader->m_hlzHeader.intensityMax;
			m_nCurMinIntensity = m_nMinIntensity;
			m_nCurMaxIntensity = m_nMaxIntensity;


			if (m_bTrans)// 统一到3D坐标下
			{
				m_renderModel.Translate(m_fMinCorX,m_fMinCorY,m_fMinHeight);
				m_renderModel.Translate(m_fMaxCorX,m_fMaxCorY,m_fMaxHeight);
			}


			// 按地址进行排序
			std::sort(m_pHdlstArea->begin(),m_pHdlstArea->end());

			// 加载数据项
			BOOL bSuc = m_SeaPointCloud->LoadAllListAreaParcels(m_pHdlstArea);
			if (bSuc)
			{
				// 遍历列表加载数据
				for (int i = 0; i != m_pHdlstArea->size(); i++)
				{
					// 由点云接口加载包数据
					CHdParcelBase* noInfo = m_pHdlstArea->at(i);

					if (noInfo && noInfo->m_pHlzPoint)
					{
						// 计算透明度信息
						//m_SeaPointCloud->CalAphaInfo(noInfo,m_nMaxIntensity,m_nMinIntensity);
						m_SeaPointCloud->CalAphaInfo(noInfo,m_nCurMaxIntensity,m_nCurMinIntensity);
					}
				}
			}

			//// 加载完成之后、根据一层中点集范围更新包围盒
			//CHdBox3df box = pLevelRec->GetExtent();

			// 加载包围盒不应该获取顶层包围盒，由于层级扩大，顶层记录的包围盒实际远大于实际数据范围,直接以头文件中记录范围即可-by zhubo 2016.0708
			CHdBox3df box;
			box.MinEdge.X = m_SeaPointCloud->m_hlzHeader.min_x;
			box.MinEdge.Y = m_SeaPointCloud->m_hlzHeader.min_y;
			box.MinEdge.Z = m_SeaPointCloud->m_hlzHeader.min_z;
			box.MaxEdge.X = m_SeaPointCloud->m_hlzHeader.max_x;
			box.MaxEdge.Y = m_SeaPointCloud->m_hlzHeader.max_y;
			box.MaxEdge.Z = m_SeaPointCloud->m_hlzHeader.max_z;

			// 更新包围盒的范围
			m_box.MinEdge.set(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z);
			m_box.MaxEdge.set(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);

			// 判断坐标是否需要转换
			if (m_bTrans)
			{

				core::aabbox3d<f32> box = m_box;

				core::aabbox3d<f32> box1;

				box1.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
				box1.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

				// 旋转矩阵存在时，外包围盒显示不正确,更新八个顶点重新生成外包围盒[2014/9/24] 蔡红云 
				core::vector3df edges[8];
				box.getEdges(edges);
				for (int i =0 ; i!=8;i++)
				{
					m_renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
					box1.addInternalPoint(edges[i]);
				}

				m_box = box1;

			}

			// 统计内存中点数
			CalPointNumInMemory();

			LeaveCriticalSection(&m_cs);

		}


		void CHdSeaDataSceneNode::RenderPoint()
		{
			// 加载完成之后进行显示
			RenderColor color;

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRamp.GetBeginColor();
			u32 endColor = m_colorRamp.GetEndColor();

			float scale = 0.0f;				// 缩放
			u8 a = 0,r = 0,g = 0,b = 0;		// 颜色值
			float transparent = 1.0f;		// 透明度

			int selStep = 0;				// 步长
			float tempValue = 0.0f;			// 临时值
			float height = 0.0f;			// 按x或y或z

			// 存储坐标值
			float ptx = 0;
			float pty = 0;
			float ptz = 0;

			for (int k=0;k < m_pHdlstArea->size();k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				// 获取亮度集合
				map<U64,u8* >::iterator it = m_pApha->find(pHdParcelBase->GetCoordAddr().GetAddress());
				unsigned char* apha = NULL;

				if (it!= m_pApha->end())
				{
					apha = it->second;
				}

				if(apha == 0)
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();
				int simpleInval = pHdParcelBase->m_SimpleInval;

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex+=simpleInval)
				{
					HlzPoint* pPoint = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pPoint)
					{
						continue;
					}

					if ((m_showStyle == SHOW_SELECT && !pPoint->isSelected()) ||
						(m_showStyle == SHOW_UNSELECT && pPoint->isSelected()) )// 渲染设置中，分类类别为false的情况下不渲染
					{
						continue;
					}

					// 这里只需要赋值透明度即可,其他rgb是在其他渲染方式计算
					color.a = *(apha+ptIndex);
					if (color.a == 0)
					{
						color.a = 255;
					}

					ptx = pPoint->x;
					pty = pPoint->y;
					ptz = pPoint->z;

					// 判断是否需要转换坐标
					if (m_bTrans)
					{
						m_renderModel.Translate(ptx,pty,ptz);
					}

					switch (m_renderStyle)
					{

					case RENDER_BY_X: // 按X轴渲染
						{
							// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
							if ( ptx < m_fMinCoord)
							{
								color.r = ((beginColor>>16) & 0xff);
								color.g = ((beginColor>>8) & 0xff);
								color.b = (beginColor & 0xff);
							}

							// 对于大于最大值的点则按照终止颜色渲染
							else if (ptx > m_fMaxCoord)
							{
								color.r = ((endColor>>16) & 0xff);
								color.g = ((endColor>>8) & 0xff);
								color.b = (endColor & 0xff);
							}

							// 其他在统计范围根据范围以及间隔进行计算
							else
							{
								selStep = (int)((ptx - m_fMinCoord) / m_fStep);

								tempValue = ptx - m_fMinCoord - selStep * m_fStep;
								scale = tempValue / m_fStep;
								m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
							}

							if (pPoint->isSelected()) // 判断点是否被选中
							{
								if (m_showStyle == SHOW_SELECT)
								{
									glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
								}
								else
								{
									glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
								}

							}
							else
							{
								glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
							}

						}
						break;

					case RENDER_BY_Y:  // 按Y轴渲染
						{

							// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
							if ( pty < m_fMinCoord)
							{
								color.r = ((beginColor>>16) & 0xff);
								color.g = ((beginColor>>8) & 0xff);
								color.b = (beginColor & 0xff);
							}

							// 对于大于最大值的点则按照终止颜色渲染
							else if (pty > m_fMaxCoord)
							{
								color.r = ((endColor>>16) & 0xff);
								color.g = ((endColor>>8) & 0xff);
								color.b = (endColor & 0xff);
							}

							// 其他在统计范围根据范围以及间隔进行计算
							else
							{
								selStep = (int)((pty - m_fMinCoord) / m_fStep);

								tempValue = pty - m_fMinCoord - selStep * m_fStep;
								scale = tempValue / m_fStep;
								m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
							}


							if (pPoint->isSelected()) // 判断点是否被选中
							{
								if (m_showStyle == SHOW_SELECT)
								{
									glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
								}
								else
								{
									glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
								}


							}
							else
							{
								glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
							}

						}
						break;
					case RENDER_BY_Z:    // 按Z轴渲染
						{

							// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
							if ( ptz < m_fMinCoord)
							{
								color.r = ((beginColor>>16) & 0xff);
								color.g = ((beginColor>>8) & 0xff);
								color.b = (beginColor & 0xff);
							}

							// 对于大于最大值的点则按照终止颜色渲染
							else if (ptz > m_fMaxCoord)
							{
								color.r = ((endColor>>16) & 0xff);
								color.g = ((endColor>>8) & 0xff);
								color.b = (endColor & 0xff);
							}

							// 其他在统计范围根据范围以及间隔进行计算
							else
							{
								selStep = (int)((ptz - m_fMinCoord) / m_fStep);

								tempValue = ptz - m_fMinCoord - selStep * m_fStep;
								scale = tempValue / m_fStep;
								m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
							}

							if (pPoint->isSelected()) // 判断点是否被选中
							{

								if (m_showStyle == SHOW_SELECT)
								{
									glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
								}
								else
								{
									glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
								}


							}
							else
							{
								glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
							}

						}
						break;

					case RENDER_BY_INTENSITY: // 按强度渲染

						if (pPoint->isSelected()) // 判断点是否被选中
						{
							if (m_showStyle == SHOW_SELECT)
							{
								glColor4ub(color.a,color.a,color.a,255);
							}
							else
							{
								glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
							}


						}
						else
						{
							glColor4ub(color.a,color.a,color.a,255);
						}

						break;

					case RENDER_BY_CYCLERAMP: // 按循环色带渲染

						CalcuCycleRampRender(color,ptx,pty,ptz);

						if (pPoint->isSelected()) // 判断点是否被选中
						{
							if (m_showStyle == SHOW_SELECT)
							{
								glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
							}
							else
							{
								glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
							}
						}
						else
						{
							glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?color.a:255);
						}
						break;

					case RENDER_BY_DEFAULT: // 按照默认渲染

						glColor4ub(m_defaultClr.r,m_defaultClr.g,m_defaultClr.b,m_bShowIntenRender?color.a:255);
						break;

					case RENDER_BY_RGB: // 按照RGB渲染

						if (pPoint->isSelected()) // 判断点是否被选中
						{
							glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
						}
						else
						{
							//glColor4ub(0,0,0,m_bShowIntenRender?color.a:255);  // 袁亮 点云RGB渲染   20160628
 							glColor4ub(pPoint->color.getRed(), pPoint->color.getGreen(), pPoint->color.getBlue(), m_bShowIntenRender?color.a:255);
						}            		
						break;

					default:
						break;

					}

					glVertex3d(ptx,pty,ptz);

				} //for (int ptIndex=0;ptIndex<count;ptIndex+=simpleInval)

			}//for (int k=0;k < m_pHdlstArea->size();k++)

		}

		// 当鼠标滚动时，屏幕分辨率改变切换层显示 、如果切换层时返回TRUE
		BOOL CHdSeaDataSceneNode::ChangeLevlByScale()
		{

			// 如果视图为空返回
			if (!m_pView)
			{
				return FALSE;
			}

			// 获取当前显示比例尺
			float scale = m_pView->GetDisplayScale();


			I16 lastLevlNum =  m_hlzReader->GetCurrentLevelNo();

			// 根据当前比例尺加载点云
			m_hlzReader->SetCurrentLevelByScale(scale);

			I16 curLevlNum = m_hlzReader->GetCurrentLevelNo();


			return lastLevlNum!=curLevlNum;

		}

		// 根据当前视口内满足一定点数切换层级
		BOOL CHdSeaDataSceneNode::ChangeLevlByPtNum(bool quckcam)
		{
			// 如果视图为空返回
			if (!m_pView)
			{
				return FALSE;
			}

			EnterCriticalSection(&m_cs);
			I16 lastLevlNum =  m_hlzReader->GetCurrentLevelNo();

			int LoadTred = 0;

			if (quckcam)
			{
				LoadTred = m_LoadThredInQucikCamera;
			}
			else
			{
				LoadTred = m_LoadThredIn3DCamrera;		
			}

			CHdBox3df cameraBox;
			GetCameraBox(cameraBox);

			// 获取显示列表
			map<U16,CHdLevel*> lstLevel = m_hlzReader->GetListLevel(); 

			map<U16,CHdLevel*>::iterator it = lstLevel.begin();

			bool bfind = false; // 判断是否找到合适层

			for (;it != lstLevel.end();++it)
			{
				U16 LevelNum = it->first;

				CHdListAreaNoInf listArea;

				// 获取满足条件的显示列表
				m_hlzReader->GetIntersectAreas(LevelNum,cameraBox,&listArea);

				u64 ptCount = 0;

				for (int i = 0; i<listArea.size(); i++)
				{
					CHdParcelBase* parcl = listArea.at(i);

					CHdBox3df box = parcl->GetExtent();

					core::aabbox3df adbox;
					adbox.MaxEdge.set(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
					adbox.MinEdge.set(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z);

					if (parcl && CheckLoad(adbox))
					{

						if (quckcam)
						{
							// 判断加上距离限制
							irr::scene::ICameraSceneNode* pCamera = SceneManager->getActiveCamera();

							// 相机为空、返回
							if (!pCamera)
							{
								LeaveCriticalSection(&m_cs);
								return FALSE;
							}

							core::vector3df pos = pCamera->getPosition();

							if (m_bTrans)
							{
								m_renderModel.AntiTranslate(pos.X,pos.Y,pos.Z);
							}

							double x = adbox.getCenter().X - pos.X;
							double y = adbox.getCenter().Y - pos.Y;
							double z = adbox.getCenter().Z - pos.Z;

							double dist = sqrt(x*x+ y*y + z*z);

							if (dist < m_distance)
							{
								ptCount += parcl->GetPtCount(); // 统计点数
							}
						}

						else
						{
							ptCount += parcl->GetPtCount(); // 统计点数
						}			

					}

				}

				if (ptCount < LoadTred) // 统计点数，如果找到大于阈值的层级就退出
				{
					if (m_hlzReader->GetCurrentLevelNo() != LevelNum)
					{
						// 切换层时，先卸载当前层数据
						m_hlzReader->SetCurrentLevelByNum(LevelNum);

					}

					break;
				}
			}	

			I16 curLevlNum = m_hlzReader->GetCurrentLevelNo();
			LeaveCriticalSection(&m_cs);
			return lastLevlNum!=curLevlNum;

		}

		BOOL CHdSeaDataSceneNode::GetAreaListByCameraBox()
		{
			// 获取相机
			irr::scene::ICameraSceneNode* pCamera = SceneManager->getActiveCamera();

			// 相机为空、返回
			if (!pCamera)
			{
				return FALSE;
			}

			CHdBox3df cameraBox;
			GetCameraBox(cameraBox);

			// ***一层的区域数据、先清空数组**
			m_pHdlstArea->clear();
			m_hlzReader->UnloadOutOfExtent(cameraBox);

			//m_SeaPointCloud->m_hlzReader->UnLoadCurLevlData();

			core::vector3df pos = pCamera->getPosition();
			core::vector3df target = pCamera->getTarget();

			if (m_bTrans)
			{
				m_renderModel.AntiTranslate(pos.X,pos.Y,pos.Z);
			}


			F32 eye[3] = {pos.X,pos.Y,pos.Z};
			core::vector3df vecnorm = (target-pos).normalize();

			F32 vec[3] = {vecnorm.X,vecnorm.Y,vecnorm.Z};


			//m_hlzReader->GetCurLevelAreasByEye(eye,vec,200.0,&m_HdlstArea.GetRenderList());

			//m_hlzReader->GetCurLevelAreas(cameraBox,&m_HdlstArea.GetRenderList());

			m_hlzReader->GetCurLevelAreasByEye(cameraBox,eye,m_pHdlstArea);


			// 如果没有数据则返回
			if (m_pHdlstArea->size()<=0)
			{
				return FALSE;
			}

			return TRUE;

		}

		//! 根据视口重新加载数据
		BOOL CHdSeaDataSceneNode::ReloadData()
		{
			// 南京测绘勘察研究院项目修改 --20160712-张恒
			if (m_lockMemory)
			{
				return TRUE;
			}

			//long t1,t2;
			//t1 = clock();

			// 判断是否合法 !m_pView->IsViewRenderAllNode()||
			if(m_pView == NULL || m_SeaPointCloud->m_hlzReader->IsOpen()== FALSE || (!IsVisible&&!m_pView->GetCalReLoad()))
			{
				return FALSE;
			}


			bool quickcam = false;

			// 快速相机浏览时阈值设为800w，3D飞行浏览时阈值设置为200w
			ISceneView * pScneView = dynamic_cast<ISceneView*>(m_pView);

			if (pScneView && pScneView->getCameraTool()&& pScneView->getCameraTool()->GetID
				() == COMMAND_QUICK_CAMERA)
			{
				quickcam = true;

			}
			else
			{
				quickcam = false;

			}


			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 如果滚动了鼠标，则切换比例尺、改变当前层
			//if (m_bChangLvl)
			//{
			// 切换层之后，把上一层的强度数据清空
			if (/*ChangeLevlByScale()*/ ChangeLevlByPtNum(quickcam)) 
			{
				// 释放强度内存
				m_SeaPointCloud->ClearAMry();
			}

			// 切换完成之后、置为非切换
			//m_bChangLvl = FALSE;
			//}

			// 根据相机的包围盒获取显示区域
			if (!GetAreaListByCameraBox())
			{
				LeaveCriticalSection(&m_cs);
				return FALSE;
			}

			//DetectStLvl(quickcam);// 监测点数，确保在一定阈值范围内

			// 需要的话可按照距离排序
			std::sort(m_pHdlstArea->begin(),m_pHdlstArea->end(),SortPbByDis);

			// 用于记录需要加载的包数据指针
			CHdListAreaNoInf pHdlstTmpArea;

			// 遍历列表加载数据
			for (int i = 0; i<m_pHdlstArea->size(); i++)
			{
				// 检查数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(i);

				if (pHdParcelBase)
				{

					CHdBox3df box = pHdParcelBase->GetExtent();

					core::aabbox3df adbox;
					adbox.MaxEdge.set(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
					adbox.MinEdge.set(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z);

					// 判断加上距离限制
					irr::scene::ICameraSceneNode* pCamera = SceneManager->getActiveCamera();

					// 相机为空、返回
					if (!pCamera)
					{
						continue;
					}

					core::vector3df pos = pCamera->getPosition();

					if (m_bTrans)
					{
						m_renderModel.AntiTranslate(pos.X,pos.Y,pos.Z);
					}

					double x = adbox.getCenter().X - pos.X;
					double y = adbox.getCenter().Y - pos.Y;
					double z = adbox.getCenter().Z - pos.Z;

					double dist = sqrt(x*x+ y*y + z*z);

					if (CheckLoad(adbox))
					{						
						if (quickcam)
						{
							if (dist < m_distance)
							{
								pHdlstTmpArea.push_back(pHdParcelBase);
								//// 由点云接口加载包数据
								//if (m_SeaPointCloud->LoadSpecParcel(pHdParcelBase))
								//{
								//	// 此处统计强度信息并插入字典表中
								//	m_SeaPointCloud->CalAphaInfo(pHdParcelBase,m_nMaxIntensity,m_nMinIntensity);
								//}		
							}
						}
						else
						{
							pHdlstTmpArea.push_back(pHdParcelBase);
							//if (m_SeaPointCloud->LoadSpecParcel(pHdParcelBase))
							//{
							//	// 此处统计强度信息并插入字典表中
							//	m_SeaPointCloud->CalAphaInfo(pHdParcelBase,m_nMaxIntensity,m_nMinIntensity);
							//}			
						}						
					}
					else
					{
						// 不在视口内卸载
						if (m_SeaPointCloud->UnLoadSpecParcel(pHdParcelBase))
						{
							// 卸载指定层的强度信息
							m_SeaPointCloud->ReleaseAMry(pHdParcelBase->GetCoordAddr().GetAddress());
						}			

					}
				}
			}

			// 一次加载内存
			if (pHdlstTmpArea.size() > 0)
			{
				// 加载数据项
				BOOL bSuc = m_SeaPointCloud->LoadAllListAreaParcels(&pHdlstTmpArea);
				if (bSuc)
				{
					// 遍历列表加载数据
					for (int i = 0; i != pHdlstTmpArea.size(); i++)
					{
						// 由点云接口加载包数据
						CHdParcelBase* noInfo = pHdlstTmpArea.at(i);

						if (noInfo && noInfo->m_pHlzPoint)
						{
							// 计算透明度信息
							//m_SeaPointCloud->CalAphaInfo(noInfo,m_nMaxIntensity,m_nMinIntensity);
							m_SeaPointCloud->CalAphaInfo(noInfo,m_nCurMaxIntensity,m_nCurMinIntensity);
						}
					}
				}
			}

			// 统计内存中点数
			CalPointNumInMemory();

			// 判断是否需要自动过滤
			if (m_bAutoFiltr)
			{
				m_SeaPointCloud->UpdateFilterPcd();
			}

			//UpdateSltPoints();
			if (m_SeaPointCloud->GetFilterManager() && m_SeaPointCloud->GetFilterManager()->getFilterCount() > 0)
			{
				for (int i = 0; i != m_pHdlstArea->size(); i++)
				{
					m_SeaPointCloud->GetFilterManager()->doFilter((*m_pHdlstArea)[i], NULL, NULL);
				}
			}

			LeaveCriticalSection(&m_cs);

			//t2 = clock();
			//char strText[128];
			//string strName = m_SeaPointCloud->GetPointCloudPath();
			//strName = strName.substr(strName.find_last_of("\\") + 1);
			//sprintf_s(strText,"%s%s%ld\n",strName.data(),"reload耗时: ",t2-t1);
			//OutputDebugString(strText);

			return TRUE;
		}

		// 检测合适的层数
		BOOL CHdSeaDataSceneNode::DetectStLvl(bool quckcam)
		{		
			EnterCriticalSection(&m_cs);
			u64 count = 0; // 点数

			//******首先遍历点云看当前窗口内点数，如果大于loadthresd，则切换下一层
			//#pragma omp parallel for

			int LoadThred = 0;

			if (quckcam)
			{
				LoadThred = m_LoadThredInQucikCamera;
			}
			else
			{
				LoadThred = m_LoadThredIn3DCamrera;
			}

			// 遍历列表加载数据
			for (int i = 0; i<m_pHdlstArea->size(); i++)
			{

				// 检查数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(i);

				if (pHdParcelBase)
				{

					CHdBox3df box = pHdParcelBase->GetExtent();

					core::aabbox3df adbox;
					adbox.MaxEdge.set(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
					adbox.MinEdge.set(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z);

					if (CheckLoad(adbox) )
					{

						if (quckcam)
						{
							// 判断加上距离限制
							irr::scene::ICameraSceneNode* pCamera = SceneManager->getActiveCamera();

							// 相机为空、返回
							if (!pCamera)
							{
								return FALSE;
							}

							core::vector3df pos = pCamera->getPosition();

							if (m_bTrans)
							{
								m_renderModel.AntiTranslate(pos.X,pos.Y,pos.Z);
							}

							double x = adbox.getCenter().X - pos.X;
							double y = adbox.getCenter().Y - pos.Y;
							double z = adbox.getCenter().Z - pos.Z;

							double dist = sqrt(x*x+ y*y + z*z);

							if (dist < m_distance)
							{
								count += pHdParcelBase->GetPtCount(); // 统计点数
							}
						}

						else
						{
							count += pHdParcelBase->GetPtCount(); // 统计点数
						}

					}
				}
			}

			if (count >LoadThred) // 切换到下一层
			{
				// 获取当前层的索引
				int curlvl = m_hlzReader->GetCurrentLevelNo();

				size_t size = m_hlzReader->GetListLevel().size();

				if (curlvl >=0 && curlvl < size-1)
				{
					m_hlzReader->SetCurrentLevelByNum(curlvl+1);

					// 释放强度内存
					m_SeaPointCloud->ClearAMry();

					// 根据相机的包围盒获取显示区域
					if (!GetAreaListByCameraBox())
					{
						LeaveCriticalSection(&m_cs);
						return TRUE;
					}	

					DetectStLvl(quckcam);
				}

			}
			LeaveCriticalSection(&m_cs);
			return count<=LoadThred;

		}

		BOOL CHdSeaDataSceneNode::CreateRenderList(CHdParcelBase * pHdParcelBase)
		{
			
			if (!pHdParcelBase->m_pHlzPoint) // 如果数据为空return
			{
				return FALSE;
			}

			//获取点数
			int count = pHdParcelBase->GetPtCount();

			if (count <=0 )
			{
				return FALSE;
			}
			EnterCriticalSection(&m_cs);
			SRenderUnit RndUt;
			RndUt.size = count;
			RndUt.vertices = new video::S3DVertex2TCoords[count];

			// 获取数据
			for (int ptIndex=0;ptIndex<count;ptIndex++ )
			{
				HlzPoint* pPoint = (pHdParcelBase->m_pHlzPoint+ptIndex);
				if (pPoint)
				{
					(RndUt.vertices+ptIndex)->Pos.X = pPoint->x;
					(RndUt.vertices+ptIndex)->Pos.Y = pPoint->y;
					(RndUt.vertices+ptIndex)->Pos.Z = pPoint->z;

					// 统计按高程渲染颜色
					SColor r /*= CalcuCoordRender(pPoint->z)*/;

					(RndUt.vertices+ptIndex)->Color = r;
				}
			}

			m_Renderlist.push_back(RndUt);
			LeaveCriticalSection(&m_cs);
			return TRUE;
		}

		// 计算按坐标z渲染显示颜色
		void CHdSeaDataSceneNode::CalcuCoordRender(float h,RenderColor& color)
		{
			// 获取视图中所选择的色带条。原因是操作按循环色带渲染之后
			// 当前色带条会变动，为了保持一致。

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRamp.GetBeginColor();
			u32 endColor = m_colorRamp.GetEndColor();

			float scale = 0.0f;				// 缩放
			u8 a = 0,r = 0,g = 0,b = 0;		// 颜色值
			float transparent = 1.0f;		// 透明度

			int selStep = 0;				// 步长
			float tempValue = 0.0f;			// 临时值
			float height = 0.0f;			// 按x或y或z

			// 获取当前层的范围
			//CHdBox3df LevelBox = m_SeaPointCloud->m_hlzReader->GetCurrentLevelRec()->GetExtent();

			float fStep = m_box.getExtent().Z / 10.0f;

			float fMinCoord = m_box.MinEdge.Z;
			float fMaxCoord = m_box.MaxEdge.Z;


			// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
			if ( h < fMinCoord)
			{
				color.r = ((beginColor>>16) & 0xff);
				color.g = ((beginColor>>8) & 0xff);
				color.b = (beginColor & 0xff);
			}

			// 对于大于最大值的点则按照终止颜色渲染
			else if (h > fMaxCoord)
			{
				color.r = ((endColor>>16) & 0xff);
				color.g = ((endColor>>8) & 0xff);
				color.b = (endColor & 0xff);
			}

			// 其他在统计范围根据范围以及间隔进行计算
			else
			{
				selStep = (int)((h - fMinCoord) / fStep);

				tempValue = h - fMinCoord - selStep * fStep;
				scale = tempValue / fStep;
				m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
			}

		}

		void CHdSeaDataSceneNode::RenderList()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if (!driver)
			{
				return;
			}

			for (u32 i=0; i<m_Renderlist.size(); ++i)
			{
				driver->drawVertexPrimitiveList(m_Renderlist[i].vertices,
					m_Renderlist[i].size,0,m_Renderlist[i].size,video::EVT_2TCOORDS,scene::EPT_POINTS);
			}
		}

		// 获取当前显示的层数
		int  CHdSeaDataSceneNode::GetCurShowLevl()
		{
			EnterCriticalSection(&m_cs);

			// 判断是否打开
			if (m_SeaPointCloud->m_hlzReader->IsOpen())
			{
				LeaveCriticalSection(&m_cs);
				return m_hlzReader->GetCurrentLevelNo();
			}

			else
			{
				LeaveCriticalSection(&m_cs);
				return -1;

			}
		}

		// 返回内存中的点数
		int CHdSeaDataSceneNode::GetPointNumInMemory()
		{
			return m_ptNum;
		}

		void CHdSeaDataSceneNode::CalPointNumInMemory()
		{
			m_ptNum = 0;

			int size = m_pHdlstArea->size();
			if (size <= 0)
			{				
				return;
			}
			EnterCriticalSection(&m_cs);
			
			// 加载完成之后进行显示
			for (int k=0;k<size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase||!pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				m_ptNum+= pHdParcelBase->GetPtCount();
			}	
			LeaveCriticalSection(&m_cs);
		}

		BOOL CHdSeaDataSceneNode::CheckLoad(core::aabbox3df box)
		{

			ISceneCollisionManager* pCollisionMgr = getSceneManager()->getSceneCollisionManager();

			if (!pCollisionMgr)
			{
				return FALSE;
			}

			// 判断是否要转换坐标 
			if (m_bTrans)
			{
				m_renderModel.Translate(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z);
				m_renderModel.Translate(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z);

			}

			// 首先获取box的8个角点
			core::vector3df coner[8];
			core::vector3df prjconer[8];
			box.getEdges(coner);

			for (int i=0; i<8; i++)
			{
				//投影点
				pCollisionMgr->projectPointPixels(coner[i].X,coner[i].Y,coner[i].Z,
					prjconer[i].X,prjconer[i].Y,prjconer[i].Z);
			}

			core::vector2di px_min(I32_MAX,I32_MAX);
			core::vector2di px_max(I32_MIN,I32_MIN);

			// 求取屏幕矩形范围
			for (int i=0;i<8;i++)
			{
				px_min.X = min(px_min.X,prjconer[i].X); px_min.Y = min(px_min.Y,prjconer[i].Y);
				px_max.X = max(px_max.X,prjconer[i].X); px_max.Y = max(px_max.Y,prjconer[i].Y);
			}

			const bool any_cr_zs_neg = (prjconer[0].Z<0 ||prjconer[1].Z<0 ||prjconer[2].Z<0 ||prjconer[3].Z<0 ||prjconer[4].Z<0 ||prjconer[5].Z<0 ||prjconer[6].Z<0 ||prjconer[7].Z<0);
			const bool any_cr_zs_pos = (prjconer[0].Z>0 ||prjconer[1].Z>0 ||prjconer[2].Z>0 ||prjconer[3].Z>0 ||prjconer[4].Z>0 ||prjconer[5].Z>0 ||prjconer[6].Z>0 ||prjconer[7].Z>0);
			const bool box_crosses_image_plane = any_cr_zs_pos && any_cr_zs_neg;

			if (!box_crosses_image_plane && ( px_min.X>=m_pView->GetWindowWidth() || px_min.Y>=m_pView->GetWindowHeight() || px_max.X<0 || px_max.Y<0) )
			{
				return FALSE;
			}// 不可见

			return TRUE;

		}

		//! 设置点云渲染方式
		void CHdSeaDataSceneNode::SetRenderStyle(const ENUM_RENDERSTYLE& style)
		{
			// 如果点云渲染方式相同，则返回
			//if (m_renderStyle == style)
			//{
			//	return;
			//}

			//float rndMinx = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinX;
			//float rndMiny = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinY;
			//float rndMinz = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMinZ;
			//float rndMaxx = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxX; 
			//float rndMaxy = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxY;
			//float rndMaxz = m_SeaPointCloud->m_hlzReader->m_hlzHeader.renderMaxZ;

			//// 坐标转换
			//if (m_bTrans)
			//{
			//	m_renderModel.Translate(rndMinx,rndMiny,rndMinz);
			//	m_renderModel.Translate(rndMaxx,rndMaxy,rndMaxz);

			//}

			// 根据不同的渲染方式，更新不同的属性
			switch (style)
			{
			case RENDER_BY_X:
				m_fMinCoord = m_fMinCorX;
				m_fMaxCoord = m_fMaxCorX;
				m_fStep = (m_fMaxCoord- m_fMinCoord)/ 10.0f;
				break;
			case RENDER_BY_Y:
				m_fMinCoord = m_fMinCorY;
				m_fMaxCoord = m_fMaxCorY;
				m_fStep = (m_fMaxCoord- m_fMinCoord)/ 10.0f;
				break;
			case RENDER_BY_Z:
				m_fMinCoord = m_fMinHeight;
				m_fMaxCoord = m_fMaxHeight;
				m_fStep = (m_fMaxCoord- m_fMinCoord)/ 10.0f;
				break;
			default:
				break;
			}
			m_renderStyle = style;

		}

		//! 获取点云渲染方式
		ENUM_RENDERSTYLE CHdSeaDataSceneNode::GetRenderStyle() const
		{
			return m_renderStyle;

		}

		//! 计算按循环色带渲染颜色
		void CHdSeaDataSceneNode::CalcuCycleRampRender(RenderColor&color ,float x, float y, float z)
		{
			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			float transparent = 1.0f;

			int selStep = 0;
			float tempValue = 0.0f;
			float height = 0.0f;
			float tmpHeight = 0.f;

			bool flag = false;

			switch (m_nAxis)
			{
			case 0:
				{
					height = z;

					// 求解余数
					tmpHeight =  fmodf(height - m_fMinHeight, m_fCycleStep);

					// 循环色带渲染修改为按色带顺序重复渲染，不需要跳转顺序，张恒-20160804
					//float tmp = (height - m_fMinHeight) / m_fCycleStep;
					//if ((tmp >= 1 && (int)(tmp+1) % 2 == 0) )
					//{
					//	// 如果 高差为步长的整数倍，那么进行标记
					//	float tmphgt = height - m_fMinHeight;
					//	if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
					//	{
					//		flag = true;
					//	}

					//	tmpHeight = m_fCycleStep - tmpHeight ;

					//}
				}
				break;
			case 1:
				{
					height = x;

					// 求解余数
					tmpHeight =  fmodf(height - m_fMinCorX,m_fCycleStep);
					//float tmp = (height - m_fMinCorX) / m_fCycleStep;
					//if (tmp >= 1 && (int)tmp % 2 == 0)
					//{
					//	// 如果 高差为步长的整数倍，那么进行标记
					//	float tmphgt = height - m_fMinCorX;
					//	if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
					//	{
					//		flag = true;
					//	}
					//	tmpHeight = m_fCycleStep - tmpHeight ;
					//}

				}
				break;
			case 2:
				{
					height = y;

					// 求解余数
					tmpHeight =  fmodf(height - m_fMinCorY, m_fCycleStep);
					//float tmp = (height - m_fMinCorY) / m_fCycleStep;
					//if (tmp >= 1 && (int)tmp % 2 == 0)
					//{
					//	// 如果 高差为步长的整数倍，那么进行标记
					//	float tmphgt = height - m_fMinCorY;
					//	if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
					//	{
					//		flag = true;
					//	}
					//	tmpHeight = m_fCycleStep - tmpHeight;
					//}
				}

				break;
			default:
				break;
			}

			// 根据余数求解在色带十个颜色中的哪一个颜色
			selStep = (int)(tmpHeight / (m_fCycleStep / 10.f));

			// 求解在该色带中的比例值
			tempValue = tmpHeight - selStep * (m_fCycleStep/10.f);
			scale = tempValue / (m_fCycleStep/10.f);

			// 如果为过渡带， 反减比例
			/*if (flag)
			{
			scale = 1 - scale;
			}*/

			m_colorRampCycle.GetColor4ub(scale, a, color.r, color.g, color.b, selStep+ 1);

		}

		void CHdSeaDataSceneNode::SetDefaultColor( const SColorf& clr )
		{
			m_defaultClr.a = hd::u8(clr.a * 255.0f);
			m_defaultClr.r = hd::u8(clr.r * 255.0f);
			m_defaultClr.g = hd::u8(clr.g * 255.0f);
			m_defaultClr.b = hd::u8(clr.b * 255.0f);
		}

		//! 设置点显示大小
		void CHdSeaDataSceneNode::SetPointSize(f32 size)
		{
			m_material.Thickness = size;
		}

		//! 获取点显示大小
		f32 CHdSeaDataSceneNode::GetPointSize() const
		{
			return  m_material.Thickness ;
		}

		//! 设置点云数据
		BOOL CHdSeaDataSceneNode::SetPointCloud(CSeaPointCloud* pcd)
		{

			if (!pcd)
			{
				return FALSE;
			}

			CBursaWolfModel model = pcd->GetModel();

			m_hlzReader = dynamic_cast<CHdCoreData*>(pcd->m_hlzReader);

			if (!m_hlzReader)
			{
				return FALSE;
			}


			return SetPointCloud(pcd,model);

		}

		//! 设置点云数据
		BOOL CHdSeaDataSceneNode::SetPointCloud(CSeaPointCloud* pcd,CBursaWolfModel model)
		{

			if (pcd != m_SeaPointCloud)
			{
				m_bPcdChanged = true;
			}

			m_SeaPointCloud = pcd;
			m_pHdlstArea = &pcd->GetHdListAreaNoInf();
			m_pApha = &pcd->GetAphaList();

			if (m_SeaPointCloud /*&& m_pointCloud->count() > 0*/)
			{
				// 获取绝对坐标转换模型
				m_absModel = model;

				// 计算场景结点的显示模型参数 [2014/03/19 危迟]
				if (m_RenderCount == 0)
				{
					// 获取视图的模型参数
					CBursaWolfModel* pBursaModel = m_pView->GetTransModel();

					if (pBursaModel)
					{
						// 如果当前视图iScan3D或者多测站视图
						if (m_pView->GetViewType() == E_HVT_MULTISCAN3D ||  
							strcmp(m_pView->GetName(),"iScan3DView") == 0)
						{
							m_renderModel =(*pBursaModel)*m_absModel;
							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}

						}
						// 如果当前视图为快速视图
						else if (m_pView->GetViewType() == E_HVT_QUICK)
						{
							m_renderModel = m_absModel;
							m_bTrans = false;			
						}
						else if(m_pView->GetViewType() == E_HVT_FACADEEDIT)
						{
							CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
							m_renderModel = ivtModel*m_absModel;

							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}
						}
						//如果当前视图为单测站三位视图
						else
						{
							m_renderModel = (*pBursaModel)*m_absModel;
							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}
							//m_bTrans = false;
						}
					}
					m_RenderCount++;
				}
			}

			return TRUE;

		}

		// 获取相机外包围盒
		void CHdSeaDataSceneNode::GetCameraBox(CHdBox3df & camerabox)
		{

			// 获取相机
			irr::scene::ICameraSceneNode* pCamera = SceneManager->getActiveCamera();

			// 相机为空、返回
			if (!pCamera)
			{
				return;
			}


			// 获取相机的包围盒
			core::aabbox3df cambox =  pCamera->getBoundingBox();

			core::aabbox3df boxTemp;

			// 坐标反转过来，保持一致
			if (m_bTrans)
			{
				core::aabbox3d<f32> box = cambox;

				core::aabbox3d<f32> box1;

				box1.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
				box1.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

				// 旋转矩阵存在时，外包围盒显示不正确,更新八个顶点重新生成外包围盒[2014/9/24] 蔡红云 
				core::vector3df edges[8];
				box.getEdges(edges);
				for (int i =0 ; i!=8;i++)
				{
					m_renderModel.AntiTranslate(edges[i].X,edges[i].Y,edges[i].Z);
					box1.addInternalPoint(edges[i]);
				}

				cambox = box1;

			}

			camerabox.MaxEdge.set(cambox.MaxEdge.X,cambox.MaxEdge.Y,cambox.MaxEdge.Z);
			camerabox.MinEdge.set(cambox.MinEdge.X,cambox.MinEdge.Y,cambox.MinEdge.Z);

		}

		BOOL CHdSeaDataSceneNode::GetViewCenter(float& cx,float& cy,float& cz)
		{
			cx = cy = cz = 0.0f;
			if(m_SeaPointCloud == NULL)
				return FALSE;
			double dx = 0.0;
			double dy = 0.0;
			double dz = 0.0;

			double x,y,z;


			EnterCriticalSection(&m_cs);

			m_ptNum = 0;
			u32 ptCount = 0;

			int size = m_pHdlstArea->size();
			if (size <= 0)
			{
				LeaveCriticalSection(&m_cs);
				return FALSE;
			}

			// 加载完成之后进行显示
			for (int k=0;k<size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase||!pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pPoint = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pPoint)
					{
						continue;
					}

					if (m_bTrans)
					{
						// 先转换绝对坐标,再用视图坐标转换显示坐标
						x = pPoint->x;
						y = pPoint->y;
						z = pPoint->z;


						m_renderModel.Translate(x,y,z);
						dx += x;
						dy += y;
						dz += z;
					}
					else
					{
						dx += pPoint->x;
						dy += pPoint->y;
						dz += pPoint->z;
					}
					ptCount++;

				}
			}

			LeaveCriticalSection(&m_cs);

			if (ptCount > 0)
			{
				cx = (float)(dx / ptCount);
				cy = (float)(dy / ptCount);
				cz = (float)(dz / ptCount);
			}
			return ptCount > 0;

		}

		void CHdSeaDataSceneNode::ShowIntensityRender(BOOL bShow)
		{
			if (bShow == m_bShowIntenRender)
			{
				return;
			}

			m_bShowIntenRender = bShow;
		}

		// 根据屏幕坐标获取点云坐标 
		// 函数返回最近的距离值 
		unsigned int CHdSeaDataSceneNode::Get3DPosFromScrPos(
			PointXYZIPRGBA& ptPoint,	  // 返回的相对坐标值
			f64& x,f64& y,f64& z,         // 返回的绝对坐标值
			int srcX,int srcY,			  // 屏幕坐标
			int tol,					  // 屏幕查找范围
			bool bFindVisibleOnly,        // 是否查找未显示的点
			bool bSelected                //是否被选择    
			)
		{

			EnterCriticalSection(&m_cs);
			x = 0.0;
			y = 0.0;
			z = 0.0;

			irr::core::recti irrRect; // 屏幕矩形
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			core::vector3df coord;
			core::position2di screenPos;

			u32 srcMinDist = U32_MAX;
			u32 srcDist = 0;
			//候选点与点击点x/y方向的距离，应该使用有符号整形！必须考虑符号转型带来的严重影响，因为有符号负数赋值给无符号整形会变成一个很大的正数。 袁亮  20160923
			int srcDx = 0;
			int srcDy = 0;
			F64 ptX = 0.0;
			F64 ptY = 0.0;
			F64 ptZ = 0.0;

			if(m_SeaPointCloud == NULL || (!isVisible() && bFindVisibleOnly))
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			core::aabbox3df parcebox;

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				CHdBox3df extent = pHdParcelBase->GetExtent();

				float xmin = extent.MinEdge.X;
				float ymin = extent.MinEdge.Y;
				float zmin = extent.MinEdge.Z;
				float xmax = extent.MaxEdge.X;
				float ymax = extent.MaxEdge.Y;
				float zmax = extent.MaxEdge.Z;

				if (m_bTrans)
				{

					m_renderModel.Translate(xmin,ymin,zmin);
					m_renderModel.Translate(xmax,ymax,zmax);
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}
				else
				{
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}

				if (!rgnFrustum.isCubeIn(parcebox))
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pt)
					{
						continue;
					}

					// 此处根据该节点显示模式进行判断
					if (((m_showStyle == SHOW_UNSELECT) && pt->isSelected())||
						((m_showStyle == SHOW_SELECT) && !pt->isSelected()))
					{
						continue;
					}

					//if (bSelected && pt->isSelected()) // 过滤掉选中点
					//{
					//	continue;
					//}



					if (m_bTrans)
					{			
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;
						m_renderModel.Translate(ptX,ptY,ptZ);
						coord.set((float)ptX,(float)ptY,(float)ptZ);
					}
					else
					{
						coord.set(pt->x,pt->y,pt->z);
					}

					screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
					if (!irrRect.isPointInside(screenPos))
					{
						continue;
					}
					
					srcDx = screenPos.X - srcX;
					srcDy = screenPos.Y - srcY;
					srcDist = srcDx * srcDx + srcDy * srcDy;
					if (srcDist < srcMinDist)
					{
						srcMinDist = srcDist;
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;

						// 返回的绝对坐标
						m_absModel.Translate(ptX,ptY,ptZ);
						x = ptX;
						y = ptY;
						z = ptZ;

						// 返回的相对坐标
						ptPoint.x = pt->x;
						ptPoint.y = pt->y;
						ptPoint.z = pt->z;
						ptPoint.intensity = pt->intensity;
						//保存点的颜色信息  袁亮  20160625
						ptPoint.r = pt->color.getRed();
						ptPoint.g = pt->color.getGreen();
						ptPoint.b = pt->color.getBlue();

					}
				}//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
			}//for (int k=0;k < size;k++)

			LeaveCriticalSection(&m_cs);
			return srcMinDist;
		}

		// 获取屏幕点附近的点集
		unsigned int CHdSeaDataSceneNode::Get3DBufferPtFromScrPos(
			vector<core::vector3df>* pts,	  // 返回的相对坐标值
			int srcX,int srcY,			  // 屏幕坐标
			int tol,					  // 屏幕查找范围
			bool bFindVisibleOnly,        // 是否查找未显示的点
			bool bSelected        // 是否过滤掉选中点 
			)
		{
			EnterCriticalSection(&m_cs);

			irr::core::recti irrRect; // 屏幕矩形
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			core::vector3df coord;
			core::position2di screenPos;
			
			F64 ptX = 0.0;
			F64 ptY = 0.0;
			F64 ptZ = 0.0;

			u64 ptcount = 0;

			if(m_SeaPointCloud == NULL || (!isVisible() && bFindVisibleOnly))
			{
				LeaveCriticalSection(&m_cs);
				return ptcount;
			}

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_cs);
				return ptcount;
			}

			core::aabbox3df parcebox;

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				CHdBox3df extent = pHdParcelBase->GetExtent();

				float xmin = extent.MinEdge.X;
				float ymin = extent.MinEdge.Y;
				float zmin = extent.MinEdge.Z;
				float xmax = extent.MaxEdge.X;
				float ymax = extent.MaxEdge.Y;
				float zmax = extent.MaxEdge.Z;

				if (m_bTrans)
				{

					m_renderModel.Translate(xmin,ymin,zmin);
					m_renderModel.Translate(xmax,ymax,zmax);
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}
				else
				{
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}

				if (!rgnFrustum.isCubeIn(parcebox))
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pt)
					{
						continue;
					}

					if (bSelected && pt->isSelected()) // 过滤掉选中点
					{
						continue;
					}

					if (m_bTrans)
					{			
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;
						m_renderModel.Translate(ptX,ptY,ptZ);
						coord.set((float)ptX,(float)ptY,(float)ptZ);
					}
					else
					{
						coord.set(pt->x,pt->y,pt->z);
					}

					screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
					if (!irrRect.isPointInside(screenPos))
					{
						continue;
					}

					// 返回3D坐标
					pts->push_back(coord);
					pt->setSelected();
					ptcount++;

				}//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
			}//for (int k=0;k < size;k++)

			LeaveCriticalSection(&m_cs);
			return pts->size();
		
		}


		// 更新选择的点云
		void CHdSeaDataSceneNode::UpdateSltPoints( )
		{
			EnterCriticalSection(&m_csSlct);

			if (m_vctSelRegion.size() == 0) // 没有选择状态时重置
			{	
				LeaveCriticalSection(&m_csSlct);
				return;
			}

			if (m_vctSelRegion.size() == 1) // 只有一个选择栈时，判断是全选还是新建选择
			{
				if (m_vctSelRegion[0]->selectMode == SELECT_NEW) // 新建选择
				{
					//先重置所有点云的选择状态
					SetUnSelect();
				}
				else if(m_vctSelRegion[0]->selectMode == SELECT_ALL) // 全部选择
				{
					SelectAll();
					LeaveCriticalSection(&m_csSlct);
					return;
				}
				//else if (m_vctSelRegion[0]->selectMode == SELECT_UNSEL)// 反选
				//{
				//	InvertSelect();
				//	LeaveCriticalSection(&m_csSlct);
				//	return;
				//}
			}
			//所有点初始化为为选中状态，按照完整选择栈逐一进行选择判断，这样得到的点选状态才是正确的。   袁亮   20160924
			SetUnSelect();

			// 范围判断相关变量
			core::aabbox3d<f32> cube;
			bool bIsCubeIn = false;

			//遍历点云坐标
			core::vector3df coord;
			core::position2di screenPos;

			F64 ptX = 0;
			F64	ptY = 0;
			F64 ptZ = 0;

			F32 xmin,ymin,zmin,xmax,ymax,zmax;
			xmin = ymin = zmin = xmax = ymax = zmax = 0.0;
			F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
			xminD = yminD = zminD =xmaxD = ymaxD = zmaxD =0.0;

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_csSlct);
				return;
			}

			// 获取显示的点云区域信息
			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				// 判断当前圈是否和rgh相交
				CHdBox3df box = pHdParcelBase->GetExtent();

				xmin = box.MinEdge.X;
				ymin = box.MinEdge.Y;
				zmin = box.MinEdge.Z;

				xmax = box.MaxEdge.X;
				ymax = box.MaxEdge.Y;
				zmax = box.MaxEdge.Z;

				if (m_bTrans)
				{
					xminD =  xmin;
					yminD =  ymin;
					zminD =  zmin;

					xmaxD = xmax;
					ymaxD = ymax;
					zmaxD = zmax;

					m_renderModel.TranslateExtentW(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);
					cube.MinEdge.set((f32)xminD, (f32)yminD, (f32)zminD);
					cube.MaxEdge.set((f32)xmaxD, (f32)ymaxD, (f32)zmaxD);
				}
				else
				{
					cube.MinEdge.set((f32)xmin, (f32)ymin, (f32)zmin);
					cube.MaxEdge.set((f32)xmax, (f32)ymax, (f32)zmax);
				}

				for (int i = 0;i < m_vctSelRegion.size();i++)
				{
					int slmd = m_vctSelRegion[i]->selectMode;
					bIsCubeIn= m_vctSelRegion[i]->vctFrustum.isCubeIn(cube);

					if (slmd == SELECT_NEW || slmd == SELECT_ADD || slmd == SELECT_MIUS)
					{
						bIsCubeIn= m_vctSelRegion[i]->vctFrustum.isCubeIn(cube);

						// 判断是否和设置区域相交
						//if(!bIsCubeIn)
						//{
						//	continue;
						//}
					}
					
					// 判断是否和设置区域相交
					//if(!bIsCubeIn && !m_vctSelRegion[i]->bInside)
					//{
					//	//获取点数
					//	int count = pHdParcelBase->GetPtCount();
					//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
					//	{
					//		HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);
					//
					//		if (!pt)
					//		{
					//			continue;
					//		}
					//
					//		if (m_vctSelRegion[i]->selectMode == SELECT_NEW)
					//		{
					//			pt->setSelected();
					//		}
					//		else if (m_vctSelRegion[i]->selectMode == SELECT_ADD)
					//		{
					//			pt->setSelected();
					//		}
					//		else if (m_vctSelRegion[i]->selectMode == SELECT_MIUS)
					//		{
					//			pt->setUnSelected();
					//		}
					//	}
					//	continue;
					//}//if(!bIsCubeIn && !m_vctSelRegion[i]->bInside)

					//获取点数
					int count = pHdParcelBase->GetPtCount();

					// 获取数据
					for (int ptIndex=0;ptIndex<count;ptIndex++ )
					{
						HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

						if (!pt)
						{
							continue;
						}

						if (m_bTrans)
						{			
							ptX = pt->x;
							ptY = pt->y;
							ptZ = pt->z;
							m_renderModel.Translate(ptX,ptY,ptZ);
							coord.set((float)ptX,(float)ptY,(float)ptZ);
						}
						else
						{
							coord.set(pt->x,pt->y,pt->z);
						}


						bool bSelected = false;


						//如果是反选
						if (slmd == (int)SELECT_UNSEL)
						{					

							pt->setXorSelected();

							continue;
						}

						if (slmd == (int)SELECT_ALL)
						{
							pt->setSelected();
							continue;
						}


						
												
						//点的包围盒是否中选区内部
						BOOL bInside = FALSE;

						if (bIsCubeIn)
						{
							screenPos = m_vctSelRegion[i]->getScreenCoordinatesFrom3DPosition(coord);

							if (screenPos.X <= m_vctSelRegion[i]->width ||
								screenPos.Y <= m_vctSelRegion[i]->height ||
								screenPos.X >= 0 || screenPos.Y >= 0)
							{
								bInside = ::PtInRegion(m_vctSelRegion[i]->rgnPoly,screenPos.X,screenPos.Y);
							}
						}

						if (m_vctSelRegion[i]->selectMode == SELECT_NEW)       // 新建选区模式
						{
							//bSelected = bInside;
							if(m_vctSelRegion[i]->bInside && bInside)
							{
								pt->setSelected();
							}
							else if (!m_vctSelRegion[i]->bInside && !bInside)
							{
								pt->setSelected();
							}
							else
							{
								pt->setUnSelected();
							}
						}
						else if (m_vctSelRegion[i]->selectMode ==SELECT_ADD)// 增加选区模式
						{
							//bSelected = (bInside||bSelected);

							if(m_vctSelRegion[i]->bInside && bInside)
							{
								pt->setSelected();
							}
							else if (!m_vctSelRegion[i]->bInside && !bInside)
							{
								pt->setSelected();
							}
						}
						else if (m_vctSelRegion[i]->selectMode == SELECT_MIUS)//减少选区模式
						{
							if(m_vctSelRegion[i]->bInside && bInside)
							{
								pt->setUnSelected();
							}
							else if (!m_vctSelRegion[i]->bInside && !bInside)
							{
								pt->setUnSelected();
							}
						}					

					}// for (int ptIndex=0;ptIndex<count;ptIndex++ )

				}//	for (int i = 0;i < m_vctSelRegion.size();i++)
			}//for (int k=0;k < size;k++)

			LeaveCriticalSection(&m_csSlct);
		}
		void CHdSeaDataSceneNode::UpdateSltInfo(SelectRegionFrustum* pSelRegion)
		{
			// 锁定代码块
			EnterCriticalSection(&m_csSlct);

			if (pSelRegion->selectMode == SELECT_NEW || pSelRegion->selectMode == SELECT_ALL)
			{
				for (int i=0; i<m_vctSelRegion.size();i++)
				{
					if (m_vctSelRegion[i])
					{
						delete m_vctSelRegion[i];
						m_vctSelRegion[i] = NULL;
					}
				}

				m_vctSelRegion.clear();
			}

			if (m_vctSelRegion.size() == 1)
			{

				if (m_vctSelRegion[0]->selectMode == SELECT_ALL && pSelRegion->selectMode ==  SELECT_UNSEL)
				{
					delete pSelRegion;
					pSelRegion = NULL;

					delete m_vctSelRegion[0];
					m_vctSelRegion.clear();
					LeaveCriticalSection(&m_csSlct);
					return;
				}

			}

			m_vctSelRegion.push_back(pSelRegion);

			LeaveCriticalSection(&m_csSlct);
		}

		void CHdSeaDataSceneNode::ClearSltRgn()
		{
			// 锁定代码块
			EnterCriticalSection(&m_csSlct);
			for (int i=0; i<m_vctSelRegion.size();i++)
			{
				if (m_vctSelRegion[i])
				{
					delete m_vctSelRegion[i];
					m_vctSelRegion[i] = NULL;
				}
			}
			m_vctSelRegion.clear();
			LeaveCriticalSection(&m_csSlct);

		}


		// 外部设置，清除所有选择
		void CHdSeaDataSceneNode::SetUnSelect()
		{	
			EnterCriticalSection(&m_csSlct);
			m_selectCount = 0;

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_csSlct);
				return;
			}

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int i=0;i<count;i++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

					if(!pt->isValid())
					{
						continue;
					}

					if (pt->isSelected())
					{
						pt->setUnSelected();
					}

				}
			}

			LeaveCriticalSection(&m_csSlct);
		}

		// 选择全部
		void CHdSeaDataSceneNode::SelectAll()
		{
			EnterCriticalSection(&m_csSlct);
			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_csSlct);
				return;
			}

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int i=0;i<count;i++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

					if(!pt->isValid() || pt->isSelected())
					{
						continue;
					}

					pt->setSelected();

				}
			}

			LeaveCriticalSection(&m_csSlct);
		}

		// 反选
		void CHdSeaDataSceneNode::InvertSelect()
		{
			EnterCriticalSection(&m_csSlct);
			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_csSlct);
				return;
			}

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int i=0;i<count;i++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

					if(!pt->isValid())
					{
						continue;
					}

					pt->setXorSelected();

				}
			}

			LeaveCriticalSection(&m_csSlct);
		}

		//! 更新选择过滤以刷新显示
		hd::u32 CHdSeaDataSceneNode::updateForFilter()
		{
			if (m_pHdlstArea == NULL)
			{
				return 0;
			}
			EnterCriticalSection(&m_csSlct);

			//! 过滤管理器
			//ptcloud::hdFilterManager* filter_manager = m_pView->getFilterManager();
			ptcloud::hdFilterManager* filter_manager = m_SeaPointCloud->GetFilterManager();

			u64 nSelectedCount = 0;
			if (filter_manager)
			{
				u32 parcelCount = m_pHdlstArea->size();
				for (int k=0; k<parcelCount; k++)
				{
					CHdParcelBase* pParcel = (*m_pHdlstArea)[k];
					filter_manager->doFilterFresh(pParcel, NULL, NULL);

					int nPtCount = pParcel->GetPtCount();
					for (int i = 0; i < nPtCount; i ++)
					{
						if ((pParcel->m_pHlzPoint + i)->isSelected())
						{
							nSelectedCount ++;
						}
					}
				}
			}

			//处理颜色
			SetRenderStyle(m_renderStyle);

			LeaveCriticalSection(&m_csSlct);

			return nSelectedCount;
		}

		void CHdSeaDataSceneNode::SetShowStyle(ENUM_SHOWSTYLE eStyle )
		{
			m_showStyle = eStyle;
		}

		hd::ENUM_SHOWSTYLE CHdSeaDataSceneNode::GetShowStyle() const
		{
			return m_showStyle;
		}

		void CHdSeaDataSceneNode::GetMaxMinZ( float& nMaxZ, float& nMinZ )
		{
			nMaxZ = m_fMaxHeight;
			nMinZ = m_fMinHeight;
		}

		// 设置循环色带默认色带条
		void CHdSeaDataSceneNode::SetDefaultColorByCycle(int cursel)
		{
			if (cursel == m_colorRampCycle.GetCurCursel())
			{
				return;
			}

			m_colorRampCycle.SetRampColor4f(cursel);

			if (m_renderStyle == RENDER_BY_CYCLERAMP )
			{

				if (m_nAxis == 0)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_nAxis == 1)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_nAxis == 2)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}

			}
		}

		u32 CHdSeaDataSceneNode::getSelectCount()
		{
			EnterCriticalSection(&m_csSlct);
			int size = m_pHdlstArea->size();

			if (size <=0 )
			{
				LeaveCriticalSection(&m_csSlct);
				return 0;
			}

			u64 ptCount = 0;

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int i=0;i<count;i++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

					if(!pt->isValid() || !pt->isSelected())
						continue;

					ptCount++;

				}
			}
			LeaveCriticalSection(&m_csSlct);
			return ptCount;
		}

		//! 根据传入的坐标值获取以该点为中心一定范围内所有点海量点云得平均值作返回
		bool CHdSeaDataSceneNode::GetAveragePosFromSea3DPos( f64& x,f64& y,f64& z, float areaTol /*= 0.1*/ ,bool bVisiable/*= true*/)
		{
			if (areaTol < 0)
			{	
				return false;
			}

			// 定义selbox中心显示坐标的中间变量
			irr::core::aabbox3df selBox;
			f64 fShowCenterX = x;
			f64 fShowCenterY = y;
			f64 fShowCenterZ = z;
			f64 ptX = 0.0;
			f64 ptY = 0.0;
			f64 ptZ = 0.0;
			x = y = z = 0.0;
			core::vector3df coord;
			u64 nCount = 0;// 点数

			EnterCriticalSection(&m_cs);

			// 点云不可见或者其内存数据不存在时返回				
			if(m_SeaPointCloud == NULL || bVisiable && !isVisible())
			{
				LeaveCriticalSection(&m_cs);
				return false;
			}

			// 绝对坐标转换相对坐标
			m_absModel.AntiTranslate(fShowCenterX,fShowCenterY,fShowCenterZ);

			// 相对坐标转换显示坐标
			if (m_bTrans)
			{
				m_renderModel.Translate(fShowCenterX,fShowCenterY,fShowCenterZ);
			}

			// 显示坐标设置box
			selBox.MinEdge.set((f32)(fShowCenterX - areaTol) ,(f32)(fShowCenterY - areaTol),(f32)(fShowCenterZ - areaTol));
			selBox.MaxEdge.set((f32)(fShowCenterX + areaTol) ,(f32)(fShowCenterY + areaTol),(f32)(fShowCenterZ + areaTol));

			int size =  m_pHdlstArea->size();
			if (size <= 0)
			{
				LeaveCriticalSection(&m_cs);
				return false;
			}

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pPoint = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pPoint)
					{
						continue;
					}

					if (m_bTrans)
					{			
						ptX = pPoint->x;
						ptY = pPoint->y;
						ptZ = pPoint->z;
						m_renderModel.Translate(ptX,ptY,ptZ);
						coord.set((float)ptX,(float)ptY,(float)ptZ);
					}
					else
					{
						coord.set(pPoint->x,pPoint->y,pPoint->z);
					}

					if (selBox.isPointInside(coord))
					{
						// 在box范围内，先将相对坐标转换绝对坐标再累加
						ptX = pPoint->x;
						ptY = pPoint->y;
						ptZ = pPoint->z;
						m_absModel.Translate(ptX,ptY,ptZ);

						x += ptX;
						y += ptY;
						z += ptZ;
						nCount++;
					}

				}//for (int ptIndex=0;ptIndex<count;ptIndex++ )

			}//for (int k=0;k < size;k++)

			// 点数大于0 返回真
			if (nCount > 0)
			{
				x /= nCount;
				y /= nCount;
				z /= nCount;
				LeaveCriticalSection(&m_cs);
				return true;
			}

			LeaveCriticalSection(&m_cs);
			return false;

		}

		bool CHdSeaDataSceneNode::Get3dPointFromPano(core::vector3df inScanPos,core::vector3df EndPos, float& outX,
			float& outY,float& outZ)
		{
			EnterCriticalSection(&m_cs);

			if(m_SeaPointCloud == NULL )
			{
				LeaveCriticalSection(&m_cs);
				return false;
			}

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_cs);
				return false;
			}
			
			outX = 0.0;
			outY = 0.0;
			outZ = 0.0;

			core::line3df line;

			line.start = inScanPos;
			line.end = EndPos;

			core::vector3df vec = line.getVector();
			vec.normalize();


			line.end = line.start + vec*50000;
			
			float linedist = line.getLength();

			//查找时，还可根据工程和扫描头进行过滤；

			// 搜索在射线上的所有点并且取距离scanpos最近的那个点

			// 1mm 误差
			float thread = 2.f;

			float dist1 = line.getDistanceToPointP(EndPos);
			core::aabbox3df parcebox;

			vector<core::vector3df>* pNearPts = new vector<core::vector3df>();

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				CHdBox3df extent = pHdParcelBase->GetExtent();

				float xmin = extent.MinEdge.X;
				float ymin = extent.MinEdge.Y;
				float zmin = extent.MinEdge.Z;
				float xmax = extent.MaxEdge.X;
				float ymax = extent.MaxEdge.Y;
				float zmax = extent.MaxEdge.Z;

				// 范围盒
				core::aabbox3df extentbox;
				extentbox.MinEdge.set(xmin,ymin,zmin);
				extentbox.MaxEdge.set(xmax,ymax,zmax);

				// 验证盒子和线是否相交
				bool binsect= extentbox.intersectsWithLine(line);

				//// 不相交进行下次
				//if (!binsect)
				//{
				//	continue;
				//}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pt)
					{
						continue;
					}

					core::vector3df point(pt->x,pt->y,pt->z);

					float disttmp = line.getDistanceToPointP(point);

					if (abs(disttmp)<thread)
					{
						pNearPts->push_back(point);
					}


				}//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
			}//for (int k=0;k < size;k++)

			if (pNearPts->size() == 0)
			{
				LeaveCriticalSection(&m_cs);
				return false;
			}

			int findIndex = 0;
			float dist = pNearPts->at(0).getDistanceFrom(inScanPos);

			// 从搜索最近的点中找到距离scanpos最近的点作为
			for (int i= 1; i<pNearPts->size();i++)
			{
				float distmp = pNearPts->at(i).getDistanceFrom(inScanPos);
				if (distmp<dist)
				{
					findIndex = i;
					dist = distmp;
				}
			}

			// 找到之后赋值
			outX = pNearPts->at(findIndex).X;
			outY = pNearPts->at(findIndex).Y;
			outZ = pNearPts->at(findIndex).Z;


			pNearPts->clear();
			delete pNearPts;
			pNearPts = NULL;
			LeaveCriticalSection(&m_cs);

			return true;
		}

		// 测试代码，函数返回最近的距离值 
		unsigned int CHdSeaDataSceneNode::Get3DPosFromScrPosTst( 
			//const core::vector3df RelativeRot,
			//const core::vector3df RelativeTrans,
			//const core::vector3df RelativeScale,
			core::matrix4& camMatrix, 
			core::dimension2d<u32> dim, 
			f64& x,f64& y,f64& z, /* ?氐木宰曛?*/ 
			int srcX,int srcY, /* 屏?坐标 */ int tol /* 屏?查找 段?*/ )
		{
			//// 内部构成矩阵
			//core::matrix4 camMatrix;
			//camMatrix.setRotationDegrees(RelativeRot);
			//camMatrix.setTranslation(RelativeTrans);
			//camMatrix.setScale(RelativeScale);

			EnterCriticalSection(&m_cs);
			x = 0.0;
			y = 0.0;
			z = 0.0;

			irr::core::recti irrRect; // 屏幕矩形
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

			//const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			//irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			//SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			core::vector3df coord;
			core::position2di screenPos;

			u32 srcMinDist = U32_MAX;
			u32 srcDist = 0;
			//候选点与点击点x/y方向的距离，应该使用有符号整形！必须考虑符号转型带来的严重影响，因为有符号负数赋值给无符号整形会变成一个很大的正数。 袁亮  20160923
			int srcDx = 0;
			int srcDy = 0;
			F64 ptX = 0.0;
			F64 ptY = 0.0;
			F64 ptZ = 0.0;

			if(m_SeaPointCloud == NULL )
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			core::aabbox3df parcebox;

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				CHdBox3df extent = pHdParcelBase->GetExtent();

				float xmin = extent.MinEdge.X;
				float ymin = extent.MinEdge.Y;
				float zmin = extent.MinEdge.Z;
				float xmax = extent.MaxEdge.X;
				float ymax = extent.MaxEdge.Y;
				float zmax = extent.MaxEdge.Z;

				if (m_bTrans)
				{

					m_renderModel.Translate(xmin,ymin,zmin);
					m_renderModel.Translate(xmax,ymax,zmax);
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}
				else
				{
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				}

				//if (!rgnFrustum.isCubeIn(parcebox))
				//{
				//	continue;
				//}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pt)
					{
						continue;
					}

					if (m_bTrans)
					{			
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;
						m_renderModel.Translate(ptX,ptY,ptZ);
						coord.set((float)ptX,(float)ptY,(float)ptZ);
					}
					else
					{
						coord.set(pt->x,pt->y,pt->z);
					}

					//screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
					screenPos = GetScreenPosFrom3dPosTst(camMatrix,dim,coord);
					if (!irrRect.isPointInside(screenPos))
					{
						continue;
					}

					//SceneManager->getSceneCollisionManager()->get3DPositionFromScreenPos();

					srcDx = screenPos.X - srcX;
					srcDy = screenPos.Y - srcY;
					srcDist = srcDx * srcDx + srcDy * srcDy;
					if (srcDist < srcMinDist)
					{
						srcMinDist = srcDist;
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;

						// 返回的绝对坐标
						m_absModel.Translate(ptX,ptY,ptZ);
						x = ptX;
						y = ptY;
						z = ptZ;

						//// 返回的相对坐标
						//ptPoint.x = pt->x;
						//ptPoint.y = pt->y;
						//ptPoint.z = pt->z;
						//ptPoint.intensity = pt->intensity;

					}
				}//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
			}//for (int k=0;k < size;k++)

			LeaveCriticalSection(&m_cs);
			return srcMinDist;
		}

		// 测试代码，测试根据传入矩阵由三维坐标计算屏幕坐标
		core::position2d<s32> CHdSeaDataSceneNode::GetScreenPosFrom3dPosTst(core::matrix4& camMatrix,core::dimension2d<u32> dim,const core::vector3df& pos3d)
		{
			f32 transformedPos[4] = { pos3d.X, pos3d.Y, pos3d.Z, 1.0f };

			camMatrix.multiplyWith1x4Matrix(transformedPos);

			if (transformedPos[3] < 0)
				return core::position2d<s32>(-10000,-10000);

			const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :
				core::reciprocal(transformedPos[3]);

			return core::position2d<s32>(
				core::round32(dim.Width * transformedPos[0] * zDiv) + dim.Width,
				dim.Height - core::round32(dim.Height * (transformedPos[1] * zDiv)));
		}

		unsigned int CHdSeaDataSceneNode::Get3DPosFromScrPos1( PointXYZIPRGBA& ptPoint, /* ?氐南喽宰曛?*/ f64& x,f64& y,f64& z, /* ?氐木宰曛?*/ int srcX,int srcY, /* 屏?坐标 */ int tol, /* 屏?查找 段?*/ bool bFindVisibleOnly, /* 是 癫檎椅聪允镜牡?*/ bool bSelected /*= false /* 是 窆说粞≈械?*/ )
		{
			EnterCriticalSection(&m_cs);
			x = 0.0;
			y = 0.0;
			z = 0.0;

			irr::core::recti irrRect; // 屏幕矩形
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);

			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			// 获得方向包围盒
			CHdobBox3d obb_view_box;
			{
				// 相关参数传入准备
				ISceneManager* smgr = SceneManager;
				ICameraSceneNode* camera = smgr->getActiveCamera();

				// 由屏幕容差像素记录
				irr::core::vector2di left_up;
				irr::core::vector2di left_down;
				irr::core::vector2di right_up;
				irr::core::vector2di right_down;
				irr::core::vector2di center_pos;

				// 像素设置
				int heighth = m_pView->GetWindowHeight();
				int width = m_pView->GetWindowWidth();

				// 像素计算
				left_up.set(hd::clamp(srcX - tol,0,width),hd::clamp(srcY + tol,0,heighth));
				left_down.set(hd::clamp(srcX - tol,0,width),hd::clamp(srcY - tol,0,heighth));
				right_up.set(hd::clamp(srcX + tol,0,width),hd::clamp(srcY + tol,0,heighth));
				right_down.set(hd::clamp(srcX + tol,0,width),hd::clamp(srcY - tol,0,heighth));
				center_pos.set(hd::clamp(srcX,0,width),hd::clamp(srcY,0,heighth));

				// 获得相机近平面、远平面
				core::plane3df near_plane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_NEAR_PLANE];
				core::plane3df far_plane = camera->getViewFrustum()->planes[camera->getViewFrustum()->VF_FAR_PLANE];

				// 计算屏幕像素中点
				core::line3df line_center = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(center_pos);

				// 计算四个角点相机交线
				core::line3df line_left_up = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(left_up);
				core::line3df line_left_down = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(left_down);
				core::line3df line_right_up = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(right_up);
				core::line3df line_right_down = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(right_down);

				// 计算射线与远平面四个交点
				core::vector3df far_left_up;
				core::vector3df far_left_down;
				core::vector3df far_right_up;
				core::vector3df far_right_down;
				far_plane.getIntersectionWithLine(line_left_up.start,line_left_up.getVector().normalize(),far_left_up);
				far_plane.getIntersectionWithLine(line_left_down.start,line_left_down.getVector().normalize(),far_left_down);
				far_plane.getIntersectionWithLine(line_right_up.start,line_right_up.getVector().normalize(),far_right_up);
				far_plane.getIntersectionWithLine(line_right_down.start,line_right_down.getVector().normalize(),far_right_down);

				// 计算中线在近平面、远平面交点
				core::vector3df far_center;
				core::vector3df near_center;
				far_plane.getIntersectionWithLine(line_center.start,line_center.getVector().normalize(),far_center);
				near_plane.getIntersectionWithLine(line_center.start,line_center.getVector().normalize(),near_center);

				// 计算长度值及向量
				core::vector3df far_to_near_center_line = near_center - far_center;
				irr::f32 view_dist = far_to_near_center_line.getLength();
				far_to_near_center_line.normalize();

				// 计算对应在四个近平面交点
				core::vector3df near_left_up;
				core::vector3df near_left_down;
				core::vector3df near_right_up;
				core::vector3df near_right_down;

				near_left_up = far_left_up + far_to_near_center_line * view_dist;
				near_left_down = far_left_down + far_to_near_center_line * view_dist;
				near_right_up = far_right_up + far_to_near_center_line * view_dist;
				near_right_down = far_right_down + far_to_near_center_line * view_dist;

				f32 obb_view_box_vertex[24];

				// 使用相机trans[16]的前12位存储远平面四个角点，对应方向包围盒的0,1,2,3
				obb_view_box_vertex[0] = far_left_up.X;
				obb_view_box_vertex[1] = far_left_up.Y;
				obb_view_box_vertex[2] = far_left_up.Z;

				obb_view_box_vertex[3] = far_right_up.X;
				obb_view_box_vertex[4] = far_right_up.Y;
				obb_view_box_vertex[5] = far_right_up.Z;

				obb_view_box_vertex[6] = far_right_down.X;
				obb_view_box_vertex[7] = far_right_down.Y;
				obb_view_box_vertex[8] = far_right_down.Z;

				obb_view_box_vertex[9] = far_left_down.X;
				obb_view_box_vertex[10] = far_left_down.Y;
				obb_view_box_vertex[11] = far_left_down.Z;

				obb_view_box_vertex[12] = near_left_up.X;
				obb_view_box_vertex[13] = near_left_up.Y;
				obb_view_box_vertex[14] = near_left_up.Z;

				obb_view_box_vertex[15] = near_right_up.X;
				obb_view_box_vertex[16] = near_right_up.Y;
				obb_view_box_vertex[17] = near_right_up.Z;

				obb_view_box_vertex[18] = near_right_down.X;
				obb_view_box_vertex[19] = near_right_down.Y;
				obb_view_box_vertex[20] = near_right_down.Z;

				obb_view_box_vertex[21] = near_left_down.X;
				obb_view_box_vertex[22] = near_left_down.Y;
				obb_view_box_vertex[23] = near_left_down.Z;

				obb_view_box.SetVertex(obb_view_box_vertex);
			}


			core::vector3df coord;
			core::position2di screenPos;

			u32 srcMinDist = U32_MAX;
			u32 srcDist = 0;
			//候选点与点击点x/y方向的距离，应该使用有符号整形！必须考虑符号转型带来的严重影响，因为有符号负数赋值给无符号整形会变成一个很大的正数。 袁亮  20160923
			int srcDx = 0;
			int srcDy = 0;
			F64 ptX = 0.0;
			F64 ptY = 0.0;
			F64 ptZ = 0.0;

			if(m_SeaPointCloud == NULL || (!isVisible() && bFindVisibleOnly))
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			int size = m_pHdlstArea->size();
			if (size <=0 )
			{
				LeaveCriticalSection(&m_cs);
				return srcMinDist;
			}

			core::aabbox3df parcebox;
			CHdobBox3d obb_parcel_box;
			f32 obb_parcel_box_vertex[24];

			for (int k=0;k < size;k++)
			{
				// 获取数据
				CHdParcelBase * pHdParcelBase = m_pHdlstArea->at(k);

				if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
				{
					continue;
				}

				CHdBox3df extent = pHdParcelBase->GetExtent();

				float xmin = extent.MinEdge.X;
				float ymin = extent.MinEdge.Y;
				float zmin = extent.MinEdge.Z;
				float xmax = extent.MaxEdge.X;
				float ymax = extent.MaxEdge.Y;
				float zmax = extent.MaxEdge.Z;

				if (m_bTrans)
				{

					m_renderModel.Translate(xmin,ymin,zmin);
					m_renderModel.Translate(xmax,ymax,zmax);
					//parcebox.MinEdge.set(xmin,ymin,zmin);
					//parcebox.MaxEdge.set(xmax,ymax,zmax);
				}
				//else
				//{
					parcebox.MinEdge.set(xmin,ymin,zmin);
					parcebox.MaxEdge.set(xmax,ymax,zmax);
				//}

				// 角点赋值
				obb_parcel_box_vertex[0] = xmin;
				obb_parcel_box_vertex[1] = ymax;
				obb_parcel_box_vertex[2] = zmin;

				obb_parcel_box_vertex[3] = xmax;
				obb_parcel_box_vertex[4] = ymax;
				obb_parcel_box_vertex[5] = zmin;

				obb_parcel_box_vertex[6] = xmax;
				obb_parcel_box_vertex[7] = ymin;
				obb_parcel_box_vertex[8] = zmin;

				obb_parcel_box_vertex[9] = xmin;
				obb_parcel_box_vertex[10] = ymin;
				obb_parcel_box_vertex[11] = zmin;

				obb_parcel_box_vertex[12] = xmin;
				obb_parcel_box_vertex[13] = ymax;
				obb_parcel_box_vertex[14] = zmax;

				obb_parcel_box_vertex[15] = xmax;
				obb_parcel_box_vertex[16] = ymax;
				obb_parcel_box_vertex[17] = zmax;

				obb_parcel_box_vertex[18] = xmax;
				obb_parcel_box_vertex[19] = ymin;
				obb_parcel_box_vertex[20] = zmax;

				obb_parcel_box_vertex[21] = xmin;
				obb_parcel_box_vertex[22] = ymin;
				obb_parcel_box_vertex[23] = zmax;

				obb_parcel_box.SetVertex(obb_parcel_box_vertex);

				//if (!rgnFrustum.isCubeIn(parcebox))
				//{
				//	continue;
				//}

				// 不相交则不处理
				if (obb_view_box.BoxIntersect(&obb_parcel_box) <= 0)
				{
					continue;
				}

				//获取点数
				int count = pHdParcelBase->GetPtCount();

				// 获取数据
				for (int ptIndex=0;ptIndex<count;ptIndex++ )
				{
					HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+ptIndex);

					if (!pt)
					{
						continue;
					}

					// 此处根据该节点显示模式进行判断
					if (((m_showStyle == SHOW_UNSELECT) && pt->isSelected())||
						((m_showStyle == SHOW_SELECT) && !pt->isSelected()))
					{
						continue;
					}

					//if (bSelected && pt->isSelected()) // 过滤掉选中点
					//{
					//	continue;
					//}



					if (m_bTrans)
					{			
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;
						m_renderModel.Translate(ptX,ptY,ptZ);
						coord.set((float)ptX,(float)ptY,(float)ptZ);
					}
					else
					{
						coord.set(pt->x,pt->y,pt->z);
					}

					screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
					if (!irrRect.isPointInside(screenPos))
					{
						continue;
					}

					srcDx = screenPos.X - srcX;
					srcDy = screenPos.Y - srcY;
					srcDist = srcDx * srcDx + srcDy * srcDy;
					if (srcDist < srcMinDist)
					{
						srcMinDist = srcDist;
						ptX = pt->x;
						ptY = pt->y;
						ptZ = pt->z;

						// 返回的绝对坐标
						m_absModel.Translate(ptX,ptY,ptZ);
						x = ptX;
						y = ptY;
						z = ptZ;

						// 返回的相对坐标
						ptPoint.x = pt->x;
						ptPoint.y = pt->y;
						ptPoint.z = pt->z;
						ptPoint.intensity = pt->intensity;
						//保存点的颜色信息  袁亮  20160625
						ptPoint.r = pt->color.getRed();
						ptPoint.g = pt->color.getGreen();
						ptPoint.b = pt->color.getBlue();

					}
				}//	for (int ptIndex=0;ptIndex<count;ptIndex++ )
			}//for (int k=0;k < size;k++)

			LeaveCriticalSection(&m_cs);
			return srcMinDist;
		}

	}
}
