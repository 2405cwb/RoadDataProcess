/*! hnColorRamp.cpp
********************************************************************************
<PRE>
模块名       : hnPointCloud
文件名       : hnColorRamp.cpp
相关文件     : hnColorRamp.h
文件实现功能 : 渐变色
作者         : 谢卓
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期			版本			修改人				修改内容
2017/8/2		1.0			谢卓					创建
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "hnColorRamp.h"
//#include "../../3rd/opencv3.2.0/include/opencv/cv.hpp"
//#include "../../3rd/opencv3.2.0/include/opencv2/opencv.hpp"
//#include <utility>
#include <iostream>


//using namespace cv;
using namespace std;


namespace hnImageAlgorithm
{
	// 构造函数;
	hnColorRamp::hnColorRamp() : m_colorRamp(ENUM_BLUE_TO_RED_N)
	{
		//m_Node = new int[MAP_LEN];
		//m_Color = new Byte*[4];
		//for (int n = 0;n < 4;n++)
		//{
		//	Byte* ptr = m_Color[n];
		//	ptr = new Byte[MAP_LEN];
		//}
		m_NbNode = 0;
		m_Size = MAP_LEN;
		InitColor();
		BuildNodes();
		Build();
		//SaveImage();
	}

	// 构造函数;
	hnColorRamp::hnColorRamp(COLORRAMP_TYPE colorRamp) : m_colorRamp(colorRamp)
	{
		m_NbNode = 0;
		m_Size = MAP_LEN;

		InitColor();
		BuildNodes();
		Build();
		//SaveImage();
	}

	// 析构函数;
	hnColorRamp::~hnColorRamp()
	{
	}

	// 从彩色值返回灰度值;
	Byte hnColorRamp::Gray(Byte r, Byte g, Byte b)
	{
		unsigned int rgb = 1000000 * r + 1000 * g + b;
		int gray = m_rgb2gray[rgb];

		return ((Byte)gray);
	}

	// 初始化节点颜色;
	void hnColorRamp::InitColor()
	{
		memset(m_Color, 0, MAP_LEN * 4);

		if (m_colorRamp == ENUM_BLUE_TO_RED_N)
		{
			m_Color[3][0] = 1; m_Color[0][0] = 0; m_Color[1][0] = 60; m_Color[2][0] = 255;
			m_Color[3][104] = 1; m_Color[0][104] = 0; m_Color[1][104] = 180; m_Color[2][104] = 160;
			m_Color[3][120] = 1; m_Color[0][120] = 0; m_Color[1][120] = 180; m_Color[2][120] = 60;
			m_Color[3][128] = 1; m_Color[0][128] = 0; m_Color[1][128] = 220; m_Color[2][128] = 0;
			m_Color[3][136] = 1; m_Color[0][136] = 60; m_Color[1][136] = 180; m_Color[2][136] = 0;
			m_Color[3][152] = 1; m_Color[0][152] = 160; m_Color[1][152] = 180; m_Color[2][152] = 0;
			m_Color[3][255] = 1; m_Color[0][255] = 255; m_Color[1][255] = 60; m_Color[2][255] = 0;
		}
		else if (m_colorRamp == ENUM_GB_TO_RED_N)
		{
			//m_Color[3][0] = 1; m_Color[0][0] = 0; m_Color[1][0] = 60; m_Color[2][0] = 255;
			//m_Color[3][104] = 1; m_Color[0][104] = 0; m_Color[1][104] = 180; m_Color[2][104] = 160;
			//m_Color[3][120] = 1; m_Color[0][120] = 0; m_Color[1][120] = 180; m_Color[2][120] = 60;

			m_Color[3][0] = 1; m_Color[0][0] = 0; m_Color[1][0] = 60; m_Color[2][0] = 255;
			m_Color[3][30] = 1; m_Color[0][30] = 0; m_Color[1][30] = 90; m_Color[2][30] = 160;
			m_Color[3][60] = 1; m_Color[0][60] = 0; m_Color[1][60] = 120; m_Color[2][60] = 120;
			m_Color[3][90] = 1; m_Color[0][90] = 0; m_Color[1][90] = 150; m_Color[2][90] = 80;
			m_Color[3][112] = 1; m_Color[0][112] = 0; m_Color[1][112] = 150; m_Color[2][112] = 60;

			m_Color[3][128] = 1; m_Color[0][128] = 0; m_Color[1][128] = 0; m_Color[2][128] = 0;
			m_Color[3][140] = 1; m_Color[0][140] = 0; m_Color[1][140] = 0; m_Color[2][140] = 0;
			m_Color[3][145] = 1; m_Color[0][145] = 0; m_Color[1][145] = 0; m_Color[2][145] = 30;

			m_Color[3][255] = 1; m_Color[0][255] = 255; m_Color[1][255] = 0; m_Color[2][255] = 0;

			//m_Color[3][0] = 1; m_Color[0][0] = 255; m_Color[1][0] = 0; m_Color[2][0] = 0;
			//m_Color[3][30] = 1; m_Color[0][30] = 200; m_Color[1][30] = 30; m_Color[2][30] = 0;
			//m_Color[3][60] = 1; m_Color[0][60] = 160; m_Color[1][60] = 60; m_Color[2][60] = 0;
			//m_Color[3][90] = 1; m_Color[0][90] = 120; m_Color[1][90] = 90; m_Color[2][90] = 0;

			//m_Color[3][112] = 1; m_Color[0][112] = 60; m_Color[1][112] = 150; m_Color[2][112] = 60;

			//m_Color[3][125] = 1; m_Color[0][125] = 30; m_Color[1][125] = 180; m_Color[2][125] = 90;
			//m_Color[3][128] = 1; m_Color[0][128] = 30; m_Color[1][128] = 180; m_Color[2][128] = 120;
			////m_Color[3][128] = 1; m_Color[0][128] = 255; m_Color[1][128] = 255; m_Color[2][128] = 255;
			//m_Color[3][132] = 1; m_Color[0][132] = 0; m_Color[1][132] = 210; m_Color[2][132] = 150;
			//m_Color[3][170] = 1; m_Color[0][170] = 0; m_Color[1][170] = 80; m_Color[2][170] = 120;
			//m_Color[3][200] = 1; m_Color[0][200] = 0; m_Color[1][200] = 40; m_Color[2][200] = 280;

			//m_Color[3][255] = 1; m_Color[0][255] = 0; m_Color[1][255] = 0; m_Color[2][255] = 255;
		}
		else if (m_colorRamp == ENUM_RED_TO_BLUE_N)
		{
			m_Color[3][0] = 1; m_Color[0][0] = 255; m_Color[1][0] = 60; m_Color[2][0] = 0;
			m_Color[3][104] = 1; m_Color[0][104] = 160; m_Color[1][104] = 180; m_Color[2][104] = 0;
			m_Color[3][120] = 1; m_Color[0][120] = 60; m_Color[1][120] = 180; m_Color[2][120] = 0;
			m_Color[3][128] = 1; m_Color[0][128] = 0; m_Color[1][128] = 180; m_Color[2][128] = 0;
			m_Color[3][136] = 1; m_Color[0][136] = 0; m_Color[1][136] = 180; m_Color[2][136] = 60;
			m_Color[3][152] = 1; m_Color[0][152] = 0; m_Color[1][152] = 180; m_Color[2][152] = 160;
			m_Color[3][255] = 1; m_Color[0][255] = 0; m_Color[1][255] = 60; m_Color[2][255] = 255;
		}
		else if (m_colorRamp == ENUM_BLUE_TO_RED_Y)
		{
			m_Color[3][0] = 1; m_Color[0][0] = 0; m_Color[1][0] = 60; m_Color[2][0] = 255;
			m_Color[3][64] = 1; m_Color[0][64] = 0; m_Color[1][64] = 255; m_Color[2][64] = 255;
			m_Color[3][128] = 1; m_Color[0][128] = 0; m_Color[1][128] = 255; m_Color[2][128] = 0;
			m_Color[3][192] = 1; m_Color[0][192] = 255; m_Color[1][192] = 255; m_Color[2][192] = 0;
			m_Color[3][255] = 1; m_Color[0][255] = 255; m_Color[1][255] = 60; m_Color[2][255] = 0;
		}
		else if (m_colorRamp == ENUM_RED_TO_BLUE_Y)
		{
			m_Color[3][0] = 1; m_Color[0][0] = 255; m_Color[1][0] = 60; m_Color[2][0] = 0;
			m_Color[3][64] = 1; m_Color[0][64] = 255; m_Color[1][64] = 255; m_Color[2][64] = 0;
			m_Color[3][128] = 1; m_Color[0][128] = 0; m_Color[1][128] = 255; m_Color[2][128] = 0;
			m_Color[3][192] = 1; m_Color[0][192] = 0; m_Color[1][192] = 255; m_Color[2][192] = 255;
			m_Color[3][255] = 1; m_Color[0][255] = 0; m_Color[1][255] = 60; m_Color[2][255] = 255;
		}
	}

	// 获取每个渐变色节点;
	void hnColorRamp::BuildNodes()
	{
		m_Color[3][0] = 1;
		m_Color[3][m_Size - 1] = 1;

		m_NbNode = 0;
		for (int i = 0; i < m_Size; i++)
		{
			if (m_Color[3][i] == 1)
			{
				m_Node[m_NbNode] = i;
				m_NbNode++;
			}
		}
			
	}

	// 生成每个索引的颜色;
	void hnColorRamp::Build()
	{
		int x1, y1, x2, y2;
		double a, b;
		for (Byte k = 0; k < 3; k++)
		{
			for (int i = 0; i < m_NbNode - 1; i++)
			{
				x1 = (int)m_Node[i];
				x2 = (int)m_Node[i + 1];

				y1 = m_Color[k][x1];
				y2 = m_Color[k][x2];

				a = (double)(y2 - y1) / (double)(x2 - x1);
				b = (double)y1 - a * (double)x1;

				for (int j = x1; j < x2; j++)
					m_Color[k][j] = (Byte)(a * (double)j + b);
			}
		}

		// 构建rgb到gray的反映射表;
		unsigned int rgb;
		for (int i = 0; i < m_Size; ++i)
		{
			rgb = m_Color[0][i] * 1000000 + m_Color[1][i] * 1000 + m_Color[2][i];
			m_rgb2gray.insert(make_pair(rgb, i));
		}
	}

	//// 保存渐变色带
	//void hnColorRamp::SaveImage()
	//{
	//	Mat image(10, MAP_LEN, CV_8UC3);

	//	for (int i = 0; i < image.cols; i++)
	//	{
	//		for (int j = 0; j < image.rows; j++)
	//		{
	//			image.at<Vec3b>(j, i)[0] = m_Color[0][i];
	//			image.at<Vec3b>(j, i)[1] = m_Color[1][i];
	//			image.at<Vec3b>(j, i)[2] = m_Color[2][i];
	//		}
	//	}

	//	imwrite("ColorRamp.bmp", image);
	//	image.release();
	//}
}
