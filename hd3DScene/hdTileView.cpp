#include "StdAfx.h"
#include "hdTileView.h"
#include <io.h>
#include "hdCommandDef.h"
#include "CImageWriterBMP.h"
#include "CImageWriterPNG.h"
#include "CImageWriterJPG.h"
#include "irrlicht.h"
#include "CImage.h"
#include "..\hdCommon\sceneData\HdFileData.h"
#include "..\hdCommon\point_types.h"

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

using namespace irr::video;
using namespace irr::io;

namespace hd
{
	namespace scene
	{
		CHdTileView::CHdTileView(void)
			:ISceneView()
		{
			m_viewType = E_HVT_QUICK ;
			m_pPanoSN = NULL;
		}

		CHdTileView::~CHdTileView(void)
		{

		}
				
		//! 判断某一个HDI索引对应全景影像是否存在，若存在，返回true,否则，返回false
		bool CHdTileView::isPanoExist( int index,s32 cameraNo )
		{
			if (index >= 0 && index < (int)(m_hdiVec.size()))
			{
				if (m_bFileMode)		// 影像以文件形式存在
				{
					// 获取影像文件目录
					string strImageFolder = GetImageFilePath(cameraNo);

					//获取hdi中的影像名
					if (strImageFolder != "")
					{
						const HD_HDIINFO tmpHdi = *(m_hdiVec._Myfirst + index);

						//解析轨迹点名
						string hdiName(tmpHdi.strImageName);

						string imgName = strImageFolder + "\\" + hdiName + IMG_EXT;

						// 判断文件是否存在
						if (_access_s(imgName.data(),00) == 0)
						{
							return true;
						}
						else
						{
							return false;
						}
					}
				}
				else					// 影像以数据库形式存在
				{
					// 得到数据库路径
					string strDBPath = GetImageDbPath(cameraNo);
					if (strDBPath != "")
					{
						const HD_HDIINFO& tmpHdi = *(m_hdiVec._Myfirst + index);

						// 得到数据库路径，未打开则打开数据
						if (!m_pImageDb->IsOpen())
						{
							m_pImageDb->Open(strDBPath.data());

						}

						// 目前未实现直接查询对应全景ID数据在数据库中是否存在
						HD_IMAGEDATA imageData;
						if (m_pImageDb->IsOpen() && m_pImageDb->ReadImage(tmpHdi.strImageName, imageData) && imageData.pImageData)
						{
							return 1;
						}
					}
				}
			}

			return false;
		}

