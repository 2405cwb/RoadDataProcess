/*! HdTINPointCloud.h
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : HdTINPointCloud.h
相关文件     : HdTINPointCloud.cpp
文件实现功能 : 点云模型TIN内存结构
作者         : 研发部 冯晶
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容  
2014/03/05   1.0        冯晶                 新建
</PRE>
*******************************************************************************/
#pragma once
#include "HdQuadSourceIndex.h"

namespace hd
{
	class HDPOINTCLOUD_API CHdTINPointCloud
	{
	public:
		// 构造
		CHdTINPointCloud();

		// 带参数构造
		CHdTINPointCloud(bool isTif2Obj);

		// 析构
		~CHdTINPointCloud();

	private:
		// 禁止拷贝
		CHdTINPointCloud(const CHdTINPointCloud &pcd)
		{
		}

		// 禁止复制
		const CHdTINPointCloud& operator=(const CHdTINPointCloud& other)
		{
			return *this;
		}

	private:
		vector<S3DVertex2TCoords>		m_pts;					// 点数组,点云或DEM
		vector<u32>						m_TrianlgeIndex;		// 顶点索引，存当前需要渲染的三角形索引 其中定义了三角形结构体
		u32								m_nIndicesToRender;		// 渲染的索引数
		HdPtType						m_HdPtType;			// 点云类型	
		CBursaWolfModel					m_transModel;			// 转换绝对坐标对象	
		string							m_filePath;				// 数据文件路径
		core::aabbox3df					m_box;					// 包围盒

		CHdQuadSourceIndex*				m_TriagleQuadIndex;		// 顶点索引

		double							m_dx;					// x方向偏移量
		double							m_dy;					// y方向偏移量
		double							m_dz;					// z方向偏移量

		bool							m_bisTif2Obj;			// 是否为tif生成tin
		float							m_MaxGridSize;			// 最大网格大小：dem生成tin过滤掉三角形包围盒xy方向上大于该值的三角形
		float							m_MaxTriaSIzeLen;		// 最大三角形边长：过滤掉大于该边长的三角形
	public:
		// 获取内存中点个数,如果抽稀加载则小于countInFile()
		inline virtual u64 count() const { return m_pts.size();}

		// 构建三角形 对于在加载文件时，发现文件中未存储三角形索引 可以调用该接口构建三角形 [2014/04/09 危迟]
		bool BuildTriangleMesh();

		// 获取tin数据相对转绝对的偏移
		void GetTransCoord(double& dx, double& dy, double& dz) { dx = m_dx; dy = m_dy; dz = m_dz; }

		// 获取全局坐标, 传进来的参数为相对坐标，传出去的坐标为全局坐标
		void GetGlobalCoord(double& gx, double& gy, double& gz);

		// 获取相对坐标, 传进来的参数为绝对坐标，传出去的坐标为相对坐标
		void GetabsCoord(double& x, double& y, double& z);

		//// 获取点云中的模型
		//const CBursaWolfModel& GetModel()
		//{
		//	m_transModel.Angle2RotateMatrix();
		//	m_transModel.Parameter2matrix();
		//	return m_transModel;
		//}

		//// 设置点云模型
		//void SetModel(CBursaWolfModel model,bool bNeedCal = false)
		//{
		//	m_transModel = model;

		//	if (bNeedCal)
		//	{
		//		m_transModel.matrix2Parameter();
		//	}
		//}

		// 清理
		inline void clear() 
		{
			m_pts.clear();
			m_TrianlgeIndex.clear();
			m_nIndicesToRender = 0;
			m_filePath = "";

			// 清除索引
			if (m_TriagleQuadIndex)
			{
				delete m_TriagleQuadIndex;
				m_TriagleQuadIndex = NULL;
			}
		}

		// 获取四叉树索引
		CHdQuadSourceIndex* getQuadIndex() { return m_TriagleQuadIndex;}
 		// 设置点云
		bool SetModelData(PointCloud* pcd) 
		{
			return true;
		}

		// 获取点云
		inline vector<S3DVertex2TCoords>& getVertexBuffer() 
		{
			return m_pts; 
		}

