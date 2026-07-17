#include "PackSdk.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Shlobj.h>
#pragma comment(lib, "shell32.lib")
#else
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <codecvt>
#include <locale>
#endif

#pragma pack(push, 1)
// v1 索引文件头（72 字节）。packNo 表示该 idx 对应的 dat 分卷号。
struct DEMO_IDX_FILE_HEADER
{
	char magic[8];
	uint32_t version;
	uint32_t headerSize;
	uint32_t recordSize;
	uint32_t packNo;
	uint64_t recordCount;
	uint64_t createTimeValue;
	uint64_t reserved[4];
};

// v2 索引文件头（72 字节，布局与 v1 相同，偏移 16 的字段含义改为 datFileCount）。
struct DEMO_IDX_FILE_HEADER_V2
{
	char magic[8];
	uint32_t version;
	uint32_t headerSize;
	uint32_t recordSize;
	uint32_t datFileCount;
	uint64_t recordCount;
	uint64_t createTimeValue;
	uint64_t reserved[4];
};

// v1 索引记录（32 字节）。
struct DEMO_IDX_RECORD
{
	uint64_t sourceIndex;
	uint64_t timeValue;
	uint64_t datOffset;
	uint32_t jpgSize;
	uint32_t reserved;
};

// v2 索引记录（36 字节）。多了 datNo 字段指明该帧属于哪个 dat 文件。
struct DEMO_IDX_RECORD_V2
{
	uint64_t sourceIndex;
	uint64_t timeValue;
	uint32_t datNo;
	uint64_t datOffset;
	uint32_t jpgSize;
	uint32_t reserved;
};

struct DEMO_DAT_FRAME_HEADER
{
	uint32_t magic;
	uint32_t headerSize;
	uint64_t sourceIndex;
	uint64_t timeValue;
	uint32_t jpgSize;
	uint32_t reserved;
};
#pragma pack(pop)

static const uint32_t DEMO_DAT_MAGIC = 0x3147504A;

struct PackFileEntry
{
	uint32_t packNo;
	std::string idxPath;
	std::string datPath;
	uint64_t idxRecordCount;
	uint64_t availableRecordCount;
	bool missingDat;
	bool badIdx;
};

struct FrameEntry
{
	uint64_t globalIndex;
	uint32_t packNo;
	uint32_t datNo;
	uint32_t packFrameIndex;
	uint32_t statusFlags;
	uint64_t sourceIndex;
	uint64_t timeValue;
	uint64_t datOffset;
	uint32_t jpgSize;
	std::string datPath;
};

struct PackHandleImpl
{
	std::string rootDir;
	std::vector<PackFileEntry> packs;
	std::vector<FrameEntry> frames;
	std::unordered_map<uint64_t, uint64_t> sourceIndexMap;
	std::wstring lastError;
	std::string lastErrorUtf8;
	uint64_t warningCount;
	uint32_t formatVersion;

	PackHandleImpl() : warningCount(0), formatVersion(0) {}
};

static std::wstring g_lastError;
static std::string g_lastErrorUtf8;

#if defined(_WIN32)
// Windows 内部统一使用 UTF-8 保存路径；调用 Win32 API 前再转成 UTF-16。
static std::wstring Utf8ToWide(const std::string& src)
{
	if (src.empty()) return std::wstring();
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, NULL, 0);
	if (len <= 0) return std::wstring();
	std::vector<wchar_t> buffer((size_t)len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &buffer[0], len);
	return std::wstring(&buffer[0]);
}

// 把 Windows 宽字符路径或错误文本转换成 UTF-8，供跨平台核心逻辑和 Utf8 接口使用。
static std::string WideToUtf8(const wchar_t* src)
{
	if (src == NULL || src[0] == 0) return std::string();
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	if (len <= 0) return std::string();
	std::vector<char> buffer((size_t)len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, &buffer[0], len, NULL, NULL);
	return std::string(&buffer[0]);
}
#else
// Linux/Ubuntu 下 wchar_t 是 4 字节，宽字符接口主要为了保持 ABI 一致；核心路径仍使用 UTF-8。
static std::wstring Utf8ToWideString(const std::string& src)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.from_bytes(src);
}

// Linux/Ubuntu 宽字符到 UTF-8 转换，用于兼容 Pack_Open 的 wchar_t 版本。
static std::string WideToUtf8(const wchar_t* src)
{
	if (src == NULL || src[0] == 0) return std::string();
	std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.to_bytes(src);
}

// Linux/Ubuntu UTF-8 到宽字符转换，用于生成宽字符错误摘要。
static std::wstring Utf8ToWide(const std::string& src)
{
	return Utf8ToWideString(src);
}
#endif

// 设置没有 handle 时的全局错误，例如 Pack_Open 参数错误或打开失败。
static void SetGlobalError(const std::wstring& text)
{
	g_lastError = text;
	g_lastErrorUtf8 = WideToUtf8(text.c_str());
}

