#include "winui3style_contracts_p.h"

#include "winui3geometry_p.h"
#include "winui3density_p.h"
#include "winui3style_properties_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemView>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFontMetrics>
#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QProgressBar>
#include <QTableView>
#include <QTabBar>
#include <QStyleOptionButton>
#include <QStyleOptionMenuItem>
#include <QStyleOptionProgressBar>
#include <QStyleOptionToolButton>
#include <QStyleOptionViewItem>
#include <QTreeView>
#include <QToolButton>

namespace WinUI3::Private {
namespace {

bool toggleSwitch(const QWidget *widget)
{
    return qobject_cast<const QCheckBox *>(widget)
        && widget->property(Style::ToggleSwitchProperty).toBool();
}

bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget)
        && widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
        && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

bool comboBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
        && qobject_cast<const QComboBox *>(widget->parentWidget());
}

bool textBoxHelperButton(const QWidget *widget)
{
    return qobject_cast<const QAbstractButton *>(widget)
        && qobject_cast<const QLineEdit *>(widget->parentWidget());
}

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

bool comboPopupItemView(const QWidget *widget)
{
    const QAbstractItemView *view = itemView(widget);
    if (!view || !view->window()
        || view->window()->windowType() != Qt::Popup)
        return false;
    return qobject_cast<const QComboBox *>(view->window()->parentWidget());
}

bool calendarPopupItemView(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
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
    if (!qobject_cast<const QListView *>(view)
        && !qobject_cast<const QTreeView *>(view))
        return nullptr;
    return view;
}

int treeItemIndent(const QStyleOptionViewItem &option,
                   const QAbstractItemView *view)
{
    const auto *tree = qobject_cast<const QTreeView *>(view);
    if (!tree || !option.index.isValid() || option.index.column() != 0)
        return 0;

    int depth = 0;
    for (QModelIndex parent = option.index.parent(); parent.isValid();
         parent = parent.parent()) {
        ++depth;
    }
    if (tree->rootIsDecorated())
        ++depth;
    return depth * tree->indentation();
}

} // namespace

int pixelMetric(const Style *style, QStyle::PixelMetric metric,
                const QStyleOption *option, const QWidget *widget)
{
    if (const auto value = pixelMetricValue(metric, toggleSwitch(widget), widget))
        return *value;
    return style->QProxyStyle::pixelMetric(metric, option, widget);
}

