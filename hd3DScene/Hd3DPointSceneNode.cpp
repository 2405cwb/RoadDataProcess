/*! Hd3DPointSceneNode.cpp
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : Hd3DPointSceneNode.cpp
相关文件     : Hd3DPointSceneNode.h
文件实现功能 : 3D视图中点场景结点显示，不实时生成点节点，采用移动点的方式，防止
			  频繁实时生成释放点导致程序崩溃	   
作者         : 研发部 冯晶
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/04/29    1.0     冯晶			   创建并实现三维视图点场景结点显示
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "Hd3DPointSceneNode.h"
#include "..\hdPointCloud\hdSysSetting.h"


// 构造
CHdPointSceneNode3D::CHdPointSceneNode3D(CHdSxPoint3D* point, EHD_POINT_STYLE style, ISceneNode* parent, ISceneManager* mgr, int id)
	: IObjectSceneNode(NULL, video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
{
	// 初始化材质
	m_Material.Wireframe = false;
	m_Material.Lighting = false;
	setAutomaticCulling(irr::scene::EAC_OFF);

	// 设置点的样式
	m_Style = style;

	// 传入显示坐标
	m_pPoint = point;

	// 设置包围盒
	m_BBox.MinEdge.set(core::vector3df((float)m_pPoint->m_x,(float) m_pPoint->m_y, (float)m_pPoint->m_z));
	m_BBox.MaxEdge.set(core::vector3df((float)m_pPoint->m_x, (float)m_pPoint->m_y, (float)m_pPoint->m_z));
}

// 析构函数
CHdPointSceneNode3D::~CHdPointSceneNode3D(void)
{
}

// pre render event
void CHdPointSceneNode3D::OnRegisterSceneNode()
{
	// 对于可见的场景结点注册渲染
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}

// 渲染
void CHdPointSceneNode3D::render()
{
	// 获取the video driver
	video::IVideoDriver* driver = SceneManager->getVideoDriver();

	// 获取current active camera
	irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

	// 判断合法性
	if (!camera || !driver || !m_pPoint ||! m_pView)
	{
		return;
	}

	// sets transformation
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

	// 设置纹理
	driver->setMaterial(m_Material);

	ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);

	// 判断视图的合法性
	if (!m_pPoint || (!m_pPoint->isPixelPoint() && pView->GetViewType() != E_HVT_3D &&
		pView->GetViewType() != E_HVT_ORTHO3D && pView->GetViewType() != E_HVT_MULTISCAN3D &&
		pView->GetViewType() != E_HVT_3DVIEWMATCH && pView->GetViewType() != E_HVT_SKETCH_ISCAN3D))
	{
		return;
	}

	//画点时，点的大小，又像素为单位计算

	//标记点的大小，江静，2015/9/11
	int nPtSize = 5;              
	nPtSize=hd::CHdSysSetting::getSysSetting()->measureSetting.MarkPointSize;
	//m_Material.Thickness=nPtSize;

	core::vector3df centerPos;						// 三维坐标
	core::position2di screenPos;					// 屏幕坐标		

	// 将三维坐标换算成屏幕坐标
	centerPos.X = (float)m_pPoint->m_x;
	centerPos.Y = (float)m_pPoint->m_y;
	centerPos.Z = (float)m_pPoint->m_z;
	screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(centerPos);									

	// 给定点的颜色为红色
	video::SColor curColor = video::SColor(255, 255, 0, 0);
	video::SColor color = m_bSelected?m_SelectedColor:curColor;

	// 根据样式绘制点
	switch (m_Style)
	{
	case EPS_POINT:
		{
			driver->drawPixel(screenPos.X, screenPos.Y, color);
		}
		break;
	case EPS_CIRCLE:
		{
			driver->draw2DPolygon(screenPos, nPtSize,color,16);
			driver->drawPixel(screenPos.X, screenPos.Y, color);
		}
	case EPS_SQUARE:
		{
			driver->draw2DRectangle(core::rect<int>(screenPos.X - nPtSize,screenPos.Y - nPtSize,screenPos.X + nPtSize,screenPos.Y + nPtSize),
				color, color, color, color);
		}
	case EPS_CROSS:
		{
			driver->draw2DLine(core::vector2di(screenPos.X - nPtSize, screenPos.Y),
				core::vector2di(screenPos.X + nPtSize, screenPos.Y),color);
			driver->draw2DLine(core::vector2di(screenPos.X, screenPos.Y - nPtSize),
				core::vector2di(screenPos.X, screenPos.Y + nPtSize),color);
		}
		break;
	case EPS_CROSSANDCIRCLE:
		{
			driver->draw2DLine(core::vector2di(screenPos.X - nPtSize, screenPos.Y),
				core::vector2di(screenPos.X + nPtSize, screenPos.Y),color);
			driver->draw2DLine(core::vector2di(screenPos.X, screenPos.Y - nPtSize),
				core::vector2di(screenPos.X, screenPos.Y + nPtSize),color);
			driver->draw2DPolygon(screenPos, nPtSize,color,16);
			driver->drawPixel(screenPos.X, screenPos.Y, color);
		}
		break;
	} 
}

// 获取包围盒
const core::aabbox3d<float>& CHdPointSceneNode3D::getBoundingBox() const
{
	return m_BBox;
}

// 由于CPolyline对象会在外部被更新，所以需要有接口去更新SceneNode的范围
void CHdPointSceneNode3D::UpdateBoundingBox()
{
	m_BBox.MinEdge.set(core::vector3df((float)m_pPoint->m_x, (float)m_pPoint->m_y, (float)m_pPoint->m_z));
	m_BBox.MaxEdge.set(core::vector3df((float)m_pPoint->m_x, (float)m_pPoint->m_y, (float)m_pPoint->m_z));
}

// 获取材质
video::SMaterial& CHdPointSceneNode3D::getMaterial(unsigned int i)
{
	return m_Material;
}

// 场景结点使用材质数量
unsigned int CHdPointSceneNode3D::getMaterialCount() const
{
	return 1;
}

// 设置景结点属性
void CHdPointSceneNode3D::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
}

// 读场景结点属性
void CHdPointSceneNode3D::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
}