// 同时设置当前 handle 错误和全局错误。调用方可用 Pack_GetLastError 读取。
static void SetError(PackHandleImpl* handle, const std::wstring& text)
{
	if (handle != NULL)
	{
		handle->lastError = text;
		handle->lastErrorUtf8 = WideToUtf8(text.c_str());
	}
	SetGlobalError(text);
}

// 拼接路径。Windows 输出反斜杠，Linux/Ubuntu 输出斜杠；已带分隔符时不重复添加。
static std::string JoinPath(const std::string& left, const std::string& right)
{
	if (left.empty()) return right;
	char last = left[left.size() - 1];
	if (last == '\\' || last == '/') return left + right;
#if defined(_WIN32)
	return left + "\\" + right;
#else
	return left + "/" + right;
#endif
}

// 判断普通文件是否存在。Windows 走 UTF-16 Win32 API，Linux 走 stat。
static bool FileExists(const std::string& path)
{
#if defined(_WIN32)
	DWORD attr = GetFileAttributesW(Utf8ToWide(path).c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

// 判断目录是否存在。用于打开 pack 根目录和创建导出目录前的检查。
static bool DirectoryExists(const std::string& path)
{
#if defined(_WIN32)
	DWORD attr = GetFileAttributesW(Utf8ToWide(path).c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

// 递归创建目录。Windows 使用 SHCreateDirectoryExW；Linux 逐级 mkdir。
static bool EnsureDirectory(const std::string& path)
{
	if (path.empty()) return false;
	if (DirectoryExists(path)) return true;
#if defined(_WIN32)
	int ret = SHCreateDirectoryExW(NULL, Utf8ToWide(path).c_str(), NULL);
	return ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS || ret == ERROR_FILE_EXISTS;
#else
	std::string current;
	size_t start = 0;
	if (!path.empty() && path[0] == '/')
	{
		current = "/";
		start = 1;
	}
	while (start <= path.size())
	{
		size_t pos = path.find('/', start);
		std::string part = path.substr(start, pos == std::string::npos ? std::string::npos : pos - start);
		if (!part.empty())
		{
			if (!current.empty() && current[current.size() - 1] != '/') current += "/";
			current += part;
			if (!DirectoryExists(current) && mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
			{
				return false;
			}
		}
		if (pos == std::string::npos) break;
		start = pos + 1;
	}
	return DirectoryExists(path);
#endif
}

// 保存单张图前确保父目录存在。文件名本身不会被创建成目录。
static bool EnsureParentDirectory(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	if (pos == std::string::npos) return true;
	return EnsureDirectory(path.substr(0, pos));
}

// 获取文件 64 位长度。pack dat 通常很大，不能使用 32 位文件大小。
static bool GetFileSize64(const std::string& path, uint64_t* outSize)
{
	if (outSize == NULL) return false;
#if defined(_WIN32)
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExW(Utf8ToWide(path).c_str(), GetFileExInfoStandard, &data)) return false;
	ULARGE_INTEGER value;
	value.HighPart = data.nFileSizeHigh;
	value.LowPart = data.nFileSizeLow;
	*outSize = value.QuadPart;
	return true;
#else
	struct stat st;
	if (stat(path.c_str(), &st) != 0) return false;
	*outSize = (uint64_t)st.st_size;
	return true;
#endif
}

// 打开文件。封装这个函数是为了在 Windows 下支持中文路径。
static FILE* OpenFile(const std::string& path, const char* mode)
{
#if defined(_WIN32)
	std::wstring wmode = Utf8ToWide(mode);
	FILE* fp = NULL;
	_wfopen_s(&fp, Utf8ToWide(path).c_str(), wmode.c_str());
	return fp;
#else
	return fopen(path.c_str(), mode);
#endif
}

// 64 位 seek。datOffset 是 uint64_t，必须支持大文件偏移。
static int Seek64(FILE* fp, uint64_t offset)
{
#if defined(_WIN32)
	return _fseeki64(fp, (__int64)offset, SEEK_SET);
#else
	return fseeko(fp, (off_t)offset, SEEK_SET);
#endif
}

// 从文件名 Pack_000123.idx 中解析分卷号 123。
static bool ParsePackNoFromIdxName(const std::string& name, uint32_t* packNo)
{
	if (packNo == NULL || name.size() != 15) return false;
	if (name.compare(0, 5, "Pack_") != 0 || name.compare(11, 4, ".idx") != 0) return false;
	uint32_t value = 0;
	for (size_t i = 5; i < 11; ++i)
	{
		if (name[i] < '0' || name[i] > '9') return false;
		value = value * 10 + (uint32_t)(name[i] - '0');
	}
	*packNo = value;
	return true;
}

// 根据分卷号生成 dat 文件名，例如 Pack_000123.dat。
static std::string MakePackDatName(uint32_t packNo)
{
	char name[64];
#if defined(_WIN32)
	sprintf_s(name, "Pack_%06u.dat", packNo);
#else
	snprintf(name, sizeof(name), "Pack_%06u.dat", packNo);
#endif
	return std::string(name);
}

// 读取固定长度数据，长度不足直接返回失败。用于读文件头、索引记录和帧头。
static bool ReadExact(FILE* fp, void* buffer, size_t bytes)
{
	return fp != NULL && buffer != NULL && fread(buffer, 1, bytes, fp) == bytes;
}

// 校验 idx 文件头是否是 SDK 支持的 v1 格式。
static bool IsValidIdxHeaderV1(const DEMO_IDX_FILE_HEADER& header)
{
	return memcmp(header.magic, "IOVIDX1", 7) == 0 &&
		header.magic[7] == 0 &&
		header.version == 1 &&
		header.headerSize == sizeof(DEMO_IDX_FILE_HEADER) &&
		header.recordSize == sizeof(DEMO_IDX_RECORD);
}

// 校验 idx 文件头是否是 SDK 支持的 v2 格式（单文件 PackIndex.idx）。
static bool IsValidIdxHeaderV2(const DEMO_IDX_FILE_HEADER_V2& header)
{
	return memcmp(header.magic, "IOVIDX2", 7) == 0 &&
		header.magic[7] == 0 &&
		header.version == 2 &&
		header.headerSize == sizeof(DEMO_IDX_FILE_HEADER_V2) &&
		header.recordSize == sizeof(DEMO_IDX_RECORD_V2);
}

// 校验一帧 idx 记录对应的 dat 帧头和 JPEG 数据范围。
// 该函数不读取完整 JPEG，只验证 offset、帧头字段和数据长度是否合理。
static int ReadFrameHeaderAndValidate(const FrameEntry& frame, std::wstring* error)
{
	uint64_t datSize = 0;
	if (!GetFileSize64(frame.datPath, &datSize))
	{
		if (error != NULL) *error = L"dat file not found or cannot read size.";
		return PACK_ERROR_NOT_FOUND;
	}
	if (frame.datOffset + sizeof(DEMO_DAT_FRAME_HEADER) > datSize)
	{
		if (error != NULL) *error = L"dat offset points outside file.";
		return PACK_ERROR_FORMAT;
	}

	FILE* fp = OpenFile(frame.datPath, "rb");
	if (fp == NULL)
	{
		if (error != NULL) *error = L"failed to open dat file.";
		return PACK_ERROR_OPEN_FAILED;
	}
	if (Seek64(fp, frame.datOffset) != 0)
	{
		fclose(fp);
		if (error != NULL) *error = L"failed to seek dat file.";
		return PACK_ERROR_IO;
	}

	DEMO_DAT_FRAME_HEADER datHeader;
	if (!ReadExact(fp, &datHeader, sizeof(datHeader)))
	{
		fclose(fp);
		if (error != NULL) *error = L"failed to read dat frame header.";
		return PACK_ERROR_IO;
	}
	fclose(fp);

	if (datHeader.magic != DEMO_DAT_MAGIC ||
		datHeader.headerSize != sizeof(DEMO_DAT_FRAME_HEADER) ||
		datHeader.sourceIndex != frame.sourceIndex ||
		datHeader.timeValue != frame.timeValue ||
		datHeader.jpgSize != frame.jpgSize)
	{
		if (error != NULL) *error = L"idx record and dat frame header do not match.";
		return PACK_ERROR_FORMAT;
	}
	if (frame.datOffset + sizeof(DEMO_DAT_FRAME_HEADER) + frame.jpgSize > datSize)
	{
		if (error != NULL) *error = L"jpeg data extends beyond dat file.";
		return PACK_ERROR_FORMAT;
	}
	return PACK_OK;
}

// 扫描根目录下的 Pack_*.idx 文件，按分卷号排序，并补出对应 dat 路径。
static std::vector<PackFileEntry> FindPackFiles(const std::string& rootDir)
{
	std::vector<PackFileEntry> packs;
#if defined(_WIN32)
	std::wstring pattern = Utf8ToWide(JoinPath(rootDir, "Pack_*.idx"));
	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW(pattern.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) return packs;
	do
	{
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;
		std::string fileName = WideToUtf8(findData.cFileName);
		uint32_t packNo = 0;
		if (!ParsePackNoFromIdxName(fileName, &packNo)) continue;
		PackFileEntry entry;
		entry.packNo = packNo;
		entry.idxPath = JoinPath(rootDir, fileName);
		entry.datPath = JoinPath(rootDir, MakePackDatName(packNo));
		entry.idxRecordCount = 0;
		entry.availableRecordCount = 0;
		entry.missingDat = !FileExists(entry.datPath);
		entry.badIdx = false;
		packs.push_back(entry);
	} while (FindNextFileW(hFind, &findData));
	FindClose(hFind);
#else
	DIR* dir = opendir(rootDir.c_str());
	if (dir == NULL) return packs;
	struct dirent* ent = NULL;
	while ((ent = readdir(dir)) != NULL)
	{
		std::string fileName = ent->d_name;
		uint32_t packNo = 0;
		if (!ParsePackNoFromIdxName(fileName, &packNo)) continue;
		PackFileEntry entry;
		entry.packNo = packNo;
		entry.idxPath = JoinPath(rootDir, fileName);
		entry.datPath = JoinPath(rootDir, MakePackDatName(packNo));
		entry.idxRecordCount = 0;
		entry.availableRecordCount = 0;
		entry.missingDat = !FileExists(entry.datPath);
		entry.badIdx = false;
		packs.push_back(entry);
	}
	closedir(dir);
#endif
	std::sort(packs.begin(), packs.end(), [](const PackFileEntry& a, const PackFileEntry& b) {
		return a.packNo < b.packNo;
	});
	return packs;
}

// 打开 pack 数据集时构建全局帧索引（v1 格式）。
// 这里会读取所有 idx 记录，但不会读取 JPEG 数据体，因此打开速度主要取决于 idx 总大小。
static int ScanPackDirectory(PackHandleImpl* handle)
{
	if (handle == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	if (!DirectoryExists(handle->rootDir))
	{
		SetError(handle, L"rootDir does not exist or is not a directory.");
		return PACK_ERROR_NOT_FOUND;
	}

	handle->formatVersion = 1;

	std::vector<PackFileEntry> packs = FindPackFiles(handle->rootDir);
	if (packs.empty())
	{
		SetError(handle, L"no valid Pack_*.idx files found.");
		return PACK_ERROR_NOT_FOUND;
	}

	for (size_t p = 0; p < packs.size(); ++p)
	{
		PackFileEntry& pack = packs[p];
		FILE* fp = OpenFile(pack.idxPath, "rb");
		if (fp == NULL)
		{
			pack.badIdx = true;
			handle->warningCount++;
			continue;
		}

		uint64_t idxSize = 0;
		if (!GetFileSize64(pack.idxPath, &idxSize) || idxSize < sizeof(DEMO_IDX_FILE_HEADER))
		{
			fclose(fp);
			pack.badIdx = true;
			handle->warningCount++;
			continue;
		}

		DEMO_IDX_FILE_HEADER header;
		if (!ReadExact(fp, &header, sizeof(header)) || !IsValidIdxHeaderV1(header))
		{
			fclose(fp);
			pack.badIdx = true;
			handle->warningCount++;
			continue;
		}

		pack.idxRecordCount = header.recordCount;
		pack.availableRecordCount = (idxSize - sizeof(DEMO_IDX_FILE_HEADER)) / sizeof(DEMO_IDX_RECORD);
		if (pack.availableRecordCount != pack.idxRecordCount || header.packNo != pack.packNo) handle->warningCount++;
		if (pack.missingDat) handle->warningCount++;

		for (uint64_t i = 0; i < pack.availableRecordCount; ++i)
		{
			DEMO_IDX_RECORD record;
			if (!ReadExact(fp, &record, sizeof(record)))
			{
				handle->warningCount++;
				break;
			}
			if (pack.missingDat || record.jpgSize == 0)
			{
				handle->warningCount++;
				continue;
			}

			FrameEntry frame;
			frame.globalIndex = (uint64_t)handle->frames.size();
			frame.packNo = pack.packNo;
			frame.datNo = pack.packNo;
			frame.packFrameIndex = (uint32_t)i;
			frame.statusFlags = (i >= pack.idxRecordCount) ? PACK_FRAME_STATUS_IDX_RECORD_AFTER_HEADER_COUNT : PACK_FRAME_STATUS_OK;
			frame.sourceIndex = record.sourceIndex;
			frame.timeValue = record.timeValue;
			frame.datOffset = record.datOffset;
			frame.jpgSize = record.jpgSize;
			frame.datPath = pack.datPath;
			handle->frames.push_back(frame);
			if (handle->sourceIndexMap.find(frame.sourceIndex) == handle->sourceIndexMap.end())
			{
				handle->sourceIndexMap[frame.sourceIndex] = frame.globalIndex;
			}
		}
		fclose(fp);
	}

	handle->packs.swap(packs);
	if (handle->frames.empty())
	{
		SetError(handle, L"pack directory opened, but no readable frames were indexed.");
		return PACK_ERROR_FORMAT;
	}
	return PACK_OK;
}

// 扫描 v2 格式的全局 PackIndex.idx 并构建帧索引。
// v2 只有一个索引文件，通过每条记录中的 datNo 字段关联到多个 .dat 文件。
static int ScanPackDirectoryV2(PackHandleImpl* handle)
{
	if (handle == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	if (!DirectoryExists(handle->rootDir))
	{
		SetError(handle, L"rootDir does not exist or is not a directory.");
		return PACK_ERROR_NOT_FOUND;
	}

	std::string idxPath = JoinPath(handle->rootDir, "PackIndex.idx");
	if (!FileExists(idxPath))
	{
		SetError(handle, L"PackIndex.idx not found.");
		return PACK_ERROR_NOT_FOUND;
	}

	FILE* fp = OpenFile(idxPath, "rb");
	if (fp == NULL)
	{
		SetError(handle, L"failed to open PackIndex.idx.");
		return PACK_ERROR_OPEN_FAILED;
	}

	uint64_t idxSize = 0;
	if (!GetFileSize64(idxPath, &idxSize) || idxSize < sizeof(DEMO_IDX_FILE_HEADER_V2))
	{
		fclose(fp);
		SetError(handle, L"PackIndex.idx is too small or unreadable.");
		return PACK_ERROR_FORMAT;
	}

	DEMO_IDX_FILE_HEADER_V2 header;
	if (!ReadExact(fp, &header, sizeof(header)) || !IsValidIdxHeaderV2(header))
	{
		fclose(fp);
		SetError(handle, L"PackIndex.idx has invalid v2 header.");
		return PACK_ERROR_FORMAT;
	}

	handle->formatVersion = 2;

	uint64_t availableRecords = (idxSize - sizeof(DEMO_IDX_FILE_HEADER_V2)) / sizeof(DEMO_IDX_RECORD_V2);
	if (availableRecords != header.recordCount) handle->warningCount++;

	// 扫描所用：datNo → dat 路径映射
	std::unordered_map<uint32_t, std::string> datPathMap;
	std::unordered_map<uint32_t, bool> datExistsMap;

	for (uint64_t i = 0; i < availableRecords; ++i)
	{
		DEMO_IDX_RECORD_V2 record;
		if (!ReadExact(fp, &record, sizeof(record)))
		{
			handle->warningCount++;
			break;
		}

		// 按需建立 dat 文件路径映射
		uint32_t datNo = record.datNo;
		if (datPathMap.find(datNo) == datPathMap.end())
		{
			std::string datPath = JoinPath(handle->rootDir, MakePackDatName(datNo));
			datPathMap[datNo] = datPath;
			datExistsMap[datNo] = FileExists(datPath);
		}

		if (!datExistsMap[datNo] || record.jpgSize == 0)
		{
			handle->warningCount++;
			continue;
		}

		FrameEntry frame;
		frame.globalIndex = (uint64_t)handle->frames.size();
		frame.packNo = datNo;
		frame.datNo = datNo;
		frame.packFrameIndex = (uint32_t)i;
		frame.statusFlags = (i >= header.recordCount) ? PACK_FRAME_STATUS_IDX_RECORD_AFTER_HEADER_COUNT : PACK_FRAME_STATUS_OK;
		frame.sourceIndex = record.sourceIndex;
		frame.timeValue = record.timeValue;
		frame.datOffset = record.datOffset;
		frame.jpgSize = record.jpgSize;
		frame.datPath = datPathMap[datNo];
		handle->frames.push_back(frame);
		if (handle->sourceIndexMap.find(frame.sourceIndex) == handle->sourceIndexMap.end())
		{
			handle->sourceIndexMap[frame.sourceIndex] = frame.globalIndex;
		}
	}
	fclose(fp);

	// 用扫描到的唯一 datNo 构建 packs 列表，供 Verify / GetDatasetInfo 使用
	std::vector<PackFileEntry> packs;
	for (auto it = datPathMap.begin(); it != datPathMap.end(); ++it)
	{
		uint32_t datNo = it->first;
		PackFileEntry entry;
		entry.packNo = datNo;
		entry.idxPath = idxPath;
		entry.datPath = it->second;
		entry.idxRecordCount = header.recordCount;
		entry.availableRecordCount = availableRecords;
		entry.missingDat = !datExistsMap[datNo];
		entry.badIdx = false;
		packs.push_back(entry);
		if (entry.missingDat) handle->warningCount++;
	}
	std::sort(packs.begin(), packs.end(), [](const PackFileEntry& a, const PackFileEntry& b) {
		return a.packNo < b.packNo;
	});
	handle->packs.swap(packs);

	if (handle->frames.empty())
	{
		SetError(handle, L"pack directory opened (v2), but no readable frames were indexed.");
		return PACK_ERROR_FORMAT;
	}
	return PACK_OK;
}

// 把 C ABI 的不透明句柄转换回内部对象指针。
static PackHandleImpl* ToHandle(PACK_HANDLE handle)
{
	return reinterpret_cast<PackHandleImpl*>(handle);
}

// UTF-8 路径版本打开接口，是跨平台核心入口。
// Windows 宽字符 Pack_Open 也会转到这里，保证后续逻辑只有一套。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_OpenUtf8(
	const char* rootDir,
	const PACK_OPEN_OPTIONS* options,
	PACK_HANDLE* outHandle)
{
	if (outHandle == NULL)
	{
		SetGlobalError(L"outHandle is null.");
		return PACK_ERROR_INVALID_ARGUMENT;
	}
	*outHandle = NULL;
	if (rootDir == NULL || rootDir[0] == 0)
	{
		SetGlobalError(L"rootDir is empty.");
		return PACK_ERROR_INVALID_ARGUMENT;
	}

	PackHandleImpl* handle = new PackHandleImpl();
	handle->rootDir = rootDir;

	// 优先探测 v2 格式（PackIndex.idx），不存在则回退 v1（Pack_*.idx）。
	int ret = ScanPackDirectoryV2(handle);
	if (ret == PACK_ERROR_NOT_FOUND)
	{
		ret = ScanPackDirectory(handle);
	}
	if (ret != PACK_OK)
	{
		SetGlobalError(handle->lastError);
		delete handle;
		return ret;
	}

	if (options != NULL && (options->flags & PACK_OPEN_VERIFY_ON_OPEN) != 0)
	{
		PACK_VERIFY_REPORT report;
		Pack_Verify((PACK_HANDLE)handle, &report);
	}

	*outHandle = (PACK_HANDLE)handle;
	return PACK_OK;
}

// 宽字符路径版本打开接口，主要服务 Windows/C#/Win32 调用方。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_Open(
	const wchar_t* rootDir,
	const PACK_OPEN_OPTIONS* options,
	PACK_HANDLE* outHandle)
{
	return Pack_OpenUtf8(WideToUtf8(rootDir).c_str(), options, outHandle);
}

// 释放 handle 内部索引、路径和错误缓存。
extern "C" PACKSDK_API void PACKSDK_CALL Pack_Close(PACK_HANDLE handle)
{
	delete ToHandle(handle);
}

// 返回数据集摘要，不触发额外磁盘读取。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_GetDatasetInfo(PACK_HANDLE handle, PACK_DATASET_INFO* info)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || info == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	memset(info, 0, sizeof(*info));
	info->structSize = sizeof(PACK_DATASET_INFO);
	info->formatVersion = h->formatVersion > 0 ? h->formatVersion : 1;
	info->packCount = (uint32_t)h->packs.size();
	info->frameCount = (uint64_t)h->frames.size();
	info->warningCount = h->warningCount;
	info->hasWarning = h->warningCount > 0 ? 1 : 0;
	for (size_t i = 0; i < h->frames.size(); ++i)
	{
		uint64_t t = h->frames[i].timeValue;
		if (t == 0) continue;
		if (info->firstTimeValue == 0 || t < info->firstTimeValue) info->firstTimeValue = t;
		if (t > info->lastTimeValue) info->lastTimeValue = t;
	}
	return PACK_OK;
}

// 返回全局帧数，等价于内部 frames 数组长度。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_GetFrameCount(PACK_HANDLE handle, uint64_t* count)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || count == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	*count = (uint64_t)h->frames.size();
	return PACK_OK;
}

// 复制指定全局序号的帧元信息到调用方结构体。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_GetFrameInfo(PACK_HANDLE handle, uint64_t globalIndex, PACK_FRAME_INFO* frameInfo)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || frameInfo == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	if (globalIndex >= h->frames.size())
	{
		SetError(h, L"globalIndex is out of range.");
		return PACK_ERROR_OUT_OF_RANGE;
	}
	const FrameEntry& frame = h->frames[(size_t)globalIndex];
	memset(frameInfo, 0, sizeof(*frameInfo));
	frameInfo->structSize = sizeof(PACK_FRAME_INFO);
	frameInfo->packNo = frame.packNo;
	frameInfo->packFrameIndex = frame.packFrameIndex;
	frameInfo->statusFlags = frame.statusFlags;
	frameInfo->globalIndex = frame.globalIndex;
	frameInfo->sourceIndex = frame.sourceIndex;
	frameInfo->timeValue = frame.timeValue;
	frameInfo->datOffset = frame.datOffset;
	frameInfo->jpgSize = frame.jpgSize;
	frameInfo->width = -1;
	frameInfo->height = -1;
	return PACK_OK;
}

// 通过 sourceIndex 查找全局序号。重复 sourceIndex 时保留第一次出现的位置。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_FindBySourceIndex(PACK_HANDLE handle, uint64_t sourceIndex, uint64_t* globalIndex)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || globalIndex == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	auto it = h->sourceIndexMap.find(sourceIndex);
	if (it == h->sourceIndexMap.end())
	{
		SetError(h, L"sourceIndex was not found.");
		return PACK_ERROR_NOT_FOUND;
	}
	*globalIndex = it->second;
	return PACK_OK;
}

