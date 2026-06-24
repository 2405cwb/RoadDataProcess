#include "hnOutputXRCityRoad.h"
#include "hnOutputRoadDiseaseXR_CityRoad.h"

bool __stdcall OutputXRDxfCityRoad(const char* fpath, int diseaseNum, Disease_C* disease, GridDisease_C* gridDisease, int direction)
{
	 std::string str = fpath;
	hnGridDiseaseInfo gridDiseaseInfo;
	gridDiseaseInfo.strName = string(gridDisease->strName);
	gridDiseaseInfo.strBegMile = string(gridDisease->strBegMile);
	gridDiseaseInfo.strEndMile = string(gridDisease->strEndMile);
	gridDiseaseInfo.dBegMileage = gridDisease->dBegMileage;
	gridDiseaseInfo.dEndMileage = gridDisease->dEndMileage;
	gridDiseaseInfo.dRoadWidth = gridDisease->dRoadWidth;

	gridDiseaseInfo.nRoadTotalNum = gridDisease->nRoadTotalNum;
	vector<Disease_C> vecDisease; vecDisease.resize(diseaseNum);
	for (int i = 0; i < diseaseNum; i++)
	{
		vecDisease[i] = disease[i];
	}
	hnOutputRoadDiseaseXR_CityRoad m_outputXR(500);
	m_outputXR.outDisease(fpath, vecDisease, gridDiseaseInfo, direction);
	return true;
}