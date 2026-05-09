/*! PointCloud.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : PointCloud.h
相关文件     : 
文件实现功能 : 点云内存结构
作者         : 龚书林
版本         : 1.0
版权		 : CopyRight @ 2013 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/05/31   1.0      龚书林    
2013/09/25            冯晶              添加功能,设置手动分类窗口中的选择集点数
										,第一次选择集合与第二次选择集点数
</PRE>
*******************************************************************************/

#ifndef __HD_POINT_CLOUD_H_INCLUDED__
#define __HD_POINT_CLOUD_H_INCLUDED__

#pragma once
#pragma  warning(disable:4251)    // 禁止再导出类中提示std模板库警告

#include "hdPointCloud.h"
#include "..\hdCore\hdDefs.h"
#include "..\hdCommon\point_types.h"
#include "..\hdHLSlib\IHLSReader.h"
#include "..\hdCore\hdArray.h"
#include "..\hdCore\hdBlkArray.h"
#include "..\hdCore\hdBox3d.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "..\hd3DEngine\include\aabbox3d.h"
#include <vector>
#include <list>
#include "..\3rd\Eigen\Core"
#include "..\3rd\Eigen\Geometry"

#include "..\hdCommon\hdHdiStruct.h"
#include "..\hdCore\HdobBox3d.h"
#include <time.h>
#include "hdFilterManager.h"

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
		class CScanSceneNode;
	}
	class CScaleImage;

	// 结构体用于存放每圈点云的obb box的八个顶点
	struct LoopObboxVertex
	{
		// 顶点0
		f32 vertex0[3];

		// 顶点1
		f32 vertex1[3];

		// 顶点2
		f32 vertex2[3];

		// 顶点3
		f32 vertex3[3];

		// 顶点4
		f32 vertex4[3];

		// 顶点5
		f32 vertex5[3];

		// 顶点6
		f32 vertex6[3];

		// 顶点7
		f32 vertex7[3];

		// 初始化
		LoopObboxVertex()
		{
			memset(vertex0,0,sizeof(f32)*3);
			memset(vertex1,0,sizeof(f32)*3);
			memset(vertex2,0,sizeof(f32)*3);
			memset(vertex3,0,sizeof(f32)*3);

			memset(vertex4,0,sizeof(f32)*3);
			memset(vertex5,0,sizeof(f32)*3);
			memset(vertex6,0,sizeof(f32)*3);
			memset(vertex7,0,sizeof(f32)*3);
		}
	};

	typedef enum hdPtSelectMode
	{
		SELECT_NEW = 0,		// 新建选择
		SELECT_ADD = 1,			// 增加选择
		SELECT_MIUS = 2,		// 减少选择
		SELECT_UNSEL = 3,		// 反选择
        SELECT_ALL = 4          //全选
	};
	enum hdLoadSimpleMode
	{
		E_LOAD_NUM_SIMPLE   = 0,   // 按数据量进行抽稀加载
		E_LOAD_SPACE_SIMPLE = 1    // 按照空间进行抽稀加载
	};


	//class hd_kd_tree_t;
//template <typename PointT = PointXYZI,typename T = float>
class HDPOINTCLOUD_API PointCloud //class HDCOMMON_API
{
	friend class hd::fm::CHdPointSelect;
	friend class hd::fm::CHdPickCentriod;
	friend class hd::scene::CHdPlanarView;
	friend class hd::scene::CScanSceneNode;
	friend class hd::CScaleImage;
private:
	// 点数组,从物理文件读取到内存中的点,可能抽稀读取,用于显示
	hdBlkArray<PointXYZIPRGBA>  m_pts;
	//! 查询结果扫描列序号
	std::vector<u32> m_queryResult;
	//! 每圈的全局下标,第0圈下标是0,第1圈下标是0圈个数,第n圈是前n-1圈点数总和
	//std::vector<u64> m_vecLoopSuf;
	//! 读取游标
	u32  m_curRoop;

	// 点云过滤器（朱立雄 2017-3-20）
	hd::ptcloud::hdFilterManager*        m_pFilterManager;

