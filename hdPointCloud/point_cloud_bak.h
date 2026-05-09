/*! PointCloud.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : PointCloud.h
相关文件     : 
文件实现功能 : 点云内存结构
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/31   1.0      龚书林    
</PRE>
*******************************************************************************/

#ifndef __HD_POINT_CLOUD_H_INCLUDED__
#define __HD_POINT_CLOUD_H_INCLUDED__
#pragma  warning(disable:4251)
#include "hdPointCloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCommon\point_types.h"
#include "..\hdHLSlib\HLSReader.h"
#include "..\hdCore\hdArray.h"
#include "..\hdCommon\BursaWolfModel.h"
#include <vector>
#include "Eigen/Core"
#include "Eigen/Geometry"

using namespace std;

namespace hd
{
	namespace fm
	{
		class CHdPointSelect;
		class CHdPickCentriod;
	}
	namespace scene
	{
		class CHdPlanarView;
	}
	
	//class hd_kd_tree_t;
//template <typename PointT = PointXYZI,typename T = float>
class HDPOINTCLOUD_API PointCloud //class HDCOMMON_API
{
	friend class hd::fm::CHdPointSelect;
	friend class hd::fm::CHdPickCentriod;
	friend class hd::scene::CHdPlanarView;

private:
	// 点数组,从物理文件读取到内存中的点,可能抽稀读取,用于显示
	hdArray<PointXYZIPRGBA>  m_pts;
protected:

	// 级别数量
	u32 m_simpleLevel;
	// 点云索引
	void*   m_pIndex;
	// 点云是否改变,用于构建索引判断
	bool	m_bChange;
	//! 选择集数组
	std::vector<u32>  m_selectionIDs;
	// 灰度最大值
	u32		m_maxIntensity;
	// 灰度最小值
	u32		m_minIntensity;
	// 点云中的靶球,用灰度最大值所在点搜索0.0725m范围的点
	//std::vector<PointXYZI> m_smrPts;
	//		最大距离
	f32		m_maxDistance;
	//		最小距离
	f32		m_minDistance;
	// 点云文件头信息
	HLSreader m_hlsReader;
	// 内存中有效点数
	u32		m_validCount;
	// 选择集中点个数
	u32		m_selectCount;
	//! 建立索引标记
	bool	m_fullIndex;
	//! 加载起始圈比例范围
	f64     m_startScale;
	f64		m_endScale;
	//! 加载点数控制
	u32		m_loadSimple;
	//! 转换绝对坐标对象
	CBursaWolfModel m_transModel;
private:
	//! 禁止拷贝
	PointCloud(const PointCloud &pcd)
	{
	}
	//! 禁止复制
	const PointCloud& operator=(const PointCloud& other)
	{
		return *this;
	}
	//! 获取当前读取的点,坐标系是本地坐标
	void get_point(PointXYZIPRGBA& pt);
	//! 获取当前读取的点,坐标系是全局坐标
	void get_point(PointXYZI_D& pt);
protected:
	void setSelectCount(u32 count);

public:
	//外部设置点云是否改变(靶球查找部分临时pcd变量调用需要)
	void SetPointCloudChanged(bool bChanged){m_bChange = bChanged;}
	BOOL GetPointCloudChanged(){return m_bChange;}

	// 点云文件头
	HLSheader m_header;
	// 内存中抽稀后的文件头,点数和行列数≤文件中的相应值
	HLSheader m_simpleHeader;

	PointCloud():m_pIndex(0),m_bChange(false),m_maxIntensity(0),m_minIntensity(65536),
		m_minDistance(F32_MAX),m_maxDistance(F32_MIN),m_selectCount(0),m_simpleLevel(1),
		m_covariance_matrix(),m_xyz_centroid(),m_validCount(0),m_fullIndex(true),
		m_startScale(0.0),m_endScale(1.0),m_loadSimple(5000000){}
	
	~PointCloud()
	{
		clear();
	}

	inline PointXYZIPRGBA& operator[](u32 index)
	{ 
		return m_pts[index];
	}

