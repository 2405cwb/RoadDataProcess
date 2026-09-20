#include "../PackSdk/PackSdk.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

struct Config
{
	std::string packDir;
	std::string jpgDir;
	std::string exportDir;
	uint64_t countLimit;
	int loops;
	size_t batchSize;
	bool randomOrder;
	bool warmup;

	Config()
		: countLimit(0),
		  loops(1),
		  batchSize(100),
		  randomOrder(false),
		  warmup(false)
	{
	}
};

struct BenchResult
{
	std::string name;
	uint64_t framesRead;
	uint64_t bytesRead;
	double seconds;
	size_t batchSize;

	BenchResult()
		: framesRead(0),
		  bytesRead(0),
		  seconds(0.0),
		  batchSize(100)
	{
	}
};

static void PrintUsage()
{
	std::cout
		<< "PackReadBenchmark 用法:\n"
		<< "  PackReadBenchmark -pack <图片包目录> [-jpgDir <旧单图目录>]\n"
		<< "                    [-exportDir <生成旧单图基准目录>]\n"
		<< "                    [-count N] [-loops N] [-batchSize N] [-random] [-warmup]\n\n"
		<< "示例:\n"
		<< "  PackReadBenchmark -pack .\\output\\20260616_105858_665 -count 5000 -batchSize 100\n"
		<< "  PackReadBenchmark -pack .\\pack_data -exportDir .\\single_baseline -count 5000\n"
		<< "  PackReadBenchmark -pack .\\pack_data -jpgDir .\\exported_jpg -loops 3 -random\n\n"
		<< "说明:\n"
		<< "  -pack 通过 PackSdk 读取 JPEG 原始字节，不解码 JPEG 像素。\n"
		<< "  -exportDir 从 pack 前 N 张导出单 jpg，用于和旧方案公平对比。\n"
		<< "  -jpgDir 递归读取 *.jpg/*.jpeg，作为旧单图读取基准。\n";
}

static bool ParseUInt64(const std::string& text, uint64_t* value)
{
	if (!value)
	{
		return false;
	}
	char* end = NULL;
	unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
	if (!end || *end != '\0')
	{
		return false;
	}
	*value = static_cast<uint64_t>(parsed);
	return true;
}

static bool ParseInt(const std::string& text, int* value)
{
	if (!value)
	{
		return false;
	}
	char* end = NULL;
	long parsed = std::strtol(text.c_str(), &end, 10);
	if (!end || *end != '\0')
	{
		return false;
	}
	*value = static_cast<int>(parsed);
	return true;
}

static bool ParseArgs(const std::vector<std::string>& args, Config* cfg)
{
	if (!cfg)
	{
		return false;
	}

	for (size_t i = 1; i < args.size(); ++i)
	{
		const std::string& arg = args[i];
		if (arg == "-h" || arg == "--help")
		{
			PrintUsage();
			return false;
		}
		else if ((arg == "-pack" || arg == "--pack") && i + 1 < args.size())
		{
			cfg->packDir = args[++i];
		}
		else if ((arg == "-jpgDir" || arg == "--jpgDir") && i + 1 < args.size())
		{
			cfg->jpgDir = args[++i];
		}
		else if ((arg == "-exportDir" || arg == "--exportDir") && i + 1 < args.size())
		{
			cfg->exportDir = args[++i];
		}
		else if ((arg == "-count" || arg == "--count") && i + 1 < args.size())
		{
			if (!ParseUInt64(args[++i], &cfg->countLimit))
			{
				std::cerr << "Invalid count value.\n";
				return false;
			}
		}
		else if ((arg == "-loops" || arg == "--loops") && i + 1 < args.size())
		{
			if (!ParseInt(args[++i], &cfg->loops) || cfg->loops <= 0)
			{
				std::cerr << "Invalid loops value.\n";
				return false;
			}
		}
		else if ((arg == "-batchSize" || arg == "--batchSize") && i + 1 < args.size())
		{
			uint64_t parsed = 0;
			if (!ParseUInt64(args[++i], &parsed) || parsed == 0)
			{
				std::cerr << "Invalid batchSize value.\n";
				return false;
			}
			cfg->batchSize = static_cast<size_t>(parsed);
		}
		else if (arg == "-random" || arg == "--random")
		{
			cfg->randomOrder = true;
		}
		else if (arg == "-warmup" || arg == "--warmup")
		{
			cfg->warmup = true;
		}
		else
		{
			std::cerr << "Unknown argument: " << arg << "\n";
			PrintUsage();
			return false;
		}
	}

	if (cfg->packDir.empty() && cfg->jpgDir.empty() && cfg->exportDir.empty())
	{
		PrintUsage();
		return false;
	}

	if (!cfg->exportDir.empty() && cfg->packDir.empty())
	{
		std::cerr << "-exportDir requires -pack.\n";
		return false;
	}

	return true;
}

