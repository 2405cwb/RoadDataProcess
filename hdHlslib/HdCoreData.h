/*! HdCoreData.h
********************************************************************************
<PRE>
模块名       : hdHLSLib
文件名       : HdCoreData.h
相关文件     : 
文件实现功能 : hlz点云管理类
作者         : 龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              创建
2015/01/26   1.0      龚书林    
</PRE>
*******************************************************************************/

#pragma once
#include <map>
#include <vector>
#include "inc\mydefs.hpp"
#include "HlzDefs.h"
#include "..\hdCore\hdBox3d.h"
#include "IHLSReader.h"
#include "..\hdCommon\point_types2.h"
using namespace std;

namespace hd
{
	// 前置声明结构体,避免包含过多文件头
	struct HlzPoint;
	struct HdPointXYZ;
	struct HdRefPoint;
	struct PointXYZIPRGBA;
	// 前置声明类
	class CHdLevel;
	class CHdBlockset;
	class CHdBlock;
	class ChdParcel;
	class CHdParcelBase;

	//Parcel列表类型
	typedef vector<CHdParcelBase*> CHdListAreaNoInf;

class HLS_API CHdCoreData:public IHLSReader
{
private:
	
    // windows文件句柄
	HANDLE					m_pDataFile;
	
    // 内存映射句柄
	HANDLE					m_fileMap;
	
    // 数据文件编号
	U16						m_curFileIndex;
	
    // 内存映射数据区起始位置
	char*					m_mapAddress;
	
     // 数据所在路径
	char  m_dir[256];

    // 数据文件名称,不包含扩展名
	char  m_name[256];
	
	// 数据文件扩展名
	char  m_datExt[256];
    // 文件标识,0NTFS,1FAT32,2FAT
	U8  m_fileFlag;	
	
    // 代码锁,支持多线程
	CRITICAL_SECTION		m_cs;
	
	// 是否使用内存映射
	bool m_bUseMemMap;
    // 层列表
	map<U16,CHdLevel*>      m_lstLevel; 

    // 当前层号
	I16						m_currentLevel;

	// 是否打开文件
	BOOL                    m_isOpen;
	
	// 记录索引文件完整路径
	string m_strIndexFilePath;

	bool                    m_bSplitHld;  //hld文件是否分文件存储   袁亮   20160823

	bool                    m_bNewHlz; //是否为hlz 3.2格式文件（3.1格式被污染，视为3.0格式）
   // 私有成员函数
private:
	// 打开文件映射
	BOOL OpenFile(const char* filePath);
	//! 关闭内存映射
	void CloseMapdata();
	//! 根据7参数计算矩阵
	void ComputeMatrix();
	//! 从内存流中解析hlz文件空间索引
	bool ParseIndex(char* buf);
	//! 从内存流中解析hlz 3.0文件空间索引
	bool ParseOldIndex(char* buf);
	//! 从内存流中解析hlz 3.2文件空间索引
	bool ParseNewIndex(char* buf);
	//! 更新hlz3.0空间索引文件
	BOOL UpdateOldIndex();
	//! 更新hlz3.2空间索引文件
	BOOL UpdateNewIndex();

public:
	CHdCoreData(void);
	virtual ~CHdCoreData(void);

	// 文件头记录
	/*HLZheader				m_hlzHeader;*/

	// 加载数据库
	virtual BOOL Open(const char* strFileName);
	
    // 关闭对象
	virtual BOOL Close();
	
    // 关闭文件及映射
	void CloseFile();
	
    // 获取索引记录
	map<U16,CHdLevel*>& GetListLevel(){return m_lstLevel;}

	// 更新比例尺
	void UpdateScale();
		
    // 获取相交的块
	void GetIntersectBlock();

	// 添加层记录
	BOOL AddLevelRec(CHdLevel* pLevelRec);

    // 获取层记录
	CHdLevel* GetLevelRec(U16 nLevelNo);
	
