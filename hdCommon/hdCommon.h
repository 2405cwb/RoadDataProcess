// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 HDCOMMON_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// HDCOMMON_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#pragma once

#ifdef HDCOMMON_EXPORTS
#define HDCOMMON_API __declspec(dllexport)
#else
#define HDCOMMON_API __declspec(dllimport)
#endif

//! 获取当前应用程序路径
HDCOMMON_API char* getCurrentDir(HMODULE module = NULL);

//! 打开目录选择对话框,返回TRUE得到路径,否则不能获取路径
HDCOMMON_API BOOL BrowseFolder(
	HWND hWnd,				// 父窗口
	char* strDispName,		// 显示名称
	const char* strTile,	// 选择目录对话框标题
	char* strSelDir);		// 返回路径

//! 获取当前语言ID,0代表中文,1代表英文
HDCOMMON_API int GetLanguageID();

//! 获取当前HDSV语言ID,0代表中文,1代表英文 ,YANGFFENG 2014/2/20
HDCOMMON_API int GetSvLanguageID();


//! 获取配置文件中TIN过滤设置
HDCOMMON_API void GetTINfilter(float& maxGridsize, float& maxTriaSide);

// Function name	: isDirectory
// Description	    : determine if the specified path is a directory or not
// Return type		: bool (return true if the specified path is a directoy false if not)
HDCOMMON_API bool IsDirectory(const char* path);

// Function name	: returnFileExtension
// void returnFileExtension(LPCTSTR path, _TCHAR * extension);
HDCOMMON_API void ReturnFileExtension(const char* path, char * extension);

// Function name	: fileTypeExists
// Return type		: bool (true if file exists / false if not)
// Argument         : LPCTSTR dirpath (in format c:\temp)
// Argument         : LPCTSTR filter  (filter *.abc )
HDCOMMON_API bool FileTypeExists(const char* dirpath, char* filter);

// Function name	: fileExists
// Description	    : Return the existance of a file
// Return type		: bool (TRUE if exists FALSE if not)
HDCOMMON_API bool FileExists(const char* filePath);

// Function name	: returnFilesystemPath
// Description	    : return the path part of a file name including the trailing slash
HDCOMMON_API void ReturnFilesystemPath(const char* fileLoc, char* path);

// Function name	: ReturnFileName
// Description	    : return the file from a provide path 'c:\temp\file.ext becomes' 'file.ext'
HDCOMMON_API void ReturnFileName(const char* loc, char* fileName);

// 获取文本文件的字符编码类型
// 文件的字符集在Windows下有两种，一是ANSI，一是Unicode，对于Unicode支持三种编码，小尾编码Unicode
// 大尾编码BigEndianUnicode以及UTF-8编码
// 危迟 2015/06/02
// FilePath: 文本文件路径
// 返回值：1- UTF-8 2-Unicode 3-Unicode big Endian 4-ASCII
HDCOMMON_API long GetTextEncode(const char* FilePath);
