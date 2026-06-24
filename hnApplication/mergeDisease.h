#pragma once
#include "hnapplication_global.h"
#include <QPoint>
#include "../hnCommon/hnRoadStruct.h"
#include <QVector>
#include <QRect>
using namespace hnCommon;
using namespace std;
class HNAPPLICATION_EXPORT mergeDisease
{
public:
	mergeDisease();
	~mergeDisease();
protected:
	//人工模式合并病害
	virtual void bigFrameMergeDiseases(const QPoint &screenPoint) = 0;

	//自动化模式合并病害
	virtual void littleFrameMergeDiseases(const QPoint &screenPoint) = 0;

	//计算两个病害以及病害之间的的所有自动化模式矩形 坐标系是大image
	virtual QVector<QRect> caculateLittleFrameRects(QVector<hnRoadDiseaseInfo> diseases) = 0;

};

