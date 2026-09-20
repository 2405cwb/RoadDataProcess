#include "ImagePool.h"
#include "SimpleLog.h"
#include "turbojpeg.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <gdiplus.h>
#include <Shlobj.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <queue>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

struct DiskStressConfig
{
	DiskStressConfig()
		: pixelMode("auto")
		, fps(20.0)
		, totalCount(0)
		, durationSec(0)
		, threadCount(4)
		, poolBlockCount(80)
		, jpegQuality(85)
		, logIntervalMs(1000)
		, jpegQueueMaxMB(0)
		, packMaxMB(800)
		, packFlushEvery(20)
		, preloadImageCount(30)
	{
	}

	std::string inputImage;
	std::string inputDir;
	std::string outputDir;
	std::string saveMode;
	std::string pixelMode;
	double fps;
	long long totalCount;
	int durationSec;
	int threadCount;
	int poolBlockCount;
	int jpegQuality;
	int logIntervalMs;
	int jpegQueueMaxMB;
	int packMaxMB;
	int packFlushEvery;
	int preloadImageCount;
};

#pragma pack(push, 1)
struct DEMO_IDX_FILE_HEADER
{
	char magic[8];
	DWORD version;
	DWORD headerSize;
	DWORD recordSize;
	DWORD packNo;
	unsigned long long recordCount;
	unsigned long long createTimeValue;
	unsigned long long reserved[4];
};

struct DEMO_IDX_RECORD
{
	unsigned long long sourceIndex;
	unsigned long long timeValue;
	unsigned long long datOffset;
	DWORD jpgSize;
	DWORD reserved;
};

struct DEMO_DAT_FRAME_HEADER
{
	DWORD magic;
	DWORD headerSize;
	unsigned long long sourceIndex;
	unsigned long long timeValue;
	DWORD jpgSize;
	DWORD reserved;
};

struct JPEG_WRITE_TASK
{
	JPEG_WRITE_TASK()
		: jpegBuf(NULL)
		, jpegSize(0)
		, index(0)
		, compressMs(0)
	{
		curTime[0] = 0;
	}

	unsigned char* jpegBuf;
	unsigned long jpegSize;
	int index;
	DWORD compressMs;
	char curTime[32];
};

struct PACK_WRITE_METRIC
{
	PACK_WRITE_METRIC()
		: appendMs(0)
		, flushMs(0)
		, openMs(0)
		, closeMs(0)
		, flushCount(0)
		, rotateCount(0)
	{
	}

	DWORD appendMs;
	DWORD flushMs;
	DWORD openMs;
	DWORD closeMs;
	int flushCount;
	int rotateCount;
};

enum SOURCE_PIXEL_MODE
{
	SOURCE_PIXEL_MODE_AUTO = 0,
	SOURCE_PIXEL_MODE_GRAY = 1,
	SOURCE_PIXEL_MODE_COLOR = 2
};

struct SOURCE_IMAGE_FRAME
{
	std::string path;
	std::vector<unsigned char> rawData;
	int width;
	int height;
	int pixelFormat;
	int channelCount;
	SOURCE_PIXEL_MODE pixelMode;

	SOURCE_IMAGE_FRAME()
		: width(0)
		, height(0)
		, pixelFormat(TJPF_GRAY)
		, channelCount(1)
		, pixelMode(SOURCE_PIXEL_MODE_GRAY)
	{
	}
};
#pragma pack(pop)

static std::wstring ToWideString(const std::string& src)
{
	if (src.empty())
	{
		return std::wstring();
	}

	int size = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, NULL, 0);
	if (size <= 0)
	{
		return std::wstring();
	}

	std::vector<wchar_t> buffer(size);
	MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, &buffer[0], size);
	return std::wstring(&buffer[0]);
}

static std::string GetExeDir()
{
	char path[MAX_PATH];
	path[0] = 0;
	GetModuleFileNameA(NULL, path, MAX_PATH);

	char* p = strrchr(path, '\\');
	if (p != NULL)
	{
		*p = 0;
	}
	return std::string(path);
}

static std::string JoinPath(const std::string& left, const std::string& right)
{
	if (left.empty())
	{
		return right;
	}

	if (left[left.size() - 1] == '\\' || left[left.size() - 1] == '/')
	{
		return left + right;
	}
	return left + "\\" + right;
}

static bool IsAbsolutePath(const std::string& path)
{
	if (path.size() >= 2 && path[1] == ':')
	{
		return true;
	}
	if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
	{
		return true;
	}
	return false;
}

static std::string NormalizePath(const std::string& path)
{
	char fullPath[MAX_PATH];
	fullPath[0] = 0;
	DWORD len = GetFullPathNameA(path.c_str(), MAX_PATH, fullPath, NULL);
	if (len > 0 && len < MAX_PATH)
	{
		return std::string(fullPath);
	}
	return path;
}

static std::string ToLowerString(const std::string& text)
{
	std::string lower = text;
	for (size_t i = 0; i < lower.size(); ++i)
	{
		if (lower[i] >= 'A' && lower[i] <= 'Z')
		{
			lower[i] = (char)(lower[i] - 'A' + 'a');
		}
	}
	return lower;
}

static bool HasSupportedImageExtension(const std::string& path)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
	{
		return false;
	}

	std::string ext = ToLowerString(path.substr(pos));
	return ext == ".jpg" ||
		ext == ".jpeg" ||
		ext == ".bmp" ||
		ext == ".png" ||
		ext == ".tif" ||
		ext == ".tiff";
}

static bool ListImageFiles(const std::string& dirPath, std::vector<std::string>& fileList, std::string& err)
{
	fileList.clear();
	if (dirPath.empty())
	{
		err = "inputDir path is empty.";
		return false;
	}

	std::string searchPath = JoinPath(dirPath, "*");
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		err = "FindFirstFile failed.";
		return false;
	}

	do
	{
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			continue;
		}

		std::string fileName = findData.cFileName;
		if (!HasSupportedImageExtension(fileName))
		{
			continue;
		}

		fileList.push_back(JoinPath(dirPath, fileName));
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);

	std::sort(fileList.begin(), fileList.end(),
		[](const std::string& left, const std::string& right)
		{
			return ToLowerString(left) < ToLowerString(right);
		});

	if (fileList.empty())
	{
		err = "no supported images found in inputDir.";
		return false;
	}

	return true;
}

static std::string ResolvePathByExeDir(const std::string& path)
{
	if (path.empty())
	{
		return path;
	}
	if (IsAbsolutePath(path))
	{
		return NormalizePath(path);
	}
	return NormalizePath(JoinPath(GetExeDir(), path));
}

static bool EnsureDirectory(const std::string& path)
{
	if (path.empty())
	{
		return false;
	}

	DWORD attr = GetFileAttributesA(path.c_str());
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		return true;
	}

	int ret = SHCreateDirectoryExA(NULL, path.c_str(), NULL);
	return ret == ERROR_SUCCESS || ret == ERROR_ALREADY_EXISTS || ret == ERROR_FILE_EXISTS;
}

static std::string MakeNowStringForName()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char buf[32];
	sprintf_s(buf, "%04d%02d%02d_%02d%02d%02d_%03d",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);
	return std::string(buf);
}

static std::string MakeFrameTimeString()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char buf[32];
	sprintf_s(buf, "%04d%02d%02d%02d%02d%02d%03d",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);
	return std::string(buf);
}

static std::string MakeNowStringForDisplay()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	char buf[64];
	sprintf_s(buf,
		"%04d-%02d-%02d %02d:%02d:%02d.%03d",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond,
		st.wMilliseconds);
	return std::string(buf);
}

static SOURCE_PIXEL_MODE ParseSourcePixelMode(const std::string& text)
{
	std::string lower = ToLowerString(text);
	if (lower == "gray" || lower == "grey")
	{
		return SOURCE_PIXEL_MODE_GRAY;
	}
	if (lower == "color" || lower == "colour" || lower == "rgb" || lower == "bgr")
	{
		return SOURCE_PIXEL_MODE_COLOR;
	}
	return SOURCE_PIXEL_MODE_AUTO;
}

static const char* PixelModeToString(SOURCE_PIXEL_MODE mode)
{
	switch (mode)
	{
	case SOURCE_PIXEL_MODE_GRAY:
		return "gray";
	case SOURCE_PIXEL_MODE_COLOR:
		return "color";
	default:
		return "auto";
	}
}

static unsigned long long ParseFrameTimeValue(const char* timeText)
{
	if (timeText == NULL || timeText[0] == 0)
	{
		return 0;
	}
	return _strtoui64(timeText, NULL, 10);
}

static bool LoadSourceImage(const std::string& path, SOURCE_PIXEL_MODE requestMode, SOURCE_IMAGE_FRAME& frame, std::string& err)
{
	std::wstring wpath = ToWideString(path);
	if (wpath.empty())
	{
		err = "inputImage path is empty or invalid.";
		return false;
	}

	Bitmap source(wpath.c_str());
	if (source.GetLastStatus() != Ok)
	{
		err = "GDI+ failed to load input image.";
		return false;
	}

	int width = (int)source.GetWidth();
	int height = (int)source.GetHeight();
	if (width <= 0 || height <= 0)
	{
		err = "inputImage width/height is invalid.";
		return false;
	}

	Bitmap converted(width, height, PixelFormat24bppRGB);
	Graphics graphics(&converted);
	if (graphics.DrawImage(&source, 0, 0, width, height) != Ok)
	{
		err = "GDI+ failed to convert input image.";
		return false;
	}

	Rect rect(0, 0, width, height);
	BitmapData bitmapData;
	if (converted.LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bitmapData) != Ok)
	{
		err = "GDI+ LockBits failed.";
		return false;
	}

	SOURCE_PIXEL_MODE actualMode = requestMode;
	if (requestMode == SOURCE_PIXEL_MODE_AUTO)
	{
		bool allGray = true;
		for (int y = 0; y < height && allGray; ++y)
		{
			unsigned char* pRow = (unsigned char*)bitmapData.Scan0 + y * bitmapData.Stride;
			for (int x = 0; x < width; ++x)
			{
				unsigned char b = pRow[x * 3 + 0];
				unsigned char g = pRow[x * 3 + 1];
				unsigned char r = pRow[x * 3 + 2];
				if (b != g || g != r)
				{
					allGray = false;
					break;
				}
			}
		}
		actualMode = allGray ? SOURCE_PIXEL_MODE_GRAY : SOURCE_PIXEL_MODE_COLOR;
	}

	frame.width = width;
	frame.height = height;
	frame.pixelMode = actualMode;
	if (actualMode == SOURCE_PIXEL_MODE_COLOR)
	{
		frame.channelCount = 3;
		frame.pixelFormat = TJPF_BGR;
		frame.rawData.resize((size_t)width * (size_t)height * 3);
		for (int y = 0; y < height; ++y)
		{
			unsigned char* pRow = (unsigned char*)bitmapData.Scan0 + y * bitmapData.Stride;
			memcpy(&frame.rawData[(size_t)y * (size_t)width * 3], pRow, (size_t)width * 3);
		}
	}
	else
	{
		frame.channelCount = 1;
		frame.pixelFormat = TJPF_GRAY;
		frame.rawData.resize((size_t)width * (size_t)height);
		for (int y = 0; y < height; ++y)
		{
			unsigned char* pRow = (unsigned char*)bitmapData.Scan0 + y * bitmapData.Stride;
			for (int x = 0; x < width; ++x)
			{
				unsigned char b = pRow[x * 3 + 0];
				unsigned char g = pRow[x * 3 + 1];
				unsigned char r = pRow[x * 3 + 2];
				frame.rawData[(size_t)y * (size_t)width + (size_t)x] =
					(unsigned char)((299 * r + 587 * g + 114 * b + 500) / 1000);
			}
		}
	}

	converted.UnlockBits(&bitmapData);
	return true;
}

