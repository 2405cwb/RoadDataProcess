#pragma once
#include "hnHighAcc2Plane.h"
#include "hnhighAccConvertPlane_global.h"
#include "..\hnQtCommon\HighAccuracyInfo.h"
class  HIGHACCCONVERTPLANE_EXPORT  hnHighAcc2PlaneWrapper
{
public:
	hnHighAcc2PlaneWrapper();
	~hnHighAcc2PlaneWrapper();
	void  initialParam(POS_CONVERT_INFO * paramInfo);
	bool  convertBLHToProjection(double dL, double dB, double dH, double& dEast, double& dNorth, double& dHeight);
protected:
private:
	hnHighAcc2Plane * m_impl;
}; 