	//! 点云加载到内存抽稀方式,默认是按照数量进行抽稀 chy 2017-3-22
	hdLoadSimpleMode m_load_simple_mode;

	//! 空间抽稀加载距离
	float m_load_simple_dist;

	
protected:

	// 级别数量
	u32 m_simpleLevel;
	// 点云索引
	void*   m_pIndex;
	// 点云是否改变,用于构建索引判断
	bool	m_bChange;
	// 判断点云是否编辑
	bool    m_bEdited;
	// 点云是否为编辑模式
	s32		m_editMode;
	//! 选择集二维数组 [zfei 2014/7/7]
	std::list<std::vector<u32>> m_listSelectionIDs;
	//! 分类选中选择集二维数组
	std::list<std::vector<u32>> m_ClassSelectionIDs;
	// 灰度最大值
	u32		m_maxIntensity;
	// 灰度最小值
	u32		m_minIntensity;
	//		最大距离
	f32		m_maxDistance;
	//		最小距离
	f32		m_minDistance;
	// 点云文件头信息
	IHLSReader* m_hlsReader;
	// 内存中有效点数
	u32		m_validCount;
	// 文件中的有效点数
	u64		m_HlsValidCount;
	// 选择集中点个数
	u32		m_selectCount;
	// 分类选中点个数
	u32		m_selectCountF;
	// 二次选择点个数
   // u32     m_selectCountS;
	//! 建立索引标记
	bool	m_fullIndex;
	//! 加载起始圈比例范围
	f64     m_startScale;
	f64		m_endScale;
	//! 加载点数控制
	u32		m_loadSimple;
	//! 转换绝对坐标对象
	CBursaWolfModel m_transModel;
	//! 记录车载工程点云当前的点云扫描头编号
	int    m_nScanNo;
	//! 删除的点数
	int    m_delPcdNum;

	/************* 试验记录每圈点云获得obb box的六个角点 ******************/
	//! 记录加载至内存的每圈点云的角点值，obb box主方向为指向下一圈点云方向

	////! 记录每圈点云中心点位置
	//std::vector<f32> m_vecLoopCenter;

	//! 记录obb 顶点
	std::vector<LoopObboxVertex> m_vecObbVertex;

	//! 标记同级目录下lin是否存在
	bool m_bLinExist;

public:
	//! 获得查询总圈的记录
	std::vector<u32>& GetQueryResult(){return m_queryResult;}

	//! lin文件是否存在，用于设置其渲染
	bool isLinExist(){return m_bLinExist;}

	////! 统计每圈点云，计算其pos中心（距离扫描头越近，其点应该越密，以最密的点段的平均值作为pos中心点（高程值不同没有影响））
	//void StatLoopForPos();

	//! 由点云路径查找同级目录下LIN文件,引用返回lin文件是否存在，若存在，返回lin的全路径
	string GetPosFileByHlsPath(bool& bLinExist);

	//! 读取LIN文件
	void ReadLin(const char* strLinPath,std::vector<HD_SCANHDIINFO>& vecScanInfo);

	//! 由pos中心点、目标点计算obb box,传入参数posX(YZ)为中心点,targetY(YZ)为目标点
	void CalculLoopObbBox(u32 loopIndex,f32 posX,f32 posY,f32 posZ,f32 targetX,f32 targetY,f32 targetZ);

	//! 计算文件中点云每圈点云的obb box，将其值依次push进m_vecObbBox(iscan)
	void CalculateObbBox();

	//! 计算文件中点云每圈点云的obb box，其值存入m_vecObbBox(hd ls 300)
	void CalculatObbBoxInStation();

	//! 从文件中获取该圈点中索引值最小的有效点坐标（本地坐标）
	void GetNearestPtInLoop(int loopInexInFile,PointXYZIPRGBA& pt);

	//! 获得对应索引的八个角点值vertex[24]
	void GetObbVertex(u32 loopIndex,f32* vertex);

	////! 统计每圈点云的中心位置，以该中心位置代替pos中心
	//void CalculLoopCenter();
	/************* end ****************************************************/
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
	//void get_point(PointXYZIPRGBA& pt);
	//! 获取当前读取的点,坐标系是全局坐标
	//void get_point(PointXYZI_D& pt);
//protected:

public:
	bool setSelectCount();

