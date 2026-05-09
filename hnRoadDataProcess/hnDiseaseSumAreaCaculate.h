#pragma once

#include <QObject>
#include "../hnApplication/hnDataManager.h"
#include "../hnProject/hnProject.h"
#include "../hnCommon/hnRoadStruct.h"

using namespace hnApp;

//沥青面积和
struct AsphaltSumArea
{
	double JL_light;	//龟裂_轻
	double JL_middle;	//龟裂_中
	double JL_weight;	//龟裂_重
	double KZLF_light;	//块状裂缝_轻
	double KZLF_weight;	//块状裂缝_重
	double ZXLF_light;	//纵向裂缝_轻
	double ZXLF_weight;	//纵向裂缝_重
	double HXLF_light;	//横向裂缝_轻
	double HXLF_weight;	//横向裂缝_重
	double CX_light;	//沉陷_轻
	double CX_weight;	//沉陷_重
	double CZ_light;	//车辙_轻
	double CZ_weight;	//车辙_重
	double BLYB_light;	//波浪拥包_轻
	double BLYB_weight;	//波浪拥包_重
	double KC_light;	//坑槽_轻
	double KC_weight;	//坑槽_重
	double SS_light;	//松散_轻
	double SS_weight;	//松散_重
	double FY;			//泛油
	double XBKZ;		//修补块状
	double XBTZ;		//修补条状
};
//水泥面积和 Cement
struct CementSumArea
{

};

class hnDiseaseSumAreaCaculate : public QObject
{
	Q_OBJECT

public:
	hnDiseaseSumAreaCaculate(QObject *parent = Q_NULLPTR);
	~hnDiseaseSumAreaCaculate();

public:
	// 获取病害的面积和，这个面积和的顺序需要和Excel表格模板还有数据库接口获取病害类型的顺序完全一致，由开发人员自行判别。
	// 路面材质 0 - 沥青 1 - 水泥 2 - 砂石
	//   mutWeight   传入病害面积  diseases 计算面积的时候是否乘以权重   如果是病害明细表用到 里面展示的应该是未乘以权重的病害面积
	QVector<double> caculateSumArea(const QVector<hnCommon::hnRoadDiseaseInfo> & diseases,bool  mutWeight,
		const HnProjectEnums::StandardParmTypeEnum standard, int roadSurfaceType , hnPro::hnProject* project);

	//判断一个数组中是不是都是0
	bool isAllZero(const QVector<double> &vector);

};
