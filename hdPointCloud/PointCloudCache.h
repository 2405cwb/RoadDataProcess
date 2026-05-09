#pragma once
#include "point_cloud.h"
#include "HdModelPointCloud.h"
#include "HdTINPointCloud.h"
#include "SeaPointCloud.h"
#include <string>
using namespace hd;

typedef struct _PCD 
{
	_PCD()
	{
		strPcdFile = "";
		pPointCloud = NULL;
	}

	~_PCD()
	{
		if (pPointCloud)
		{
			delete pPointCloud;
			pPointCloud = NULL;
		}
	}

	std::string		strPcdFile;
	PointCloud*		pPointCloud;
}PCD;

typedef struct _DEM 
{
	_DEM()
	{
		strPcdFile = "";
		pDem = NULL;
	}

	~_DEM()
	{
		if (pDem)
		{
			delete pDem;
			pDem = NULL;
		}
	}

	std::string				strPcdFile;		// 点云文件路径或者DEM文件路径
	CHdModelPointCloud*		pDem;
}DEM;

typedef struct _TIN 
{
	_TIN()
	{
		strPcdFile = "";
		pTIN = NULL;
	}

	~_TIN()
	{
		if (pTIN)
		{
			delete pTIN;
			pTIN = NULL;
		}
	}

	std::string				strPcdFile;		// 点云文件路径或者TIN文件路径
	CHdTINPointCloud*		pTIN;
}TIN;

typedef struct _SEAPCD 
{
	_SEAPCD()
	{
		strPcdFile = "";
		pSEAPCD = NULL;
	}

	~_SEAPCD()
	{
		if (pSEAPCD)
		{
			delete pSEAPCD;
			pSEAPCD = NULL;
		}
	}

	std::string				strPcdFile;		// 海量点云文件路径
	CSeaPointCloud*		    pSEAPCD;
}SEAPCD;

class HDPOINTCLOUD_API CPointCloudCache
{
private:
	// 定义一个单实例销毁辅助类,gsl
	class CHdCacheCleaner
	{
	public:
		CHdCacheCleaner(){}
		virtual ~CHdCacheCleaner()
		{
			if (m_pCache != NULL)
			{
				delete m_pCache;
				m_pCache = NULL;
			}
		}
	};

	CPointCloudCache(void);
	~CPointCloudCache(void);

private:
	static CPointCloudCache* m_pCache;		//单实例对象

	vector<PCD*>		m_pPcds;			//点云队列
	vector<DEM*>		m_pDems;			//DEM队列
    vector<TIN*>		m_pTins;			//TIN队列
	vector<SEAPCD*>     m_pSeaPcds;       //海量点云对象
			
	unsigned int		m_nMaxPtSize;		//最大点数
	unsigned int		m_nMaxPtSizePerFile;	//单个点云文件最大加载点云个数

	HWND				m_hMsgReceiver;		//消息接收窗体句柄
	long				m_nMsgID;			//消息ID
	long				m_nDemMsgID;		//Dem消息ID
	long				m_nTinMsgID;		//Dem消息ID
	long                m_nSeaPcdMsgID;     // SeaPcd 消息ID
	int					m_editMode;			//编辑模式
	bool                m_bAverLoad;        //标记是否平均加载，为否表明需要以最大内存加载单个点云
	hdLoadSimpleMode    m_load_simple_mode; // 点云抽稀加载方式
	float               m_sapce_filter_dist;// 空间过滤距离

public:
	//! 获取单实例对象
	static CPointCloudCache* GetCacheInstance()
	{
		if (m_pCache == NULL)
		{
			m_pCache = new CPointCloudCache;
		}
		return m_pCache;
	}

	//! 应用程序退出,手动释放单实例对象
	static void destroyCacheInstance()
	{
		if (m_pCache != NULL)
		{
			delete m_pCache;
			m_pCache = NULL;
		}
	}

	//! 获取点云队列
	vector<PCD*> GetPcdDeque(){ return m_pPcds;};

	//! 外部设置是否根据系统设置平均加载点云
	void SetbAverLoad(bool bAverage);

	//! 外部获得是否是平均加载模式
	bool isAverLoad();

	//! 根据当前可用内存更新加载阈值,传入参数为需要加载的点云总点数
	void UpdateMaxPtSizePerFile(U64 curLoadCount);

	//! 设置点云编辑模式
	void setEditMode(int mode);
	//!  获取点云编辑模式
	int getEditMode() { return m_editMode; }
	//! 设置加载点个数
	void SetLoadSimple(unsigned int loadSimple){m_nMaxPtSizePerFile = loadSimple;}
	//! 获取加载点个数
	unsigned int GetLoadSimple(){return m_nMaxPtSizePerFile;}

	//! 设置抽稀加载方式
	void setLoadSimpleMode(int mode);
	//! 获取抽稀加载方式
	hdLoadSimpleMode getLoadSimpleMode() { return m_load_simple_mode; }
	//! 设置空间抽稀加载距离
	void setSpaceLoadSimpleDist(float dist) { m_sapce_filter_dist = dist; }
	//! 获取空间抽稀加载距离
	float getSpaceLoadSimpleDist() { return m_sapce_filter_dist;}
	
