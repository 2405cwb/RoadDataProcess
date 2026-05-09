/*! @hd3DView_Dom
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hd3DView_Dom.cpp
相关文件     : hd3DView.h
文件实现功能 : dom与点云叠加显示视图(俯视图浏览)
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/07/15   1.0      朱旭波                创建
2015/12/28   1.1      张阳                将HdSxDomPcd3DView.cpp从HdApplication移至Hd3DScene中
</PRE>
*******************************************************************************/

#include "StdAfx.h"
#include "hd3DView.h"
#include "CScanSceneNode.h"
#include "hd3DCamera.h"
#include "..\hdPointCloud\Point_Cloud.h"
#include "HdSeaDataSceneNode.h"
#include <time.h>
#include "CRoutePointSceneNode.h"
#include "HdGdalIO.h"
#include "..\hd3DScene\include\BasicObject\hdGeoRaster.h"
#include "..\hd3DEngine\CImage.h"
#include "..\hd3DEngine\include\IImageWriter.h"
#include "..\hd3DEngine\CImageWriterJPG.h"
#include "OverViewDataManger.h"
#include "CScanSceneNode.h"
#include "CDomSymSceneNode.h"
#include "..\hdCommon\HdLayerObject.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace hd;
using namespace irr::video;

namespace hd
{
    namespace scene
    {
        // 添加一个新的dom对象至sn
        IObjectSceneNode* CHd3DView::AddDomData(const char* strDomPath)
        {
            CDomSymbSceneNode* pSn = NULL;

            // 判断dom文件路径是否存在
            if (_access(strDomPath,04) != 0)
            {
                return pSn;
            }
            string strPath = strDomPath;
            std::transform(strPath.begin(),strPath.end(),strPath.begin(),tolower);

            // 判断是否需要new新的dom sn
            irr::core::array<ISceneNode*> vecSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);

            // 根据路径判断，该dom文件是否已添加
            for (unsigned int n = 0;n < vecSn.size();n++)
            {
                // 转换
                CDomSymbSceneNode* pDomTmpSn = dynamic_cast<CDomSymbSceneNode*>(vecSn[n]);
                if (!pDomTmpSn)
                {
                    continue;
                }

                // 判断路径是否相同
                string strTmpPath = pDomTmpSn->GetDomTexturePah();
                std::transform(strTmpPath.begin(),strTmpPath.end(),strTmpPath.begin(),tolower);

                // 比较路径是否相同
                if (strcmp(strPath.data(),strTmpPath.data()) == 0)
                {
                    // 若相同则赋值并跳出循环
                    pSn = pDomTmpSn;
                    break;
                }
            }

            // 若已经存在，则直接返回即可
            if (pSn)
            {
                return NULL;
            }

            // 若不存在，则添加
            if (!AddImagePic(strDomPath,E_LIT_OVERVIEW))
            {
				return NULL;
            }

            // 添加完成后，再调用RefreshOverView来构建纹理等信息
            bool bRet = RefreshOverView(true);
            if (!bRet)
            {
                return pSn;
            }

            // 清空
            vecSn.clear();

            // 清空后重新查找遍历
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);

            // 根据路径判断，该dom文件是否已添加
            for (unsigned int n = 0;n < vecSn.size();n++)
            {
                // 转换
                CDomSymbSceneNode* pDomTmpSn = dynamic_cast<CDomSymbSceneNode*>(vecSn[n]);
                if (!pDomTmpSn)
                {
                    continue;
                }

                // 判断路径是否相同
                string strTmpPath = pDomTmpSn->GetDomTexturePah();
                std::transform(strTmpPath.begin(),strTmpPath.end(),strTmpPath.begin(),tolower);

                // 比较路径是否相同
                if (strcmp(strPath.data(),strTmpPath.data()) == 0)
                {
                    // 若相同则赋值并跳出循环
                    pSn = pDomTmpSn;
                    break;
                }
            }

