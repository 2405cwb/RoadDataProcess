#pragma once
#include "Trans.h"

template <class T>
class BASICOBJECT_API CTransProject :
	public CTrans<T>
{
public:
	CTransProject(void);
	virtual ~CTransProject(void);

	virtual bool Initialize(POINT2D<T>* pSrcPt, POINT2D<T>* pDestPt, int nPtNum);
	virtual bool Trans(POINT2D<T>& ptInput, POINT2D<T>& ptOutput);
	virtual bool Trans(POINT2D<T>* pPtInput, POINT2D<T>* pPtOutput, int nPtNum);
};

