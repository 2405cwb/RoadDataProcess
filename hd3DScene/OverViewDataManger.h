/*!@file
*******************************************************************************************************
<PRE>
模块名		：hdDrawCanvas
文件名		：OverViewDataCache.h
相关文件	: HdMdcPaint.h
文件实现功能：管理画板的金字塔数据
作者		：马振明
版本		：1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/6/15	1.0			马振明		 创建
</PRE>
******************************************************************************************************/

#pragma once
#include "include\BasicObject\BaseStruct.h"
#include "include\BasicObject\BaseRect.h"
#include <vector>
#include "HdRaterCache.h"
#include "include\BasicObject\hdGeoRaster.h"
#include "OverViewInfo.h"

using namespace std;
using namespace base;
using namespace hd;

namespace hd
{
    namespace scene
    {
        //class CHdGeoRaster;
        //class COverViewInfo;

        // 金字塔数据缓存获取
        class HD3DSCENE_API COverViewDataManger
        {
        public:
            // 初始化，缓存默认为100M
            explicit COverViewDataManger(float fMemory=0.1/*,int nThreadNum =5*/);
            ~COverViewDataManger(void);

            //添加图像,获取金字塔信息
            bool AddImage(const char* strImagePath);

            // 判断是否有金字塔数据
            bool IsOverView() const
            {
                return !m_vectOverViewInfos.empty();
            }

            // 根据包围盒获取当前的基本矢量信息,并传入绘制的高度，实时获取当前每个影像对应的金字塔比例
            bool GetRasters(const Chd2DBoundingBoxd& box,float fScale,vector<CHdRasterBufferPtr>& vectShowRasters);

            // 获取当前的基本矢量信息,并传入绘制的高度，实时获取当前每个影像对应的金字塔比例
            bool GetRasters(float fScale,vector<CHdRasterBufferPtr>& vectShowRasters);

            // 获取当前的基本矢量信息,并传入绘制的高度，实时获取当前每个影像对应的金字塔比例
            bool GetRasters(vector<CHdRasterBufferPtr>& vectShowRasters);

            // 获取包围盒
            Chd2DBoundingBoxd GetBox() const;
            // 	{
            // 		return m_BoundingBox;
            // 	}

            // 获取实际地理范围分辨率
            float GetCell() const
            {
                return m_fCell;
            }

            // 返回数量
            int GetCount() const
            {
                return m_vectOverViewInfos.size();
            }

            // 设置某张影像是否使用，主要用于图片的隐藏和显示
            bool SetUse(const char* strImagePath,bool bUsed);

            // 删除
            bool DeleteImage(const char* strImagePath);

            // 清空
            void Clear();

            // 设置所有是否可用
            bool SetAllUse(bool bUsed);

        private:
            vector<COverViewInfo*> m_vectOverViewInfos;					// 金字塔信息
            CHdRaterCache*		   m_pRasterCache;						// 缓存
            //Chd2DBoundingBoxd	   m_BoundingBox;						// 包围盒
            float				   m_fCell;								// 对应的像素分辨率
        };
    }
}



