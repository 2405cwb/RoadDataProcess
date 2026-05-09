/*! SeaPointCloud.cpp
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : SeaPointCloud.cpp
相关文件     : SeaPointCloud.h
文件实现功能 : 海量点云内存结构
作者         : 蔡红云
版本         : 1.0
版权		 : CopyRight @ 2015 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/04/11   1.0      蔡红云
2015/10/6    1.1      蔡红云              增加按POS高度过滤、按POS距离过滤
2015/11/03   1.2      朱旭波			  增加从monogo服务器读取数据
</PRE>
*******************************************************************************/
#include "StdAfx.h"
#include "SeaPointCloud.h"
#include "..\hdHLSlib\HLSReadOpener.h"
#include "..\hdHLSlib\HlzDefs.h"
#include "..\hdCommon\point_types2.h"
#include "..\hdHLSlib\HdLevel.h"
#include "..\hdHLSlib\HdBlock.h"
#include "..\hdHLSlib\HdBlockset.h"
#include "..\hdHLSlib\HdParcel.h"
#include <algorithm>
#include <time.h>
#include "..\hdHlslib\HLZWriter.h"
#include "hdLog.h"

namespace hd
{

	CSeaPointCloud::CSeaPointCloud(void):m_hlzReader(NULL)
	{
		::InitializeCriticalSectionAndSpinCount( &m_cs, 0x80000402);
		m_byPosFilter = -1;

		m_sel = 0;
		m_FilterminHeight = 0;
		m_FiltermaxHeight = 0;
		m_bhigh = TRUE;

		m_bless = TRUE;
		m_distance = 0.0;

		m_strRouteName = "";
		m_nScanNo = -1;

		m_pFilterManager = new hd::ptcloud::hdFilterManager;
	}


	CSeaPointCloud::~CSeaPointCloud(void)
	{
		if (m_hlzReader)
		{
			delete m_hlzReader;
			m_hlzReader = NULL;
		}

		delete m_pFilterManager;
		m_pFilterManager = NULL;

		// 释放apha内存
		ClearAMry();

		::DeleteCriticalSection(&m_cs);
	}

	// 打开文件
	BOOL CSeaPointCloud::open(const char* hlzPath)
	{
		EnterCriticalSection(&m_cs);
		BOOL bRet = FALSE;

		// 点云文件打开管理对象
		CHLSReadOpener hlsOpener;
		m_hlzReader = hlsOpener.Open(hlzPath);
		if (m_hlzReader)
		{
			m_hlzHeader = m_hlzReader->m_hlzHeader;
			bRet = TRUE;
		}
		LeaveCriticalSection(&m_cs);
		return bRet;
	}

	// 设置点云中的模型
	void CSeaPointCloud::SetModel(CBursaWolfModel model,bool bNeedCal)
	{
		m_transModel = model;

		if (bNeedCal)
		{
			m_transModel.matrix2Parameter();
		}

		m_hlzHeader.offsetX = m_transModel.m_fOffset[0];
		m_hlzHeader.offsetY = m_transModel.m_fOffset[1];
		m_hlzHeader.offsetZ = m_transModel.m_fOffset[2];
		m_hlzHeader.rotateX = m_transModel.m_fAngle[0];
		m_hlzHeader.rotateY = m_transModel.m_fAngle[1];
		m_hlzHeader.rotateZ = m_transModel.m_fAngle[2];
		m_hlzHeader.scale =  m_transModel.m_fScale;

		SetPointCloudEdited(true);
	}

	// 获取点云中的模型
	const CBursaWolfModel& CSeaPointCloud::GetModel()
	{

		m_transModel.m_fOffset[0] =  m_hlzHeader.offsetX;
		m_transModel.m_fOffset[1] =  m_hlzHeader.offsetY;
		m_transModel.m_fOffset[2] =  m_hlzHeader.offsetZ;
		m_transModel.m_fAngle[0]  =  m_hlzHeader.rotateX;
		m_transModel.m_fAngle[1]  =  m_hlzHeader.rotateY;
		m_transModel.m_fAngle[2]  =  m_hlzHeader.rotateZ;
		m_transModel.m_fScale     =  m_hlzHeader.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();
		return m_transModel;
	}

