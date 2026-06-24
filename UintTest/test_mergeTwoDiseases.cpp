#include "test_mergeTwoDiseases.h"
#include "../hnRoadDataProcess/mergeTwoDiseases.h"
#include <QTest>

test_mergeTwoDiseases::test_mergeTwoDiseases(QObject *parent)
	: QObject(parent)
{
}

test_mergeTwoDiseases::~test_mergeTwoDiseases()
{
}

void test_mergeTwoDiseases::VMirror_merge()
{
	hnCommon::hnRoadDiseaseInfo diseaseUp;
	diseaseUp.dDmiStart = 2.1;
	diseaseUp.dDmiEnd = 2.4;
	hnCommon::hn2dRectI rectUp;
	rectUp.p0.m_dmi = 2;
	rectUp.p0.x = 50;
	rectUp.p0.y = 50;

	rectUp.p1.m_dmi = 2;
	rectUp.p1.x = 100;
	rectUp.p1.y = 50;

	rectUp.p2.m_dmi = 2;
	rectUp.p2.x = 100;
	rectUp.p2.y = 100;

	rectUp.p3.m_dmi = 2;
	rectUp.p3.x = 50;
	rectUp.p3.y = 100;
	diseaseUp.vec2dRect.push_back(rectUp);

	hnCommon::hnRoadDiseaseInfo diseaseDown;
	diseaseDown.dDmiStart = 1;
	diseaseDown.dDmiEnd = 1.2;
	hnCommon::hn2dRectI rect2;
	rect2.p0.m_dmi = 0;
	rect2.p0.x = 100;
	rect2.p0.y = 50;

	rect2.p1.m_dmi = 0;
	rect2.p1.x = 200;
	rect2.p1.y = 50;

	rect2.p2.m_dmi = 0;
	rect2.p2.x = 200;
	rect2.p2.y = 100;

	rect2.p3.m_dmi = 0;
	rect2.p3.x = 100;
	rect2.p3.y = 100;
	diseaseDown.vec2dRect.push_back(rect2);

	mergeTwoDiseases mergeObj;
	hnCommon::hnRoadDiseaseInfo dstDisease = mergeObj.merge(diseaseUp, diseaseDown, 2, 1024,true);

	bool isSucceed = false;
	if (dstDisease.vec2dRect.empty())
	{
		QVERIFY(isSucceed);
		return;
	}

	isSucceed = (dstDisease.vec2dRect.at(0).p0.x == 50) &&	// p0
		(dstDisease.vec2dRect.at(0).p0.y == 100) &&
		(dstDisease.vec2dRect.at(0).p0.m_dmi == 2) &&
		(dstDisease.vec2dRect.at(0).p1.x == 200) &&			//p1
		(dstDisease.vec2dRect.at(0).p1.y == 100) &&
		(dstDisease.vec2dRect.at(0).p1.m_dmi == 2) &&
		(dstDisease.vec2dRect.at(0).p2.x == 200) &&			//p2
		(dstDisease.vec2dRect.at(0).p2.y == 50) &&
		(dstDisease.vec2dRect.at(0).p2.m_dmi == 0) &&
		(dstDisease.vec2dRect.at(0).p3.x == 50) &&			//p3
		(dstDisease.vec2dRect.at(0).p3.y == 50) &&
		(dstDisease.vec2dRect.at(0).p3.m_dmi == 0) &&
		(1 == dstDisease.dDmiStart) &&						// 里程
		(2.4 == dstDisease.dDmiEnd) &&
		(1.7 == dstDisease.dMileage);

	QVERIFY(isSucceed);
}


void test_mergeTwoDiseases::noVMirror_merge()
{
	hnCommon::hnRoadDiseaseInfo diseaseUp;
	diseaseUp.dDmiStart = 2.1;
	diseaseUp.dDmiEnd = 2.4;
	hnCommon::hn2dRectI rectUp;
	rectUp.p0.m_dmi = 2;
	rectUp.p0.x = 50;
	rectUp.p0.y = 50;

	rectUp.p1.m_dmi = 2;
	rectUp.p1.x = 100;
	rectUp.p1.y = 50;

	rectUp.p2.m_dmi = 2;
	rectUp.p2.x = 100;
	rectUp.p2.y = 100;

	rectUp.p3.m_dmi = 2;
	rectUp.p3.x = 50;
	rectUp.p3.y = 100;
	diseaseUp.vec2dRect.push_back(rectUp);

	hnCommon::hnRoadDiseaseInfo diseaseDown;
	diseaseDown.dDmiStart = 1;
	diseaseDown.dDmiEnd = 1.2;
	hnCommon::hn2dRectI rect2;
	rect2.p0.m_dmi = 0;
	rect2.p0.x = 100;
	rect2.p0.y = 50;

	rect2.p1.m_dmi = 0;
	rect2.p1.x = 200;
	rect2.p1.y = 50;

	rect2.p2.m_dmi = 0;
	rect2.p2.x = 200;
	rect2.p2.y = 100;

	rect2.p3.m_dmi = 0;
	rect2.p3.x = 100;
	rect2.p3.y = 100;
	diseaseDown.vec2dRect.push_back(rect2);

	mergeTwoDiseases mergeObj;
	hnCommon::hnRoadDiseaseInfo dstDisease = mergeObj.merge(diseaseUp, diseaseDown, 2, 1024,false);

	bool isSucceed = false;
	if (dstDisease.vec2dRect.empty())
	{
		QVERIFY(isSucceed);
		return;
	}

	isSucceed = (dstDisease.vec2dRect.at(0).p0.x == 50) &&	// p0
		(dstDisease.vec2dRect.at(0).p0.y == 50) &&
		(dstDisease.vec2dRect.at(0).p0.m_dmi == 2) &&
		(dstDisease.vec2dRect.at(0).p1.x == 200) &&			//p1
		(dstDisease.vec2dRect.at(0).p1.y == 50) &&
		(dstDisease.vec2dRect.at(0).p1.m_dmi == 2) &&
		(dstDisease.vec2dRect.at(0).p2.x == 200) &&			//p2
		(dstDisease.vec2dRect.at(0).p2.y == 100) &&
		(dstDisease.vec2dRect.at(0).p2.m_dmi == 0) &&
		(dstDisease.vec2dRect.at(0).p3.x == 50) &&			//p3
		(dstDisease.vec2dRect.at(0).p3.y == 100) &&
		(dstDisease.vec2dRect.at(0).p3.m_dmi == 0) &&
		(1 == dstDisease.dDmiStart) &&						// 里程
		(2.4 == dstDisease.dDmiEnd) &&
		(1.7 == dstDisease.dMileage);

	QVERIFY(isSucceed);
}