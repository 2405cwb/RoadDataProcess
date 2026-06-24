/*! HdModelPointCloud.h
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdModelPointCloud.h
相关文件     : HdModelPointCloud.cpp
文件实现功能 : 点云模型DEM内存结构
作者         : 研发部 冯晶
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2014/02/11   1.0       冯晶                 新建
</PRE>
*******************************************************************************/
#pragma once
#include "hdPointCloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCommon\point_types.h"
#include "..\hdCore\hdBox3d.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "..\hd3DEngine\include\aabbox3d.h"
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <string>
#include <vector>
#include "..\hdCommon\point_types.h"
#include "..\hd3DEngine\include\S3DVertex.h"
#include "point_cloud.h"

using namespace std;
using namespace irr;
using namespace irr::video;

namespace hd
{
	// 三角形
	typedef struct _Trianlge32
	{
		s32 a;
		s32 b;
		s32 c;
		_Trianlge32()
			: a(0), b(1), c(2) {}
		_Trianlge32(s32 aa, s32 bb, s32 cc)
			: a(aa), b(bb), c(cc) {}
	}Trianlge32;

	struct hdTrianglePtIndex
	{
		// 记录该点在DEM文件中所在列
		int col;

		// 记录所在行
		int row;

		// 注：通过该行列可定位至m_vecDemData数据，
		// 同时也可由此行列获得其在DEM中的X、Y坐标
	};

	// 点云类型
	typedef enum HdPtType
	{
		NORMAL_POINTCLOUD = 0,	// 常规点云
		DEM_POINTCLOUD,			// DEM点云
	};

	// 投影三种方式
	typedef enum DEM_PRO_STYLE
	{
		DEM_UNDER= 0,		// 最低点
		DEM_MID,			// 平均值
		DEM_ABOVE,			// 最高点
	};

	class HDPOINTCLOUD_API CHdModelPointCloud
	{
	public:
		// 构造
		CHdModelPointCloud();

		// 析构
		virtual ~CHdModelPointCloud();

	private:
		// 禁止拷贝
		CHdModelPointCloud(const CHdModelPointCloud &pcd)
		{
		}

		// 禁止复制
		const CHdModelPointCloud& operator=(const CHdModelPointCloud& other)
		{
			return *this;
		}

	private:
		vector<S3DVertex2TCoords>		m_pts;				// 点数组,从物理文件读取到内存中的点,可能抽稀读取,用于显示
		HdPtType						m_HdPtType;			// 点云类型
		CBursaWolfModel					m_transModel;		// 转换绝对坐标对象
        vector<u32>						m_TrianlgeIndex;	// 顶点索引，存当前需要渲染的三角形索引 其中定义了三角形结构体
		
		string							m_filePath;			// 数据文件路径
		double							m_ModelX;			// X偏移量
		double							m_ModelY;			// Y偏移量

		bool                            m_bCutFill;         // 标记是否记录了挖填量高度差
	
		int								m_ProjectStyle;		// 投影方式
		u32 m_npoints;		                 // 记录原始文件总点数
		u32 m_nrows;		                 // 记录DEM文件行数
		u32 m_ncols;		                 // 记录DEM文件列数
		float m_noDataValue;                 // 记录DEM文件无效值
		float m_stepx;                       // 记录DEM文件中格网步长
		float m_stepy;                       // 同上
		double m_minx;                       // X最小值
		double m_miny;                       // y最小值
		double m_maxx;                       // x最大值
		double m_maxy;                       // y最大值
		double m_minz;                       // z最小值
		double m_maxz;                       // z最大值
		bool m_bBufferInMemory;				 // 记录缓存方式是内存缓存还是文件缓存，true为内存缓存

public:
		vector<vector<float>> m_vecDemData;  // 读取DEM文件后存入该容器中，存储的为减去偏移量后浮点
		float* m_pPtBuffer;					 // 构TIN时数据缓存指针---三角网

/************************************堆体相关***********************************************/
		// 记录tif堆体分析时格网高程变化量
		vector<vector<float>> m_vecdHInTif;

