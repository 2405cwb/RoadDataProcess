/*! @PanoTileAidHelper
********************************************************************************
<PRE>
模块名       : HD3DEngine
文件名       : PanoTileAidHelper.h
相关文件     : PanoTileAidHelper.cpp, CPanoSceneNode
文件实现功能 : 全景切片显示辅助类，用于实现全景切片显示
作者         : 软件部，朱旭波
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2016/05/03   1.0      朱旭波              新增加内容
</PRE>
*******************************************************************************/
#include "stdafx.h"
#include "PanoTileAidHelper.h"

#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "CTriangleSelector.h"
#include "SMesh.h"
#include "IAnimatedMesh.h"
#include <os.h>
#include <math.h>
#include <time.h>
#include "COpenGLExtensionHandler.h"

#include "..\hd3DEngine\CImage.h"

namespace hd
{
	#define PANOTITLE_CRITERION	1

	namespace scene
	{
		// 构造
		CPanoTileAidHelper::CPanoTileAidHelper(IHdView* pView)
		{
			m_pView = pView;

			m_strCurPanoID = "";
			m_nCurSphere = 1;
			m_pSphere_1_0 = NULL;
			m_pSphere_1_3 = NULL;
			m_pSphere_1_4 = NULL;
			m_pSphere_2_0 = NULL;
			m_pSphere_2_3 = NULL;
			m_pSphere_2_4 = NULL;
			m_CacheList.clear();

			// 初始化球体信息
			InitialSpheres();
		}

		// 析构
		CPanoTileAidHelper::~CPanoTileAidHelper(void)
		{
			// 清空缓存列表,此时数据内存也会释放
			ClearCache();

			// 内存释放
			if (m_pSphere_1_0)
			{
				delete m_pSphere_1_0;
				m_pSphere_1_0 = NULL;
			}

			// 内存释放
			if (m_pSphere_1_3)
			{
				delete m_pSphere_1_3;
				m_pSphere_1_3 = NULL;
			}

			// 内存释放
			if (m_pSphere_1_4)
			{
				delete m_pSphere_1_4;
				m_pSphere_1_4 = NULL;
			}

			// 内存释放
			if (m_pSphere_2_0)
			{
				delete m_pSphere_2_0;
				m_pSphere_2_0 = NULL;
			}

			// 内存释放
			if (m_pSphere_2_3)
			{
				delete m_pSphere_2_3;
				m_pSphere_2_3 = NULL;
			}

			// 内存释放
			if (m_pSphere_2_4)
			{
				delete m_pSphere_2_4;
				m_pSphere_2_4 = NULL;
			}
		}

		// 初始化设置球体信息
		void CPanoTileAidHelper::InitialSpheres()
		{
			// new对应球体对象-1号
			m_pSphere_1_0 = new CPanoTileSphere(m_pView,1,1,12.0f);
			m_pSphere_1_3 = new CPanoTileSphere(m_pView,8,4,10.0f);
			m_pSphere_1_4 = new CPanoTileSphere(m_pView,16,8,8.0f);

			// new对应球体对象-1号
			m_pSphere_2_0 = new CPanoTileSphere(m_pView,1,1,12.0f);
			m_pSphere_2_3 = new CPanoTileSphere(m_pView,8,4,10.0f);
			m_pSphere_2_4 = new CPanoTileSphere(m_pView,16,8,8.0f);
		}