static std::string ReadIniString(const std::string& iniPath, const char* section, const char* key, const char* defaultValue)
{
	char buffer[1024];
	buffer[0] = 0;
	GetPrivateProfileStringA(section, key, defaultValue, buffer, (DWORD)_countof(buffer), iniPath.c_str());
	return std::string(buffer);
}

static int ReadIniInt(const std::string& iniPath, const char* section, const char* key, int defaultValue)
{
	return GetPrivateProfileIntA(section, key, defaultValue, iniPath.c_str());
}

static long long ReadIniInt64(const std::string& iniPath, const char* section, const char* key, long long defaultValue)
{
	std::string value = ReadIniString(iniPath, section, key, "");
	if (value.empty())
	{
		return defaultValue;
	}
	return _atoi64(value.c_str());
}

static double ReadIniDouble(const std::string& iniPath, const char* section, const char* key, double defaultValue)
{
	std::string value = ReadIniString(iniPath, section, key, "");
	if (value.empty())
	{
		return defaultValue;
	}
	return atof(value.c_str());
}

class CDiskStressApp
{
public:
	CDiskStressApp()
		: m_width(0)
		, m_height(0)
		, m_stopRequested(false)
		, m_producerFinished(false)
		, m_compressFinished(false)
		, m_nextOutputIndex(0)
		, m_packDataFile(NULL)
		, m_packIndexFile(NULL)
		, m_packSerialNo(0)
		, m_packCurrentNo(-1)
		, m_packCreateTimeValue(0)
		, m_packCurrentFrameCount(0)
		, m_packFramesSinceFlush(0)
		, m_pendingJpegBytes(0)
		, m_totalDropped(0)
		, m_totalSaved(0)
		, m_secSavedCount(0)
		, m_secSingleCompressedCount(0)
		, m_secDroppedCount(0)
		, m_secCompressMs(0)
		, m_secWriteMs(0)
		, m_secOpenMs(0)
		, m_secFileWriteMs(0)
		, m_secCloseMs(0)
		, m_secDirCreateMs(0)
		, m_secDirCreateCount(0)
		, m_secCompressMaxMs(0)
		, m_secWriteMaxMs(0)
		, m_secOpenMaxMs(0)
		, m_secFileWriteMaxMs(0)
		, m_secCloseMaxMs(0)
		, m_secDirCreateMaxMs(0)
		, m_secPackCompressedCount(0)
		, m_secPackWrittenCount(0)
		, m_secPackAppendMs(0)
		, m_secPackFlushCount(0)
		, m_secPackFlushMs(0)
		, m_secPackRotateCount(0)
		, m_secPackOpenMs(0)
		, m_secPackCloseMs(0)
		, m_secPackAppendMaxMs(0)
		, m_secPackFlushMaxMs(0)
		, m_secPackOpenMaxMs(0)
		, m_secPackCloseMaxMs(0)
		, m_runStartTickMs(0)
		, m_packAbnormalSampleCount(0)
		, m_packPendingSampleCount(0)
		, m_packTotalFlushEventCount(0)
		, m_packFlushOver100Count(0)
		, m_packFlushOver300Count(0)
		, m_packFlushOver500Count(0)
		, m_packPeakPendingJpegCount(0)
		, m_packPeakPendingJpegMB(0.0)
		, m_packMinRawFree(2147483647)
		, m_packPeakRawQueue(0)
		, m_packGlobalMaxCompressMs(0)
		, m_packGlobalMaxAppendMs(0)
		, m_packGlobalMaxFlushMs(0)
		, m_packGlobalMaxOpenMs(0)
		, m_packGlobalMaxCloseMs(0)
		, m_singleAbnormalSampleCount(0)
		, m_singlePendingSampleCount(0)
		, m_singleMinRawFree(2147483647)
		, m_singlePeakRawQueue(0)
		, m_singleGlobalMaxCompressMs(0)
		, m_singleGlobalMaxOpenMs(0)
		, m_singleGlobalMaxWriteMs(0)
		, m_singleGlobalMaxCloseMs(0)
		, m_singleGlobalMaxDirCreateMs(0)
	{
		InitializeCriticalSection(&m_statLock);
		InitializeCriticalSection(&m_lostLock);
		InitializeCriticalSection(&m_packLock);
		InitializeCriticalSection(&m_jpegLock);
	}

	~CDiskStressApp()
	{
		DWORD closeMs = 0;
		CloseCurrentPackFiles(closeMs);
		ClearPendingJpegTasks();
		DeleteCriticalSection(&m_jpegLock);
		DeleteCriticalSection(&m_packLock);
		DeleteCriticalSection(&m_lostLock);
		DeleteCriticalSection(&m_statLock);
	}

	void RequestStop()
	{
		m_stopRequested = true;
	}

	int Run(const DiskStressConfig& cfg)
	{
		m_cfg = cfg;
		m_stopRequested = false;
		m_producerFinished = false;
		m_compressFinished = false;
		m_nextOutputIndex = 0;
		m_totalDropped = 0;
		m_totalSaved = 0;
		m_runStartTickMs = GetTickCount64();
		ResetPackSummaryState();
		if (!Prepare())
		{
			return 1;
		}

		m_log.PrintLine("DiskStressDemo started.");

		for (int i = 0; i < m_cfg.threadCount; ++i)
		{
			m_saveThreads.push_back(std::thread(&CDiskStressApp::SaveThreadProc, this, i));
		}
		m_writerThread = std::thread(&CDiskStressApp::PackWriterThreadProc, this);
		m_statThread = std::thread(&CDiskStressApp::StatThreadProc, this);
		m_producerThread = std::thread(&CDiskStressApp::ProducerThreadProc, this);

		m_producerThread.join();
		for (size_t i = 0; i < m_saveThreads.size(); ++i)
		{
			m_saveThreads[i].join();
		}
		m_compressFinished = true;
		if (m_writerThread.joinable())
		{
			m_writerThread.join();
		}
		DWORD packCloseMs = 0;
		CloseCurrentPackFiles(packCloseMs);

		m_stopRequested = true;
		m_statThread.join();

		FlushLostLogs();
		FlushSaveLog(true);
		WriteSummaryLog();

		std::ostringstream oss;
		oss << "DiskStressDemo finished. saved=" << m_totalSaved
			<< ", dropped=" << m_totalDropped
			<< ", outputDir=" << m_runOutputDir;
		m_log.PrintLine(oss.str());
		m_log.Close();
		m_saveLog.Close();
		m_summaryLog.Close();
		return 0;
	}

private:
	bool LoadSourceFrames()
	{
		m_sourceFrames.clear();
		m_width = 0;
		m_height = 0;

		if (!m_cfg.inputDir.empty())
		{
			return LoadSourceImagesFromDir();
		}

		if (!m_cfg.inputImage.empty())
		{
			return LoadSingleSourceImage();
		}

		std::cout << "inputImage and inputDir are both empty." << std::endl;
		return false;
	}

	bool LoadSingleSourceImage()
	{
		SOURCE_IMAGE_FRAME frame;
		frame.path = m_cfg.inputImage;

		std::string err;
		if (!LoadSourceImage(frame.path, ParseSourcePixelMode(m_cfg.pixelMode), frame, err))
		{
			std::cout << "load input image failed: " << err << std::endl;
			m_log.PrintLine(std::string("load input image failed: ") + err);
			return false;
		}

		m_width = frame.width;
		m_height = frame.height;
		m_sourceFrames.push_back(frame);
		return true;
	}

	bool LoadSourceImagesFromDir()
	{
		std::vector<std::string> imageFiles;
		std::string err;
		if (!ListImageFiles(m_cfg.inputDir, imageFiles, err))
		{
			std::cout << "scan inputDir failed: " << err << std::endl;
			m_log.PrintLine(std::string("scan inputDir failed: ") + err);
			return false;
		}

		int targetCount = m_cfg.preloadImageCount;
		if (targetCount <= 0 || targetCount > (int)imageFiles.size())
		{
			targetCount = (int)imageFiles.size();
		}

		for (int i = 0; i < targetCount; ++i)
		{
			SOURCE_IMAGE_FRAME frame;
			frame.path = imageFiles[i];
			if (!LoadSourceImage(frame.path, ParseSourcePixelMode(m_cfg.pixelMode), frame, err))
			{
				std::cout << "load image from inputDir failed: " << frame.path << ", err=" << err << std::endl;
				m_log.PrintLine(std::string("load image from inputDir failed: ") + frame.path + ", err=" + err);
				return false;
			}

			if (m_sourceFrames.empty())
			{
				m_width = frame.width;
				m_height = frame.height;
			}
			else if (frame.width != m_width || frame.height != m_height)
			{
				std::ostringstream oss;
				oss << "image size mismatch: " << frame.path
					<< ", current=" << frame.width << "x" << frame.height
					<< ", expected=" << m_width << "x" << m_height;
				std::cout << oss.str() << std::endl;
				m_log.PrintLine(oss.str());
				return false;
			}

			m_sourceFrames.push_back(frame);
		}

		if (m_sourceFrames.empty())
		{
			std::cout << "no source images preloaded." << std::endl;
			m_log.PrintLine("no source images preloaded.");
			return false;
		}

		int grayCount = 0;
		int colorCount = 0;
		for (size_t i = 0; i < m_sourceFrames.size(); ++i)
		{
			if (m_sourceFrames[i].pixelMode == SOURCE_PIXEL_MODE_COLOR)
			{
				++colorCount;
			}
			else
			{
				++grayCount;
			}
		}

		std::ostringstream oss;
		oss << "preloaded source images from inputDir: found=" << imageFiles.size()
			<< ", loaded=" << m_sourceFrames.size()
			<< ", gray=" << grayCount
			<< ", color=" << colorCount
			<< ", first=" << m_sourceFrames.front().path;
		if (m_sourceFrames.size() > 1)
		{
			oss << ", last=" << m_sourceFrames.back().path;
		}
		m_log.PrintLine(oss.str());
		return true;
	}

