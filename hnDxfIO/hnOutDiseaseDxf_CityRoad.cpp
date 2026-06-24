#include "StdAfx.h"
#include "hnOutDiseaseDxf_CityRoad.h"
//道路宽（不算路肩）
#define ROAD_LENGTH_Y m_nPixelWidth * 3.75
//路肩宽
#define SHOULDER_LENGTH_Y 0//m_nPixelWidth*1.5
//相邻两车道之间的空白宽
#define BLANK_BETWEEN_RAOD_Y m_nPixelWidth*7
//道路长
#define SCALE_ROAD_X 10*m_nPixelWidth
//道路车道文字偏移X（1车道、2车道）
#define ROAD_TEXT_POS_X -0.8*m_nPixelWidth
//道路车道文字大小（1车道、2车道）
#define ROAD_TEXT_LENGTH 0.4*m_nPixelWidth
//道路刻度线文字偏移Y（+100，+200）
#define MILE_TEXT_POS_Y 3*m_nPixelWidth
//道路刻度线文字长度（+100，+200）
#define MILE_TEXT_LENGTH 1*m_nPixelWidth
//最下面车道的起始Y（不包括符号库长度）
#define ROAD_OFFSET_Y 12*m_nPixelWidth
//最下面车道的起始X（不包括符号库长度）
#define DRAW_ROAD_X 14*m_nPixelWidth
#define DRAW_ROAD_Y 10*m_nPixelWidth
//符号库单行符号的宽度
#define DRAW_ROAD_SYMBOL_LINE 2.5*m_nPixelWidth
//A3边框缩放比例
#define MAINFORM_A3 0.0139
//最上面车道的Y，额外空白的宽
#define MAINFRAME_EXTRA_TOP 0
#define NOT_CREATE_PATTEN 1  //线裂
ofstream filef("E://dota00.txt");

hnOutDiseaseDxf_CityRoad::hnOutDiseaseDxf_CityRoad(void)
{
	// 单位长度
	m_nPixelWidth = 1;

	m_nSymbolWidth = 2.0*m_nPixelWidth;
	m_nSymbolHeight = 2.0*m_nPixelWidth;

	// 框架长宽
	m_nMainWidth = 127 * m_nPixelWidth;
	m_nMainHeight = 55 * m_nPixelWidth;

	m_nCenterType = 0;
	m_nLineType = 0;
	m_roadRealWidth = 3.75;
	symbolLine = 2;
	
}


hnOutDiseaseDxf_CityRoad::~hnOutDiseaseDxf_CityRoad(void)
{
}
static int disease_count = 0;
bool hnOutDiseaseDxf_CityRoad::outDiseaseDxf(const char* outPath, hnGridDiseaseInfo gridDisease)
{
	// Test
	DL_Dxf* dxf = new DL_Dxf();
	DL_Codes::version exportVersion = DL_Codes::AC1015;

	DL_WriterA* dw = dxf->out(outPath, exportVersion);
	if (dw == NULL)
	{
		MessageBox(NULL, L"dxf无法打开", NULL, MB_OK);
		return false;
	}
	m_gridDisease = gridDisease;

	// 初始化
	initCAD(dxf, dw);
	initSymbolBlock(dxf, dw);//需要先计算图例框有多少行
	initMainFrameBlock(dxf, dw);
	initDrawSymbolBlock(dxf, dw);
	initRoadBlock(dxf, dw);
	initRemarks(dxf, dw);

	// 绘制对象
	drawMainFrame(dxf, dw, 0, 0, 1, 1);
	drawRoad(dxf, dw, DRAW_ROAD_X, DRAW_ROAD_SYMBOL_LINE*symbolLine + ROAD_OFFSET_Y, 1, 1);
	drawSymbol(dxf, dw, 45 * m_nPixelWidth, DRAW_ROAD_SYMBOL_LINE*symbolLine + 5.5*m_nPixelWidth, 1, 1);

	drawRemark(gridDisease.strBegMile, gridDisease.strEndMile, m_nLineType, dxf, dw);

	// 病害块名称
	char strBlock[256] = { 0 };

	// 绘制病害
	for (int i = 0; i < gridDisease.vecDiseaseInfo.size(); i++)
	{  
		
		memset(strBlock, 0, 256);
		//sprintf(strBlock, "病害%d", i);
		sprintf(strBlock, "病害%d", disease_count + i);
		filef << "绘制病害" << gridDisease.vecDiseaseInfo.size() << "__" << strBlock << endl;
		drawDisease(strBlock, gridDisease.vecDiseaseInfo[i], dxf, dw);
	}
	disease_count += gridDisease.vecDiseaseInfo.size();
	// 写入dxf
	dxf->writeObjects(*dw);
	dxf->writeObjectsEnd(*dw);

	dw->dxfEOF();
	dw->close();
	delete dw;
	dw = NULL;
	delete dxf;
	dxf = NULL;
	return true;
}

// 设置中心线类型
void hnOutDiseaseDxf_CityRoad::setCenterType(int nCenterType, int nLineType)
{
	m_nCenterType = nCenterType;
	m_nLineType = nLineType;

}


