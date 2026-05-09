#include "stdafx.h"
#include "CScanSceneNode.h"
#include "COpenGLExtensionHandler.h"
#include "..\hdPointCloud\hdSysSetting.h"
#include "..\hdCore\Hd3dBoxEx.h"
#include "hd3DView.h"
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <ppl.h>
//#include <psapi.h>
//#include <windows.h>

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

//#define SAMPLE_COUNT (500000.0)
using namespace hd::fm;

// 定义全局变量，标记设置正在设置点云，使用加锁会造成进度条对话框无法显示
bool g_bSettingPcd = false;

namespace hd
{
	namespace scene
	{
		static float classification_colors[16][4] = 
		{{0.0f,0.3f,0.3f,0.3f},// created (black)
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
#pragma region 256彩色值映射表
		static COLORREF classification[] = 
		{ 
			RGB( 0xFF, 0xFF, 0xFF ),  //White  未分类
			RGB( 0xD2, 0x69, 0x1E ),  //Chocolate 巧克力色 地面
			RGB( 0x7C, 0xFC, 0x00 ),  //LawnGreen 草地绿  较低的植被
			RGB( 0x00, 0xFF, 0x00 ),  // Lime green绿色 一般高的植被
			RGB( 0x22, 0x8B, 0x22 ),  //ForestGreen 森林绿  较高的植被
			RGB( 0xFF, 0xFF, 0x00 ),  //yellow 黄色 建筑物
			RGB( 0x00, 0x00, 0x00 ),  //Black 黑色  低洼点
			RGB( 0xFF, 0x00, 0xFF ),  //Fuchsia 深红色 噪声
			RGB( 0x00, 0x00, 0xFF ),  //Blue 水体
			RGB( 0xFF, 0x61, 0x00 ),  //orange 橙色	
			RGB( 0x87, 0x26, 0x57 ),  // 草莓色
			RGB( 0x00, 0xC7, 0x8C ),  // 孔雀蓝
			RGB( 0xB0, 0xE0, 0xE6 ),  //PowderBlue 浅灰蓝色
			RGB( 0xCD, 0x95, 0x3F ), 
			RGB( 0xFF, 0x90, 0xCB ), 
			RGB( 0x80, 0x00, 0x80 ),  //Purple
			RGB( 0xB0, 0x90, 0xE6 ), 
			RGB( 0x80, 0x90, 0x80 ), 
			RGB( 0xFF, 0x90, 0x00 ), 
			RGB( 0xBC, 0xCF, 0x8F ), 
			RGB( 0xF0, 0xF8, 0xFF ),  //AliceBlue
			RGB( 0xFA, 0xEB, 0xD7 ),  //AntiqueWhite
			RGB( 0x00, 0xFF, 0xFF ),  //Aqua
			RGB( 0x7F, 0xFF, 0xD4 ),  //Aquamarine
			RGB( 0xF0, 0x6F, 0xFF ),  //Azure
			RGB( 0xF5, 0xF5, 0xDC ),  //Beige
			RGB( 0xFF, 0xE4, 0xC4 ),  //Bisque
			RGB( 0xFF, 0xEB, 0xCD ),  //BlanchedAlmond
			RGB( 0x8A, 0x2B, 0xE2 ),  //BlueViolet
			RGB( 0xA5, 0x2A, 0x2A ),  //Brown
			RGB( 0xDE, 0xB8, 0x87 ),  //BurlyWood
			RGB( 0x5F, 0x9E, 0xA0 ),  //CadetBlue
			RGB( 0x7F, 0xFF, 0x00 ),  //Chartreuse
			RGB( 0xFF, 0x7F, 0x50 ),  //Coral
			RGB( 0x64, 0x95, 0xED ),  //CornflowerBlue
			RGB( 0xFF, 0xF8, 0xDC ),  //Cornsilk
			RGB( 0xDC, 0x14, 0x3C ),  //Crimson
			RGB( 0x00, 0xFF, 0xFF ),  //Cyan
			RGB( 0x00, 0x00, 0x8B ),  //DarkBlue
			RGB( 0x00, 0x8B, 0x8B ),  //DarkCyan
			RGB( 0xB8, 0x86, 0x0B ),  //DarkGoldenrod 
			RGB( 0xA9, 0xA9, 0xA9 ),  //DarkGray
			RGB( 0x00, 0x64, 0x00 ),  //DarkGreen
			RGB( 0xBD, 0xB7, 0x6B ),  //DarkKhaki
			RGB( 0x8B, 0x00, 0x8B ),  //DarkMagenta
			RGB( 0x55, 0x6B, 0x2F ),  //DarkOliveGreen
			RGB( 0xFF, 0x8C, 0x00 ),  //DarkOrange
			RGB( 0x99, 0x32, 0xCC ),  //DarkOrchid
			RGB( 0x8B, 0x00, 0x00 ),  //DarkRed
			RGB( 0xE9, 0x96, 0x7A ),  //DarkSalmon
			RGB( 0x8F, 0xBC, 0x8F ),  //DarkSeaGreen
			RGB( 0x48, 0x3D, 0x8B ),  //DarkSlateBlue
			RGB( 0x2F, 0x4F, 0x4F ),  //DarkSlateGray
			RGB( 0x00, 0xCE, 0xD1 ),  //DarkTurquoise
			RGB( 0x94, 0x00, 0xD3 ),  //DarkViolet
			RGB( 0xFF, 0x14, 0x93 ),  //DeepPink
			RGB( 0x00, 0xBF, 0xFF ),  //DeepSkyBlue
			RGB( 0x69, 0x69, 0x69 ),  //DimGray
			RGB( 0x1E, 0x90, 0xFF ),  //DodgerBlue
			RGB( 0xB2, 0x22, 0x22 ),  //FireBrick 
			RGB( 0xFF, 0xFA, 0xF0 ),  //FloralWhite
			RGB( 0xDC, 0xDC, 0xDC ),  //Gainsboro
			RGB( 0xF8, 0xF8, 0xFF ),  //GhostWhite
			RGB( 0xFF, 0xD7, 0x00 ),  //Gold
			RGB( 0xDA, 0xA5, 0x20 ),  //Goldenrod 
			RGB( 0x80, 0x80, 0x80 ),  //Gray
			RGB( 0x00, 0x80, 0x00 ),  //Green
			RGB( 0xAD, 0xFF, 0x2F ),  //GreenYellow
			RGB( 0xF0, 0xFF, 0xF0 ),  //Honeydew
			RGB( 0xFF, 0x69, 0xB4 ),  //HotPink
			RGB( 0xCD, 0x5C, 0x5C ),  //IndianRed
			RGB( 0x4B, 0x00, 0x82 ),  //Indigo
			RGB( 0xFF, 0xFF, 0xF0 ),  //Ivory
			RGB( 0xF0, 0xE6, 0x8C ),  //Khaki
			RGB( 0xE6, 0xE6, 0xFA ),  //Lavender
			RGB( 0xFF, 0xF0, 0xF5 ),  //LavenderBlush
			RGB( 0xFF, 0xFA, 0xCD ),  //LemonChiffon
			RGB( 0xAD, 0xD8, 0xE6 ),  //LightBlue
			RGB( 0xF0, 0x80, 0x80 ),  //LightCoral
			RGB( 0xE0, 0xFF, 0xFF ),  //LightCyan
			RGB( 0xFA, 0xFA, 0xD2 ),  //LightGoldenrodYellow
			RGB( 0x90, 0xEE, 0x90 ),  //LightGreen
			RGB( 0xD3, 0xD3, 0xD3 ),  //LightGrey
			RGB( 0xFF, 0xB6, 0xC1 ),  //LightPink
			RGB( 0xFF, 0xA0, 0x7A ),  //LightSalmon
			RGB( 0x20, 0xB2, 0xAA ),  //LightSeaGreen
			RGB( 0x87, 0xCE, 0xFA ),  //LightSkyBlue
			RGB( 0x77, 0x88, 0x99 ),  //LightSlateGray
			RGB( 0xB0, 0xC4, 0xDE ),  //LightSteelBlue
			RGB( 0xFF, 0xFF, 0xE0 ),  //LightYellow
			RGB( 0x32, 0xCD, 0x32 ),  //LimeGreen
			RGB( 0xFA, 0xF0, 0xE6 ),  //Linen
			RGB( 0x80, 0x00, 0x00 ),  //Maroon
			RGB( 0x66, 0xCD, 0xAA ),  //MediumAquamarine
			RGB( 0x00, 0x00, 0xCD ),  //MediumBlue
			RGB( 0xBA, 0x55, 0xD3 ),  //MediumOrchid
			RGB( 0x93, 0x70, 0xDB ),  //MediumPurple
			RGB( 0x3C, 0xB3, 0x71 ),  //MediumSeaGreen
			RGB( 0x7B, 0x68, 0xEE ),  //MediumSlateBlue
			RGB( 0x00, 0xFA, 0x9A ),  //MediumSpringGreen
			RGB( 0x48, 0xD1, 0xCC ),  //MediumTurquoise
			RGB( 0xC7, 0x15, 0x85 ),  //MediumVioletRed
			RGB( 0x19, 0x19, 0x70 ),  //MidnightBlue
			RGB( 0xF5, 0xFF, 0xFA ),  //MintCream
			RGB( 0xFF, 0xE4, 0xE1 ),  //MistyRose
			RGB( 0xFF, 0xE4, 0xB5 ),  //Moccasin
			RGB( 0xFF, 0xDE, 0xAD ),  //NavajoWhite
			RGB( 0x00, 0x00, 0x80 ),  //Navy
			RGB( 0xFD, 0xF5, 0xE6 ),  //OldLace
			RGB( 0x80, 0x80, 0x00 ),  //Olive
			RGB( 0x6B, 0x8E, 0x23 ),  //OliveDrab
			RGB( 0xFF, 0xA5, 0x00 ),  //Orange
			RGB( 0xFF, 0x45, 0x00 ),  //OrangeRed
			RGB( 0xDA, 0x70, 0xD6 ),  //Orchid
			RGB( 0xEE, 0xE8, 0xAA ),  //PaleGoldenrod
			RGB( 0x98, 0xFB, 0x98 ),  //PaleGreen
			RGB( 0xAF, 0xEE, 0xEE ),  //PaleTurquoise
			RGB( 0xDB, 0x70, 0x93 ),  //PaleVioletRed
			RGB( 0xFF, 0xEF, 0xD5 ),  //PapayaWhip
			RGB( 0xFF, 0xDA, 0xB9 ),  //PeachPuff
			RGB( 0xCD, 0x85, 0x3F ),  //Peru
			RGB( 0xFF, 0xC0, 0xCB ),  //Pink
			RGB( 0xDD, 0xA0, 0xDD ),  //Plum
			RGB( 0x80, 0x00, 0x80 ),  //Purple
			RGB( 0xFF, 0x00, 0x00 ),  //Red
			RGB( 0xBC, 0x8F, 0x8F ),  //RosyBrown
			RGB( 0x41, 0x69, 0xE1 ),  //RoyalBlue
			RGB( 0x8B, 0x45, 0x13 ),  //SaddleBrown
			RGB( 0xFA, 0x80, 0x72 ),  //Salmon
			RGB( 0xF4, 0xA4, 0x60 ),  //SandyBrown
			RGB( 0x2E, 0x8B, 0x57 ),  //SeaGreen
			RGB( 0xFF, 0xF5, 0xEE ),  //Seashell
			RGB( 0xA0, 0x52, 0x2D ),  //Sienna
			RGB( 0xC0, 0xC0, 0xC0 ),  //Silver
			RGB( 0x87, 0xCE, 0xEB ),  //SkyBlue
			RGB( 0x6A, 0x5A, 0xCD ),  //SlateBlue
			RGB( 0x70, 0x80, 0x90 ),  //SlateGray
			RGB( 0xFF, 0xFA, 0xFA ),  //Snow
			RGB( 0x00, 0xFF, 0x7F ),  //SpringGreen
			RGB( 0x46, 0x82, 0xB4 ),  //SteelBlue
			RGB( 0xD2, 0xB4, 0x8C ),  //Tan
			RGB( 0x00, 0x80, 0x80 ),  //Teal
			RGB( 0xD8, 0xBF, 0xD8 ),  //Thistle
			RGB( 0xFF, 0x63, 0x47 ),  //Tomato
			RGB( 0x40, 0xE0, 0xD0 ),  //Turquoise
			RGB( 0xEE, 0x82, 0xEE ),  //Violet
			RGB( 0xF5, 0xDE, 0xB3 ),  //Wheat
			RGB( 0xF5, 0xF5, 0xF5 ),  //WhiteSmoke
			RGB( 0x9A, 0xCD, 0x32 ),  //YellowGreen
			RGB( 0xF1, 0xF8, 0xF1 ), 
			RGB( 0xFB, 0xEB, 0xD7 ), 
			RGB( 0xE0, 0xF6, 0xFF ), 
			RGB( 0x4F, 0xF3, 0xD4 ), 
			RGB( 0xF2, 0xFF, 0xFF ), 
			RGB( 0xF2, 0xF5, 0xDC ), 
			RGB( 0xF2, 0xE4, 0xC4 ), 
			RGB( 0xDD, 0x90, 0xDD ), 
			RGB( 0x02, 0x00, 0x00 ), 
			RGB( 0xF2, 0xEB, 0xCD ), 
			RGB( 0x02, 0x02, 0xFF ), 
			RGB( 0x82, 0x2C, 0xE2 ), 
			RGB( 0xA6, 0x3A, 0x2A ), 
			RGB( 0xDF, 0xB0, 0x87 ), 
			RGB( 0x5F, 0x9F, 0xA4 ), 
			RGB( 0x4F, 0xF1, 0x04 ), 
			RGB( 0xD3, 0x69, 0x1E ), 
			RGB( 0xFF, 0x7F, 0x53 ), 
			RGB( 0x63, 0x95, 0xED ), 
			RGB( 0xFF, 0xF8, 0xDF ), 
			RGB( 0xDD, 0x1D, 0x3C ), 
			RGB( 0x03, 0xFF, 0xF6 ), 
			RGB( 0x03, 0x00, 0x8B ), 
			RGB( 0x06, 0x8C, 0x8B ), 
			RGB( 0xB0, 0x86, 0x0B ), 
			RGB( 0xA8, 0xA7, 0xA9 ), 
			RGB( 0x00, 0x66, 0x00 ), 
			RGB( 0xBD, 0xA7, 0x6B ), 
			RGB( 0x8B, 0xA0, 0x8B ), 
			RGB( 0x55, 0xAB, 0x2F ), 
			RGB( 0xFF, 0xAC, 0x00 ), 
			RGB( 0x99, 0xA2, 0xCC ), 
			RGB( 0x8B, 0xA0, 0x00 ), 
			RGB( 0xE9, 0xA6, 0x7A ), 
			RGB( 0x8F, 0xAC, 0x8F ), 
			RGB( 0x48, 0xAD, 0x8B ), 
			RGB( 0x2F, 0xAF, 0x4F ), 
			RGB( 0x00, 0xAE, 0xD1 ), 
			RGB( 0x94, 0xA0, 0xD3 ), 
			RGB( 0xFF, 0xA4, 0x93 ), 
			RGB( 0x00, 0xAF, 0xFF ), 
			RGB( 0x69, 0xA9, 0x69 ), 
			RGB( 0x1E, 0xA0, 0xFF ), 
			RGB( 0xB2, 0xA2, 0x22 ), 
			RGB( 0xFF, 0xAA, 0xF0 ), 
			RGB( 0x22, 0xAB, 0x22 ), 
			RGB( 0xFF, 0xA0, 0xFF ), 
			RGB( 0xDC, 0xAC, 0xDC ), 
			RGB( 0xF8, 0xA8, 0xFF ), 
			RGB( 0xFF, 0xA7, 0x00 ), 
			RGB( 0xDA, 0xA5, 0x20 ), 
			RGB( 0x80, 0xC0, 0x80 ), 
			RGB( 0x00, 0xC0, 0x00 ), 
			RGB( 0xAD, 0xCF, 0x2F ), 
			RGB( 0xF0, 0xCF, 0xF0 ), 
			RGB( 0xFF, 0xC9, 0xB4 ), 
			RGB( 0xCD, 0xCC, 0x5C ), 
			RGB( 0x4B, 0xC0, 0x82 ), 
			RGB( 0xFF, 0xCF, 0xF0 ), 
			RGB( 0xF0, 0xC6, 0x8C ), 
			RGB( 0xE6, 0xC6, 0xFA ), 
			RGB( 0xFF, 0xC0, 0xF5 ), 
			RGB( 0x7C, 0xCC, 0x00 ), 
			RGB( 0xFF, 0xCA, 0xCD ), 
			RGB( 0xAD, 0xC8, 0xE6 ), 
			RGB( 0xF0, 0xC0, 0x80 ), 
			RGB( 0xE0, 0xCF, 0xFF ), 
			RGB( 0xFA, 0x9A, 0xD2 ), 
			RGB( 0x90, 0x9E, 0x90 ), 
			RGB( 0xD3, 0x93, 0xD3 ), 
			RGB( 0xFF, 0x96, 0xC1 ), 
			RGB( 0xFF, 0x90, 0x7A ), 
			RGB( 0x20, 0x92, 0xAA ), 
			RGB( 0x87, 0x9E, 0xFA ), 
			RGB( 0x77, 0x98, 0x99 ), 
			RGB( 0xB0, 0x94, 0xDE ), 
			RGB( 0xFF, 0x9F, 0xE0 ), 
			RGB( 0x00, 0x9F, 0x00 ), 
			RGB( 0x32, 0x9D, 0x32 ), 
			RGB( 0xFA, 0x90, 0xE6 ), 
			RGB( 0xFF, 0x90, 0xFF ), 
			RGB( 0x80, 0x90, 0x00 ), 
			RGB( 0x66, 0x4D, 0xAA ), 
			RGB( 0x00, 0x40, 0xCD ), 
			RGB( 0xBA, 0x45, 0xD3 ), 
			RGB( 0x93, 0x40, 0xDB ), 
			RGB( 0x3C, 0x43, 0x71 ), 
			RGB( 0x7B, 0x48, 0xEE ), 
			RGB( 0x00, 0x4A, 0x9A ), 
			RGB( 0x48, 0x41, 0xCC ), 
			RGB( 0xC7, 0x45, 0x85 ), 
			RGB( 0x19, 0x49, 0x70 ), 
			RGB( 0xF5, 0x4F, 0xFA ), 
			RGB( 0xFF, 0x44, 0xE1 ), 
			RGB( 0xFF, 0x44, 0xB5 ), 
			RGB( 0xFF, 0x4E, 0xAD ), 
			RGB( 0x00, 0x40, 0x80 ), 
			RGB( 0xFD, 0x95, 0xE6 ), 
			RGB( 0x80, 0x90, 0x00 ), 
			RGB( 0x6B, 0x9E, 0x23 ), 
			RGB( 0xFF, 0x95, 0x00 ), 
			RGB( 0xFF, 0x95, 0x00 ), 
			RGB( 0xDA, 0x90, 0xD6 ), 
			RGB( 0xEE, 0x98, 0xAA ), 
			RGB( 0x98, 0x9B, 0x98 ), 
			RGB( 0xAF, 0x9E, 0xEE ), 
			RGB( 0xDB, 0x90, 0x93 ), 
			RGB( 0xFF, 0x9F, 0xD5 ), 
			RGB( 0xFF, 0x9A, 0xB9 ), 
			RGB( 0xCD, 0x95, 0x3F ), 
			RGB( 0xFF, 0x9A, 0xB9 ),
			RGB( 0xFF, 0x90, 0xCB ), 
			RGB( 0xDD, 0x90, 0xDD ), 
			RGB( 0xB0, 0x90, 0xE6 ), 
			RGB( 0x80, 0x90, 0x80 ), 
			RGB( 0xFF, 0x90, 0x00 ), 
			RGB( 0xBC, 0xCF, 0x8F ), 
			RGB( 0x41, 0xC9, 0xE1 )
		};
#pragma  endregion
		static float colours_white[4] = {0.7f,0.7f,0.7f,1.0f};
		static float colours_light_blue[4] = {0.2f,0.2f,0.6f,1.0f};

		ISceneManager*			g_pSM = NULL;
		core::matrix4			g_viewTransMat;			// 三维坐标转换屏幕坐标矩阵
		core::dimension2d<u32>  g_viewDim;

		// 设置屏幕坐标和三维坐标转换参数
		void SetCoordTransMat()
		{
			if(g_pSM)
			{
				ICameraSceneNode* pCamera = g_pSM->getActiveCamera();
				IVideoDriver* pDriver = g_pSM->getVideoDriver();
				const core::rect<s32>& viewPort = pDriver->getViewPort();
				g_viewDim.set(viewPort.getWidth() / 2,viewPort.getHeight() / 2);

				g_viewTransMat = pCamera->getProjectionMatrix();
				g_viewTransMat *= pCamera->getViewMatrix();
			}
		}

		//! 判断aabbox立方盒是否在视图内
		bool IsCubeIn(core::aabbox3d<f32>& cube)
		{
			if(g_pSM)
			{
				ICameraSceneNode* pCamera = g_pSM->getActiveCamera();
				return pCamera->getViewFrustum()->isCubeIn(cube);
			}
			else
				return false;
		}
		// 三维坐标转换屏幕坐标
		BOOL ViewCoordTrans(const f32& x,const f32& y,const f32& z,s32& srcX,s32& srcY)
		{
			f32 transformedPos[4] = { x, y, z, 1.0f };

			g_viewTransMat.multiplyWith1x4Matrix(transformedPos);

			if (transformedPos[3] < 0)
			{
				srcX = -1;
				srcY = -1;
				return FALSE;
			}

			const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :1.0f/transformedPos[3];

			srcX = s32(g_viewDim.Width * transformedPos[0] * zDiv + 0.5f) + g_viewDim.Width;
			srcY = g_viewDim.Height - s32(g_viewDim.Height * (transformedPos[1] * zDiv) + 0.5);	
			return !(srcX < 0 || srcX > g_viewDim.Width * 2 || srcY < 0 || srcY > g_viewDim.Height * 2);
		}

		CScanSceneNode::~CScanSceneNode(void)
		{
			::DeleteCriticalSection(&m_cs);

			for (u32 i = 0;i<m_renderColors.size();i++)
			{
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + i);
				colors.clear();
			}
			m_renderColors.clear();

			std::vector<u8> ().swap(m_vecInView);
		}

