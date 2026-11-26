// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3style_contracts_p.h"

#include "winui3geometry_p.h"
#include "winui3density_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3typography_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QCheckBox>
#include <QMenu>
#include <QMenuBar>
#include <QCalendarWidget>
#include <QCommandLinkButton>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFontMetrics>
#include <QFrame>
#include <QListView>
#include <QProgressBar>
#include <QTableView>
#include <QTabBar>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSpinBox>
#include <QStyleOptionToolButton>
#include <QStyleOptionViewItem>
#include <QTreeView>
#include <QToolButton>
#include <QWizard>

namespace WinUI3::Private {
namespace {

// DATAGRIDTEXTCOLUMN_leftMargin/_rightMargin (both 12.0) from the pinned
// DataGridTextColumn template. Shared by the cell text rect and the item
// view's content size so a ResizeToContents column reserves the gutter it
// paints into.
constexpr int tableCellHorizontalPadding = 12;

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

bool comboPopupItemView(const QWidget *widget)
{
    const QAbstractItemView *view = itemView(widget);
    if (!view || !view->window() || view->window()->windowType() != Qt::Popup)
        return false;
    return qobject_cast<const QComboBox *>(view->window()->parentWidget());
}

bool calendarPopupItemView(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

const QAbstractItemView *selectionMarkerView(const QWidget *widget)
{
    const QAbstractItemView *view = itemView(widget);
    if (!view || (view->window() && view->window()->windowType() == Qt::Popup))
        return nullptr;
    if (qobject_cast<const QTableView *>(view))
        return nullptr;
    if (!qobject_cast<const QListView *>(view) && !qobject_cast<const QTreeView *>(view))
        return nullptr;
    return view;
}

} // namespace

int pixelMetric(const Style *style, QStyle::PixelMetric metric, const QStyleOption *option,
                const QWidget *widget)
{
    // Qt paints command-link contents itself and applies these metrics
    // immediately on press. Fluent's button composition keeps content fixed.
    if ((metric == QStyle::PM_ButtonShiftHorizontal || metric == QStyle::PM_ButtonShiftVertical)
        && qobject_cast<const QCommandLinkButton *>(widget))
        return 0;
    if (const auto value = pixelMetricValue(metric, toggleSwitch(widget), widget))
        return *value;
    return style->QProxyStyle::pixelMetric(metric, option, widget);
}

// Contents-size handlers: one per CT_* kind, extracted from the
// sizeFromContents switch by pure code motion. Each takes the running size
// and returns the adjusted hint (early geometry returns become plain
// returns); the switch only dispatches.
static QSize pushButtonSize(const QStyleOption *option, QSize size, const QWidget *widget,
                            const DensityMetrics &density)
{
    if (const auto *command = qobject_cast<const QCommandLinkButton *>(widget)) {
        // CommandLink's title is BodyStrong typography (issue #112).
        const QFont titleFont = winuiStrongFont(command->font());
        const int textWidth =
                qMax(QFontMetrics(titleFont).horizontalAdvance(command->text()),
                     command->fontMetrics().horizontalAdvance(command->description()));
        size.setWidth(qMax(160, textWidth + 64));
        size.setHeight(
                qMax(64, QFontMetrics(titleFont).height() + command->fontMetrics().height() + 22));
        return size;
    }
    size += QSize(2 * density.buttonHorizontalPadding, 2 * density.buttonVerticalPadding);
    if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
        button && (button->features & QStyleOptionButton::HasMenu))
        size.rwidth() += 22;
    size.setWidth(qMax(size.width(), 32));
    // Preserve the WinUI template for its default font, but an
    // application-supplied larger font needs enough room for its ink.
    size.setHeight(qMax(density.buttonHeight,
                        option ? option->fontMetrics.height() + 2 * density.buttonVerticalPadding
                               : size.height()));
    return size;
}

static QSize comboBoxSize(const QStyleOption *option, QSize size, const QWidget *widget,
                          const DensityMetrics &density, DensityMode mode)
{
    if (mode == DensityMode::Compact) {
        size.rwidth() += 2 * density.comboHorizontalPadding;
        const int targetHeight = density.comboBoxHeight;
        if (option
            && (option->fontMetrics.height() + 2 * density.comboVerticalPadding > targetHeight + 4))
            size.setHeight(option->fontMetrics.height() + 2 * density.comboVerticalPadding);
        else
            size.setHeight(targetHeight);
    } else {
        // Preserve the established Standard geometry pixel-for-pixel.
        size += QSize(2 * density.comboHorizontalPadding, 2 * density.comboVerticalPadding);
        size.setHeight(qMax(density.comboBoxHeight,
                            option ? option->fontMetrics.height() + 2 * density.comboVerticalPadding
                                   : size.height()));
    }
    if (const auto *combo = qobject_cast<const QComboBox *>(widget);
        combo && option && size.width() > 0) {
        // The closed-label slot (SC_ComboBoxEditField) reserves the left
        // padding plus the arrow slot, but sizeFromContents only sees
        // the text extent Qt offers: live rasterization of the same
        // item can exceed that extent ("Standard densi..."). Guarantee
        // the slot covers the longest item: grow the hint when the
        // offered text plus chrome exceeds it. An icon on any row
        // reserves a 16 px slot plus an 8 px gap in the paint path
        // (CE_ComboBoxLabel), so icon combos need that chrome too
        // ("Docum..." live). First-open popup tests still guard the
        // mapped metric after any sizeFromContents change
        // (METHODOLOGY §1).
        const QFontMetrics metrics(option->fontMetrics);
        int longest = 0;
        bool anyIcon = false;
        for (int row = 0; row < combo->count(); ++row) {
            longest = qMax(longest, metrics.horizontalAdvance(combo->itemText(row)));
            anyIcon = anyIcon || !combo->itemIcon(row).isNull();
        }
        int need = longest + density.comboEditLeftPadding + density.comboArrowWidth;
        if (anyIcon)
            need += 16 + 8;
        size.setWidth(qMax(size.width(), need));
    }
    size.setWidth(qMax(size.width(), 120));
    return size;
}

static QSize lineEditSize(const QStyleOption *option, QSize size, const DensityMetrics &density,
                          DensityMode mode)
{
    if (mode == DensityMode::Compact) {
        size.rwidth() += 2 * density.lineEditHorizontalPadding;
        const int targetHeight = density.textBoxHeight;
        if (option
            && (option->fontMetrics.height() + 2 * density.lineEditVerticalPadding
                > targetHeight + 4))
            size.setHeight(option->fontMetrics.height() + 2 * density.lineEditVerticalPadding);
        else
            size.setHeight(targetHeight);
    } else {
        size += QSize(2 * density.lineEditHorizontalPadding, 2 * density.lineEditVerticalPadding);
        size.setHeight(
                qMax(density.textBoxHeight,
                     option ? option->fontMetrics.height() + 2 * density.lineEditVerticalPadding
                            : size.height()));
    }
    return size;
}

static QSize spinBoxSize(const QStyleOption *option, QSize size, const QWidget *widget,
                         const DensityMetrics &density, DensityMode mode)
{
    // The left inset is part of the editor in every button mode. Only the
    // step slots are reclaimed when NoButtons removes them.
    size.rwidth() += 12;
    const auto *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option);
    const bool noButtons = spin && spin->buttonSymbols == QAbstractSpinBox::NoButtons;
    if (!noButtons)
        size.rwidth() += verticalSpinButtons(widget) ? density.verticalSpinButtonWidth
                                                     : 2 * density.spinButtonWidth;
    if (mode == DensityMode::Compact) {
        const int targetHeight = density.textBoxHeight;
        if (option && (option->fontMetrics.height() + 8 > targetHeight + 4))
            size.setHeight(option->fontMetrics.height() + 8);
        else
            size.setHeight(targetHeight);
    } else {
        size.setHeight(qMax(size.height(),
                            qobject_cast<const QDateTimeEdit *>(widget) ? density.textBoxHeight
                                                                        : density.buttonHeight));
    }
    size.setWidth(qMax(size.width(), 120));
    return size;
}

