#include "StdAfx.h"
#include "hnComputeIRI.h"


hnComputeIRI::hnComputeIRI(void)
{
	m_sZU[0] = 0.9966071;
	m_sZU[1] = 1.091514e-02;
	m_sZU[2] = -2.083274e-03;
	m_sZU[3] = 3.190145e-04;
	m_sZU[4] = -0.5563044;
	m_sZU[5] = 0.9438768;
	m_sZU[6] = -0.8324718;
	m_sZU[7] = 5.064701e-02;
	m_sZU[8] = 2.153176e-02;
    m_sZU[9] = 2.126763e-03;
    m_sZU[10] = 0.7508714;
    m_sZU[11] = 8.221888e-03;
	m_sZU[12] = 3.335013;
	m_sZU[13] = 0.3376467;
	m_sZU[14] = -39.12762;
	m_sZU[15] = 0.4347664;

	// m_pZU;
	m_pZU[0] = 5.47610e-03;
	m_pZU[1] = 1.388776;
	m_pZU[2] = 0.2275968;
	m_pZU[3] = 35.79262;

	// m_zSU
    m_zSU[0] = m_zSU[1] = m_zSU[2] = m_zSU[3] = 0.0;

	// m_oldZSU
	m_oldZSU[0] = m_oldZSU[1] = m_oldZSU[2] = m_oldZSU[3] = 0.0;
} 


hnComputeIRI::~hnComputeIRI(void)
{
}


// 计算IRI
void hnComputeIRI::calculateIRI(double dIntervel, vector<double> listIRIInfo, double& irival,
	double DeltLen/* = 0.25*/)
{

	DeltLen = 0.25;
	//计算10m的平整度
	double irisum = 0.0;
	int plusenum = (int)(dIntervel / DeltLen);


	double YSU = 0.0;
	m_oldZSU[0] = listIRIInfo[1] - listIRIInfo[0];
	//m_oldZSU[1] = listIRIInfo[1] - listIRIInfo[0];
	//m_oldZSU[2] = 0;
	//m_oldZSU[3] = 0;*/
	//iridata: 250mm采样间距纵断面
	//for (int i = 1; i < listIRIInfo.size(); ++i)
	//{
	//	//YSU = (listIRIInfo[i] - listIRIInfo[i - 1]) / DeltLen;
	//	for (int zi = 0; zi < 4; ++zi)
	//	{
	//		m_zSU[zi] = 0;
	//		for (int zj = 0; zj < 4; ++zj)
	//		{
	//			m_zSU[zi] += m_sZU[zi * 4 + zj] * m_oldZSU[zj];
	//		}
	//		m_zSU[zi] += m_pZU[zi] * YSU;
	//	}
	//	double valueTemp = abs(m_zSU[0] - m_zSU[2]);
	//	irisum += valueTemp;

	//	for (int zi = 0; zi < 4; ++zi)
	//	{
	//		m_oldZSU[zi] = m_zSU[zi];
	//	}
	//}

	//irival = irisum / plusenum;
}
