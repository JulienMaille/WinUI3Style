#include "winui3theme_p.h"

#include "winui3tokens_p.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QSettings>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3::Private {

namespace {

#ifdef Q_OS_WIN
// Reading the native registry through QSettings and querying DWM are both
// comparatively expensive for calls made from the style's paint/palette
// paths. Keep the latest system values for one appearance refresh slice. The
// native watcher invalidates this cache before checking and a slow watchdog
// covers missed notifications, while avoiding duplicate reads from dark(),
// standardPalette(), and accentColor() during the same frame.
constexpr qint64 systemAppearanceCacheLifetimeMs = 250;

struct SystemAppearanceCache
{
    QElapsedTimer darkAge;
    bool darkInitialized = false;
    bool dark = false;
    QElapsedTimer accentAge;
    bool accentInitialized = false;
    SystemAccentRamp accentRamp;

    bool darkFresh() const
    {
        return darkAge.isValid()
            && darkAge.elapsed() < systemAppearanceCacheLifetimeMs;
    }

    bool accentFresh() const
    {
        return accentAge.isValid()
            && accentAge.elapsed() < systemAppearanceCacheLifetimeMs;
    }
};

SystemAppearanceCache &systemAppearanceCache()
{
    // All callers are on Qt's GUI thread. Keeping this cache process-local
    // also avoids sharing QSettings instances across threads.
    static SystemAppearanceCache cache;
    return cache;
}
#endif

} // namespace

bool systemUsesDarkTheme()
{
#ifdef Q_OS_WIN
    auto &cache = systemAppearanceCache();
    if (cache.darkInitialized && cache.darkFresh())
        return cache.dark;
    QSettings settings(QStringLiteral(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    cache.dark = settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
    cache.darkInitialized = true;
    cache.darkAge.start();
    return cache.dark;
#else
    return qGray(QApplication::palette().color(QPalette::Window).rgb()) < 128;
#endif
}

void invalidateSystemAppearanceCache()
{
#ifdef Q_OS_WIN
    auto &cache = systemAppearanceCache();
    cache.darkInitialized = false;
    cache.accentInitialized = false;
#endif
}

SystemAccentRamp systemAccentRamp()
{
    SystemAccentRamp ramp;
#ifdef Q_OS_WIN
    auto &cache = systemAppearanceCache();
    if (cache.accentInitialized && cache.accentFresh())
        return cache.accentRamp;
    // Explorer stores the Windows accent ramp as RGBA entries ordered
    // Light3, Light2, Light1, Accent, Dark1, Dark2, Dark3, complement.
    // These are the same SystemAccentColor* roles consumed by WinUI's
    // Common_themeresources_any.xaml.
    BYTE nativeBytes[32] = {};
    DWORD nativeSize = sizeof(nativeBytes);
    const LSTATUS paletteStatus = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
        L"AccentPalette", RRF_RT_REG_BINARY, nullptr, nativeBytes, &nativeSize);
    const QByteArray bytes = paletteStatus == ERROR_SUCCESS
        ? QByteArray(reinterpret_cast<const char *>(nativeBytes), int(nativeSize))
        : QByteArray{};
    const auto entry = [&bytes](int index) {
        const int offset = index * 4;
        if (bytes.size() < offset + 3)
            return QColor{};
        return QColor(quint8(bytes.at(offset)), quint8(bytes.at(offset + 1)),
                      quint8(bytes.at(offset + 2)));
    };
    const SystemAccentRamp explorerRamp{entry(3), entry(2), entry(1), entry(4)};
    // AccentPalette is the exact SystemAccentColor role family consumed by
    // WinUI. DwmGetColorizationColor is not equivalent: Windows may apply its
    // colorization-balance transform, yielding an intermediate colour that is
    // visibly wrong when used to synthesize Light2/Dark1.
    if (explorerRamp.accent.isValid() && explorerRamp.light1.isValid()
        && explorerRamp.light2.isValid() && explorerRamp.dark1.isValid()) {
        ramp = explorerRamp;
    } else {
        DWORD color = 0;
        BOOL opaque = FALSE;
        if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
            // DwmGetColorizationColor returns 0xAARRGGBB.
            ramp.accent = QColor::fromRgb((color >> 16) & 0xff,
                                          (color >> 8) & 0xff,
                                          color & 0xff);
        }
    }
#endif
    if (!ramp.accent.isValid())
        ramp.accent = QColor(0, 120, 212);
    if (!ramp.light1.isValid())
        ramp.light1 = mix(ramp.accent, QColor(Qt::white), 0.15);
    if (!ramp.light2.isValid())
        ramp.light2 = mix(ramp.accent, QColor(Qt::white), 0.32);
    if (!ramp.dark1.isValid())
        ramp.dark1 = mix(ramp.accent, QColor(Qt::black), 0.18);
#ifdef Q_OS_WIN
    cache.accentRamp = ramp;
    cache.accentInitialized = true;
    cache.accentAge.start();
#endif
    return ramp;
}

