#pragma once

#include <QObject>

class test_DrPciCalculator : public QObject
{
	Q_OBJECT

private slots:
	void overlapBoundaryExamples();
	void reverseDirectionProperty();
	void adjacentSegmentConservationProperty();
	void noDiseaseAndStandardPoints();
	void interpolationClampsEndpoints();
	void cityStandardExamplesAndClamp();
};
