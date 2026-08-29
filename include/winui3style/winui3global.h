#pragma once

#include <QtCore/qglobal.h>

#if defined(WINUI3STYLE_STATIC)
#  define WINUI3STYLE_EXPORT
#elif defined(WINUI3STYLE_LIBRARY)
#  define WINUI3STYLE_EXPORT Q_DECL_EXPORT
#else
#  define WINUI3STYLE_EXPORT Q_DECL_IMPORT
#endif

