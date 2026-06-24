#ifndef __HD_FILTER_SELECTION_H_INCLUDED__
#define __HD_FILTER_SELECTION_H_INCLUDED__
#include "hdFilter.h"
#include "../hd3DEngine/include/dimension2d.h"
#include "../hd3DEngine/include/vector2d.h"
#include "../hd3DEngine/include/vector3d.h"

namespace hd
{
	namespace ptcloud
	{
		//! 选择类型
		enum E_Select_Mode
		{
			E_SELECT_MODE_NEW,           // 新建选择
			E_SELECT_MODE_ADD,           // 增加选择
			E_SELECT_MODE_MINUS          // 减少选择
		};

		//! 选择环境
		enum E_Select_Environment
		{
			E_SELECT_GRAYIMAGE,          // 灰度图选择
			E_SELECT_SCENE               // 场景选择
		};
		
		class HDPOINTCLOUD_API hdFilterSelection : public hdFilter
		{
		public:
			// 构造函数，构造场景选择器
			hdFilterSelection(E_Select_Mode select_type, irr::core::dimension2du view_size, f32* view_prj);

			// 构造函数，构造灰度图选择器
			hdFilterSelection(E_Select_Mode select_type);
			
			virtual ~hdFilterSelection();

			//! 获取过滤器类型
			virtual E_Filter_Type getFilterType(){return E_FILTER_TYPE_UNKNOWN;}

			//! 获取选择环境
			virtual E_Select_Environment getSelectEnvironment(){return m_select_envi;}

			//! 获取选择模式
			E_Select_Mode getSelectMode(){return m_select_mode;}

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(hdVector<PointXYZIPRGBA>* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans) = 0;

			//! 过滤处理，更新选中状态--处理所有过滤器
			virtual void doFilter(CHdParcelBase* pt_array,
				CBursaWolfModel* render_trans, CBursaWolfModel* pcd_trans) = 0;

			//! 判断一个包围盒是否与选择区域相交，相交返回 true
			virtual bool isIntersects(CHdBox3df& box) = 0;

		protected:
			//! 选择环境
			E_Select_Environment m_select_envi;
			//! 选择模式
			E_Select_Mode m_select_mode;
			//! 视口大小
			irr::core::dimension2du m_view_size;
			//! 视口转换参数
			f32 m_view_prj[16];

		protected:
			//! 转换点
			void viewPrjTrans(f32* pos);
			//! 显示转屏幕坐标
			irr::core::vector2di getScreenCoordinatesFrom3DPosition(const irr::core::vector3df& pos);
			//! 显示转屏幕坐标
			irr::core::vector2df getScreenCoordinatesFrom3DPositionf(const irr::core::vector3df& pos);
		};
	}
}

#endif