	// 设置所有选择点未选中
	void SetUnSelect();
	
	// 设置手动分类窗口中的选择集点数, TheCount为第一次选中
	bool setSelectCountF();
	void SetUnSelectF();

	//外部设置点云是否编辑改变(靶球查找部分临时pcd变量调用需要)
	void SetPointCloudChanged(bool bChanged){m_bEdited = bChanged;}
	
	//获取点云是否被编辑
	BOOL GetPointCloudChanged(){return m_bEdited;}

	// 获得当前点云对应扫描头编号
	int GetPointCloudScanNo(){return m_nScanNo;}
	
	// 设置当前点云对应扫描头编号
	void SetPointCloudScanNo(int scanNo){m_nScanNo = scanNo;}

	// 设置取消当前点云选中的点
	void DeselectPts();

	// 获得当前圈的有效点个数
    u32 GetValidCountInLoop( u32 index );

	// 获取点云加载比例
	void GetLoadScale(f64& fStartScale, f64& fEndScale) { fStartScale = m_startScale; fEndScale = m_endScale;}

	//获取HlsReader
	IHLSReader* GetHlsReader() { return m_hlsReader;}

	// 点云文件头
	HLSheader m_header;
	
	// 内存中抽稀后的文件头,点数和行列数≤文件中的相应值
	HLSheader m_simpleHeader;

	PointCloud():m_hlsReader(0),m_pIndex(0),m_bChange(false),m_maxIntensity(0),m_minIntensity(0xffffffff),
		m_minDistance(F32_MAX),m_maxDistance(F32_MIN),m_selectCount(0),m_selectCountF(0),m_simpleLevel(1),
		m_covariance_matrix(),m_xyz_centroid(),m_validCount(0),m_HlsValidCount(0),m_fullIndex(true),m_bEdited(false),
		m_startScale(0.0),m_endScale(1.0),m_loadSimple(5000000),m_curRoop(0),m_editMode(1),m_nScanNo(0),m_delPcdNum(0)
	{
		m_bLinExist = false;
		m_pFilterManager = new hd::ptcloud::hdFilterManager;
		m_load_simple_mode = E_LOAD_NUM_SIMPLE;
		m_load_simple_dist = 0.02f;
	}
	
	~PointCloud()
	{	
		clear();		
		delete m_pFilterManager;
	}
	
	inline hdBlkArray<PointXYZIPRGBA>& getPoints()
	{
		return m_pts;
	}

	inline PointXYZIPRGBA& operator[](u32 index)
	{ 
		return m_pts[index];
	}

	inline const PointXYZIPRGBA& operator[](u32 index) const
	{ 
		return m_pts[index];
	}

	//! 获取内存中的圈数
	inline u32 getLoopCount(){return m_pts.blockCount();}

	//! 通过索引编号获得文件圈物理编号
	inline s32 getLoopIndex(int index){return m_pts.getLoopIdx(index);}
	
	//! 获取圈范围
	inline void getLoopExtent(u32 index,f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax);				
	
	//! 根据圈索引,获取圈数据
	inline hdVector<PointXYZIPRGBA>& getLoop(u32 index){return m_pts.getBlock(index);}
	inline bool getLoop(u32 index,hdVector<PointXYZIPRGBA>** pts){return m_pts.getBlock(index,pts);}
	
	// 获取一圈扩展属性,index是圈
	template<class A>
	inline A* getBlockAttr(u32 index)
	{
		u32 size = 0;
		return m_pts.getBlockAttr<A>(index,size);
	}
	
	// 获取选择点的扩展属性,selIndex选择集序号
	template<class A> 
	inline A& getSelectPtAttr(u32 selIndex)
	{
		//return m_pts.getAttr<A>(*(m_selectionIDs._Myfirst + selIndex));

		return m_pts.getAttr<A>(getIndexBySelectionID(selIndex));
	}
	
	// 获取点的扩展属性,index点的序号
	template<class A> 
	inline A& getAttr(u32 index)
	{
		return m_pts.getAttr<A>(index);
	}

