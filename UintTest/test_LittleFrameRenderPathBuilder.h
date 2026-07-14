#pragma once

#include <QObject>

class test_LittleFrameRenderPathBuilder : public QObject
{
	Q_OBJECT

private slots:
	void exactBoundaryCounts_data();
	void exactBoundaryCounts();
	void rowMergeStartsAfterExactLimit();
	void excessiveRunsUsePerImageBounds();
	void validCountsNeverProduceEmptyPath();
	void inputOrderDoesNotChangeRenderSummary();
};
