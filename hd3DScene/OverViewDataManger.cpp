#include "StdAfx.h"
#include "OverViewDataManger.h"
#include "HdGdalIO.h"
#include "HdPyramid.h"
#include "OverViewInfo.h"
#include "hdCheckMemory.h"
#include "ReadRasterThread.h"

// 定义一个简单的结构体，用于读取数据
struct READRASTER 
{
    float fScale;
    string strPath;
    Chd2DBoundingBoxd box;
};

namespace hd
{
    namespace scene
    {
        COverViewDataManger::COverViewDataManger(float fMemory/*,int nThread*/)
            :m_pRasterCache(new CHdRaterCache(fMemory/*,nThread*/))
            ,m_fCell(1)
        {
            m_vectOverViewInfos.clear();
            // 检测内存泄露
            _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        }


        COverViewDataManger::~COverViewDataManger(void)
        {
            Clear();
        }



        //添加图像,获取金字塔信息
        bool COverViewDataManger::AddImage(const char* strImagePath)
        {
            // 获取信息
            ChdGdalIO gdalIO;
            bool bResult = gdalIO.Open(strImagePath);
            if (!bResult)
            {
                return false;
            }
			if (gdalIO.GetDataType() != GDT_Byte) // 暂时只支持字节类型的图像
			{
				gdalIO.Close();
				return false;
			}

            // 获取影像的基本信息
            IMAGEEXTINFO* pImageInfo = new IMAGEEXTINFO();
            gdalIO.GetExtInfo(pImageInfo);

            vector<OVERVIEWLEVEL*> vectLevels;
            gdalIO.GetOverViewInfo(vectLevels);

            gdalIO.Close();

            // 如果没有金字塔，生成金字塔
            if (vectLevels.empty())
            {
                CHdPyramid pyramid;
                if (pyramid.CreatePyramids(strImagePath))
                {
                    gdalIO.Open(strImagePath);
                    gdalIO.GetOverViewInfo(vectLevels);
                    gdalIO.Close();
                }
                else
                {
                    delete pImageInfo;
                    pImageInfo = NULL;
                    return false;
                }
            }

            // 设置基本的金字塔信息
            COverViewInfo* pOverViewInfo = new COverViewInfo();
            pOverViewInfo->SetImageInfo(ImageExtInfoSPtr(pImageInfo));
            pOverViewInfo->SetLevel(vectLevels);
            pOverViewInfo->SetUsed(true);

            // 判断是否以及存在，如果以及存在，不再重新添加
            if (find(m_vectOverViewInfos.begin(),m_vectOverViewInfos.end(),pOverViewInfo)
                !=m_vectOverViewInfos.end())
            {
                // 以及存在
                delete pOverViewInfo;
                pOverViewInfo = NULL;

                return false;
            }

            // 计算添加图像的像素分辨率
            float fCell = pImageInfo->GetScale();/*pImageInfo->dTop-pImageInfo->dBottom)/pImageInfo->nHeight;*/

            // 如果大于，赋值
            if (m_fCell>fCell)
            {
                m_fCell =fCell;
            }

            // 添加到记录中
            m_vectOverViewInfos.push_back(pOverViewInfo);

            return true;
        }

        // 根据包围盒获取当前的基本矢量信息
        bool COverViewDataManger::GetRasters(const Chd2DBoundingBoxd& box,float fScale,vector<CHdRasterBufferPtr>& vectShowRasters)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            //int nBoxHeight = box.getHeight()/fScale;

            int nSize = m_vectOverViewInfos.size();

            // 记录要读取的图像数据
            vector<READRASTER*> vectReadRaster;