		CScanSceneNode::CScanSceneNode(irr::scene::ISceneNode* parent,irr::scene::ISceneManager* mgr,s32 id)
			:IObjectSceneNode(NULL,video::SColor(255,255,0,0),g_selColor,parent,mgr,id),
			m_transparence(0.3f)/*,m_renderTin(FALSE)*/,m_bPcdChanged(false)/*,m_pView(NULL)*/,m_RenderCount(0),m_bTrans(false),
			m_renderBBox(FALSE),
			m_colorRampByCol(0,0,4),
			m_colorRampZ(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4),
			m_colorRampCycle(COLORARGB(255,0,0,180),COLORARGB(255,180,0,0),4)
		{
			::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000400 );
			//g_pSM = SceneManager;
			m_countInView = 0;

			m_pointCloud = NULL;
			m_renderCls = CLASS_ALL;
			m_renderStyle = RENDER_BY_Z;
			m_showStyle = SHOW_ALL;
			m_simple = 1;
			m_simpleCount = 5000000;
			m_material.Wireframe = false;
			m_material.Lighting = false;
			m_material.Thickness = 2; // 大小 为2 
			m_material.ZWriteEnable = true;
            m_bIsRender = true;

			m_material.ZBuffer = ECFN_LESS;

			m_seed = (u32)time(NULL);
			m_maxIntensity_rcp = 1.0;
			//m_defaultClr.set(0.5,0.5,0.5);
			m_defaultClr.r = m_defaultClr.g = m_defaultClr.b = 255 /2;
			m_PointsizeBtn = 2; // 粒子大小按钮为2 蔡红云 2013/11/12
			m_SubSample = FALSE;
			m_MaxIntensity = 255;
			m_MinIntensity = 0;
			m_nMinIntensity = 0;
			m_nMaxIntensity = 0;
			m_nMaxCol = 0;
			m_nMinCol = 0;
			m_fMinHeight = 0.0f;
			m_fMaxHeight = 0.0f;
			m_fMinCorX = 0.0f;
			m_fMaxCorX = 0.0f;
			m_fMinCorY = 0.0f;
			m_fMaxCorY = 0.0f;
			m_fMinCoord = 0.0f;
			m_fMaxCoord = 0.0f;
			m_fStep = 0.0f;
			m_fCycleStep = 10.f;
			m_nAxis = 0;

			m_fMinDist = 0.0f;
			m_fMaxDist = 0.0f;

			for (int i = 0; i< 256;i++)
			{
				m_nColorIndex[i] = i;
				//m_bRenderClassIndex[i] = true;
			}
			m_nFeatureThreshold = 0;
			m_nFeatureTypeIndex = 0;
			m_dFeatureThreshold = 0.0f;
			m_bShowIntenRender = TRUE;
			m_nCurSelCount = 0;
			m_bAreaRender = false;
			setAutomaticCulling(irr::scene::EAC_OFF);
			m_renderAreaStyle = RENDER_AREA_BY_DEFAULT;
			m_selPtColor = RGB(0,0,255);
			//m_bIsRenderSetting = false;
			m_scant = 0;
			m_isStateCoord = false;
			m_fAreaMaxCoord = 0.0f;
			m_fAreaMinCoord = 0.0f;
			m_fAreaMaxX = 0.0f;
			m_fAreaMinX = 0.0f;
			m_fAreaMaxY = 0.0f;
			m_fAreaMinY = 0.0f;
			m_fAreaMaxHeight = 0.0f;
			m_fAreaMinHeight = 0.0f;
			m_bRenderSBox = FALSE;
			m_bUseMat = false;

			m_renderColors.clear();

			m_bLockMemoryPts = false;

			m_ColoraSetting = 255;
		}

        void CScanSceneNode::RefreshRenderModel(CBursaWolfModel& absModel)
        {
            // 获取视图的模型参数
            CBursaWolfModel* pBursaModel = m_pView->GetTransModel();

            if (pBursaModel)
            {
                if (m_pView->GetViewType() == E_HVT_SKETCH_ISCAN3D || m_pView->GetViewType() == E_HVT_MULTISCAN3D)
                {
                    m_renderModel =(*pBursaModel)*absModel;
                    if (m_renderModel.IsIdentity())
                    {
                        m_bTrans = false;
                    }
                    else
                    {
                        m_bTrans = true;
                    }
                }
            }
        }

		//! 设置点云数据
		BOOL CScanSceneNode::SetPointCloud(PointCloud* pcd,CBursaWolfModel model)
		{
			//EnterCriticalSection(&m_cs);
			g_bSettingPcd = true;

			if (pcd != m_pointCloud)
			{
				m_bPcdChanged = true;
			}

			m_pointCloud = pcd;
			if (m_pointCloud && m_pointCloud->count() > 0)
			{
				// 获取绝对坐标转换模型
				m_absModel = model;

				// 计算场景结点的显示模型参数 [2014/03/19 危迟]
				if (m_RenderCount == 0)
				{
					// 获取视图的模型参数
					CBursaWolfModel* pBursaModel = m_pView->GetTransModel();

					if (pBursaModel)
					{
						// 如果当前视图iScan3D或者多测站视图
						if (m_pView->GetViewType() == E_HVT_MULTISCAN3D || m_pView->GetViewType() == E_HVT_SKETCH_ISCAN3D ||
							strcmp(m_pView->GetName(),"iScan3DView") == 0)
						{
							m_renderModel =(*pBursaModel)*m_absModel;
							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}

						}
						// 如果当前视图为快速视图
						else if (m_pView->GetViewType() == E_HVT_QUICK || m_pView->GetViewType() == E_HVT_REG_QUICK)
						{
							m_renderModel = m_absModel;
							m_bTrans = false;			
						}
						else if(m_pView->GetViewType() == E_HVT_FACADEEDIT)
						{
							CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
							m_renderModel = ivtModel*m_absModel;

							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}
						}
						//如果当前视图为单测站三位视图
						else
						{
							m_renderModel = (*pBursaModel)*m_absModel;
							if (m_renderModel.IsIdentity())
							{
								m_bTrans = false;
							}
							else
							{
								m_bTrans = true;
							}
							//m_bTrans = false;
						}
					}
					m_RenderCount++;
				}
				//m_distImg.setMinMax(m_pointCloud->getMinDistance(),m_pointCloud->getMaxDistance());

				// 如果头文件得到的点云范围不正确，则重新统计--zhubo.2014.01.08
				float minX,minY,minZ,maxX,maxY,maxZ;
				minX = minY = minZ = F32_MAX;
				maxX = maxY = maxZ = F32_MIN;

				//m_pointCloud->UpdateExtent();
				minX = m_pointCloud->m_header.min_x;
				minY = m_pointCloud->m_header.min_y;
				minZ = m_pointCloud->m_header.min_z;
				maxX = m_pointCloud->m_header.max_x;
				maxY = m_pointCloud->m_header.max_y;
				maxZ = m_pointCloud->m_header.max_z;

				f64 fLoadStartScale,fLoadEndScale;
				fLoadStartScale = fLoadEndScale = 0.0;
				m_pointCloud->GetLoadScale(fLoadStartScale,fLoadEndScale);

				if (fabs(maxX - minX) <= 0.00000001 || 
					fabs(maxY - minY) <= 0.00000001 ||
					fabs(maxZ - minZ) <= 0.00000001 ||
					!(fabs(fLoadStartScale - 0.0) <= 0.00000001 && fabs(fLoadEndScale - 1.0f) <= 0.00000001))
				{
					minX = minY = minZ = F32_MAX;
					maxX = maxY = maxZ = F32_MIN;

					u32 loopCount = m_pointCloud->getLoopCount();
					for (u32 n = 0;n<loopCount;n++)
					{
						const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
						for (U32 i = 0;i < pts.size();i++)
						{
							const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
							if (!pt.isValid())
								continue;
							minX = MIN(minX,pt.x);
							minY = MIN(minY,pt.y);
							minZ = MIN(minZ,pt.z);

							maxX = MAX(maxX,pt.x);
							maxY = MAX(maxY,pt.y);
							maxZ = MAX(maxZ,pt.z);
						}
					}

					m_pointCloud->m_header.min_x = minX;
					m_pointCloud->m_header.min_y = minY;
					m_pointCloud->m_header.min_z = minZ;
					m_pointCloud->m_header.max_x = maxX;
					m_pointCloud->m_header.max_y = maxY;
					m_pointCloud->m_header.max_z = maxZ;
				}
				core::aabbox3d<f32> box((f32)(m_pointCloud->m_header.min_x),(f32)(m_pointCloud->m_header.min_y),
					(f32)(m_pointCloud->m_header.min_z),(f32)(m_pointCloud->m_header.max_x),
					(f32)(m_pointCloud->m_header.max_y),(f32)(m_pointCloud->m_header.max_z));

				m_box = box;
				// 动态计算抽样比例
				m_simple = m_pointCloud->getSimpleLevel();//(u32)(m_pointCloud->count() / (f32)m_simpleCount + 0.5);

				if(m_simple == 0) m_simple = 1;

				if(m_pointCloud->getMaxIntensity() != 0)
				{
					m_maxIntensity_rcp = 1.0f / (m_pointCloud->getMaxIntensity() - m_pointCloud->getMinIntensity());
				}

				m_countInView = m_pointCloud->count();

				try
				{
					// 默认设置所有圈在屏幕范围内
					u32 loopCount = m_pointCloud->getLoopCount();
					m_vecInView.resize(loopCount);

					for (u32 i = 0;i < loopCount;i++)
					{
						m_vecInView.at(i) = 1;
					}

					m_renderColors.resize(loopCount);

					for (u32 i = 0;i < loopCount;i++)
					{
						hdVector<RenderColor>& loopColor = *(m_renderColors._Myfirst + i);
						loopColor.resize(m_pointCloud->getLoop(i).size());	

					}
				}
				catch (...)
				{
					m_renderColors.clear();
					m_pointCloud = NULL;
					//LeaveCriticalSection(&m_cs);
					g_bSettingPcd = false;
					return FALSE;
				}

				// 根据强度计算透明度是必须首先要处理的
				StatIntensity();
				CalcuIntensityRender();
				// 计算高程颜色
				SetRenderStyle(RENDER_BY_Z);
				SetAreaRenderStyle(m_renderAreaStyle);
			}

			//LeaveCriticalSection(&m_cs);
			g_bSettingPcd = false;
			return TRUE;
		}

		BOOL CScanSceneNode::SetPointCloud( PointCloud* pcd )
		{
			if (!pcd)
			{
				return FALSE;
			}

			CBursaWolfModel model = pcd->GetModel();

			return SetPointCloud(pcd,model);
		}

		//! 统计视图范围内点个数
		u64 CScanSceneNode::GetCountInView()
		{
			if(m_pointCloud == NULL)
				return 0;
			u64 count = m_pointCloud->count();

			irr::core::aabbox3df loopExtent;
			if (m_pointCloud->getEditMode() == 1)
			{		
				// 设置FOV目的,是让camera调用recalculateProjectionMatrix
				//SceneManager->getActiveCamera()->setFOV(SceneManager->getActiveCamera()->getFOV());
				g_pSM = SceneManager;
				if (!SceneManager || (SceneManager && SceneManager->getActiveCamera() == NULL))
				{
					return 0;
				}
				const SViewFrustum* pViewFrustom = SceneManager->getActiveCamera()->getViewFrustum();
				f32 xmin,ymin,zmin,xmax,ymax,zmax;
				f64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;

				u32 loopCount = m_pointCloud->getLoopCount();
				m_vecInView.resize(loopCount);
				count = 0;
				for (u32 n = 0;n < loopCount;n++)
				{
					irr::core::aabbox3df loopExtentTmp;
					loopExtentTmp.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
					loopExtentTmp.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

					// 存在只加载某一段点云情况，需进行判断
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					if (pts.size() <= 0)
					{
						m_vecInView.at(n) = 0;
						continue;
					}

					m_pointCloud->getLoopExtent(n,xmin,ymin,zmin,xmax,ymax,zmax);

					xminD = xmin;
					yminD = ymin;
					zminD = zmin;

					xmaxD = xmax;
					ymaxD = ymax;
					zmaxD = zmax;

					loopExtent.MinEdge.set(xmin,ymin,zmin);
					loopExtent.MaxEdge.set(xmax,ymax,zmax);

					if (m_bTrans)
					{
						//m_absModel.TranslateExtentW(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);
						//m_renderModel.TranslateExtentW(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);
						core::vector3df edges[8];
						loopExtent.getEdges(edges);
						for (int i =0 ; i!=8;i++)
						{
							m_renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
							loopExtentTmp.addInternalPoint(edges[i]);
						}
						loopExtent = loopExtentTmp;

					}
					/*		loopExtent.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
					loopExtent.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);*/
					if(pViewFrustom->isCubeIn(loopExtent))
					{
						// 获得该圈点云的有效点个数，而不是总点数
						count += m_pointCloud->GetValidCountInLoop(n);
						//count += m_pointCloud->getLoop(n).size();
						m_vecInView.at(n) = 1;
					}
					else
					{
						m_vecInView.at(n) = 0;
					}
				}				
			}
			return count;
		}

		/*	void CScanSceneNode::SetBoundbox(BOOL IsShowBox, core::aabbox3d<f32> box)
		{
		m_SpecialBox = box;
		m_bRenderSBox = IsShowBox; 
		}*/

		void CScanSceneNode::UpdateExtent(const hd::CHd3dBoxEx extent)
		{
			if(!m_pView)
			{
				return;
			}
			u32 loopCount = m_pointCloud->getLoopCount();

			if (m_pView->GetViewType() == E_HVT_3D)
			{
				m_vecInView.resize(loopCount);
				m_countInView = 0;
				for (u32 i = 0; i< loopCount;i++)
				{
					// 如果该圈内有点被选中，则需要渲染该圈
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(i);
					for (unsigned int j = 0;j < pts.size();j++)
					{
						PointXYZIPRGBA& pt = *(pts._Myfirst + j);
						if (!pt.isValid())
						{
							continue;
						}
						if (!pt.isSelected())
						{
							continue;
						}
						else
						{
							m_vecInView.at(i) = 1;
							break;
						}
						m_vecInView.at(i) = 0;
					}
				}
			}

			// 更新包围盒
			m_box.MinEdge.X = extent.boundingBox.MinEdge.X;
			m_box.MinEdge.Y = extent.boundingBox.MinEdge.Y;
			m_box.MinEdge.Z = extent.boundingBox.MinEdge.Z;
			m_box.MaxEdge.X = extent.boundingBox.MaxEdge.X;
			m_box.MaxEdge.Y = extent.boundingBox.MaxEdge.Y;
			m_box.MaxEdge.Z = extent.boundingBox.MaxEdge.Z;
		}

		void CScanSceneNode::CalcuRenderColor()
		{
			u32 loopCount = m_pointCloud->getLoopCount();
			// 申请渲染颜色内存
			m_renderColors.resize(loopCount);
			for (u32 i = 0;i < loopCount;i++)
			{
				hdVector<RenderColor>& loopColor = *(m_renderColors._Myfirst + i);
				loopColor.resize(m_pointCloud->getLoop(i).size());
			}

			// 根据强度计算透明度是必须首先要处理的
			CalcuIntensityRender();

			if (m_renderStyle == RENDER_BY_Z || m_renderStyle == RENDER_BY_X || m_renderStyle == RENDER_BY_Y)
			{
				//! 统计高度范围
				if (getType() == ESNT_CLASSIFY_PTD)
				{
					StatCoord();
				}
				// 否则由视图统计坐标范围
				else
				{
					m_pView->statAllScanSndeStatCoord();
				}

				if (m_renderStyle == RENDER_BY_Z)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_renderStyle == RENDER_BY_X)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_renderStyle == RENDER_BY_Y)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCoordRender();
			}
			else if (m_renderStyle == RENDER_BY_DISTANCE)
			{
				//! 统计距离范围
				StatDistance();
				CalcuDistRender();
			}
			else if(m_renderStyle == RENDER_BY_COL)
			{
				CalcuColRender();
			}
			else if (m_renderStyle == RENDER_BY_CYCLERAMP)
			{
				if (m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0)
				{
					//! 统计高度范围
					if (getType() == ESNT_CLASSIFY_PTD)
					{
						StatCoord();
					}
					else
					{
						m_pView->statAllScanSndeStatCoord();

					}
				}

				if (m_nAxis == 0)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_nAxis == 1)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_nAxis == 2)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				CalcuCycleRampRender();
			}
		}

		BOOL CScanSceneNode::ReloadData()
		{
			if(m_pView == NULL || m_pointCloud == NULL)
				return FALSE;

			g_pSM = SceneManager;

			// 编辑模式下需要统计视图范围内的数据
			if (m_pointCloud->getEditMode() == 1)
			{
				// 编辑模式下，如果锁定内存，也不让其更新数据
				if (m_bLockMemoryPts)
				{
					return TRUE;
				}

				if (getType() == ESNT_CLASSIFY_PTD)
				{
					m_countInView = GetCountInView();
				}
				else
				{
					// 视图改变了,统计在视图内的点个数和每圈是否在范围内
					if (m_pointCloud->isLinExist())
					{
						if ( CHdSysSetting::getSysSetting()->iScanSetting.loadByObbBox)
						{
							m_countInView = GetCountInViewByObbBox();
						}
						else
						{
							m_countInView = GetCountInView();
						}

					}
					else
					{
						m_countInView = GetCountInView();
					}
				}

				return m_countInView > 0 ? TRUE:FALSE;
			}
			else
			{
				// 浏览模式下锁定了内存,则不更新数据[zf 2014/6/7]
				if (m_bLockMemoryPts)
				{
					return TRUE;
				}
			}

			// 锁定代码块
			EnterCriticalSection(&m_cs);
			// 浏览模式要根据当前视图范围加载数据
			int srcWidth = m_pView->GetWindowWidth();
			int srcHeight = m_pView->GetWindowHeight();
			SetCoordTransMat();
			BOOL bRet = FALSE;
			if (m_pointCloud->isLinExist())
			{
				if (CHdSysSetting::getSysSetting()->iScanSetting.loadByObbBox)
				{
					// 获得视锥的aab box
					const core::aabbox3df& box = SceneManager->getActiveCamera()->getViewFrustum()->getBoundingBox();

					// 获得视锥的obb box
					CHdobBox3d viewBox;
					GetObbBoxByView(viewBox);

					bRet = m_pointCloud->LoadByObbViewPort(viewBox,box);
					UpdateVecInView();
				}
				else
				{
					const core::aabbox3df& box = SceneManager->getActiveCamera()->getViewFrustum()->getBoundingBox();

					//// 传入引用vec
					//m_vecInView.clear();
					bRet = m_pointCloud->LoadByViewPort(/*m_vecInView,*/IsCubeIn,srcWidth,srcHeight,//ViewCoordTrans
						CHdBox3df(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z,
						box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z),SceneManager->getActiveCamera()->isOrthogonal());

					if (bRet)
					{
						UpdateVecInView();
					}
				}
			}
			else
			{
				const core::aabbox3df& box = SceneManager->getActiveCamera()->getViewFrustum()->getBoundingBox();

				//// 传入引用vec
				//m_vecInView.clear();
				bRet = m_pointCloud->LoadByViewPort(/*m_vecInView,*/IsCubeIn,srcWidth,srcHeight,//ViewCoordTrans
					CHdBox3df(box.MinEdge.X,box.MinEdge.Y,box.MinEdge.Z,
					box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z),SceneManager->getActiveCamera()->isOrthogonal());

				if (bRet)
				{
					UpdateVecInView();
				}
			}

			if (bRet)
			{
				CalcuRenderColor();
			}
			LeaveCriticalSection(&m_cs);
			return bRet;
		}

		//! 统计当前视图范围内坐标范围
		void CScanSceneNode::CalcuCoordRenderView(int renderSimpleCol,int renderSimpleRow)
		{
			float maxHeight = 0.0f;
			float minHeight = 0.0f;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;

			//统计高度分布，映射至0-999
			memset(mapHeight,0,sizeof(int)*1000);
			int validCount = 0;
			float height = 0.0f;
			minHeight = m_renderStyle == RENDER_BY_Z ? m_box.MinEdge.Z : 
				(m_renderStyle == RENDER_BY_X?m_box.MinEdge.X:m_box.MinEdge.Y);

			maxHeight = m_renderStyle == RENDER_BY_Z ? m_box.MaxEdge.Z : 
				(m_renderStyle == RENDER_BY_X?m_box.MaxEdge.X:m_box.MaxEdge.Y);

			float boxHeight = maxHeight - minHeight;
			float boxHeightRcp = 1000.0f / (boxHeight);

			u32 loopCount = m_pointCloud->getLoopCount();
			int editMode = m_pointCloud->getEditMode();
			for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
			{
				// 编辑模式下,静态点云需要判断当前圈是否在视图内
				if (editMode == 1 && (m_vecInView.at(n)) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i+=renderSimpleRow)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid())
						continue;
					validCount++;
					RenderColor& color = *(colors._Myfirst + i);

					height = pt.z;
					if (m_renderStyle == RENDER_BY_X)
					{
						height = pt.x;
					}
					else if (m_renderStyle == RENDER_BY_Y)
					{
						height = pt.y;
					}

					selNum = (int)((height - minHeight) * boxHeightRcp);
					selNum = clamp(selNum,0,999);
					mapHeight[selNum]++;
				}
			}

			//调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
			selNum = 0;
			for (int j = 999;j >= 0;j--)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(validCount * 0.03))
				{
					selMax = j;
					break;
				}
			}

			selNum = 0;
			for (int j = 0;j <= 999;j++)
			{
				selNum += mapHeight[j];
				if (selNum >= (int)(validCount * 0.03))
				{
					selMin = j;
					break;
				}
			}

			//重新设置新的最大最小高程值
			m_fMinCoord = minHeight + selMin * (boxHeight) / 1000.0f;
			m_fMaxCoord = maxHeight + selMax * (boxHeight) / 1000.0f;

			m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

			// -------计算按坐标显示颜色-----------
			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			float transparent = 1.0f;

			int selStep = 0;
			float tempValue = 0.0f;

			// 遍历内存点云,计算每个点渲染颜色
			for (u32 n = 0;n < loopCount;n++)// 遍历圈
			{
				// 编辑模式下,静态点云需要判断当前圈是否在视图内
				if (editMode == 1 && (m_vecInView.at(n)) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i++)// 遍历圈内每个点
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid())
						continue;

					RenderColor& color = *(colors._Myfirst + i);

					height = pt.z;
					if (m_renderStyle == RENDER_BY_X)
					{
						height = pt.x;
					}
					else if (m_renderStyle == RENDER_BY_Y)
					{
						height = pt.y;
					}

					if (height < m_fMinCoord)
					{
						color.c_color = beginColor;
					}
					else if (height > m_fMaxCoord)
					{
						color.c_color = endColor;
					}
					else
					{
						selStep = (int)((height - m_fMinCoord) / m_fStep);

						tempValue = height - m_fMinCoord - selStep * m_fStep;
						scale = tempValue / m_fStep;
						m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
						//color.r = u8(r * 255);
						//color.g = u8(g * 255);
						//color.b = u8(b * 255);
					}
				}
			}

		}

		//! 统计坐标分布范围
		void CScanSceneNode::StatCoord()
		{	
			//long t1,t2,t3;
			//t1 = clock();
			// 如果已经统计过坐标范围、无需统计 
			if (m_isStateCoord)
			{
				return;
			}
			m_heightStep.clear();
			unsigned long long count = m_pointCloud->count();
			U32 validCount = m_pointCloud->getValidCount();
			float maxHeight = 0.0f;
			float minHeight = 0.0f;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;
			if (m_pointCloud == NULL || count <= 0 || validCount <= 0)
			{
				return;
			}
			// 多测站或者ISan视图下存在多个点云时
			// 重新统计最大值和最小值

			core::aabbox3d<f32> box = m_box;
			CBursaWolfModel absModel;
			CBursaWolfModel pcdModel; //点云转换模型
			//bool bTrans = false;
			//pcdModel = m_pointCloud->GetModel();
			//CBursaWolfModel* pBursaModel = m_pView->GetTransModel();
			//if (pBursaModel)
			//{
			//	// 判断视图模型与场景结点模型是否一致
			//	if (!((*pBursaModel) == m_absModel/*pcdModel*/))
			//	{
			//		CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
			//		absModel = m_absModel/*ivtModel * pcdModel*/;
			//		bTrans = true;
			//	}
			//}
			// 获取box的中心
			irr::core::vector3df center = box.getCenter();
			irr::core::vector3df extent = box.getExtent();

			//if (bTrans /*&& m_scant != 0*/)
			if (m_bTrans)
			{
				//absModel.Translate(center.X,center.Y,center.Z);
				m_renderModel.Translate(center.X,center.Y,center.Z);
				box.MinEdge.X = center.X - extent.X/2.f;
				box.MaxEdge.X = center.X + extent.X/2.f;
				box.MinEdge.Y = center.Y - extent.Y/2.f;
				box.MaxEdge.Y = center.Y + extent.Y/2.f;
				box.MinEdge.Z = center.Z - extent.Z/2.f;
				box.MaxEdge.Z = center.Z + extent.Z/2.f;

			} 

			u32 loopCount = m_pointCloud->getLoopCount();
#pragma region 统计z分布
			{

				m_heightStep.resize(10);

				//统计高度分布，映射至0-999
				memset(mapHeight,0,sizeof(int)*1000);

				int iCount = 0;
				float height = 0.0f;
				minHeight = box.MinEdge.Z;
				maxHeight = box.MaxEdge.Z;
				float boxHeight = maxHeight - minHeight;
				float boxHeightRcp = 1000.0f / (boxHeight);

//#pragma omp parallel for
				for (u32 n = 0;n<loopCount;n++)
				{
					// 在浏览模式下，不在视图范围内的圈数据不渲染
					if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
					{
						continue;
					}

					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
//#pragma omp parallel for
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

						if (pt.isValid())
						{
							PointXYZIPRGBA pttmp = pt;
							//if (bTrans /*&& m_scant != 0*/)
							if (m_bTrans)
							{
								//absModel.Translate(pttmp.x, pttmp.y, pttmp.z);
								m_renderModel.Translate(pttmp.x, pttmp.y, pttmp.z);
							}
							selNum = (int)((pttmp.z - minHeight) * boxHeightRcp);
							selNum = clamp(selNum,0,999);
							mapHeight[selNum]++;
						}
					}
				}				

				//调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapHeight[j];
					if (selNum >= (int)(validCount * 0.03))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapHeight[j];
					if (selNum >= (int)(validCount * 0.03))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小高程值
				minHeight = box.MinEdge.Z + selMin * (boxHeight) / 1000.0f;
				maxHeight = box.MinEdge.Z + selMax * (boxHeight) / 1000.0f;

				float tempHeight = (maxHeight - minHeight) / 10.0f;
				for (int i = 0; i < 10;i++)
				{
					m_heightStep[i] = minHeight + (i + 1) * tempHeight;
				}
				m_fMinHeight = minHeight;
				m_fMaxHeight = maxHeight;
			}
