// SPDX-License-Identifier: LGPL-2.1-or-later
// Shared visual-test helpers (plan step 9): probes + frameReal/setFrame +
// colorDistance/verifyHitSurface/renderComplex/inkPixels live here so split
// binaries cannot regrow copy-pasted copies.
#pragma once

#include <winui3style/animatedstack.h>
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3qtcompat_p.h"
#include "../src/winui3tokens_p.h"

#include <QLabel>
#include <QListWidget>
#include <QTreeWidget>
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDockWidget>
#include <QFocusEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QSplitter>
#include <QSignalSpy>
#include <QSizeGrip>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOptionGroupBox>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyledItemDelegate>
#include <QStyleOptionToolButton>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QToolBar>
#include <QTimer>
#include <QTest>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#include <QtMath>
#include <QStandardItemModel>
#include <QCoreApplication>
#include <QScreen>
#include <QDebug>
#include <QPixmap>

#include <algorithm>
#include <cmath>
#include <limits>

#if defined(Q_OS_WIN) && defined(WINUI3STYLE_NATIVE_CAPTURE)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#  pragma comment(lib, "dwmapi.lib")

// Includes the native caption, unlike QWidget::grab(). Own a copy before
// releasing the DIB; callers pair pixel checks with same-state contracts.
inline QImage nativeWindowFrame(WId id)
{
    const HWND hwnd = reinterpret_cast<HWND>(id);
    RECT bounds{};
    if (!GetWindowRect(hwnd, &bounds))
        return {};
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    const HDC dc = CreateCompatibleDC(nullptr);
    if (!dc)
        return {};
    void *pixels = nullptr;
    const HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    QImage frame;
    if (bitmap) {
        const HGDIOBJ previous = SelectObject(dc, bitmap);
        if (PrintWindow(hwnd, dc, 2))
            frame = QImage(static_cast<uchar *>(pixels), width, height, QImage::Format_RGB32)
                            .copy();
        SelectObject(dc, previous);
        DeleteObject(bitmap);
    }
    DeleteDC(dc);
    return frame;
}
#endif

// Desktop capture can run ahead of a redirected popup surface and record only
// its DWM shadow. Synchronize the compositor without adding arbitrary sleeps.
inline void flushNativeCompositor()
{
#if defined(Q_OS_WIN) && defined(WINUI3STYLE_NATIVE_CAPTURE)
    DwmFlush();
#endif
}

// Drain Qt's deferred-delete queue so top-level inventories/counts measure a
// settled widget set (previous cycles' deletes land BEFORE measurement).
[[maybe_unused]] static void drainDeferredDeletion()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

[[maybe_unused]] static int colorDistance(const QColor &a, const QColor &b)
{
    return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue())
            + qAbs(a.alpha() - b.alpha());
}

// Median color of a desktop-image strip, opaque pixels only, sorted by
// perceived grey (desktop shadow tint tracks the median, a stray hardware
// cursor sprite only contaminates a localized blob).
[[maybe_unused]] static QColor medianStripColor(const QImage &desktop, const QRect &strip)
{
    if (strip.isEmpty() || !desktop.rect().contains(strip))
        return QColor();
    QList<QColor> pixels;
    pixels.reserve(strip.width() * strip.height());
    for (int y = strip.top(); y <= strip.bottom(); ++y) {
        for (int x = strip.left(); x <= strip.right(); ++x) {
            const QColor color = desktop.pixelColor(x, y);
            if (color.alpha() == 255)
                pixels.append(color);
        }
    }
    if (pixels.isEmpty())
        return QColor();
    std::sort(pixels.begin(), pixels.end(),
              [](const QColor &a, const QColor &b) { return qGray(a.rgb()) < qGray(b.rgb()); });
    return pixels.at(pixels.size() / 2);
}

// One immutable desktop capture for correlated surface/background measurements.
struct DesktopTestFrame
{
    QImage image;
    QRect screenRect;

