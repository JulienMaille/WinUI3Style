// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3surfaces_p.h"

#include "winui3paint_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"
#include "winui3backdrop_p.h"

#include <winui3style/winui3style.h>
#include "winui3qtcompat_p.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCalendarWidget>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QHash>
#include <QImage>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3::Private {

bool calendarView(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

using namespace PaintPrivate;
namespace {

constexpr auto contentDialogFooterName = "_winui_content_dialog_footer_surface";
// Mica temporarily changes style-owned surfaces after polish. Its snapshot
// must not reuse the style's original* properties: those describe the state
// before polish, not the opaque state immediately before Mica was enabled.
constexpr auto backdropSurfacePaletteProperty = "_winui_backdrop_surface_palette";
constexpr auto backdropSurfacePaletteExplicitProperty = "_winui_backdrop_surface_palette_explicit";
constexpr auto backdropSurfaceAutoFillProperty = "_winui_backdrop_surface_auto_fill";
constexpr auto backdropSurfaceTranslucentProperty = "_winui_backdrop_surface_translucent";
constexpr auto backdropSurfaceNoSystemBackgroundProperty =
        "_winui_backdrop_surface_no_system_background";
constexpr auto backdropSurfaceStyledBackgroundProperty =
        "_winui_backdrop_surface_styled_background";
constexpr auto backdropSurfaceOpaquePaintProperty = "_winui_backdrop_surface_opaque_paint";

class ContentDialogFooterSurface final : public QWidget
{
public:
    explicit ContentDialogFooterSurface(QDialog *dialog) : QWidget(dialog), m_dialog(dialog)
    {
        setObjectName(QString::fromLatin1(contentDialogFooterName));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        dialog->installEventFilter(this);
        queueGeometrySync();
    }

    void setColors(const QColor &fill, const QColor &stroke)
    {
        if (m_fill == fill && m_stroke == stroke)
            return;
        m_fill = fill;
        m_stroke = stroke;
        update();
    }

    void queueGeometrySync()
    {
        if (m_syncQueued)
            return;
        m_syncQueued = true;
        QTimer::singleShot(0, this, [this] {
            m_syncQueued = false;
            syncGeometry();
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_dialog
            && (event->type() == QEvent::Resize || event->type() == QEvent::Show
                || event->type() == QEvent::LayoutRequest || event->type() == QEvent::ChildAdded)) {
            queueGeometrySync();
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), m_fill);
        painter.setPen(QPen(m_stroke, 1));
        painter.drawLine(rect().topLeft(), rect().topRight());
    }

private:
    void syncGeometry()
    {
        if (!m_dialog)
            return;
        auto *messageBox = qobject_cast<QMessageBox *>(m_dialog.data());
        if (messageBox) {
            // QMessageBox recalculates and fixes its size in showEvent after
            // the style's polish pass. Reassert the ContentDialog minimum on
            // the queued, post-show layout pass so the body is not crushed
            // into a single text line above an oversized footer.
            // Each mutator below re-triggers Resize/LayoutRequest, which
            // re-queues this sync via the event filter. Guard every write
            // by value so a converged dialog settles instead of ping-ponging
            // (a persistent <184px height would otherwise loop forever).
            // The explicit resize runs only on a pass that actually raised a
            // constraint: once the minimum is asserted the layout converges
            // on its own, and re-requesting the same height while the dialog
            // reports <184px would re-queue this sync forever.
            bool progressed = false;
            if (QLayout *layout = m_dialog->layout()) {
                if (layout->sizeConstraint() != QLayout::SetMinimumSize) {
                    layout->setSizeConstraint(QLayout::SetMinimumSize);
                    progressed = true;
                }
            }
            const QSize maxWanted(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            if (m_dialog->maximumSize() != maxWanted) {
                m_dialog->setMaximumSize(maxWanted.width(), maxWanted.height());
                progressed = true;
            }
            const QSize minWanted(qMax(320, m_dialog->minimumWidth()),
                                  qMax(184, m_dialog->minimumHeight()));
            if (m_dialog->minimumSize() != minWanted) {
                m_dialog->setMinimumSize(minWanted.width(), minWanted.height());
                progressed = true;
            }
            if (progressed && m_dialog->height() < 184)
                m_dialog->resize(m_dialog->width(), 184);
        }
        QDialogButtonBox *buttons = m_dialog->findChild<QDialogButtonBox *>();
        if (!buttons) {
            hide();
            return;
        }
        const int commandInset =
                m_dialog->layout() ? qMax(0, m_dialog->layout()->contentsMargins().bottom()) : 24;
        const int buttonTop = buttons->mapTo(m_dialog, QPoint(0, 0)).y();
        const int top = qBound(0, buttonTop - commandInset, m_dialog->height());
        setGeometry(0, top, m_dialog->width(), m_dialog->height() - top);
        lower();
        setVisible(m_dialog->isVisible());

        if (messageBox) {
            auto *label = messageBox->findChild<QLabel *>(QStringLiteral("qt_msgbox_label"),
                                                          Qt::FindDirectChildrenOnly);
            auto *icon = messageBox->findChild<QLabel *>(QStringLiteral("qt_msgboxex_icon_label"),
                                                         Qt::FindDirectChildrenOnly);
            if (label && label->isVisible()) {
                // QMessageBox keeps the message widgets top-aligned when its
                // height is enlarged to the ContentDialog minimum. Centre the
                // actual content in the region above the command footer, then
                // align the icon with the message instead of the layout row.
                const int contentCenter = QRect(0, 0, messageBox->width(), top).center().y();
                label->move(label->x(),
                            label->y() + contentCenter - label->geometry().center().y());
                if (icon && icon->isVisible()) {
                    icon->move(icon->x(), contentCenter - icon->rect().center().y());
                }
            }
        }
    }

    QPointer<QDialog> m_dialog;
    QColor m_fill;
    QColor m_stroke;
    bool m_syncQueued = false;
};

ContentDialogFooterSurface *contentDialogFooter(QDialog *dialog, bool create)
{
    if (!dialog)
        return nullptr;
    auto *surface = static_cast<ContentDialogFooterSurface *>(dialog->findChild<QWidget *>(
            QString::fromLatin1(contentDialogFooterName), Qt::FindDirectChildrenOnly));
    if (!surface && create)
        surface = new ContentDialogFooterSurface(dialog);
    return surface;
}

// True for a QCalendarWidget or any of its descendants. Shared with the popup
// surface TU, which resolves calendar pickers the same way.

struct LineEditClearButtonCache
{
    QHash<QLineEdit *, QPointer<QAbstractButton>> buttons;
    QHash<QLineEdit *, QMetaObject::Connection> cleanupConnections;
};

LineEditClearButtonCache &lineEditClearButtonCache()
{
    // The cache lives until process exit, like the frame registry, so a
    // widget cannot race its cleanup with static destruction.
    static auto *cache = new LineEditClearButtonCache;
    return *cache;
}

} // namespace

void remember(QWidget *widget, const char *property, const QVariant &value)
{
    if (widget && !widget->property(property).isValid())
        widget->setProperty(property, value);
}

// Qt-mask equivalent of the DWM corner preference for QMenu popups:
// submenus materialize their native HWND after Show (or never resize, so
// the Resize re-apply never fires), leaving them square while the top
// level is rounded. A Qt rounded-rect mask clips the same OverlayRadius
// on every platform, costs no geometry change, and is a no-op under the
// live DWM preference (the native rounding already clips inside it).
// Only ever applied to Qt::Popup QMenu windows from preparePopupSurface,
// so no other surface can pick it up; clearMask() on teardown restores.
namespace {
constexpr auto menuMaskRetryProperty = "_winui_menu_mask_retry";
constexpr int menuMaskMaxRetries = 5;
} // namespace
void applyMenuRoundedMask(QMenu *menu)
{
    if (!menu || !menu->isWindow() || menu->windowType() != Qt::Popup)
        return;
#ifdef Q_OS_WIN
    // A granted DWM corner preference already clips the native flyout and
    // its shadow. Stacking Qt's rasterized region over it creates short dark
    // seams at the top/bottom edge after a Mica cycle. Keep the Qt mask only
    // when native rounding is unavailable (including offscreen captures).
    if (QGuiApplication::platformName() == QStringLiteral("windows")) {
        constexpr DWORD cornerPreferenceAttribute = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
        DWORD preference = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(reinterpret_cast<HWND>(menu->winId()),
                                            cornerPreferenceAttribute, &preference,
                                            sizeof(preference)))
            && preference == 2) { // DWMWCP_ROUND
            if (!menu->mask().isEmpty())
                menu->clearMask();
            return;
        }
    }
#endif
    const QSize size = menu->size();
    if (size.isEmpty()) {
        // Show can arrive before QMenu computes its layout size. Retry
        // queued past the show so the submenu still rounds instead of
        // staying square. The attempt count stays set across retries and
        // is cleared only on success below: clearing it before re-entering
        // would make every retry look like a first attempt and reschedule
        // forever for a menu that never gains a size (a 0ms singleShot
        // chain / CPU spin). Bound the chain instead so a permanently
        // zero-size menu settles unmasked.
        const int attempts = menu->property(menuMaskRetryProperty).toInt();
        if (attempts >= menuMaskMaxRetries)
            return;
        menu->setProperty(menuMaskRetryProperty, attempts + 1);
        QTimer::singleShot(0, menu, [menu] { applyMenuRoundedMask(menu); });
        return;
    }
    menu->setProperty(menuMaskRetryProperty, {});
    // Rasterize the SAME rounded rect paint lays (OverlayRadius) and
    // threshold at 50% alpha: nominal r8 is preserved while paint and
    // mask coincide at the half-coverage line — no dark fringe inside
    // over the composited material, no backdrop hairline outside.
    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, size.width(), size.height()), OverlayRadius, OverlayRadius);
    QImage raster(size, QImage::Format_ARGB32_Premultiplied);
    raster.fill(Qt::transparent);
    QPainter rasterPainter(&raster);
    rasterPainter.setRenderHint(QPainter::Antialiasing);
    rasterPainter.fillPath(path, Qt::white);
    rasterPainter.end();
    // Scanline runs: kept pixels (>= 50% alpha) union into the region.
    QRegion mask;
    for (int y = 0; y < size.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(raster.constScanLine(y));
        int runStart = -1;
        for (int x = 0; x <= size.width(); ++x) {
            const bool kept = x < size.width() && qAlpha(line[x]) >= 128;
            if (kept && runStart < 0)
                runStart = x;
            else if (!kept && runStart >= 0) {
                mask += QRect(runStart, y, x - runStart, 1);
                runStart = -1;
            }
        }
    }
    menu->setMask(mask);
}

