#ifndef __I_HN_PROJECT_SETTING_H_INCLUDED__
#define __I_HN_PROJECT_SETTING_H_INCLUDED__
#include <string>
#include <vector>
#include "..\hnConvert\hnDataCombineStructInfo.h"
#include "hnpavementcreate3d_global.h"

namespace hn
{
	//! 定义导出格式选择，选择格式目前支持HLS、HLZ
	enum ENUM_COMBINE_EXPORT_PCD_TYPE
	{
		E_COMBINE_EXPORT_PCD_HLS = 0,
		E_COMBINE_EXPORT_PCD_HLZ = 1,
		E_COMBINE_EXPORT_PCD_LAS = 2,
		E_COMBINE_EXPORT_PCD_IPRC = 3
	};

	enum ENUM_PROJECT_SPHERE_TYPE
	{
		E_COMBINE_PROJECT_SPHERE_BEIJING54 = 0,
		E_COMBINE_PROJECT_SPHERE_XIAN80 = 1,
		E_COMBINE_PROJECT_SPHERE_WGS84 = 2,
		E_COMBINE_PROJECT_SPHERE_China2000 = 3
	};

	enum ENUM_PROJECT_ZONE_TYPE
	{
		E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_3 = 0,
		E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_6 = 1,
		E_COMBINE_PROJECT_TYPE_MERCATOR = 2,
		E_COMBINE_PROJECT_TYPE_UTM = 3,
		E_COMBINE_PROJECT_TYPE_Web_Mercator = 4
	};

	enum ENUM_HEIGHT_FIT_MODEL_TYPE
	{
		//! 固定差改正;
		E_COMBINE_HEIGHT_FIT_MODEL_CONSTANT = 0,

		//! 平面拟合；
		E_COMBINE_HEIGHT_FIT_MODEL_PLANE = 1,

		//! 二维曲面拟合;
		E_COMBINE_HEIGHT_FIT_MODEL_CURVE = 2
	};

	//! 输出文件设置结构体
	struct COMBINE_EXPORT_FILES_STRUCT
	{
		COMBINE_EXPORT_FILES_STRUCT()
		{
			export_pcd_type = E_COMBINE_EXPORT_PCD_HLZ;
			need_export_lin_file = 1;
			need_export_time_file = 0;
			need_split_export_file = 0;
			need_export_single_line = 0;
			export_single_line_index = 16;
			need_export_pavement_image = 0;
			need_use_gpu_acc = 0;
			hlz_grid_size_index = 2;
		}

		ENUM_COMBINE_EXPORT_PCD_TYPE export_pcd_type;
		int need_export_lin_file;
		int need_export_time_file;
		int need_split_export_file;


		// 新增，设置多线激光输出第N线数据;
		int need_export_single_line; // 是否需要输出单线数据;
		int export_single_line_index;

		// 新增，输出，暂无用;
		int need_export_pavement_image;

		// 新增，输出采用GPU加速;
		int need_use_gpu_acc;

		// 新增，输出HLZ时选择设置0层格网等级;
		int hlz_grid_size_index;
	};

	//! 控制点范围输出设置结构体
	struct COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT
	{
		COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT()
		{
			is_use_ctrl_pts_radius_export = 0;
			use_ctrl_pts_radius = 0.5;
		}

		//! 标识是否使用控制点范围输出，0标识不采用，1标识采用
		int is_use_ctrl_pts_radius_export;

		//! 控制点范围输出半径;
		double use_ctrl_pts_radius;
	};

	//! 时间范围输出结构体
	struct COMBINE_USE_TIME_EXPORT_SET_STRUCT
	{
		COMBINE_USE_TIME_EXPORT_SET_STRUCT()
		{
			use_resimple_step = 0;
			is_use_time_range_export = 0;
			vec_combine_time_range.clear();
		}

		//! 标识使用时间范围输出;
		int is_use_time_range_export;

		//! 时间段范围记录;
		std::vector<COMBINE_TIME_RANGE> vec_combine_time_range;

		//! 标记抽稀输出 5 * use_resimple_step;
		int use_resimple_step;
	};