		// 根据传入参数获取数据,在此前，若为定位，则需先调用获取0级数据用于更新设置新球、全景ID等
		void CPanoTileAidHelper::GetPanoTiles( irr::core::vector3df pos,core::matrix4 m,
			vector<CHdSvTileInfoBuffer*>& vecTileBuffer)
		{
			// 视图存在性判断
			if (!m_pView)
			{
				return;
			}
			ISceneView* pSceneView = (ISceneView*)(m_pView);
			
			// 由视图获得对应比例值
			float fScale = 0.0f;
			fScale = pSceneView->GetDisplayScale();
			int nCurLevel = 0;
			if (fScale <= 4.0f)
			{
				nCurLevel = 4;
			}
			else
			{
				nCurLevel = 3;
			}

			// 记录当前层级，用于后续渲染时使用
			m_nCurLevel = nCurLevel;

			// 根据层级及当前球索引进行计算,获取需要从本地or服务器获取的切片数据集vecTileBuffer
			if (m_nCurSphere == 1)
			{
				// 球1，层级3
				if (nCurLevel == 3)
				{
					CalPanoTile(nCurLevel,m_pSphere_1_3,pos,m,vecTileBuffer);
				}
				else if (nCurLevel == 4) // 球1，层级4
				{
					CalPanoTile(nCurLevel,m_pSphere_1_4,pos,m,vecTileBuffer);
				}
			}
			else if (m_nCurSphere == 2)
			{
				// 球2，层级3
				if (nCurLevel == 3)
				{
					CalPanoTile(nCurLevel,m_pSphere_2_3,pos,m,vecTileBuffer);
				}
				else if (nCurLevel == 4) // 球2，层级4
				{
					CalPanoTile(nCurLevel,m_pSphere_2_4,pos,m,vecTileBuffer);
				}
			}
		}
		
		// 传入全景ID，若与原记录不同，则更换球体
		void CPanoTileAidHelper::SetPanoID( const char* strNewPanoID )
		{
			// 获得全景新ID
			string strNewID = strNewPanoID;

			// 标记判断是否需要做清空处理
			bool bNeedClear = false;
			if (m_strCurPanoID != "")
			{
				// 表示需要更换
				if (strcmp(strNewPanoID,m_strCurPanoID.data())!= 00)
				{
					bNeedClear = true;
				}
			}
			else
			{
				m_strCurPanoID = strNewID;
			}

			if (bNeedClear)
			{
				// 站点切换，需要清除之前的缓存
				ClearCache();

				// 清除标记球纹理
				ClearSphereTexture(m_nCurSphere);

				// 更改标记球ID,记录全景ID
				m_nCurSphere = (m_nCurSphere==1) ? 2:1;
				m_strCurPanoID = strNewID;
			}
		}

		// 清空指定球纹理
		void CPanoTileAidHelper::ClearSphereTexture( int nSphereID )
		{
			// 仅适用1,2来进行标记
			if (nSphereID != 1 && nSphereID != 2)
			{
				return;
			}

			// 根据条件进行清空纹理
			if (nSphereID == 1)
			{
				// 0级
				if (m_pSphere_1_0)
				{
					m_pSphere_1_0->clearAllTextures();
				}

				// 3级
				if (m_pSphere_1_3)
				{
					m_pSphere_1_3->clearAllTextures();
				}

				// 4级
				if (m_pSphere_1_4)
				{
					m_pSphere_1_4->clearAllTextures();
				}
			}
			else if (nSphereID == 2)
			{
				// 0级
				if (m_pSphere_2_0)
				{
					m_pSphere_2_0->clearAllTextures();
				}

				// 3级
				if (m_pSphere_2_3)
				{
					m_pSphere_2_3->clearAllTextures();
				}

				// 4级
				if (m_pSphere_2_4)
				{
					m_pSphere_2_4->clearAllTextures();
				}
			}
		}

		// 根据全景ID获取0级切片数据
		CHdSvTileInfoBuffer* CPanoTileAidHelper::GetZeroPanoTile( const char* strPanoID )
		{
			// 条件性判断
			if (!strPanoID)
			{
				return NULL;
			}

			// 设置ID，根据条件判断是否需要清空
			bool bNameChanged = false;
			SetPanoID(strPanoID);

			// 组合构成0级切片ID
			int nLevel = 0;
			int col,row;
			col = row = 0;
			char strTitleID[512];
			sprintf_s(strTitleID,"%s-%d-%d-%d-%d",strPanoID, PANOTITLE_CRITERION, nLevel, row, col);

			// 检查缓存是否已存在
			CHdSvTileInfoBuffer* pZeroTile = NULL;
			bool bExist = CheckTileCacheExist(strTitleID,pZeroTile);

			// 若已存在，则不需重新new对象
			if (bExist && pZeroTile)
			{
				return pZeroTile;
			}
			else
			{
				// new一个切片对象,由缓存器管理，当站点切换时清空缓存
				pZeroTile = new CHdSvTileInfoBuffer(strTitleID);
				m_CacheList.push_back(pZeroTile);
			}

			return pZeroTile;
		}

