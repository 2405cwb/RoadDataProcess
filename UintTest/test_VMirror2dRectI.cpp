#include "test_VMirror2dRectI.h"
#include "../hnRoadDataProcess/VMirror2dRectI.h"
#include "../hnCommon/hn2dPointWithMile.h"
#include <QTest>
test_VMirror2dRectI::test_VMirror2dRectI(QObject *parent)
	: QObject(parent)
{
}

test_VMirror2dRectI::~test_VMirror2dRectI()
{
}

void test_VMirror2dRectI::VMirrorPoint()
{
	hnCommon::hn2dPointWithMileI point;
	const int pixHeight = 1000;
	point.x = 100;
	point.y = 100;

	VMirror2dRectI testObj;
	const hnCommon::hn2dPointWithMileI dstPoint = testObj.VMirrorPoint(point, pixHeight);

	QVERIFY(900 == dstPoint.y);
}

void test_VMirror2dRectI::VMirrorRect()
{
	const int pixHeight = 1000;
	hnCommon::hn2dRectI rect;
	rect.p0.y = 800;
	rect.p1.y = 800;
	rect.p2.y = 900;
	rect.p3.y = 900;

	VMirror2dRectI testObj;
	const hnCommon::hn2dRectI mirrorRect = testObj.VMirrorRect(rect, pixHeight);

	bool isOk = false;
	isOk = (200 == mirrorRect.p0.y) && (200 == mirrorRect.p1.y) && (100 == mirrorRect.p2.y) && (100 == mirrorRect.p3.y);

	QVERIFY(isOk == true);
}
