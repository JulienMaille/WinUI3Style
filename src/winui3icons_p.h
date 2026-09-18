// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Test seams for the Fluent icon font cache (spec/METHODOLOGY.md section 6:
// defect tests target the mechanism, not the screenshot). Production code
// must not use these; they expose the GUI-thread-only IconRuntime so the
// icons unit test can prove worker-thread paint never touches the shared
// cache and that cached fonts cannot escape as dangling references.
//
// Fixed signatures: fluentFontForTest() returns a QFont by value
// (implicitly shared, cheap) so callers never alias the QCache-owned slot,
// and offThreadFluentFontForTest() constructs the font without touching the
// shared IconRuntime.

#include <winui3style/winui3global.h>

#include <QFont>

namespace WinUI3::Private {

WINUI3STYLE_EXPORT QFont fluentFontForTest(int pixelSize);
WINUI3STYLE_EXPORT QFont offThreadFluentFontForTest(int pixelSize);
WINUI3STYLE_EXPORT bool iconFontCacheResolvedForTest();
WINUI3STYLE_EXPORT void invalidateIconCachesForTest();

} // namespace WinUI3::Private
