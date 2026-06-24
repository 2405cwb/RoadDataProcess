#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNQTCOMMON_LIB)
#  define HNQTCOMMON_EXPORT Q_DECL_EXPORT
# else
#  define HNQTCOMMON_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNQTCOMMON_EXPORT
#endif
