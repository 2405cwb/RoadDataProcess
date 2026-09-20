#pragma once

#include "VirtualImageSequence.h"

/*
 * 第二阶段统一命名入口。
 * 现有 ISequenceFrameSource 不改名、不删除，新项目可直接使用 IImageSource。
 * 两者是同一个接口，因此不会出现两套解码链路。
 */
using IImageSource = ISequenceFrameSource;
using FileImageSource = FileSequenceFrameSource;
using PackImageSource = PackSequenceFrameSource;
