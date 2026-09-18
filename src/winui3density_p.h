// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Metrics are deliberately kept in a private, header-only contract while the
// mode itself uses the public enum. A local winuiDensity dynamic property
// always wins over the style's densityMode property, which also makes the
// contract usable from Designer-created forms.

#include <winui3style/winui3style.h>

#include <QMetaType>
#include <QApplication>
#include <QStyle>
#include <QVariant>
#include <QWidget>

namespace WinUI3::Private {

using DensityMode = WinUI3::DensityMode;

struct DensityMetrics
{
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

// All 54 fields are int, so the struct size pins the field count: adding,
// removing, or reordering a field breaks the static_assert below instead of
// silently misaligning the positional tables that used to live here.
static_assert(sizeof(DensityMetrics) == 54 * sizeof(int),
              "DensityMetrics field count changed: update the builder functions below");

// The project builds as C++17 (see CMAKE_CXX_STANDARD), so C++20
// designated initializers are unavailable. These constexpr builders assign
// every field by name instead, which keeps each value attached to its field
// across reorders/inserts. Values are pinned evidence-values: do not change
// a number without updating the pinned WinUI source comment beside it.
inline constexpr DensityMetrics makeStandardDensityMetrics()
{
    DensityMetrics m{};
    // Documented Compact Sizing controls (Default profile).
    m.textBoxHeight = 32;
    m.comboBoxHeight = 32;
    m.comboPopupItemHeight = 40;
    m.menuBarItemHeight = 32;
    m.menuItemHeight = 36;
    m.menuItemHeightInComboBox = 40;
    m.menuSeparatorHeight = 7;
    m.listItemHeight = 40;
    m.treeItemHeight = 28;
    m.tableItemHeight = 36;
    m.navigationItemHeight = 40;
    m.headerHeight = 32;
    // Existing contracts for controls outside that list.
    m.buttonHeight = 32;
    m.toolButtonHeight = 32;
    m.tabHeight = 32;
    m.spinButtonWidth = 36;
    m.verticalSpinButtonWidth = 32;
    m.comboArrowWidth = 38;
    m.toolButtonMenuWidth = 24;
    // Insets.
    m.buttonHorizontalPadding = 12;
    m.buttonVerticalPadding = 6;
    m.comboHorizontalPadding = 25;
    m.comboVerticalPadding = 6;
    m.lineEditHorizontalPadding = 8;
    m.lineEditVerticalPadding = 6;
    m.menuBarHorizontalPadding = 12;
    m.menuBarVerticalPadding = 6;
    m.tabHorizontalPadding = 8;
    m.tabVerticalPadding = 4;
    m.headerHorizontalPadding = 12;
    m.headerVerticalPadding = 4;
    m.toolButtonHorizontalPadding = 6;
    m.toolButtonVerticalPadding = 6;
    m.menuItemHorizontalPadding = 8;
    m.menuItemShortcutGap = 20;
    m.menuItemIconSlot = 42;
    m.menuItemNoIconSlot = 16;
    m.comboEditLeftPadding = 12;
    // Stable template slots.
    m.indicatorSize = 20;
    m.toggleTrackWidth = 40;
    m.toggleTrackHeight = 20;
    m.toggleSlotWidth = 40;
    m.toggleSlotHeight = 32;
    m.scrollBarExtent = 12;
    m.scrollBarSliderMinimum = 30;
    m.sliderThickness = 20;
    m.sliderLength = 20;
    m.sliderGrooveMargin = 14;
    m.sliderGrooveThickness = 4;
    m.sliderHandleSize = 18;
    m.tabCloseWidth = 32;
    m.tabCloseHeight = 24;
    m.itemSelectionGutter = 12;
    m.treeIndent = 20;
    return m;
}

inline constexpr DensityMetrics makeCompactDensityMetrics()
{
    DensityMetrics m{};
    // Compact Sizing resource: the editor/menu/list family loses one
    // standard 8px spacing step (32 -> 24 for editors and MenuBar).
    m.textBoxHeight = 24;
    m.comboBoxHeight = 24;
    m.comboPopupItemHeight = 32;
    m.menuBarItemHeight = 24;
    m.menuItemHeight = 36;
    m.menuItemHeightInComboBox = 32;
    m.menuSeparatorHeight = 7;
    m.listItemHeight = 32;
    m.treeItemHeight = 24;
    m.tableItemHeight = 36;
    m.navigationItemHeight = 32;
    m.headerHeight = 32;
    // Buttons, tabs, and sliders are not on that resource page; preserve.
    m.buttonHeight = 32;
    m.toolButtonHeight = 32;
    m.tabHeight = 32;
    m.spinButtonWidth = 36;
    m.verticalSpinButtonWidth = 32;
    m.comboArrowWidth = 30;
    m.toolButtonMenuWidth = 24;
    // Insets follow only the controls whose template is compacted.
    m.buttonHorizontalPadding = 12;
    m.buttonVerticalPadding = 6;
    m.comboHorizontalPadding = 25;
    m.comboVerticalPadding = 4;
    m.lineEditHorizontalPadding = 8;
    m.lineEditVerticalPadding = 4;
    m.menuBarHorizontalPadding = 8;
    m.menuBarVerticalPadding = 4;
    m.tabHorizontalPadding = 8;
    m.tabVerticalPadding = 4;
    m.headerHorizontalPadding = 12;
    m.headerVerticalPadding = 4;
    m.toolButtonHorizontalPadding = 6;
    m.toolButtonVerticalPadding = 6;
    m.menuItemHorizontalPadding = 8;
    m.menuItemShortcutGap = 20;
    m.menuItemIconSlot = 42;
    m.menuItemNoIconSlot = 16;
    m.comboEditLeftPadding = 12;
    // Stable template slots.
    m.indicatorSize = 20;
    m.toggleTrackWidth = 40;
    m.toggleTrackHeight = 20;
    m.toggleSlotWidth = 40;
    m.toggleSlotHeight = 32;
    m.scrollBarExtent = 12;
    m.scrollBarSliderMinimum = 30;
    m.sliderThickness = 20;
    m.sliderLength = 20;
    m.sliderGrooveMargin = 14;
    m.sliderGrooveThickness = 4;
    m.sliderHandleSize = 18;
    m.tabCloseWidth = 32;
    m.tabCloseHeight = 24;
    m.itemSelectionGutter = 12;
    m.treeIndent = 20;
    return m;
}

inline constexpr DensityMetrics standardDensityMetrics = makeStandardDensityMetrics();

inline constexpr DensityMetrics compactDensityMetrics = makeCompactDensityMetrics();

inline constexpr const DensityMetrics &densityMetrics(DensityMode mode)
{
    return mode == DensityMode::Compact ? compactDensityMetrics : standardDensityMetrics;
}

inline bool parseDensity(const QVariant &value, DensityMode *mode)
{
    if (!value.isValid() || !mode)
        return false;

    if (value.userType() == qMetaTypeId<WinUI3::DensityMode>()) {
        *mode = value.value<WinUI3::DensityMode>();
        return *mode == DensityMode::Standard || *mode == DensityMode::Compact;
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
    if (value.userType() == QMetaType::Bool)
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
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        DensityMode mode = DensityMode::Standard;
        if (parseDensity(candidate->property("winuiDensity"), &mode))
            return mode;
    }

    // QMenuBar and a few other Qt geometry queries call the style with a
    // null widget.  Resolve the application style in that case; otherwise
    // those queries silently fall back to Standard while a global Compact
    // profile is active.  Prefer the concrete API so setDensityMode() and a
    // dynamic densityMode property have exactly the same result.
    const QStyle *style = widget ? widget->style() : (qApp ? qApp->style() : nullptr);
    if (const auto *winui = qobject_cast<const WinUI3::Style *>(style)) {
        if (widget) {
            // The winuiDensity ancestor walk above already found nothing, so
            // effectiveDensityMode(widget) would only re-walk the same
            // ancestors before returning the global mode. Return it directly.
            return winui->densityMode();
        }
        // Null widget: resolve through the same densityMode property parse
        // as the generic path below (the Q_PROPERTY exposes the same global
        // mode), falling back to the concrete accessor.
        DensityMode mode = winui->densityMode();
        if (parseDensity(style->property("densityMode"), &mode))
            return mode;
        return winui->densityMode();
    }
    if (style) {
        DensityMode mode = DensityMode::Standard;
        if (parseDensity(style->property("densityMode"), &mode))
            return mode;
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

inline const DensityMetrics &densityMetricsFor(const QWidget *widget,
                                               const WinUI3::Style *fallbackStyle)
{
    if (widget)
        return densityMetricsFor(widget);
    return densityMetrics(fallbackStyle ? fallbackStyle->densityMode() : densityModeFor(nullptr));
}

} // namespace WinUI3::Private
