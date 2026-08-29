#include "winui3viewrenderers_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3style_properties_p.h"
#include "winui3surfaces_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionHeader>
#include <QStyleOptionTab>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTabBar>
#include <QTreeView>
#include <QVariant>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0)
{
    return framePropertyRegistry().real(widget, name, fallback);
}

bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
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
constexpr int itemSelectionMarkerWidth = 3;
constexpr int itemSelectionMarkerInset = 2;
constexpr int comboPopupItemPaddingLeft = 11;
constexpr int comboPopupItemPaddingTop = 5;
constexpr int comboPopupItemPaddingRight = 11;
constexpr int comboPopupItemPaddingBottom = 7;

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

QRect selectionMarkerRect(const QStyleOptionViewItem &option,
                          const QAbstractItemView *view)
{
    if (!view || !view->viewport())
        return {};

    // The TreeView marker belongs to the row's viewport, not to the tree
    // content slot. Keeping it in the leading viewport gutter prevents the
    // hierarchy indentation from moving the selection affordance.
    const QRect viewport = view->viewport()->rect();
    const int y = option.rect.center().y() - 8;
    if (option.direction == Qt::RightToLeft) {
        return QRect(viewport.right() - itemSelectionMarkerInset
                         - itemSelectionMarkerWidth + 1,
                     y, itemSelectionMarkerWidth, 16);
    }
    return QRect(viewport.left() + itemSelectionMarkerInset, y,
                 itemSelectionMarkerWidth, 16);
}

} // namespace

bool drawViewPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *widget)
{
    if (element == QStyle::PE_FrameTabBarBase) {
        const Tokens t = tokens(option->palette);
        painter->setPen(t.strokeSecondary);
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        return true;
    }

    if (element == QStyle::PE_FrameTabWidget) {
        const Tokens t = tokens(option->palette);
        roundedRect(painter, QRectF(option->rect).adjusted(0, 0, -1, -1),
                    t.control, t.stroke, ControlRadius);
        return true;
    }

    if (element == QStyle::PE_PanelItemViewItem) {
        const auto *viewOption = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        const QAbstractItemView *view = itemView(widget);
        const bool popup = widget && widget->window()
            && widget->window()->windowType() == Qt::Popup;
        const bool comboPopup = popup && widget->window()->parentWidget()
            && qobject_cast<const QComboBox *>(widget->window()->parentWidget());
        const Tokens t = tokens(option->palette);
        const Tokens itemTokens = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        const bool tree = qobject_cast<const QTreeView *>(view);
        const bool table = qobject_cast<const QTableView *>(view);
        const bool selected = option->state & QStyle::State_Selected;
        const bool hovered = option->state & QStyle::State_MouseOver;
        const bool pressedItem = hovered && (option->state & QStyle::State_Sunken);
        QColor fill = Qt::transparent;
        if (pressedItem)
            fill = itemTokens.subtlePressed;
        else if (selected && hovered)
            fill = itemTokens.subtlePressed;
        else if (selected || hovered)
            fill = itemTokens.subtleHover;
        const qreal popupPress = comboPopup
            ? progress(widget, pressProperty,
                       pressedItem ? 1.0 : 0.0)
            : 0.0;
        QRectF itemRect = popup
            ? QRectF(option->rect).adjusted(5, 2, -5, -2)
            : tree ? QRectF(option->rect).adjusted(4, 2, -4, -2)
                   : table ? QRectF(option->rect)
                           : QRectF(option->rect).adjusted(2, 1, -2, -1);
        if (fill.alpha() > 0)
            roundedRect(painter, itemRect, fill, Qt::transparent,
                        popup ? 3.0 : table ? 0.0 : ControlRadius);
        const bool firstColumn = !viewOption || !viewOption->index.isValid()
            || viewOption->index.column() == 0;
        if (selected && comboPopup && firstColumn) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(enabled ? itemTokens.selectionAccent
                                       : itemTokens.accentFillDisabled);
            // WinUI's selected-item pill compresses to 62.5% while the
            // pointer is held. The interaction controller uses the same
            // 167 ms transition as the current DropdownContent template.
            const qreal indicatorWidth = 3.0;
            const qreal indicatorHeight = 16.0
                * (1.0 - 0.375 * popupPress);
            const qreal indicatorX = option->direction == Qt::RightToLeft
                ? itemRect.right() - indicatorWidth : itemRect.left();
            // The pill is aligned to the content slot created by the
            // ComboBoxItem's 5px/7px vertical padding.  At the normal 16px
            // height this is visually the same center as the row, while the
            // explicit slot keeps the compressed 0.625 state deterministic
            // at fractional device-pixel ratios.
            const QRectF itemContent = itemRect.adjusted(
                comboPopupItemPaddingLeft, comboPopupItemPaddingTop,
                -comboPopupItemPaddingRight, -comboPopupItemPaddingBottom);
            painter->drawRoundedRect(
                QRectF(indicatorX,
                       itemContent.center().y() - indicatorHeight / 2.0,
                       indicatorWidth, indicatorHeight),
                1.5, 1.5);
            painter->restore();
        } else if (selected && popup && firstColumn) {
            const QRect checkRect = QStyle::visualRect(option->direction, option->rect,
                QRect(option->rect.left() + 12,
                      option->rect.center().y() - 8, 16, 16));
            icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled).paint(painter,
                checkRect,
                Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        } else if (selected && viewOption && selectionMarkerView(widget)
                   && firstColumn) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(enabled ? itemTokens.selectionAccent
                                       : itemTokens.accentFillDisabled);
            const QRectF indicator = selectionMarkerRect(*viewOption, view);
            painter->drawRoundedRect(indicator, 1.5, 1.5);
            painter->restore();
        }
        return true;
    }

    if (element == QStyle::PE_IndicatorBranch) {
        const Tokens t = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        if (!(option->state & QStyle::State_Children))
            return true;
        const bool open = option->state & QStyle::State_Open;
        const Icon glyph = open ? Icon::ChevronDown
            : option->direction == Qt::RightToLeft ? Icon::ChevronLeft
                                                   : Icon::ChevronRight;
        const int extent = 12;
        icon(glyph, enabled ? t.textPrimary : t.textDisabled).paint(painter,
            QRect(option->rect.center().x() - extent / 2,
                  option->rect.center().y() - extent / 2, extent, extent),
            Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        return true;
    }

    if (element == QStyle::PE_IndicatorHeaderArrow) {
        const Tokens t = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        const Icon glyph = option->state & QStyle::State_UpArrow
            ? Icon::ChevronUp : Icon::ChevronDown;
        icon(glyph, enabled ? t.textSecondary : t.textDisabled).paint(painter,
            QRect(option->rect.center().x() - 6, option->rect.center().y() - 6,
                  12, 12), Qt::AlignCenter,
            enabled ? QIcon::Normal : QIcon::Disabled);
        return true;
    }

    if (element == QStyle::PE_FrameDockWidget) {
        const Tokens t = tokens(option->palette);
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(t.stroke, 1));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
        return true;
    }

    if (element == QStyle::PE_IndicatorDockWidgetResizeHandle) {
        QStyleOption splitter = *option;
        // Qt describes dock separators in the opposite axis to CE_Splitter.
        splitter.state.setFlag(QStyle::State_Horizontal,
                               !(option->state & QStyle::State_Horizontal));
        style->drawControl(QStyle::CE_Splitter, &splitter, painter, widget);
        return true;
    }

    return false;
}

