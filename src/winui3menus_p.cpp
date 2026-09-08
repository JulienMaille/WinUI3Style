// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3menus_p.h"

#include "winui3density_p.h"
#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QCursor>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMenu>
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
        const QString family = fontFamilies.contains(fluent) ? fluent : mdl2;
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

void paintMenuChevron(QPainter *painter, const QRect &menuRect, Qt::LayoutDirection direction,
                      const QColor &color)
{
    const QRect logical(menuRect.right() - MenuChevronRightPadding - MenuChevronSlotSize + 1,
                        menuRect.center().y() - MenuChevronSlotSize / 2, MenuChevronSlotSize,
                        MenuChevronSlotSize);
    const QRect chevron = QStyle::visualRect(direction, menuRect, logical);
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->setFont(menuChevronFont());
    painter->setPen(color);
    painter->drawText(chevron, Qt::AlignCenter, menuChevronGlyph(direction == Qt::RightToLeft));
    painter->restore();
}

} // namespace

bool drawMenuPrimitive(const Style *, QStyle::PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget)
{
    if (element == QStyle::PE_PanelMenuBar) {
        // Fill from the palette Window role: the chrome sync makes that role
        // transparent over a live composited backdrop, so the menu bar
        // reveals the material instead of painting an opaque band. Clear
        // explicitly on a direct backdrop: neither this fill nor Qt's
        // auto-fill (deliberately disabled there) erases stale pixels, so
        // resize/expose frames would otherwise leave permanent ghosts.
        if (paintsDirectlyOnBackdrop(widget)) {
            painter->save();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(option->rect, Qt::transparent);
            painter->restore();
            return true;
        }
        painter->fillRect(option->rect, option->palette.brush(QPalette::Window));
        return true;
    }

    if (element == QStyle::PE_PanelMenu || element == QStyle::PE_FrameMenu) {
        const Tokens t = tokens(option->palette);
        // Menu flyout surface: opaque SolidBackgroundFill fallback, or the
        // translucent acrylic tint on a composited popup (the palette Window
        // role already carries the right variant). Windows rounds the popup
        // window so the OverlayRadius paint stays visible. Cover the whole
        // widget, not just the option rect: the 2px contents margins and the
        // rounded-corner cutouts are never painted otherwise and keep their
        // white backing-store pixels as a permanent light frame.
        const QRect surface = widget ? widget->rect() : option->rect;
        const QColor fill = option->palette.color(QPalette::Window);
        const QColor stroke = t.dark ? QColor(0, 0, 0, 51) : QColor(0, 0, 0, 15);
        // The 1px flyout stroke must read as a solid hairline on every
        // edge, even over a live material: rebuild the panel (not just
        // its rect, which excludes the contents margins) then draw the
        // stroke on top, centered on device pixels so antialiasing does
        // not flatten it to half-strength grey on two of the four sides.
        if (paintsDirectlyOnBackdrop(widget)) {
            painter->save();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(surface, Qt::transparent);
            painter->restore();
            roundedRect(painter, QRectF(surface), fill, Qt::transparent, OverlayRadius);
        } else {
            painter->fillRect(surface, fill);
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(stroke, 1.0));
        painter->drawRoundedRect(QRectF(surface).adjusted(0.5, 0.5, -0.5, -0.5), OverlayRadius,
                                 OverlayRadius);
        painter->restore();
        return true;
    }

    return false;
}

