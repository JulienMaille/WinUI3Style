#pragma once

// Metrics are deliberately kept in a private, header-only contract while the
// mode itself uses the public enum. A local winuiDensity dynamic property
// always wins over the style's densityMode property, which also makes the
// contract usable from Designer-created forms.

#include <winui3style/winui3style.h>

#include <QMetaType>
#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace WinUI3::Private {

using DensityMode = WinUI3::DensityMode;

struct DensityMetrics {
    // The controls for which WinUI's Compact Sizing resource is documented.
    int textBoxHeight;
    int comboBoxHeight;
    int comboPopupItemHeight;
    int menuBarItemHeight;
    int menuItemHeight;
    int menuItemHeightInComboBox;
    int menuSeparatorHeight;
    int listItemHeight;
    int treeItemHeight;
    int tableItemHeight;
    int navigationItemHeight;
    int headerHeight;

    // Controls not covered by the Compact Sizing page retain their current
    // contract.  Keeping their values here prevents a later density change
    // from accidentally changing their painting or hit targets.
    int buttonHeight;
    int toolButtonHeight;
    int tabHeight;
    int spinButtonWidth;
    int verticalSpinButtonWidth;
    int comboArrowWidth;
    int toolButtonMenuWidth;

    // Horizontal/vertical insets.  These are per-side values unless the
    // field name says "Total".
    int buttonHorizontalPadding;
    int buttonVerticalPadding;
    int comboHorizontalPadding;
    int comboVerticalPadding;
    int lineEditHorizontalPadding;
    int lineEditVerticalPadding;
    int menuBarHorizontalPadding;
    int menuBarVerticalPadding;
    int tabHorizontalPadding;
    int tabVerticalPadding;
    int headerHorizontalPadding;
    int headerVerticalPadding;
    int toolButtonHorizontalPadding;
    int toolButtonVerticalPadding;
    int menuItemHorizontalPadding;
    int menuItemShortcutGap;
    int menuItemIconSlot;
    int menuItemNoIconSlot;
    int comboEditLeftPadding;

    // Stable template slots (not density-scaled by WinUI).
    int indicatorSize;
    int toggleTrackWidth;
    int toggleTrackHeight;
    int toggleSlotWidth;
    int toggleSlotHeight;
    int scrollBarExtent;
    int scrollBarSliderMinimum;
    int sliderThickness;
    int sliderLength;
    int sliderGrooveMargin;
    int sliderGrooveThickness;
    int sliderHandleSize;
    int tabCloseWidth;
    int tabCloseHeight;
    int itemSelectionGutter;
    int treeIndent;
};

inline constexpr DensityMetrics standardDensityMetrics{
    // Documented Compact Sizing controls (Default profile).
    32, 32, 40, 32, 36, 40, 7, 40, 28, 36, 40, 32,
    // Existing contracts for controls outside that list.
    32, 32, 32, 36, 32, 38, 24,
    // Insets.
    12, 6, 25, 6, 8, 6, 12, 6, 8, 4, 12, 4, 6, 6, 8, 20, 42, 16, 12,
    // Stable template slots.
    20, 40, 20, 40, 32, 12, 30, 20, 20, 14, 4, 18, 32, 24, 12, 20
};

inline constexpr DensityMetrics compactDensityMetrics{
    // Compact Sizing resource: the editor/menu/list family loses one
    // standard 8px spacing step (32 -> 24 for editors and MenuBar).
    24, 24, 32, 24, 36, 32, 7, 32, 24, 36, 32, 32,
    // Buttons, tabs, and sliders are not on that resource page; preserve.
    32, 32, 32, 36, 32, 30, 24,
    // Insets follow only the controls whose template is compacted.
    12, 6, 25, 4, 8, 4, 8, 4, 8, 4, 12, 4, 6, 6, 8, 20, 42, 16, 12,
    // Stable template slots.
    20, 40, 20, 40, 32, 12, 30, 20, 20, 14, 4, 18, 32, 24, 12, 20
};

inline constexpr const DensityMetrics &densityMetrics(DensityMode mode)
{
    return mode == DensityMode::Compact ? compactDensityMetrics
                                        : standardDensityMetrics;
}

inline bool parseDensity(const QVariant &value, DensityMode *mode)
{
    if (!value.isValid() || !mode)
        return false;

    if (value.userType() == qMetaTypeId<WinUI3::DensityMode>()) {
        *mode = value.value<WinUI3::DensityMode>();
        return *mode == DensityMode::Standard
            || *mode == DensityMode::Compact;
    }

    const QString text = value.toString().trimmed().toLower();
    if (text == QLatin1String("compact") || text == QLatin1String("dense")) {
        *mode = DensityMode::Compact;
        return true;
    }
    if (text == QLatin1String("standard") || text == QLatin1String("default")
        || text == QLatin1String("normal")) {
        *mode = DensityMode::Standard;
        return true;
    }

    // Q_PROPERTY enums arrive as an integer QVariant on both Qt 5 and Qt 6.
    // Do not interpret an accidental boolean dynamic property as a density.
    if (value.type() == QVariant::Bool)
        return false;
    bool ok = false;
    const int numeric = value.toInt(&ok);
    if (!ok || (numeric != 0 && numeric != 1))
        return false;
    *mode = numeric == 1 ? DensityMode::Compact : DensityMode::Standard;
    return true;
}

inline DensityMode densityModeFor(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        DensityMode mode = DensityMode::Standard;
        if (parseDensity(candidate->property("winuiDensity"), &mode))
            return mode;
    }

    if (widget) {
        if (const QStyle *style = widget->style()) {
            DensityMode mode = DensityMode::Standard;
            if (parseDensity(style->property("densityMode"), &mode))
                return mode;
        }
    }
    return DensityMode::Standard;
}

inline DensityMode effectiveDensity(const QWidget *widget)
{
    return densityModeFor(widget);
}

inline const DensityMetrics &densityMetricsFor(const QWidget *widget)
{
    return densityMetrics(densityModeFor(widget));
}

} // namespace WinUI3::Private