QSize sizeFromContents(const Style *style, QStyle::ContentsType type,
                       const QStyleOption *option, const QSize &contentsSize,
                       const QWidget *widget)
{
    const DensityMetrics &density = densityMetricsFor(widget, style);
    QSize size = contentsSize;
    switch (type) {
    case QStyle::CT_PushButton:
        size += QSize(2 * density.buttonHorizontalPadding,
                       2 * density.buttonVerticalPadding);
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
            button && (button->features & QStyleOptionButton::HasMenu))
            size.rwidth() += 22;
        size.setWidth(qMax(size.width(), 32));
        size.setHeight(qMax(size.height(), density.buttonHeight));
        break;
    case QStyle::CT_ComboBox:
        if (densityModeFor(widget) == DensityMode::Compact) {
            size.rwidth() += 2 * density.comboHorizontalPadding;
            // QComboBox may pass a contents height cached under the previous
            // profile. Recompute Compact from the current font so that stale
            // Standard contents cannot pin it at the old height.
            size.setHeight(qMax(density.comboBoxHeight,
                                option ? option->fontMetrics.height()
                                             + 2 * density.comboVerticalPadding
                                       : contentsSize.height()));
        } else {
            // Preserve the established Standard geometry pixel-for-pixel.
            size += QSize(2 * density.comboHorizontalPadding,
                          2 * density.comboVerticalPadding);
            size.setHeight(qMax(size.height(), density.comboBoxHeight));
        }
        size.setWidth(qMax(size.width(), 120));
        break;
    case QStyle::CT_LineEdit:
        size += QSize(2 * density.lineEditHorizontalPadding,
                       2 * density.lineEditVerticalPadding);
        size.setHeight(qMax(size.height(), density.textBoxHeight));
        break;
    case QStyle::CT_SpinBox:
        size += QSize(verticalSpinButtons(widget)
                          ? density.verticalSpinButtonWidth + 12
                          : 2 * density.spinButtonWidth + 12,
                      0);
        size.setHeight(qMax(size.height(),
                            qobject_cast<const QDateTimeEdit *>(widget)
                                ? density.textBoxHeight
                                : density.buttonHeight));
        size.setWidth(qMax(size.width(), 120));
        break;
    case QStyle::CT_ToolButton:
        if (textBoxHelperButton(widget)) {
            // QLineEditIconButton is a private QToolButton.  Its geometry is
            // used by QLineEdit to reserve the DeleteButton slot, so leaving
            // the historical 30x32 helper size in Compact mode makes a
            // 24px TextBox grow back to the Standard height.  Keep the
            // established Standard slot byte-for-byte and compact both axes
            // with the editor template.
            if (densityModeFor(widget) == DensityMode::Compact)
                return QSize(density.textBoxHeight, density.textBoxHeight);
            return QSize(30, density.toolButtonHeight);
        }
        if (widget && qobject_cast<const QTabBar *>(widget->parentWidget()))
            return QSize(density.tabCloseWidth, density.tabCloseHeight);
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const QFontMetrics metrics(tool->fontMetrics);
            const int textWidth = metrics.horizontalAdvance(tool->text);
            const int iconWidth = tool->icon.isNull() ? 0
                : (tool->iconSize.isValid() ? tool->iconSize.width() : 16);
            if (const auto *toolButton = qobject_cast<const QToolButton *>(widget)) {
                if (toolButton->toolButtonStyle() == Qt::ToolButtonTextBesideIcon
                    && textWidth > 0 && iconWidth > 0)
                    size.setWidth(qMax(size.width(), textWidth + iconWidth + 22));
                else if (toolButton->toolButtonStyle() == Qt::ToolButtonTextOnly)
                    size.setWidth(qMax(size.width(), textWidth + 16));
            }
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup)
                size.rwidth() += density.toolButtonMenuWidth;
        }
        if (size.isEmpty())
            size = QSize(20, 20);
        size += QSize(2 * density.toolButtonHorizontalPadding,
                      2 * density.toolButtonVerticalPadding);
        size.setHeight(qMax(size.height(), density.toolButtonHeight));
        size.setWidth(qMax(size.width(), density.toolButtonHeight));
        break;
    case QStyle::CT_MenuBarItem:
        size = contentsSize + QSize(2 * density.menuBarHorizontalPadding,
                                    2 * density.menuBarVerticalPadding);
        size.setHeight(qMax(size.height(), density.menuBarItemHeight));
        break;
    case QStyle::CT_TabBarTab:
        size += QSize(2 * density.tabHorizontalPadding,
                      2 * density.tabVerticalPadding);
        size.setHeight(density.tabHeight);
        size.setWidth(qBound(100, size.width(), 240));
        break;
    case QStyle::CT_MenuItem:
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                size.setHeight(density.menuSeparatorHeight);
                break;
            }
            const QStringList parts = menu->text.split(QLatin1Char('\t'));
            const QFontMetrics metrics(menu->font);
            const int mainWidth = metrics.horizontalAdvance(parts.value(0));
            const int shortcutWidth = parts.size() > 1
                ? metrics.horizontalAdvance(parts.value(1)) : 0;
            const int trailing = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? density.toolButtonMenuWidth : 0;
            const bool comboItem = qobject_cast<const QComboBox *>(widget);
            const int leading = comboItem && menu->icon.isNull()
                ? density.menuItemNoIconSlot : density.menuItemIconSlot;
            const int requiredWidth = leading + mainWidth
                + 2 * density.menuItemHorizontalPadding + trailing
                + (shortcutWidth > 0 ? density.menuItemShortcutGap + shortcutWidth : 0);
            size.setWidth(qMax(size.width(), qMax(requiredWidth, 120)));
            size.setHeight(comboItem ? density.menuItemHeightInComboBox
                                     : density.menuItemHeight);
        }
        break;
    case QStyle::CT_ItemViewItem:
        if (comboPopupItemView(widget)) {
            // ComboBoxItem has its own resource.  Keep it separate from the
            // ListView row metric even while both profiles currently resolve
            // to the same 40/32 values; the templates are independently
            // configurable in WinUI and must not silently drift together.
            size.setHeight(density.comboPopupItemHeight);
        } else if (widget && widget->window()
                   && widget->window()->windowType() == Qt::Popup) {
            size.setHeight(density.listItemHeight);
        } else if (qobject_cast<const QTreeView *>(itemView(widget))) {
            size.setHeight(density.treeItemHeight);
        } else if (qobject_cast<const QTableView *>(itemView(widget))) {
            size.setHeight(density.tableItemHeight);
        } else {
            size.setHeight(density.listItemHeight);
        }
        break;
    case QStyle::CT_HeaderSection:
        size += QSize(2 * density.headerHorizontalPadding,
                      2 * density.headerVerticalPadding);
        size.setHeight(qMax(size.height(), density.headerHeight));
        break;
    case QStyle::CT_CheckBox:
        if (toggleSwitch(widget)) {
            const auto *check = qstyleoption_cast<const QStyleOptionButton *>(option);
            QString onText = widget->property(Style::ToggleSwitchOnTextProperty).toString();
            QString offText = widget->property(Style::ToggleSwitchOffTextProperty).toString();
            if (check && onText.isEmpty() && offText.isEmpty())
                onText = offText = check->text;
            const QFontMetrics metrics = check ? check->fontMetrics
                                               : widget->fontMetrics();
            const int labelWidth = qMax(metrics.horizontalAdvance(onText),
                                        metrics.horizontalAdvance(offText));
            size = QSize(labelWidth > 0 ? qMax(154, 50 + labelWidth)
                                        : density.toggleSlotWidth,
                         density.toggleSlotHeight);
            break;
        }
        // Label is painted from x=32 with a 4px right margin; Qt passes only
        // the text size as contents, so the hint must cover indicator, gap,
        // and trailing margin to avoid clipping the last glyph.
        size += QSize(36, 8);
        size.setHeight(qMax(size.height(), 28));
        break;
    case QStyle::CT_RadioButton:
        size += QSize(36, 8);
        size.setHeight(qMax(size.height(), 28));
        break;
    case QStyle::CT_ProgressBar: {
        // QStyleOptionProgressBar::orientation only exists from Qt 5.13;
        // query the widget for a 5.12-compatible check.
        const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
        const bool horizontal = !progressBar
            || progressBar->orientation() == Qt::Horizontal;
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        const bool showsText = bar && bar->textVisible && !bar->text.isEmpty()
            && horizontal;
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(showsText
            ? qMax(size.height(), size.height() + 6) // text + thin underline
            : qMax(size.height(), 8));
        break;
    }
    case QStyle::CT_Slider:
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), density.buttonHeight));
        break;
    case QStyle::CT_ScrollBar:
        size.setWidth(qMax(size.width(), 12));
        size.setHeight(qMax(size.height(), 12));
        break;
    case QStyle::CT_GroupBox:
        size.setWidth(qMax(size.width(), contentsSize.width() + 24));
        size.setHeight(qMax(size.height(), contentsSize.height() + 48));
        break;
    default:
        return style->QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }
    return size;
}