bool drawMenuControl(const Style *, QStyle::ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget)
{
    if (element != QStyle::CE_MenuBarItem && element != QStyle::CE_MenuBarEmptyArea
        && element != QStyle::CE_MenuItem) {
        return false;
    }

    if (element == QStyle::CE_MenuBarEmptyArea) {
        if (widget && widget->property(Style::SurfaceProperty).isValid())
            painter->fillRect(option->rect, widget->palette().color(QPalette::Window));
        return true;
    }

    const Tokens t = tokens(option->palette);

    if (element == QStyle::CE_MenuBarItem) {
        if (const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (widget && widget->property(Style::SurfaceProperty).isValid())
                painter->fillRect(item->rect, widget->palette().color(QPalette::Window));
            // QMenuBar owns the animation properties, while this option is
            // painted once per QAction. Never let the shared bar progress
            // leak into a sibling item that is not active.
            const bool enabled = item->state & QStyle::State_Enabled;
            const bool selected = enabled && (item->state & QStyle::State_Selected);
            const bool sunken = enabled && (item->state & QStyle::State_Sunken);
            const qreal hover = selected ? progress(widget, hoverProperty, 1.0) : 0.0;
            const qreal press = sunken ? progress(widget, pressProperty, 1.0) : 0.0;
            QColor fill = mix(Qt::transparent, t.subtleHover, hover);
            fill = mix(fill, t.subtlePressed, press);
            if (fill.alpha() > 0)
                roundedRect(painter, QRectF(item->rect).adjusted(2, 2, -2, -2), fill,
                            Qt::transparent, ControlRadius);
            painter->setFont(item->font);
            painter->setPen(item->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled);
            // CT_MenuBarItem reserves the density-specific horizontal inset.
            // Keeping the historical fixed 10 px paint inset in Compact
            // (which reserves 8 px) clipped four pixels from tight labels.
            const int textInset = qMin(10, densityMetricsFor(widget).menuBarHorizontalPadding);
            painter->drawText(item->rect.adjusted(textInset, 0, -textInset, 0),
                              Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextSingleLine,
                              item->text);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_MenuItem) {
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            // Combo popup rows arrive with the QComboBox as the style
            // widget; plain menu rows arrive with their QMenu. The two map
            // to different upstream templates (ComboBoxItem vs
            // MenuFlyoutItem) with different hover contracts.
            const QComboBox *combo = qobject_cast<const QComboBox *>(widget);
            const bool comboItem = combo != nullptr;
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->setPen(t.stroke);
                painter->drawLine(menu->rect.left() + 12, menu->rect.center().y(),
                                  menu->rect.right() - 12, menu->rect.center().y());
                return true;
            }
            // Combo rows: Qt's Selected flag conflates the current row with
            // hover tracking that nothing reliably clears (stuck highlight
            // with the pointer elsewhere). Drive the transient hover fill
            // from the live cursor instead: a row is hovered iff the cursor
            // is really inside it. The current/checkable marker below keeps
            // using checked/current state, so selection rendering is
            // untouched. Offscreen keeps the flag path (no cursor there;
            // deterministic captures unchanged).
            bool showHover = false;
            if (comboItem) {
                if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
                    showHover = menu->state & QStyle::State_MouseOver;
                } else if (combo->view() && combo->view()->viewport()) {
                    const QWidget *viewport = combo->view()->viewport();
                    const QRect rowGlobal(viewport->mapToGlobal(menu->rect.topLeft()),
                                          menu->rect.size());
                    showHover = rowGlobal.contains(QCursor::pos());
                }
            }
            const bool showPressed = menu->state & QStyle::State_Sunken;
            // Combo rows paint their own selection pill separately below;
            // their hover fill must NOT cover the whole row (WinUI reserves
            // the leading icon/check slot). Plain menu rows use the full
            // inset fill, driven by Qt's Selected flag like MenuFlyout's
            // PointerOver state.
            const bool showHoverFill =
                    comboItem ? showHover : (menu->state & QStyle::State_Selected);
            if (showHoverFill || showPressed) {
                // QMenu's native erase fills the Selected row full-bleed
                // behind our inset pill (nothing Qt-side insets it), so
                // rebuild the row from the popup surface first in the
                // opaque fallback: the pill then reads as an inset card
                // on every edge. MenuFlyout maps item PointerOver/Pressed
                // to SubtleFillColorSecondary/Tertiary over the flyout
                // surface. On a composited acrylic surface the native
                // erase is a no-op (WA_StyledBackground): Source-blend the
                // already translucent surface roles explicitly so the
                // pill composites exactly one SubtleFill layer.
                if (paintsDirectlyOnBackdrop(widget)) {
                    painter->save();
                    painter->setCompositionMode(QPainter::CompositionMode_Source);
                    painter->fillRect(menu->rect, option->palette.color(QPalette::Window));
                    painter->restore();
                } else {
                    painter->fillRect(menu->rect, option->palette.color(QPalette::Window));
                }
                roundedRect(painter, QRectF(menu->rect).adjusted(4, 2, -4, -2),
                            showPressed ? t.subtlePressed : t.subtleHover, Qt::transparent,
                            ControlRadius);
            }

            const bool enabled = menu->state & QStyle::State_Enabled;
            const QRect leading = QStyle::visualRect(
                    menu->direction, menu->rect,
                    QRect(menu->rect.left() + 12, menu->rect.center().y() - 8, 16, 16));
            if (comboItem && menu->checked) {
                const QWidget *interactionSurface =
                        combo->view() ? combo->view()->viewport() : nullptr;
                // WinUI's SelectedPressed state exists only when the already
                // selected row itself is held. Pressing another row must not
                // animate the marker that remains on the selected row.
                const qreal press = menu->state & QStyle::State_Selected
                        ? progress(interactionSurface, pressProperty, 0.0)
                        : 0.0;
                const qreal markerHeight = 16.0 * (1.0 - 0.375 * press);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(enabled ? t.selectionAccent : t.accentFillDisabled);
                const qreal x = menu->direction == Qt::RightToLeft ? leading.right() + 4.0
                                                                   : leading.left() - 6.0;
                painter->drawRoundedRect(
                        QRectF(x, menu->rect.center().y() - markerHeight / 2.0, 3.0, markerHeight),
                        1.5, 1.5);
                painter->restore();
            }
            // A ComboBox selection marker and its item decoration occupy
            // separate leading slots in WinUI. The marker must not replace
            // the selected item's icon.
            if (comboItem && !menu->icon.isNull()) {
                paintThemedIcon(painter, menu->icon, leading, Qt::AlignCenter,
                                enabled ? t.textPrimary : t.textDisabled,
                                enabled ? QIcon::Normal : QIcon::Disabled);
            } else if (!comboItem && menu->checked) {
                WinUI3::icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled)
                        .paint(painter, leading, Qt::AlignCenter,
                               enabled ? QIcon::Normal : QIcon::Disabled);
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
            const qsizetype secondTab =
                    firstTab >= 0 ? menuText.indexOf(QLatin1Char('\t'), firstTab + 1) : -1;
            const bool hasShortcut = firstTab >= 0;
            // QFontMetrics and QPainter in the supported Qt baseline take
            // QString rather than QStringView. fromRawData() gives them a
            // non-owning view, so neither field is copied just for paint.
            const QString itemText = hasShortcut
                    ? QString::fromRawData(menu->text.constData(), firstTab)
                    : menu->text;
            const qsizetype shortcutLength = hasShortcut
                    ? (secondTab >= 0 ? secondTab - firstTab - 1 : menuText.size() - firstTab - 1)
                    : 0;
            const QString shortcutText = hasShortcut
                    ? QString::fromRawData(menu->text.constData() + firstTab + 1, shortcutLength)
                    : QString();
            painter->setFont(menu->font);
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            const QFontMetrics metrics(menu->font);
            const int submenuWidth = menu->menuItemType == QStyleOptionMenuItem::SubMenu ? 24 : 0;
            const int shortcutWidth = hasShortcut ? metrics.horizontalAdvance(shortcutText) : 0;
            const int shortcutRight = menu->rect.right() - 16 - submenuWidth;
            const int shortcutLeft = shortcutRight - shortcutWidth;
            const int textRight = shortcutWidth > 0 ? shortcutLeft - 20 : shortcutRight;
            const int textLeft = comboItem && menu->icon.isNull() ? 16 : 42;
            const QRect textRect =
                    QStyle::visualRect(menu->direction, menu->rect,
                                       QRect(menu->rect.left() + textLeft, menu->rect.top(),
                                             qMax(0, textRight - menu->rect.left() - textLeft + 1),
                                             menu->rect.height()));
            painter->drawText(
                    textRect,
                    QStyle::visualAlignment(menu->direction, Qt::AlignLeft | Qt::AlignVCenter),
                    metrics.elidedText(itemText, Qt::ElideRight, textRect.width()));
            if (hasShortcut) {
                painter->setPen(enabled ? t.textSecondary : t.textDisabled);
                const QRect shortcutRect = QStyle::visualRect(
                        menu->direction, menu->rect,
                        QRect(shortcutLeft, menu->rect.top(), shortcutWidth, menu->rect.height()));
                painter->drawText(
                        shortcutRect,
                        QStyle::visualAlignment(menu->direction, Qt::AlignRight | Qt::AlignVCenter),
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
