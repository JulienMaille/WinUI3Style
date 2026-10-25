// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3viewrenderers_p.h"

#include "winui3paint_p.h"
#include "winui3menus_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3surfaces_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QLayout>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSizeGrip>
#include <QTextEdit>

#include <QAbstractItemView>
#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QCompleter>
#include <QListView>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionDockWidget>
#include <QStyleOptionHeader>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSizeGrip>
#include <QStyleOptionTab>
#include <QStyleOptionViewItem>
#include <QTableView>
#include <QTabBar>
#include <QTreeView>
#include <QVariant>

namespace WinUI3::Private {
using namespace PaintPrivate;

namespace {

QFont controlLabelFont(const QWidget *widget)
{
    QFont font = widget ? widget->font() : QApplication::font();
    // The stock WinUI label uses 12 px over the style's 14 px application
    // font. An explicit widget font or a replaced application font belongs
    // to the host application and must not be silently reset here.
    if (!widget || (!widget->testAttribute(Qt::WA_SetFont) && font.pixelSize() == 14))
        font.setPixelSize(12);
    return font;
}

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

bool completerPopupView(const QAbstractItemView *view)
{
    return view
            && (view->property(completerOwnerProperty).isValid()
                || qobject_cast<const QCompleter *>(view->parent()));
}

// QCalendarWidget uses a QTableView for its day grid, but that grid is not a
// WinUI flyout list. Applying the flyout's leading checkmark/content gutter to
// each seven-column day cell leaves no room for the date text (the old popup
// renderer starts text at x=42). Keep calendar cells on the regular item-view
// geometry while still allowing the popup surface itself to be prepared.
bool calendarPopupView(const QWidget *widget)
{
    if (!widget)
        return false;
    if (qobject_cast<const QCalendarWidget *>(widget->window()))
        return true;
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

constexpr int itemSelectionMarkerWidth = 3;
constexpr int itemSelectionMarkerInset = 2;
constexpr int treeBranchMarkerClearance = 4;
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
    if (!qobject_cast<const QListView *>(view) && !qobject_cast<const QTreeView *>(view))
        return nullptr;
    return view;
}

QRect selectionMarkerRect(const QStyleOptionViewItem &option, const QAbstractItemView *view)
{
    if (!view || !view->viewport())
        return {};

    // The TreeView marker belongs to the row's viewport, not to the tree
    // content slot. Keeping it in the leading viewport gutter prevents the
    // hierarchy indentation from moving the selection affordance.
    const QRect viewport = view->viewport()->rect();
    const int y = option.rect.center().y() - 8;
    if (option.direction == Qt::RightToLeft) {
        return QRect(viewport.right() - itemSelectionMarkerInset - itemSelectionMarkerWidth + 1, y,
                     itemSelectionMarkerWidth, 16);
    }
    return QRect(viewport.left() + itemSelectionMarkerInset, y, itemSelectionMarkerWidth, 16);
}

} // namespace

bool calendarNavigationBarWidget(const QWidget *widget)
{
    return widget && widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
            && qobject_cast<const QCalendarWidget *>(widget->parentWidget());
}

bool drawWidgetBackdropPrimitive(const Style *, const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget)
{
    if (!option || !painter)
        return false;
    if (calendarNavigationBarWidget(widget)) {
        if (widget->window()->windowType() == Qt::Popup) {
            const QColor surface = widget->window()->palette().color(QPalette::Window);
            if (!clearForBackdropFill(painter, widget, option->rect, surface))
                painter->fillRect(option->rect, surface);
            return true;
        }
        // Inline lane under a granted backdrop must read the same
        // single-composite ink as the day cells: Source-fill the resolved bar
        // surface instead of layering a second SourceOver fill over the
        // calendar fill beneath (which drifts toward opaque). Refused and
        // offscreen fallbacks keep the plain fill.
        const QColor lane = widget->palette().color(QPalette::Window);
        if (!clearForBackdropFill(painter, widget, option->rect, lane))
            painter->fillRect(option->rect, lane);
        return true;
    }

    if (isCardViewport(widget) && eraseForBackdrop(painter, widget, option->rect)) {
        // In-card scroll viewport (reviewer 2026-10-06 tree-selection
        // overlay): the viewport's auto-filled Base layer would double over
        // the card veil, so the sync stops the autofill and the viewport
        // repaints the card tone itself here — erase plus Source replace
        // (exact tone, never layered, smear-safe on scroll exposes).
        // Roles stay intact for tone reads; bars and plain containers keep
        // their own paths. Must precede the card-interior guard below,
        // which would otherwise return without painting (stale blits).
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_Source);
        painter->fillRect(option->rect, veiledCard(tokens(option->palette).layer, true));
        painter->restore();
        return true;
    }

    if (insideOpaqueCard(widget)) {
        // Card interiors own their surface: the card already painted its
        // veiled fill (CC_GroupBox / PE_FrameTabWidget / the SettingsCard
        // frame), and Qt dispatches PE_Widget here for any child with
        // WA_StyledBackground. The Source-clear below would punch that fill
        // to alpha 0 (the Dialogs-page holes; same guard as the slider and
        // scrollbar erases). Return without painting so the card fill the
        // parent already laid survives, and so this skipped primitive never
        // falls through to the coveredPrimitive assert.
        return true;
    }

