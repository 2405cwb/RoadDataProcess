#pragma once
#include "hdBasicObject.h"
#include "hdPoint2D.h"

template <class T>
class BASICOBJECT_API CTrans :
	public CHdBasicObject
{
public:
	CTrans(void) 
	{ 
		m_eType = E_TID_TRANS; 
		m_pParam = NULL; 
	}

	virtual ~CTrans(void) 
	{
		if (m_pParam != NULL)
		{
			delete [] m_pParam;
			m_pParam = NULL;
		}
	}

	virtual bool Initialize(POINT2D<T>* pSrcPt, POINT2D<T>* pDestPt, int nPtNum) = 0;
	virtual bool Trans(POINT2D<T>& ptInput, POINT2D<T>& ptOutput) = 0;
	virtual bool Trans(POINT2D<T>* pPtInput, POINT2D<T>* pPtOutput, int nPtNum) = 0;

protected:
	double*		m_pParam;
};

