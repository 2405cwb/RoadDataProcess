#include "test_DrPciCalculator.h"
#include "../hnRoadDataProcess/DrPciCalculator.h"

#include <QTest>
#include <cmath>
#include <random>

namespace
{
	void verifyArea(double diseaseStart, double diseaseEnd, double area,
		double segmentStart, double segmentEnd, double expected)
	{
		double actual = -1.0;
		QVERIFY(hnDrPci::clippedWeightedArea(diseaseStart, diseaseEnd, area, 1.0,
			segmentStart, segmentEnd, actual));
		QVERIFY2(std::fabs(actual - expected) < 1e-9, "clipped disease area differs");
	}
}

void test_DrPciCalculator::overlapBoundaryExamples()
{
	verifyArea(12.0, 18.0, 6.0, 10.0, 20.0, 6.0);  // fully inside
	verifyArea(5.0, 15.0, 10.0, 10.0, 20.0, 5.0);  // crosses start
	verifyArea(15.0, 25.0, 10.0, 10.0, 20.0, 5.0); // crosses end
	verifyArea(0.0, 30.0, 30.0, 10.0, 20.0, 10.0); // covers segment
	verifyArea(0.0, 10.0, 10.0, 10.0, 20.0, 0.0);  // touches endpoint
	verifyArea(10.0, 10.0, 2.0, 10.0, 20.0, 2.0);  // zero length at start
	verifyArea(20.0, 20.0, 2.0, 10.0, 20.0, 0.0);  // half-open end
}

void test_DrPciCalculator::reverseDirectionProperty()
{
	std::mt19937 generator(20260714u);
	std::uniform_real_distribution<double> coordinate(-1000.0, 1000.0);
	for (int i = 0; i < 1000; ++i)
	{
		const double ds = coordinate(generator);
		const double de = coordinate(generator);
		const double ss = coordinate(generator);
		const double se = coordinate(generator);
		double forward = 0.0;
		double reverse = 0.0;
		const double fixedArea = 37.5;
		QVERIFY(hnDrPci::clippedWeightedArea(ds, de, fixedArea, 1.0, ss, se, forward));
		QVERIFY(hnDrPci::clippedWeightedArea(de, ds, fixedArea, 1.0, se, ss, reverse));
		QVERIFY(std::fabs(forward - reverse) < 1e-9);
	}
}

void test_DrPciCalculator::adjacentSegmentConservationProperty()
{
	std::mt19937 generator(52102018u);
	std::uniform_real_distribution<double> start(-20.0, 19.9);
	std::uniform_real_distribution<double> length(0.01, 60.0);
	for (int i = 0; i < 1000; ++i)
	{
		const double ds = start(generator);
		const double de = ds + length(generator);
		double whole = 0.0, left = 0.0, right = 0.0;
		QVERIFY(hnDrPci::clippedWeightedArea(ds, de, 25.0, 1.0, 0.0, 20.0, whole));
		QVERIFY(hnDrPci::clippedWeightedArea(ds, de, 25.0, 1.0, 0.0, 10.0, left));
		QVERIFY(hnDrPci::clippedWeightedArea(ds, de, 25.0, 1.0, 10.0, 20.0, right));
		QVERIFY(std::fabs(whole - left - right) < 1e-9);
	}
}

void test_DrPciCalculator::noDiseaseAndStandardPoints()
{
	double dr = -1.0;
	QVERIFY(hnDrPci::calculateDr(0.0, 1000.0, dr));
	QCOMPARE(dr, 0.0);

	double pci = 0.0;
	QVERIFY(hnDrPci::calculatePowerPci(dr, 15.0, 0.412, pci));
	QCOMPARE(pci, 100.0);

	const double asphaltDr[] = { 0.4, 2.0, 5.5, 11.0 };
	const double cementDr[] = { 0.8, 4.0, 9.5, 18.0 };
	const double expected[] = { 90.0, 80.0, 70.0, 60.0 };
	for (int i = 0; i < 4; ++i)
	{
		QVERIFY(hnDrPci::calculatePowerPci(asphaltDr[i], 15.0, 0.412, pci));
		QVERIFY(std::fabs(pci - expected[i]) < 0.6);
		QVERIFY(hnDrPci::calculatePowerPci(cementDr[i], 10.66, 0.461, pci));
		QVERIFY(std::fabs(pci - expected[i]) < 0.6);
	}

	QVERIFY(!hnDrPci::calculateDr(1.0, 0.0, dr));
	QVERIFY(!hnDrPci::calculateDr(1.0, -10.0, dr));
	QVERIFY(!hnDrPci::calculatePowerPci(1.0, 0.0, 0.412, pci));
}

void test_DrPciCalculator::interpolationClampsEndpoints()
{
	const QVector<double> density{ 0.0, 0.01, 0.10 };
	const QVector<double> deduction{ 0.0, 10.0, 50.0 };
	double result = -1.0;
	QVERIFY(hnDrPci::interpolateClamped(density, deduction, -1.0, result));
	QCOMPARE(result, 0.0);
	QVERIFY(hnDrPci::interpolateClamped(density, deduction, 0.10, result));
	QCOMPARE(result, 50.0);
	QVERIFY(hnDrPci::interpolateClamped(density, deduction, 1.0, result));
	QCOMPARE(result, 50.0);
}

void test_DrPciCalculator::cityStandardExamplesAndClamp()
{
	double pci = 0.0;
	QVERIFY(hnDrPci::calculateCityPciFromCategories(
		QVector<double>{ 5.24, 7.80, 11.08, 5.62 }, pci));
	QVERIFY(std::fabs(pci - 82.51) < 0.011);
	QVERIFY(hnDrPci::calculateCityPciFromCategories(
		QVector<double>{ 13.28, 6.79, 9.34, 7.54 }, pci));
	QVERIFY(std::fabs(pci - 78.37) < 0.011);
	QVERIFY(hnDrPci::calculateCityPciFromCategories(QVector<double>{ 200.0 }, pci));
	QCOMPARE(pci, 0.0);
}
