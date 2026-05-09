//---------------------------------------------------------------------------
//	 emap_prjtrans_api.h
//   中海达：坐标投影转换
//
//
//---------------------------------------------------------------------------

#ifndef _EMAP_PRJTRANS_API_H_
#define _EMAP_PRJTRANS_API_H_

#include <math.h>
#include "emap_prjtrans_base.h"

//----------------------------------------------------------------------------
//#ifdef __cplusplus
//extern "C"
//{
//#endif
	//----------------------------------------------------------------------------
	// 大地坐标转为空间直角坐标(BLH->XYZ);
	void BtoX(double a, double f, double B, double L, double H, double *m_X, double *m_Y, double *m_Z);

	// 空间直角坐标转换为大地坐标(XYZ->BLH);
	void XtoB(double a, double f, double X, double Y, double Z, double *m_B, double *m_L, double *m_H);

	// 四参数平面转换;
	void xtox(ZHDFourPar fPar, double *x, double *y);

	// 四参数平面转换反算;
	void xtox_false(ZHDFourPar fPar, double *x, double *y);

	// 七参数正算;
	void XtoX_Bursa_Simple__(ZHDSevenPar sPar, double *X, double *Y, double *Z);

	// 七参数反算;
	void XtoX_Bursa_Simple_False__(ZHDSevenPar sPar,double *X, double *Y, double *Z);

	// 通常的高程拟合计算;
	void HFixCalus2(int Model, ZHDHFixPar par, double x, double y, double *dResult);

	/// <summary>
	/// 大地坐标转换为平面坐标
	/// </summary>
	/// <param name="nModel">转换模型</param>
	/// <param name="ea">椭球长半轴(m)</param>
	/// <param name="ef">椭球扁率的倒数</param>
	/// <param name="par">投影参数</param>
	/// <param name="dB">纬度(弧度)</param>
	/// <param name="dL">经度(弧度)</param>
	/// <param name="H">投影点高程(带投影面高投影的时候会修改)</param>
	/// <param name="dx">平面坐标X(m)</param>
	/// <param name="dy">平面坐标Y(m)</param>
	/// <param name="bNorth">北方向</param>
	/// <param name="bEast">东方向</param>
	void BLtoxy(int nModel, double ea, double ef, ZHDProjPars par, double dB, double dL, 
		double &H, double &dx, double &dy, bool bNorth, bool bEast);

	/// <summary>
	/// 平面坐标转换为大地坐标，即投影反算
	/// </summary>
	/// <param name="nModel">投影反算模型</param>
	/// <param name="ea">椭球长半轴(m)</param>
	/// <param name="ef">椭球扁率的倒数</param>
	/// <param name="pars">投影参数</param>
	/// <param name="dx">平面坐标X(m)</param>
	/// <param name="dy">平面坐标Y(m)</param>
	/// <param name="dB">纬度(弧度)</param>
	/// <param name="dL">经度(弧度)</param>
	/// <param name="bNorth">北方向</param>
	/// <param name="bEast">东方向</param>
	void xytoBL(int nModel, double ea, double ef, ZHDProjPars par, double dx, double dy, double dh, 
		double &dB, double &dL, double &dH, bool bNorth, bool bEast);

//	//----------------------------------------------------------------------------
//#ifdef __cplusplus
//}
//#endif
//----------------------------------------------------------------------------
#endif	//	_EMAP_PRJTRANS_API_H_
// EOF emap_prjtrans_api.h