static std::string JoinPath(const std::string& left, const std::string& right)
{
	if (left.empty())
	{
		return right;
	}
	char tail = left[left.size() - 1];
	if (tail == '\\' || tail == '/')
	{
		return left + right;
	}
#if defined(_WIN32)
	return left + "\\" + right;
#else
	return left + "/" + right;
#endif
}

static std::string ToLowerAscii(std::string value)
{
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] >= 'A' && value[i] <= 'Z')
		{
			value[i] = static_cast<char>(value[i] - 'A' + 'a');
		}
	}
	return value;
}

static bool HasJpegExtension(const std::string& path)
{
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
	{
		return false;
	}
	std::string ext = ToLowerAscii(path.substr(dot));
	return ext == ".jpg" || ext == ".jpeg";
}

static std::string FormatFrameFileName(uint64_t index)
{
	char name[64];
#if defined(_MSC_VER) && _MSC_VER < 1900
	sprintf_s(name, sizeof(name), "frame_%09llu.jpg", static_cast<unsigned long long>(index));
#else
	std::snprintf(name, sizeof(name), "frame_%09llu.jpg", static_cast<unsigned long long>(index));
#endif
	return name;
}

#if defined(_WIN32)
static std::wstring Utf8ToWide(const std::string& src)
{
	if (src.empty())
	{
		return std::wstring();
	}
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, NULL, 0);
	if (len <= 0)
	{
		return std::wstring();
	}
	std::wstring out(static_cast<size_t>(len - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, &out[0], len);
	return out;
}

static std::string WideToUtf8(const wchar_t* src)
{
	if (!src || !src[0])
	{
		return std::string();
	}
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
	if (len <= 0)
	{
		return std::string();
	}
	std::string out(static_cast<size_t>(len - 1), '\0');
	WideCharToMultiByte(CP_UTF8, 0, src, -1, &out[0], len, NULL, NULL);
	return out;
}

static bool IsDirectoryUtf8(const std::string& path)
{
	std::wstring wide = Utf8ToWide(path);
	DWORD attr = GetFileAttributesW(wide.c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static bool EnsureDirectoryUtf8(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}
	if (IsDirectoryUtf8(path))
	{
		return true;
	}
	std::wstring wide = Utf8ToWide(path);
	return CreateDirectoryW(wide.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

static FILE* OpenFileUtf8(const std::string& path, const wchar_t* mode)
{
	std::wstring wide = Utf8ToWide(path);
	return _wfopen(wide.c_str(), mode);
}

static void EnumerateJpegFiles(const std::string& rootDir, std::vector<std::string>* files)
{
	if (!files)
	{
		return;
	}

	std::string pattern = JoinPath(rootDir, "*");
	std::wstring widePattern = Utf8ToWide(pattern);
	WIN32_FIND_DATAW data;
	HANDLE handle = FindFirstFileW(widePattern.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0)
		{
			continue;
		}

		std::string name = WideToUtf8(data.cFileName);
		std::string child = JoinPath(rootDir, name);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			EnumerateJpegFiles(child, files);
		}
		else if (HasJpegExtension(child))
		{
			files->push_back(child);
		}
	} while (FindNextFileW(handle, &data));

	FindClose(handle);
}
#else
static bool IsDirectoryUtf8(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool EnsureDirectoryUtf8(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}
	if (IsDirectoryUtf8(path))
	{
		return true;
	}
	return mkdir(path.c_str(), 0755) == 0 || IsDirectoryUtf8(path);
}

static FILE* OpenFileUtf8(const std::string& path, const char* mode)
{
	return fopen(path.c_str(), mode);
}

static void EnumerateJpegFiles(const std::string& rootDir, std::vector<std::string>* files)
{
	if (!files)
	{
		return;
	}

	DIR* dir = opendir(rootDir.c_str());
	if (!dir)
	{
		return;
	}

	struct dirent* entry = NULL;
	while ((entry = readdir(dir)) != NULL)
	{
		if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		std::string child = JoinPath(rootDir, entry->d_name);
		if (IsDirectoryUtf8(child))
		{
			EnumerateJpegFiles(child, files);
		}
		else if (HasJpegExtension(child))
		{
			files->push_back(child);
		}
	}

	closedir(dir);
}
#endif

static bool ReadWholeFile(const std::string& path, std::vector<unsigned char>* buffer)
{
	if (!buffer)
	{
		return false;
	}

#if defined(_WIN32)
	FILE* fp = OpenFileUtf8(path, L"rb");
#else
	FILE* fp = OpenFileUtf8(path, "rb");
#endif
	if (!fp)
	{
		return false;
	}

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return false;
	}
	long size = ftell(fp);
	if (size < 0)
	{
		fclose(fp);
		return false;
	}
	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return false;
	}

	buffer->resize(static_cast<size_t>(size));
	if (size > 0)
	{
		size_t got = fread(buffer->data(), 1, static_cast<size_t>(size), fp);
		if (got != static_cast<size_t>(size))
		{
			fclose(fp);
			return false;
		}
	}

	fclose(fp);
	return true;
}