// 读取指定图片的 JPEG 原始字节。
// buffer 为空时只返回 requiredSize；buffer 足够时才进行帧头校验和数据读取。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_ReadJpeg(PACK_HANDLE handle, uint64_t globalIndex, void* buffer, uint32_t bufferSize, uint32_t* requiredSize)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	if (globalIndex >= h->frames.size())
	{
		SetError(h, L"globalIndex is out of range.");
		return PACK_ERROR_OUT_OF_RANGE;
	}
	const FrameEntry& frame = h->frames[(size_t)globalIndex];
	if (requiredSize != NULL) *requiredSize = frame.jpgSize;
	if (buffer == NULL || bufferSize == 0) return PACK_OK;
	if (bufferSize < frame.jpgSize)
	{
		SetError(h, L"buffer is too small.");
		return PACK_ERROR_BUFFER_TOO_SMALL;
	}

	uint64_t datSize = 0;
	if (!GetFileSize64(frame.datPath, &datSize))
	{
		SetError(h, L"dat file not found or cannot read size.");
		return PACK_ERROR_OPEN_FAILED;
	}
	if (frame.datOffset + sizeof(DEMO_DAT_FRAME_HEADER) + frame.jpgSize > datSize)
	{
		SetError(h, L"jpeg data range exceeds dat file size.");
		return PACK_ERROR_FORMAT;
	}

	FILE* fp = OpenFile(frame.datPath, "rb");
	if (fp == NULL)
	{
		SetError(h, L"failed to open dat file.");
		return PACK_ERROR_OPEN_FAILED;
	}
	if (Seek64(fp, frame.datOffset) != 0)
	{
		fclose(fp);
		SetError(h, L"failed to seek dat frame header.");
		return PACK_ERROR_IO;
	}

	DEMO_DAT_FRAME_HEADER header;
	if (!ReadExact(fp, &header, sizeof(header)))
	{
		fclose(fp);
		SetError(h, L"failed to read dat frame header.");
		return PACK_ERROR_IO;
	}
	if (header.magic != DEMO_DAT_MAGIC ||
		header.headerSize != sizeof(DEMO_DAT_FRAME_HEADER) ||
		header.jpgSize != frame.jpgSize ||
		header.sourceIndex != frame.sourceIndex ||
		header.timeValue != frame.timeValue)
	{
		fclose(fp);
		SetError(h, L"dat frame header does not match idx record.");
		return PACK_ERROR_FORMAT;
	}

	if (fread(buffer, 1, frame.jpgSize, fp) != frame.jpgSize)
	{
		fclose(fp);
		SetError(h, L"failed to read jpeg data.");
		return PACK_ERROR_IO;
	}
	fclose(fp);
	return PACK_OK;
}

