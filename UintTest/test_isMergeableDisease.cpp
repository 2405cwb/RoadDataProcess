#include "test_isMergeableDisease.h"
#include "../hnRoadDataProcess/IsMergeableDisease.h"
#include <QtTest/QTest>
test_isMergeableDisease::test_isMergeableDisease(QObject *parent)
	: QObject(parent)
{
}

test_isMergeableDisease::~test_isMergeableDisease()
{
}

void test_isMergeableDisease::VMirror_isMergeableDisease()
{
	hnCommon::hn2dRectI rect;

	hnCommon::hnRoadDiseaseInfo down;
	down.dDmi = 2;
	down.vec2dRect.push_back(rect);
	down.vec2dRect.at(0).p0.x = 100;
	down.vec2dRect.at(0).p1.x = 200;
	down.vec2dRect.at(0).p3.y = 990;

	hnCommon::hnRoadDiseaseInfo up;
	up.vec2dRect.push_back(rect);
	up.dDmi = 4;
	up.vec2dRect.at(0).p0.x = 110;
	up.vec2dRect.at(0).p1.x = 210;
	up.vec2dRect.at(0).p0.y = 10;
	IsMergeableDisease obj;
	int upMinDisIndex = 0, downMinDisIndex = 0;
	QVERIFY(true == obj.IsMergeable(down, up, true, 1000, 2, upMinDisIndex, downMinDisIndex));
}

void test_isMergeableDisease::noVMirror_isMergeableDisease()
{
	hnCommon::hn2dRectI rect;

	hnCommon::hnRoadDiseaseInfo down;
	down.dDmi = 2;
	down.vec2dRect.push_back(rect);
	down.vec2dRect.at(0).p0.x = 100;
	down.vec2dRect.at(0).p1.x = 200;
	down.vec2dRect.at(0).p0.y = 10;

	hnCommon::hnRoadDiseaseInfo up;
	up.vec2dRect.push_back(rect);
	up.dDmi = 4;
	up.vec2dRect.at(0).p0.x = 110;
	up.vec2dRect.at(0).p1.x = 210;
	up.vec2dRect.at(0).p3.y = 990;
	IsMergeableDisease obj;
	int upMinDisIndex = 0, downMinDisIndex = 0;

	//验证条件是否为真
	QVERIFY(true == obj.IsMergeable(down, up, false, 1000, 2, upMinDisIndex, downMinDisIndex));
}
