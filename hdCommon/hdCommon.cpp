// hdCommon.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "hdCommon.h"
#include "hdSceneStr.h"
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>


LPITEMIDLIST g_pIDList;

static TCHAR g_szdir[MAX_PATH];

// 选择目录的回调函数
int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	LPITEMIDLIST tmp = (LPITEMIDLIST)lpData;
	switch (uMsg)
	{
	case BFFM_INITIALIZED:	// 初始化消息
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)g_szdir);
		break;
	case BFFM_SELCHANGED:	// 选择路径变化
		{
			SHGetPathFromIDList((LPCITEMIDLIST)tmp, g_szdir);
			::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)g_szdir);
		}
		break;
	default:
		break;
	}

	return 0;
}

HDCOMMON_API char* getCurrentDir(HMODULE module)
{
	char path[MAX_PATH] = {0};
	//_get_pgmptr(&path);//_pgmptr;
	GetModuleFileName(module,path,MAX_PATH);

	//char* path = _pgmptr;
	char drive[MAX_PATH] = {0};
	char dir[MAX_PATH] = {0};
	char filename[MAX_PATH] = {0};
	char ext[MAX_PATH] = {0};

	_splitpath(path,drive,dir,filename,ext);
	static char filePath[MAX_PATH] = {0};
	_makepath(filePath,drive,dir,NULL,NULL);
	return filePath;
}
//! 打开目录选择对话框,返回TRUE得到路径,否则不能获取路径
HDCOMMON_API BOOL BrowseFolder(
	HWND hWnd,				// 父窗口
	char* strDispName,		// 显示名称
	const char* strTile,	// 选择目录对话框标题
	char* strSelDir)		// 返回路径
{
	BROWSEINFO bi;
	bi.hwndOwner = hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = strDispName;
	bi.lpszTitle = strTile;
	// 此处更改是为了使路径不显示在对话框中，避免多次打开时，显示路径静态文本框被挡住，而且这种模式也能记住上次打开的路径 fengjing
	bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;// BIF_STATUSTEXT
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)g_pIDList;
	bi.iImage = 0;

	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	
	if (lp && SHGetPathFromIDList(lp, strSelDir))
	{
		g_pIDList = lp;
		strcat_s(strSelDir,256,"\\\0");
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

HDCOMMON_API int GetLanguageID()
{
	// 0表示中午,1表示英文
	static int id = -1;		
	if (id != -1)
	{
		// 字符串替换时只需要读一次配置文件 fengjing
		return id;
	}
	string filePath = getCurrentDir();
	filePath += "sysSetting.xml";

	TiXmlDocument doc(filePath.c_str());
	if (!doc.LoadFile())
	{
		// 系统配置文件不存在时，内存中初始化语言为中文
		id = 0;
		return id;
	}

	TiXmlElement* rootElement = doc.RootElement();
	if (rootElement)
	{
		string strValue;
		string strText;

		// 读取常规属性设置
		TiXmlElement* element = rootElement->FirstChildElement("commonSetting");
		TiXmlElement* childElement = element->FirstChildElement();
		while (childElement)
		{
			strValue = childElement->Value();
			strText = childElement->GetText();

			if (strValue == "UILanguage")
			{
				id = atoi(strText.c_str());
				break;
			}	

			childElement = childElement->NextSiblingElement();
		}
	}
	// 如果配置文件里面没有语言设置，默认中文
	if(id == -1)
	{
		id = 0;
	}
	return id;
}

//! 获取当前HDSV语言ID,0代表中文,1代表英文
HDCOMMON_API int GetSvLanguageID()
{
	// 0表示中文,1表示英文
	static int id = -1;
	if (id != -1)
	{
		// 字符串替换时只需要读一次配置文件 fengjing
		return id;
	}

	string filePath = getCurrentDir();
	filePath += "hdPtStreeView.Config";

	TiXmlDocument doc(filePath.c_str());
	if (!doc.LoadFile())
	{
		// 系统配置文件不存在时，内存中初始化语言为中文
		id = 0;
		return id;
	}

	TiXmlElement* pFileConElement = doc.RootElement();
	if (pFileConElement)
	{
		string strType = pFileConElement->Value();
		if (strType == "Conventional")
		{
			// 找到子节点
			TiXmlElement* pFirstConChildElement = pFileConElement->FirstChildElement();

			string strValue;
			string strText;
			while (pFirstConChildElement)
			{
				strValue = pFirstConChildElement->Value();

				if (strValue == "UILanguage")
				{
					if (pFirstConChildElement->GetText())
					{
						// 中英文
						id = atoi(pFirstConChildElement->GetText());
						break;
					}					
				}

				// 查找下一个节点
				pFirstConChildElement = pFirstConChildElement->NextSiblingElement();
			}
		}
	}
	
	return id;
}


// Function name	: isDirectory
// Description	    : determine if the specified path is a directory or not
// Return type		: bool (return true if the specified path is a directoy false if not)
bool IsDirectory(const char* path)
{
	char absPath[_MAX_PATH];
	::_fullpath(absPath, path, _MAX_PATH);

	DWORD attr = ::GetFileAttributes(absPath);
	if (attr == 0xFFFFFFFF || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		return false;   // does not exist or is not a directory
	else
		return true;
}

// Function name	: returnFileExtension
void ReturnFileExtension(const char* path, char* extension)
{
	if (extension == NULL)
		return;

	char absPath[_MAX_PATH];
	char ext[_MAX_EXT];

	::_fullpath(absPath, path, _MAX_PATH);
	::_splitpath(absPath, NULL, NULL, NULL, ext);
	strcpy(extension,ext);
}

// Function name	: fileTypeExists
// Return type		: bool (true if file exists / false if not)
// Argument         : LPCTSTR dirpath (in format c:\temp)
// Argument         : LPCTSTR filter  (filter *.abc )
bool FileTypeExists(const char* dirpath, char* filter)
{
	char path[_MAX_PATH];
	::strcpy(path, dirpath);

	// prepare to construct the filter.
	if (path[strlen(path) - 1] != ('\\'))
		::strcat(path, TEXT("\\"));

	// append the filter
	::strcat(path, filter);

	// search
	HANDLE hSearch;
	WIN32_FIND_DATA findData;
	hSearch = ::FindFirstFile(path, &findData);

	if (hSearch == INVALID_HANDLE_VALUE)
		return false;

	::FindClose(hSearch);
	return true;
}


// Function name	: fileExists
// Description	    : Return the existance of a file
// Return type		: bool (TRUE if exists FALSE if not)
bool FileExists(LPCTSTR loc)
{
	return (::GetFileAttributes(loc) != -1);
}

// Function name	: returnFilesystemPath
void ReturnFilesystemPath(const char* fileLoc, char* path)
{
	char tDrive[_MAX_DRIVE];
	char tDir[_MAX_DIR]; 

	::_splitpath( fileLoc, tDrive, tDir, NULL, NULL);
	
	sprintf(path,"%s%s",tDrive,tDir);
}

// Function name	: ReturnFileName
void ReturnFileName(const char* loc, char * fileName)
{
	char tFname[_MAX_FNAME];
	char tExt[_MAX_EXT];

	::_splitpath( loc, NULL, NULL, tFname, tExt);

	sprintf(fileName,"%s%s",tFname,tExt);	
}

//! 获取配置文件中TIN过滤设置
HDCOMMON_API void GetTINfilter(float& maxGridsize, float& maxTriaSide)
{
	string filePath = getCurrentDir();
	filePath += "sysSetting.xml";

	TiXmlDocument doc(filePath.c_str());
	if (!doc.LoadFile())
	{
		// 系统配置文件不存在时，内存中初始化语言为中文
		maxGridsize = 20.0;
		maxTriaSide = 20.0;
		return;
	}

	TiXmlElement* rootElement = doc.RootElement();
	if (rootElement)
	{
		string strValue;
		string strText;

		// 读取过滤设置
		TiXmlElement* element = rootElement->FirstChildElement("filterSetting");
		TiXmlElement* childElement = element->FirstChildElement();

		while (childElement)
		{
			strValue = childElement->Value();
			strText = childElement->GetText();

			if (strValue == "maxGridSize")
			{
				maxGridsize = (float)atof(strText.c_str());
			}
			if (strValue == "maxTriaSIzeLen")
			{
				maxTriaSide = (float)atof(strText.c_str());
			}
			childElement = childElement->NextSiblingElement();
		}	
	}
	return;
}

HDCOMMON_API long GetTextEncode( const char* FilePath )
{
	long nType = -1;

	// 打开要判断的文件
	FILE* pFile = NULL;

	pFile = fopen(FilePath,"r");

	if (pFile)
	{

		unsigned char* chFileFlag = new unsigned char[3];

		fread(chFileFlag,1,3,pFile);

		if(chFileFlag[0]   ==   0xEF   &&   chFileFlag[1]   ==   0xBB   &&   chFileFlag[2]   ==   0xBF)
			nType = 1;	//UTF-8
		else if (chFileFlag[0]   ==   0xFF   &&   chFileFlag[1]   ==   0xFE)
			nType = 2;	//Unicode
		else if (chFileFlag[0]   ==   0xFE   &&   chFileFlag[1]   ==   0xFF)
			nType = 3;	//Unicode big endian text
		else  
			nType = 4;	//ASCII

		delete []chFileFlag;
		chFileFlag = NULL;
	}

	fclose(pFile);
	pFile = NULL;

	return nType;
}