	inline const Normal& getNormal(u32 index) const
	{
		return m_normal[index];
	}
	
	inline u64 getNormalCount() const
	{
		return m_normal.count();
	}

	//! 从文件中读取点,并非在内存m_pts中,坐标系是本地坐标
	inline BOOL getPoint(
		int index,				// 点序号
		PointXYZIPRGBA& pt)		// 返回的坐标
	{
		if(m_hlsReader && m_hlsReader->Read_Point(pt,index))
		{
			return TRUE;
		}
		return FALSE;
	}
	
	//! 获取抽样级别
	inline u32 getSimpleLevel(){return m_simpleLevel;}

	//! 获取抽稀模式
	inline hdLoadSimpleMode getLoadSimpleMode() { return m_load_simple_mode; }

	//! 设置抽稀模式
	inline void setLoadSimpleMode(hdLoadSimpleMode load_simple_model) { m_load_simple_mode = load_simple_model;}

	//! 设置空间抽稀距离阈值
	inline void setSpaceSimpleDist(float dist) { m_load_simple_dist = dist;}

	//! 从文件中读取点,并非在内存m_pts中,坐标系是全局坐标
	inline BOOL getPointGlobal(
		int index,				// 点序号
		PointXYZI_D& pt)		// 返回的坐标
	{
		PointXYZIPRGBA pt_loc;
		if(m_hlsReader && m_hlsReader->Read_Point(pt_loc,index))
		{
			m_hlsReader->GetCoordinate(pt_loc.x,pt_loc.y,pt_loc.z,pt.x,pt.y,pt.z);
			pt.intensity = pt_loc.getIntensity();
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
			//if(m_hlsReader.read_fast(index))
			PointXYZIPRGBA pt_loc;
			if(m_hlsReader && m_hlsReader->Read_Point(pt_loc,index))
			{
				m_hlsReader->GetCoordinate(pt_loc.x,pt_loc.y,pt_loc.z,pt.x,pt.y,pt.z);
				pt.intensity = pt_loc.getIntensity();
				return TRUE;
			}
		}
		return FALSE;
	}
	
	//! 根据序号得到内存中的点绝对坐标,
	inline BOOL getGlobal(
		u64 index,				// 点序号,内存中点序号,0-count()
		PointXYZI_D& ptGlobal)	// 返回点绝对坐标
	{
		u64 nCount = m_pts.count();
		if(index < 0 || index >= nCount)
			return FALSE;
		const PointXYZIPRGBA& pt = m_pts[index];
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
		return m_pts[getIndexBySelectionID(index)];
	}
	
