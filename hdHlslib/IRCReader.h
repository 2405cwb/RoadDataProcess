#pragma once
#include "ihlsreader.h"

namespace hd
{
class CIRCReader :
	public IHLSReader
{
public:
	CIRCReader(void);
	virtual ~CIRCReader(void);
};

}