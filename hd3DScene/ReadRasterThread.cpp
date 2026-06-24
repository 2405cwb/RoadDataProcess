#include "StdAfx.h"
#include "ReadRasterThread.h"
#include "HdRasterBuffer.h"

namespace hd
{
	namespace scene
	{
		CReadRasterThread::CReadRasterThread(const ZThread::CountedPtr<CHdRasterBuffer>& pGeoRater,const char* strPath,double dScale)
			:m_pImageBuffer(pGeoRater)
			,m_strImagePath(strPath)
			,m_dScale(dScale)
		{
		}


		CReadRasterThread::~CReadRasterThread(void)
		{
		}

		// Ö´ÐÐ
		void CReadRasterThread::run()
		{
			if (m_pImageBuffer && !m_pImageBuffer->isCanceled())
			{
				m_pImageBuffer->ReadRaster();
			}
		}
	}
}
