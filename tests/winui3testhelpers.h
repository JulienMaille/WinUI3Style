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

#include <cmath>
#include <limits>

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

static int colorDistance(const QColor &a, const QColor &b)
{
    return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue())
            + qAbs(a.alpha() - b.alpha());
}

static void verifyHitSurface(const QStyle *style, QStyle::ComplexControl control,
                             const QStyleOptionComplex *option, const QWidget *widget,
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

static QImage renderComplex(const QStyle *style, QStyle::ComplexControl control,
                            const QStyleOptionComplex *option, const QWidget *widget, qreal dpr)
{
    const QSize physical(qRound(option->rect.width() * dpr), qRound(option->rect.height() * dpr));
    QImage image(physical, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(dpr);
    image.fill(widget->palette().color(QPalette::Window));
    QPainter painter(&image);
    style->drawComplexControl(control, option, &painter, widget);
    return image;
}

static int inkPixels(const QImage &image, const QRect &logicalRect, qreal dpr,
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

static qreal frameReal(const QObject *object, const char *name, qreal fallback = 0.0)
{
    return WinUI3::Private::framePropertyRegistry().real(object, name, fallback);
}

static bool frameBool(const QObject *object, const char *name, bool fallback = false)
{
    const QVariant value = WinUI3::Private::framePropertyRegistry().value(object, name);
    return value.isValid() ? value.toBool() : fallback;
}

static QVariant frameValue(const QObject *object, const char *name)
{
    return WinUI3::Private::framePropertyRegistry().value(object, name);
}

static void setFrame(QObject *object, const char *name, const QVariant &value)
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
