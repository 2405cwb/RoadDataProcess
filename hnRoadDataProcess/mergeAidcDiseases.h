#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include "../hnCommon/hnRoadStruct.h"



class mergeAidcDiseases : public QObject
{
	Q_OBJECT

public:
	mergeAidcDiseases(double roadWidth, int pixHeight, bool isVMirror, bool isDiseaseMap , QObject *parent = nullptr);
	~mergeAidcDiseases();

public:
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>>
		mergeDiseases(QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases);

private:
	std::vector<hnCommon::hnRoadDiseaseInfo > mergeSingleDisease(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos);
	
	// 自动化模式病害合并
	std::vector<hnCommon::hnRoadDiseaseInfo> mergeSingleDiseaseMut(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos);

	// 人工模式病害合并
	std::vector<hnCommon::hnRoadDiseaseInfo> BigFrameMergeSingleDisease(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos);

	// 计算直线经过的矩形框
	void getConnectRects(QVector<QRect>& AllRectSet, QLine minDisLine, QVector<QRect> &connectRects);

	void collectionLittteFrame(QVector<hnCommon::hnRoadDiseaseInfo>& SingleDiseaseSets, hnCommon::hnRoadDiseaseInfo &tmpDiseaseUp);

	// 判断直线和矩形框是否相交
	bool isintersectBetweenRectAndLine(const QRect & rect, const QLineF & line);

	bool combineMergeableDisease(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int newID, hnCommon::hnRoadDiseaseInfo &newDisease);

	bool BigFramecombineMergeableDisease(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int newID, hnCommon::hnRoadDiseaseInfo & newDisease);

	std::vector<hnCommon::hnRoadDiseaseInfo > transDiseaseWithMirror(std::vector<hnCommon::hnRoadDiseaseInfo > & disease, bool isVMirror, int imgHeight);

	void sortDiseaseWithSameDmi(std::vector<hnCommon::hnRoadDiseaseInfo>& disease, std::vector<std::vector<hnCommon::hnRoadDiseaseInfo>>& sortedDiseaseSets);

	void merge2dRectWithoutOverlap(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int imgHeight, int imgWidth);

	
	
	
	//路面宽度 米
	double m_roadWidth;
	// 图片像素高度
	int m_pixHeight;
	// 图片像素宽度
	int m_pixWidth;
	// 是否垂直镜像
	bool m_isVMirror;
	// 是否进行 2d 映射到 3d
	bool m_isDiseaseMap;

};
