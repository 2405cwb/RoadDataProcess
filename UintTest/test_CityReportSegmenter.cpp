#include "../hnRoadDataProcess/hnCityReportSegmenter.h"
#include <cassert>
#include <iostream>
#include <limits>

int main()
{
    hnCityReportSegmenter splitter;
    assert(splitter.split(35, 835, 200, {}) == std::vector<double>({35,235,435,635,835}));
    assert(splitter.split(835, 35, 200, {}) == std::vector<double>({835,635,435,235,35}));
    assert(splitter.split(35, 234, 200, {}) == std::vector<double>({35,234}));
    assert(splitter.split(35, 534, 200, {}) == std::vector<double>({35,534}));
    assert(splitter.split(35, 535, 200, {}) == std::vector<double>({35,235,435,535}));
    assert(splitter.split(35, 536, 200, {}) == std::vector<double>({35,235,435,536}));
    assert(splitter.split(35, 1235, 200, {350,800}) ==
        std::vector<double>({35,235,350,550,750,800,1235}));
    assert(splitter.split(1235, 35, 200, {350,800}) ==
        std::vector<double>({1235,800,750,550,350,235,35}));
    assert(splitter.split(350, 850, 200, {0,350,350,850,900}) ==
        std::vector<double>({350,550,750,850}));
    assert(splitter.split(0, 600, 0, {}) == std::vector<double>({0,200,400,600}));
    assert(splitter.split(0, 600, -20, {}) == std::vector<double>({0,200,400,600}));
    assert(splitter.split(600, 600, 200, {}).empty());
    assert(splitter.split(0, 600, 1000, {}) == std::vector<double>({0,600}));
    assert(splitter.split(-535, -35, 200, {}) == std::vector<double>({-535,-335,-135,-35}));
    const double nan = std::numeric_limits<double>::quiet_NaN();
    assert(splitter.split(nan, 600, 200, {}).empty());
    assert(splitter.split(0, 600, nan, {nan}) == std::vector<double>({0,200,400,600}));
    // Short/long-chain mapping changes DMI lengths, never the chosen stake grid.
    const auto boundaries = splitter.split(0, 1000, 200, {500});
    double totalDmi = 0;
    for (size_t i = 1; i < boundaries.size(); ++i)
    {
        assert(boundaries[i] > boundaries[i - 1]);
        const double startDmi = boundaries[i - 1] * 1.1;
        const double endDmi = boundaries[i] * 1.1;
        totalDmi += endDmi - startDmi;
    }
    assert(std::abs(totalDmi - 1100) < 1e-8);
    std::cout << "City report segmentation: 17 scenarios passed\n";
}
