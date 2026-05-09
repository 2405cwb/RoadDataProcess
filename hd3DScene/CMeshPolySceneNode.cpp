#include "StdAfx.h"
#include "CMeshPolySceneNode.h"
#include "matrix4.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{
	namespace scene
	{

CMeshPolySceneNode::CMeshPolySceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
	:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent, mgr, id)
{
#ifdef _DEBUG
	setDebugName("CMeshPolySceneNode");
#endif

	m_Material.Wireframe = false;
	m_Material.Lighting = false;
	setAutomaticCulling(irr::scene::EAC_OFF);
}


CMeshPolySceneNode::~CMeshPolySceneNode(void)
{
}

void CMeshPolySceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}

void CMeshPolySceneNode::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	irr::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
	
	if (!camera || !driver ||! m_pView)
		return;
	if (m_pView->GetViewType() != E_HVT_QUICK &&
		m_pView->GetViewType() != E_HVT_3D)
		return;

	if (m_centerPt.equals(core::vector3df()))
	{
		return;
	}

	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	driver->setMaterial(m_Material);

	static core::vector3df normalZ(90.0f,90.0f,0.0f);

	m_normal.normalize();
	core::plane3df plane(m_centerPt,m_normal);
	
	irr::core::matrix4 mat;
	core::vector3df rotate = m_normal.getHorizontalAngle();

	f32 dist =/* 0.4f*/1.0f;	//zfei 增加矩形框的大小2013-7-15

	//使用红线绘制法向量
	m_Color.set(0,255,0,0);
	driver->draw3DLine(m_centerPt,m_centerPt + 2*m_normal,m_Color);	//zfei 增加法线的长度2013-7-15

	//绘制面
	m_Color.set(100,0,200,0);
	if (fabs(fabs(rotate.Z) - 270.0f ) < 20.0f ||
		fabs(fabs(rotate.Z) - 90.0f ) < 20.0f)//垂直水平面
	{
		core::position2di scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(m_centerPt);
		driver->draw2DEllipse(scrPos,60,30,m_Color);
	}
	else
	{
		core::position2di scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(m_centerPt);
		rotate.Z = 0.0f;
		
		mat.setRotationDegrees(rotate);
		
		core::position2df ptScr[4];

		core::vector3df off(-dist,-dist,0);
		mat.rotateVect(off);
		core::vector3df pt(m_centerPt.X + off.X ,m_centerPt.Y + off.Y,m_centerPt.Z + off.Z);
		scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);
		ptScr[0].X = (f32)scrPos.X;
		ptScr[0].Y = (f32)scrPos.Y;

		off.set(-dist,dist,0);
		mat.rotateVect(off);
		pt.set(m_centerPt.X + off.X,m_centerPt.Y + off.Y,m_centerPt.Z + off.Z);
		scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);
		ptScr[1].X = (f32)scrPos.X;
		ptScr[1].Y = (f32)scrPos.Y;

		off.set(dist,dist,0);
		mat.rotateVect(off);
		pt.set(m_centerPt.X + off.X,m_centerPt.Y + off.Y,m_centerPt.Z + off.Z);
		scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);
		ptScr[2].X = (f32)scrPos.X;
		ptScr[2].Y = (f32)scrPos.Y;

		off.set(dist,-dist,0);
		mat.rotateVect(off);
		pt.set(m_centerPt.X + off.X,m_centerPt.Y + off.Y,m_centerPt.Z + off.Z);
		scrPos = getSceneManager()->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(pt);
		ptScr[3].X = (f32)scrPos.X;
		ptScr[3].Y = (f32)scrPos.Y;

		bool bDraw;
		driver->draw2DPoly(bDraw,ptScr,4,m_Color);
		bDraw = bDraw;
	}
}

const core::aabbox3d<f32>& CMeshPolySceneNode::getBoundingBox() const
{
	return m_bbox;
}

void CMeshPolySceneNode::UpdateBoundingBox()
{

}

video::SMaterial& CMeshPolySceneNode::getMaterial( u32 i )
{
	return m_Material;
}

irr::u32 CMeshPolySceneNode::getMaterialCount() const
{
	return 1;
}

void CMeshPolySceneNode::serializeAttributes( io::IAttributes* out, io::SAttributeReadWriteOptions* options/*=0*/ ) const
{

}

void CMeshPolySceneNode::deserializeAttributes( io::IAttributes* in, io::SAttributeReadWriteOptions* options/*=0*/ )
{

}

ISceneNode* CMeshPolySceneNode::clone( ISceneNode* newParent/*=0*/, ISceneManager* newManager/*=0*/ )
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	CMeshPolySceneNode* nb = new CMeshPolySceneNode(newParent, newManager, ID);

	nb->cloneMembers(this, newManager);

	if ( newParent )
		nb->drop();
	return nb;
}

	}
}