	struct COMBINE_COORD_PROJECT_SET_STRUCT
	{
		COMBINE_COORD_PROJECT_SET_STRUCT()
		{
			sphere_target_type = E_COMBINE_PROJECT_SPHERE_WGS84;
			sphere_zone_type = E_COMBINE_PROJECT_TYPE_GAUSS_ZONE_3;
			centre_longtitude = 114.0;
			east_offset = 500000.0;
			project_height = 0.0;
			average_latitude = 0.0;
			heading_adj = 0.0;
			undo_proj = 0;
			proj_scale = 1.0;
		}

		//! 目标椭球参数;
		ENUM_PROJECT_SPHERE_TYPE sphere_target_type;
		
		//! 投影方法
		ENUM_PROJECT_ZONE_TYPE sphere_zone_type;

		//! 中央经线
		double centre_longtitude;

		//! 东向加常数
		double east_offset;

		//! 投影面高程
		double project_height;

		//! 平均纬度
		double average_latitude;

		//! 七参数转换时非标准带航向补偿;
		double heading_adj;

		//! 标记是否使用投影转换，若为1表示不使用，即直接运用POS中的投影坐标;
		int undo_proj;

		//! 投影比例因子;
		double proj_scale;
	};

	struct COMBINE_COORD_CONVERT_SET_STRUCT
	{
		COMBINE_COORD_CONVERT_SET_STRUCT()
		{
			use_coord_convert = 0;
			use_coord_convert_model = 0;
			seven_param_dx = seven_param_dy = seven_param_dz = seven_param_rx = seven_param_ry = seven_param_rz = seven_param_k = 0.0;
			four_param_dx = four_param_dy = four_param_dr = 0.0;
			four_param_k = 1.0;
			height_fit_model = E_COMBINE_HEIGHT_FIT_MODEL_CONSTANT;
			height_fit_n0 = height_fit_e0 = 0.0;
			height_fit_a = height_fit_b = height_fit_c = 0.0;
			height_fit_d = height_fit_e = height_fit_f = 0.0;
		}

		//！ 标记使用坐标转换;
		int use_coord_convert;

		//! 转换模式，0表示采用四参数加高程拟合转换，1表示采用七参数布尔莎模型;
		int use_coord_convert_model;

		//! 七参数模型信息记录;
		double seven_param_dx;
		double seven_param_dy;
		double seven_param_dz;
		double seven_param_rx;
		double seven_param_ry;
		double seven_param_rz;
		double seven_param_k;

		//! 四参数模型信息记录;
		double four_param_dx;
		double four_param_dy;
		double four_param_dr;
		double four_param_k;

		//! 高程拟合模型类型记录,可考虑参考论文在各拟合模型中添加最小二乘配置，另外线性拟合可能更适用;
		ENUM_HEIGHT_FIT_MODEL_TYPE height_fit_model;

		//! 高程拟合参数设置;
		double height_fit_n0;
		double height_fit_e0;
		double height_fit_a;
		double height_fit_b;
		double height_fit_c;
		double height_fit_d;
		double height_fit_e;
		double height_fit_f;
	};

	struct COMBINE_TIME_DELAY_SET_STRUCT
	{
		COMBINE_TIME_DELAY_SET_STRUCT()
		{
			pcd_time_delay = 0;
			pano_time_delay = 0;
			is_use_angle_filter = 0;
			filter_start_angle = 0.0;
			filter_end_angle = 360.0;
		}

		// 时间延时，毫秒值记录整型;
		int pcd_time_delay;

		// 全景时间延时记录，毫秒值记录;
		int pano_time_delay;

		// 是否使用角度过滤;
		int is_use_angle_filter;

		// 过滤起始角度,终止角度;
		double filter_start_angle;
		double filter_end_angle;
	};

	struct COMBINE_2D_POINT_LINE_STRUCT
	{
		COMBINE_2D_POINT_LINE_STRUCT()
		{
			startCoordX = 0.0;
			startCoordY = 0.0;
			endCoordX = 0.0;
			endCoordY = 0.0;
			filterWidth = 0.0;
		}

		void initial()
		{
			startCoordX = 0.0;
			startCoordY = 0.0;
			endCoordX = 0.0;
			endCoordY = 0.0;
		}

		double startCoordX;
		double startCoordY;
		double endCoordX;
		double endCoordY;
		double filterWidth;
	};

