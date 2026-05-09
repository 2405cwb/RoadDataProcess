/*! @HlzMemCloudBuilder
********************************************************************************
<PRE>
模块名       : HdHlsLib
文件名       : HlzMemCloudBuilder.h
相关文件     : 
文件实现功能 : 封装接口支持pos数据转换为hlz文件
作者         : 袁亮
版本         : VER 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2015/07/05   1.0      袁亮	              创建
</PRE>
*******************************************************************************/

#pragma once
#include "mydefs.hpp"
#include <vector>
#include <map>
#include "..\hdCore\hdBox3d.h"
#include "..\hdCommon\point_types.h"
#include "HLZWriter.h"
#include "HlzBuilderDefs.h"

using namespace std;
namespace hd
{
	 struct PointXYZIPRGBA_D
	{
		hd::f64 x, y, z;
		hd::u16 intensity;
		union
		{
			hd::u8 r;
			hd::u8 g;
			hd::u8 b;
			hd::u8 prop;
		};
		hd::u32 c_color;
        
		PointXYZIPRGBA_D()
			:x(0.0),y(0.0),z(0.0),intensity(0),prop(1),b(0),g(0),r(0)
		{

		}

		inline bool isValid() const
		{
			return !(x == 0.0 && y == 0.0 && z == 0.0);
		}
	};

	class CHLZWriter;
	class CHdCoreData;

	class HLS_API CHlzMemCloudBuilder
	{
	public:
		CHlzMemCloudBuilder(void);
		~CHlzMemCloudBuilder(void);

		/*
		  函数功能：
				   设置hlz文件转换及输出参数。
		  函数参数：
				   [in] savePath : hlz文件输出绝对路径
				   [in] savePathNext : hlz文件输出绝对路径（跨带点云切割会产生两个hlz文件，此时需要传入第二个hlz文件的保存路径；非跨带情况下，此输入参数将被忽略）
				   [in] fullExtent: 点云文件的估计绝对空间范围，必须是实际空间范围的超集
				   [in] centerLongitude: 中央经线值，结合fullExtent可判断是否需要分带存储
				   [in] compress: 是否对hlz数据文件进行压缩存储
				   [in] processCallback: 进度回调函数, float为当前阶段进度值（0-1之间），const char*为当前阶段文字描述
		  函数返回值：
					true : 设置成功
					false : 设置失败
		  备注：函数作用等价于CHlzCloudBuilder::buildHlz()接口中，SplitInput(MultiZone)Files()之前的所有操作
		*/
		bool OpenFile(const char* savePath, const char* savePathNext, const CHdBox3dd& fullExtent, double centerLongitude, bool compress = false, void (*procCallback)(float, const char*) = NULL);

		//按照本地缓存hlz文件方式存储，不使用云存储分带方式    袁亮    20160818
		bool CreateHlz(const char* savePath, const CHdBox3dd& fullExtent, bool compress = false, void (*processCallback)(float, const char*) = NULL);

		//! 外部接口设置，是否抽稀零层及抽稀精度，若为false，该抽稀精度为1层的抽稀精度1/2
		void SetResampleZero(bool bResample,double dGridPrecision);
		/*
		  函数功能：
		           向hlz文件中追加“写入”点数据
		  函数参数：
		           [in] points : 点数据vector（包含全局坐标、强度、颜色等信息）
		  函数返回值：
				   true : 缓存成功
				   false : 缓存失败
		  备注： 
		           此接口只对当前点云数据进行了块集划分及文件缓存操作，并没有开始进行hlz文件的写入；停止追加数据，并获得最终的hlz文件，请调用Flush()接口；
				   等价于CHlzCloudBuilder::buildhlz()中的SplitInput(MultiZone)Files()操作
		*/
		bool WriteFile(const vector<PointXYZIPRGBA_D>& points); 

		//按照本地缓存hlz文件方式存储，不使用云存储分带方式    袁亮    20160818
		bool AppendData(const vector<PointXYZIPRGBA_D>& points);
		/*
		  函数功能：
		            将所有缓存文件中的点数据写入到hlz文件中
		  函数参数：无
		  函数返回值：
		            true : 写入成功
					false : 写入失败
		  备注：
		           等价于CHlzCloudBuilder::buildhlz()函数中SplitInput(MultiZone)Files()之后的所有操作
		*/
		bool Flush();    

		/*
		  函数功能：释放资源
		  函数参数：无
		  函数返回值：无
		*/
		void CloseFile();


	private:
		bool RemoveTempFiles(const char* path);
        
		bool RemoveBSFiles(std::map<U32, BlockSetFileInfo>& bsFiles);

		inline void getPtBlockIndex(PointXYZIPRGBA& pt, const CHdVector3df& center, U8& index); //根据点与外包盒中心点的位置关系，判断点所属八叉划分空间序号

		void getBlockBox(const CHdVector3df& mid, const CHdVector3df& halfSize, CHdBox3df& box,	U16 index); //根据八叉划分空间序号计算其外包盒

		//! 根据当前层的块集编号确定上一层的块集
		void getPreLevelBlockset(
			U16 xNo,					// 输入当前块集的位置X
			U16 yNo,					// 输入当前块集的位置Y
			CHdLevel* pPreLevel,			// 输出上一层
			std::vector<BlockSetFileInfo>& preBlocksetFiles);	// 当前块集对应的上一层块集