// UTF-8 路径版本的单图保存接口。内部先调用 Pack_ReadJpeg，再把字节写到目标文件。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_SaveJpegUtf8(PACK_HANDLE handle, uint64_t globalIndex, const char* outputPath)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || outputPath == NULL || outputPath[0] == 0) return PACK_ERROR_INVALID_ARGUMENT;
	uint32_t required = 0;
	int ret = Pack_ReadJpeg(handle, globalIndex, NULL, 0, &required);
	if (ret != PACK_OK) return ret;
	std::vector<unsigned char> data(required);
	ret = Pack_ReadJpeg(handle, globalIndex, data.empty() ? NULL : &data[0], required, &required);
	if (ret != PACK_OK) return ret;
	if (!EnsureParentDirectory(outputPath))
	{
		SetError(h, L"failed to create output parent directory.");
		return PACK_ERROR_IO;
	}
	FILE* fp = OpenFile(outputPath, "wb");
	if (fp == NULL)
	{
		SetError(h, L"failed to create output jpeg file.");
		return PACK_ERROR_OPEN_FAILED;
	}
	size_t written = fwrite(data.empty() ? NULL : &data[0], 1, data.size(), fp);
	fclose(fp);
	if (written != data.size())
	{
		SetError(h, L"failed to write output jpeg file.");
		return PACK_ERROR_IO;
	}
	return PACK_OK;
}