    static DesktopTestFrame capture(QScreen *screen)
    {
        return screen ? DesktopTestFrame{ screen->grabWindow(0).toImage(), screen->geometry() }
                      : DesktopTestFrame{};
    }
    QRect pixels(const QRect &global) const
    {
        if (image.isNull() || screenRect.isEmpty() || global.isEmpty())
            return {};
        const qreal sx = qreal(image.width()) / screenRect.width();
        const qreal sy = qreal(image.height()) / screenRect.height();
        return QRect(qRound((global.x() - screenRect.x()) * sx),
                     qRound((global.y() - screenRect.y()) * sy),
                     qMax(1, qRound(global.width() * sx)), qMax(1, qRound(global.height() * sy)));
    }
    QColor colorAt(const QPoint &global) const
    {
        const QRect local = pixels(QRect(global, QSize(1, 1)));
        return !local.isEmpty() && image.rect().contains(local) ? image.pixelColor(local.topLeft())
                                                                : QColor();
    }
    bool uniform(const QRect &global, const QColor &expected) const
    {
        const QRect local = pixels(global);
        if (local.isEmpty() || !image.rect().contains(local))
            return false;
        for (int y = local.top(); y <= local.bottom(); ++y)
            for (int x = local.left(); x <= local.right(); ++x)
                if (colorDistance(image.pixelColor(x, y), expected) > 2)
                    return false;
        return true;
    }
};

// Two pre-input frames must match the stock opaque host palette within
// tolerance 2 throughout the sampled fill region. Occlusion/cursor contamination
// blocks evidence; matching medians alone cannot establish uniformity.
inline bool stableOpaqueDesktopBaseline(QWidget &host, const QRect &globalRegion)
{
    const QColor expected = host.palette().color(QPalette::Window);
    QTest::qWait(500);
    const auto first = DesktopTestFrame::capture(WinUI3::Private::widgetScreen(&host));
    QTest::qWait(100);
    const auto second = DesktopTestFrame::capture(WinUI3::Private::widgetScreen(&host));
    const QColor firstMedian = first.image.isNull()
            ? QColor()
            : medianStripColor(first.image, first.pixels(globalRegion));
    const QColor secondMedian = second.image.isNull()
            ? QColor()
            : medianStripColor(second.image, second.pixels(globalRegion));
    const int firstDelta = firstMedian.isValid() ? colorDistance(firstMedian, expected) : 999;
    const int secondDelta = secondMedian.isValid() ? colorDistance(secondMedian, expected) : 999;
    const bool valid = host.isVisible() && expected.alpha() == 255
            && !host.testAttribute(Qt::WA_TranslucentBackground)
            && host.geometry().contains(globalRegion) && firstDelta <= 2 && secondDelta <= 2
            && first.uniform(globalRegion, expected) && second.uniform(globalRegion, expected);
    qWarning().noquote() << "desktop baseline" << (valid ? "ready" : "BLOCKED/unverifiable")
                         << "region=" << globalRegion << "expected=" << expected
                         << "firstMedian=" << firstMedian << "secondMedian=" << secondMedian
                         << "deltas=" << firstDelta << secondDelta;
    return valid;
}

