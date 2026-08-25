#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QWidget>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3 {

namespace {

constexpr auto backdropProperty = "_winui_backdrop";
constexpr auto originalWindowColorProperty = "_winui_original_window_color";
constexpr auto originalPaletteProperty = "_winui_backdrop_original_palette";
constexpr auto originalPaletteExplicitProperty = "_winui_backdrop_original_palette_explicit";
constexpr auto originalTranslucentProperty = "_winui_backdrop_original_translucent";
constexpr auto originalNoSystemBackgroundProperty = "_winui_backdrop_original_no_system_background";
constexpr auto originalOpaquePaintProperty = "_winui_backdrop_original_opaque_paint";
constexpr auto originalAutoFillProperty = "_winui_backdrop_original_auto_fill";

#ifdef Q_OS_WIN
QColor themedWindowColor()
{
    QColor result = QApplication::palette().color(QPalette::Window);
    if (const auto *style = qobject_cast<const Style *>(QApplication::style()))
        result = style->standardPalette().color(QPalette::Window);
    return result;
}
#endif

void rememberBackdropState(QWidget *window)
{
    const auto remember = [window](const char *name, const QVariant &value) {
        if (!window->property(name).isValid())
            window->setProperty(name, value);
    };
    remember(originalPaletteProperty, QVariant::fromValue(window->palette()));
    remember(originalPaletteExplicitProperty,
             window->testAttribute(Qt::WA_SetPalette));
    remember(originalTranslucentProperty,
             window->testAttribute(Qt::WA_TranslucentBackground));
    remember(originalNoSystemBackgroundProperty,
             window->testAttribute(Qt::WA_NoSystemBackground));
    remember(originalOpaquePaintProperty,
             window->testAttribute(Qt::WA_OpaquePaintEvent));
    remember(originalAutoFillProperty, window->autoFillBackground());
    remember(originalWindowColorProperty,
             window->palette().color(QPalette::Window));
}

void restoreBackdropState(QWidget *window)
{
    if (window->property(originalTranslucentProperty).isValid())
        window->setAttribute(Qt::WA_TranslucentBackground,
                             window->property(originalTranslucentProperty).toBool());
    if (window->property(originalNoSystemBackgroundProperty).isValid())
        window->setAttribute(Qt::WA_NoSystemBackground,
                             window->property(originalNoSystemBackgroundProperty).toBool());
    if (window->property(originalOpaquePaintProperty).isValid())
        window->setAttribute(Qt::WA_OpaquePaintEvent,
                             window->property(originalOpaquePaintProperty).toBool());
    if (window->property(originalAutoFillProperty).isValid())
        window->setAutoFillBackground(
            window->property(originalAutoFillProperty).toBool());
    if (window->property(originalPaletteProperty).isValid()) {
        if (window->property(originalPaletteExplicitProperty).toBool())
            window->setPalette(
                window->property(originalPaletteProperty).value<QPalette>());
        else
            window->setPalette(QPalette());
    }
    window->setProperty(backdropProperty, {});
    window->setProperty(originalWindowColorProperty, {});
    window->setProperty(originalPaletteProperty, {});
    window->setProperty(originalPaletteExplicitProperty, {});
    window->setProperty(originalTranslucentProperty, {});
    window->setProperty(originalNoSystemBackgroundProperty, {});
    window->setProperty(originalOpaquePaintProperty, {});
    window->setProperty(originalAutoFillProperty, {});
}

} // namespace

namespace Private {

void prepareBackdropSurface(QWidget *window, Backdrop backdrop)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow() || backdrop == Backdrop::None)
        return;

    rememberBackdropState(window);
    window->setProperty(backdropProperty, static_cast<int>(backdrop));

    // Qt documents WA_TranslucentBackground as a pre-create attribute on
    // Windows. Popup HWNDs are created lazily between polish and Show, so the
    // alpha-capable surface must be configured before that lifetime boundary.
    // The offscreen QPA has no HWND or DWM surface; changing its backing-store
    // alpha mode would alter the deterministic fallback PNGs instead.
    if (QGuiApplication::platformName() != QStringLiteral("offscreen")) {
        window->setAttribute(Qt::WA_TranslucentBackground, true);
        window->setAttribute(Qt::WA_NoSystemBackground, true);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(false);
    }
#else
    Q_UNUSED(window)
    Q_UNUSED(backdrop)
#endif
}

} // namespace Private

