#ifndef _HN_ROADTYPE_DEF_
#define _HN_ROADTYPE_DEF_
namespace hnCommon
{
	// 路面类型
	enum ROAD_SURFACE_TYPE
	{
        ROAD_LQ_SURFACE = 0,  //沥青路面
		ROAD_SN_SURFACE,      // 水泥路面
		ROAD_SS_SURFACE       // 砂石路面
	};
	//道路打标标志  
    // 标注信息--如路面材质：沥青、材质、砂石；    路面单元：单元  ；   路面等级:一级公路，二级公路   路面标准:等级公路2018  低等级农村路  ;路面情况
	enum ROAD_MARK_TYPE
	{
		ROAD_SURFACE = 0,
		ROAD_UNIT,
		ROAD_GRAD,
		ROAD_STANDARD,
		ROAD_SOMETHING,
		None
	};

	// 作业模式
	enum ROAD_WORK_TYPE
	{
		ROAD_WORK_LARGE_RECT = 0,	// 人工模式作业
		ROAD_WORK_SMALL_RECT,		// 自动化模式作业
		DESIGN						// 设计模式
	};
	 
	static const char* workTypeToQString(ROAD_WORK_TYPE workType)
	{
		switch (workType)
		{
		case hnCommon::ROAD_WORK_LARGE_RECT:
			return "人工模式";
			break;
		case hnCommon::ROAD_WORK_SMALL_RECT:
			return "自动化模式";
			break;
		case hnCommon::DESIGN:
			return "设计模式";
			break;
		default:
			break;
		}
		return "";
	}
	static ROAD_WORK_TYPE qstringToWorkType(const char * standard)
	{
		if (strcmp(standard,"人工模式")==0)
		{
			return ROAD_WORK_TYPE::ROAD_WORK_LARGE_RECT;
		}
		else if (strcmp(standard,"自动化模式")==0)
		{
			return  ROAD_WORK_TYPE::ROAD_WORK_SMALL_RECT;
		}
		else
		{
			return  ROAD_WORK_TYPE:: DESIGN;

		}
	}

	// 病害等级
	enum DISEASE_LEVEL
	{
		NO_LEVEL = 0,   // 无等级
		L_LEVEL,        // 低等级
		M_LEVEL,        // 中等级
		H_LEVEL         // 高等级
	};

	// 工程类型
	enum PROJECT_TYPE
	{
		PROJECT_2D_TYPE = 0,  // 二维工程
		PROJECT_XD_3D_TYPE,   // 相对三维工程
		PROJECT_JD_3D_TYPE,   // 绝对三维工程    作为单独三维工程的标记
		PROJECT_23D_TYPE      // 二三维一体化
	};

	// 视图类型
	enum VIEW_TYPE
	{
        // 路面破损相机视图
		VIEW_2D_CAMERA_TYPE = 0,

		// 点云影像视图
		VIEW_3D_CAMERA_TYPE,

		// 景观视图
		VIEW_STREET_CAMERA_TYPE,

		// 相对点云视图
		VIEW_XD_PTCLOUD_TYPE,

		// 绝对点云视图
		VIEW_JD_PTCLOUD_TYPE
	};

	enum STREET_VIEW_TYPE
	{
		// 左侧景观
		STREET_LEFT_VIEW = 0,

		// 右侧景观
		STREET_RIGHT_VIEW
	};
	
}

#endif // _HN_ROADTYPE_DEF_