    // 添加BlockSet记录
	BOOL AddBlockSetRec(CHdBlockset* pBlockSetRec);
	
    // 获取BlockSet记录
	CHdBlockset* GetBlockSetRec(U16 nLevelNo, U32 nBlockSetNo);

	// 根据比例尺设置当前层,dScale比例尺分母
	void SetCurrentLevelByScale(F64 dScale);

	// 根据层号设置当前层
	void SetCurrentLevelByNum(int num);

	// 根据点数判断设置当前层
	void SetCurrentLevelByPtNum(const int ptNum,const CHdBox3df& rect);

	// 获取当前显示层记录
	CHdLevel* GetCurrentLevelRec();

	// 获取当前显示层编号
	I16 GetCurrentLevelNo();

	// 改变当前层Level
	BOOL ChangeCurrentLevel(short nLevelNo, BOOL bChangeScale);

	// 切换到上一层Level
	BOOL ChangeOneUpLevel(BOOL bChangeScale);

	// 切换到下一层Level
	BOOL ChangeOneDownLevel(BOOL bChangeScale);

	// 根据层号、块集、块、包编号，获取范围
	CHdBox3df CalcParcelRect(short nLevelNo,short nBlockSetNo,short nBlockNo,short nParcelNo);

	// 获取指定位置Parcel
	BOOL GetIntersectArea(short nLevelNo,
		F32 dPubX,
		F32 dPubY,
		F32 dPubZ,
		unsigned short& nBlockSetNo,
		unsigned short& nBlockNo,
		unsigned short& nParcelNo);

	// 获取指定范围Parcel范围列表
	int GetIntersectAreas(CHdLevel* pLevelRec,
		const CHdBox3df& rect,
		CHdListAreaNoInf* pLstAreaNoInf);
    
    // 获取指定层的Parcel范围列表
    int GetLevelAreas(CHdLevel* pLevelRec, CHdListAreaNoInf* pListAreaNoinf);

	// 获取指定层所有Parcel范围列表，包括无点块集等
	int GetLevelAllAreas(CHdLevel* pLevelRec, CHdListAreaNoInf* pListAreaNoinf);
	
	// 获取指定层的BlockSet列表
	int GetLevelBSAreas(short nLevelNo, CHdListAreaNoInf* pListAreaNoinf);

	// 获取指定层的一定范围的BlockSet列表
	int GetLevelBSAreas(short nLevelNo, const CHdBox3df& rect,CHdListAreaNoInf* pListAreaNoinf);

	// 获得指定块集的所有包含数据的子块包
	int GetBsAreas(CHdBlockset* poBlockSet,CHdListAreaNoInf* pListAreaNoinf);

    // 获取当前层指定范围的Parcel范围列表
    int GetCurLevelAreas(const CHdBox3df& rect,CHdListAreaNoInf* pLstAreaNoInf);

	// 根据视点到包得范围距离来判断Parcel范围列表
	int GetCurLevelAreasByEye(const CHdBox3df& rect,const F32 eye[3],CHdListAreaNoInf* pLstAreaNoInf);

	// 获取指定层,指定范围下的包列表
	int GetIntersectAreas(short nLevelNo,
		const CHdBox3df& rect,
		CHdListAreaNoInf* pLstAreaNoInf );

	// 卸载范围外点云
	int UnloadOutOfExtent(const CHdBox3df& rect);

	// 卸载当前层数据
	int UnLoadCurLevlData();

	// 获取指定数据包
	BOOL LoadSpecParcel(CHdParcelBase* noInfo);
	// 获取指定块数据
	BOOL LoadSpecBlock(CHdBlock* poBlock);
	// 获取指定块集数据
	BOOL LoadSpecBlockSet(CHdBlockset* poBlockSet);
	// 获取指定数据包
	//获取数据包的颜色信息   袁亮   20160625
	BOOL LoadSpecParcelData(CHdParcelBase* noInfo,HdPointXYZ** pPtBlock,U8** pPIntensity, HdPtColor** ppPtColor);

