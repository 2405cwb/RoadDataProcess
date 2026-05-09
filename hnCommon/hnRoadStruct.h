#ifndef _HN_ROAD_STRUCT_
#define _HN_ROAD_STRUCT_
#include "stdafx.h"
#include "hnCommonDef.h"
#include "hn3dRect.h"
#include "hnRect.h"
#include <vector>
#include "../hnCommon/hnRoadTypeDef.h"
using namespace std;

namespace hnCommon
{
	// 里程桩校准数据
	typedef struct _HN_MILEAGE_PILE_
	{
		_HN_MILEAGE_PILE_()
		{
			nID = 0;
			nDMi = 0;
			dEnclMile = 0.0;
			dGpsTimer = 0.0;
			dTrueMile = 0.0;
			memset(strAddFile111, 0, SQL_ADDFILE_LEN);
			memset(strAddFile2, 0, SQL_ADDFILE_LEN);
			memset(strAddFile3, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			memset(strRemark, 0, SQL_ADDFILE_LEN);
		}
		bool operator==(const _HN_MILEAGE_PILE_& other) const
		{
			bool equal = dTrueMile == other.dTrueMile&&
				dEnclMile == other.dEnclMile;
			 
			return equal;
		}
		bool operator<(const _HN_MILEAGE_PILE_& other) const
		{
			return dTrueMile < other.dTrueMile;
		}
		// ID索引
		int nID;

		// 编码脉冲值
		long long nDMi;

		// 编码器里程   编码器值*车轮周长/编码器频率的到的绝对距离
		double dEnclMile;

		// gps时间
		double dGpsTimer;

		// 真实里程  
		double dTrueMile;

		// 道路图像路径
		char strAddFile111[SQL_ADDFILE_LEN];

		// 备用字段2
		char strAddFile2[SQL_ADDFILE_LEN];

		// 备用字段3
		char strAddFile3[SQL_ADDFILE_LEN];

		// 备用字段4
		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5
		char strAddFile5[SQL_ADDFILE_LEN];

		// 备注
		char strRemark[SQL_ADDFILE_LEN];

	}hnMilePile;

	// 打标信息
	typedef struct _HN_MARK_INFO_
	{
		_HN_MARK_INFO_()
		{
			nID = 0;
			nType = 0;
			dEnclMile = 0.0;
			dGpsTimer = 0.0;
			dTrueMile = 0.0;
			memset(strMark, 0, SQL_NAME_LEN);
			memset(strAddFile1, 0, SQL_ADDFILE_LEN);
			memset(strAddFile2, 0, SQL_ADDFILE_LEN);
			memset(strAddFile3, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			memset(strRemark, 0, SQL_ADDFILE_LEN);
		}

		bool operator==(const _HN_MARK_INFO_& other) const
		{
			bool equal = dTrueMile == other.dTrueMile&&
				nType == other.nType &&
				strcmp(strMark, other.strMark)==0 &&
				strcmp(strRemark, other.strRemark)==0;
			return equal;
		}
		bool operator<(const _HN_MARK_INFO_& other) const
		{
			return dTrueMile< other.dTrueMile;
		}

	public:
	/*	bool operator<(const _HN_MARK_INFO_& other)
		{
			return  dTrueMile < other.dTrueMile;
		}*/
		// 索引ID
		int nID;

		// 打标类型:  0-路面材质；1-路面单元；2-路面等级; 3-路面标准；4-路面情况
		//1-路面单元；   4-路面情况  仅仅在出表的时候有作用
		int nType;
		
		// 编码器里程
		double dEnclMile;

		// gps时间
		double dGpsTimer;

		// 真实里程
		double dTrueMile;
        
		// 标注信息--如路面材质：沥青、材质、砂石；    路面单元：单元  ；   路面等级:一级公路，二级公路    等级标准:等级公路2018  低等级农村路
		char strMark[SQL_NAME_LEN];

		// 备用字段1
		char strAddFile1[SQL_ADDFILE_LEN];

		// 备用字段2
		char strAddFile2[SQL_ADDFILE_LEN];

		// 备用字段3
		char strAddFile3[SQL_ADDFILE_LEN];

		// 备用字段4
		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5
		char strAddFile5[SQL_ADDFILE_LEN];

		// 备注
		char strRemark[SQL_ADDFILE_LEN];

	}hnMarkInfo;

	// 工程配置信息
	typedef struct _HN_PROJECT_INFO_
	{
		_HN_PROJECT_INFO_()
		{
			nID = 1;
			nGradIndex = -1;
			memset(strProvince, 0, SQL_NAME_LEN);
			memset(strCity, 0, SQL_NAME_LEN);
			memset(strCounty, 0, SQL_NAME_LEN);
			memset(strRoadName, 0, SQL_NAME_LEN);
			dBegMile = 0.0;
			nDrawType = 0;
			dEndMile = 0.0;
			dBegEnclMile = 0.0;
			dEndEnclMile = 0.0;
			nLineType = 1;
			memset(strDate, 0, SQL_NAME_LEN);
			memset(strTimer, 0, SQL_NAME_LEN);
			memset(strRoadLevel, 0, SQL_NAME_LEN);
			nRSurfaceType = 0;
			memset(strSurveyor, 0, SQL_NAME_LEN);
			memset(strWeather, 0, SQL_NAME_LEN);
			dLength = 0.0;
			nWorkType = 0;
			memset(strModel, 0, SQL_ADDFILE_LEN);
			nImageExtent = 0;
			nFrequency = 0;
			dWheelPerimeter = 0.0;
			dRoadWidth = 3.75;
			dRoadLength = 2;
			picPixelX = 0;
			picPixelY = 0;
			dRadioX = 0;
			dRadioY = 0;
			dStartDmi = 0; 
			dUserBegMile = -1;
			dUserEndMile = -1;
			memset(strRoadStandard, 0, SQL_NAME_LEN);
			memset(strAddFile2, 0, SQL_ADDFILE_LEN);
			memset(strAddFile3, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			memset(strRemark, 0, SQL_ADDFILE_LEN);
		}
		 
		

		// 获取材质
		string getRSurfaceType() const
		{
			string strRSurfaceType = "";

			if (nRSurfaceType == 0)
			{
				strRSurfaceType = "沥青";
			}
			else if (nRSurfaceType == 1)
			{
				strRSurfaceType = "水泥";
			}
			else
			{
				strRSurfaceType = "砂石";
			}

			return strRSurfaceType;
		}

		// 获取行别
		string getLineType()
		{
			string strLineType = "";
			
			if (nLineType ==1)
			{
				strLineType = "上行";
			}
			else
			{
				strLineType = "下行";
			}
			
			return strLineType;
		}

		// 索引编号
		int nID;

		// 省份
		char strProvince[SQL_NAME_LEN];

		// 城市
		char strCity[SQL_NAME_LEN];

		// 区/县
		char strCounty[SQL_NAME_LEN];

		// 道路名称
		char strRoadName[SQL_NAME_LEN];

		// 车道
		char strRoadNO[SQL_NAME_LEN];

		// 道路编号
		char strNumber[SQL_NAME_LEN];

		//绘制模式  0-人工模式 1-自动化模式 2-设计模式
		int  nDrawType;

		//工程起点编码器值 
		double dStartDmi;

		// 工程起点桩号
		double dBegMile;

		// 工程终点桩号
		double dEndMile;

		//多工程出表 用户定义的有效起点
		double dUserBegMile;

		//多工程出表 用户定义的有效终点
		double dUserEndMile;

		// 工程起点编码器里程已经累计的值
		double dBegEnclMile;

		// 工程终点编码器里程（此次总共跑了多少距离） 名称应该为dEndEnclLength更合适
		double dEndEnclMile;

	
		//行车方向：1-上行；-1 下行  
		int nLineType;

		// 日期，格式如XXXX:XX:XX
		char strDate[SQL_NAME_LEN];

		// 时间，格式为XX:XX:XX
		char strTimer[SQL_NAME_LEN];

		// 公路等级
		char strRoadLevel[SQL_NAME_LEN];

		int nGradIndex;

		
		// 道路规范  获得所有数据时候从xml文件获得，打开工程后找到这个数据获得成果数据库
		char strRoadStandard[SQL_NAME_LEN];

		// 路面材质 0 沥青 1水泥  2砂石
		int nRSurfaceType;

		// 检测员
		char strSurveyor[SQL_NAME_LEN];

		// 天气
		char strWeather[SQL_NAME_LEN];

		// 长度
		double dLength;

		// 二三维区分
		int nWorkType;

		// 硬件模块
		char strModel[SQL_ADDFILE_LEN];

		// 影像高度范围
		int nImageExtent;

		// 编码器频率
		int nFrequency;

        // 轮子周长
		double dWheelPerimeter;

		// 每张图片代表的道路宽度  横向的
		double dRoadWidth;

		//道路长度  每张图片代表的路面长度  纵向的
		double dRoadLength;

		//横向像素值
		int picPixelX;

		//纵向像素值
		int picPixelY;

		//x方向比例
		double dRadioX;

		//y方向比例
		double dRadioY;

		// 备用字段2
		char strAddFile2[SQL_ADDFILE_LEN];

		// 备用字段3
		char strAddFile3[SQL_ADDFILE_LEN];

		// 备用字段4
		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5
		char strAddFile5[SQL_ADDFILE_LEN];

		// 备注
		char strRemark[SQL_ADDFILE_LEN];

	}hnProjectSetInfo;

	// 病害信息
	typedef struct _HN_ROAD_DISEASE_INFO_
	{
		_HN_ROAD_DISEASE_INFO_()
		{
			nID = 0;
			dMileage = 0.0;
			nRSurfaceType = 0;
			nDrawType = 0;
			nLevel = 0;
			dLength = 0.0;
			dWidth = 0.0;
			dArea = 0.0;
			dDepth = 0.0;
			nPixelLen = 0;
			nPixelWid = 0;
			dRealLen = 0.0;
			dReaWidth = 0.0;
			nGpsCnt = 0;
			nRectCnt = 0;
			n3dCnt = 0;
			dDmi = 0;
			dReaWidth = 0;
			ndiseaseType = 0;
			dDmiEnd = 0;
			dDmiStart = 0;
			diseaseWeight = 0.0;
			memset(strRemark, 0, SQL_ADDFILE_LEN);
			memset(strDiseaseTableName, 0, SQL_ADDFILE_LEN);
			memset(strRoadStandard, 0, SQL_NAME_LEN);
			memset(strDisName, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			
		}
		bool isValid()
		{
			return strlen(this->strDiseaseTableName) != 0;
		}
	 
		void setLevel(const char* level)
		{
			if (0 == strcmp(level, "无"))
			{
				nLevel = 0;
			}
			else if(0 == strcmp(level, "轻"))
			{
				nLevel = 1;
			}
			else if (0 == strcmp(level, "中"))
			{
				nLevel = 2;
			}
			else if (0 == strcmp(level, "重"))
			{
				nLevel = 3;
			}
			else
			{
				nLevel = 0;
			}
		};

		bool operator==(const _HN_ROAD_DISEASE_INFO_& other) const
		{
			bool equal =
				dMileage == other.dMileage&&
				std::strcmp(other.strDiseaseTableName, other.strDiseaseTableName) == 0&& 
				other.dArea == dArea&& other.nID == this->nID;

			return equal;
		}
		bool operator<(const _HN_ROAD_DISEASE_INFO_& other) const
		{
			return dMileage < other.dMileage;
		} 
		// 索引编号
		int nID;

		// 编码器里程
		double dMileage;

		//里程
		double dDmi;

		// 终点里程
		double dDmiEnd;

		//起点里程
		double dDmiStart;

		//病害所处路面宽度
		double dRoadWidth;

		// 道路规范参数
		char strRoadStandard[SQL_NAME_LEN];

		// 路面材质：0-沥青；1-水泥；2-砂石
		int nRSurfaceType;

		// 绘制类型：0-人工模式；1-自动化模式 2-设计模式面状 3-设计模式线状
		int nDrawType;

		// 病害等级：0-无；1-轻；2-中；-3重
		int nLevel;
		// 长度 病害真实长度 纵向
		double dLength;

		// 宽度 病害真实宽度 横向
		double dWidth;

		// 面积 （计算长度*计算宽度） 
		double dArea;

		// 深度
		double dDepth;

		// 病害拉框像素长度 纵向
		int nPixelLen;

		// 病害拉框像素宽度 横向
		int nPixelWid;

		// 计算长度  (乘以比例因子) 纵向
		double dRealLen;

		// 计算宽度  (乘以比例因子) 横向
		double dReaWidth;
		
		// 病害坐标
		vector<hn2dRectI> vec2dRect;

		//病害框个数
		int nRectCnt;

		//三维顶点坐标
		vector<hn3dRectI> vec3dRect;

		//三维顶点个数
		int n3dCnt;

		// 病害框左右gps时间，自动化模式的则为每个自动化模式的左右gps时间
		vector<double> vecGpsTimer;

		//gps时间个数
		int nGpsCnt;

		// 备注
		char strRemark[SQL_ADDFILE_LEN];
		//必须配置的字段   数据库表名
		char strDiseaseTableName[SQL_ADDFILE_LEN];

		// 病害类型  0-路面破损 
		// cwb 0-普通病害 1-沿线设施 2-路基损坏   1和2都是景观病害
		int ndiseaseType;

		// 病害权重
		double diseaseWeight;

		// 病害类型
		char strDisName[SQL_ADDFILE_LEN];

		// 备用字段4    空[人工病害]  0[人工病害] 1[自动化病害] 2[删除人工病害] 3[删除自动化病害]
 		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5 暂设置为当前时间
		char strAddFile5[SQL_ADDFILE_LEN];
	}hnRoadDiseaseInfo;
	 
	// 病害设置信息
	typedef struct _HN_DISEASE_SETTING_INFO
	{
		_HN_DISEASE_SETTING_INFO()
		{
			nID = 0;
			nDiseaseIndex = 0;
			memset(strDiseaseName, 0, SQL_NAME_LEN);
			memset(strDiseaseTypeName, 0, SQL_NAME_LEN);
			memset(strDBTableName, 0, SQL_NAME_LEN);
			memset(strDisFullName, 0, SQL_NAME_LEN);
			nDiseaseType = 0;
			nRoadSurfaceType = 0;
			nDrawType = 0;
			nLevel = 0;
			fEffectType = 0;
			nShowState = 0;
			fWidget = 0.0;
			fEffectWid = 0.0;
			fValidLen = 0.0;
			fValidArea = 0.0;
			nAreaFormula = 0.0;
			nDWKF = 0;
			memset(strRoadType, 0, SQL_NAME_LEN);
			memset(nShortcutKey, 0, SQL_ADDFILE_LEN);
			memset(strSHMD, 0, SQL_ADDFILE_LEN);
			memset(strDXKF, 0, SQL_ADDFILE_LEN);
			memset(strDescribe, 0, SQL_ADDFILE_LEN);
			memset(strAddFile1, 0, SQL_ADDFILE_LEN);
			memset(strAddFile2, 0, SQL_ADDFILE_LEN);
			memset(strAddFile3, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			memset(strRemark, 0, SQL_ADDFILE_LEN);
		}
		bool operator==(const _HN_DISEASE_SETTING_INFO& other) const
		{
			return strDisFullName == other.strDiseaseName;
		}
		bool operator<(const _HN_DISEASE_SETTING_INFO& other) const
		{
			return nDiseaseIndex < other.nDiseaseIndex;
		}
		//ID
		int nID;

		// 病害枚举索引
		int nDiseaseIndex;

		//记录节点完整名称 cwb
		char strDisFullName[SQL_ADDFILE_LEN];

		// 病害名称 如：路肩损坏
		char strDiseaseName[SQL_NAME_LEN];
		
		// 病害类型名称 如：路肩损坏.轻
		char strDiseaseTypeName[SQL_NAME_LEN];

		// 病害类型  0-路面破损 
		// cwb 0-普通病害 1-沿线设施 2-路基损坏
		int nDiseaseType;

		// 病害所属道路类型
		char strRoadType[SQL_NAME_LEN];

		// 病害所属路面材质   路面材质 0 - 沥青 1 - 水泥 2 - 砂石
		int nRoadSurfaceType;

		// 绘制类型0-人工模式；1-自动化模式 2-设计模式 面状 3-设计模式 线状
		int nDrawType;

		// 病害所属表名
		char strDBTableName[SQL_NAME_LEN];

		// 病害等级  //cwb 0 无 1轻 2中 3重
		int nLevel;

		// 显示状态
		int nShowState;

		// 权重
		float fWidget;

		//损坏类型 cwb  上海
		int	fEffectType;

		// 影响宽度
		float fEffectWid;

		// 有效长度
		float fValidLen;

		// 有效面积
		float fValidArea;

		// 面积公式
		int nAreaFormula;

		// 快捷键
		char nShortcutKey[SQL_ADDFILE_LEN];

		// 单位扣分--附属设置和路基调查
		float nDWKF;

		// 影响计量--附属设置和路基调查   0：按面积来判别  1：按个数来判别
		float dEffectMeasure;

		// 损坏密度
		char strSHMD[SQL_ADDFILE_LEN];

		// 单项扣分项
		char strDXKF[SQL_ADDFILE_LEN];

		// 描述
		char strDescribe[SQL_ADDFILE_LEN];

		// 备用字段1
		char strAddFile1[SQL_ADDFILE_LEN];

		// 备用字段2
		char strAddFile2[SQL_ADDFILE_LEN];

		// 备用字段3
		char strAddFile3[SQL_ADDFILE_LEN];

		// 备用字段4
		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5
		char strAddFile5[SQL_ADDFILE_LEN];

		// 备注
		char strRemark[SQL_ADDFILE_LEN];
	}hnDiseaseSetInfo;

	// 道路类型参数
	typedef struct _HN_ROAD_TYPE_SETTING_INFO
	{
		_HN_ROAD_TYPE_SETTING_INFO()
		{
            nID = 0;
			nRSurfaceType = 0;
			nDrawType = 0;
			memset(strRoadFullName, 0, SQL_NAME_LEN);
			memset(strSN_wi, 0, SQL_NAME_LEN);
			memset(strLQ_wi, 0, SQL_NAME_LEN);
			//dRutThreslod = 0.0;
			dRutThreslodUp=0.0;
			dRutThreslodDown = 0.0;
			dRQI_w1 = 0.0;
			dRQI_w2 = 0.0;
			nRutIndex = 0;
			memset(nRoadLevel, 0, SQL_NAME_LEN);
			memset(strRoadType, 0, SQL_NAME_LEN);
			memset(strRQILevel, 0, SQL_NAME_LEN);
			memset(strRDILevel, 0, SQL_NAME_LEN);
			memset(strPWILevel, 0, SQL_NAME_LEN);
			memset(strMTDLevel, 0, SQL_NAME_LEN);
			memset(strIRILevel, 0, SQL_NAME_LEN);
			memset(strPCILevel, 0, SQL_NAME_LEN);
			memset(strPQILevel, 0, SQL_NAME_LEN);
			memset(strPBILevel, 0, SQL_NAME_LEN);
			memset(strMQILevel, 0, SQL_NAME_LEN);
			memset(strRealV, 0, SQL_NAME_LEN);
			memset(strAmendPara, 0, SQL_NAME_LEN);
			dRQI_a0 = 0.0;
			dRQI_a1 = 0.0;
			dPCI_a0 = 0.0;
			dPCI_a1 = 0.0;
			dPQI_WPCI = 0.0;
			dPQI_WRQI = 0.0;
			dPQI_WRDI = 0.0;
			dPQI_WPBI = 0.0;
			dPQI_WPWI = 0.0;
			dRDI_a = 0.0;
			dRDI_b = 0.0;
			dRDI_RDa = 0.0;
			dRDI_RDb = 0.0;
			dRDI_a0 = 0.0;
			dRDI_a1 = 0.0;
			dPWI_a0 = 0.0;
			dPWI_a1 = 0.0;
			dMQI_WSCI = 0.0;
			dMQI_WPQI = 0.0;
			dMQI_WBCI = 0.0;
			dMQI_WTCI = 0.0;
			memset(strPBI_KFBZ, 0, SQL_NAME_LEN);
			memset(strPBI_KF, 0, SQL_NAME_LEN);
			memset(strAddFile1, 0, SQL_ADDFILE_LEN);
			memset(strAddFile2, 0, SQL_ADDFILE_LEN);
			memset(strAddFile3, 0, SQL_ADDFILE_LEN);
			memset(strAddFile4, 0, SQL_ADDFILE_LEN);
			memset(strAddFile5, 0, SQL_ADDFILE_LEN);
			memset(strRemark, 0, SQL_ADDFILE_LEN);
		}

		// 索引编号
		int nID;
		//记录节点完整名称 cwb
		char strRoadFullName[SQL_ADDFILE_LEN];

		// 道路类型参数：0-等级公路2018；1-城镇道路；2-低等级农村路
		char strRoadType[SQL_NAME_LEN];

		// 路面材质：0-沥青；1-水泥；2-砂石
		int nRSurfaceType;

		// 绘制类型0-人工模式；1-自动化模式
		int nDrawType;

		// 道路等级
		char nRoadLevel[SQL_NAME_LEN];

		// 水泥路面权函数曲线
		char strSN_wi[SQL_NAME_LEN];

		// 沥青路面权函数曲线
		char strLQ_wi[SQL_NAME_LEN];

		// 车辙阈值和轻度索引
		//2022.4.2 cwb 上下限修正
		//Rut重度
		float dRutThreslodUp;
		//Rut轻度
		float dRutThreslodDown;
		int nRutIndex;


		//2022.4.2 cwb IRI速度修正（湖南农村路） 
		//实测速度	cwb IRI速度修正（湖南农村路）
		char strRealV[SQL_NAME_LEN];
		//修正系数	cwb IRI速度修正（湖南农村路）
		char strAmendPara[SQL_NAME_LEN];


		// RQI等级区间
		char strRQILevel[SQL_NAME_LEN];

		// RDI等级区间
		char strRDILevel[SQL_NAME_LEN];

		// PWI等级区间
		char strPWILevel[SQL_NAME_LEN];

		// MTD等级区间
		char strMTDLevel[SQL_NAME_LEN];

		// IRI等级区间
		char strIRILevel[SQL_NAME_LEN];

		// PCI等级区间
		char strPCILevel[SQL_NAME_LEN];

		// PQI等级区间
		char strPQILevel[SQL_NAME_LEN];

		// PBI等级区间
		char strPBILevel[SQL_NAME_LEN];

		// MQI等级区间
		char strMQILevel[SQL_NAME_LEN];

		// RQI系数
		float dRQI_a0;
		float dRQI_a1;
		//cwb 
		float dRQI_w1;
		float dRQI_w2;
		//
		// PCI系数
		float dPCI_a0;
		float dPCI_a1;

		// PQI 系数
		float dPQI_WPCI;
		float dPQI_WRQI;
		float dPQI_WRDI;
		float dPQI_WPBI;
		float dPQI_WPWI;

		// RDI系数
		float dRDI_a;
		float dRDI_b;
		float dRDI_RDa;
		float dRDI_RDb;
		float dRDI_a0;
		float dRDI_a1;

		// PWI系数
		float dPWI_a0;
		float dPWI_a1;

		// MQI系数
		float dMQI_WSCI;
		float dMQI_WPQI;
		float dMQI_WBCI;
		float dMQI_WTCI;

		// PBI系数
		char strPBI_KFBZ[SQL_NAME_LEN];
		char strPBI_KF[SQL_NAME_LEN];

		// 备用字段1
		char strAddFile1[SQL_ADDFILE_LEN];

		// 备用字段2
		char strAddFile2[SQL_ADDFILE_LEN];

		// 备用字段3
		char strAddFile3[SQL_ADDFILE_LEN];

		// 备用字段4
		char strAddFile4[SQL_ADDFILE_LEN];

		// 备用字段5
		char strAddFile5[SQL_ADDFILE_LEN];

		// 备注
		char strRemark[SQL_ADDFILE_LEN];

	}hnRoadTypeSetInfo;

	// 采集工程信息
	typedef struct _HN_PROJECT_DATA_INFO
	{
		_HN_PROJECT_DATA_INFO()
		{
			memset(strProjectPath, 0, SQL_ADDFILE_LEN);
			memset(str3dProjectPath, 0, SQL_ADDFILE_LEN);
			memset(str2DProName, 0, SQL_NAME_LEN);
			memset(str3DProName, 0, SQL_NAME_LEN);
			memset(strProJectName, 0, SQL_NAME_LEN);
			nFrequency = 0; 
			dWheelPerimeter = 0.0;
			
		}

		// 工程路径
		char strProjectPath[SQL_ADDFILE_LEN];
		char strProJectName[SQL_NAME_LEN];
		//3d工程路径
		char str3dProjectPath[SQL_ADDFILE_LEN];
		// 设置参数
		hnProjectSetInfo proSetInfo;

		// 二维工程名称
		char str2DProName[SQL_NAME_LEN];

		// 三维工程名称
		char str3DProName[SQL_NAME_LEN];

		// 病害表集合
		vector<string> vecDiseaseTable;

		// 编码器频率
		int nFrequency;

		// 轮子周长
		double dWheelPerimeter;

		
		// 桩号信息
		vector<hnMilePile> vecMilePile;

	}hnProjectDataInfo;

	// 控制点信息
	typedef struct _HN_KZD_DATA_INFO
	{
		_HN_KZD_DATA_INFO()
		{
			nID = -1;
			memset(strKzdName, 0, SQL_NAME_LEN);
			dGpsTimer = 0.0;
			memset(strImageName, 0, SQL_NAME_LEN);
			nLocX = 0;
			nLocY = 0;
			dX = 0.0;
			dY = 0.0;
			dZ = 0.0;
			memset(strRemaks, 0, SQL_QUERY_LEN);
		}

		// 索引
		int nID;

		// 控制点名称 命名方式：控制点+ID   如：控制点_2
		char strKzdName[SQL_NAME_LEN];

		// 控制点GPS时间
		double dGpsTimer;

		// 里程
		double dMileage;

		// 图像名称 不含路径 RGB 、 GREY
		char strImageName[SQL_NAME_LEN];

		// 图片X位置 坐标系以左上角为原点
		int nLocX;

		// 图片Y位置 坐标系以左上角为原点
		int nLocY;

		// 三维坐标-X
		double dX;

		// 三维坐标-Y
		double dY;

		// 三维坐标-Z
		double dZ;

		// 备注
		char strRemaks[SQL_QUERY_LEN];
	}hnKZDDataInfo;
}

#endif // !_HN_ROAD_STRUCT_

