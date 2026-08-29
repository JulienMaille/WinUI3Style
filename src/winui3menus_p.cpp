#include "winui3menus_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QComboBox>
#include <QFontDatabase>
#include <QPainter>
#include <QStyleOptionMenuItem>
#include <QVariant>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

// WinUI keeps a 16px chevron Viewbox, but the FontIcon itself is FontSize 12.
// Keep the existing 24px trailing column and center the smaller glyph in it.
constexpr int MenuChevronSlotSize = 16;
constexpr int MenuChevronFontSize = 12;
constexpr int MenuChevronRightPadding = 9;

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0)
{
    return framePropertyRegistry().real(widget, name, fallback);
}

const QFont &menuChevronFont()
{
    static const QFont font = [] {
        const QString fluent = QStringLiteral("Segoe Fluent Icons");
        const QString mdl2 = QStringLiteral("Segoe MDL2 Assets");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const QStringList fontFamilies = QFontDatabase::families();
#else
        // QFontDatabase::families() is static from Qt 6 onward.
        const QFontDatabase fontDatabase;
        const QStringList fontFamilies = fontDatabase.families();
#endif
        const QString family = fontFamilies.contains(fluent)
            ? fluent : mdl2;
        QFont result(family);
        result.setPixelSize(MenuChevronFontSize);
        return result;
    }();
    return font;
}

const QString &menuChevronGlyph(bool rightToLeft)
{
    static const QString right(1, QChar(0xE974));
    static const QString left(1, QChar(0xE76B));
    return rightToLeft ? left : right;
}

void paintMenuChevron(QPainter *painter, const QRect &menuRect,
                      Qt::LayoutDirection direction, const QColor &color)
{
    const QRect logical(menuRect.right() - MenuChevronRightPadding
                            - MenuChevronSlotSize + 1,
                        menuRect.center().y() - MenuChevronSlotSize / 2,
                        MenuChevronSlotSize, MenuChevronSlotSize);
    const QRect chevron = QStyle::visualRect(direction, menuRect, logical);
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setFont(menuChevronFont());
    painter->setPen(color);
    painter->drawText(chevron, Qt::AlignCenter,
                      menuChevronGlyph(direction == Qt::RightToLeft));
    painter->restore();
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
        // The menu flyout surface is opaque (WinUI's SolidBackgroundFill
        // fallback), with the SurfaceStrokeColorFlyout border. Windows rounds
        // the opaque popup window so the OverlayRadius paint stays visible.
        const QColor fill = t.dark ? QColor(44, 44, 44)
                                   : QColor(252, 252, 252);
        const QColor stroke = t.dark ? QColor(0, 0, 0, 51)
                                     : QColor(0, 0, 0, 15);
        roundedRect(painter, option->rect, fill, stroke, OverlayRadius);
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
            // Keep the first two fields of the historical split('\t')
            // contract.  A third field remains intentionally ignored, just as
            // parts.value(1) was before.
            const QString &menuText = menu->text;
            const qsizetype firstTab = menuText.indexOf(QLatin1Char('\t'));
            const qsizetype secondTab = firstTab >= 0
                ? menuText.indexOf(QLatin1Char('\t'), firstTab + 1) : -1;
            const bool hasShortcut = firstTab >= 0;
            // QFontMetrics and QPainter in the supported Qt baseline take
            // QString rather than QStringView. fromRawData() gives them a
            // non-owning view, so neither field is copied just for paint.
            const QString itemText = hasShortcut
                ? QString::fromRawData(menu->text.constData(), firstTab)
                : menu->text;
            const qsizetype shortcutLength = hasShortcut
                ? (secondTab >= 0 ? secondTab - firstTab - 1
                                  : menuText.size() - firstTab - 1)
                : 0;
            const QString shortcutText = hasShortcut
                ? QString::fromRawData(menu->text.constData() + firstTab + 1,
                                       shortcutLength)
                : QString();
            painter->setFont(menu->font);
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            const QFontMetrics metrics(menu->font);
            const int submenuWidth = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? 24 : 0;
            const int shortcutWidth = hasShortcut
                ? metrics.horizontalAdvance(shortcutText) : 0;
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
                              metrics.elidedText(itemText, Qt::ElideRight,
                                                 textRect.width()));
            if (hasShortcut) {
                painter->setPen(enabled ? t.textSecondary : t.textDisabled);
                const QRect shortcutRect = QStyle::visualRect(menu->direction, menu->rect,
                    QRect(shortcutLeft, menu->rect.top(), shortcutWidth,
                          menu->rect.height()));
                painter->drawText(shortcutRect,
                                  QStyle::visualAlignment(menu->direction,
                                                  Qt::AlignRight | Qt::AlignVCenter),
                                  shortcutText);
            }
            if (menu->menuItemType == QStyleOptionMenuItem::SubMenu) {
                paintMenuChevron(painter, menu->rect, menu->direction,
                                 enabled ? t.textPrimary : t.textDisabled);
            }
            return true;
        }
        return false;
    }

    return false;
}

} // namespace WinUI3::Private
