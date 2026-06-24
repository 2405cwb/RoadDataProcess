//#pragma once
//#include "inc\mydefs.hpp"
//#include "HlzBuilderDefs.h"
//#include "..\hdCore\hdBox3d.h"
//#include "..\hdCore\hdVector3d.h"
//#include "..\hdCommon\point_types.h"
//#include "HLZWriter.h"
//#include "HLZ31Writer.h"
//#include <vector>
//#include <string>
//using namespace hd;
//using namespace std;
//
//namespace hd
//{
//	class CHdLevel31;
//	class IHLSReader;
//	class CHLZWriter;
//	class CHdCoreData;
//	
//	template<class T>
//	class  CHlz31Builder
//	{
//	public:
//		CHlz31Builder(void);
//		~CHlz31Builder(void);
//
//		//! 添加一个hls文件或者las文件
//		void addFile(const std::string& hlsFile)
//		{
//			m_hlsFileList.push_back(hlsFile);
//		}
//
//		//! 添加一系列hls文件或者las文件
//		void addFiles(const std::vector<const std::string>& hlsFiles)
//		{
//			m_hlsFileList.insert(m_hlsFileList.end(), hlsFiles.begin(), hlsFiles.end());
//		}
//
//		//! 清空转换文件列表
//		void clearFiles()
//		{
//			m_hlsFileList.clear();
//		}
//
//		//! 外部接口设置，第0层数据块集格网层级
//		void SetGridLevel(U8 gridLevel)
//		{
//			m_gridLevel = gridLevel;
//		}
//
//		//! 开启hlz数据转换
//		bool buildHlz(const char* savePath, bool isCompress = false, void (*processCallback)(float, const char*) = NULL);
//
//		//! 关闭文件，清理资源
//		void Close();
//
//	private:
//		//读取ini配置文件，获取转换参数配置信息
//		void GetConvertConfig();
//
//		// 清除包文件数据
//		void ClearParcelData();
//
//		// 清空临时块集文件的内存空间
//		void ClearBlocksetData();
//
//		//清空分拨中间缓存
//
//		void ClearSplitMemBuf(vector<DivideMemoryBuf<T>> & vecMemBuf);
//
//		//! 删除所有临时文件 2013/11/19 蔡红云 
//		bool RemoveTempFiles(const char* path); 
//
//		//! 删除块集文件
//		BOOL RemoveBSFiles(std::map<U64, BlockSetFileInfo31>& bsFiles);
//
//		//! 切割输入文件
//		bool SplitInputFiles(const char* savePath);
//
//		//! 按标准格网切割Hls成为块集
//		bool SplitBlockSetFiles(const char* savePath, const std::string& strHlsFile);
//
//		//! 切割Las到块集文件
//		bool SplitBlockSetLas(const char* savePath, const std::string& strLasFile);
//
//		//! 对块集进行切割成块
//		bool SplitBlockFiles(BlockSetFileInfo31& procBlockSet, std::vector<BlockFileInfo31>& arrayBlocks, U8 BSNo, BlockSetFileInfo31& nextBSInfo);
//
//		//! 对块切割成包,包成每个包的点云数量不超过64000
//		bool SplitParcelFiles(const CHdBox3di&	blockBox, const char* strBlockFile,	const char* savePath, std::vector<ParcelFileInfo31>& arrayParcels);
//
//		// 对块集进行切割成块,并且将切分结果保存在内存中
//		bool SplitBlockset2Block_Buffer(BlockSetFileInfo31& procBlockSet, std::vector<BlockFileInfo31>& arrayBlocks, U8 BSNo, BlockSetFileInfo31& nextBSInfo);
//
//		// 对内存中的块切割成包
//		bool SplitBlock2Parcel_Buffer(const CHdBox3di&	blockBox, const char* strBlockID, const char* strPacelID, std::vector<ParcelFileInfo31>& arrayParcels);
//
//		// 对内存中的包递归切分
//		bool SplitParcel2Parcel_Buffer(const CHdBox3di&	blockBox, const char* srcParcelID, const string destPacelID, std::vector<ParcelFileInfo31>& arrayParcels);
//
//		// 将块文件分割成子块，保证每个子块点数小于MAX_POINT_NUM
//		bool SplitBlock2Parts(const CHdBox3di& blockBox, const string strBlockFile, std::vector<subBlockFileInfo31>& subBlk);
//
//		// 子块文件加载到内存中分包
//		bool SplitSbkFile2Parcel_Buffer(const CHdBox3di& blockBox, const string strSbkFile, std::vector<ParcelFileInfo31>& vecParcelInfo);
//
//		//! 将第0层数据写入到文件
//		U64  writeLevelData(CHdLevel31* pLevel);
//
//		//! 统计点云强度范围
//		bool StatIntensity(IHLSReader* pHlsReader, U16& min, U16& max);	
//
//		//计算HLS文件中的数据的最大值、最小值
//		bool CalHlsExtent(hd::IHLSReader* reader, double& x_min, double& x_max,double& y_min,double& y_max,double& z_min, double& z_max);
//
//		//! 根据点的坐标确定其所在的块位置
//		// 将块集切分为八块，即XYZ方向各切一刀;编码从左到右,从下到上
//		//    6--7
//		//  4 --5|
//		//  | |  |
//		//	| 2--3
//		//	|/  /
//		//  0--1
//		inline void getPtBlockIndex(T& pt, const CHdVector3di& mid, U8& index);
//
//		//! 获取块（或者包）的空间范围
//		template<class X>
//		void getBlockBox(const CHdVector3d<X>& mid, const CHdVector3d<X>& halfSize,	CHdBox3d<X>& box,	U16 index);
//
//		//! 根据空间范围确定包所在块内部的编号
//		void getParcelNo(const CHdBox3di& blockBox, const CHdBox3di& parcelBox, U8& divCount, U16& xNo, U16& yNo, U16& zNo);
//
//		//! 根据当前块集的编号，计算所属下层块集的编号与所属的子空间编号
//		void getNextLevelBSIndex(const EntityIndex& curIndex, EntityIndex& nextIndex, U8& BSNo);
//
//		//! 根据当前块集在下级块集中的编号，将当前块集中点坐标转换为下级块集坐标
//		bool GetNextLevelPos(const T& prePos, U8 index, T& curPos);
//
//		void getParcelBox(CHdBox3dd& parcelBox, const CHdBox3dd& blockBox, U8 divCount, U16 xNo, U16 yNo, U16 zNo);
//
//		//! 对点进行抽稀操作，结果存放到指定文件中
//		bool simplePoints(const std::vector<T>& pts, U8 BSNo, BlockSetFileInfo31& BSInfo);
//
//		//! 对点进行256*256*256子空间划分，根据划分结果重新排列点顺序
//		bool subDivide(std::vector<T>& pts, std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts);
//
//		//! 对点进行256*256*256子空间划分，根据划分结果重新排列点顺序
//		bool subDivide(std::vector<T*>& pts, std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts);
//
//		//! 将点属性信息拆分到不同buffer中
//		int SplitPt2Buffers(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, bool compress, U8** pPtBaseAttri, U32& baseAttriLen,  U32& cmpLen,  HdPtColor**  pPtColor);  
//
//		int SplitPt2Buffers_Normal(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, U8** pPtBaseAttri, U32& baseAttriLen,  HdPtColor**  pPtColor);
//
//		int SplitPt2Buffers_Cmp(const std::vector<SubEntityIndex>& subIndices, T* ppPtAry[], int count, U8** pPtBaseAttri, U32& baseAttriLen,  U32& cmpLen,  HdPtColor**  pPtColor);  
//
//		template <class X>
//		void FreeSplitBuffer(std::map<EntityIndex, DivideMemoryBuf<X>>& BsPointsArray);
//
//		void FreeIndexBuffer(std::vector<SubEntityIndex>& subIndices, std::vector<T*>& sortedPts);
//
//		// 数据复制
//		inline void VecCopy3fv(double v[3], const double a[3])
//		{
//			v[0] = a[0];
//			v[1] = a[1];
//			v[2] = a[2];
//		}
//
//		// 比较最大值最小值
//		inline void VecUpdateMinMax3dv(double min[3], double max[3], const double v[3])
//		{
//			if (v[0]<min[0]) min[0]=v[0]; else if (v[0]>max[0]) max[0]=v[0];
//			if (v[1]<min[1]) min[1]=v[1]; else if (v[1]>max[1]) max[1]=v[1];
//			if (v[2]<min[2]) min[2]=v[2]; else if (v[2]>max[2]) max[2]=v[2];
//		}
//
//		void GetBoundBox(I32 xNo, I32 yNo, I32 zNo, U8 level, const std::vector<T>& pts, CHdBox3dd& realBox);
//
//		void GetBoundBox(I32 xNo, I32 yNo, I32 zNo, U8 level, T* pts[], int count, CHdBox3dd& realBox);
//
//	private:
//		//! 文件转换列表
//		std::vector<std::string>  m_hlsFileList;
//
//		//! 总有效点数
//		U64  m_ptNum;
//
//		//! 反射强度最小值
//		U16  m_iMin;
//
//		//! 反射强度最大值
//		U16  m_iMax;
//
//		//! 0层块集格网层级
//		U8   m_gridLevel;  
//
//		FILE* m_logFile;
//
//		//hlz文件写入对象
//		CHLZ31Writer* m_pHlzWrite;
//
//		//所有hlz层对象信息
//		std::map<U16, CHdLevel31*> m_pLevels;
//
//		//! 坐标统计变量
//		CoordStatInfo m_coordStats;
//
//		//! 进度回调函数
//		void (*m_callback)(float,const char*);
//
//		//文件输出目录
//		std::string  m_savePath;
//
//		//输出hlz文件名
//		std::string  m_hlzFileName;
//
//		//完整空间包围盒
//		CHdBox3dd    m_fullExtent;
//
//		//块集临时文件
//		std::map<EntityIndex, BlockSetFileInfo31> m_BlocksetFiles;
//
//		//块集数据临时存储容器
//		std::vector<T>  m_blsDataBuf;
//
//		// 块数据存储数组，每个键值对表示一个块
//		std::map<string, vector<T*> > m_pBlockData;
//
//		// 包临时数据存储数组
//		std::map<string, vector<T*> > m_ParcelData;
//
//		//分割阈值配置项
//		U32  m_entity_point_threshold; //块集、块、包分割阈值
//		U32  m_level_point_threshold;  //层最少点数
//		U32  m_memory_point_throughput; //内存点吞吐量
//
//		//抽稀配置项
//		U32 m_sample_size; //抽稀点样本大小
//		U32 m_sample_bat_num; //并行抽稀批数
//		U8  m_sample_gran; //层级抽稀粒度
//	};
//
//	EXPIMP_TEMPLATE template class HLS_API CHlz31Builder<XYZIPRGB>;
//	EXPIMP_TEMPLATE template class HLS_API CHlz31Builder<XYZIP>;
//}
//