#pragma endregion
			//t2 = clock();
			//t3 = t2 - t1;
			//int tst = t3;
			//char strTest[256] = {0};
			//sprintf_s(strTest,"%s:%d","使用parallel花费时间: ",tst);
			//OutputDebugString(strTest);
#pragma region  统计X分布
			{
				// 统计X分布
				m_xStep.resize(10);
				int mapCorX[1000];
				//统计X坐标分布，映射至0-255
				float maxCorX = 0.0f;
				float minCorX = 0.0f;
				float fCorX = 0.0f;
				selMax = 0;
				selMin = 0;
				selNum = 0;
				memset(mapCorX,0,sizeof(int)*1000);

				maxCorX = box.MaxEdge.X;
				minCorX = box.MinEdge.X;
				float boxDx = maxCorX - minCorX;
				float boxDxRcp = 1000.0f/boxDx;

				for (u32 n = 0;n<loopCount;n++)
				{
					// 在浏览模式下，不在视图范围内的圈数据不渲染
					if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
					{
						continue;
					}

					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if (pt.isValid())
						{
							//fCorX = pt.x;
							PointXYZIPRGBA pttmp = pt;
							//if (bTrans/* && m_scant != 0*/)
							if (m_bTrans)
							{
								//absModel.Translate(pttmp.x, pttmp.y, pttmp.z);
								m_renderModel.Translate(pttmp.x, pttmp.y, pttmp.z);
							}
							selNum = (int)((pttmp.x - minCorX) * boxDxRcp);
							selNum = clamp(selNum,0,999);
							mapCorX[selNum]++;
						}
					}
				}

				//调整X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapCorX[j];
					if (selNum >= (int)(validCount * 0.000003))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapCorX[j];
					if (selNum >= (int)(validCount* 0.000003))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小X值
				minCorX = box.MinEdge.X + selMin * (boxDx) / 1000.0f;
				maxCorX = box.MinEdge.X + selMax * (boxDx) / 1000.0f;

				float tempX = (maxCorX - minCorX) / 10.0f;
				for (int i = 0; i < 10;i++)
				{
					m_xStep[i] = minCorX + (i + 1) * tempX;
				}
				m_fMinCorX = minCorX;
				m_fMaxCorX = maxCorX;
			}
#pragma endregion

#pragma region 统计Y分布
			{
				// 统计Y分布
				m_yStep.resize(10);
				int mapCorY[1000];
				//统计Y坐标分布，映射至0-999
				float maxCorY = 0.0f;
				float minCorY = 0.0f;
				float fCorY = 0.0f;
				selMax = 0;
				selMin = 0;
				selNum = 0;
				memset(mapCorY,0,sizeof(int)*1000);

				maxCorY = box.MaxEdge.Y;
				minCorY = box.MinEdge.Y;
				float boxDy = maxCorY - minCorY;
				float boxDyRcp = 1000.0f/boxDy;

				for (u32 n = 0;n<loopCount;n++)
				{
					// 在浏览模式下，不在视图范围内的圈数据不渲染
					if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
					{
						continue;
					}

					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if (pt.isValid())
						{
							//fCorY = pt.y;
							PointXYZIPRGBA pttmp = pt;
							//if (bTrans /*&& m_scant != 0*/)
							if (m_bTrans)
							{
								//absModel.Translate(pttmp.x, pttmp.y, pttmp.z);
								m_renderModel.Translate(pttmp.x, pttmp.y, pttmp.z);
							}
							selNum = (int)((pttmp.y - minCorY) * boxDyRcp);
							selNum = clamp(selNum,0,999);
							mapCorY[selNum]++;
						}
					}
				}
				//调整X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapCorY[j];
					if (selNum >= (int)(validCount * 0.000003))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapCorY[j];
					if (selNum >= (int)(validCount * 0.000003))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小Y值
				minCorY = box.MinEdge.Y + selMin * (boxDy) / 1000.0f;
				maxCorY = box.MinEdge.Y + selMax * (boxDy) / 1000.0f;

				float tempY = (maxCorY - minCorY) / 10.0f;
				for (int i = 0; i < 10;i++)
				{
					m_yStep[i] = minCorY + (i + 1) * tempY;
				}
				m_fMinCorY = minCorY;
				m_fMaxCorY = maxCorY;
			}
#pragma endregion 

			m_bPcdChanged = false;
			m_isStateCoord = true;
		}

		//! 统计区域坐标分布范围
		void CScanSceneNode::StatAreaCoord()
		{
			//m_heightStep.clear();
			unsigned long long count = m_pointCloud->count();
			U32 selPtCount = m_pointCloud->getSelectCount();
			float maxHeight = 0.0f;
			float minHeight = 0.0f;
			int selMax = 0;
			int selMin = 0;
			int mapHeight[1000];
			int selNum = 0;

			u32 loopCount = m_pointCloud->getLoopCount();
			if (m_pointCloud == NULL || count <= 0 || selPtCount <= 0)
			{
				return;
			}

			// 统计选择点云的box范围
			float minAreaX,minAreaY,minAreaZ,maxAreaX,maxAreaY,maxAreaZ;
			minAreaX = minAreaY = minAreaZ = F32_MAX;
			maxAreaX = maxAreaY = maxAreaZ = F32_MIN;
			for (u32 i = 0;i < loopCount;i++)
			{
				const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(i);
				for (u32 j = 0;j < pts.size();j++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + j);
					if (pt.isValid() && pt.isSelected())
					{
						minAreaX = MIN(minAreaX,pt.x);
						minAreaY = MIN(minAreaY,pt.y);
						minAreaZ = MIN(minAreaZ,pt.z);
						maxAreaX = MAX(maxAreaX,pt.x);
						maxAreaY = MAX(maxAreaY,pt.y);
						maxAreaZ = MAX(maxAreaZ,pt.z);
					}
				}
			}
#pragma region 统计z分布
			{
				if (m_pointCloud == NULL || count <= 0 || selPtCount <= 0)
				{
					return;
				}
				//m_heightStep.resize(10);

				//统计高度分布，映射至0-999
				memset(mapHeight,0,sizeof(int)*1000);

				int iCount = 0;
				float height = 0.0f;
				minHeight = minAreaZ;
				maxHeight = maxAreaZ;
				float boxHeight = maxHeight - minHeight;
				float boxHeightRcp = 1000.0f / (boxHeight);

				for (u32 n = 0;n<loopCount;n++)
				{
					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if (pt.isValid() && pt.isSelected())
						{
							selNum = (int)((pt.z - minHeight) * boxHeightRcp);
							selNum = clamp(selNum,0,999);
							mapHeight[selNum]++;
						}
					}
				}				

				//调整高度分布，将点数较少部分3%剔除不参与统计，获得新的最大最小高程值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapHeight[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapHeight[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小高程值
				minHeight = minAreaZ + selMin * (boxHeight) / 1000.0f;
				maxHeight = minAreaZ + selMax * (boxHeight) / 1000.0f;

				//float tempHeight = (maxHeight - minHeight) / 10.0f;
				//for (int i = 0; i < 10;i++)
				//{
				//	m_heightStep[i] = minHeight + (i + 1) * tempHeight;
				//}
				m_fAreaMinHeight = minHeight;
				m_fAreaMaxHeight = maxHeight;
			}
#pragma endregion

#pragma region  统计X分布
			{
				// 统计X分布
				//m_xStep.resize(10);
				int mapCorX[1000];
				//统计X坐标分布，映射至0-255
				float maxCorX = 0.0f;
				float minCorX = 0.0f;
				float fCorX = 0.0f;
				selMax = 0;
				selMin = 0;
				selNum = 0;
				memset(mapCorX,0,sizeof(int)*1000);

				maxCorX = maxAreaX;
				minCorX = minAreaX;
				float boxDx = maxCorX - minCorX;
				float boxDxRcp = 1000.0f/boxDx;

				for (u32 n = 0;n<loopCount;n++)
				{
					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if (pt.isValid() && pt.isSelected())
						{
							//fCorX = pt.x;
							selNum = (int)((pt.x - minCorX) * boxDxRcp);
							selNum = clamp(selNum,0,999);
							mapCorX[selNum]++;
						}
					}
				}

				//调整X坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapCorX[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapCorX[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小X值
				minCorX = minAreaX + selMin * (boxDx) / 1000.0f;
				maxCorX = minAreaX + selMax * (boxDx) / 1000.0f;

				//float tempX = (maxCorX - minCorX) / 10.0f;
				//for (int i = 0; i < 10;i++)
				//{
				//	m_xStep[i] = minCorX + (i + 1) * tempX;
				//}
				m_fAreaMinX = minCorX;
				m_fAreaMaxX = maxCorX;
			}
#pragma endregion

#pragma region 统计Y分布
			{
				// 统计Y分布
				//m_yStep.resize(10);
				int mapCorY[1000];
				//统计Y坐标分布，映射至0-999
				float maxCorY = 0.0f;
				float minCorY = 0.0f;
				float fCorY = 0.0f;
				selMax = 0;
				selMin = 0;
				selNum = 0;
				memset(mapCorY,0,sizeof(int)*1000);

				maxCorY = maxAreaY;
				minCorY = minAreaY;
				float boxDy = maxCorY - minCorY;
				float boxDyRcp = 1000.0f/boxDy;

				for (u32 n = 0;n<loopCount;n++)
				{
					const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (U32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if (pt.isValid() && pt.isSelected())
						{
							//fCorY = pt.y;
							selNum = (int)((pt.y - minCorY) * boxDyRcp);
							selNum = clamp(selNum,0,999);
							mapCorY[selNum]++;
						}
					}
				}
				//调整Y坐标分布，将点数较少部分剔除不参与统计，获得新的最大最小X值
				selNum = 0;
				for (int j = 999;j >= 0;j--)
				{
					selNum += mapCorY[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMax = j;
						break;
					}
				}

				selNum = 0;
				for (int j = 0;j <= 999;j++)
				{
					selNum += mapCorY[j];
					if (selNum >= (int)(selPtCount * 0.03))
					{
						selMin = j;
						break;
					}
				}

				//重新设置新的最大最小Y值
				minCorY = minAreaY + selMin * (boxDy) / 1000.0f;
				maxCorY = minAreaY + selMax * (boxDy) / 1000.0f;

				//float tempY = (maxCorY - minCorY) / 10.0f;
				//for (int i = 0; i < 10;i++)
				//{
				//	m_yStep[i] = minCorY + (i + 1) * tempY;
				//}
				m_fAreaMinY = minCorY;
				m_fAreaMaxY = maxCorY;
			}
