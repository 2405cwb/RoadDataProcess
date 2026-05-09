#pragma once
#include "hdBasicObject.h"
#include "BaseStruct.h"
using namespace std;

class BASICOBJECT_API CSkitchZone
{
public:
	CSkitchZone(void);
	~CSkitchZone(void);

	// 构造函数，传入原始影像的参数
	void SetSrcImagePath(const char* strSrcImagePath)
	{
		m_strSrcImagePath = strSrcImagePath;
	}

	// 获取原始图像路径
	string GetSrcImagePath() const
	{
		return m_strSrcImagePath;
	}

	// 得到拼接线的基本信息
	ImageExtInfoSPtr GetExtInfo() const
	{
		return m_pImageInfo;
	}

	// 得到拼接线的基本信息
	void SetExtInfo(ImageExtInfoSPtr pExtInfo)
	{
		m_pImageInfo = pExtInfo;
	}

	// 根据坐标值得到对应的值,如果得到了，返回0或1，没有得到，返回-1
	int GetValue(double dx,double dy);

	// 根据坐标值得到对应的值,如果得到了，返回0或1，没有得到，返回-1,并得到对应的行列号
	int GetValue(double dx,double dy,int& nW,int& nH);

	// 申请内存
	void AllocData();

	// 释放数据
	void ReleaseData();

	bool* GetData() const
	{
		return pBitData;
	}
private:
	ImageExtInfoSPtr m_pImageInfo;							// 拼接线的基本信息
	string m_strSrcImagePath;								// 关联的原始影像路径
	bool* pBitData;											// 拼接线数据
};

// 定义智能指针
typedef shared_ptr<CSkitchZone> CSkitchZonePtr;