	//! 从分类选择集获取点
	inline PointXYZIPRGBA& getClassSelectionPoint(int index)
	{
		return m_pts[getIndexByClassSelectionID(index)];
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

	// 设置点云中的模型 [2013/11/25 危迟]
	void SetModel(CBursaWolfModel model,bool bNeedCal = false)
	{
		m_transModel = model;

		if (bNeedCal)
		{
			m_transModel.matrix2Parameter();
		}

		m_header.offsetX = m_transModel.m_fOffset[0];
		m_header.offsetY = m_transModel.m_fOffset[1];
		m_header.offsetZ = m_transModel.m_fOffset[2];
		m_header.rotateX = m_transModel.m_fAngle[0];
		m_header.rotateY = m_transModel.m_fAngle[1];
		m_header.rotateZ = m_transModel.m_fAngle[2];
		m_header.scale = m_transModel.m_fScale;

		SetPointCloudChanged(true);
	}

	//! 获取内存中点个数,如果抽稀加载则小于countInFile()
	inline virtual u64 count() const {return m_pts.count();}
	
	//! 获取内存中的有效点个数
	inline u32 getValidCount() const{return m_validCount;}

	//! 获取有效点个数, 获取文件的有效点数 fengjing
	u64 getValidCount(const char* hlsFile);

	
	//! 获取选中点个数
	inline u32 getSelectCount() const{return m_selectCount;}
	inline u32 getSelectCountF() const{return m_selectCountF;}

	// 编译不过，此处添加无用接口--add by zhubo2014.08.19
	inline u32 getSelectCountS() const{return 0;}
	
	//! 获取初次选中分类点个数
	//inline u32 getSelectCountF() const {return m_selectCount;}
	
	//! 获取文件中点个数
	inline u32 countInFile() const{return (u32)(m_header.number_of_point_records);}
	
	//! 获取是否为规则点云,即按行列排序的点云
	inline bool isNormalPointCloud() const;
	
	//! 清理
	inline void clear() 
	{
		if(m_pIndex != 0)
		{
			delete m_pIndex;
			m_pIndex = 0;
		}
		if (m_hlsReader)
		{
			delete m_hlsReader;
			m_hlsReader = NULL;
		}
		m_maxIntensity = 0;
		m_minIntensity = 0;

		// 清空时同时应更改标记为false，防止紧接着下次打开时上次的标记状态还存在-zhubo.11.13
		m_bEdited = false;

		// 对于vector，clear并没有释放内存，只是改变了size，为了避免这种情况，使用swap彻底改变内存，
		// 虽然不会产生内存泄露，但是防止多次操作造成连续内存不足够 fengjing
		//m_SecSelectionIDs.clear();
		//std::vector<u32> ().swap(m_SecSelectionIDs);
		m_ClassSelectionIDs.clear();
		m_listSelectionIDs.clear();

		m_vecObbVertex.clear();
		std::vector<LoopObboxVertex> ().swap(m_vecObbVertex);

		m_selectCount = 0;
		m_selectCountF = 0;

		m_pts.clear();
		m_header.clean();
	}

	//! 通过选择集的序号获得内存中点云的真实序号[zfei 2014/7/8]
	u32 getIndexBySelectionID(int selIndex) const;

	//! 通过分类选择集的序号获得内存中点云的真实序号 [危迟 2014/08/18]
	inline u32 getIndexByClassSelectionID(int selIndex) const
	{

		// 链表的位置;
		int nListPos = selIndex / (int)0xffff;

		// 链表内部的位置;
		int nListInnerPos = selIndex % (int)0xffff;

		// 迭代器移动到指定的链表位置;
		list<vector<u32>>::const_iterator it = m_ClassSelectionIDs.begin();
// 		for (int i = 0; i < nListPos; i++)
// 		{
// 			it++;
// 		}
		std::advance(it, nListPos);

		// 获得点的序号;
		
		const vector<u32>& vecSelIDs = (*it);

		return vecSelIDs.at(nListInnerPos);
	}

	//! 删除选中的点云
	void deleteSelectPoints();
	
	//! 删除选中的点云,index选择集序号
	void deleteSelectPoint(int index)
	{
		//PointXYZIPRGBA& pt = m_pts[(*(m_selectionIDs._Myfirst + index))];

		PointXYZIPRGBA& pt = m_pts[getIndexBySelectionID(index)];
		pt.setDeleted();
	}
	
	//! 删除指定点,index点云内存序号
	void deletePoint(int index)
	{
		PointXYZIPRGBA& pt = m_pts[index];
		pt.setDeleted();
	}

	// 返回点数
	inline size_t kdtree_get_point_count() const 
	{
		return (size_t)(m_fullIndex ? m_pts.count():m_selectCount);
	}
	
	// 返回距离
	inline float kdtree_distance(const float *p1, const size_t idx_p2,size_t size) const
	{
		if (idx_p2 >= m_pts.count())
		{
			return 0.f;
		}
		const PointXYZIPRGBA& pt = m_fullIndex ? m_pts[idx_p2] ://*(m_pts._Myfirst + idx_p2) :
									m_pts[getIndexBySelectionID(idx_p2)];						
									//m_pts[*(m_selectionIDs._Myfirst + idx_p2)];//*(m_pts._Myfirst + (*(m_selectionIDs._Myfirst + idx_p2)));

		const float d0 = p1[0] - pt.x;
		const float d1 = p1[1] - pt.y;
		const float d2 = p1[2] - pt.z;

		return d0*d0+d1*d1+d2*d2;
	}
	
	// 返回点
	inline float kdtree_get_pt(const size_t idx, int dim) const
	{
		const PointXYZIPRGBA& pt = m_fullIndex ? m_pts[idx] :
									m_pts[getIndexBySelectionID(idx)];
									//m_pts[*(m_selectionIDs._Myfirst + idx)];

		if (!pt.isValid())
		{
		   int i = 0;
		}
		if (dim==0) return pt.x;
		else if (dim==1) return pt.y;
		else return pt.z;
	}
	
	// 返回范围
	template <class BBOX>
	bool kdtree_get_bbox(BBOX &bb) const { return false; }

	// 构建索引
	//template <typename PointT,typename T>
	bool buildIndex(bool bFullIndex = true);
	
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
	
	//! 将hls点云文件一次性加载到内存,通过空间抽稀加载
	BOOL loadSpaceSimple(
		const char* hlsFile,			// 加载点云路径
		float filter_dist = 0.02f,		// 空间去重抽稀距离
		void (*loadCallback)(float,const char*) = NULL);// 加载进度回调函数

	//! 加载索引
	CLoopIndex* loadLoopIndex()
	{
		if(m_hlsReader)
			return m_hlsReader->GetLoopIndex();
		else
			return NULL;
	}
	
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
	
	//! 根据视口查询加载数据
	inline BOOL LoadByViewPort(
		/*std::vector<u8>& vecInview ,*/
		bool (*IsCubeIn)(irr::core::aabbox3df& cube),
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		const CHdBox3df& box,							// 视锥体外边框
		const bool isOrthogonal,						// 是否为正射投影
		HRGN srcRgn = NULL);							// 屏幕选择范围,用于用户拉框查询

	//! 根据视锥Obbbox实时从文件中加载点云，传入参数为视锥obbbox
	inline BOOL LoadByObbViewPort( CHdobBox3d viewBox,irr::core::aabbox3df viewAabbox/*,std::vector<u8>& vecInView*/);

	//! 根据视图范围加载数据
	inline BOOL LoadByEnvelope(double xmin, double xmax, double ymin, double ymax);							

	//! 查询有多少个扫描列或者块在范围内,查询坐标是相对坐标,返回列数或块数
	inline u32 QueryByExtent(f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax);
	inline u32 QueryByExtent(f64 xmin,f64 ymin,f64 zmin,f64 xmax,f64 ymax,f64 zmax);

	//! 统计矩形框范围内点云的高度范围,xmin,xmax,ymin,ymax是输入参数,zmin和zmax是计算结果
	BOOL QueryZRange(f64 xmin,f64 xmax,f64 ymin,f64 ymax,
		bool bStatSel,		// true统计选中,false 统计所有
		f64& zmin,f64& zmax);
	
	//! 查询所有列或者块,返回列数或块数
	inline u32 QueryAll();
	
	//! 重置查询游标到起始位置
	inline void ResetRead();
	
	//! 读取下一个扫描列,返回的点数组需要做二次判断是否在范围内.返回是否成功
	inline BOOL ReadNext(hdVector<PointXYZIPRGBA>& pPtBuf);
	
	//! 结束读取
	inline void EndRead();
	
	//! 得到当前圈范围
	inline void GetExtent(f32& xmin,f32& ymin,f32& zmin,f32& xmax,f32& ymax,f32& zmax);
	
	inline void SelectPoint(HRGN hRgn,
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&));		// 三维坐标转屏幕坐标回调函数);