	bool Prepare()
	{
		if (m_cfg.outputDir.empty())
		{
			m_cfg.outputDir = JoinPath(GetExeDir(), "output");
		}
		if (!EnsureDirectory(m_cfg.outputDir))
		{
			std::cout << "failed to create outputDir: " << m_cfg.outputDir << std::endl;
			return false;
		}

		m_runOutputDir = JoinPath(m_cfg.outputDir, MakeNowStringForName());
		if (!EnsureDirectory(m_runOutputDir))
		{
			std::cout << "failed to create run directory: " << m_runOutputDir << std::endl;
			return false;
		}

		std::string logPath = JoinPath(m_runOutputDir, "log.txt");
		std::string saveLogPath = JoinPath(m_runOutputDir, "SaveLog.txt");
		std::string summaryLogPath = JoinPath(m_runOutputDir, "SaveSummary.txt");
		if (!m_log.Open(logPath.c_str()) || !m_saveLog.Open(saveLogPath.c_str()) || !m_summaryLog.Open(summaryLogPath.c_str()))
		{
			std::cout << "failed to open log files." << std::endl;
			return false;
		}

		if (!LoadSourceFrames())
		{
			return false;
		}

		size_t blockSize = 0;
		int grayCount = 0;
		int colorCount = 0;
		for (size_t i = 0; i < m_sourceFrames.size(); ++i)
		{
			if (m_sourceFrames[i].rawData.size() > blockSize)
			{
				blockSize = m_sourceFrames[i].rawData.size();
			}
			if (m_sourceFrames[i].pixelMode == SOURCE_PIXEL_MODE_COLOR)
			{
				++colorCount;
			}
			else
			{
				++grayCount;
			}
		}
		if (!m_pool.InitPool(blockSize, m_cfg.poolBlockCount))
		{
			std::cout << "InitPool failed." << std::endl;
			m_log.PrintLine("InitPool failed.");
			return false;
		}

		std::ostringstream oss;
		oss << "config: inputImage=" << m_cfg.inputImage
			<< ", inputDir=" << m_cfg.inputDir
			<< ", outputDir=" << m_runOutputDir
			<< ", saveMode=" << m_cfg.saveMode
			<< ", pixelMode=" << m_cfg.pixelMode
			<< ", width=" << m_width
			<< ", height=" << m_height
			<< ", preloadImageCount=" << m_cfg.preloadImageCount
			<< ", loadedSourceCount=" << m_sourceFrames.size()
			<< ", loadedGrayCount=" << grayCount
			<< ", loadedColorCount=" << colorCount
			<< ", fps=" << m_cfg.fps
			<< ", totalCount=" << m_cfg.totalCount
			<< ", durationSec=" << m_cfg.durationSec
			<< ", threadCount=" << m_cfg.threadCount
			<< ", poolBlockCount=" << m_cfg.poolBlockCount
			<< ", jpegQuality=" << m_cfg.jpegQuality
			<< ", logIntervalMs=" << m_cfg.logIntervalMs
			<< ", jpegQueueMaxMB=" << m_cfg.jpegQueueMaxMB
			<< ", packMaxMB=" << m_cfg.packMaxMB
			<< ", packFlushEvery=" << m_cfg.packFlushEvery;
		m_log.PrintLine(oss.str());
		return true;
	}

	void ProducerThreadProc()
	{
		long long index = 0;
		std::chrono::steady_clock::time_point beginTime = std::chrono::steady_clock::now();
		double effectiveFps = m_cfg.fps;
		if (effectiveFps < 0.1)
		{
			effectiveFps = 0.1;
		}

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcNow;
		QueryPerformanceFrequency(&qpcFreq);
		QueryPerformanceCounter(&qpcNow);

		LONGLONG intervalTicks = (LONGLONG)((double)qpcFreq.QuadPart / effectiveFps + 0.5);
		if (intervalTicks < 1)
		{
			intervalTicks = 1;
		}
		LONGLONG nextTick = qpcNow.QuadPart;

		while (!m_stopRequested)
		{
			if (m_cfg.totalCount > 0 && index >= m_cfg.totalCount)
			{
				break;
			}

			if (m_cfg.durationSec > 0)
			{
				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
				long long elapsedMs = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(now - beginTime).count();
				if (elapsedMs >= (long long)m_cfg.durationSec * 1000)
				{
					break;
				}
			}

			QueryPerformanceCounter(&qpcNow);
			if (qpcNow.QuadPart < nextTick)
			{
				DWORD sleepMs = (DWORD)(((nextTick - qpcNow.QuadPart) * 1000) / qpcFreq.QuadPart);
				if (sleepMs > 1)
				{
					Sleep(sleepMs - 1);
				}
				else
				{
					Sleep(0);
				}
				continue;
			}

			char* pBlock = m_pool.AcquireBlock();
			index++;
			if (pBlock == NULL)
			{
				AddLostFrame(index, "应用层缓存池为空，保存线程处理速度跟不上");
			}
			else
			{
				const SOURCE_IMAGE_FRAME& sourceFrame = m_sourceFrames[(size_t)((index - 1) % (long long)m_sourceFrames.size())];
				memcpy(pBlock, &sourceFrame.rawData[0], sourceFrame.rawData.size());
				std::string timeText = MakeFrameTimeString();
				m_pool.PushData(pBlock, sourceFrame.rawData.size(), sourceFrame.width, sourceFrame.height, sourceFrame.pixelFormat, sourceFrame.channelCount, (int)index, timeText.c_str());
			}

			nextTick += intervalTicks;
			if (qpcNow.QuadPart - nextTick > intervalTicks * 4)
			{
				nextTick = qpcNow.QuadPart + intervalTicks;
			}
		}

		m_producerFinished = true;
		m_log.PrintLine("producer thread finished.");
	}

	void SaveThreadProc(int workerId)
	{
		DISK_IMAGE_TASK task;
		tjhandle tjInstance = tjInitCompress();
		unsigned char* jpegBuf = NULL;
		unsigned long jpegSize = 0;

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);