		///本地切片浏览按角度获取切片
		bool CHdTileView::GetTileImagesByAngle( float fScale,float horiBeg,float horiEnd, float vertiBeg,float vertiEnd,const char* strPanoID,vector<vector<HD_SV_TILEINFO*>>& vecInfo )
		{


			if (fScale > 0 && fScale <= 10.0f)
			{
				// 比例越大，层级越小
				m_CurLevel = (ceil)((10.0f - fScale)/(0.5f*4));
			}
			else if (fScale < 0) // 超出范围默认加载第二层级
			{
				m_CurLevel = 5;
			}
			else if (fScale > 10.0f)
			{
				m_CurLevel = 1;
			}
			//判断该层切片是否存在
			if (NULL != strPanoID && m_pTileDBOpr->IsOpen())
			{
				bool bExist = IsCurLevelTilePanoExist(strPanoID,m_CurLevel);
				if (!bExist)
				{
					if (m_CurLevel > 0)
					{
						while (!bExist)
						{
						
							m_CurLevel -= 1;
							bExist = IsCurLevelTilePanoExist(strPanoID,m_CurLevel);
							if (m_CurLevel <= 0 )
							{
								break;
							}
							else if (m_CurLevel >= 5)
							{
								continue;
							}
						}
					}
				
					else if(m_CurLevel <= 0)
					{
						while (!bExist)
						{
						
							m_CurLevel += 1;
							bExist = IsCurLevelTilePanoExist(strPanoID,m_CurLevel);
							if (m_CurLevel >= 5)
							{
								break;
							}
						}
					}
				}
				/*if (!bExist)
				{
					if (m_CurLevel >= 4)
					{
						while (!bExist)
						{
							m_CurLevel -= 1;

							bExist = IsCurLevelTilePanoExist(strPanoID,m_CurLevel);
							if (m_CurLevel <= 0)
							{
								break;
							}
						}
					}
					else
					{
						while (!bExist)
						{
							m_CurLevel += 1;
							bExist = IsCurLevelTilePanoExist(strPanoID,m_CurLevel);
							if (m_CurLevel >= 5)
							{
								break;
							}
						}
					}
				
				}*/
			}


			//根据角度得到切片的ID，
			int nWtCount,nHtCount;
			nWtCount = pow(2.0,m_CurLevel);
			nHtCount = pow(2.0,m_CurLevel - 1);
			float horiStep,vertStep;

			// 水平、垂直角分辨率
			horiStep = 360.0f / nWtCount;
			vertStep = 180.0f / nHtCount;

			// 分别计算水平、垂直起始、终止角分辨率,行列起始编号从0开始，需向下取整
			int nStartCol,nEndCol,nStartRow,nEndRow;
			nStartCol = floor(horiBeg/horiStep); // 向下取整，则包括终止位置
			nEndCol = ceil(horiEnd/horiStep);    //xiang shang qu zheng 
			nStartRow = floor((90.0f - vertiBeg)/vertStep);

			nEndRow = ceil((90.0f - vertiEnd)/vertStep);

			// 若存在角度行列异常，互换起始、终止
			if (nStartRow > nEndRow)
			{
				int nTmp = nEndRow;
				nEndRow = nStartRow;
				nStartRow = nTmp;
			}
			if (nStartCol > nEndCol)
			{
				int nTmp = nEndCol;
				nEndCol = nStartCol;
				nStartCol = nTmp;
			}

			//记录索引
			m_nBegRow = nStartRow;
			m_nEndRow = nEndRow;
			m_nBegCol = nStartCol;
			m_nEndCol = nEndCol;
			// resize
			vector<vector<string>> vecTileIDs;
			vecTileIDs.resize(nEndRow - nStartRow);
			for (int n = 0;n < nEndRow - nStartRow;n++)
			{
				vector<string>& vecTt = *(vecTileIDs._Myfirst + n);
				vecTt.resize(nEndCol - nStartCol);
			}

			// 角度范围连续，构成的行列也连续
			for (int row = nStartRow;row < nEndRow;row++)
			{
				vector<string>& vecTt = *(vecTileIDs._Myfirst + row - nStartRow);
				for (int col = nStartCol;col < nEndCol;col++)
				{
					char strTitleID[512];
					sprintf_s(strTitleID,"%s-%d-%d-%d-%d",strPanoID,PANOTITLE_CRITERION,m_CurLevel,row,col);
					string strTitle(strTitleID);
					*(vecTt._Myfirst + col - nStartCol) = strTitle;
				}
			}
			if (vecTileIDs.size() <= 0)
			{
				return false;
			}
			vecInfo.resize(vecTileIDs.size());
			for (unsigned int n = 0;n < vecTileIDs.size();n++)
			{
				vector<string>& vecIDtmp = *(vecTileIDs._Myfirst + n);
				vector<HD_SV_TILEINFO*>& vecInfoTmp = *(vecInfo._Myfirst + n);
				vecInfoTmp.resize(vecIDtmp.size());
			}

			// vec赋值
			for (unsigned int n = 0;n < vecTileIDs.size();n++)
			{
				vector<string>& vecIDtmp = *(vecTileIDs._Myfirst + n);
				vector<HD_SV_TILEINFO*>& vecInfoTmp = *(vecInfo._Myfirst + n);
				for (unsigned int m = 0;m < vecIDtmp.size();m++)
				{
					string& strInfoID = *(vecIDtmp._Myfirst + m);
					HD_SV_TILEINFO*& pInfo = *(vecInfoTmp._Myfirst + m);
					AddTile2Cache(strInfoID.data(),pInfo);//添加到缓存中
					//pInfo = new HD_SV_TILEINFO;
					//strcpy_s(pInfo->strTileID,strInfoID.data());
				}
			}

			return true;
		}


		//查询并判断切片第0层
		bool CHdTileView::GetLocalZeroTilePano(HD_SV_TILEINFO*& pZeroInfo,const char* strPanoID)
		{

			// 组合构成0级切片ID
			int nLevel = 0;

			int col,row;
			col = row = 0;
			char strTitleID[512];
			sprintf_s(strTitleID,"%s-%d-%d-%d-%d",strPanoID, PANOTITLE_CRITERION, nLevel, row, col);

			bool bRet(false);
	
			AddTile2Cache(strTitleID,pZeroInfo);
			bRet = GetTileImage(strTitleID, *pZeroInfo);//根据0层切片ID获取切片影像

			return bRet;

		}


		bool CHdTileView::GetLocalPanoTitles( float fScale, float horiBeg, float horiEnd, float vertiBeg, float vertiEnd, const char* strPanoID, vector<vector<HD_SV_TILEINFO*>>& vecInfo )
		{
			// 内存获取数据，可能不包含影像信息
			bool bRet = GetTileImagesByAngle(fScale,horiBeg, horiEnd, vertiBeg, vertiEnd,strPanoID, vecInfo);
			// 条件判断
			if (!bRet)
			{
				return bRet;
			}

			// 从数据库中读取对应切片ID的切片数据，vec存储切片指针，外部获取时需做非空性判断
			for (unsigned int n = 0;n < vecInfo.size();n++)
			{
				//vector<string>& vecTtStr = *(vecTitleID._Myfirst + n);
				vector<HD_SV_TILEINFO*>& vecTitle = *(vecInfo._Myfirst + n);
				for (unsigned int m = 0;m < vecTitle.size();m++)
				{
					HD_SV_TILEINFO*& pInfo = *(vecTitle._Myfirst + m);
					// 获得切片
					if (pInfo == NULL) // 非空判断
					{
						continue;
					}
					else
					{
						// 内存中已存在影像数据，不做处理
						if (pInfo->nSize > 0)
						{
							continue;
						}
						// 若不存在，则请求本地，判断是否存在
						bRet = GetTileImage(pInfo->strTileID, *pInfo);

					}
				}
			}


		}

