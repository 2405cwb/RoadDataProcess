#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNIO_LIB)
#  define HNIO_EXPORT Q_DECL_EXPORT
# else
#  define HNIO_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNIO_EXPORT
#endif