    if (eraseForBackdrop(painter, widget, option->rect, ControlRadius)) {
        // erase is disabled here (StyledBackground, no autofill) and neither
        // the island nor its descendants repaint fully on scroll/hover/page
        // switches, so shifted frames accumulate as permanent smear until a
        // resize reallocates the buffer. DWM composites the material
        // underneath. The island itself (zero-alpha Window role) clears and
        // returns; descendants clear the same way and fall through so their
        // content paints over the freshly erased rect (the island has no
        // fill to punch through). Unrelated widgets keep Qt's default erase
        // path: the paintsDirectlyOnBackdrop gate already excluded opaque
        // islands and non-Composited fallbacks, so this branch never runs
        // for them.
        if (widget->palette().color(QPalette::Window).alpha() == 0)
            return true;
        if (widget->window()->windowType() == Qt::Popup) {
            // Recipe 1 completion for a granted popup backdrop: PE_PanelMenu
            // has already laid the ONE popup tint over the DWM acrylic
            // (preparePopupSurface re-tints Window/Base to it, 178 Dark /
            // 242 Light) and this erase just Source-cleared a child rect
            // inside it. Without the refill the view/viewport tail stays
            // alpha 0 — the fully transparent band behind the last
            // combo-popup rows. Source-fill the SAME tint, clipped to the
            // popup's rounded surface (OverlayRadius, mapped into widget
            // coordinates) so the corner cutouts outside the flyout keep
            // their transparency, then return; NONE never reaches this
            // branch because the gate is false without a granted backdrop.
            const QColor tint = widget->palette().color(QPalette::Window);
            const QWidget *popup = widget->window();
            const QPoint origin = widget->mapTo(popup, QPoint(0, 0));
            const QRect surface(-origin, popup->size());
            painter->save();
            QPainterPath clip;
            clip.addRoundedRect(QRectF(surface), OverlayRadius, OverlayRadius);
            // Intersect, never replace: dirty-region clips arrive already set.
            painter->setClipPath(clip, Qt::IntersectClip);
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(option->rect, tint);
            painter->restore();
            return true;
        }
    }
    return false;
}

bool drawChromePrimitive(const Style *, QStyle::PrimitiveElement element,
                         const QStyleOption *option, QPainter *painter, const QWidget *widget)
{
    if (!option || !painter)
        return false;
    if (element != QStyle::PE_FrameFocusRect && element != QStyle::PE_PanelTipLabel
        && element != QStyle::PE_PanelStatusBar && element != QStyle::PE_IndicatorArrowDown
        && element != QStyle::PE_IndicatorArrowLeft && element != QStyle::PE_IndicatorArrowRight
        && element != QStyle::PE_IndicatorArrowUp && element != QStyle::PE_Frame
        && element != QStyle::PE_IndicatorToolBarSeparator)
        return false;
    using namespace WinUI3::Private;
    const WinUI3::Private::Tokens t = tokens(option->palette);
    const bool enabled = option->state & QStyle::State_Enabled;

    if (element == QStyle::PE_FrameFocusRect) {
        if (!keyboardFocusVisible(widget))
            return true;
        const qreal focus =
                progress(widget, focusProperty, option->state & QStyle::State_HasFocus ? 1.0 : 0.0);
        if (focus <= 0.01)
            return true;
        QColor outer = t.focusOuter;
        QColor inner = t.focusInner;
        outer.setAlphaF(outer.alphaF() * focus);
        inner.setAlphaF(inner.alphaF() * focus);
        paintFocusRing(painter, option->rect, outer, inner, 1, 3, ControlRadius + 2, ControlRadius);
        return true;
    }

    if (element == QStyle::PE_PanelTipLabel) {
        const QColor fill = t.tooltipFill;
        roundedRect(painter, QRectF(option->rect).adjusted(1, 1, -1, -1), fill, t.tooltipStroke, 4);
        return true;
    }

    if (element == QStyle::PE_PanelStatusBar) {
        // Recipe 2 (mirror PE_PanelMenuBar): reveal the live material under
        // a composited backdrop. Erase first; on a direct backdrop paint
        // only the 1px top stroke and return transparent with no Window
        // fill. The opaque fill below is the non-Composited fallback.
        if (eraseForBackdrop(painter, widget, option->rect)) {
            painter->setPen(QPen(t.stroke, 1));
            painter->drawLine(option->rect.topLeft(), option->rect.topRight());
            return true;
        }
        painter->fillRect(option->rect, option->palette.color(QPalette::Window));
        painter->setPen(QPen(t.stroke, 1));
        painter->drawLine(option->rect.topLeft(), option->rect.topRight());
        return true;
    }

    if (element == QStyle::PE_IndicatorArrowDown || element == QStyle::PE_IndicatorArrowLeft
        || element == QStyle::PE_IndicatorArrowRight || element == QStyle::PE_IndicatorArrowUp) {
        WinUI3::icon(arrowIcon(element), enabled ? t.textPrimary : t.textDisabled)
                .paint(painter, option->rect, Qt::AlignCenter,
                       enabled ? QIcon::Normal : QIcon::Disabled);
        return true;
    }

    if (element == QStyle::PE_Frame && widget && widget->window()
        && widget->window()->windowType() == Qt::Popup) {
        // preparePopupSurface already rebound the popup palette's Window role
        // to the raised translucent-layer stand-in color.
        const QColor popupSurface = option->palette.color(QPalette::Window);
        controlSurface(painter, option->rect, popupSurface, t.flyoutStroke, t.flyoutStroke,
                       OverlayRadius);
        return true;
    }

    if (element == QStyle::PE_Frame && qobject_cast<const QAbstractItemView *>(widget)) {
        roundedRect(painter, QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), Qt::transparent,
                    t.stroke, ControlRadius);
        return true;
    }

    if (element == QStyle::PE_Frame) {
        if (const QWidget *editor = richTextEditor(widget)) {
            const bool focused = editor->hasFocus();
            const bool editorEnabled = option->state & QStyle::State_Enabled;
            const QColor fill = !editorEnabled ? t.controlDisabled
                    : focused
                    ? t.editorFocusedFill
                    : (option->state & QStyle::State_MouseOver ? t.controlHover : t.control);
            eraseForBackdrop(painter, widget, option->rect, ControlRadius);
            controlSurface(painter, option->rect,
                           veiledCard(fill, paintsDirectlyOnBackdrop(widget)), t.stroke,
                           t.strokeSecondary, ControlRadius);
            if (focused)
                drawEditorFocusUnderline(painter, QRectF(option->rect), t.accentFill,
                                         ControlRadius);
            return true;
        }
        return false;
    }

    // PE_IndicatorToolBarSeparator
    painter->save();
    painter->setPen(QPen(t.stroke, 1));
    if (option->state & QStyle::State_Horizontal) {
        const int x = option->rect.center().x();
        painter->drawLine(x, option->rect.top() + 8, x, option->rect.bottom() - 8);
    } else {
        const int y = option->rect.center().y();
        painter->drawLine(option->rect.left() + 8, y, option->rect.right() - 8, y);
    }
    painter->restore();
    return true;
}

