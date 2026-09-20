#pragma once
// Public SDK integer types.
// On MSVC (including VS2015/v140) use compiler built-ins directly instead of
// importing uint64_t/uint32_t from potentially mixed CRT/STL headers.
#if defined(_MSC_VER)
typedef unsigned __int8  IMAGEPACK_U8;
typedef unsigned __int32 IMAGEPACK_U32;
typedef unsigned __int64 IMAGEPACK_U64;
typedef __int32          IMAGEPACK_I32;
#else
#  include <stdint.h>
typedef uint8_t  IMAGEPACK_U8;
typedef uint32_t IMAGEPACK_U32;
typedef uint64_t IMAGEPACK_U64;
typedef int32_t  IMAGEPACK_I32;
#endif

#if defined(_WIN32)
#  ifdef IMAGEPACK_DLL_EXPORTS
#    define IMAGEPACK_API __declspec(dllexport)
#  else
#    define IMAGEPACK_API __declspec(dllimport)
#  endif
#  define IMAGEPACK_CALL __cdecl
#else
#  define IMAGEPACK_API
#  define IMAGEPACK_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IMAGEPACK_API_VERSION_MAJOR 1
#define IMAGEPACK_API_VERSION_MINOR 1
#define IMAGEPACK_API_VERSION_PATCH 1
#define IMAGEPACK_API_VERSION ((IMAGEPACK_API_VERSION_MAJOR << 16) | (IMAGEPACK_API_VERSION_MINOR << 8) | IMAGEPACK_API_VERSION_PATCH)
#define IMAGEPACK_INVALID_INDEX ((IMAGEPACK_U64)(~(IMAGEPACK_U64)0))

typedef void* IMAGEPACK_HANDLE;

typedef enum IMAGEPACK_RESULT_CODE {
    IMAGEPACK_OK = 0,
    IMAGEPACK_E_INVALID_ARGUMENT = -1,
    IMAGEPACK_E_INVALID_HANDLE = -2,
    IMAGEPACK_E_NOT_OPEN = -3,
    IMAGEPACK_E_WRONG_MODE = -4,
    IMAGEPACK_E_STRUCT_SIZE_MISMATCH = -5,
    IMAGEPACK_E_OPEN_FAILED = -10,
    IMAGEPACK_E_READ_FAILED = -11,
    IMAGEPACK_E_WRITE_FAILED = -12,
    IMAGEPACK_E_BUFFER_TOO_SMALL = -13,
    IMAGEPACK_E_INDEX_OUT_OF_RANGE = -14,
    IMAGEPACK_E_VERIFY_FAILED = -20,
    IMAGEPACK_E_EXPORT_FAILED = -21,
    IMAGEPACK_E_CANCELLED = -22,
    IMAGEPACK_E_INTERNAL = -100
} IMAGEPACK_RESULT_CODE;

typedef enum IMAGEPACK_API_OPEN_MODE {
    IMAGEPACK_API_MODE_NONE = 0,
    IMAGEPACK_API_MODE_WRITE = 1,
    IMAGEPACK_API_MODE_READ = 2
} IMAGEPACK_API_OPEN_MODE;

typedef enum IMAGEPACK_API_VERIFY_LEVEL {
    IMAGEPACK_API_VERIFY_STRUCTURE = 0,
    IMAGEPACK_API_VERIFY_JPEG_MARKER = 1,
    IMAGEPACK_API_VERIFY_JPEG_DECODE = 2
} IMAGEPACK_API_VERIFY_LEVEL;

typedef enum IMAGEPACK_API_EXPORT_SCOPE {
    IMAGEPACK_API_EXPORT_ALL = 0,
    IMAGEPACK_API_EXPORT_GLOBAL_INDEX_RANGE = 1,
    IMAGEPACK_API_EXPORT_SOURCE_INDEX_LIST = 2,
    IMAGEPACK_API_EXPORT_TIME_RANGE = 3
} IMAGEPACK_API_EXPORT_SCOPE;

#pragma pack(push, 8)
typedef struct IMAGEPACK_API_PACK_INFO {
    IMAGEPACK_U32 structSize;
    IMAGEPACK_U64 createTimeValue;
    IMAGEPACK_U64 imageCount;
    IMAGEPACK_U32 dataFileCount;
} IMAGEPACK_API_PACK_INFO;