		// 清空缓存列表,此时数据内存也会释放
		void CPanoTileAidHelper::ClearCache()
		{
			// 用于记录线程池部分未处理完成的buf
			std::vector<CHdSvTileInfoBuffer*> vecTile;

			// 先取消之前的线程执行,释放内存
			for (list<CHdSvTileInfoBuffer*>::iterator iter = m_CacheList.begin();iter != m_CacheList.end();iter++)
			{
				// 内存释放
				CHdSvTileInfoBuffer* pInfo = (*iter);
				if (pInfo)
				{
					// 先取消操作，由于是由多线程处理，cancel操作并不能保证处理完成，共享内存可能出现异常
					pInfo->cancel();

					// 内存部分处理完成后，表明已从多线程退出，此时异常内存
					if (pInfo->isFinished())
					{
						// 内存释放
						delete pInfo;
						pInfo = NULL;
					}
					else
					{
						vecTile.push_back(pInfo);
					}
				}
			}

			// 清空处理
			m_CacheList.clear();

			//// 尝试释放剩余内存
			//for (unsigned int m = 0;m < vecTile.size();m++)
			//{
			//	CHdSvTileInfoBuffer* pInfo = *(vecTile._Myfirst + m);
			//	if (pInfo)
			//	{
			//		// 内存释放
			//		delete pInfo;
			//		pInfo = NULL;
			//	}
			//}
			//vecTile.clear();

			// 重新将未处理完成的buf重新push进m_CacheList
			for (unsigned int m = 0;m < vecTile.size();m++)
			{
				CHdSvTileInfoBuffer* pInfo = vecTile.at( m);
				m_CacheList.push_back(pInfo);
			}
		}

		// 设置0级纹理
		void CPanoTileAidHelper::SetZeroPanoTexture( CHdSvTileInfoBuffer* pZeroInfo )
		{
			// 条件判断，根据当前球进行设置
			if (m_nCurSphere == 1)
			{
				m_pSphere_1_0->SetZeroTexture(pZeroInfo);
			}
			else if (m_nCurSphere == 2)
			{
				m_pSphere_2_0->SetZeroTexture(pZeroInfo);
			}
		}

		// 内部计算，球体切片设置获取(适用于计算三、四级球体)
		void CPanoTileAidHelper::CalPanoTile( int nLevel,CPanoTileSphere* pTileSphere,irr::core::vector3df pos,core::matrix4 m,vector<CHdSvTileInfoBuffer*>& vecTileBuffer )
		{
			// 用于计算的sn 位置、矩阵参数传入
			pTileSphere->SetPanoSnPosition(pos);
			pTileSphere->SetPanoSnMatrix(m);

			// 计算获取当前视口范围内有哪些切片
			vector<int> vecInView;
			vecInView = pTileSphere->CalInView();

			// 获得水平、垂直切片个数
			int nHoriCount = pTileSphere->GetHoriTileCount();
			int nVertCount = pTileSphere->GetVertTileCount();
			
			// 遍历处理
			// 计算
			for (int nHori = 0;nHori < nHoriCount;nHori++)
			{
				// 垂直
				for (int nVert = 0;nVert < nVertCount;nVert++)
				{
					// 索引计算
					int index = nHori * nVertCount + nVert;

					// 不在视口范围内数据不考虑
					if (vecInView[index] != 1)
					{
						continue;
					}

					// 获取纹理
					ITexture* pTexture = pTileSphere->GetTexture(index);
					if (pTexture) // 存在则不作任何处理
					{
						continue;
					}

					// 组合构成切片ID
					int col,row;
					col = nHori;
					row = nVert;
					char strTitleID[512];
					sprintf_s(strTitleID,"%s-%d-%d-%d-%d",m_strCurPanoID.data(), PANOTITLE_CRITERION, nLevel, row, col);

					// 不存在，首先根据切片ID检查缓存器中是否存在
					CHdSvTileInfoBuffer* pInfoBuf = NULL;
					bool bExist = CheckTileCacheExist(strTitleID,pInfoBuf);

					// 若缓存器中存在，需要进一步检查数据对象是否已下载下来
					if (bExist && pInfoBuf)
					{
						// 若二进制数据已经获取完成，则加入纹理中
						if (pInfoBuf->GetSize() > 0) // 表明多线程已经下载完成，数据共享已完成
						{
							pTileSphere->SetTexture(index,pInfoBuf);
						}

						// 继续下一个记录
						continue;
					}
					
					// 不存在，则需new对象，添加缓存，并交由多线程进行处理，此处予以记录
					pInfoBuf = new CHdSvTileInfoBuffer(strTitleID);
					m_CacheList.push_back(pInfoBuf);
					vecTileBuffer.push_back(pInfoBuf);
				}
			}
		}

