#pragma once

#include <QObject>
#include "../hnCommon/hnRoadStruct.h"

class mergeTwoDiseases : public QObject
{
	Q_OBJECT

public:
	mergeTwoDiseases(QObject *parent = nullptr);
	~mergeTwoDiseases();

public:
	hnCommon::hnRoadDiseaseInfo merge(const hnCommon::hnRoadDiseaseInfo &up, const hnCommon::hnRoadDiseaseInfo &down,double roadWidth,int pixHeight,bool isVMirror);
	
};
