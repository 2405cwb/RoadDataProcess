/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdCommon
文件名		：hdHdiStruct.h
相关文件	: hdHdiStruct.cpp
文件实现功能：定义轨迹索引文件结构。
作者		：杨峰
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2012/2/27	1.0			杨峰		创建
2012/2/28	1.0			龚书林		增加扫描轨迹
</PRE>
******************************************************************************************************/

#pragma once
#include "hdCommon.h"
#include <vector>
#include "hdConstDef.h"
#include "..\hdCore\hdTime.h"

using namespace std;
namespace hd
{
	// 定义全景站点轨迹文件数据结构
	struct HD_HDIINFO	//HDCOMMON_API
	{
		// 构造函数，赋初值
		HD_HDIINFO()
		{
			ReSet();
		}

		void ReSet()
		{
			memset(strImageName, 0, PANO_ID_LEN);
			nCameraNo = 0;
			dX = dY = dZ = 0.0;
			dB = dL = 0.0;
			dYaw = dPitch = dRoll = 0.0;
			fVEast = fVNorth = fVUp = 0.f;
			nQuality = 0;

			fX_Param = fY_Param = fZ_Param = 0.0;			// x、y、z坐标调整参数
			fYaw_Param = fPitch_Param = fRoll_Param = 0.0;	// 航向角、俯仰角、翻滚角调整参数
		}

		// 序列化，从文件中的一行解析数据
		bool Serialize(const char* strLine)
		{
			ReSet();
			if (strLine != NULL)
			{
				int field = sscanf(strLine, "%s%d%d%d%d%d%d%d%d%lf%lf%lf%lf%lf%lf%lf%lf%f%f%f%d", 
					strImageName, &nCameraNo, &tGatherTime.year, 
					&tGatherTime.month, &tGatherTime.day, 
					&tGatherTime.hour, &tGatherTime.minute, 
					&tGatherTime.second, &tGatherTime.milliSecond,
					&dX, &dY, &dZ, &dL, &dB, &dYaw, &dPitch, &dRoll,
					&fVEast,&fVNorth,&fVUp,&nQuality);

				return field >= 17;
			}
			return false;
		}

		// 解析hdi文件到内存vector数组.gsl-2013/9/22
		static void Serialize(const char* linPath,std::vector<HD_HDIINFO>& vecHdi)
		{
			if(linPath == NULL)
				return;
			FILE* fp = fopen(linPath,"r");
			if (fp == NULL)
			{
				return ;
			}

			vecHdi.clear();
			char strLine[1024] = {0};
			////第一行跳过
			//fgets(strLine,1024,fp);
			// 每次申请5000个记录;
			int bufSize = 5000;
			vecHdi.resize(bufSize);
			int loopCount = 0;
			// 解析所有行获取hdi;
			while(!feof(fp))
			{
				memset(strLine,0,1024);
				fgets(strLine,1024,fp);
				HD_HDIINFO& hdi = vecHdi.at(loopCount);
				if (hdi.Serialize(strLine))
				{
					loopCount++;
					if (loopCount >= (int)vecHdi.size())
					{
						vecHdi.resize(vecHdi.size() + bufSize);
					}
				}
			}
			fclose(fp);
			fp = NULL;
			vecHdi.resize(loopCount);
		}

		// 判断对象是否在球体内
		bool IsInSphere(double _dX, double _dY, double _dZ, double _dR)
		{
			double dist = (dX - _dX)*(dX - _dX) + (dY - _dY)*(dY - _dY);
			if (dist <= _dR*_dR)
			{
				return true;
			}

			return false;
		}

		char		strImageName[PANO_ID_LEN];	// 影像唯一标识
		int			nCameraNo;					// 相机号
		HDTIME		tGatherTime;				// 采集时间
		double		dX;							// x坐标
		double 		dY;							// y坐标
		double 		dZ;							// z坐标
		double 		dB;							// 纬度
		double 		dL;							// 经度
		double		dYaw;						// 相机航向角
		double		dPitch;						// 相机俯仰角
		double		dRoll;						// 相机翻滚角
		
		float		fVEast;						// 东向运动速度
		float		fVNorth;					// 北向运动速度
		float		fVUp;						// 运动速度
		int			nQuality;					// GPS结算质量

		float		fX_Param;					// x坐标调整参数
		float		fY_Param;					// y坐标调整参数
		float		fZ_Param;					// z坐标调整参数
		float		fYaw_Param;					// 相机航向角调整参数
		float		fPitch_Param;				// 相机俯仰角调整参数
		float		fRoll_Param;				// 相机翻滚角调整参数
	};

