#pragma once
#include "../hnCommon/hnRoadStruct.h"
//景观病害管理
using namespace hnCommon;
class StreetDiseaseManage
{
public:
	StreetDiseaseManage();
	~StreetDiseaseManage();
public:
	hnDiseaseSetInfo StreetDis;  
	//累计扣分
	double Score;
    
	int Count;
	double  Area;
};

