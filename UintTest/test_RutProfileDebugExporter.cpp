#include "test_RutProfileDebugExporter.h"

#include "../hnRoadDataProcess/RutProfileDebugExporter.h"
#include <QtTest/QTest>

void test_RutProfileDebugExporter::parsesPileText()
{
	double value = 0.0;
	QVERIFY(RutProfileDebugExporter::parsePileText(QStringLiteral("K1490+500"), value));
	QCOMPARE(value, 1490500.0);
	QVERIFY(RutProfileDebugExporter::parsePileText(QStringLiteral("k12+34.5"), value));
	QCOMPARE(value, 12034.5);
	QVERIFY(RutProfileDebugExporter::parsePileText(QStringLiteral("1490500"), value));
	QCOMPARE(value, 1490500.0);
	QVERIFY(!RutProfileDebugExporter::parsePileText(QStringLiteral("K12+1000"), value));
	QVERIFY(!RutProfileDebugExporter::parsePileText(QStringLiteral("K12+ABC"), value));
}

void test_RutProfileDebugExporter::convertsDmiToFrameBoundaries()
{
	QCOMPARE(RutProfileDebugExporter::firstFrameAtOrAfter(100.0, 1.0), qint64(1000));
	QCOMPARE(RutProfileDebugExporter::lastFrameAtOrBefore(100.0, 1.0), qint64(1000));
	QCOMPARE(RutProfileDebugExporter::firstFrameAtOrAfter(100.01, 1.0), qint64(1001));
	QCOMPARE(RutProfileDebugExporter::lastFrameAtOrBefore(100.09, 1.0), qint64(1000));
	QCOMPARE(RutProfileDebugExporter::firstFrameAtOrAfter(100.0, 2.0), qint64(2000));
	QCOMPARE(RutProfileDebugExporter::lastFrameAtOrBefore(100.0, 0.5), qint64(500));
}
