#ifndef _HN_2D_PROJECT_H_
#define _HN_2D_PROJECT_H_
#include "hnproject_global.h"
#include <QString>
#include <vector>
#include <QDebug>
#include <QStringList>
#include<QMap>
#include "..\hnCommon\hnRoadStruct.h"
#include "..\hnCommon\hnRoadTypeDef.h"
#include "..\hnQtCommon\ExcelGPS.h"
#include "HnProjectEnums.h"
#include <QSettings>
#include "hnProject.h"
using namespace std;
using namespace hnCommon;

class  HnXRSettings;
namespace hnPro
{
	class HNPROJECT_EXPORT hn2DProject
	{
	public:
		hn2DProject();
		~hn2DProject();

		// 初始化2D工程
		bool init(QString strProjectPath, QString strProjectName, hnProjectSetInfo m_projectInfo, PROJECT_TYPE type);

		// 获取是否水平镜像
		bool getIsHMirrored();

		// 设置是否水平镜像
		void setIsHMirrored(bool mirrored);

		// 获取是否垂直镜像
		bool getIsVMirrored();
		
		// 设置是否垂直镜像
		void setIsVMirrored(bool mirrored);

		//根据编码器里程，获取图片底部的编码器里程和y坐标
		void getPixEncoderMile(const double encoderMile, const double yScale, double &pixEncoderMile,int &y);

		// 获取里程桩数据
		void add2dMilePile(vector<hnMilePile>& vecMilePile);

		// 获取打标数据  如果返回值大于1 代表从二维打标文本中找到了新增打标数据
		QVector<hnCommon::hnMarkInfo>& add2dMarkInfo(vector<hnMarkInfo>& vecMarkInfo,hnProject * pro);

		// 获取平整度路径
		QString getIRIPath() { return m_strIRIPath; }

		// 获取车辙路径
		QString getLeftRutPath() { return m_strLeftRutPath; }

		QString getRightRutPath() { return m_strRightRutPath; }

		QString getRutResultPath() { return m_strRutResultPath; }
		// 获取构造深度路径
		QString getMTDPath() { return m_strMTDPath; }

		// 获取跳车路径
		QString getPBIPath() { return m_strPBIPath; }

		QString getBasePath() { return m_strProjectPath; }

		QString getSettingPath() { return m_strSettinginiPath; }

		//获得gps2mile.txt文件路径

		QString getGpsResultFilePath() {return  m_gpsResultFilePath;}

		//获得图片路径列表
		// qstring  图片完整路径  
		QStringList getRoadPicturePath() { return m_vecRoadPicMilePath; }

		 
		// qstring  图片完整路径
		QStringList getLeftStreetPicturePath() { return m_vecRoadLeftStreetPicMilePath; }

		// qstring  图片完整路径
		QStringList getRightStreetPicturePath() { return m_vecRoadRightStreetPicMilePath; }

		//通过key可判断 该工程具有哪些设备类型
		QMap<HnProjectEnums::EquipMentEnum,QString> getEquipmentBasePath() { return m_EquipmentBasePath; }

		//供测试用
		QString  getMilesTextPath() { return m_strMilesTextPath; }

		QStringList addPicturePaths(const QString& basePath0, const QString& basePath1);

		//初始化工程道路信息
		bool initRoadInfo(const QString& basePath);

		//初始化工程设备信息
		void initEquipmentBasePath(const QString& basePath);
		
		//初始化gps信息
		void initGpsInfos();

		//根据传入桩号 找到最近gps定位
		//TODO 根据桩号及坐标返回待更新，需要高精度模块
		_EXCELGPS_ findCloseGpsInfo(double targetMile, int x , int y );

		//根据传入里程 找到最近gps定位
		//TODO 根据里程及坐标返回待更新，需要高精度模块
		_EXCELGPS_ findCloseGpsInfoFromDmi(double targetDmi, int x, int y);

		//获得打标路径
		QString getMarkFilePath() { return m_strMarkInfoPath; }

		//获取打标列表 完整
		QString getFullRoadTypeMarkFilePath() { return m_strFullRoadTypeMarkInfoPath; }

		//获得较桩路径
		QString getMilePilePath() { return m_strMilePilePath; }

		QString getMileStoneCaliInfoFilePath() {
			return m_MileStoneCaliInfoFilePath
				;
		}

		int getRoadPictureWidth() { return m_RoadPictureWidth; }

		int getRoadPictureHeight() { return m_RoadPictureHeight; }

		//获得多工程用户桩号文件路径
		QString getUserMilePath() { return m_strUserMilePath; }
		
		hnCommon::ROAD_MARK_TYPE getMarkType(const QString& strType);

		QString get2dProjectinfoPath() { return m_projectPath; }

		//是否为二三维设备
		bool  getIs23DEquipment() { return is23DEquipment; }

		//获得gps信息列表
		QVector<_EXCELGPS_>  getGpsInfoList();