            ZoomToFull();

            //ZoomToFullExtent();
            //irr::core::aabbox3df box = pSn->getBoundingBox();
            //irr::core::vector3df domPos;
            //domPos = box.getCenter();
            //ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
            //cam->setPosition(irr::core::vector3df(domPos.X,domPos.Y,1000.0f));
            //cam->setTarget(irr::core::vector3df(domPos.X,domPos.Y,0.0f));
            //cam->setUpVector(core::vector3df(0.0f, 0.0f, 1.0f));

            // 返回最终查找结果
            return pSn;
        }

        // 添加图片
        bool CHd3DView::AddImagePic(const CHdGeoRaster* pImageBuffer, const char* strID)
        {
            // 判空
            if (NULL == pImageBuffer)
            {
                return false;
            }

            // 更新绘制区域的地理范围
            //m_PaintRect.InsertBox(pImageBuffer->GetBoundingBox());
            Chd2DBoundingBoxd box = pImageBuffer->GetBoundingBox();
            m_viewBoundingBox.insetRect(box);  // 记录为绝对坐标

            // 设置地理范围的间隔大小
            float fCellSizeX(0),fCellSizeY(0);
            pImageBuffer->GetCellSize(fCellSizeX,fCellSizeY);

            // 记录最小的分辨率
            if (m_RWCellSize > fCellSizeX)
            {
                m_RWCellSize = fCellSizeX;
            }

            // 获得长宽
            int nWidth(0),nHeight(0);
            pImageBuffer->GetRowsCols(nHeight,nWidth);

            // 添加一个RasterSn
            irr::core::vector3df domPos; // 该值记录dom纹理贴图的中心位置

            double fTmpX,fTmpY,fTmpZ; // 记录dom中心位置绝对坐标
            fTmpX = box.getCenter().X;
            fTmpY = box.getCenter().Y;
            fTmpZ = 0.0f;

            // 比较路径
            CDomSymbSceneNode* pDomSn = NULL;
            string strPath = strID;
            std::transform(strPath.begin(),strPath.end(),strPath.begin(),tolower);

            // 根据标记ID判断是否存在该SN
            irr::core::array<ISceneNode*> arrSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,arrSn);
            for (unsigned int n = 0;n < arrSn.size();n++)
            {
                // 逐记录判断
                CDomSymbSceneNode* pTmpSn = dynamic_cast<CDomSymbSceneNode*>(arrSn[n]);
                if (!pTmpSn)
                {
                    continue;
                }

                // 根据strID判断
                string strTmpPath = pTmpSn->GetDomTexturePah();
                std::transform(strTmpPath.begin(),strTmpPath.end(),strTmpPath.begin(),tolower);

                // 比较路径是否相同
                if (strcmp(strPath.data(),strTmpPath.data()) == 0)
                {
                    // 若相同则赋值并跳出循环
                    pDomSn = pTmpSn;
                    break;
                }
            }

            // 转换显示坐标
            CBursaWolfModel* pViewModel = GetTransModel();
            pViewModel->Translate(fTmpX,fTmpY,fTmpZ);
            domPos.set(fTmpX,fTmpY,fTmpZ);

            // 根据tif金字塔信息计算真实的宽高
            fTmpX = box.GetMinX();
            fTmpY = box.GetMinY();
            fTmpZ = 0.0f;
            pViewModel->Translate(fTmpX,fTmpY,fTmpZ);

            float fHalfW = domPos.X - fTmpX;
            float fHalfH = domPos.Y - fTmpY;

            if (!pDomSn) // 不存在则创建
            {
                pDomSn = new CDomSymbSceneNode(GetSceneManager()->getRootSceneNode(),GetSceneManager(),-1,domPos,2.0f/*core::dimension2d<f32>(fHalfW * 2.0f, fHalfH * 2.0f)*/);
                //pDomSn = new CDomSymbSceneNode(GetSceneManager()->getRootSceneNode(),GetSceneManager(),-1,domPos,min(nHeight, nWidth));
                pDomSn->SetView(this);
                pDomSn->SetDomTexturePath(strPath);
                pDomSn->drop();
            }

            //// 更新纹理
            //long sizeL = nWidth * nHeight * pImageBuffer->GetBandNum() * pImageBuffer->GetBPP();
            //irr::io::IReadFile* pReadFile = irr::io::createMemoryReadFile(pImageBuffer->m_pBuffer,
            //	sizeL,strID,false);
            //IVideoDriver* driver = m_irrDevice->getVideoDriver();
            //ITexture* pTexture = driver->getTexture(pReadFile);

            // 添加纹理
            video::IVideoDriver* driver = m_irrDevice->getVideoDriver();
            CImage* pImg = new CImage(ECF_R8G8B8,irr::core::dimension2d<u32>(nWidth,nHeight),pImageBuffer->m_pBuffer,true,false);
            ITexture* pTexture = driver->addTexture(strID,pImg);
            if (!pTexture)
            {
                return false;
            }

            //// 测试代码--start
            //IImageWriter* imgWrite = new CImageWriterJPG();
            //if (!imgWrite)
            //{
            //	return 0;
            //}

            //string imagePath = "G:\\test1.jpg";
            //irr::io::IWriteFile* imgFile = irr::io::createWriteFile(imagePath.data(),false);
            //if (!imgFile)
            //{
            //	return 0;
            //}

            //imgWrite->writeImage(imgFile,pImg);
            //imgFile->drop();
            //imgWrite->drop();
            //// 测试代码--end

            pImg->drop();

            // 更新sn纹理
            pDomSn->UpdateTexture(pTexture,box);

            return true;
        }

        // 通过路径添加影像，并制定影像添加的方式,默认采用金字塔
        bool CHd3DView::AddImagePic(const char* strImage,E_LOAD_IMAGE_TYPE eLoadImageType)
        {
            // 如果采用金字塔形式
            if (E_LIT_OVERVIEW == eLoadImageType)
            {
                if (!m_OverViewDataManger->AddImage(strImage))
				{
					return false;
				}
                Chd2DBoundingBoxd box = m_OverViewDataManger->GetBox();

                //m_viewBoundingBox.insetRect(box);
                m_viewBoundingBox = box;

                // 记录分辨率
                m_RWCellSize = m_OverViewDataManger->GetCell();

                //m_viewBoundingBox.SetLB(box.GetMinX(),box.GetMinY());
                //m_viewBoundingBox.SetUR(box.GetMaxX(),box.GetMaxY());

                //m_PaintRect.SetRWCellSize(m_OverViewDataManger->GetCell());
            }
            else
            {
                // 直接读取影像
                ChdGdalIO gdalRead;
                if (!gdalRead.Open(strImage))
                {
					return false;
				}
				if (gdalRead.GetDataType() != GDT_Byte) // 暂时只支持字节类型的图像
				{
					gdalRead.Close();
					return false;
				}

				CHdGeoRaster* pGeoRaster = gdalRead.ReadImage();
				AddImagePic(pGeoRaster,strImage);

                gdalRead.Close();
            }
			return true;
        }

        // 刷新前调用更新显示金字塔
        bool CHd3DView::RefreshOverView(bool isAddDom)
        {
            // 判断是否含有金字塔数据
            if (!m_OverViewDataManger->IsOverView())
            {
                return false;
            }

            // 更新包围盒
            UpdateBoxByView();

            // 获得金字塔数据
            vector<CHdRasterBufferPtr> vectRasters;
            Chd2DBoundingBoxd showBox = m_viewBoundingBox;

            if (isAddDom)
            {
                m_OverViewDataManger->GetRasters(GetCurPecent(),vectRasters);
            }
            else
            {
                m_OverViewDataManger->GetRasters(showBox,GetCurPecent(),vectRasters);
            }

            // 将数据更新
            if (vectRasters.empty())
            {
                return false;
            }
            else
            {
                // 遍历添加图像
                int nSize = vectRasters.size();
                for (int i=0;i<nSize;i++)
                {
                    // 获得更新后的金字塔数据，用于更新sn的纹理信息
                    CHdRasterBufferPtr pBuffer =  vectRasters.at(i);
                    CHdGeoRaster* pRaster = pBuffer->GetRater();
                    if (pRaster && pRaster->m_pBuffer)
                    {
                        AddImagePic(pRaster,pBuffer->GetStrImagePath().c_str());
                    }
                }
            }

            Refresh();
            return true;
        }

        // 通过ID删除影像
        bool CHd3DView::DeleteImage(const char* strImageID)
        {
            // 从金字塔中删除
            if (m_OverViewDataManger)
            {
                return m_OverViewDataManger->DeleteImage(strImageID);
            }

            return false;
        }

        // 通过ID移除节点，移除sn时移除影像金字塔内信息
        bool CHd3DView::DeleteSN(const char* strID)
        {
            // 字符串获取
            string strPath = strID;
            std::transform(strPath.begin(),strPath.end(),strPath.begin(),tolower);
            CDomSymbSceneNode* pSn = NULL;

            // 获得视图内所有dom sn
            irr::core::array<ISceneNode*> vecSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);

            // 根据路径判断，该dom文件是否已添加
            for (unsigned int n = 0;n < vecSn.size();n++)
            {
                // 转换
                CDomSymbSceneNode* pDomTmpSn = dynamic_cast<CDomSymbSceneNode*>(vecSn[n]);
                if (!pDomTmpSn)
                {
                    continue;
                }

                // 判断路径是否相同
                string strTmpPath = pDomTmpSn->GetDomTexturePah();
                std::transform(strTmpPath.begin(),strTmpPath.end(),strTmpPath.begin(),tolower);

                // 比较路径是否相同
                if (strcmp(strPath.data(),strTmpPath.data()) == 0)
                {
                    // 若相同则赋值并跳出循环
                    pSn = pDomTmpSn;
                    break;
                }
            }

            // 若查找到，对应处理
            if (pSn)
            {
                // 移除sn
                RemoveSceneNode(pSn);

                // 从金字塔中移除该数据对象
                DeleteImage(strID);
                return true;
            }

            return false;
        }

        // 根据视锥更新包围盒
        void CHd3DView::UpdateBoxByView()
        {
            // 获得当前视锥包围盒-显示坐标
            irr::scene::ICameraSceneNode* camera = GetSceneManager()->getActiveCamera();
            irr::core::aabbox3df viewBox = camera->getViewFrustum()->getBoundingBox();

            // 获得视图model
            CBursaWolfModel* pViewModel = GetTransModel();

            // 更新
            double dTmpX,dTmpY,dTmpZ;
            dTmpZ = viewBox.MinEdge.Z;
            dTmpX = viewBox.MinEdge.X;
            dTmpY = viewBox.MinEdge.Y;
            pViewModel->AntiTranslate(dTmpX,dTmpY,dTmpZ);

            // 记录坐标
            double dtX,dtY,dtZ;
            dtX = viewBox.MaxEdge.X;
            dtY = viewBox.MaxEdge.Y;
            dtZ = viewBox.MaxEdge.Z;
            pViewModel->AntiTranslate(dtX,dtY,dtZ);

            // 更新记录
            m_viewBoundingBox.SetLB(dTmpX,dTmpY);
            m_viewBoundingBox.SetUR(dtX,dtY);
        }

        // 获得当前比例
        double CHd3DView::GetCurPecent()
        {
            // 根据当前包围盒的高度和实际地理范围间隔大小以及绘制区域大小计算
            double fBoxHeight = m_viewBoundingBox.getHeight();
            double fScale = 0.0f;
            int height = GetWindowHeight();
            fScale = height * m_RWCellSize / fBoxHeight;

            return fScale;
        }

        // 根据dom所有sn缩放至全部查看
        void CHd3DView::ZoomToFull()
        {
            // 获得所有dom sn的全部外包围盒
            bool bFirstBox = true;
            irr::core::array<ISceneNode*> vecSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);
            int nSNCount = vecSn.size();
            core::aabbox3d<f32> bbox;

            // 获得所有外包围盒
            int scanCount = 0;
            core::vector3df firstScanPos;
            for (int i = 0; i < nSNCount; i++)
            {
                const core::aabbox3d<f32>& box = vecSn[i]->getBoundingBox();
                if (bFirstBox)
                {
                    firstScanPos.X = box.getCenter().X;
                    firstScanPos.Y = box.getCenter().Y;
                    firstScanPos.Z = box.getCenter().Z;
                    bbox = box;
                    bFirstBox = false;
                }
                else
                {
                    bbox.addInternalBox(box);
                }

                scanCount++;
            }

            // 条件性判断
            if (scanCount == 0)
            {
                return;
            }

            ICameraSceneNode* cam = GetSceneManager()->getActiveCamera();
            cam->setFarValue(2000);
            cam->setFOV(core::PI / 2.5f);

            // 条件性判断，只需要处理正交视图
            if (cam->isOrthogonal())
            {
                // 正交模式下 看到全部的对象 将外包盒子 作为视景体
                // 设置视景体宽度和高度
                f32 vWidth = cam->getWidthofViewVolume();
                f32 vHeight = cam->getHeightofViewVolume();
                f32 NearD = cam->getNearValue();
                f32 FarD = cam->getFarValue();
                f32 Dx = (bbox.MaxEdge.X - bbox.MinEdge.X)*FarD/vWidth;
                f32 Dz = (bbox.MaxEdge.Z - bbox.MinEdge.Z)*FarD/vHeight;

                if (bbox.MaxEdge.X - bbox.MinEdge.X > 0 && bbox.MaxEdge.Z - bbox.MinEdge.Z > 0)
                {
                    cam->setHeightofViewVolume(1.2*(bbox.MaxEdge.Z - bbox.MinEdge.Z));
                    cam->setWidthofViewVolume(1.2*(bbox.MaxEdge.X - bbox.MinEdge.X));
                }

                f32 distCT = ((Dx>Dz)?Dx:Dz) + (bbox.MaxEdge.Y - bbox.MinEdge.Y)/2.0f;

                core::vector3df offsetNormal(0.f,0.f,0.f);

                offsetNormal.X = 0.0f;
                offsetNormal.Y = 0.0f;
                offsetNormal.Z = 1.f;
                offsetNormal.normalize();
                cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));

                //target点设置在box中心
                if (scanCount == 1)
                {
                    // 只有一站,设置到原点
                    core::vector3df newPos = firstScanPos + offsetNormal*distCT;
                    m_oriTarget = firstScanPos;
                    m_oriPosition = newPos;
                }
                else
                {
                    core::vector3df newTarget((bbox.MaxEdge.X + bbox.MinEdge.X)/2.0f, 
                        (bbox.MaxEdge.Y + bbox.MinEdge.Y)/2.0f, 
                        (bbox.MaxEdge.Z + bbox.MinEdge.Z)/2.0f);

                    core::vector3df newPos = newTarget + offsetNormal*distCT;//core::vector3df(0.0f,-1.0f,0.0f)
                    m_oriPosition = newPos;
                }

                cam->setPosition(m_oriPosition);
                cam->setTarget(m_oriTarget);

                m_rotCentre = m_oriTarget;

                float distFar = m_oriTarget.getDistanceFrom(m_oriPosition) * 2;
                if (distFar > cam->getFarValue())
                {
                    cam->setFarValue(distFar);
                }
            }
        }

        // 添加点云sn
        //IObjectSceneNode* CHd3DView::AddPcdData(const char* strPcdPath)
        //{
        //    CHdApplication* hdApp = CHdApplication::getAppInstance();

        //    // 添加点云数据
        //    PointCloud* pcd = hdApp->GetPointCloudByPath(strPcdPath,true);
        //    if (!pcd || pcd->count() <= 0)
        //    {
        //        return NULL;
        //    }

        //    // 添加sn
        //    CHdPcdObject pcdObj(pcd);
        //    IObjectSceneNode* pSn = AddObject(&pcdObj);
        //    if (pSn == NULL)
        //    {
        //        return NULL;
        //    }

        //    // 检查sn
        //    CScanSceneNode* pPcdSN = dynamic_cast<CScanSceneNode*>(pSn);
        //    if(pPcdSN == NULL)
        //    {
        //        //SendMsgToWindowsView(WM_USER_CLOSEVIEW,0,LPARAM(0));
        //        return NULL;
        //    }

        //    // 返回sn
        //    return pPcdSN;
        //}

        // 测试代码，截图功能
        void CHd3DView::screenShot()
        {
            irr::core::array<ISceneNode*> vecSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);

            for (unsigned int n = 0;n < vecSn.size();n++)
            {
                CDomSymbSceneNode* pDomSn = dynamic_cast<CDomSymbSceneNode*>(vecSn[n]);
                if (pDomSn)
                {
                    ITexture* pTexture = pDomSn->getTexture();
                    IImage* pImg = pTexture->getImage();
                    if (!pImg)
                    {
                        continue;
                    }

                    // 写文件
                    // 测试代码--start
                    IImageWriter* imgWrite = new CImageWriterJPG();
                    if (!imgWrite)
                    {
                        continue;
                    }

                    // 路径设置
                    string imagePath = "G:\\";
                    char strT[128];
                    sprintf_s(strT,"%d%s",n,".jpg");
                    string strP = strT;
                    imagePath += strP;

                    // 打印输出
                    irr::io::IWriteFile* imgFile = irr::io::createWriteFile(imagePath.data(),false);
                    if (!imgFile)
                    {
                        continue;
                    }

                    // 句柄资源释放
                    imgWrite->writeImage(imgFile,pImg);
                    imgFile->drop();
                    imgWrite->drop();
                    // 测试代码--end
                }
            }
        }

        // 根据传入路径获取dom sn
        IObjectSceneNode* CHd3DView::GetDomSceneNode(const char* strDomPath)
        {
            CDomSymbSceneNode* pSn = NULL;

            // 判断dom文件路径是否存在
            if (_access(strDomPath,04) != 0)
            {
                return pSn;
            }
            string strPath = strDomPath;
            std::transform(strPath.begin(),strPath.end(),strPath.begin(),tolower);

            // 判断是否需要new新的dom sn
            irr::core::array<ISceneNode*> vecSn;
            GetSceneManager()->getSceneNodesFromType(ESNT_HD_DOM,vecSn);

            // 根据路径判断，该dom文件是否已添加
            for (unsigned int n = 0;n < vecSn.size();n++)
            {
                // 转换
                CDomSymbSceneNode* pDomTmpSn = dynamic_cast<CDomSymbSceneNode*>(vecSn[n]);
                if (!pDomTmpSn)
                {
                    continue;
                }

                // 判断路径是否相同
                string strTmpPath = pDomTmpSn->GetDomTexturePah();
                std::transform(strTmpPath.begin(),strTmpPath.end(),strTmpPath.begin(),tolower);

                // 比较路径是否相同
                if (strcmp(strPath.data(),strTmpPath.data()) == 0)
                {
                    // 若相同则赋值并跳出循环
                    pSn = pDomTmpSn;
                    break;
                }
            }

            // 不论是否存在，返回指针
            return pSn;
        }
    }
}