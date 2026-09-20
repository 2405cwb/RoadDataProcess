#pragma once

// 仅影响 SDK 显示缓存，不修改源图片、数据库或 Pack 内原始数据。
struct ImageDisplayAdjustments
{
    int brightness = 0; // [-100, 100]，0 表示不调整亮度
    int contrast = 100; // [0, 200]，100 表示原始对比度
    int sharpen = 0;    // [0, 100]，0 表示不锐化
};