bool drawChromeControl(const Style *style, QStyle::ControlElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget)
{
    if (!option || !painter)
        return false;
    if (element != QStyle::CE_ShapedFrame && element != QStyle::CE_ComboBoxLabel
        && element != QStyle::CE_ProgressBar && element != QStyle::CE_ProgressBarGroove
        && element != QStyle::CE_ProgressBarContents && element != QStyle::CE_ProgressBarLabel
        && element != QStyle::CE_ToolBar && element != QStyle::CE_SizeGrip)
        return false;
    using namespace WinUI3::Private;
    const Tokens t = tokens(option->palette);

    if (element == QStyle::CE_ShapedFrame && widget
        && widget->property(WinUI3::Style::SettingsCardProperty).toBool()) {
        // Card-interactive iff an expandable child is bound: trailing-only
        // cards (Notifications/Updates) keep the resting control fill and
        // never show hover/press. The expandable child lives under the
        // _winui_settings_card_expandableHost, so presence of that host's
        // layout marks interactivity without including settingscard.h here.
        const QWidget *host =
                widget->findChild<QWidget *>(QStringLiteral("_winui_settings_card_expandableHost"));
        const bool interactive = host && host->layout() && host->layout()->count() > 0;
        const qreal hover = interactive
                ? progress(widget, hoverProperty,
                           option->state & QStyle::State_MouseOver ? 1.0 : 0.0)
                : 0.0;
        const qreal press = interactive
                ? progress(widget, pressProperty, option->state & QStyle::State_Sunken ? 1.0 : 0.0)
                : 0.0;
        QColor fill = mix(t.control, t.controlHover, hover * (1.0 - press));
        fill = mix(fill, t.controlPressed, press);
        // Cards veil over a live material (VeiledCardAlpha, same recipe as
        // CC_GroupBox): an opaque card bands over Mica. Erase first so
        // hover frames cannot accumulate, then veil; off-material the fill
        // stays opaque as before.
        eraseForBackdrop(painter, widget, option->rect, OverlayRadius);
        if (paintsDirectlyOnBackdrop(widget))
            fill.setAlpha(qMin(fill.alpha(), VeiledCardAlpha));
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary, OverlayRadius);
        if (keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(2, 2, -3, -3), 6, 6);
            painter->setPen(QPen(t.focusInner, 1.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(4, 4, -5, -5), 5, 5);
            painter->restore();
        }
        return true;
    }

    if (element == QStyle::CE_ComboBoxLabel) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (combo->editable)
                return true;
            QRect content = style->subControlRect(QStyle::CC_ComboBox, combo,
                                                  QStyle::SC_ComboBoxEditField, widget);
            if (!combo->currentIcon.isNull()) {
                const QSize iconSize = combo->iconSize.isValid() ? combo->iconSize : QSize(16, 16);
                const QRect logicalIcon(content.left(),
                                        content.center().y() - iconSize.height() / 2,
                                        iconSize.width(), iconSize.height());
                const QRect iconRect = QStyle::visualRect(option->direction, content, logicalIcon);
                paintThemedIcon(
                        painter, combo->currentIcon, iconRect, Qt::AlignCenter,
                        option->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled,
                        option->state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled);
                if (option->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            painter->setPen(option->state & QStyle::State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(
                    content,
                    QStyle::visualAlignment(option->direction, Qt::AlignLeft | Qt::AlignVCenter),
                    option->fontMetrics.elidedText(combo->currentText, Qt::ElideRight,
                                                   content.width()));
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_ProgressBar) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            style->drawControl(QStyle::CE_ProgressBarGroove, bar, painter, widget);
            style->drawControl(QStyle::CE_ProgressBarContents, bar, painter, widget);
            if (bar->textVisible)
                style->drawControl(QStyle::CE_ProgressBarLabel, bar, painter, widget);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_ProgressBarGroove) {
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
        const bool horizontal = !progressBar || progressBar->orientation() == Qt::Horizontal;
        const bool textAboveLine = horizontal && bar && bar->textVisible && !bar->text.isEmpty();
        const QRect groove = horizontal
                ? (textAboveLine
                           // The label owns the bar's body; the track becomes a thin
                           // underline so the text never overlaps the fill.
                           ? QRect(option->rect.left(), option->rect.bottom() - 2,
                                   option->rect.width(), 3)
                           : QRect(option->rect.left(), option->rect.center().y() - 2,
                                   option->rect.width(), 4))
                : QRect(option->rect.center().x() - 2, option->rect.top(), 4,
                        option->rect.height());
        roundedRect(painter, groove, t.stroke, Qt::transparent, 2);
        return true;
    }

    if (element == QStyle::CE_ProgressBarContents) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
            const bool horizontal = !progressBar || progressBar->orientation() == Qt::Horizontal;
            const bool inverted =
                    progressBar ? progressBar->invertedAppearance() : bar->invertedAppearance;
            const bool textAboveLine =
                    horizontal && bar && bar->textVisible && !bar->text.isEmpty();
            const QRect track = horizontal
                    ? (textAboveLine ? QRect(option->rect.left(), option->rect.bottom() - 2,
                                             option->rect.width(), 3)
                                     : QRect(option->rect.left(), option->rect.center().y() - 2,
                                             option->rect.width(), 4))
                    : QRect(option->rect.center().x() - 2, option->rect.top(), 4,
                            option->rect.height());
            const QColor indicatorColor =
                    bar->state & QStyle::State_Enabled ? t.accentFill : t.accentFillDisabled;
            if (bar->minimum == 0 && bar->maximum == 0) {
                // ProgressRing-style indeterminate progress uses two
                // independently moving indicators.  The phase is advanced
                // by a widget-owned timer, so every repaint is intentional
                // and capture mode can freeze it at a stable value.
                const qreal phase = Style::animationsAllowed()
                        ? qBound<qreal>(0.0, progress(widget, progressPhaseProperty, 0.0), 1.0)
                        : 0.35;
                const int axis = horizontal ? track.width() : track.height();
                const int firstLength = qMax(12, axis / 4);
                const int secondLength = qMax(10, axis / 6);
                const bool reverse = horizontal ? (inverted != (bar->direction == Qt::RightToLeft))
                                                : !bar->invertedAppearance;
                const auto drawIndicator = [&](int length, qreal offset) {
                    const qreal travel = axis + length;
                    const int distance = qRound(travel * std::fmod(phase + offset, 1.0));
                    QRect indicator = track;
                    if (horizontal) {
                        const int left = reverse ? axis - distance : distance - length;
                        indicator.setLeft(track.left() + left);
                        indicator.setWidth(length);
                    } else {
                        const int top = reverse ? axis - distance : distance - length;
                        indicator.setTop(track.top() + top);
                        indicator.setHeight(length);
                    }
                    indicator = indicator.intersected(track);
                    if (!indicator.isEmpty())
                        roundedRect(painter, indicator, indicatorColor, Qt::transparent, 2);
                };
                drawIndicator(firstLength, 0.0);
                drawIndicator(secondLength, 0.5);
                return true;
            }
            const qint64 range = qint64(bar->maximum) - qint64(bar->minimum);
            const qint64 value = qint64(bar->progress) - qint64(bar->minimum);
            const qreal ratio = range > 0 ? qBound<qreal>(0, qreal(value) / qreal(range), 1) : 0;
            QRect fill = track;
            if (horizontal) {
                const int length = qRound(track.width() * ratio);
                if (inverted != (bar->direction == Qt::RightToLeft))
                    fill.setLeft(track.right() - length + 1);
                else
                    fill.setWidth(length);
            } else {
                // Qt fills a vertical, non-inverted bar from the bottom up
                // (QCommonStyle flips "reverse" for vertical orientation), so
                // invertedAppearance must grow from the top instead.
                const int length = qRound(track.height() * ratio);
                if (inverted)
                    fill.setHeight(length);
                else
                    fill.setTop(track.bottom() - length + 1);
            }
            if (!fill.isEmpty())
                roundedRect(painter, fill, indicatorColor, Qt::transparent, 2);
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_ProgressBarLabel) {
        // WinUI's ProgressBar shows no inline percentage text, but Qt-based
        // applications may render meaningful information there with
        // textVisible=true. Keep their label: paint
        // it centered over the bar using the style's text color.
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            if (bar->textVisible && !bar->text.isEmpty()) {
                painter->save();
                const QPalette &pal = option->palette;
                const QPalette::ColorGroup group = !(bar->state & QStyle::State_Enabled)
                        ? QPalette::Disabled
                        : (bar->state & QStyle::State_Active ? QPalette::Active
                                                             : QPalette::Inactive);
                const QColor fg = pal.color(group, QPalette::WindowText);
                painter->setPen(QPen(fg));
                const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
                const bool horizontal =
                        !progressBar || progressBar->orientation() == Qt::Horizontal;
                if (horizontal) {
                    // The thin track sits at the bottom of the rect; the label
                    // owns the area above it.  Elide so long transfer strings
                    // (e.g. "2,513,696 (22.0%)") never bleed past the bar.
                    const QRect labelRect = bar->rect.adjusted(0, 0, 0, -6);
                    painter->drawText(labelRect, Qt::AlignCenter,
                                      option->fontMetrics.elidedText(bar->text, Qt::ElideRight,
                                                                     labelRect.width()));
                } else {
                    painter->drawText(bar->rect, Qt::AlignCenter, bar->text);
                }
                painter->restore();
            }
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_ToolBar) {
        // Recipe 2 (mirror PE_PanelMenuBar): on a direct backdrop the erase
        // reveals the live material, so return with no fill. The
        // SurfaceProperty-gated Window fill below is the non-Composited
        // fallback.
        if (eraseForBackdrop(painter, widget, option->rect))
            return true;
        if (widget && widget->property(Style::SurfaceProperty).isValid())
            painter->fillRect(option->rect, widget->palette().color(QPalette::Window));
        return true;
    }

    // CE_SizeGrip. Reached only for that element: every branch above returns
    // on match (or false on a failed option cast), so unlike the pre-S1
    // facade chain of separate `if (element == …)` blocks, nothing else may
    // fall through here. An unguarded tail once painted these dots on every
    // plain QFrame (CE_ShapedFrame without the settings-card property).
    // CE_SizeGrip. Reached only for that element: every branch above returns
    // on match (or false on a failed option cast), so unlike the pre-S1
    // facade chain of separate `if (element == …)` blocks, nothing else may
    // fall through here. An unguarded tail once painted these dots on every
    // plain QFrame (CE_ShapedFrame without the settings-card property).
    if (element != QStyle::CE_SizeGrip)
        return false;
    const auto *grip = qstyleoption_cast<const QStyleOptionSizeGrip *>(option);
    const Qt::Corner corner = grip ? grip->corner : Qt::BottomRightCorner;
    const bool right = corner == Qt::TopRightCorner || corner == Qt::BottomRightCorner;
    const bool bottom = corner == Qt::BottomLeftCorner || corner == Qt::BottomRightCorner;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(t.textTertiary);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column <= row; ++column) {
            const int dx = 3 + column * 4;
            const int dy = 3 + row * 4;
            const qreal x = right ? option->rect.right() - dx : option->rect.left() + dx;
            const qreal y = bottom ? option->rect.bottom() - dy : option->rect.top() + dy;
            painter->drawEllipse(QPointF(x, y), 1.0, 1.0);
        }
    }
    painter->restore();
    return true;
}

