#include "hdBasicObject.h"

namespace base
{
	//! 获取当前应用程序路径
	BASICOBJECT_API char* getCurrentDir(HMODULE module = NULL);

	//! 打开目录选择对话框,返回TRUE得到路径,否则不能获取路径
	BASICOBJECT_API BOOL BrowseFolder(
		HWND hWnd,				// 父窗口
		char* strDispName,		// 显示名称
		const char* strTile,	// 选择目录对话框标题
		char* strSelDir);		// 返回路径

	// Function name	: isDirectory
	// Description	    : determine if the specified path is a directory or not
	// Return type		: bool (return true if the specified path is a directoy false if not)
	BASICOBJECT_API bool IsDirectory(const char* path);

	// Function name	: returnFileExtension
	// void returnFileExtension(LPCTSTR path, _TCHAR * extension);
	BASICOBJECT_API void ReturnFileExtension(const char* path, char * extension);

	// Function name	: fileTypeExists
	// Return type		: bool (true if file exists / false if not)
	// Argument         : LPCTSTR dirpath (in format c:\temp)
	// Argument         : LPCTSTR filter  (filter *.abc )
	BASICOBJECT_API bool FileTypeExists(const char* dirpath, char* filter);

	// Function name	: fileExists
	// Description	    : Return the existance of a file
	// Return type		: bool (TRUE if exists FALSE if not)
	BASICOBJECT_API bool FileExists(const char* filePath);

	// Function name	: returnFilesystemPath
	// Description	    : return the path part of a file name including the trailing slash
	BASICOBJECT_API void ReturnFilesystemPath(const char* fileLoc, char* path);

	// Function name	: ReturnFileName
	// Description	    : return the file from a provide path 'c:\temp\file.ext becomes' 'file.ext'
	BASICOBJECT_API void ReturnFileName(const char* loc, char* fileName);

	// 获取当前可用内存,返回单位为M
	BASICOBJECT_API float GetValidMemory();
}