static QSize toolButtonSize(const QStyleOption *option, QSize size, const QWidget *widget,
                            const DensityMetrics &density, DensityMode mode)
{
    if (textBoxHelperButton(widget)) {
        // Qt positions these children explicitly; keep size hints within the editor height.
        if (mode == DensityMode::Compact)
            return QSize(density.textBoxHeight, density.textBoxHeight);
        return QSize(30, density.toolButtonHeight);
    }
    if (widget && qobject_cast<const QTabBar *>(widget->parentWidget()))
        return QSize(density.tabCloseWidth, density.tabCloseHeight);
    if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
        const QFontMetrics metrics(tool->fontMetrics);
        const int textWidth = metrics.horizontalAdvance(tool->text);
        const int iconWidth =
                tool->icon.isNull() ? 0 : (tool->iconSize.isValid() ? tool->iconSize.width() : 16);
        if (const auto *toolButton = qobject_cast<const QToolButton *>(widget)) {
            if (toolButton->toolButtonStyle() == Qt::ToolButtonTextBesideIcon && textWidth > 0
                && iconWidth > 0)
                size.setWidth(qMax(size.width(), textWidth + iconWidth + 22));
            else if (toolButton->toolButtonStyle() == Qt::ToolButtonTextOnly)
                size.setWidth(qMax(size.width(), textWidth + 16));
        }
        if (tool->features & QStyleOptionToolButton::MenuButtonPopup)
            size.rwidth() += density.toolButtonMenuWidth;
    }
    if (size.isEmpty())
        size = QSize(20, 20);
    size += QSize(2 * density.toolButtonHorizontalPadding, 2 * density.toolButtonVerticalPadding);
    size.setHeight(qMax(size.height(), density.toolButtonHeight));
    size.setWidth(qMax(size.width(), density.toolButtonHeight));
    return size;
}

static QSize menuBarItemSize(QSize size, const DensityMetrics &density, const QSize &contentsSize)
{
    size = contentsSize
            + QSize(2 * density.menuBarHorizontalPadding, 2 * density.menuBarVerticalPadding);
    size.setHeight(qMax(size.height(), density.menuBarItemHeight));
    return size;
}

static QSize tabBarTabSize(const QStyleOption *option, QSize size, const DensityMetrics &density)
{
    size += QSize(2 * density.tabHorizontalPadding, 2 * density.tabVerticalPadding);
    size.setHeight(qMax(density.tabHeight,
                        option ? option->fontMetrics.height() + 2 * density.tabVerticalPadding
                               : size.height()));
    size.setWidth(qBound(100, size.width(), 240));
    if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
        const bool vertical = tab->shape == QTabBar::RoundedEast
                || tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::TriangularEast
                || tab->shape == QTabBar::TriangularWest;
        if (vertical)
            size.transpose();
    }
    return size;
}

