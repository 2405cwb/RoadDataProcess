#include "StdAfx.h"
#include "hdFilterSelection.h"

using namespace irr::core;

namespace hd
{
	namespace ptcloud
	{		
		hdFilterSelection::hdFilterSelection(E_Select_Mode select_type, dimension2du view_size, f32* view_prj)
			:m_select_mode(select_type), m_view_size(view_size)
		{
			if (view_prj)
			{
				memcpy(m_view_prj, view_prj, 16*sizeof(f32));
			}			
			m_select_envi = E_SELECT_SCENE;    // 场景选择
		}

		hdFilterSelection::hdFilterSelection(E_Select_Mode select_type)
		{
			m_select_envi = E_SELECT_GRAYIMAGE;    // 灰度图选择
			m_select_mode = select_type;    
		}

		hdFilterSelection::~hdFilterSelection()
		{
		}

		//! 转换点
		void hdFilterSelection::viewPrjTrans(f32* pos)
		{
			/*
			0  1  2  3
			4  5  6  7
			8  9  10 11
			12 13 14 15
			*/

			f32 mat[4] = {pos[0], pos[1], pos[2], pos[3]};

			pos[0] = m_view_prj[0]*mat[0] + m_view_prj[4]*mat[1] + m_view_prj[8]*mat[2]  + m_view_prj[12]*mat[3];
			pos[1] = m_view_prj[1]*mat[0] + m_view_prj[5]*mat[1] + m_view_prj[9]*mat[2]  + m_view_prj[13]*mat[3];
			pos[2] = m_view_prj[2]*mat[0] + m_view_prj[6]*mat[1] + m_view_prj[10]*mat[2] + m_view_prj[14]*mat[3];
			pos[3] = m_view_prj[3]*mat[0] + m_view_prj[7]*mat[1] + m_view_prj[11]*mat[2] + m_view_prj[15]*mat[3];
		}

		//! 显示转屏幕坐标
		vector2di hdFilterSelection::getScreenCoordinatesFrom3DPosition( const vector3df& pos )
		{
			dimension2du dim(m_view_size);
			dim.Width /= 2;
			dim.Height /=2;

			f32 transformedPos[4] = { pos.X, pos.Y, pos.Z, 1.0f };
			viewPrjTrans(transformedPos);

			if (transformedPos[3] < 0)
				return vector2di(-10000,-10000);

			const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :	reciprocal(transformedPos[3]);

			return vector2di(round32(dim.Width * transformedPos[0] * zDiv) + dim.Width,
				dim.Height - round32(dim.Height * (transformedPos[1] * zDiv)));
		}

		//! 显示转屏幕坐标
		vector2df hdFilterSelection::getScreenCoordinatesFrom3DPositionf( const vector3df& pos )
		{
			dimension2du dim(m_view_size);
			dim.Width /= 2;
			dim.Height /=2;

			f32 transformedPos[4] = { pos.X, pos.Y, pos.Z, 1.0f };
			viewPrjTrans(transformedPos);

			if (transformedPos[3] < 0)
				return vector2df(-10000.0f,-10000.0f);

			const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :	reciprocal(transformedPos[3]);

			return vector2df((dim.Width * transformedPos[0] * zDiv) + dim.Width,
				dim.Height - (dim.Height * (transformedPos[1] * zDiv)));
		}
	}
}