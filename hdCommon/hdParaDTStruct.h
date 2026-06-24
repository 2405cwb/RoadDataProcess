/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdParaDTStruct.h
相关文件	: hdCommon.h、hdConstDef.h
文件实现功能：定义全景影像、全景参数、扫描仪参数数据库表结构。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/2/27	1.0			杨峰		创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include "hdConstDef.h"
#include "..\hdCore\hdTime.h"

namespace hd
{
	// 全景影像 数据表结构
	struct HD_IMAGEDATA
	{
		// 构造函数，赋初值
		HD_IMAGEDATA()
		{
			memset(strImageID, 0, PANO_ID_LEN);
			pImageData = NULL;
			nSize = 0;
			pBlurAreaData = NULL;
			nBadSize = 0;
		}

		// 析构函数，清空内存
		~HD_IMAGEDATA()
		{
			if (pImageData)
			{
				delete []pImageData;
				pImageData = NULL;
			}
			if (pBlurAreaData)
			{
				delete []pBlurAreaData;
				pBlurAreaData = NULL;
			}
		}

		// 分配影像内存大小
		void AllocSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pImageData = new BYTE[nBytes];
				memset(pImageData, 0, nBytes);
				nSize = nBytes;
			}
		}

		// 分配影像模糊区域内存大小
		void AllocBadSize(long nBytes)
		{
			if (nBytes > 0)
			{
				pBlurAreaData = new BYTE[nBytes];
				memset(pBlurAreaData, 0, nBytes);
				nBadSize = nBytes;
			}
		}
		
		char		strImageID[PANO_ID_LEN];	// 影像唯一标识
		BYTE*		pImageData;					// 影像文件
		long		nSize;						// 影像文件所占内存大小
		BYTE*		pBlurAreaData;				// 影像模糊区域数据
		long		nBadSize;					// 影像模糊区域数据所占内存大小
		// ********************影像模糊区域数据说明*********************************
		// 格式: nCout|nType|fDigtal1|fDigtal2|......
		// nCount: 模糊区域个数 int
		// nType: 模糊区域的类型 int，其中，nType=0代表矩形，nType=1代表圆
		// nDigtal1,nDigtal2:模糊区域的坐标 int， 
		// ***************** 当nType=0时，fDigtal1为矩形左下角像素坐标 fDigtal2为矩形右上角像素坐标
		// ***************** 当nType=1时，fDigtal1为圆心像素坐标 fDigtal2为半径
	};

	// iScan检校记录 数据表结构
	struct HD_ISCANPARA
	{
		// 构造函数，赋初值
		HD_ISCANPARA()
		{
			memset(striScanNo, 0, DEVICE_NO_LEN);
			memset(strMemoInfo, 0, REMARK_LEN);
		}

		char		striScanNo[DEVICE_NO_LEN];	// iScan系统编号
		HDTIME		tParaTime;					// 参数检校时间
		char		strMemoInfo[REMARK_LEN];	// 备注
	};

	// 全景相机检校参数 数据表结构
	struct HD_ISCANPANO_PARA
	{
		// 构造函数，赋初值
		HD_ISCANPANO_PARA()
		{
			memset(striScanNo, 0, DEVICE_NO_LEN);
			memset(striScanPanoNo, 0, SENSE_NO_LEN);
			dX = dY = dZ = 0.0f;
			dYaw = dPitch = dRoll = 0.0f;
			nWidth = nHeight = 0;
			dHoriStartAngle = 0.0f;
			dHoriEndAngle = 360.0f;
			dVertStartAngle = 90.0f;
			dVertEndAngle = -90.0f;
			memset(strMemoInfo, 0, REMARK_LEN);
		}

		char		striScanNo[DEVICE_NO_LEN];		// iScan系统编号
		char		striScanPanoNo[SENSE_NO_LEN];	// 全景相机编号
		HDTIME		tParaTime;						// 参数检校时间
		double		dX;								// 相对位置X
		double		dY;								// 相对位置Y
		double		dZ;								// 相对位置Z
		double		dYaw;							// X轴夹角
		double		dPitch;							// Y轴夹角
		double		dRoll;							// Z轴夹角
		int			nWidth;							// 影像分辨率宽度
		int			nHeight;						// 影像分辨率高度
		double		dHoriStartAngle;				// 水平起始角，默认0.0
		double		dHoriEndAngle;					// 水平结束角，默认360.0
		double		dVertStartAngle;				// 垂直起始角，默认90.0
		double		dVertEndAngle;					// 垂直结束角，默认-90.0
		char		strMemoInfo[REMARK_LEN];		// 备注
	};

	// 单镜头相机内参参数表结构
	struct HD_ISCAN_MZ_IMAGEINNER_PARA
	{
		// 参数初始化
		HD_ISCAN_MZ_IMAGEINNER_PARA()
		{
			type = 1;
			fx = 4187.934952;
			fy = 4189.726750;
			cx = 3664.696590;
			cy = 2434.354557;
			k1 = 0.350193;
			k2 = -11.124644;
			k3 = 23.537792;
			k4 = 0.527588;
			k5 = -11.793853;
			k6 = 24.534852;
			p1 = -0.000996;
			p2 = -0.000229;  

			fArmX = 0.016405;
			fArmY = -0.011660;
			fArmZ = 0.147287;
			fRPhi = -1.403450;
			fROmg = 0.003103;
			fRKap = 3.132507;

			lmd1 = lmd2 = lmd3 = 0;
			//type = 0;
			//fx = fy = cx = cy = k1 = k2 = k3 = k4 = k5 = k6 = p1 = p2 = lmd1 = lmd2 = lmd3 = 0;
			//fArmX = fArmY = fArmZ = fRPhi = fROmg = fRKap = 0;

			// 添加扫描头到pos的标定姿态
			fOffsetX = 0.055260;
			fOffsetY = 0.084715;
			fOffsetZ = 0.289917;
			fAngleX = 0.021002;
			fAngleY = -0.019546;
			fAngleZ = -0.342863;
		}

		// 类型 0-ZFI内置相机 1-HDSY D800 14mm广角 2-HDSY D800 鱼眼
		int type;

		// fx
		double fx;

		// fy
		double fy;

		// cx
		double cx;

		// cy
		double cy;

		// k1
		double k1;

		// k2
		double k2;

		// k3
		double k3;

		// k4
		double k4;

		// k5
		double k5;

		// k6
		double k6;

		// p1
		double p1;

		// p2
		double p2;

		// lmd1
		double lmd1;

		// lmd2
		double lmd2;

		// lmd3
		double lmd3;

		// armX
		double fArmX;

		// armY
		double fArmY;

		// armZ
		double fArmZ;

		// phi
		double fRPhi;

		// omega
		double fROmg;

		// kappa
		double fRKap;

		// 参数检校时间
		HDTIME		tParaTime;

		// 添加扫描头到pos的位置姿态关系
		double fOffsetX;
		double fOffsetY;
		double fOffsetZ;
		double fAngleX;
		double fAngleY;
		double fAngleZ;
	};

	// 激光扫描仪检校参数 数据表结构
	struct HD_ISCANLIDAR_PARA
	{
		// 构造函数，赋初值
		HD_ISCANLIDAR_PARA()
		{
			memset(striScanNo, 0, DEVICE_NO_LEN);
			memset(striScanLidarNo, 0, SENSE_NO_LEN);
			dX = dY = dZ = 0.0f;
			dYaw = dPitch = dRoll = 0.0f;
			memset(strMemoInfo, 0, REMARK_LEN);
		}

		char		striScanNo[DEVICE_NO_LEN];		// iScan系统编号
		char		striScanLidarNo[SENSE_NO_LEN];	// 传感器编号
		HDTIME		tParaTime;						// 参数检校时间
		double		dX;								// 相对位置X
		double		dY;								// 相对位置Y
		double		dZ;								// 相对位置Z
		double		dYaw;							// X轴夹角
		double		dPitch;							// Y轴夹角
		double		dRoll;							// Z轴夹角
		char		strMemoInfo[REMARK_LEN];		// 备注
	};
}