static QSize menuItemSize(const QStyleOption *option, QSize size, const QWidget *widget,
                          const DensityMetrics &density)
{
    if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
        if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
            size.setHeight(density.menuSeparatorHeight);
            return size;
        }
        const QStringList parts = menu->text.split(QLatin1Char('\t'));
        const QFontMetrics metrics(menu->font);
        const int mainWidth = metrics.horizontalAdvance(parts.value(0));
        const int shortcutWidth = parts.size() > 1 ? metrics.horizontalAdvance(parts.value(1)) : 0;
        const int trailing = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? density.toolButtonMenuWidth
                : 0;
        const bool comboItem = qobject_cast<const QComboBox *>(widget);
        const int leading = comboItem && menu->icon.isNull() ? density.menuItemNoIconSlot
                                                             : density.menuItemIconSlot;
        const int requiredWidth = leading + mainWidth + 2 * density.menuItemHorizontalPadding
                + trailing + (shortcutWidth > 0 ? density.menuItemShortcutGap + shortcutWidth : 0);
        size.setWidth(qMax(size.width(), qMax(requiredWidth, 120)));
        size.setHeight(comboItem ? density.menuItemHeightInComboBox : density.menuItemHeight);
        // Extension (not in WinUI Compact Sizing): Compact menu-popup rows
        // shrink 36 -> 32 like list rows. Visible popups are resized to
        // the new profile by the density invalidation below.
        if (!comboItem && density.menuBarItemHeight == 24)
            size.setHeight(32);
    }
    return size;
}

static QSize itemViewItemSize(const Style *style, QSize size, const QStyleOption *option,
                              const QWidget *widget, const DensityMetrics &density,
                              DensityMode mode)
{
    // QStyledItemDelegate::sizeHint() asks the style for the entire item and
    // passes an empty content size. Resolve the intrinsic text / decoration /
    // check-indicator extent from Qt's generic item layout: without it a
    // ResizeToContents header sees a zero-width hint and falls back to the
    // header label width, which is the CommitForm column-collapse defect.
    if (qstyleoption_cast<const QStyleOptionViewItem *>(option))
        size = size.expandedTo(style->QProxyStyle::sizeFromContents(QStyle::CT_ItemViewItem, option,
                                                                    QSize(), widget));

    if (comboPopupItemView(widget)) {
        // ComboBoxItem has its own resource.  Keep it separate from the
        // ListView row metric even while both profiles currently resolve
        // to the same 40/32 values; the templates are independently
        // configurable in WinUI and must not silently drift together.
        size.setHeight(density.comboPopupItemHeight);
    } else if (widget && widget->window() && widget->window()->windowType() == Qt::Popup) {
        size.setHeight(density.listItemHeight);
    } else if (qobject_cast<const QTreeView *>(itemView(widget))) {
        size.setHeight(density.treeItemHeight);
    } else if (const auto *table = qobject_cast<const QTableView *>(itemView(widget))) {
        if (!calendarPopupItemView(widget)) {
            if (const auto *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
                item && (item->index.flags() & Qt::ItemIsEditable)) {
                const int editorHeight = lineEditSize(option, QSize(), density, mode).height();
                size.setHeight(qMax(size.height(), editorHeight + int(table->showGrid())));
            }
            size.setHeight(qMax(size.height(), density.tableItemHeight));
        } else {
            size.setHeight(standardDensityMetrics.tableItemHeight);
        }
        // SE_ItemViewItemText insets a table cell by the DataGrid text
        // column's 12 px margin on both sides. The content size must carry
        // that same gutter, or ResizeToContents paints the text into a slot
        // narrower than the string it just measured.
        size.rwidth() += 2 * tableCellHorizontalPadding;
    } else {
        size.setHeight(density.listItemHeight);
    }
    return size;
}

static QSize headerSectionSize(const Style *style, QSize size, const QStyleOption *option,
                               const QWidget *widget, const DensityMetrics &density,
                               DensityMode mode)
{
    const bool compactItemView = mode == DensityMode::Compact && widget
            && (qobject_cast<const QTableView *>(widget->parentWidget())
                || qobject_cast<const QTreeView *>(widget->parentWidget()))
            && !calendarPopupItemView(widget);
    const int verticalPadding =
            compactItemView ? CompactItemViewHeaderVerticalPadding : density.headerVerticalPadding;
    const int minimumHeight = compactItemView ? CompactItemViewHeaderHeight : density.headerHeight;
    // QHeaderView::sectionSizeFromContents passes an empty content size and
    // expects the style to measure the title itself. Padding alone made every
    // section's hint the same minimum (43 px here), so a ResizeToContents
    // column with only a check indicator in its cells sized the header to the
    // cell instead of the "Commit" title, clipping the title away.
    if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
        int contentWidth = header->fontMetrics.horizontalAdvance(header->text);
        if (!header->icon.isNull()) {
            contentWidth += style->pixelMetric(QStyle::PM_SmallIconSize, header, widget) + 8;
        }
        int width = 2 * density.headerHorizontalPadding + contentWidth;
        // CE_Header reserves the pinned DataGrid template's trailing 32 px
        // sort column for every section whenever sorting is enabled. The
        // content hint has to carry it too, or a sortable title is clipped by
        // the very column the painter reserves.
        if (header->sortIndicator != QStyleOptionHeader::None)
            width += 32;
        size.rwidth() = qMax(size.width(), width);
        if (compactItemView)
            size.setHeight(qMax(size.height(), header->fontMetrics.height()));
    } else {
        size.rwidth() += 2 * density.headerHorizontalPadding;
    }
    size.rheight() += 2 * verticalPadding;
    size.setHeight(qMax(size.height(), minimumHeight));
    return size;
}

