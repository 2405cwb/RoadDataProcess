#include "StdAfx.h"
#include "HDEntity.h"
#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

namespace hd
{

	CHDEntity::CHDEntity(void)
	{
		memset(m_ID, 0, OBJECT_ID_LEN_L);
		m_pObject = NULL;
	}

	CHDEntity::CHDEntity(const char* pID, CHDObject* pObject)
	{
		strcpy(m_ID, pID);
		m_pObject = pObject;
	}

	CHDEntity::~CHDEntity()
	{
	}

	const char* CHDEntity::GetID() const
	{
		return m_ID;
	}

	void CHDEntity::SetID(const char* pID)
	{
		memset(m_ID, 0, OBJECT_ID_LEN_L);
		strcpy(m_ID, pID);
	}

	CHDObject* CHDEntity::GetHDObject() const
	{
		return m_pObject;
	}

	void CHDEntity::SetObject(CHDObject* pObject)
	{
		m_pObject = pObject;
	}

}