	struct COMBINE_DIST_FILTER_SET_STRUCT
	{
		COMBINE_DIST_FILTER_SET_STRUCT()
		{
			nleft_to_centre = 1;
			use_npfilter = 0;
			use_height_filter = 0;
			use_2dline_filter = 0;
			filter_height = 0.0;
			filter_2d_dist = 0.0;
			vecLinePoints.clear();
			vec_npfilter_time_range.clear();

			use_move_range_filter = 0;
			move_left_coord = 2.7;
			move_right_coord = 2.7;
			move_up_coord = 4.8;
			move_down_coord = 1.2;
		}

		void addLinePoint(char* strLine)
		{
			double tmpDist = 0.0;
			COMBINE_2D_POINT_LINE_STRUCT pointLine;
			int nSize = sscanf_s(strLine,"%lf,%lf,%lf,%lf;%lf;\n",
				&pointLine.startCoordX,&pointLine.startCoordY,&pointLine.endCoordX,&pointLine.endCoordY,&tmpDist);
			if (nSize >= 5)
			{
				pointLine.filterWidth = tmpDist;
				vecLinePoints.push_back(pointLine);
			}
			else
			{
				nSize = sscanf_s(strLine,"%lf,%lf,%lf,%lf;\n",
					&pointLine.startCoordX,&pointLine.startCoordY,&pointLine.endCoordX,&pointLine.endCoordY);
				if (nSize >= 4)
				{
					pointLine.filterWidth = 0.0;
					vecLinePoints.push_back(pointLine);
				}
			}
			
		}

		// 是否高程过滤;
		int use_height_filter;
		double filter_height;

		// 是否水平过滤;
		int use_2dline_filter;
		double filter_2d_dist;
		std::vector<COMBINE_2D_POINT_LINE_STRUCT> vecLinePoints;

		//! 时间段范围记录;
		int use_npfilter;
		int nleft_to_centre; // 为双轨道，需确定哪一项居中，为1表示相对于行进方向，左向指向隧道中间;
		std::vector<COMBINE_TIME_RANGE> vec_npfilter_time_range;

		// 使用范围距离(载体坐标系)进行过滤;
		int use_move_range_filter;
		double move_left_coord;
		double move_right_coord;
		double move_up_coord;
		double move_down_coord;
	};

	struct COMBINE_ROAD_3D_SET_STRUCT 
	{
		COMBINE_ROAD_3D_SET_STRUCT()
		{
			use_split_mode = 0;
			road_3d_split_param = 0;
			road_3d_proj_miles = 0.0;
			max_thread_count_index = 3;
			type_disease = 3;
			is_customize = false;
			use_exportGrey = 1;
			use_exportDepth = 1;
			use_exportRoad = 0;
			use_addRgbToHlz = 0;
			use_calcRgbByDepth = 1;
			use_calcRgbByRoad = 0;
		}

		//! 使用分段处理;
		int use_split_mode;

		//! 标识三维路面点云数据融合分段阈值，1KM起步;
		int road_3d_split_param;

		//! 记录该工程三维路面总里程信息，仅用于显示;
		double road_3d_proj_miles;

		//! 分段计算时运行同时运行的最大线程数。不能超过系统允许的总线程数;
		int max_thread_count_index;

		//！ 路面深度图渲染方案，根据病害严重程度选择,共8种
		//！ 0：-5-5 -10—10  -15—15  1：-20—20  -25-25  7：-40—40
		int type_disease;

		//是否使用自定义
		bool is_customize;

		// 输出灰度图;
		int use_exportGrey;

		// 是否深度图;
		int use_exportDepth;

		// 是否输出道路线形图;
		int use_exportRoad;

		// 是否添加RGB信息到HLZ，附加深度属性;
		int use_addRgbToHlz;

		// 根据深度信息赋值RGB;
		int use_calcRgbByDepth;

		// 根据道路线性信息赋值RGB;
		int use_calcRgbByRoad;
	};


	////! iscan工程数据管理
	//struct COMBINE_ISCAN_MENU

	class HNPAVEMENTCREATE3D_EXPORT hnProjectSetting
	{
	public:
		hnProjectSetting(void);
		~hnProjectSetting(void);

		//! 设置工程路径，内存记录;
		void setProjectDir(const char* str_project_dir);
		std::string getProjectDir();

