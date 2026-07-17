#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
#  ifdef PACKSDK_EXPORTS
#    define PACKSDK_API __declspec(dllexport)
#  else
#    define PACKSDK_API __declspec(dllimport)
#  endif
#  define PACKSDK_CALL __stdcall
#else
#  if defined(PACKSDK_EXPORTS)
#    define PACKSDK_API __attribute__((visibility("default")))
#  else
#    define PACKSDK_API
#  endif
#  define PACKSDK_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* PACK_HANDLE;

// SDK 统一返回 PACK_RESULT。C ABI 不抛异常，Qt/C# 薄封装可以把这些错误码转换成异常或提示框。
enum PACK_RESULT
{
	PACK_OK = 0,
	PACK_ERROR_INVALID_ARGUMENT = -1,
	PACK_ERROR_OPEN_FAILED = -2,
	PACK_ERROR_FORMAT = -3,
	PACK_ERROR_NOT_FOUND = -4,
	PACK_ERROR_OUT_OF_RANGE = -5,
	PACK_ERROR_BUFFER_TOO_SMALL = -6,
	PACK_ERROR_IO = -7
};

// 打开图片包目录时的行为开关。首版只提供可选的打开时校验，默认只构建索引以保证打开速度。
enum PACK_OPEN_FLAGS
{
	PACK_OPEN_DEFAULT = 0,
	PACK_OPEN_VERIFY_ON_OPEN = 1
};

// 批量导出时的命名方式。
// LEGACY_DIRS 会模拟旧单图目录结构：Image_0000\000_time.jpg。
// FLAT_SOURCE_INDEX 会把所有图片导出到同一目录：sourceIndex_time.jpg。
enum PACK_NAMING_MODE
{
	PACK_NAMING_LEGACY_DIRS = 0,
	PACK_NAMING_FLAT_SOURCE_INDEX = 1
};

// 单帧状态标记。首版只标记一种异常：idx 文件里存在超过 header.recordCount 的尾部记录。
enum PACK_FRAME_STATUS
{
	PACK_FRAME_STATUS_OK = 0,
	PACK_FRAME_STATUS_IDX_RECORD_AFTER_HEADER_COUNT = 1
};

// 打开参数结构。调用方必须把 structSize 设置为 sizeof(PACK_OPEN_OPTIONS)，便于后续版本扩展字段。
struct PACK_OPEN_OPTIONS
{
	uint32_t structSize;
	uint32_t flags;
};

// 数据集摘要信息，对应一个采集输出目录。
// firstTimeValue/lastTimeValue 是采集端写入的 yyyyMMddHHmmssfff 数字时间，不是 Unix 时间戳。
struct PACK_DATASET_INFO
{
	uint32_t structSize;
	uint32_t formatVersion;
	uint32_t packCount;
	uint32_t hasWarning;
	uint64_t frameCount;
	uint64_t firstTimeValue;
	uint64_t lastTimeValue;
	uint64_t warningCount;
};

// 单张图片的索引信息。
// width/height 当前固定为 -1，因为 pack v1 没有保存宽高；调用方可通过 JPEG 解码获得真实尺寸。
struct PACK_FRAME_INFO
{
	uint32_t structSize;
	uint32_t packNo;
	uint32_t packFrameIndex;
	uint32_t statusFlags;
	uint64_t globalIndex;
	uint64_t sourceIndex;
	uint64_t timeValue;
	uint64_t datOffset;
	uint32_t jpgSize;
	int32_t width;
	int32_t height;
};

// 校验报告。summary 是宽字符摘要，Ubuntu 调用方如果需要 UTF-8 文本可使用 Pack_GetLastErrorUtf8 获取错误详情。
struct PACK_VERIFY_REPORT
{
	uint32_t structSize;
	uint32_t packCount;
	uint32_t missingDatCount;
	uint32_t badIdxCount;
	uint64_t checkedFrameCount;
	uint64_t badFrameCount;
	uint64_t warningCount;
	wchar_t summary[512];
};

// Windows/宽字符导出回调。返回 0 表示继续导出，返回非 0 表示取消导出。
typedef int (PACKSDK_CALL *PACK_EXPORT_CALLBACK)(
	uint64_t globalIndex,
	uint64_t totalCount,
	const wchar_t* outputPath,
	int result,
	void* userData);

// UTF-8 导出回调，Ubuntu/Linux 和跨平台 Qt 推荐使用。
typedef int (PACKSDK_CALL *PACK_EXPORT_CALLBACK_UTF8)(
	uint64_t globalIndex,
	uint64_t totalCount,
	const char* outputPath,
	int result,
	void* userData);

// 打开一个图片包目录，目录下应包含 Pack_000000.idx / Pack_000000.dat 这种成对文件。
// Windows/C# 可用宽字符路径；Ubuntu/Linux 请使用 Pack_OpenUtf8。
// 成功后 outHandle 非空，必须调用 Pack_Close 释放。
PACKSDK_API int PACKSDK_CALL Pack_Open(
	const wchar_t* rootDir,
	const PACK_OPEN_OPTIONS* options,
	PACK_HANDLE* outHandle);