// Same-frame DWM shadow probe for a settled popup: median near vs far strip
// per side, all from ONE desktop capture (host band, surface band and shadow
// strips must never come from different frames — desktop drift between grabs
// manufactured phantom depth deltas). Negative depth = near band darker.
// *usableSides counts sides whose strips stayed inside the host rect and the
// grab; the debug string always logs per-side near/far greys so an invalid
// side is visible instead of silently averaged away.
[[maybe_unused]] static void probeDesktopShadowDepth(const DesktopTestFrame &frame,
                                                     const QRect &popupFrame, const QRect &hostRect,
                                                     qreal *depth, int *usableSides, QString *debug)
{
    *depth = 0.0;
    *usableSides = 0;
    if (debug)
        *debug = QStringLiteral("BLOCKED: missing capture or shadow ring outside host");
    // The 16px ring around the popup must sit over the host's uniform
    // opaque fill; over wallpaper the near/far medians are meaningless.
    if (frame.image.isNull() || popupFrame.isEmpty() || hostRect.isEmpty()
        || !hostRect.contains(popupFrame.adjusted(-16, -16, 16, 16)))
        return;
    struct Side
    {
        QRect nearStrip;
        QRect farStrip;
    };
    const int cx = popupFrame.center().x();
    const int cy = popupFrame.center().y();
    const Side sides[4] = {
        { QRect(cx - 40, popupFrame.top() - 5, 80, 4),
          QRect(cx - 40, popupFrame.top() - 15, 80, 4) },
        { QRect(cx - 40, popupFrame.bottom() + 2, 80, 4),
          QRect(cx - 40, popupFrame.bottom() + 12, 80, 4) },
        { QRect(popupFrame.left() - 5, cy - 40, 4, 80),
          QRect(popupFrame.left() - 15, cy - 40, 4, 80) },
        { QRect(popupFrame.right() + 2, cy - 40, 4, 80),
          QRect(popupFrame.right() + 12, cy - 40, 4, 80) },
    };
    qreal total = 0.0;
    int count = 0;
    QStringList notes;
    for (const Side &side : sides) {
        const QRect nearLocal = frame.pixels(side.nearStrip);
        const QRect farLocal = frame.pixels(side.farStrip);
        if (nearLocal.isEmpty() || farLocal.isEmpty() || !frame.image.rect().contains(nearLocal)
            || !frame.image.rect().contains(farLocal)) {
            notes << QStringLiteral("excluded");
            continue;
        }
        const QColor nearColor = medianStripColor(frame.image, nearLocal);
        const QColor farColor = medianStripColor(frame.image, farLocal);
        notes << QStringLiteral("n=%1,f=%2")
                         .arg(nearColor.isValid() ? nearColor.name(QColor::HexRgb)
                                                  : QStringLiteral("?"))
                         .arg(farColor.isValid() ? farColor.name(QColor::HexRgb)
                                                 : QStringLiteral("?"));
        if (!nearColor.isValid() || !farColor.isValid())
            continue;
        const int sideDepth = qGray(nearColor.rgb()) - qGray(farColor.rgb());
        total += sideDepth;
        ++count;
    }
    *usableSides = count;
    *depth = count > 0 ? total / count : 0.0;
    if (debug)
        *debug = notes.join(QStringLiteral(";"));
}

class PopupGeometryProbe final : public QObject
{
public:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == popup && event->type() == QEvent::Show) {
            visible = true;
            if (combo && combo->currentIndex() >= 0) {
                const QModelIndex selected = combo->model()->index(
                        combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
                selectedCenterAtShow = combo->view()->viewport()->mapToGlobal(
                        combo->view()->visualRect(selected).center());
                scrollValueAtShow = combo->view()->verticalScrollBar()->value();
            }
            geometryAtShow = popup->geometry();
        } else if (object == popup && event->type() == QEvent::Hide) {
            visible = false;
        } else if (visible && object == popup && event->type() == QEvent::Move) {
            ++movesAfterShow;
        } else if (visible && event->type() == QEvent::Resize) {
            ++resizesAfterShow;
        } else if (visible && event->type() == QEvent::LayoutRequest) {
            ++layoutsAfterShow;
        }
        return false;
    }

    void reset()
    {
        visible = false;
        movesAfterShow = 0;
        resizesAfterShow = 0;
        selectedCenterAtShow = {};
        scrollValueAtShow = -1;
        geometryAtShow = {};
        layoutsAfterShow = 0;
    }

    bool visible = false;
    int movesAfterShow = 0;
    int resizesAfterShow = 0;
    int layoutsAfterShow = 0;
    QComboBox *combo = nullptr;
    QWidget *popup = nullptr;
    QPoint selectedCenterAtShow;
    int scrollValueAtShow = -1;
    QRect geometryAtShow;
};

class ExposedSplitter final : public QSplitter
{
public:
    using QSplitter::moveSplitter;
    using QSplitter::QSplitter;
};

class SolidPage final : public QWidget
{
public:
    SolidPage(const QColor &color, const QString &text, QWidget *parent = nullptr)
        : QWidget(parent), m_color(color), m_text(text)
    {
        setAutoFillBackground(false);
    }

protected:
    QSize sizeHint() const override { return QSize(300, 80); }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), m_color);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, m_text);
    }

