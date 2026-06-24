#pragma once
#include "hnapplication_global.h"
#include <QString>
#include <QMessageBox>
#include "../hnCommon/hn3dRect.h"
#include "depthDialog.h"
#include "DepthDisease.h"
#include "rectAlgorithm.h"
#include "drawDiseases.h"

using namespace hnCommon;
 
//深度计算
class HNAPPLICATION_EXPORT depthCaculate
{
public:
	depthCaculate();

public:
	void setOpenDepthCaculate(const bool isOpen);

	bool getOpenDepthCaculate();

protected:
	//计算人工模式深度信息，如果返回值为false，则取消画病害
	bool caculateBigFrameDiseaseDepth(hnRoadDiseaseInfo &disease);

	//计算自动化模式深度信息，如果返回值为false，则取消画病害
	// 使用函数指针
	// 两个引用都传*this就行
	bool caculateLittleFrameDiseaseDepth(hnRoadDiseaseInfo &disease,
		rectAlgorithm &algorithm,
		QRect(rectAlgorithm::*mergeRectsFun)(QVector<QRect> littleRects),
		drawDiseases &widget,
		std::vector<hnCommon::hn3dRectI> (drawDiseases::*generateLargeFrameHn3dRectFun)(const QRect &unitedRect),
		QVector<QRect> tmpLittleFrameDiseaseRects);

	//重载函数，直接传入病害的引用，获取病害的信息
	bool checkDepth(hnRoadDiseaseInfo &diseaseInfo);

private:
	/*
	* 函数名称：checkDepthResult
	* 函数功能：检查深度计算的结果
	* 参数1：深度计算的结果
	* 参数2：病害面积
	* 参数3：病害类型，如果轻中重不对，自动修正
	* 返回值：0：正常，保存深度计算的结果	-1：不在范围内，则不写入病害	-2计算出来深度的程度[轻重中]与画的不一致，自动修正
	*/
	int checkDepthResult(hnRoadDiseaseInfo &diseaseInfo);

	//int checkDepthResult(double depth, double area, QString &diseaseTypeName);

	// 判断病害是否为变形类病害 deformation n.变形
	bool isDeformationDisease(const QString &diseaseTypeName);

	//获取深度信息计算的配置
	QMap<HnProjectEnums::StandardParmTypeEnum, QVector<DepthDisease>> getDepthDiseaseConfigDatas();

	//沉陷类病害配置文件读取   cwb  20231226
	void readDepthDiseaseConfig();

protected:
	bool m_isOPenDepthCaculate;

private: 
	QMap<HnProjectEnums::StandardParmTypeEnum, QVector<DepthDisease>> m_depthDiseases;

};

