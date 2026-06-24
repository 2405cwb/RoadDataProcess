#pragma once
#include "IObjectSceneNode.h"
#include "..\..\hd3DEngine\include\irrlicht.h"

using namespace irr;
using namespace hd::scene;

namespace hd
{
	namespace scene
	{
class HD3DSCENE_API CMeshPolySceneNode : 
	public IObjectSceneNode
{
public:
	//! constructor
	CMeshPolySceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
	~CMeshPolySceneNode(void);

	//! pre render event
	virtual void OnRegisterSceneNode();

	//! render
	virtual void render();

	//! returns the axis aligned bounding box of this node
	virtual const core::aabbox3d<f32>& getBoundingBox() const;

	void UpdateBoundingBox();

	virtual video::SMaterial& getMaterial(u32 i);

	//! returns amount of materials used by this scene node.
	virtual u32 getMaterialCount() const;			

	//! Writes attributes of the scene node.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

	//! Reads attributes of the scene node.
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

	//! Returns type of the scene node
	virtual ESCENE_NODE_TYPE getType() const { return ESNT_MESHPOLY; }

	//! Creates a clone of this scene node and its children.
	virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);
	
	void setPoint(core::vector3df& pt,core::vector3df& normal)
	{
		m_centerPt = pt;
		m_normal = normal;

		m_bbox.MinEdge.set(m_centerPt);
		m_bbox.MaxEdge.set(m_centerPt);
	}
private:	
	core::aabbox3d<f32>		m_bbox;			//BoundingBox
	core::vector3df			m_centerPt;		//以平面方式绘制圆
	core::vector3df			m_normal;		//圆的法向量
	video::SMaterial		m_Material;
};

	}
}
