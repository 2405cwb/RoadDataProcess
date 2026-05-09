#pragma once

#include "hnqtcommon_global.h" 
class HNQTCOMMON_EXPORT  DataIdx
{
public:

	int _Idx;
	int _LastIdx;
	int _NextIdx;

public:
	DataIdx();
	DataIdx(int ci, int li, int ni);
	~DataIdx();
};