bool applyBackdrop(QWidget *window, Backdrop backdrop)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow())
        return false;

    // Remember state even for a direct None request. This keeps disabling an
    // unknown/external backdrop idempotent instead of manufacturing an
    // explicit palette or changing QWidget attributes as a side effect.
    rememberBackdropState(window);
    if (backdrop != Backdrop::None)
        Private::prepareBackdropSurface(window, backdrop);
    window->setProperty(backdropProperty, static_cast<int>(backdrop));
    const bool nativeSurface =
        QGuiApplication::platformName() != QStringLiteral("offscreen");

    if (backdrop == Backdrop::None) {
        // A popup may have been polished (and its alpha surface prepared) but
        // never shown. Do not manufacture an HWND during unpolish/destruction
        // merely to clear DWM attributes that were never applied.
        if (!window->windowHandle()) {
            restoreBackdropState(window);
            return true;
        }
        window->setAttribute(Qt::WA_TranslucentBackground, false);
        window->setAttribute(Qt::WA_NoSystemBackground, false);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(true);
    } else {
        // These paint-surface flags are needed even when the platform has no
        // DWM compositor, so the offscreen fallback retains its transparent
        // popup corners. The translucent HWND flag is handled by the
        // pre-create helper only for native Windows surfaces.
        window->setAttribute(Qt::WA_NoSystemBackground, true);
        window->setAttribute(Qt::WA_OpaquePaintEvent, false);
        window->setAutoFillBackground(false);
    }

    QPalette materialPalette = window->palette();
    const QColor themedWindowColor = WinUI3::themedWindowColor();
    QColor windowColor = backdrop == Backdrop::None
        ? window->property(originalWindowColorProperty).value<QColor>()
        : themedWindowColor;
    windowColor.setAlpha(backdrop == Backdrop::None ? 255 : 0);
    materialPalette.setColor(QPalette::Window, windowColor);
    window->setPalette(materialPalette);

    if (!nativeSurface) {
        if (backdrop == Backdrop::None)
            restoreBackdropState(window);
        return true;
    }

    constexpr DWORD systemBackdropAttribute = 38;
    constexpr DWORD useHostBackdropBrushAttribute = 17;
    constexpr DWORD immersiveDarkModeAttribute = 20;
    constexpr DWORD borderColorAttribute = 34;
    constexpr DWORD captionColorAttribute = 35;
    constexpr DWORD textColorAttribute = 36;
    constexpr DWORD redirectionBitmapAlphaAttribute = 39;
    constexpr COLORREF colorNone = 0xFFFFFFFE;
    constexpr int backdropAuto = 0;
    constexpr int backdropNone = 1;
    constexpr int backdropMainWindow = 2;
    constexpr int backdropTransientWindow = 3;
    constexpr int backdropTabbedWindow = 4;

    int value = backdropNone;
    switch (backdrop) {
    case Backdrop::None: value = backdropNone; break;
    case Backdrop::Mica: value = backdropMainWindow; break;
    case Backdrop::MicaAlt: value = backdropTabbedWindow; break;
    case Backdrop::Acrylic: value = backdropTransientWindow; break;
    default: value = backdropAuto; break;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL dark = qGray(themedWindowColor.rgb()) < 128;
    DwmSetWindowAttribute(hwnd, immersiveDarkModeAttribute, &dark, sizeof(dark));
    const COLORREF caption = backdrop == Backdrop::None
        ? RGB(windowColor.red(), windowColor.green(), windowColor.blue()) : colorNone;
    const COLORREF text = dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    DwmSetWindowAttribute(hwnd, borderColorAttribute, &colorNone, sizeof(colorNone));
    DwmSetWindowAttribute(hwnd, captionColorAttribute, &caption, sizeof(caption));
    DwmSetWindowAttribute(hwnd, textColorAttribute, &text, sizeof(text));
    // DWMWA_USE_HOSTBACKDROPBRUSH opts into a Win32 Composition brush created
    // by the application. Qt does not create that brush; enabling it here
    // leaves the client surface empty. System backdrops are instead drawn by
    // DWMWA_SYSTEMBACKDROP_TYPE, so explicitly disable the host-brush path.
    const BOOL useHostBackdrop = FALSE;
    DwmSetWindowAttribute(hwnd, useHostBackdropBrushAttribute,
                          &useHostBackdrop, sizeof(useHostBackdrop));
    // Qt's translucent backing store is premultiplied ARGB. This attribute is
    // available on newer Windows 11 builds; older builds simply reject it,
    // while the system-backdrop result remains authoritative below.
    const BOOL useRedirectionAlpha = backdrop != Backdrop::None;
    DwmSetWindowAttribute(hwnd, redirectionBitmapAlphaAttribute,
                          &useRedirectionAlpha, sizeof(useRedirectionAlpha));
    const HRESULT backdropResult = DwmSetWindowAttribute(
        hwnd, systemBackdropAttribute, &value, sizeof(value));

    MARGINS margins{};
    if (backdrop != Backdrop::None)
        margins = {-1, -1, -1, -1};
    const HRESULT frameResult = DwmExtendFrameIntoClientArea(hwnd, &margins);
    const bool applied = SUCCEEDED(backdropResult) && SUCCEEDED(frameResult);
    if (backdrop == Backdrop::None) {
        restoreBackdropState(window);
    }
    if (!applied && backdrop != Backdrop::None) {
        if (window->property(originalTranslucentProperty).isValid())
            window->setAttribute(Qt::WA_TranslucentBackground,
                                 window->property(originalTranslucentProperty).toBool());
        window->setAttribute(Qt::WA_NoSystemBackground, false);
        if (window->property(originalOpaquePaintProperty).isValid())
            window->setAttribute(Qt::WA_OpaquePaintEvent,
                                 window->property(originalOpaquePaintProperty).toBool());
        window->setAutoFillBackground(true);
        QPalette fallback = window->palette();
        QColor fallbackWindowColor = themedWindowColor;
        fallbackWindowColor.setAlpha(255);
        fallback.setColor(QPalette::Window, fallbackWindowColor);
        window->setPalette(fallback);
    }
    return applied;
#else
    Q_UNUSED(window)
    Q_UNUSED(backdrop)
    return false;
#endif
}

} // namespace WinUI3
