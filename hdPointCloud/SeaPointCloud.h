/*! SeaPointCloud.h
********************************************************************************
<PRE>
模块名       : hdPointCloud
文件名       : SeaPointCloud.h
相关文件     : 
文件实现功能 : 海量点云内存结构
作者         : 蔡红云
版本         : 1.0
版权		 : CopyRight @ 2015 海达数云
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/04/11   1.0      蔡红云              创建实现
2015/10/6    1.1      蔡红云              增加按POS高度过滤、按POS距离过滤
</PRE>
*******************************************************************************/
#pragma once
#include "hdPointCloud.h"
#include "..\hdHLSlib\HdCoreData.h"
#include "..\hdHlslib\HlzDefs.h"
#include "..\hdCommon\BursaWolfModel.h"
#include "..\hdCommon\hdHdiStruct.h"
#include <list>
#include <vector>
#include "..\hdCommon\hdMongoDBStructDef.h"
#include "hdFilterManager.h"

namespace hd
{

	class HDPOINTCLOUD_API CSeaPointCloud
	{
	public:
		CSeaPointCloud(void);
		~CSeaPointCloud(void);
	public:

		BOOL open(const char* hlzPath);
	
		// 点云文件读取
		IHLSReader* m_hlzReader;

		// HLZ 头文件
		HLZheader m_hlzHeader;

		// 获取点云中的模型
		const CBursaWolfModel& GetModel();

		// 设置点云中的模型
		void SetModel(CBursaWolfModel model,bool bNeedCal = false);
				
		//外部设置点云是否编辑改变(靶球查找部分临时pcd变量调用需要)
		void SetPointCloudEdited(bool bEdited)
		{
			m_bEdited = bEdited;
		}

		//获取点云是否被编辑
		BOOL GetPointCloudEdited()
		{
			return m_bEdited;
		}

		// 清除透明度内存
		void ClearAMry();

		// 即时释放透明度关联内存 index--> 包地址
		void ReleaseAMry(U64 index);

		// 计算透明度信息
		void CalAphaInfo(CHdParcelBase* pHdParcelBase, int maxinsity, int minintensity);

		// 返回显示区域列表
		CHdListAreaNoInf& GetHdListAreaNoInf();

		//设置显示区域列表 lixialiang---- add 2016/04/20
		void SetHdListAreaNoInf(CHdListAreaNoInf& pListAreaInfo);

		// 返回透明度字典表
		map<U64,hd::u8* >& GetAphaList();

		//*********** 按POS过滤相关接口*******************************************
		
		// 寻找最近的HDI x,y,z 坐标   vechdi 轨迹点数组
		void FindNearHdi(double&x,double& y, double& z ,std::vector<HD_SCANHDIINFO>& vechdi);

		// 取消选中点
		void DeselectPts();

		// 按pos高度过滤
		int FilterPcdByPosHeight();

		// 按高度过滤
		int FilterPcdByHeight();

		// 按pos距离过滤
		int FilterPcdByPosDistace();

		// 按pos高度过滤参数设置
		void SetFilterminHeight(double h); // 设置高度下限
		void SetFiltermaxHeight(double h);// 设置高度上限
		void SetSel(int sel);   // 设置过滤模式
		void Setbhigh(BOOL bh); // 是否高于

		// 按pos距离过滤参数
		void SetBless(BOOL bh);// 是否下于
		void SetDistance(double d);// 设置距离

		// 设置按pos过滤的方式 fs -1 ，0 按Pos高度,1按Pos距离,2按高度
		void SetbyPosFilter(int fs);

		// 更新按pos进行过滤的点云
		void UpdateFilterPcd();

		// 初始化按Pos过滤信息 vechdi hdi轨迹点信息
		bool InitFltByPosInf(std::vector<HD_SCANHDIINFO>& vechdi);

		// 设置过滤时所使用的Lin文件
		void SetLinPahForFltr(const string& path);

		// 获取点云文件路径
		inline string GetPointCloudPath()
		{
			if (m_hlzReader)
			{
				return m_hlzReader->GetFilePath();
			}
			return string("");
		} 

		// 加载hlz文件头
		BOOL loadHlzFileHeader(const char* hlzFile);

		// 获取当前点云所在目录下的lin文件路径
		string GetLinPath();

		// 卸载包数据
		BOOL UnLoadSpecParcel(CHdParcelBase* noInfo);

		// 一次load所有包数据
		BOOL LoadAllListAreaParcels(CHdListAreaNoInf* pHdlstArea);

	  //--------- 以下为骆虎强在201/1/6添加,为支持hlz格式点云的深度图修改,按照PointCloud类迁移----------//

	  //! 查询有多少个扫描列或者块在范围内,查询坐标是相对坐标,返回列数或块数
	  u32 QueryByExtent(f32 xmin,f32 ymin,f32 zmin,f32 xmax,f32 ymax,f32 zmax);
	  u32 QueryByExtent(f64 xmin,f64 ymin,f64 zmin,f64 xmax,f64 ymax,f64 zmax);

	  //! 读取下一个扫描列,返回的点数组需要做二次判断是否在范围内.返回是否成功
	  BOOL ReadNext(hdVector<HlzPoint>& pPtBuf, int bufIndex);

	  // 获取点云的过滤器（朱立雄 2017-3-20）
	  hd::ptcloud::hdFilterManager* GetFilterManager(){ return m_pFilterManager; }

protected:
    //! 深度图查询出在范围内的list
    CHdListAreaNoInf        m_pListAreaNoInf;

  //--------- 以上为骆虎强在201/1/6添加,为支持hlz格式点云的深度图修改,按照PointCloud类迁移----------//

	protected:
		
		//! 转换绝对坐标对象
		CBursaWolfModel m_transModel;

		// 点云是否被编辑
		BOOL            m_bEdited;

	private:

		hd::ptcloud::hdFilterManager*   m_pFilterManager;

		// 显示区域
		CHdListAreaNoInf  m_HdlstArea;

		// 强度关联
		map<U64,hd::u8* > m_apha; 

		// 代码锁,支持多线程
		CRITICAL_SECTION		m_cs;

		// 按POS过滤的方式 - 1,默认 不采用按Pos过滤模式，0 按Pos高度过滤 1 按Pos距离过滤
		int m_byPosFilter;

		// 按pos高度过滤参数
		int m_sel;// 所选方式
		double m_FilterminHeight;//高度下限
		double m_FiltermaxHeight;// 高度上限
		BOOL m_bhigh; // 是否高于

		// 按pos距离过滤参数
		BOOL m_bless; // 是否小于 
		double m_distance;// 距离
		string m_linPathForFlt;//过滤时选择的lin文件

		// 记录该点云所属工程名
		string m_strRouteName;

		// 记录点云所对应的工程扫描头
		int m_nScanNo;

		// 字符串替代
		void replace_all_distinct(string&str,const string & oldstr,const string& newstr);
	
		// 禁止拷贝
		CSeaPointCloud &operator = (const CSeaPointCloud& other);
		CSeaPointCloud(const CSeaPointCloud& other);
		
	};

}

