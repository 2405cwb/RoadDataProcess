#ifndef _HN_COORD_SYSTEM_PROJ_API_H_
#define _HN_COORD_SYSTEM_PROJ_API_H_
#include "hnPositionProj.h"
namespace hn
{
	class HNPOSITIONPROJ_API hnPositionApi
	{
	public:
		hnPositionApi(void);
		~hnPositionApi(void);

	public:
		/*!
		* @brief 封装基本的坐标转换接口，wgs84经纬度转高斯投影，此接口作为调用示例，建议直接调用IHdPJTranslator对象
		* @details 
		* @param[in] B 纬度
		* @param[in] L 经度
		* @param[in] H 高
		* @param[in] centre 中央经线
		* @param[out] proj_x 输出投影坐标X
		* @param[out] proj_y 输出投影坐标Y
		* @param[out] proj_z 输出投影坐标Z
		* @param[in] proj_z 输入带宽，即确定为三度带还是六度带
		* @param[in] east_offset 输入东向偏移
		*/
		void wgs84BlhToGaussPrjXyz(double B,double L,double H,double centre,double& proj_x,double& proj_y,double& proj_z, int w=3,double east_offset = 500000);

		/*!
		* @brief 封装基本的坐标转换接口，wgs84高斯投影转经纬度，此接口作为调用示例，建议直接调用IHdPJTranslator对象
		* @details 
		* @param[in] proj_x 投影东向坐标
		* @param[in] proj_y 投影北向坐标
		* @param[in] proj_z 投影高
		* @param[in] centre 中央经线
		* @param[out] B 输出纬度，单位为度
		* @param[out] L 输出经度，单位为度
		* @param[out] H 输出高
		* @param[in] w 输入带宽，即确定为三度带还是六度带
		* @param[in] east_offset 输入东向偏移
		*/
		void wgs84GaussPrjXyzToBlh(double proj_x,double proj_y,double proj_z,double centre,double& B,double& L,double& H,int w=3,double east_offset = 500000);

	};
}
#endif