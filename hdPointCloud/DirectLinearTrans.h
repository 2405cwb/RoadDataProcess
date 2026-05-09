#pragma once
#include "..\hdCommon\point_types.h"
#include "hdPointCloud.h"
#include <vector>

using namespace std;
using namespace hd;

typedef struct tagExteriorOrientation
{
	double Xs;
	double Ys;
	double Zs;
	double phi;
	double omega;
	double kappa;

}ExteriorOrientation;

typedef struct tagInteriorOrientation
{
	double x0;
	double y0;
	double fx;
	double fy;
	double dBeta;
	double ds;
}InteriorOrientation;

class HDPOINTCLOUD_API CDirectLinearTrans
{
public:
	CDirectLinearTrans(void);
	~CDirectLinearTrans(void);
public:
	////计算M矩阵
	void ComputeMMatrix(vector<PanoControlPoint> &controlPoints,int imgWidth, int imgHeight );
	//计算物方点XYZ对应的像素坐标 结果保存在controlPoints.imageX和controlPoints.imageY
	bool GetPixelByXYZ(PanoControlPoint &controlPoints);
	bool GetPixelByXYZ(double x,double y,double z,double& imageX,double& imageY);

	//根据M矩阵计算内外方位元素
	bool ComputeOrientation();
	InteriorOrientation getInteriorOritention(){return m_intOrt;}
    ExteriorOrientation getExteriorOrientation(){return m_extOrt;}

	double* getM(){return M;}
	void setM(double* _M)
	{
		memcpy(M, _M, 12*sizeof(double));
		m_bMComputed = true;
		ComputeOrientation();

	}
private:
	double M[12];
	double m_k1;
	int m_imgWidth;
	int m_imgHeight;
	ExteriorOrientation m_extOrt;//外方位元素
	InteriorOrientation m_intOrt;//内方位元素
public:
	bool m_bMComputed;
};