	// 计算透明度信息
	void CSeaPointCloud::CalAphaInfo(CHdParcelBase* pHdParcelBase ,int maxinsity, int minintensity)
	{
		EnterCriticalSection(&m_cs);

		if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
		{
			LeaveCriticalSection(&m_cs);
			return;
		}

		//获取点数
		int count = pHdParcelBase->GetPtCount();

		if (count <=0 )
		{
			LeaveCriticalSection(&m_cs);
			return;
		}

		// 判断强度列表中是否已经存在，如果存在就不计算
		if (m_apha.count(pHdParcelBase->GetCoordAddr().GetAddress()))
		{
			LeaveCriticalSection(&m_cs);
			return;
		}

		u8* pApha = new unsigned char[count];

		float colorI = 1.0f; // 强度

		// 强度步长
		int intensityStep = maxinsity- minintensity;

		// 获取数据
		for (int ptIndex=0;ptIndex<count;ptIndex++ )
		{
			HlzPoint* pPoint = (pHdParcelBase->m_pHlzPoint+ptIndex);

			if (!pPoint)
			{
				continue;
			}

			// 实时计算颜色
			colorI = (float)((pPoint->intensity - minintensity) * 0.85f/ intensityStep);
			colorI+=0.1f;

			if (colorI < 0.1)
			{
				colorI = 0.1f;
			}

			else if (colorI > 0.95)
			{
				colorI = 0.95f;
			}

			u8 apha =  u8(colorI*255);

			*(pApha+ptIndex) = apha;

		}//for (int ptIndex=0;ptIndex<count;ptIndex++ )

		m_apha.insert(make_pair(pHdParcelBase->GetCoordAddr().GetAddress(),pApha));
		LeaveCriticalSection(&m_cs);
	}

	// 释放Apha内存
	void CSeaPointCloud::ReleaseAMry(U64 index)
	{
		EnterCriticalSection(&m_cs);

		// 进行查找
		map<U64,u8* >::iterator it = m_apha.find(index);

		if (it!=m_apha.end())
		{
			u8* pAha = m_apha.at(index);

			if (pAha)
			{
				delete [] pAha;
				pAha = NULL;
			}
			m_apha.erase(it);
		}

		LeaveCriticalSection(&m_cs);
	}

	// 释放所有透明度内存
	void CSeaPointCloud::ClearAMry()
	{
		EnterCriticalSection(&m_cs);
		map<u64,u8*>::iterator it;
		for (it = m_apha.begin();it != m_apha.end();++it)
		{
			if(it->second)
			{
				delete it->second;
				it->second = NULL;
			}
		}
		m_apha.clear();
		LeaveCriticalSection(&m_cs);
	}

	// 返回显示区域列表
	CHdListAreaNoInf& CSeaPointCloud::GetHdListAreaNoInf()
	{
		return m_HdlstArea;
	}

	void CSeaPointCloud::SetHdListAreaNoInf(CHdListAreaNoInf& pListAreaInfo)
	{
		m_HdlstArea = pListAreaInfo;
	}

	// 返回透明度字典表
	map<U64,hd::u8* >& CSeaPointCloud::GetAphaList()
	{
		return m_apha;
	}

	// 寻找最近的HDI x,y,z 坐标   vechdi 轨迹点数组
	void CSeaPointCloud::FindNearHdi(double&x,double& y, double& z ,std::vector<HD_SCANHDIINFO>& vechdi)
	{

		if (vechdi.empty())
		{
			return;
		}

		// 求取最近距离
		double mindist = sqrt((vechdi[0].dX- x)*(vechdi[0].dX- x) + (vechdi[0].dY- y)*(vechdi[0].dY- y)
			);
		int mini = 0;

		for (int i= 1; i<vechdi.size(); i++)
		{

			double distTemp = sqrt((vechdi[i].dX- x)*(vechdi[i].dX- x) + (vechdi[i].dY- y)*(vechdi[i].dY- y)
				);
			if (mindist > distTemp) // 更新信息
			{
				mindist = distTemp;
				mini = i;
			}

		}

		x = vechdi[mini].dX;
		y = vechdi[mini].dY;
		z = vechdi[mini].dZ;

	}