    // 获取指定数据包
    BOOL LoadSpecParcelInfo(
        CHdParcelBase* noInfo,          // 输入, 块数据
        HdPointXYZ** pPointArray,        // 输出, 点坐标数据
        HdIntensity** pIntensityArray,   // 输出, 强度数据
        HdPtColor** pColorArray,         // 输出, 颜色数据
        HdPtClass** pClassArray,         // 输出, 分类数据
        U32 ptNum);                     // 输出, 点个数

	// 卸载指定包数据
	BOOL UnLoadSpecParcel(CHdParcelBase* noInfo);

	// 判断是否已经打开文件
	BOOL IsOpen(){ return m_isOpen;}

	//! 获取当前点全局坐标
	//inline void GetCoordinate(double& x,double& y,double& z);

	// 读取一个点
	inline virtual BOOL Read_Point(PointXYZIPRGBA& pt,U64 index);

	// 读取一圈,将数据拷贝到目标数组
	inline virtual BOOL ReadLoop(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,								// 圈号
		U32 simple = 1);					// 抽稀加载间隔		

	// 读取一圈,将数据拷贝到目标数组,包含无效点
	inline virtual BOOL ReadLoopFull(
		hdVector<PointXYZIPRGBA>& ptBuf,		// 外部传入的数组,外部管理指针	
		I32 loop,								// 圈号
		U32 simple = 1);					// 抽稀加载间隔			
	
	// 更新一圈数据,到数据文件
	inline virtual BOOL UpdateLoop(
		const hdVector<PointXYZIPRGBA>& ptBuf,	// 需要更新的列
		I32 loop);	

	//! 写入块集、块、包数据，用于mongo从点云接口直接调用
	BOOL WriteData(
		const HdPointXYZ* ptBuf,	// 待写入的坐标数据
		const U8* intenBuf,			// 待写入的强度数据
		const HdPtColor* ptColorBuf, //支持同步写入颜色数据信息   袁亮  20160625
		U64 count,					// 点数
		U64& addrCoord,				// 返回的坐标地址
		U64& addrIntensity,		    // 返回的强度地址
	    U64& addrColor);            // 返回的颜色地址    袁亮  20160625

	// 更新写入索引文件
	BOOL UpdateIndex();

	//! 更新写入文件头
	void WriteHeader();

    // 根据头文件解析ID
    bool ResolveParcelID(CHdParcelBase* pParcelBase, int& nZone, int& nLevelID, int& nBlockSetID, int& nBlockID, int& nParcelID, string& strParcelID);
};

	// 物理文件数据块到内存点云转换
	HLS_API int PtBlock2PtArray(
		const HdPointXYZ* pPtBlock,		// 输入,物理块数据
		int count,						// 输入,点个数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HlzPoint** ppOut,				// 输出，解析后坐标
		U8* pIntensity = NULL,			// 输入,可选
	    const HdPtColor* pPtColor = NULL); //可选输入，颜色信息   袁亮   20160625

	// 物理文件数据块到内存点云转换
	HLS_API int PtBlock2PtArray(
		const HdPointXYZ* pPtBlock,		// 输入,物理块数据
		int count,						// 输入,点个数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HlzPoint** ppOut,				// 输出，解析后坐标
		F32 offSetX,
		F32 offSetY,
		U8* pIntensity = NULL,			// 输入,可选
	    const HdPtColor* pPtColor = NULL); //可选输入，颜色信息   袁亮   20160625

