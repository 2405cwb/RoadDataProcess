#ifndef _HN_DATA_MAMAGER_H
#define _HN_DATA_MAMAGER_H
#include "hnapplication_global.h"
#include "..\hdPointCloud\SeaPointCloud.h"
#include <QString>
#include <vector>
#include <map>
#include "..\hnCommon\hnRoadStruct.h"
#include "..\hnCommon\hnRoadTypeDef.h"
#include "HnProjectEnums.h"
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "..\hd3DScene\HdViewScrBuffer.h"
#include <QProgressDialog>
#include "../hnConvert/hnDataCombineStructInfo.h"
using namespace hd;
using namespace hnCommon;
using namespace std;
class HnXRSettings;
namespace hnPro
{
	class hnProjectManager;
	class hnProject;
}

namespace hnApp
{
	
	class HNAPPLICATION_EXPORT hnDataManager
	{
	private:
		hnDataManager();
		~hnDataManager();

		// 定义一个单实例销毁辅助类
		class hdDataManagerCleaner
		{
		public:
			hdDataManagerCleaner() {}
			virtual ~hdDataManagerCleaner()
			{
				if (m_pDataMagager != NULL)
				{
					delete m_pDataMagager;
					m_pDataMagager = NULL;
				}
			}
		};

	public:

		// 根据传入路径获取所有工程
		bool getAllProject(QString strFolder,vector<hnProjectDataInfo>& vecProData, PROJECT_TYPE&nWorkType);

		// 打开工程
		bool initProject(vector<hnProjectDataInfo>& vecProData, QProgressDialog& progress);
		
		//设置当前工程
		bool setCurrentProject(const QString& proName);

		hnPro::hnProject* getCurrentProject();

		// 关闭所有工程
		void closeProject();

		//关闭当前工程
		void closeCurrentProject();

		// 工程是否打开
		bool isOpenProject();

		//内存中是否有工程
		bool isHasProject();
	 
	public:  
		// 获取道路等级
		QVector<QString> getRoadLevel(HnProjectEnums::StandardParmTypeEnum strStandard);

		// 根据 道路标准  获得 道路材质
		QVector<QString> getRoadSurfaceTypes(HnProjectEnums::StandardParmTypeEnum strStandard);

		//根据道路标准获得绘制模式
		QVector<QString> getDrawTypes(HnProjectEnums::StandardParmTypeEnum strStandard);
		  
 


		//设置每个工程病害表名称
		void setProjectDiseaseVector(std::vector<hnProjectDataInfo>& vecProData);

		QVector<hnDiseaseSetInfo> getRoadDisease(const HnProjectEnums::StandardParmTypeEnum& standard, ROAD_WORK_TYPE nDrawType, ROAD_SURFACE_TYPE nRoadSurfaceType,int diseaseType);
		
		//获取当前工程病害名字列表 
		QStringList getCurrentProjectRoadDiseaseNames();

		//根据传入工程  获得该工程对应的 工程病害列表   standard与roadSurfaceType填写none则自动使用传入工程参数   cwb
		QVector<hnDiseaseSetInfo> getProjectRoadDiseaseNames(hnPro::hnProject* project,HnProjectEnums::StandardParmTypeEnum standard,int roadSurfaceType =-1);

		//获得当前路面病害列表    
		QVector<hnDiseaseSetInfo> getCurrentProjectRoadDiseases(const hnMile& mile); //cwb

	 

	   //获得当前景观病害列表  type : 1-沿线设施 2-路基损坏
		QVector<hnDiseaseSetInfo> getCurrentProjectStreetDiseases(const hnMile& mile, int type); //cwb、
		QVector<hnDiseaseSetInfo> getCurrentProjectStreetDiseases(HnProjectEnums::StandardParmTypeEnum standard, int type); //cwb、

		hnPro::hnProjectManager* getProjectManager() { return m_pProjectManager; }

		void setDiseaseCalcuteSize(hnRoadDiseaseInfo& dise);
		
