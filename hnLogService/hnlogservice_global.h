#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNLOGSERVICE_LIB)
#  define HNLOGSERVICE_EXPORT Q_DECL_EXPORT
# else
#  define HNLOGSERVICE_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNLOGSERVICE_EXPORT
#endif