bool drawViewControl(const Style *style, QStyle::ControlElement element,
                     const QStyleOption *option, QPainter *painter,
                     const QWidget *widget,
                     const TableEditorOverlap &tableEditorOverlap)
{
    if (element == QStyle::CE_TabBarTab) {
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            style->drawControl(QStyle::CE_TabBarTabShape, tab, painter, widget);
            style->drawControl(QStyle::CE_TabBarTabLabel, tab, painter, widget);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_ItemViewItem) {
        if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            if (source->backgroundBrush.style() != Qt::NoBrush) {
                painter->fillRect(source->rect, source->backgroundBrush);
            } else if (source->features & QStyleOptionViewItem::Alternate) {
                painter->fillRect(source->rect,
                                  source->palette.brush(QPalette::AlternateBase));
            }
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, source, painter, widget);
            const QAbstractItemView *view = itemView(widget);
            const auto *tableView = qobject_cast<const QTableView *>(view);
            const bool popup = widget && widget->window()
                && widget->window()->windowType() == Qt::Popup;
            const bool comboPopup = popup && widget->window()->parentWidget()
                && qobject_cast<const QComboBox *>(widget->window()->parentWidget());
            const Tokens itemTokens = tokens(option->palette);
            const bool enabled = source->state & QStyle::State_Enabled;
            const bool checkedPopupSelection = popup && !comboPopup
                && (source->state & QStyle::State_Selected);

            if (source->features & QStyleOptionViewItem::HasCheckIndicator) {
                QStyleOptionButton check;
                check.rect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator,
                                                   source, widget);
                check.palette = source->palette;
                check.state = source->state;
                check.state.setFlag(QStyle::State_Selected, false);
                check.state.setFlag(QStyle::State_On,
                    source->checkState == Qt::Checked);
                check.state.setFlag(QStyle::State_NoChange,
                    source->checkState == Qt::PartiallyChecked);
                check.state.setFlag(QStyle::State_Off,
                    source->checkState == Qt::Unchecked);
                style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &check, painter, widget);
            }

            if ((source->features & QStyleOptionViewItem::HasDecoration)
                && !source->icon.isNull() && !checkedPopupSelection) {
                const QRect decoration = style->subElementRect(
                    QStyle::SE_ItemViewItemDecoration, source, widget);
                paintThemedIcon(painter, source->icon, decoration,
                    source->decorationAlignment,
                    enabled ? itemTokens.textPrimary : itemTokens.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    source->state & QStyle::State_Selected ? QIcon::On : QIcon::Off);
            }

            const bool tableEditing = tableView
                && ((source->state & QStyle::State_Editing)
                    || (tableEditorOverlap
                        && tableEditorOverlap(tableView, source->index,
                                              source->rect)));
            if ((source->features & QStyleOptionViewItem::HasDisplay)
                && !tableEditing) {
                const bool hasLeadingContent =
                    (source->features & QStyleOptionViewItem::HasDecoration)
                    || (source->features & QStyleOptionViewItem::HasCheckIndicator);
                QRect textRect = popup
                    ? source->rect.adjusted(
                        comboPopup && !hasLeadingContent
                            ? comboPopupItemPaddingLeft + 5 : 42,
                        0,
                        comboPopup && !hasLeadingContent
                            ? -(comboPopupItemPaddingRight + 5) : -12,
                        0)
                    : style->subElementRect(QStyle::SE_ItemViewItemText, source, widget);
                painter->save();
                painter->setFont(source->font);
                painter->setPen(enabled ? itemTokens.textPrimary
                                        : itemTokens.textDisabled);
                Qt::Alignment alignment = Qt::Alignment(source->displayAlignment)
                    | Qt::AlignVCenter;
                alignment = QStyle::visualAlignment(source->direction, alignment);
                const bool wraps = source->features & QStyleOptionViewItem::WrapText;
                const int textFlags = int(alignment)
                    | (wraps ? int(Qt::TextWordWrap) : int(Qt::TextSingleLine));
                const QString text = wraps ? source->text
                    : source->fontMetrics.elidedText(source->text,
                        source->textElideMode, textRect.width());
                painter->drawText(textRect, textFlags, text);
                painter->restore();
            }

            if ((source->state & QStyle::State_HasFocus) && keyboardFocusVisible(view)) {
                const bool tree = qobject_cast<const QTreeView *>(view);
                const bool table = qobject_cast<const QTableView *>(view);
                const QRectF focusRect = tree
                    ? QRectF(source->rect).adjusted(4, 2, -4, -2)
                    : table ? QRectF(source->rect).adjusted(1, 1, -1, -1)
                            : QRectF(source->rect).adjusted(2, 1, -2, -1);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(itemTokens.focusOuter, 2.0));
                painter->drawRoundedRect(focusRect.adjusted(1, 1, -1, -1),
                                         table ? 0.0 : 5.0,
                                         table ? 0.0 : 5.0);
                painter->setPen(QPen(itemTokens.focusInner, 1.0));
                painter->drawRoundedRect(focusRect.adjusted(3, 3, -3, -3),
                                         table ? 0.0 : 3.0,
                                         table ? 0.0 : 3.0);
                painter->restore();
            }
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_Splitter) {
        const Tokens t = tokens(option->palette);
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
        const qreal press = progress(widget, pressProperty,
                                     option->state & QStyle::State_Sunken ? 1.0 : 0.0);
        QColor color = mix(t.stroke, t.strokeStrong, hover);
        color = mix(color, t.accentFill, press);
        const qreal thickness = 1.0 + hover + press;
        const bool horizontal = option->state & QStyle::State_Horizontal;
        QRectF handle;
        if (horizontal) {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.height() - 12));
            const QRectF splitterRect(option->rect);
            handle = QRectF(splitterRect.center().x() - thickness / 2.0,
                            splitterRect.center().y() - length / 2.0,
                            thickness, length);
        } else {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.width() - 12));
            const QRectF splitterRect(option->rect);
            handle = QRectF(splitterRect.center().x() - length / 2.0,
                            splitterRect.center().y() - thickness / 2.0,
                            length, thickness);
        }
        handle = snappedSplitterGrip(handle, horizontal, painter);
        roundedRect(painter, handle, color, Qt::transparent, thickness / 2.0);
        return true;
    }

    if (element == QStyle::CE_DockWidgetTitle) {
        if (const auto *dock = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            const Tokens t = tokens(option->palette);
            painter->save();
            painter->fillRect(dock->rect, t.layer);
            painter->setPen(t.stroke);
            if (dock->verticalTitleBar)
                painter->drawLine(dock->rect.topRight(), dock->rect.bottomRight());
            else
                painter->drawLine(dock->rect.bottomLeft(), dock->rect.bottomRight());

            QRect titleRect = style->subElementRect(QStyle::SE_DockWidgetTitleBarText,
                                                    dock, widget);
            if (dock->verticalTitleBar) {
                const QRect transposed = dock->rect.transposed();
                titleRect = QRect(transposed.left() + dock->rect.bottom() - titleRect.bottom(),
                                  transposed.top() + titleRect.left() - dock->rect.left(),
                                  titleRect.height(), titleRect.width());
                painter->translate(transposed.left(), transposed.top() + transposed.width());
                painter->rotate(-90);
                painter->translate(-transposed.left(), -transposed.top());
            }
            QFont font = widget ? widget->font() : QApplication::font();
            font.setWeight(QFont::DemiBold);
            painter->setFont(font);
            painter->setPen(dock->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(titleRect,
                              QStyle::visualAlignment(dock->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              painter->fontMetrics().elidedText(
                                  dock->title, Qt::ElideRight, titleRect.width()));
            painter->restore();
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_TabBarTabShape) {
        const Tokens t = tokens(option->palette);
        const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option);
        const bool selected = option->state & QStyle::State_Selected;
        const bool north = !tab || tab->shape == QTabBar::RoundedNorth
            || tab->shape == QTabBar::TriangularNorth;
        if (selected && north) {
            const QColor selectedFill = t.dark ? QColor(44, 44, 44)
                                                : QColor(251, 251, 251);
            const QRectF rect = QRectF(option->rect).adjusted(1, 1, -1, 0);
            const qreal radius = 8.0;
            QPainterPath surface;
            surface.moveTo(rect.left(), rect.bottom());
            surface.lineTo(rect.left(), rect.top() + radius);
            surface.quadTo(rect.left(), rect.top(), rect.left() + radius,
                           rect.top());
            surface.lineTo(rect.right() - radius, rect.top());
            surface.quadTo(rect.right(), rect.top(), rect.right(),
                           rect.top() + radius);
            surface.lineTo(rect.right(), rect.bottom());
            surface.lineTo(rect.left(), rect.bottom());
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->fillPath(surface, selectedFill);
            QPainterPath border;
            border.moveTo(rect.left(), rect.bottom());
            border.lineTo(rect.left(), rect.top() + radius);
            border.quadTo(rect.left(), rect.top(), rect.left() + radius,
                          rect.top());
            border.lineTo(rect.right() - radius, rect.top());
            border.quadTo(rect.right(), rect.top(), rect.right(),
                          rect.top() + radius);
            border.lineTo(rect.right(), rect.bottom());
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.stroke, 1));
            painter->drawPath(border);
            painter->restore();
        } else {
            QColor fill = Qt::transparent;
            if (option->state & QStyle::State_Sunken)
                fill = t.controlPressed;
            else if (option->state & QStyle::State_MouseOver)
                fill = t.layer;
            roundedRect(painter, QRectF(option->rect).adjusted(2, 2, -2, -2),
                        fill, Qt::transparent, ControlRadius);
            if (!selected && !(option->state & QStyle::State_MouseOver)) {
                painter->setPen(t.stroke);
                const int separatorX = option->direction == Qt::RightToLeft
                    ? option->rect.left() : option->rect.right();
                painter->drawLine(separatorX, option->rect.top() + 8,
                                  separatorX, option->rect.bottom() - 8);
            }
        }
        if ((option->state & QStyle::State_HasFocus) && keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                     ControlRadius, ControlRadius);
            painter->setPen(QPen(t.focusInner, 1));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(5, 5, -5, -5),
                                     ControlRadius - 1, ControlRadius - 1);
            painter->restore();
        }
        return true;
    }

    if (element == QStyle::CE_TabBarTabLabel) {
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            const Tokens t = tokens(option->palette);
            const bool enabled = tab->state & QStyle::State_Enabled;
            const bool selected = tab->state & QStyle::State_Selected;
            QRect textRect = tab->rect.adjusted(8, 0, -8, 0);
            if (tab->leftButtonSize.isValid())
                textRect.adjust(tab->leftButtonSize.width() + 4, 0, 0, 0);
            if (tab->rightButtonSize.isValid())
                textRect.adjust(0, 0, -tab->rightButtonSize.width() - 4, 0);
            painter->save();
            QFont font = widget ? widget->font() : QApplication::font();
            font.setPixelSize(12);
            font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
            painter->setFont(font);
            painter->setPen(enabled
                ? (selected ? t.textPrimary : t.textSecondary)
                : t.textDisabled);
            if (!tab->icon.isNull()) {
                const QRect logicalIcon(textRect.left(), textRect.center().y() - 8,
                                        16, 16);
                const QRect iconRect = QStyle::visualRect(tab->direction, tab->rect,
                                                  logicalIcon);
                paintThemedIcon(painter, tab->icon, iconRect, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    selected ? QIcon::On : QIcon::Off);
                if (tab->direction == Qt::RightToLeft)
                    textRect.setRight(iconRect.left() - 10);
                else
                    textRect.setLeft(iconRect.right() + 10);
            }
            painter->drawText(textRect,
                              QStyle::visualAlignment(tab->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              tab->fontMetrics.elidedText(tab->text, Qt::ElideRight,
                                                          textRect.width()));
            painter->restore();
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_HeaderSection) {
        const Tokens t = tokens(option->palette);
        QColor fill = t.layer;
        if (option->state & QStyle::State_Sunken)
            fill = t.subtlePressed;
        else if (option->state & QStyle::State_MouseOver)
            fill = t.subtleHover;
        painter->fillRect(option->rect, fill);
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        const int separatorX = option->direction == Qt::RightToLeft
            ? option->rect.left() : option->rect.right();
        painter->drawLine(separatorX, option->rect.top(),
                          separatorX, option->rect.bottom());
        painter->restore();
        return true;
    }

    if (element == QStyle::CE_Header) {
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            style->drawControl(QStyle::CE_HeaderSection, header, painter, widget);
            QStyleOptionHeader label = *header;
            label.sortIndicator = QStyleOptionHeader::None;
            if (header->sortIndicator != QStyleOptionHeader::None) {
                const QRect arrowRect = headerSortIndicatorRect(*header);
                if (header->direction == Qt::RightToLeft)
                    label.rect.setLeft(qMin(label.rect.right(), arrowRect.right() + 6));
                else
                    label.rect.setRight(qMax(label.rect.left(), arrowRect.left() - 6));
            }
            style->drawControl(QStyle::CE_HeaderLabel, &label, painter, widget);
            if (header->sortIndicator != QStyleOptionHeader::None) {
                QStyleOption arrow;
                arrow.rect = headerSortIndicatorRect(*header);
                arrow.palette = header->palette;
                arrow.state = header->state;
                arrow.state.setFlag(QStyle::State_UpArrow,
                    header->sortIndicator == QStyleOptionHeader::SortUp);
                style->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &arrow, painter, widget);
            }
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_HeaderLabel) {
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            const Tokens t = tokens(option->palette);
            QRect content = header->rect.adjusted(12, 0, -10, 0);
            painter->save();
            QFont font = widget ? widget->font() : QApplication::font();
            font.setPixelSize(12);
            font.setWeight(QFont::DemiBold);
            painter->setFont(font);
            painter->setPen(header->state & QStyle::State_Enabled
                                ? t.textSecondary : t.textDisabled);
            if (!header->icon.isNull()) {
                const QRect logicalIcon(content.left(), content.center().y() - 8,
                                        16, 16);
                const QRect iconRect = QStyle::visualRect(header->direction, header->rect,
                                                  logicalIcon);
                paintThemedIcon(painter, header->icon, iconRect,
                    Qt::AlignCenter, header->state & QStyle::State_Enabled
                        ? t.textSecondary : t.textDisabled,
                    header->state & QStyle::State_Enabled
                        ? QIcon::Normal : QIcon::Disabled);
                if (header->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            const Qt::Alignment horizontal = header->textAlignment
                & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter);
            painter->drawText(content,
                QStyle::visualAlignment(header->direction, horizontal | Qt::AlignVCenter)
                    | Qt::TextSingleLine,
                header->fontMetrics.elidedText(header->text, Qt::ElideRight,
                                                content.width()));
            painter->restore();
            return true;
        }
        return true;
    }

    return false;
}

} // namespace WinUI3::Private
