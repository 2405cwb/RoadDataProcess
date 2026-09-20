#pragma once

#include <QImage>
#include <cmath>

// 显示用亮度曲线，不修改传入原图；零位保持原始像素。
class hnImageBrightness
{
public:
    // value 范围 -100～300，正值提亮、负值压暗，返回独立调整后的图像。
    QImage adjust(const QImage& source, int value) const
    {
        if (source.isNull() || value == 0)
        {
            return source;
        }
        const double gamma = std::pow(2.0, -qBound(-100, value, 300) / 100.0);
        int table[256];
        for (int i = 0; i < 256; ++i)
        {
            table[i] = qBound(0, qRound(255.0 * std::pow(i / 255.0, gamma)), 255);
        }
        // 统一为非预乘格式，保留透明度，避免通道次序与行对齐问题。
        QImage result = source.convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < result.height(); ++y)
        {
            QRgb* row = reinterpret_cast<QRgb*>(result.scanLine(y));
            for (int x = 0; x < result.width(); ++x)
            {
                const QRgb pixel = row[x];
                row[x] = qRgba(table[qRed(pixel)], table[qGreen(pixel)],
                    table[qBlue(pixel)], qAlpha(pixel));
            }
        }
        return result;
    }
};
