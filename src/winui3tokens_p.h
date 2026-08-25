#pragma once

#include <QColor>
#include <QPalette>
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

inline Tokens tokens(const QPalette &palette)
{
    Tokens t;
    t.dark = qGray(palette.color(QPalette::Window).rgb()) < 128;
    t.textPrimary = t.dark ? QColor(255, 255, 255) : QColor(0, 0, 0, 228);
    t.textSecondary = t.dark ? QColor(255, 255, 255, 197) : QColor(0, 0, 0, 158);
    t.textTertiary = t.dark ? QColor(255, 255, 255, 135) : QColor(0, 0, 0, 114);
    t.textDisabled = t.dark ? QColor(255, 255, 255, 93) : QColor(0, 0, 0, 92);
    t.surface = palette.color(QPalette::Window);
    t.layer = t.dark ? QColor(58, 58, 58, 76) : QColor(255, 255, 255, 128);
    t.control = t.dark ? QColor(255, 255, 255, 15) : QColor(255, 255, 255, 179);
    t.controlHover = t.dark ? QColor(255, 255, 255, 21) : QColor(249, 249, 249, 128);
    t.controlPressed = t.dark ? QColor(255, 255, 255, 8) : QColor(249, 249, 249, 77);
    t.controlDisabled = t.dark ? QColor(255, 255, 255, 11) : QColor(249, 249, 249, 77);
    t.subtleHover = t.dark ? QColor(255, 255, 255, 15) : QColor(0, 0, 0, 9);
    t.subtlePressed = t.dark ? QColor(255, 255, 255, 10) : QColor(0, 0, 0, 6);
    t.stroke = t.dark ? QColor(255, 255, 255, 18) : QColor(0, 0, 0, 15);
    t.strokeSecondary = t.dark ? QColor(255, 255, 255, 24) : QColor(0, 0, 0, 41);
    t.strokeStrong = t.dark ? QColor(255, 255, 255, 139) : QColor(0, 0, 0, 114);
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
    // build instead of collapsing both roles onto Highlight.
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
    // A custom Windows accent can be arbitrarily light or dark. Resolve the
    // foreground from the actual fill so Accent buttons remain readable even
    // when the user selects a pale yellow or near-black accent.
    t.textOnAccentPrimary = contrastText(t.accentFill);
    t.textOnAccentSecondary = t.textOnAccentPrimary;
    t.textOnAccentSecondary.setAlpha(179);
    t.textOnAccentDisabled = t.textOnAccentPrimary;
    t.textOnAccentDisabled.setAlpha(135);
    t.danger = QColor(196, 43, 28);
    return t;
}

} // namespace WinUI3::Private
