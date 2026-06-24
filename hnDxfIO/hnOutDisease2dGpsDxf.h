#pragma once

#include "..\hnDxfIO\dl_dxf.h"
#include "..\hnDxfIO\dl_creationadapter.h"
#include "..\hnDxfIO\test_creationclass.h"
#include "..\hnDxfIO\dl_creationadapter.h"
#include <vector>
#include "../hnCommon/hn2dDiseaseDef.h"
#include "../hnCommon/hn2dPointDef.h"
class hnOutDisease2dGpsDxf
{
public:
	hnOutDisease2dGpsDxf();
	~hnOutDisease2dGpsDxf();
	/*
	*接口名称：OutputXRDxf
	*功能：等级公路2018 人工模式 输出病害CAD图像
	*参数1：输出的文件路径
	*参数2：传入的病害信息 包含病害名称 病害四个点三维数据 桩号
	*disType:病害绘制类型
	*/
	bool outDiseaseDxf(const char* filePath, std::vector<hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>*> diss, int disType); 
private:
	// 初始化CAD 做一些必要准备工作
	void initCAD(DL_Dxf* dxf, DL_WriterA* dw);

	//简单的哈希函数,用于根据文字 生成颜色
	unsigned int simpleHash(const std::string&str);
};

