// stdafx.cpp : source file that includes just the standard includes
//	YuGuang.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

//void init_bm(int width,int height);
void init_bm();

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	init_bm();
    return TRUE;
}