#pragma endregion 
		}

		void CScanSceneNode::SetRenderClass( const ENUM_POINTCLASS& renderCls )
		{
			m_renderCls = renderCls;
		}

		ENUM_POINTCLASS CScanSceneNode::GetRenderClass() const
		{
			return m_renderCls;
		}

		void CScanSceneNode::SetRenderStyle( const ENUM_RENDERSTYLE& style )
		{
			// 若为区域渲染，考虑区域点云可能变化，而需要进行二次渲染
			if (m_renderStyle == style && (style != RENDER_BY_Z && style != RENDER_BY_X && style != RENDER_BY_Y && style != RENDER_BY_COL && style != RENDER_BY_CYCLERAMP && style != RENDER_BY_DISTANCE))
			{
				return;
			}
			m_renderStyle = style;

			if (style == RENDER_BY_Z || style == RENDER_BY_X || style == RENDER_BY_Y)
			{
				// m_bShowIntenRender该值应该是外部传入，初始构造默认为true就可以了，内部不应该改变-zhubo
				//m_bShowIntenRender = TRUE;

				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if (  m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0 || m_bAreaRender)
				{
					//! 统计高度范围
					// 如果点云是分类点云单独统计自己的坐标范围
					if (getType() == ESNT_CLASSIFY_PTD  )
					{
						StatCoord();
					}
					// 否则通过视图来统计坐标范围 
					else
					{
						if (scnList.size()!= m_scant  )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}
						else
						{
							if (m_bAreaRender)
							{
								StatCoord();
							}
						}
					}
					m_bAreaRender = false;
				}

				if (m_renderStyle == RENDER_BY_Z)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_renderStyle == RENDER_BY_X)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_renderStyle == RENDER_BY_Y)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCoordRender();

				//更新计算设置区域渲染颜色条颜色 
				//CalcuAreaCoordRender();// [2014/7/24 蔡红云 默认不是按照区域渲染，不需要计算，计算和区域渲染设置保持一致 ]
			}
			else if (style == RENDER_BY_INTENSITY)
			{
				//m_bShowIntenRender = TRUE;
				if (m_nMinIntensity == 0 && m_nMaxIntensity == 0)
				{
					StatIntensity();
				}
				CalcuIntensityRender();
				//CalcIntensityRenderTest();
			}
			else if (style == RENDER_BY_DISTANCE)
			{
				// 在iScan视图下，LIN文件存在时，根据轨迹点范围渲染
				if (m_pointCloud->isLinExist() && strcmp(m_pView->GetName(),"iScan3DView") == 0)
				{
					if (m_fMinDist == 0.0f && m_fMaxDist == 0.0f)
					{
						StatDistanceToLoopCenter();
					}

					// 计算分配渲染颜色
					CalcuDistToCenterRender();
				}
				else
				{
					if (m_fMinDist == 0.0f && m_fMaxDist == 0.0f)
					{
						StatDistance();
					}

					CalcuDistRender();
				}

			}
			else if(style == RENDER_BY_COL)
			{
				//m_bShowIntenRender = TRUE;
				CalcuColRender();
			}
			else if (style == RENDER_BY_RGB)
			{
				// 用RGB显示情况下,不显示透明度
				//ShowIntensityRender(FALSE);
			}
			else if (style == RENDER_BY_DEFAULT)
			{
				//m_bShowIntenRender = TRUE;
			}
			else if (style == RENDER_BY_CYCLERAMP)
			{
				//m_bShowIntenRender = TRUE;
				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if (  m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0)
				{
					//! 统计高度范围
					// 如果是分类点云，自己统计自己坐标范围
					if (getType() == ESNT_CLASSIFY_PTD)
					{
						StatCoord();
					}
					// 否则由视图统计坐标范围
					else
					{
						if (scnList.size()!= m_scant )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}

					}
				}

				if (m_nAxis == 0)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_nAxis == 1)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_nAxis == 2)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				//m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCycleRampRender();
			}
		}

		//! 计算强度显示颜色
		void CScanSceneNode::CalcuIntensityRender()
		{
			float colorI = 0.0f;
			u32 loopCount = m_pointCloud->getLoopCount();
			int intensityStep = m_nMaxIntensity - m_nMinIntensity;
			for (u32 n = 0;n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					// 点被选中时，不更新值
					if(!pt.isValid()/* || pt.isSelected()*/)
						continue;

					RenderColor& color = *(colors._Myfirst + i);

					colorI = 0.0f;
					// 计算透明度使得colorI在0.1-0.95之间,使得灰度值不至于太黑或太亮
					colorI = (float)((pt.getIntensity() - m_nMinIntensity) * 0.85f/ intensityStep);
					colorI+=0.1f;

					colorI = clamp(colorI,0.1f,0.95f);
					// 这里只需要赋值透明度即可,其他rgb是在其他渲染方式计算
					color.a = u8(colorI * 255);
				}
			}
		}

		//! 计算按坐标x/y/z显示颜色
		void CScanSceneNode::CalcuCoordRender()
		{
			// 获取视图中所选择的色带条。原因是操作按循环色带渲染之后
			// 当前色带条会变动，为了保持一致。
			float cyclstep = 10.0;
			int cur = 4;
			int axis = 0;
			m_pView->getCyclInfo(cyclstep, axis, cur);

			// 如果不相等，则重新设置颜色带
			if (cur == m_colorRampZ.GetCurCursel())
			{
				m_colorRampZ.SetRampColor4f(cur);
			}

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			float transparent = 1.0f;

			int selStep = 0;
			float tempValue = 0.0f;
			float height = 0.0f;

			// 乘法处理速度更快
			float fSubStep = 1.0f / m_fStep;

			// 获取视图所对应的变换模型
			CBursaWolfModel* pBursaModel = m_pView->GetTransModel();
			// 转换模型
			//CBursaWolfModel absModel;
			//CBursaWolfModel pcdModel; //点云转换模型
			//bool bTrans = false;
			//pcdModel = m_pointCloud->GetModel();
			//if (pBursaModel)
			//{
			//	// 判断视图模型与场景结点模型是否一致
			//	if (!((*pBursaModel) == m_absModel/*pcdModel*/))
			//	{
			//		CBursaWolfModel ivtModel = pBursaModel->getAntiModel();
			//		absModel = m_absModel/*ivtModel * pcdModel*/;
			//		bTrans = true;
			//	}
			//}

			// 遍历内存点云,计算每个点渲染颜色
			u32 loopCount = m_pointCloud->getLoopCount();
#pragma omp parallel for
			for (u32 n = 0;n < loopCount;n++)// 遍历圈
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
#pragma omp parallel for
				for (u32 i = 0;i < pts.size();i++)// 遍历圈内每个点
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid()/* || pt.isSelected()*/)
						continue;
					PointXYZIPRGBA pttmp = pt;
					//if (bTrans/* && m_scant >1*/)
					if (m_bTrans)
					{
						//absModel.Translate(pttmp.x, pttmp.y, pttmp.z);
						m_renderModel.Translate(pttmp.x, pttmp.y, pttmp.z);
					}

					RenderColor& color = *(colors._Myfirst + i);

					if (m_renderStyle == RENDER_BY_Z)
					{
						height = pttmp.z;
					}
					else if (m_renderStyle == RENDER_BY_X)
					{
						height = pttmp.x;
					}
					else if (m_renderStyle == RENDER_BY_Y)
					{
						height = pttmp.y;
					}

					if (height < m_fMinCoord)
					{
						color.r = ((beginColor>>16) & 0xff);
						color.g = ((beginColor>>8) & 0xff);
						color.b = (beginColor & 0xff);
					}
					else if (height > m_fMaxCoord)
					{
						color.r = ((endColor>>16) & 0xff);
						color.g = ((endColor>>8) & 0xff);
						color.b = (endColor & 0xff);
					}
					else
					{
						selStep = (int)((height - m_fMinCoord) * fSubStep);// m_fStep

						tempValue = height - m_fMinCoord - selStep * m_fStep;
						scale = tempValue * fSubStep;// / m_fStep
						m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
					}
				}
			}
		}

		//! 计算按循环色带渲染颜色
		void CScanSceneNode::CalcuCycleRampRender()
		{
			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			float transparent = 1.0f;

			int selStep = 0;
			float tempValue = 0.0f;
			float height = 0.0f;
			float tmpHeight = 0.f;
			// 遍历内存点云,计算每个点渲染颜色
			u32 loopCount = m_pointCloud->getLoopCount();
			float minZ = m_pointCloud->m_header.min_z;
			float maxZ = m_pointCloud->m_header.max_z;
			float minX = m_pointCloud->m_header.min_x;
			float maxX = m_pointCloud->m_header.max_x;
			float minY = m_pointCloud->m_header.min_y;
			float maxY = m_pointCloud->m_header.max_y;

			// 更新最小值
			core::aabbox3d<f32> box = m_box;

			// 获取box的中心
			irr::core::vector3df center = box.getCenter();
			irr::core::vector3df extent = box.getExtent();

			minZ = box.MinEdge.Z;
			minX = box.MinEdge.X;
			minY = box.MinEdge.Y;
			// 通过视图统计点云外包围盒xyz最大最小值 
			m_pView->StateAllBoxMin(minX, minY, minZ);
			// 当视图中有多个点云时，更新按色带循环渲染信息
			if (m_scant > 1)
			{

				int cyclaxis = 0;
				int cyclcur = 4;
				float cyclstep = 10.0;
				m_pView->getCyclInfo(cyclstep, cyclaxis, cyclcur);
				m_nAxis = cyclaxis;
				m_fCycleStep = cyclstep;

				if (cyclcur != m_colorRampCycle.GetCurCursel())
				{
					m_colorRampCycle.SetRampColor4f(cyclcur);
				}
			}
			// 默认按照Z轴去计算
			u32 beginColor = m_colorRampCycle.GetBeginColor();
			u32 endColor = m_colorRampCycle.GetEndColor();

			// 转换乘法参与计算
			float fSubCycleStep = 1.0f / m_fCycleStep;

#pragma omp parallel for
			for (u32 n = 0;n < loopCount;n++)// 遍历圈
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
#pragma omp parallel for
				for (u32 i = 0;i < pts.size();i++)// 遍历圈内每个点
				{
					// 为了求取过渡带的正确色彩
					// 标记过渡带，如步长为3，那么过渡带为高差为3.0、6.0等3的整数倍点
					bool flag = false;
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid() /*|| pt.isSelected()*/)
						continue;
					PointXYZIPRGBA pttmp = pt;
					//if (bTrans /*&& m_scant > 1*/)
					if (m_bTrans)
					{
						//absModel.Translate(pttmp.x, pttmp.y, pttmp.z);
						m_renderModel.Translate(pttmp.x, pttmp.y, pttmp.z);
					}

					RenderColor& color = *(colors._Myfirst + i);

					// 按Z方向高差
					if (m_nAxis == 0)
					{
						height = pttmp.z;

						// 求解余数
						tmpHeight =  fmodf(height - m_fMinHeight, m_fCycleStep);
						float tmp = (height - m_fMinHeight) * fSubCycleStep;// m_fCycleStep
						if ((tmp >= 1 && (int)(tmp+1) % 2 == 0) )
						{
							// 如果 高差为步长的整数倍，那么进行标记
							float tmphgt = height - m_fMinHeight;
							if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
							{
								flag = true;
							}
							tmpHeight = m_fCycleStep - tmpHeight ;

						}

					}
					// 按X方向高差
					else if (m_nAxis == 1)
					{
						height = pttmp.x;
						// 求解余数
						tmpHeight =  fmodf(height - m_fMinCorX,m_fCycleStep);
						float tmp = (height - m_fMinCorX) * fSubCycleStep;
						if (tmp >= 1 && (int)tmp % 2 == 0)
						{
							// 如果 高差为步长的整数倍，那么进行标记
							float tmphgt = height - m_fMinCorX;
							if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
							{
								flag = true;
							}
							tmpHeight = m_fCycleStep - tmpHeight ;
						}
					}
					// 按Y方向高差
					else if (m_nAxis == 2)
					{
						height = pttmp.y;
						// 求解余数
						tmpHeight =  fmodf(height - m_fMinCorY, m_fCycleStep);
						float tmp = (height - m_fMinCorY) * fSubCycleStep;
						if (tmp >= 1 && (int)tmp % 2 == 0)
						{
							// 如果 高差为步长的整数倍，那么进行标记
							float tmphgt = height - m_fMinCorY;
							if ((tmphgt - (int)tmp * m_fCycleStep) == 0)
							{
								flag = true;
							}
							tmpHeight = m_fCycleStep - tmpHeight;
						}
					}

					{
						// 根据余数求解在色带十个颜色中的哪一个颜色
						selStep = (int)(tmpHeight / (m_fCycleStep / 10.f));
						// 求解在该色带中的比例值
						tempValue = tmpHeight - selStep * (m_fCycleStep/10.f);
						scale = tempValue / (m_fCycleStep/10.f);
						// 如果为过渡带， 反减比例
						if (flag)
						{
							scale = 1 - scale;
						}
						m_colorRampCycle.GetColor4ub(scale, a, color.r, color.g, color.b, selStep+ 1);
					}
				}
			}
		}

		//! 计算按列显示颜色
		void CScanSceneNode::CalcuColRender()
		{
			u32 loopCount = m_pointCloud->getLoopCount();

			int col = 0;
			int step = 0;
			int steptemp;
			int stepCol = m_nMaxCol - m_nMinCol;
			float scaleTemp = 0.0f;
			float scale = 0.0f;
			u8 a /*,r ,g ,b*/;

#pragma omp parallel for
			for (u32 n = 0;n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
#pragma omp parallel for
				for (u32 i = 0;i < pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid()/* || pt.isSelected()*/)
						continue;

					RenderColor& color = *(colors._Myfirst + i);

					col = n;
					if ( n >= m_nMinCol && n <= m_nMaxCol )
					{
						scaleTemp = (float)(( col - m_nMinCol ) * 1.0f / (stepCol));
						steptemp = (int)(scaleTemp * 10.0f);
						scale = (scaleTemp - 0.1f * steptemp) * 10;
						step = steptemp + 1;


						if (step > 9)
						{
							int i = 0;
							i++;
						}
						m_colorRampByCol.GetColor4ub(scale,a,color.r,color.g,color.b,step);
					}
					else
					{
						color.r = 255 / 2;
						color.g = 255 / 2;
						color.b = 255 / 2;
					}	
				}
			}
		}

		//! 计算按距离显示颜色(修改为使用m_colorRampZ颜色条渲染)
		void CScanSceneNode::CalcuDistRender()
		{
			int selStep = 0;
			u8 a = 0;
			float dist = 0.0f;
			float tempValue = 0.0f;
			float scale = 0.0f;
			float r = 0,g = 0,b = 0;

			u32 loopCount = m_pointCloud->getLoopCount();
			float minDist = m_fMinDist;
			float maxDist = m_fMaxDist;

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			//// 设置距离深度图的最大最小值范围
			//m_distImg
#pragma omp parallel for
			for (u32 n = 0;n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
#pragma omp parallel for
				for (u32 i = 0;i < pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid())
						continue;

					RenderColor& color = *(colors._Myfirst + i);

					dist = pt.x * pt.x + pt.y * pt.y + pt.z * pt.z;
					//r = 0;g = 0;b = 0;
					//m_distImg.GetRGB3FByDist(dist,r,g,b);

					//color.r = u8(r * 255);
					//color.g = u8(g * 255);
					//color.b = u8(b * 255);

					if (dist < minDist)
					{
						color.r = ((beginColor>>16) & 0xff);
						color.g = ((beginColor>>8) & 0xff);
						color.b = (beginColor & 0xff);
					}
					else if (dist > maxDist)
					{
						color.r = ((endColor>>16) & 0xff);
						color.g = ((endColor>>8) & 0xff);
						color.b = (endColor & 0xff);
					}
					else
					{
						selStep = (int)((dist - minDist) * 10 /(maxDist - minDist));

						tempValue = dist - minDist - selStep * (maxDist - minDist) / 10.0f;
						scale = tempValue * 10/(maxDist - minDist);
						m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
					}
				}
			}
		}


		ENUM_RENDERSTYLE CScanSceneNode::GetRenderStyle() const
		{
			return m_renderStyle;
		}

		void CScanSceneNode::OnRegisterSceneNode()
		{
			if (IsVisible)
			{
				SceneManager->registerNodeForRendering(this);
			}
			ISceneNode::OnRegisterSceneNode();
		}

		void CScanSceneNode::render()
		{
			// 正在设置点云过程中，不能渲染
			if (g_bSettingPcd)
			{
				return;
			}

            // 自定义颜色
            COLORREF colorUser = RGB(0, 299, 240);

			// 锁定代码块
			EnterCriticalSection(&m_cs);
			if(m_pointCloud == NULL || !(m_pointCloud->count() > 0))
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			if(m_pView == NULL)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			if (m_pView->IsViewRenderAllNode() != m_bIsRender)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			// 编辑模式下判断范围相关变量
			int editMode = m_pointCloud->getEditMode();

			// 防止除零情况
			if (m_simpleCount == 0)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

            //if (GetPointCloud()->GetPointCloudPath() == "E:\\Test\\CZ\\Scans\\CZ1\\CZ1.hls")
            //{
            //    char strTest[124];
            //    sprintf(strTest, "%s: %f, %f, %f\n", "平移之前的坐标", getPosition().X, getPosition().Y, getPosition().Z);
            //    OutputDebugString(strTest);
            //}

			u32 renderSimpleCol = (u32)m_countInView / m_simpleCount;
			if(renderSimpleCol == 0 || editMode == 0)
				renderSimpleCol = 1;
			u32 renderSimpleRow = renderSimpleCol;

			// 设置材质和渲染状态
			int simpleCount = 0;
			simpleCount = m_simpleCount;

			core::matrix4 mat(m_bUseMat?m_IrrAbsMat:AbsoluteTransformation);
			video::IVideoDriver* driver = SceneManager->getVideoDriver();
			if (!driver)
			{
				LeaveCriticalSection(&m_cs);
				return;
			}

			driver->setTransform(video::ETS_WORLD,mat);
			f32 lastSize = m_material.Thickness;
			// 在视图中设置抽稀显示状态下 才抽稀显示 [2013/12/9 危迟]
			if (SceneManager->IsAnimating() && m_pView->IsPointCloudSimpleRender())
			{
				// 动态浏览抽稀1/4显示
#if _DEBUG
				renderSimpleRow *= 4;
#else
				renderSimpleRow = 3;
#endif
			}
			else
			{
				renderSimpleRow = 1;
			}

			// 设置渲染材质
			driver->setMaterial(m_material);

			// 设置渲染状态为3D模式
			driver->setRenderStates3DMode();

			// 蔡红云 2013/11/7 场景中的点云节点是否显示包围盒由视图中盒子显示状态判定
			m_renderBBox = m_pView->GetboxShowState();


			if (m_renderBBox /*&& (m_renderStyle != RENDER_BY_DEFAULT ||
							 (m_renderStyle == RENDER_BY_DEFAULT && m_pView->GetViewType() == E_HVT_REG))*/) 
			{
				core::aabbox3d<f32> box = m_box;
				core::aabbox3d<f32> box1;

				box1.MaxEdge.set(F32_MIN,F32_MIN,F32_MIN);
				box1.MinEdge.set(F32_MAX,F32_MAX,F32_MAX);

				if (m_bTrans)
				{
					// 旋转矩阵存在时，外包围盒显示不正确,更新八个顶点重新生成外包围盒[2014/9/24] 蔡红云 
					core::vector3df edges[8];
					box.getEdges(edges);
					for (int i =0 ; i!=8;i++)
					{
						m_renderModel.Translate(edges[i].X,edges[i].Y,edges[i].Z);
						box1.addInternalPoint(edges[i]);
					}
				} 

				driver->draw3DBox(m_bTrans?box1:box, m_bSelected?(video::SColor(255,GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor))):(video::SColor(255,255,255,255)));
			}

            //if (GetPointCloud()->GetPointCloudPath() == "E:\\Test\\CZ\\Scans\\CZ1\\CZ1.hls")
            //{
            //    char strTest[124];
            //    sprintf(strTest, "%s: %f, %f, %f\n", "平移之后的坐标", getPosition().X, getPosition().Y, getPosition().Z);
            //    OutputDebugString(strTest);
            //}

			glEnable(GL_BLEND);
			
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glRasterPos3f(10,10,0);

			glBegin(GL_POINTS);

			//设置随机数种子
			std::srand(m_seed);

			u32 i = 0;
			//m_simple * 2 - 1用于随机数求余，使得随机数平均数大概为m_simple;
			//u32 simple = m_simple * 2 - 1;
			u32 simpleIdx = 1;
			float boundingBoxHeight = fabs(m_box.MaxEdge.Z - m_box.MinEdge.Z);

			hdVector<PointXYZIPRGBA> tmpPts;
			hdVector<RenderColor> tmpColors;
			if (m_renderStyle == RENDER_BY_FEATURE_BIN)
			{
				u32 loopCount = m_pointCloud->getLoopCount();

#pragma region 按照特征类别渲染
				switch (m_nFeatureTypeIndex)
				{
				case 0:
					{
						// 计算实际判断的强度阈值
						float scale = ((float)m_nFeatureThreshold)/255.0f;
						int minInten = m_pointCloud->getMinIntensity();
						int maxInten = m_pointCloud->getMaxIntensity();
						m_dFeatureThreshold = scale*((float)(maxInten - minInten)) + (float)minInten;

						for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
						{
							// 编辑模式下,静态点云需要判断当前圈是否在视图内
							if (editMode == 1 && (m_vecInView.at(n)) == 0)
							{
								continue;
							}
							hdVector<PointXYZIPRGBA>* pPts = NULL;
							hdVector<RenderColor>* pColors = NULL;
							if (editMode == 1)
							{
								hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
								hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
								pPts = &pts;
								pColors = &colors;
							}
							else
							{
								// 此处用拷贝赋值的方法,而非引用,避免多线程访问冲突
								tmpPts = m_pointCloud->getLoop(n);
								tmpColors = *(m_renderColors._Myfirst + n);
								pPts = &tmpPts;
								pColors = &tmpColors;
							}

							if(pColors->size() != pPts->size())
							{
								continue;
							}
							for (i = 0;i < pPts->size();i+=renderSimpleRow)
							{
								const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
								if(!pt.isValid())
									continue;
								RenderColor& color = *(pColors->_Myfirst + i);
								RenderPointByIntensityFeature(pt,boundingBoxHeight,color);
							}
						}
					}
					break;
				case 1:
					{
						// 计算实际判断的高度阈值
						float scale = ((float)m_nFeatureThreshold)/255.0f;
						//float minZ = m_fMinHeight;
						//float maxZ = m_fMaxHeight;
						float minZ = m_pointCloud->m_header.min_z;
						float maxZ = m_pointCloud->m_header.max_z;
						m_dFeatureThreshold = scale*(maxZ - minZ) + minZ;

						for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
						{
							// 编辑模式下,静态点云需要判断当前圈是否在视图内
							if (editMode == 1 && (m_vecInView.at(n)) == 0)
							{
								continue;
							}
							hdVector<PointXYZIPRGBA>* pPts = NULL;
							hdVector<RenderColor>* pColors = NULL;
							if (editMode == 1)
							{
								hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
								hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
								pPts = &pts;
								pColors = &colors;
							}
							else
							{
								// 此处用拷贝赋值的方法,而非引用,避免多线程访问冲突
								tmpPts = m_pointCloud->getLoop(n);
								tmpColors = *(m_renderColors._Myfirst + n);
								pPts = &tmpPts;
								pColors = &tmpColors;
							}

							if(pColors->size() != pPts->size())
							{
								continue;
							}
							for (i = 0;i < pPts->size();i+=renderSimpleRow)
							{
								const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
								if(!pt.isValid())
									continue;
								RenderColor& color = *(pColors->_Myfirst + i);
								RenderPointByHeightFeature(pt,boundingBoxHeight,color);
							}
						}
					}
					break;
				case 2:
					{
						hdBlkArray<PointXYZIPRGBA>& pts = m_pointCloud->getPoints();
						//计算实际判断的投影阈值
						m_dFeatureThreshold = (float)m_nFeatureThreshold;
						for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
						{
							// 编辑模式下,静态点云需要判断当前圈是否在视图内
							if (editMode == 1 && (m_vecInView.at(n)) == 0)
							{
								continue;
							}
							hdVector<PointXYZIPRGBA>* pPts = NULL;
							hdVector<RenderColor>* pColors = NULL;
							if (editMode == 1)
							{
								hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
								hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
								pPts = &pts;
								pColors = &colors;
							}
							else
							{
								// 此处用拷贝赋值的方法,而非引用,避免多线程访问冲突
								tmpPts = m_pointCloud->getLoop(n);
								tmpColors = *(m_renderColors._Myfirst + n);
								pPts = &tmpPts;
								pColors = &tmpColors;
							}

							if(pColors->size() != pPts->size())
							{
								continue;
							}
							int* cs = m_pointCloud->getBlockAttr<int>(n);

							for (i = 0;i < pPts->size();i+=renderSimpleRow)
							{
								const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
								if(!pt.isValid())
									continue;
								int feature = cs[i];
								RenderColor& color = *(pColors->_Myfirst + i);
								RenderPointByProjFeature(pt,feature,boundingBoxHeight,color);
							}
						}
					}
					break;
				case 3:
					{
						hdBlkArray<PointXYZIPRGBA>& pts = m_pointCloud->getPoints();
						//计算实际判断的角度阈值
						m_dFeatureThreshold = (float)m_nFeatureThreshold;
						for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
						{
							// 编辑模式下,静态点云需要判断当前圈是否在视图内
							if (editMode == 1 && (m_vecInView.at(n)) == 0)
							{
								continue;
							}
							hdVector<PointXYZIPRGBA>* pPts = NULL;
							hdVector<RenderColor>* pColors = NULL;
							if (editMode == 1)
							{
								hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
								hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
								pPts = &pts;
								pColors = &colors;
							}
							else
							{
								// 此处用拷贝赋值的方法,而非引用,避免多线程访问冲突
								tmpPts = m_pointCloud->getLoop(n);
								tmpColors = *(m_renderColors._Myfirst + n);
								pPts = &tmpPts;
								pColors = &tmpColors;
							}

							if(pColors->size() != pPts->size())
							{
								continue;
							}
							float* cs = m_pointCloud->getBlockAttr<float>(n);

							for (i = 0;i < pPts->size();i+=renderSimpleRow)
							{
								const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
								if(!pt.isValid())
									continue;
								float feature = cs[i];
								RenderColor& color = *(pColors->_Myfirst + i);
								RenderPointByAngleFeature(pt,feature,boundingBoxHeight,color);
							}
						}
					}
					break;
				}//switch (m_nFeatureTypeIndex)
#pragma endregion
			}
			else	// 按坐标、强度渲染、默认、扫描列渲染
			{
				m_selPtColor = CHdSysSetting::getSysSetting()->measureSetting.selPointColor;
				u32 loopCount = m_pointCloud->getLoopCount();
				float fSupTmp = 1.0f / 255.0f;

#pragma omp parallel for
				for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
				{
					// 编辑模式下,静态点云需要判断当前圈是否在视图内
					if (editMode == 1 && (m_vecInView.at(n)) == 0)
					{
						continue;
					}
					// 增加浏览模式时，判断不在视锥范围内当前圈不计算渲染
					else if (editMode == 0 && (m_vecInView.at(n)) == 0)
					{
						continue;
					}

					hdVector<PointXYZIPRGBA>* pPts = NULL;
					hdVector<RenderColor>* pColors = NULL;
					if (editMode == 1)
					{
						hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
						hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
						pPts = &pts;
						pColors = &colors;
					}
					else
					{
						// 此处用拷贝赋值的方法,而非引用,避免多线程访问冲突
						tmpPts = m_pointCloud->getLoop(n);
						tmpColors = *(m_renderColors._Myfirst + n);
						pPts = &tmpPts;
						pColors = &tmpColors;
					}

					unsigned int nPts = pPts->size();
					unsigned int nColors = pColors->size();

					if(pColors->size() != pPts->size())
					{
						continue;
					}

#pragma omp parallel for
					for (i = 0;i < pPts->size();i+=renderSimpleRow)
					{
						const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
						if(!pt.isValid())
							continue;

						RenderColor& color = *(pColors->_Myfirst + i);

#pragma region 根据renderstyle渲染
						if ((m_showStyle == SHOW_SELECT && !pt.isSelected()) ||
							(m_showStyle == SHOW_UNSELECT && pt.isSelected()) ||
							(m_pView->GetbRenderSetting() && !(m_pView->GetIsRenderClass(pt.prop))))// 渲染设置中，分类类别为false的情况下不渲染
						{
							continue;
						}

						// 计算透明度的实际值 fengjing
						u8 colora = (u8)(((float)color.a * fSupTmp) * m_ColoraSetting);

						// 如果仅仅显示选择点云,则不需要高亮显示
						// 选择的点云按区域渲染，没有被选中的点云按全部渲染，仅显示（不）选择点云时按选中状态进行渲染--zhubo.2013.12.19
						//if (/*m_showStyle != SHOW_SELECT && */pt.isSelected())//选中,pt.isSelected()
						//{
						//	// 选中点就用区域渲染获取的颜色进行绘制、非选中点就用本身计算所得颜色进行绘制[蔡红云 2014/7/24 蔡红云]
						//	switch (m_renderAreaStyle)
						//	{
						//	case RENDER_AREA_BY_INTENSITY:
						//		glColor4ub(colora,colora,colora,/*m_bShowIntenRender?color.a:*/255);
						//		break;
						//	case RENDER_AREA_BY_Z:
						//	case RENDER_AREA_BY_Y:
						//	case RENDER_AREA_BY_X:
						//		glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);
						//		break;
						//	case RENDER_AREA_BY_DEFAULT:
						//		glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?colora:255);
						//		break;
						//	case RENDER_AREA_BY_CLASSIFY:
						//		{
						//			// 分类属性标记
						//			if (m_pView->GetIsRenderClass(pt.prop))
						//			{
						//				int nId = pt.prop;
						//				if (0 == nId)
						//				{
						//					glColor4ub(GetRValue(m_sClassColor[1]),
						//						GetGValue(m_sClassColor[1]),
						//						GetBValue(m_sClassColor[1]),m_bShowIntenRender?colora:255);
						//				}
						//				else
						//				{
						//					glColor4ub(GetRValue(m_sClassColor[nId]),
						//						GetGValue(m_sClassColor[nId]),
						//						GetBValue(m_sClassColor[nId]),m_bShowIntenRender?colora:255);
						//				}
						//			}
						//			break;
						//		}
						//	}
						//}
					/*	else
						{*/
							switch (m_renderStyle)
							{
							case RENDER_BY_X:
							case RENDER_BY_Y:
							case RENDER_BY_Z:
							case RENDER_BY_COL:
							case RENDER_BY_DISTANCE:
							case RENDER_BY_CYCLERAMP:
								glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);

								if (pt.isSelected()) // 判断点是否被选中
								{

									if (m_showStyle == SHOW_SELECT)
									{
										glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);
									}
									else
									{
										glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
									}


								}
								else
								{
									glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);
								}
								
								break;
							case RENDER_BY_DEFAULT:
								// 选中的点云高亮显示，与选择工具选中的颜色一致
								if (GetSelected() && (m_pView->GetViewType() == E_HVT_MULTISCAN3D || m_pView->GetViewType() == E_HVT_SKETCH_ISCAN3D))
								{
									glColor4ub(GetRValue(m_selPtColor), GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?colora:180);
								}
								else
								{
									if (pt.isSelected()) // 判断点是否被选中
									{

										if (m_showStyle == SHOW_SELECT && m_bIsRender)
										{
											glColor4ub(m_defaultClr.r, m_defaultClr.g, m_defaultClr.b,m_bShowIntenRender?colora:180);
										}
                                        else if (m_showStyle == SHOW_SELECT)
                                        {
                                            glColor4ub(GetRValue(colorUser), GetGValue(colorUser), GetBValue(colorUser), m_bShowIntenRender ? colora : 255);
                                        }
										else
										{
											glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
										}


									}
									else
									{
										glColor4ub(m_defaultClr.r, m_defaultClr.g, m_defaultClr.b,m_bShowIntenRender?colora:255);
									}
																		
								}
								break;
							case RENDER_BY_RGB:
								{
									if (pt.isSelected()) // 判断点是否被选中
									{

										if (m_showStyle == SHOW_SELECT)
										{
											glColor4ub(pt.r , pt.g , pt.b,255);
										}
										else
										{
											glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
										}


									}
									else
									{
										glColor4ub(pt.r , pt.g , pt.b,255);
									}
								}
								break;
							case RENDER_BY_CLASS:
								if (pt.isSelected()) // 判断点是否被选中
								{

									if (m_showStyle == SHOW_SELECT)
									{
										if (m_pView->GetIsRenderClass(pt.prop))
										{
											int nId = pt.prop;
											if (0 == nId)
											{
												glColor4ub(GetRValue(m_sClassColor[1]),
													GetGValue(m_sClassColor[1]),
													GetBValue(m_sClassColor[1]),m_bShowIntenRender?colora:255);
											}
											else
											{
												glColor4ub(GetRValue(m_sClassColor[nId]),
													GetGValue(m_sClassColor[nId]),
													GetBValue(m_sClassColor[nId]),m_bShowIntenRender?colora:255);
											}
										}
									}
									else
									{
										glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
									}

								}
								else
								{
									if (m_pView->GetIsRenderClass(pt.prop))
									{
										int nId = pt.prop;
										if (0 == nId)
										{
											glColor4ub(GetRValue(m_sClassColor[1]),
												GetGValue(m_sClassColor[1]),
												GetBValue(m_sClassColor[1]),m_bShowIntenRender?colora:255);
										}
										else
										{
											glColor4ub(GetRValue(m_sClassColor[nId]),
												GetGValue(m_sClassColor[nId]),
												GetBValue(m_sClassColor[nId]),m_bShowIntenRender?colora:255);
										}
									}
								}
								break;
							case RENDER_BY_INTENSITY:
								if (pt.isSelected()) // 判断点是否被选中
								{
									if (m_showStyle == SHOW_SELECT)
									{
										glColor4ub(colora,colora,colora,255);
									}
									else
									{
										glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?color.a:255);
									}


								}
								else
								{
									glColor4ub(colora,colora,colora,255);
								}

								break;
							}
						/*}*/

						//glPointSize(10.0f);
						if (m_bTrans)
						{
							// 先转换绝对坐标,再用视图坐标转换显示坐标 
							double x = pt.x;
							double y = pt.y;
							double z = pt.z;

							//m_absModel.Translate(x,y,z);
							m_renderModel.Translate(x,y,z);
							glVertex3d(x,y,z);
						} 
						else
						{
							glVertex3f(pt.x,pt.y,pt.z);
						}
						//RenderPoint(pt,boundingBoxHeight,color,n);