static std::vector<uint64_t> MakeOrder(uint64_t count, bool randomOrder)
{
	std::vector<uint64_t> order;
	order.reserve(static_cast<size_t>(count));
	for (uint64_t i = 0; i < count; ++i)
	{
		order.push_back(i);
	}
	if (randomOrder)
	{
		std::mt19937_64 rng(20260616);
		std::shuffle(order.begin(), order.end(), rng);
	}
	return order;
}

static void PrintResult(const BenchResult& result)
{
	double mb = static_cast<double>(result.bytesRead) / (1024.0 * 1024.0);
	double fps = result.seconds > 0.0 ? static_cast<double>(result.framesRead) / result.seconds : 0.0;
	double mbps = result.seconds > 0.0 ? mb / result.seconds : 0.0;
	double msPerFrame = result.framesRead > 0 ? result.seconds * 1000.0 / static_cast<double>(result.framesRead) : 0.0;
	uint64_t batchCount = result.batchSize > 0
		? (result.framesRead + static_cast<uint64_t>(result.batchSize) - 1) / static_cast<uint64_t>(result.batchSize)
		: 0;
	double msPerBatch = batchCount > 0 ? result.seconds * 1000.0 / static_cast<double>(batchCount) : 0.0;

	std::cout << "\n[" << result.name << "]\n"
		<< "读取张数        : " << result.framesRead << "\n"
		<< "读取字节数      : " << result.bytesRead << "\n"
		<< "读取耗时(秒)    : " << result.seconds << "\n"
		<< "吞吐量(张/秒)   : " << fps << "\n"
		<< "吞吐量(MB/秒)   : " << mbps << "\n"
		<< "平均每张耗时(ms): " << msPerFrame << "\n"
		<< "批大小          : " << result.batchSize << "\n"
		<< "平均每批耗时(ms): " << msPerBatch << "\n";
	std::cout << "METRIC"
		<< " framesRead=" << result.framesRead
		<< " bytesRead=" << result.bytesRead
		<< " elapsedSeconds=" << result.seconds
		<< " throughputFPS=" << fps
		<< " throughputMBps=" << mbps
		<< " avgMsPerFrame=" << msPerFrame
		<< " batchSize=" << result.batchSize
		<< " avgMsPerBatch=" << msPerBatch
		<< "\n";
}