	// 扫描轨迹线结构体
	struct HD_SCANHDIINFO //HDCOMMON_API
	{
		// 构造函数，赋初值
		HD_SCANHDIINFO()
		{
			
		}

		void ReSet()
		{
			iNo = -1;
			dGpsSecond = 0.0;
			dX = dY = dZ = 0.0;
			dB = dL = 0.0;
			dYaw = dPitch = dRoll = 0.0;
		}
		// 序列化，从文件中的一行解析数据
		bool Serialize(const char* strLine)
		{
			ReSet();
			if (strLine != NULL)
			{
				int field = sscanf_s(strLine, "%d%lf%d%d%d%d%d%d%d%lf%lf%lf%lf%lf%lf", //%lf%lf
					&iNo,&dGpsSecond, &tScanTime.year, 
					&tScanTime.month, &tScanTime.day, 
					&tScanTime.hour, &tScanTime.minute, 
					&tScanTime.second, &tScanTime.milliSecond,
					&dX, &dY, &dZ, &dYaw, &dPitch, &dRoll);//&dL,&dB, 
				return field >= 15;
			}
			return false;
		}

		// 解析lin文件到内存vector数组.gsl-2013/9/22
		static void Serialize(const char* linPath,std::vector<HD_SCANHDIINFO>& vecScanHdi)
		{
			if(linPath == NULL)
				return;
			FILE* fp = fopen(linPath,"r");
			if (fp == NULL)
			{
				return ;
			}

			vecScanHdi.clear();
			char strLine[1024] = {0};
			//第一行跳过
			fgets(strLine,1024,fp);
			// 每次申请5000个记录
			int bufSize = 5000;
			vecScanHdi.resize(bufSize);
			int loopCount = 0;
			// 解析所有行获取scan hdi
			while(!feof(fp))
			{
				memset(strLine,0,1024);
				fgets(strLine,1024,fp);
				HD_SCANHDIINFO& hdi = vecScanHdi.at(loopCount);
				if (hdi.Serialize(strLine))
				{
					loopCount++;
					if (loopCount >= (int)vecScanHdi.size())
					{
						vecScanHdi.resize(vecScanHdi.size() + bufSize);
					}
				}
			}
			fclose(fp);
			fp = NULL;
			vecScanHdi.resize(loopCount);
		}

		int         iNo;						// 扫描圈
		double		dGpsSecond;					// 采集时间GPS秒
		HDTIME		tScanTime;					// 采集时间,GPS时间对应的年月日时分秒毫秒
		double		dX;							// x坐标
		double 		dY;							// y坐标
		double 		dZ;							// z坐标
		double 		dB;							// 纬度
		double 		dL;							// 精度
		double		dYaw;						// 相机航向角
		double		dPitch;						// 相机俯仰角
		double		dRoll;						// 相机翻滚角

		//float		fVEast;						// 东向运动速度
		//float		fVNorth;					// 北向运动速度
		//float		fVUp;						// 运动速度
		//int			nQuality;					// GPS结算质量
	};

	// 定义存储va dom影像信息tfw文件的相关信息
	struct HD_VA_TFW_PARA
	{
		// 构造函数，赋初值
		HD_VA_TFW_PARA()
		{

		}

		void ReSet()
		{
			dStepX = 0.0;
			dRotateX = 0.0;
			dRotateY = 0.0;
			dStepY = 0.0;
			dLeftUpCoordX = 0.0;
			dLeftUpCoordY = 0.0;
		}

		// 序列化tfw文件
		bool Serialize(const char* strPath)
		{
			// 路径判断，打开文件读取
			if(strPath == NULL)
				return false;
			FILE* fp = fopen(strPath,"r");
			if (fp == NULL)
			{
				return false;
			}

			// 初始化
			ReSet();

			// 读取信息
			strcpy(strTfwPath,strPath);  // 拷贝路径
			fscanf_s(fp,"%lf\n",&dStepX);
			fscanf_s(fp,"%lf\n",&dRotateX);
			fscanf_s(fp,"%lf\n",&dRotateY);
			fscanf_s(fp,"%lf\n",&dStepY);
			fscanf_s(fp,"%lf\n",&dLeftUpCoordX);
			fscanf_s(fp,"%lf\n",&dLeftUpCoordY);
			fclose(fp);
			fp = NULL;

			return true;
		}

		// 定义相关参数
		char strTfwPath[512]; // 记录路径名
		double dStepX;        // 横向坐标分辨率
		double dRotateX;      // 旋转量
		double dRotateY;      // 旋转量
		double dStepY;        // 纵向坐标分辨率
		double dLeftUpCoordY; // 影像左上角大地坐标Y，不含投影带号
		double dLeftUpCoordX; // 影像左上角大地坐标X
	};
}