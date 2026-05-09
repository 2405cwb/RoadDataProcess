#pragma once
#include "iobjectscenenode.h"
#include "..\hd3Dengine\include\irrlicht.h"
#include "..\hdPointCloud\HdPtCloud.h"
#include "ISceneView.h"
using namespace irr;
using namespace irr::video;
namespace hd
{	
	namespace scene
	{
		class CILevelSceneNode;	
class HD3DSCENE_API CIScanSceneNode :
	public IObjectSceneNode
{
private:
	//CHL2Reader m_hl2Reader;
	CHdPtCloud m_ptCloud;
	U32        m_loadLimit;
	//! 材质,每个SceneNode必须包含此对象
	video::SMaterial	m_material;
	//! 点云块外包范围
	core::aabbox3d<f32> m_box;
	//! ISceneView
	ISceneView* m_pSceneView;
	//! 默认颜色
	SColorf		m_defaultClr;

	vector<PointXYZ> m_ptBuf;
	/************************************************************************/
	/*                   ISceneNode接口实现                                 */
	/************************************************************************/
public:
	virtual void OnRegisterSceneNode();

	virtual void render();
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const;

	virtual u32 GetMaterialCount() const;

	virtual video::SMaterial& GetMaterial(u32 i);

	virtual ESCENE_NODE_TYPE getType() const { return ESNT_TESTSCAN_POINT; }
public:
	CIScanSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);
	virtual ~CIScanSceneNode(void);

	void SetPath(const char* path);

	BOOL ReloadData();
	
	u64 GetPointCount(){return m_ptCloud.count();}
};

	}
}