	//! 将点云设为彩色的类型 add by wkl 2012/8/11
	inline void setAsColorPointCloud() 
	{
		this->m_header.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
		this->m_simpleHeader.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
	};

	//! 关闭,重新打开,用于写文件时调用
	void close()
	{
		if(m_hlsReader)
			m_hlsReader->Close();
	}
	BOOL open();	

	//! 设置加载点个数
	void setLoadSimple(u32 loadSimple){m_loadSimple = loadSimple;}

	//! 获取加载点个数
	u32 getLoadSimple(){return m_loadSimple;}

	//! 设置编辑模式,1编辑模式,0浏览模式
	void setEditMode(s32 mode);

	//! 获取编辑模式
	s32 getEditMode(){return m_editMode;}

	/******************以下接口用于点云动态内存处理***************/
	//! 添加点,用于内存动态处理,不和点云文件关联
	/*inline void addPoint(const PointXYZIPRGBA& pt)
	{
		m_pts.push_back(pt);
		m_bChange = true;
	}*/
	
	//! 根据点数申请内存,用于内存动态处理,不和点云文件关联
	inline void resize(int row,int col)
	{
		m_simpleHeader.number_of_col = col;
		m_simpleHeader.number_of_row = row;
		m_simpleHeader.number_of_point_records = col * row;
		m_header = m_simpleHeader;
		m_pts.resize((unsigned int)(m_simpleHeader.number_of_point_records));
	}

