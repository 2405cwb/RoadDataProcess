
#include "LasVisualization.h"
#include "HLSReader.h"
#include "COpenGLExtensionHandler.h"

#include <time.h>

#define SAMPLE_COUNT (500000.0)
namespace hd
{
	namespace hls
	{

		static float classification_colors[16][4] = 
		{{0.0f,0.0f,0.0f,0.3f},// created (black)
		{0.3f,0.3f,0.3f,0.3f}, // unclassified (grey)
		{0.7f,0.5f,0.5f,0.3f}, // ground (brown)
		{0.0f,0.8f,0.0f,0.3f}, // vegetation low (green)
		{0.2f,0.8f,0.2f,0.3f}, // vegetation medium (green)
		{0.4f,0.8f,0.4f,0.3f}, // vegetation hight (green)
		{0.2f,0.2f,0.8f,0.3f}, // building (light blue)
		{0.9f,0.4f,0.7f,0.3f}, // lowpoint (violett)
		{1.0f,0.0f,0.0f,0.3f}, // mass point (red)
		{0.0f,0.0f,1.0f,0.3f}, // water (blue)

		{0.3f,0.3f,0.3f,0.3f}, // user-defined (grey)
		{0.3f,0.3f,0.3f,0.3f}, // user-defined (grey)
		{1.0f,1.0f,0.0f,0.3f}, // overlap (yellow)
		{0.3f,0.3f,0.3f,0.3f}, // user-defined (grey)
		{0.3f,0.3f,0.3f,0.3f}, // user-defined (grey)
		{0.3f,0.3f,0.3f,0.3f}, // user-defined (grey)
		};


CLasVisualization::~CLasVisualization(void)
{
	//m_pointCloud.pts.clear();
	//m_pointCloud.header.clean();
	//m_smrPts.clear();
}

CLasVisualization::CLasVisualization(scene::ISceneNode* parent,scene::ISceneManager* mgr,s32 id)
	:scene::ISceneNode(parent,mgr,id),
	m_transparence(0.3),
	m_colorRamp1(COLORARGB(77,0,0,255),COLORARGB(77,255,0,0)),
	m_colorRamp2(COLORARGB(77,255,0,0),COLORARGB(77,0,255,255)),
	m_colorRamp3(COLORARGB(77,0,255,255),COLORARGB(77,0,0,255))
{
	m_renderCls = CLASS_ALL;
	m_renderStyle = RENDER_BY_RGB;
	m_simple = 1;
	m_material.Wireframe = false;
	m_material.Lighting = false;
	m_seed = time(NULL);
	m_maxIntensity_rcp = 1.0;
}

BOOL CLasVisualization::loadLasFile( const char* lasFile ,void (*loadCallback)(float ,const char*))
{
	if (SceneManager ==NULL || lasFile == NULL)
	{
		return FALSE;
	}

	BOOL bRet = m_pointCloud.loadLasFile(lasFile,loadCallback);
	if (bRet)
	{
		m_distImg.setMinMax(m_pointCloud.getMinDistance(),m_pointCloud.getMaxDistance());

		core::aabbox3d<f32> box((f32)(m_pointCloud.m_header.min_x),(f32)(m_pointCloud.m_header.min_y),
			(f32)(m_pointCloud.m_header.min_z),(f32)(m_pointCloud.m_header.max_x),
			(f32)(m_pointCloud.m_header.max_y),(f32)(m_pointCloud.m_header.max_z));

		m_box = box;
		//动态计算抽样比例
		m_simple = m_pointCloud.count() / SAMPLE_COUNT + 0.5;
		if(m_simple == 0) m_simple = 1;

		if(m_pointCloud.getMaxIntensity() != 0)
		{
			m_maxIntensity_rcp = 1.0f / m_pointCloud.getMaxIntensity();
		}
		// 自动检索靶球
		//m_smrPts.resize(500);
		//m_pointCloud.searchSMR(m_smrPts);
	}
	return bRet;
}

BOOL CLasVisualization::loadHlsFile( const char* hlsFile,void (*loadCallback)(float,const char*) )
{
	if (SceneManager ==NULL || hlsFile == NULL)
	{
		return FALSE;
	}

	BOOL bRet = m_pointCloud.loadHlsFile(hlsFile,loadCallback);
	if (bRet)
	{
		m_distImg.setMinMax(m_pointCloud.getMinDistance(),m_pointCloud.getMaxDistance());

		core::aabbox3d<f32> box((f32)(m_pointCloud.m_header.min_x),(f32)(m_pointCloud.m_header.min_y),
			(f32)(m_pointCloud.m_header.min_z),(f32)(m_pointCloud.m_header.max_x),
			(f32)(m_pointCloud.m_header.max_y),(f32)(m_pointCloud.m_header.max_z));
		
		m_box = box;
		// 动态计算抽样比例
		m_simple = m_pointCloud.count() / SAMPLE_COUNT + 0.5;
		if(m_simple == 0) m_simple = 1;

		if(m_pointCloud.getMaxIntensity() != 0)
		{
			m_maxIntensity_rcp = 1.0f / m_pointCloud.getMaxIntensity();
		}
		// 自动检索靶球
		//m_smrPts.resize(500);
		//m_pointCloud.searchSMR(m_smrPts);
	}
	return bRet;
}

void CLasVisualization::setRenderClass( const PointClass& renderCls )
{
	m_renderCls = renderCls;
}

PointClass CLasVisualization::getRenderClass() const
{
	return m_renderCls;
}

void CLasVisualization::setRenderStyle( const RenderStyle& renderStyle )
{
	m_renderStyle = renderStyle;
}

RenderStyle CLasVisualization::getRenderStyle() const
{
	return m_renderStyle;
}

void CLasVisualization::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this);
	}
	ISceneNode::OnRegisterSceneNode();
}

