#include "StdAfx.h"
#include "HdRasterBuffer.h"
#include "HdGdalIO.h"
#include "include\BasicObject\hdCheckMemory.h"

namespace hd
{
    namespace scene
    {
        CHdRasterBuffer::CHdRasterBuffer(void)
            :m_pRaster(NULL)
            ,m_bCancel(false)
            ,m_bFailed(false)
            ,m_bIsLoad(false)
            ,m_strImagePath("")
            ,m_dScale(1)
            ,m_fCell(1)
        {
            // 检测内存泄露
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        }


        CHdRasterBuffer::~CHdRasterBuffer(void)
        {
            // 释放内存
            if (m_pRaster)
            {
                delete m_pRaster;
                m_pRaster = NULL;
            }
            m_bCancel = true;
        }


        // 设置路径
        void CHdRasterBuffer::SetImagePath(const char* strImagePath)
        {
            ZThread::Guard<ZThread::Mutex> g(m_lock);
            m_strImagePath = strImagePath;
        }

        // 设置缩放比例
        void CHdRasterBuffer::SetScale(float fScale)
        {
            ZThread::Guard<ZThread::Mutex> g(m_lock);
            m_dScale = fScale;
        }

        // 设置包围盒，只读取包围盒范围内
        void CHdRasterBuffer::SetBoundbox(const Chd2DBoundingBoxd& bRect)
        {
            ZThread::Guard<ZThread::Mutex> g(m_lock);
            m_bBox = bRect;
        }

        // 读取Raster数据
        void CHdRasterBuffer::ReadRaster()
        {
            ZThread::Guard<ZThread::Mutex> g(m_lock);
            ChdGdalIO gdalIO;

            // 如果打开成功
            if (gdalIO.Open(m_strImagePath.c_str()))
            {
                m_pRaster = gdalIO.ReadImage(m_bBox,m_dScale);

                // 判断是否读取成功
                if (!m_pRaster)
                {
                    m_bFailed = true;
                }
                else
                {
                    m_bFailed = false;
                }
                m_bIsLoad = true;
            }
            else
            {

                m_bFailed = true;
                m_bIsLoad = true;
            }

            gdalIO.Close();
            // 设置为取消
            m_bCancel = true;
        }
    }
}