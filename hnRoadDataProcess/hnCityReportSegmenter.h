#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Match the 2D city report's ascending stake-based partition, then reverse
// the boundaries for a descending survey. DMI conversion belongs to the caller.
class hnCityReportSegmenter
{
public:
    std::vector<double> split(double start, double end, double length,
        std::vector<double> unitBoundaries) const
    {
        std::vector<double> result;
        if (!std::isfinite(start) || !std::isfinite(end) || start == end)
        {
            return result;
        }
        if (!std::isfinite(length) || length <= 0)
        {
            length = 200;
        }
        const double low = (std::min)(start, end);
        const double high = (std::max)(start, end);
        unitBoundaries.erase(std::remove_if(unitBoundaries.begin(), unitBoundaries.end(),
            [](double value) { return !std::isfinite(value); }), unitBoundaries.end());
        std::sort(unitBoundaries.begin(), unitBoundaries.end());
        result.push_back(low);
        for (double boundary : unitBoundaries)
        {
            if (std::isfinite(boundary) && boundary > result.back() && boundary < high)
            {
                append(result, boundary, length);
            }
        }
        // The legacy rule checks the entire final road unit, not each remainder.
        if (high - result.back() < 500)
        {
            result.push_back(high);
        }
        else
        {
            append(result, high, length);
        }
        if (start > end)
        {
            std::reverse(result.begin(), result.end());
        }
        return result;
    }

private:
    void append(std::vector<double>& result, double end, double length) const
    {
        while (result.back() < end)
        {
            const double next = (std::min)(end, result.back() + length);
            if (next <= result.back())
            {
                result.push_back(end);
                break;
            }
            result.push_back(next);
        }
    }
};