	// 取消选中点
	void CSeaPointCloud::DeselectPts()
	{
		EnterCriticalSection(&m_cs);
		int size = m_HdlstArea.size();
		if (size <=0 )
		{
			LeaveCriticalSection(&m_cs);
			return;
		}

		for (int k=0;k < size;k++)
		{
			// 获取数据
			CHdParcelBase * pHdParcelBase = m_HdlstArea[k];

			if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
			{
				continue;
			}

			//获取点数
			int count = pHdParcelBase->GetPtCount();

			// 获取数据
			for (int i=0;i<count;i++ )
			{
				HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

				if(!pt->isValid())
				{
					continue;
				}

				if (pt->isSelected())
				{
					pt->setUnSelected();
				}

			}
		}

		LeaveCriticalSection(&m_cs);

	}

	// 按pos高度过滤
	int CSeaPointCloud::FilterPcdByPosHeight()
	{

		std::vector<HD_SCANHDIINFO> vechdi;

		// 初始化
		if (!InitFltByPosInf(vechdi))
		{
			return -3;// lin文件有问题
		}

		EnterCriticalSection(&m_cs);
		int size = m_HdlstArea.size();

		if (size <=0)
		{
			LeaveCriticalSection(&m_cs);
			return 1;// 点云没有加载
		}

		for (int k=0;k <size;k++)
		{
			// 获取数据
			CHdParcelBase * pHdParcelBase = m_HdlstArea[k];

			if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
			{
				continue;
			}

			//获取点数
			int count = pHdParcelBase->GetPtCount();

			if (count<=0)
			{
				continue;
			}

			double posx = pHdParcelBase->GetExtent().getCenter().X;
			double posy = pHdParcelBase->GetExtent().getCenter().Y;
			double posz = pHdParcelBase->GetExtent().getCenter().Z;

			m_transModel.Translate(posx,posy,posz);

			// 查找最近hdi
			FindNearHdi(posx,posy,posz,vechdi);

			// 获取数据
			for (int i=0;i<count;i++ )
			{
				HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

				double x = pt->x;
				double y = pt->y;
				double z = pt->z;

				m_transModel.Translate(x,y,z);

				pt->setUnSelected();

				if (m_sel==2 )
				{
					// 根据高度关系,判读是否选中
					if (z > (posz + m_FilterminHeight) &&
						z < (posz+ m_FiltermaxHeight))
					{
						pt->setSelected();
					}
				}
				else
				{
					// 根据高度关系,判读是否选中
					if ((m_bhigh && z > (posz + m_FilterminHeight)) ||
						(!m_bhigh && z < (posz + m_FilterminHeight)))
					{
						pt->setSelected();
					}

				}//for (int i=0;i<count;i++ )
			} //for (int k=0;k <size;k++)
		}//for (int k=0;k <size;k++)

		LeaveCriticalSection(&m_cs);
		return 1;
	}

	// 按高度过滤
	int CSeaPointCloud::FilterPcdByHeight()
	{		
		EnterCriticalSection(&m_cs);
		int size = m_HdlstArea.size();

		if (size <=0)
		{
			LeaveCriticalSection(&m_cs);
			return 1;// 点云没有加载
		}

		for (int k=0;k <size;k++)
		{
			// 获取数据
			CHdParcelBase * pHdParcelBase = m_HdlstArea[k];

			if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
			{
				continue;
			}

			//获取点数
			int count = pHdParcelBase->GetPtCount();

			if (count<=0)
			{
				continue;
			}

			double posx = pHdParcelBase->GetExtent().getCenter().X;
			double posy = pHdParcelBase->GetExtent().getCenter().Y;
			double posz = pHdParcelBase->GetExtent().getCenter().Z;

			m_transModel.Translate(posx,posy,posz);

			
			// 获取数据
			for (int i=0;i<count;i++ )
			{
				HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

				double x = pt->x;
				double y = pt->y;
				double z = pt->z;

				m_transModel.Translate(x,y,z);

				pt->setUnSelected();

				if (m_sel==2 )
				{
					// 根据高度关系,判读是否选中
					if (z > m_FilterminHeight &&
						z < m_FiltermaxHeight)
					{
						pt->setSelected();
					}
				}
				else
				{
					// 根据高度关系,判读是否选中
					if ((m_bhigh && z >  m_FilterminHeight) ||
						(!m_bhigh && z < m_FilterminHeight))
					{
						pt->setSelected();
					}

				}//for (int i=0;i<count;i++ )
			} //for (int k=0;k <size;k++)
		}//for (int k=0;k <size;k++)

		LeaveCriticalSection(&m_cs);
		return 1;
	}

