#pragma once
#include "hnapplication_global.h"
#include "../hnCommon/hnRoadTypeDef.h"
class HNAPPLICATION_EXPORT projectType
{
public:
	projectType();
	~projectType();
protected:
	//工程的类型  二维、三维、二三维
	hnCommon::PROJECT_TYPE  m_projectType;
};

