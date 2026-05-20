#ifndef _HN_PROJECT_H_
#define _HN_PROJECT_H_
#include "hnproject_global.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "..\hnCommon\hnRoadTypeDef.h"
#include "..\hnDataTable\hnDBSqlite.h"
#include "hnMile.h"
#include <QVector>
#include <QString>
#include "..\hnCommon\hnRoadTypeDef.h"
#include "..\hnConfigService\HnXRSettings.h"
using namespace hnCommon;
//using namespace hnDataTable;

namespace hnPro
{
	// 前置声明
	class hn2DProject;
	class hn3DProject;

	class HNPROJECT_EXPORT hnProject
	{
	public:
		hnProject();
		~hnProject();

		// 打开工程
		bool openProject(hnProjectDataInfo& curProDataInfo);

		
		
		// 关闭工程
		void closeProject();

		// 设置当前工程
		void setCurProject();

	public:
		// 获取当前db
		hnDBSqlite* getCurDB() { return m_pDbSqlite; }

		// 获取工程类型
		PROJECT_TYPE getProjectType(){ return m_nProjectType; }

		// 获取工程名称
		QString getProjectName() { return m_strProjectName; }

		//获取工程文件夹路径
		QString getAbsulotelyPath();

		// 获取二维工程名称
		QString get2DProName() { return m_str2DProName; }

		// 获取二维工程路径
		QString get2DProPath() { return m_str2DProPath; }

		// 获取三维工程名称
		QString get3DProName() { return m_str3DProName; }

		// 获取三维工程路径
		QString get3DProPath() { return m_str3DProPath; }

		// 获取db
		hnDBSqlite* getDB() { return m_pDbSqlite; }

		// 获取工程配置信息
	hnProjectSetInfo  getCurProSetInfo()  { return m_projectInfo; }

		//根据桩号获得路面图像路径
		QString getRoadPicturePath(double mile);

		QString getStreetPicturePath(double mile);

		ROAD_WORK_TYPE getBaseDrawType();

		//获得基础道路标准 
		HnProjectEnums::StandardParmTypeEnum  getBaseStandard();

		hnCommon::ROAD_SURFACE_TYPE getBaseSurface();

	    // 获取二维工程
		hn2DProject* get2DProject() { return m_p2DProject; }

		// 获取三维工程
		hn3DProject* get3DProject() { return m_p3DProject; }

		// 初始化三三维编码器里程差
		void init2d3dMileDiff();

		// 三三维编码器里程差，是二维图片起点里程 - 三维里程起点里程的差值 单位米
		double get2d3dMileDiff();

		//设置二三维编码器里程差 是二维图片起点里程 - 三维里程起点里程的差值 单位米
		void set2d3dMileDiff(const double diff);

	private:

		bool getOrCreateResultDb(QString projectPath, hnProjectDataInfo& curProDataInfo,QString& dbDirPath, QString & dbFilePath);

		// 三三维编码器里程差，是二维图片起点里程 - 三维里程起点里程的差值 单位米
		double m_2d3dDiff;

	private:
		void checkMileDiffFile(const QString &fileName);
	public:
		// 相对里程转绝对里程
		double enclToTrueMile(double dEnclMile);

		// 绝对里程转相对里程
		double trueMileToEncl(double dTrueMile);

		// 写入工程配置信息
		bool writeProjectSettingInfo(const hnProjectDataInfo& curProDataInfo);

		//获取结果数据库位置
		QString getDbResultFilePath() {
			return m_strDbFilePath
				;
		}

		QString getDbResultDirPath()
		{
			return m_strDbDirPath;
		}
	public:

		//初始化 m_vecMile    每次 分析较桩文件  分析打标文件 生成桩号列表   changeMark：是否是用户更新了marks
		void initMileList(bool notNeedUpdateMileVector = true);

		//获得道路图像间隔
		double getRoadSpace();

		//获得景观图像间隔
		double getStreetSpace() { return m_leftStreetSpce; }

	    //获得完整的桩号，图片路径等信息 //在用户点击 setCurProject（）后这里才生成 m_vecMile;
		QVector<hnMile> getCurrentMileVector();

		//根据传入桩号返回最近的HnMile
		hnMile getCloseMile(double targetMile);
		
		//根据传入的里程返回最近的HnMile
		hnMile getCloseMileFromDmi(double dmi);

		//根据传入的里程返回景观的HnMile
		hnMile getCloseStreetMileFromDim(double dmi);

		//获取左边景观病害的桩号
		QVector<hnMile> getLeftStreetMiles();

