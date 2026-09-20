#pragma once

#include <QtCore/QtGlobal>
#include <cmath>

// 按行驶距离选择图片；与桩号增减方向、屏幕缩放和翻页方式无关。
class hnImageDistanceNavigation
{
public:
    // distance、interval 单位为米；无有效图片或参数时返回 -1。
    int imageIndex(double distance, double interval, int count) const
    {
        if (count <= 0 || !std::isfinite(distance) || !std::isfinite(interval) || interval <= 0.0)
        {
            return -1;
        }
        // 只消除浮点运算的边界尾差，不提前切换到下一段。
        const double index = std::floor(qMax(0.0, distance) / interval + 1e-9);
        return static_cast<int>(qMin(index, static_cast<double>(count - 1)));
    }

    // 景观主动翻页落在目标图片起点，避免携带路面浏览留下的段内偏移。
    double stepDistance(double distance, double interval, int count, int step) const
    {
        const int current = imageIndex(distance, interval, count);
        if (current < 0)
        {
            return 0.0;
        }
        const int target = qBound(0, current + step, count - 1);
        return target * interval;
    }
};
