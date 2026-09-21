// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <winui3style/winui3global.h>

#include <QColor>
#include <QObject>
#include <QProxyStyle>

#include <memory>

class QCheckBox;
class QDialog;
class QAbstractItemView;
class QAbstractSpinBox;
class QFrame;
class QString;
class QStyle;
class QWidget;

namespace WinUI3 {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
Q_NAMESPACE_EXPORT(WINUI3STYLE_EXPORT)
#else
#  ifdef Q_MOC_RUN
Q_NAMESPACE
#  else
extern WINUI3STYLE_EXPORT const QMetaObject staticMetaObject;
#  endif
#endif

enum class ThemeMode { System, Light, Dark };

// Controls the density metrics used by the style. Compact is the WinUI 3
// compact sizing profile; it is intentionally independent from theme colors.
enum class DensityMode { Standard, Compact };

enum class ControlRole { Standard, Accent, Subtle, Navigation, Destructive };

Q_ENUM_NS(ThemeMode)
Q_ENUM_NS(DensityMode)
Q_ENUM_NS(ControlRole)

class StylePrivate;

class WINUI3STYLE_EXPORT Style final : public QProxyStyle
{
    Q_OBJECT
    Q_PROPERTY(WinUI3::ThemeMode themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(WinUI3::DensityMode densityMode READ densityMode WRITE setDensityMode NOTIFY
                       densityChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit Style(ThemeMode mode = ThemeMode::System);
    explicit Style(DensityMode density);
    Style(ThemeMode mode, DensityMode density);
    // QProxyStyle composition entry point: wraps a custom base style (e.g.
    // QFusionStyle) instead of the default QCommonStyle. Additive overload;
    // the existing ctors above keep working identically.
    Style(QStyle *base, ThemeMode mode, DensityMode density);
    ~Style() override;

    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);

    DensityMode densityMode() const;
    void setDensityMode(DensityMode mode);

    // Resolves a widget's local winuiDensity property through its parent
    // chain, falling back to this style's global density mode.
    DensityMode effectiveDensityMode(const QWidget *widget) const;

    QColor accentColor() const;
    void setAccentColor(const QColor &color);

    // The style and compound widgets use the same switch so capture mode,
    // reduced-motion environments, and application code cannot disagree.
    static bool animationsAllowed();

    // Whether mnemonic underlines are currently shown (Alt pressed, Windows
    // Alt-reveal contract). Tracked by the style's event filter; paint sites
    // query it through SH_UnderlineShortcut. GUI-thread-only: both the setter
    // and the getter assert the GUI thread (the setter repaints top-level
    // widgets, which is itself GUI-thread-only).
    static bool altMnemonicsVisible();
    static void setAltMnemonicsVisible(bool visible);

    static void setControlRole(QWidget *widget, ControlRole role);
    static ControlRole controlRole(const QWidget *widget);

    // Dynamic-property helpers are convenient from C++ and Designer. The
    // same property can also be set directly in a .ui file.
    static constexpr const char *DensityProperty = "winuiDensity";
    static void setDensityMode(QWidget *widget, DensityMode mode);
    static DensityMode densityMode(const QWidget *widget);
    static void clearDensityMode(QWidget *widget);

    // Designer-friendly semantic properties. Applications may use the helper
    // functions, but every visual variant can also be declared as a dynamic
    // property in a .ui file without including this library's headers.
    static constexpr const char *ControlRoleProperty = "winuiControlRole";
    static constexpr const char *BackdropProperty = "winuiBackdrop";
    static constexpr const char *SurfaceProperty = "winuiSurface";
    static constexpr const char *ToggleSwitchProperty = "winuiToggleSwitch";
    static constexpr const char *ToggleSwitchOnTextProperty = "winuiOnText";
    static constexpr const char *ToggleSwitchOffTextProperty = "winuiOffText";
    static constexpr const char *SettingsCardProperty = "winuiSettingsCard";
    static constexpr const char *NavigationViewProperty = "winuiNavigationView";
    static constexpr const char *VerticalSpinButtonsProperty = "winuiVerticalSpinButtons";
    static constexpr const char *ContentDialogProperty = "winuiContentDialog";
    static void setToggleSwitch(QCheckBox *checkBox, bool enabled = true);
    static bool isToggleSwitch(const QCheckBox *checkBox);
    static void setToggleSwitchText(QCheckBox *checkBox, const QString &onText,
                                    const QString &offText);
    static void setSettingsCard(QFrame *frame, bool enabled = true);
    static void setNavigationView(QAbstractItemView *view, bool enabled = true);
    static void setVerticalSpinButtons(QAbstractSpinBox *spinBox, bool enabled = true);
    static bool hasVerticalSpinButtons(const QAbstractSpinBox *spinBox);
    static void setContentDialog(QDialog *dialog, bool enabled = true);

    QPalette standardPalette() const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override;
    void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                     const QWidget *widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget = nullptr) const override;
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &contentsSize,
                           const QWidget *widget = nullptr) const override;
    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget = nullptr) const override;
    SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                     const QPoint &position,
                                     const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override;
    QIcon standardIcon(StandardPixmap icon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const override;

    void polish(QApplication *application) override;
    void polish(QWidget *widget) override;
    void polish(QPalette &palette) override;
    void unpolish(QApplication *application) override;
    void unpolish(QWidget *widget) override;

signals:
    void themeChanged(WinUI3::ThemeMode mode);
    void densityChanged(WinUI3::DensityMode mode);
    void accentColorChanged(const QColor &color);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // GUI-thread-only: repolishes the application palette, theme, and
    // density registrations and emits themeChanged/densityChanged. Must run
    // on the GUI thread (touches top-level widgets and palettes).
    void refreshApplicationAppearance();
    // GUI-thread-only: re-runs the owned-palette surface pass for one window
    // (islands + registered palette owners) after its effective backdrop
    // surface changed, so an inline calendar converges onto the granted
    // material instead of keeping the opaque surface computed at polish.
    void refreshOwnedPalettes(QWidget *window);
    // GUI-thread-only: invalidates density geometry for scope (or every
    // top-level window when null) and repaints synchronously.
    void invalidateDensity(QWidget *scope = nullptr);
    // GUI-thread-only: polls the system theme/accent cache and, on change,
    // calls refreshApplicationAppearance() plus themeChanged/accent signals.
    void checkSystemAppearance();
    std::unique_ptr<StylePrivate> d;
};

} // namespace WinUI3

Q_DECLARE_METATYPE(WinUI3::ThemeMode)
Q_DECLARE_METATYPE(WinUI3::DensityMode)
Q_DECLARE_METATYPE(WinUI3::ControlRole)