void rememberPalette(QWidget *widget)
{
    if (!widget)
        return;
    remember(widget, originalPaletteExplicitProperty, widget->testAttribute(Qt::WA_SetPalette));
    remember(widget, originalPaletteProperty, QVariant::fromValue(widget->palette()));
}

void restoreRememberedPalette(QWidget *widget)
{
    if (!widget || !widget->property(originalPaletteProperty).isValid())
        return;
    if (widget->property(originalPaletteExplicitProperty).toBool())
        widget->setPalette(widget->property(originalPaletteProperty).value<QPalette>());
    else
        widget->setPalette(QPalette());
}

QPalette effectivePopupPalette(QWidget *widget, const QPalette &fallback)
{
    if (!widget || !widget->property(originalPaletteExplicitProperty).toBool())
        return fallback;
    return widget->property(originalPaletteProperty).value<QPalette>().resolve(fallback);
}

// transparentized(): preserve RGB, drive alpha toward 0. The tokens file
// owns color values; this helper only modulates an existing role color so
// no numeric QColor literal ever appears in a _p.cpp painter.
inline QColor transparentized(const QColor &color, int alpha)
{
    QColor result = color;
    result.setAlpha(alpha);
    return result;
}

void setBackdropPalette(QWidget *surface, const QPalette &palette)
{
    // Temporary Mica palettes are style writes, not application overrides.
    // Keep the implicit-owner snapshot in step so the effective-surface
    // refresh does not permanently disown scroll-area and shell palettes.
    const QVariant snapshot = surface->property(implicitPaletteSnapshotProperty);
    const bool styleOwned = surface->property(ownedPaletteProperty).toBool() && snapshot.isValid()
            && surface->palette() == snapshot.value<QPalette>();
    // A parent's Window-role change can also re-resolve palettes on its
    // children (navigationPanel is a child of centralWidget). Remember only
    // descendants still matching their style snapshot before that write.
    QList<QPointer<QWidget>> inheritedOwners;
    for (QWidget *child : surface->findChildren<QWidget *>()) {
        const QVariant childSnapshot = child->property(implicitPaletteSnapshotProperty);
        if (child->property(ownedPaletteProperty).toBool() && childSnapshot.isValid()
            && child->palette() == childSnapshot.value<QPalette>())
            inheritedOwners.append(child);
    }
    if (styleOwned)
        surface->setProperty(implicitPaletteSnapshotProperty, QVariant::fromValue(palette));
    surface->setPalette(palette);
    if (styleOwned)
        surface->setProperty(implicitPaletteSnapshotProperty,
                             QVariant::fromValue(surface->palette()));
    for (const QPointer<QWidget> &child : inheritedOwners) {
        if (child)
            child->setProperty(implicitPaletteSnapshotProperty,
                               QVariant::fromValue(child->palette()));
    }
}