		// 记录cutfill部分存在的box
		vector<core::aabbox3df> m_vecCutFillBox;

/************************************堆体相关***********************************************/

		// 转换至栅格坐标
		void world_to_raster(const double* world, float* raster);

		// 将三角网转换写至二进制临时文件
		void raster_triangle_to_file(FILE* pTempFile,float kill_threshold_squared,const float* a, const float* b, const float* c);

		// 将三角网转换至对应内存格网
		void raster_triangle_to_memory(vector<vector<float>>& vecPcd,float kill_threshold_squared,const float* a, const float* b, const float* c);

		// 内存流数据写入TIF格式文件
		void BufferMemory2Tif(const char* savePath,void (*processCallback)(float,const char*) = NULL);

		// 临时文件写入TIF格式文件
		void BufferFile2Tif(FILE* pTempFile,const char* savePath,void (*processCallback)(float,const char*) = NULL);

		// 内存流数据写入asc格式文件
		void BufferMemory2Asc(const char* savePath,void (*processCallback)(float,const char*) = NULL);

		// 临时文件写入asc格式文件
		void BufferFile2Asc(FILE* pTempFile,const char* savePath,void (*processCallback)(float,const char*) = NULL);

	private:
		// 判断四点中存在几个无效点
		int getValidCount(float* fIndexZ,int& index);
	public:
		// 构建

		// 获取内存中点个数,如果抽稀加载则小于countInFile()
		inline virtual u64 count() const { return m_pts.size();}

		// 获取当前点云类型
		HdPtType GetPointCloudType() const {return m_HdPtType;}

		// 当前点云类型
		void SetPointCloudType(HdPtType PtType)   { m_HdPtType = PtType;}

		// 获取点云文件路径用于修改点云文件导出时的文件名获取方法
		inline string GetDataPath()
		{
			return m_filePath;
		} 

		// 获取dem数据相对转绝对的偏移
		void GetTransCoord(double& dx, double& dy) { dx = m_ModelX; dy = m_ModelY; }

		// 获取全局坐标, 传进来的参数为相对坐标，传出去的坐标为全局坐标, 对于dem，只对x与y存在偏移
		void GetGlobalCoord(double& gx, double& gy);

		// 获取相对坐标, 传进来的参数为绝对坐标，传出去的坐标为相对坐标,对于dem，只对x与y存在偏移
		void GetabsCoord(double& x, double& y);

		// 判断dem是否加载
		bool IsDemDataLoaded(){ return (m_vecDemData.size() > 0 ? true:false); }

        // 获取包围盒
		void GetBoudingBox(core::aabbox3df&	box);

		// 设置包围盒
		void SetBoudingBox(core::aabbox3df	box);

		// 设置点云模型
		void SetModel(CBursaWolfModel model,bool bNeedCal = false)
		{
			m_transModel = model;

			if (bNeedCal)
			{
				m_transModel.matrix2Parameter();
			}
		}

		// 获取点云中的模型
		const CBursaWolfModel& GetModel()
		{
			m_transModel.Angle2RotateMatrix();
			m_transModel.Parameter2matrix();
			return m_transModel;
		}

		// 设置点云到模型
		bool SetDemData(PointCloud* pcd)
		{
			return true;
		}

		// 获取模型中顶点数组
		inline vector<S3DVertex2TCoords>& getVertexBuffer()
		{
			return m_pts;
		}

		// 获取模型中顶点
		inline bool getVertexByIndex(S3DVertex2TCoords& pts, int index)
		{
			// 判断合法性
			if (index >= 0 && index < (int)m_pts.size())
			{
				pts = m_pts[index];
				return true;
			}
			return false;
		}
/************************** 以文件中相邻格网构建三角形*********************************/
		// 构建详细的三角形(针对DEM文件所有数据)（与其SN一样，采用相邻格网构成三角形（区别于TIN））
		bool BuildTriangleInDemFile();

		float GetZ(int col,int row);