		// 设置DEM 待实现
		bool SetModelData(CHdModelPointCloud* Dem)
		{
			return true;
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

		// 设置tif生成obj过滤三角形阈值
		void SetMaxGridSize(float maxgrid) { m_MaxGridSize = maxgrid; }


		// 设置hls生成obj过滤三角形阈值
		void SetMaxTriaSIzeLen(float maxTriaSIzeLen) { m_MaxTriaSIzeLen = maxTriaSIzeLen; }

		// 获取当前点云类型
		HdPtType GetPointCloudType() const {return m_HdPtType;}

		// 获取文件路径
		string GetFilePath()  { return m_filePath; }

		// 获取模型中顶点
		inline bool getVertexByIndex(S3DVertex2TCoords& pts, int index)
		{
			if (index >= 0 && index < (int)m_pts.size())
			{
				pts = m_pts[index];
				return true;
			}
			return false;
		}

		// 获取渲染顶点索引
		inline vector<u32>& GetRenderIndex()
		{
			return m_TrianlgeIndex;
		}

		// 获取包围盒
		const core::aabbox3d<f32>& getBoundingBox() const
		{
			return m_box;
		}

		// 设置包围盒
		void SetBoundingBox(core::aabbox3d<f32> box)
		{
			m_box = box;
		}

		// 获取渲染三角形索引
		u32 GetRenderIndiceCount() const { return m_nIndicesToRender;}

		// 创建索引
		void CreateQuadIndex();

		// 使三角形按照邻接位置关系进行排序
		void GetOrderTriaIndex(vector<u32>& items);

		// 删除指定三角形
		bool DeleteQuadTriaIndex(vector<u32>& items);

		// 按范围进行搜索
		void SearchQuadIndex(const core::aabbox3df& SelBox, vector<u32>& items);

		// 按视椎体进行搜索
		void SearchQuadIndex(const irr::scene::SViewFrustum& frustum, vector<u32>& items);

		// 计算法向量
		void CalculateNormal(void (*loadCallback)(float,const char*) = NULL);

		//// 计算垂足
		//void CalcuPoint2Line(irr::core::vector3dd& vec, int id0, int id1, int id2);

		// 对于dem到tin的数据，通过包围盒过滤狭长的三角形
		//bool CalculateTriBox(irr::core::vector3df id0, irr::core::vector3df id1, irr::core::vector3df id2);

		// 加载点云文件，支持格式hls，las，laz，bin
		int loadFile(const char* hlsFile, void (*loadCallback)(float,const char*) = NULL);

		// 抽稀加载点云,loopCount为抽稀的间隔数，add by mzm[2014.5.26]
		int loadHlsFile(const char* hlsFile,int loopCount,void (*loadCallback)(float,const char*) = NULL);

		// 将hls文件一次性加载到内存
		int loadHlsFile(const char* hlsFile, void (*loadCallback)(float,const char*) = NULL);

		// 将内存中的点云加载至模型
		int loadHlsFile(PointCloud* pts, void (*loadCallback)(float,const char*) = NULL);

		// 将las点云文件一次性加载到内存
		int loadLasFile(const char* lasFile, void (*loadCallback)(float,const char*) = NULL);

		// 将tif文件加载进内存
		BOOL loadTifFile(const char* tifFile, void (*loadCallback)(float,const char*) = NULL);

		// 将tif文件加载进内存
		BOOL loadDatFile(const char* datFile, void (*loadCallback)(float,const char*) = NULL);

		// 将asc文件加载到内存
	    BOOL loadAscFile(const char* ascFile, void (*loadCallback)(float,const char*) = NULL);

		// 将obj文件加载进内存
		BOOL loadObjFile(const char* tifFile, void (*loadCallback)(float,const char*) = NULL);

		// 将模型写进obj
		void WriteObjFile(const char* objFile, void (*loadCallback)(float,const char*) = NULL);

		// 加载vtk文件
		BOOL LoadVtkFile(const char* vtkFile, void (*loadCallback)(float,const char*) = NULL);

		// 保存vtk文件
		void WriteVtkFile(const char* vtkFile, void (*loadCallback)(float,const char*) = NULL);

		private:
		/*************************************构TIN算法优化 起************************************/
		// 统计TIN顶点平面范围,平面范围内等分为4*4格网,返回等分间隔
		void StatCoordInLoadFile(float& fMinX,float& fMaxX,float& fMinY,float& fMaxY);
		/*************************************构TIN算法优化 止************************************/
	};
}


