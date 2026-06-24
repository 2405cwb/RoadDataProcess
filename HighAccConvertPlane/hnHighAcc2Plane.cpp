#include "hnHighAcc2Plane.h"

void hnHighAcc2Plane::initialParam(POS_CONVERT_INFO* paramInfo)
{
	// 设置原始数据属性;
	memset(&m_src_param, 0, sizeof(m_src_param));
	m_src_param.coorSystem = E_COOR_SYSTEM_TYPE_GEO;
	m_src_param.coorUnit = E_COOR_UNIT_TYPE_DEGREE;
	m_src_param.earthType = E_EARTH_TYPE_WGS84;

	// 设置目标数据属性;
	memset(&m_dst_param, 0, sizeof(m_dst_param));
	m_dst_param.coorSystem = E_COOR_SYSTEM_TYPE_PRJ;
	m_dst_param.coorUnit = E_COOR_UNIT_TYPE_METER;

	// 目标数据对象目标椭球体;
	switch (paramInfo->nSphereType)
	{
	case 0:
	{
		m_dst_param.earthType = E_EARTH_TYPE_Beijing54;
		break;
	}

	case 1:
	{
		m_dst_param.earthType = E_EARTH_TYPE_Xian80;
		break;
	}
	case 2:
	{
		m_dst_param.earthType = E_EARTH_TYPE_WGS84;
		break;
	}
	case 3:
	{
		m_dst_param.earthType = E_EARTH_TYPE_China2000;
		break;
	}
	}

	// 坐标投影方法;
	switch (paramInfo->nProjectType)
	{
	case 0:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		m_dst_param.W = 3;
		break;
	}
	case 1:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Gauss_Kruger;
		m_dst_param.W = 6;
		break;
	}
	case 2:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_Mercator;
		m_dst_param.W = 3;
		break;
	}
	case 3:
	{
		m_dst_param.prjType = E_PROJECT_TYPE_UTM;
		m_dst_param.W = 3;
		break;
	}
	}

	// 中央经线等设置;
	m_dst_param.Lo = paramInfo->dCenterL * PI64 / 180.0;
	m_dst_param.Ko = paramInfo->dProjectScale;
	m_dst_param.FE = paramInfo->dEastAdd;
	m_dst_param.Bc = paramInfo->dAverageLat;
	m_dst_param.PH = paramInfo->dProjectHeight;

	// 构建对象;
	CreateIHdPJTranslator(&m_ptr_convert_translator);
	m_ptr_convert_translator->SetSrcSpatialRef(&m_src_param);
	m_ptr_convert_translator->SetDstSpatialRef(&m_dst_param);

	switch (paramInfo->nUseConvertModel)
	{
	case 1:
	{
		E_PJTFourPar_T pj_four_param;
		pj_four_param.Dx = paramInfo->dFourX;
		pj_four_param.Dy = paramInfo->dFourY;
		pj_four_param.T = paramInfo->dFourR * PI64 / (180.0 * 3600.0);
		pj_four_param.K = paramInfo->dFourK;
		m_ptr_convert_translator->SetFourParam(1, &pj_four_param);
		break;
	}
	case 2:
	{
		// 为七参数转换模型;
		E_PJTSevenPar_T pj_seven_param;
		pj_seven_param.DX = paramInfo->dOffsetX;
		pj_seven_param.DY = paramInfo->dOffsetY;
		pj_seven_param.DZ = paramInfo->dOffsetZ;
		pj_seven_param.WX = paramInfo->dRotateX * PI64 / (180.0 * 3600.0);//  * PI64 /(180.0 * 3600.0)
		pj_seven_param.WY = paramInfo->dRotateY * PI64 / (180.0 * 3600.0);
		pj_seven_param.WZ = paramInfo->dRotateZ * PI64 / (180.0 * 3600.0);
		pj_seven_param.K = paramInfo->dK;

		m_ptr_convert_translator->SetSevenParam(2, &pj_seven_param);
		break;
	}
	}
}

bool hnHighAcc2Plane::convertBLHToProjection(double dL, double dB, double dH, double& dEast, double& dNorth, double& dHeight)
{
	if (!m_ptr_convert_translator)
	{
		return false;
	}

	double out_x, out_y, out_z;
	m_ptr_convert_translator->TransLators_BLToXYZ(dB, dL, dH, &out_x, &out_y, &out_z);
	//m_ptr_convert_translator->Translator(dB, dL, dH, &out_x, &out_y, &out_z);
	m_ptr_convert_translator->TranslatorXYZByParam(out_x, out_y, out_z, &dEast, &dNorth, &dHeight);
	return true;
}
