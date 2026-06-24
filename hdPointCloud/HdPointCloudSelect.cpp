/*!@file
*******************************************************************************************************
<PRE>
模块名		：PointSelect
文件名		：PointCloudSelected.cpp
相关文件	: PointCloudSelected.h
文件实现功能：按矢量范围选中点云
作者		：张恒
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2016/02/17	1.0			张恒		创建
</PRE>
******************************************************************************************************/

#include "StdAfx.h"
#include "HdPointCloudSelect.h"
#include "..\hdHlslib\HLSReader.h"

#include "ogr_api.h"
#include "gdal.h"
#include "gdal_alg.h"
#include "gdal_priv.h"

namespace hd
{

	PointCloudSelected::PointCloudSelected(void)
	{

	}

	PointCloudSelected::~PointCloudSelected(void)
	{
	}

	void PointCloudSelected::SelectPtByUnionVector(string vectorPath, PointCloud* pcd, double dfDist)
	{
		OGRGeometry* bufferGeo = UnionBuffer(vectorPath, dfDist);
		if (bufferGeo == NULL || bufferGeo->IsEmpty())
		{
			return;
		}

		SelectPoint(bufferGeo, pcd);
	}

	void PointCloudSelected::SelectPtByVector(OGRFeature* pFeature, PointCloud* pcd, double dfDist)
	{
		OGRGeometry* hGeo = pFeature->GetGeometryRef();
		OGRGeometry* bufferGeo = (iszero(dfDist))?hGeo:hGeo->Buffer(dfDist);
		if (bufferGeo == NULL || bufferGeo->IsEmpty())
		{
			return;
		}

		SelectPoint(bufferGeo, pcd);
	}

	void PointCloudSelected::SelectPoint(OGRGeometry* bufferGeo, PointCloud* pcd)
	{
		//读取Hls点云文件
		IHLSReader* hlsReader = pcd->GetHlsReader();

		if(hlsReader == NULL)
			return;

		// 获取点云圈数
		U32 loopCount = pcd->getLoopCount();
		if (loopCount <= 0)
		{
			return;
		}

		OGRPoint tmpPt;

		if (bufferGeo == NULL || bufferGeo->IsEmpty())
		{
			return;
		}

		OGREnvelope env;
		bufferGeo->getEnvelope(&env);

		//一列一列读取
		pcd->QueryAll();
		pcd->ResetRead();
		for (u32 k = 0; k< loopCount;k++)
		{
			//判断当前圈点云是否在缓冲区范围内
			float tempXmin, tempXmax, tempYmin, tempYmax, tempZmin, tempZmax;
			double lpXmin, lpXmax, lpYmin, lpYmax, lpZmin, lpZmax;

			// 转为全局坐标
			pcd->getLoopExtent(k, tempXmin, tempYmin, tempZmin, tempXmax, tempYmax, tempZmax);
			hlsReader->GetCoordinate(tempXmin, tempYmin, tempZmin, lpXmin, lpYmin, lpZmin);
			hlsReader->GetCoordinate(tempXmax, tempYmax, tempZmax, lpXmax, lpYmax, lpZmax);

			if (lpXmax<env.MinX || lpXmin>env.MaxX
				|| lpYmax<env.MinY || lpYmin>env.MaxY)
			{
				continue;
			}

			hdVector<PointXYZIPRGBA>& pts = pcd->getLoop(k);
			if (pts.size() <= 0)
			{
				continue;
			}

			// 判断点是否在缓冲区范围之内
			for (int p = 0;p <pts.size();p++)
			{
				double dtmpx, dtmpy, dtmpz;	
				PointXYZIPRGBA& pt = *(pts._Myfirst + p);
				if (!pt.isValid())
				{
					continue;
				}

				// 转为全局坐标
				hlsReader->GetCoordinate(pt.x, pt.y, pt.z,
					dtmpx, dtmpy, dtmpz);
				if (dtmpx<env.MinX || dtmpx>env.MaxX
					|| dtmpy<env.MinY || dtmpy>env.MaxY)
				{
					continue;
				}
				tmpPt.setX(dtmpx);
				tmpPt.setY(dtmpy);

				//判断是否在缓冲区范围内
				if (bufferGeo->Contains(&tmpPt))
				{
					pt.setSelected();					
				}				
			}
		}
	}

	OGRGeometry* PointCloudSelected::UnionBuffer(string vectorPath, double dfDist)
	{
		OGRGeometry* unionGeo = NULL;
		GDALDataset* hSrcDS = (GDALDataset*)GDALOpenEx(vectorPath.data(), GDAL_OF_READONLY | GDAL_OF_VECTOR, NULL, NULL, NULL);
		if (hSrcDS == NULL)
		{
			return NULL;
		}
		OGRLayer* hLayer = hSrcDS->GetLayer(0);
		if (!hLayer)
		{
			delete hSrcDS;
			return NULL;
		}
		OGRFeature* hFeat = NULL;
		hLayer->ResetReading();
		while ((hFeat = hLayer->GetNextFeature()) != NULL)
		{
			OGRGeometry* hGeo = hFeat->GetGeometryRef();		
			OGRGeometry* bufferGeo = (iszero(dfDist))?hGeo:hGeo->Buffer(dfDist);
			if (unionGeo == NULL)
			{
				unionGeo = bufferGeo;		
			}
			else
			{
				unionGeo = unionGeo->Union(bufferGeo);	
			}
		}

		delete hSrcDS;

		return unionGeo;
	}	
}