/*! LasMeshSceneNode.h
********************************************************************************
<PRE>
模块名       : hdVisualization
文件名       : LasVisualization.h
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
</PRE>
*******************************************************************************/

#pragma once

#include <irrlicht.h>
#include "driverChoice.h"
#include "lasdefinitions.hpp"
#include "lasreader.hpp"
#include "..\hdCommon\point_cloud.h"
#include "..\hdCommon\color_ramp.h"
#include "..\hdCommon\DistanceImg.h"

using namespace irr;
using namespace hd;

namespace hd
{
	namespace hls
	{

		//void (*loadCallback)(float persent,const char* msg);
/*
点云中的点分类
*/
typedef enum PointClass {
	CLASS_ALL = 0,
	CLASS_FIRST,
	CLASS_LAST,
	CLASS_GROUND,
	CLASS_OBJECT,
	CLASS_BUILDING,
	CLASS_VEGETATION,
	CLASS_MASS_POINTS,
	CLASS_WATER,
	CLASS_UNCLASSIFIED,
	CLASS_OVERLAP
} PointClass;

typedef enum RenderStyle {
	RENDER_BY_RGB = 0,				//根据点云文件中的RGB显示
	RENDER_BY_INTENSITY,			//根据反射强度渲染
	RENDER_BY_HEIGHT,				//根据Z值渲染
	RENDER_BY_DISTANCE,				//根据距离渲染
	RENDER_BY_CLASS,				//按分类渲染
} RenderStyle;

class IRRLICHT_API CLasVisualization
	:public scene::ISceneNode
{
private:
	//! 点云数据
	PointCloud	m_pointCloud;
	//! 点云中的靶球
	std::vector<PointXYZI> m_smrPts;
	//! 点数
	//long long			    m_pointCount;
	//! 点云数组,长度是:pointcount * 3
	//f32*					m_pPointCloud;
	//! 点云属性,长度是:pointcount
	//u8*						m_pPointProp;
	//! 点云渲染颜,,长度是:pointcount * 4
	//u8*						m_pColorBuf;
	//! 点云反射强度,长度是pointcount
	//u16*					m_pItensify;
	//! 材质,每个SceneNode必须包含此对象
	video::SMaterial	m_material;
	//! 点云块外包范围
	core::aabbox3d<f32> m_box;

	//! 点云显示分类
	PointClass m_renderCls;
	//! 点云符号化显示方式
	RenderStyle m_renderStyle;

	//! 抽样比例
	u32			m_simple;

	//! 抽样随机数
	u32			m_seed;

	//! 最大反射值的倒数,用乘法计算灰度灰度颜色,加速计算
	float       m_maxIntensity_rcp;

	//! 渲染颜色条
	ColorRamp   m_colorRamp1;
	ColorRamp   m_colorRamp2;
	ColorRamp   m_colorRamp3;
	//! 渲染透明度
	float		m_transparence;
	//! 深度图运算
	CDistanceImg m_distImg;
private:

	//! 渲染指定点
	void renderPoint(u32 i,float boundingBoxHeight);
/************************************************************************/
/*                   ISceneNode接口实现                                 */
/************************************************************************/
public:
	virtual void OnRegisterSceneNode();

	virtual void render();

	virtual const core::aabbox3d<f32>& getBoundingBox() const;

	virtual u32 getMaterialCount() const;

	virtual video::SMaterial& getMaterial(u32 i);


/************************************************************************/
/*                   CLasVisualization接口实现                          */
/************************************************************************/
public:
	//CLasVisualization();
	CLasVisualization(scene::ISceneNode* parent,scene::ISceneManager* mgr,s32 id);
	~CLasVisualization(void);
	//! 将las点云文件一次性加载到内存
	BOOL loadLasFile(const char* lasFile,void (*loadCallback)(float,const char*));
	//! 将hls点云文件一次性加载到内存
	BOOL loadHlsFile(const char* hlsFile,void (*loadCallback)(float,const char*));
	//! 设置点云文件显示类型
	void setRenderClass(const PointClass& renderCls);
	//! 获取点云文件显示类型
	PointClass getRenderClass() const;
	//! 设置点云渲染方式
	void setRenderStyle(const RenderStyle& renderStyle);
	//! 获取点云渲染方式
	RenderStyle getRenderStyle() const;
	//! 设置点显示大小
	void setPointSize(f32 size);
	//! 获取点显示大小
	f32 getPointSize() const;
	//! 获取点个数
	u32 getPointCount() const;
	//! 设置抽样比例
	void setSimple(u32 simple);
	//! 设置抽样比例
	u32 getSimple() const;
};

	}
}