void rememberBackdropSurfaceState(QWidget *surface)
{
    if (!surface)
        return;
    remember(surface, backdropSurfacePaletteProperty, QVariant::fromValue(surface->palette()));
    remember(surface, backdropSurfacePaletteExplicitProperty,
             surface->testAttribute(Qt::WA_SetPalette));
    remember(surface, backdropSurfaceAutoFillProperty, surface->autoFillBackground());
    remember(surface, backdropSurfaceTranslucentProperty,
             surface->testAttribute(Qt::WA_TranslucentBackground));
    remember(surface, backdropSurfaceNoSystemBackgroundProperty,
             surface->testAttribute(Qt::WA_NoSystemBackground));
    remember(surface, backdropSurfaceStyledBackgroundProperty,
             surface->testAttribute(Qt::WA_StyledBackground));
    remember(surface, backdropSurfaceOpaquePaintProperty,
             surface->testAttribute(Qt::WA_OpaquePaintEvent));
}

QPalette backdropSurfaceSnapshotPalette(const QWidget *surface)
{
    if (!surface)
        return QPalette();
    const QVariant saved = surface->property(backdropSurfacePaletteProperty);
    return saved.isValid() ? saved.value<QPalette>() : QPalette();
}

void rebaseBackdropSurfaceSnapshotPalette(QWidget *surface, const QPalette &palette)
{
    if (!surface)
        return;
    surface->setProperty(backdropSurfacePaletteProperty, QVariant::fromValue(palette));
}

void transparentizeSurface(QWidget *surface)
{
    if (!surface)
        return;
    rememberBackdropSurfaceState(surface);
    QPalette palette = surface->palette();
    // The Window role reveals the live material; keep text roles intact.
    palette.setColor(QPalette::Window, transparentized(palette.color(QPalette::Window), 0));
    setBackdropPalette(surface, palette);
    // Never auto-fill here: an opaque fill would paint a solid band over
    // the composited material (the white-veil ghost).
    surface->setAutoFillBackground(false);
    surface->setAttribute(Qt::WA_StyledBackground, true);
    surface->update();
}

void restoreTransparentizedSurface(QWidget *surface)
{
    if (!surface || !surface->property(backdropSurfacePaletteProperty).isValid())
        return;
    if (surface->property(backdropSurfacePaletteExplicitProperty).toBool())
        setBackdropPalette(surface,
                           surface->property(backdropSurfacePaletteProperty).value<QPalette>());
    else
        setBackdropPalette(surface, QPalette());
    if (surface->property(backdropSurfaceAutoFillProperty).isValid())
        surface->setAutoFillBackground(surface->property(backdropSurfaceAutoFillProperty).toBool());
    if (surface->property(backdropSurfaceTranslucentProperty).isValid())
        surface->setAttribute(Qt::WA_TranslucentBackground,
                              surface->property(backdropSurfaceTranslucentProperty).toBool());
    if (surface->property(backdropSurfaceNoSystemBackgroundProperty).isValid())
        surface->setAttribute(
                Qt::WA_NoSystemBackground,
                surface->property(backdropSurfaceNoSystemBackgroundProperty).toBool());
    if (surface->property(backdropSurfaceStyledBackgroundProperty).isValid())
        surface->setAttribute(Qt::WA_StyledBackground,
                              surface->property(backdropSurfaceStyledBackgroundProperty).toBool());
    if (surface->property(backdropSurfaceOpaquePaintProperty).isValid())
        surface->setAttribute(Qt::WA_OpaquePaintEvent,
                              surface->property(backdropSurfaceOpaquePaintProperty).toBool());
    surface->setProperty(backdropSurfacePaletteProperty, {});
    surface->setProperty(backdropSurfacePaletteExplicitProperty, {});
    surface->setProperty(backdropSurfaceAutoFillProperty, {});
    surface->setProperty(backdropSurfaceTranslucentProperty, {});
    surface->setProperty(backdropSurfaceNoSystemBackgroundProperty, {});
    surface->setProperty(backdropSurfaceStyledBackgroundProperty, {});
    surface->setProperty(backdropSurfaceOpaquePaintProperty, {});
    surface->update();
}

bool isTransparentizedForBackdrop(const QWidget *surface)
{
    return surface && surface->property(backdropSurfacePaletteProperty).isValid();
}

void transparentizeForBackdrop(QWidget *surface)
{
    transparentizeSurface(surface);
}

void restoreTransparentizedForBackdrop(QWidget *surface)
{
    restoreTransparentizedSurface(surface);
}

// Defined below (after the sync entry points): restores the exact pre-Mica
// style-owned state before repainting. Forward-declared here so the shared
// scroll-chain/container helpers above can mirror every sync link.
static void restoreBackdropSurface(QWidget *widget);

void makeChromeSurfacesTransparent(QWidget *window)
{
    if (!window)
        return;
    // Menu bar, tool bars, status bar: WinUI draws no chrome panels over
    // Mica; the material shows through and controls float on it. Every menu
    // bar is visited (not just the first): nested demo windows carry their
    // own bars and a singular findChild would arm whichever bar the object
    // tree yields first, leaving the real menuBar opaque over the material.
    // In-card samples are skipped: they sit on the card's veiled fill
    // (recipe 1), so transparentizing them would punch the card to alpha 0.
    for (QMenuBar *menuBar : window->findChildren<QMenuBar *>()) {
        if (insideOpaqueCard(menuBar))
            continue;
        transparentizeSurface(menuBar);
    }
    for (QToolBar *toolBar : window->findChildren<QToolBar *>()) {
        if (insideOpaqueCard(toolBar))
            continue;
        transparentizeSurface(toolBar);
    }
    for (QStatusBar *statusBar : window->findChildren<QStatusBar *>()) {
        if (insideOpaqueCard(statusBar))
            continue;
        transparentizeSurface(statusBar);
    }
}

void restoreChromeSurfaces(QWidget *window)
{
    if (!window)
        return;
    // Restore the saved recipe, even if the application has since reparented
    // a bar into a card. Never-armed bars are already a no-op.
    for (QMenuBar *menuBar : window->findChildren<QMenuBar *>()) {
        restoreTransparentizedSurface(menuBar);
    }
    for (QToolBar *toolBar : window->findChildren<QToolBar *>()) {
        restoreTransparentizedSurface(toolBar);
    }
    for (QStatusBar *statusBar : window->findChildren<QStatusBar *>()) {
        restoreTransparentizedSurface(statusBar);
    }
}