		//获取右边景观病害的桩号
		QVector<hnMile> getRightStreetMiles();


		//始终返回根据编码器里程从小到大排序的达标列表
		QVector<hnCommon::hnMarkInfo> getCurrentMarkVector();

		QVector<hnCommon::hnMilePile> getCurrentMilePileVector();



		//添加病害
		bool AddDisease();

		hnMile getCurrentRoadMile() { return m_currentMile; }

		double getCurrent3DRoadDmi(){return m_current3dDmi; }

		//设置当前路面hnMile
		void setCurrentRoadMile(const hnMile & mile);

		//设置单三维模式下的 当前路面桩号   陈智超备注：参数编码器里程
		void setCurrent3dRoadDmi(const double& dmi);

		//用户修改打标列表   写入打标数据  重新加载HnMile  
		//返回值 false表示不需要刷新界面（打标为材质切换等需要进行材质切换）
		bool changeMark(QVector<hnCommon::hnMarkInfo>& marks,const QVector<int>&deleteMarkIndexs);

		//添加打标
		bool addMark( hnCommon::hnMarkInfo& mark);

		//删除打标
		bool deleteMark(const hnCommon::hnMarkInfo& Mark);
		bool deleteMark(int id);

		//用户修改较桩列表 写入较桩数据  重新加载hnmile
		void changeMilePile(QVector <hnCommon::hnMilePile>& piles);
		void addMilePile( hnCommon::hnMilePile& plile);

		 void  deleteMilePile(int id);

		//人工操作  重写打标数据库
		void updataMarkDatabase();

		//人工操作  重写较桩数据库
		void updataMilePileDatabase();

		//手动导出内业修正后的外业文本，不覆盖外业原始文件
		void exportCorrectedFieldTextFiles();
	private: //cwb
		bool saveMarksToResultDb();
		bool saveMileagePilesToResultDb();
		bool saveProjectSettingToResultDb();
		void import2DFieldDataToResultDb();
		bool isRoadAttributeMark(int nType) const;

		bool  read2dSetting(const QString& path );

		//获得文件夹内第一张图片像素值 设置配置参数
		int getPixResoluion(const QString &pixDirName);

		void read2dProjectinfo(const QString& projectinfoPath);
	private:
		// 获取是否存在成果db
		bool getResultDB(const QString dbDirPath, QString& strDB,bool isOld);
		hnMile m_currentMile;
		double m_current3dDmi;
		//更新数据库
		void updatePorjectDb();

		void updatePorjectText();
		//更新  打标  较桩  工程配置   相关的 所有 文本，xml，数据库记录
		void updatePorjectAllSettingSource();
	private:
		bool m_bNeedImportFieldMilePilesToResultDb;
		bool m_bNeedImportFieldMarksToResultDb;


		HnXRSettings* m_xrSetting;
		// 工程根目录
		QString m_strProjectPath;

		// 工程名称
		QString m_strProjectName;

		// 二维数据工程名称
		QString m_str2DProName;

		// 三维数据工程名称
		QString m_str3DProName;

		// 二维工程数据路径
		QString m_str2DProPath;

		// 三维工程数据路径
		QString m_str3DProPath;  

		//结果数据库db文件夹路径
		QString m_strDbDirPath;
		
		//结果数据库db文件路径
		QString m_strDbFilePath;
	private:

		QVector<hnDiseaseSetInfo> currentDiseaseSetInfo;

		QVector<hnMile> currentMileVec;

		QVector<hnMile> currentLeftStreetMileVec;
		QVector<hnMile> currentRightStreetMileVec;

		// 工程配置信息
		hnProjectSetInfo m_projectInfo;
		
		// 较桩里程桩信息
		vector<hnMilePile> m_vecMileagePile;

		//动态生成的里程桩信息
		QVector <hnMile> m_vecMile;

		//工程病害信息
		//QVector<hnRoadDiseaseInfo>m_vecDisData;
		

		// 打标信息
		//内业修改只写成果库；外业 RoadStatuMarkInfo.txt 不自动覆盖。
		vector<hnMarkInfo> m_vecMarkInfo;

		// 工程类型
		PROJECT_TYPE m_nProjectType;

		// 数据库
		hnDBSqlite* m_pDbSqlite;

		// 二维工程
		hn2DProject* m_p2DProject;

		// 三维工程
		hn3DProject* m_p3DProject;

		//道路图像间距
		double  m_roadSpace;
		//左边景观图像间距
		double  m_leftStreetSpce;

		//右边景观图像间距
		double  m_rightStreetSpce;
	};
}

#endif