		while (true)
		{
			if (m_pool.GetDataOne(task))
			{
				DWORD compressMs = 0;

				jpegBuf = NULL;
				jpegSize = 0;
				QueryPerformanceCounter(&qpcBegin);
				if (tjInstance == NULL ||
					tjCompress2(
						tjInstance,
						(const unsigned char*)task.buff,
						task.width,
						task.width * task.channelCount,
						task.height,
						task.pixelFormat,
						&jpegBuf,
						&jpegSize,
						task.pixelFormat == TJPF_GRAY ? TJSAMP_GRAY : TJSAMP_444,
						m_cfg.jpegQuality,
						0) != 0)
				{
					QueryPerformanceCounter(&qpcEnd);
					compressMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);
					AddLostFrame(task.index, "TurboJPEG 压缩失败");
					if (jpegBuf != NULL)
					{
						tjFree(jpegBuf);
						jpegBuf = NULL;
					}
					if (tjInstance != NULL)
					{
						tjDestroy(tjInstance);
						tjInstance = tjInitCompress();
					}
					m_pool.RecycleBlock(task.buff);
					continue;
				}
				QueryPerformanceCounter(&qpcEnd);
				compressMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

				m_pool.RecycleBlock(task.buff);
				task.buff = NULL;

				if (!PushPendingJpegTask(jpegBuf, jpegSize, task.index, task.curTime, compressMs))
				{
					if (m_cfg.saveMode == "pack")
					{
						AddPackCompressSample(compressMs);
					}
					else
					{
						AddSingleCompressSample(compressMs);
					}
					AddLostFrame(task.index, m_cfg.jpegQueueMaxMB > 0 ? "JPG待写队列超过上限" : "压缩后放入待写队列失败");
					if (jpegBuf != NULL)
					{
						tjFree(jpegBuf);
						jpegBuf = NULL;
					}
					continue;
				}

				if (m_cfg.saveMode == "pack")
				{
					AddPackCompressSample(compressMs);
				}
				jpegBuf = NULL;
			}
			else
			{
				if (m_producerFinished && m_pool.GetReadySize() <= 0)
				{
					break;
				}
				Sleep(1);
			}
		}

		if (tjInstance != NULL)
		{
			tjDestroy(tjInstance);
			tjInstance = NULL;
		}

		std::ostringstream oss;
		oss << "save thread " << workerId << " finished.";
		m_log.PrintLine(oss.str());
	}

	void PackWriterThreadProc()
	{
		JPEG_WRITE_TASK task;
		int lastDirIndex = -1;
		std::string currentDir;
		while (true)
		{
			if (GetPendingJpegTask(task))
			{
				bool writeOk = false;
				if (m_cfg.saveMode == "pack")
				{
					PACK_WRITE_METRIC metric;
					writeOk = WritePackFrame(task, metric);
					if (writeOk)
					{
						AddPackWriteSample(metric);
						m_totalSaved++;
					}
				}
				else
				{
					DWORD dirCreateMs = 0;
					DWORD openMs = 0;
					DWORD fileWriteMs = 0;
					DWORD closeMs = 0;
					writeOk = WriteSingleFrame(task, lastDirIndex, currentDir, dirCreateMs, openMs, fileWriteMs, closeMs);
					if (writeOk)
					{
						AddSaveTimingSample(task.compressMs, openMs, fileWriteMs, closeMs, dirCreateMs);
						m_totalSaved++;
					}
				}

				if (!writeOk)
				{
					AddLostFrame(task.index, m_cfg.saveMode == "pack" ? "整包写入失败" : "单图写入失败");
				}

				if (task.jpegBuf != NULL)
				{
					tjFree(task.jpegBuf);
					task.jpegBuf = NULL;
				}
			}
			else
			{
				if (m_compressFinished && GetPendingJpegCount() <= 0)
				{
					break;
				}
				Sleep(1);
			}
		}

		m_log.PrintLine(m_cfg.saveMode == "pack" ? "pack writer thread finished." : "single writer thread finished.");
	}

	void StatThreadProc()
	{
		while (!m_stopRequested)
		{
			Sleep(m_cfg.logIntervalMs);
			FlushLostLogs();
			FlushSaveLog(false);

			if (m_cfg.saveMode == "pack")
			{
				if (m_producerFinished && m_compressFinished && m_pool.GetReadySize() <= 0 && GetPendingJpegCount() <= 0)
				{
					break;
				}
			}
			else if (m_producerFinished && m_pool.GetReadySize() <= 0)
			{
				break;
			}
		}
	}

	bool EnsureImageDir(int dirIndex, std::string& absDir, DWORD& dirCreateMs)
	{
		char dirName[64];
		sprintf_s(dirName, "Image_%04d", dirIndex);
		absDir = JoinPath(m_runOutputDir, dirName);

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);
		QueryPerformanceCounter(&qpcBegin);
		bool ok = EnsureDirectory(absDir);
		QueryPerformanceCounter(&qpcEnd);
		dirCreateMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);
		return ok;
	}

	bool WriteSingleFrame(
		const JPEG_WRITE_TASK& task,
		int& lastDirIndex,
		std::string& currentDir,
		DWORD& dirCreateMs,
		DWORD& openMs,
		DWORD& fileWriteMs,
		DWORD& closeMs)
	{
		if (task.jpegBuf == NULL || task.jpegSize == 0)
		{
			return false;
		}

		long long outputIndex = ++m_nextOutputIndex;
		int dirIndex = (int)((outputIndex - 1) / 1000);
		int surIndex = (int)((outputIndex - 1) % 1000);

		if (lastDirIndex != dirIndex || currentDir.empty())
		{
			if (!EnsureImageDir(dirIndex, currentDir, dirCreateMs))
			{
				return false;
			}
			lastDirIndex = dirIndex;
		}

		char imagePath[MAX_PATH];
		sprintf_s(imagePath, "%s\\%03d_%s.jpg", currentDir.c_str(), surIndex, task.curTime);

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);

		FILE* fp = NULL;
		QueryPerformanceCounter(&qpcBegin);
		fopen_s(&fp, imagePath, "wb");
		QueryPerformanceCounter(&qpcEnd);
		openMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);
		if (fp == NULL)
		{
			return false;
		}

		QueryPerformanceCounter(&qpcBegin);
		size_t writtenSize = fwrite(task.jpegBuf, 1, task.jpegSize, fp);
		QueryPerformanceCounter(&qpcEnd);
		fileWriteMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

		QueryPerformanceCounter(&qpcBegin);
		fclose(fp);
		QueryPerformanceCounter(&qpcEnd);
		closeMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

		return writtenSize == task.jpegSize;
	}

	void FillCurrentPackIdxHeader(DEMO_IDX_FILE_HEADER& header)
	{
		memset(&header, 0, sizeof(header));
		memcpy(header.magic, "IOVIDX1", 7);
		header.version = 1;
		header.headerSize = sizeof(header);
		header.recordSize = sizeof(DEMO_IDX_RECORD);
		header.packNo = (m_packCurrentNo >= 0) ? (DWORD)m_packCurrentNo : 0;
		header.recordCount = (unsigned long long)m_packCurrentFrameCount;
		header.createTimeValue = m_packCreateTimeValue;
	}

	bool FlushCurrentPackIdxHeaderUnlocked()
	{
		if (m_packIndexFile == NULL)
		{
			return true;
		}

		DEMO_IDX_FILE_HEADER header;
		FillCurrentPackIdxHeader(header);

		__int64 endPos = _ftelli64(m_packIndexFile);
		if (endPos < 0)
		{
			return false;
		}

		if (_fseeki64(m_packIndexFile, 0, SEEK_SET) != 0)
		{
			return false;
		}

		size_t written = fwrite(&header, 1, sizeof(header), m_packIndexFile);
		fflush(m_packIndexFile);
		if (_fseeki64(m_packIndexFile, endPos, SEEK_SET) != 0)
		{
			return false;
		}

		return written == sizeof(header);
	}

	bool WritePackFrame(const JPEG_WRITE_TASK& task, PACK_WRITE_METRIC& metric)
	{
		if (task.jpegBuf == NULL || task.jpegSize == 0)
		{
			return false;
		}

		EnterCriticalSection(&m_packLock);

		bool ok = RotatePackFilesIfNeeded(task.jpegSize, metric);
		if (!ok || m_packDataFile == NULL || m_packIndexFile == NULL)
		{
			LeaveCriticalSection(&m_packLock);
			return false;
		}

		unsigned long long timeValue = ParseFrameTimeValue(task.curTime);

		DEMO_DAT_FRAME_HEADER datHeader;
		datHeader.magic = 0x3147504A;
		datHeader.headerSize = sizeof(datHeader);
		datHeader.sourceIndex = (unsigned long long)task.index;
		datHeader.timeValue = timeValue;
		datHeader.jpgSize = (DWORD)task.jpegSize;
		datHeader.reserved = 0;

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);
		QueryPerformanceCounter(&qpcBegin);

		__int64 offset = _ftelli64(m_packDataFile);
		size_t writeCount = 0;
		writeCount += fwrite(&datHeader, 1, sizeof(datHeader), m_packDataFile);
		writeCount += fwrite(task.jpegBuf, 1, task.jpegSize, m_packDataFile);

		DEMO_IDX_RECORD idxRecord;
		idxRecord.sourceIndex = (unsigned long long)task.index;
		idxRecord.timeValue = timeValue;
		idxRecord.datOffset = (unsigned long long)offset;
		idxRecord.jpgSize = (DWORD)task.jpegSize;
		idxRecord.reserved = 0;
		size_t idxWritten = fwrite(&idxRecord, 1, sizeof(idxRecord), m_packIndexFile);
		QueryPerformanceCounter(&qpcEnd);
		metric.appendMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

		m_packCurrentFrameCount++;
		m_packFramesSinceFlush++;
		if (m_cfg.packFlushEvery <= 1 || m_packFramesSinceFlush >= m_cfg.packFlushEvery)
		{
			QueryPerformanceCounter(&qpcBegin);
			if (fflush(m_packDataFile) != 0)
			{
				ok = false;
			}
			if (!FlushCurrentPackIdxHeaderUnlocked())
			{
				ok = false;
			}
			QueryPerformanceCounter(&qpcEnd);
			metric.flushMs = (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);
			metric.flushCount = 1;
			m_packFramesSinceFlush = 0;
		}

		size_t expectedDataBytes = sizeof(datHeader) + task.jpegSize;
		ok = ok && (writeCount == expectedDataBytes && idxWritten == sizeof(idxRecord));

		LeaveCriticalSection(&m_packLock);
		return ok;
	}

	bool RotatePackFilesIfNeeded(unsigned long jpegSize, PACK_WRITE_METRIC& metric)
	{
		unsigned long long packMaxBytes = (unsigned long long)m_cfg.packMaxMB * 1024ULL * 1024ULL;
		if (packMaxBytes < (unsigned long long)sizeof(DEMO_DAT_FRAME_HEADER) + 1ULL)
		{
			packMaxBytes = (unsigned long long)sizeof(DEMO_DAT_FRAME_HEADER) + 1ULL;
		}

		if (m_packDataFile != NULL && m_packIndexFile != NULL)
		{
			__int64 currentDatSize = _ftelli64(m_packDataFile);
			if (currentDatSize >= 0)
			{
				unsigned long long projectedSize =
					(unsigned long long)currentDatSize +
					(unsigned long long)sizeof(DEMO_DAT_FRAME_HEADER) +
					(unsigned long long)jpegSize;
				if (m_packCurrentFrameCount <= 0 || projectedSize <= packMaxBytes)
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}

		DWORD localCloseMs = 0;
		if (!CloseCurrentPackFilesUnlocked(localCloseMs))
		{
			return false;
		}
		metric.closeMs += localCloseMs;

		char datName[64];
		char idxName[64];
		sprintf_s(datName, "Pack_%06d.dat", m_packSerialNo);
		sprintf_s(idxName, "Pack_%06d.idx", m_packSerialNo);
		std::string datPath = JoinPath(m_runOutputDir, datName);
		std::string idxPath = JoinPath(m_runOutputDir, idxName);

		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);

		FILE* datFile = NULL;
		FILE* idxFile = NULL;
		QueryPerformanceCounter(&qpcBegin);
		fopen_s(&datFile, datPath.c_str(), "wb");
		fopen_s(&idxFile, idxPath.c_str(), "wb");
		QueryPerformanceCounter(&qpcEnd);
		metric.openMs += (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

		if (datFile == NULL || idxFile == NULL)
		{
			if (datFile != NULL)
			{
				fclose(datFile);
			}
			if (idxFile != NULL)
			{
				fclose(idxFile);
			}
			return false;
		}

		metric.rotateCount++;
		m_packDataFile = datFile;
		m_packIndexFile = idxFile;
		m_packCurrentNo = m_packSerialNo;
		m_packCreateTimeValue = ParseFrameTimeValue(MakeFrameTimeString().c_str());
		m_packCurrentFrameCount = 0;
		m_packFramesSinceFlush = 0;

		DEMO_IDX_FILE_HEADER header;
		FillCurrentPackIdxHeader(header);
		size_t headerWritten = fwrite(&header, 1, sizeof(header), m_packIndexFile);
		if (headerWritten != sizeof(header))
		{
			fclose(m_packDataFile);
			fclose(m_packIndexFile);
			m_packDataFile = NULL;
			m_packIndexFile = NULL;
			m_packCurrentNo = -1;
			m_packCreateTimeValue = 0;
			m_packCurrentFrameCount = 0;
			m_packFramesSinceFlush = 0;
			return false;
		}

		m_packSerialNo++;
		return true;
	}

	void CloseCurrentPackFiles(DWORD& closeMs)
	{
		EnterCriticalSection(&m_packLock);
		CloseCurrentPackFilesUnlocked(closeMs);
		LeaveCriticalSection(&m_packLock);
	}

	bool CloseCurrentPackFilesUnlocked(DWORD& closeMs)
	{
		LARGE_INTEGER qpcFreq;
		LARGE_INTEGER qpcBegin;
		LARGE_INTEGER qpcEnd;
		QueryPerformanceFrequency(&qpcFreq);

		if (m_packDataFile == NULL && m_packIndexFile == NULL)
		{
			m_packCurrentFrameCount = 0;
			m_packFramesSinceFlush = 0;
			m_packCurrentNo = -1;
			m_packCreateTimeValue = 0;
			return true;
		}

		QueryPerformanceCounter(&qpcBegin);
		if (m_packDataFile != NULL || m_packIndexFile != NULL)
		{
			FlushCurrentPackIdxHeaderUnlocked();
		}

		if (m_packDataFile != NULL)
		{
			fflush(m_packDataFile);
		}
		if (m_packIndexFile != NULL)
		{
			fflush(m_packIndexFile);
		}
		if (m_packDataFile != NULL)
		{
			fclose(m_packDataFile);
			m_packDataFile = NULL;
		}
		if (m_packIndexFile != NULL)
		{
			fclose(m_packIndexFile);
			m_packIndexFile = NULL;
		}
		QueryPerformanceCounter(&qpcEnd);
		closeMs += (DWORD)((qpcEnd.QuadPart - qpcBegin.QuadPart) * 1000 / qpcFreq.QuadPart);

		m_packCurrentFrameCount = 0;
		m_packFramesSinceFlush = 0;
		m_packCurrentNo = -1;
		m_packCreateTimeValue = 0;
		return true;
	}

	void AddLostFrame(long long index, const char* reason)
	{
		std::ostringstream oss;
		oss << "丢帧序号=" << index << "，原因=" << reason;

		EnterCriticalSection(&m_lostLock);
		m_lostLogs.push(oss.str());
		LeaveCriticalSection(&m_lostLock);

		EnterCriticalSection(&m_statLock);
		m_secDroppedCount++;
		m_totalDropped++;
		LeaveCriticalSection(&m_statLock);
	}

	void FlushLostLogs()
	{
		std::queue<std::string> localQueue;
		EnterCriticalSection(&m_lostLock);
		std::swap(localQueue, m_lostLogs);
		LeaveCriticalSection(&m_lostLock);

		while (!localQueue.empty())
		{
			m_log.PrintLine(localQueue.front());
			localQueue.pop();
		}
	}

	bool PushPendingJpegTask(unsigned char* jpegBuf, unsigned long jpegSize, int index, const char* curTime, DWORD compressMs)
	{
		if (jpegBuf == NULL || jpegSize == 0)
		{
			return false;
		}

		unsigned long long jpegQueueMaxBytes = 0;
		if (m_cfg.jpegQueueMaxMB > 0)
		{
			jpegQueueMaxBytes = (unsigned long long)m_cfg.jpegQueueMaxMB * 1024ULL * 1024ULL;
		}

		EnterCriticalSection(&m_jpegLock);
		if (jpegQueueMaxBytes > 0 && m_pendingJpegBytes + jpegSize > jpegQueueMaxBytes)
		{
			LeaveCriticalSection(&m_jpegLock);
			return false;
		}

		JPEG_WRITE_TASK task;
		task.jpegBuf = jpegBuf;
		task.jpegSize = jpegSize;
		task.index = index;
		task.compressMs = compressMs;
		if (curTime != NULL)
		{
			strncpy_s(task.curTime, _countof(task.curTime), curTime, _TRUNCATE);
		}
		m_pendingJpegQueue.push(task);
		m_pendingJpegBytes += jpegSize;
		int pendingCount = (int)m_pendingJpegQueue.size();
		double pendingMB = (double)m_pendingJpegBytes / (1024.0 * 1024.0);
		LeaveCriticalSection(&m_jpegLock);

		EnterCriticalSection(&m_statLock);
		if (pendingCount > m_packPeakPendingJpegCount || pendingMB > m_packPeakPendingJpegMB)
		{
			m_packPeakPendingJpegCount = pendingCount;
			m_packPeakPendingJpegMB = pendingMB;
			m_packPeakPendingTime = MakeNowStringForDisplay();
		}
		LeaveCriticalSection(&m_statLock);
		return true;
	}

	bool GetPendingJpegTask(JPEG_WRITE_TASK& task)
	{
		EnterCriticalSection(&m_jpegLock);
		if (m_pendingJpegQueue.empty())
		{
			LeaveCriticalSection(&m_jpegLock);
			return false;
		}

		task = m_pendingJpegQueue.front();
		m_pendingJpegQueue.pop();
		if (m_pendingJpegBytes >= task.jpegSize)
		{
			m_pendingJpegBytes -= task.jpegSize;
		}
		else
		{
			m_pendingJpegBytes = 0;
		}
		LeaveCriticalSection(&m_jpegLock);
		return true;
	}

	int GetPendingJpegCount()
	{
		EnterCriticalSection(&m_jpegLock);
		int count = (int)m_pendingJpegQueue.size();
		LeaveCriticalSection(&m_jpegLock);
		return count;
	}

	unsigned long long GetPendingJpegBytes()
	{
		EnterCriticalSection(&m_jpegLock);
		unsigned long long bytes = m_pendingJpegBytes;
		LeaveCriticalSection(&m_jpegLock);
		return bytes;
	}

	void ClearPendingJpegTasks()
	{
		EnterCriticalSection(&m_jpegLock);
		while (!m_pendingJpegQueue.empty())
		{
			JPEG_WRITE_TASK task = m_pendingJpegQueue.front();
			m_pendingJpegQueue.pop();
			if (task.jpegBuf != NULL)
			{
				tjFree(task.jpegBuf);
				task.jpegBuf = NULL;
			}
		}
		m_pendingJpegBytes = 0;
		LeaveCriticalSection(&m_jpegLock);
	}

	void AddPackCompressSample(DWORD compressMs)
	{
		EnterCriticalSection(&m_statLock);
		m_secPackCompressedCount++;
		m_secCompressMs += compressMs;
		if (compressMs > m_secCompressMaxMs)
		{
			m_secCompressMaxMs = compressMs;
		}
		LeaveCriticalSection(&m_statLock);
	}

	void AddSingleCompressSample(DWORD compressMs)
	{
		const std::string nowText = MakeNowStringForDisplay();

		EnterCriticalSection(&m_statLock);
		m_secSingleCompressedCount++;
		m_secCompressMs += compressMs;
		if (compressMs > m_secCompressMaxMs)
		{
			m_secCompressMaxMs = compressMs;
		}
		if (compressMs > m_singleGlobalMaxCompressMs)
		{
			m_singleGlobalMaxCompressMs = compressMs;
			m_singleGlobalMaxCompressTime = nowText;
		}
		LeaveCriticalSection(&m_statLock);
	}

	void AddPackWriteSample(const PACK_WRITE_METRIC& metric)
	{
		EnterCriticalSection(&m_statLock);
		m_secPackWrittenCount++;
		m_secPackAppendMs += metric.appendMs;
		if (metric.appendMs > m_secPackAppendMaxMs)
		{
			m_secPackAppendMaxMs = metric.appendMs;
		}

		if (metric.flushCount > 0)
		{
			m_secPackFlushCount += metric.flushCount;
			m_secPackFlushMs += metric.flushMs;
			m_packTotalFlushEventCount += metric.flushCount;
			if (metric.flushMs >= 100)
			{
				m_packFlushOver100Count += metric.flushCount;
			}
			if (metric.flushMs >= 300)
			{
				m_packFlushOver300Count += metric.flushCount;
			}
			if (metric.flushMs >= 500)
			{
				m_packFlushOver500Count += metric.flushCount;
			}
			if (metric.flushMs > m_secPackFlushMaxMs)
			{
				m_secPackFlushMaxMs = metric.flushMs;
			}
		}

		if (metric.rotateCount > 0)
		{
			m_secPackRotateCount += metric.rotateCount;
			m_secPackOpenMs += metric.openMs;
			m_secPackCloseMs += metric.closeMs;
			if (metric.openMs > m_secPackOpenMaxMs)
			{
				m_secPackOpenMaxMs = metric.openMs;
			}
			if (metric.closeMs > m_secPackCloseMaxMs)
			{
				m_secPackCloseMaxMs = metric.closeMs;
			}
		}
		LeaveCriticalSection(&m_statLock);
	}

	void ResetPackSummaryState()
	{
		EnterCriticalSection(&m_statLock);
		m_packAbnormalSampleCount = 0;
		m_packPendingSampleCount = 0;
		m_packTotalFlushEventCount = 0;
		m_packFlushOver100Count = 0;
		m_packFlushOver300Count = 0;
		m_packFlushOver500Count = 0;
		m_packPeakPendingJpegCount = 0;
		m_packPeakPendingJpegMB = 0.0;
		m_packPeakPendingTime.clear();
		m_packMinRawFree = 2147483647;
		m_packMinRawFreeTime.clear();
		m_packPeakRawQueue = 0;
		m_packPeakRawQueueTime.clear();
		m_packGlobalMaxCompressMs = 0;
		m_packGlobalMaxCompressTime.clear();
		m_packGlobalMaxAppendMs = 0;
		m_packGlobalMaxAppendTime.clear();
		m_packGlobalMaxFlushMs = 0;
		m_packGlobalMaxFlushTime.clear();
		m_packGlobalMaxOpenMs = 0;
		m_packGlobalMaxOpenTime.clear();
		m_packGlobalMaxCloseMs = 0;
		m_packGlobalMaxCloseTime.clear();

		m_singleAbnormalSampleCount = 0;
		m_singlePendingSampleCount = 0;
		m_singleMinRawFree = 2147483647;
		m_singleMinRawFreeTime.clear();
		m_singlePeakRawQueue = 0;
		m_singlePeakRawQueueTime.clear();
		m_singleGlobalMaxCompressMs = 0;
		m_singleGlobalMaxCompressTime.clear();
		m_singleGlobalMaxOpenMs = 0;
		m_singleGlobalMaxOpenTime.clear();
		m_singleGlobalMaxWriteMs = 0;
		m_singleGlobalMaxWriteTime.clear();
		m_singleGlobalMaxCloseMs = 0;
		m_singleGlobalMaxCloseTime.clear();
		m_singleGlobalMaxDirCreateMs = 0;
		m_singleGlobalMaxDirCreateTime.clear();
		LeaveCriticalSection(&m_statLock);
	}

	void UpdatePackPeriodicSummary(
		int rawReadySize,
		int rawFreeSize,
		int pendingJpegCount,
		double pendingJpegMB,
		DWORD secCompressMaxMs,
		DWORD secPackAppendMaxMs,
		DWORD secPackFlushMaxMs,
		DWORD secPackOpenMaxMs,
		DWORD secPackCloseMaxMs,
		bool abnormal)
	{
		const std::string nowText = MakeNowStringForDisplay();

		EnterCriticalSection(&m_statLock);
		if (abnormal)
		{
			m_packAbnormalSampleCount++;
		}
		if (pendingJpegCount > 0)
		{
			m_packPendingSampleCount++;
		}
		if (rawFreeSize < m_packMinRawFree)
		{
			m_packMinRawFree = rawFreeSize;
			m_packMinRawFreeTime = nowText;
		}
		if (rawReadySize > m_packPeakRawQueue)
		{
			m_packPeakRawQueue = rawReadySize;
			m_packPeakRawQueueTime = nowText;
		}
		if (pendingJpegCount > m_packPeakPendingJpegCount || pendingJpegMB > m_packPeakPendingJpegMB)
		{
			m_packPeakPendingJpegCount = pendingJpegCount;
			m_packPeakPendingJpegMB = pendingJpegMB;
			m_packPeakPendingTime = nowText;
		}
		if (secCompressMaxMs > m_packGlobalMaxCompressMs)
		{
			m_packGlobalMaxCompressMs = secCompressMaxMs;
			m_packGlobalMaxCompressTime = nowText;
		}
		if (secPackAppendMaxMs > m_packGlobalMaxAppendMs)
		{
			m_packGlobalMaxAppendMs = secPackAppendMaxMs;
			m_packGlobalMaxAppendTime = nowText;
		}
		if (secPackFlushMaxMs > m_packGlobalMaxFlushMs)
		{
			m_packGlobalMaxFlushMs = secPackFlushMaxMs;
			m_packGlobalMaxFlushTime = nowText;
		}
		if (secPackOpenMaxMs > m_packGlobalMaxOpenMs)
		{
			m_packGlobalMaxOpenMs = secPackOpenMaxMs;
			m_packGlobalMaxOpenTime = nowText;
		}
		if (secPackCloseMaxMs > m_packGlobalMaxCloseMs)
		{
			m_packGlobalMaxCloseMs = secPackCloseMaxMs;
			m_packGlobalMaxCloseTime = nowText;
		}
		LeaveCriticalSection(&m_statLock);
	}

	void UpdateSinglePeriodicSummary(
		int rawReadySize,
		int rawFreeSize,
		int pendingJpegCount,
		bool abnormal)
	{
		const std::string nowText = MakeNowStringForDisplay();

		EnterCriticalSection(&m_statLock);
		if (abnormal)
		{
			m_singleAbnormalSampleCount++;
		}
		if (pendingJpegCount > 0)
		{
			m_singlePendingSampleCount++;
		}
		if (rawFreeSize < m_singleMinRawFree)
		{
			m_singleMinRawFree = rawFreeSize;
			m_singleMinRawFreeTime = nowText;
		}
		if (rawReadySize > m_singlePeakRawQueue)
		{
			m_singlePeakRawQueue = rawReadySize;
			m_singlePeakRawQueueTime = nowText;
		}
		LeaveCriticalSection(&m_statLock);
	}

	void WriteSummaryLog()
	{
		unsigned long long runMs = (m_runStartTickMs > 0) ? (GetTickCount64() - m_runStartTickMs) : 0;
		double runSec = (double)runMs / 1000.0;
		long long totalSaved = m_totalSaved;
		long long totalDropped = m_totalDropped;
		char line[512];
		if (m_cfg.saveMode == "pack")
		{
			long long abnormalSamples = 0;
			long long pendingSamples = 0;
			long long totalFlushEvents = 0;
			long long flushOver100 = 0;
			long long flushOver300 = 0;
			long long flushOver500 = 0;
			int peakPendingCount = 0;
			double peakPendingMB = 0.0;
			std::string peakPendingTime;
			int minRawFree = 0;
			std::string minRawFreeTime;
			int peakRawQueue = 0;
			std::string peakRawQueueTime;
			DWORD maxCompressMs = 0;
			std::string maxCompressTime;
			DWORD maxAppendMs = 0;
			std::string maxAppendTime;
			DWORD maxFlushMs = 0;
			std::string maxFlushTime;
			DWORD maxOpenMs = 0;
			std::string maxOpenTime;
			DWORD maxCloseMs = 0;
			std::string maxCloseTime;

			EnterCriticalSection(&m_statLock);
			abnormalSamples = m_packAbnormalSampleCount;
			pendingSamples = m_packPendingSampleCount;
			totalFlushEvents = m_packTotalFlushEventCount;
			flushOver100 = m_packFlushOver100Count;
			flushOver300 = m_packFlushOver300Count;
			flushOver500 = m_packFlushOver500Count;
			peakPendingCount = m_packPeakPendingJpegCount;
			peakPendingMB = m_packPeakPendingJpegMB;
			peakPendingTime = m_packPeakPendingTime;
			minRawFree = (m_packMinRawFree == 2147483647) ? m_pool.GetBlockCount() : m_packMinRawFree;
			minRawFreeTime = m_packMinRawFreeTime;
			peakRawQueue = m_packPeakRawQueue;
			peakRawQueueTime = m_packPeakRawQueueTime;
			maxCompressMs = m_packGlobalMaxCompressMs;
			maxCompressTime = m_packGlobalMaxCompressTime;
			maxAppendMs = m_packGlobalMaxAppendMs;
			maxAppendTime = m_packGlobalMaxAppendTime;
			maxFlushMs = m_packGlobalMaxFlushMs;
			maxFlushTime = m_packGlobalMaxFlushTime;
			maxOpenMs = m_packGlobalMaxOpenMs;
			maxOpenTime = m_packGlobalMaxOpenTime;
			maxCloseMs = m_packGlobalMaxCloseMs;
			maxCloseTime = m_packGlobalMaxCloseTime;
			LeaveCriticalSection(&m_statLock);

			m_summaryLog.PrintLine("pack模式总结开始");
			sprintf_s(line,
				"运行总时长=%.1f秒，总保存张数=%lld，总丢帧=%lld，压缩线程数=%d，原图池块数=%d，JPG待写上限=%dMB，packFlushEvery=%d，packMaxMB=%d",
				runSec,
				totalSaved,
				totalDropped,
				m_cfg.threadCount,
				m_cfg.poolBlockCount,
				m_cfg.jpegQueueMaxMB,
				m_cfg.packFlushEvery,
				m_cfg.packMaxMB);
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"待写JPG峰值=%d张，约%.1fMB，发生时间=%s",
				peakPendingCount,
				peakPendingMB,
				peakPendingTime.empty() ? "无" : peakPendingTime.c_str());
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"原图空闲块最小值=%d，发生时间=%s；待压缩队列峰值=%d，发生时间=%s",
				minRawFree,
				minRawFreeTime.empty() ? "无" : minRawFreeTime.c_str(),
				peakRawQueue,
				peakRawQueueTime.empty() ? "无" : peakRawQueueTime.c_str());
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"刷盘最大耗时=%lums，发生时间=%s；刷盘事件总数=%lld，>=100ms次数=%lld，>=300ms次数=%lld，>=500ms次数=%lld",
				maxFlushMs,
				maxFlushTime.empty() ? "无" : maxFlushTime.c_str(),
				totalFlushEvents,
				flushOver100,
				flushOver300,
				flushOver500);
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"入包最大耗时=%lums，发生时间=%s；压缩最大耗时=%lums，发生时间=%s",
				maxAppendMs,
				maxAppendTime.empty() ? "无" : maxAppendTime.c_str(),
				maxCompressMs,
				maxCompressTime.empty() ? "无" : maxCompressTime.c_str());
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"换包打开最大耗时=%lums，发生时间=%s；换包关闭最大耗时=%lums，发生时间=%s",
				maxOpenMs,
				maxOpenTime.empty() ? "无" : maxOpenTime.c_str(),
				maxCloseMs,
				maxCloseTime.empty() ? "无" : maxCloseTime.c_str());
			m_summaryLog.PrintLine(line);

			sprintf_s(line,
				"异常采样次数=%lld，待写队列非空采样次数=%lld",
				abnormalSamples,
				pendingSamples);
			m_summaryLog.PrintLine(line);

			std::string diagnosis;
			if (totalDropped > 0)
			{
				diagnosis = "综合判断：出现丢帧，当前缓存或写盘能力不足，需要优先扩大缓存或降低压力。";
			}
			else if (peakPendingCount >= 50 || peakPendingMB >= 50.0)
			{
				diagnosis = "综合判断：未丢帧，但写盘线程曾明显落后，JPG待写队列出现较大堆积，主要矛盾在写盘侧长尾。";
			}
			else if (maxFlushMs >= 300)
			{
				diagnosis = "综合判断：未丢帧，存在明显刷盘长尾，但二级缓存已吸收波动。";
			}
			else
			{
				diagnosis = "综合判断：整体稳定，未见明显写盘积压。";
			}
			m_summaryLog.PrintLine(diagnosis);

			if (minRawFree <= m_cfg.poolBlockCount / 4)
			{
				m_summaryLog.PrintLine("补充判断：原图池曾接近吃满，需要关注原图缓存余量。");
			}
			else
			{
				m_summaryLog.PrintLine("补充判断：原图池余量整体充足，回调侧已基本与写盘侧解耦。");
			}

			m_summaryLog.PrintLine("pack模式总结结束");
			m_summaryLog.Flush();
			return;
		}

		long long abnormalSamples = 0;
		long long pendingSamples = 0;
		int peakPendingCount = 0;
		double peakPendingMB = 0.0;
		std::string peakPendingTime;
		int minRawFree = 0;
		std::string minRawFreeTime;
		int peakRawQueue = 0;
		std::string peakRawQueueTime;
		DWORD maxCompressMs = 0;
		std::string maxCompressTime;
		DWORD maxOpenMs = 0;
		std::string maxOpenTime;
		DWORD maxWriteMs = 0;
		std::string maxWriteTime;
		DWORD maxCloseMs = 0;
		std::string maxCloseTime;
		DWORD maxDirCreateMs = 0;
		std::string maxDirCreateTime;

		EnterCriticalSection(&m_statLock);
		abnormalSamples = m_singleAbnormalSampleCount;
		pendingSamples = m_singlePendingSampleCount;
		peakPendingCount = m_packPeakPendingJpegCount;
		peakPendingMB = m_packPeakPendingJpegMB;
		peakPendingTime = m_packPeakPendingTime;
		minRawFree = (m_singleMinRawFree == 2147483647) ? m_pool.GetBlockCount() : m_singleMinRawFree;
		minRawFreeTime = m_singleMinRawFreeTime;
		peakRawQueue = m_singlePeakRawQueue;
		peakRawQueueTime = m_singlePeakRawQueueTime;
		maxCompressMs = m_singleGlobalMaxCompressMs;
		maxCompressTime = m_singleGlobalMaxCompressTime;
		maxOpenMs = m_singleGlobalMaxOpenMs;
		maxOpenTime = m_singleGlobalMaxOpenTime;
		maxWriteMs = m_singleGlobalMaxWriteMs;
		maxWriteTime = m_singleGlobalMaxWriteTime;
		maxCloseMs = m_singleGlobalMaxCloseMs;
		maxCloseTime = m_singleGlobalMaxCloseTime;
		maxDirCreateMs = m_singleGlobalMaxDirCreateMs;
		maxDirCreateTime = m_singleGlobalMaxDirCreateTime;
		LeaveCriticalSection(&m_statLock);

		m_summaryLog.PrintLine("single模式总结开始");
		sprintf_s(line,
			"运行总时长=%.1f秒，总保存张数=%lld，总丢帧=%lld，压缩线程数=%d，原图池块数=%d，JPG待写上限=%dMB",
			runSec,
			totalSaved,
			totalDropped,
			m_cfg.threadCount,
			m_cfg.poolBlockCount,
			m_cfg.jpegQueueMaxMB);
		m_summaryLog.PrintLine(line);

		sprintf_s(line,
			"待写JPG峰值=%d张，约%.1fMB，发生时间=%s",
			peakPendingCount,
			peakPendingMB,
			peakPendingTime.empty() ? "无" : peakPendingTime.c_str());
		m_summaryLog.PrintLine(line);

		sprintf_s(line,
			"原图空闲块最小值=%d，发生时间=%s；待压缩队列峰值=%d，发生时间=%s",
			minRawFree,
			minRawFreeTime.empty() ? "无" : minRawFreeTime.c_str(),
			peakRawQueue,
			peakRawQueueTime.empty() ? "无" : peakRawQueueTime.c_str());
		m_summaryLog.PrintLine(line);

		sprintf_s(line,
			"打开最大耗时=%lums，发生时间=%s；写入最大耗时=%lums，发生时间=%s；关闭最大耗时=%lums，发生时间=%s；建目录最大耗时=%lums，发生时间=%s",
			maxOpenMs,
			maxOpenTime.empty() ? "无" : maxOpenTime.c_str(),
			maxWriteMs,
			maxWriteTime.empty() ? "无" : maxWriteTime.c_str(),
			maxCloseMs,
			maxCloseTime.empty() ? "无" : maxCloseTime.c_str(),
			maxDirCreateMs,
			maxDirCreateTime.empty() ? "无" : maxDirCreateTime.c_str());
		m_summaryLog.PrintLine(line);

		sprintf_s(line,
			"压缩最大耗时=%lums，发生时间=%s；异常采样次数=%lld，待写队列非空采样次数=%lld",
			maxCompressMs,
			maxCompressTime.empty() ? "无" : maxCompressTime.c_str(),
			abnormalSamples,
			pendingSamples);
		m_summaryLog.PrintLine(line);

		std::string diagnosis;
		if (totalDropped > 0)
		{
			diagnosis = "综合判断：出现丢帧，当前缓存或单图写盘能力不足，需要优先扩大缓存或降低压力。";
		}
		else if (peakPendingCount >= 50 || peakPendingMB >= 50.0)
		{
			diagnosis = "综合判断：未丢帧，但单图写入线程曾明显落后，JPG待写队列出现较大堆积，主要矛盾在单图写盘侧。";
		}
		else if (maxOpenMs >= 100 || maxCloseMs >= 100)
		{
			diagnosis = "综合判断：未丢帧，但单图模式存在明显开关文件长尾，需要重点关注文件系统元数据开销。";
		}
		else
		{
			diagnosis = "综合判断：整体稳定，未见明显单图写盘积压。";
		}
		m_summaryLog.PrintLine(diagnosis);

		if (minRawFree <= m_cfg.poolBlockCount / 4)
		{
			m_summaryLog.PrintLine("补充判断：原图池曾接近吃满，需要关注原图缓存余量。");
		}
		else
		{
			m_summaryLog.PrintLine("补充判断：原图池余量整体充足，二级缓存已起到隔离作用。");
		}

		m_summaryLog.PrintLine("single模式总结结束");
		m_summaryLog.Flush();
	}

	void AddSaveTimingSample(DWORD compressMs, DWORD openMs, DWORD fileWriteMs, DWORD closeMs, DWORD dirCreateMs)
	{
		DWORD writeMs = openMs + fileWriteMs + closeMs;
		const std::string nowText = MakeNowStringForDisplay();

		EnterCriticalSection(&m_statLock);
		m_secSavedCount++;
		m_secSingleCompressedCount++;
		m_secCompressMs += compressMs;
		m_secWriteMs += writeMs;
		m_secOpenMs += openMs;
		m_secFileWriteMs += fileWriteMs;
		m_secCloseMs += closeMs;

		if (dirCreateMs > 0)
		{
			m_secDirCreateCount++;
			m_secDirCreateMs += dirCreateMs;
			if (dirCreateMs > m_secDirCreateMaxMs)
			{
				m_secDirCreateMaxMs = dirCreateMs;
			}
		}

		if (compressMs > m_secCompressMaxMs)
		{
			m_secCompressMaxMs = compressMs;
		}
		if (compressMs > m_singleGlobalMaxCompressMs)
		{
			m_singleGlobalMaxCompressMs = compressMs;
			m_singleGlobalMaxCompressTime = nowText;
		}
		if (writeMs > m_secWriteMaxMs)
		{
			m_secWriteMaxMs = writeMs;
		}
		if (openMs > m_secOpenMaxMs)
		{
			m_secOpenMaxMs = openMs;
		}
		if (openMs > m_singleGlobalMaxOpenMs)
		{
			m_singleGlobalMaxOpenMs = openMs;
			m_singleGlobalMaxOpenTime = nowText;
		}
		if (fileWriteMs > m_secFileWriteMaxMs)
		{
			m_secFileWriteMaxMs = fileWriteMs;
		}
		if (fileWriteMs > m_singleGlobalMaxWriteMs)
		{
			m_singleGlobalMaxWriteMs = fileWriteMs;
			m_singleGlobalMaxWriteTime = nowText;
		}
		if (closeMs > m_secCloseMaxMs)
		{
			m_secCloseMaxMs = closeMs;
		}
		if (closeMs > m_singleGlobalMaxCloseMs)
		{
			m_singleGlobalMaxCloseMs = closeMs;
			m_singleGlobalMaxCloseTime = nowText;
		}
		if (dirCreateMs > m_singleGlobalMaxDirCreateMs)
		{
			m_singleGlobalMaxDirCreateMs = dirCreateMs;
			m_singleGlobalMaxDirCreateTime = nowText;
		}
		LeaveCriticalSection(&m_statLock);
	}

	void FlushSaveLog(bool force)
	{
		if (m_cfg.saveMode == "pack")
		{
			long long secDroppedCount = 0;
			long long totalDropped = 0;
			long long secCompressMs = 0;
			long long secPackCompressedCount = 0;
			long long secPackWrittenCount = 0;
			long long secPackAppendMs = 0;
			long long secPackFlushCount = 0;
			long long secPackFlushMs = 0;
			long long secPackRotateCount = 0;
			long long secPackOpenMs = 0;
			long long secPackCloseMs = 0;
			DWORD secCompressMaxMs = 0;
			DWORD secPackAppendMaxMs = 0;
			DWORD secPackFlushMaxMs = 0;
			DWORD secPackOpenMaxMs = 0;
			DWORD secPackCloseMaxMs = 0;

			EnterCriticalSection(&m_statLock);
			secDroppedCount = m_secDroppedCount;
			totalDropped = m_totalDropped;
			secCompressMs = m_secCompressMs;
			secPackCompressedCount = m_secPackCompressedCount;
			secPackWrittenCount = m_secPackWrittenCount;
			secPackAppendMs = m_secPackAppendMs;
			secPackFlushCount = m_secPackFlushCount;
			secPackFlushMs = m_secPackFlushMs;
			secPackRotateCount = m_secPackRotateCount;
			secPackOpenMs = m_secPackOpenMs;
			secPackCloseMs = m_secPackCloseMs;
			secCompressMaxMs = m_secCompressMaxMs;
			secPackAppendMaxMs = m_secPackAppendMaxMs;
			secPackFlushMaxMs = m_secPackFlushMaxMs;
			secPackOpenMaxMs = m_secPackOpenMaxMs;
			secPackCloseMaxMs = m_secPackCloseMaxMs;

			m_secDroppedCount = 0;
			m_secCompressMs = 0;
			m_secCompressMaxMs = 0;
			m_secPackCompressedCount = 0;
			m_secPackWrittenCount = 0;
			m_secPackAppendMs = 0;
			m_secPackFlushCount = 0;
			m_secPackFlushMs = 0;
			m_secPackRotateCount = 0;
			m_secPackOpenMs = 0;
			m_secPackCloseMs = 0;
			m_secPackAppendMaxMs = 0;
			m_secPackFlushMaxMs = 0;
			m_secPackOpenMaxMs = 0;
			m_secPackCloseMaxMs = 0;
			LeaveCriticalSection(&m_statLock);

			int rawReadySize = m_pool.GetReadySize();
			int rawFreeSize = m_pool.GetFreeSize();
			int pendingJpegCount = GetPendingJpegCount();
			double pendingJpegMB = (double)GetPendingJpegBytes() / (1024.0 * 1024.0);
			bool idle = (secPackCompressedCount <= 0 && secPackWrittenCount <= 0 && rawReadySize <= 0 && pendingJpegCount <= 0 && secDroppedCount <= 0);
			if (idle)
			{
				return;
			}

			double avgCompress = 0.0;
			double avgAppend = 0.0;
			double avgFlush = 0.0;
			double avgOpen = 0.0;
			double avgClose = 0.0;
			if (secPackCompressedCount > 0)
			{
				avgCompress = (double)secCompressMs / (double)secPackCompressedCount;
			}
			if (secPackWrittenCount > 0)
			{
				avgAppend = (double)secPackAppendMs / (double)secPackWrittenCount;
			}
			if (secPackFlushCount > 0)
			{
				avgFlush = (double)secPackFlushMs / (double)secPackFlushCount;
			}
			if (secPackRotateCount > 0)
			{
				avgOpen = (double)secPackOpenMs / (double)secPackRotateCount;
				avgClose = (double)secPackCloseMs / (double)secPackRotateCount;
			}

			int abnormalFreeThreshold = m_pool.GetBlockCount() / 2;
			if (abnormalFreeThreshold < 1)
			{
				abnormalFreeThreshold = 1;
			}
			bool abnormal =
				(rawReadySize > 0) ||
				(pendingJpegCount > 0) ||
				(rawFreeSize < abnormalFreeThreshold) ||
				(secPackFlushMaxMs >= 100) ||
				(secPackAppendMaxMs >= 20) ||
				(secDroppedCount > 0);
			UpdatePackPeriodicSummary(
				rawReadySize,
				rawFreeSize,
				pendingJpegCount,
				pendingJpegMB,
				secCompressMaxMs,
				secPackAppendMaxMs,
				secPackFlushMaxMs,
				secPackOpenMaxMs,
				secPackCloseMaxMs,
				abnormal);
			const char* statType = abnormal ? "pack模式异常统计" : "pack模式正常统计";

			char line[1024];
			sprintf_s(line,
				"%s，本秒压缩张数=%lld，本秒写包张数=%lld，压缩平均耗时=%.1fms，压缩最大耗时=%lums，入包平均耗时=%.1fms，入包最大耗时=%lums，刷盘次数=%lld，刷盘平均耗时=%.1fms，刷盘最大耗时=%lums，换包次数=%lld，换包打开平均=%.1fms，换包打开最大=%lums，换包关闭平均=%.1fms，换包关闭最大=%lums，待压缩队列=%d，原图空闲块=%d，待写JPG=%d，待写JPG总MB=%.1f，本秒丢帧=%lld，总丢帧=%lld",
				statType,
				secPackCompressedCount,
				secPackWrittenCount,
				avgCompress,
				secCompressMaxMs,
				avgAppend,
				secPackAppendMaxMs,
				secPackFlushCount,
				avgFlush,
				secPackFlushMaxMs,
				secPackRotateCount,
				avgOpen,
				secPackOpenMaxMs,
				avgClose,
				secPackCloseMaxMs,
				rawReadySize,
				rawFreeSize,
				pendingJpegCount,
				pendingJpegMB,
				secDroppedCount,
				totalDropped);
			m_saveLog.PrintLine(line);
			return;
		}

		long long secSavedCount = 0;
		long long secSingleCompressedCount = 0;
		long long secDroppedCount = 0;
		long long secCompressMs = 0;
		long long secWriteMs = 0;
		long long secOpenMs = 0;
		long long secFileWriteMs = 0;
		long long secCloseMs = 0;
		long long secDirCreateMs = 0;
		long long secDirCreateCount = 0;
		DWORD secCompressMaxMs = 0;
		DWORD secWriteMaxMs = 0;
		DWORD secOpenMaxMs = 0;
		DWORD secFileWriteMaxMs = 0;
		DWORD secCloseMaxMs = 0;
		DWORD secDirCreateMaxMs = 0;
		long long totalDropped = 0;

		EnterCriticalSection(&m_statLock);
		secSavedCount = m_secSavedCount;
		secSingleCompressedCount = m_secSingleCompressedCount;
		secDroppedCount = m_secDroppedCount;
		secCompressMs = m_secCompressMs;
		secWriteMs = m_secWriteMs;
		secOpenMs = m_secOpenMs;
		secFileWriteMs = m_secFileWriteMs;
		secCloseMs = m_secCloseMs;
		secDirCreateMs = m_secDirCreateMs;
		secDirCreateCount = m_secDirCreateCount;
		secCompressMaxMs = m_secCompressMaxMs;
		secWriteMaxMs = m_secWriteMaxMs;
		secOpenMaxMs = m_secOpenMaxMs;
		secFileWriteMaxMs = m_secFileWriteMaxMs;
		secCloseMaxMs = m_secCloseMaxMs;
		secDirCreateMaxMs = m_secDirCreateMaxMs;
		totalDropped = m_totalDropped;

		m_secSavedCount = 0;
		m_secSingleCompressedCount = 0;
		m_secDroppedCount = 0;
		m_secCompressMs = 0;
		m_secWriteMs = 0;
		m_secOpenMs = 0;
		m_secFileWriteMs = 0;
		m_secCloseMs = 0;
		m_secDirCreateMs = 0;
		m_secDirCreateCount = 0;
		m_secCompressMaxMs = 0;
		m_secWriteMaxMs = 0;
		m_secOpenMaxMs = 0;
		m_secFileWriteMaxMs = 0;
		m_secCloseMaxMs = 0;
		m_secDirCreateMaxMs = 0;
		LeaveCriticalSection(&m_statLock);

		int readySize = m_pool.GetReadySize();
		int freeSize = m_pool.GetFreeSize();
		int pendingJpegCount = GetPendingJpegCount();
		double pendingJpegMB = (double)GetPendingJpegBytes() / (1024.0 * 1024.0);
		bool idle = (secSavedCount <= 0 && readySize <= 0 && pendingJpegCount <= 0 && secDroppedCount <= 0);
		if (idle)
		{
			return;
		}

		double avgCompress = 0.0;
		double avgWrite = 0.0;
		double avgOpen = 0.0;
		double avgFileWrite = 0.0;
		double avgClose = 0.0;
		double avgDirCreate = 0.0;
		if (secSingleCompressedCount > 0)
		{
			avgCompress = (double)secCompressMs / (double)secSingleCompressedCount;
		}
		if (secSavedCount > 0)
		{
			avgWrite = (double)secWriteMs / (double)secSavedCount;
			avgOpen = (double)secOpenMs / (double)secSavedCount;
			avgFileWrite = (double)secFileWriteMs / (double)secSavedCount;
			avgClose = (double)secCloseMs / (double)secSavedCount;
		}
		if (secDirCreateCount > 0)
		{
			avgDirCreate = (double)secDirCreateMs / (double)secDirCreateCount;
		}

		int abnormalFreeThreshold = m_pool.GetBlockCount() / 2;
		if (abnormalFreeThreshold < 1)
		{
			abnormalFreeThreshold = 1;
		}
		bool abnormal = (readySize > 0 || pendingJpegCount > 0 || freeSize < abnormalFreeThreshold || secWriteMaxMs >= 100 || secCompressMaxMs >= 80 || secDroppedCount > 0);
		UpdateSinglePeriodicSummary(
			readySize,
			freeSize,
			pendingJpegCount,
			abnormal);
		const char* statType = abnormal ? "保存线程异常统计" : "保存线程正常统计";

		char line[1024];
		sprintf_s(line,
			"%s，本秒保存张数=%lld，压缩平均耗时=%.1fms，压缩最大耗时=%lums，写盘总平均耗时=%.1fms，写盘总最大耗时=%lums，打开平均=%.1fms，打开最大=%lums，写入平均=%.1fms，写入最大=%lums，关闭平均=%.1fms，关闭最大=%lums，建目录次数=%lld，建目录平均=%.1fms，建目录最大=%lums，待压缩队列=%d，原图空闲块=%d，待写JPG=%d，待写JPG总MB=%.1f，本秒丢帧=%lld，总丢帧=%lld",
			statType,
			secSavedCount,
			avgCompress,
			secCompressMaxMs,
			avgWrite,
			secWriteMaxMs,
			avgOpen,
			secOpenMaxMs,
			avgFileWrite,
			secFileWriteMaxMs,
			avgClose,
			secCloseMaxMs,
			secDirCreateCount,
			avgDirCreate,
			secDirCreateMaxMs,
			readySize,
			freeSize,
			pendingJpegCount,
			pendingJpegMB,
			secDroppedCount,
			totalDropped);
		m_saveLog.PrintLine(line);
	}

