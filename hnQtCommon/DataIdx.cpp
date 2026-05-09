#include "DataIdx.h"
 
DataIdx::DataIdx()
{
} 
DataIdx::DataIdx(int ci, int li, int ni)
{
	_Idx = ci;
	_LastIdx = li;
	_NextIdx = ni;
}

DataIdx::~DataIdx()
{
}