typedef struct IMAGEPACK_API_IMAGE_INFO {
    IMAGEPACK_U32 structSize;
    IMAGEPACK_U64 globalIndex;
    IMAGEPACK_U64 sourceIndex;
    IMAGEPACK_U64 timeValue;
    IMAGEPACK_U32 dataFileNumber;
    IMAGEPACK_U64 dataOffset;
    IMAGEPACK_U32 jpegSize;
} IMAGEPACK_API_IMAGE_INFO;

typedef struct IMAGEPACK_API_VERIFY_REPORT {
    IMAGEPACK_U32 structSize;
    IMAGEPACK_I32 verifyLevel;
    IMAGEPACK_U64 snapshotImageCount;
    IMAGEPACK_U64 checkedCount;
    IMAGEPACK_U64 failedGlobalIndex;
    IMAGEPACK_I32 cancelled;
} IMAGEPACK_API_VERIFY_REPORT;

typedef struct IMAGEPACK_API_EXPORT_REQUEST {
    IMAGEPACK_U32 structSize;
    IMAGEPACK_I32 scope;
    const char* outputDirectoryUtf8;
    IMAGEPACK_U64 beginGlobalIndex;
    IMAGEPACK_U64 endGlobalIndex;
    const IMAGEPACK_U64* sourceIndexes;
    IMAGEPACK_U64 sourceIndexCount;
    IMAGEPACK_U64 beginTimeValue;
    IMAGEPACK_U64 endTimeValue;
} IMAGEPACK_API_EXPORT_REQUEST;

typedef struct IMAGEPACK_API_EXPORT_RESULT {
    IMAGEPACK_U32 structSize;
    IMAGEPACK_U64 snapshotImageCount;
    IMAGEPACK_U64 requestedCount;
    IMAGEPACK_U64 exportedCount;
    IMAGEPACK_U64 failedCount;
    IMAGEPACK_I32 cancelled;
} IMAGEPACK_API_EXPORT_RESULT;
#pragma pack(pop)

// 返回非 0 继续；返回 0 请求取消 Verify / Export。
typedef int (IMAGEPACK_CALL *IMAGEPACK_PROGRESS_CALLBACK)(IMAGEPACK_U64 completed, IMAGEPACK_U64 total, void* userData);

// SDK / 生命周期 -------------------------------------------------------------
IMAGEPACK_API IMAGEPACK_U32 IMAGEPACK_CALL ImagePack_GetApiVersion(void);
IMAGEPACK_API const char* IMAGEPACK_CALL ImagePack_GetApiVersionString(void);
IMAGEPACK_API const char* IMAGEPACK_CALL ImagePack_GetResultDescription(IMAGEPACK_I32 resultCode);
IMAGEPACK_API IMAGEPACK_HANDLE IMAGEPACK_CALL ImagePack_Create(void);
IMAGEPACK_API void IMAGEPACK_CALL ImagePack_Destroy(IMAGEPACK_HANDLE handle);

// 所有 const char* 路径参数均为 UTF-8；成功返回 IMAGEPACK_OK，失败返回负数错误码。
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetOpenMode(IMAGEPACK_HANDLE handle, IMAGEPACK_I32* outMode);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_IsOpen(IMAGEPACK_HANDLE handle, IMAGEPACK_I32* outIsOpen);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetLastError(IMAGEPACK_HANDLE handle,
                                                        char* buffer,
                                                        IMAGEPACK_U32 bufferSize,
                                                        IMAGEPACK_U32* outRequiredSize);

// 写入 ----------------------------------------------------------------------
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_OpenWriter(IMAGEPACK_HANDLE handle,
                                                       const char* outputDirUtf8,
                                                       const char* fileTimeTagUtf8,
                                                       IMAGEPACK_U64 createTimeValue);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_Append(IMAGEPACK_HANDLE handle,
                                                  IMAGEPACK_U64 sourceIndex,
                                                  IMAGEPACK_U64 timeValue,
                                                  const IMAGEPACK_U8* jpegData,
                                                  IMAGEPACK_U32 jpegSize);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_Flush(IMAGEPACK_HANDLE handle);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_Close(IMAGEPACK_HANDLE handle);

