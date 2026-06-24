#pragma once
#include "stdafx.h"
#include "..\3rd\ZThread\include\zthread\CountedPtr.h"
#include "..\3rd\ZThread\include\zthread\Cancelable.h"
#include "..\3rd\ZThread\include\zthread\Guard.h"
#include "..\3rd\ZThread\include\zthread\Mutex.h"
#include <string>
#include "include\BasicObject\BaseRect.h"
#include "include\BasicObject\hdGeoRaster.h"


//using namespace ZThread;

//class CHdGeoRaster;
namespace hd
{
    namespace scene
    {

        class HD3DSCENE_API CHdRasterBuffer : public ZThread::Cancelable
        {
        public:
            CHdRasterBuffer(void);
            ~CHdRasterBuffer(void);

            // 取消
            void cancel()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                m_bCancel = true;
            }

            // 判断是否取消
            bool isCanceled()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_bCancel;
            }

            // 获取数据
            CHdGeoRaster* GetRater()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_pRaster;
            }

            // 设置路径
            void SetImagePath(const char* strImagePath);

            // 设置包围盒，只读取包围盒范围内
            void SetBoundbox(const Chd2DBoundingBoxd& bRect);

            // 读取Raster数据
            void ReadRaster();

            // 设置缩放比例
            void SetScale(float fScale);

            // 获取级别
            float GetScale()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_dScale;
            }

            string GetStrImagePath()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_strImagePath;
            }

            bool IsLoad()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_bIsLoad;
            }

            bool IsFailed()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_bFailed;
            }

            // 获取包围盒
            Chd2DBoundingBoxd GetBox()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_bBox;
            }

            // 设置像素分辨率
            void SetCell(float fCell)
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                m_fCell = fCell;
            }

            float GetCell()
            {
                ZThread::Guard<ZThread::Mutex> g(m_lock);
                return m_fCell;
            }

        private:
            CHdGeoRaster* m_pRaster;						// 数据
            bool m_bIsLoad;									// 是否加载完成
            bool m_bFailed;									// 是否加载失败
            string m_strImagePath;							// 影像路径
            bool m_bCancel;									// 是否取消标识
            float m_dScale;									// 缩放大小
            float m_fCell;									// 图像像素分辨率
            Chd2DBoundingBoxd m_bBox;						// 包围盒
            ZThread::Mutex	m_lock;									// 锁住标识,用于多线程互斥
        };
    }
}

