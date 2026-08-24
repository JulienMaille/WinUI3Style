#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QWidget>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3 {

bool applyBackdrop(QWidget *window, Backdrop backdrop)
{
#ifdef Q_OS_WIN
    if (!window || !window->isWindow())
        return false;

    constexpr auto backdropProperty = "_winui_backdrop";
    constexpr auto originalWindowColorProperty = "_winui_original_window_color";
    constexpr auto originalPaletteProperty = "_winui_backdrop_original_palette";
    constexpr auto originalPaletteExplicitProperty = "_winui_backdrop_original_palette_explicit";
    constexpr auto originalTranslucentProperty = "_winui_backdrop_original_translucent";
    constexpr auto originalNoSystemBackgroundProperty = "_winui_backdrop_original_no_system_background";
    constexpr auto originalOpaquePaintProperty = "_winui_backdrop_original_opaque_paint";
    constexpr auto originalAutoFillProperty = "_winui_backdrop_original_auto_fill";
    const auto remember = [window](const char *name, const QVariant &value) {
        if (!window->property(name).isValid())
            window->setProperty(name, value);
    };
    // Remember state even for a direct None request. This keeps disabling an
    // unknown/external backdrop idempotent instead of manufacturing an
    // explicit palette or changing QWidget attributes as a side effect.
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
    if (!window->property(originalWindowColorProperty).isValid())
        window->setProperty(originalWindowColorProperty, window->palette().color(QPalette::Window));
    window->setProperty(backdropProperty, static_cast<int>(backdrop));

    // DWM draws the backdrop behind the window's redirection surface.  Keep
    // Qt's top-level surface alpha-capable and stop QWidget from erasing it;
    // DWMWA_USE_HOSTBACKDROPBRUSH below lets the compositor supply the pixels.
    if (!window->isVisible())
        window->setAttribute(Qt::WA_TranslucentBackground, backdrop != Backdrop::None);
    window->setAttribute(Qt::WA_NoSystemBackground, backdrop != Backdrop::None);
    window->setAttribute(Qt::WA_OpaquePaintEvent, false);
    window->setAutoFillBackground(backdrop == Backdrop::None);
    QPalette materialPalette = window->palette();
    QColor themedWindowColor = QApplication::palette().color(QPalette::Window);
    if (const auto *style = qobject_cast<const Style *>(QApplication::style()))
        themedWindowColor = style->standardPalette().color(QPalette::Window);
    QColor windowColor = backdrop == Backdrop::None
        ? window->property(originalWindowColorProperty).value<QColor>()
        : themedWindowColor;
    windowColor.setAlpha(backdrop == Backdrop::None ? 255 : 0);
    materialPalette.setColor(QPalette::Window, windowColor);
    window->setPalette(materialPalette);

    constexpr DWORD systemBackdropAttribute = 38;
    constexpr DWORD useHostBackdropBrushAttribute = 17;
    constexpr DWORD immersiveDarkModeAttribute = 20;
    constexpr DWORD borderColorAttribute = 34;
    constexpr DWORD captionColorAttribute = 35;
    constexpr DWORD textColorAttribute = 36;
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
    const BOOL useHostBackdrop = backdrop != Backdrop::None;
    DwmSetWindowAttribute(hwnd, useHostBackdropBrushAttribute,
                          &useHostBackdrop, sizeof(useHostBackdrop));
    const HRESULT backdropResult = DwmSetWindowAttribute(
        hwnd, systemBackdropAttribute, &value, sizeof(value));

    MARGINS margins{};
    if (backdrop != Backdrop::None)
        margins = {-1, -1, -1, -1};
    const HRESULT frameResult = DwmExtendFrameIntoClientArea(hwnd, &margins);
    const bool applied = SUCCEEDED(backdropResult) && SUCCEEDED(frameResult);
    if (backdrop == Backdrop::None) {
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
    if (!applied && backdrop != Backdrop::None) {
        window->setAttribute(Qt::WA_NoSystemBackground, false);
        window->setAutoFillBackground(true);
        QPalette fallback = window->palette();
        fallback.setColor(QPalette::Window, themedWindowColor);
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
