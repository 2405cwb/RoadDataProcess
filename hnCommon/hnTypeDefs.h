/*! hnDefs.h
********************************************************************************
<PRE>
模块名       : hnCore
文件名       : hnDefs.h
相关文件     : 
文件实现功能 : 定义基本数据类型
作者         : 谢卓
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期			版本			修改人				修改内容
2017/07/15		1.0			谢卓					创建
</PRE>
*******************************************************************************/

#ifndef HNDEFS_H
#define HNDEFS_H

namespace hnCommon
{
	//! 8 bit unsigned variable.
	/** This is a typedef for unsigned char, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int8		u8;
#else
	typedef unsigned char		u8;
#endif

	//! 8 bit signed variable.
	/** This is a typedef for signed char, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int8			s8;
#else
	typedef signed char		s8;
#endif

	//! 8 bit character variable.
	/** This is a typedef for char, it ensures portability of the engine. */
	typedef char			c8;

	//! 16 bit unsigned variable.
	/** This is a typedef for unsigned short, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int16	u16;
#else
	typedef unsigned short		u16;
#endif

	//! 16 bit signed variable.
	/** This is a typedef for signed short, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int16			s16;
#else
	typedef signed short		s16;
#endif

	//! 32 bit unsigned variable.
	/** This is a typedef for unsigned int, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef unsigned __int32	u32;
#else
	typedef unsigned int		u32;
#endif

	//! 32 bit signed variable.
	/** This is a typedef for signed int, it ensures portability of the engine. */
#ifdef _MSC_VER
	typedef __int32			s32;
#else
	typedef signed int		s32;
#endif

#if defined(_WIN32)            // 64 byte integer under Windows 
typedef unsigned __int64   u64;
typedef __int64            s64;
#else                          // 64 byte integer elsewhere ... 
typedef unsigned long long u64;
typedef long long          s64;
#endif

	// 64 bit signed variable.
	// This is a typedef for __int64, it ensures portability of the engine.
	// This type is currently not used by the engine and not supported by compilers
	// other than Microsoft Compilers, so it is outcommented.
	//typedef __int64				s64;

	//! 32 bit floating point variable.
	/** This is a typedef for float, it ensures portability of the engine. */
	typedef float				f32;

	//! 64 bit floating point variable.
	/** This is a typedef for double, it ensures portability of the engine. */
	typedef double				f64;

	typedef char				I8;
	typedef short				I16;
	typedef int					I32;	
	typedef unsigned char		Byte;
}

#endif // HNDEFS_H