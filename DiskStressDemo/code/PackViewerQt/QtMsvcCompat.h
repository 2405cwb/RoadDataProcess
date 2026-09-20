#pragma once

// VS2015/VS2017 等旧 MSVC 工具集如果搭配很新的 Windows 10/11 SDK，
// 可能会在 ucrt\wchar.h 中遇到 `_mm_loadu_si64` 未定义。
// 新版 UCRT 头假定编译器提供该 intrinsic，但旧编译器只提供 `_mm_loadl_epi64`。
// 这个兼容头必须在任何 Qt/CRT 头文件之前包含，用旧 intrinsic 补一个等价实现。
#if defined(_MSC_VER) && defined(_M_X64) && _MSC_VER < 1920
#include <emmintrin.h>
static __inline __m128i _mm_loadu_si64(void const* p)
{
	return _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p));
}
#endif