QRect subElementRect(const Style *style, QStyle::SubElement element,
                     const QStyleOption *option, const QWidget *widget)
{
    const DensityMetrics &density = densityMetricsFor(widget, style);
    if (element == QStyle::SE_PushButtonContents)
        return option->rect.adjusted(8, 4, -8, -4);
    if (element == QStyle::SE_ToolButtonLayoutItem)
        return option->rect.adjusted(4, 2, -4, -2);
    if (element == QStyle::SE_CheckBoxIndicator || element == QStyle::SE_RadioButtonIndicator) {
        const QRect logical(option->rect.left() + 4,
                            option->rect.center().y() - 10, 20, 20);
        return QStyle::visualRect(option->direction, option->rect, logical);
    }
    if (element == QStyle::SE_CheckBoxContents || element == QStyle::SE_RadioButtonContents) {
        const QRect logical = option->rect.adjusted(32, 0, -4, 0);
        return QStyle::visualRect(option->direction, option->rect, logical);
    }
    if (element == QStyle::SE_CheckBoxClickRect || element == QStyle::SE_RadioButtonClickRect) {
        const QRect indicator = subElementRect(
            style, element == QStyle::SE_CheckBoxClickRect ? QStyle::SE_CheckBoxIndicator
                                                            : QStyle::SE_RadioButtonIndicator,
            option, widget);
        const QRect contents = subElementRect(
            style, element == QStyle::SE_CheckBoxClickRect ? QStyle::SE_CheckBoxContents
                                                            : QStyle::SE_RadioButtonContents,
            option, widget);
        return option->rect.united(indicator).united(contents);
    }
    if (element == QStyle::SE_LineEditContents) {
        // QComboBox has already positioned its private editor in the exact
        // SC_ComboBoxEditField slot. Applying the standalone TextBox's 10px
        // inset again shifts editable text relative to the normal label.
        if (comboBoxEditor(widget))
            return option->rect;
        if (!spinBoxEditor(widget)) {
            const QRect logical = option->rect.adjusted(10, 5, -6, -6);
            return QStyle::visualRect(option->direction, option->rect, logical);
        }
    }
    QRect result = style->QProxyStyle::subElementRect(element, option, widget);
    if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        const QAbstractItemView *view = selectionMarkerView(widget);
        if (view && (element == QStyle::SE_ItemViewItemCheckIndicator
                     || element == QStyle::SE_ItemViewItemDecoration
                     || element == QStyle::SE_ItemViewItemText)) {
            const int offset = treeItemIndent(*source, view)
                + density.itemSelectionGutter;
            const int delta = source->direction == Qt::RightToLeft ? -offset : offset;
            result.translate(delta, 0);
            QRect content = source->rect;
            if (source->direction == Qt::RightToLeft)
                content.setRight(qMax(content.left() - 1,
                                      content.right() - offset));
            else
                content.setLeft(qMin(content.right() + 1,
                                     content.left() + offset));
            result = result.intersected(content);
        }
    }
    const bool popup = widget && widget->window()
        && widget->window()->windowType() == Qt::Popup
        && !calendarPopupItemView(widget);
    if (popup && element == QStyle::SE_ItemViewItemText) {
        result.setLeft(qMax(result.left(),
                            option->rect.left() + density.menuItemIconSlot));
        result.setRight(qMin(result.right(),
                             option->rect.right() - density.menuItemHorizontalPadding));
    }
    return result;
}

