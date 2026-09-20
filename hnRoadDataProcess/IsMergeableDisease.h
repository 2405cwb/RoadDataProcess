#pragma once

#include <QObject>
#include "../hnCommon/hnRoadStruct.h"
//#include "../hnApplication/hnDataManager.h"

class IsMergeableDisease : public QObject
{
	Q_OBJECT

public:
	IsMergeableDisease(QObject *parent = nullptr);
	~IsMergeableDisease();

public:
	bool IsMergeable(const hnCommon::hnRoadDiseaseInfo &diseaseDown, const hnCommon::hnRoadDiseaseInfo &diseaseUp,
		bool isVMirror, int pixHeight, double roadWidth, int &minIndexUp, int &minIndexDown, int gYThreshold = 700, int gLeftRightpixelThreshold = 700);

	bool BigFrameIsMergeable(const hnCommon::hnRoadDiseaseInfo &diseaseDown, const hnCommon::hnRoadDiseaseInfo &diseaseUp,
		int pixHeight, double imageInterval, int gYThreshold = 400, int gLeftRightpixelThreshold = 300);

	double verticalMinDistance(const hnCommon::hnRoadDiseaseInfo & diseaseDown, const hnCommon::hnRoadDiseaseInfo & diseaseUp, int pixHeight, int &minIndexUp, int &minIndexDown);

	hnCommon::hnRoadDiseaseInfo pureMerge(const hnCommon::hnRoadDiseaseInfo & up, const hnCommon::hnRoadDiseaseInfo & down, int minIndexUp, int minIndexDown, int pixHeight);


	

};