namespace {

// One visual surface for the scrolled chain: the area frame, its viewport,
// both bars, and the QScrollArea container widget each paint PE_Widget over
// the live material, so every link gets the same no-fill recipe. The scroll
// bars join the chain too: bars carry style-owned explicit palettes (polish
// registers palette owners, the theme refresh writes them), so they never
// inherit the island's transparentized roles. An unarmed bar keeps its opaque
// Window role and the CC_ScrollBar painter takes the Window-fill fallback
// over the live material.
// Card interiors take a partial arm (reviewer 2026-10-06 tree-selection
// overlay): the viewport's auto-filled Base layer would double over the card
// veil, so only the autofill stops (roles stay intact for tone reads) and
// the viewport repaints the card tone itself via PE (erase + Source refill,
// smear-safe). Bars keep their own paint paths untouched. The arm reuses the
// same backdrop-surface snapshot keys as transparentizeSurface (palette is
// remembered unchanged) so restore is uniform.

void restoreScrollChain(QAbstractScrollArea *area);

void transparentizeScrollChain(QAbstractScrollArea *area)
{
    if (!area)
        return;
    // A reparent can change the recipe during an active backdrop. Return the
    // old arm first so the next snapshot never captures temporary attributes.
    // Bare NavigationView areas are exempt: the navigation prepare owns their
    // area/viewport recipe (Base alpha 0), and rolling it back here would
    // undo settled nav state on every sync while the re-arm below cannot
    // rebuild the Base role. In-card nav keeps its own recipe via the branch
    // below, as before.
    const bool inCard = insideOpaqueCard(area);
    const bool bareNavigation =
            !inCard && area->property(Style::NavigationViewProperty).toBool();
    if (!bareNavigation
        && ((inCard && area->property(backdropSurfacePaletteProperty).isValid())
            || (!inCard && area->viewport()
                && area->viewport()->property(backdropSurfacePaletteProperty).isValid())))
        restoreScrollChain(area);
    if (inCard) {
        // NavigationView-marked views keep their own recipe
        // (prepareNavigationSurface leaves in-card nav opaque + autofilled):
        // never second-guess it here.
        if (!area->property(Style::NavigationViewProperty).toBool()) {
            if (QWidget *viewport = area->viewport()) {
                remember(viewport, backdropSurfacePaletteProperty,
                         QVariant::fromValue(viewport->palette()));
                remember(viewport, backdropSurfacePaletteExplicitProperty,
                         viewport->testAttribute(Qt::WA_SetPalette));
                remember(viewport, backdropSurfaceAutoFillProperty,
                         viewport->autoFillBackground());
                remember(viewport, backdropSurfaceStyledBackgroundProperty,
                         viewport->testAttribute(Qt::WA_StyledBackground));
                remember(viewport, backdropSurfaceOpaquePaintProperty,
                         viewport->testAttribute(Qt::WA_OpaquePaintEvent));
                viewport->setAutoFillBackground(false);
                viewport->setAttribute(Qt::WA_StyledBackground, true);
                viewport->setAttribute(Qt::WA_OpaquePaintEvent, false);
                viewport->update();
            }
        }
        return;
    }
    guardIslandScrollArea(area);
    transparentizeForBackdrop(area);
    if (QWidget *viewport = area->viewport())
        transparentizeForBackdrop(viewport);
    if (QScrollBar *verticalBar = area->verticalScrollBar())
        transparentizeForBackdrop(verticalBar);
    if (QScrollBar *horizontalBar = area->horizontalScrollBar())
        transparentizeForBackdrop(horizontalBar);
    if (auto *scrollArea = qobject_cast<QScrollArea *>(area)) {
        if (QWidget *container = scrollArea->widget())
            transparentizeForBackdrop(container);
    }
}

void restoreScrollChain(QAbstractScrollArea *area)
{
    if (!area)
        return;
    if (QWidget *viewport = area->viewport()) {
        restoreBackdropSurface(viewport);
        viewport->update();
    }
    // Each restore is a no-op for widgets that were never transparentized.
    restoreBackdropSurface(area);
    if (QWidget *viewport = area->viewport())
        restoreBackdropSurface(viewport);
    if (QScrollBar *verticalBar = area->verticalScrollBar())
        restoreBackdropSurface(verticalBar);
    if (QScrollBar *horizontalBar = area->horizontalScrollBar())
        restoreBackdropSurface(horizontalBar);
    if (auto *scrollArea = qobject_cast<QScrollArea *>(area)) {
        if (QWidget *container = scrollArea->widget())
            restoreBackdropSurface(container);
    }
    area->update();
}

// Card keep-opaque gate for the generic container walk below: group cards,
// SettingsCards, the explicit palette preview, the inline calendar frame and
// the QTabWidget pane/pages own their surface fill by design and must never
// punch through to Mica. Delegates to the shared structural card walk so the
// sync, the PE_Widget erase guard and the range-control erases agree on one
// definition.
bool keepOpaqueContainer(const QWidget *widget)
{
    if (!widget)
        return true;
    return isCardSurface(widget);
}

// Generic container links that sit directly on the material with no card:
// plain QFrame panels inside the dockHost subtree and bare StyledPanel links.
// Links inside a card (QTabWidget pane/pages, viewports under a group card)
// are gated out too: the card fill owns that surface. Each link is
// transparentized through the same helper so toggle-off restore returns it.
void transparentizeBareContainer(QWidget *widget)
{
    if (!widget || keepOpaqueContainer(widget) || insideOpaqueCard(widget))
        return;
    transparentizeForBackdrop(widget);
}

void restoreBareContainer(QWidget *widget)
{
    if (!widget)
        return;
    restoreBackdropSurface(widget);
}

// True when the widget sits under an opted-in island or the central shell:
// the same subtree scope the window sync arms scroll chains and bare
// containers in. Anything else keeps its opaque fill by design.
bool underLiveScope(const QWidget *widget)
{
    return anyAncestorToWindow(widget, [](const QWidget *ancestor) {
        if (isContentLayerSurface(ancestor->property(Style::SurfaceProperty)))
            return true;
        return ancestor->objectName() == QStringLiteral("centralWidget");
    });
}

bool underDockHost(const QWidget *widget)
{
    return anyAncestorToWindow(widget, [](const QWidget *ancestor) {
        return ancestor->objectName() == QStringLiteral("dockHost");
    });
}

} // namespace