// 初始化CAD
void hnOutDiseaseDxf_CityRoad::initCAD(DL_Dxf* dxf, DL_WriterA* dw)
{
	if (dw == NULL)
		printf("Cannot open file 'file.dxf' for writing.");

	dxf->writeHeader(*dw);
	dw->sectionEnd();
	dw->sectionTables();
	dxf->writeVPort(*dw);
	dw->tableLinetypes(8);
	dxf->writeLinetype(*dw, DL_LinetypeData("BYBLOCK", "BYBLOCK", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("BYLAYER", "BYLAYER", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("CONTINUOUS", "Continuous", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("DASHED", "DASHED", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("DASHED2", "DASHED2", 0, 0, 0.0));
	double patternCenter[4] = { 31.75 ,-6.349999999999997 ,6.349999999999997 ,-6.349999999999997 };
	dxf->writeLinetype(*dw, DL_LinetypeData("CENTER", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____", 0/*70*/, 4/*73*/, 50.8/*40*/, patternCenter));
	double patternDot[4] = { 24.0 ,-3.0 ,0.0 ,-3.0 };
	dxf->writeLinetype(*dw, DL_LinetypeData("ACAD_ISO04W100", "ISO long-dash dot ____ . ____ . ____ . ____ . _", 0/*70*/, 4/*73*/, 30.0/*40*/, patternDot));

	double patternCenter2[4] = { 0.4 ,-0.1 ,0.1 ,-0.1 };
	dxf->writeLinetype(*dw, DL_LinetypeData("CENTERBLOCK", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____", 0/*70*/, 4/*73*/, 0.7/*40*/, patternCenter2));
	dw->tableEnd();

	int numberOfLayers = 12;
	dw->tableLayers(numberOfLayers);

	//0必须存在
	dxf->writeLayer(*dw,
		DL_LayerData("0", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			256,
			"CONTINUOUS", 1.0));

	//红色
	dxf->writeLayer(*dw,
		DL_LayerData("Red", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::red,
			256,
			"CONTINUOUS", 1.0));

	//蓝色
	dxf->writeLayer(*dw,
		DL_LayerData("Blue", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::blue,
			30,
			"CONTINUOUS", 1.0));

	//绿色
	dxf->writeLayer(*dw,
		DL_LayerData("Green", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::green,
			30,
			"CONTINUOUS", 1.0));

	//黑色
	dxf->writeLayer(*dw,
		DL_LayerData("Black", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			30.0,
			"CONTINUOUS", 1.0));

	//黑色
	dxf->writeLayer(*dw,
		DL_LayerData("MainFrame", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			1.0,
			"CONTINUOUS", 1.0));

	// 
	dxf->writeLayer(*dw,
		DL_LayerData("Test", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			-1,
			"CONTINUOUS", 1.0));

	// 
	dxf->writeLayer(*dw,
		DL_LayerData("Test1", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			20,
			"CONTINUOUS", 1.0));

	dxf->writeLayer(*dw,
		DL_LayerData("Test2", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			1.0,
			"CENTERBLOCK", 1.0));

	dxf->writeLayer(*dw,
		DL_LayerData("Test3", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			0.8,
			"CENTERBLOCK", 1.0));

	////白色
	//dxf->writeLayer(*dw,
	//	DL_LayerData("White", 0),
	//	DL_Attributes(
	//		std::string(""),
	//		DL_Codes::white,
	//		100,
	//		"CONTINUOUS", 1.0));

	// 
	dxf->writeLayer(*dw,
		DL_LayerData("Test5", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			30,
			"CONTINUOUS", 1.0));

	dxf->writeLayer(*dw,
		DL_LayerData("Test6", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			50,
			"CONTINUOUS", 1.0));


	dw->tableEnd();

	dw->tableStyle(1);
	DL_StyleData style("Standard", 0, 0.0, 1, 0.0, 0, 2.0, "宋体", "");
	style.bold = false;
	style.italic = false;
	dxf->writeStyle(*dw, style);
	dw->tableEnd();

	dxf->writeView(*dw);
	dxf->writeUcs(*dw);

	dw->tableAppid(1);
	dxf->writeAppid(*dw, "ACAD");
	dw->tableEnd();

	dxf->writeDimStyle(*dw, 1, 1, 1, 1, 1);

	dxf->writeBlockRecord(*dw);

	if (m_gridDisease.vecDiseaseInfo.size() > 0)
	{
		int diseaseNum = m_gridDisease.vecDiseaseInfo[0]->nDiseaseSum - NOT_CREATE_PATTEN;
		for (int i = 0; i < diseaseNum; i++)
		{
			dxf->writeBlockRecord(*dw, m_gridDisease.vecDiseaseInfo[0]->getBlockName(i));
			filef << "声明Block:" << m_gridDisease.vecDiseaseInfo[0]->getBlockName(i) << endl;
		}
		dxf->writeBlockRecord(*dw, "图例");
		dxf->writeBlockRecord(*dw, "线裂");
	}
	dxf->writeBlockRecord(*dw, "符号线框");
	dxf->writeBlockRecord(*dw, "边框1");
	dxf->writeBlockRecord(*dw, "边框2");
	dxf->writeBlockRecord(*dw, "边框3");
	dxf->writeBlockRecord(*dw, "备注1");
	//dxf->writeBlockRecord(*dw, "备注2");
	dxf->writeBlockRecord(*dw, "车道1");
	dxf->writeBlockRecord(*dw, "车道2");

	// 病害块名称
	char strBlock[256] = { 0 };

	// 绘制病害
	for (int i = 0; i < m_gridDisease.vecDiseaseInfo.size(); i++)
	{
		string strBlockName = m_gridDisease.vecDiseaseInfo[i]->getBlockName();
		if (strBlockName == "")
		{
			filef << "初始化病害失败" << strBlock << "Type:" << m_gridDisease.vecDiseaseInfo[i]->nDiseaseType << endl;
			continue;
		}

		memset(strBlock, 0, 256);
		sprintf(strBlock, "病害%d", i + disease_count);
		filef << "初始化病害" << strBlock << endl;
		if ((m_gridDisease.vecDiseaseInfo[i]->getDiseaseType() == LIQING_ROAD_TYPE &&
			m_gridDisease.vecDiseaseInfo[i]->nDiseaseType != hnLiqingDiseaseInfo_CityRoad::XL_DISEASE_TYPE
			) || (m_gridDisease.vecDiseaseInfo[i]->getDiseaseType() == SHUINI_ROAD_TYPE&&
				m_gridDisease.vecDiseaseInfo[i]->nDiseaseType !=hnShuiNiDiseaseInfo_CityRoad::XL_SN_DISEASE_TYPE
				))
		{
			dxf->writeBlockRecord(*dw, strBlock);
			filef << "声明Block:" << m_gridDisease.vecDiseaseInfo.size() << "__" << strBlock << endl;
		}
	}
	
	dw->tableEnd();

	dw->sectionEnd();

	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("*Model_Space", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Model_Space");
	dxf->writeBlock(*dw, DL_BlockData("*Paper_Space", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Paper_Space");
	dxf->writeBlock(*dw, DL_BlockData("*Paper_Space0", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Paper_Space0");
	dw->sectionEnd();
}

// 初始化绘制图例模块
void hnOutDiseaseDxf_CityRoad::initDrawSymbolBlock(DL_Dxf* dxf, DL_WriterA* dw)
{
	double dXScale = 1.0 / 7.0;
	double dYScale = 1.0 / 5.0;

	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("符号线框", 0, 0.0, 0.0, 0.0));
	dxf->writeLine(*dw, DL_LineData(0.0, 0.0, 0.0, 1.0, 0.0, 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(1.0, 0.0, 0.0, 1.0, 1.0, 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(1.0, 1.0, 0.0, 0.0, 1.0, 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(0.0, 1.0, 0.0, 0.0, 0.0, 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeEndBlock(*dw, "符号线框");
	dw->sectionEnd();
}

// 初始化图例块模型
void hnOutDiseaseDxf_CityRoad::initSymbolBlock(DL_Dxf* dxf, DL_WriterA* dw)
{
	double dXScale = 0.0;
	double dYScale = 0.0;

	if (m_gridDisease.vecDiseaseInfo.size() == 0)
	{
		return;
	}
	int diseaseNum = m_gridDisease.vecDiseaseInfo[0]->nDiseaseSum;
	for (int i = 0; i < diseaseNum - NOT_CREATE_PATTEN; i++)
	{
		string diseaseName = m_gridDisease.vecDiseaseInfo[0]->getBlockName(i);
		dw->sectionBlocks();
		dxf->writeBlock(*dw, DL_BlockData(diseaseName, 0, 0.0, 0.0, 0.0));
		filef << "定义Block：" << diseaseName << endl;
		dXScale = 1.0 / 7.0;
		dYScale = 1.0 / 5.0;
		if (i == 0)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "*", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 1)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "#", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 2)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*-0.12, dYScale*3.3, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.28, dYScale*3.3, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.66, dYScale*3.3, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.04, dYScale*3.3, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.427, dYScale*3.3, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*-0.12, dYScale*0.8, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.28, dYScale*0.8, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.66, dYScale*0.8, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.04, dYScale*0.8, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.427, dYScale*0.8, 0, 0, 0, 0, dXScale*1.20, 1, 0, 0, 0, "△", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 3)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.1, dYScale*3.1, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.4, dYScale*3.1, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.7, dYScale*3.1, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.1, dYScale*0.7, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.4, dYScale*0.7, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.7, dYScale*0.7, 0, 0, 0, 0, dXScale*1.6, 1, 0, 0, 0, "※", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 4)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*-0.234, dYScale*3.3, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.17, dYScale*3.3, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.57, dYScale*3.3, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.97, dYScale*3.3, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.362, dYScale*3.3, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*-0.234, dYScale*0.86, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.17, dYScale*0.86, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.57, dYScale*0.86, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.97, dYScale*0.86, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.362, dYScale*0.86, 0, 0, 0, 0, dXScale*1.32, 1, 0, 0, 0, "∨", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 5)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.08, dYScale*3.35, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.82, dYScale*3.35, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.56, dYScale*3.35, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.30, dYScale*3.35, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*0.08, dYScale*0.85, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.82, dYScale*0.85, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.56, dYScale*0.85, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.30, dYScale*0.85, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "∽", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 6)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*-0.245, dYScale*3.28, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.15, dYScale*3.28, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.55, dYScale*3.28, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.95, dYScale*3.28, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.35, dYScale*3.28, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*-0.245, dYScale*0.8, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.15, dYScale*0.8, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.55, dYScale*0.8, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.95, dYScale*0.8, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.35, dYScale*0.8, 0, 0, 0, 0, dXScale*1.33, 1, 0, 0, 0, "◎", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 7)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.08, dYScale*3.3, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.82, dYScale*3.3, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.56, dYScale*3.3, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.30, dYScale*3.3, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*0.08, dYScale*0.8, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*1.82, dYScale*0.8, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.56, dYScale*0.8, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.30, dYScale*0.8, 0, 0, 0, 0, dXScale*1.2, 1, 0, 0, 0, "≈", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 8)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*-0.608, dYScale*3.05, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.7825, dYScale*3.05, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.165, dYScale*3.05, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.5475, dYScale*3.05, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.938, dYScale*3.05, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

			dxf->writeText(*dw, DL_TextData(dXScale*-0.60, dYScale*0.6, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.7825, dYScale*0.6, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.165, dYScale*0.6, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*3.5475, dYScale*0.6, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*4.98, dYScale*0.6, 0, 0, 0, 0, dXScale*1.9, 1, 0, 0, 0, "∪", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		//====新增 城镇
		else if (i == 9)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "M", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		else if (i == 10)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "&", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

		}
		else if (i == 11)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "§", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

		}
		else if (i == 12)
		{
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, "®", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

		}
		else if (i > 12)
		{
			char chNew[4];
			string str = "∪";
			memcpy(chNew, str.c_str(), 4);
			chNew[1] += i;

			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*3.0, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*0.4, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*2.8, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
			dxf->writeText(*dw, DL_TextData(dXScale*5.2, dYScale*0.6, 0, 0, 0, 0, dXScale*2.0, 1, 0, 0, 0, chNew, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		}
		dxf->writeEndBlock(*dw, diseaseName);
		dw->sectionEnd();
	}

	
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("线裂", 0, 0.0, 0.0, 0.0));
	dxf->writeLine(*dw, DL_LineData(0.0, 0.5, 0.0, 1.0, 0.5, 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeEndBlock(*dw, "线裂");
	dw->sectionEnd();


	// 图例
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("图例", 0, 0.0, 0.0, 0.0));
	dxf->writeText(*dw, DL_TextData(0.0, 1.2*m_nPixelWidth, 0, 0, 0, 0, 0.8*m_nPixelWidth, 1, 0, 0, 0, "图例", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
	double dNewSymbolWid = 3.0*m_nPixelWidth;
	int curIndex = 0;
	for (int i = 0; i < diseaseNum - NOT_CREATE_PATTEN; i++)
	{
		string diseaseName = m_gridDisease.vecDiseaseInfo[0]->getBlockName(i);
		dxf->writeInsert(*dw, DL_InsertData(diseaseName, (3.0 + (curIndex % 8) * 5)*m_nPixelWidth + 0.5*m_nPixelWidth, (2.0 - 2.5*(curIndex / 8))*m_nPixelWidth, 0.0, m_nSymbolWidth, m_nSymbolHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));
		dxf->writeInsert(*dw, DL_InsertData("符号线框", (3.0 + (curIndex % 8) * 5)*m_nPixelWidth, (2.0 - 2.5*(curIndex / 8))*m_nPixelWidth, 0.0, dNewSymbolWid, m_nSymbolHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));
		dxf->writeText(*dw, DL_TextData((6.3 + (curIndex % 8) * 5)*m_nPixelWidth, (2.8 - 2.5*(curIndex / 8))*m_nPixelWidth, 0, 0, 0, 0, 0.5*m_nPixelWidth, 1, 0, 0, 0, diseaseName, "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
		if (diseaseName.size() <= 4)//Test
		{
			curIndex++;
		}
		else
		{
			curIndex += 2;
		}
	}
	symbolLine = curIndex / 8 + 1;
	filef << "symbolLine" << symbolLine << endl;
	// 图例线框
	
	dxf->writeInsert(*dw, DL_InsertData("线裂", 43 * m_nPixelWidth, 2.95*m_nPixelWidth, 0.0, dNewSymbolWid, 1, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));
	
	// 图例名称
	dxf->writeText(*dw, DL_TextData(46.3*m_nPixelWidth, 2.8*m_nPixelWidth, 0, 0, 0, 0, 0.5*m_nPixelWidth, 1, 0, 0, 0, "线裂", "Standard", 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
	
	dxf->writeEndBlock(*dw, "图例");
	dw->sectionEnd();
}

// 初始化框架模块
void hnOutDiseaseDxf_CityRoad::initMainFrameBlock(DL_Dxf* dxf, DL_WriterA* dw)
{
	double singleRoadLength = (ROAD_LENGTH_Y*m_nRoadTotal + SHOULDER_LENGTH_Y + BLANK_BETWEEN_RAOD_Y) * 5;
	m_nMainHeight = singleRoadLength + DRAW_ROAD_SYMBOL_LINE*symbolLine + ROAD_OFFSET_Y + MAINFRAME_EXTRA_TOP;
	// 绘制外边框1
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("边框1", 0, 0.0, 0.0, 0.0));
	dxf->writeLine(*dw, DL_LineData(0, 0, 0, m_nMainWidth, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(m_nMainWidth, 0, 0, m_nMainWidth, m_nMainHeight, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(m_nMainWidth, m_nMainHeight, 0, 0, m_nMainHeight, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(0, m_nMainHeight, 0, 0, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 8.0));//2
	dxf->writeEndBlock(*dw, "边框1");
	dw->sectionEnd();

	// 绘制外边框3 A3纸的边缘
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("边框3", 0, 0.0, 0.0, 0.0));
	dxf->writeLine(*dw, DL_LineData(0, 0, 0, 140, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 2.0));//2
	dxf->writeLine(*dw, DL_LineData(140, 0, 0, 140, 99, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 2.0));//2
	dxf->writeLine(*dw, DL_LineData(140, 99, 0, 0, 99, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 2.0));//2
	dxf->writeLine(*dw, DL_LineData(0, 99, 0, 0, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 2.0));//2
	dxf->writeEndBlock(*dw, "边框3");
	dw->sectionEnd();

	// 绘制外边框2
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("边框2", 0, 0.0, 0.0, 0.0));

	//// 左边框-横向
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 0, 0, 0, 0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 3*m_nPixelWidth, 0, 0, 3*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 7*m_nPixelWidth, 0, 0, 7*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 11*m_nPixelWidth, 0, 0, 11*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 14*m_nPixelWidth, 0, 0, 14*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 18*m_nPixelWidth, 0, 0, 18*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 22*m_nPixelWidth, 0, 0, 22*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 25*m_nPixelWidth, 0, 0, 25*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 29*m_nPixelWidth, 0, 0, 29*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 33*m_nPixelWidth, 0, 0, 33*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2

	//// 左边框-纵向
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 0, 0, -2.0*m_nPixelWidth, 3*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 3*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 7*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 7*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 11*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 11*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 14*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 14*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 18*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 18*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 22*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 22*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 25*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 25*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 29*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2
	//dxf->writeLine(*dw, DL_LineData(-2.0*m_nPixelWidth, 29*m_nPixelWidth, 0, -2.0*m_nPixelWidth, 33*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));//2

	//// 左边框文字
	//dxf->writeMText(*dw, DL_MTextData(-0.01*m_nPixelWidth, 0.4*m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 0.6*m_nPixelWidth, m_nPixelWidth, 7, 3, 1, 1.0, "专业", "Standard;", (0.5)*3.1415926), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	//dxf->writeMText(*dw, DL_MTextData(-0.01*m_nPixelWidth, (0.4+3.5)*m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 0.6*m_nPixelWidth, m_nPixelWidth, 7, 3, 1, 1.0, "签名", "Standard;", (0.5)*3.1415926), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	//dxf->writeMText(*dw, DL_MTextData(-0.01*m_nPixelWidth, (0.4+7.5)*m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 0.6*m_nPixelWidth, m_nPixelWidth, 7, 3, 1, 1.0, "日期", "Standard;", (0.5)*3.1415926), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 底部边框
	dxf->writeLine(*dw, DL_LineData(0, 480.0*m_nPixelWidth, 0, 1680 * m_nPixelWidth, 480.0*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(1680.*m_nPixelWidth, 480.*m_nPixelWidth, 0, 1680.*m_nPixelWidth, 0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(1680.*m_nPixelWidth, 480.*m_nPixelWidth, 0, 4320 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(1680.*m_nPixelWidth, 240.*m_nPixelWidth, 0, 4320 * m_nPixelWidth, 240.*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(4320.*m_nPixelWidth, 480.*m_nPixelWidth, 0, 4320.*m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//1
	dxf->writeLine(*dw, DL_LineData(4320 * m_nPixelWidth, 480.*m_nPixelWidth, 0, 4800 * m_nPixelWidth, 480.*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(4320 * m_nPixelWidth, 240.*m_nPixelWidth, 0, 4800 * m_nPixelWidth, 240.*m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(4800 * m_nPixelWidth, 480.*m_nPixelWidth, 0, 4800 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//2
	dxf->writeLine(*dw, DL_LineData(4800 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 5280 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(4800 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 5280 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(5280 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 5280 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//3
	dxf->writeLine(*dw, DL_LineData(5280 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 5760 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(5280 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 5760 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(5760 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 5760 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//4
	dxf->writeLine(*dw, DL_LineData(5760 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 6240 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(5760 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 6240 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(6240 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 6240 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//5
	dxf->writeLine(*dw, DL_LineData(6240 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 6720 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(6240 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 6720 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(6720 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 6720 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//6
	dxf->writeLine(*dw, DL_LineData(6720 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 7200 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(6720 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 7200 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(7200 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 7200 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//7
	dxf->writeLine(*dw, DL_LineData(7200 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 7680 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(7200 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 7680 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(7680 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 7680 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//8
	dxf->writeLine(*dw, DL_LineData(7680 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 8160 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(7680 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 8160 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(8160 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 8160 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	//9
	dxf->writeLine(*dw, DL_LineData(8160 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 8640 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(8160 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 8640 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(8640 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 8640 * m_nPixelWidth, 0.0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 10
	dxf->writeLine(*dw, DL_LineData(8640 * m_nPixelWidth, 480 * m_nPixelWidth, 0, 9120 * m_nPixelWidth, 480 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeLine(*dw, DL_LineData(8640 * m_nPixelWidth, 240 * m_nPixelWidth, 0, 9120 * m_nPixelWidth, 240 * m_nPixelWidth, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 项目名称
	dxf->writeMText(*dw, DL_MTextData(2815 * m_nPixelWidth, 247 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "项目名称", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 任务号
	dxf->writeMText(*dw, DL_MTextData(4395 * m_nPixelWidth, 255 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "任务号", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 专业
	dxf->writeMText(*dw, DL_MTextData(4953 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "专业", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeMText(*dw, DL_MTextData(4953 * m_nPixelWidth, 24 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "路面", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 设计
	dxf->writeMText(*dw, DL_MTextData(5412 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "设计", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 复核
	dxf->writeMText(*dw, DL_MTextData(5892 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "复核", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 审核
	dxf->writeMText(*dw, DL_MTextData(6372 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "审核", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 图号
	dxf->writeMText(*dw, DL_MTextData(6852 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "图号", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 页码
	dxf->writeMText(*dw, DL_MTextData(7332 * m_nPixelWidth, 256, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "页码", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 版次
	dxf->writeMText(*dw, DL_MTextData(7812 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "版次", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));

	// 日期
	dxf->writeMText(*dw, DL_MTextData(8292 * m_nPixelWidth, 256 * m_nPixelWidth, 0.0, 0.0, 0.0, 0.0, 60 * m_nPixelWidth, 60 * m_nPixelWidth, 7, 3, 1, 1.0, "日期", "Standard;", 0.0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 8.0));
	dxf->writeEndBlock(*dw, "边框2");
	dw->sectionEnd();

}

// 初始化道路模块
void hnOutDiseaseDxf_CityRoad::initRoadBlock(DL_Dxf* dxf, DL_WriterA* dw)
{
	// 车道实现
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("车道1", 0, 0.0, 0.0, 0.0));

	for (int i = 0; i < 5; i++)
	{
		double roadWidth = SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*m_nRoadTotal;
		double startRoadPos = (roadWidth + BLANK_BETWEEN_RAOD_Y)*i;
		// 绘制400-500里程车道
		dxf->writeLine(*dw, DL_LineData(0.0, startRoadPos, 0, 10 * SCALE_ROAD_X, startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(0.0, startRoadPos + SHOULDER_LENGTH_Y, 0, 10 * SCALE_ROAD_X, startRoadPos + SHOULDER_LENGTH_Y, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(0.0, startRoadPos + roadWidth, 0, 10 * SCALE_ROAD_X, startRoadPos + roadWidth, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2

		dxf->writeLine(*dw, DL_LineData(0 * SCALE_ROAD_X, startRoadPos, 0, 0 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(1 * SCALE_ROAD_X, startRoadPos, 0, 1 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(2 * SCALE_ROAD_X, startRoadPos, 0, 2 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(3 * SCALE_ROAD_X, startRoadPos, 0, 3 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(4 * SCALE_ROAD_X, startRoadPos, 0, 4 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(5 * SCALE_ROAD_X, startRoadPos, 0, 5 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(6 * SCALE_ROAD_X, startRoadPos, 0, 6 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(7 * SCALE_ROAD_X, startRoadPos, 0, 7 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(8 * SCALE_ROAD_X, startRoadPos, 0, 8 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(9 * SCALE_ROAD_X, startRoadPos, 0, 9 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2
		dxf->writeLine(*dw, DL_LineData(10 * SCALE_ROAD_X, startRoadPos, 0, 10 * SCALE_ROAD_X, roadWidth + startRoadPos, 0), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));//2

																																											 // 绘制文本--车道信息
		if (SHOULDER_LENGTH_Y > 0)
		{
			dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + 0.27 * 6 * m_nPixelWidth, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, "硬路肩", "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
		}
		if (m_nLineType == 0)//上行
		{
			if (m_nRoadTotal == 1)
			{
				string str = "右幅";
				dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(m_nRoadTotal - 1) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
			}
			else if (m_nRoadTotal == 2)
			{
				string str = "右幅";
				dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(m_nRoadTotal - 2) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
				str = "左幅";
				dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(m_nRoadTotal - 1) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
			}
			else
			{
				for (int j = m_nRoadTotal; j > 0; j--)
				{
					string str = to_string((long long)j) + "车道";
					dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(m_nRoadTotal - j) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
				}
			}
		}
		else if (m_nLineType == 1)//下行
		{
			if (m_nRoadTotal == 1)
			{
				string str = "左幅";
				dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(m_nRoadTotal - 1) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
			}
			else
			{
				for (int j = m_nRoadTotal; j > 0; j--)
				{
					string str = to_string((long long)j) + "车道";
					dxf->writeText(*dw, DL_TextData(ROAD_TEXT_POS_X, startRoadPos + SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*(j - 1) + ROAD_LENGTH_Y / 4 * 3, 0, 0, 0, 0, ROAD_TEXT_LENGTH, 1, 0, 0, 0, str.c_str(), "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
				}
			}
		}

		// 绘制文本--里程信息
		for (int j = 0; j < 11; j++)
		{
			char ch[8]; string str;
			sprintf(ch, "%03d", (4 - i)*(m_divideMile / 5) + j*(m_divideMile / 5 / 10)); str = ch;
			dxf->writeText(*dw, DL_TextData(j*SCALE_ROAD_X - 0.7*m_nPixelWidth, startRoadPos + roadWidth + MILE_TEXT_POS_Y, 0, 0, 0, 0, MILE_TEXT_LENGTH, 1, 0, 0, 0, "+" + str, "Standard", -0.5*3.1415926), DL_Attributes("Test", 256, -1, "BYLAYER", 1.0));
		}


	}
	dxf->writeEndBlock(*dw, "车道1");
	dw->sectionEnd();


	// 车道分割线-虚线
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("车道2", 0, 0.0, 0.0, 0.0));
	double dDist = SCALE_ROAD_X * 10 / 50;
	double roadWidth = SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*m_nRoadTotal;
	double startRoadPos = (roadWidth + BLANK_BETWEEN_RAOD_Y);
	for (int i = 0; i < 50; i += 2)
	{
		for (int j = 0; j < m_nRoadTotal; j++)
		{
			dxf->writeLine(*dw, DL_LineData(i*dDist, startRoadPos * 0 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0, (i + 1)*dDist, startRoadPos * 0 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0), DL_Attributes("Test2", 256, -1, "BYBLOCK", 1.0));//2
			dxf->writeLine(*dw, DL_LineData(i*dDist, startRoadPos * 1 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0, (i + 1)*dDist, startRoadPos * 1 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0), DL_Attributes("Test2", 256, -1, "BYBLOCK", 1.0));//2
			dxf->writeLine(*dw, DL_LineData(i*dDist, startRoadPos * 2 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0, (i + 1)*dDist, startRoadPos * 2 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0), DL_Attributes("Test2", 256, -1, "BYBLOCK", 1.0));//2
			dxf->writeLine(*dw, DL_LineData(i*dDist, startRoadPos * 3 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0, (i + 1)*dDist, startRoadPos * 3 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0), DL_Attributes("Test2", 256, -1, "BYBLOCK", 1.0));//2
			dxf->writeLine(*dw, DL_LineData(i*dDist, startRoadPos * 4 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0, (i + 1)*dDist, startRoadPos * 4 + SHOULDER_LENGTH_Y + (j + 1)*ROAD_LENGTH_Y, 0), DL_Attributes("Test2", 256, -1, "BYBLOCK", 1.0));//2
		}
	}

	dxf->writeEndBlock(*dw, "车道2");
	dw->sectionEnd();
}

// 绘制主框架
void hnOutDiseaseDxf_CityRoad::drawMainFrame(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
	double dScaleX, double dScaleY)
{
	dw->sectionEntities();
	dxf->writeInsert(*dw, DL_InsertData("边框1", dx, dy, 0, dScaleX, dScaleY, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
	dxf->writeInsert(*dw, DL_InsertData("边框2", dx, dy, 0, MAINFORM_A3, MAINFORM_A3, 1, 0, 1, 1, 0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeInsert(*dw, DL_InsertData("边框3", dx - 10, dy - 5, 0, dScaleX, dScaleY, 1, 0, 1, 1, 0, 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dw->sectionEnd();
}

// 初始化说明模块
void hnOutDiseaseDxf_CityRoad::initRemarks(DL_Dxf* dxf, DL_WriterA* dw)
{
	// 备注
	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("备注1", 0, 0.0, 0.0, 0.0));
	dxf->writeText(*dw, DL_TextData(0.0, 0.0, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, "起点:", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	//dxf->writeText(*dw, DL_TextData(2.0*m_nPixelWidth, 0.0, 0, 0, 0, 0, 0.5*m_nPixelWidth, 1, 0, 0, 0, "K999+000", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeText(*dw, DL_TextData(12.0*m_nPixelWidth, 0.0, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, "止点:", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	//dxf->writeText(*dw, DL_TextData(8.0*m_nPixelWidth, 0.0, 0, 0, 0, 0, 0.5*m_nPixelWidth, 1, 0, 0, 0, "K999+500", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeText(*dw, DL_TextData(24.0*m_nPixelWidth, 0.0, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, "方向:", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	//dxf->writeText(*dw, DL_TextData(14.0*m_nPixelWidth, 0.0, 0, 0, 0, 0, 0.5*m_nPixelWidth, 1, 0, 0, 0, "下行", "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeEndBlock(*dw, "备注1");
	dw->sectionEnd();

}

// 绘制图例
void hnOutDiseaseDxf_CityRoad::drawSymbol(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
	double dScaleX, double dScaleY)
{
	dw->sectionEntities();
	dxf->writeInsert(*dw, DL_InsertData("图例", dx, dy, 0, dScaleX, dScaleY, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));

	double dBegX = dx + 42 * m_nPixelWidth;
	double dBegY = dy + 0.45*m_nPixelWidth;

	/*// 裂缝修补
	dxf->writeLine(*dw, DL_LineData(dBegX, dBegY, 0, dBegX + 0.2*m_nSymbolWidth, dBegY, 0), DL_Attributes("MainFrame", 256, -1.0, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(dBegX + 0.2*m_nSymbolWidth, dBegY, 0, dBegX + 0.8*m_nSymbolWidth, dBegY, 0.0), DL_Attributes("Test6", 256, -1.0, "BYLAYER", 8.0));//2
	dxf->writeLine(*dw, DL_LineData(dBegX + 0.8*m_nSymbolWidth, dBegY, 0, dBegX + 1.0*m_nSymbolWidth, dBegY, 0.0), DL_Attributes("MainFrame", 256, -1.0, "BYLAYER", 8.0));//2
	*/
	dw->sectionEnd();
}

// 绘制里程以及车道表示
void hnOutDiseaseDxf_CityRoad::drawRoad(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
	double dScaleX, double dScaleY)
{

	dw->sectionEntities();
	dxf->writeInsert(*dw, DL_InsertData("车道1", dx, dy, 0, dScaleX, dScaleY, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
	dxf->writeInsert(*dw, DL_InsertData("车道2", dx, dy, 0, dScaleX, dScaleY, 1, 0, 1, 1, 0, 0), DL_Attributes("Black", 256, -1, "BYLAYER", 1.0));
	dw->sectionEnd();
}

// 绘制病害
void hnOutDiseaseDxf_CityRoad::drawDisease(const char* strBlock,
	hnDiseaseInfoBase* diseaseInfo, DL_Dxf* dxf, DL_WriterA* dw)
{
	// 块名称
	string strBlockName = diseaseInfo->getBlockName();
	if (strBlockName == "")
	{
		filef << "写入病害失败" << strBlock << "Type:" << diseaseInfo->nDiseaseType << endl;
		return;
	}

	 if (diseaseInfo->getDiseaseType() == LIQING_ROAD_TYPE &&diseaseInfo->nDiseaseType == hnLiqingDiseaseInfo_CityRoad::XL_DISEASE_TYPE	
		|| diseaseInfo->getDiseaseType() == SHUINI_ROAD_TYPE &&diseaseInfo->nDiseaseType == hnShuiNiDiseaseInfo_CityRoad::XL_SN_DISEASE_TYPE)
	{
		vector<hn2dPt> vecPt;
		vecPt = diseaseInfo->vecPt;
		vector<hn2dPt> vecPtNew;
		double dx, dy;
		dx = dy = 0.0;

		for (int i = 0; i < vecPt.size(); i++)
		{
			convertCoord(vecPt[i].dx, vecPt[i].dy, dx, dy);
			vecPt[i].dx = dx;
			vecPt[i].dy = dy;
		}
		double dLBX, dLBY, dWidth, dHeight;
		getBoundary(diseaseInfo, dLBX, dLBY, dWidth, dHeight);
		
			vecPtNew.push_back(hn2dPt(dLBX, dLBY + dHeight / 2));
			vecPtNew.push_back(hn2dPt(dLBX + dWidth, dLBY + dHeight / 2));
		
		// 绘制病害
		dw->sectionEntities();
		for (int i = 0; i < vecPtNew.size() - 1; i++)
		{
			dxf->writeLine(*dw, DL_LineData(vecPtNew[i].dx, vecPtNew[i].dy, 0.0, vecPtNew[i + 1].dx, vecPtNew[i + 1].dy, 0.0), DL_Attributes("MainFrame", 256, -1.0, "BYLAYER", 8.0));//2
		}

		dw->sectionEnd();
	}
	else
	{
		//if( diseaseInfo->nDiseaseType< diseaseInfo->nDiseaseSum - NOT_CREATE_PATTEN)
		{
			double dx, dy;
			dx = dy = 0.0;
			for (int i = 0; i < diseaseInfo->vecPt.size(); i++)
			{
				convertCoord(diseaseInfo->vecPt[i].dx, diseaseInfo->vecPt[i].dy, dx, dy);

				diseaseInfo->vecPt[i].dx = dx;
				diseaseInfo->vecPt[i].dy = dy;
			}

			double dWidth, dHeight, dAngle;
			dWidth = dHeight = dAngle = 0.0;
			double m_nSymbolWidth_ = 1.0, m_nSymbolHeight_ = 1.0;
			// 原点
			hn2dPt ptOri;
			getRectInfo(diseaseInfo->vecPt, ptOri, dWidth, dHeight, dAngle);
			//dWidth = dWidth*m_nPixelWidth;
			//dHeight = dHeight*m_nPixelWidth;
			dw->sectionBlocks();
			dxf->writeBlock(*dw, DL_BlockData(strBlock, 0, 0.0, 0.0, 0.0));
			filef << "更新Block病害:" << strBlock << endl;
			double dWidthSub = dWidth*2. / 3.;
			// 分情况考虑
			if (dWidth > m_nSymbolWidth && dHeight < m_nSymbolHeight)
			{
				if (dWidth > dHeight)
				{
					double dValue = dWidth - dHeight;

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0.5*dValue, 0.0, 0, dHeight, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}
				else
				{
					double dValue = dHeight - (dWidth);

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0, dValue*0.5, 0, dWidth, dWidth, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}
			}
			else if (dWidth < m_nSymbolWidth && dHeight > m_nSymbolHeight)
			{
				if (dWidth > dHeight)
				{
					double dValue = dWidth - dHeight;

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0.5*dValue, 0.0, 0, dHeight, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}
				else
				{
					double dValue = dHeight - (dWidth);

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0, dValue*0.5, 0, dWidth, dWidth, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}
			}
			else if (dWidth < m_nSymbolWidth && dHeight < m_nSymbolHeight)
			{
				if (dWidth > dHeight)
				{
					double dValue = dWidth - dHeight;

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0.5*dValue, 0.0, 0, dHeight, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}
				else
				{
					double dValue = dHeight - dWidth;

					// 绘制病害
					dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					dxf->writeInsert(*dw, DL_InsertData(strBlockName, 0, dValue*0.5, 0, dWidth, dWidth, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
				}

			}
			else
			{
				int nRowCnt = dWidth / m_nSymbolWidth;
				int nColCnt = dHeight / m_nSymbolHeight;

				double dRow = dWidth - m_nSymbolWidth*nRowCnt;
				double dCol = dHeight - m_nSymbolHeight*nColCnt;

				// 绘制病害
				dxf->writeInsert(*dw, DL_InsertData("符号线框", 0.0, 0.0, 0, dWidth, dHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));

				for (int i = 0; i < nRowCnt; i++)
				{
					for (int j = 0; j < nColCnt; j++)
					{
						dxf->writeInsert(*dw, DL_InsertData(strBlockName, m_nSymbolWidth*i + dRow / 2.0, m_nSymbolHeight*j + dCol / 2.0, 0, m_nSymbolWidth, m_nSymbolHeight, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
					}
				}
			}

			dxf->writeEndBlock(*dw, strBlock);
			dw->sectionEnd();

			// 插入病害
			dw->sectionEntities();
			dxf->writeInsert(*dw, DL_InsertData(strBlock, ptOri.dx, ptOri.dy, 0, 1, 1, 1, dAngle, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
			dw->sectionEnd();

		}
	}

}

// 绘制备注
void hnOutDiseaseDxf_CityRoad::drawRemark(string strBegMileage, string strEndMileage, int nLineType,
	DL_Dxf* dxf, DL_WriterA* dw)
{
	string strLineType = "";
	if (nLineType == 0)
	{
		strLineType = "上行";
	}
	else
	{
		strLineType = "下行";
	}

	dw->sectionEntities();
	dxf->writeInsert(*dw, DL_InsertData("备注1", 12.7*m_nPixelWidth, 11.8*m_nPixelWidth, 0, 1.0, 1.0, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));
	//dxf->writeInsert(*dw, DL_InsertData("备注2", 104 * m_nPixelWidth, 11.8*m_nPixelWidth, 0, 1.0, 1.0, 1, 0, 1, 1, 0, 0), DL_Attributes("Test1", 256, -1, "BYLAYER", 1.0));

	// 里程以及上下行
	dxf->writeText(*dw, DL_TextData(16.0*m_nPixelWidth, 12.0*m_nPixelWidth, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, strBegMileage, "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeText(*dw, DL_TextData(28.0*m_nPixelWidth, 12.0*m_nPixelWidth, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, strEndMileage, "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));
	dxf->writeText(*dw, DL_TextData(40.0*m_nPixelWidth, 12.0*m_nPixelWidth, 0, 0, 0, 0, 0.9*m_nPixelWidth, 1, 0, 0, 0, strLineType, "Standard", 0), DL_Attributes("MainFrame", 256, -1, "BYLAYER", 1.0));

	dxf->writeLine(*dw, DL_LineData(16.3*m_nPixelWidth, 11.7*m_nPixelWidth, 0, 22.3*m_nPixelWidth, 11.7*m_nPixelWidth, 0), DL_Attributes("Test3", 256, -1, "BYLAYER", 1.0));//2
	dxf->writeLine(*dw, DL_LineData(28.0*m_nPixelWidth, 11.7*m_nPixelWidth, 0, 34.1*m_nPixelWidth, 11.7*m_nPixelWidth, 0), DL_Attributes("Test3", 256, -1, "BYLAYER", 1.0));//2
	dxf->writeLine(*dw, DL_LineData(40.0*m_nPixelWidth, 11.7*m_nPixelWidth, 0, 43.0*m_nPixelWidth, 11.7*m_nPixelWidth, 0), DL_Attributes("Test3", 256, -1, "BYLAYER", 1.0));//2
	dw->sectionEnd();
}

// 坐标转换--usertoscreen
void hnOutDiseaseDxf_CityRoad::convertCoord(double dx, double dy, double& outX, double& outY)
{
	double dTemp = dx;

	int nRowType = 0;
	if (dTemp < m_divideMile / 5)
	{
		nRowType = 4;
	}
	else if (dTemp >= m_divideMile / 5 && dTemp < m_divideMile / 5 * 2)
	{
		nRowType = 3;
		dTemp = dTemp - m_divideMile / 5;
	}
	else if (dTemp >= m_divideMile / 5 * 2 && dTemp < m_divideMile / 5 * 3)
	{
		nRowType = 2;
		dTemp = dTemp - m_divideMile / 5 * 2;
	}
	else if (dTemp >= m_divideMile / 5 * 3 && dTemp < m_divideMile / 5 * 4)
	{
		nRowType = 1;
		dTemp = dTemp - m_divideMile / 5 * 3;
	}
	else
	{
		nRowType = 0;
		dTemp = dTemp - m_divideMile / 5 * 4;
	}

	// X坐标
	outX = dTemp / (m_divideMile / 5.)*(10 * SCALE_ROAD_X) + DRAW_ROAD_X;

	dTemp = SHOULDER_LENGTH_Y + (m_nRoadTotal)*ROAD_LENGTH_Y;

	outY = dTemp - abs(dy) / m_roadRealWidth *ROAD_LENGTH_Y;

	outY = outY + DRAW_ROAD_SYMBOL_LINE*symbolLine + ROAD_OFFSET_Y + nRowType*(SHOULDER_LENGTH_Y + ROAD_LENGTH_Y*m_nRoadTotal + BLANK_BETWEEN_RAOD_Y);
	//if(m_nCenterType == 2)
	//{
	//	// 计算Y坐标--第一车道左侧
	//	dTemp = ROAD_LENGTH_Y // (2.5+3.75*2)/10.0*6.0*m_nPixelWidth;
	//}
	//if (dy > 0.0)
	//{
	//	outY = dTemp - abs(dy) / 10.0*6.0*m_nPixelWidth;
	//}
	//else
	//{
	//	outY = dTemp + abs(dy) / 10.0*6.0*m_nPixelWidth;
	//}

	//outY = outY + 10*m_nPixelWidth + nRowType*9.0*m_nPixelWidth;
}

// 获取包围盒
void hnOutDiseaseDxf_CityRoad::getBoundary(hnDiseaseInfoBase* diseaseInfo, double& dLBX, double& dLBY, double& dWidth, double& dHeight)
{
	double dMinX, dMinY, dMaxX, dMaxY;
	dMinX = dMinY = 10000000.0;
	dMaxX = dMaxY = -10000000.0;

	double dx, dy;
	dx = dy = 0.0;

	for (int i = 0; i < diseaseInfo->vecPt.size(); i++)
	{
		convertCoord(diseaseInfo->vecPt[i].dx, diseaseInfo->vecPt[i].dy, dx, dy);

		if (dMinX > dx)
		{
			dMinX = dx;
		}

		if (dMinY > dy)
		{
			dMinY = dy;
		}

		if (dMaxX < dx)
		{
			dMaxX = dx;
		}

		if (dMaxY < dy)
		{
			dMaxY = dy;
		}
	}

	dLBX = dMinX;
	dLBY = dMinY;
	dWidth = abs(dMaxX - dMinX);
	dHeight = abs(dMaxY - dMinY);
}

// 获取病害外边框的参数信息
void hnOutDiseaseDxf_CityRoad::getRectInfo(vector<hn2dPt>& vecPt, hn2dPt& ptOri,
	double& dWidth, double& dHeight, double& dAngle)
{
	// 点位0-1表示横向  点位1-2表示高度
	vector<hn2dPt> vecTemp;

	// 先判断横向和竖向
	if (abs(vecPt[0].dx - vecPt[1].dx) < abs(vecPt[2].dx - vecPt[1].dx))
	{
		// 点0和1为竖向
		vecTemp.push_back(vecPt[1]);
		vecTemp.push_back(vecPt[2]);
		vecTemp.push_back(vecPt[3]);
		vecTemp.push_back(vecPt[0]);

		vecPt = vecTemp;
		vecTemp.clear();
	}

	// 先判断横向
	if (vecPt[0].dx > vecPt[1].dx)
	{
		vecTemp.push_back(vecPt[1]);
		vecTemp.push_back(vecPt[0]);
		vecTemp.push_back(vecPt[3]);
		vecTemp.push_back(vecPt[2]);
	}
	else
	{
		vecTemp = vecPt;
	}

	// 再判断纵向
	if (vecTemp[1].dy > vecTemp[2].dy)
	{
		vecPt.clear();
		vecPt.push_back(vecTemp[3]);
		vecPt.push_back(vecTemp[2]);
		vecPt.push_back(vecTemp[1]);
		vecPt.push_back(vecTemp[0]);
	}
	else
	{
		vecPt = vecTemp;
	}



	dWidth = sqrt((vecPt[0].dx - vecPt[1].dx)*(vecPt[0].dx - vecPt[1].dx) +
		(vecPt[0].dy - vecPt[1].dy)*(vecPt[0].dy - vecPt[1].dy));

	dHeight = sqrt((vecPt[2].dx - vecPt[1].dx)*(vecPt[2].dx - vecPt[1].dx) +
		(vecPt[2].dy - vecPt[1].dy)*(vecPt[2].dy - vecPt[1].dy));

	ptOri.dx = vecPt[0].dx;
	ptOri.dy = vecPt[0].dy;

	// 获取角度
	dAngle = -1.0;

	if (vecPt[1].dx == vecPt[0].dx)
	{
		dAngle = 90.0;

		return;
	}


	dAngle = atan((vecPt[1].dy - vecPt[0].dy) / (vecPt[1].dx - vecPt[0].dx))*180.0 / 3.1415926;
}


