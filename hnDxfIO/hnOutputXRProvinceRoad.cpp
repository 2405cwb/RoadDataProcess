#include"hnOutputXRProvinceRoad.h"
//#include "hnOutDiseaseDxf.h"
#include "hnOutputRoadDiseaseXR_ProvinceRoad.h"

//static hnOutDiseaseDxf m_dxf;
//static hnOutputRoadDiseaseXR m_outputXR(100);

bool __stdcall OutputXRDxfProvinceRoad(const char* filePath, std::vector<Disease_C> diseases,
	const GridDisease_C& gridDisease, int direction, int roadType)
{
 	string str = filePath;
	hnGridDiseaseInfo gridDiseaseInfo;
	gridDiseaseInfo.strName = string(gridDisease.strName);
	gridDiseaseInfo.strBegMile = string(gridDisease.strBegMile);
	gridDiseaseInfo.strEndMile = string(gridDisease.strEndMile);
	gridDiseaseInfo.dBegMileage = gridDisease.dBegMileage;
	gridDiseaseInfo.dEndMileage = gridDisease.dEndMileage;
	gridDiseaseInfo.dRoadWidth = gridDisease.dRoadWidth;
	gridDiseaseInfo.nRoadTotalNum = gridDisease.nRoadTotalNum;

	hnOutputRoadDiseaseXRProvinceRoad exportDxf(500);
	exportDxf.outDisease(filePath, diseases,gridDiseaseInfo,direction,roadType);
	return true;
}