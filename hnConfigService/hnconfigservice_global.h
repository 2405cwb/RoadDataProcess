#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNCONFIGSERVICE_LIB)
#  define HNCONFIGSERVICE_EXPORT Q_DECL_EXPORT
# else
#  define HNCONFIGSERVICE_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNCONFIGSERVICE_EXPORT
#endif
