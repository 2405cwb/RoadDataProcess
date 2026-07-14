#pragma once

#include <algorithm>
#include <cmath>
#include <QVector>

namespace hnDrPci
{
	static const double kEpsilon = 1e-9;

	inline bool isFinite(double value)
	{
		return std::isfinite(value) != 0;
	}

	inline double clampScore(double value)
	{
		if (!isFinite(value)) return 0.0;
		return (std::max)(0.0, (std::min)(100.0, value));
	}

	inline double intervalOverlapLength(double firstStart, double firstEnd,
		double secondStart, double secondEnd)
	{
		const double firstMin = (std::min)(firstStart, firstEnd);
		const double firstMax = (std::max)(firstStart, firstEnd);
		const double secondMin = (std::min)(secondStart, secondEnd);
		const double secondMax = (std::max)(secondStart, secondEnd);
		return (std::max)(0.0, (std::min)(firstMax, secondMax) - (std::max)(firstMin, secondMin));
	}

	// A zero-length disease is a point in [segmentMin, segmentMax), so a shared
	// boundary cannot be counted by both adjacent segments.
	inline bool clippedWeightedArea(double diseaseStart, double diseaseEnd,
		double diseaseArea, double diseaseWeight, double segmentStart, double segmentEnd,
		double& weightedArea)
	{
		weightedArea = 0.0;
		if (!isFinite(diseaseStart) || !isFinite(diseaseEnd) ||
			!isFinite(diseaseArea) || !isFinite(diseaseWeight) ||
			!isFinite(segmentStart) || !isFinite(segmentEnd) ||
			diseaseArea < 0.0 || diseaseWeight < 0.0)
		{
			return false;
		}

		const double segmentMin = (std::min)(segmentStart, segmentEnd);
		const double segmentMax = (std::max)(segmentStart, segmentEnd);
		if (segmentMax - segmentMin <= kEpsilon) return false;

		const double diseaseLength = std::fabs(diseaseEnd - diseaseStart);
		if (diseaseLength <= kEpsilon)
		{
			if (diseaseStart >= segmentMin && diseaseStart < segmentMax)
				weightedArea = diseaseArea * diseaseWeight;
			return isFinite(weightedArea);
		}

		const double overlap = intervalOverlapLength(diseaseStart, diseaseEnd, segmentStart, segmentEnd);
		weightedArea = diseaseArea * diseaseWeight * overlap / diseaseLength;
		return isFinite(weightedArea);
	}

	inline bool calculateDr(double weightedDamageArea, double surveyArea, double& dr)
	{
		dr = 0.0;
		if (!isFinite(weightedDamageArea) || !isFinite(surveyArea) ||
			weightedDamageArea < 0.0 || surveyArea <= kEpsilon)
			return false;
		dr = 100.0 * weightedDamageArea / surveyArea;
		return isFinite(dr);
	}

	inline bool calculatePowerPci(double dr, double a0, double a1, double& pci)
	{
		pci = 0.0;
		if (!isFinite(dr) || !isFinite(a0) || !isFinite(a1) ||
			dr < 0.0 || a0 <= 0.0 || a1 <= 0.0)
			return false;
		pci = clampScore(100.0 - a0 * std::pow(dr, a1));
		return true;
	}

	inline double cityWeight(double ratio)
	{
		return ((3.0 * ratio - 5.5) * ratio + 3.5) * ratio;
	}

	inline bool calculateCityPciFromCategories(const QVector<double>& categoryDeductions,
		double& pci)
	{
		pci = 0.0;
		double total = 0.0;
		for (int i = 0; i < categoryDeductions.size(); ++i)
		{
			if (!isFinite(categoryDeductions[i]) || categoryDeductions[i] < 0.0)
				return false;
			total += categoryDeductions[i];
		}
		if (!isFinite(total)) return false;
		if (total <= kEpsilon) { pci = 100.0; return true; }

		double weightedDeduction = 0.0;
		for (int i = 0; i < categoryDeductions.size(); ++i)
		{
			const double ratio = categoryDeductions[i] / total;
			weightedDeduction += categoryDeductions[i] * cityWeight(ratio);
		}
		pci = clampScore(100.0 - weightedDeduction);
		return isFinite(pci);
	}

	inline bool interpolateClamped(const QVector<double>& density,
		const QVector<double>& deduction, double value, double& result)
	{
		result = 0.0;
		if (density.size() != deduction.size() || density.isEmpty() || !isFinite(value))
			return false;
		for (int i = 0; i < density.size(); ++i)
		{
			if (!isFinite(density[i]) || !isFinite(deduction[i]) ||
				(i > 0 && density[i] < density[i - 1]))
				return false;
		}
		if (value <= density.first()) { result = deduction.first(); return true; }
		if (value >= density.last()) { result = deduction.last(); return true; }

		for (int i = 1; i < density.size(); ++i)
		{
			if (value > density[i]) continue;
			const double span = density[i] - density[i - 1];
			if (span <= kEpsilon)
				result = deduction[i];
			else
				result = deduction[i - 1] + (value - density[i - 1]) / span
					* (deduction[i] - deduction[i - 1]);
			return isFinite(result);
		}
		return false;
	}
}