		//! 设置工程名作为唯一性标识关联管理;
		void setProjectName(const char* str_project_name);
		std::string getProjectName();

		//! 设置控制点文件路径;
		void setCtrlPath(const char* str_ctrl_path);
		std::string getCtrlPath();

		//! 设置POS文件路径;
		void setPosPath(const char* str_pos_path);
		std::string getPosPath();

		//! 设置定义格式导出类型
		void setPcdExportType(ENUM_COMBINE_EXPORT_PCD_TYPE export_type);

		//! 获取内存记录的点云格式导出类型
		ENUM_COMBINE_EXPORT_PCD_TYPE getPcdExportType();

		//! 设置文件导出信息;
		void setExportFileInfo(COMBINE_EXPORT_FILES_STRUCT& export_file_info);

		//! 获取导出文件信息;
		COMBINE_EXPORT_FILES_STRUCT& getExportFilesInfo();

		//! 设定是否按控制点范围输出，1表示采用范围输出，0表示不采用;
		void setUseCtrlRadiusExportInfo(COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT& ctrl_radius_info);

		//! 获取是否按控制点范围输出信息
		COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT& getUseCtrlRadiusExportInfo();

		//! 设置按时间范围设置设置输出，标志为1标识使用，为0表示不使用;
		void setUseTimeRangeExportInfo(COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_range_info);

		//! 获取设置时间范围信息设置输出范围
		COMBINE_USE_TIME_EXPORT_SET_STRUCT& getUseTimeRangeExportInfo();

		//! 设置目标椭球坐标投影相关信息;
		void setSphereProjectInfo(COMBINE_COORD_PROJECT_SET_STRUCT& sphere_project_info);

		//! 获取目标椭球坐标投影相关信息;
		COMBINE_COORD_PROJECT_SET_STRUCT& getSphereProjectInfo();

		//! 设置坐标转换相关参数信息;
		void setCoordConvertInfo(COMBINE_COORD_CONVERT_SET_STRUCT& coord_convert_info);

		//! 获取坐标转换相关参数信息;
		COMBINE_COORD_CONVERT_SET_STRUCT& getCoordConvertInfo();

		//! 设置时间延时信息;
		void setTimeDelayInfo(COMBINE_TIME_DELAY_SET_STRUCT& time_delay_info);

		//！获取时间延时信息;
		COMBINE_TIME_DELAY_SET_STRUCT& getTimeDelayInfo();

		//! 获取过滤设置相关信息;
		COMBINE_DIST_FILTER_SET_STRUCT& getFilterInfo();

		//! 获取三维路面设置相关信息;
		COMBINE_ROAD_3D_SET_STRUCT& getRoad3dSetInfo();

	private:
		//! 设置工程名作为唯一性标识关联管理;
		std::string m_str_project_name;

		//! 设置工程文件夹目录绝对路径;
		std::string m_str_project_dir;

		//! 记录POS文件绝对路径；
		std::string m_str_pos_path;

		//! 记录控制带你文件绝对路径;
		std::string m_str_ctrl_path;

		////! 定义记录点云导出格式信息记录;
		//ENUM_COMBINE_EXPORT_PCD_TYPE m_combine_export_type;

		COMBINE_EXPORT_FILES_STRUCT m_combine_export_file_info;

		//! 定义是否按控制点范围输出，设置输出半径；
		COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT m_combine_use_ctrl_pts_info;

		//! 定义是否按时间范围设置输出，设置时间范围段;
		COMBINE_USE_TIME_EXPORT_SET_STRUCT m_combine_use_time_range_export_info;

		//! 定义目标投影相关信息;
		COMBINE_COORD_PROJECT_SET_STRUCT m_combine_sphere_project_info;

		//! 坐标转换相关参数信息设置;
		COMBINE_COORD_CONVERT_SET_STRUCT m_combine_coord_convert_info;

		//! 时间延时信息设置;
		COMBINE_TIME_DELAY_SET_STRUCT m_combine_time_delay_info;

		//! 距离过滤信息设置;
		COMBINE_DIST_FILTER_SET_STRUCT m_combine_dist_filter_info;

		//! 三维路面设置相关信息;
		COMBINE_ROAD_3D_SET_STRUCT m_combine_road_3d_info;
	};
}

#endif