// 使用 UTF-8 路径打开图片包目录。跨平台 Qt、Ubuntu/Linux C++ 程序推荐调用这个接口。
PACKSDK_API int PACKSDK_CALL Pack_OpenUtf8(
	const char* rootDir,
	const PACK_OPEN_OPTIONS* options,
	PACK_HANDLE* outHandle);

// 关闭 Pack_Open / Pack_OpenUtf8 返回的句柄。允许传 NULL，传 NULL 时不做任何事。
PACKSDK_API void PACKSDK_CALL Pack_Close(PACK_HANDLE handle);

// 获取当前图片包目录的摘要信息，例如 pack 数量、图片总数、时间范围、警告数量。
PACKSDK_API int PACKSDK_CALL Pack_GetDatasetInfo(
	PACK_HANDLE handle,
	PACK_DATASET_INFO* info);

// 获取可读取图片总数。globalIndex 的合法范围是 [0, count)。
PACKSDK_API int PACKSDK_CALL Pack_GetFrameCount(
	PACK_HANDLE handle,
	uint64_t* count);

// 按全局序号获取某张图片的索引信息，不读取 JPEG 数据体。
PACKSDK_API int PACKSDK_CALL Pack_GetFrameInfo(
	PACK_HANDLE handle,
	uint64_t globalIndex,
	PACK_FRAME_INFO* frameInfo);

// 按采集端 sourceIndex 查找全局序号。sourceIndex 来自采集端帧序号，从 1 开始。
PACKSDK_API int PACKSDK_CALL Pack_FindBySourceIndex(
	PACK_HANDLE handle,
	uint64_t sourceIndex,
	uint64_t* globalIndex);

// 读取某张图片的 JPEG 原始字节。
// 第一次可传 buffer=NULL 或 bufferSize=0 查询 requiredSize；第二次分配足够 buffer 后读取。
// SDK 会先校验 idx 记录和 dat 帧头是否匹配，再读取 JPEG 数据。
PACKSDK_API int PACKSDK_CALL Pack_ReadJpeg(
	PACK_HANDLE handle,
	uint64_t globalIndex,
	void* buffer,
	uint32_t bufferSize,
	uint32_t* requiredSize);

// 把某张图片保存为单独 jpg 文件。Windows/C# 可用宽字符路径；Ubuntu/Linux 请使用 Pack_SaveJpegUtf8。
PACKSDK_API int PACKSDK_CALL Pack_SaveJpeg(
	PACK_HANDLE handle,
	uint64_t globalIndex,
	const wchar_t* outputPath);

// UTF-8 路径版本的单张保存接口。
PACKSDK_API int PACKSDK_CALL Pack_SaveJpegUtf8(
	PACK_HANDLE handle,
	uint64_t globalIndex,
	const char* outputPath);

// 批量导出全部图片。Windows/宽字符路径版本。
// callback 可为 NULL；非 NULL 时每导出一张都会回调，回调返回非 0 可取消导出。
PACKSDK_API int PACKSDK_CALL Pack_ExportAll(
	PACK_HANDLE handle,
	const wchar_t* outputDir,
	uint32_t namingMode,
	PACK_EXPORT_CALLBACK callback,
	void* userData);

// UTF-8 路径版本的批量导出接口。Ubuntu/Linux 和跨平台 Qt 推荐调用。
PACKSDK_API int PACKSDK_CALL Pack_ExportAllUtf8(
	PACK_HANDLE handle,
	const char* outputDir,
	uint32_t namingMode,
	PACK_EXPORT_CALLBACK_UTF8 callback,
	void* userData);

// 校验当前索引中的所有图片：idx 文件头、dat 文件存在性、dat 帧头、offset、jpgSize、sourceIndex/timeValue 一致性。
// 首版只报告坏帧/缺失文件，不自动修复 idx。
PACKSDK_API int PACKSDK_CALL Pack_Verify(
	PACK_HANDLE handle,
	PACK_VERIFY_REPORT* report);

// 获取最近一次错误，返回内部缓存指针；调用方不要释放，也不要长期保存该指针。
PACKSDK_API const wchar_t* PACKSDK_CALL Pack_GetLastError(PACK_HANDLE handle);

// UTF-8 版本错误文本。Ubuntu/Linux 推荐使用。
PACKSDK_API const char* PACKSDK_CALL Pack_GetLastErrorUtf8(PACK_HANDLE handle);

// 预留的跨语言释放接口。当前 SDK 暂未返回需要调用方释放的动态内存。
PACKSDK_API void PACKSDK_CALL Pack_FreeMemory(void* ptr);

#ifdef __cplusplus
}
#endif
