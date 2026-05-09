#include "ConstDef.h"
#include <string>

using namespace std;

// 定义全景站点轨迹文件数据结构
struct HD_HDIINFO	
{
	// 构造函数，赋初值
	HD_HDIINFO()
	{
		ReSet();
	}

	void ReSet()
	{
		//memset(strImageName, 0, OBJECT_ID_LEN);
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
		int nTmp;
		if (strLine != NULL)
		{
			int field = sscanf(strLine, "%s%d%d%d%d%d%d%d%d%lf%lf%lf%lf%lf%lf%lf%lf%f%f%f%d", 
				strImageName, &nCameraNo, &nTmp, 
				&nTmp, &nTmp, 
				&nTmp, &nTmp, 
				&nTmp, &nTmp,
				&dX, &dY, &dZ, &dL, &dB, &dYaw, &dPitch, &dRoll,
				&fVEast,&fVNorth,&fVUp,&nQuality);

			return field >= 17;
		}
		return false;
	}



	char		strImageName[OBJECT_ID_LEN];	// 影像唯一标识
	int			nCameraNo;					// 相机号
	/*HDTIME		tGatherTime;				// 采集时间*/
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