void armPolishedSurfaceForBackdrop(QWidget *widget)
{
    if (!widget || widget->isWindow())
        return;
    QWidget *win = widget->window();
    if (!win || win->windowType() == Qt::Popup)
        return;
    if (backdropEffectiveSurface(win) != BackdropSurface::Composited)
        return;
    if (insideOpaqueCard(widget)) {
        // Card interiors keep their fill; only a scroll area's viewport takes
        // the partial arm (same branch the window sync uses).
        if (auto *area = qobject_cast<QAbstractScrollArea *>(widget))
            transparentizeScrollChain(area);
        return;
    }
    // Late-created viewport, bars or container join an already-armed chain.
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget->parentWidget());
        area && isTransparentizedForBackdrop(area)) {
        if (widget == area->viewport() || qobject_cast<QScrollBar *>(widget))
            transparentizeSurface(widget);
        else if (auto *scrollArea = qobject_cast<QScrollArea *>(area);
                 scrollArea && widget == scrollArea->widget())
            transparentizeSurface(widget);
        return;
    }
    if (qobject_cast<QMenuBar *>(widget) || qobject_cast<QToolBar *>(widget)
        || qobject_cast<QStatusBar *>(widget)) {
        transparentizeSurface(widget);
        return;
    }
    if (isContentLayerSurface(widget->property(Style::SurfaceProperty))) {
        if (widget->palette().color(QPalette::Window).alpha() == 0)
            return;
        transparentizeSurface(widget);
        if (auto *islandArea = qobject_cast<QAbstractScrollArea *>(widget))
            guardIslandScrollArea(islandArea);
        return;
    }
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget)) {
        if (underLiveScope(widget))
            transparentizeScrollChain(area);
        return;
    }
    if (!underLiveScope(widget))
        return;
    if (qobject_cast<QTabWidget *>(widget))
        transparentizeBareContainer(widget);
    else if (auto *frame = qobject_cast<QFrame *>(widget)) {
        if (frame->frameShape() == QFrame::StyledPanel || underDockHost(widget))
            transparentizeBareContainer(frame);
    }
}

void reconvergeSurfaceForBackdrop(QWidget *widget)
{
    if (!widget)
        return;
    // Two passes, parent-first: findChildren yields ancestors before
    // descendants. Restoring everything first lets a scroll area re-arm its
    // chain (including the partial in-card viewport arm) without a later
    // child restore undoing it.
    restoreTransparentizedForBackdrop(widget);
    const QList<QWidget *> subtree = widget->findChildren<QWidget *>();
    for (QWidget *child : subtree)
        restoreTransparentizedForBackdrop(child);
    armPolishedSurfaceForBackdrop(widget);
    for (QWidget *child : subtree)
        armPolishedSurfaceForBackdrop(child);
}

// Generic container links under one scope (island or shell): QTabWidget
// panes, dockHost frames, bare StyledPanel links. Cards keep their fill by
// design via the helper gate. One definition for sync and restore.
void syncContainerLinks(QWidget *scope)
{
    for (QTabWidget *tabs : scope->findChildren<QTabWidget *>()) {
        transparentizeBareContainer(tabs);
        if (QWidget *pane = tabs->widget(tabs->currentIndex()))
            transparentizeBareContainer(pane);
    }
    if (QWidget *dockHost = scope->findChild<QWidget *>(QStringLiteral("dockHost"))) {
        for (QFrame *frame : dockHost->findChildren<QFrame *>())
            transparentizeBareContainer(frame);
    }
    for (QFrame *frame : scope->findChildren<QFrame *>()) {
        if (frame->frameShape() == QFrame::StyledPanel)
            transparentizeBareContainer(frame);
    }
}

void restoreContainerLinks(QWidget *scope)
{
    for (QTabWidget *tabs : scope->findChildren<QTabWidget *>()) {
        restoreBareContainer(tabs);
        for (int index = 0; index < tabs->count(); ++index)
            restoreBareContainer(tabs->widget(index));
    }
    if (QWidget *dockHost = scope->findChild<QWidget *>(QStringLiteral("dockHost"))) {
        for (QFrame *frame : dockHost->findChildren<QFrame *>())
            restoreBareContainer(frame);
    }
    for (QFrame *frame : scope->findChildren<QFrame *>()) {
        if (frame->frameShape() == QFrame::StyledPanel)
            restoreBareContainer(frame);
    }
}

void syncContentSurfacesForBackdrop(QWidget *window)
{
    if (!window)
        return;
    // Content/layer islands opted into the style-owned surface contract
    // reveal the material too; anything else keeps its opaque fill so a
    // child control can never punch through an opaque island. A content
    // island that is itself already transparent keeps walking the gate in
    // paintsDirectlyOnBackdrop instead of disabling clears.
    const QList<QWidget *> islands = window->findChildren<QWidget *>();
    for (QWidget *island : islands) {
        if (!isContentLayerSurface(island->property(Style::SurfaceProperty)))
            continue;
        // The area frame, its viewport, and its container widget are one
        // visual surface with the island: without the chain the
        // Controls-heading band above the first card keeps its opaque fill
        // and ghosts exactly like the island did (healed only by resize).
        // Scroll areas and viewports are armed, not skipped: the queued
        // guard repaint covers their blit smear, and the toggle-off restore
        // below returns every link.
        for (QAbstractScrollArea *area : island->findChildren<QAbstractScrollArea *>())
            transparentizeScrollChain(area);
        if (auto *islandArea = qobject_cast<QAbstractScrollArea *>(island))
            guardIslandScrollArea(islandArea);
        // Generic container links under the island (plain dockHost frames,
        // bare StyledPanel links): same recipe so they stop banding over the
        // live material. Cards keep their fill by design — that now includes
        // the QTabWidget pane/pages (gated out inside the helper), whose
        // PE_FrameTabWidget lays the veiled card tone.
        syncContainerLinks(island);
        if (island->palette().color(QPalette::Window).alpha() == 0)
            continue;
        transparentizeSurface(island);
    }
    // Left shell (centralWidget, navigationPanel): not opt-in islands, but
    // they paint PE_Widget straight onto the live material through the
    // window (no opaque island between them and the window, so the gate
    // passes). Same no-fill recipe, otherwise their backing-store rows keep
    // the opaque frame until a resize reallocates (aliased "Mica backdrop"
    // label, incomplete left Mica background). Toggle-off restore below
    // returns every link (restore is a no-op for never-transparentized).
    if (auto *central = window->findChild<QWidget *>(QStringLiteral("centralWidget"))) {
        transparentizeForBackdrop(central);
        if (auto *nav = central->findChild<QWidget *>(QStringLiteral("navigationPanel")))
            transparentizeForBackdrop(nav);
        // Bare scroll areas under the shell (navigationList and friends):
        // not under any opted-in island, so the loop above never visits
        // them. Same scrolled-chain recipe with the blit guard on each area.
        for (QAbstractScrollArea *area : central->findChildren<QAbstractScrollArea *>())
            transparentizeScrollChain(area);
        syncContainerLinks(central);
    }
}

