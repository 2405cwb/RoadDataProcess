#pragma once
#include "..\hnDxfIO\dl_dxf.h"
#include "..\hnDxfIO\dl_creationadapter.h"
#include "..\hnDxfIO\test_creationclass.h"
#include "..\hnDxfIO\dl_creationadapter.h"
#include "hnDiseaseDxfDef.h"
class DXFLIB_EXPORT hnOutDiseaseDxf_CityRoad
{
public:
	hnOutDiseaseDxf_CityRoad(void);
	~hnOutDiseaseDxf_CityRoad(void);

public:
	bool outDiseaseDxf(const char* outPath, hnGridDiseaseInfo gridDisease);

	// 设置中心线类型
	void setCenterType(int nCenterType, int nLineType);

	//设置车道总数
	void setRoadTotal(int nRoadTotal) { m_nRoadTotal = nRoadTotal; };
	//设置车道检测宽度
	void setRoadWidth(double roadRealWidth) { m_roadRealWidth = roadRealWidth; };

	void setDivideMile(double divideMile) { m_divideMile = divideMile; };
	
private:
	// 初始化CAD
	void initCAD(DL_Dxf* dxf, DL_WriterA* dw);

	// 初始化绘制图例模块
	void initDrawSymbolBlock(DL_Dxf* dxf, DL_WriterA* dw);

	// 初始化图例块模型
	void initSymbolBlock(DL_Dxf* dxf, DL_WriterA* dw);

	// 初始化框架模块
	void initMainFrameBlock(DL_Dxf* dxf, DL_WriterA* dw);

	// 初始化道路模块
	void initRoadBlock(DL_Dxf* dxf, DL_WriterA* dw);

	// 初始化说明模块
	void initRemarks(DL_Dxf* dxf, DL_WriterA* dw);

	// 绘制主框架
	void drawMainFrame(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
		double dScaleX, double dScaleY);

	// 绘制图例
	void drawSymbol(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
		double dScaleX, double dScaleY);

	// 绘制里程以及车道表示
	void drawRoad(DL_Dxf* dxf, DL_WriterA* dw, double dx, double dy,
		double dScaleX, double dScaleY);

	// 绘制病害
	void drawDisease(const char* strBlock, hnDiseaseInfoBase* diseaseInfo, DL_Dxf* dxf, DL_WriterA* dw);

	// 绘制备注
	void drawRemark(string strBegMileage, string strEndMileage, int nLineType,
		DL_Dxf* dxf, DL_WriterA* dw);

	// 坐标转换--usertoscreen
	void convertCoord(double dx, double dy, double& outX, double& outY);

	// 获取包围盒
	void getBoundary(hnDiseaseInfoBase* diseaseInfo, double& dLBX, double& dLBY, double& dWidth, double& dHeight);

	// 获取病害外边框的参数信息
	void getRectInfo(vector<hn2dPt>& vecPt, hn2dPt& ptOri, double& dWidth, double& dHeight, double& dAngle);
private:
	double m_nSymbolWidth;
	double m_nSymbolHeight;

	double m_roadRealWidth;
	// 每一米的单位长度
	double m_nPixelWidth;

	// 框架长宽
	double m_nMainWidth;
	double m_nMainHeight;

	// 中心线类型 0---中心线为第一车道中心 1---中心线为第二车道中心 2---中心线为第一车道左侧
	int m_nCenterType;

	// 上下行
	int m_nLineType;
	

	//车道总数
	int m_nRoadTotal;
	// 
	hnGridDiseaseInfo m_gridDisease;

	//图例框行数
	int symbolLine;

	int m_divideMile;
	//病害总数量
	
};