	// 缓存一个点云文件
	PointCloud* Cache(const char* strPcdFile,bool bOnlyHeader = false,void (*loadCallback)(float,const char*) = NULL);
	// 缓存一个点云文件
	PointCloud* Cache(const char* strPcdFile,hd::f64 nStartCol,hd::f64 nEndCol,bool bOnlyHeader = false,void (*loadCallback)(float,const char*) = NULL);
	// 草图视图下缓存文件
	PointCloud* DraftCache(const char* strPcdFile,bool bOnlyHeader = false,void (*loadCallback)(float,const char*) = NULL);
	// 草图视图下缓存文件
	PointCloud* DraftCache(const char* strPcdFile,hd::f64 nStartCol,hd::f64 nEndCol,bool bOnlyHeader = false,void (*loadCallback)(float,const char*) = NULL);

	// 移除点云
	void Remove(PointCloud* pcd);
	// 移除点云
	void Remove(const char* strPcd);
	// 移除所有点云	
	void RemoveAllPointCloud();
	// 点云是否加载 及是否只加载文件头
	bool IsPointCloudLoaded(const char* strPcdFile,bool& bOnlyHeader);
	// 获取存在的点云
	PointCloud* GetLoadedPointCloud(const char* strPcdFile);
	// 设置最大点数
	void SetMaxPtSize(unsigned int nMaxPtSize) { m_nMaxPtSize = nMaxPtSize; }
	// 得到最大点数
	unsigned int GetMaxPtSize() { return m_nMaxPtSize; }
	// 设置单个文件最大加载点数
	void SetMaxPtSizePerFile(unsigned int nMaxPtSizePerFile) { m_nMaxPtSizePerFile = nMaxPtSizePerFile; }
	// 得到单个文件最大加载点数
	unsigned int GetMaxPtSizePerFile() { return m_nMaxPtSizePerFile; }
	// 设置消息接收者
	void SetMsgReceiver(HWND hWnd) { m_hMsgReceiver = hWnd; }
	// 获取消息接受窗口句柄
	HWND GetMsgReceiver() { return m_hMsgReceiver; }
	// 设置消息ID
	void SetPCDDestoyMsgID(long nMsgID) { m_nMsgID = nMsgID; }

	// 设置点云模型消息ID fengjing
	void SetDEMDestoyMsgID(long nMsgID) { m_nDemMsgID = nMsgID; }

	// 设置TIN消息ID fengjing
	void SetTINDestoyMsgID(long nMsgID) { m_nTinMsgID = nMsgID; }

	// 设置海量点云消息ID
	void SetSEAPCDDestoyMsgID(long nMsgID) { m_nSeaPcdMsgID = nMsgID;}

	/************************************************************************/
	/*                            DEM内存管理                               */
	/************************************************************************/
	// 缓存一个DEM文件 fengjing
	CHdModelPointCloud* CacheDem(const char* strPcdFile, void (*loadCallback)(float,const char*) = NULL);

	// 移除DEM fengjing
	void Remove(CHdModelPointCloud* dem);

	// 移除所有点云	fengjing
	void RemoveAllDEMPcd();

	// 点云是否加载 fengjing
	bool IsDEMPcdLoaded(const char* strPcdFile,bool& bOnlyHeader);

	// 获取存在的点云 fengjing
	CHdModelPointCloud* GetLoadedDemPcd(const char* strPcdFile);

	/************************************************************************/
	/*                            TIN内存管理                               */
	/************************************************************************/
	// 缓存一个DEM文件 fengjing
	CHdTINPointCloud* CacheTin(const char* strPcdFile, void (*loadCallback)(float,const char*) = NULL);

	// 移除TIN fengjing
	void Remove(CHdTINPointCloud* tin);

	// 移除所有点云	fengjing
	void RemoveAllTinPcd();

	// 点云是否加载 fengjing
	bool IsTinPcdLoaded(const char* strPcdFile,bool& bOnlyHeader);

	// 获取存在的点云 fengjing
	CHdTINPointCloud* GetLoadedTinPcd(const char* strPcdFile);

	/************************************************************************/
	/*                            CSeaPcd内存管理                               */
	/************************************************************************/
	
	// 缓存一个SeaPcd文件
	CSeaPointCloud* CacheSeaPcd(const char* strPcdFile, void (*loadCallback)(float,const char*) = NULL);

	// 移除海量点云
	void Remove(CSeaPointCloud* pSeaPcd);

	// 移除所有海量点云
	void RemoveAllSeaPcd();

	// 海量点云是否加载
	bool IsSeaPcdLoaded(const char* strPcdFile,bool& bOnlyHeader);

	// 获取存在的点云
	CSeaPointCloud* GetLoadedSeaPcd(const char* strPcdFile);

	// 此处判断所打开的点云文件是否为索引索引文件
	bool IsPcdIndexFile(const char* filePath);

	// 获取可用内存
	U64 GetAvailPhy();

private:
	
	// 得到当前缓存池中的点个数
	U64 GetSize();

	// 检查缓存池中所有点所占内存是否走出最大值，如果超出，则依次将最后的点云移除，直到小于最大值
	BOOL CheckBuffer(U64 toLoadCount);

	// 检查缓冲池
	BOOL CheckBufferForDem(U64 toLoadCount);

	// 检查缓冲池
	BOOL CheckBufferForTIN(U64 toLoadCount);

	// 判读当前操作系统是32还是64位
	typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS)(HANDLE,PBOOL);

	LPFN_ISWOW64PROCESS fnIsWow64Process;

	BOOL IsWow64();
	

};

