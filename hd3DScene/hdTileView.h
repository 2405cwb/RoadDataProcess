/*! hdQuickView.h
********************************************************************************
<PRE>
模块名       : hd3DScene
文件名       : hdTileView.h
相关文件     : 
文件实现功能 : 实现切片视图封装 
作者         : 程鹏
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
				
*******************************************************************************/

#pragma once
#include <string>
#include <map>
#include <vector>
#include "hdIScanRoute.h"

#include "..\hdCore\hdDefs.h"
#include "..\hdCore\hdTime.h"
#include "..\hdCommon\hdHdiStruct.h"
#include "..\hdCommon\hdParaDTStruct.h"
#include "..\hdCommon\hdAppDTStruct.h"
#include "..\hdCommon\HdOriImageByHdiStruct.h"
#include "..\hdDBOperator\HdTileDBOpr.h"
#include "..\hdCommon\hdDTEnumDef.h"


//#include "sceneData\hdSceneScan.h"

using namespace irr;
using namespace hd;
using namespace hd::scene;

namespace hd
{
	class CHdPanoPicFileData;
	class CHdPanoData;

	namespace scene
	{
		//! 快速视图，相机位置在原点旋转浏览
		class HD3DSCENE_API CHdTileView :
			public ISceneView
		{		
			  private:

        //! 轨迹工程所在目录
        string m_folder;

        //! 轨迹名称
        string m_routeName;

        //! 格式版本
        string m_version;

        //! 设备编号
        string m_deviceNo;

        //! 相机个数
        s32 m_cameraCount;

		//! 相机no，CameraInfo中记载
		s32 m_cameraNo;

		//! camera type 0-5表示全景相机，6表示为单镜头相机
		s32 m_cameraType;

        //! POS个数
        s32 m_posCount;

        //! 扫描仪个数
        s32 m_scanCount;

        //! 扫描圈时间
        map<s32,vector<HD_SCANHDIINFO>>	m_scanPosMap;

        //! 影像地理参考
        vector<HD_HDIINFO>	m_hdiVec;

        //! 影像数据库
        CHdImageSqliteDb*  m_pImageDb;

        //! 相机参数 [2014/06/12 危迟]
        HD_ISCANPANO_PARA m_PanoPara;

        //! 缩略图影像存在判断
        BOOL m_bRegImageExist;

        //! 图片是否为文件形式  [2013/12/16 危迟]
        BOOL m_bFileMode;

        //! 前缀名称,如iScan或pScan
        string m_strPreName;

		public:
			CHdTileView(void);
			virtual ~CHdTileView(void);

			//! 根据图像比例获取球面坐标
			bool isPanoExist( int index,s32 cameraNo );

			bool GetTileImagesByAngle( float fScale,float horiBeg,float horiEnd, float vertiBeg,float vertiEnd,const char* strPanoID,vector<vector<HD_SV_TILEINFO*>>& vecInfo );

		    bool GetLocalZeroTilePano(HD_SV_TILEINFO*& pZeroInfo,const char* strPanoID);

			bool GetLocalPanoTitles( float fScale, float horiBeg, float horiEnd, float vertiBeg, float vertiEnd, const char* strPanoID, vector<vector<HD_SV_TILEINFO*>>& vecInfo );

			int GetCurPanoLevel();

			bool AddTile2Cache( const char* strTitleName,HD_SV_TILEINFO*& pInfo );

			double GetCacheListSize();

			void ClearCache();

			bool CheckListPanoCacheExist( const char* strTitleName,HD_SV_TILEINFO*& pInfoNew );

			void GetCurTileIndexRange( int& nBegRow, int& nEndRow, int& nBegCol, int& nEndCol );

			bool IsCurLevelTilePanoExist( const char* strPanoID ,int PanoLevel );

		    bool IsTileDBValid(const char* strPanoID ,int PanoLevel);

		//	void GetSpherePosByScanPos(const core::vector3df& scanPos, core::vector3df& spherePos);

		//	//! 根据扫描点坐标获取角度
		//	void GetAngleByScanPos(const core::vector3df& scanPos, core::vector2df& angle);

		//protected:
		//	//! 重载设置相机函数，以区别于3D视图
		//	virtual void SetCamera();

		protected:
			CPanoSceneNode* m_pPanoSN;

		};
	}
}