QRect subControlRect(const Style *style, QStyle::ComplexControl control,
                     const QStyleOptionComplex *option,
                     QStyle::SubControl subControl, const QWidget *widget)
{
    if (const auto rect = complexControlRect(control, option, subControl, widget))
        return *rect;
    return style->QProxyStyle::subControlRect(control, option, subControl, widget);
}

int styleHint(const Style *style, QStyle::StyleHint hint,
              const QStyleOption *option, const QWidget *widget,
              QStyleHintReturn *returnData)
{
    switch (hint) {
    case QStyle::SH_Widget_Animate: return Style::animationsAllowed() ? 1 : 0;
    case QStyle::SH_ScrollBar_Transient: return 1;
    case QStyle::SH_ComboBox_Popup: return 1;
    case QStyle::SH_ComboBox_PopupFrameStyle: return QFrame::NoFrame;
    case QStyle::SH_ComboBox_ListMouseTracking:
    case QStyle::SH_MenuBar_MouseTracking:
    case QStyle::SH_Menu_MouseTracking: return 1;
    case QStyle::SH_Menu_SubMenuPopupDelay: return 400;
    case QStyle::SH_Slider_AbsoluteSetButtons: return Qt::LeftButton;
    case QStyle::SH_ToolButtonStyle: return Qt::ToolButtonFollowStyle;
    default: return style->QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

QIcon standardIcon(const Style *style, QStyle::StandardPixmap standard,
                   const QStyleOption *option, const QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    switch (standard) {
    case QStyle::SP_ArrowBack: return WinUI3::icon(Icon::Back);
    case QStyle::SP_ArrowDown: return WinUI3::icon(Icon::ChevronDown);
    case QStyle::SP_ArrowLeft: return WinUI3::icon(Icon::ChevronLeft);
    case QStyle::SP_ArrowRight: return WinUI3::icon(Icon::ChevronRight);
    case QStyle::SP_ArrowUp: return WinUI3::icon(Icon::ChevronUp);
    case QStyle::SP_BrowserReload: return WinUI3::icon(Icon::Refresh);
    case QStyle::SP_DialogApplyButton: return WinUI3::icon(Icon::Check);
    case QStyle::SP_DialogCancelButton:
    case QStyle::SP_DockWidgetCloseButton:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    case QStyle::SP_TabCloseButton:
#endif
    case QStyle::SP_TitleBarCloseButton: return WinUI3::icon(Icon::Close);
    case QStyle::SP_LineEditClearButton: return WinUI3::icon(Icon::Clear);
    case QStyle::SP_DialogSaveButton:
    case QStyle::SP_DialogYesButton: return WinUI3::icon(Icon::Save);
    case QStyle::SP_DirIcon:
    case QStyle::SP_DirOpenIcon: return WinUI3::icon(Icon::Folder);
    case QStyle::SP_FileDialogNewFolder: return WinUI3::icon(Icon::Add);
    case QStyle::SP_MediaPause: return WinUI3::icon(Icon::Pause);
    case QStyle::SP_MediaPlay: return WinUI3::icon(Icon::Play);
    case QStyle::SP_MediaStop: return WinUI3::icon(Icon::Stop);
    case QStyle::SP_MessageBoxCritical: return WinUI3::icon(Icon::Error);
    case QStyle::SP_MessageBoxInformation: return WinUI3::icon(Icon::Info);
    case QStyle::SP_MessageBoxQuestion: return WinUI3::icon(Icon::Help);
    case QStyle::SP_MessageBoxWarning: return WinUI3::icon(Icon::Warning);
    case QStyle::SP_ToolBarHorizontalExtensionButton:
    case QStyle::SP_ToolBarVerticalExtensionButton: return WinUI3::icon(Icon::More);
    default: return style->QProxyStyle::standardIcon(standard, option, widget);
    }
}

} // namespace WinUI3::Private