private:
	DiskStressConfig m_cfg;
	std::string m_runOutputDir;
	CImagePool m_pool;
	CSimpleLog m_log;
	CSimpleLog m_saveLog;
	CSimpleLog m_summaryLog;
	std::vector<SOURCE_IMAGE_FRAME> m_sourceFrames;
	int m_width;
	int m_height;
	std::atomic<bool> m_stopRequested;
	std::atomic<bool> m_producerFinished;
	std::atomic<bool> m_compressFinished;
	std::atomic<long long> m_nextOutputIndex;
	std::atomic<long long> m_totalDropped;
	std::atomic<long long> m_totalSaved;
	std::thread m_producerThread;
	std::thread m_statThread;
	std::thread m_writerThread;
	std::vector<std::thread> m_saveThreads;
	CRITICAL_SECTION m_packLock;
	CRITICAL_SECTION m_jpegLock;
	FILE* m_packDataFile;
	FILE* m_packIndexFile;
	int m_packSerialNo;
	int m_packCurrentNo;
	unsigned long long m_packCreateTimeValue;
	int m_packCurrentFrameCount;
	int m_packFramesSinceFlush;
	std::queue<JPEG_WRITE_TASK> m_pendingJpegQueue;
	unsigned long long m_pendingJpegBytes;

	CRITICAL_SECTION m_statLock;
	CRITICAL_SECTION m_lostLock;
	std::queue<std::string> m_lostLogs;

	long long m_secSavedCount;
	long long m_secSingleCompressedCount;
	long long m_secDroppedCount;
	long long m_secCompressMs;
	long long m_secWriteMs;
	long long m_secOpenMs;
	long long m_secFileWriteMs;
	long long m_secCloseMs;
	long long m_secDirCreateMs;
	long long m_secDirCreateCount;
	DWORD m_secCompressMaxMs;
	DWORD m_secWriteMaxMs;
	DWORD m_secOpenMaxMs;
	DWORD m_secFileWriteMaxMs;
	DWORD m_secCloseMaxMs;
	DWORD m_secDirCreateMaxMs;
	long long m_secPackCompressedCount;
	long long m_secPackWrittenCount;
	long long m_secPackAppendMs;
	long long m_secPackFlushCount;
	long long m_secPackFlushMs;
	long long m_secPackRotateCount;
	long long m_secPackOpenMs;
	long long m_secPackCloseMs;
	DWORD m_secPackAppendMaxMs;
	DWORD m_secPackFlushMaxMs;
	DWORD m_secPackOpenMaxMs;
	DWORD m_secPackCloseMaxMs;
	unsigned long long m_runStartTickMs;
	long long m_packAbnormalSampleCount;
	long long m_packPendingSampleCount;
	long long m_packTotalFlushEventCount;
	long long m_packFlushOver100Count;
	long long m_packFlushOver300Count;
	long long m_packFlushOver500Count;
	int m_packPeakPendingJpegCount;
	double m_packPeakPendingJpegMB;
	std::string m_packPeakPendingTime;
	int m_packMinRawFree;
	std::string m_packMinRawFreeTime;
	int m_packPeakRawQueue;
	std::string m_packPeakRawQueueTime;
	DWORD m_packGlobalMaxCompressMs;
	std::string m_packGlobalMaxCompressTime;
	DWORD m_packGlobalMaxAppendMs;
	std::string m_packGlobalMaxAppendTime;
	DWORD m_packGlobalMaxFlushMs;
	std::string m_packGlobalMaxFlushTime;
	DWORD m_packGlobalMaxOpenMs;
	std::string m_packGlobalMaxOpenTime;
	DWORD m_packGlobalMaxCloseMs;
	std::string m_packGlobalMaxCloseTime;
	long long m_singleAbnormalSampleCount;
	long long m_singlePendingSampleCount;
	int m_singleMinRawFree;
	std::string m_singleMinRawFreeTime;
	int m_singlePeakRawQueue;
	std::string m_singlePeakRawQueueTime;
	DWORD m_singleGlobalMaxCompressMs;
	std::string m_singleGlobalMaxCompressTime;
	DWORD m_singleGlobalMaxOpenMs;
	std::string m_singleGlobalMaxOpenTime;
	DWORD m_singleGlobalMaxWriteMs;
	std::string m_singleGlobalMaxWriteTime;
	DWORD m_singleGlobalMaxCloseMs;
	std::string m_singleGlobalMaxCloseTime;
	DWORD m_singleGlobalMaxDirCreateMs;
	std::string m_singleGlobalMaxDirCreateTime;
};

