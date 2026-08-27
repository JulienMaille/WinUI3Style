#include "winui3menus_p.h"

#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QComboBox>
#include <QPainter>
#include <QStyleOptionMenuItem>
#include <QVariant>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0)
{
    if (!widget)
        return fallback;
    const QVariant value = widget->property(name);
    return value.isValid() ? value.toReal() : fallback;
}

} // namespace

bool drawMenuPrimitive(const Style *, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter,
                       const QWidget *)
{
    if (element == QStyle::PE_PanelMenuBar) {
        const Tokens t = tokens(option->palette);
        painter->fillRect(option->rect, t.surface);
        return true;
    }

    if (element == QStyle::PE_PanelMenu) {
        const Tokens t = tokens(option->palette);
        QColor fill = t.layer;
        fill.setAlpha(238);
        roundedRect(painter, option->rect, fill, t.stroke, OverlayRadius);
        return true;
    }

    return false;
}

bool drawMenuControl(const Style *, QStyle::ControlElement element,
                     const QStyleOption *option, QPainter *painter,
                     const QWidget *widget)
{
    if (element != QStyle::CE_MenuBarItem
        && element != QStyle::CE_MenuBarEmptyArea
        && element != QStyle::CE_MenuItem) {
        return false;
    }

    if (element == QStyle::CE_MenuBarEmptyArea)
        return true;

    const Tokens t = tokens(option->palette);

    if (element == QStyle::CE_MenuBarItem) {
        if (const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            // QMenuBar owns the animation properties, while this option is
            // painted once per QAction. Never let the shared bar progress
            // leak into a sibling item that is not active.
            const bool enabled = item->state & QStyle::State_Enabled;
            const bool selected = enabled && (item->state & QStyle::State_Selected);
            const bool sunken = enabled && (item->state & QStyle::State_Sunken);
            const qreal hover = selected
                ? progress(widget, hoverProperty, 1.0) : 0.0;
            const qreal press = sunken
                ? progress(widget, pressProperty, 1.0) : 0.0;
            QColor fill = mix(Qt::transparent, t.subtleHover, hover);
            fill = mix(fill, t.subtlePressed, press);
            if (fill.alpha() > 0)
                roundedRect(painter, QRectF(item->rect).adjusted(2, 2, -2, -2),
                            fill, Qt::transparent, ControlRadius);
            painter->setFont(item->font);
            painter->setPen(item->state & QStyle::State_Enabled
                                ? t.textPrimary : t.textDisabled);
            painter->drawText(item->rect.adjusted(10, 0, -10, 0),
                              Qt::AlignCenter | Qt::TextShowMnemonic
                                  | Qt::TextSingleLine,
                              item->text);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_MenuItem) {
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const bool comboItem = qobject_cast<const QComboBox *>(widget);
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->setPen(t.stroke);
                painter->drawLine(menu->rect.left() + 12, menu->rect.center().y(),
                                  menu->rect.right() - 12, menu->rect.center().y());
                return true;
            }
            if (menu->checked || menu->state & (QStyle::State_Selected | QStyle::State_Sunken))
                roundedRect(painter,
                            QRectF(menu->rect).adjusted(comboItem ? 5 : 4, 2,
                                                       comboItem ? -5 : -4, -2),
                            menu->state & QStyle::State_Sunken ? t.subtlePressed
                                                      : t.subtleHover,
                            Qt::transparent, ControlRadius);

            const bool enabled = menu->state & QStyle::State_Enabled;
            const QRect leading = QStyle::visualRect(menu->direction, menu->rect,
                                             QRect(menu->rect.left() + 12,
                                                   menu->rect.center().y() - 8,
                                                   16, 16));
            if (comboItem && menu->checked) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(enabled ? t.selectionAccent : t.accentFillDisabled);
                const qreal x = leading.left()
                    + (menu->direction == Qt::RightToLeft ? 13.0 : -6.0);
                painter->drawRoundedRect(
                    QRectF(x, menu->rect.center().y() - 8.0, 3.0, 16.0),
                    1.5, 1.5);
                painter->restore();
            } else if (menu->checked) {
                WinUI3::icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled).paint(painter,
                    leading,
                    Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            } else if (!menu->icon.isNull()) {
                paintThemedIcon(painter, menu->icon, leading, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled);
            }
            const QStringList parts = menu->text.split(QLatin1Char('\t'));
            painter->setFont(menu->font);
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            const QFontMetrics metrics(menu->font);
            const int submenuWidth = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? 24 : 0;
            const int shortcutWidth = parts.size() > 1
                ? metrics.horizontalAdvance(parts.value(1)) : 0;
            const int shortcutRight = menu->rect.right() - 16 - submenuWidth;
            const int shortcutLeft = shortcutRight - shortcutWidth;
            const int textRight = shortcutWidth > 0
                ? shortcutLeft - 20 : shortcutRight;
            const int textLeft = comboItem && menu->icon.isNull() ? 16 : 42;
            const QRect textRect = QStyle::visualRect(menu->direction, menu->rect,
                QRect(menu->rect.left() + textLeft, menu->rect.top(),
                      qMax(0, textRight - menu->rect.left() - textLeft + 1),
                      menu->rect.height()));
            painter->drawText(textRect,
                              QStyle::visualAlignment(menu->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              metrics.elidedText(parts.value(0), Qt::ElideRight,
                                                 textRect.width()));
            if (parts.size() > 1) {
                painter->setPen(enabled ? t.textSecondary : t.textDisabled);
                const QRect shortcutRect = QStyle::visualRect(menu->direction, menu->rect,
                    QRect(shortcutLeft, menu->rect.top(), shortcutWidth,
                          menu->rect.height()));
                painter->drawText(shortcutRect,
                                  QStyle::visualAlignment(menu->direction,
                                                  Qt::AlignRight | Qt::AlignVCenter),
                                  parts.value(1));
            }
            if (menu->menuItemType == QStyleOptionMenuItem::SubMenu) {
                const QRect submenu = QStyle::visualRect(menu->direction, menu->rect,
                    QRect(menu->rect.right() - 24, menu->rect.center().y() - 8,
                          16, 16));
                WinUI3::icon(menu->direction == Qt::RightToLeft ? Icon::ChevronLeft
                                                         : Icon::ChevronRight,
                     enabled ? t.textPrimary : t.textDisabled).paint(painter, submenu,
                    Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            }
            return true;
        }
        return false;
    }

    return false;
}

} // namespace WinUI3::Private