// 宽字符路径版本的单图保存接口。主要用于 Windows/C#。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_SaveJpeg(PACK_HANDLE handle, uint64_t globalIndex, const wchar_t* outputPath)
{
	return Pack_SaveJpegUtf8(handle, globalIndex, WideToUtf8(outputPath).c_str());
}

// 生成批量导出目标路径。命名模式决定是旧目录结构还是扁平 sourceIndex 命名。
static std::string MakeExportPath(const std::string& outputDir, uint32_t namingMode, const FrameEntry& frame, uint64_t globalIndex)
{
	char name[128];
	if (namingMode == PACK_NAMING_FLAT_SOURCE_INDEX)
	{
#if defined(_WIN32)
		sprintf_s(name, "%llu_%llu.jpg", (unsigned long long)frame.sourceIndex, (unsigned long long)frame.timeValue);
#else
		snprintf(name, sizeof(name), "%llu_%llu.jpg", (unsigned long long)frame.sourceIndex, (unsigned long long)frame.timeValue);
#endif
		return JoinPath(outputDir, name);
	}
	char dirName[64];
	char fileName[128];
#if defined(_WIN32)
	sprintf_s(dirName, "Image_%04llu", (unsigned long long)(globalIndex / 1000));
	sprintf_s(fileName, "%03llu_%llu.jpg", (unsigned long long)(globalIndex % 1000), (unsigned long long)frame.timeValue);
#else
	snprintf(dirName, sizeof(dirName), "Image_%04llu", (unsigned long long)(globalIndex / 1000));
	snprintf(fileName, sizeof(fileName), "%03llu_%llu.jpg", (unsigned long long)(globalIndex % 1000), (unsigned long long)frame.timeValue);
#endif
	return JoinPath(JoinPath(outputDir, dirName), fileName);
}