	inline const PointXYZIPRGBA& operator[](u32 index) const
	{ 
		return m_pts[index];
	}
	inline const Normal& getNormal(u32 index) const
	{
		return m_normal[index];
	}
	//! 从文件中读取点,并非在内存m_pts中,坐标系是本地坐标
	inline BOOL getPoint(
		int index,				// 点序号
		PointXYZIPRGBA& pt)		// 返回的坐标
	{
		if(m_hlsReader.read_fast(index))
		{
			get_point(pt);
			return TRUE;
		}
		return FALSE;
	}
	//! 从文件中读取点,并非在内存m_pts中,坐标系是全局坐标
	inline BOOL getPointGlobal(
		int index,				// 点序号
		PointXYZI_D& pt)		// 返回的坐标
	{
		if(m_hlsReader.read_fast(index))
		{
			get_point(pt);
			return TRUE;
		}
		return FALSE;
	}
	//! 从文件中读取点,并非在内存m_pts中,坐标系是全局坐标
	inline BOOL getPointGlobal(
		float scaleX,			// x方向比例
		float scaleY,			// y方向比例
		PointXYZI_D& pt)		// 返回的坐标
	{
		if(isNormalPointCloud())
		{
			int imageX = (int)(hd_round(scaleX * m_header.number_of_col));
			int imageY = (int)(hd_round(scaleY * m_header.number_of_row));
			int index = imageX*m_header.number_of_row + imageY;		//数据是按列存储
			if(m_hlsReader.read_fast(index))
			{
				get_point(pt);
				return TRUE;
			}
		}
		return FALSE;
	}
	//! 根据序号得到内存中的点绝对坐标,
	inline BOOL getGlobal(
		int index,				// 点序号,内存中点序号,0-count()
		PointXYZI_D& ptGlobal)	// 返回点绝对坐标
	{
		int nCount = m_pts.count();
		if(index < 0 || index >= nCount)
			return FALSE;
		const PointXYZIPRGBA& pt = m_pts[index];
		if(!pt.isValid())
			return FALSE;
		ptGlobal.x = pt.x;
		ptGlobal.y = pt.y;
		ptGlobal.z = pt.z;
		ptGlobal.intensity = pt.intensity;
		m_transModel.Translate(ptGlobal.x,ptGlobal.y,ptGlobal.z);
		return TRUE;
	}
	// 获取绝对坐标范围
	const void GetGlobalExtent(double& xmin,double& ymin,double& zmin,double& xmax,double& ymax,double& zmax);
	//! 从选择集获取点
	inline PointXYZIPRGBA& getSelectionPoint(int index)
	{
		return m_pts[(*(m_selectionIDs._Myfirst + index))];
	}
	// 获取点云中的模型
	const CBursaWolfModel& GetModel()
	{
		//CBursaWolfModel model;
		m_transModel.m_fOffset[0] = m_header.offsetX;
		m_transModel.m_fOffset[1] = m_header.offsetY;
		m_transModel.m_fOffset[2] = m_header.offsetZ;
		m_transModel.m_fAngle[0] = m_header.rotateX;
		m_transModel.m_fAngle[1] = m_header.rotateY;
		m_transModel.m_fAngle[2] = m_header.rotateZ;
		m_transModel.m_fScale = m_header.scale;
		m_transModel.Angle2RotateMatrix();
		m_transModel.Parameter2matrix();
		return m_transModel;
	}
	//! 获取内存中点个数,如果抽稀加载则小于countInFile()
	inline virtual u64 count() const {return m_pts.count();}
	//! 获取有效点个数
	inline u32 getValidCount() const{return m_validCount;}
	//! 获取有校点个数
	inline u32 getSelectCount() const{return m_selectCount;}
	//! 获取文件中点个数
	inline u32 countInFile() const{return (u32)(m_header.number_of_point_records);}
	//! 获取是否为规则点云,即按行列排序的点云
	inline bool isNormalPointCloud() const;
	//! 清理
	inline void clear() 
	{
		if(m_pIndex != 0)
			delete m_pIndex;
		m_pIndex = 0;
		m_maxIntensity = 0;
		m_minIntensity = 0;
		m_pts.clear();
		m_header.clean();
	}
	//! 删除选中的点云
	void deleteSelectPoints();
	//! 删除选中的点云,index选择集序号
	void deleteSelectPoint(int index)
	{
		PointXYZIPRGBA& pt = m_pts[(*(m_selectionIDs._Myfirst + index))];
		//pt.select = (pt.select | 0x4);		// 标记删除 二进制 00000100
		pt.setDeleted();
	}
	//! 删除指定点,index点云内存序号
	void deletePoint(int index)
	{
		PointXYZIPRGBA& pt = m_pts[index];
		//pt.select = (pt.select | 0x4);		// 标记删除 二进制 00000100
		pt.setDeleted();
	}

	// 返回点数
	inline size_t kdtree_get_point_count() const 
	{
		return m_fullIndex ? m_pts.count():m_selectCount;
	}
	// 返回距离
	inline float kdtree_distance(const float *p1, const size_t idx_p2,size_t size) const
	{
		const PointXYZIPRGBA& pt = m_fullIndex ? m_pts[idx_p2] ://*(m_pts._Myfirst + idx_p2) :
									m_pts[*(m_selectionIDs._Myfirst + idx_p2)];//*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + idx_p2)));

		const float d0 = p1[0] - pt.x;
		const float d1 = p1[1] - pt.y;
		const float d2 = p1[2] - pt.z;

		return d0*d0+d1*d1+d2*d2;
	}
	// 返回点
	inline float kdtree_get_pt(const size_t idx, int dim) const
	{
		const PointXYZIPRGBA& pt = m_fullIndex ? m_pts[idx] :
									m_pts[*(m_selectionIDs._Myfirst + idx)];

		if (dim==0) return pt.x;
		else if (dim==1) return pt.y;
		else return pt.z;
	}
	// 返回范围
	template <class BBOX>
	bool kdtree_get_bbox(BBOX &bb) const { return false; }