		//获取当前层级
		int CHdTileView::GetCurPanoLevel()
		{
			return m_CurLevel;
		}

		//将切片加入缓存
		bool CHdTileView::AddTile2Cache( const char* strTitleName,HD_SV_TILEINFO*& pInfo )
		{
			// 条件判断
			if (!strTitleName)
			{
				return false;
			}

			// 添加条件判断
			if (pInfo)
			{
				return true;
			}

			// 首先查找该切片ID的缓存是否已存在于内存中，若已存在，则直接返回处理
			double g_memBuf = 0.1;//储存空间大小阈值设置为0.1G
			bool bRet = false;
			bool bExist = CheckListPanoCacheExist(strTitleName,pInfo);
			if (bExist)
			{
				bRet = true;
			}
			else
			{
				// 如果内存过大，从顶部删除,y
				while(GetCacheListSize() > g_memBuf)
				{
					// 先取消，再删除
					list<HD_SV_TILEINFO*>::iterator iter = m_CachePanoList.begin();
					HD_SV_TILEINFO* pOld = (*iter);
					delete pOld;
					pOld = NULL;
					m_CachePanoList.pop_front();
				}

				pInfo = new HD_SV_TILEINFO;
				strcpy_s(pInfo->strTileID,strTitleName);
				m_CachePanoList.push_back(pInfo);
			}

			bRet = true;

			return bRet;
		}

		//获取缓存列表的内存大小
		double CHdTileView::GetCacheListSize()
		{
			float fMemory =0;
			// 循环查找
			for (list<HD_SV_TILEINFO*>::iterator iter = m_CachePanoList.begin();iter!=m_CachePanoList.end();iter++)
			{
				// 获得迭代器指针
				HD_SV_TILEINFO* pBuffer = (*iter);
				if (!pBuffer)
				{
					continue;
				}

				// 主要根据指针内部的缓存大小计算，其他字段可以忽略不计
				/*fMemory += pBuffer->nSize/1024/1024/1024;*/
				long nMem = 1024 * 1024 * 1024 * 1.0f;
				fMemory += 1.0f * pBuffer->nSize / nMem;

			}

			return fMemory;
		}

		//清空缓存
		void CHdTileView::ClearCache()
		{
			// 释放内存
			for (list<HD_SV_TILEINFO*>::iterator iter = m_CachePanoList.begin();iter != m_CachePanoList.end();iter++)
			{
				// 内存释放
				HD_SV_TILEINFO* pInfo = (*iter);
				if (pInfo)
				{
					delete pInfo;
					pInfo = NULL;
				}
			}

			// 清空缓存器
			m_CachePanoList.clear();
		}

		//缓存列表中对应全景切片是否存在
		bool CHdTileView::CheckListPanoCacheExist( const char* strTitleName,HD_SV_TILEINFO*& pInfoNew )
		{
			// 条件判断
			if (!strTitleName)
			{
				return false;
			}

			// 检查list中是否已存在
			bool bRet = false;
			for (list<HD_SV_TILEINFO*>::iterator iter = m_CachePanoList.begin();iter!=m_CachePanoList.end();iter++)
			{
				// 逐记录判断
				HD_SV_TILEINFO* pInfo = (*iter);
				if (!pInfo)
				{
					continue;
				}

				// 若存在则跳出循环
				if (strcmp(strTitleName,pInfo->strTileID) == 0)
				{
					pInfoNew = pInfo;
					bRet = true;
					break;
				}
			}

			// 返回结果
			return bRet;
		}

		//获取当前记录的角度范围
		void CHdTileView::GetCurTileIndexRange( int& nBegRow, int& nEndRow, int& nBegCol, int& nEndCol )
		{
			nBegRow = m_nBegRow;
			nBegCol = m_nBegCol;
			nEndRow = m_nEndRow;
			nEndCol = m_nEndCol;
		}


		//判断某一层级的切片是否存在
		bool CHdTileView::IsCurLevelTilePanoExist( const char* strPanoID ,int PanoLevel )
		{
			char TmpstrTitleID[512];
			sprintf_s(TmpstrTitleID,"%s-%d-%d-%d-%d",strPanoID, PANOTITLE_CRITERION, m_CurLevel, 0, 0);///行列号为0，0的切片存在则该层切片存在
			HD_SV_TILEINFO tmpInfo;
			bool bRet = m_pTileDBOpr->GetImageTile(TmpstrTitleID,tmpInfo);
		
			return bRet;
		}

		//判断本地切片是否存在且有效，有效的情况应为至少存在一张0层全景影像和两张该影像切割后的1层切片影像
		bool CHdTileView::IsTileDBValid(const char* strPanoID ,int PanoLevel)
		{
			if ( m_pTileDBOpr->GetTileCountFromTable() >= 3 )
			{
				bool bRet = IsCurLevelTilePanoExist(strPanoID,PanoLevel);
				return bRet;
			}
			else
			{
				return false;
			}
		}

	}
}