// UTF-8 路径版本的批量导出接口。每张图都通过 Pack_SaveJpegUtf8 输出。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_ExportAllUtf8(PACK_HANDLE handle, const char* outputDir, uint32_t namingMode, PACK_EXPORT_CALLBACK_UTF8 callback, void* userData)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || outputDir == NULL || outputDir[0] == 0) return PACK_ERROR_INVALID_ARGUMENT;
	if (!EnsureDirectory(outputDir))
	{
		SetError(h, L"failed to create output directory.");
		return PACK_ERROR_IO;
	}

	uint64_t total = (uint64_t)h->frames.size();
	for (uint64_t i = 0; i < total; ++i)
	{
		std::string path = MakeExportPath(outputDir, namingMode, h->frames[(size_t)i], i);
		int ret = Pack_SaveJpegUtf8(handle, i, path.c_str());
		if (callback != NULL)
		{
			int cbRet = callback(i, total, path.c_str(), ret, userData);
			if (cbRet != 0)
			{
				SetError(h, L"export cancelled by callback.");
				return ret == PACK_OK ? PACK_ERROR_IO : ret;
			}
		}
		if (ret != PACK_OK) return ret;
	}
	return PACK_OK;
}

// 宽字符路径版本的批量导出接口。回调里返回宽字符输出路径。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_ExportAll(PACK_HANDLE handle, const wchar_t* outputDir, uint32_t namingMode, PACK_EXPORT_CALLBACK callback, void* userData)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || outputDir == NULL || outputDir[0] == 0) return PACK_ERROR_INVALID_ARGUMENT;
	std::string utf8OutputDir = WideToUtf8(outputDir);
	if (!EnsureDirectory(utf8OutputDir))
	{
		SetError(h, L"failed to create output directory.");
		return PACK_ERROR_IO;
	}
	uint64_t total = (uint64_t)h->frames.size();
	for (uint64_t i = 0; i < total; ++i)
	{
		std::string path = MakeExportPath(utf8OutputDir, namingMode, h->frames[(size_t)i], i);
		int ret = Pack_SaveJpegUtf8(handle, i, path.c_str());
		if (callback != NULL)
		{
			std::wstring wpath = Utf8ToWide(path);
			int cbRet = callback(i, total, wpath.c_str(), ret, userData);
			if (cbRet != 0)
			{
				SetError(h, L"export cancelled by callback.");
				return ret == PACK_OK ? PACK_ERROR_IO : ret;
			}
		}
		if (ret != PACK_OK) return ret;
	}
	return PACK_OK;
}