static bool BenchmarkPack(const Config& cfg, bool warmup, BenchResult* result)
{
	if (!result)
	{
		return false;
	}

	PACK_OPEN_OPTIONS opt;
	opt.structSize = sizeof(opt);
	opt.flags = PACK_OPEN_DEFAULT;

	PACK_HANDLE handle = NULL;
	auto openStart = std::chrono::steady_clock::now();
	int ret = Pack_OpenUtf8(cfg.packDir.c_str(), &opt, &handle);
	auto openEnd = std::chrono::steady_clock::now();
	if (ret != PACK_OK)
	{
		std::cerr << "Pack_OpenUtf8 failed: " << Pack_GetLastErrorUtf8(NULL) << "\n";
		return false;
	}

	uint64_t totalCount = 0;
	ret = Pack_GetFrameCount(handle, &totalCount);
	if (ret != PACK_OK)
	{
		std::cerr << "Pack_GetFrameCount failed: " << Pack_GetLastErrorUtf8(handle) << "\n";
		Pack_Close(handle);
		return false;
	}

	uint64_t readCount = totalCount;
	if (cfg.countLimit > 0 && cfg.countLimit < readCount)
	{
		readCount = cfg.countLimit;
	}

	std::vector<uint32_t> sizes;
	sizes.resize(static_cast<size_t>(readCount));
	uint32_t maxSize = 0;
	for (uint64_t i = 0; i < readCount; ++i)
	{
		PACK_FRAME_INFO info;
		std::memset(&info, 0, sizeof(info));
		info.structSize = sizeof(info);
		ret = Pack_GetFrameInfo(handle, i, &info);
		if (ret != PACK_OK)
		{
			std::cerr << "Pack_GetFrameInfo failed at " << i << ": " << Pack_GetLastErrorUtf8(handle) << "\n";
			Pack_Close(handle);
			return false;
		}
		sizes[static_cast<size_t>(i)] = info.jpgSize;
		if (info.jpgSize > maxSize)
		{
			maxSize = info.jpgSize;
		}
	}

	std::vector<uint64_t> order = MakeOrder(readCount, cfg.randomOrder);
	std::vector<unsigned char> buffer(maxSize);
	volatile uint64_t checksum = 0;

	if (!warmup)
	{
		double openSeconds = std::chrono::duration<double>(openEnd - openStart).count();
		std::cout << "打开并建立索引耗时(秒): " << openSeconds << "\n";
		std::cout << "Pack 总图片数          : " << totalCount << "\n";
		std::cout << "本次测试图片数         : " << readCount << "\n";
	}

	auto start = std::chrono::steady_clock::now();
	for (int loop = 0; loop < cfg.loops; ++loop)
	{
		for (size_t i = 0; i < order.size(); ++i)
		{
			uint64_t frameIndex = order[i];
			uint32_t required = 0;
			ret = Pack_ReadJpeg(
				handle,
				frameIndex,
				buffer.data(),
				static_cast<uint32_t>(buffer.size()),
				&required);
			if (ret != PACK_OK)
			{
				std::cerr << "Pack_ReadJpeg failed at " << frameIndex << ": " << Pack_GetLastErrorUtf8(handle) << "\n";
				Pack_Close(handle);
				return false;
			}

			checksum += required;
			if (required > 0)
			{
				checksum += buffer[0];
			}
			result->framesRead += 1;
			result->bytesRead += required;
		}
	}
	auto end = std::chrono::steady_clock::now();

	Pack_Close(handle);
	result->name = cfg.randomOrder ? "Pack SDK 随机读取" : "Pack SDK 顺序读取";
	result->seconds = std::chrono::duration<double>(end - start).count();
	result->batchSize = cfg.batchSize;
	if (!warmup)
	{
		std::cout << "校验值          : " << checksum << "\n";
	}
	return true;
}

static bool ExportPackSubset(const Config& cfg)
{
	PACK_OPEN_OPTIONS opt;
	opt.structSize = sizeof(opt);
	opt.flags = PACK_OPEN_DEFAULT;

	PACK_HANDLE handle = NULL;
	int ret = Pack_OpenUtf8(cfg.packDir.c_str(), &opt, &handle);
	if (ret != PACK_OK)
	{
		std::cerr << "Pack_OpenUtf8 failed: " << Pack_GetLastErrorUtf8(NULL) << "\n";
		return false;
	}

	uint64_t totalCount = 0;
	ret = Pack_GetFrameCount(handle, &totalCount);
	if (ret != PACK_OK)
	{
		std::cerr << "Pack_GetFrameCount failed: " << Pack_GetLastErrorUtf8(handle) << "\n";
		Pack_Close(handle);
		return false;
	}

	uint64_t exportCount = totalCount;
	if (cfg.countLimit > 0 && cfg.countLimit < exportCount)
	{
		exportCount = cfg.countLimit;
	}

	if (!EnsureDirectoryUtf8(cfg.exportDir))
	{
		std::cerr << "Create exportDir failed: " << cfg.exportDir << "\n";
		Pack_Close(handle);
		return false;
	}

	std::cout << "导出基准图片数       : " << exportCount << "\n";
	auto start = std::chrono::steady_clock::now();
	for (uint64_t i = 0; i < exportCount; ++i)
	{
		std::string outputPath = JoinPath(cfg.exportDir, FormatFrameFileName(i));
		ret = Pack_SaveJpegUtf8(handle, i, outputPath.c_str());
		if (ret != PACK_OK)
		{
			std::cerr << "Pack_SaveJpegUtf8 failed at " << i << ": " << Pack_GetLastErrorUtf8(handle) << "\n";
			Pack_Close(handle);
			return false;
		}
		if ((i + 1) % 1000 == 0 || i + 1 == exportCount)
		{
			std::cout << "已导出 " << (i + 1) << " / " << exportCount << "\n";
		}
	}
	auto end = std::chrono::steady_clock::now();
	Pack_Close(handle);

	double seconds = std::chrono::duration<double>(end - start).count();
	double fps = seconds > 0.0 ? static_cast<double>(exportCount) / seconds : 0.0;
	std::cout << "导出耗时(秒)         : " << seconds << "\n"
		<< "导出速度(张/秒)      : " << fps << "\n";
	std::cout << "METRIC"
		<< " framesRead=" << exportCount
		<< " bytesRead=0"
		<< " elapsedSeconds=" << seconds
		<< " throughputFPS=" << fps
		<< " throughputMBps=0"
		<< " avgMsPerFrame=" << (exportCount > 0 ? seconds * 1000.0 / static_cast<double>(exportCount) : 0.0)
		<< " batchSize=" << cfg.batchSize
		<< " avgMsPerBatch=0"
		<< "\n";
	return true;
}