static QSize checkBoxSize(const QStyleOption *option, QSize size, const QWidget *widget,
                          const DensityMetrics &density, DensityMode mode)
{
    if (toggleSwitch(widget)) {
        const auto *check = qstyleoption_cast<const QStyleOptionButton *>(option);
        QString onText = widget->property(Style::ToggleSwitchOnTextProperty).toString();
        QString offText = widget->property(Style::ToggleSwitchOffTextProperty).toString();
        if (check && onText.isEmpty() && offText.isEmpty())
            onText = offText = check->text;
        const QFontMetrics metrics = check ? check->fontMetrics : widget->fontMetrics();
        const int labelWidth =
                qMax(metrics.horizontalAdvance(onText), metrics.horizontalAdvance(offText));
        size = QSize(labelWidth > 0 ? qMax(154, 50 + labelWidth) : density.toggleSlotWidth,
                     density.toggleSlotHeight);
        return size;
    }
    // Label is painted from x=32 with a 4px right margin; Qt passes only
    // the text size as contents, so the hint must cover indicator, gap,
    // and trailing margin to avoid clipping the last glyph.
    if (mode == DensityMode::Compact) {
        size += QSize(36, 4);
        size.setHeight(qMax(size.height(), 24));
    } else {
        size += QSize(36, 8);
        size.setHeight(qMax(size.height(), 28));
    }
    return size;
}

static QSize radioButtonSize(QSize size, const DensityMetrics &density, DensityMode mode)
{
    Q_UNUSED(density);
    if (mode == DensityMode::Compact) {
        size += QSize(36, 4);
        size.setHeight(qMax(size.height(), 24));
    } else {
        size += QSize(36, 8);
        size.setHeight(qMax(size.height(), 28));
    }
    return size;
}

static QSize progressBarSize(const QStyleOption *option, QSize size, const QWidget *widget)
{
    // QStyleOptionProgressBar::orientation only exists from Qt 5.13;
    // query the widget for a 5.12-compatible check.
    const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
    const bool horizontal = !progressBar || progressBar->orientation() == Qt::Horizontal;
    const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
    const bool showsText = bar && bar->textVisible && !bar->text.isEmpty() && horizontal;
    if (horizontal) {
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(showsText ? size.height() + 6 // text + thin underline
                                 : qMax(size.height(), 8));
    } else {
        size.setWidth(qMax(size.width(), 8));
        size.setHeight(qMax(size.height(), 120));
    }
    return size;
}

static QSize sliderSize(const QStyleOption *option, QSize size, const DensityMetrics &density)
{
    const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
    const bool horizontal = !slider || slider->orientation == Qt::Horizontal;
    if (horizontal) {
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), density.buttonHeight));
    } else {
        size.setWidth(qMax(size.width(), density.buttonHeight));
        size.setHeight(qMax(size.height(), 120));
    }
    return size;
}

static QSize scrollBarSize(QSize size)
{
    size.setWidth(qMax(size.width(), 12));
    size.setHeight(qMax(size.height(), 12));
    return size;
}

static QSize groupBoxSize(QSize size, const QSize &contentsSize)
{
    size.setWidth(qMax(size.width(), contentsSize.width() + 24));
    size.setHeight(qMax(size.height(), contentsSize.height() + 48));
    return size;
}

QSize sizeFromContents(const Style *style, QStyle::ContentsType type, const QStyleOption *option,
                       const QSize &contentsSize, const QWidget *widget)
{
    const DensityMode mode = densityModeFor(widget, style);
    const DensityMetrics &density = densityMetrics(mode);
    QSize size = contentsSize;
    switch (type) {
    case QStyle::CT_PushButton:
        size = pushButtonSize(option, size, widget, density);
        break;
    case QStyle::CT_ComboBox:
        size = comboBoxSize(option, size, widget, density, mode);
        break;
    case QStyle::CT_LineEdit:
        size = lineEditSize(option, size, density, mode);
        break;
    case QStyle::CT_SpinBox:
        size = spinBoxSize(option, size, widget, density, mode);
        break;
    case QStyle::CT_ToolButton:
        if (widget && widget->property(dockTitleButtonProperty).toBool())
            return QSize(DockTitleButtonExtent, DockTitleButtonExtent);
        size = toolButtonSize(option, size, widget, density, mode);
        break;
    case QStyle::CT_MenuBarItem:
        size = menuBarItemSize(size, density, contentsSize);
        break;
    case QStyle::CT_TabBarTab:
        size = tabBarTabSize(option, size, density);
        break;
    case QStyle::CT_MenuItem:
        size = menuItemSize(option, size, widget, density);
        break;
    case QStyle::CT_ItemViewItem:
        size = itemViewItemSize(style, size, option, widget, density, mode);
        break;
    case QStyle::CT_HeaderSection:
        size = headerSectionSize(style, size, option, widget, density, mode);
        break;
    case QStyle::CT_CheckBox:
        size = checkBoxSize(option, size, widget, density, mode);
        break;
    case QStyle::CT_RadioButton:
        size = radioButtonSize(size, density, mode);
        break;
    case QStyle::CT_ProgressBar:
        size = progressBarSize(option, size, widget);
        break;
    case QStyle::CT_Slider:
        size = sliderSize(option, size, density);
        break;
    case QStyle::CT_ScrollBar:
        size = scrollBarSize(size);
        break;
    case QStyle::CT_GroupBox:
        size = groupBoxSize(size, contentsSize);
        break;
    default:
        return style->QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }
    return size;
}