#pragma endregion
					}
				}
			}

			//绘制测站中心
			if (m_pView->GetViewType() == E_HVT_MULTISCAN3D)
			{
				double x = 0.0;
				double y = 0.0;
				double z = 0.0;
				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);

				glPointSize(5.0f);
				glColor3ub(255,0,0);
				glVertex3d(x,y,z);
			}

            //// 绘制草图测站中心
            //if (m_pView->GetViewType() == E_HVT_SKETCH_ISCAN3D)
            //{
            //    // 草图测站中心三角形和测站名
            //}

			glEnd();
			//glFlush();
			glDisable(GL_BLEND);
			m_material.Thickness = lastSize;
			if (m_RenderCount == 0)
			{
				m_RenderCount++;
			}

			::LeaveCriticalSection(&m_cs);
		}

		void CScanSceneNode::RenderPointByIntensityFeature(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color)
		{
			if ((m_showStyle == SHOW_SELECT && !pt.isSelected())
				||(m_showStyle == SHOW_UNSELECT && pt.isSelected()))
			{
				return;
			}

			//测试用强度做特征
			if (pt.getIntensity() > m_dFeatureThreshold )
			{
				glColor4ub(GetRValue(classification[0]),
					GetGValue(classification[0]),
					GetBValue(classification[0]),m_bShowIntenRender?color.a:255);
			}
			else
			{
				glColor4ub(GetRValue(classification[29]),
					GetGValue(classification[29]),
					GetBValue(classification[29]),m_bShowIntenRender ? color.a:255);
			}

			if (m_bTrans)
			{
				// 先转换绝对坐标,再用视图坐标转换显示坐标
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;

				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);

				glVertex3d(x,y,z);
			} 
			else
			{
				glVertex3d(pt.x,pt.y,pt.z);
			}
		}

		void CScanSceneNode::RenderPointByHeightFeature(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color)
		{
			if ((m_showStyle == SHOW_SELECT && !pt.isSelected())
				||(m_showStyle == SHOW_UNSELECT && pt.isSelected()))
			{
				return;
			}

			if (pt.z < m_dFeatureThreshold )
			{
				glColor4ub(GetRValue(classification[0]),
					GetGValue(classification[0]),
					GetBValue(classification[0]),m_bShowIntenRender?color.a:255);
			}
			else
			{
				glColor4ub(GetRValue(classification[29]),
					GetGValue(classification[29]),
					GetBValue(classification[29]),m_bShowIntenRender ? color.a:255);
			}

			if (m_bTrans)
			{
				// 先转换绝对坐标,再用视图坐标转换显示坐标
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;

				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);

				glVertex3d(x,y,z);
			} 
			else
			{
				glVertex3d(pt.x,pt.y,pt.z);
			}
		}

		void CScanSceneNode::RenderPointByProjFeature(const PointXYZIPRGBA& pt,int feature, float boundingBoxHeight,const RenderColor& color)
		{
			if ((m_showStyle == SHOW_SELECT && !pt.isSelected())
				||(m_showStyle == SHOW_UNSELECT && pt.isSelected()))
			{
				return;
			}

			if (feature < m_dFeatureThreshold )
			{
				glColor4ub(GetRValue(classification[0]),
					GetGValue(classification[0]),
					GetBValue(classification[0]),m_bShowIntenRender?color.a:255);
			}
			else
			{
				glColor4ub(GetRValue(classification[29]),
					GetGValue(classification[29]),
					GetBValue(classification[29]),m_bShowIntenRender?color.a:255);
			}

			if (m_bTrans)
			{
				// 先转换绝对坐标,再用视图坐标转换显示坐标
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;

				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);

				glVertex3d(x,y,z);
			} 
			else
			{
				glVertex3d(pt.x,pt.y,pt.z);
			}
		}

		void CScanSceneNode::RenderPointByAngleFeature(const PointXYZIPRGBA& pt,float feature,float boundingBoxHeight,const RenderColor& color)
		{
			if ((m_showStyle == SHOW_SELECT && !pt.isSelected())
				||(m_showStyle == SHOW_UNSELECT && pt.isSelected()))
			{
				return;
			}

			if (feature > m_dFeatureThreshold )
			{
				glColor4ub(GetRValue(classification[0]),
					GetGValue(classification[0]),
					GetBValue(classification[0]),m_bShowIntenRender?color.a:255);
			}
			else
			{
				glColor4ub(GetRValue(classification[29]),
					GetGValue(classification[29]),
					GetBValue(classification[29]),m_bShowIntenRender?color.a:255);
			}

			if (m_bTrans)
			{
				// 先转换绝对坐标,再用视图坐标转换显示坐标
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;

				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);

				glVertex3d(x,y,z);
			} 
			else
			{
				glVertex3d(pt.x,pt.y,pt.z);
			}
		}

		void CScanSceneNode::RenderPoint(const PointXYZIPRGBA& pt,float boundingBoxHeight,const RenderColor& color,int selPoint)
		{
			if ((m_showStyle == SHOW_SELECT && !pt.isSelected()) ||
				(m_showStyle == SHOW_UNSELECT && pt.isSelected()) ||
				(m_pView->GetbRenderSetting() && !(m_pView->GetIsRenderClass(pt.prop))))// 渲染设置中，分类类别为false的情况下不渲染
			{
				return;
			}

			// 计算透明度的实际值 fengjing
			u8 colora = (u8)(((float)color.a / 255) * m_ColoraSetting);

			// 如果仅仅显示选择点云,则不需要高亮显示
			// 选择的点云按区域渲染，没有被选中的点云按全部渲染，仅显示（不）选择点云时按选中状态进行渲染--zhubo.2013.12.19
			if (/*m_showStyle != SHOW_SELECT && */pt.isSelected())//选中,pt.isSelected()
			{
				// 选中点就用区域渲染获取的颜色进行绘制、非选中点就用本身计算所得颜色进行绘制[蔡红云 2014/7/24 蔡红云]
				switch (m_renderAreaStyle)
				{
				case RENDER_AREA_BY_INTENSITY:
					glColor4ub(colora,colora,colora,/*m_bShowIntenRender?color.a:*/255);
					break;
				case RENDER_AREA_BY_Z:
				case RENDER_AREA_BY_Y:
				case RENDER_AREA_BY_X:
					glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);
					break;
				case RENDER_AREA_BY_DEFAULT:
					glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?colora:255);
					break;
				case RENDER_AREA_BY_CLASSIFY:
					{
						// 分类属性标记
						if (m_pView->GetIsRenderClass(pt.prop))
						{
							int nId = pt.prop;
							if (0 == nId)
							{
								glColor4ub(GetRValue(m_sClassColor[1]),
									GetGValue(m_sClassColor[1]),
									GetBValue(m_sClassColor[1]),m_bShowIntenRender?colora:255);
							}
							else
							{
								glColor4ub(GetRValue(m_sClassColor[nId]),
									GetGValue(m_sClassColor[nId]),
									GetBValue(m_sClassColor[nId]),m_bShowIntenRender?colora:255);
							}
						}

						break;
					}
				}
			}
			else
			{
				switch (m_renderStyle)
				{
				case RENDER_BY_X:
				case RENDER_BY_Y:
				case RENDER_BY_Z:
				case RENDER_BY_COL:
				case RENDER_BY_DISTANCE:
				case RENDER_BY_CYCLERAMP:
					glColor4ub(color.r,color.g,color.b,m_bShowIntenRender?colora:255);

					break;
				case RENDER_BY_DEFAULT:
					// 选中的点云高亮显示，与选择工具选中的颜色一致
					if (GetSelected() && (m_pView->GetViewType() == E_HVT_MULTISCAN3D || m_pView->GetViewType() == E_HVT_SKETCH_ISCAN3D))
					{
						glColor4ub(GetRValue(m_selPtColor),GetGValue(m_selPtColor),GetBValue(m_selPtColor),m_bShowIntenRender?colora:255);
					}
					else
					{
						glColor4ub(m_defaultClr.r, m_defaultClr.g, m_defaultClr.b,m_bShowIntenRender?colora:255);
					}

					break;
				case RENDER_BY_RGB:
					glColor4ub(pt.r , pt.g , pt.b,255);

					break;
				case RENDER_BY_CLASS:
					if (m_pView->GetIsRenderClass(pt.prop))
					{
						int nId = pt.prop;
						if (0 == nId)
						{
							glColor4ub(GetRValue(m_sClassColor[1]),
								GetGValue(m_sClassColor[1]),
								GetBValue(m_sClassColor[1]),m_bShowIntenRender?colora:255);
						}
						else
						{
							glColor4ub(GetRValue(m_sClassColor[nId]),
								GetGValue(m_sClassColor[nId]),
								GetBValue(m_sClassColor[nId]),m_bShowIntenRender?colora:255);
						}
					}

					break;
				case RENDER_BY_INTENSITY:
					glColor4ub(colora,colora,colora,/*m_bShowIntenRender?color.a:*/255);

					break;
				}
			}

			//glPointSize(10.0f);
			if (m_bTrans)
			{
				// 先转换绝对坐标,再用视图坐标转换显示坐标 
				double x = pt.x;
				double y = pt.y;
				double z = pt.z;

				//m_absModel.Translate(x,y,z);
				m_renderModel.Translate(x,y,z);
				glVertex3d(x,y,z);
			} 
			else
			{
				glVertex3f(pt.x,pt.y,pt.z);
			}
		}

		const core::aabbox3d<f32>& CScanSceneNode::getBoundingBox() const
		{
			return m_box;
		}

		irr::u32 CScanSceneNode::GetMaterialCount() const
		{
			return 1;
		}

		video::SMaterial& CScanSceneNode::GetMaterial( u32 i )
		{
			return m_material;
		}	

		void CScanSceneNode::SetPointSize( f32 size )
		{
			m_material.Thickness = size;
		}

		irr::f32 CScanSceneNode::GetPointSize() const
		{
			return m_material.Thickness;
		}

		irr::f32 CScanSceneNode::GetTransparence() const
		{
			return m_transparence;
		}
		irr::u32 CScanSceneNode::GetPointCount() const
		{
			if (m_pointCloud)
			{
				return (u32)(m_pointCloud->count() - m_pointCloud->getDelPcdNum());
			}
			else
				return 0;
		}

		void CScanSceneNode::SetSimple( u32 simple )
		{
			if (simple > 0 && simple < 15)
			{
				m_simple = simple;
			}
		}

		irr::u32 CScanSceneNode::GetSimpleCount() const
		{
			return m_simpleCount;
		}

		//! 设置最大最小反射强度
		void CScanSceneNode::SetMaxMinIntensity(int nMaxIntensity, int nMinIntensity)
		{
			if (nMaxIntensity > nMinIntensity)
			{
				m_MaxIntensity = nMaxIntensity;
				m_MinIntensity = nMinIntensity;
			}
		}

		//! 得到最大最小反射强度
		void CScanSceneNode::GetMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity)
		{
			nMaxIntensity = m_MaxIntensity;
			nMinIntensity = m_MinIntensity;
		}

		void CScanSceneNode::GetRealMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity)
		{
			if (m_pointCloud)
			{
				nMaxIntensity = m_pointCloud->getMaxIntensity();
				nMinIntensity = m_pointCloud->getMinIntensity();
			}
		}

		void CScanSceneNode::GetRenderMaxMinIntensity(int& nMaxIntensity, int& nMinIntensity)
		{
			nMaxIntensity = m_nMaxIntensity;
			nMinIntensity = m_nMinIntensity;
		}

		irr::u32 CScanSceneNode::GetSimple() const
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

		void CScanSceneNode::SetShowStyle(ENUM_SHOWSTYLE eStyle )
		{
			m_showStyle = eStyle;
		}

		hd::ENUM_SHOWSTYLE CScanSceneNode::GetShowStyle() const
		{
			return m_showStyle;
		}

		void CScanSceneNode::SetColorIndex(unsigned int colorArr[],int arrLength)
		{
			if (arrLength <= 0 || arrLength > 256)
			{
				return;
			}
			for (int i = 0; i< arrLength;i++)
			{
				m_nColorIndex[i] = colorArr[i];
			}
		}

		//设置类别颜色 fengjing
		void CScanSceneNode::SetClassColorValue(COLORREF ClassColor[], int arrlength)
		{
			if (arrlength <= 0 || arrlength > 256)
			{
				return;
			}
			for (int i = 0; i< arrlength;i++)
			{
				m_sClassColor[i] = ClassColor[i];
			}
		}

		void CScanSceneNode::ReStatIntensity()
		{
			ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);

			if (!pView)
			{
				return;
			}

			u64 count = m_pointCloud->count();
			U32 validCount = m_pointCloud->getValidCount();
			if (m_pointCloud == NULL || count <= 0 || validCount <= 0)
			{
				return;
			}

			int mapIntensity[1000] = {0};
			int maxIntensity = m_pointCloud->getMaxIntensity();
			int minIntensity = m_pointCloud->getMinIntensity();
			int maxNum = 0;
			int selMax = 0;
			int minNum = 0;
			int selMin = 0;
			int selNum = 0;
			float floorRatio = 0.15f;
			float ceilRatio = 0.05f;

			pView->GetIntensityStrethThreshold(floorRatio,ceilRatio);

			f32 intensityRcp = 1000.0f / (maxIntensity - minIntensity);
			u32 loopCount = m_pointCloud->getLoopCount();

			// 统计强度分布情况
			for (u32 n = 0; n < loopCount;n++)
			{
				const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);

				for (u32 i = 0; i< pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

					if (!pt.isValid())
					{
						continue;
					}
					selNum = (int)((pt.getIntensity() - minIntensity) * intensityRcp);
					selNum = clamp(selNum,0,999);
					mapIntensity[selNum]++;
				}
			}
			//对于MDL扫描头，计算强度大的%1位置
			//对于Faro扫描头，计算强度大5%位置
			int i = 0,j = 0;
			for (j = 999;j >= 0;j--)
			{
				maxNum += mapIntensity[j];
				if (maxNum >= (int)(count * ceilRatio))
				{
					selMax = j;
					break;
				}
			}

			for (i = 0;i < 1000;i++)
			{
				minNum += mapIntensity[i];
				if (minNum >= (int)(count * floorRatio))
				{
					selMin = i;
					break;
				}
			}

			// 去除强度大的和强度小之后的强度范围
			m_nMinIntensity = (int)(minIntensity + (maxIntensity - minIntensity) * 1.0f * selMin / 1000.0f);
			m_nMaxIntensity = (int)(minIntensity + (maxIntensity - minIntensity) * 1.0f * selMax / 1000.0f);

			CalcuIntensityRender();
		}

		void CScanSceneNode::StatIntensity()
		{
			u64 count = m_pointCloud->count();
			U32 validCount = m_pointCloud->getValidCount();
			if (m_pointCloud == NULL || count <= 0 || validCount <= 0)
			{
				return;
			}

			// 视图
			ISceneView* pView = dynamic_cast<ISceneView*>(m_pView);
			if (!pView)
			{
				return;
			}

			int mapIntensity[1000] = {0};
			int maxIntensity = m_pointCloud->getMaxIntensity();
			int minIntensity = m_pointCloud->getMinIntensity();
			int maxNum = 0;
			int selMax = 0;
			int minNum = 0;
			int selMin = 0;
			int selNum = 0;
			//float floorRatio = 0.01;
			//float ceilRatio = 0.01;

			float floorRatio = 0.15f;
			float ceilRatio = 0.05f;

			// 通过点云头文件中存储的扫描仪型号来判断 对于iScan-S-Z建议采用如下拉伸值
			if (strcmp(m_pointCloud->m_header.system_identifier,"ISCAN-S-Z") == 0)
			{	
				floorRatio = 0.001f;
				ceilRatio = 0.15f;
			}
			else if (strcmp(m_pointCloud->m_header.system_identifier,"HD 3LS ZFS") == 0) // Z+F5010得到的扫描数据采用如下拉伸值
			{
				floorRatio = 0.01f;
				ceilRatio = 0.25f;
			}
			else
			{
				pView->GetIntensityStrethThreshold(floorRatio,ceilRatio);
			}

			f32 intensityRcp = 1000.0f / (maxIntensity - minIntensity);
			u32 loopCount = m_pointCloud->getLoopCount();
			// 统计强度分布情况
//#pragma omp parallel for
			for (u32 n = 0; n < loopCount;n++)
			{
				const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
//#pragma omp parallel for
				for (u32 i = 0; i< pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

					if (!pt.isValid())
					{
						continue;
					}
					selNum = (int)((pt.getIntensity() - minIntensity) * intensityRcp);
					selNum = clamp(selNum,0,999);
					mapIntensity[selNum]++;
				}
			}
			//对于MDL扫描头，计算强度大的%1位置
			//对于Faro扫描头，计算强度大5%位置
			int i = 0,j = 0;
			for (j = 999;j >= 0;j--)
			{
				maxNum += mapIntensity[j];
				if (maxNum >= (int)(count * ceilRatio))
				{
					selMax = j;
					break;
				}
			}
			//对于MDL扫描头，计算强度大的%1位置
			//对于Faro扫描头，计算强度大15%位置
			for (i = 0;i < 1000;i++)
			{
				minNum += mapIntensity[i];
				if (minNum >= (int)(count * floorRatio))
				{
					selMin = i;
					break;
				}
			}
			// 去除强度大的和强度小之后的强度范围
			m_nMinIntensity = (int)(minIntensity + (maxIntensity - minIntensity) * 1.0f * selMin / 1000.0f);
			m_nMaxIntensity = (int)(minIntensity + (maxIntensity - minIntensity) * 1.0f * selMax / 1000.0f);
		}

		void CScanSceneNode::ShowBoundingBox( BOOL bShow )
		{
			m_renderBBox = bShow;
		}

		void CScanSceneNode::GetMaxMinZ( float& nMaxZ, float& nMinZ )
		{
			nMaxZ = m_fMaxHeight;
			nMinZ = m_fMinHeight;
		}

		void CScanSceneNode::ShowIntensityRender(BOOL bShow)
		{
			if (bShow == m_bShowIntenRender)
			{
				return;
			}

			m_bShowIntenRender = bShow;
		}

		void CScanSceneNode::SetDefaultColorByZ(int cursel)
		{
			if (cursel == m_colorRampZ.GetCurCursel())
			{
				return;
			}

			m_colorRampZ.SetRampColor4f(cursel);

			if (m_renderStyle == RENDER_BY_Z || m_renderStyle == RENDER_BY_X || m_renderStyle == RENDER_BY_Y)
			{
				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if ( m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0)
				{
					//! 统计高度范围
					if (getType() == ESNT_CLASSIFY_PTD)
					{
						StatCoord();
					}
					// 由视图统计坐标范围
					else
					{	
						if (scnList.size()!= m_scant )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}

					}

				}

				if (m_renderStyle == RENDER_BY_Z)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_renderStyle == RENDER_BY_X)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_renderStyle == RENDER_BY_Y)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCoordRender();
			}
		}

		void CScanSceneNode::SetDefaultColorByCycle(int cursel)
		{
			if (cursel == m_colorRampCycle.GetCurCursel())
			{
				return;
			}

			m_colorRampCycle.SetRampColor4f(cursel);

			if (m_renderStyle == RENDER_BY_CYCLERAMP )
			{
				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if ( m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0)
				{
					//! 统计高度范围
					if (getType() == ESNT_CLASSIFY_PTD)
					{
						StatCoord();
					}
					// 通过视图统计坐标范围
					else
					{
						if (scnList.size()!= m_scant )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}
					}

				}

				if (m_nAxis == 0)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_nAxis == 1)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_nAxis == 2)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				//m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCycleRampRender();
			}
		}

		void CScanSceneNode::SetDefaultColorByCol(int cursel )
		{
			if (cursel == m_colorRampByCol.GetCurCursel())
			{
				return;
			}

			m_colorRampByCol.SetRampColor4f(cursel);
			if(m_renderStyle == RENDER_BY_COL)
			{
				CalcuColRender();
			}
		}

		void CScanSceneNode::SetDefaultColor( const SColorf& clr )
		{
			m_defaultClr.a = hd::u8(clr.a * 255.0f);
			m_defaultClr.r = hd::u8(clr.r * 255.0f);
			m_defaultClr.g = hd::u8(clr.g * 255.0f);
			m_defaultClr.b = hd::u8(clr.b * 255.0f);
		}

		//! 获取当前视图范围点云质心
		BOOL CScanSceneNode::GetViewCenter(float& cx,float& cy,float& cz)
		{
			cx = cy = cz = 0.0f;
			if(m_pointCloud == NULL)
				return FALSE;
			double dx = 0.0;
			double dy = 0.0;
			double dz = 0.0;

			double x,y,z;

			u32 renderSimpleCol = (u32)m_countInView / m_simpleCount;
			if(renderSimpleCol == 0)
				renderSimpleCol = 1;
			u32 renderSimpleRow = renderSimpleCol * 4;
			int editMode = m_pointCloud->getEditMode();
			u32 loopCount = m_pointCloud->getLoopCount();

			// 锁定代码块 防止多线程访问非法造成死机 fengjing 2014/05/13
			EnterCriticalSection(&m_cs);

			u32 ptCount = 0;
			for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
			{
				// 编辑模式下,静态点云需要判断当前圈是否在视图内
				if (editMode == 1 && (m_vecInView.at(n)) == 0)
				{
					continue;
				}

				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				// 非编辑模式下，有些圈（列）会换进换出内存，可能存在部分圈不在内存中 fengjing 2014/05/13
				if (!pts._Myfirst)
				{
					continue;
				}

				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i+=renderSimpleRow)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

					if(!pt.isValid())
					{
						continue;
					}

					if (m_bTrans)
					{
						// 先转换绝对坐标,再用视图坐标转换显示坐标
						x = pt.x;
						y = pt.y;
						z = pt.z;
						//m_absModel.Translate(x,y,z);
						m_renderModel.Translate(x,y,z);
						dx += x;
						dy += y;
						dz += z;
					}
					else
					{
						dx += pt.x;
						dy += pt.y;
						dz += pt.z;
					}
					ptCount++;
				}
			}

			LeaveCriticalSection(&m_cs);

			if (ptCount > 0)
			{
				cx = (float)(dx / ptCount);
				cy = (float)(dy / ptCount);
				cz = (float)(dz / ptCount);
			}
			return ptCount > 0;
		}

		//! 获取旋转中心点,用屏幕中心点射线,求离射线最近点
		float CScanSceneNode::GetRotCenter(float& cx,float& cy,float& cz,int srcX,int srcY)
		{
			cx = cy = cz = 0.0f;
			if(m_pointCloud == NULL || m_pView == NULL && m_simpleCount == 0)
				return FALSE;
			double dx = 0.0;
			double dy = 0.0;
			double dz = 0.0;

			irr::core::vector3df clostPt;

			float minDist = F32_MAX;

			u32 renderSimpleCol = (u32)m_countInView / m_simpleCount;
			if(renderSimpleCol == 0)
				renderSimpleCol = 1;
			u32 renderSimpleRow = renderSimpleCol * 4;
			int editMode = m_pointCloud->getEditMode();
			u32 loopCount = m_pointCloud->getLoopCount();

			//int height = m_pView->GetWindowHeight();
			//int width = m_pView->GetWindowWidth();

			irr::core::position2di srcCenter(srcX,srcY);

			irr::core::line3df rayLine = m_pView->GetSceneManager()->getSceneCollisionManager()->getRayFromScreenCoordinates(srcCenter);

			if (editMode == 0)
			{
				EnterCriticalSection(&m_cs);
			}
			u32 ptCount = 0;			
			for (u32 n = 0;n<loopCount;n+=renderSimpleCol)
			{
				// 编辑模式下,静态点云需要判断当前圈是否在视图内
				if (editMode == 1 && (m_vecInView.at(n)) == 0)
				{
					continue;
				}
				hdVector<PointXYZIPRGBA>* pPts = NULL;
				hdVector<PointXYZIPRGBA> tmpPts;
				if (editMode == 1)
				{
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					pPts = &pts;
				}
				else
				{
					if (!m_pointCloud->getLoop(n,&pPts))
					{
						continue;
					}
					if (pPts == NULL || pPts->size() == 0 || pPts->_Myfirst == NULL)
					{
						continue;
					}
					try
					{
						tmpPts = *pPts;
						pPts = &tmpPts;
					}
					catch (...)
					{
						continue;
					}
				}

				for (u32 i = 0;i < pPts->size();i+=renderSimpleRow)
				{
					const PointXYZIPRGBA& pt = *(pPts->_Myfirst + i);
					if(!pt.isValid())
						continue;

					clostPt.X = pt.x;
					clostPt.Y = pt.y;
					clostPt.Z = pt.z;
					if (m_bTrans)
					{
						// 先转换绝对坐标,再用视图坐标转换显示坐标
						//m_absModel.Translate(clostPt.X,clostPt.Y,clostPt.Z);
						m_renderModel.Translate(clostPt.X,clostPt.Y,clostPt.Z);
					}

					float dist = rayLine.getDistanceToPoint(clostPt);
					if (dist < minDist)
					{
						cx = clostPt.X;
						cy = clostPt.Y;
						cz = clostPt.Z;
						minDist = dist;
					}
				}
			}
			if (editMode == 0)
			{
				LeaveCriticalSection(&m_cs);
			}
			return minDist;
		}

		//! 在车载全景影像定位时,查询设置需要显示的点云,xyz和点云显示坐标系一致
		void CScanSceneNode::QueryByHDI(float cx,float cy,float cz,float dist)		
		{
			if(m_pointCloud == NULL)
				return ;

			// 设置FOV目的,是让camera调用recalculateProjectionMatrix;
			f32 xmin,ymin,zmin,xmax,ymax,zmax;
			f64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;

			//根据设置点为中心点显示一定范围内点云Box
			xmin = cx - dist;
			xmax = cx + dist;
			ymin = cy - dist;
			ymax = cy + dist;
			zmin = cz - dist;
			zmax = cz + dist;

			// 设置显示坐标
			irr::core::aabbox3df queryExtent;
			queryExtent.MinEdge.set(xmin,ymin,zmin);
			queryExtent.MaxEdge.set(xmax,ymax,zmax);

			// 编辑模式下 使用内存点云
			if (m_pointCloud->getEditMode() == 1)
			{
				if (m_bLockMemoryPts)
				{
					return;
				}

				irr::core::aabbox3df loopExtent;
				u32 loopCount = m_pointCloud->getLoopCount();
				m_vecInView.resize(loopCount);
				m_countInView = 0;
				for (u32 n = 0;n < loopCount;n++)
				{
					// 获取每圈范围,并计算到显示坐标系后,在和查询范围进行判断
					m_pointCloud->getLoopExtent(n,xmin,ymin,zmin,xmax,ymax,zmax);

					xminD = xmin;
					yminD = ymin;
					zminD = zmin;

					xmaxD = xmax;
					ymaxD = ymax;
					zmaxD = zmax;
					if (m_bTrans)
					{
						//m_absModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);
						m_renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);
					}

					loopExtent.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
					loopExtent.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);

					if(loopExtent.intersectsWithBox(queryExtent))
					{
						m_countInView += m_pointCloud->getLoop(n).size();
						m_vecInView.at(n) = 1;
					}
					else
					{
						m_vecInView.at(n) = 0;
					}
				}
			}
			// 浏览模式 要按照文件中的点云  [2014/02/25 危迟]
			else										
			{
				g_pSM = SceneManager;
				// 锁定代码块
				EnterCriticalSection(&m_cs);

				xminD = xmin;
				yminD = ymin;
				zminD = zmin;
				xmaxD = xmax;
				ymaxD = ymax;
				zmaxD = zmax;
				m_pointCloud->GetModel().Translate(xminD,yminD,zminD);
				m_pointCloud->GetModel().Translate(xmaxD,ymaxD,zmaxD);
				// 计算出绝对坐标范围进行查询
				BOOL bRet = m_pointCloud->LoadByEnvelope(xminD,xmaxD,yminD,ymaxD);

				// 更新视图可见圈
				UpdateVecInView();

				if (bRet)
				{
					CalcuRenderColor();
				}
				LeaveCriticalSection(&m_cs);
			}

		}

		bool CScanSceneNode::isLoopInView(int n)
		{
			if (m_vecInView.at(n) == 1)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		void CScanSceneNode::CalcuAreaCoordRender()
		{
			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			float scale = 0.0f;
			u8 a = 0,r = 0,g = 0,b = 0;
			float transparent = 1.0f;

			int selStep = 0;
			float tempValue = 0.0f;
			float height = 0.0f;
			float step = (m_fAreaMaxCoord - m_fAreaMinCoord) / 10.0f;

			// 遍历内存点云,计算每个点渲染颜色
			u32 loopCount = m_pointCloud->getLoopCount();
			for (u32 n = 0;n < loopCount;n++)// 遍历圈
			{
				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i++)// 遍历圈内每个点
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid())
						continue;
					// 若不是选择点，则维持其原来的渲染颜色，不做更改
					if (!pt.isSelected())
					{
						continue;
					}

					RenderColor& color = *(colors._Myfirst + i);

					if (m_renderAreaStyle == RENDER_AREA_BY_Z)
					{
						height = pt.z;
					}
					else if (m_renderAreaStyle == RENDER_AREA_BY_X)
					{
						height = pt.x;
					}
					else if (m_renderAreaStyle == RENDER_AREA_BY_Y)
					{
						height = pt.y;
					}

					if (height < m_fAreaMinCoord)
					{
						color.r = ((beginColor>>16) & 0xff);
						color.g = ((beginColor>>8) & 0xff);
						color.b = (beginColor & 0xff);
					}
					else if (height > m_fAreaMaxCoord)
					{
						color.r = ((endColor>>16) & 0xff);
						color.g = ((endColor>>8) & 0xff);
						color.b = (endColor & 0xff);
					}
					else
					{
						selStep = (int)((height - m_fAreaMinCoord) / step);

						tempValue = height - m_fAreaMinCoord - selStep * step;
						scale = tempValue / step;
						m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
					}
				}
			}
		}

		void CScanSceneNode::SetAreaRenderStyle( const ENUM_RENDERSTYLE& style )
		{
			if (m_renderAreaStyle == style && (style == RENDER_AREA_BY_Z || style == RENDER_AREA_BY_X || style == RENDER_AREA_BY_Y) 
				&& m_nCurSelCount == m_pointCloud->getSelectCount())
			{
				return;
			}

			m_renderAreaStyle = style;
			if (style == RENDER_AREA_BY_Z || style == RENDER_AREA_BY_X ||style == RENDER_AREA_BY_Y/* || style == RENDER_AREA_BY_INTENSITY || style == RENDER_AREA_BY_DEFAULT*/)
			{
				// 若选择点云点数与之前记录点数不一致，则须重新计算区域统计
				if (m_nCurSelCount != m_pointCloud->getSelectCount())
				{
					//SetRenderStyle(GetRenderStyle()); [2014/7/24 蔡红云，没有必要设置]
					StatAreaCoord();
					//if (m_nCurSelCount != m_pointCloud->getSelectCount())
					//{
					//	SetRenderStyle(GetRenderStyle());
					//}
					m_nCurSelCount = m_pointCloud->getSelectCount();
					m_bAreaRender = true;
				}

				if (style == RENDER_AREA_BY_Z)
				{
					m_fAreaMinCoord = m_fAreaMinHeight;
					m_fAreaMaxCoord = m_fAreaMaxHeight;
				}
				else if (style == RENDER_AREA_BY_X)
				{
					m_fAreaMinCoord = m_fAreaMinX;
					m_fAreaMaxCoord = m_fAreaMaxX;
				}
				else if (style == RENDER_AREA_BY_Y)
				{
					m_fAreaMinCoord = m_fAreaMinY;
					m_fAreaMaxCoord = m_fAreaMaxY;
				}
				//m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				//计算设置区域渲染颜色条颜色
				CalcuAreaCoordRender();
			}
		}

		hd::ENUM_RENDERSTYLE CScanSceneNode::GetAreaRenderStyle() const
		{
			return m_renderAreaStyle;
		}

		void CScanSceneNode::QueryByTime( double scaleStart,double scaleEnd )
		{
			if(m_pointCloud == NULL)
				return ;

			// 编辑模式 使用内存中的点云
			if (m_pointCloud->getEditMode() == 1)
			{
				if (m_bLockMemoryPts)
				{
					return;
				}

				u32 loopCount = m_pointCloud->getLoopCount();
				u32 loopCountInFile = m_pointCloud->m_header.number_of_col;

				m_vecInView.resize(loopCount);
				m_countInView = 0;

				// 获取比例在文件中的对应的列数
				s32 curLoopIndexInFile = 0;
				int startLoopInFile = 0;
				int endLoopInFile = 0;
				startLoopInFile = floor(scaleStart * loopCountInFile + 0.5);
				endLoopInFile = floor(scaleEnd * loopCountInFile + 0.5);

				// 防止越界
				if (endLoopInFile >= loopCountInFile)
				{

					endLoopInFile = loopCountInFile - 1;
				}

				//// 获得内存中的比例值
				//int startLoop = 0;
				//int endLoop = 0;
				//startLoop = (int)(scaleStart * loopCount);
				//endLoop = (int)(scaleEnd * loopCount);

				for (u32 n = 0;n < loopCount;n++)
				{
					curLoopIndexInFile = m_pointCloud->getLoopIndex(n);
					//if (n >= startLoop && n <= endLoop)
					if (curLoopIndexInFile >= startLoopInFile && curLoopIndexInFile <= endLoopInFile)
					{
						m_countInView += m_pointCloud->getLoop(n).size();
						m_vecInView.at(n) = 1;
					}
					else
					{
						m_vecInView.at(n) = 0;
					}
				}
			}
			// 浏览模式 使用文件中的点云   [2014/02/25 危迟]
			else
			{
				g_pSM = SceneManager;
				// 锁定代码块
				EnterCriticalSection(&m_cs);

				BOOL bRet = m_pointCloud->loadHlsByScale(scaleStart,scaleEnd,NULL);

				if (bRet)
				{
					CalcuRenderColor();
				}
				LeaveCriticalSection(&m_cs);

			}

		}

		void CScanSceneNode::GetMinMaxValue( float& maxValue, float& minValue )
		{
			// 默认偏移量采用该点云的偏移量
			f32 offsetx = (f32)m_pointCloud->m_header.offsetX;
			f32 offsety = (f32)m_pointCloud->m_header.offsetY;
			f32 offsetz = (f32)m_pointCloud->m_header.offsetZ;
			//  2013/11/7 蔡红云 通过从视图获取 最终偏移量
			//  解决多次站视图下，按高程渲染不一致问题
			m_pView->getAllScanSnodeExt(offsetx, offsety, offsetz);
			if (m_pointCloud)
			{
				if (m_renderStyle == RENDER_BY_Z)
				{			
					//maxValue = m_fMaxCoord + m_pointCloud->m_header.offsetZ;
					//minValue = m_fMinCoord + m_pointCloud->m_header.offsetZ;
					maxValue = m_fMaxCoord + offsetz;
					minValue = m_fMinCoord + offsetz;

				}
				else if (m_renderStyle == RENDER_BY_X)
				{
					/*maxValue = m_fMaxCoord + m_pointCloud->m_header.offsetX;
					minValue = m_fMinCoord + m_pointCloud->m_header.offsetX;*/
					maxValue = m_fMaxCoord + offsetx;
					minValue = m_fMinCoord + offsetx;

				}
				else if (m_renderStyle == RENDER_BY_Y)
				{
					/*	maxValue = m_fMaxCoord + m_pointCloud->m_header.offsetY;
					minValue = m_fMinCoord + m_pointCloud->m_header.offsetY;*/
					maxValue = m_fMaxCoord + offsety;
					minValue = m_fMinCoord + offsety;
				}
			}

		}

		void CScanSceneNode::StatDistance()
		{
			u64 count = m_pointCloud->count();
			U32 validCount = m_pointCloud->getValidCount();
			if (m_pointCloud == NULL || count <= 0 || validCount <= 0)
			{
				return;
			}

			int mapDistance[1000] = {0};
			int maxDist = (int)m_pointCloud->getMaxDistance();
			int minDist = (int)m_pointCloud->getMinDistance();
			int maxNum = 0;
			int selMax = 0;
			int minNum = 0;
			int selMin = 0;
			int selNum = 0;

			f32 distRcp = 1000.0f / (maxDist - minDist);
			u32 loopCount = m_pointCloud->getLoopCount();

			// 统计距离分布情况
			for (u32 n = 0; n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);

				for (u32 i = 0; i< pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

					if (!pt.isValid())
					{
						continue;
					}

					float dist = pt.x * pt.x + pt.y * pt.y + pt.z * pt.z;
					selNum = (int)((dist - minDist) * distRcp);
					selNum = clamp(selNum,0,999);
					mapDistance[selNum]++;
				}
			}
			//计算距离大的%1位置
			int i = 0,j = 0;
			for (j = 999;j >= 0;j--)
			{
				maxNum += mapDistance[j];
				if (maxNum >= (int)(count * 0.01))
				{
					selMax = j;
					break;
				}
			}
			//计算距离小的%1位置
			for (i = 0;i < 1000;i++)
			{
				minNum += mapDistance[i];
				if (minNum >= (int)(count * 0.01))
				{
					selMin = i;
					break;
				}
			}
			// 去除距离大的和距离小之后的强度范围
			m_fMinDist= minDist + (maxDist - minDist) * 1.0f * selMin / 1000.0f;
			m_fMaxDist = minDist+ (maxDist - minDist) * 1.0f * selMax / 1000.0f;
		}

		void SetClassMap(CClassificationMap ClassMap)
		{

		}

		unsigned int CScanSceneNode::Get3DPosFromScrPos( PointXYZIPRGBA& ptPoint, f64& x,f64& y,f64& z, int srcX,int srcY, int tol,  bool bFindVisibleOnly )
		{
			// 			if (tol < 0)
			// 			{
			// 				return U32_MAX;
			// 			}

			x = 0.0;
			y = 0.0;
			z = 0.0;
			irr::core::recti irrRect;
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			bool bClassifyView = false;
			core::vector3df coord;
			core::position2di screenPos;

			irr::core::aabbox3df loopBox;
			F32 xmin,ymin,zmin,xmax,ymax,zmax;
			F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
			u32 srcMinDist = U32_MAX,srcDist;
			//候选点与点击点x/y方向的距离，应该使用有符号整形！必须考虑符号转型带来的严重影响，因为有符号负数赋值给无符号整形会变成一个很大的正数。 袁亮  20160923
			int srcDx,srcDy;
			F64 ptX,ptY,ptZ;

			if(m_pointCloud == NULL || (!isVisible() && bFindVisibleOnly))
				return srcMinDist;

			// 如果查找全部点云
			if (!bFindVisibleOnly)
			{					
				u32 loopCount = m_pointCloud->getLoopCount();
				for (u32 j = 0;j<loopCount;j++)
				{
					// 不在视椎体内的点排除
					if (m_vecInView[j] == 0)
					{
						continue;
					}

					m_pointCloud->getLoopExtent(j,xmin,ymin,zmin,xmax,ymax,zmax);
					if (m_bTrans)
					{
						xminD = xmin;
						yminD = ymin;
						zminD = zmin;
						xmaxD = xmax;
						ymaxD = ymax;
						zmaxD = zmax;
						m_renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

						loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
						loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
					}
					else
					{
						loopBox.MinEdge.set(xmin,ymin,zmin);
						loopBox.MaxEdge.set(xmax,ymax,zmax);
					}
					if (!rgnFrustum.isCubeIn(loopBox))
					{
						continue;
					}

					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(j);
					for (u32 n = 0;n < pts.size();n++)
					{
						PointXYZIPRGBA& pt = *(pts._Myfirst + n);
						if (!pt.isValid())
						{
							continue;
						}

						if (m_bTrans)
						{			
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;
							m_renderModel.Translate(ptX,ptY,ptZ);
							coord.set((float)ptX,(float)ptY,(float)ptZ);
						}
						else
						{
							coord.set(pt.x,pt.y,pt.z);
						}

						screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
						if (!irrRect.isPointInside(screenPos))
							continue;

						srcDx = screenPos.X - srcX;
						srcDy = screenPos.Y - srcY;
						srcDist = srcDx * srcDx + srcDy * srcDy;
						if (srcDist < srcMinDist)
						{
							srcMinDist = srcDist;
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;

							// 返回的绝对坐标
							m_absModel.Translate(ptX,ptY,ptZ);
							x = ptX;
							y = ptY;
							z = ptZ;

							// 返回的相对坐标
							ptPoint = pt;

						}

					}//for (u32 n = 0;n < pts.size();n++)
				}//for (u32 i = 0;i<loopCount;i++)
			}
			// 如果查找渲染可见的点
			else
			{
				u32 loopCount = m_vecInView.size();

				for (u32 n = 0;n < loopCount;n++)
				{
					// 不在视椎体内的点排除 fengjing
					if (!m_vecInView[n])
					{
						continue;
					}

					m_pointCloud->getLoopExtent(m_vecInView[n],xmin,ymin,zmin,xmax,ymax,zmax);

					if (m_bTrans)
					{
						xminD = xmin;
						yminD = ymin;
						zminD = zmin;
						xmaxD = xmax;
						ymaxD = ymax;
						zmaxD = zmax;
						m_renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

						loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
						loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
					}
					else
					{
						loopBox.MinEdge.set(xmin,ymin,zmin);
						loopBox.MaxEdge.set(xmax,ymax,zmax);
					}

					// 遍历在视图中的圈
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (u32 n = 0;n < pts.size();n++)
					{
						PointXYZIPRGBA& pt = *(pts._Myfirst + n);
						if (!pt.isValid())
						{
							continue;
						}

						if (m_bTrans)
						{			
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;
							m_renderModel.Translate(ptX,ptY,ptZ);
							coord.set((float)ptX,(float)ptY,(float)ptZ);
						}
						else
						{
							coord.set(pt.x,pt.y,pt.z);
						}

						screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
						if (!irrRect.isPointInside(screenPos))
							continue;

						srcDx = screenPos.X - srcX;
						srcDy = screenPos.Y - srcY;
						srcDist = srcDx * srcDx + srcDy * srcDy;
						if (srcDist < srcMinDist)
						{
							srcMinDist = srcDist;
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;

							// 返回的绝对坐标
							m_absModel.Translate(ptX,ptY,ptZ);
							x = ptX;
							y = ptY;
							z = ptZ;

							// 返回的相对坐标及点的强度信息等
							ptPoint = pt;

						}
					}
				}				
			}

			return srcMinDist/* < U32_MAX*/;
		}

		unsigned int CScanSceneNode::Get3DPosFromScrPos( PointXYZIPRGBA& ptPoint, f64& x,f64& y,f64& z, int srcX,int srcY, int tol,  bool bFindVisibleOnly, bool bSeclect )
		{
			// 			if (tol < 0)
			// 			{
			// 				return U32_MAX;
			// 			}

			x = 0.0;
			y = 0.0;
			z = 0.0;
			irr::core::recti irrRect;
			irrRect.LowerRightCorner.set(srcX + tol,srcY + tol);
			irrRect.UpperLeftCorner.set(srcX - tol,srcY - tol);
			const irr::scene::SViewFrustum* pViewFrustum = SceneManager->getActiveCamera()->getViewFrustum();
			irr::scene::SViewFrustum rgnFrustum = *pViewFrustum;
			SceneManager->GetViewFrustum(irrRect,&rgnFrustum);

			bool bClassifyView = false;
			core::vector3df coord;
			core::position2di screenPos;

			irr::core::aabbox3df loopBox;
			F32 xmin,ymin,zmin,xmax,ymax,zmax;
			F64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;
			u32 srcMinDist = U32_MAX,srcDist;
			//候选点与点击点x/y方向的距离，应该使用有符号整形！必须考虑符号转型带来的严重影响，因为有符号负数赋值给无符号整形会变成一个很大的正数。 袁亮  20160923
			int srcDx,srcDy;
			F64 ptX,ptY,ptZ;

			if(m_pointCloud == NULL || (!isVisible() && bFindVisibleOnly))
				return srcMinDist;

			// 如果查找全部点云
			if (!bFindVisibleOnly)
			{					
				u32 loopCount = m_pointCloud->getLoopCount();
				for (u32 j = 0;j<loopCount;j++)
				{
					// 不在视椎体内的点排除
					if (m_vecInView[j] == 0)
					{
						continue;
					}

					m_pointCloud->getLoopExtent(j,xmin,ymin,zmin,xmax,ymax,zmax);
					if (m_bTrans)
					{
						xminD = xmin;
						yminD = ymin;
						zminD = zmin;
						xmaxD = xmax;
						ymaxD = ymax;
						zmaxD = zmax;
						m_renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

						loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
						loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
					}
					else
					{
						loopBox.MinEdge.set(xmin,ymin,zmin);
						loopBox.MaxEdge.set(xmax,ymax,zmax);
					}
					if (!rgnFrustum.isCubeIn(loopBox))
					{
						continue;
					}

					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(j);
					for (u32 n = 0;n < pts.size();n++)
					{
						PointXYZIPRGBA& pt = *(pts._Myfirst + n);
						
						if (!pt.isValid())
						{
							continue;
						}

						if (bSeclect && !pt.isSelected())
						{
							continue;
						}
						
						if (m_bTrans)
						{			
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;
							m_renderModel.Translate(ptX,ptY,ptZ);
							coord.set((float)ptX,(float)ptY,(float)ptZ);
						}
						else
						{
							coord.set(pt.x,pt.y,pt.z);
						}

						screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
						if (!irrRect.isPointInside(screenPos))
							continue;

						srcDx = screenPos.X - srcX;
						srcDy = screenPos.Y - srcY;
						srcDist = srcDx * srcDx + srcDy * srcDy;
						if (srcDist < srcMinDist)
						{
							srcMinDist = srcDist;
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;

							// 返回的绝对坐标
							m_absModel.Translate(ptX,ptY,ptZ);
							x = ptX;
							y = ptY;
							z = ptZ;

							// 返回的相对坐标
							ptPoint = pt;

						}

					}//for (u32 n = 0;n < pts.size();n++)
				}//for (u32 i = 0;i<loopCount;i++)
			}
			// 如果查找渲染可见的点
			else
			{
				u32 loopCount = m_vecInView.size();

				for (u32 n = 0;n < loopCount;n++)
				{
					// 不在视椎体内的点排除 fengjing
					if (!m_vecInView[n])
					{
						continue;
					}

					m_pointCloud->getLoopExtent(m_vecInView[n],xmin,ymin,zmin,xmax,ymax,zmax);

					if (m_bTrans)
					{
						xminD = xmin;
						yminD = ymin;
						zminD = zmin;
						xmaxD = xmax;
						ymaxD = ymax;
						zmaxD = zmax;
						m_renderModel.TranslateExtent(xminD,yminD,zminD,xmaxD,ymaxD,zmaxD);

						loopBox.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
						loopBox.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
					}
					else
					{
						loopBox.MinEdge.set(xmin,ymin,zmin);
						loopBox.MaxEdge.set(xmax,ymax,zmax);
					}

					// 遍历在视图中的圈
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					for (u32 n = 0;n < pts.size();n++)
					{
						PointXYZIPRGBA& pt = *(pts._Myfirst + n);
						if (!pt.isValid())
						{
							continue;
						}

						if (m_bTrans)
						{			
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;
							m_renderModel.Translate(ptX,ptY,ptZ);
							coord.set((float)ptX,(float)ptY,(float)ptZ);
						}
						else
						{
							coord.set(pt.x,pt.y,pt.z);
						}

						screenPos = SceneManager->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(coord);
						if (!irrRect.isPointInside(screenPos))
							continue;

						srcDx = screenPos.X - srcX;
						srcDy = screenPos.Y - srcY;
						srcDist = srcDx * srcDx + srcDy * srcDy;
						if (srcDist < srcMinDist)
						{
							srcMinDist = srcDist;
							ptX = pt.x;
							ptY = pt.y;
							ptZ = pt.z;

							// 返回的绝对坐标
							m_absModel.Translate(ptX,ptY,ptZ);
							x = ptX;
							y = ptY;
							z = ptZ;

							// 返回的相对坐标及点的强度信息等
							ptPoint = pt;

						}
					}
				}				
			}

			return srcMinDist/* < U32_MAX*/;
		}

		hd::u64 CScanSceneNode::GetTotalCountInView()
		{
			if(m_pointCloud == NULL)
				return 0;
			u64 count = m_pointCloud->count();
			u64 inViewCount = 0;

			if (m_pointCloud->getEditMode() == 1)
			{	
				g_pSM = SceneManager;
				const SViewFrustum* pViewFrustom = SceneManager->getActiveCamera()->getViewFrustum();

				//f32 xmin,ymin,zmin,xmax,ymax,zmax;
				//f64 xminD,yminD,zminD,xmaxD,ymaxD,zmaxD;

				irr::core::aabbox3df viewFrustBox;
				viewFrustBox = pViewFrustom->getBoundingBox();
				u32 loopCount = m_pointCloud->getLoopCount();
				m_vecInView.resize(loopCount);
				count = 0;
				for (u32 n = 0;n < loopCount;n++)
				{
					//if (n == 0)
					//{
					//	m_pointCloud->getLoopExtent(n,xmin,ymin,zmin,xmax,ymax,zmax);

					//	hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					//}


					if (m_vecInView.at(n) = 0)
						continue;

					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);

					for (u32 i = 0;i < pts.size();i++)
					{
						const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
						if(!pt.isValid())
							continue;

						if (isPtInViewBox(pt))
						{
							inViewCount++;
						}
						//ptX = pt.x;
						//ptY = pt.y;
						//ptZ = pt.z;

						//if (m_bTrans)
						//{
						//	m_renderModel.Translate(ptX,ptY,ptZ);
						//}
						//vecPt.X = ptX;
						//vecPt.Y = ptY;
						//vecPt.Z = ptZ;

						//if (viewFrustBox.isPointInside(vecPt))
						//{
						//	inViewCount++;
						//}
					}
					//m_pointCloud->getLoopExtent(n,xmin,ymin,zmin,xmax,ymax,zmax);

					//xminD = xmin;
					//yminD = ymin;
					//zminD = zmin;

					//xmaxD = xmax;
					//ymaxD = ymax;
					//zmaxD = zmax;
					//loopExtent.MinEdge.set((f32)xminD,(f32)yminD,(f32)zminD);
					//loopExtent.MaxEdge.set((f32)xmaxD,(f32)ymaxD,(f32)zmaxD);
					//if(pViewFrustom->isCubeIn(loopExtent))
					//{
					//	count += m_pointCloud->getLoop(n).size();
					//	*(m_vecInView._Myfirst + n) = 1;
					//}
					//else
					//{
					//	*(m_vecInView._Myfirst + n) = 0;
					//}
				}				
			}
			return inViewCount;
		}

		bool CScanSceneNode::isPtInViewBox( const PointXYZIPRGBA& pt )
		{
			g_pSM = SceneManager;
			const SViewFrustum* pViewFrustom = SceneManager->getActiveCamera()->getViewFrustum();

			f64 ptX,ptY,ptZ;
			irr::core::aabbox3df viewFrustBox;
			irr::core::vector3df vecPt;
			viewFrustBox = pViewFrustom->getBoundingBox();

			ptX = pt.x;
			ptY = pt.y;
			ptZ = pt.z;

			if (m_bTrans)
			{
				m_renderModel.Translate(ptX,ptY,ptZ);
			}
			vecPt.X = (f32)ptX;
			vecPt.Y = (f32)ptY;
			vecPt.Z = (f32)ptZ;

			if (viewFrustBox.isPointInside(vecPt)) //  isPointTotalInside
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		// 获得视锥的obbbox
		void CScanSceneNode::GetObbBoxByView(CHdobBox3d& viewObbox)
		{
			if (g_pSM == NULL)
			{
				return;
			}

			ICameraSceneNode* pCamera = g_pSM->getActiveCamera();
			const SViewFrustum* pViewFrustom = pCamera->getViewFrustum();

			// 定义临时变量值
			f32 vertex[24] = {0.0};
			core::vector3df tmpVec;

			// 顶点0
			tmpVec = pViewFrustom->getFarRightUp();
			vertex[0] = tmpVec.X;
			vertex[1] = tmpVec.Y;
			vertex[2] = tmpVec.Z;

			// 顶点1
			tmpVec = pViewFrustom->getFarLeftUp();
			vertex[3] = tmpVec.X;
			vertex[4] = tmpVec.Y;
			vertex[5] = tmpVec.Z;

			// 顶点2
			tmpVec = pViewFrustom->getFarLeftDown();
			vertex[6] = tmpVec.X;
			vertex[7] = tmpVec.Y;
			vertex[8] = tmpVec.Z;

			// 顶点3
			tmpVec = pViewFrustom->getFarRightDown();
			vertex[9] = tmpVec.X;
			vertex[10] = tmpVec.Y;
			vertex[11] = tmpVec.Z;

			// 顶点4
			tmpVec = pViewFrustom->getNearRightUp();
			vertex[12] = tmpVec.X;
			vertex[13] = tmpVec.Y;
			vertex[14] = tmpVec.Z;

			// 顶点5
			tmpVec = pViewFrustom->getNearLeftUp();
			vertex[15] = tmpVec.X;
			vertex[16] = tmpVec.Y;
			vertex[17] = tmpVec.Z;

			// 顶点6
			tmpVec = pViewFrustom->getNearLeftDown();
			vertex[18] = tmpVec.X;
			vertex[19] = tmpVec.Y;
			vertex[20] = tmpVec.Z;

			// 顶点7
			tmpVec = pViewFrustom->getNearRightDown();
			vertex[21] = tmpVec.X;
			vertex[22] = tmpVec.Y;
			vertex[23] = tmpVec.Z;


			viewObbox.SetVertex(vertex);
		}

		// 通过obbbox相交判断获得需要渲染的点、圈
		hd::u64 CScanSceneNode::GetCountInViewByObbBox()
		{
			if(m_pointCloud == NULL)
				return 0;
			u64 count = m_pointCloud->count();

			// 获得视锥
			g_pSM = SceneManager;

			// 获得视锥的obb box
			CHdobBox3d obbViewBox;
			GetObbBoxByView(obbViewBox);

			//点云圈的obb box
			CHdobBox3d pcdObbox;

			// 编辑模式
			if (m_pointCloud->getEditMode() == 1)
			{		
				// 定义顶点数组
				f32 vertex[24] = {0.0};

				u32 loopCount = m_pointCloud->getLoopCount();
				m_vecInView.resize(loopCount);
				count = 0;
				for (u32 n = 0;n < loopCount;n++)
				{
					// 存在只加载某一段点云情况，需进行判断
					hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
					if (pts.size() <= 0)
					{
						m_vecInView.at(n) = 0;
						continue;
					}

					// 每次循环之前先初始化
					memset(vertex,0,sizeof(f32) * 24);

					// 此处获得的顶点坐标为点云的相对坐标，需要转换到视图显示坐标系
					m_pointCloud->GetObbVertex(n,vertex);

					// 八个顶点分别转到视图显示坐标系下
					for (int i = 0;i < 8;i++)
					{
						if (m_bTrans)
						{
							m_renderModel.Translate(vertex[i*3],vertex[i*3+1],vertex[i*3+2]);
						}
					}

					// 点云圈obb box顶点赋值
					pcdObbox.SetVertex(vertex);

					// 用视锥的obb box与圈的obb box相交判断
					if(pcdObbox.BoxIntersect(&obbViewBox) == 1)
					{
						// 获得该圈点云的有效点个数，而不是总点数
						count += m_pointCloud->GetValidCountInLoop(n);
						m_vecInView.at(n) = 1;
					}
					else
					{
						m_vecInView.at(n) = 0;
					}
				}				
			}
			return count;
		}

		int CScanSceneNode::GetRenderSimpleLevel()
		{
			if (!m_pointCloud)
			{
				return 0;
			}

			// 浏览模式下，返回点云从加载到显示的抽稀比例
			int editMode = m_pointCloud->getEditMode();
			if (editMode == 0)
			{
				return m_pointCloud->getSimpleLevel();
			}

			// 编辑模式下，返回抽稀比例
			if (m_simpleCount > 0)
			{
				return m_countInView > m_simpleCount ? (int)(m_countInView / m_simpleCount) : 1;
			}
			else
			{
				return 0;
			}
		}

		void CScanSceneNode::UpdateVecInView()
		{
			// 获得当前内存中可见总圈数
			std::vector<u32> vecQeuryResult = m_pointCloud->GetQueryResult();

			if (m_vecInView.size() <= 0)
			{
				u32 loopCount = m_pointCloud->getLoopCount();
				m_vecInView.resize(loopCount);
			}

			// 更新视图中可见圈信息,首先全部置零
			for (unsigned int u = 0;u < m_vecInView.size();u++)
			{
				m_vecInView.at(u) = 0;
			}

			int nQueryInView = 0;

			// 根据查询结果更新可见
			for (unsigned int n = 0; n < vecQeuryResult.size();n++)
			{
				nQueryInView = vecQeuryResult.at(n);

				// 范围更新
				if (nQueryInView < 0 || nQueryInView >= (int)m_vecInView.size())
				{
					continue;
				}

				// 更新查询，设置可见
				m_vecInView.at(nQueryInView) = 1;
			}
		}

		void CScanSceneNode::StatDistanceToLoopCenter()
		{
			u64 count = m_pointCloud->count();
			U32 validCount = m_pointCloud->getValidCount();
			if (m_pointCloud == NULL || count <= 0 || validCount <= 0)
			{
				return;
			}

			// 非编辑模式不可用
			if (CHdSysSetting::getSysSetting()->commonSetting.editMode != 1)
			{
				return;
			}

			// 获得lin路径
			bool bLinExist = false;
			string strLinPath = m_pointCloud->GetPosFileByHlsPath(bLinExist);
			if (!bLinExist)
			{
				return;
			}

			// 定义存放lin文件中pos信息的vec
			std::vector<HD_SCANHDIINFO> vecScanInfo;

			// 从文件中读取lin
			m_pointCloud->ReadLin( strLinPath.data(),vecScanInfo);

			m_fMinDist = 0.0f;
			m_fMaxDist = F32_MIN;

			// 获得内存中圈数
			u32 loopCount = m_pointCloud->getLoopCount();

			f64 posX,posY,posZ;
			posX = posY = posZ = 0.0;

			// 遍历统计计算
			for (u32 n = 0;n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at( n) == 0)
				{
					continue;
				}

				// 获得每圈点云的物理圈号
				u32 loopIndex = m_pointCloud->getLoopIndex(n);
				if (loopIndex < 0 || loopIndex >= vecScanInfo.size())
				{
					continue;
				}

				// 逐点统计计算
				HD_SCANHDIINFO& hdi = vecScanInfo.at(loopIndex);

				// 该圈pos中心点
				posX = hdi.dX;
				posY = hdi.dY;
				posZ = hdi.dZ;

				// lin存储为绝对坐标，转换为相对坐标
				m_pointCloud->GetModel().AntiTranslate(posX,posY,posZ);

				// 标记，只计算一圈点云
				bool bCal = false;

				// 由于每圈点云大致距离pos中心距离较一致，只统计最大距离点
				const hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				for (u32 i = 0; i < pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);

					if (!pt.isValid())
					{
						continue;
					}

					// 计算点到pos中心点距离
					float dist = sqrt(pow(pt.x - posX,2) + pow(pt.y - posY,2) + pow(pt.z - posZ,2));
					m_fMaxDist = MAX(dist,m_fMaxDist);
					bCal = true;
				}

				// 已经计算过一圈点云，跳出循环
				if (bCal)
				{
					break;
				}
			}
		}

		void CScanSceneNode::CalcuDistToCenterRender()
		{
			int selStep = 0;
			u8 a = 0;
			float dist = 0.0f;
			float tempValue = 0.0f;
			float scale = 0.0f;
			float r = 0,g = 0,b = 0;

			u32 loopCount = m_pointCloud->getLoopCount();
			float minDist = m_fMinDist;
			float maxDist = m_fMaxDist;

			// 获取颜色条起始和终止颜色
			u32 beginColor = m_colorRampZ.GetBeginColor();
			u32 endColor = m_colorRampZ.GetEndColor();

			// 获得lin路径
			bool bLinExist = false;
			string strLinPath = m_pointCloud->GetPosFileByHlsPath(bLinExist);
			if (!bLinExist)
			{
				return;
			}

			// 定义存放lin文件中pos信息的vec
			std::vector<HD_SCANHDIINFO> vecScanInfo;

			// 从文件中读取lin
			m_pointCloud->ReadLin( strLinPath.data(),vecScanInfo);

			// 定义中间变量
			f64 posX,posY,posZ;
			posX = posY = posZ = 0.0;

			// 遍历分配颜色
			for (u32 n = 0;n < loopCount;n++)
			{
				// 在浏览模式下，不在视图范围内的圈数据不渲染
				if (m_pointCloud->getEditMode() == 0 && m_vecInView.at(n) == 0)
				{
					continue;
				}

				// 获得每圈点云的物理圈号
				u32 loopIndex = m_pointCloud->getLoopIndex(n);
				if (loopIndex < 0 || loopIndex >= vecScanInfo.size())
				{
					continue;
				}

				// 逐点统计计算
				HD_SCANHDIINFO& hdi = vecScanInfo.at(loopIndex);

				// 该圈pos中心点
				posX = hdi.dX;
				posY = hdi.dY;
				posZ = hdi.dZ;

				// lin存储为绝对坐标，转换为相对坐标
				m_pointCloud->GetModel().AntiTranslate(posX,posY,posZ);

				// 获取一圈点
				hdVector<PointXYZIPRGBA>& pts = m_pointCloud->getLoop(n);
				hdVector<RenderColor>& colors = *(m_renderColors._Myfirst + n);
				for (u32 i = 0;i < pts.size();i++)
				{
					const PointXYZIPRGBA& pt = *(pts._Myfirst + i);
					if(!pt.isValid())
						continue;

					RenderColor& color = *(colors._Myfirst + i);

					// 计算距离值
					dist = sqrt(pow(pt.x - posX,2) + pow(pt.y - posY,2) + pow(pt.z - posZ,2));

					if (dist < minDist)
					{
						color.r = ((beginColor>>16) & 0xff);
						color.g = ((beginColor>>8) & 0xff);
						color.b = (beginColor & 0xff);
					}
					else if (dist > maxDist)
					{
						color.r = ((endColor>>16) & 0xff);
						color.g = ((endColor>>8) & 0xff);
						color.b = (endColor & 0xff);
					}
					else
					{
						selStep = (int)((dist - minDist) * 10 /(maxDist - minDist));

						tempValue = dist - minDist - selStep * (maxDist - minDist) / 10.0f;
						scale = tempValue * 10/(maxDist - minDist);
						m_colorRampZ.GetColor4ub(scale,a,color.r,color.g,color.b,selStep + 1);
					}
				}
			}
		}
		
		//! 设置过滤类型以刷新显示
		hd::u32 CScanSceneNode::updateForFilter()
		{
			if (m_pointCloud == NULL)
			{
				return 0;
			}

			//! 过滤管理器
			//ptcloud::hdFilterManager* filter_manager = m_pView->getFilterManager();
			ptcloud::hdFilterManager* filter_manager = m_pointCloud->GetFilterManager();

			u32 nSelectedCount = 0;
			if (filter_manager)
			{
				u32 loopCount = m_pointCloud->getLoopCount();
				for (int k=0; k<loopCount; k++)
				{
					hdVector< PointXYZIPRGBA >& vLoopPoint = m_pointCloud->getLoop(k);
					filter_manager->doFilterFresh(&vLoopPoint, NULL, NULL);

					unsigned int nPointCount = vLoopPoint.size();
					for (unsigned int i = 0; i < nPointCount; i ++)
					{
						if (vLoopPoint[i].isSelected())
						{
							nSelectedCount ++;
						}
					}
				}
			}

			//处理颜色//! 重新渲染数据			
			//reRenderStyle();

			return nSelectedCount;
		}

		//! 重新渲染数据
		void CScanSceneNode::reRenderStyle()
		{
			//// 若为区域渲染，考虑区域点云可能变化，而需要进行二次渲染
			//if (m_renderStyle == style && (style != RENDER_BY_Z && style != RENDER_BY_X && style != RENDER_BY_Y && style != RENDER_BY_COL && style != RENDER_BY_CYCLERAMP && style != RENDER_BY_DISTANCE))
			//{
			//	return;
			//}
			//m_renderStyle = style;

			ENUM_RENDERSTYLE style = m_renderStyle;
			if (style == RENDER_BY_Z || style == RENDER_BY_X || style == RENDER_BY_Y)
			{
				// m_bShowIntenRender该值应该是外部传入，初始构造默认为true就可以了，内部不应该改变-zhubo
				//m_bShowIntenRender = TRUE;

				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if (  m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0 || m_bAreaRender)
				{
					//! 统计高度范围
					// 如果点云是分类点云单独统计自己的坐标范围
					if (getType() == ESNT_CLASSIFY_PTD  )
					{
						StatCoord();
					}
					// 否则通过视图来统计坐标范围 
					else
					{
						if (scnList.size()!= m_scant  )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}
						else
						{
							if (m_bAreaRender)
							{
								StatCoord();
							}
						}
					}
					m_bAreaRender = false;
				}

				if (m_renderStyle == RENDER_BY_Z)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_renderStyle == RENDER_BY_X)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_renderStyle == RENDER_BY_Y)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCoordRender();

				//更新计算设置区域渲染颜色条颜色 
				//CalcuAreaCoordRender();// [2014/7/24 蔡红云 默认不是按照区域渲染，不需要计算，计算和区域渲染设置保持一致 ]
			}
			else if (style == RENDER_BY_INTENSITY)
			{
				//m_bShowIntenRender = TRUE;
				if (m_nMinIntensity == 0 && m_nMaxIntensity == 0)
				{
					StatIntensity();
				}
				CalcuIntensityRender();
				//CalcIntensityRenderTest();
			}
			else if (style == RENDER_BY_DISTANCE)
			{
				// 在iScan视图下，LIN文件存在时，根据轨迹点范围渲染
				if (m_pointCloud->isLinExist() && strcmp(m_pView->GetName(),"iScan3DView") == 0)
				{
					if (m_fMinDist == 0.0f && m_fMaxDist == 0.0f)
					{
						StatDistanceToLoopCenter();
					}

					// 计算分配渲染颜色
					CalcuDistToCenterRender();
				}
				else
				{
					if (m_fMinDist == 0.0f && m_fMaxDist == 0.0f)
					{
						StatDistance();
					}

					CalcuDistRender();
				}

			}
			else if(style == RENDER_BY_COL)
			{
				//m_bShowIntenRender = TRUE;
				CalcuColRender();
			}
			else if (style == RENDER_BY_RGB)
			{
				// 用RGB显示情况下,不显示透明度
				//ShowIntensityRender(FALSE);
			}
			else if (style == RENDER_BY_DEFAULT)
			{
				//m_bShowIntenRender = TRUE;
			}
			else if (style == RENDER_BY_CYCLERAMP)
			{
				//m_bShowIntenRender = TRUE;
				core::array<ISceneNode*> scnList;
				SceneManager->getSceneNodesFromType(ESNT_SCAN_POINT, scnList);
				if (  m_bPcdChanged || m_heightStep.size() == 0 ||
					m_xStep.size() == 0 || m_yStep.size() == 0)
				{
					//! 统计高度范围
					// 如果是分类点云，自己统计自己坐标范围
					if (getType() == ESNT_CLASSIFY_PTD)
					{
						StatCoord();
					}
					// 否则由视图统计坐标范围
					else
					{
						if (scnList.size()!= m_scant )
						{
							m_pView->statAllScanSndeStatCoord();
							m_scant = scnList.size();
						}

					}
				}

				if (m_nAxis == 0)
				{
					m_fMinCoord = m_fMinHeight;
					m_fMaxCoord = m_fMaxHeight;
				}
				else if (m_nAxis == 1)
				{
					m_fMinCoord = m_fMinCorX;
					m_fMaxCoord = m_fMaxCorX;
				}
				else if (m_nAxis == 2)
				{
					m_fMinCoord = m_fMinCorY;
					m_fMaxCoord = m_fMaxCorY;
				}
				//m_fStep = (m_fMaxCoord - m_fMinCoord) / 10.0f;

				CalcuCycleRampRender();
			}
		}
	}
}