static bool BenchmarkJpegDir(const Config& cfg, bool warmup, BenchResult* result)
{
	if (!result)
	{
		return false;
	}

	if (!IsDirectoryUtf8(cfg.jpgDir))
	{
		std::cerr << "jpgDir is not a directory: " << cfg.jpgDir << "\n";
		return false;
	}

	std::vector<std::string> files;
	EnumerateJpegFiles(cfg.jpgDir, &files);
	std::sort(files.begin(), files.end());
	if (cfg.countLimit > 0 && cfg.countLimit < files.size())
	{
		files.resize(static_cast<size_t>(cfg.countLimit));
	}

	if (cfg.randomOrder)
	{
		std::mt19937_64 rng(20260616);
		std::shuffle(files.begin(), files.end(), rng);
	}

	if (files.empty())
	{
		std::cerr << "No jpg/jpeg files found in: " << cfg.jpgDir << "\n";
		return false;
	}

	if (!warmup)
	{
		std::cout << "本次测试 JPG 文件数    : " << files.size() << "\n";
	}

	std::vector<unsigned char> buffer;
	volatile uint64_t checksum = 0;
	auto start = std::chrono::steady_clock::now();
	for (int loop = 0; loop < cfg.loops; ++loop)
	{
		for (size_t i = 0; i < files.size(); ++i)
		{
			if (!ReadWholeFile(files[i], &buffer))
			{
				std::cerr << "Read jpg failed: " << files[i] << "\n";
				return false;
			}

			checksum += static_cast<uint64_t>(buffer.size());
			if (!buffer.empty())
			{
				checksum += buffer[0];
			}
			result->framesRead += 1;
			result->bytesRead += static_cast<uint64_t>(buffer.size());
		}
	}
	auto end = std::chrono::steady_clock::now();

	result->name = cfg.randomOrder ? "旧单图随机读取" : "旧单图顺序读取";
	result->seconds = std::chrono::duration<double>(end - start).count();
	result->batchSize = cfg.batchSize;
	if (!warmup)
	{
		std::cout << "校验值          : " << checksum << "\n";
	}
	return true;
}

static int Run(const std::vector<std::string>& args)
{
	Config cfg;
	if (!ParseArgs(args, &cfg))
	{
		return 1;
	}

	std::cout << "循环次数        : " << cfg.loops << "\n"
		<< "批大小          : " << cfg.batchSize << "\n"
		<< "读取顺序        : " << (cfg.randomOrder ? "随机" : "顺序") << "\n"
		<< "测试数量限制    : " << cfg.countLimit << "\n";

	if (!cfg.exportDir.empty())
	{
		if (!ExportPackSubset(cfg))
		{
			return 4;
		}
	}

	if (!cfg.packDir.empty() && cfg.warmup)
	{
		BenchResult warm;
		BenchmarkPack(cfg, true, &warm);
	}
	if (!cfg.jpgDir.empty() && cfg.warmup)
	{
		BenchResult warm;
		BenchmarkJpegDir(cfg, true, &warm);
	}

	if (!cfg.packDir.empty())
	{
		BenchResult result;
		if (!BenchmarkPack(cfg, false, &result))
		{
			return 2;
		}
		PrintResult(result);
	}

	if (!cfg.jpgDir.empty())
	{
		BenchResult result;
		if (!BenchmarkJpegDir(cfg, false, &result))
		{
			return 3;
		}
		PrintResult(result);
	}

	return 0;
}

#if defined(_WIN32)
int wmain(int argc, wchar_t* argv[])
{
	std::vector<std::string> args;
	args.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; ++i)
	{
		args.push_back(WideToUtf8(argv[i]));
	}
	return Run(args);
}
#else
int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	args.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; ++i)
	{
		args.push_back(argv[i]);
	}
	return Run(args);
}
#endif