void CLasVisualization::render()
{
	if(!(m_pointCloud.count() > 0))
		return;

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setMaterial(m_material);
	driver->setTransform(video::ETS_WORLD,AbsoluteTransformation);
	
	driver->setRenderStates3DMode();
	glBegin(GL_POINTS);
	//设置随机数种子
	std::srand(m_seed);
	
	u32 i = 0;
	//m_simple * 2 - 1用于随机数求余，使得随机数平均数大概为m_simple;
	u32 simple = m_simple * 2 - 1;
	u32 simpleIdx = 1;
	float boundingBoxHeight = fabs(m_box.MaxEdge.Z - m_box.MinEdge.Z);
	// 显示原始点云
	for (i = 0;i < m_pointCloud.count();)
	{
		renderPoint(i,boundingBoxHeight);
		//根据随机数抽稀
		if (m_simple != 1 && SceneManager->IsAnimating())//
		{
			i += (rand() % simple + 1);
			/*i += simpleIdx;
			simpleIdx++;
			if(simpleIdx >= simple)
				simpleIdx = 1;*/
		}
		else
		{
			i++;
		}
	}
	// 显示靶球
	for (i = 0;i < m_smrPts.size();i++)
	{
		glColor3f(1.0f, 0.0f, 0.0f);	//用红色显示靶球
		glVertex3f(m_smrPts[i].x,m_smrPts[i].y,m_smrPts[i].z);
	}
	glEnd();
}

