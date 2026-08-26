#include "winui3theme_p.h"

#include "winui3tokens_p.h"

#include <QApplication>
#include <QSettings>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3::Private {

bool systemUsesDarkTheme()
{
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return qGray(QApplication::palette().color(QPalette::Window).rgb()) < 128;
#endif
}

SystemAccentRamp systemAccentRamp()
{
    SystemAccentRamp ramp;
#ifdef Q_OS_WIN
    // Explorer stores the Windows accent ramp as BGRA entries ordered
    // Light3, Light2, Light1, Accent, Dark1, Dark2, Dark3, complement.
    // These are the same SystemAccentColor* roles consumed by WinUI's
    // Common_themeresources_any.xaml.
    // DWM is the live system source used by the shell and WinUI Gallery. Do
    // not combine its current base with Explorer's cached role entries: that
    // produces a ramp whose selection and control-fill roles belong to
    // different accent families when the registry lags behind the shell.
    DWORD color = 0;
    BOOL opaque = FALSE;
    bool dwmAccentAvailable = false;
    if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
        QColor result = QColor::fromRgba(color);
        result.setAlpha(255);
        if (result.isValid()) {
            ramp.accent = result;
            ramp.light2 = mix(ramp.accent, QColor(Qt::white), 0.32);
            ramp.dark1 = mix(ramp.accent, QColor(Qt::black), 0.18);
            dwmAccentAvailable = true;
        }
    }

    if (!dwmAccentAvailable) {
        // AccentPalette is a complete fallback source. It is intentionally
        // read only when DWM cannot provide the live family, so the three
        // roles remain atomic.
        QSettings settings(QStringLiteral(
            "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent"),
            QSettings::NativeFormat);
        const QByteArray bytes = settings.value(QStringLiteral("AccentPalette")).toByteArray();
        const auto entry = [&bytes](int index) {
            const int offset = index * 4;
            if (bytes.size() < offset + 3)
                return QColor{};
            return QColor(quint8(bytes.at(offset + 2)), quint8(bytes.at(offset + 1)),
                          quint8(bytes.at(offset)));
        };
        ramp.light2 = entry(1);
        ramp.accent = entry(3);
        ramp.dark1 = entry(4);
    }
#endif
    if (!ramp.accent.isValid())
        ramp.accent = QColor(0, 120, 212);
    if (!ramp.light2.isValid())
        ramp.light2 = mix(ramp.accent, QColor(Qt::white), 0.32);
    if (!ramp.dark1.isValid())
        ramp.dark1 = mix(ramp.accent, QColor(Qt::black), 0.18);
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
    const QColor accentFill = explicitAccent
        ? mix(accent, darkTheme ? QColor(Qt::white) : QColor(Qt::black),
              darkTheme ? 0.32 : 0.18)
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

    const QColor textOnAccent = contrastText(accent);
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
