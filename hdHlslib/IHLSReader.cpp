#include "IHLSReader.h"
#include "..\hdCore\hdMatrix.h"

namespace hd
{
IHLSReader::IHLSReader(void)
{
	m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0;
	m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[4] =
	m_matrix[6] = m_matrix[7] = m_matrix[8] = m_matrix[9] =
	m_matrix[11] = m_matrix[12] = m_matrix[13] = m_matrix[14] = 0.0;
	m_pLoopIndex = NULL;
	memset(m_filepath,0,1024);
	m_dataType = E_HDT_HLS;
}


IHLSReader::~IHLSReader(void)
{
	if (m_pLoopIndex)
	{
		delete[] m_pLoopIndex;
		m_pLoopIndex = NULL;
	}
	m_matrix[0] = m_matrix[5] = m_matrix[10] = m_matrix[15] = 1.0;
	m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[4] =
	m_matrix[6] = m_matrix[7] = m_matrix[8] = m_matrix[9] =
	m_matrix[11] = m_matrix[12] = m_matrix[13] = m_matrix[14] = 0.0;
	memset(m_filepath,0,1024);
	
}

// 申请索引空间
inline CLoopIndex* IHLSReader::AllocLoopIndex(u32 count)
{
	if (m_pLoopIndex)
	{
		delete[] m_pLoopIndex;
		m_pLoopIndex = NULL;
	}

	// 进行内存分配失败异常捕捉 [蔡红云 2014/3/12]
	try
	{
		m_pLoopIndex = new CLoopIndex[count];
	}

	catch (...)
	{
		return NULL;
	}
	
	return m_pLoopIndex;
}

void IHLSReader::GetCoordinate( const F32& x,const F32& y,const F32& z, F64& outX,F64& outY,F64& outZ )
{
	// 将本地坐标转换为全局坐标
	double tmpX = x;
	double tmpY = y;
	double tmpZ = z;
	outX = m_matrix[0]*tmpX + m_matrix[1]*tmpY + m_matrix[2]*tmpZ + m_matrix[3];
	outY = m_matrix[4]*tmpX + m_matrix[5]*tmpY + m_matrix[6]*tmpZ + m_matrix[7];
	outZ = m_matrix[8]*tmpX + m_matrix[9]*tmpY + m_matrix[10]*tmpZ + m_matrix[11];
	double w = m_matrix[12]*tmpX + m_matrix[13]*tmpY + m_matrix[14]*tmpZ + m_matrix[15];

	double f = 1.0/w;
	outX = static_cast<double>(outX*f);
	outY = static_cast<double>(outY*f);
	outZ = static_cast<double>(outZ*f);
}

bool IHLSReader::AntiTranslate(float& x, float& y, float& z) const
{
	float tempX = x;
	float tempY = y;
	float tempZ = z;

	double antiR[9];
	antiR[0] = m_matrix[0];
	antiR[1] = m_matrix[1];
	antiR[2] = m_matrix[2];
	antiR[3] = m_matrix[4];
	antiR[4] = m_matrix[5];
	antiR[5] = m_matrix[6];
	antiR[6] = m_matrix[8];
	antiR[7] = m_matrix[9];
	antiR[8] = m_matrix[10];

	//memcpy(antiR, m_fRotateMatrix, 9*sizeof(double));
	invers_matrix(antiR, 3);

	double x1 = (tempX - m_matrix[3])/m_matrix[15];
	double y1 = (tempY - m_matrix[7])/m_matrix[15];
	double z1 = (tempZ - m_matrix[11])/m_matrix[15];

	x = (float)(antiR[0]*x1 + antiR[1]*y1 + antiR[2]*z1);
	y = (float)(antiR[3]*x1 + antiR[4]*y1 + antiR[5]*z1);
	z = (float)(antiR[6]*x1 + antiR[7]*y1 + antiR[8]*z1);

	return true;
}

bool IHLSReader::AntiTranslate(double& x, double& y, double& z) const
{
	double tempX = x;
	double tempY = y;
	double tempZ = z;

	double antiR[9];
	antiR[0] = m_matrix[0];
	antiR[1] = m_matrix[1];
	antiR[2] = m_matrix[2];
	antiR[3] = m_matrix[4];
	antiR[4] = m_matrix[5];
	antiR[5] = m_matrix[6];
	antiR[6] = m_matrix[8];
	antiR[7] = m_matrix[9];
	antiR[8] = m_matrix[10];

	//memcpy(antiR, m_fRotateMatrix, 9*sizeof(double));
	invers_matrix(antiR, 3);

	double x1 = (tempX - m_matrix[3])/m_matrix[15];
	double y1 = (tempY - m_matrix[7])/m_matrix[15];
	double z1 = (tempZ - m_matrix[11])/m_matrix[15];

	x = (antiR[0]*x1 + antiR[1]*y1 + antiR[2]*z1);
	y = (antiR[3]*x1 + antiR[4]*y1 + antiR[5]*z1);
	z = (antiR[6]*x1 + antiR[7]*y1 + antiR[8]*z1);

	return true;
}

void IHLSReader::GetCoordinate( F64& x,F64& y,F64& z )
{
	// 将本地坐标转换为全局坐标
	double tmpX = x;
	double tmpY = y;
	double tmpZ = z;
	x = m_matrix[0]*tmpX + m_matrix[1]*tmpY + m_matrix[2]*tmpZ + m_matrix[3];
	y = m_matrix[4]*tmpX + m_matrix[5]*tmpY + m_matrix[6]*tmpZ + m_matrix[7];
	z = m_matrix[8]*tmpX + m_matrix[9]*tmpY + m_matrix[10]*tmpZ + m_matrix[11];
	double w = m_matrix[12]*tmpX + m_matrix[13]*tmpY + m_matrix[14]*tmpZ + m_matrix[15];

	double f = 1.0/w;
	x = static_cast<double>(x*f);
	y = static_cast<double>(y*f);
	z = static_cast<double>(z*f);
}

void IHLSReader::ComputeMatrix()
{
	m_header.computeMatrix(m_matrix);
}

BOOL IHLSReader::CanUpdate()
{
	return m_header.get_pointformat() != HLS2_POINTFORMAT_RHVI &&
		   m_header.get_pointformat() != HLS_POINTFORMAT_RHVI;
}

}