// Item-view row: calendar day circles, popup pills (combo / menu / AutoSuggest),
// the selection check and the accent marker. Split out of drawViewPrimitive so
// the row contract is readable (and testable) on its own.
static bool drawItemViewItemPrimitive(const QStyleOption *option, QPainter *painter,
                                      const QWidget *widget)
{
    const auto *viewOption = qstyleoption_cast<const QStyleOptionViewItem *>(option);
    const QAbstractItemView *view = itemView(widget);
    const bool calendar = calendarPopupView(widget);
    const bool popup =
            widget && widget->window() && widget->window()->windowType() == Qt::Popup && !calendar;
    const auto *popupCombo = popup && widget->window()->parentWidget()
            ? qobject_cast<const QComboBox *>(widget->window()->parentWidget())
            : nullptr;
    const bool comboPopup = popupCombo;
    const bool autoSuggestPopup = popup && completerPopupView(view);
    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & QStyle::State_Enabled;
    const bool tree = qobject_cast<const QTreeView *>(view);
    const bool table = qobject_cast<const QTableView *>(view);
    const bool selected = option->state & QStyle::State_Selected;
    const bool hovered = option->state & QStyle::State_MouseOver;
    const bool pressedItem = hovered && (option->state & QStyle::State_Sunken);
    // Keyboard current is hover: arrows move view->currentIndex() with no
    // MouseOver, and QCompleter drives selection the same way. The
    // current row must read exactly like the hovered row.
    const bool keyboardCurrent = popup && view && viewOption && viewOption->index.isValid()
            && viewOption->index == view->currentIndex();
    const bool calendarHeader =
            calendar && viewOption && viewOption->index.isValid() && viewOption->index.row() == 0;
    if (calendar) {
        if (calendarHeader)
            return true;
        QColor calendarFill = Qt::transparent;
        if (selected)
            calendarFill = enabled ? t.accentFill : t.accentFillDisabled;
        else if (pressedItem)
            calendarFill = t.subtlePressed;
        else if (hovered)
            calendarFill = t.subtleHover;
        if (calendarFill.alpha() > 0) {
            const int side = qMin(32, qMin(option->rect.width() - 4, option->rect.height() - 4));
            const QRectF circle(option->rect.center().x() - side / 2.0,
                                option->rect.center().y() - side / 2.0, side, side);
            roundedRect(painter, circle, calendarFill, Qt::transparent, side / 2.0);
        }
        return true;
    }
    QColor fill = Qt::transparent;
    if (pressedItem)
        fill = t.subtlePressed;
    else if (selected && hovered)
        fill = t.subtlePressed;
    else if (selected || hovered || keyboardCurrent)
        fill = t.subtleHover;

    // Pointer events and animation invalidation belong to the viewport.
    // Delegates may invoke the style with either the view or its viewport,
    // so resolve the actual event surface explicitly.
    const QWidget *popupInteractionSurface =
            popupCombo && popupCombo->view() && popupCombo->view()->viewport()
            ? popupCombo->view()->viewport()
            : view && view->viewport() ? view->viewport()
                                       : widget;
    const qreal popupPress = comboPopup
            ? progress(popupInteractionSurface, pressProperty, pressedItem ? 1.0 : 0.0)
            : 0.0;
    // ComboBoxItem LayoutRoot carries the official Margin="5,2,5,2",
    // so the PointerOver/Selected SubtleFill sits 5px inside the row
    // on both sides. Plain MenuFlyoutItem uses Margin="4,2,4,2".
    // Shared with the CE_MenuItem path via popupRowPillRect so the two
    // popup renderers cannot drift apart. Non-popup rows keep their own
    // tree/table/plain geometry untouched.
    QRectF itemRect = popup ? popupRowPillRect(option->rect, comboPopup)
            : tree          ? QRectF(option->rect).adjusted(4, 2, -4, -2)
            : table         ? QRectF(option->rect)
                            : QRectF(option->rect).adjusted(2, 1, -2, -1);
    if (fill.alpha() > 0) {
        // AutoSuggest rows must keep the opaque CE frame under the shared
        // hover pill. Other popup types retain their material erase recipe.
        if (autoSuggestPopup) {
            // Base already owns the resolved flyout RGB. Normalize only
            // alpha: resolving popupSurfaceColor again would lift it twice.
            painter->save();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(option->rect, withAlpha(option->palette.color(QPalette::Base), 255));
            painter->restore();
        } else {
            // Erase-then-rebuild in cards (reviewer 2026-10-06
            // tree-selection overlay): the erase above cleared the card
            // fill across the full row rect but the pill below only
            // covers an inset rounded rect — corners and inter-row seams
            // would read alpha 0 as visible edges. Rebuild the veiled
            // card tone first (the view Base role is never
            // transparentized, only Window is, so the tone is intact),
            // then lay the pill over it. Outside cards the bare erase
            // stands (recipe-2 material reveal).
            eraseForBackdropWithCardRefill(painter, widget, option->rect,
                                           veiledCard(t.layer, true));
        }
        paintPopupRowPill(painter, itemRect, fill);
    }
    const bool firstColumn =
            !viewOption || !viewOption->index.isValid() || viewOption->index.column() == 0;
    if (selected && comboPopup && firstColumn) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(enabled ? t.selectionAccent : t.accentFillDisabled);
        // WinUI's selected-item pill compresses to 62.5% while the
        // pointer is held. The interaction controller uses the same
        // 167 ms transition as the current DropdownContent template.
        const qreal indicatorWidth = 3.0;
        const qreal indicatorHeight = 16.0 * (1.0 - 0.375 * popupPress);
        const qreal indicatorX = option->direction == Qt::RightToLeft
                ? itemRect.right() - indicatorWidth
                : itemRect.left();
        // The pill is aligned to the content slot created by the
        // ComboBoxItem's 5px/7px vertical padding.  At the normal 16px
        // height this is visually the same center as the row, while the
        // explicit slot keeps the compressed 0.625 state deterministic
        // at fractional device-pixel ratios.
        const QRectF itemContent =
                itemRect.adjusted(comboPopupItemPaddingLeft, comboPopupItemPaddingTop,
                                  -comboPopupItemPaddingRight, -comboPopupItemPaddingBottom);
        painter->drawRoundedRect(QRectF(indicatorX,
                                        itemContent.center().y() - indicatorHeight / 2.0,
                                        indicatorWidth, indicatorHeight),
                                 1.5, 1.5);
        painter->restore();
    } else if (selected && popup && !autoSuggestPopup && firstColumn) {
        const QRect checkRect = QStyle::visualRect(
                option->direction, option->rect,
                QRect(option->rect.left() + 12, option->rect.center().y() - 8, 16, 16));
        icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled)
                .paint(painter, checkRect, Qt::AlignCenter,
                       enabled ? QIcon::Normal : QIcon::Disabled);
    } else if (selected && viewOption && selectionMarkerView(widget) && firstColumn) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(enabled ? t.selectionAccent : t.accentFillDisabled);
        const QRectF indicator = selectionMarkerRect(*viewOption, view);
        painter->drawRoundedRect(indicator, 1.5, 1.5);
        painter->restore();
    }
    return true;
}