// Restore the exact pre-Mica style-owned state before repainting. Reverting to
// the pre-polish state instead would make a content island non-opaque.
static void restoreBackdropSurface(QWidget *widget)
{
    if (!widget)
        return;
    restoreTransparentizedForBackdrop(widget);
    widget->update();
}

void restoreContentSurfacesForBackdrop(QWidget *window)
{
    if (!window)
        return;
    const QList<QWidget *> islands = window->findChildren<QWidget *>();
    for (QWidget *island : islands) {
        if (!isContentLayerSurface(island->property(Style::SurfaceProperty)))
            continue;
        // The restore helper returns early for never-transparentized islands,
        // and leaves the style's pre-polish snapshots intact for unpolish.
        restoreBackdropSurface(island);
        // The sync above transparentized the scrolled chain alongside the
        // island: restore every link the same way (each restore is a no-op
        // for widgets that were never transparentized).
        const QList<QAbstractScrollArea *> areas = island->findChildren<QAbstractScrollArea *>();
        for (QAbstractScrollArea *area : areas)
            restoreScrollChain(area);
        restoreContainerLinks(island);
    }
    // Shell links transparentized by the sync above (no-ops if never armed).
    if (auto *central = window->findChild<QWidget *>(QStringLiteral("centralWidget"))) {
        restoreBackdropSurface(central);
        if (auto *nav = central->findChild<QWidget *>(QStringLiteral("navigationPanel"))) {
            restoreBackdropSurface(nav);
        }
        for (QAbstractScrollArea *area : central->findChildren<QAbstractScrollArea *>())
            restoreScrollChain(area);
        restoreContainerLinks(central);
    }
}

void stopDialogAnimations(QDialog *dialog)
{
    if (!dialog)
        return;
    const auto groups = dialog->findChildren<QParallelAnimationGroup *>(
            QStringLiteral("_winui_dialog_animation"), Qt::FindDirectChildrenOnly);
    for (QParallelAnimationGroup *group : groups) {
        group->stop();
        delete group;
    }
    dialog->setWindowOpacity(1.0);
    dialog->setProperty("_winui_dialog_animating", false);
}

void restoreContentDialogState(QDialog *dialog, bool clearSavedState)
{
    if (!dialog)
        return;
    stopDialogAnimations(dialog);
    delete contentDialogFooter(dialog, false);
    restoreRememberedPalette(dialog);
    if (dialog->property(originalAutoFillProperty).isValid())
        dialog->setAutoFillBackground(dialog->property(originalAutoFillProperty).toBool());
    if (dialog->property(originalMinimumSizeProperty).isValid())
        dialog->setMinimumSize(dialog->property(originalMinimumSizeProperty).value<QSize>());
    if (dialog->property(originalMaximumSizeProperty).isValid())
        dialog->setMaximumSize(dialog->property(originalMaximumSizeProperty).value<QSize>());
    if (QLayout *layout = dialog->layout()) {
        if (dialog->property(originalMarginsProperty).isValid())
            layout->setContentsMargins(dialog->property(originalMarginsProperty).value<QMargins>());
        if (dialog->property(originalSpacingProperty).isValid())
            layout->setSpacing(dialog->property(originalSpacingProperty).toInt());
        if (dialog->property(originalLayoutConstraintProperty).isValid())
            layout->setSizeConstraint(static_cast<QLayout::SizeConstraint>(
                    dialog->property(originalLayoutConstraintProperty).toInt()));
    }
    dialog->setProperty(ownedPaletteProperty, {});
    if (clearSavedState) {
        dialog->setProperty(originalPaletteProperty, {});
        dialog->setProperty(originalPaletteExplicitProperty, {});
        dialog->setProperty(originalAutoFillProperty, {});
        dialog->setProperty(originalMinimumSizeProperty, {});
        dialog->setProperty(originalMaximumSizeProperty, {});
        dialog->setProperty(originalLayoutConstraintProperty, {});
        dialog->setProperty(originalMarginsProperty, {});
        dialog->setProperty(originalSpacingProperty, {});
    }
}

void prepareContentDialogState(QDialog *dialog, bool dark)
{
    if (!dialog)
        return;
    rememberPalette(dialog);
    remember(dialog, originalAutoFillProperty, dialog->autoFillBackground());
    remember(dialog, originalMinimumSizeProperty, QVariant::fromValue(dialog->minimumSize()));
    remember(dialog, originalMaximumSizeProperty, QVariant::fromValue(dialog->maximumSize()));
    if (QLayout *layout = dialog->layout()) {
        remember(dialog, originalMarginsProperty, QVariant::fromValue(layout->contentsMargins()));
        remember(dialog, originalSpacingProperty, layout->spacing());
        remember(dialog, originalLayoutConstraintProperty, int(layout->sizeConstraint()));
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);
    }
    dialog->setProperty(ownedPaletteProperty, true);
    QPalette palette = dialog->palette();
    const QColor commandFill = dialogCommandFillColor(dark);
    // ContentDialog uses LayerFillColorAlt for its content region and
    // SolidBackgroundFillColorBase for the full-width command footer.
    // Always derive the layer from the fixed base token. prepare() is called
    // again on runtime theme changes; lifting the already lifted color would
    // drift #2C2C2C to #383838 on the second pass.
    QPalette basePalette = palette;
    basePalette.setColor(QPalette::Window, commandFill);
    palette.setColor(QPalette::Window, popupSurfaceColor(basePalette));
    dialog->setPalette(palette);
    dialog->setAutoFillBackground(true);
    dialog->setMinimumSize(320, 184);
    dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ContentDialogFooterSurface *footer = contentDialogFooter(dialog, true);
    footer->setColors(commandFill, tokens(palette).stroke);
    footer->queueGeometrySync();
}

namespace {

constexpr auto contentDialogScrimProperty = "_winui_content_dialog_scrim";

// Simulates the WinUI ContentDialog smoke layer: while a content dialog over a
// parent window is open, a translucent child paints SmokeFillColorDefault
// (#4D000000, theme-independent) over the whole parent client area. The
// dialog itself stays an opaque native window above the dimmed surface.
class ContentDialogScrim final : public QWidget
{
public:
    explicit ContentDialogScrim(QWidget *parentWindow) : QWidget(parentWindow)
    {
        setObjectName(QStringLiteral("_winui_content_dialog_scrim"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        setGeometry(parentWindow->rect());
        parentWindow->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == parentWidget() && event->type() == QEvent::Resize)
            setGeometry(parentWidget()->rect());
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), dialogScrimColor());
    }
};