            // 遍历查找在范围内的影像
            for (int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                if (pOverInfo && pOverInfo->GetUsed() && pOverInfo->GetImageInfo())
                {
                    ImageExtInfoSPtr pExtinfo = pOverInfo->GetImageInfo();

                    // 先判断是否在包围盒内
                    if (pExtinfo->dLeft <box.GetMaxX()
                        && pExtinfo->dRight>box.GetMinX()
                        && pExtinfo->dTop>box.GetMinY()
                        && pExtinfo->dBottom<box.GetMaxY())
                    {
                        // 获取最近一个级别的金字塔，然后读取
                        int nLevelCount = pOverInfo->GetLevelCount();

                        // 缩放级别的差
                        float dScaleDiff(FLT_MAX);
                        float dReadScale =0;

                        // 0.75的倍数说明离全图的那个级别更近
                        if (fScale>0.75)
                        {
                            dReadScale =1.0;
                        }
                        else
                        {
                            // 获取最接近的一个级别
                            for(int j=0;j<nLevelCount;j++)
                            {
                                float fTmpScale = pOverInfo->GetLevel(j)->fScaleY;
                                float fDiff = abs(fScale-fTmpScale);

                                // 如果小于，赋值
                                if (fDiff<dScaleDiff)
                                {
                                    dScaleDiff = fDiff;
                                    dReadScale = fTmpScale;
                                }
                            }
                        }


                        // 定义一个buffer对象
                        CHdRasterBuffer* pRasterBuffer = new CHdRasterBuffer();

                        pRasterBuffer->SetImagePath(pExtinfo->strImagePath.c_str());
                        pRasterBuffer->SetScale(dReadScale);

                        // 读数据时外扩一部分
                        double dExt = pExtinfo->GetScale()*100.0/dReadScale;
                        //double dExtX = (pExtinfo->dRight - pExtinfo->dLeft) * pExtinfo->GetScale() / dReadScale / 2;
                        //double dExtY = (pExtinfo->dTop - pExtinfo->dBottom) * pExtinfo->GetScale() / dReadScale / 2;

                        // 计算每个图形需要读取的包围盒
                        //double dBoxLeft = MAX(pExtinfo->dLeft,box.GetMinX()-dExtX);
                        //double dBoxRight = MIN(pExtinfo->dRight,box.GetMaxX()+dExtX);
                        //double dBoxTop = MIN(pExtinfo->dTop,box.GetMaxY()+dExtY);
                        //double dBoxBottom = MAX(pExtinfo->dBottom,box.GetMinY()-dExtY);

                        double dBoxLeft = MAX(pExtinfo->dLeft,box.GetMinX()-dExt);
                        double dBoxRight = MIN(pExtinfo->dRight,box.GetMaxX()+dExt);
                        double dBoxTop = MIN(pExtinfo->dTop,box.GetMaxY()+dExt);
                        double dBoxBottom = MAX(pExtinfo->dBottom,box.GetMinY()-dExt);

                        Chd2DBoundingBoxd readBox(dBoxLeft,dBoxTop,dBoxRight,dBoxBottom);

                        pRasterBuffer->SetBoundbox(readBox);

                        // 添加到缓存中，开启线程，读取	
                        m_pRasterCache->AddCache(pRasterBuffer);

                        // 记录要显示的图像
                        READRASTER* pReadRaster = new READRASTER;
                        pReadRaster->strPath = pExtinfo->strImagePath;
                        pReadRaster->fScale = dReadScale;
                        pReadRaster->box = readBox;

                        vectReadRaster.push_back(pReadRaster);
                    }
                }

            }

            // 获取要显示的图像
            int nShowSize = vectReadRaster.size();

            if (nShowSize==0)
            {
                return false;
            }

            // 遍历获取
            for (int i=0;i<nShowSize;i++)
            {
                READRASTER* pReadRaster = vectReadRaster.at(i);

                // 判断是否有效
                if (pReadRaster)
                {
                    // 判断是否读取成功
                    CHdRasterBufferPtr pRaster = m_pRasterCache->GetRaster(pReadRaster->strPath.c_str(),pReadRaster->fScale,pReadRaster->box);
                    if (pRaster)
                    {
                        vectShowRasters.push_back(pRaster);
                    }

                    // 释放读取的内存
                    if (pReadRaster)
                    {
                        delete pReadRaster;
                        pReadRaster = NULL;
                    }

                }
            }

            return true;
        }

