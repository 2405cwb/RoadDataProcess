#pragma once
#include "hdbasicobject.h"


class  CHdBasicPoint :
	public CHdBasicObject
{
public:
	CHdBasicPoint(void) { m_eType = E_TID_POINT; };
	virtual ~CHdBasicPoint(void) {}
};

