#ifndef HNCONVERT_GLOBAL_H
#define HNCONVERT_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef HNCONVERT_LIB
# define HNCONVERT_EXPORT Q_DECL_EXPORT
#else
# define HNCONVERT_EXPORT Q_DECL_IMPORT
#endif

#endif // HNCONVERT_GLOBAL_H
