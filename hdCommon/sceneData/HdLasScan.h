#pragma once
#include "..\hdCommon.h"
#include "..\hdobject.h"
#include "hd3LSScanStruct.h"

namespace hd
{
class HDCOMMON_API CHdLasScan :
	public CHDObject
{
public:
	CHdLasScan(void);
	virtual ~CHdLasScan(void);

	CHdLasScan(_HD_LAS_DATA& hdLas);

	CHdLasScan(CHdLasScan& hdss);
	
public:
	HD_LAS_DATA				LasData;			//扫描原始数据记录

	virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_FILE_LAS; }
	void Serialize(TiXmlElement* element, bool bSave);
};

}