void CLasVisualization::renderPoint( u32 i ,float boundingBoxHeight)
{
	bool do_render = false;
	if (!m_pointCloud[i].isValid())
		return;
	// 判断是否显示
	switch (m_renderCls)
	{
	case CLASS_ALL:
		do_render = true;
		break;
	case CLASS_FIRST:
		if(m_pointCloud[i].prop & 64)do_render = true;
		do_render = true;
		break;
	case CLASS_LAST:
		if(m_pointCloud[i].prop & 128)do_render = true;
		break;
	case CLASS_GROUND:
		if((m_pointCloud[i].prop & 63) == 2)do_render = true;
		break;
	case CLASS_OBJECT:
		if((m_pointCloud[i].prop & 63) > 2 && (m_pointCloud[i].prop & 63) < 7)do_render = true;
		break;
	case CLASS_BUILDING:
		if((m_pointCloud[i].prop & 63) == 6)do_render = true;
		break;
	case CLASS_VEGETATION:
		if((m_pointCloud[i].prop & 63) > 2 && (m_pointCloud[i].prop & 63) < 6)do_render = true;
		break;
	case CLASS_MASS_POINTS:
		if((m_pointCloud[i].prop & 63) == 8)do_render = true;
		break;
	case CLASS_WATER:
		if((m_pointCloud[i].prop & 63) == 9)do_render = true;
		break;
	case CLASS_UNCLASSIFIED:
		if((m_pointCloud[i].prop & 63) == 1)do_render = true;
		break;
	case CLASS_OVERLAP:
		if((m_pointCloud[i].prop & 63) == 12)do_render = true;
		break;
	}

	if (do_render)
	{
		//判断使用的显示颜色
		if (m_renderStyle == RENDER_BY_RGB)
		{
			glColor4ub(m_pointCloud[i].r , m_pointCloud[i].g , m_pointCloud[i].b,m_transparence * 255);//m_pointCloud[i]._unused					
		}
		else if (m_renderStyle == RENDER_BY_INTENSITY)
		{
			float colorI = m_pointCloud[i].intensity * m_maxIntensity_rcp;//(float)m_pointCloud.getMaxIntensity();
			glColor4f(colorI,colorI,colorI,m_transparence);
		}
		else if (m_renderStyle == RENDER_BY_HEIGHT)
		{
			float height = m_pointCloud[i].z - m_box.MinEdge.Z;
			/*float color = height  * 8 / boundingBoxHeight;
			glColor3f(color ,color,color);*/
			//float color = 1.0f;
			float scale = 0.0f;
			float a = 0.0,r,g,b;
			if (height < (boundingBoxHeight/6))
			{
				scale = height/(boundingBoxHeight/6);
				m_colorRamp1.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
			else if (height < 2 * (boundingBoxHeight/6))
			{
				scale = (height-(boundingBoxHeight/6))/(boundingBoxHeight/6);
				m_colorRamp2.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
			else if (height < 3*(boundingBoxHeight/6))
			{
				scale = (height-2 * (boundingBoxHeight/6))/(boundingBoxHeight/6);
				m_colorRamp3.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
			else if (height < 4*(boundingBoxHeight/6))
			{
				scale = (height - 3 * (boundingBoxHeight/6))/(boundingBoxHeight/6);
				m_colorRamp1.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
			else if (height < 5*(boundingBoxHeight/6))
			{
				scale = (height - 4 * (boundingBoxHeight/6))/(boundingBoxHeight/6);
				m_colorRamp2.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
			else if (height < 6*(boundingBoxHeight/6))
			{
				scale = (height - 5 * (boundingBoxHeight/6))/(boundingBoxHeight/6);
				m_colorRamp3.GetColor4f(scale,a,r,g,b);
				glColor4f(r,g,b,m_transparence);
			}
		}
		else if (m_renderStyle == RENDER_BY_DISTANCE)
		{
			//float maxDist = m_box.MaxEdge.getLength();
			//core::vector3df pt(m_pointCloud[i].x,m_pointCloud[i].y,m_pointCloud[i].z);
			float dist = m_pointCloud[i].x * m_pointCloud[i].x + 
				m_pointCloud[i].y * m_pointCloud[i].y + 
				m_pointCloud[i].z * m_pointCloud[i].z;
			float r,g,b;
			m_distImg.GetRGB3FByDist(dist,r,g,b);
			glColor4f(r,g,b,m_transparence);
			//if (maxDist < 900)//小于900认为是以扫描原点为中心的坐标
			//{
			//	float dist =  pt.getLength();
			//	if (dist < (maxDist/3))
			//	{
			//		glColor4f(0.1f+0.7f*dist/(maxDist/3),0.1f,0.1f,m_transparence);
			//	}
			//	else if (dist < 2*(maxDist/3))
			//	{
			//		glColor4f(0.8f,0.1f+0.7f*(dist-(maxDist/3))/(maxDist/3),0.1f,m_transparence);
			//	}
			//	else
			//	{
			//		glColor4f(0.8f, 0.8f, 0.1f+0.7f*(dist-2*(maxDist/3))/(maxDist/3),m_transparence);
			//	}
			//}
			//else//以中心点为原点计算距离
			//{
			//	float dist =  pt.getDistanceFrom(m_box.getCenter());
			//	maxDist = m_box.getCenter().getDistanceFrom(m_box.MaxEdge);
			//	if (dist < (maxDist/3))
			//	{
			//		glColor4f(0.1f+0.7f*dist/(maxDist/3),0.1f,0.1f,m_transparence);
			//	}
			//	else if (dist < 2*(maxDist/3))
			//	{
			//		glColor4f(0.8f,0.1f+0.7f*(dist-(maxDist/3))/(maxDist/3),0.1f,m_transparence);
			//	}
			//	else
			//	{
			//		glColor4f(0.8f, 0.8f, 0.1f+0.7f*(dist-2*(maxDist/3))/(maxDist/3),m_transparence);
			//	}
			//}
		}
		else if (m_renderStyle == RENDER_BY_CLASS)
		{
			glColor4fv(classification_colors[m_pointCloud[i].prop & 15]);
		}
		//glVertex3f(m_pointCloud[i].x,m_pointCloud[i].z,m_pointCloud[i].y);
		//glVertex3f(m_pointCloud[i].x,m_pointCloud[i].z,0-m_pointCloud[i].y);
		glVertex3f(m_pointCloud[i].x,m_pointCloud[i].y,m_pointCloud[i].z);
	}
}

const core::aabbox3d<f32>& CLasVisualization::getBoundingBox() const
{
	return m_box;
}

irr::u32 CLasVisualization::getMaterialCount() const
{
	return 1;
}

video::SMaterial& CLasVisualization::getMaterial( u32 i )
{
	return m_material;
}

void CLasVisualization::setPointSize( f32 size )
{
	m_material.Thickness = size;
}

irr::f32 CLasVisualization::getPointSize() const
{
	return m_material.Thickness;
}

irr::u32 CLasVisualization::getPointCount() const
{
	return m_pointCloud.count();
}

void CLasVisualization::setSimple( u32 simple )
{
	if (simple > 0 && simple < 15)
	{
		m_simple = simple;
	}
}

irr::u32 CLasVisualization::getSimple() const
{
	if (SceneManager->IsAnimating())
	{
		return m_simple;
	}
	else
	{
		return 1;
	}
}

	}
}