// 读取 ----------------------------------------------------------------------
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_OpenReader(IMAGEPACK_HANDLE handle, const char* indexPathUtf8);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_Refresh(IMAGEPACK_HANDLE handle, IMAGEPACK_I32* outWasReset);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetPackInfo(IMAGEPACK_HANDLE handle, IMAGEPACK_API_PACK_INFO* outInfo);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetImageCount(IMAGEPACK_HANDLE handle, IMAGEPACK_U64* outCount);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetImageInfo(IMAGEPACK_HANDLE handle,
                                                        IMAGEPACK_U64 globalIndex,
                                                        IMAGEPACK_API_IMAGE_INFO* outInfo);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetFirstImageInfo(IMAGEPACK_HANDLE handle,
                                                             IMAGEPACK_API_IMAGE_INFO* outInfo);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetLastImageInfo(IMAGEPACK_HANDLE handle,
                                                            IMAGEPACK_API_IMAGE_INFO* outInfo);

// 分页读取。items=NULL/capacity=0 可先查询 outTotalMatched；capacity 不足时返回 BUFFER_TOO_SMALL。
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetImageList(IMAGEPACK_HANDLE handle,
                                                        IMAGEPACK_U64 beginGlobalIndex,
                                                        IMAGEPACK_U64 maximumCount,
                                                        IMAGEPACK_API_IMAGE_INFO* items,
                                                        IMAGEPACK_U64 capacity,
                                                        IMAGEPACK_U64* outWritten,
                                                        IMAGEPACK_U64* outTotalMatched);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_FindBySourceIndex(IMAGEPACK_HANDLE handle,
                                                             IMAGEPACK_U64 sourceIndex,
                                                             IMAGEPACK_API_IMAGE_INFO* items,
                                                             IMAGEPACK_U64 capacity,
                                                             IMAGEPACK_U64* outWritten,
                                                             IMAGEPACK_U64* outTotalMatched);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_FindByTimeValue(IMAGEPACK_HANDLE handle,
                                                           IMAGEPACK_U64 timeValue,
                                                           IMAGEPACK_API_IMAGE_INFO* items,
                                                           IMAGEPACK_U64 capacity,
                                                           IMAGEPACK_U64* outWritten,
                                                           IMAGEPACK_U64* outTotalMatched);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_FindByTimeRange(IMAGEPACK_HANDLE handle,
                                                           IMAGEPACK_U64 beginTimeValue,
                                                           IMAGEPACK_U64 endTimeValue,
                                                           IMAGEPACK_API_IMAGE_INFO* items,
                                                           IMAGEPACK_U64 capacity,
                                                           IMAGEPACK_U64* outWritten,
                                                           IMAGEPACK_U64* outTotalMatched);

// 新增：查找 timeValue 距离 target 最近的一张；时间差写入 outAbsoluteDifference。
// 当前实现为 O(N) 扫描读取快照，适合时间定位/预览；高频百万级调用时建议上层缓存索引。
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_FindNearestByTimeValue(IMAGEPACK_HANDLE handle,
                                                                  IMAGEPACK_U64 targetTimeValue,
                                                                  IMAGEPACK_API_IMAGE_INFO* outInfo,
                                                                  IMAGEPACK_U64* outAbsoluteDifference);

// JPEG 两阶段读取：先 GetJpegSize，再分配 buffer，最后 ReadJpeg。
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_GetJpegSize(IMAGEPACK_HANDLE handle,
                                                       IMAGEPACK_U64 globalIndex,
                                                       IMAGEPACK_U32* outJpegSize);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_ReadJpeg(IMAGEPACK_HANDLE handle,
                                                    IMAGEPACK_U64 globalIndex,
                                                    IMAGEPACK_U8* buffer,
                                                    IMAGEPACK_U32 bufferSize,
                                                    IMAGEPACK_U32* outActualSize);

// 校验 / 导出 ---------------------------------------------------------------
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_Verify(IMAGEPACK_HANDLE handle,
                                                   IMAGEPACK_I32 verifyLevel,
                                                   IMAGEPACK_API_VERIFY_REPORT* outReport,
                                                   IMAGEPACK_PROGRESS_CALLBACK callback,
                                                   void* userData);
IMAGEPACK_API int IMAGEPACK_CALL ImagePack_ExportImages(IMAGEPACK_HANDLE handle,
                                                        const IMAGEPACK_API_EXPORT_REQUEST* request,
                                                        IMAGEPACK_API_EXPORT_RESULT* outResult,
                                                        IMAGEPACK_PROGRESS_CALLBACK callback,
                                                        void* userData);

#ifdef __cplusplus
}
#endif
