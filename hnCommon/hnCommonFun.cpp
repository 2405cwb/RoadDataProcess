#include "stdafx.h"
#include "hnCommonFun.h" 
namespace hnCommon
{
	// ·Ö¸îstring
	bool splitString(char* strOri, char* strSplit, vector<string>& vecOut)
	{
		vecOut.clear();

		char *p = NULL;
		p = strtok(strOri, strSplit);

		while (p)
		{
			vecOut.push_back(p);
			p = strtok(NULL, strSplit);
		}

		return true;
	}
}