	int  CSeaPointCloud::FilterPcdByPosDistace()
	{

		std::vector<HD_SCANHDIINFO> vechdi;
		string strLinPath;

		// 初始化
		if (!InitFltByPosInf(vechdi))
		{
			return -3;// lin文件有问题
		}


		EnterCriticalSection(&m_cs);

		int size = m_HdlstArea.size();
		if (size <=0 )
		{
			LeaveCriticalSection(&m_cs);
			return 1;// 点云没有加载
		}

		for (int k=0;k <size;k++)
		{
			// 获取数据
			CHdParcelBase * pHdParcelBase = m_HdlstArea[k];

			if (!pHdParcelBase || !pHdParcelBase->m_pHlzPoint) // 如果数据为空就执行下次
			{
				continue;
			}

			//获取点数
			int count = pHdParcelBase->GetPtCount();

			if (count<=0)
			{
				continue;
			}

			double posx = pHdParcelBase->GetExtent().getCenter().X;
			double posy = pHdParcelBase->GetExtent().getCenter().Y;
			double posz = pHdParcelBase->GetExtent().getCenter().Z;

			m_transModel.Translate(posx,posy,posz);

			// 查找最近hdi
			FindNearHdi(posx,posy,posz,vechdi);

			// 获取数据
			for (int i=0;i<count;i++ )
			{
				HlzPoint* pt = (pHdParcelBase->m_pHlzPoint+i);

				double x = pt->x;
				double y = pt->y;
				double z = pt->z;

				m_transModel.Translate(x,y,z);

				pt->setUnSelected();

				double xx = posx - x;
				double yy = posy - y;
				double zz = posz - z;

				double disTmp = sqrt(xx*xx + yy * yy + zz * zz);

				// 根据距离关系,判读是否选中
				if (m_bless)
				{
					if (disTmp < m_distance)
					{
						pt->setSelected();
					}
				}
				else
				{
					if (disTmp > m_distance)
					{
						pt->setSelected();
					}
				}
			}//for (int i=0;i<count;i++ )
		}//for (int k=0;k <size;k++)

		LeaveCriticalSection(&m_cs);
		return 1;

	}

	// 设置高度下限	
	void CSeaPointCloud::SetFilterminHeight(double h) 
	{
		m_FilterminHeight = h;
	}

	// 设置高度上限
	void CSeaPointCloud::SetFiltermaxHeight(double h)
	{
		m_FiltermaxHeight = h;
	}

	// 设置过滤模式
	void CSeaPointCloud::SetSel(int sel)
	{
		m_sel = sel;
	}

	// 是否高于
	void CSeaPointCloud::Setbhigh(BOOL bh)
	{
		m_bhigh = bh;
	}

	// 是否下于
	void CSeaPointCloud::SetBless(BOOL bh)
	{
		m_bless = bh;
	}

	// 设置距离
	void CSeaPointCloud::SetDistance(double d)
	{
		m_distance = d;
	}

	// 设置按pos过滤的方式 fs -1 ，0 按高度,1按距离
	void CSeaPointCloud::SetbyPosFilter(int fs)
	{
		m_byPosFilter = fs;
	}

	// 更新按pos进行过滤的点云
	void CSeaPointCloud::UpdateFilterPcd()
	{
		// 按Pos高度过滤
		if (m_byPosFilter == 0)
		{
			FilterPcdByPosHeight();
		}
		else if (m_byPosFilter == 1) // 按Pos距离过滤
		{
			FilterPcdByPosDistace();
		}
		else if (m_byPosFilter == 2)//按高度过滤
		{
			FilterPcdByHeight();
		}
	}

	// 字符串替代
	void CSeaPointCloud::replace_all_distinct(string&str,const string & oldstr,const string& newstr)
	{
		for (string::size_type pos(0);pos!=string::npos; pos+=newstr.length())
		{
			if (pos=str.find(oldstr,pos)!=string::npos)
			{
				str.replace(pos,oldstr.length(),newstr);
			}
			else
			{
				break;
			}
		}
	}