class SliderValueTip final : public QWidget
{
public:
    explicit SliderValueTip(QSlider *slider)
        : QWidget(slider,
                  Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                          | Qt::WindowTransparentForInput)
    {
        setObjectName(QStringLiteral("_winui_slider_value_tip"));
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    void showValue(const QString &text, const QPoint &anchor, bool horizontal)
    {
        m_text = text;
        const QFontMetrics metrics(font());
        resize(qMax(32, metrics.horizontalAdvance(text) + 16), 32);
        QPoint position = horizontal ? QPoint(anchor.x() - width() / 2, anchor.y() - height())
                                     : QPoint(anchor.x(), anchor.y() - height() / 2);
        if (QScreen *screen = QGuiApplication::screenAt(anchor)) {
            const QRect available = screen->availableGeometry();
            position.setX(qBound(available.left(), position.x(), available.right() - width() + 1));
            position.setY(qBound(available.top(), position.y(), available.bottom() - height() + 1));
        }
        move(position);
        show();
        raise();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Private::Tokens t = Private::tokens(palette());
        QColor fill = palette().color(QPalette::ToolTipBase);
        fill.setAlpha(242);
        QPainter painter(this);
        roundedRect(&painter, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), fill,
                    t.strokeSecondary, Private::ControlRadius);
        painter.setPen(palette().color(QPalette::ToolTipText));
        painter.drawText(rect().adjusted(8, 0, -8, 0), Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
};

constexpr auto sliderToolTipDebounceTimerName = "_winui_slider_tooltip_debounce_timer";
constexpr int sliderToolTipDebounceInterval = 16;

SliderValueTip *sliderValueTip(QSlider *slider)
{
    if (!slider)
        return nullptr;
    return static_cast<SliderValueTip *>(slider->findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"), Qt::FindDirectChildrenOnly));
}

void clearSliderValueToolTip(QSlider *slider)
{
    if (!slider)
        return;

    if (auto *timer = slider->findChild<QTimer *>(
                QString::fromLatin1(sliderToolTipDebounceTimerName), Qt::FindDirectChildrenOnly)) {
        timer->stop();
    }
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, false);
    framePropertyRegistry().clear(slider, sliderToolTipValueProperty);
    framePropertyRegistry().clear(slider, sliderToolTipSurfacePendingProperty);
    if (auto *tip = sliderValueTip(slider))
        tip->hide();
}

void updateSliderValueToolTipNow(QSlider *slider)
{
    if (!slider)
        return;
    if (!slider->isEnabled()) {
        // A slider can be disabled while the trailing debounce timer is
        // pending. Clear both the inspectable state and the native popup;
        // otherwise the last value remains visible until the next drag.
        clearSliderValueToolTip(slider);
        return;
    }

    QStyleOptionSlider option;
    option.initFrom(slider);
    option.orientation = slider->orientation();
    option.minimum = slider->minimum();
    option.maximum = slider->maximum();
    option.sliderPosition = slider->sliderPosition();
    option.sliderValue = slider->value();
    option.singleStep = slider->singleStep();
    option.pageStep = slider->pageStep();
    option.upsideDown = slider->orientation() == Qt::Horizontal
            ? (slider->invertedAppearance() != (slider->layoutDirection() == Qt::RightToLeft))
            : !slider->invertedAppearance();
    const QRect handle = slider->style()->subControlRect(QStyle::CC_Slider, &option,
                                                         QStyle::SC_SliderHandle, slider);
    const QPoint anchor = slider->orientation() == Qt::Horizontal
            ? QPoint(handle.center().x(), handle.top() - 8)
            : QPoint(handle.right() + 8, handle.center().y());
    const QString valueText = QString::number(slider->value());
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty, valueText);
    // The headless platform cannot host a non-activating tool window and
    // synthesizes a FocusOut when one is shown. State/value remain testable;
    // the popup itself is covered by the native Windows test path.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    auto *tip = sliderValueTip(slider);
    if (!tip)
        tip = new SliderValueTip(slider);
    tip->showValue(valueText, slider->mapToGlobal(anchor), slider->orientation() == Qt::Horizontal);
}

QTimer *sliderToolTipDebounceTimer(QSlider *slider)
{
    if (!slider)
        return nullptr;
    if (auto *timer = slider->findChild<QTimer *>(
                QString::fromLatin1(sliderToolTipDebounceTimerName), Qt::FindDirectChildrenOnly)) {
        return timer;
    }
    auto *timer = new QTimer(slider);
    timer->setObjectName(QString::fromLatin1(sliderToolTipDebounceTimerName));
    timer->setSingleShot(true);
    timer->setInterval(sliderToolTipDebounceInterval);
    const QPointer<QSlider> guarded(slider);
    QObject::connect(timer, &QTimer::timeout, slider, [guarded] {
        if (!guarded)
            return;
        // Close the current frame's leading-edge window so the next pointer
        // move can schedule a fresh refresh.
        framePropertyRegistry().set(guarded, sliderToolTipSurfacePendingProperty, false);
        if (guarded->isEnabled()
            && framePropertyRegistry().value(guarded, sliderToolTipVisibleProperty).toBool())
            updateSliderValueToolTipNow(guarded);
    });
    return timer;
}

bool isLineEditClearButton(const QLineEdit *lineEdit, const QAbstractButton *button)
{
    if (!lineEdit || !button)
        return false;
    for (QAction *action : lineEdit->actions())
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (action->associatedObjects().contains(button))
#else
        // QAction::associatedObjects() is Qt 6; Qt 5 only lists widgets.
        if (action->associatedWidgets().contains(const_cast<QAbstractButton *>(button)))
#endif
            return false;
    // QLineEdit's private clear affordance is deliberately not a QAction;
    // custom leading/trailing actions are associated above.
    return true;
}

} // namespace

void cacheLineEditClearButton(QLineEdit *lineEdit, QAbstractButton *button)
{
    if (!lineEdit)
        return;

    auto &cache = lineEditClearButtonCache();
    if (!button) {
        cache.buttons.remove(lineEdit);
        if (const auto connection = cache.cleanupConnections.take(lineEdit))
            QObject::disconnect(connection);
        return;
    }

    cache.buttons.insert(lineEdit, QPointer<QAbstractButton>(button));
    if (!cache.cleanupConnections.contains(lineEdit)) {
        cache.cleanupConnections.insert(lineEdit,
                                        QObject::connect(lineEdit, &QObject::destroyed, [lineEdit] {
                                            auto &cache = lineEditClearButtonCache();
                                            cache.buttons.remove(lineEdit);
                                            cache.cleanupConnections.remove(lineEdit);
                                        }));
    }
}

const QAbstractButton *lineEditClearButton(const QLineEdit *lineEdit)
{
    if (!lineEdit)
        return nullptr;
    const auto &cache = lineEditClearButtonCache();
    const auto button = cache.buttons.constFind(const_cast<QLineEdit *>(lineEdit));
    return button == cache.buttons.cend() ? nullptr : button->data();
}

