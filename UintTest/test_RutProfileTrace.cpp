#include "test_RutProfileTrace.h"

#include "../hnAlgorithm/hnComputeCUT.h"
#include <QtTest/QTest>
#include <cmath>
#include <vector>

namespace
{
	std::vector<float> createSyntheticRutProfile()
	{
		std::vector<float> values(2048, 0.0f);
		for (int i = 0; i < static_cast<int>(values.size()); ++i)
		{
			const double x = static_cast<double>(i);
			const double leftPit = -12.0 * std::exp(-std::pow((x - 520.0) / 150.0, 2.0));
			const double centerCrown = 8.0 * std::exp(-std::pow((x - 1024.0) / 170.0, 2.0));
			const double rightPit = -10.0 * std::exp(-std::pow((x - 1510.0) / 160.0, 2.0));
			values[i] = static_cast<float>(0.002 * x + leftPit + centerCrown + rightPit);
		}
		return values;
	}

	void compareFloatVectors(const std::vector<float>& left, const std::vector<float>& right)
	{
		QCOMPARE(left.size(), right.size());
		for (size_t i = 0; i < left.size(); ++i)
		{
			QCOMPARE(left[i], right[i]);
		}
	}
}

void test_RutProfileTrace::traceDoesNotChangeRutResult()
{
	const std::vector<float> original = createSyntheticRutProfile();
	std::vector<float> untracedHeight = original;
	std::vector<float> tracedHeight = original;
	std::vector<float> untracedFiltered(original.size(), 0.0f);
	std::vector<float> tracedFiltered(original.size(), 0.0f);
	int untracedIndexes[20] = { 0 };
	int tracedIndexes[20] = { 0 };
	float untracedValues[20] = { 0.0f };
	float tracedValues[20] = { 0.0f };
	float untracedLeft = 0.0f;
	float untracedRight = 0.0f;
	float untracedK = 0.0f;
	float tracedLeft = 0.0f;
	float tracedRight = 0.0f;
	float tracedK = 0.0f;

	hnComputeCUT untracedCalculator;
	const float untracedRut = untracedCalculator.computerut3(
		static_cast<int>(original.size()), untracedHeight.data(), untracedFiltered.data(),
		0, static_cast<int>(original.size()), untracedLeft, untracedRight, untracedK,
		33, untracedIndexes, untracedValues);

	hnRutProfileTrace trace;
	hnComputeCUT tracedCalculator;
	const float tracedRut = tracedCalculator.computerut3(
		static_cast<int>(original.size()), tracedHeight.data(), tracedFiltered.data(),
		0, static_cast<int>(original.size()), tracedLeft, tracedRight, tracedK,
		33, tracedIndexes, tracedValues, &trace);

	QCOMPARE(tracedRut, untracedRut);
	QCOMPARE(tracedLeft, untracedLeft);
	QCOMPARE(tracedRight, untracedRight);
	QCOMPARE(tracedK, untracedK);
	compareFloatVectors(tracedHeight, untracedHeight);
	compareFloatVectors(tracedFiltered, untracedFiltered);
	compareFloatVectors(trace.worldHeight, original);
	QCOMPARE(trace.filteredHeight.size(), original.size());
	QCOMPARE(trace.leftRut, tracedLeft);
	QCOMPARE(trace.rightRut, tracedRight);
	for (int i = 0; i < 5; ++i)
	{
		QCOMPARE(trace.featureIndexes[i], tracedIndexes[i]);
		QCOMPARE(trace.featureValues[i], tracedValues[i]);
	}
}
