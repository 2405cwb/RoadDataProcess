#include "test_mergeAidcDiseases.h"
#include "../hnRoadDataProcess/mergeAidcDiseases.h"
#include <QtTest/QTest>
test_mergeAidcDiseases::test_mergeAidcDiseases(QObject *parent)
	: QObject(parent)
{
}

test_mergeAidcDiseases::~test_mergeAidcDiseases()
{
}

void test_mergeAidcDiseases::isMergeableDisease_true()
{
	/*hnCommon::hn2dRectI rect;

	hnCommon::hnRoadDiseaseInfo down;
	down.dDmiEnd = 1;
	down.vec2dRect.push_back(rect);
	down.vec2dRect.at(0).p0.x = 100;
	down.vec2dRect.at(0).p1.x = 200;

	hnCommon::hnRoadDiseaseInfo up;
	up.vec2dRect.push_back(rect);
	up.dDmiEnd = 1.1;
	up.vec2dRect.at(0).p0.x = 110;
	up.vec2dRect.at(0).p1.x = 210;
	mergeAidcDiseases obj;
	QVERIFY(true == obj.isMergeableDisease(down, up));*/
}