		// 获取道路类型参数 cwb
		bool getRoadTypeSetInfo(HnProjectEnums::StandardParmTypeEnum strRoadStandard, QString strRoadLevel, ROAD_SURFACE_TYPE nRoadSurfaceType, hnRoadTypeSetInfo& roadTypeSetInfo);
		bool getRoadTypeSetInfo(const hnProjectSetInfo& settingInfo, hnRoadTypeSetInfo& roadTypeSetInfo);
	public:

		// 获取实例
		static hnDataManager* getDataManager();

		// 销毁实例
		static void destoryDataManager();

		// 初始化道路规范参数
		bool initRoadStandardInfo();

		// 设置当前道路类型
		bool setCurRoadTypeName(HnProjectEnums::StandardParmTypeEnum strRoadTypeNam);

		// 根据指定道路类型的病害表集合信息
		bool getTableNameFromRoadType(HnProjectEnums::StandardParmTypeEnum strRoadType, vector<QString>& vecRoadDiseaseTableName);

		// 获取道路类型
		bool getRoadTypeName(vector<HnProjectEnums::StandardParmTypeEnum>& vecRoadTypeName);

		//获取道路类型名称
		QStringList getRoadStandardNames();

		// 根据病害类型名称获取路面病害设置参数
		bool getDiseaseSetInfo(QString strDiseaseTypeName, HnProjectEnums::StandardParmTypeEnum strRoadStandard, ROAD_WORK_TYPE nDrawType, ROAD_SURFACE_TYPE nRoadSurfaceType,
			hnDiseaseSetInfo& diseaseSetInfo);

		bool getStreetDiseaseSetInfo(QString strDiseaseTypeName, HnProjectEnums::StandardParmTypeEnum strRoadStandard, hnDiseaseSetInfo& diseaseSetInfo);

		// 获取当前道路类型参数  还会将当前路面标准设置为传入值注意
		bool getCurrentRoadTypeSetInfo(HnProjectEnums::StandardParmTypeEnum strRoadStandard, QString strRoadLevel, ROAD_SURFACE_TYPE nRoadSurfaceType, hnRoadTypeSetInfo& roadTypeSetInfo);

		// 获取材质类型
		vector<QString> getRoadSurfaceType();

		//获取某个病害等级的所有表名
		QVector<QString> getTableNames(HnProjectEnums::StandardParmTypeEnum levelType);

		//获取病害表名对应的病害名 前面是病害表名，后面的病害的名字
		QMap<QString, QString> getTableNamesDiseaseNamesMap(const HnProjectEnums::StandardParmTypeEnum levelType,int drawType);

		//获取病害表名对应的病害名 前面是病害表名，后面的病害的完整信息
		QMap<QString, std::vector<hnDiseaseSetInfo>> getTableNamesDiseaseInfoMap(const HnProjectEnums::StandardParmTypeEnum levelType, int drawType);

		

		//传入路面材质字符串反馈枚举
		ROAD_SURFACE_TYPE  getRoadSurfaceFromStr(QString surface);

		//当前工程路面标准下 获取病害名称对应的表名
		QString getTableName(const QString &diseaseName);

		map<HnProjectEnums::StandardParmTypeEnum, vector<hnRoadTypeSetInfo>> getRoadSetInfo() {  return m_mapRoadTypeSetInfo;}

	public:
		// 根据左上、左下以及右上三个图像像素坐标获取病害深度信息
		bool getDiseaseDepth(hn3dPointWithMileI ptLU, hn3dPointWithMileI ptLD, hn3dPointWithMileI ptRU, double& dDepth);

		//获得三维点坐标
		bool getDisease3DPoint(hn3dPointWithMileI pt, hn3dPointD& pt3D);

		// 获取经纬度
		bool getDiseaseLoction(hn3dPointWithMileI pt, double& dLatitude, double& dLongtitude, double& dHeight);

		bool getDiseaseLoction(hn3dPointWithMileI pt, double& dLatitude, double& dLongtitude, double& dHeight, hnPro::hnProject* project);
	private:
		// 查找类型
		bool findData(const vector<QString>& vecOriData, QString strFindData);

