#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNAPPLICATION_LIB)
#  define HNAPPLICATION_EXPORT Q_DECL_EXPORT
# else
#  define HNAPPLICATION_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNAPPLICATION_EXPORT
#endif