// 校验已索引的全部帧。该接口会遍历 dat 帧头，但不会解码 JPEG。
extern "C" PACKSDK_API int PACKSDK_CALL Pack_Verify(PACK_HANDLE handle, PACK_VERIFY_REPORT* report)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h == NULL || report == NULL) return PACK_ERROR_INVALID_ARGUMENT;
	memset(report, 0, sizeof(*report));
	report->structSize = sizeof(PACK_VERIFY_REPORT);
	report->packCount = (uint32_t)h->packs.size();
	report->warningCount = h->warningCount;
	for (size_t i = 0; i < h->packs.size(); ++i)
	{
		if (h->packs[i].missingDat) report->missingDatCount++;
		if (h->packs[i].badIdx) report->badIdxCount++;
	}
	for (size_t i = 0; i < h->frames.size(); ++i)
	{
		report->checkedFrameCount++;
		std::wstring error;
		if (ReadFrameHeaderAndValidate(h->frames[i], &error) != PACK_OK) report->badFrameCount++;
	}
	std::wstringstream ss;
	ss << L"packs=" << report->packCount
		<< L", frames=" << report->checkedFrameCount
		<< L", badFrames=" << report->badFrameCount
		<< L", badIdx=" << report->badIdxCount
		<< L", missingDat=" << report->missingDatCount
		<< L", warnings=" << report->warningCount;
	std::wstring summary = ss.str();
	wcsncpy(report->summary, summary.c_str(), 511);
	report->summary[511] = 0;
	if (report->badFrameCount > 0 || report->badIdxCount > 0 || report->missingDatCount > 0)
	{
		SetError(h, report->summary);
	}
	return PACK_OK;
}

// 返回宽字符错误文本。返回的是内部缓存，下一次 SDK 调用可能覆盖。
extern "C" PACKSDK_API const wchar_t* PACKSDK_CALL Pack_GetLastError(PACK_HANDLE handle)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h != NULL) return h->lastError.c_str();
	return g_lastError.c_str();
}

// 返回 UTF-8 错误文本。Ubuntu/Linux 和跨平台 Qt 推荐使用。
extern "C" PACKSDK_API const char* PACKSDK_CALL Pack_GetLastErrorUtf8(PACK_HANDLE handle)
{
	PackHandleImpl* h = ToHandle(handle);
	if (h != NULL) return h->lastErrorUtf8.c_str();
	return g_lastErrorUtf8.c_str();
}

// 预留释放函数，用于未来 SDK 返回动态分配内存时保持 ABI 完整。
extern "C" PACKSDK_API void PACKSDK_CALL Pack_FreeMemory(void* ptr)
{
	free(ptr);
}