        // 获取当前的基本矢量信息
        bool COverViewDataManger::GetRasters(float fScale,vector<CHdRasterBufferPtr>& vectShowRasters)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            //int nBoxHeight = box.getHeight()/fScale;

            int nSize = m_vectOverViewInfos.size();

            // 记录要读取的图像数据
            vector<READRASTER*> vectReadRaster;

            // 遍历查找在范围内的影像
            for (int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                if (pOverInfo && pOverInfo->GetUsed() && pOverInfo->GetImageInfo())
                {
                    ImageExtInfoSPtr pExtinfo = pOverInfo->GetImageInfo();

                    // 先判断是否在包围盒内
                    //if (pExtinfo->dLeft <box.GetMaxX()
                    //    && pExtinfo->dRight>box.GetMinX()
                    //    && pExtinfo->dTop>box.GetMinY()
                    //    && pExtinfo->dBottom<box.GetMaxY())
                    {
                        // 获取最近一个级别的金字塔，然后读取
                        int nLevelCount = pOverInfo->GetLevelCount();

                        // 缩放级别的差
                        float dScaleDiff(FLT_MAX);
                        float dReadScale =0;

                        // 0.75的倍数说明离全图的那个级别更近
                        if (fScale>0.75)
                        {
                            dReadScale =1.0;
                        }
                        else
                        {
                            // 获取最接近的一个级别
                            for(int j=0;j<nLevelCount;j++)
                            {
                                float fTmpScale = pOverInfo->GetLevel(j)->fScaleY;
                                float fDiff = abs(fScale-fTmpScale);

                                // 如果小于，赋值
                                if (fDiff<dScaleDiff)
                                {
                                    dScaleDiff = fDiff;
                                    dReadScale = fTmpScale;
                                }
                            }
                        }


                        // 定义一个buffer对象
                        CHdRasterBuffer* pRasterBuffer = new CHdRasterBuffer();

                        pRasterBuffer->SetImagePath(pExtinfo->strImagePath.c_str());
                        pRasterBuffer->SetScale(dReadScale);

                        // 读数据时外扩一部分
                        //double dExt = pExtinfo->GetScale()*5.0/dReadScale;

                        // 计算每个图形需要读取的包围盒
                        //double dBoxLeft = MAX(pExtinfo->dLeft,box.GetMinX()-dExt);
                        //double dBoxRight = MIN(pExtinfo->dRight,box.GetMaxX()+dExt);
                        //double dBoxTop = MIN(pExtinfo->dTop,box.GetMaxY()+dExt);
                        //double dBoxBottom = MAX(pExtinfo->dBottom,box.GetMinY()-dExt);

                        double dBoxLeft = pExtinfo->dLeft;
                        double dBoxRight = pExtinfo->dRight;
                        double dBoxTop = pExtinfo->dTop;
                        double dBoxBottom = pExtinfo->dBottom;

                        Chd2DBoundingBoxd readBox(dBoxLeft,dBoxTop,dBoxRight,dBoxBottom);

                        pRasterBuffer->SetBoundbox(readBox);

                        // 添加到缓存中，开启线程，读取	
                        m_pRasterCache->AddCache(pRasterBuffer);

                        // 记录要显示的图像
                        READRASTER* pReadRaster = new READRASTER;
                        pReadRaster->strPath = pExtinfo->strImagePath;
                        pReadRaster->fScale = dReadScale;
                        pReadRaster->box = readBox;

                        vectReadRaster.push_back(pReadRaster);
                    }
                }

            }

            // 获取要显示的图像
            int nShowSize = vectReadRaster.size();

            if (nShowSize==0)
            {
                return false;
            }

            // 遍历获取
            for (int i=0;i<nShowSize;i++)
            {
                READRASTER* pReadRaster = vectReadRaster.at(i);

                // 判断是否有效
                if (pReadRaster)
                {
                    // 判断是否读取成功
                    CHdRasterBufferPtr pRaster = m_pRasterCache->GetRaster(pReadRaster->strPath.c_str(),pReadRaster->fScale,pReadRaster->box);
                    if (pRaster)
                    {
                        vectShowRasters.push_back(pRaster);
                    }

                    // 释放读取的内存
                    if (pReadRaster)
                    {
                        delete pReadRaster;
                        pReadRaster = NULL;
                    }

                }
            }