	// 构建索引
	//template <typename PointT,typename T>
	void buildIndex(bool bFullIndex = true);
	//! 获得树索引
	void* getKdIndex(){return m_pIndex;}
	//! 加载索引
	void loadIndex(const char* path);
	//! 保存索引
	void saveIndex(const char* path);
	//! 距离搜索,query_point搜索的点,IndicesDists搜索返回值,searchParams搜索参数
	size_t radiusSearch(const float *query_point,const float radius,std::vector<std::pair<size_t,float>>& IndicesDists,bool sort);
	//! 搜索最近的n个点
	void knnSearch(const float *query_point,int neighbour,size_t* ref_index,float* ref_dist_sqr);
	//! 将las点云文件一次性加载到内存
	BOOL loadLasFile(const char* lasFile,void (*loadCallback)(float,const char*) = NULL);
	//! 将hls点云文件一次性加载到内存
	BOOL loadHlsFile(const char* hlsFile,void (*loadCallback)(float,const char*) = NULL);
	
	//! 获取最大灰度值
	inline u32 getMaxIntensity() const {return m_maxIntensity;}
	//! 获取最小灰度值
	inline u32 getMinIntensity() const { return m_minIntensity; }
	//! 获取最大距离
	inline f32 getMaxDistance() const{return m_maxDistance;}
	//! 获取最小距离
	inline f32 getMinDistance() const{return m_minDistance;}
	//! 获取hls点云文件的头文件信息 add by yf 2012/6/29
	BOOL loadHlsFileHeader(const char* hlsFile);
	//! 加载点云数据,用于调用loadHlsFileHeader的后续加载点云
	BOOL loadHlsData(void (*loadCallback)(float,const char*) = NULL);
	//! 根据比例范围加载点云数据,用于调用loadHlsFileHeader的后续加载点云
	BOOL loadHlsByScale(
		f64 startScale,		// 起始比例 0.0-1.0
		f64 endScale,		// 终止比例 0.0-1.0	endScale >= startScale
		void (*loadCallback)(float,const char*) = NULL);
	//! 根据范围加载

	//! 将点云设为彩色的类型 add by wkl 2012/8/11
	inline void setAsColorPointCloud() 
	{
		this->m_header.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
		this->m_simpleHeader.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
	};

	//! 关闭,重新打开,用于写文件时调用
	void close(){m_hlsReader.close();}
	BOOL open()
	{
		BOOL bRet = m_hlsReader.open(m_hlsReader.m_filepath);
		m_header = m_hlsReader.m_header;
		return bRet;
	}
	//! 设置加载点个数
	void setLoadSimple(u32 loadSimple){m_loadSimple = loadSimple;}
	//! 获取加载点个数
	u32 getLoadSimple(){return m_loadSimple;}
/******************以下接口用于点云动态内存处理***************/
	//! 添加点,用于内存动态处理,不和点云文件关联
	inline void addPoint(const PointXYZIPRGBA& pt)
	{
		m_pts.push_back(pt);
		m_bChange = true;
	}
	//! 根据点数申请内存,用于内存动态处理,不和点云文件关联
	inline void resize(int row,int col)
	{
		m_simpleHeader.number_of_col = col;
		m_simpleHeader.number_of_row = row;
		m_simpleHeader.number_of_point_records = col * row;
		m_header = m_simpleHeader;
		m_pts.resize((unsigned int)(m_simpleHeader.number_of_point_records));
	}
	//! 获取点云数据指针
	inline PointXYZIPRGBA* data(){return m_pts.data();}
	//! 按反射强度降序排序,注意排序后点云就不是按行列排序
	inline void sortDsc()
	{
		m_simpleHeader.number_of_col = 1;
		m_simpleHeader.number_of_row = (U32)(m_simpleHeader.number_of_point_records);
		m_pts.sort();
	}

   //! 获取点云文件路径用于修改点云文件导出时的文件名获取方法-zhubo
    inline string GetPointCloudPath(){return m_hlsReader.m_filepath;} 

private:
	//std::vector<Normal>		m_normal;
	hdArray<Normal>				m_normal;

	/** \brief Placeholder for the 3x3 covariance matrix at each surface patch. */
	EIGEN_ALIGN16 Eigen::Matrix3f m_covariance_matrix;

	/** \brief 16-bytes aligned placeholder for the XYZ centroid of a surface patch. */
	Eigen::Vector4f m_xyz_centroid;

	unsigned int computeMeanAndCovarianceMatrix (
		const std::vector<size_t> &indices,
		Eigen::Matrix3f &covariance_matrix,
		Eigen::Vector4f &centroid);

	void computePointNormal(const std::vector<size_t> &indices, float &nx, float &ny, float &nz, float &curvature);

	void solvePlaneParameters(const Eigen::Matrix3f &covariance_matrix,
		float &nx, float &ny, float &nz, float &curvature);

	void flipNormalTowardsViewpoint (const PointXYZIPRGBA &point, float vp_x, float vp_y, float vp_z,
		float &nx, float &ny, float &nz);

public:

	void computeNormal(
		int k,			// 邻近的K个点
		float dist,		// dist距离范围内点,暂时未使用dist
		void (*loadCallback)(float,const char*) = NULL,//进度回调
		bool bFull = true);// true所有点云建立法向量,false选择集建立法向量
};


}
#endif