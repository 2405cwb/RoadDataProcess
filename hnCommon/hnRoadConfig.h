#pragma once
#include "stdafx.h"
namespace hnCommon {
	class HNCOMMOMAPI hnRoadConfig
	{
	public:

		~hnRoadConfig();
		static hnRoadConfig* getInstance();
	public:
		void readData();
		void writeData();
	private:
		hnRoadConfig();
		hnRoadConfig(const hnRoadConfig&);
		static hnRoadConfig* _hnRoadConfig;
	public:
		/// <summary>
		/// 图像横向分辨率X，像素
		/// </summary>
		int ImageWidth;
		/// <summary>
		/// 图像纵向分辨率Y，像素
		/// </summary>
		int ImageHeight;

		/// <summary>
		/// 路面图像真实宽度X，m
		/// </summary>
		double RealWidth;
		/// <summary>
		/// 路面图像真实高度Y，m
		/// </summary>
		double RealHeight;

		/// <summary>
		/// 路面检测宽度，m
		/// </summary>
		double DetectWidth;

		/// <summary>
		/// 宽度方向像素分辨率，m
		/// </summary>
		double WidthScale;

		/// <summary>
		/// 高度方向像素分辨率，m
		/// </summary>
		double HeightScale;

		/// <summary>
		/// 全幅照片宽度方向0.1m的小方格数量，RealWidth / 0.1
		/// </summary>
		int PartWidthNum;

		/// <summary>
		/// 全幅照片高度方向0.1m的小方格数量，RealHeight / 0.1
		/// </summary>
		int PartHeightNum;

		/// <summary>
		/// 小方格宽度方向占的像素数，ImageWidth * 1.0 / PartWidthNum
		/// </summary>
		int PartImgWidth;

		/// <summary>
		/// 小方格高度方向占的像素数，ImageHeight * 1.0 / PartHeightNum
		/// </summary>
		int PartImgHeight;

	};

}