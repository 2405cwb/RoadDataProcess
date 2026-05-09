#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNPAVEMENTCREATE3D_LIB)
#  define HNPAVEMENTCREATE3D_EXPORT Q_DECL_EXPORT
# else
#  define HNPAVEMENTCREATE3D_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNPAVEMENTCREATE3D_EXPORT
#endif