	// 初始化按Pos过滤信息 vechdi hdi轨迹点信息
	bool CSeaPointCloud::InitFltByPosInf(std::vector<HD_SCANHDIINFO>& vechdi)
	{
		// 清除
		vechdi.clear();
		HD_SCANHDIINFO::Serialize(m_linPathForFlt.c_str(),vechdi);
		if (vechdi.size() == 0)
		{
			return false;
		}

		// 获取模型
		GetModel();

		return true;

	}

	// 设置过滤时所使用的Lin文件
	void CSeaPointCloud::SetLinPahForFltr(const string& path)
	{
		m_linPathForFlt = path;
	}
	
	// 加载hlz文件头
	BOOL CSeaPointCloud::loadHlzFileHeader(const char* hlzFile)
	{
		if(m_hlzReader == NULL)
		{
			CHLSReadOpener hlsOpener;
			m_hlzReader = hlsOpener.Open(hlzFile);
		}

		if(m_hlzReader == NULL)
		{
			return FALSE;
		}
		m_hlzHeader = m_hlzReader->GetHlzHeader();
		m_transModel.m_fOffset[0] = m_hlzHeader.offsetX;
		m_transModel.m_fOffset[1] = m_hlzHeader.offsetY;
		m_transModel.m_fOffset[2] = m_hlzHeader.offsetZ;
		m_transModel.m_fAngle[0] =  m_hlzHeader.rotateX;
		m_transModel.m_fAngle[1] =  m_hlzHeader.rotateY;
		m_transModel.m_fAngle[2] =  m_hlzHeader.rotateZ;
		m_transModel.m_fScale =     m_hlzHeader.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();
		return TRUE;
	}

	string CSeaPointCloud::GetLinPath()
	{	
		// 获取点云文件路径
		string strPath = GetPointCloudPath();

		string strLinPath = strPath;
		int pos = strLinPath.find_last_of("\\");
		string strName = strLinPath.substr(pos + 1);

		if (pos != string::npos)
		{
			pos = strName.find(".hlz");
			strName.replace(pos,4,".lin");
			std::transform(strName.begin(),strName.end(),strName.begin(),tolower);
			pos = strName.find("-pcd-");
			if (pos != string::npos)
			{				
				strName.replace(pos,5,"-pos-");
				pos = strPath.find_last_of("\\");
				strLinPath = strPath.substr(0,pos + 1) + strName;
				if (_access(strLinPath.c_str(), 04) == 0)
				{
					return strLinPath;
				}

			}//if (pos != string::npos)

		}//if (pos != string::npos)

		return "";
	}

	 hd::u32 CSeaPointCloud::QueryByExtent( f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax )
	 {
		 if(m_hlzReader == NULL)
		 {
			 return 0;
		 }

		 u32 index = 0;
		 u32 colPtCount = 0;
     
		 if (m_hlzReader->m_hlzHeader.number_of_col <= 0)
		 {
			 return 0;
		 }

		 // [1] 得到CHdCoreData
		 CHdCoreData* pCoreData = (CHdCoreData*)(m_hlzReader);
		 if (!pCoreData)
		 {
			 return index;
		 }

		 // [2] 构造范围包围盒以便查询
		 CHdBox3df box;
		 box.MaxEdge.set(xmax, ymax, zmax);
		 box.MinEdge.set(xmin, ymin, zmin);

		 m_pListAreaNoInf.clear();

		 // [3] 获取第0层级在范围内的所有Arealist
		 CHdLevel* pLevel = pCoreData->GetLevelRec(0);
		 pCoreData->GetIntersectAreas(pLevel, box, &m_pListAreaNoInf);
     
		 index = m_pListAreaNoInf.size();          

		 return index;
	 }

