#include "hnProjectSetting.h"

namespace hn
{
	hnProjectSetting::hnProjectSetting(void)
	{
		// 相关参数初始化
		m_str_project_name = "";

	}


	hnProjectSetting::~hnProjectSetting(void)
	{
	}

	void hnProjectSetting::setProjectName( const char* str_project_name )
	{
		m_str_project_name = str_project_name;
	}

	void hnProjectSetting::setPcdExportType( ENUM_COMBINE_EXPORT_PCD_TYPE export_type )
	{
		m_combine_export_file_info.export_pcd_type = export_type;
	}

	hn::ENUM_COMBINE_EXPORT_PCD_TYPE hnProjectSetting::getPcdExportType()
	{
		return m_combine_export_file_info.export_pcd_type;
	}

	void hnProjectSetting::setUseCtrlRadiusExportInfo( COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT& ctrl_radius_info )
	{
		m_combine_use_ctrl_pts_info = ctrl_radius_info;
	}

	COMBINE_USE_CTRL_PTS_EXPORT_RADIUS_STRUCT& hnProjectSetting::getUseCtrlRadiusExportInfo()
	{
		return m_combine_use_ctrl_pts_info;
	}

	void hnProjectSetting::setUseTimeRangeExportInfo( COMBINE_USE_TIME_EXPORT_SET_STRUCT& use_time_range_info )
	{
		m_combine_use_time_range_export_info = use_time_range_info;
	}

	COMBINE_USE_TIME_EXPORT_SET_STRUCT& hnProjectSetting::getUseTimeRangeExportInfo()
	{
		return m_combine_use_time_range_export_info;
	}

	void hnProjectSetting::setSphereProjectInfo( COMBINE_COORD_PROJECT_SET_STRUCT& sphere_project_info )
	{
		m_combine_sphere_project_info = sphere_project_info;
	}

	COMBINE_COORD_PROJECT_SET_STRUCT& hnProjectSetting::getSphereProjectInfo()
	{
		return m_combine_sphere_project_info;
	}

	void hnProjectSetting::setCtrlPath( const char* str_ctrl_path )
	{
		m_str_ctrl_path = str_ctrl_path;
	}

	void hnProjectSetting::setPosPath( const char* str_pos_path )
	{
		m_str_pos_path = str_pos_path;
	}

	std::string hnProjectSetting::getCtrlPath()
	{
		return m_str_ctrl_path;
	}

	std::string hnProjectSetting::getPosPath()
	{
		return m_str_pos_path;
	}

	std::string hnProjectSetting::getProjectName()
	{
		return m_str_project_name;
	}

	void hnProjectSetting::setProjectDir( const char* str_project_dir )
	{
		m_str_project_dir = str_project_dir;
	}

	std::string hnProjectSetting::getProjectDir()
	{
		return m_str_project_dir;
	}

	void hnProjectSetting::setCoordConvertInfo( COMBINE_COORD_CONVERT_SET_STRUCT& coord_convert_info )
	{
		m_combine_coord_convert_info = coord_convert_info;
	}

	COMBINE_COORD_CONVERT_SET_STRUCT& hnProjectSetting::getCoordConvertInfo()
	{
		return m_combine_coord_convert_info;
	}

	void hnProjectSetting::setExportFileInfo( COMBINE_EXPORT_FILES_STRUCT& export_file_info )
	{
		m_combine_export_file_info = export_file_info;
	}

	COMBINE_EXPORT_FILES_STRUCT& hnProjectSetting::getExportFilesInfo()
	{
		return m_combine_export_file_info;
	}

	void hnProjectSetting::setTimeDelayInfo( COMBINE_TIME_DELAY_SET_STRUCT& time_delay_info )
	{
		m_combine_time_delay_info = time_delay_info;
	}

	COMBINE_TIME_DELAY_SET_STRUCT& hnProjectSetting::getTimeDelayInfo()
	{
		return m_combine_time_delay_info;
	}

	COMBINE_DIST_FILTER_SET_STRUCT& hnProjectSetting::getFilterInfo()
	{
		return m_combine_dist_filter_info;
	}

	COMBINE_ROAD_3D_SET_STRUCT& hnProjectSetting::getRoad3dSetInfo()
	{
		return m_combine_road_3d_info;
	}

}

