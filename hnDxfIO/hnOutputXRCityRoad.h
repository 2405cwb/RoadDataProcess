#include <Windows.h>
#include<stdio.h>
#define STRING_LEN 64

#ifndef STRUCT_DISEASE_C
#define STRUCT_DISEASE_C
typedef struct Disease_C
{
	int beginTrueMile;
	char roadNum[STRING_LEN];
	char diseaseType[STRING_LEN];
	char diseaseDegree[STRING_LEN];
	double rectHeight;
	double rectWidth;
	double distToCenter;
	double diseaseArea;
	double calcHeight;
	double calcWidth;
	bool bOnRoad;
}Disease_C;
#endif
#ifndef STRUCT_GRID_DISEASE_C
#define STRUCT_GRID_DISEASE_C
typedef struct GridDisease_C
{
	char strName[STRING_LEN];
	char strBegMile[STRING_LEN];
	char strEndMile[STRING_LEN];
	double dBegMileage;
	double dEndMileage;
	double dRoadWidth;
	int nRoadTotalNum;
}GridDisease_C;
#endif

extern "C" _declspec(dllexport)bool _stdcall OutputXRDxfCityRoad(const char* path,int diseaseNum,Disease_C* disease,GridDisease_C* gridDisease,int direction);