private:
    QColor m_color;
    QString m_text;
};

class CountingHintWidget final : public QWidget
{
public:
    explicit CountingHintWidget(const QSize &hint, QWidget *parent = nullptr)
        : QWidget(parent), m_hint(hint)
    {
    }

    QSize sizeHint() const override
    {
        ++sizeHintCalls;
        return m_hint;
    }

    QSize m_hint;
    mutable int sizeHintCalls = 0;
};

class LayoutLifecycleProbe final : public QObject
{
public:
    bool eventFilter(QObject *, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::LayoutRequest:
            ++layoutRequests;
            break;
        case QEvent::Resize:
            ++resizes;
            break;
        case QEvent::Paint:
            ++paints;
            break;
        default:
            break;
        }
        return false;
    }

    int layoutRequests = 0;
    int resizes = 0;
    int paints = 0;
};

class UpdateRequestProbe final : public QObject
{
public:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::UpdateRequest)
            ++updateRequests;
        else if (event->type() == QEvent::Paint)
            ++paints;
        return false;
    }

    int updateRequests = 0;
    int paints = 0;
};

class DisableAnimationsGuard final
{
public:
    DisableAnimationsGuard()
        : existed(qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS")),
          previous(qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS"))
    {
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    }

    ~DisableAnimationsGuard()
    {
        if (existed)
            qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previous);
        else
            qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    }

    bool existed;
    QByteArray previous;
};

[[maybe_unused]] static void verifyHitSurface(const QStyle *style, QStyle::ComplexControl control,
                                              const QStyleOptionComplex *option,
                                              const QWidget *widget,
                                              const QRect &interactiveRect = {},
                                              const QList<QRect> &additionalHitRegions = {})
{
    const QRect rect =
            interactiveRect.isValid() ? interactiveRect.intersected(option->rect) : option->rect;
    const QList<QPoint> edges = { rect.topLeft(),
                                  rect.topRight(),
                                  rect.bottomLeft(),
                                  rect.bottomRight(),
                                  QPoint(rect.center().x(), rect.top()),
                                  QPoint(rect.center().x(), rect.bottom()),
                                  QPoint(rect.left(), rect.center().y()),
                                  QPoint(rect.right(), rect.center().y()),
                                  rect.center() };
    for (const QPoint &point : edges)
        QVERIFY2(style->hitTestComplexControl(control, option, point, widget) != QStyle::SC_None,
                 qPrintable(QStringLiteral("hole at %1,%2").arg(point.x()).arg(point.y())));

    // These are logical control-sized surfaces, so an exhaustive integer
    // raster catches one-pixel holes at fractional-DPR rounding boundaries.
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        for (int x = rect.left(); x <= rect.right(); ++x) {
            const QPoint point(x, y);
            QVERIFY2(style->hitTestComplexControl(control, option, point, widget)
                             != QStyle::SC_None,
                     qPrintable(QStringLiteral("hole at %1,%2").arg(point.x()).arg(point.y())));
        }
    }

    const QList<QPoint> outside = { rect.topLeft() - QPoint(1, 1),
                                    rect.topRight() + QPoint(1, -1),
                                    rect.bottomLeft() + QPoint(-1, 1),
                                    rect.bottomRight() + QPoint(1, 1),
                                    QPoint(rect.left() - 1, rect.center().y()),
                                    QPoint(rect.right() + 1, rect.center().y()),
                                    QPoint(rect.center().x(), rect.top() - 1),
                                    QPoint(rect.center().x(), rect.bottom() + 1) };
    for (const QPoint &point : outside) {
        bool allowed = false;
        for (const QRect &region : additionalHitRegions)
            allowed = allowed || region.contains(point);
        if (allowed)
            continue;
        const QStyle::SubControl hit = style->hitTestComplexControl(control, option, point, widget);
        QVERIFY2(hit == QStyle::SC_None,
                 qPrintable(QStringLiteral("outside point %1,%2 hit %3")
                                    .arg(point.x())
                                    .arg(point.y())
                                    .arg(int(hit))));
    }
}

