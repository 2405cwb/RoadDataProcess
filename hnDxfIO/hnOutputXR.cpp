#include"hnOutputXR.h"
#include "hnOutDiseaseDxf.h"
#include "hnOutputRoadDiseaseXR.h"
#include "hnOutDisease3dDxf.h" 
#include "hnOutDisease2dGpsDxf.h"
static hnOutDiseaseDxf m_dxf;


/*
*接口名称：OutputXRDxf
*功能：等级公路2018 人工模式 病害面积表 输出CAD
*参数1：输出的文件路径
*参数2：传入的病害信息
*参数3：分段病害信息
*参数4：上下行 大于0从小到大，小于0从大到小
*/
bool __stdcall OutputXRDxf(const char* filePath, std::vector<Disease_C> diseases, const GridDisease_C& gridDisease,int direction,int  roadType)
{
	hnGridDiseaseInfo gridDiseaseInfo;
	gridDiseaseInfo.strName = string(gridDisease.strName);
	gridDiseaseInfo.strBegMile = string(gridDisease.strBegMile);
	gridDiseaseInfo.strEndMile = string(gridDisease.strEndMile);
	gridDiseaseInfo.dBegMileage = gridDisease.dBegMileage;
	gridDiseaseInfo.dEndMileage = gridDisease.dEndMileage;
	gridDiseaseInfo.dRoadWidth = gridDisease.dRoadWidth;
	gridDiseaseInfo.nRoadTotalNum = gridDisease.nRoadTotalNum;

	hnOutputRoadDiseaseXR outputRoadDiseaseXR(500);
	outputRoadDiseaseXR.outDisease(filePath, diseases, gridDiseaseInfo, direction, roadType);
	return true;
}

 
bool __stdcall OutputDisease3dDxf(const char* filePath, std::vector<hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>*> diss,int diseaseType)
{
	hnOutDisease3dDxf dxf;
	return dxf.outDiseaseDxf(filePath,diss,diseaseType);
	 
}

bool __stdcall OutputDisease2dGpsDxf(const char* filePath, std::vector<hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>*> diss, int diseaseType)
{
	hnOutDisease2dGpsDxf dxf;
	return dxf.outDiseaseDxf(filePath, diss, diseaseType);

}

bool __stdcall OutputDiseaseLine3dDxf(const char* filePath, std::vector<hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>*> diss, int diseaseType)
{
	hnOutDisease3dDxf dxf;
	return dxf.outDiseaseLineDxf(filePath, diss, diseaseType);

}
