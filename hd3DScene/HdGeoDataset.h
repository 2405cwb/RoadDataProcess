/*! HdGeoDataset.h
********************************************************************************
<PRE>
模块名       : Hd3DScene
文件名       : HdGeoDataset.h
相关文件     : 
文件实现功能 : 定义带地理信息的数据集基类
作者         : 朱立雄
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/10/17   1.0      朱立雄                创建
</PRE>
*******************************************************************************/
#ifndef HD3DSCENE_HDGEODATASET_H
#define HD3DSCENE_HDGEODATASET_H

#include "stdafx.h"

class OGRSpatialReference;

namespace hd
{
	namespace scene
	{
		class CHdSpatialReference;
		class CHdGeoEnvelope;

		// 包含地理信息的数据集基类
		class HD3DSCENE_API CHdGeoDataset
		{
		public:
			CHdGeoDataset(){}
			virtual~ CHdGeoDataset(){}
		private:
			CHdGeoDataset(const CHdGeoDataset&);
			CHdGeoDataset& operator= (const CHdGeoDataset&);

		public:
			virtual const CHdSpatialReference* GetSpatialReference() const = 0;
			virtual const CHdGeoEnvelope* GetGeoEnvelope() const = 0;

		};

		// 空间坐标系
		class HD3DSCENE_API CHdSpatialReference
		{
		public:
			CHdSpatialReference();
			virtual~ CHdSpatialReference();
		private:
			CHdSpatialReference(const CHdSpatialReference&);
			CHdSpatialReference& operator= (const CHdSpatialReference&);

		public:
			void CreateFromWKT(const char* pcWkt);
			void CreateFromEPSG(int epsg);
			
			bool IsGeographic() const;
			bool IsProjected() const;
			bool IsGeocentric() const;

			bool IsSame(const CHdSpatialReference& anotherSR) const;

			CHdSpatialReference* Clone() const;

		private:
			OGRSpatialReference* m_pOgrSR;
		};

		// 地理范围
		class HD3DSCENE_API CHdGeoEnvelope
		{
		public:
			CHdGeoEnvelope(){}
			CHdGeoEnvelope(double dfMinX, double dfMaxX, double dfMinY, double dfMaxY)
			{
				m_dfMinX = dfMinX;  m_dfMaxX = dfMaxX;
				m_dfMinY = dfMinY;  m_dfMaxY = dfMaxY;
			}
			virtual~ CHdGeoEnvelope(){}

			double GetMinX() const{return m_dfMinX;}
			double GetMaxX() const{return m_dfMaxX;}
			double GetMinY() const{return m_dfMinY;}
			double GetMaxY() const{return m_dfMaxY;}

		private:
			double m_dfMinX, m_dfMaxX, m_dfMinY, m_dfMaxY;
		};
	}
}

#endif