QRect subElementRect(const Style *style, QStyle::SubElement element, const QStyleOption *option,
                     const QWidget *widget)
{
    if (!option)
        return {};
    const DensityMode mode = densityModeFor(widget, style);
    const DensityMetrics &density = densityMetrics(mode);
    if (element == QStyle::SE_PushButtonContents) {
        // WinUI Button_themeresources.xaml pins ButtonPadding to 11,5,11,6;
        // the ContentPresenter centres the label above the geometric centre.
        // Translate the established Qt content box by that pinned asymmetry
        // instead of inventing a new inset pair (issue #119).
        const QRect logical = option->rect.adjusted(8, 4, -8, -4);
        return logical.translated(0, -WinuiVerticalContentCenterOffset);
    }
    if (element == QStyle::SE_ToolButtonLayoutItem)
        // QToolBar interprets this sub-element as external layout margins,
        // not content padding. Insets here enlarge adjacent QToolButtons into
        // one another; the later sibling then clips the earlier button's
        // rounded trailing corners. Content padding is already handled by
        // CT_ToolButton and CE_ToolButtonLabel.
        return option->rect;
    if (element == QStyle::SE_CheckBoxIndicator || element == QStyle::SE_RadioButtonIndicator) {
        const QRect logical(option->rect.left() + 4, option->rect.center().y() - 10, 20, 20);
        return QStyle::visualRect(option->direction, option->rect, logical);
    }
    if (element == QStyle::SE_CheckBoxContents || element == QStyle::SE_RadioButtonContents) {
        const QRect logical = option->rect.adjusted(32, 0, -4, 0);
        return QStyle::visualRect(option->direction, option->rect, logical);
    }
    if (element == QStyle::SE_CheckBoxClickRect || element == QStyle::SE_RadioButtonClickRect) {
        const QRect indicator = subElementRect(style,
                                               element == QStyle::SE_CheckBoxClickRect
                                                       ? QStyle::SE_CheckBoxIndicator
                                                       : QStyle::SE_RadioButtonIndicator,
                                               option, widget);
        const QRect contents = subElementRect(style,
                                              element == QStyle::SE_CheckBoxClickRect
                                                      ? QStyle::SE_CheckBoxContents
                                                      : QStyle::SE_RadioButtonContents,
                                              option, widget);
        return option->rect.united(indicator).united(contents);
    }
    if (element == QStyle::SE_LineEditContents && keySequenceEditor(widget)) {
        // The shortcut recorder's editor paints keycaps in the TextBox panel.
        // An empty text slot stops QLineEdit's line control from drawing the
        // raw sequence (and caret/selection) over them.
        return QRect();
    }
    if (element == QStyle::SE_LineEditContents) {
        // QComboBox has already positioned its private editor in the exact
        // SC_ComboBoxEditField slot. Applying the standalone TextBox's 10px
        // inset again shifts editable text relative to the normal label.
        if (comboBoxEditor(widget))
            return option->rect;
        if (!spinBoxEditor(widget)) {
            const bool compact = mode == DensityMode::Compact;
            const QRect logical = compact ? option->rect.adjusted(10, 2, -6, -2)
                                          : option->rect.adjusted(10, 5, -6, -6);
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    if (element == QStyle::SE_DockWidgetCloseButton || element == QStyle::SE_DockWidgetFloatButton
        || element == QStyle::SE_DockWidgetTitleBarText) {
        const auto *dock = qstyleoption_cast<const QStyleOptionDockWidget *>(option);
        if (!dock)
            return {};
        const bool vertical = dock->verticalTitleBar;
        const int length = vertical ? dock->rect.height() : dock->rect.width();
        const int thickness = vertical ? dock->rect.width() : dock->rect.height();
        const int slot = DockTitleButtonExtent;
        const int count = int(dock->closable) + int(dock->floatable);
        const int stack = count ? count * slot + (count - 1) * DockTitleButtonGap : 0;
        QRect logical;
        if (element == QStyle::SE_DockWidgetTitleBarText) {
            logical = QRect(
                    DockTitleInset, 0,
                    qMax(0, length - 2 * DockTitleInset - stack - (count ? DockTitleButtonGap : 0)),
                    thickness);
        } else {
            if ((element == QStyle::SE_DockWidgetCloseButton && !dock->closable)
                || (element == QStyle::SE_DockWidgetFloatButton && !dock->floatable))
                return {};
            const int offset = element == QStyle::SE_DockWidgetFloatButton && dock->closable
                    ? slot + DockTitleButtonGap
                    : 0;
            logical = QRect(length - DockTitleInset - slot - offset, (thickness - slot) / 2, slot,
                            slot);
        }
        if (vertical) {
            // The title reads bottom-up; its trailing edge (buttons) is top.
            return QRect(dock->rect.left() + logical.y(),
                         dock->rect.top() + length - logical.x() - logical.width(),
                         logical.height(), logical.width());
        }
        logical.translate(dock->rect.topLeft());
        return QStyle::visualRect(dock->direction, dock->rect, logical);
    }
    QRect result = style->QProxyStyle::subElementRect(element, option, widget);
    // WinUI docks the tab close button against the right edge of the tab;
    // Qt leaves an 8px gap. Slide the slot closer to the edge.
    if (element == QStyle::SE_TabBarTabRightButton) {
        const int shift = qMin(8, option->rect.right() - result.right());
        if (shift > 0)
            result.translate(shift, 0);
    }
    if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        if (element == QStyle::SE_ItemViewItemText
            && qobject_cast<const QTableView *>(itemView(widget))
            && !calendarPopupItemView(widget)) {
            // QTableView is a consistency extension (Qt has no core WinUI
            // DataGrid), so its text inset comes from the pinned DataGrid
            // text column: DATAGRIDTEXTCOLUMN_leftMargin / _rightMargin are
            // both 12.0 and the generated TextBlock gets exactly that margin
            // (DataGridTextColumn.cs), which is also the column header's
            // Padding="12,0,0,0" leading inset. A header and its cells must
            // share the leading edge.
            if (source->direction == Qt::RightToLeft) {
                result.setLeft(
                        qMax(result.left(), source->rect.left() + tableCellHorizontalPadding));
                result.setRight(
                        qMin(result.right(), source->rect.right() - tableCellHorizontalPadding));
            } else {
                result.setLeft(
                        qMax(result.left(), source->rect.left() + tableCellHorizontalPadding));
                result.setRight(
                        qMin(result.right(), source->rect.right() - tableCellHorizontalPadding));
            }
        }
        const QAbstractItemView *view = selectionMarkerView(widget);
        if (view
            && (element == QStyle::SE_ItemViewItemCheckIndicator
                || element == QStyle::SE_ItemViewItemDecoration
                || element == QStyle::SE_ItemViewItemText)) {
            // The delegate's option.rect already includes QTreeView's
            // hierarchy offset. Adding the model depth here doubled the
            // indentation at every level.
            const int offset = density.itemSelectionGutter;
            const int delta = source->direction == Qt::RightToLeft ? -offset : offset;
            result.translate(delta, 0);
            QRect content = source->rect;
            if (source->direction == Qt::RightToLeft)
                content.setRight(qMax(content.left() - 1, content.right() - offset));
            else
                content.setLeft(qMin(content.right() + 1, content.left() + offset));
            result = result.intersected(content);
        }
        if (element == QStyle::SE_ItemViewItemText
            && qobject_cast<const QTreeView *>(itemView(widget))
            && (source->features & QStyleOptionViewItem::HasDecoration) && !source->icon.isNull()) {
            // WinUI's TreeView item template puts a 10 px spacer between the
            // 20 px icon and the label (TreeViewTemplateSelectorSample:
            // TextBlock Margin="0,0,10,0"). QCommonStyle's default leaves only
            // ~6 px, which crowds the icon (issue #55). Anchor the text to the
            // decoration's trailing edge plus that spacer.
            const QRect decoration =
                    style->subElementRect(QStyle::SE_ItemViewItemDecoration, option, widget);
            constexpr int treeIconTextGap = 10;
            if (source->direction == Qt::RightToLeft)
                result.setRight(qMin(result.right(), decoration.left() - 1 - treeIconTextGap));
            else
                result.setLeft(qMax(result.left(), decoration.right() + 1 + treeIconTextGap));
        }
    }
    const bool popup = widget && widget->window() && widget->window()->windowType() == Qt::Popup
            && !calendarPopupItemView(widget);
    if (popup && element == QStyle::SE_ItemViewItemText) {
        // Reserve the icon-slot inset on the leading edge in the reading
        // direction, mirroring the reservation for RTL layouts.
        const QStyleOptionViewItem *item = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        const Qt::LayoutDirection direction = item ? item->direction : option->direction;
        const int iconSlot = density.menuItemIconSlot;
        const int padding = density.menuItemHorizontalPadding;
        if (direction == Qt::RightToLeft) {
            result.setRight(qMin(result.right(), option->rect.right() - iconSlot));
            result.setLeft(qMax(result.left(), option->rect.left() + padding));
        } else {
            result.setLeft(qMax(result.left(), option->rect.left() + iconSlot));
            result.setRight(qMin(result.right(), option->rect.right() - padding));
        }
    }
    return result;
}

QRect subControlRect(const Style *style, QStyle::ComplexControl control,
                     const QStyleOptionComplex *option, QStyle::SubControl subControl,
                     const QWidget *widget)
{
    // Null-option entry guard (mirrors complexControlHitTest): a null
    // option yields an empty rect instead of falling through into
    // QProxyStyle with a null dereference.
    if (!option)
        return QRect();
    if (const auto rect = complexControlRect(control, option, subControl, widget))
        return *rect;
    return style->QProxyStyle::subControlRect(control, option, subControl, widget);
}

int styleHint(const Style *style, QStyle::StyleHint hint, const QStyleOption *option,
              const QWidget *widget, QStyleHintReturn *returnData)
{
    switch (hint) {
    case QStyle::SH_DockWidget_ButtonsHaveFrame:
        // Dock title buttons are bare glyphs on the title card. On 0,
        // QDockWidgetTitleButton skips its own PE_PanelButtonTool, so the
        // stock Qt tool-button frame is never drawn around them (Qt would
        // otherwise paint that frame under our own CC_ToolButton panel).
        // QCommonStyle's default of 1 framed them as stock tool buttons.
        return 0;
    case QStyle::SH_Widget_Animate:
        return Style::animationsAllowed() ? 1 : 0;
    case QStyle::SH_ScrollBar_Transient:
        return 1;
    case QStyle::SH_ComboBox_Popup:
        return 1;
    case QStyle::SH_ComboBox_PopupFrameStyle:
        return QFrame::NoFrame;
    case QStyle::SH_WizardStyle:
        // QWizard's ClassicStyle draws a blue banner + #003399 Segoe UI
        // title; AeroStyle forces white page/palette. ModernStyle gives a
        // flat header that takes the style palette (live defect 2026-09-09:
        // unreadable blue title, blue header, white footer in Dark).
        return int(QWizard::ModernStyle);
    case QStyle::SH_ComboBox_ListMouseTracking: // == _Current
    case QStyle::SH_MenuBar_MouseTracking:
    case QStyle::SH_Menu_MouseTracking:
        return 1;
    case QStyle::SH_Menu_SubMenuPopupDelay:
        return 400;
    case QStyle::SH_MessageBox_TextInteractionFlags:
        return int(Qt::LinksAccessibleByMouse);
    case QStyle::SH_Slider_AbsoluteSetButtons:
        return Qt::LeftButton;
    case QStyle::SH_ToolButtonStyle:
        return Qt::ToolButtonFollowStyle;
    case QStyle::SH_UnderlineShortcut:
        // Qt-idiomatic Alt-reveal (QWindowsStyle: SPI_GETKEYBOARDCUES plus an
        // Alt event filter; underlines hidden by default on Windows). The Alt
        // press is tracked in Style::eventFilter (every polished widget is
        // filtered); ask the live style instance, never cache here.
        return Style::altMnemonicsVisible() ? 1 : 0;
    default:
        return style->QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

QIcon standardIcon(const Style *style, QStyle::StandardPixmap standard, const QStyleOption *option,
                   const QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    // Issue #69: only exact Fluent counterparts are paired here. A lookalike
    // is not a pairing: folder vs open folder, back vs forward, skip vs seek
    // and muted vs unmuted must remain distinguishable glyphs. Anything
    // without an exact counterpart stays on the base style.
    switch (standard) {
    case QStyle::SP_ArrowBack:
        return WinUI3::icon(Icon::ChevronLeft);
    case QStyle::SP_ArrowDown:
        return WinUI3::icon(Icon::ChevronDown);
    case QStyle::SP_ArrowLeft:
        return WinUI3::icon(Icon::ChevronLeft);
    case QStyle::SP_ArrowRight:
        return WinUI3::icon(Icon::ChevronRight);
    case QStyle::SP_ArrowForward:
    case QStyle::SP_CommandLink:
        return WinUI3::icon(Icon::ChevronRight);
    case QStyle::SP_ArrowUp:
        return WinUI3::icon(Icon::ChevronUp);
    case QStyle::SP_BrowserReload:
        return WinUI3::icon(Icon::Refresh);
    case QStyle::SP_DialogApplyButton:
        return WinUI3::icon(Icon::Check);
    case QStyle::SP_DialogCancelButton:
    case QStyle::SP_DockWidgetCloseButton:
        return WinUI3::icon(Icon::Close);
    case QStyle::SP_DialogCloseButton:
    case QStyle::SP_DialogNoButton:
    case QStyle::SP_LineEditClearButton:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QStyle::SP_TabCloseButton:
#endif
        // These four X-mark slots share the same Clear (E894) glyph: the
        // Windows reference draws the same outlined cross for each of them.
        return WinUI3::icon(Icon::Clear);
    case QStyle::SP_TitleBarCloseButton:
        // Title-bar close uses Fluent's ChromeClose, not the dialog Cancel
        // mark: the two sit in different chrome families.
        return WinUI3::icon(Icon::ChromeClose);
    case QStyle::SP_DialogSaveButton:
        return WinUI3::icon(Icon::Save);
    case QStyle::SP_DialogYesButton:
        return WinUI3::icon(Icon::Accept);
    case QStyle::SP_DialogOkButton:
        return WinUI3::icon(Icon::ReturnKey);
    case QStyle::SP_DialogDiscardButton:
        // Fluent Delete (E74D, trash) is the exact counterpart of Qt's Discard
        // button: the Windows reference draws the same trash mark. It shares
        // the glyph with SP_TrashIcon for that reason, not as a lookalike.
        return WinUI3::icon(Icon::Delete);
    case QStyle::SP_DirIcon:
    case QStyle::SP_DirClosedIcon:
        return WinUI3::icon(Icon::Folder);
    case QStyle::SP_DirOpenIcon:
    case QStyle::SP_DialogOpenButton:
        // FolderOpen (E838) is a distinct glyph; a closed and an open folder
        // must not be told apart only by a tooltip.
        return WinUI3::icon(Icon::FolderOpen);
    case QStyle::SP_DirHomeIcon:
        return WinUI3::icon(Icon::Home);
    case QStyle::SP_FileIcon:
        return WinUI3::icon(Icon::Document);
    case QStyle::SP_FileLinkIcon:
        // OpenFile (E8E5) is the file-with-arrow mark the Windows reference
        // draws here, distinct from the plain Document glyph of SP_FileIcon.
        return WinUI3::icon(Icon::OpenFile);
    case QStyle::SP_FileDialogContentsView:
    case QStyle::SP_FileDialogDetailedView:
        return WinUI3::icon(Icon::List);
    case QStyle::SP_FileDialogNewFolder:
        return WinUI3::icon(Icon::NewFolder);
    case QStyle::SP_MediaPause:
        return WinUI3::icon(Icon::Pause);
    case QStyle::SP_MediaPlay:
        return WinUI3::icon(Icon::Play);
    case QStyle::SP_MediaStop:
        return WinUI3::icon(Icon::Stop);
    case QStyle::SP_BrowserStop:
        return WinUI3::icon(Icon::ErrorBadge);
    case QStyle::SP_MediaSkipBackward:
        return WinUI3::icon(Icon::Previous);
    case QStyle::SP_MediaSkipForward:
        return WinUI3::icon(Icon::Next);
    case QStyle::SP_MediaSeekBackward:
        return WinUI3::icon(Icon::Rewind);
    case QStyle::SP_MediaSeekForward:
        return WinUI3::icon(Icon::FastForward);
    case QStyle::SP_MediaVolume:
        return WinUI3::icon(Icon::Volume);
    case QStyle::SP_MediaVolumeMuted:
        return WinUI3::icon(Icon::Mute);
    case QStyle::SP_MessageBoxCritical:
        return WinUI3::icon(Icon::ErrorBadge);
    case QStyle::SP_MessageBoxInformation:
        return WinUI3::icon(Icon::Info);
    case QStyle::SP_MessageBoxQuestion:
        // Unknown (E9CE) is the circled question mark the Windows reference
        // draws here. Help (E897) is a bare ? and stays on title-bar context
        // help, whose reference is bare too.
        return WinUI3::icon(Icon::Unknown);
    case QStyle::SP_MessageBoxWarning:
        return WinUI3::icon(Icon::Warning);
    case QStyle::SP_TitleBarMinButton:
        return WinUI3::icon(Icon::Minimize);
    case QStyle::SP_TitleBarMaxButton:
        return WinUI3::icon(Icon::Maximize);
    case QStyle::SP_TitleBarShadeButton:
        // Fluent CaretSolidUp (F090) is the solid up-caret of the Windows
        // title-bar shade control, matching the Qt reference raster.
        return WinUI3::icon(Icon::CaretSolidUp);
    case QStyle::SP_TitleBarUnshadeButton:
        // The matched down-caret of the pair: CaretSolidDown (F08E), the
        // Windows unshade control, distinct from the up-caret above.
        return WinUI3::icon(Icon::CaretSolidDown);
    case QStyle::SP_TitleBarMenuButton:
        return WinUI3::icon(Icon::ButtonMenu);
    case QStyle::SP_ToolBarHorizontalExtensionButton:
    case QStyle::SP_ToolBarVerticalExtensionButton:
        return WinUI3::icon(Icon::More);
    case QStyle::SP_VistaShield:
        return WinUI3::icon(Icon::Shield);
    case QStyle::SP_DesktopIcon:
        return WinUI3::icon(Icon::Monitor);
    case QStyle::SP_ComputerIcon:
        // TVMonitor (E7F4) is the monitor-with-stand the Windows reference
        // draws here; DeviceMonitorNoPic (E7FB) is the flatter Desktop mark.
        return WinUI3::icon(Icon::TVMonitor);
    case QStyle::SP_DriveCDIcon:
    case QStyle::SP_DriveDVDIcon:
        return WinUI3::icon(Icon::StorageOptical);
    case QStyle::SP_DriveHDIcon:
        return WinUI3::icon(Icon::HardDrive);
    case QStyle::SP_DriveNetIcon:
        // Fluent MapDrive (E8CE) is the mapped/network-drive mark: a drive with
        // a network connector. It is distinct from the local HardDrive EDA2,
        // matching the distinct Windows-reference raster for this slot.
        return WinUI3::icon(Icon::MapDrive);
    case QStyle::SP_DriveFDIcon:
        // E74E is the Fluent floppy — Qt's Windows style uses the same mark
        // for the removable-disk slot.
        return WinUI3::icon(Icon::Save);
    case QStyle::SP_TrashIcon:
        return WinUI3::icon(Icon::Delete);
    case QStyle::SP_DialogHelpButton:
        return WinUI3::icon(Icon::GetHelpApp);
    case QStyle::SP_TitleBarContextHelpButton:
        return WinUI3::icon(Icon::Help);
    case QStyle::SP_FileDialogBack:
        return WinUI3::icon(Icon::ChevronLeft);
    case QStyle::SP_FileDialogToParent:
        return WinUI3::icon(Icon::ChevronUp);
    case QStyle::SP_FileDialogInfoView:
        return WinUI3::icon(Icon::Info);
    case QStyle::SP_FileDialogListView:
        // The Windows reference draws a four-tile grid for this slot, not the
        // List mark used by ContentsView/DetailedView. GridView F0E2 is the
        // exact counterpart, so this slot is paired by artwork, not by name.
        return WinUI3::icon(Icon::GridView);
    case QStyle::SP_TitleBarNormalButton:
        // Qt's dock float button and every other "restore" slot: the base
        // Windows style hands back a two-window mark drawn with 2 px strokes and
        // filled faces, which reads as a heavy blob beside the 1 px close cross.
        // Fluent's ChromeRestore keeps the same two-window shape at the same
        // thin stroke as the rest of the title row.
        return WinUI3::icon(Icon::Restore);
    default:
        return style->QProxyStyle::standardIcon(standard, option, widget);
    }
}

} // namespace WinUI3::Private