namespace {

constexpr auto lineEditHelperUpdatePendingProperty = "_winui_line_edit_helper_update_pending";

qulonglong nextLineEditHelperUpdateToken()
{
    // QWidget lifecycle and style events are confined to the GUI thread.
    static qulonglong token = 0;
    return ++token;
}

void scheduleLineEditHelperUpdate(QLineEdit *lineEdit, Style *style)
{
    if (!lineEdit
        || framePropertyRegistry().value(lineEdit, lineEditHelperUpdatePendingProperty).isValid())
        return;

    const qulonglong token = nextLineEditHelperUpdateToken();
    framePropertyRegistry().set(lineEdit, lineEditHelperUpdatePendingProperty,
                                QVariant::fromValue(token));
    const QPointer<QLineEdit> guardedLineEdit(lineEdit);
    const QPointer<Style> guardedStyle(style);
    QTimer::singleShot(0, lineEdit, [guardedLineEdit, guardedStyle, token] {
        if (!guardedLineEdit
            || framePropertyRegistry()
                            .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                            .toULongLong()
                    != token)
            return;

        // updateReadOnlyDeleteAffordance() has no style parameter for callers
        // that only need the visibility contract. Resolve it at execution time
        // so a preceding update call can still be coalesced with preparation.
        Style *helperStyle = guardedStyle.data();
        if (!helperStyle)
            helperStyle = qobject_cast<Style *>(guardedLineEdit->style());

        QAbstractButton *clearButton = nullptr;
        for (QAbstractButton *button : guardedLineEdit->findChildren<QAbstractButton *>()) {
            if (helperStyle) {
                // QLineEdit creates its private clear affordance lazily. Depending
                // on that timing, it can miss the parent's polish pass and never
                // deliver hover events to the style. Prepare both the private
                // affordance and QAction helper buttons as soon as they exist.
                button->setAttribute(Qt::WA_Hover, true);
                button->installEventFilter(helperStyle);
                if (!framePropertyRegistry().value(button, hoverProperty).isValid())
                    framePropertyRegistry().set(button, hoverProperty,
                                                button->isEnabled() && button->underMouse() ? 1.0
                                                                                            : 0.0);
                if (!framePropertyRegistry().value(button, pressProperty).isValid())
                    framePropertyRegistry().set(button, pressProperty, 0.0);
            }

            if (isLineEditClearButton(guardedLineEdit, button)) {
                clearButton = button;
                button->setVisible(!guardedLineEdit->isReadOnly() && guardedLineEdit->isEnabled()
                                   && guardedLineEdit->hasFocus()
                                   && guardedLineEdit->isClearButtonEnabled()
                                   && !guardedLineEdit->text().isEmpty());
            }
        }
        cacheLineEditClearButton(guardedLineEdit, clearButton);
        if (framePropertyRegistry()
                    .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                    .toULongLong()
            == token) {
            framePropertyRegistry().clear(guardedLineEdit, lineEditHelperUpdatePendingProperty);
        }
    });
}

} // namespace

void showContentDialogScrim(QDialog *dialog)
{
    if (!dialog)
        return;
    hideContentDialogScrim(dialog);
    QWidget *owner = dialog->parentWidget();
    owner = owner ? owner->window() : nullptr;
    if (!owner)
        return;
    auto *scrim = new ContentDialogScrim(owner);
    dialog->setProperty(contentDialogScrimProperty,
                        QVariant::fromValue(static_cast<QObject *>(scrim)));
    // The scrim is parented to the owner window, not the dialog, so either
    // side can die first. If the owner (hence the scrim) is destroyed, the
    // raw property would dangle and a later hideContentDialogScrim() would
    // deleteLater() freed memory: clear the property synchronously on the
    // scrim's own destroyed signal. If the dialog dies first, the scrim
    // would otherwise orphan on the owner: schedule its teardown. The
    // equality guard keeps a queued destroy of a replaced scrim from
    // clearing a newer scrim's property.
    QObject::connect(scrim, &QObject::destroyed, dialog, [dialog, scrim] {
        if (dialog->property(contentDialogScrimProperty).value<QObject *>() == scrim)
            dialog->setProperty(contentDialogScrimProperty, {});
    });
    QObject::connect(dialog, &QObject::destroyed, scrim, [scrim] { scrim->deleteLater(); });
    scrim->show();
    scrim->raise();
}

void hideContentDialogScrim(QDialog *dialog)
{
    if (!dialog)
        return;
    // The scrim's destroyed signal clears this property synchronously when
    // the owner tears down first, so a dangling raw pointer can never reach
    // the deleteLater() below. The equality-cleared slot above also makes
    // an independently destroyed scrim a no-op here.
    auto *scrim = dialog->property(contentDialogScrimProperty).value<QObject *>();
    if (!scrim)
        return;
    dialog->setProperty(contentDialogScrimProperty, {});
    scrim->deleteLater();
}

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit)
{
    scheduleLineEditHelperUpdate(lineEdit, nullptr);
}

void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style)
{
    scheduleLineEditHelperUpdate(lineEdit, style);
}

void cancelLineEditHelperUpdate(QLineEdit *lineEdit)
{
    if (lineEdit)
        framePropertyRegistry().clear(lineEdit, lineEditHelperUpdatePendingProperty);
}

void showSliderValueToolTip(QSlider *slider)
{
    if (!slider || !slider->isEnabled())
        return;
    // Synchronous inspectable state is always current, regardless of how the
    // native popup is coalesced.
    framePropertyRegistry().set(slider, sliderToolTipVisibleProperty, true);
    framePropertyRegistry().set(slider, sliderToolTipValueProperty,
                                QString::number(slider->value()));
    // The headless platform cannot host a non-activating tool window; keep the
    // state synchronous and avoid allocating a timer that cannot fire.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    // A refresh is already scheduled in this frame; its timeout picks up the
    // newest value, so subsequent pointer moves within the frame are coalesced.
    if (framePropertyRegistry().value(slider, sliderToolTipSurfacePendingProperty).toBool())
        return;
    // Leading edge of a new frame (or the first show): refresh the popup
    // immediately so it tracks the handle with no trailing-edge lag, then arm
    // a one-shot timer to re-apply the latest value at the frame boundary. The
    // trailing design in 7e2e480 only repainted after the mouse paused, which
    // made the tooltip visibly trail the handle during a fast drag.
    framePropertyRegistry().set(slider, sliderToolTipSurfacePendingProperty, true);
    if (auto *timer = sliderToolTipDebounceTimer(slider))
        timer->start();
    updateSliderValueToolTipNow(slider);
}

void hideSliderValueToolTip(QSlider *slider)
{
    clearSliderValueToolTip(slider);
}
} // namespace WinUI3::Private
