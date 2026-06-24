#include "StdAfx.h"
#include "HdPtCloud.h"
#include "..\hdCore\hdBox3d.h"

namespace hd
{

CHdPtCloud::CHdPtCloud(void)
	:m_detailSimple(3000000),//m_simpleLevel(1),m_loadSimple(5000000),
	m_count(0),m_curRoop(0),m_pCurLoopBuf(NULL)
{
	m_loadSimple = 1800000;
}

CHdPtCloud::~CHdPtCloud(void)
{
}

BOOL CHdPtCloud::LoadHlsFile(void (*loadCallback)(float,const char*) /*= NULL*/ )
{
	if(m_hlsReader.m_header.number_of_point_records <= 0)
		return FALSE;
	m_count = 0;
	if (m_hlsReader.m_header.number_of_point_records <= m_loadSimple)
	{
		m_simpleLevel = 1;
	}
	else
	{
		m_simpleLevel = m_hlsReader.m_header.number_of_point_records / m_loadSimple + 1;
	}

	// 获取范围内的列号
	m_queryResult.clear();
	m_queryResult.resize(m_hlsReader.m_header.number_of_col);
	m_vecLoopSuf.clear();
	m_vecLoopSuf.resize(m_hlsReader.m_header.number_of_col + 1);
	u32 index = 0;
	u32 colPtCount;
	CLoopIndex* pLoopIdx = m_hlsReader.GetLoopIndex();
	u32 step = m_hlsReader.m_header.is_mesh()?1:m_simpleLevel;

	for (u32 i = 0;i< m_hlsReader.m_header.number_of_col;i+=step)
	{
		CLoopIndex& loopIdx = pLoopIdx[i];
		colPtCount = loopIdx.m_loopIdx.count;// sizeof(PointXYZI);
		*(m_queryResult._Myfirst + index) = i;
		*(m_vecLoopSuf._Myfirst + index) = m_count;
		m_count += colPtCount;
		index++;
	}
	*(m_vecLoopSuf._Myfirst + index) = m_count;
	m_vecLoopSuf.resize(index + 1);
	m_queryResult.resize(index);
		
	return TRUE;
}

BOOL CHdPtCloud::Open( const char* hlsFile )
{
	BOOL bRet = m_hlsReader.Open(hlsFile);
	m_header = m_hlsReader.m_header;
	m_simpleHeader = m_header;
	return bRet;
}

// 判断当前列是否在视窗内
BOOL CHdPtCloud::IsLoopInView(
	PointXYZIPRGBA* ptBuf,								// 点数组
	u32		   ptCount,								// 总点数
	BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
	s32 srcWidth,									// 视口宽度
	s32 srcHeight,									// 视口高度
	u32 uStart,										// 起始点号
	u32 uStep,										// 采样判断步长
	HRGN srcRgn)										
{
	// 如果一直递归循环到每隔ptCount / 20个点进行判断时,则认为当前列不在视窗内
	if (uStep <= ptCount / 20)
	{
		return FALSE;
	}
	BOOL bInView = FALSE;
	
	for (u32 i = uStart;i < ptCount;i+=uStep)
	{
		const PointXYZIPRGBA& pt = ptBuf[i];
		s32 srcX = -1,srcY = -1;
		if (ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
		{
			if (srcRgn)
			{				
				i = 0;
				while(!bInView && i < ptCount)
				{
					PointXYZIPRGBA& pt = ptBuf[i++];
					if(ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
					{
						bInView = ::PtInRegion(srcRgn,srcX,srcY);
					}
				}
			}
			else
				bInView = TRUE;
			break;
		}
	}
	//return bInView;
	if (bInView)
	{
		return TRUE;
	}
	else
		return IsLoopInView(ptBuf,ptCount,ViewTrans,srcWidth,srcHeight,uStep / 2,uStart == 0? uStep:uStep / 2,srcRgn);
}

//! 根据视口查询加载数据
BOOL CHdPtCloud::LoadByViewPort(
	BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
	s32 srcWidth,									// 屏幕宽度
	s32 srcHeight,									// 屏幕高度
	const CHdBox3df& box,							// 视锥体外边框
	const bool isOrthogonal,						// 是否为正射投影
	HRGN srcRgn)									// 屏幕选择范围,用于用户拉框查询	
{
	if (ViewTrans == NULL)
	{
		return FALSE;
	}
#ifdef _DEBUG
		LARGE_INTEGER tFreq;
	QueryPerformanceFrequency(&tFreq);
	LARGE_INTEGER tStartQ;
	QueryPerformanceCounter(&tStartQ);
#endif
	CLoopIndex* pLoopIdx = m_hlsReader.GetLoopIndex();
	// 检查列范围,每隔多少列判断一次,如果是块存储,则不能抽稀判断
	BOOL bIsMesh = m_hlsReader.m_header.is_mesh();
	u32 checkStep = bIsMesh ? 1:20;	
	u32 ptCount;
	u64 inViewCount = 0;
	m_count = 0;
	std::vector<u8> vecInview;
	vecInview.resize(m_hlsReader.m_header.number_of_col);

	s32 srcX = -1,srcY = -1;
	u32 startLoop,endLoop;
	// 每隔checkStep圈进行判断加载
	for (u32 i = checkStep / 2;i < m_hlsReader.m_header.number_of_col;i+=checkStep)
	{
		PointXYZIPRGBA* ptBuf = NULL;
		CLoopIndex& loopIdx = pLoopIdx[i];
		if(!m_hlsReader.ReadLoop(ptBuf,ptCount,i,false))
			continue;
		if(!box.intersectsWithBox(loopIdx.m_loopIdx.xmin,loopIdx.m_loopIdx.ymin,
			loopIdx.m_loopIdx.zmin,loopIdx.m_loopIdx.xmax,
			loopIdx.m_loopIdx.ymax,loopIdx.m_loopIdx.zmax))
			continue;
		BOOL bInView = TRUE;
		if(!bIsMesh && !isOrthogonal)// 如果是按块存储,不需要判断点在视图内
		{
			// 先判断最后一个点是否在范围内
			const PointXYZIPRGBA& pt = ptBuf[ptCount-1];
			if (!ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
			{
				// 判断其他点是否在范围内
				bInView = IsLoopInView(ptBuf,ptCount,ViewTrans,srcWidth,srcHeight,0,ptCount / 2);			
			}
			else
			{
				bInView = TRUE;
			}
		}
		
		// 如果在范围内,则统计点数
		if (bInView)
		{
			*(vecInview._Myfirst + i) = 1;
			if (checkStep == 1)
			{
				loopIdx = pLoopIdx[i];
				inViewCount += loopIdx.m_loopIdx.count;
			}
			else
			{
				// 统计相邻loopStep列的点数
				startLoop = i - checkStep / 2;
				endLoop = MIN((i + checkStep / 2),m_hlsReader.m_header.number_of_col);
				for (u32 k = startLoop;k < endLoop;k++)
				{
					loopIdx = pLoopIdx[k];
					inViewCount += loopIdx.m_loopIdx.count;
				}
			}
		}
	}
	if(inViewCount ==0)
		return FALSE;
#ifdef _DEBUG
	LARGE_INTEGER tStartR;
	QueryPerformanceCounter(&tStartR);
#endif // _DEBUG
	m_simpleLevel = 1;
	if (inViewCount <= m_loadSimple)
	{
		m_simpleLevel = 1;
	}
	else
	{
		m_simpleLevel = inViewCount / m_loadSimple + 1;
	}

	s32 remedy = 0;
	u32 colPtCount = 0;
	BOOL bChange = FALSE;
	m_count = 0;
	// 获取范围内的列号
	m_queryResult.clear();
	m_queryResult.resize(m_hlsReader.m_header.number_of_col);
	m_vecLoopSuf.clear();
	m_vecLoopSuf.resize(m_hlsReader.m_header.number_of_col + 1);
	u32 index = 0;
	for (u32 i = 0;i< vecInview.size();i++)
	{
		u8 uIsInView = *(vecInview._Myfirst + i);
		if (uIsInView != 1)
		{			
			continue;
		}

		// 加载相邻loopStep列的点数
		if (checkStep == 1)
		{
			CLoopIndex& loopIdx = pLoopIdx[i];
			colPtCount = loopIdx.m_loopIdx.count;// loopIdx.size / sizeof(PointXYZI);
			*(m_queryResult._Myfirst + index) = i;
			*(m_vecLoopSuf._Myfirst + index) = m_count;
			index++;
			m_count += colPtCount;
		}
		else
		{
			s32 k = 0;
			s32 startLoop = i - checkStep / 2 + remedy;
			s32 endLoop = MIN((i + checkStep / 2),m_hlsReader.m_header.number_of_col);
			for (k = startLoop;k < endLoop;k++)
			{
				CLoopIndex& loopIdx = pLoopIdx[k];
				colPtCount = loopIdx.m_loopIdx.count;// loopIdx.size / sizeof(PointXYZI);

				if ((k - startLoop) % m_simpleLevel == 0)
				{
					*(m_queryResult._Myfirst + index) = k;
					*(m_vecLoopSuf._Myfirst + index) = m_count;
					index++;
					m_count += colPtCount;
				}
			}
			remedy = i + checkStep / 2 - k;
		}
	}
	*(m_vecLoopSuf._Myfirst + index) = m_count;
	m_vecLoopSuf.resize(index + 1);
	m_queryResult.resize(index);
#ifdef _DEBUG
	LARGE_INTEGER tEndR;
	QueryPerformanceCounter(&tEndR);
	char strDbg[256] = {0};
	sprintf(strDbg,"query:%lf\tload:%lf\n",
		(double)(tStartR.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,
		(double)(tEndR.QuadPart - tStartR.QuadPart)/tFreq.QuadPart);
	OutputDebugString(strDbg);
#endif

	return bChange;
}

//! 查询有多少个扫描列或者块在范围内,返回列数或块数
hd::u32 CHdPtCloud::QueryByExtent( f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax )
{
	CLoopIndex* pLoopIdx = m_hlsReader.GetLoopIndex();
	if (pLoopIdx == NULL || m_hlsReader.m_header.number_of_col <= 0 || m_hlsReader.m_header.number_of_point_records)
	{
		return 0;
	}
	m_count = 0;
	m_queryResult.clear();
	m_queryResult.resize(m_hlsReader.m_header.number_of_col);
	m_vecLoopSuf.clear();
	m_vecLoopSuf.resize(m_hlsReader.m_header.number_of_col + 1);
	u32 index = 0;
	u32 colPtCount = 0;
	for (u32 i = 0;i<m_hlsReader.m_header.number_of_col;i++)
	{
		CLoopIndex& loopIdx = pLoopIdx[i];
		if (loopIdx.m_loopIdx.xmax < xmin || loopIdx.m_loopIdx.xmin > xmax ||
			loopIdx.m_loopIdx.ymax < ymin || loopIdx.m_loopIdx.ymin > ymax ||
			loopIdx.m_loopIdx.zmax < zmin || loopIdx.m_loopIdx.zmin > zmax)
		{
			continue;
		}
		*(m_queryResult._Myfirst + index) = i;
		*(m_vecLoopSuf._Myfirst + index) = m_count;

		colPtCount = loopIdx.m_loopIdx.count;//loopIdx.size / sizeof(PointXYZI);
		m_count += colPtCount;
		index++;
	}
	*(m_vecLoopSuf._Myfirst + index) = m_count;
	m_vecLoopSuf.resize(index + 1);
	m_queryResult.resize(index);
	m_curRoop = 0;
	return index;
}

//! 查询所有列或者块
hd::u32 CHdPtCloud::QueryAll()
{
	CLoopIndex* pLoopIdx = m_hlsReader.GetLoopIndex();
	if (pLoopIdx == NULL || m_hlsReader.m_header.number_of_col <= 0 || m_hlsReader.m_header.number_of_point_records)
	{
		return 0;
	}
	m_count = 0;
	m_queryResult.clear();
	m_queryResult.resize(m_hlsReader.m_header.number_of_col);
	m_vecLoopSuf.clear();
	m_vecLoopSuf.resize(m_hlsReader.m_header.number_of_col + 1);
	u32 colPtCount = 0;
	u32 i = 0;
	for (i = 0;i<m_hlsReader.m_header.number_of_col;i++)
	{		
		*(m_queryResult._Myfirst + i) = i;

		CLoopIndex& loopIdx = pLoopIdx[i];
		colPtCount = loopIdx.m_loopIdx.count;//loopIdx.size / sizeof(PointXYZI);
		*(m_vecLoopSuf._Myfirst + i) = m_count;
		m_count += (colPtCount);
	}
	*(m_vecLoopSuf._Myfirst + i) = m_count;
	
	m_curRoop = 0;
	return m_hlsReader.m_header.number_of_col;
}

void CHdPtCloud::ResetRead()
{
	m_curRoop = 0;
}

//! 读取下一个扫描列,返回是否成功
BOOL CHdPtCloud::ReadNext( 
	PointXYZIPRGBA*& pPtBuf,				// 返回的点数组指针,外部定义空的PointXYZI指针传入即可
	u32& ptCount )					// 返回点个数
{
	pPtBuf = NULL;
	ptCount = 0;
	if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
	{
		return FALSE;
	}
	u32 loopIndex = *(m_queryResult._Myfirst + (m_curRoop++));
	return m_hlsReader.ReadLoop(pPtBuf,ptCount,loopIndex,false);
}

void CHdPtCloud::GetExtent( f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax )
{
	if(m_queryResult.size() == 0 || m_curRoop >= m_queryResult.size())
	{
		return;
	}
	u32 loopIndex = *(m_queryResult._Myfirst + m_curRoop);
	CLoopIndex* pLoopIdx = m_hlsReader.GetLoopIndex();
	const CLoopIndex& loopIdx = pLoopIdx[m_curRoop];
	xmin = loopIdx.m_loopIdx.xmin;
	ymin = loopIdx.m_loopIdx.ymin;
	zmin = loopIdx.m_loopIdx.zmin;
	xmax = loopIdx.m_loopIdx.xmax;
	ymax = loopIdx.m_loopIdx.ymax;
	zmax = loopIdx.m_loopIdx.zmax;
}

void CHdPtCloud::SelectPoint(HRGN hRgn,
	s32 srcWidth,									// 视口宽度
	s32 srcHeight,									// 视口高度
	BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&))		// 三维坐标转屏幕坐标回调函数);
{
	if(m_queryResult.size() == 0 )
	{
		return ;
	}

#ifdef _DEBUG
	LARGE_INTEGER tFreq;
	QueryPerformanceFrequency(&tFreq);
	LARGE_INTEGER tStartQ;
	QueryPerformanceCounter(&tStartQ);
#endif

	ResetRead();

	PointXYZIPRGBA* ptBuf = NULL;
	u32 ptCount = 0;
	u32 inRgn = 0;
	while(ReadNext(ptBuf,ptCount))
	{
		// 先判断最后一个点是否在范围内
		BOOL bInView = FALSE;
		const PointXYZIPRGBA& pt = ptBuf[ptCount-1];
		s32 srcX = -1,srcY = -1;
		if (!ViewTrans(pt.x,pt.y,pt.z,srcX,srcY))
		{
			// 判断其他点是否在范围内
			bInView = IsLoopInView(ptBuf,ptCount,ViewTrans,srcWidth,srcHeight,0,ptCount / 2,hRgn);			
		}
		else
		{
			for (u32 i = 0;i<ptCount;i++)
			{
				PointXYZIPRGBA& pt = ptBuf[i];
				ViewTrans(pt.x,pt.y,pt.z,srcX,srcY);
				if(PtInRegion(hRgn,srcX,srcY))
				{
					pt.setSelected();
					//pt.intensity = (pt.intensity | 0x8000);	
				}
			}
			bInView = TRUE;
		}
		if (bInView)
		{
			inRgn++;
		}
	}
	inRgn += 0;
#ifdef _DEBUG
	LARGE_INTEGER tEnd;
	QueryPerformanceCounter(&tEnd);
	char strDbg[256] = {0};
	sprintf(strDbg,"select:%lf\tinRgn:%d\n",
		(double)(tEnd.QuadPart - tStartQ.QuadPart)/tFreq.QuadPart,inRgn);
	OutputDebugString(strDbg);
#endif

}

void CHdPtCloud::EndRead()
{
	m_hlsReader.CloseFile();
}

}