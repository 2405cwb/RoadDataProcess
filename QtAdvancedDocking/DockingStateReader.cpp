//============================================================================
/// \file   DockingStateReader.cpp
/// \author Uwe Kindler
/// \date   29.11.2019
/// \brief  Implementation of CDockingStateReader
//============================================================================

//============================================================================
//                                   INCLUDES
//============================================================================
#include "DockingStateReader.h"

namespace hn
{

//============================================================================
void CDockingStateReader::setFileVersion(int FileVersion)
{
	m_FileVersion = FileVersion;
}

//============================================================================
int CDockingStateReader::fileVersion() const
{
	return m_FileVersion;
}
} // namespace hn

//---------------------------------------------------------------------------
// EOF DockingStateReader.cpp
