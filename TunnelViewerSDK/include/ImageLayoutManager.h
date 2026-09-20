#pragma once

#include "VirtualImageSequence.h"

/*
 * ImageSequenceModel 已经承担累计坐标、正反布局、名称索引和 O(logN) 定位。
 * 这里提供通用 SDK 名称，不复制模型、不改变现有布局算法。
 */
using ImageLayoutManager = ImageSequenceModel;
