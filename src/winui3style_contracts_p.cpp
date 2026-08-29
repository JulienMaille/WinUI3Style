#include "winui3style_contracts_p.h"

#include "winui3geometry_p.h"
#include "winui3style_properties_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemView>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QFontMetrics>
#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QTableView>
#include <QTabBar>
#include <QStyleOptionButton>
#include <QStyleOptionMenuItem>
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

constexpr int itemSelectionGutter = 12;

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
    if (const auto value = pixelMetricValue(metric, toggleSwitch(widget)))
        return *value;
    return style->QProxyStyle::pixelMetric(metric, option, widget);
}

QSize sizeFromContents(const Style *style, QStyle::ContentsType type,
                       const QStyleOption *option, const QSize &contentsSize,
                       const QWidget *widget)
{
    QSize size = contentsSize;
    switch (type) {
    case QStyle::CT_PushButton:
        size += QSize(24, 12);
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
            button && (button->features & QStyleOptionButton::HasMenu))
            size.rwidth() += 22;
        size.setWidth(qMax(size.width(), 32));
        size.setHeight(qMax(size.height(), 32));
        break;
    case QStyle::CT_ComboBox:
        size += QSize(50, 12);
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 32));
        break;
    case QStyle::CT_LineEdit:
        size += QSize(16, 12);
        size.setHeight(qMax(size.height(), 32));
        break;
    case QStyle::CT_SpinBox:
        size += QSize(verticalSpinButtons(widget) ? 44 : 84, 0);
        size.setHeight(qMax(size.height(), 32));
        size.setWidth(qMax(size.width(), 120));
        break;
    case QStyle::CT_ToolButton:
        if (textBoxHelperButton(widget))
            return QSize(30, 32);
        if (widget && qobject_cast<const QTabBar *>(widget->parentWidget()))
            return QSize(32, 24);
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
                size.rwidth() += 24;
        }
        if (size.isEmpty())
            size = QSize(20, 20);
        size += QSize(12, 12);
        size.setHeight(qMax(size.height(), 32));
        size.setWidth(qMax(size.width(), 32));
        break;
    case QStyle::CT_MenuBarItem:
        size = contentsSize + QSize(24, 12);
        size.setHeight(qMax(size.height(), 32));
        break;
    case QStyle::CT_TabBarTab:
        size += QSize(16, 8);
        size.setHeight(32);
        size.setWidth(qBound(100, size.width(), 240));
        break;
    case QStyle::CT_MenuItem:
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                size.setHeight(7);
                break;
            }
            const QStringList parts = menu->text.split(QLatin1Char('\t'));
            const QFontMetrics metrics(menu->font);
            const int mainWidth = metrics.horizontalAdvance(parts.value(0));
            const int shortcutWidth = parts.size() > 1
                ? metrics.horizontalAdvance(parts.value(1)) : 0;
            const int trailing = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? 24 : 0;
            const bool comboItem = qobject_cast<const QComboBox *>(widget);
            const int leading = comboItem && menu->icon.isNull() ? 16 : 42;
            const int requiredWidth = leading + mainWidth + 16 + trailing
                + (shortcutWidth > 0 ? 20 + shortcutWidth : 0);
            size.setWidth(qMax(size.width(), qMax(requiredWidth, 120)));
            size.setHeight(comboItem ? 40 : 36);
        }
        break;
    case QStyle::CT_ItemViewItem:
        if (widget && widget->window()
            && widget->window()->windowType() == Qt::Popup) {
            size.setHeight(40);
        } else if (qobject_cast<const QTreeView *>(itemView(widget))) {
            size.setHeight(28);
        } else if (qobject_cast<const QTableView *>(itemView(widget))) {
            size.setHeight(36);
        } else {
            size.setHeight(40);
        }
        break;
    case QStyle::CT_HeaderSection:
        size += QSize(24, 8);
        size.setHeight(qMax(size.height(), 32));
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
            size = QSize(labelWidth > 0 ? qMax(154, 50 + labelWidth) : 40, 40);
            break;
        }
        size += QSize(32, 8);
        size.setHeight(qMax(size.height(), 28));
        break;
    case QStyle::CT_RadioButton:
        size += QSize(32, 8);
        size.setHeight(qMax(size.height(), 28));
        break;
    case QStyle::CT_ProgressBar:
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 20));
        break;
    case QStyle::CT_Slider:
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 32));
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
    if (element == QStyle::SE_LineEditContents && !spinBoxEditor(widget)) {
        const QRect logical = option->rect.adjusted(10, 5, -6, -6);
        return QStyle::visualRect(option->direction, option->rect, logical);
    }
    QRect result = style->QProxyStyle::subElementRect(element, option, widget);
    if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        const QAbstractItemView *view = selectionMarkerView(widget);
        if (view && (element == QStyle::SE_ItemViewItemCheckIndicator
                     || element == QStyle::SE_ItemViewItemDecoration
                     || element == QStyle::SE_ItemViewItemText)) {
            const int offset = treeItemIndent(*source, view) + itemSelectionGutter;
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
        && widget->window()->windowType() == Qt::Popup;
    if (popup && element == QStyle::SE_ItemViewItemText) {
        result.setLeft(qMax(result.left(), option->rect.left() + 42));
        result.setRight(qMin(result.right(), option->rect.right() - 12));
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
