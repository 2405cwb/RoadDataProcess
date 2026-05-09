#include "HLSReadOpener.h"
#include "HLS2Reader.h"
#include "HL1Reader.h"
#include "HdCoreData.h"
namespace hd
{
	CHLSReadOpener::CHLSReadOpener(void)
	{
	}

	CHLSReadOpener::~CHLSReadOpener(void)
	{
	}

	IHLSReader* CHLSReadOpener::Open( const char* filePath )
	{
		IHLSReader* pReader = NULL;
		
		// 打开索引文件
		FILE* pIndexFile = fopen(filePath,"rb");
		if (pIndexFile == NULL)
		{
			return pReader;
		}

		BOOL bRet = FALSE;
		
		// 读取文件头
		char file_signature[5] = {0};
		fread(file_signature,1,4,pIndexFile);
		if (file_signature[0] == 'H' &&
			file_signature[1] == 'L' &&
			file_signature[2] == 'S' &&
			file_signature[3] == 'F')
		{
			pReader = new CHL1Reader();
			bRet = pReader->Open(filePath);
			pReader->m_dataType = E_HDT_HLS;
		}
		else if (
			file_signature[0] == 'H' &&
			file_signature[1] == 'L' &&
			file_signature[2] == 'S' &&
			file_signature[3] == '2')
		{
			pReader = new CHLS2Reader();
			bRet = pReader->Open(filePath);
			pReader->m_dataType = E_HDT_HLS;
		}
		else if (
			file_signature[0] == 'H' &&
			file_signature[1] == 'L' &&
			file_signature[2] == 'Z'/* &&
			file_signature[3] == 'F'*/)
		{

			pReader = new CHdCoreData();
			bRet = pReader->Open(filePath);
			pReader->m_dataType = E_HDT_HLZ;
		}

		fclose(pIndexFile);
		if (!bRet)
		{
			delete pReader;
			pReader = NULL;
		}
		return pReader;
	}

}