		// 通过行列索引获取该点顶点的坐标值，该坐标值为绝对坐标
		S3DVertex2TCoords GetValue(u32 index);

/************************** 以文件中相邻格网构建三角形*********************************/

/************************************堆体相关***********************************************/
		// 获得记录高程差异值的vec
		vector<vector<float>>& GetCutFillHeighth()
		{
			return m_vecdHInTif;
		}

		// 计算每个格网的显示坐标的box,存储下来
		void CalculateCutfillBoxes();

		// 设置是否记录挖填高度差值
		void SetCutFill(bool bCut)
		{
			m_bCutFill = bCut;
		}

		// 获得是否设置记录了挖填高度差值
		bool GetCutFill()
		{
			return m_bCutFill;
		}

		// 获得计算的cutfill的box
		vector<core::aabbox3df>& GetCutFillBoxes()
		{
			return m_vecCutFillBox;
		}

/************************************堆体相关***********************************************/

		// 获取渲染顶点索引
		inline vector<u32>& GetRenderIndex()
		{
			return m_TrianlgeIndex;
		}

		// 获取某个定点的三角形渲染顶点索引 预留接口
	    vector<u32> GetTrianlgesByIndex(int index)
		{
			return m_TrianlgeIndex;
		}

		// 根据范围获取三角形 预留接口
		vector<u32> GetTrianlgesByIndex(double xmin, double ymin, double zmin, double xmax, double ymax, double zmax)
		{
			return m_TrianlgeIndex;
		}

		// 获取行号
		inline u32 GetRowNum() { return m_nrows;}

		// 获取列号
		inline u32 GetColNum() { return m_ncols;}

		// 清理
		inline void clear() 
		{
			// 释放够tin的缓存指针
			if (m_pPtBuffer)
			{
				delete m_pPtBuffer;
				m_pPtBuffer = NULL;
			}

			// 将参数置为初始值
			m_npoints = 0;
			m_nrows = 0;
			m_ncols = 0;
			m_noDataValue = -9999.0f;
			m_stepx = 1.0f;			
			m_stepy = 1.0f;		
			m_minx = 1e30f;			
			m_miny = 1e30f;			
			m_minz = 1e30f;			
			m_maxx = 1e30f;		
			m_maxy = 1e30f;			
			m_maxz = 1e30f;			

			// 清除顶点与索引
			m_pts.clear();
			m_TrianlgeIndex.clear();
			m_vecDemData.clear();
		}

		// 投影点云
		int HLSProGrid(const char* hlsfile, void (*loadCallback)(float, const char*) = NULL);

		// 插值
		int Interpolation(DEM_PRO_STYLE proStyle, void (*loadCallback)(float, const char*) = NULL);

		// 加载点云文件，支持格式hls，las，laz，bin
		int loadFile(const char* hlsFile, void (*loadCallback)(float,const char*) = NULL);
		
		// 加载投影点云文件
		int loadProHlsFile(const char* hlsFile, void (*loadCallback)(float,const char*) = NULL);

		// 将hls文件一次性加载到内存
		int loadHlsFile(const char* hlsFile, void (*loadCallback)(float,const char*) = NULL);

		// 将las点云文件一次性加载到内存
		int loadLasFile(const char* lasFile, void (*loadCallback)(float,const char*) = NULL);

		// 将tif文件加载进内存
		BOOL loadTifFile(const char* tifFile, void (*loadCallback)(float,const char*) = NULL);

		// 将asc文件加载到内存
		int loadAscFile(const char* ascFile, void (*loadCallback)(float,const char*) = NULL);

		// 设置格网大小
		void SetStepSize(float xStep);

		// 设置投影方式
		void SetProStyle(int prostyle) { m_ProjectStyle = prostyle; }

		// 获取x步长
		inline float GetXStepSize() { return m_stepx; }

		// 获取y步长
		inline float GetYStepSize() { return m_stepy; }

		// 获取x最小值
		inline double GetXmin() { return m_minx; }

		// 获取y最小值
		inline double GetYmin() { return m_miny; }

		// 导出tif or asc
		void Write2Dem(const char* savePath,void (*processCallback)(float,const char*) = NULL);
	};
}