		double caculateStreetLength(QPoint p1, QPoint p2);
	private:
		//检查镜像配置文件，如果不存在则新建
		void checkMirroredFile(const QString &fileName);

		bool readBinToFloatArray(const QString& filePath, int width,int height,QVector<QVector<float>>&data);

		hnProjectSetInfo m_projectInfo;
	private:
		//单例  全局设置
		HnXRSettings* m_xrSetting;

		//当前二维工程路径
		QString m_strProjectPath;

		// 平整度路径
		QString m_strIRIPath;

		// 车辙路径
		QString m_strLeftRutPath;
		QString m_strRightRutPath;
		//车辙结果
		QString m_strRutResultPath;
		// 构造深度路径
		QString m_strMTDPath;

		// 跳车路径
		QString m_strPBIPath;

		// 里程桩路径
		QString m_strMilePilePath;

		//保存用户较桩文本，给二维软件使用读取
		QString m_MileStoneCaliInfoFilePath;

		// 打标路径
		QString m_strMarkInfoPath;

		//完整路面材质打标列表
		QString m_strFullRoadTypeMarkInfoPath;

		//二维工程 setting.ini 文件路径
		QString m_strSettinginiPath;
		
		//二维配置文件地址 
		QString m_projectPath;

		//路面图片路径列表
		QStringList m_vecRoadPicMilePath;

		//路面图像宽度
		int m_RoadPictureWidth;
		//路面图像宽度
		int m_RoadPictureHeight;

		//左景观图片路径列表
		QStringList m_vecRoadLeftStreetPicMilePath;

		//右景观图片路径列表
		QStringList m_vecRoadRightStreetPicMilePath;

		//普通gps2Mile文件地址
		QString m_gpsResultFilePath;

		//trigger与gps文件地址
		QMap<HnProjectEnums::EquipMentEnum, QString> m_EquipmentBasePath;

		QString m_strMilesTextPath;

		//用户多工程桩号配置文件
		QString m_strUserMilePath;

		bool setPath(const QString& path, QString& setPath, bool mustSet = false);

		bool is23DEquipment = false; // 判断工程是否由二三维外业设备采集
		public: //setting.ini里面的工程配置数据 

		QString _Province;//省

		QString _City;//市

		QString _District;//县

		QString _CityCode = "";//县级行政区划代码

		QString _RoadCode;//路线代码

		QString _RoadName;//路线名

		int _UnitNum = 10000;//单元编号

		QString _RoadGrade;//公路等级

		QString _RoadNum;//车道

		/// <summary>
		/// 路面材质，0--沥青，1--水泥，2-砂石
		/// </summary>
		short _RoadType;

		QString _DataDate;

		QString _DataTime;

		QString _DataPerson;

		QString _DataWeather;

		QVector<_EXCELGPS_>  _GpsInfos;

		//老设备 20220815cwb
		 float _PlusLength;
		 int DAQSampleFrequency = 8000;

		 int _StartMile;//起点桩号
		/// <summary>
		/// 行车方向，1--上行，-1--下行
		/// </summary>
		 int _Direction;

		 int _EndMile;//终点桩号

		 int _EndDmi;//总里程

		 int _DmiMileLen;//里程桩号关联数组个数
		 //double[, ] _DmiMile;//里程桩号关联数组
		 //double[] _D2MScale;//里程/桩号的系数

		 //农村路景观标定文件
		 QVector<QVector<float>>u_jgDatas;
		 QVector<QVector<float>>v_jgDatas;

public:  
		 bool _IsIRIMTD = false;//是否采集了平整度构造深度
		 bool _IsDIRIMTD = false;//是否是双平整度构造深度，true-双，false-单

		  /// <summary>
		  /// 是否采集了中间构造深度，true-是，false-否
		  /// </summary>
		  bool _IsMMTD = false;

		/// <summary>
		/// 是否采集了车辙，true-是，false-否
		/// </summary>
		 bool _IsRut = false;
		/// <summary>
		/// 车辙模块模式，0-2D单车辙模块，1-2D双车辙模块，2-3D车辙模块
		/// </summary>
		int _RutMode = 0;
		/// <summary>
		/// 几何线形工作模式，0-不采集几何线形数据，1-采集几何线形数据
		/// </summary>
		int _GeoAlig = 0;

		bool _IsDStreet = false;//是否是双景观，true-双，false-单
		bool _IsRoad = false;//是否采集了路面
		bool _IsStreet = false;//是否采集了景观
		bool _IsPano = false;//是否采集了全景

		int _RutDis = 50;//车辙出值间距

		int _RoadImgDis = 2;//路面图像采集间距

		int _StreetImgDis = 20;//景观图像采集间距
		int _StreeRightImgDis = 20;//右侧景观图像采集间距


		//辅助给景观图像翻页
		int leftStreetImgIndex = 0;
		int rightStreetImgIndex = 0;

		int _PanoImgDis = 20;//景观图像采集间距

		double _DMIScale = 1.0; //编码器相关系数
	};
}

#endif