	/*!
	* @brief 物理文件数据块到内存点云转换, 用于hlz3.1格式点坐标解析
	* @param[in] point_block 输入,物理块数据
	* @param[in] count 输入,点个数
	* @param[in] ref_point 输入,块左下角参考点
	* @param[out] point_out 输出，解析后坐标
	* @param[in] intensity 输入,强度数据
	* @param[in] point_color 可选输入，颜色信息
	* @param[in] offSet_x X方向上偏移量
	* @param[in] offSet_y Y方向上偏移量
	* @result 转换失败返回0，成功则返回1
	*/
	HLS_API int PtBlock2PtArrayWithHeader(const HdPointXYZ* point_block,	int count,	const HdRefPointd* ref_point, HlzPoint** point_out,	 
		u8* intensity = NULL, const HdPtColor* point_color = NULL, f64 header_offset_x = 0.0,f64 header_offset_y = 0.0,f64 header_offset_z = 0.0); 

	// 内存点云到物理文件数据块
	int PtArray2PtBlock(
		const PointXYZIPRGBA* pPtAry,	// 输入,点数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标
		U8**		 pPIntensity,		// 输出,物理块数据强度
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		U16 minIntensity = 0,			// 最小反射强度
		U16 maxIntensity = 0);			// 最大反射强度

	// 内存点云到物理文件数据块，针对云存储计算包围盒偏移量
	int PtArray2PtBlock(
		const PointXYZIPRGBA* pPtAry,	// 输入,点数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标
		U8**		 pPIntensity,		// 输出,物理块数据强度
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		F64 offsetX = 0.0,              // 块集整体偏移量
		F64 offsetY = 0.0,		
		U16 minIntensity = 0,			// 最小反射强度
		U16 maxIntensity = 0);			// 最大反射强度   

	// 内存点云到物理文件数据块内存点云到物理文件数据块，针对云存储计算包围盒偏移量
	int pPtArray2PtBlock(
		PointXYZIPRGBA* pPtPAry[],	    // 输入,指向点的指针的数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标
		U8**		 pPIntensity,		// 输出,物理块数据强度
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		U16 minIntensity = 0,			// 最小反射强度
		U16 maxIntensity = 0);			// 最大反射强度

	// 内存点云到物理文件数据块，针对云存储计算包围盒偏移量-xyg
	int pPtArray2PtBlock(
		PointXYZIPRGBA* pPtPAry[],	    // 输入,指向点的指针的数组
		int count,						// 输入,点数
		const HdRefPoint* pRefPt,		// 输入,块左下角参考点
		HdPointXYZ** pPtBlock,			// 输出,物理块数据坐标
		U8**		 pPIntensity,		// 输出,物理块数据强度
		HdPtColor**  pPtColor,          // 输出，物理块数据颜色，格式为R5G6B5，外部使用完毕后要释放   袁亮   20160625
		F64 offsetX = 0.0,              // 块整体偏移量
		F64 offsetY = 0.0,
		U16 minIntensity = 0,           // 最小反射强度
		U16 maxIntensity = 0) ;	        // 最大反射强度	

	// 根据带号、层号和parcelData包围盒获取对应坐标转换绝对坐标偏移量
	int GetParcelDataOffsetXY(
		U8 ZoneID,                      // 投影区域带号
		U16 LevelNO,                    // 层号
		F64 boxMinX,F64 boxMinY,        // ParcelData包围盒minEdge 
		F64& offsetX,                   // X偏移量
		F64& offsetY);                  // Y偏移量

	// 不使用硬编码，获取更大的灵活性   袁亮  20161105
	// 根据带号、层号和bolckset包围盒获取对应BlockSetNO
	int GetBlockSetNO(
		F32 stepX,                       //块集网格尺寸
		F32 stepY,                       //块集网格尺寸
		CHdBox3df& fullBox,             // 点云包围盒
		CHdBox3df& blockSetBox,         // 块集包围盒
		U32& blocksetNO);               // 块集编号

	// 根据带号、层号和bolck包围盒获取对应块编号BlockNO
	int GetBlockNO(
		CHdBox3df& blockSetBox,         // 点云包围盒
		CHdBox3df& blockBox,            // 块包围盒
		U16& blockNO);                  // 块编号
}