	 hd::u32 CSeaPointCloud::QueryByExtent( f64 xmin,f64 ymin,f64 zmin,f64 xmax,f64 ymax,f64 zmax )
	 {
		 if(m_hlzReader == NULL)
		 {
			 return 0;
		 }

		 const CBursaWolfModel& transModel = GetModel();
		 // 底平面4个点
		 double x1 = xmin,y1 = ymin,z1 = zmin;
		 double x2 = xmax,y2 = ymin,z2 = zmin;
		 double x3 = xmax,y3 = ymax,z3 = zmin;
		 double x4 = xmin,y4 = ymax,z4 = zmin;
		 // 顶平面4个点
		 double x5 = xmin,y5 = ymin,z5 = zmax;
		 double x6 = xmax,y6 = ymin,z6 = zmax;
		 double x7 = xmax,y7 = ymax,z7 = zmax;
		 double x8 = xmin,y8 = ymax,z8 = zmax;

		 transModel.AntiTranslate(x1,y1,z1);
		 transModel.AntiTranslate(x2,y2,z2);
		 transModel.AntiTranslate(x3,y3,z3);
		 transModel.AntiTranslate(x4,y4,z4);

		 transModel.AntiTranslate(x5,y5,z5);
		 transModel.AntiTranslate(x6,y6,z6);
		 transModel.AntiTranslate(x7,y7,z7);
		 transModel.AntiTranslate(x8,y8,z8);

		 xmin = MIN(MIN(MIN(MIN(x1,x2),x3),x4),MIN(MIN(MIN(x5,x6),x7),x8));
		 ymin = MIN(MIN(MIN(MIN(y1,y2),y3),y4),MIN(MIN(MIN(y5,y6),y7),y8));
		 zmin = MIN(MIN(MIN(MIN(z1,z2),z3),z4),MIN(MIN(MIN(z5,z6),z7),z8));

		 xmax = MAX(MAX(MAX(MAX(x1,x2),x3),x4),MAX(MAX(MAX(x5,x6),x7),x8));
		 ymax = MAX(MAX(MAX(MAX(y1,y2),y3),y4),MAX(MAX(MAX(y5,y6),y7),y8));
		 zmax = MAX(MAX(MAX(MAX(z1,z2),z3),z4),MAX(MAX(MAX(z5,z6),z7),z8));

		 return QueryByExtent((f32)xmin,(f32)ymin,(f32)zmin,(f32)xmax,(f32)ymax,(f32)zmax);
	 }

	 BOOL CSeaPointCloud::ReadNext(hdVector<HlzPoint>& pPtBuf,int bufIndex)
	 {
		 if(m_hlzReader == NULL || !m_hlzReader->IsOpen())
		 {
			 return FALSE;
		 }

		 // [1] 得到CHdCoreData
		 CHdCoreData* pCoreData = (CHdCoreData*)(m_hlzReader);
		 if (!pCoreData)
		 {
			 return FALSE;
		 }

		 pPtBuf.clear();

		 //int allocSize = 0;
	  //   // [2] 遍历Arealist加载数据
	  //   for (int i = 0; i < m_pListAreaNoInf.size(); i++)
	  //   {
	  //       CHdParcelBase* noInfo = *(m_pListAreaNoInf._Myfirst + i);
	  //       if (!noInfo)
	  //       {
	  //           continue;
	  //       }
	  //       
	  //       // 获取点数
	  //       int count = noInfo->GetPtCount();
	  //       
			// allocSize += count;
		 //}

		 //pPtBuf.resize(allocSize);

		 //// 该变量记录每个包的偏移
		 //int j = 0;

		 // [2] 遍历Arealist加载数据
		 //for (int i = 0; i < m_pListAreaNoInf.size(); i++)
		 //{
			 CHdParcelBase* noInfo = m_pListAreaNoInf[bufIndex];
			 if (!noInfo)
			 {
				 return FALSE;
			 }         

			 pCoreData->LoadSpecParcel(noInfo);
		 
			 // 获取点数
			 int count = noInfo->GetPtCount();

			 pPtBuf.resize(count);

			 // [3] 获取得到点数据
			 for (int k = 0; k < count; k++)
			 {
				 HlzPoint* hlzPt = (noInfo->m_pHlzPoint + k);

				 if(!hlzPt->isValid())
				 {
					 continue;
				 }
             
				 pPtBuf[k] = (*hlzPt);
			 }

			 //j = count;

			 pCoreData->UnLoadSpecParcel(noInfo);
		 //}

			 return TRUE;
	 }

