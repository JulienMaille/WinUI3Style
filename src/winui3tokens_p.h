#pragma once

#include <QColor>
#include <QPalette>
#include <array>
#include <cstddef>
#include <cmath>

namespace WinUI3::Private {

constexpr int FasterDuration = 83;
constexpr int FastDuration = 167;
// AnimatedAcceptVisualSource uses the fast control transition for the
// NormalOffToNormalOn segment. Its generated source has a short initial hold
// followed by the path reveal; the Qt implementation remaps those keyframes
// onto this same 167 ms control duration.
constexpr int CheckBoxDuration = FastDuration;
constexpr int NormalDuration = 250;
constexpr qreal ControlRadius = 4.0;
constexpr qreal OverlayRadius = 8.0;

struct Tokens {
    bool dark = false;
    QColor textPrimary;
    QColor textSecondary;
    QColor textTertiary;
    QColor textDisabled;
    QColor surface;
    QColor layer;
    QColor control;
    QColor controlHover;
    QColor controlPressed;
    QColor controlDisabled;
    QColor subtleHover;
    QColor subtlePressed;
    QColor stroke;
    QColor strokeSecondary;
    QColor strokeStrong;
    QColor accentStroke;
    QColor accentStrokeSecondary;
    QColor focusOuter;
    QColor focusInner;
    // WinUI keeps selection and control-fill roles distinct even when the
    // platform accent happens to make them visually similar.
    QColor selectionAccent;
    QColor accentFill;
    QColor accentFillHover;
    QColor accentFillPressed;
    QColor accentFillDisabled;
    // WinUI's check/radio/toggle templates always use white ink on their
    // accent fill. Keep this separate from textOnAccentPrimary, which is
    // contrast-resolved so arbitrary custom accent buttons remain readable.
    QColor controlOnAccentPrimary;
    QColor controlOnAccentDisabled;
    QColor textOnAccentPrimary;
    QColor textOnAccentSecondary;
    QColor textOnAccentDisabled;
    QColor danger;
};

inline QColor mix(const QColor &a, const QColor &b, qreal amount)
{
    amount = qBound<qreal>(0.0, amount, 1.0);
    return QColor::fromRgbF(
        a.redF() + (b.redF() - a.redF()) * amount,
        a.greenF() + (b.greenF() - a.greenF()) * amount,
        a.blueF() + (b.blueF() - a.blueF()) * amount,
        a.alphaF() + (b.alphaF() - a.alphaF()) * amount);
}

inline QColor withAlpha(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

// Opaque flyout/toolbar surface, derived from the palette Window color with a
// fixed per-theme lift so popup and window fills keep their WinUI relationship
// for any palette rather than only for the default one. Reproduces the
// previous hardcoded values exactly for the default palette
// (#202020 -> #2C2C2C dark, #F3F3F3 -> #FCFCFC light).
inline QColor popupSurfaceColor(const QPalette &palette)
{
    const QColor window = palette.color(QPalette::Window);
    const int lift = qGray(window.rgb()) < 128 ? 12 : 9;
    return QColor(qMin(255, window.red() + lift),
                  qMin(255, window.green() + lift),
                  qMin(255, window.blue() + lift));
}

inline qreal relativeLuminance(const QColor &color)
{
    const auto channel = [](qreal value) {
        return value <= 0.04045 ? value / 12.92
                                : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(color.redF())
         + 0.7152 * channel(color.greenF())
         + 0.0722 * channel(color.blueF());
}

inline QColor contrastText(const QColor &background)
{
    const qreal luminance = relativeLuminance(background);
    const qreal contrastWithBlack = (luminance + 0.05) / 0.05;
    const qreal contrastWithWhite = 1.05 / (luminance + 0.05);
    return contrastWithBlack >= contrastWithWhite ? QColor(Qt::black)
                                                   : QColor(Qt::white);
}

inline Tokens buildTokens(const QPalette &palette)
{
    Tokens t;
    // Palette anchors. Most WinUI tokens are alpha-modulated versions of the
    // text ink or the paper color, so overriding the application or widget
    // palette propagates through the whole ramp. Only states with no
    // QPalette role (hover elevations, accent strokes, danger) stay pinned to
    // their WinUI constants below. For the palette built by
    // WinUI3::standardPalette() every derivation here reproduces the original
    // hardcoded theme values exactly (guarded by
    // tst_winui3style::paletteDerivedTokensMatchWinUIConstants).
    const QColor ink = palette.color(QPalette::WindowText);
    t.dark = qGray(palette.color(QPalette::Window).rgb()) < 128;
    t.textPrimary = ink;
    t.textSecondary = withAlpha(ink, t.dark ? 197 : 158);
    t.textTertiary = withAlpha(ink, t.dark ? 135 : 114);
    t.textDisabled = withAlpha(ink, t.dark ? 93 : 92);
    t.surface = palette.color(QPalette::Window);
    t.layer = palette.color(QPalette::Base);
    t.control = palette.color(QPalette::Button);
    // Hover fills: the dark ramp overlays the ink; the light ramp sits white
    // cards on top of a darker window, so it blends the control fill with the
    // paper beneath. Pressed either fades the overlay or pushes toward ink.
    t.controlHover = t.dark ? withAlpha(ink, 21)
                            : withAlpha(mix(t.control, t.surface, 0.5), 128);
    // Light pressed must land visibly darker than the resting fill; an
    // alpha-only change over the light window reads as no response. 26/255 of
    // the way from the control fill to ink reproduces WinUI
    // ControlFillColorTertiary (#E5E5E5) at the control opacity exactly.
    t.controlPressed = t.dark ? withAlpha(ink, 8)
                              : withAlpha(mix(t.control, ink, 26.0 / 255.0),
                                          179);
    t.controlDisabled = palette.color(QPalette::Disabled, QPalette::Button);
    t.subtleHover = withAlpha(ink, 15);
    t.subtlePressed = withAlpha(ink, t.dark ? 10 : 22);
    t.stroke = palette.color(QPalette::Mid);
    t.strokeSecondary = palette.color(QPalette::Midlight);
    t.strokeStrong = withAlpha(ink, t.dark ? 139 : 114);
    t.accentStroke = QColor(255, 255, 255, 20);
    t.accentStrokeSecondary = t.dark ? QColor(0, 0, 0, 35) : QColor(0, 0, 0, 102);
    t.focusOuter = t.dark ? Qt::white : Qt::black;
    t.focusInner = t.dark ? Qt::black : Qt::white;
    t.selectionAccent = palette.color(QPalette::Highlight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    t.accentFill = palette.color(QPalette::Accent);
    if (!t.accentFill.isValid())
        t.accentFill = t.selectionAccent;
#else
    // QPalette::Accent was introduced in Qt 6.6. Keep the WinUI selection
    // role distinct from the control-fill ramp for the supported Qt 6.5
    // build instead of collapsing both roles onto Highlight. The selection
    // role is the raw accent (QPalette::Highlight); map it onto WinUI's
    // AccentFillColorDefault ramp: Dark1 in Light, Light2 in Dark.
    t.accentFill = mix(t.selectionAccent,
                       t.dark ? QColor(Qt::white) : QColor(Qt::black),
                       t.dark ? 0.32 : 0.18);
#endif
    // WinUI maps pointer-over and pressed AccentFill brushes to the same
    // accent-ramp colour at 90% and 80% opacity. Do not synthesize unrelated
    // lighter/darker hues here.
    t.accentFillHover = t.accentFill;
    t.accentFillHover.setAlphaF(t.accentFill.alphaF() * 0.9);
    t.accentFillPressed = t.accentFill;
    t.accentFillPressed.setAlphaF(t.accentFill.alphaF() * 0.8);
    t.accentFillDisabled = t.dark ? QColor(255, 255, 255, 40)
                                  : QColor(0, 0, 0, 55);
    // CheckBox, RadioButton and ToggleSwitch use TextOnAccentFillColorPrimary
    // for their checked glyph/knob: black in Dark, white in Light.
    t.controlOnAccentPrimary = t.dark ? QColor(Qt::black) : QColor(Qt::white);
    t.controlOnAccentDisabled = t.dark ? QColor(255, 255, 255, 135)
                                       : QColor(Qt::white);
    // WinUI's TextOnAccentFillColorPrimary is a theme resource, not a
    // contrast calculation: white in Light and black in Dark.
    t.textOnAccentPrimary = t.dark ? QColor(Qt::black) : QColor(Qt::white);
    t.textOnAccentSecondary = t.dark ? QColor(0, 0, 0, 128)
                                     : QColor(255, 255, 255, 179);
    t.textOnAccentDisabled = t.textOnAccentPrimary;
    t.textOnAccentDisabled.setAlpha(135);
    t.danger = QColor(196, 43, 28);
    return t;
}

inline Tokens tokens(const QPalette &palette)
{
    // QPalette::cacheKey() changes with the palette's contents, including
    // widget-local overrides and runtime accent/theme updates. A small
    // thread-local ring avoids locks and heap allocations in paint paths
    // while bounding retained palette variants.
    struct CacheEntry {
        qint64 key = 0;
        bool valid = false;
        Tokens value;
    };
    thread_local std::array<CacheEntry, 8> cache;
    thread_local std::size_t next = 0;

    const qint64 key = palette.cacheKey();
    for (const CacheEntry &entry : cache) {
        if (entry.valid && entry.key == key)
            return entry.value;
    }

    CacheEntry &entry = cache[next++ % cache.size()];
    entry.value = buildTokens(palette);
    entry.key = key;
    entry.valid = true;
    return entry.value;
}

} // namespace WinUI3::Private