static CDiskStressApp* g_app = NULL;

BOOL WINAPI ConsoleHandlerRoutine(DWORD ctrlType)
{
	if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT || ctrlType == CTRL_CLOSE_EVENT)
	{
		if (g_app != NULL)
		{
			g_app->RequestStop();
		}
		return TRUE;
	}
	return FALSE;
}

static void PrintUsage()
{
		std::cout
		<< "DiskStressDemo usage:\n"
		<< "  DiskStressDemo.exe [-input path] [-inputDir dir] [-output dir] [-fps 20] [-count 0] [-duration 0]\n"
		<< "                     [-threads 4] [-pool 80] [-quality 85] [-logInterval 1000] [-jpegQueueMB 0]\n"
		<< "                     [-mode single|pack] [-pixelMode auto|gray|color] [-packMB 800] [-packFlush 20] [-preload 30]\n"
		<< "Config file: DiskStressDemo.ini (same directory as exe)\n";
}

static bool LoadConfigFromIniAndArgs(int argc, char* argv[], DiskStressConfig& cfg)
{
	std::string exeDir = GetExeDir();
	std::string iniPath = JoinPath(exeDir, "DiskStressDemo.ini");

	cfg.inputImage = ReadIniString(iniPath, "input", "imagePath", cfg.inputImage.c_str());
	cfg.inputDir = ReadIniString(iniPath, "input", "imageDir", cfg.inputDir.c_str());
	cfg.pixelMode = ReadIniString(iniPath, "input", "pixelMode", cfg.pixelMode.c_str());
	cfg.outputDir = ReadIniString(iniPath, "output", "rootDir", cfg.outputDir.c_str());
	cfg.saveMode = ReadIniString(iniPath, "output", "saveMode", "single");
	cfg.fps = ReadIniDouble(iniPath, "stress", "fps", cfg.fps);
	cfg.totalCount = ReadIniInt64(iniPath, "stress", "totalCount", cfg.totalCount);
	cfg.durationSec = ReadIniInt(iniPath, "stress", "durationSec", cfg.durationSec);
	cfg.threadCount = ReadIniInt(iniPath, "stress", "threadCount", cfg.threadCount);
	cfg.poolBlockCount = ReadIniInt(iniPath, "stress", "poolBlockCount", cfg.poolBlockCount);
	cfg.jpegQuality = ReadIniInt(iniPath, "stress", "jpegQuality", cfg.jpegQuality);
	cfg.logIntervalMs = ReadIniInt(iniPath, "stress", "logIntervalMs", cfg.logIntervalMs);
	cfg.jpegQueueMaxMB = ReadIniInt(iniPath, "stress", "jpegQueueMaxMB", cfg.jpegQueueMaxMB);
	cfg.packMaxMB = ReadIniInt(iniPath, "pack", "packMaxMB", cfg.packMaxMB);
	cfg.packFlushEvery = ReadIniInt(iniPath, "pack", "packFlushEvery", cfg.packFlushEvery);
	cfg.preloadImageCount = ReadIniInt(iniPath, "input", "preloadImageCount", cfg.preloadImageCount);

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			PrintUsage();
			return false;
		}
		else if ((arg == "-input" || arg == "--input") && i + 1 < argc)
		{
			cfg.inputImage = argv[++i];
		}
		else if ((arg == "-inputDir" || arg == "--inputDir") && i + 1 < argc)
		{
			cfg.inputDir = argv[++i];
		}
		else if ((arg == "-output" || arg == "--output") && i + 1 < argc)
		{
			cfg.outputDir = argv[++i];
		}
		else if ((arg == "-pixelMode" || arg == "--pixelMode") && i + 1 < argc)
		{
			cfg.pixelMode = argv[++i];
		}
		else if ((arg == "-fps" || arg == "--fps") && i + 1 < argc)
		{
			cfg.fps = atof(argv[++i]);
		}
		else if ((arg == "-count" || arg == "--count") && i + 1 < argc)
		{
			cfg.totalCount = _atoi64(argv[++i]);
		}
		else if ((arg == "-duration" || arg == "--duration") && i + 1 < argc)
		{
			cfg.durationSec = atoi(argv[++i]);
		}
		else if ((arg == "-threads" || arg == "--threads") && i + 1 < argc)
		{
			cfg.threadCount = atoi(argv[++i]);
		}
		else if ((arg == "-pool" || arg == "--pool") && i + 1 < argc)
		{
			cfg.poolBlockCount = atoi(argv[++i]);
		}
		else if ((arg == "-quality" || arg == "--quality") && i + 1 < argc)
		{
			cfg.jpegQuality = atoi(argv[++i]);
		}
		else if ((arg == "-logInterval" || arg == "--logInterval") && i + 1 < argc)
		{
			cfg.logIntervalMs = atoi(argv[++i]);
		}
		else if ((arg == "-jpegQueueMB" || arg == "--jpegQueueMB") && i + 1 < argc)
		{
			cfg.jpegQueueMaxMB = atoi(argv[++i]);
		}
		else if ((arg == "-mode" || arg == "--mode") && i + 1 < argc)
		{
			cfg.saveMode = argv[++i];
		}
		else if ((arg == "-packMB" || arg == "--packMB") && i + 1 < argc)
		{
			cfg.packMaxMB = atoi(argv[++i]);
		}
		else if ((arg == "-packFlush" || arg == "--packFlush") && i + 1 < argc)
		{
			cfg.packFlushEvery = atoi(argv[++i]);
		}
		else if ((arg == "-preload" || arg == "--preload") && i + 1 < argc)
		{
			cfg.preloadImageCount = atoi(argv[++i]);
		}
	}

	cfg.inputImage = ResolvePathByExeDir(cfg.inputImage);
	cfg.inputDir = ResolvePathByExeDir(cfg.inputDir);
	cfg.outputDir = ResolvePathByExeDir(cfg.outputDir);
	cfg.pixelMode = ToLowerString(cfg.pixelMode);
	if (cfg.pixelMode != "gray" && cfg.pixelMode != "color")
	{
		cfg.pixelMode = "auto";
	}
	cfg.saveMode = ToLowerString(cfg.saveMode);
	if (cfg.saveMode != "pack")
	{
		cfg.saveMode = "single";
	}

	if (cfg.threadCount < 1)
	{
		cfg.threadCount = 1;
	}
	if (cfg.poolBlockCount < cfg.threadCount * 2)
	{
		cfg.poolBlockCount = cfg.threadCount * 2;
	}
	if (cfg.jpegQuality < 1)
	{
		cfg.jpegQuality = 1;
	}
	if (cfg.jpegQuality > 100)
	{
		cfg.jpegQuality = 100;
	}
	if (cfg.logIntervalMs < 200)
	{
		cfg.logIntervalMs = 200;
	}
	if (cfg.jpegQueueMaxMB < 0)
	{
		cfg.jpegQueueMaxMB = 0;
	}
	if (cfg.packMaxMB < 1)
	{
		cfg.packMaxMB = 1;
	}
	if (cfg.packFlushEvery < 1)
	{
		cfg.packFlushEvery = 1;
	}
	if (cfg.fps < 0.1)
	{
		cfg.fps = 0.1;
	}
	if (cfg.preloadImageCount < 1)
	{
		cfg.preloadImageCount = 1;
	}
	return true;
}

int main(int argc, char* argv[])
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken = 0;
	if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok)
	{
		std::cout << "GDI+ startup failed." << std::endl;
		return 1;
	}

	DiskStressConfig cfg;
	if (!LoadConfigFromIniAndArgs(argc, argv, cfg))
	{
		GdiplusShutdown(gdiplusToken);
		return 1;
	}

	CDiskStressApp app;
	g_app = &app;
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

	int ret = app.Run(cfg);

	g_app = NULL;
	GdiplusShutdown(gdiplusToken);
	return ret;
}
