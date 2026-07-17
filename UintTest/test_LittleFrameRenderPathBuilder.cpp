#include "test_LittleFrameRenderPathBuilder.h"

#include "../hnApplication/LittleFrameRenderPathBuilder.h"
#include <QtTest/QTest>
#include <algorithm>

using namespace hnApp;

namespace
{
	QVector<LittleFrameRenderCell> makeSingleRowCells(int count, const QString& imageName = QStringLiteral("image-1"))
	{
		QVector<LittleFrameRenderCell> cells;
		cells.reserve(count);
		for (int i = 0; i < count; ++i)
		{
			LittleFrameRenderCell cell;
			cell.imageName = imageName;
			cell.sourceRect = QRect(i * 11, 0, 10, 10);
			cell.sceneRect = QRectF(i * 11.0, 100.0, 10.0, 10.0);
			cells.append(cell);
		}
		return cells;
	}

	QVector<LittleFrameRenderCell> makeSeparatedRows(int count)
	{
		QVector<LittleFrameRenderCell> cells;
		cells.reserve(count);
		for (int i = 0; i < count; ++i)
		{
			LittleFrameRenderCell cell;
			cell.imageName = QStringLiteral("image-1");
			cell.sourceRect = QRect(0, i * 20, 10, 10);
			cell.sceneRect = QRectF(50.0, i * 20.0, 10.0, 10.0);
			cells.append(cell);
		}
		return cells;
	}
}

void test_LittleFrameRenderPathBuilder::exactBoundaryCounts_data()
{
	QTest::addColumn<int>("count");
	for (int count : QVector<int>{14, 15, 16, 17, 19, 25, 256})
	{
		QTest::newRow(qPrintable(QString::number(count))) << count;
	}
}

void test_LittleFrameRenderPathBuilder::exactBoundaryCounts()
{
	QFETCH(int, count);
	const LittleFrameRenderResult result = LittleFrameRenderPathBuilder::build(makeSingleRowCells(count));
	QVERIFY(!result.path.isEmpty());
	QCOMPARE(result.mode, LittleFrameRenderExactCells);
	QCOMPARE(result.segmentCount, count);
	QCOMPARE(result.path.boundingRect(), QRectF(0.0, 100.0, (count - 1) * 11.0 + 10.0, 10.0));
}

void test_LittleFrameRenderPathBuilder::rowMergeStartsAfterExactLimit()
{
	const LittleFrameRenderResult result = LittleFrameRenderPathBuilder::build(makeSingleRowCells(257));
	QVERIFY(!result.path.isEmpty());
	QCOMPARE(result.mode, LittleFrameRenderRowRuns);
	QCOMPARE(result.segmentCount, 1);
	QCOMPARE(result.path.boundingRect(), QRectF(0.0, 100.0, 2826.0, 10.0));
}

void test_LittleFrameRenderPathBuilder::excessiveRunsUsePerImageBounds()
{
	const LittleFrameRenderResult result = LittleFrameRenderPathBuilder::build(makeSeparatedRows(513));
	QVERIFY(!result.path.isEmpty());
	QCOMPARE(result.mode, LittleFrameRenderRowRuns);
	QCOMPARE(result.segmentCount, 513);
	QCOMPARE(result.path.boundingRect(), QRectF(50.0, 0.0, 10.0, 10250.0));
}

void test_LittleFrameRenderPathBuilder::validCountsNeverProduceEmptyPath()
{
	for (int count = 1; count <= 300; ++count)
	{
		const LittleFrameRenderResult result = LittleFrameRenderPathBuilder::build(makeSingleRowCells(count));
		QVERIFY2(!result.path.isEmpty(), qPrintable(QStringLiteral("empty path for count=%1").arg(count)));
		QVERIFY(result.segmentCount > 0);
		QVERIFY(result.segmentCount <= count);
		QCOMPARE(result.path.boundingRect(), QRectF(0.0, 100.0, (count - 1) * 11.0 + 10.0, 10.0));
	}
}

void test_LittleFrameRenderPathBuilder::inputOrderDoesNotChangeRenderSummary()
{
	QVector<LittleFrameRenderCell> forward = makeSingleRowCells(300);
	QVector<LittleFrameRenderCell> reverse = forward;
	std::reverse(reverse.begin(), reverse.end());

	const LittleFrameRenderResult forwardResult = LittleFrameRenderPathBuilder::build(forward);
	const LittleFrameRenderResult reverseResult = LittleFrameRenderPathBuilder::build(reverse);
	QCOMPARE(reverseResult.mode, forwardResult.mode);
	QCOMPARE(reverseResult.segmentCount, forwardResult.segmentCount);
	QCOMPARE(reverseResult.path.boundingRect(), forwardResult.path.boundingRect());
}