	 // 卸载包数据
	 BOOL CSeaPointCloud::UnLoadSpecParcel( CHdParcelBase* noInfo )
	 {
		 // 释放内存
		 EnterCriticalSection(&m_cs);
		 if (noInfo)
		 {
			 noInfo->Release();
			 LeaveCriticalSection(&m_cs);
			 return  TRUE;
		 }

		 LeaveCriticalSection(&m_cs);
		 return FALSE;
	 }

	 // 一次load全部数据
	 BOOL CSeaPointCloud::LoadAllListAreaParcels( CHdListAreaNoInf* pHdlstArea )
	 {
		 EnterCriticalSection(&m_cs);

		 // 使用vec记录需要从服务器上获取的所有包
		 CHdListAreaNoInf pVecAreaFrmData;
		 std::vector<string> vecParcelID;

		 // 定义中间变量
		 BOOL bRet = FALSE;
		 CHdCoreData* pCoreData = (CHdCoreData*)(m_hlzReader);

		 // 定义中间变量，初始化
		 string strParcelID = "";
		 int zoneID,levelID,blockSetID,blockID,parcelID;
		 zoneID = m_hlzHeader.iZoneID;
		 double dOffsetX,dOffsetY,dOffsetZ;
		 dOffsetX = m_hlzHeader.offsetX;
		 dOffsetY = m_hlzHeader.offsetY;
		 dOffsetZ = m_hlzHeader.offsetZ;

		 char strTmp[128];

		 // 首先，将不需要从服务器上获取数据的全部加载
		 for (int i = 0; i < pHdlstArea->size(); i++)
		 {
			 CHdParcelBase* noInfo = pHdlstArea->at(i);

			 // 判断是否已经有内存点云数据
			 if (noInfo->m_pHlzPoint)
			 {
				 continue;
			 }

			 // 由数据地址指针进行判断,判断是否无效,若地址无效
			 if (!noInfo->GetCoordAddr().IsInValid())
			 {
				 // 若本地地址有效，则由本地直接读取
				 bRet = pCoreData->LoadSpecParcel(noInfo);
			 }
			 else
			 {
				 // 尝试将基类转子类,再根据子类重组数据库唯一字段
				 CHdBlockset* pBlockSet = dynamic_cast<CHdBlockset*>(noInfo);
				 CHdBlock* pBlock = dynamic_cast<CHdBlock*>(noInfo);
				 CHdParcel* pParcel = dynamic_cast<CHdParcel*>(noInfo);

				 if (pBlockSet) // 若块集存在
				 {
					 levelID = pBlockSet->m_levelNo;
					 blockSetID = pBlockSet->m_nBlockSetNo;
					 sprintf_s(strTmp,"%d-%d-%d",zoneID,levelID,blockSetID);
					 strParcelID = strTmp;
				 }

				 // 若块存在
				 if (pBlock)
				 {
					 // 获得父节点块集
					 pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);
					 levelID = pBlockSet->m_levelNo; // 层级
					 blockSetID = pBlockSet->m_nBlockSetNo; // 块集
					 blockID = pBlock->m_nBlockNo;

					 sprintf_s(strTmp,"%d-%d-%d-%d",zoneID,levelID,blockSetID,blockID);
					 strParcelID = strTmp;
				 }

				 // 若包存在
				 if (pParcel)
				 {
					 // 逐级获得父节点
					 pBlock = dynamic_cast<CHdBlock*>(pParcel->m_pParent);
					 pBlockSet = dynamic_cast<CHdBlockset*>(pBlock->m_pParent);

					 // 包信息赋值
					 levelID = pBlockSet->m_levelNo; // 层级
					 blockSetID = pBlockSet->m_nBlockSetNo; // 块集
					 blockID = pBlock->m_nBlockNo;
					 parcelID = pParcel->m_nParcelNo;

					 sprintf_s(strTmp,"%d-%d-%d-%d-%d",zoneID,levelID,blockSetID,blockID,parcelID);
					 strParcelID = strTmp;
				 }

				 pVecAreaFrmData.push_back(noInfo);
				 vecParcelID.push_back(strParcelID);
			 }
		 }

		 // 没有需要查询服务器项则直接返回
		 if (vecParcelID.size() <= 0)
		 {
			 LeaveCriticalSection(&m_cs);
			 return TRUE;
		 }

		
		 LeaveCriticalSection(&m_cs);
		 return TRUE;
	 }

}
