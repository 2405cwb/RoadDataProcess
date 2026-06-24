
#include "StdAfx.h"
#include "CScanPartPointsSceneNode.h"
#include "..\hdCommon\CClassificationMap.h"

namespace hd
{
	namespace scene
	{
		CScanPartPointsSceneNode::CScanPartPointsSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id),
			m_colorRamp(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),m_colorRampCycle(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4)
		{
			::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000402 );

			m_Material.Wireframe = false;
			m_Material.Lighting = false;
			setAutomaticCulling(irr::scene::EAC_OFF);

			m_nMinIntensity = INT_MAX;
			m_nMaxIntensity = 0;

			m_fMinHeight = 0.0f;
			m_fMaxHeight = 0.0f;

			m_fStep = 0.05f;
			m_renderStyle = RENDER_BY_INTENSITY; // 指定默认渲染方式

			m_brdrBox = false;

			m_bShowIntenRender = TRUE; // 是否显示透明度
			m_nAxis = 0; // 默认循环色带按Z方向

			// 顶点坐标
			m_vertices = NULL;   

			// 顶点颜色
			m_verColors = NULL; 

			// 点个数
			m_nPtNum = 0;

			m_rendStyle = 0;

			// 初始内存大小为3000000
			m_nMemery = 3000000;

			// 一次申请m_nMemery* 3 的内存,通过当前记录传入的点数顺序拷贝内存赋值，以此达到显示数据一直保持在m_nMemery数据量，并保持先进先出顺序;
			m_vertices = new GLfloat[m_nMemery*3];
			m_verColors = new GLfloat[m_nMemery*3];
			m_nAllInNum = 0;
			m_nCurPosition = 0;

