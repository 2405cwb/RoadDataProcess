#pragma once
#include "stdafx.h"
#include <stdlib.h>
#include <string>
#include "jpeglib.h"
#include <setjmp.h>
#include<iostream>
#include<fstream>
using namespace std;

/*
* 接口描述：加载图片，传入图片的路径 和图片的宽高
*/
extern "C" __declspec(dllexport) int YG_LoadImg(const char* fpath, int* imgw, int* imgh);


/*
* 获取图片的二进制流，原始图片的流，未经过处理的
*/
extern "C" __declspec(dllexport) void YG_GetImgBuf(BYTE* destbits);

/*
* 获取图片的二进制流，经过处理的图片流
*/
extern "C" __declspec(dllexport) void YG_GetImgBufNew(BYTE* destbits);


/*
* 描述：从图像获取照明分析参数
* 参数：图像的首地址  2023年9月11日陈智超：哪里有参数？
*/
extern "C" __declspec(dllexport) void YG_ZmGen();

/*
* 设置亮度对比度，
* 关于width  在二维项目c#代码中是这样设置的，可以参照这个来做 (m_imgwidth + 3) / 4 * 4
*/
extern "C" __declspec(dllexport) void YG_SetPara(int ld, int dbd, int width);


/*
* 设置参数之后更新imgBuffNew
*/
extern "C" __declspec(dllexport) void YG_Recover();

//根据亮度和对比度调整图片
extern "C" __declspec(dllexport) void YG_Adjust2();

//设置模式 0：不做改动   1：全局灰度校正   2：纵向灰度校正
extern "C" __declspec(dllexport) int YG_Mode(int mode);


//执行锐化效果
extern "C" __declspec(dllexport) void YG_Ruihua2();

//进行锐化设置   rh:是否支持锐化 把它看成bool   pr：锐化半径   pd:锐化强度
extern "C" __declspec(dllexport) int YG_RH_Set(int rh, int* pr, int* pd);