QColor systemAccentColor()
{
    return systemAccentRamp().accent;
}

QPalette standardPalette(bool darkTheme, const QColor &accent,
                         bool explicitAccent)
{
    const SystemAccentRamp systemRamp = explicitAccent
        ? SystemAccentRamp{} : systemAccentRamp();
    // WinUI AccentFillColorDefaultBrush is theme-specific: Light uses
    // SystemAccentColorDark1, while Dark uses SystemAccentColorLight2.
    const QColor accentFill = explicitAccent
        ? (darkTheme ? mix(accent, QColor(Qt::white), 0.32)
                     : mix(accent, QColor(Qt::black), 0.18))
        : (darkTheme ? systemRamp.light2 : systemRamp.dark1);
    QPalette palette;

    if (darkTheme) {
        palette.setColor(QPalette::Window, QColor(32, 32, 32));
        palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
        palette.setColor(QPalette::Base, QColor(58, 58, 58, 76));
        palette.setColor(QPalette::AlternateBase, QColor(255, 255, 255, 13));
        palette.setColor(QPalette::Button, QColor(255, 255, 255, 15));
        palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
        palette.setColor(QPalette::Text, QColor(255, 255, 255));
        palette.setColor(QPalette::PlaceholderText, QColor(255, 255, 255, 197));
        palette.setColor(QPalette::Mid, QColor(255, 255, 255, 18));
        palette.setColor(QPalette::Midlight, QColor(255, 255, 255, 24));
        palette.setColor(QPalette::Dark, QColor(0, 0, 0, 80));
        palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 160));
        palette.setColor(QPalette::ToolTipBase, QColor(44, 44, 44));
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                         QColor(255, 255, 255, 93));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(255, 255, 255, 11));
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255, 255, 255, 8));
    } else {
        palette.setColor(QPalette::Window, QColor(243, 243, 243));
        palette.setColor(QPalette::WindowText, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::Base, QColor(255, 255, 255, 128));
        palette.setColor(QPalette::AlternateBase, QColor(246, 246, 246, 128));
        palette.setColor(QPalette::Button, QColor(255, 255, 255, 179));
        palette.setColor(QPalette::ButtonText, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::Text, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 158));
        palette.setColor(QPalette::Mid, QColor(0, 0, 0, 15));
        palette.setColor(QPalette::Midlight, QColor(0, 0, 0, 41));
        palette.setColor(QPalette::Dark, QColor(0, 0, 0, 41));
        palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 90));
        palette.setColor(QPalette::ToolTipBase, QColor(249, 249, 249));
        palette.setColor(QPalette::ToolTipText, QColor(26, 26, 26));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                         QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(249, 249, 249, 77));
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(246, 246, 246, 128));
    }

    // TextOnAccentFillColorSelectedText is fixed white in both WinUI theme
    // dictionaries; it is distinct from button text-on-accent roles.
    const QColor textOnAccent(Qt::white);
    palette.setColor(QPalette::Highlight, accent); // system selection accent
    palette.setColor(QPalette::HighlightedText, textOnAccent);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::LinkVisited, accent.darker(112));
    palette.setColor(QPalette::BrightText, textOnAccent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // WinUI uses SystemAccentColor for selection, but Light2 in dark mode and
    // Dark1 in light mode for AccentFillColorDefault.
    palette.setColor(QPalette::Accent, accentFill);
#endif
    return palette;
}

} // namespace WinUI3::Private
