#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(HNPROJECT_LIB)
#  define HNPROJECT_EXPORT Q_DECL_EXPORT
# else
#  define HNPROJECT_EXPORT Q_DECL_IMPORT
# endif
#else
# define HNPROJECT_EXPORT
#endif
