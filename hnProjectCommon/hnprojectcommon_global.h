#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNPROJECTCOMMON_LIB)
#  define HNPROJECTCOMMON_EXPORT Q_DECL_EXPORT
# else
#  define HNPROJECTCOMMON_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNPROJECTCOMMON_EXPORT
#endif