            return true;
        }

        // 获取当前的基本矢量信息
        bool COverViewDataManger::GetRasters(vector<CHdRasterBufferPtr>& vectShowRasters)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            //int nBoxHeight = box.getHeight()/fScale;

            int nSize = m_vectOverViewInfos.size();

            // 记录要读取的图像数据
            vector<READRASTER*> vectReadRaster;

            // 遍历查找在范围内的影像
            for (int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                if (pOverInfo && pOverInfo->GetUsed() && pOverInfo->GetImageInfo())
                {
                    ImageExtInfoSPtr pExtinfo = pOverInfo->GetImageInfo();

                    // 先判断是否在包围盒内
                    //if (pExtinfo->dLeft <box.GetMaxX()
                    //    && pExtinfo->dRight>box.GetMinX()
                    //    && pExtinfo->dTop>box.GetMinY()
                    //    && pExtinfo->dBottom<box.GetMaxY())
                    {
                        // 获取最大的层级
                        float dReadScale = pOverInfo->GetLevel(pOverInfo->GetLevelCount() - 1)->fScaleY;

                        // 定义一个buffer对象
                        CHdRasterBuffer* pRasterBuffer = new CHdRasterBuffer();

                        pRasterBuffer->SetImagePath(pExtinfo->strImagePath.c_str());
                        pRasterBuffer->SetScale(dReadScale);

                        // 读数据时外扩一部分
                        //double dExt = pExtinfo->GetScale()*5.0/dReadScale;

                        // 计算每个图形需要读取的包围盒
                        //double dBoxLeft = MAX(pExtinfo->dLeft,box.GetMinX()-dExt);
                        //double dBoxRight = MIN(pExtinfo->dRight,box.GetMaxX()+dExt);
                        //double dBoxTop = MIN(pExtinfo->dTop,box.GetMaxY()+dExt);
                        //double dBoxBottom = MAX(pExtinfo->dBottom,box.GetMinY()-dExt);

                        double dBoxLeft = pExtinfo->dLeft;
                        double dBoxRight = pExtinfo->dRight;
                        double dBoxTop = pExtinfo->dTop;
                        double dBoxBottom = pExtinfo->dBottom;

                        Chd2DBoundingBoxd readBox(dBoxLeft,dBoxTop,dBoxRight,dBoxBottom);

                        pRasterBuffer->SetBoundbox(readBox);

                        // 添加到缓存中，开启线程，读取	
                        m_pRasterCache->AddCache(pRasterBuffer);

                        // 记录要显示的图像
                        READRASTER* pReadRaster = new READRASTER;
                        pReadRaster->strPath = pExtinfo->strImagePath;
                        pReadRaster->fScale = dReadScale;
                        pReadRaster->box = readBox;

                        vectReadRaster.push_back(pReadRaster);
                    }
                }

            }

            // 获取要显示的图像
            int nShowSize = vectReadRaster.size();

            if (nShowSize==0)
            {
                return false;
            }

            // 遍历获取
            for (int i=0;i<nShowSize;i++)
            {
                READRASTER* pReadRaster = vectReadRaster.at(i);

                // 判断是否有效
                if (pReadRaster)
                {
                    // 判断是否读取成功
                    CHdRasterBufferPtr pRaster = m_pRasterCache->GetRaster(pReadRaster->strPath.c_str(),pReadRaster->fScale,pReadRaster->box);
                    if (pRaster)
                    {
                        vectShowRasters.push_back(pRaster);
                    }

                    // 释放读取的内存
                    if (pReadRaster)
                    {
                        delete pReadRaster;
                        pReadRaster = NULL;
                    }

                }
            }

            return true;
        }

        // 获取包围盒
        Chd2DBoundingBoxd COverViewDataManger::GetBox() const
        {
            Chd2DBoundingBoxd boundBox;

            // 遍历计算包围盒
            if (!m_vectOverViewInfos.empty())
            {
                size_t nSize = m_vectOverViewInfos.size();

                for (int i=0;i<nSize;++i)
                {
                    COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                    if (pOverInfo && pOverInfo->GetImageInfo())
                    {
                        if (0==i)
                        {
                            boundBox.SetLB(pOverInfo->GetImageInfo()->dLeft,pOverInfo->GetImageInfo()->dBottom);
                            boundBox.SetUR(pOverInfo->GetImageInfo()->dRight,pOverInfo->GetImageInfo()->dTop);
                        }
                        else
                        {
                            boundBox.addInternalPoint(pOverInfo->GetImageInfo()->dLeft,pOverInfo->GetImageInfo()->dBottom);
                            boundBox.addInternalPoint(pOverInfo->GetImageInfo()->dRight,pOverInfo->GetImageInfo()->dTop);
                        }
                    }

                }
            }
            return boundBox;
        }

        // 设置某张影像是否使用，主要用于图片的隐藏和显示
        bool COverViewDataManger::SetUse(const char* strImagePath,bool bUsed)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            int nSize = m_vectOverViewInfos.size();

            // 遍历查找在范围内的影像
            for (int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                if (pOverInfo && pOverInfo->GetImageInfo())
                {
                    ImageExtInfoSPtr pExtinfo = pOverInfo->GetImageInfo();

                    // 先判断路径是否相等
                    if (strcmp(pExtinfo->strImagePath.c_str(),strImagePath)==0)
                    {
                        pOverInfo->SetUsed(bUsed);
                        break;
                    }
                }
            }
            return true;
        }

        // 设置所有是否可用
        bool COverViewDataManger::SetAllUse(bool bUsed)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            int nSize = m_vectOverViewInfos.size();

            // 遍历查找在范围内的影像
            for (int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverInfo = m_vectOverViewInfos.at(i);
                if (pOverInfo)
                {
                    pOverInfo->SetUsed(bUsed);
                }
            }
            return true;
        }

        // 删除
        bool COverViewDataManger::DeleteImage(const char* strImagePath)
        {
            // 遍历，获取在包围盒范围的图像，然后计算，获取其最邻近的金字塔数据
            if (m_vectOverViewInfos.empty())
            {
                return false;
            }

            int nSize = m_vectOverViewInfos.size();

            // 遍历查找在范围内的影像
            for (vector<COverViewInfo*>::iterator iter = m_vectOverViewInfos.begin();iter!=m_vectOverViewInfos.end();)
            {
                COverViewInfo* pOverInfo = (*iter);
                if (pOverInfo && pOverInfo->GetImageInfo())
                {
                    ImageExtInfoSPtr pExtinfo = pOverInfo->GetImageInfo();

                    // 转化为小写
                    std::transform(pExtinfo->strImagePath.begin(), pExtinfo->strImagePath.end(), pExtinfo->strImagePath.begin(), tolower);

                    // 先判断路径是否相等,如果匹配，删除
                    if (strcmp(pExtinfo->strImagePath.c_str(),strImagePath)==0)
                    {
                        // 释放内存
                        if (pOverInfo)
                        {
                            delete pOverInfo;
                            pOverInfo = NULL;
                        }

                        iter = m_vectOverViewInfos.erase(iter);
                    }
                    else
                    {
                        iter++;
                    }
                }
            }

            return true;
        }

        // 清空
        void COverViewDataManger::Clear()
        {
            // 释放内存
            int nSize = m_vectOverViewInfos.size();
            for(int i=0;i<nSize;i++)
            {
                COverViewInfo* pOverView = m_vectOverViewInfos.at(i);
                if (pOverView)
                {
                    delete pOverView;
                    pOverView = NULL;
                }
            }

            m_vectOverViewInfos.clear();

            // 是否缓存内存
            if (m_pRasterCache)
            {
                delete m_pRasterCache;
                m_pRasterCache = NULL;
            }
        }
    }
}