[[maybe_unused]] static QImage renderComplex(const QStyle *style, QStyle::ComplexControl control,
                                             const QStyleOptionComplex *option,
                                             const QWidget *widget, qreal dpr)
{
    const QSize physical(qRound(option->rect.width() * dpr), qRound(option->rect.height() * dpr));
    QImage image(physical, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(widget->palette().color(QPalette::Window));
    QPainter painter(&image);
    style->drawComplexControl(control, option, &painter, widget);
    return image;
}

[[maybe_unused]] static int inkPixels(const QImage &image, const QRect &logicalRect, qreal dpr,
                                      const QColor &background)
{
    const QRect physical(qFloor(logicalRect.left() * dpr), qFloor(logicalRect.top() * dpr),
                         qCeil((logicalRect.right() + 1) * dpr) - qFloor(logicalRect.left() * dpr),
                         qCeil((logicalRect.bottom() + 1) * dpr) - qFloor(logicalRect.top() * dpr));
    int count = 0;
    for (int y = physical.top(); y <= physical.bottom(); ++y) {
        for (int x = physical.left(); x <= physical.right(); ++x) {
            if (image.rect().contains(x, y)
                && colorDistance(image.pixelColor(x, y), background) > 8)
                ++count;
        }
    }
    return count;
}

[[maybe_unused]] static qreal frameReal(const QObject *object, const char *name,
                                        qreal fallback = 0.0)
{
    return WinUI3::Private::framePropertyRegistry().real(object, name, fallback);
}

[[maybe_unused]] static bool frameBool(const QObject *object, const char *name,
                                       bool fallback = false)
{
    const QVariant value = WinUI3::Private::framePropertyRegistry().value(object, name);
    return value.isValid() ? value.toBool() : fallback;
}

[[maybe_unused]] static QVariant frameValue(const QObject *object, const char *name)
{
    return WinUI3::Private::framePropertyRegistry().value(object, name);
}

[[maybe_unused]] static void setFrame(QObject *object, const char *name, const QVariant &value)
{
    WinUI3::Private::framePropertyRegistry().set(object, name, value);
}

class FrameDynamicPropertyProbe final : public QObject
{
public:
    int frameChanges = 0;

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() != QEvent::DynamicPropertyChange)
            return false;
        const auto *change = static_cast<QDynamicPropertyChangeEvent *>(event);
        if (change->propertyName() == QByteArrayLiteral("_winui_hover_progress"))
            ++frameChanges;
        return false;
    }
};

// QTreeWidget::indexFromItem is protected in Qt 5 (public since Qt 6).
// Resolve through the model instead: top-level items are row(r),0 and
// children are row(c),0 under their parent index.
inline QModelIndex treeIndexFromItem(const QTreeWidget *tree, QTreeWidgetItem *item)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return tree->indexFromItem(item);
#else
    if (!tree || !item)
        return QModelIndex();
    QTreeWidgetItem *parent = item->parent();
    if (!parent) {
        const int row = tree->indexOfTopLevelItem(item);
        return row >= 0 ? tree->model()->index(row, 0) : QModelIndex();
    }
    return tree->model()->index(parent->indexOfChild(item), 0, treeIndexFromItem(tree, parent));
#endif
}

// QLabel::pixmap(Qt::ReturnByValue) is Qt 6; Qt 5.12 has pixmap() returning
// const QPixmap*. Tests only need null/image checks: copy out, null if unset.
inline QPixmap labelPixmap(const QLabel *label)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return label->pixmap(Qt::ReturnByValue);
#else
    const QPixmap *pixmap = label->pixmap();
    return pixmap ? *pixmap : QPixmap();
#endif
}

// QAction::associatedObjects is Qt 6; Qt 5.12 has associatedWidgets.
// Tests only check membership of a button: compare widget pointers.
inline bool actionAssociatedWith(const QAction *action, const QWidget *widget)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return action->associatedObjects().contains(const_cast<QWidget *>(widget));
#else
    return action->associatedWidgets().contains(const_cast<QWidget *>(widget));
#endif
}
