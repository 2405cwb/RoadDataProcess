#include "hnHighAcc2PlaneWrapper.h"


hnHighAcc2PlaneWrapper::hnHighAcc2PlaneWrapper(): m_impl(new hnHighAcc2Plane())
{

}

hnHighAcc2PlaneWrapper::~hnHighAcc2PlaneWrapper()
{
	delete m_impl;
}

void hnHighAcc2PlaneWrapper::initialParam(POS_CONVERT_INFO * paramInfo)
{
	m_impl->initialParam(paramInfo);
}

bool hnHighAcc2PlaneWrapper::convertBLHToProjection(double dL, double dB, double dH, double& dEast, double& dNorth, double& dHeight)
{
	return m_impl->convertBLHToProjection(dL, dB, dH, dEast, dNorth, dHeight);
}
