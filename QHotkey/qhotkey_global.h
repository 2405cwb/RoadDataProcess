#pragma once
#ifdef QHOTKEY_SHARED
#	ifdef QHOTKEY_LIBRARY
#		define QHOTKEY_EXPORT Q_DECL_EXPORT
#	else
#		define QHOTKEY_EXPORT Q_DECL_IMPORT
#	endif
#else
#	define QHOTKEY_EXPORT
#endif