	inline void resize(unsigned int connts)
	{
		m_simpleHeader.number_of_col = connts/1024;
		m_simpleHeader.number_of_row = 1024;
		m_simpleHeader.number_of_point_records = connts;
		m_header = m_simpleHeader;
		m_pts.resize(connts);
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
    inline string GetPointCloudPath()
	{
		if (m_hlsReader)
		{
			return m_hlsReader->GetFilePath();
		}
		return string("");
	} 

	// 获取点云的过滤器（朱立雄 2017-3-20）
	hd::ptcloud::hdFilterManager* GetFilterManager(){ return m_pFilterManager; }

	// 过滤内存中的点（朱立雄 2017-4-24）
	// bUseAll: true 表示用所有过滤器重新过滤，false 表示只用最后添加的过滤器进行增量过滤
	bool ApplyFilters(bool bUseAll);

private:
	// 判断当前列是否在视窗内
	inline BOOL IsLoopInView(
		PointXYZIPRGBA* ptBuf,					// 点数组
		u32	count,
		BOOL (*ViewTrans)(const f32&,const f32&,const f32&,s32&,s32&),		// 三维坐标转屏幕坐标回调函数
		s32 srcWidth,									// 视口宽度
		s32 srcHeight,									// 视口高度
		u32 uStart,										// 起始点号
		u32 uStep,										// 采样判断步长
		HRGN srcRgn = NULL);										
	
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

	void computePointScatter(const std::vector<size_t> &indices, float &nLine, float &nPlane, float &nShpere);

	//! 求取点集的散列参数（线性参数、平面性参数、球性参数）[zfei 2014/9/11]
	void solveScatterParmeters(const Eigen::Matrix3f &covariance_matrix,
		float &nLine, float &nPlane, float &nSphere);

	void solvePlaneParameters(const Eigen::Matrix3f &covariance_matrix,
		float &nx, float &ny, float &nz, float &curvature);

	void flipNormalTowardsViewpoint (const PointXYZIPRGBA &point, float vp_x, float vp_y, float vp_z,
		float &nx, float &ny, float &nz);

public:
	//! 计算点云的散列性 [zfei 2014/9/11]
	void computeScatter(
		int k,
		float dist,		//暂时未使用dist
		void (*loadCallback)(float,const char*) = NULL,
		bool bFull = true);

	void computeNormal(
		int k,			// 邻近的K个点
		float dist,		// dist距离范围内点,暂时未使用dist
		void (*loadCallback)(float,const char*) = NULL,//进度回调
		bool bFull = true);// true所有点云建立法向量,false选择集建立法向量
	
	inline int getDelPcdNum(){ return m_delPcdNum;}

	//! 导出选择过滤范围数据至文件（hls）
	bool export_filter(const char* save_hls_file, CBursaWolfModel* render_transformation,
		ptcloud::hdFilterManager* filter_manager, u32 simple_size=1, void (*callBack)(float,const char*) = NULL);

};


//点坐标数组按照距离范围进行格网抽稀，每个格网中仅保留与格网中心点最近的点
template <class T>
u32 simpleByDistance(
	const hdVector<T>& raw_pts,		// 输入点数组
	f64 distance,					// 简化距离
	hdVector<T>& simple_pts,		// 简化后点数组
	CHdBox3df* pBox = NULL);		// 输入点数组空间范围,允许为空

}
#endif