			// 设置一个1 * 1 的初始包围盒;
			m_BBox.MinEdge.X = 0.0;
			m_BBox.MinEdge.Y = 0.0;
			m_BBox.MinEdge.Z = 0.0;
			m_BBox.MaxEdge.X = 1.0;
			m_BBox.MaxEdge.Y = 1.0;
			m_BBox.MaxEdge.Z = 1.0;
		}

		void CScanPartPointsSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
				SceneManager->registerNodeForRendering(this);

			ISceneNode::OnRegisterSceneNode();
		}

		CScanPartPointsSceneNode::~CScanPartPointsSceneNode(void)
		{
			delete[] m_vertices;
			delete[] m_verColors;
		}

		const core::aabbox3d<f32>& CScanPartPointsSceneNode::getBoundingBox() const
		{
			return m_BBox;
		}

		void CScanPartPointsSceneNode::UpdateBoundingBox()
		{
			if (!m_pView)
			{
				return;
			}

			m_BBox.MinEdge.X = F32_MAX;
			m_BBox.MinEdge.Y = F32_MAX;
			m_BBox.MinEdge.Z = F32_MAX;
			m_BBox.MaxEdge.X = F32_MIN;
			m_BBox.MaxEdge.Y = F32_MIN;
			m_BBox.MaxEdge.Z = F32_MIN;

			hnCommon::hnPointXYZIF pt;

			// 与已有的包围盒范围进行比较;
			int count = m_pPartPcd.size();
			for (int i = 0; i< count;i++)
			{
				pt = m_pPartPcd.at(i);

				if (pt.x < m_BBox.MinEdge.X )
				{
					m_BBox.MinEdge.X  = pt.x;
				}
				if (pt.x > m_BBox.MaxEdge.X)
				{
					m_BBox.MaxEdge.X = pt.x;
				}
				if (pt.y < m_BBox.MinEdge.Y)
				{
					m_BBox.MinEdge.Y = pt.y;
				}
				if (pt.y > m_BBox.MaxEdge.Y)
				{
					m_BBox.MaxEdge.Y = pt.y;
				}
				if (pt.z < m_BBox.MinEdge.Z)
				{
					m_BBox.MinEdge.Z = pt.z;
				}
				if (pt.z > m_BBox.MaxEdge.Z)
				{
					m_BBox.MaxEdge.Z = pt.z;
				}
			}

			m_fStep = (m_BBox.MaxEdge.Z - m_BBox.MinEdge.Z) / 10.0f;
		}

		// 更新包围盒
		void CScanPartPointsSceneNode::UpdateBoundingBox(vector<hnCommon::hnPointXYZIF>& pPartPts,bool isFirst)
		{
			if (!m_pView)
			{
				return;
			}

			// 如果不是第一次;
			if (isFirst)
			{
				m_BBox.MinEdge.X = F32_MAX;
				m_BBox.MinEdge.Y = F32_MAX;
				m_BBox.MinEdge.Z = F32_MAX;
				m_BBox.MaxEdge.X = F32_MIN;
				m_BBox.MaxEdge.Y = F32_MIN;
				m_BBox.MaxEdge.Z = F32_MIN;
			}


			hnCommon::hnPointXYZIF pt;

			for (int i = 0; i< m_nPtNum;i++)
			{
				pt = pPartPts.at(i);

				if (pt.x < m_BBox.MinEdge.X )
				{
					m_BBox.MinEdge.X  = pt.x;
				}
				if (pt.x > m_BBox.MaxEdge.X)
				{
					m_BBox.MaxEdge.X = pt.x;
				}
				if (pt.y < m_BBox.MinEdge.Y)
				{
					m_BBox.MinEdge.Y = pt.y;
				}
				if (pt.y > m_BBox.MaxEdge.Y)
				{
					m_BBox.MaxEdge.Y = pt.y;
				}
				if (pt.z < m_BBox.MinEdge.Z)
				{
					m_BBox.MinEdge.Z = pt.z;
				}
				if (pt.z > m_BBox.MaxEdge.Z)
				{
					m_BBox.MaxEdge.Z = pt.z;
				}
			}
		}

		video::SMaterial& CScanPartPointsSceneNode::getMaterial(u32 i)
		{
			return m_Material;
		}

		u32 CScanPartPointsSceneNode::getMaterialCount() const
		{
			return 1;
		}

		void CScanPartPointsSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
		{
		}

		void CScanPartPointsSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
		{
		}

		ISceneNode* CScanPartPointsSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
		{
			if (!newParent)
				newParent = Parent;
			if (!newManager)
				newManager = SceneManager;

			CScanPartPointsSceneNode* nb = new CScanPartPointsSceneNode(newParent, newManager, ID);

			nb->cloneMembers(this, newManager);

			if ( newParent )
				nb->drop();
			return nb;
		}

		// 渲染回调函数
		void CScanPartPointsSceneNode::render()
		{
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
			
			if (!camera || !driver ||! m_pView)
				return;
			if (m_pView->GetViewType() != E_HVT_3D 
				/*&& m_pView->GetViewType() != E_HVT_PLANAR*/)
				return;

			if (m_pPartPcd.size() <= 0)
			{
				return;
			}
			// 设置场景结点绝对位置
			core::matrix4 mat(AbsoluteTransformation);

			// 设置转换至世界坐标系
			driver->setTransform(video::ETS_WORLD,mat);

			// 设置材质
			f32 lastSize = m_Material.Thickness;

			driver->setMaterial(m_Material);

			// 设置3D渲染模式
			driver->setRenderStates3DMode();

			if (m_brdrBox) 
			{
				driver->draw3DBox(m_BBox, video::SColor(255,255,255,255));
			}

			if (m_rendStyle)
			{
				// 锁定代码块
				EnterCriticalSection(&m_cs);

				// 渲染点云
				RenderPoint();

				LeaveCriticalSection(&m_cs);
			}
			else
			{
				// 锁定代码块
				EnterCriticalSection(&m_cs);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glRasterPos3f(10,10,0);
				glPointSize(2.0);
				glBegin(GL_POINTS);

				// 渲染点云
				RenderPoint();

				glEnd();
				glDisable(GL_BLEND);

				LeaveCriticalSection(&m_cs);
			}
		}

		// 渲染点云
		void CScanPartPointsSceneNode::RenderPoint()
		{
			long t1,t2;
			t1= t2 = 0.0;

			t1 = clock();

			if (m_rendStyle)
			{
				if (m_nPtNum <= 0)
				{
					return;
				}

				if (!m_vertices)
				{
					return;
				}

				glEnableClientState(GL_COLOR_ARRAY);
				glEnableClientState(GL_VERTEX_ARRAY);

				glColorPointer(3, GL_FLOAT, 0, m_verColors);
				glVertexPointer(3, GL_FLOAT, 0, m_vertices);

				glDrawArrays(GL_POINTS, 0 ,m_nPtNum);

				glDisableClientState(GL_VERTEX_ARRAY);
				glDisableClientState(GL_COLOR_ARRAY);

				t2 = clock();
				char strText[128];
				sprintf_s(strText,"%s%ld\n","ReadPoint耗时: ",t2-t1);
				OutputDebugString(strText);

				return;
			}

			// 加载完成之后进行显示
			RenderColor color;

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRamp.GetBeginColor();
			u32 endColor = m_colorRamp.GetEndColor();

			float scale = 0.0f;				// 缩放
			u8 a = 0;		                 // 颜色值

			int selStep = 0;				// 步长
			float tempValue = 0.0f;			// 临时值
			float height = 0.0f;			// 按x或y或z
			hnCommon::hnPointXYZIF pt;
			
			for (int i = 0; i < m_pPartPcd.size(); i++)
			{
				pt = m_pPartPcd.at(i);

				// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染
				if ( pt.z < m_BBox.MinEdge.Z)
				{
					color.r = ((beginColor>>16) & 0xff);
					color.g = ((beginColor>>8) & 0xff);
					color.b = (beginColor & 0xff);
				}

				// 对于大于最大值的点则按照终止颜色渲染
				else if (pt.z > m_BBox.MaxEdge.Z)
				{
					color.r = ((endColor>>16) & 0xff);
					color.g = ((endColor>>8) & 0xff);
					color.b = (endColor & 0xff);
				}

				// 其他在统计范围根据范围以及间隔进行计算
				else
				{
					selStep = (int)((pt.z - m_BBox.MinEdge.Z) / m_fStep);

					tempValue = pt.z - m_BBox.MinEdge.Z - selStep * m_fStep;
					scale = tempValue / m_fStep;
					m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);

					//GetColor4ub(f32 scale, u8& a, u8& r, u8& g, u8& b, int step)

					int test = 0;
				}

				glColor4ub(color.r, color.g, color.b, 255);

				glVertex3f(pt.x,pt.y,pt.z);
			}

			//t2 = clock();
			//char strText[128];
			//sprintf_s(strText,"%s%ld\n","ReadPoint耗时: ",t2-t1);
			//OutputDebugString(strText);
		}

		// 外部更新点云
		bool CScanPartPointsSceneNode::SetPartPoints(vector<hnCommon::hnPointXYZIF>& pPartPts,bool bIntial)
		{
			ISceneView* p3DView = dynamic_cast<ISceneView*>(m_pView);

			if (!p3DView)
			{
				return false;
			}

			if (!m_rendStyle)
			{
				// 锁定代码块
				EnterCriticalSection(&m_cs);
				m_nPtNum = pPartPts.size();
				m_pPartPcd = pPartPts;

				LeaveCriticalSection(&m_cs);

				if (m_pPartPcd.size() <= 0)
				{
					return false;
				}

				UpdateBoundingBox();

				return true;
			}

			// 需要初始化;
			if (bIntial)
			{
				memset(m_vertices,0,sizeof(GLfloat) * m_nMemery * 3);
				memset(m_verColors,0,sizeof(GLfloat) * m_nMemery * 3);
				m_nCurPosition = 0;
				m_nAllInNum = 0;
			}
		
			// 锁定代码块
			EnterCriticalSection(&m_cs);

			// 需要添加的点数;
			int needAddPts = pPartPts.size();
			if (m_nAllInNum < m_nMemery)
			{
				m_nAllInNum += pPartPts.size();
			}

			UpdateBoundingBox(pPartPts);

			//if (m_vertices && m_nPtNum > m_nMemery)
			//{
			//	delete[] m_vertices;
			//	m_vertices= NULL;

   //             delete[] m_verColors;
			//	m_verColors = NULL;

			//	m_verColors = new GLfloat[m_nPtNum*3];
			//	m_vertices = new GLfloat[m_nPtNum*3];

			//	m_nMemery = m_nPtNum;
			//}

			// 统计坐标范围
			m_heightStep.clear();
			float maxHeight = 0.0f;
			float minHeight = 0.0f;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;
			m_heightStep.resize(10);

			//统计高度分布，映射至0-999
			memset(mapHeight,0,sizeof(int)*1000);

			int iCount = 0;
			float height = 0.0f;
			minHeight = m_BBox.MinEdge.Z;
			maxHeight = m_BBox.MaxEdge.Z;
			//float boxHeight = maxHeight - minHeight;
			//float boxHeightRcp = 1000.0f / (boxHeight);

			////#pragma omp parallel for
			//for (u32 n = 0; n<m_nPtNum; n++)
			//{
			//	hnPoint3d pt = *(pPartPts._Myfirst + n);

			//	selNum = (int)((pt.z - minHeight) * boxHeightRcp);
			//	selNum = clamp(selNum,0,999);
			//	mapHeight[selNum]++;
			//}				

			////调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
			//selNum = 0;
			//for (int j = 999;j >= 0;j--)
			//{
			//	selNum += mapHeight[j];
			//	if (selNum >= (int)(m_nPtNum * 0.03))
			//	{
			//		selMax = j;
			//		break;
			//	}
			//}

			//selNum = 0;
			//for (int j = 0;j <= 999;j++)
			//{
			//	selNum += mapHeight[j];
			//	if (selNum >= (int)(m_nPtNum * 0.03))
			//	{
			//		selMin = j;
			//		break;
			//	}
			//}

			////重新设置新的最大最小高程值
			//minHeight = m_BBox.MinEdge.Z + selMin * (boxHeight) / 1000.0f;
			//maxHeight = m_BBox.MinEdge.Z + selMax * (boxHeight) / 1000.0f;

			//float tempHeight = (maxHeight - minHeight) / 10.0f;
			//for (int i = 0; i < 10;i++)
			//{
			//	m_heightStep[i] = minHeight + (i + 1) * tempHeight;
			//}

			m_fMinHeight = minHeight;
			m_fMaxHeight = maxHeight;
			//////////////////////////////////////

			// 加载完成之后进行显示
			RenderColor color;

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRamp.GetBeginColor();
			u32 endColor = m_colorRamp.GetEndColor();

			float scale = 0.0f;				// 缩放
			u8 a = 0;		                 // 颜色值

			int selStep = 0;				// 步长
			float tempValue = 0.0f;			// 临时值
			/*float */height = 0.0f;			// 按x或y或z
			hnCommon::hnPointXYZIF pt;

			for (int i = 0; i < pPartPts.size();i++)
			{
				pt = pPartPts.at(i);

				// 统计最值时将孤点去掉，对于小于最小值的点则按照起始颜色渲染;
				if ( pt.z < m_fMinHeight)
				{
					color.r = ((beginColor>>16) & 0xff);
					color.g = ((beginColor>>8) & 0xff);
					color.b = (beginColor & 0xff);
				}

				// 对于大于最大值的点则按照终止颜色渲染
				else if (pt.z > m_fMaxHeight)
				{
					color.r = ((endColor>>16) & 0xff);
					color.g = ((endColor>>8) & 0xff);
					color.b = (endColor & 0xff);
				}

				// 其他在统计范围根据范围以及间隔进行计算;
				else
				{
					selStep = (int)((pt.z - m_fMinHeight) / m_fStep);

					tempValue = pt.z - m_fMinHeight - selStep * m_fStep;
					scale = tempValue / m_fStep;
					m_colorRamp.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
				}

				// 判断索引信息是否溢出;
				int ncurIndex = m_nCurPosition + i;
				if (ncurIndex >= m_nMemery)
				{
					ncurIndex = ncurIndex - m_nMemery;
				}

				m_vertices[ ncurIndex * 3 ] = pt.x;
				m_vertices[ncurIndex * 3 + 1] = pt.y;
				m_vertices[ncurIndex * 3 + 2] = pt.z;

				m_verColors[ ncurIndex * 3 ] = (float)color.r / 255.0;
				m_verColors[ ncurIndex * 3 + 1] = (float)color.g / 255.0;
				m_verColors[ ncurIndex * 3 + 2] = (float)color.b / 255.0;
			}

			// 记录当前显示的点的位置;
			int curPosition = m_nCurPosition + pPartPts.size();
			if (curPosition >= m_nMemery)
			{
				curPosition = curPosition - m_nMemery;
			}
			m_nCurPosition = curPosition;

			// 更新点数信息,当前已添加的总点数大于显示阈值，则需要显示的点数即为缓存区大小;
			if (m_nAllInNum >= m_nMemery)
			{
				m_nPtNum = m_nMemery;
			}
			else
			{
				m_nPtNum = m_nAllInNum;
			}

			LeaveCriticalSection(&m_cs);

			// 重新更新包围盒;
			m_BBox.MinEdge.X = F32_MAX;
			m_BBox.MinEdge.Y = F32_MAX;
			m_BBox.MinEdge.Z = F32_MAX;
			m_BBox.MaxEdge.X = F32_MIN;
			m_BBox.MaxEdge.Y = F32_MIN;
			m_BBox.MaxEdge.Z = F32_MIN;

			// 与已有的包围盒范围进行比较;
			float fTempX,fTempY,fTempZ;
			int count = m_pPartPcd.size();
			for (int i = 0; i < m_nPtNum;i++)
			{
				fTempX = m_vertices[ i * 3 ];
				fTempY = m_vertices[i * 3 + 1];
				fTempZ = m_vertices[i * 3 + 2];

				if (fTempX < m_BBox.MinEdge.X )
				{
					m_BBox.MinEdge.X  = fTempX;
				}
				if (fTempX > m_BBox.MaxEdge.X)
				{
					m_BBox.MaxEdge.X = fTempX;
				}
				if (fTempY < m_BBox.MinEdge.Y)
				{
					m_BBox.MinEdge.Y = fTempY;
				}
				if (fTempY > m_BBox.MaxEdge.Y)
				{
					m_BBox.MaxEdge.Y = fTempY;
				}
				if (fTempZ < m_BBox.MinEdge.Z)
				{
					m_BBox.MinEdge.Z = fTempZ;
				}
				if (fTempZ > m_BBox.MaxEdge.Z)
				{
					m_BBox.MaxEdge.Z = fTempZ;
				}
			}

			
			return true;
		}

	}
}

