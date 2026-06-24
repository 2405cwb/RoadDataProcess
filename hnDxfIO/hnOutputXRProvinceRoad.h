#include <Windows.h>
#include<stdio.h>
#include <vector>
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

/*
*接口名称：OutputXRDxf
*功能：等级公路2018 人工模式 国省道 病害面积表 输出CAD
*参数1：输出的文件路径
*参数2：传入的病害信息
*参数3：分段病害信息
*参数4：上下行 大于0从小到大，小于0从大到小
*/
extern "C" __declspec(dllexport) bool __stdcall OutputXRDxfProvinceRoad(const char* filePath, std::vector<Disease_C> diseases,
	const GridDisease_C& gridDisease, int direction,int roadType);