		//! 根据当前层的块集编号确定上一层的块集(对应相临区域)
		void getPreLevelBlocksetNext(
			U16 xNo,					// 输入当前块集的位置X
			U16 yNo,					// 输入当前块集的位置Y
			CHdLevel* pPreLevel,			// 输出上一层
			std::vector<BlockSetFileInfo>& preBlocksetFiles);	// 当前块集对应的上一层块集

		void clearBlocksetData();

		void clearParcelData();

		void clearSplitMemBuf(vector<SplitMemoryBuf>& vecMemBuf);

		bool SplitBlockset2Block_Buffer(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);

		bool SplitBlockset2Block_BufferNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);

		bool SplitBlock2Parcel_Buffer(const CHdBox3df& blockBox, const char* strBlockID, const char* strPacelID, std::vector<ParcelFileInfo>& arrayParcels);

		bool SplitParcel2Parcel_Buffer(const CHdBox3df&	blockBox,const char* srcParcelID,const string destPacelID,std::vector<ParcelFileInfo>& arrayParcels);	

		bool SplitBlockFiles(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);

		bool SplitBlock2Parts(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlk);

		bool SplitSbkFile2Parcel_Buffer(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo);

		bool SplitBlockFilesNext(BlockSetFileInfo& procBlockSet,std::vector<BlockFileInfo>& arrayBlocks);

		bool SplitBlock2PartsNext(const CHdBox3df& blockBox, const string strBlockFile, std::vector<subBlockFileInfo>& subBlk); 

		bool SplitSbkFile2Parcel_BufferNext(const CHdBox3df& blockBox, const string strSbkFile, std::vector<ParcelFileInfo>& vecParcelInfo);

		void writeLevel0Data(CHdLevel* pLevel0);      		//! 将第0层数据写入到文件

		void writeLevel0DataNext(CHdLevel* pLevel0);    

		U64  writeNextLevelData(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* curLevel); //! 根据上一层数据，将其下一层数据写入到文件,返回当前层的点数
		
		U64  writeNextLevelDataNextZone(CHdLevel* pPreLevel, U8 nLevelNo, CHdLevel* curLevel);
		                                                                                                   
	private:
		CHLZWriter*   m_pHlzWrite;      //当前hlz文件写入对象
		CHLZWriter*   m_pNextHlzWrite;  //分区hlz文件写入对象
		CHdCoreData*  m_pCoreData;       //当前hlz文件数据管理对象
		CHdCoreData*  m_pCoreDataNext;   //分区hlz文件数据管理对象

		CHdBox3dd     m_fullExtent;       //当前点云空间范围
		CHdBox3dd     m_fullExtentNext;   //分区点云空间范围
		F64           m_blockSetStartX;   //切割块集起始位置X坐标
		F64           m_blockSetStartY;   //切割块集起始位置Y坐标
		F64           m_blockSetStartXNext; //跨带分割块集起始位置X坐标
		F64           m_blockSetStartYNext; //跨带分割块集起始位置Y坐标
		F64           m_zoneStartX;       //投影带起始位置X坐标
		F64           m_zoneStartY;       //投影带起始位置Y坐标
		F64           m_zoneEndX;         //投影带结束位置X坐标
		F64           m_zoneEndY;         //投影带结束位置Y坐标
		F64           m_dCenter;          //中央经线值
		F32           m_stepX;            //网格长度
		F32           m_stepY;            //网格宽度
		F64           m_MinX;
		F64           m_MinY;
		F64           m_MinZ;
		F64           m_MaxX;
		F64           m_MaxY;
		F64           m_MaxZ;
		U16			  m_iMin;             //强度最小值
		U16			  m_iMax;             //强度最大值

		U64           m_pointNum;         //hlz头文件中点数目
		U64           m_pointNumNext;     //分区hlz头文件中点数目

		//! 标记转换是否对0级格网均匀抽稀，若抽稀，精度设置为0层精度m，对应子层精度m*2
		bool          m_bResampleZero;

		//! 对格网均匀抽稀精度设置，若0层重采样抽稀，则该精度值为0层精度，否则，为1层精度
		double        m_dGridPrecision;


		CoordStatInfo  m_coordStats;      //当前hlz文件中点坐标分布统计对象
		CoordStatInfo  m_coordStatsNext;  //分区hlz文件中点坐标分布统计对象

		map<U32, BlockSetFileInfo>    m_BlocksetFiles;  //当前hlz文件块集划分结果（块集id=>块集缓存信息）
		map<U32, BlockSetFileInfo>    m_BlocksetFilesNext; //分区hlz文件块集划分结果(块集id=>块集缓存信息)
		map<string, vector<PointXYZIPRGBA *> > m_pBlockData;   //块缓存路径 => 块数据 
		map<string, vector<PointXYZIPRGBA *> > m_ParcelData;   //包缓存路径 => 包数据
        vector<PointXYZIPRGBA>   m_blsDataBuf;   	          //块集数据临时存储容器


		std::string m_savePath;    //保存路径

		void (*processCallback)(float, const char*);  //进度回调函数
	};
}