		// 检查内存缓存是否存在
		bool CPanoTileAidHelper::CheckTileCacheExist( const char* strTitleName,CHdSvTileInfoBuffer*& pInfoBuf )
		{
			// 条件判断
			if (!strTitleName)
			{
				return false;
			}

			// 检查list中是否已存在
			bool bRet = false;
			for (list<CHdSvTileInfoBuffer*>::iterator iter = m_CacheList.begin();iter!=m_CacheList.end();iter++)
			{
				// 逐记录判断
				CHdSvTileInfoBuffer* pInfo = (*iter);
				if (!pInfo)
				{
					continue;
				}

				// 若存在则跳出循环
				if (strcmp(strTitleName,pInfo->GetTileID()) == 0)
				{
					pInfoBuf = pInfo;
					bRet = true;
					break;
				}
			}

			// 返回结果
			return bRet;
		}

		// pano sn render时调用
		void CPanoTileAidHelper::Render()
		{
			// 计算当前渲染哪一层级
			if (m_nCurSphere == 1)
			{
				// 至少应保证0级切片球存在纹理
				ITexture* pTexture = m_pSphere_1_0->GetZeroTexture();
				if (!pTexture)
				{
					return;
				}

				// 渲染即可
				m_pSphere_1_0->Render();

				// 判断渲染三、四级
				if (m_nCurLevel == 3)
				{
					m_pSphere_1_3->Render();
				}
				else if (m_nCurLevel == 4)
				{
					m_pSphere_1_3->Render();
					m_pSphere_1_4->Render();
				}
			}
			else if (m_nCurSphere == 2)
			{
				// 至少应保证0级切片球存在纹理
				ITexture* pTexture = m_pSphere_2_0->GetZeroTexture();
				if (!pTexture)
				{
					return;
				}

				// 渲染0级
				m_pSphere_2_0->Render();

				// 判断渲染三、四级
				if (m_nCurLevel == 3)
				{
					m_pSphere_2_3->Render();
				}
				else if (m_nCurLevel == 4)
				{
					m_pSphere_2_3->Render();
					m_pSphere_2_4->Render();
				}
			}
		}

		// 获得0级切片纹理
		ITexture* CPanoTileAidHelper::GetZeroPanoTexture()
		{
			// 条件判断，根据当前球进行设置
			if (m_nCurSphere == 1)
			{
				return m_pSphere_1_0->GetZeroTexture();
			}
			else if (m_nCurSphere == 2)
			{
				return m_pSphere_2_0->GetZeroTexture();
			}
		}

		// 获得包围盒
		core::aabbox3d<f32>& CPanoTileAidHelper::getBoundingBox()
		{
			// 当前球判断
			if (m_nCurSphere == 1)
			{
				return m_pSphere_1_0->getBoundingBox();
			}
			else 
			{
				return m_pSphere_2_0->getBoundingBox();
			}
		}

	}
}