		// 查找所有类型文件
		bool findFile(QString strFolder, QString strFileName, vector<QString>& vecRetFile);

		// 解析工程配置文件
		bool analysisXml(QString strXmlPath, vector<hnProjectDataInfo>& vecProData);

		std::vector<std::string> subString(const std::string&s,char delimiter);

		// 解析工程配置文件
		bool analysisProjectInfo(QString strIni, hnProjectDataInfo& outProData);


		//解析 裁剪文件 CamSetting.ini
		bool analysisCamSetting(QString camPath,double& width);

		// 解析时间桩号
		bool analysisMilePile(string strMilePile, long long& nEncl,  double& dTrueMileage, double& dGpsTimer);

		//计算病害尺寸
		/*static */void calcuteDiseaseSize(int drawType,const hnDiseaseSetInfo& setInfo,hnRoadDiseaseInfo& dis);

		//1.读POS得到经纬度// 加载POS数据至内存
		bool loadPosData(const char* strPosPath, std::vector<hnPosInfo>& vecInfo);

		//公路等级排序
		static bool compareRoadLevels(const QString&a, const QString &b) 
		{
			static const QMap<QString, int> roadRank =

			{
				{ QStringLiteral("快速路"),1 },
				{ QStringLiteral("主干路"),2 },
				{ QStringLiteral("次干路"),3 },
				{ QStringLiteral("支路"), 4},
				{ QStringLiteral("高速公路"),5 },
				{ QStringLiteral("一级公路"),6 },
				{ QStringLiteral("二级公路"),7 },
				{ QStringLiteral("三级公路"),8 },
				{ QStringLiteral("四级公路"),9 },
				{ QStringLiteral("五级公路"),10 },
				{ QStringLiteral("六级公路"),11 },
			    { QStringLiteral("等外公路"),12 }
			};

			bool aIsRoad = roadRank.contains(a);
			bool bIsRoad = roadRank.contains(b);

			if (!aIsRoad&&bIsRoad)
			{
				return true;
			}
			if (aIsRoad&&!bIsRoad)
			{
				return false;
			}
			if (!aIsRoad && !bIsRoad)
			{
				return a < b;
			}
			return roadRank[a] < roadRank[b];
			 
		}
	private:
		static hnDataManager* m_pDataMagager;
		
	private:
		// 当前病害设置参数  模块名称，病害数据
	static	map<HnProjectEnums::StandardParmTypeEnum, vector<hnDiseaseSetInfo>> m_mapDiseaseSetInfo;

		// 道路类型参数
	static	map<HnProjectEnums::StandardParmTypeEnum,vector<hnRoadTypeSetInfo>> m_mapRoadTypeSetInfo;

		// 所有道路类型
	static	vector<HnProjectEnums::StandardParmTypeEnum> m_vecRoadStandard;

		// 工程管理器
		hnPro::hnProjectManager* m_pProjectManager;

		// 当前工程
		hnPro::hnProject* m_pCurProject;

		// 当前参数
		HnProjectEnums::StandardParmTypeEnum m_strCurRoadTypeName;
		vector<hnRoadTypeSetInfo> m_curRoadTypeSetInfo;
		vector<hnDiseaseSetInfo> m_curDiseaseSetInfo;

		// 当前道路类型下路面材质类型
		vector<QString> m_vecRoadSurfaceType;

		// 当前道路下道路等级
		vector<QString> m_vecRoadLevel;

		// 工程打开标记
		bool m_bOpenProject;

		// 存在工程标记
		bool m_bHasProject;

		//单例  全局设置
	 	HnXRSettings* m_xrSetting;

		// 上一个工程名称
		QString m_strPreProName;

		// 读取POS数据
		vector<hnPosInfo> m_vecPosInfo;

	public:
		// 当前工程点云数据
		vector<CSeaPointCloud*> m_vecPtCloud;

		// 自动化模式合并距离阈值
		int hMergeLittleFrameThr;
		int vMergeLittleFrameThr;
	};
}

#endif