bool drawViewPrimitive(const Style *style, QStyle::PrimitiveElement element,
                       const QStyleOption *option, QPainter *painter, const QWidget *widget)
{
    if (element == QStyle::PE_FrameTabBarBase) {
        const Tokens t = tokens(option->palette);
        painter->setPen(t.strokeSecondary);
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        return true;
    }

    if (element == QStyle::PE_FrameTabWidget) {
        const Tokens t = tokens(option->palette);
        if (eraseForBackdrop(painter, widget, option->rect)) {
            // Recipe 1 over a live material: the pane is a card surface
            // (QTabWidget + pages are card-gated in the sync), so erase
            // first, then lay the veiled card tone — the same
            // veiledCard(t.layer) recipe as CC_GroupBox — instead of the
            // recipe-2 Source-clear that left the Collections pane at
            // alpha 0. Keep the hairline stroke so the pane edge still
            // reads.
            roundedRect(painter, QRectF(option->rect).adjusted(0, 0, -1, -1),
                        veiledCard(t.layer, true), t.stroke, ControlRadius);
            return true;
        }
        // Off-material (None/refused/backdrop-never-granted): unchanged
        // opaque t.control pane card, byte-identical for the snapshot
        // matrix baselines.
        roundedRect(painter, QRectF(option->rect).adjusted(0, 0, -1, -1), t.control, t.stroke,
                    ControlRadius);
        return true;
    }

    if (element == QStyle::PE_PanelItemViewItem)
        return drawItemViewItemPrimitive(option, painter, widget);

    if (element == QStyle::PE_IndicatorBranch) {
        const Tokens t = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        if (!(option->state & QStyle::State_Children))
            return true;
        const bool open = option->state & QStyle::State_Open;
        const Icon glyph = open                        ? Icon::ChevronDown
                : option->direction == Qt::RightToLeft ? Icon::ChevronLeft
                                                       : Icon::ChevronRight;
        const int extent = 12;
        // Keep the disclosure glyph in the branch slot, but leave a small
        // clear separation from the selection pill painted at the viewport's
        // leading edge.  The shift mirrors for RTL and does not alter the
        // tree's hierarchy indentation.
        const int branchCenter = option->rect.center().x()
                + (option->direction == Qt::RightToLeft ? -treeBranchMarkerClearance
                                                        : treeBranchMarkerClearance);
        icon(glyph, enabled ? t.textPrimary : t.textDisabled)
                .paint(painter,
                       QRect(branchCenter - extent / 2, option->rect.center().y() - extent / 2,
                             extent, extent),
                       Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        return true;
    }

    if (element == QStyle::PE_IndicatorHeaderArrow) {
        const Tokens t = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        const Icon glyph =
                option->state & QStyle::State_UpArrow ? Icon::ChevronUp : Icon::ChevronDown;
        icon(glyph, enabled ? t.textSecondary : t.textDisabled)
                .paint(painter,
                       QRect(option->rect.center().x() - 6, option->rect.center().y() - 6, 12, 12),
                       Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        return true;
    }

    if (element == QStyle::PE_IndicatorTabClose) {
        // Qt 5 paints the closable-tab button through this primitive (Qt 6
        // maps the equivalent standard icon instead).  WinUI renders the
        // close button as a square, gently rounded surface (ControlRadius)
        // with a 12px X: subtle fill on hover, pressed lift.
        const Tokens t = tokens(option->palette);
        const bool enabled = option->state & QStyle::State_Enabled;
        const bool hovered = option->state & QStyle::State_MouseOver;
        const bool pressed = option->state & QStyle::State_Sunken;
        // The hit area is the full 22x22 indicator slot; fill it entirely,
        // like WinUI's TabViewItemCloseButton.
        const QRect button = QStyle::visualRect(option->direction, option->rect, option->rect);
        if (pressed && enabled) {
            roundedRect(painter, button, t.subtlePressed, Qt::transparent, ControlRadius);
        } else if (hovered && enabled) {
            roundedRect(painter, button, t.subtleHover, Qt::transparent, ControlRadius);
        }
        icon(Icon::Close,
             enabled ? (hovered || pressed ? t.textPrimary : t.textSecondary) : t.textDisabled)
                .paint(painter, button.adjusted(5, 5, -5, -5), Qt::AlignCenter,
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

// Item-view delegate row (CE_ItemViewItem): background, check/roles, text and
// decoration for list/tree/table rows. Split out of drawViewControl.
static bool drawItemViewItemControl(const Style *style, const QStyleOption *option,
                                    QPainter *painter, const QWidget *widget,
                                    const TableEditorOverlap &tableEditorOverlap)
{
    if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        const bool calendar = calendarPopupView(widget);
        const QAbstractItemView *view = itemView(widget);
        const bool popup = widget && widget->window() && widget->window()->windowType() == Qt::Popup
                && !calendar;
        const bool comboPopup = popup && widget->window()->parentWidget()
                && qobject_cast<const QComboBox *>(widget->window()->parentWidget());
        const bool autoSuggestPopup = popup && completerPopupView(view);
        if (calendar || autoSuggestPopup) {
            // Source replaces retained row pixels; SourceOver would blend
            // with the previous frame. AutoSuggest Base already contains
            // the resolved flyout RGB, even when its material tint has
            // alpha. Match PE's opaque frame without a second color lift.
            // Calendar keeps its own Base and material policy unchanged.
            QColor rowBase = source->palette.color(QPalette::Base);
            if (autoSuggestPopup)
                rowBase = withAlpha(rowBase, 255);
            painter->save();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            painter->fillRect(source->rect, rowBase);
            painter->restore();
        } else if (source->backgroundBrush.style() != Qt::NoBrush) {
            painter->fillRect(source->rect, source->backgroundBrush);
        } else if (!calendar && source->features & QStyleOptionViewItem::Alternate) {
            eraseForBackdrop(painter, widget, source->rect);
            painter->fillRect(source->rect,
                              veiledCard(source->palette.color(QPalette::AlternateBase),
                                         paintsDirectlyOnBackdrop(widget)));
        }
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, source, painter, widget);
        const auto *tableView = qobject_cast<const QTableView *>(view);
        const Tokens t = tokens(option->palette);
        const bool enabled = source->state & QStyle::State_Enabled;
        const bool calendarHeader = calendar && source->index.isValid() && source->index.row() == 0;
        const bool checkedPopupSelection = popup && !comboPopup && !autoSuggestPopup
                && (source->state & QStyle::State_Selected);

        if (source->features & QStyleOptionViewItem::HasCheckIndicator) {
            QStyleOptionButton check;
            check.rect =
                    style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, source, widget);
            check.palette = source->palette;
            check.state = source->state;
            check.state.setFlag(QStyle::State_Selected, false);
            check.state.setFlag(QStyle::State_On, source->checkState == Qt::Checked);
            check.state.setFlag(QStyle::State_NoChange, source->checkState == Qt::PartiallyChecked);
            check.state.setFlag(QStyle::State_Off, source->checkState == Qt::Unchecked);
            style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &check, painter, widget);
        }

        if ((source->features & QStyleOptionViewItem::HasDecoration) && !source->icon.isNull()
            && !checkedPopupSelection) {
            const QRect decoration =
                    style->subElementRect(QStyle::SE_ItemViewItemDecoration, source, widget);
            paintThemedIcon(painter, source->icon, decoration, source->decorationAlignment,
                            enabled ? t.textPrimary : t.textDisabled,
                            enabled ? QIcon::Normal : QIcon::Disabled,
                            source->state & QStyle::State_Selected ? QIcon::On : QIcon::Off);
        }

        const bool tableEditing = tableView && tableEditorOverlap
                && tableEditorOverlap(tableView, source->index, source->rect);
        if ((source->features & QStyleOptionViewItem::HasDisplay) && !tableEditing) {
            const bool hasLeadingContent = (source->features & QStyleOptionViewItem::HasDecoration)
                    || (source->features & QStyleOptionViewItem::HasCheckIndicator);
            // The leading 42px reservation must follow the reading
            // direction, otherwise RTL popup text overlaps the icon and
            // marker zone painted on the right edge.
            const int leading = (comboPopup || autoSuggestPopup) && !hasLeadingContent
                    ? comboPopupItemPaddingLeft + 5
                    : 42;
            const int trailing = (comboPopup || autoSuggestPopup) && !hasLeadingContent
                    ? comboPopupItemPaddingRight + 5
                    : 12;
            QRect textRect = popup
                    ? QStyle::visualRect(source->direction, source->rect,
                                         source->rect.adjusted(leading, 0, -trailing, 0))
                    : style->subElementRect(QStyle::SE_ItemViewItemText, source, widget);
            painter->save();
            painter->setFont(source->font);
            QColor textColor = enabled ? t.textPrimary : t.textDisabled;
            if (calendarHeader) {
                textColor = enabled ? t.textSecondary : t.textDisabled;
            } else if (calendar && (source->state & QStyle::State_Selected)) {
                textColor = enabled ? t.textOnAccentPrimary : t.controlOnAccentDisabled;
            } else if (calendar && source->index.isValid()) {
                // QCalendarWidget injects platform-specific weekday
                // colors (notably red weekends). WinUI uses the normal
                // foreground and only dims days outside the shown month.
                bool numberOk = false;
                const int day = source->text.toInt(&numberOk);
                const int row = source->index.row();
                const bool outsideMonth =
                        numberOk && ((row == 1 && day > 7) || (row >= 5 && day < 15));
                if (outsideMonth)
                    textColor = t.textDisabled;
            }
            painter->setPen(textColor);
            Qt::Alignment alignment = Qt::Alignment(source->displayAlignment) | Qt::AlignVCenter;
            alignment = QStyle::visualAlignment(source->direction, alignment);
            const bool wraps = source->features & QStyleOptionViewItem::WrapText;
            const int textFlags =
                    int(alignment) | (wraps ? int(Qt::TextWordWrap) : int(Qt::TextSingleLine));
            const QString text = wraps
                    ? source->text
                    : source->fontMetrics.elidedText(source->text, source->textElideMode,
                                                     textRect.width());
            painter->drawText(textRect, textFlags, text);
            painter->restore();
        }

        if ((source->state & QStyle::State_HasFocus) && keyboardFocusVisible(view)) {
            const bool tree = qobject_cast<const QTreeView *>(view);
            const bool table = qobject_cast<const QTableView *>(view);
            const QRectF focusRect = tree ? QRectF(source->rect).adjusted(4, 2, -4, -2)
                    : table               ? QRectF(source->rect).adjusted(1, 1, -1, -1)
                                          : QRectF(source->rect).adjusted(2, 1, -2, -1);
            paintFocusRing(painter, focusRect, t.focusOuter, t.focusInner, 1, 3, table ? 0.0 : 5.0,
                           table ? 0.0 : 3.0);
        }
        return true;
    }
    return false;
}

bool drawViewControl(const Style *style, QStyle::ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget,
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

    if (element == QStyle::CE_ItemViewItem)
        return drawItemViewItemControl(style, option, painter, widget, tableEditorOverlap);

    if (element == QStyle::CE_Splitter) {
        const Tokens t = tokens(option->palette);
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & QStyle::State_MouseOver ? 1.0 : 0.0);
        const qreal press =
                progress(widget, pressProperty, option->state & QStyle::State_Sunken ? 1.0 : 0.0);
        QColor color = mix(t.stroke, t.strokeStrong, hover);
        color = mix(color, t.accentFill, press);
        const qreal thickness = 1.0 + hover + press;
        const bool horizontal = option->state & QStyle::State_Horizontal;
        QRectF handle;
        if (horizontal) {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.height() - 12));
            const QRectF splitterRect(option->rect);
            handle = QRectF(splitterRect.center().x() - thickness / 2.0,
                            splitterRect.center().y() - length / 2.0, thickness, length);
        } else {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.width() - 12));
            const QRectF splitterRect(option->rect);
            handle = QRectF(splitterRect.center().x() - length / 2.0,
                            splitterRect.center().y() - thickness / 2.0, length, thickness);
        }
        handle = snappedSplitterGrip(handle, horizontal, painter);
        roundedRect(painter, handle, color, Qt::transparent, thickness / 2.0);
        return true;
    }

    if (element == QStyle::CE_DockWidgetTitle) {
        if (const auto *dock = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            const Tokens t = tokens(option->palette);
            eraseForBackdrop(painter, widget, dock->rect);
            painter->save();
            // Veil over a live material (VeiledCardAlpha, same recipe as
            // CC_GroupBox): erase-then-opaque still bands over Mica.
            painter->fillRect(dock->rect, veiledCard(t.layer, paintsDirectlyOnBackdrop(widget)));
            painter->setPen(t.stroke);
            if (dock->verticalTitleBar)
                painter->drawLine(dock->rect.topRight(), dock->rect.bottomRight());
            else
                painter->drawLine(dock->rect.bottomLeft(), dock->rect.bottomRight());

            QRect titleRect =
                    style->subElementRect(QStyle::SE_DockWidgetTitleBarText, dock, widget);
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
            painter->drawText(
                    titleRect,
                    QStyle::visualAlignment(dock->direction, Qt::AlignLeft | Qt::AlignVCenter),
                    painter->fontMetrics().elidedText(dock->title, Qt::ElideRight,
                                                      titleRect.width()));
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
            // Selected tab veils over a live material (same card recipe as
            // CC_GroupBox): an opaque selectedTabFill bands against the
            // transparentized pane under Mica. Off-material it stays opaque.
            const QColor selectedFill =
                    veiledCard(t.selectedTabFill, paintsDirectlyOnBackdrop(widget));
            const QRectF rect = QRectF(option->rect).adjusted(1, 1, -1, 0);
            const qreal radius = 8.0;
            QPainterPath surface;
            surface.moveTo(rect.left(), rect.bottom());
            surface.lineTo(rect.left(), rect.top() + radius);
            surface.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());
            surface.lineTo(rect.right() - radius, rect.top());
            surface.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);
            surface.lineTo(rect.right(), rect.bottom());
            surface.lineTo(rect.left(), rect.bottom());
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            eraseForBackdrop(painter, widget, option->rect, radius);
            painter->fillPath(surface, selectedFill);
            QPainterPath border;
            border.moveTo(rect.left(), rect.bottom());
            border.lineTo(rect.left(), rect.top() + radius);
            border.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());
            border.lineTo(rect.right() - radius, rect.top());
            border.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);
            border.lineTo(rect.right(), rect.bottom());
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.stroke, 1));
            painter->drawPath(border);
            painter->restore();
        } else {
            // Unselected pill veils its hover fill over a live material so
            // it cannot band or ghost; rest stays transparent.
            eraseForBackdrop(painter, widget, option->rect, ControlRadius);
            QColor fill = Qt::transparent;
            if (option->state & QStyle::State_Sunken)
                fill = veiledCard(t.controlPressed, paintsDirectlyOnBackdrop(widget));
            else if (option->state & QStyle::State_MouseOver)
                fill = veiledCard(t.layer, paintsDirectlyOnBackdrop(widget));
            roundedRect(painter, QRectF(option->rect).adjusted(2, 2, -2, -2), fill, Qt::transparent,
                        ControlRadius);
            const bool nextTabSelected =
                    tab && tab->selectedPosition == QStyleOptionTab::NextIsSelected;
            if (!selected && !nextTabSelected && !(option->state & QStyle::State_MouseOver)) {
                painter->setPen(t.stroke);
                const int separatorX = option->direction == Qt::RightToLeft ? option->rect.left()
                                                                            : option->rect.right();
                painter->drawLine(separatorX, option->rect.top() + 8, separatorX,
                                  option->rect.bottom() - 8);
            }
        }
        if ((option->state & QStyle::State_HasFocus) && keyboardFocusVisible(widget)) {
            paintFocusRing(painter, QRectF(option->rect), t.focusOuter, t.focusInner, 3, 5,
                           ControlRadius, ControlRadius - 1);
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
            QFont font = controlLabelFont(widget);
            font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
            painter->setFont(font);
            painter->setPen(enabled ? (selected ? t.textPrimary : t.textSecondary)
                                    : t.textDisabled);
            if (!tab->icon.isNull()) {
                const QRect logicalIcon(textRect.left(), textRect.center().y() - 8, 16, 16);
                const QRect iconRect = QStyle::visualRect(tab->direction, tab->rect, logicalIcon);
                paintThemedIcon(painter, tab->icon, iconRect, Qt::AlignCenter,
                                enabled ? t.textPrimary : t.textDisabled,
                                enabled ? QIcon::Normal : QIcon::Disabled,
                                selected ? QIcon::On : QIcon::Off);
                if (tab->direction == Qt::RightToLeft)
                    textRect.setRight(iconRect.left() - 10);
                else
                    textRect.setLeft(iconRect.right() + 10);
            }
            painter->drawText(
                    textRect,
                    QStyle::visualAlignment(tab->direction, Qt::AlignLeft | Qt::AlignVCenter),
                    painter->fontMetrics().elidedText(tab->text, Qt::ElideRight, textRect.width()));
            painter->restore();
            return true;
        }
        return false;
    }

    if (element == QStyle::CE_HeaderSection) {
        const Tokens t = tokens(option->palette);
        eraseForBackdrop(painter, widget, option->rect);
        // Rest veils over a live material; hover/pressed are already
        // translucent subtle fills and stay as-is.
        QColor fill = veiledCard(t.layer, paintsDirectlyOnBackdrop(widget));
        if (option->state & QStyle::State_Sunken)
            fill = t.subtlePressed;
        else if (option->state & QStyle::State_MouseOver)
            fill = t.subtleHover;
        painter->fillRect(option->rect, fill);
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        const int separatorX =
                option->direction == Qt::RightToLeft ? option->rect.left() : option->rect.right();
        painter->drawLine(separatorX, option->rect.top(), separatorX, option->rect.bottom());
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
            QFont font = controlLabelFont(widget);
            font.setWeight(QFont::DemiBold);
            painter->setFont(font);
            painter->setPen(header->state & QStyle::State_Enabled ? t.textSecondary
                                                                  : t.textDisabled);
            if (!header->icon.isNull()) {
                const QRect logicalIcon(content.left(), content.center().y() - 8, 16, 16);
                const QRect iconRect =
                        QStyle::visualRect(header->direction, header->rect, logicalIcon);
                paintThemedIcon(
                        painter, header->icon, iconRect, Qt::AlignCenter,
                        header->state & QStyle::State_Enabled ? t.textSecondary : t.textDisabled,
                        header->state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled);
                if (header->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            const Qt::Alignment horizontal =
                    header->textAlignment & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter);
            painter->drawText(
                    content,
                    QStyle::visualAlignment(header->direction, horizontal | Qt::AlignVCenter)
                            | Qt::TextSingleLine,
                    painter->fontMetrics().elidedText(header->text, Qt::ElideRight,
                                                      content.width()));
            painter->restore();
            return true;
        }
        return false;
    }

    return false;
}

} // namespace WinUI3::Private
