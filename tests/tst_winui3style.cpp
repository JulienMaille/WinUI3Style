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
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
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
#include <QSpinBox>
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
    using QSplitter::QSplitter;
    using QSplitter::moveSplitter;
};

class SolidPage final : public QWidget
{
public:
    SolidPage(const QColor &color, const QString &text,
              QWidget *parent = nullptr)
        : QWidget(parent)
        , m_color(color)
        , m_text(text)
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
        : QWidget(parent)
        , m_hint(hint)
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
        case QEvent::LayoutRequest: ++layoutRequests; break;
        case QEvent::Resize: ++resizes; break;
        case QEvent::Paint: ++paints; break;
        default: break;
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
        : existed(qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS"))
        , previous(qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS"))
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
    return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
        + qAbs(a.blue() - b.blue()) + qAbs(a.alpha() - b.alpha());
}

static void verifyHitSurface(const QStyle *style, QStyle::ComplexControl control,
                             const QStyleOptionComplex *option,
                             const QWidget *widget,
                             const QRect &interactiveRect = {},
                             const QList<QRect> &additionalHitRegions = {})
{
    const QRect rect = interactiveRect.isValid()
        ? interactiveRect.intersected(option->rect) : option->rect;
    const QList<QPoint> edges = {
        rect.topLeft(), rect.topRight(), rect.bottomLeft(), rect.bottomRight(),
        QPoint(rect.center().x(), rect.top()),
        QPoint(rect.center().x(), rect.bottom()),
        QPoint(rect.left(), rect.center().y()),
        QPoint(rect.right(), rect.center().y()), rect.center()
    };
    for (const QPoint &point : edges)
        QVERIFY2(style->hitTestComplexControl(control, option, point, widget)
                     != QStyle::SC_None,
                 qPrintable(QStringLiteral("hole at %1,%2")
                                .arg(point.x()).arg(point.y())));

    // These are logical control-sized surfaces, so an exhaustive integer
    // raster catches one-pixel holes at fractional-DPR rounding boundaries.
    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        for (int x = rect.left(); x <= rect.right(); ++x) {
            const QPoint point(x, y);
            QVERIFY2(style->hitTestComplexControl(control, option, point, widget)
                         != QStyle::SC_None,
                     qPrintable(QStringLiteral("hole at %1,%2")
                                    .arg(point.x()).arg(point.y())));
        }
    }

    const QList<QPoint> outside = {
        rect.topLeft() - QPoint(1, 1), rect.topRight() + QPoint(1, -1),
        rect.bottomLeft() + QPoint(-1, 1), rect.bottomRight() + QPoint(1, 1),
        QPoint(rect.left() - 1, rect.center().y()),
        QPoint(rect.right() + 1, rect.center().y()),
        QPoint(rect.center().x(), rect.top() - 1),
        QPoint(rect.center().x(), rect.bottom() + 1)
    };
    for (const QPoint &point : outside) {
        bool allowed = false;
        for (const QRect &region : additionalHitRegions)
            allowed = allowed || region.contains(point);
        if (allowed)
            continue;
        const QStyle::SubControl hit = style->hitTestComplexControl(
            control, option, point, widget);
        QVERIFY2(hit == QStyle::SC_None,
                 qPrintable(QStringLiteral("outside point %1,%2 hit %3")
                                .arg(point.x()).arg(point.y()).arg(int(hit))));
    }
}

static QImage renderComplex(const QStyle *style, QStyle::ComplexControl control,
                            const QStyleOptionComplex *option,
                            const QWidget *widget, qreal dpr)
{
    const QSize physical(qRound(option->rect.width() * dpr),
                         qRound(option->rect.height() * dpr));
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
    const QRect physical(
        qFloor(logicalRect.left() * dpr),
        qFloor(logicalRect.top() * dpr),
        qCeil((logicalRect.right() + 1) * dpr)
            - qFloor(logicalRect.left() * dpr),
        qCeil((logicalRect.bottom() + 1) * dpr)
            - qFloor(logicalRect.top() * dpr));
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

class WinUI3StyleTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void palettes();
    void paletteDerivedTokensMatchWinUIConstants();
    void customWidgetPaletteDrivesTokensAndPaint();
    void systemAccentRampIsAtomic();
    void buttonToolButtonAndIconContracts();
    void buttonPressedStateFollowsQtState();
    void buttonPressedForegroundRoles();
    void coloredIconCacheReuseAndPixelContract();
    void iconPixmapCacheDprAndPalette();
    void buttonPressedPulseContract();
    void disabledButtonHasNoInteractionState();
    void toolButtonIconVerticalCenter();
    void styleMutationRestoration();
    void accessibilityOwnershipContracts();
    void baseStyleContract();
    void controlRoles();
    void toggleConvenienceWidget();
    void toggleInteraction();
    void togglePressedThumbGeometry();
    void toggleDragInteraction();
    void toggleRtlGeometryAndInteraction();
    void backdropLifecycleContract();
    void backdropButtonRepaintDoesNotAccumulate();
    void settingsCardExpansion();
    void settingsCardDesignerPropertyBindings();
    void settingsCardTrailingWidgetsReceiveClicks();
    void settingsCardTrailingWidgetsHaveUniformHeight();
    void settingsCardChevronAndStableHeader();
    void settingsCardExpansionLoad();
    void settingsCardInteractiveFrames();
    void settingsCardExpansionInScrollingPage();
    void navigationTransition();
    void navigationInteractiveFrames();
    void renderCommonStates();
    void pluginFactory();
    void inputModalityFocus();
    void hoverAnimationProgresses();
    void textBoxInteraction();
    void clearButtonStateContract();
    void clearButtonFocusAndGlyphContract();
    void textBoxStateMatrix();
    void themeComboSizingContract();
    void indeterminateProgressDeterminism();
    void comboPopupContract();
    void comboReleaseActivationAndMarkerMotion();
    void comboPopupAssociationLifecycle();
    void comboChevronMotion();
    void comboChevronGeometry();
    void numberBoxSubcontrolContract();
    void verticalNumberBoxContract();
    void spinBoxFocusUnderlinePixelContract();
    void checkboxAcceptAnimation();
    void checkboxGlyphGeometryContract();
    void lightModeIndicatorOnAccentIsWhite();
    void darkModeIndicatorOnAccentIsBlack();
    void customAccentKeepsThemeTextSeparateFromControlInk();
    void checkboxGapHitTest();
    void checkboxDisabledStopsAnimation();
    void radioStateMotion();
    void radioDotDpiGeometry();
    void menuSizingContract();
    void menuSubmenuChevronGeometry();
    void menuPaintTabParsing();
    void menuBarOnlyActiveActionIsHighlighted();
    void groupBoxContract();
    void splitterHandleContract();
    void splitterGripPixelAlignment();
    void dockWidgetContract();
    void sliderGeometryContract();
    void sliderStateMotion();
    void sliderDragInteraction();
    void sliderValueToolTipAndFocus();
    void scrollBarContract();
    void scrollBarHorizontalAndReentry();
    void scrollAreaScrollBarIntegration();
    void tabViewContract();
    void listViewContract();
    void itemViewGutterContract();
    void treeViewContract();
    void treeSelectionMarkerLeadingEdge();
    void tableHeaderContract();
    void tableSortIndicatorGeometryContract();
    void tableEditingPaintContract();
    void tableLiveEditorSuppressesDisplay();
    void richEditBoxContract();
    void contentDialogContract();
    void contentDialogScrimLifecycle();
    void readOnlyActionRestoration();
    void animatedStackEffectsAndInterruption();
    void animatedStackLifecycleStress();
    void progressAnimationAndOrientations();
    void progressTextAndDisabledPaletteContract();
    void sliderExtremeRangeTicks();
    void rtlGeometryAndHitTesting();
    void itemViewMouseFocusReset();
    void checkboxAndRadioUncheckMotion();
    void navigationModelReconnectAndScroll();
    void navigationDelegateLifecycle();
    void runtimeAppearanceAndDialogLifecycle();
    void progressTimerScalingAndLifecycle();
    void callbackCoalescingAndAnimationReuse();
    void dpiGeometry();
    void dpiHitTestContracts();
};

void WinUI3StyleTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3StyleTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3StyleTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
}

void WinUI3StyleTest::palettes()
{
    WinUI3::Style light(WinUI3::ThemeMode::Light);
    WinUI3::Style dark(WinUI3::ThemeMode::Dark);
    QVERIFY(light.standardPalette().color(QPalette::Window).lightness()
            > dark.standardPalette().color(QPalette::Window).lightness());
    QCOMPARE(light.standardPalette().color(QPalette::Highlight), light.accentColor());
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QVERIFY(light.standardPalette().color(QPalette::Accent).lightness()
            < light.accentColor().lightness());
    QVERIFY(dark.standardPalette().color(QPalette::Accent).lightness()
            > dark.accentColor().lightness());
#endif
    QCOMPARE(light.standardPalette().color(QPalette::Button), QColor(255, 255, 255, 179));
    QCOMPARE(light.standardPalette().color(QPalette::WindowText), QColor(0, 0, 0, 228));
    QCOMPARE(light.standardPalette().color(QPalette::PlaceholderText),
             QColor(0, 0, 0, 158));
    QCOMPARE(dark.standardPalette().color(QPalette::Button), QColor(255, 255, 255, 15));
    QCOMPARE(dark.standardPalette().color(QPalette::PlaceholderText),
             QColor(255, 255, 255, 197));
}

void WinUI3StyleTest::paletteDerivedTokensMatchWinUIConstants()
{
    // Guard the palette-derived token pipeline: building tokens from the
    // style's own standard palette must reproduce the WinUI theme resources
    // byte-for-byte, so the refactor is render-neutral for default palettes.
    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        WinUI3::Style style(mode);
        const QPalette palette = style.standardPalette();
        const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);
        const bool dark = mode == WinUI3::ThemeMode::Dark;

        QCOMPARE(t.dark, dark);
        QCOMPARE(t.textPrimary, dark ? QColor(255, 255, 255)
                                     : QColor(0, 0, 0, 228));
        QCOMPARE(t.textSecondary, dark ? QColor(255, 255, 255, 197)
                                       : QColor(0, 0, 0, 158));
        QCOMPARE(t.textTertiary, dark ? QColor(255, 255, 255, 135)
                                      : QColor(0, 0, 0, 114));
        QCOMPARE(t.textDisabled, dark ? QColor(255, 255, 255, 93)
                                      : QColor(0, 0, 0, 92));
        QCOMPARE(t.layer, dark ? QColor(58, 58, 58, 76)
                               : QColor(255, 255, 255, 128));
        QCOMPARE(t.control, dark ? QColor(255, 255, 255, 15)
                                 : QColor(255, 255, 255, 179));
        QCOMPARE(t.controlHover, dark ? QColor(255, 255, 255, 21)
                                      : QColor(249, 249, 249, 128));
        QCOMPARE(t.controlPressed, dark ? QColor(255, 255, 255, 8)
                                        : QColor(229, 229, 229, 179));
        QCOMPARE(t.controlDisabled, dark ? QColor(255, 255, 255, 11)
                                         : QColor(249, 249, 249, 77));
        QCOMPARE(t.subtleHover, dark ? QColor(255, 255, 255, 15)
                                     : QColor(0, 0, 0, 15));
        QCOMPARE(t.subtlePressed, dark ? QColor(255, 255, 255, 10)
                                       : QColor(0, 0, 0, 22));
        QCOMPARE(t.stroke, dark ? QColor(255, 255, 255, 18)
                                : QColor(0, 0, 0, 15));
        QCOMPARE(t.strokeSecondary, dark ? QColor(255, 255, 255, 24)
                                         : QColor(0, 0, 0, 41));
        QCOMPARE(t.strokeStrong, dark ? QColor(255, 255, 255, 139)
                                      : QColor(0, 0, 0, 114));

        const QColor popup = WinUI3::Private::popupSurfaceColor(palette);
        QCOMPARE(popup, dark ? QColor(44, 44, 44) : QColor(252, 252, 252));
    }
}

void WinUI3StyleTest::customWidgetPaletteDrivesTokensAndPaint()
{
    // A widget-level palette override must flow into the derived tokens, and
    // the painters must follow it. Use a strongly saturated red ink so any
    // leak from a hardcoded white/black text token shows up in the pixel.
    QPalette palette = qApp->palette();
    palette.setColor(QPalette::WindowText, QColor(220, 32, 32));
    palette.setColor(QPalette::ButtonText, QColor(220, 32, 32));
    palette.setColor(QPalette::Text, QColor(220, 32, 32));

    const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);
    QCOMPARE(t.textPrimary, palette.color(QPalette::WindowText));
    QCOMPARE(t.textSecondary.rgb(), palette.color(QPalette::WindowText).rgb());

    QPushButton button(QStringLiteral("Palette probe"));
    button.setPalette(palette);
    button.resize(120, 40);
    QPixmap pixmap = button.grab();
    const QImage image = pixmap.toImage();
    bool foundRedInk = false;
    for (int y = 0; y < image.height() && !foundRedInk; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const int chroma = pixel.red() - qMax(pixel.green(),
                                                  pixel.blue());
            if (chroma > 60) {
                foundRedInk = true;
                break;
            }
        }
    }
    QVERIFY2(foundRedInk,
             "button painted with a custom palette shows no custom-ink pixels");
}

void WinUI3StyleTest::systemAccentRampIsAtomic()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setAccentColor({});

    const QColor systemAccent = style->accentColor();
    QVERIFY(systemAccent.isValid());
    const auto sameHueFamily = [](const QColor &a, const QColor &b) {
        const QColor first = a.toHsv();
        const QColor second = b.toHsv();
        // A near-gray accent has no stable hue; its roles are still coherent
        // when both are achromatic.
        if (first.saturation() < 16 || second.saturation() < 16)
            return first.saturation() < 16 && second.saturation() < 16;
        const int distance = qAbs(first.hue() - second.hue());
        return qMin(distance, 360 - distance) <= 8;
    };

    QCOMPARE(style->standardPalette().color(QPalette::Highlight), systemAccent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        style->setThemeMode(mode);
        const QPalette palette = style->standardPalette();
        const QColor controlAccent = palette.color(QPalette::Accent);
        QVERIFY(controlAccent.isValid());
        QVERIFY2(sameHueFamily(systemAccent, controlAccent),
                 qPrintable(QStringLiteral("system %1, control %2")
                                .arg(systemAccent.name(), controlAccent.name())));
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
#endif
}

void WinUI3StyleTest::buttonToolButtonAndIconContracts()
{
    const QIcon fluent = WinUI3::icon(WinUI3::Icon::Search);
    QVERIFY(WinUI3::isFluentIcon(fluent));
    QIcon lastGenerated;
    for (int index = 0; index < 2500; ++index)
        lastGenerated = WinUI3::icon(WinUI3::Icon::Search);
    QVERIFY(WinUI3::isFluentIcon(fluent));
    QVERIFY(WinUI3::isFluentIcon(lastGenerated));
    const QPixmap red = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 1.0, QColor(220, 20, 40));
    const QPixmap blue = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 1.0, QColor(20, 60, 220));
    QVERIFY(!red.isNull());
    QVERIFY(red.toImage() != blue.toImage());

    QPixmap applicationPixmap(16, 16);
    applicationPixmap.fill(QColor(20, 200, 40));
    const QIcon applicationIcon(applicationPixmap);
    QVERIFY(!WinUI3::isFluentIcon(applicationIcon));
    const QImage preserved = WinUI3::iconPixmap(
        applicationIcon, QSize(16, 16), 1.0, QColor(220, 20, 40)).toImage();
    const QColor center = preserved.pixelColor(8, 8);
    QVERIFY(center.green() > center.red());

    const QPalette originalApplicationPalette = qApp->palette();
    QPalette iconPalette = originalApplicationPalette;
    const QIcon semanticArrow = qApp->style()->standardIcon(QStyle::SP_ArrowDown);
    iconPalette.setColor(QPalette::WindowText, QColor(210, 30, 50));
    qApp->setPalette(iconPalette);
    const QImage redArrow = semanticArrow.pixmap(QSize(20, 20)).toImage();
    iconPalette.setColor(QPalette::WindowText, QColor(30, 70, 210));
    qApp->setPalette(iconPalette);
    const QImage blueArrow = semanticArrow.pixmap(QSize(20, 20)).toImage();
    qApp->setPalette(originalApplicationPalette);
    QVERIFY(redArrow != blueArrow);

    QPushButton push(QStringLiteral("Open"));
    push.setIcon(fluent);
    push.resize(140, 32);
    push.show();
    QVERIFY(!push.grab().isNull());

    QToolBar toolbar;
    auto *tool = new QToolButton(&toolbar);
    tool->setText(QStringLiteral("Options"));
    tool->setIcon(fluent);
    tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tool->setPopupMode(QToolButton::MenuButtonPopup);
    auto *menu = new QMenu(tool);
    menu->addAction(QStringLiteral("Choice"));
    tool->setMenu(menu);
    toolbar.addWidget(tool);
    toolbar.setProperty(WinUI3::Style::SurfaceProperty,
                        QStringLiteral("layer"));
    toolbar.show();
    QVERIFY(!toolbar.grab().isNull());

    QStyleOption toolbarPanel;
    toolbarPanel.initFrom(&toolbar);
    toolbarPanel.rect = QRect(0, 0, 80, 24);
    QImage toolbarPanelImage(toolbarPanel.rect.size(),
                             QImage::Format_ARGB32_Premultiplied);
    toolbarPanelImage.fill(Qt::black);
    {
        QPainter painter(&toolbarPanelImage);
        toolbar.style()->drawControl(QStyle::CE_ToolBar, &toolbarPanel,
                                     &painter, &toolbar);
    }
    QCOMPARE(toolbarPanelImage.pixelColor(70, 12),
             toolbar.palette().color(QPalette::Window));

    QStyleOptionToolButton option;
    option.initFrom(tool);
    option.rect = tool->rect();
    option.features = QStyleOptionToolButton::MenuButtonPopup;
    const QRect main = tool->style()->subControlRect(
        QStyle::CC_ToolButton, &option, QStyle::SC_ToolButton, tool);
    const QRect drop = tool->style()->subControlRect(
        QStyle::CC_ToolButton, &option, QStyle::SC_ToolButtonMenu, tool);
    QVERIFY(!main.intersects(drop));
    QCOMPARE(tool->style()->hitTestComplexControl(
                 QStyle::CC_ToolButton, &option, main.center(), tool),
             QStyle::SC_ToolButton);
    QCOMPARE(tool->style()->hitTestComplexControl(
                 QStyle::CC_ToolButton, &option, drop.center(), tool),
             QStyle::SC_ToolButtonMenu);

    QStyleOption separator;
    separator.palette = toolbar.palette();
    separator.rect = QRect(0, 0, 24, 24);
    separator.state = QStyle::State_Enabled | QStyle::State_Horizontal;
    QImage horizontalToolbarSeparator(separator.rect.size(),
                                      QImage::Format_ARGB32_Premultiplied);
    horizontalToolbarSeparator.fill(Qt::transparent);
    {
        QPainter painter(&horizontalToolbarSeparator);
        toolbar.style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator,
                                       &separator, &painter, &toolbar);
    }
    separator.state = QStyle::State_Enabled;
    QImage verticalToolbarSeparator(separator.rect.size(),
                                    QImage::Format_ARGB32_Premultiplied);
    verticalToolbarSeparator.fill(Qt::transparent);
    {
        QPainter painter(&verticalToolbarSeparator);
        toolbar.style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator,
                                       &separator, &painter, &toolbar);
    }
    QVERIFY(horizontalToolbarSeparator != verticalToolbarSeparator);

    QStyleOptionToolButton hoverOption;
    hoverOption.initFrom(tool);
    hoverOption.rect = tool->rect();
    hoverOption.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    const QColor toolbarSurface = toolbar.palette().color(QPalette::Window);
    QImage hoverTrail(tool->size(), QImage::Format_ARGB32_Premultiplied);
    hoverTrail.fill(toolbarSurface);
    WinUI3::Private::framePropertyRegistry().set(
        tool, "_winui_hover_progress", 1.0);
    {
        QPainter painter(&hoverTrail);
        tool->style()->drawPrimitive(QStyle::PE_PanelButtonTool,
                                     &hoverOption, &painter, tool);
    }
    QVERIFY(hoverTrail.pixelColor(tool->rect().center()) != toolbarSurface);
    hoverOption.state = QStyle::State_Enabled;
    WinUI3::Private::framePropertyRegistry().set(
        tool, "_winui_hover_progress", 0.0);
    {
        QPainter painter(&hoverTrail);
        tool->style()->drawPrimitive(QStyle::PE_PanelButtonTool,
                                     &hoverOption, &painter, tool);
    }
    QCOMPARE(hoverTrail.pixelColor(tool->rect().center()), toolbarSurface);
}

void WinUI3StyleTest::iconPixmapCacheDprAndPalette()
{
    const QIcon fluent = WinUI3::icon(WinUI3::Icon::Search);
    const QColor foreground(30, 110, 220, 211);
    const QPixmap first = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 1.5, foreground);
    const QPixmap second = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 1.5, foreground);
    QVERIFY(!first.isNull());
    QCOMPARE(first.devicePixelRatioF(), 1.5);
    QCOMPARE(first.size(), QSize(30, 30));
    QCOMPARE(first.toImage(), second.toImage());

    const QPixmap differentDpr = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 2.0, foreground);
    QVERIFY(!differentDpr.isNull());
    QCOMPARE(differentDpr.devicePixelRatioF(), 2.0);
    QCOMPARE(differentDpr.size(), QSize(40, 40));

    const QPixmap disabled = WinUI3::iconPixmap(
        fluent, QSize(20, 20), 1.5, foreground, QIcon::Disabled,
        QIcon::On);
    QVERIFY(!disabled.isNull());
    QCOMPARE(disabled.size(), QSize(30, 30));

    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    // Tokens derive from palette roles, so probe palettes must carry a
    // matching ink alongside the surface color, exactly like the light/dark
    // standard palettes do.
    auto renderArrow = [style](const QColor &windowColor,
                               const QColor &inkColor) {
        QStyleOption option;
        option.rect = QRect(0, 0, 24, 24);
        option.state = QStyle::State_Enabled;
        option.palette = qApp->palette();
        option.palette.setColor(QPalette::Window, windowColor);
        option.palette.setColor(QPalette::WindowText, inkColor);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &option,
                             &painter);
        return image;
    };
    const QImage light = renderArrow(Qt::white, QColor(0, 0, 0, 228));
    const QImage dark = renderArrow(Qt::black, QColor(255, 255, 255));
    QVERIFY(light != dark);
}

void WinUI3StyleTest::coloredIconCacheReuseAndPixelContract()
{
    QVERIFY(WinUI3::icon(static_cast<WinUI3::Icon>(-1)).isNull());
    QVERIFY(WinUI3::icon(static_cast<WinUI3::Icon>(999)).isNull());

    const QColor foreground(27, 108, 219, 231);
    const QSize logicalSize(20, 20);
    const qreal devicePixelRatio = 1.5;

    const QIcon first = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    const QPixmap firstPixmap = first.pixmap(logicalSize, devicePixelRatio,
                                             QIcon::Normal, QIcon::Off);
    QVERIFY(!firstPixmap.isNull());

    const auto paintDirect = [logicalSize](const QIcon &source) {
        QImage image(logicalSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        source.paint(&painter, QRect(QPoint(), logicalSize), Qt::AlignCenter,
                     QIcon::Normal, QIcon::Off);
        return image;
    };
    const QImage firstDirectPaint = paintDirect(first);
    QVERIFY(!firstDirectPaint.isNull());

    // A repeated request for the same glyph/color must reuse the cached
    // engine. cacheKey equality is the allocation/lifetime proxy; comparing
    // every raster state guards the public output at the same time.
    const QIcon second = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    QCOMPARE(first.cacheKey(), second.cacheKey());
    for (const QIcon::Mode mode : {QIcon::Normal, QIcon::Disabled,
                                   QIcon::Active, QIcon::Selected}) {
        for (const QIcon::State state : {QIcon::Off, QIcon::On}) {
            QCOMPARE(first.pixmap(logicalSize, devicePixelRatio, mode, state)
                         .toImage(),
                     second.pixmap(logicalSize, devicePixelRatio, mode, state)
                         .toImage());
        }
    }

    QCOMPARE(firstPixmap.devicePixelRatio(), devicePixelRatio);
    QVERIFY(first.cacheKey() != WinUI3::icon(WinUI3::Icon::ChevronDown,
                                               QColor(28, 108, 219, 231))
                 .cacheKey());

    // Force the bounded cache past its capacity and compare a newly-created
    // engine with the original. This catches accidental changes to the
    // QIconEngine raster path while exercising the eviction policy.
    for (int index = 0; index < 300; ++index) {
        const QColor uniqueColor((index * 53) % 256, (index * 97) % 256,
                                 (index * 193) % 256, 200 + (index % 56));
        (void)WinUI3::icon(WinUI3::Icon::ChevronDown, uniqueColor);
    }
    const QIcon rebuilt = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    QVERIFY(rebuilt.cacheKey() != first.cacheKey());
    QCOMPARE(rebuilt.pixmap(logicalSize, devicePixelRatio, QIcon::Normal,
                            QIcon::Off)
                 .toImage(),
             firstPixmap.toImage());
    QCOMPARE(paintDirect(rebuilt), firstDirectPaint);
}

static qreal frameReal(const QObject *object, const char *name,
                       qreal fallback = 0.0)
{
    return WinUI3::Private::framePropertyRegistry().real(object, name, fallback);
}

static bool frameBool(const QObject *object, const char *name,
                      bool fallback = false)
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

void WinUI3StyleTest::buttonPressedPulseContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ControlRole role : {WinUI3::ControlRole::Standard,
                                           WinUI3::ControlRole::Accent,
                                           WinUI3::ControlRole::Subtle,
                                           WinUI3::ControlRole::Destructive}) {
        QPushButton button(QStringLiteral("Rapid"));
        WinUI3::Style::setControlRole(&button, role);
        button.resize(120, 32);
        button.show();
        (void)QTest::qWaitForWindowExposed(&button);
        QTest::mouseMove(&button, button.rect().center());
        const QImage rest = button.grab().toImage();

        for (int click = 0; click < 5; ++click) {
            QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier,
                              button.rect().center());
            qApp->processEvents();
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 1.0);
            const QImage pressed = button.grab().toImage();
            QVERIFY2(pressed != rest, "a left press must produce a visible frame");
            QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier,
                                button.rect().center());
        }
        QTest::qWait(130);
        QVERIFY(frameReal(&button, "_winui_press_progress") < 0.1);

        QTest::mousePress(&button, Qt::RightButton, Qt::NoModifier,
                          button.rect().center());
        QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);
        QTest::mouseRelease(&button, Qt::RightButton, Qt::NoModifier,
                            button.rect().center());
        QTest::mousePress(&button, Qt::MiddleButton, Qt::NoModifier,
                          button.rect().center());
        QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);
        QTest::mouseRelease(&button, Qt::MiddleButton, Qt::NoModifier,
                            button.rect().center());
    }

    QToolButton tool;
    tool.setText(QStringLiteral("Rapid tool"));
    tool.resize(120, 32);
    tool.show();
    (void)QTest::qWaitForWindowExposed(&tool);
    QTest::mouseMove(&tool, tool.rect().center());
    const QImage toolRest = tool.grab().toImage();
    QTest::mousePress(&tool, Qt::LeftButton, Qt::NoModifier,
                      tool.rect().center());
    qApp->processEvents();
    QCOMPARE(frameReal(&tool, "_winui_press_progress"), 1.0);
    QVERIFY(tool.grab().toImage() != toolRest);
    QTest::mouseRelease(&tool, Qt::LeftButton, Qt::NoModifier,
                        tool.rect().center());
    QTest::qWait(130);
    QVERIFY(frameReal(&tool, "_winui_press_progress") < 0.1);
}

void WinUI3StyleTest::buttonPressedStateFollowsQtState()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ControlRole role : {WinUI3::ControlRole::Standard,
                                           WinUI3::ControlRole::Subtle}) {
        QPushButton button(QStringLiteral("Pressed"));
        WinUI3::Style::setControlRole(&button, role);
        button.resize(120, 32);
        button.ensurePolished();
        setFrame(&button, "_winui_hover_progress", 1.0);
        setFrame(&button, "_winui_press_progress", 0.0);

        QStyleOptionButton option;
        option.initFrom(&button);
        option.rect = button.rect();
        option.palette = button.palette();
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        const auto render = [&](QStyle::State state) {
            option.state = state;
            QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(option.palette.color(QPalette::Window));
            QPainter painter(&image);
            style->drawControl(QStyle::CE_PushButton, &option, &painter, &button);
            return image;
        };

        const QImage rest = render(QStyle::State_Enabled | QStyle::State_MouseOver);
        const QImage pressed = render(QStyle::State_Enabled | QStyle::State_MouseOver
                                      | QStyle::State_Sunken);
        int changed = 0;
        for (int y = 0; y < rest.height(); ++y)
            for (int x = 0; x < rest.width(); ++x)
                changed += rest.pixelColor(x, y) != pressed.pixelColor(x, y);
        QVERIFY2(changed > 0,
                 role == WinUI3::ControlRole::Standard
                     ? "standard State_Sunken frame is not visible"
                     : "subtle State_Sunken frame is not visible");
    }
}

void WinUI3StyleTest::buttonPressedForegroundRoles()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        style->setThemeMode(mode);
        const WinUI3::Private::Tokens t =
            WinUI3::Private::buildTokens(style->standardPalette());
        QCOMPARE(t.textOnAccentSecondary,
                 mode == WinUI3::ThemeMode::Dark
                     ? QColor(0, 0, 0, 128)
                     : QColor(255, 255, 255, 179));

        for (const WinUI3::ControlRole role : {
                 WinUI3::ControlRole::Standard,
                 WinUI3::ControlRole::Accent,
                 WinUI3::ControlRole::Subtle,
                 WinUI3::ControlRole::Destructive}) {
            QPushButton button(QStringLiteral("Hold"));
            WinUI3::Style::setControlRole(&button, role);
            button.resize(100, 32);
            QStyleOptionButton option;
            option.initFrom(&button);
            option.rect = button.rect();
            option.palette = style->standardPalette();
            option.text = button.text();
            const auto renderLabel = [&](QStyle::State state) {
                option.state = state;
                QImage image(option.rect.size(),
                             QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
                QPainter painter(&image);
                style->drawControl(QStyle::CE_PushButtonLabel, &option,
                                   &painter, &button);
                return image;
            };
            const QImage normal = renderLabel(QStyle::State_Enabled);
            const QImage pressed = renderLabel(QStyle::State_Enabled
                                               | QStyle::State_Sunken);
            QVERIFY2(normal != pressed,
                     qPrintable(QStringLiteral(
                         "pressed foreground unchanged for mode=%1 role=%2")
                                    .arg(int(mode)).arg(int(role))));
        }
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3StyleTest::disabledButtonHasNoInteractionState()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        style->setThemeMode(mode);
        for (const WinUI3::ControlRole role : {WinUI3::ControlRole::Standard,
                                               WinUI3::ControlRole::Subtle}) {
            QPushButton button(QStringLiteral("Disabled"));
            WinUI3::Style::setControlRole(&button, role);
            button.resize(140, 32);
            button.show();
            (void)QTest::qWaitForWindowExposed(&button);
            QTest::mouseMove(&button, button.rect().center());
            QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier,
                              button.rect().center());
            button.setEnabled(false);
            qApp->processEvents();
            QCOMPARE(frameReal(&button, "_winui_hover_progress"), 0.0);
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);

            QEvent enter(QEvent::Enter);
            QCoreApplication::sendEvent(&button, &enter);
            QMouseEvent press(QEvent::MouseButtonPress,
                              QPointF(button.rect().center()),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(&button, &press);
            QCOMPARE(frameReal(&button, "_winui_hover_progress"), 0.0);
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);

            QTest::qWait(220);
            QCOMPARE(frameReal(&button, "_winui_hover_progress"), 0.0);
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);

            auto render = [&](QStyle::State state) {
                QStyleOptionButton option;
                option.initFrom(&button);
                option.rect = button.rect();
                option.state = state;
                option.text = button.text();
                QImage image(button.size(), QImage::Format_ARGB32_Premultiplied);
                image.fill(button.palette().color(QPalette::Window));
                QPainter painter(&image);
                style->drawControl(QStyle::CE_PushButton, &option,
                                   &painter, &button);
                return image;
            };
            QCOMPARE(render(QStyle::State_None),
                     render(QStyle::State_MouseOver | QStyle::State_Sunken));
        }

        QToolButton tool;
        tool.setText(QStringLiteral("Disabled tool"));
        tool.resize(140, 32);
        tool.show();
        (void)QTest::qWaitForWindowExposed(&tool);
        QTest::mouseMove(&tool, tool.rect().center());
        QTest::mousePress(&tool, Qt::LeftButton, Qt::NoModifier,
                          tool.rect().center());
        tool.setEnabled(false);
        qApp->processEvents();
        QCOMPARE(frameReal(&tool, "_winui_hover_progress"), 0.0);
        QCOMPARE(frameReal(&tool, "_winui_press_progress"), 0.0);
        QTest::qWait(220);
        QCOMPARE(frameReal(&tool, "_winui_hover_progress"), 0.0);
        QCOMPARE(frameReal(&tool, "_winui_press_progress"), 0.0);
    }
}

void WinUI3StyleTest::toolButtonIconVerticalCenter()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QPixmap sourcePixmap(16, 16);
    sourcePixmap.fill(QColor(220, 30, 40));
    const QIcon sourceIcon(sourcePixmap);
    for (const qreal dpr : {1.0, 1.25, 1.5, 2.0}) {
        for (const QSize size : {QSize(37, 33), QSize(33, 37)}) {
            QToolBar toolbar;
            toolbar.setOrientation(size.width() > size.height()
                                       ? Qt::Horizontal : Qt::Vertical);
            QToolButton button(&toolbar);
            button.setToolButtonStyle(Qt::ToolButtonIconOnly);
            button.setIcon(sourceIcon);
            button.setIconSize(QSize(16, 16));

            QStyleOptionToolButton option;
            option.initFrom(&button);
            option.rect = QRect(QPoint(), size);
            option.state = QStyle::State_Enabled;
            option.icon = sourceIcon;
            option.iconSize = QSize(16, 16);

            QImage image(qRound(size.width() * dpr),
                         qRound(size.height() * dpr),
                         QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(dpr);
            image.fill(Qt::transparent);
            {
                QPainter painter(&image);
                style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                                   &painter, &button);
            }

            qreal sumX = 0.0;
            qreal sumY = 0.0;
            qreal weight = 0.0;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const QColor pixel = image.pixelColor(x, y);
                    if (pixel.red() < 150 || pixel.green() > 100
                        || pixel.blue() > 100 || pixel.alpha() == 0)
                        continue;
                    const qreal alpha = pixel.alphaF();
                    sumX += (x + 0.5) / dpr * alpha;
                    sumY += (y + 0.5) / dpr * alpha;
                    weight += alpha;
                }
            }
            QVERIFY(weight > 0.0);
            const QRectF content = QRectF(option.rect).adjusted(4.0, 2.0,
                                                                -4.0, -2.0);
            QVERIFY(qAbs(sumX / weight - content.center().x()) <= 0.5);
            QVERIFY(qAbs(sumY / weight - content.center().y()) <= 0.5);
        }
    }
}

void WinUI3StyleTest::styleMutationRestoration()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QCommonStyle replacementStyle;
    QLineEdit pendingHelperUpdate;
    style->polish(&pendingHelperUpdate);
    QVERIFY(frameValue(&pendingHelperUpdate,
                       "_winui_line_edit_helper_update_pending").isValid());
    style->unpolish(&pendingHelperUpdate);
    QVERIFY(!frameValue(&pendingHelperUpdate,
                        "_winui_line_edit_helper_update_pending").isValid());
    pendingHelperUpdate.setStyle(&replacementStyle);
    QCoreApplication::processEvents();
    QVERIFY(!frameValue(&pendingHelperUpdate,
                        "_winui_line_edit_helper_update_pending").isValid());

    QWidget widget;
    const QPalette originalPalette = widget.palette();
    widget.setAttribute(Qt::WA_Hover, false);
    widget.setAutoFillBackground(false);
    widget.setProperty("_winui_control_role", 77);
    style->polish(&widget);
    WinUI3::Style::setControlRole(&widget, WinUI3::ControlRole::Accent);
    QVERIFY(widget.testAttribute(Qt::WA_Hover));
    style->unpolish(&widget);
    QCOMPARE(widget.palette(), originalPalette);
    QVERIFY(!widget.autoFillBackground());
    QVERIFY(!widget.testAttribute(Qt::WA_Hover));
    QCOMPARE(widget.property("_winui_control_role").toInt(), 77);

    QWidget paletteParent;
    QPalette parentPalette = paletteParent.palette();
    parentPalette.setColor(QPalette::ButtonText, QColor(23, 91, 147));
    paletteParent.setPalette(parentPalette);
    QPushButton inheritedPaletteButton(&paletteParent);
    QVERIFY(!inheritedPaletteButton.testAttribute(Qt::WA_SetPalette));
    style->polish(&inheritedPaletteButton);
    style->unpolish(&inheritedPaletteButton);
    QVERIFY(!inheritedPaletteButton.testAttribute(Qt::WA_SetPalette));
    parentPalette.setColor(QPalette::ButtonText, QColor(147, 42, 73));
    paletteParent.setPalette(parentPalette);
    QCOMPARE(inheritedPaletteButton.palette().color(QPalette::ButtonText),
             QColor(147, 42, 73));

    QPushButton explicitPaletteButton;
    QPalette explicitPalette = explicitPaletteButton.palette();
    explicitPalette.setColor(QPalette::ButtonText, QColor(67, 45, 123));
    explicitPaletteButton.setPalette(explicitPalette);
    style->polish(&explicitPaletteButton);
    style->unpolish(&explicitPaletteButton);
    QVERIFY(explicitPaletteButton.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(explicitPaletteButton.palette(), explicitPalette);

    QFrame settingsCard;
    settingsCard.setFrameShape(QFrame::Box);
    WinUI3::Style::setSettingsCard(&settingsCard, true);
    QCOMPARE(settingsCard.frameShape(), QFrame::StyledPanel);
    WinUI3::Style::setSettingsCard(&settingsCard, false);
    QCOMPARE(settingsCard.frameShape(), QFrame::Box);

    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(3, 4, 5, 6);
    layout->setSpacing(7);
    const QMargins originalMargins = layout->contentsMargins();
    const int originalSpacing = layout->spacing();
    const QSize originalMinimum = dialog.minimumSize();
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    dialog.hide();
    style->unpolish(&dialog);
    QCOMPARE(layout->contentsMargins(), originalMargins);
    QCOMPARE(layout->spacing(), originalSpacing);
    QCOMPARE(dialog.minimumSize(), originalMinimum);

    QListView popup(nullptr);
    popup.setWindowFlag(Qt::Popup);
    popup.ensurePolished();
    popup.setSpacing(7);
    popup.viewport()->setAutoFillBackground(true);
    popup.viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
    const QMargins popupMargins = popup.contentsMargins();
    popup.show();
    QTRY_VERIFY(popup.isVisible());
    QCOMPARE(popup.contentsMargins(), popupMargins);
    QCOMPARE(popup.spacing(), 0);
    QVERIFY(popup.viewport()->testAttribute(Qt::WA_OpaquePaintEvent));
    style->unpolish(popup.viewport());
    style->unpolish(&popup);
    QCOMPARE(popup.contentsMargins(), popupMargins);
    QCOMPARE(popup.spacing(), 7);
    QVERIFY(popup.viewport()->autoFillBackground());
    QVERIFY(popup.viewport()->testAttribute(Qt::WA_OpaquePaintEvent));

    QWidget dialogParent;
    QPalette dialogParentPalette = dialogParent.palette();
    dialogParentPalette.setColor(QPalette::WindowText, QColor(18, 72, 129));
    dialogParent.setPalette(dialogParentPalette);
    QDialog liveDialog(&dialogParent);
    auto *liveLayout = new QVBoxLayout(&liveDialog);
    liveLayout->setContentsMargins(2, 3, 4, 5);
    liveLayout->setSpacing(6);
    const QMargins liveMargins = liveLayout->contentsMargins();
    const int liveSpacing = liveLayout->spacing();
    const QSize liveMinimum = liveDialog.minimumSize();
    QVERIFY(!liveDialog.testAttribute(Qt::WA_SetPalette));
    WinUI3::Style::setContentDialog(&liveDialog, true);
    liveDialog.show();
    QTRY_VERIFY(liveDialog.isVisible());
    QVERIFY(liveLayout->contentsMargins() != liveMargins);
    WinUI3::Style::setContentDialog(&liveDialog, false);
    QCOMPARE(liveLayout->contentsMargins(), liveMargins);
    QCOMPARE(liveLayout->spacing(), liveSpacing);
    QCOMPARE(liveDialog.minimumSize(), liveMinimum);
    QVERIFY(!liveDialog.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(liveDialog.palette().color(QPalette::WindowText),
             qApp->palette().color(QPalette::WindowText));

    QListView navigation;
    navigation.viewport()->setMouseTracking(false);
    WinUI3::Style::setNavigationView(&navigation);
    QAbstractItemDelegate *originalDelegate = navigation.itemDelegate();
    style->polish(&navigation);
    QVERIFY(navigation.viewport()->hasMouseTracking());
    QVERIFY(navigation.itemDelegate() != originalDelegate);
    style->unpolish(&navigation);
    QVERIFY(!navigation.viewport()->hasMouseTracking());
    QCOMPARE(navigation.itemDelegate(), originalDelegate);
}

void WinUI3StyleTest::accessibilityOwnershipContracts()
{
    QLineEdit editor;
    editor.setAccessibleName(QStringLiteral("Project name"));
    editor.setAccessibleDescription(QStringLiteral("Name of the current project"));
    QCOMPARE(editor.accessibleName(), QStringLiteral("Project name"));
    QCOMPARE(editor.accessibleDescription(),
             QStringLiteral("Name of the current project"));
    if (QAccessibleInterface *accessible = QAccessible::queryAccessibleInterface(&editor))
        QCOMPARE(accessible->role(), QAccessible::EditableText);

    QPointer<QLabel> child;
    {
        QWidget parent;
        child = new QLabel(QStringLiteral("Owned"), &parent);
        QCOMPARE(child->parentWidget(), &parent);
    }
    QVERIFY(child.isNull());
}

void WinUI3StyleTest::baseStyleContract()
{
    WinUI3::Style style(WinUI3::ThemeMode::Light);
    QVERIFY(style.baseStyle());
    QCOMPARE(QString::fromLatin1(style.baseStyle()->metaObject()->className()),
             QStringLiteral("QCommonStyle"));
    QCOMPARE(style.styleHint(QStyle::SH_Menu_MouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_MenuBar_MouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_ListMouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_Popup), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_PopupFrameStyle),
             int(QFrame::NoFrame));
    QCOMPARE(style.styleHint(QStyle::SH_Slider_AbsoluteSetButtons),
             int(Qt::LeftButton));
    QCOMPARE(style.styleHint(QStyle::SH_ToolButtonStyle),
             int(Qt::ToolButtonFollowStyle));

    QStyleOptionMenuItem item;
    item.font = qApp->font();
    item.fontMetrics = QFontMetrics(item.font);
    const QSize contents(item.fontMetrics.horizontalAdvance(QStringLiteral("File")),
                         item.fontMetrics.height());
    const QSize result = style.sizeFromContents(QStyle::CT_MenuBarItem, &item,
                                                contents);
    QVERIFY(result.width() >= contents.width() + 24);
    QVERIFY(result.height() >= 32);

    QStyleOption panel;
    panel.rect = QRect(0, 0, 80, 32);
    panel.palette = style.standardPalette();
    panel.state = QStyle::State_Enabled;
    QImage menuBar(panel.rect.size(), QImage::Format_ARGB32_Premultiplied);
    menuBar.fill(Qt::transparent);
    {
        QPainter painter(&menuBar);
        style.drawPrimitive(QStyle::PE_PanelMenuBar, &panel, &painter);
    }
    const QColor surface = panel.palette.color(QPalette::Window);
    QCOMPARE(menuBar.pixelColor(panel.rect.center()), surface);
    QCOMPARE(menuBar.pixelColor(panel.rect.left(), panel.rect.bottom()), surface);
    QCOMPARE(menuBar.pixelColor(panel.rect.center().x(), panel.rect.bottom()), surface);
}

void WinUI3StyleTest::controlRoles()
{
    QPushButton button;
    QCOMPARE(WinUI3::Style::controlRole(&button), WinUI3::ControlRole::Standard);
    WinUI3::Style::setControlRole(&button, WinUI3::ControlRole::Accent);
    QCOMPARE(WinUI3::Style::controlRole(&button), WinUI3::ControlRole::Accent);
}

void WinUI3StyleTest::toggleConvenienceWidget()
{
    WinUI3::ToggleSwitch toggle(QStringLiteral("Notifications"));
    QVERIFY(WinUI3::Style::isToggleSwitch(&toggle));
    QCOMPARE(toggle.text(), QStringLiteral("Notifications"));
    QCOMPARE(toggle.onText(), QString());
    QCOMPARE(toggle.offText(), QString());

    toggle.setOnText(QStringLiteral("On"));
    QVERIFY(!toggle.property(WinUI3::Style::ToggleSwitchOffTextProperty).isValid());

    WinUI3::ToggleSwitch offOnly(QStringLiteral("Notifications"));
    offOnly.setOffText(QStringLiteral("Off"));
    QVERIFY(!offOnly.property(WinUI3::Style::ToggleSwitchOnTextProperty).isValid());

    toggle.setOffText(QStringLiteral("Off"));
    QCOMPARE(toggle.onText(), QStringLiteral("On"));
    QCOMPARE(toggle.offText(), QStringLiteral("Off"));
    QCOMPARE(toggle.property(WinUI3::Style::ToggleSwitchOnTextProperty).toString(),
             QStringLiteral("On"));
    QCOMPARE(toggle.property(WinUI3::Style::ToggleSwitchOffTextProperty).toString(),
             QStringLiteral("Off"));

    QSignalSpy toggled(&toggle, &QAbstractButton::toggled);
    toggle.setChecked(true);
    QCOMPARE(toggled.count(), 1);
    QVERIFY(toggle.isChecked());

    toggle.resize(toggle.sizeHint());
    toggle.show();
    toggle.setChecked(false);
    QTRY_VERIFY(frameReal(&toggle, "_winui_toggle_position") < 0.01);
    toggle.setChecked(true);
    QTest::qWait(35);
    const qreal beforeTextChange =
        frameReal(&toggle, "_winui_toggle_position");
    QVERIFY(beforeTextChange > 0.0 && beforeTextChange < 0.99);
    toggle.setOnText(QStringLiteral("Enabled"));
    const qreal afterTextChange =
        frameReal(&toggle, "_winui_toggle_position");
    QVERIFY(afterTextChange < 0.99);
    QVERIFY(std::abs(afterTextChange - beforeTextChange) < 0.15);
    QTRY_VERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.99);
}

void WinUI3StyleTest::toggleInteraction()
{
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    WinUI3::Style::setToggleSwitchText(&toggle, QStringLiteral("On"),
                                       QStringLiteral("Off"));
    toggle.resize(toggle.sizeHint());
    toggle.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toggle));
    QSignalSpy spy(&toggle, &QAbstractButton::clicked);
    QTest::mouseMove(&toggle, QPoint(20, 20));
    QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QVERIFY(toggle.isChecked());
    QCOMPARE(spy.count(), 1);
    QTest::qWait(35);
    const qreal midway = frameReal(&toggle, "_winui_toggle_position");
    QVERIFY2(midway > 0.0 && midway < 1.0, qPrintable(QString::number(midway)));
    QTRY_VERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.99);
    toggle.setChecked(false);
    QTest::qWait(35);
    const qreal reverse = frameReal(&toggle, "_winui_toggle_position");
    QVERIFY(reverse > 0.0 && reverse < 1.0);
    toggle.setChecked(true);
    QTRY_VERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.99);
    QFocusEvent keyboardFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&toggle, &keyboardFocus);
    QVERIFY(frameBool(&toggle, "_winui_focus_visible"));
    toggle.setEnabled(false);
    QVERIFY(!toggle.grab().isNull());
}

void WinUI3StyleTest::togglePressedThumbGeometry()
{
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    toggle.resize(80, 32);
    toggle.setChecked(true);
    setFrame(&toggle, "_winui_toggle_position", 1.0);
    setFrame(&toggle, "_winui_hover_progress", 1.0);
    setFrame(&toggle, "_winui_press_progress", 1.0);

    QStyleOptionButton option;
    option.initFrom(&toggle);
    option.rect = toggle.rect();
    option.state = QStyle::State_Enabled | QStyle::State_On
        | QStyle::State_MouseOver | QStyle::State_Sunken;
    QImage image(toggle.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        toggle.style()->drawControl(QStyle::CE_CheckBox, &option,
                                    &painter, &toggle);
    }

    const QRectF track(option.rect.left(), option.rect.center().y() - 10,
                       40, 20);
    QRect whiteInk;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() > 200 && pixel.red() > 240
                && pixel.green() > 240 && pixel.blue() > 240)
                whiteInk |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(!whiteInk.isEmpty());
    QVERIFY(whiteInk.width() <= 17);
    QVERIFY(whiteInk.height() <= 14);
    QVERIFY2(track.right() - whiteInk.right() >= 3.0,
             "pressed on-thumb must retain WinUI's trailing inset");
}

void WinUI3StyleTest::toggleDragInteraction()
{
    QCheckBox toggle;
    toggle.setProperty(WinUI3::Style::ToggleSwitchProperty, true);
    toggle.setProperty(WinUI3::Style::ToggleSwitchOnTextProperty,
                       QStringLiteral("On"));
    toggle.setProperty(WinUI3::Style::ToggleSwitchOffTextProperty,
                       QStringLiteral("Off"));
    toggle.resize(toggle.sizeHint());
    toggle.show();
    QVERIFY(WinUI3::Style::isToggleSwitch(&toggle));
    QCOMPARE(toggle.style()->pixelMetric(QStyle::PM_IndicatorWidth,
                                         nullptr, &toggle), 40);

    QSignalSpy clicked(&toggle, &QAbstractButton::clicked);
    QTest::mousePress(&toggle, Qt::LeftButton, Qt::NoModifier, QPoint(10, 20));
    QMouseEvent move(QEvent::MouseMove, QPointF(32, 20), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &move);
    QVERIFY(frameBool(&toggle, "_winui_toggle_dragging"));
    QVERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.9);

    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(32, 20),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &release);
    QVERIFY(toggle.isChecked());
    QVERIFY(!frameBool(&toggle, "_winui_toggle_dragging"));
    QCOMPARE(clicked.count(), 1);
}

void WinUI3StyleTest::toggleRtlGeometryAndInteraction()
{
    DisableAnimationsGuard animations;
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    WinUI3::Style::setToggleSwitchText(&toggle, QStringLiteral("On"),
                                       QStringLiteral("Off"));
    toggle.setLayoutDirection(Qt::RightToLeft);
    toggle.resize(140, 32);
    toggle.setChecked(true);
    toggle.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toggle));

    // QRect::right() is inclusive. The RTL track must therefore start at
    // right - 39, exactly mirroring the LTR 40-pixel slot. The old right - 40
    // origin left one stale pixel outside the widget and disagreed with the
    // drag hit region.
    const QRect expectedTrack(toggle.rect().right() - 39,
                              toggle.rect().center().y() - 10, 40, 20);
    QStyleOptionButton option;
    option.initFrom(&toggle);
    option.rect = toggle.rect();
    option.direction = Qt::RightToLeft;
    option.text = toggle.text();
    option.state |= QStyle::State_On;
    QImage image(toggle.size(), QImage::Format_ARGB32_Premultiplied);
    const QColor background = toggle.palette().color(QPalette::Window);
    image.fill(background);
    {
        QPainter painter(&image);
        toggle.style()->drawControl(QStyle::CE_CheckBox, &option,
                                    &painter, &toggle);
    }
    const QColor accent = toggle.palette().color(QPalette::Accent);
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.center()), accent)
            < 100);
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.left() - 1,
                                           expectedTrack.center().y()),
                          background)
            < 2);
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.right(),
                                           expectedTrack.center().y()),
                          accent)
            < 100);

    // A click anywhere in the visual track remains a normal checkbox click.
    toggle.setChecked(false);
    QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier,
                      expectedTrack.center());
    QVERIFY(toggle.isChecked());

    // In RTL, the unchecked knob is on the right and a drag toward the left
    // must turn the switch on. This exercises the same 40 x 20 rect used by
    // the renderer, including its inclusive right edge.
    toggle.setChecked(false);
    QCoreApplication::processEvents();
    const QPoint offKnob(expectedTrack.right() - 10,
                         expectedTrack.center().y());
    const QPoint onKnob(expectedTrack.left() + 10,
                        expectedTrack.center().y());
    QTest::mousePress(&toggle, Qt::LeftButton, Qt::NoModifier, offKnob);
    QMouseEvent move(QEvent::MouseMove, QPointF(onKnob), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &move);
    QVERIFY(frameBool(&toggle, "_winui_toggle_dragging"));
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(onKnob),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &release);
    QVERIFY(toggle.isChecked());
    QVERIFY(!frameBool(&toggle, "_winui_toggle_dragging"));
}

void WinUI3StyleTest::backdropLifecycleContract()
{
#ifndef Q_OS_WIN
    QWidget window;
    QVERIFY(!WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica));
    return;
#else
    // DWM is deliberately not part of this unit test. The offscreen Windows
    // platform still exercises the palette/attribute/property lifecycle
    // deterministically; native composition is covered by the native test
    // target and by the gallery.
    if (QGuiApplication::platformName() != QStringLiteral("offscreen"))
        QSKIP("DWM-backed lifecycle belongs to the native test target");

    QWidget window;
    window.setAttribute(Qt::WA_TranslucentBackground, false);
    window.setAttribute(Qt::WA_NoSystemBackground, false);
    window.setAttribute(Qt::WA_OpaquePaintEvent, true);
    window.setAutoFillBackground(true);
    QPalette custom = window.palette();
    custom.setColor(QPalette::Window, QColor(19, 37, 53));
    window.setPalette(custom);

    const QPalette originalPalette = window.palette();
    const bool originalTranslucent =
        window.testAttribute(Qt::WA_TranslucentBackground);
    const bool originalNoSystemBackground =
        window.testAttribute(Qt::WA_NoSystemBackground);
    const bool originalOpaquePaint = window.testAttribute(Qt::WA_OpaquePaintEvent);
    const bool originalAutoFill = window.autoFillBackground();

    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica));
    QCOMPARE(window.property("_winui_backdrop").toInt(),
             int(WinUI3::Backdrop::Mica));
    QVERIFY(window.testAttribute(Qt::WA_NoSystemBackground));
    QVERIFY(!window.testAttribute(Qt::WA_OpaquePaintEvent));
    QVERIFY(!window.autoFillBackground());
    QCOMPARE(window.palette().color(QPalette::Window).alpha(), 0);

    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::MicaAlt));
    QCOMPARE(window.property("_winui_backdrop").toInt(),
             int(WinUI3::Backdrop::MicaAlt));
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.testAttribute(Qt::WA_TranslucentBackground),
             originalTranslucent);
    QCOMPARE(window.testAttribute(Qt::WA_NoSystemBackground),
             originalNoSystemBackground);
    QCOMPARE(window.testAttribute(Qt::WA_OpaquePaintEvent), originalOpaquePaint);
    QCOMPARE(window.autoFillBackground(), originalAutoFill);
    QCOMPARE(window.palette(), originalPalette);

    // Clearing an already-cleared backdrop must remain a no-op. In
    // particular it must not create a native handle just to reset DWM state.
    QVERIFY(WinUI3::applyBackdrop(&window, WinUI3::Backdrop::None));
    QVERIFY(!window.property("_winui_backdrop").isValid());
    QCOMPARE(window.palette(), originalPalette);
#endif
}

void WinUI3StyleTest::settingsCardExpansion()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced"));
    card.setDescription(QStringLiteral("Description"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(card.property(WinUI3::Style::SettingsCardProperty).toBool());
    QSignalSpy spy(&card, &WinUI3::SettingsCard::expandedChanged);
    QTest::mouseMove(&card, QPoint(20, 20));
    QTest::mousePress(&card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QTest::qWait(35);
    const qreal pressed = frameReal(&card, "_winui_press_progress");
    QVERIFY2(pressed > 0.0 && pressed < 1.0,
             qPrintable(QString::number(pressed)));
    QTest::mouseRelease(&card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QVERIFY(card.isExpanded());
    QCOMPARE(spy.count(), 1);
    QTest::qWait(280);
    card.setExpanded(false);
    QTest::qWait(90);
    const qreal reverse = card.property("expansionProgress").toReal();
    QVERIFY(reverse > 0.0 && reverse < 1.0);
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.01);
}

void WinUI3StyleTest::settingsCardDesignerPropertyBindings()
{
    // This is the order generated by uic for a promoted custom widget:
    // properties are applied before nested children are constructed. The
    // string bridges must therefore resolve the controls after setupUi().
    QWidget host;
    auto *card = new WinUI3::SettingsCard(&host);
    card->setObjectName(QStringLiteral("advancedCard"));
    card->setIconName(QStringLiteral("Settings"));
    card->setTrailingWidgetName(QStringLiteral("toggle"));
    card->setExpandableWidgetName(QStringLiteral("details"));
    card->setExpanded(false);
    auto *toggle = new QCheckBox(card);
    toggle->setObjectName(QStringLiteral("toggle"));
    auto *details = new QLabel(QStringLiteral("Details"), card);
    details->setObjectName(QStringLiteral("details"));
    host.resize(560, 260);
    host.show();
    QTRY_COMPARE(card->trailingWidget(), static_cast<QWidget *>(toggle));
    QTRY_COMPARE(card->expandableWidget(), static_cast<QWidget *>(details));
    QVERIFY(!card->icon().isNull());

    auto *header = card->findChild<QWidget *>(
        QStringLiteral("_winui_settings_card_headerHost"));
    QVERIFY(header);
    QTest::mouseClick(header, Qt::LeftButton, Qt::NoModifier,
                      header->rect().center());
    QTRY_VERIFY(card->isExpanded());
    QVERIFY(details->isVisible());
}

void WinUI3StyleTest::settingsCardTrailingWidgetsReceiveClicks()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);

    auto *toggleCard = new WinUI3::SettingsCard;
    toggleCard->setTitle(QStringLiteral("Notifications"));
    auto *toggle = new QCheckBox;
    WinUI3::Style::setToggleSwitch(toggle);
    toggle->setChecked(true);
    toggleCard->setTrailingWidget(toggle);
    layout->addWidget(toggleCard);

    auto *comboCard = new WinUI3::SettingsCard;
    comboCard->setTitle(QStringLiteral("Updates"));
    auto *combo = new QComboBox;
    combo->addItems({QStringLiteral("Automatic"),
                     QStringLiteral("Manual")});
    comboCard->setTrailingWidget(combo);
    layout->addWidget(comboCard);

    auto *expandableCard = new WinUI3::SettingsCard;
    expandableCard->setTitle(QStringLiteral("Advanced"));
    expandableCard->setExpandableWidget(new QLabel(QStringLiteral("Details")));
    layout->addWidget(expandableCard);

    host.resize(560, host.sizeHint().height());
    host.show();
    QCoreApplication::processEvents();

    const auto targetAt = [](QWidget *widget) {
        return QApplication::widgetAt(
            widget->mapToGlobal(widget->rect().center()));
    };

    QWidget *toggleTarget = targetAt(toggle);
    QVERIFY2(toggleTarget == toggle || toggle->isAncestorOf(toggleTarget),
             "SettingsCard swallowed the trailing toggle hit area");
    const QPoint togglePoint = toggleTarget->mapFromGlobal(
        toggle->mapToGlobal(toggle->rect().center()));
    QTest::mouseClick(toggleTarget, Qt::LeftButton, Qt::NoModifier,
                      togglePoint);
    QCOMPARE(toggle->isChecked(), false);

    QWidget *comboTarget = targetAt(combo);
    QVERIFY2(comboTarget == combo || combo->isAncestorOf(comboTarget),
             "SettingsCard swallowed the trailing combo-box hit area");
    const QPoint comboPoint = comboTarget->mapFromGlobal(
        combo->mapToGlobal(combo->rect().center()));
    QTest::mouseClick(comboTarget, Qt::LeftButton, Qt::NoModifier,
                      comboPoint);
    QTRY_VERIFY(combo->view()->isVisible());
    combo->hidePopup();

    auto *title = expandableCard->findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_title"));
    QVERIFY(title);
    QWidget *headerTarget = targetAt(title);
    QVERIFY(headerTarget);
    QTest::mouseClick(headerTarget, Qt::LeftButton, Qt::NoModifier,
                      headerTarget->mapFromGlobal(
                          title->mapToGlobal(title->rect().center())));
    QCOMPARE(expandableCard->isExpanded(), true);
}

void WinUI3StyleTest::settingsCardTrailingWidgetsHaveUniformHeight()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toggleCard = new WinUI3::SettingsCard;
    toggleCard->setTitle(QStringLiteral("Notifications"));
    toggleCard->setDescription(QStringLiteral("Show alerts and status messages"));
    auto *toggle = new QCheckBox;
    WinUI3::Style::setToggleSwitch(toggle);
    toggleCard->setTrailingWidget(toggle);
    layout->addWidget(toggleCard);

    auto *comboCard = new WinUI3::SettingsCard;
    comboCard->setTitle(QStringLiteral("Updates"));
    comboCard->setDescription(QStringLiteral("Choose how updates are installed"));
    auto *combo = new QComboBox;
    combo->addItems({QStringLiteral("Automatic"), QStringLiteral("Notify me"),
                     QStringLiteral("Manual")});
    comboCard->setTrailingWidget(combo);
    layout->addWidget(comboCard);

    host.resize(560, host.sizeHint().height());
    host.show();
    QCoreApplication::processEvents();

    QCOMPARE(toggleCard->height(), comboCard->height());
    QCOMPARE(toggleCard->sizeHint().height(), comboCard->sizeHint().height());
    QCOMPARE(toggleCard->minimumSizeHint().height(), comboCard->minimumSizeHint().height());
}

void WinUI3StyleTest::settingsCardChevronAndStableHeader()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced settings"));
    card.setDescription(QStringLiteral("A description that remains in the same header while content expands."));
    auto *trailing = new QLabel(QStringLiteral("On"));
    card.setTrailingWidget(trailing);
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(460, card.sizeHint().height());
    card.show();
    QTRY_VERIFY(card.isVisible());

    auto *headerHost = card.findChild<QWidget *>(
        QStringLiteral("_winui_settings_card_headerHost"));
    auto *title = card.findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_title"));
    auto *chevron = card.findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_chevron"));
    QVERIFY(headerHost);
    QVERIFY(title);
    QVERIFY(chevron);
    QVERIFY(chevron->isVisible());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronRight));
    const QRect chevronInCard(chevron->mapTo(&card, chevron->rect().topLeft()),
                              chevron->size());
    const QRect trailingInCard(trailing->mapTo(&card, trailing->rect().topLeft()),
                               trailing->size());
    QVERIFY(!chevronInCard.intersects(trailingInCard));

    const QRect titleGeometry = title->geometry();
    card.setExpanded(true);
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.0);
    QCOMPARE(title->geometry(), titleGeometry);

    card.setLayoutDirection(Qt::RightToLeft);
    QTRY_COMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
                 static_cast<int>(WinUI3::Icon::ChevronDown));
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.99);
    card.setExpanded(false);
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.99);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronLeft));
}

void WinUI3StyleTest::settingsCardExpansionLoad()
{
    const QList<int> cardCounts = {1, 10, 50};
    int previousLayouts = 0;
    int previousResizes = 0;
    for (const int count : cardCounts) {
        QWidget host;
        host.resize(640, qMax(240, count * 56));
        auto *layout = new QVBoxLayout(&host);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);
        QVector<WinUI3::SettingsCard *> cards;
        QVector<CountingHintWidget *> contents;
        QVector<LayoutLifecycleProbe *> probes;
        cards.reserve(count);
        contents.reserve(count);
        probes.reserve(count);

        for (int i = 0; i < count; ++i) {
            auto *card = new WinUI3::SettingsCard;
            card->setTitle(QStringLiteral("Card %1").arg(i));
            card->setDescription(QStringLiteral(
                "A long description which exercises the stable header width "
                "while the expandable content is resized."));
            auto *content = new CountingHintWidget(QSize(320, 42 + (i % 4) * 7));
            auto *probe = new LayoutLifecycleProbe;
            card->installEventFilter(probe);
            content->installEventFilter(probe);
            card->setExpandableWidget(content);
            layout->addWidget(card);
            cards.append(card);
            contents.append(content);
            probes.append(probe);
        }
        host.show();
        QCoreApplication::processEvents();

        for (WinUI3::SettingsCard *card : cards)
            card->setExpanded(true);
        QCoreApplication::processEvents();
        for (WinUI3::SettingsCard *card : cards) {
            auto *animation = card->findChild<QVariantAnimation *>(
                QStringLiteral("_winui_settings_card_expansion_animation"));
            QVERIFY(animation);
            animation->setCurrentTime(animation->duration());
        }
        QCoreApplication::processEvents();

        const int sizeHintsBeforeContentChange = contents.first()->sizeHintCalls;
        contents.first()->m_hint = QSize(320, 96);
        contents.first()->updateGeometry();
        QCoreApplication::processEvents();
        QVERIFY(contents.first()->sizeHintCalls > sizeHintsBeforeContentChange);
        auto *expandedHost = cards.first()->findChild<QWidget *>(
            QStringLiteral("_winui_settings_card_expandableHost"));
        QVERIFY(expandedHost);
        QCOMPARE(expandedHost->maximumHeight(), 112);

        for (int i = 0; i < count; ++i) {
            QVERIFY(cards.at(i)->isExpanded());
            QVERIFY(cards.at(i)->property("expansionProgress").toReal() > 0.99);
            QVERIFY(contents.at(i)->sizeHintCalls <= 4);
            QVERIFY(probes.at(i)->layoutRequests <= 8);
            QVERIFY(probes.at(i)->resizes <= 8);
            QVERIFY(probes.at(i)->paints >= 0);
        }

        // Reversing and replacing content must not retain the old height or
        // animation state. A direct animation clock advance keeps this a
        // counter/invariant test rather than a wall-clock test.
        cards.first()->setExpanded(false);
        cards.first()->setExpanded(true);
        auto *reversal = cards.first()->findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
        QVERIFY(reversal);
        reversal->setCurrentTime(reversal->duration());
        QVERIFY(cards.first()->isExpanded());
        const int beforeReplacement = contents.first()->sizeHintCalls;
        auto *replacement = new CountingHintWidget(QSize(320, 150));
        cards.first()->setExpandableWidget(replacement);
        QCOMPARE(cards.first()->isExpanded(), false);
        QCOMPARE(cards.first()->property("expansionProgress").toReal(), 0.0);
        auto *expandableHost = cards.first()->findChild<QWidget *>(
            QStringLiteral("_winui_settings_card_expandableHost"));
        QVERIFY(expandableHost);
        QCOMPARE(expandableHost->maximumHeight(), 0);
        QVERIFY(!expandableHost->isVisible());
        cards.first()->setExpanded(true);
        auto *replacementAnimation = cards.first()->findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
        QVERIFY(replacementAnimation);
        replacementAnimation->setCurrentTime(replacementAnimation->duration());
        QVERIFY(replacement->sizeHintCalls > 0);
        QVERIFY(beforeReplacement >= 0);

        int layouts = 0;
        int resizes = 0;
        for (LayoutLifecycleProbe *probe : probes) {
            layouts += probe->layoutRequests;
            resizes += probe->resizes;
        }
        if (count > 1) {
            QVERIFY2(layouts <= previousLayouts * 6 + count * 4,
                     qPrintable(QStringLiteral("layout requests grew superlinearly: %1 -> %2")
                                    .arg(previousLayouts).arg(layouts)));
            QVERIFY2(resizes <= previousResizes * 6 + count * 4,
                     qPrintable(QStringLiteral("resizes grew superlinearly: %1 -> %2")
                                    .arg(previousResizes).arg(resizes)));
        }
        previousLayouts = layouts;
        previousResizes = resizes;

        host.hide();
        qDeleteAll(probes);
    }
}

void WinUI3StyleTest::settingsCardInteractiveFrames()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Display").toUpper());
    card.setDescription(QStringLiteral(
        "The header must keep its geometry while the details are revealed."));
    card.setExpandableWidget(new SolidPage(QColor(40, 120, 200),
                                           QStringLiteral("Details")));
    card.resize(480, 240);
    card.show();
    QCoreApplication::processEvents();

    auto *header = card.findChild<QWidget *>(
        QStringLiteral("_winui_settings_card_headerHost"));
    auto *title = card.findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_title"));
    auto *description = card.findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_description"));
    auto *chevron = card.findChild<QLabel *>(
        QStringLiteral("_winui_settings_card_chevron"));
    auto *animation = card.findChild<QVariantAnimation *>(
        QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(header);
    QVERIFY(title);
    QVERIFY(description);
    QVERIFY(chevron);
    QVERIFY(animation);
    QVERIFY(chevron->isVisible());
    QVERIFY(!chevron->pixmap(Qt::ReturnByValue).isNull());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronRight));

    const QRect headerGeometry = header->geometry();
    const QRect titleGeometry = title->geometry();
    const QRect descriptionGeometry = description->geometry();
    const QImage collapsed = card.grab().toImage();
    const QPixmap collapsedChevron = chevron->pixmap(Qt::ReturnByValue);

    QTest::mouseClick(&card, Qt::LeftButton, Qt::NoModifier,
                      header->geometry().center());
    QCOMPARE(card.isExpanded(), true);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    const QImage first = card.grab().toImage();
    QTRY_VERIFY(chevron->pixmap(Qt::ReturnByValue).toImage() !=
                collapsedChevron.toImage());
    QCOMPARE(header->geometry(), headerGeometry);
    QCOMPARE(title->geometry(), titleGeometry);
    QCOMPARE(description->geometry(), descriptionGeometry);

    animation->setCurrentTime(animation->duration() / 2);
    QCoreApplication::processEvents();
    const QImage midpoint = card.grab().toImage();
    QVERIFY(midpoint != collapsed);
    QVERIFY(midpoint != first);
    QCOMPARE(header->geometry(), headerGeometry);
    QCOMPARE(title->geometry(), titleGeometry);
    QCOMPARE(description->geometry(), descriptionGeometry);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));

    QPalette palette = card.palette();
    palette.setColor(QPalette::WindowText, QColor(210, 40, 70));
    card.setPalette(palette);
    QCoreApplication::processEvents();
    QVERIFY(!chevron->pixmap(Qt::ReturnByValue).isNull());
    QEvent dprChange(QEvent::DevicePixelRatioChange);
    QCoreApplication::sendEvent(&card, &dprChange);
    QVERIFY(!chevron->pixmap(Qt::ReturnByValue).isNull());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));

    // Reversing and expanding again must reuse the current progress without
    // losing the header or leaving the chevron in the collapsed state.
    card.setExpanded(false);
    card.setExpanded(true);
    QCOMPARE(card.isExpanded(), true);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    card.hide();
    QVERIFY(animation->state() == QAbstractAnimation::Stopped);
    card.show();
    QCoreApplication::processEvents();
    QVERIFY(card.isExpanded());
    QVERIFY(animation->state() == QAbstractAnimation::Stopped);
    QCOMPARE(card.property("expansionProgress").toReal(), 1.0);
    QCOMPARE(header->geometry(), headerGeometry);
    QCOMPARE(title->geometry(), titleGeometry);
    QCOMPARE(description->geometry(), descriptionGeometry);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
}

void WinUI3StyleTest::settingsCardExpansionInScrollingPage()
{
    // Keep this composition identical to GalleryWindow::scrollingPage(): the
    // expansion must not let the parent layout redistribute the already
    // visible cards. Only Advanced's content host may grow downward.
    auto *area = new QScrollArea;
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    auto *body = new QWidget;
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(28, 20, 28, 28);
    bodyLayout->setSpacing(16);
    auto *heading = new QLabel(QStringLiteral("Settings"));
    QFont headingFont = heading->font();
    headingFont.setPixelSize(28);
    headingFont.setWeight(QFont::DemiBold);
    heading->setFont(headingFont);
    bodyLayout->addWidget(heading);

    auto *cardsLayout = new QVBoxLayout;
    auto *notifications = new WinUI3::SettingsCard;
    notifications->setTitle(QStringLiteral("Notifications"));
    notifications->setDescription(QStringLiteral("Show alerts and status messages"));
    auto *toggle = new QCheckBox;
    WinUI3::Style::setToggleSwitch(toggle);
    notifications->setTrailingWidget(toggle);
    cardsLayout->addWidget(notifications);

    auto *updates = new WinUI3::SettingsCard;
    updates->setTitle(QStringLiteral("Updates"));
    updates->setDescription(QStringLiteral("Choose how updates are installed"));
    auto *combo = new QComboBox;
    combo->addItems({QStringLiteral("Automatic"), QStringLiteral("Notify me"),
                     QStringLiteral("Manual")});
    updates->setTrailingWidget(combo);
    cardsLayout->addWidget(updates);

    auto *advanced = new WinUI3::SettingsCard;
    advanced->setTitle(QStringLiteral("Advanced options"));
    advanced->setDescription(QStringLiteral("Developer and diagnostic settings"));
    auto *details = new QTextEdit;
    details->setPlainText(QStringLiteral("Expanded settings content."));
    details->setMaximumHeight(100);
    advanced->setExpandableWidget(details);
    cardsLayout->addWidget(advanced);
    bodyLayout->addLayout(cardsLayout);
    bodyLayout->addStretch();
    area->setWidget(body);
    // Initial content fits the viewport, while the expanded details force a
    // vertical scrollbar; this is the transition that used to redistribute
    // the card stack by a few pixels in the real gallery.
    area->resize(900, 420);
    area->show();
    QCoreApplication::processEvents();
    QVERIFY(!area->verticalScrollBar()->isVisible());

    const QList<WinUI3::SettingsCard *> cards{notifications, updates, advanced};
    QVector<QRect> cardGeometries;
    QVector<QRect> headerGeometries;
    for (WinUI3::SettingsCard *card : cards) {
        auto *header = card->findChild<QWidget *>(
            QStringLiteral("_winui_settings_card_headerHost"));
        QVERIFY(header);
        QCOMPARE(card->height(), card->sizeHint().height());
        cardGeometries.append(card->geometry());
        headerGeometries.append(header->geometry());
    }
    const int advancedBottom = advanced->geometry().bottom();
    const int advancedHeight = advanced->height();
    advanced->setExpanded(true);
    auto *animation = advanced->findChild<QVariantAnimation *>(
        QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(animation);
    animation->setCurrentTime(animation->duration() / 2);
    QCoreApplication::processEvents();
    QVERIFY(advanced->property("expansionProgress").toReal() > 0.0);
    QVERIFY(advanced->property("expansionProgress").toReal() < 1.0);
    QVERIFY(area->verticalScrollBar()->isVisible());
    for (int i = 0; i < cards.size(); ++i) {
        auto *header = cards.at(i)->findChild<QWidget *>(
            QStringLiteral("_winui_settings_card_headerHost"));
        QVERIFY(header);
        if (i < 2) {
            QCOMPARE(cards.at(i)->geometry().top(), cardGeometries.at(i).top());
            QCOMPARE(header->geometry().top(), headerGeometries.at(i).top());
        } else {
            QCOMPARE(cards.at(i)->geometry().top(), cardGeometries.at(i).top());
        }
    }
    QVERIFY(advanced->geometry().bottom() >= advancedBottom);
    animation->setCurrentTime(animation->duration());
    QCoreApplication::processEvents();
    QCOMPARE(advanced->geometry().top(), cardGeometries.at(2).top());
    QVERIFY(advanced->height() > advancedHeight);
    for (int i = 0; i < 2; ++i) {
        auto *header = cards.at(i)->findChild<QWidget *>(
            QStringLiteral("_winui_settings_card_headerHost"));
        QVERIFY(header);
        QCOMPARE(cards.at(i)->geometry().top(), cardGeometries.at(i).top());
        QCOMPARE(header->geometry().top(), headerGeometries.at(i).top());
    }
    area->hide();
    delete area;
}

void WinUI3StyleTest::navigationTransition()
{
    WinUI3::NavigationView view;
    view.resize(720, 480);
    view.addPage(new QLabel(QStringLiteral("One")), QIcon(), QStringLiteral("One"));
    view.addPage(new QLabel(QStringLiteral("Two")), QIcon(), QStringLiteral("Two"));
    view.stack()->setDuration(20);
    view.show();
    QVERIFY(view.navigationList()->property(
        WinUI3::Style::NavigationViewProperty).toBool());
    QVERIFY(view.navigationList()->property(
        "_winui_navigation_delegate").value<QObject *>()
            == view.navigationList()->itemDelegate());
    const QRect second = view.navigationList()->visualItemRect(
        view.navigationList()->item(1));
    QTest::mouseMove(view.navigationList()->viewport(), second.center());
    QTest::mouseClick(view.navigationList()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, second.center());
    QTRY_COMPARE(view.currentIndex(), 1);
    QTest::qWait(90);
    const qreal indicator = frameReal(view.navigationList()->viewport(),
                                      "_winui_navigation_indicator_y");
    QVERIFY(indicator > 0.0 && indicator < second.top());
    QTRY_VERIFY(qAbs(frameReal(view.navigationList()->viewport(),
                              "_winui_navigation_indicator_y")
                     - second.top()) < 0.5);
}

void WinUI3StyleTest::navigationInteractiveFrames()
{
    WinUI3::NavigationView view;
    view.resize(720, 420);
    auto *one = new SolidPage(QColor(180, 55, 55), QStringLiteral("One"));
    auto *two = new SolidPage(QColor(55, 165, 85), QStringLiteral("Two"));
    auto *three = new SolidPage(QColor(55, 85, 190), QStringLiteral("Three"));
    view.addPage(one, QIcon(), QStringLiteral("One"));
    view.addPage(two, QIcon(), QStringLiteral("Two"));
    view.addPage(three, QIcon(), QStringLiteral("Three"));
    view.stack()->setDuration(120);
    view.show();
    QCoreApplication::processEvents();

    auto *stack = view.stack();
    const QPoint sample(8, 8);
    const QRect pageRect = stack->rect();
    QVERIFY(!pageRect.isEmpty());
    QCOMPARE(one->geometry(), pageRect);
    QCOMPARE(two->geometry(), pageRect);
    QCOMPARE(three->geometry(), pageRect);

    const QRect secondItem = view.navigationList()->visualItemRect(
        view.navigationList()->item(1));
    QTest::mouseClick(view.navigationList()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, secondItem.center());
    QCoreApplication::processEvents();
    QVERIFY(stack->isAnimating());
    QCOMPARE(stack->currentWidget(), two);
    QCOMPARE(one->isVisible(), false);
    QCOMPARE(two->isVisible(), false);
    QCOMPARE(three->isVisible(), false);
    QCOMPARE(one->geometry(), pageRect);
    QCOMPARE(two->geometry(), pageRect);
    QCOMPARE(three->geometry(), pageRect);
    auto overlays = stack->findChildren<QWidget *>(
        QStringLiteral("_winui_animated_stack_overlay"),
        Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(overlays.constFirst()->isVisible(), true);
    QVERIFY(overlays.constFirst()->graphicsEffect() == nullptr);
    const QImage first = stack->grab().toImage();
    const QColor firstPixel = first.pixelColor(sample);
    QVERIFY(colorDistance(firstPixel, QColor(55, 165, 85))
            < colorDistance(firstPixel, QColor(180, 55, 55)));

    const auto containsOutgoingPageColor = [](const QImage &image) {
        const QColor outgoing(180, 55, 55);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (colorDistance(image.pixelColor(x, y), outgoing) < 12)
                    return true;
            }
        }
        return false;
    };
    QVERIFY2(!containsOutgoingPageColor(first),
             "The first transition frame still contains outgoing-page pixels");

    auto *group = stack->findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(group);
    group->setCurrentTime(group->duration() / 2);
    QCoreApplication::processEvents();
    const QImage midpoint = stack->grab().toImage();
    QVERIFY(midpoint != first);
    const QColor midpointPixel = midpoint.pixelColor(sample);
    QVERIFY(colorDistance(midpointPixel, QColor(55, 165, 85))
            < colorDistance(midpointPixel, QColor(180, 55, 55)));
    QVERIFY2(!containsOutgoingPageColor(midpoint),
             "The midpoint still blends outgoing and incoming page content");

    // Interrupt before completion, then immediately reverse again. There is
    // always one composited snapshot and one current page; no stale page or
    // effect is allowed to remain visible underneath the new target.
    view.setCurrentIndex(2);
    QCoreApplication::processEvents();
    QCOMPARE(stack->currentWidget(), three);
    QCOMPARE(stack->isAnimating(), true);
    view.setCurrentIndex(0);
    QCoreApplication::processEvents();
    QCOMPARE(stack->currentWidget(), one);
    QCOMPARE(stack->isAnimating(), true);
    overlays = stack->findChildren<QWidget *>(
        QStringLiteral("_winui_animated_stack_overlay"),
        Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(one->isVisible(), false);
    QCOMPARE(two->isVisible(), false);
    QCOMPARE(three->isVisible(), false);
    QCOMPARE(one->geometry(), stack->rect());
    QCOMPARE(two->geometry(), stack->rect());
    QCOMPARE(three->geometry(), stack->rect());

    group = stack->findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(group);
    group->setCurrentTime(group->duration());
    QCoreApplication::processEvents();
    QVERIFY(!stack->isAnimating());
    QCOMPARE(stack->currentWidget(), one);
    QCOMPARE(one->isVisible(), true);
    QVERIFY(one->graphicsEffect() == nullptr);
    QVERIFY(two->graphicsEffect() == nullptr);
    QVERIFY(three->graphicsEffect() == nullptr);
    stack->hide();
    QVERIFY(!stack->isAnimating());
    stack->show();
    QCOMPARE(stack->currentWidget(), one);

    // currentChanged is synchronous. A consumer may redirect navigation from
    // that signal while the first transition is still being constructed.
    WinUI3::AnimatedStack reentrant;
    reentrant.setDuration(60);
    reentrant.addWidget(new SolidPage(QColor(190, 70, 70), QStringLiteral("A")));
    reentrant.addWidget(new SolidPage(QColor(70, 190, 90), QStringLiteral("B")));
    reentrant.addWidget(new SolidPage(QColor(70, 90, 190), QStringLiteral("C")));
    reentrant.resize(320, 160);
    reentrant.show();
    bool redirected = false;
    connect(&reentrant, &QStackedWidget::currentChanged,
            [&reentrant, &redirected](int index) {
        if (index == 1 && !redirected) {
            redirected = true;
            reentrant.setCurrentIndex(2);
        }
    });
    reentrant.setCurrentIndex(1);
    QCoreApplication::processEvents();
    auto *reentrantGroup = reentrant.findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_animated_stack_group"),
        Qt::FindDirectChildrenOnly);
    QVERIFY(reentrantGroup);
    reentrantGroup->setCurrentTime(reentrantGroup->duration());
    QCoreApplication::processEvents();
    QVERIFY(redirected);
    QCOMPARE(reentrant.currentIndex(), 2);
    QVERIFY(!reentrant.isAnimating());
    QCOMPARE(reentrant.findChildren<QWidget *>(
                 QStringLiteral("_winui_animated_stack_overlay"),
                 Qt::FindDirectChildrenOnly).size(), 0);
}

void WinUI3StyleTest::renderCommonStates()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    auto *normal = new QPushButton(QStringLiteral("Normal"));
    auto *accent = new QPushButton(QStringLiteral("Accent"));
    WinUI3::Style::setControlRole(accent, WinUI3::ControlRole::Accent);
    auto *disabled = new QPushButton(QStringLiteral("Disabled"));
    disabled->setEnabled(false);
    layout->addWidget(normal);
    layout->addWidget(accent);
    layout->addWidget(disabled);
    host.resize(320, 180);
    host.show();
    QTest::mouseMove(normal, normal->rect().center());
    QTest::qWait(100);
    const QImage image = host.grab().toImage();
    QCOMPARE(image.size(), host.size());
    QVERIFY(!image.isNull());
    QVERIFY(image.pixelColor(0, 0).isValid());
}

void WinUI3StyleTest::pluginFactory()
{
#ifdef WINUI3STYLE_TEST_PLUGIN
    QVERIFY(QStyleFactory::keys().contains(QStringLiteral("winui3"), Qt::CaseInsensitive));
    QScopedPointer<QStyle> loaded(QStyleFactory::create(QStringLiteral("winui3")));
    QVERIFY(loaded);
    QCOMPARE(loaded->objectName(), QStringLiteral("winui3"));
#else
    QSKIP("Style plugin was disabled at configure time");
#endif
}

void WinUI3StyleTest::inputModalityFocus()
{
    QPushButton button(QStringLiteral("Focus"));
    button.resize(button.sizeHint());
    button.show();
    QMouseEvent mousePress(QEvent::MouseButtonPress, QPointF(4, 4),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&button, &mousePress);
    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&button, &mouseFocus);
    QVERIFY(!frameBool(&button, "_winui_focus_visible"));
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(&button, &escape);
    QVERIFY(!frameBool(&button, "_winui_focus_visible"));
    QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
    QCoreApplication::sendEvent(&button, &space);
    QVERIFY(frameBool(&button, "_winui_focus_visible"));
    QCoreApplication::sendEvent(&button, &mousePress);
    QVERIFY(!frameBool(&button, "_winui_focus_visible"));
    QFocusEvent keyboardFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&button, &keyboardFocus);
    QVERIFY(frameBool(&button, "_winui_focus_visible"));
}

void WinUI3StyleTest::hoverAnimationProgresses()
{
    QPushButton button(QStringLiteral("Hover"));
    FrameDynamicPropertyProbe probe;
    button.installEventFilter(&probe);
    button.resize(button.sizeHint());
    button.show();
    if (!button.style()->styleHint(QStyle::SH_Widget_Animate, nullptr, &button))
        QSKIP("Client-area animations are disabled by the OS");
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&button, &enter);
    // The first animation-driver tick can be delayed when the full suite has
    // several native timers pending. Wait for that tick instead of assuming
    // it has happened after a fixed wall-clock sleep.
    QTRY_VERIFY_WITH_TIMEOUT(
        frameReal(&button, "_winui_hover_progress") > 0.0, 150);
    const qreal midway = frameReal(&button, "_winui_hover_progress");
    QVERIFY2(midway > 0.0 && midway < 1.0, qPrintable(QString::number(midway)));
    QTRY_VERIFY(frameReal(&button, "_winui_hover_progress") > 0.99);
    QCOMPARE(probe.frameChanges, 0);
    QVERIFY(!button.property("_winui_hover_progress").isValid());
}

void WinUI3StyleTest::textBoxInteraction()
{
    QLineEdit edit;
    edit.setText(QStringLiteral("Clear me"));
    edit.setPlaceholderText(QStringLiteral("Placeholder"));
    edit.setClearButtonEnabled(true);
    edit.resize(240, 32);
    edit.show();
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&edit, &enter);
    QCOMPARE(frameReal(&edit, "_winui_hover_progress"), 1.0);

    QStyleOptionFrame option;
    option.initFrom(&edit);
    option.rect = edit.rect();
    const QRect ltrContents = edit.style()->subElementRect(
        QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(ltrContents, edit.rect().adjusted(10, 5, -6, -6));
    option.direction = Qt::RightToLeft;
    const QRect rtlContents = edit.style()->subElementRect(
        QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(rtlContents, edit.rect().adjusted(6, 5, -10, -6));
    option.direction = Qt::LeftToRight;

    QVERIFY(!edit.style()->standardIcon(QStyle::SP_LineEditClearButton,
                                         nullptr, &edit).isNull());
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    const auto *clearButton = edit.findChildren<QAbstractButton *>().constFirst();
    QStyleOptionToolButton clearOption;
    clearOption.initFrom(clearButton);
    QCOMPARE(edit.style()->sizeFromContents(QStyle::CT_ToolButton, &clearOption,
                                             QSize(16, 16), clearButton),
             QSize(30, 32));

    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&edit, &mouseFocus);
    QCOMPARE(frameReal(&edit, "_winui_focus_progress"), 1.0);
    QVERIFY(!frameBool(&edit, "_winui_focus_visible"));

    QImage focused(edit.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        edit.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option,
                                    &painter, &edit);
    }
    const QColor accent = edit.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    QVERIFY(distance(focused.pixelColor(edit.rect().center().x(),
                                         edit.rect().bottom() - 1), accent) < 80);
    QVERIFY(distance(focused.pixelColor(5, edit.rect().bottom() - 1), accent) < 100);

    QFocusEvent tabFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&edit, &tabFocus);
    QVERIFY(frameBool(&edit, "_winui_focus_visible"));
}

void WinUI3StyleTest::clearButtonStateContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QLineEdit edit(QStringLiteral("Clear me"));
    edit.setClearButtonEnabled(true);
    edit.resize(240, 32);
    edit.show();
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    QAbstractButton *clearButton = edit.findChildren<QAbstractButton *>().constFirst();
    QVERIFY(clearButton);
    QTRY_VERIFY(clearButton->testAttribute(Qt::WA_Hover));
    QTest::mouseMove(&edit, QPoint(8, edit.rect().center().y()));
    QTRY_VERIFY(!clearButton->underMouse());
    const QImage runtimeNormal = edit.grab().toImage();
    const QRect helperRect = clearButton->geometry().intersected(edit.rect());
    QVERIFY2(helperRect.isValid(), qPrintable(QString::fromLatin1(
        "private clear button has no editor intersection")));
    QVERIFY(helperRect.height() < edit.height());
    QVERIFY(helperRect.top() > edit.rect().top());
    QRectF surfaceGeometry(helperRect);
    surfaceGeometry.setTop(edit.rect().top() + 3.0);
    surfaceGeometry.setBottom(edit.rect().bottom() - 2.0);
    const qreal surfaceSide = surfaceGeometry.height();
    if (edit.layoutDirection() == Qt::RightToLeft) {
        surfaceGeometry.setLeft(edit.rect().left() + 3.0);
        surfaceGeometry.setRight(surfaceGeometry.left() + surfaceSide);
    } else {
        surfaceGeometry.setRight(edit.rect().right() - 2.0);
        surfaceGeometry.setLeft(surfaceGeometry.right() - surfaceSide);
    }
    const QRect surfaceRect = surfaceGeometry.toAlignedRect();
    QVERIFY(surfaceRect.isValid());
    QVERIFY(qAbs(surfaceGeometry.width() - surfaceGeometry.height()) < 0.01);
    QVERIFY(qAbs(surfaceGeometry.center().x() - helperRect.center().x()) <= 1.5);
    QVERIFY(qAbs(surfaceGeometry.center().y() - edit.rect().center().y()) <= 1.0);
    UpdateRequestProbe parentRepaint;
    edit.installEventFilter(&parentRepaint);
    QCoreApplication::processEvents();
    parentRepaint.updateRequests = 0;
    parentRepaint.paints = 0;
    QTest::mouseMove(clearButton, clearButton->rect().center());
    QTRY_VERIFY(clearButton->underMouse());
    QTRY_COMPARE(frameReal(clearButton, "_winui_hover_progress"), 1.0);
    QCoreApplication::processEvents();
    const QImage runtimeHover = edit.grab().toImage();
    QVERIFY(parentRepaint.updateRequests > 0);
    QVERIFY(runtimeNormal != runtimeHover);
    const QPoint surfaceCenter(surfaceRect.center().x(), surfaceRect.center().y());
    for (const QPoint &corner : {surfaceRect.topLeft(), surfaceRect.topRight(),
                                 surfaceRect.bottomLeft(), surfaceRect.bottomRight()})
        QCOMPARE(runtimeHover.pixelColor(corner), runtimeNormal.pixelColor(corner));
    QCOMPARE(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.top() - 1),
             runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.top() - 1));
    QCOMPARE(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.bottom() + 1),
             runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.bottom() + 1));
    QVERIFY(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.top())
            != runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.top()));
    QVERIFY(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.bottom())
            != runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.bottom()));
    int visiblyChangedPixels = 0;
    int maximumChannelDelta = 0;
    for (int y = helperRect.top(); y <= helperRect.bottom(); ++y) {
        for (int x = helperRect.left(); x <= helperRect.right(); ++x) {
            const QColor before = runtimeNormal.pixelColor(x, y);
            const QColor after = runtimeHover.pixelColor(x, y);
            const int delta = std::max({qAbs(before.red() - after.red()),
                                        qAbs(before.green() - after.green()),
                                        qAbs(before.blue() - after.blue())});
            maximumChannelDelta = qMax(maximumChannelDelta, delta);
            if (delta >= 3)
                ++visiblyChangedPixels;
        }
    }
    QVERIFY2(visiblyChangedPixels >= 100,
             qPrintable(QStringLiteral("changed=%1 maxDelta=%2")
                            .arg(visiblyChangedPixels)
                            .arg(maximumChannelDelta)));
    QVERIFY2(maximumChannelDelta >= 6,
             qPrintable(QStringLiteral("maxDelta=%1")
                            .arg(maximumChannelDelta)));
    QTest::mouseMove(&edit, QPoint(8, edit.rect().center().y()));
    QTRY_COMPARE(frameReal(clearButton, "_winui_hover_progress"), 0.0);
    setFrame(clearButton, "_winui_hover_progress", 0.0);
    setFrame(clearButton, "_winui_press_progress", 0.0);

    parentRepaint.updateRequests = 0;
    QTest::mousePress(clearButton, Qt::LeftButton, Qt::NoModifier,
                      clearButton->rect().center());
    QTRY_COMPARE(frameReal(clearButton, "_winui_press_progress"), 1.0);
    QCoreApplication::processEvents();
    QVERIFY(parentRepaint.updateRequests > 0);
    const QImage runtimePressed = edit.grab().toImage();
    QVERIFY(runtimeHover != runtimePressed);
    QTest::mouseRelease(clearButton, Qt::LeftButton, Qt::NoModifier,
                        clearButton->rect().center());

    QStyleOptionToolButton option;
    option.initFrom(clearButton);
    option.rect = QRect(QPoint(), QSize(30, 32));
    option.palette = clearButton->palette();
    option.icon = clearButton->icon().isNull()
        ? style->standardIcon(QStyle::SP_LineEditClearButton, nullptr, &edit)
        : clearButton->icon();
    option.iconSize = QSize(16, 16);

    const auto render = [&](QStyle::State state, qreal hover, qreal press) {
        setFrame(clearButton, "_winui_hover_progress", hover);
        setFrame(clearButton, "_winui_press_progress", press);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(edit.palette().color(QPalette::Window));
        option.state = state;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonTool, &option,
                             &painter, clearButton);
        style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                           &painter, clearButton);
        return image;
    };

    const QImage normal = render(QStyle::State_Enabled, 0.0, 0.0);
    const QImage pointerOver = render(QStyle::State_Enabled | QStyle::State_MouseOver,
                                      1.0, 0.0);
    const QImage pressed = render(QStyle::State_Enabled | QStyle::State_MouseOver
                                  | QStyle::State_Sunken, 1.0, 1.0);
    // The private helper's own primitive intentionally stays transparent;
    // hover is painted once, at editor scope, so it cannot be clipped by the
    // private child button's small geometry.
    QCOMPARE(normal, pointerOver);
    QVERIFY(pointerOver != pressed);

    // DeleteButton keeps the secondary glyph on pointer-over and switches to
    // the tertiary foreground only while pressed.
    const auto glyphOnly = [&](QStyle::State state) {
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        option.state = state;
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                           &painter, clearButton);
        return image;
    };
    const QImage normalGlyph = glyphOnly(QStyle::State_Enabled);
    const QImage hoverGlyph = glyphOnly(QStyle::State_Enabled | QStyle::State_MouseOver);
    const QImage pressedGlyph = glyphOnly(QStyle::State_Enabled | QStyle::State_Sunken);
    QCOMPARE(normalGlyph, hoverGlyph);
    QVERIFY(normalGlyph != pressedGlyph);
}

void WinUI3StyleTest::backdropButtonRepaintDoesNotAccumulate()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.setProperty("_winui_backdrop", 1);
    QPushButton button(QStringLiteral("Mica"), &window);
    button.resize(96, 32);

    QStyleOptionButton option;
    option.initFrom(&button);
    option.rect = button.rect();
    const auto paintFrame = [&](QImage &image, QStyle::State state) {
        option.state = state;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonCommand, &option,
                             &painter, &button);
    };
    QImage normal(button.size(), QImage::Format_ARGB32_Premultiplied);
    normal.fill(Qt::transparent);
    paintFrame(normal, QStyle::State_Enabled);

    QImage hoverThenNormal(button.size(), QImage::Format_ARGB32_Premultiplied);
    hoverThenNormal.fill(Qt::transparent);
    setFrame(&button, "_winui_hover_progress", 1.0);
    paintFrame(hoverThenNormal,
               QStyle::State_Enabled | QStyle::State_MouseOver);
    setFrame(&button, "_winui_hover_progress", 0.0);
    paintFrame(hoverThenNormal, QStyle::State_Enabled);
    QCOMPARE(hoverThenNormal, normal);

    QWidget opaqueLayer(&window);
    opaqueLayer.setProperty(WinUI3::Style::SurfaceProperty,
                            QStringLiteral("content"));
    QPushButton layeredButton(QStringLiteral("Layered"), &opaqueLayer);
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&layeredButton));
}

void WinUI3StyleTest::clearButtonFocusAndGlyphContract()
{
    QWidget host;
    QVBoxLayout layout(&host);
    QLineEdit edit(QStringLiteral("Clear me"));
    edit.setClearButtonEnabled(true);
    QPushButton other(QStringLiteral("Other"));
    layout.addWidget(&edit);
    layout.addWidget(&other);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    QAbstractButton *clearButton = edit.findChildren<QAbstractButton *>().constFirst();

    other.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!clearButton->isVisible());
    edit.clear();
    edit.setText(QStringLiteral("Programmatic text"));
    QTRY_VERIFY(!clearButton->isVisible());
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(clearButton->isVisible());
    other.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!clearButton->isVisible());
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(clearButton->isVisible());

    QStyleOptionToolButton option;
    option.initFrom(clearButton);
    option.rect = QRect(QPoint(), QSize(30, 32));
    option.icon = clearButton->icon();
    option.iconSize = QSize(16, 16); // Qt default; style must enforce WinUI 12 px.
    option.state = QStyle::State_Enabled;
    QImage glyph(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    glyph.fill(Qt::transparent);
    {
        QPainter painter(&glyph);
        edit.style()->drawControl(QStyle::CE_ToolButtonLabel, &option,
                                  &painter, clearButton);
    }
    QRect ink;
    for (int y = 0; y < glyph.height(); ++y) {
        for (int x = 0; x < glyph.width(); ++x) {
            if (glyph.pixelColor(x, y).alpha() > 24)
                ink |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(!ink.isEmpty());
    QVERIFY(ink.width() <= 12);
    QVERIFY(ink.height() <= 12);
    const QPointF officialCenter(13.0, 16.0);
    QVERIFY2(qAbs(ink.center().x() - officialCenter.x()) <= 2.0,
             qPrintable(QStringLiteral("ink=%1,%2 %3x%4")
                            .arg(ink.x()).arg(ink.y())
                            .arg(ink.width()).arg(ink.height())));
    QVERIFY2(qAbs(ink.center().y() - officialCenter.y()) <= 2.0,
             qPrintable(QStringLiteral("ink=%1,%2 %3x%4")
                            .arg(ink.x()).arg(ink.y())
                            .arg(ink.width()).arg(ink.height())));
}

void WinUI3StyleTest::textBoxStateMatrix()
{
    auto renderPanel = [](WinUI3::Style &style, QStyle::State state) {
        QStyleOptionFrame option;
        option.rect = QRect(0, 0, 240, 32);
        option.palette = style.standardPalette();
        option.state = state;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style.drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter);
        return image;
    };

    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        WinUI3::Style style(mode);
        const QPalette palette = style.standardPalette();
        QCOMPARE(palette.color(QPalette::Highlight), style.accentColor());
        const QColor expectedOnAccent = qGray(style.accentColor().rgb()) < 128
            ? QColor(Qt::white) : QColor(Qt::black);
        QCOMPARE(palette.color(QPalette::HighlightedText), expectedOnAccent);
        QVERIFY(palette.color(QPalette::Disabled, QPalette::Text).alpha() <
                palette.color(QPalette::Active, QPalette::Text).alpha());
        QVERIFY(palette.color(QPalette::Disabled, QPalette::PlaceholderText).alpha() <
                palette.color(QPalette::Active, QPalette::PlaceholderText).alpha());

        const QImage normal = renderPanel(style, QStyle::State_Enabled);
        const QImage focused = renderPanel(style, QStyle::State_Enabled
                                                     | QStyle::State_HasFocus);
        const QImage disabled = renderPanel(style, QStyle::State_None);
        QVERIFY(normal != focused);
        QVERIFY(normal != disabled);
        QCOMPARE(disabled.pixelColor(disabled.rect().center().x(),
                                     disabled.rect().bottom() - 1),
                 disabled.pixelColor(4, disabled.rect().bottom() - 1));
        QVERIFY(focused.pixelColor(focused.rect().center().x(),
                                   focused.rect().bottom() - 1)
                != disabled.pixelColor(disabled.rect().center().x(),
                                       disabled.rect().bottom() - 1));
    }

    QLineEdit readOnly(QStringLiteral("Selection remains available"));
    readOnly.setClearButtonEnabled(true);
    readOnly.resize(240, 32);
    readOnly.show();
    readOnly.setReadOnly(true);
    readOnly.selectAll();
    QCOMPARE(readOnly.selectedText(), readOnly.text());
    const QString before = readOnly.text();
    QTest::keyClicks(&readOnly, QStringLiteral("blocked"));
    QCOMPARE(readOnly.text(), before);
    QTRY_VERIFY(!readOnly.findChildren<QAbstractButton *>().isEmpty());
    QTRY_VERIFY(!readOnly.findChildren<QAbstractButton *>().constFirst()->isVisible());

    QLineEdit disabled(QStringLiteral("Disabled text"));
    disabled.setPlaceholderText(QStringLiteral("Disabled placeholder"));
    disabled.setEnabled(false);
    disabled.resize(240, 32);
    disabled.show();
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::Text),
             QColor(0, 0, 0, 92));
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::PlaceholderText),
             QColor(0, 0, 0, 92));
}

void WinUI3StyleTest::indeterminateProgressDeterminism()
{
    const bool animationSettingExisted = qEnvironmentVariableIsSet(
        "WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray previousSetting = qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");

    QProgressBar bar;
    bar.setRange(0, 0);
    bar.resize(320, 24);
    bar.show();
    QImage first(bar.size(), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::transparent);
    bar.render(&first);
    QTest::qWait(25);
    QImage second(bar.size(), QImage::Format_ARGB32_Premultiplied);
    second.fill(Qt::transparent);
    bar.render(&second);

    if (animationSettingExisted)
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previousSetting);
    else
        qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    QCOMPARE(first, second);
}

void WinUI3StyleTest::themeComboSizingContract()
{
    // Keep this in lockstep with GalleryWindow's command-bar theme selector:
    // the longest item must remain available before the toolbar is shown.
    QComboBox combo;
    combo.addItems({QStringLiteral("System theme"), QStringLiteral("Light"),
                    QStringLiteral("Dark")});
    combo.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo.setMinimumContentsLength(combo.itemText(0).size());
    combo.setMinimumWidth(combo.sizeHint().width());

    const int textWidth = QFontMetrics(combo.font()).horizontalAdvance(
        combo.itemText(0));
    QVERIFY(combo.minimumWidth() >= textWidth);
    combo.resize(combo.minimumWidth(), 32);
    combo.show();
    QTRY_VERIFY(combo.isVisible());

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.currentText = combo.currentText();
    const QRect edit = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY2(edit.width() >= textWidth,
             qPrintable(QStringLiteral("edit width %1 < text width %2")
                            .arg(edit.width()).arg(textWidth)));

    combo.setEditable(true);
    QVERIFY(combo.lineEdit());
    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                 Qt::RightToLeft}) {
        combo.setLayoutDirection(direction);
        QCoreApplication::processEvents();
        option.initFrom(&combo);
        option.rect = combo.rect();
        option.direction = direction;
        const QRect labelSlot = combo.style()->subControlRect(
            QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &combo);
        QStyleOptionFrame editorOption;
        editorOption.initFrom(combo.lineEdit());
        editorOption.rect = combo.lineEdit()->rect();
        editorOption.direction = direction;
        const QRect editorContents = combo.lineEdit()->style()->subElementRect(
            QStyle::SE_LineEditContents, &editorOption, combo.lineEdit())
            .translated(combo.lineEdit()->pos());
        if (direction == Qt::LeftToRight)
            QCOMPARE(editorContents.left(), labelSlot.left());
        else
            QCOMPARE(editorContents.right(), labelSlot.right());
    }
}

void WinUI3StyleTest::comboPopupContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({QStringLiteral("Blue"), QStringLiteral("Green"), QStringLiteral("Red")});
    combo.resize(200, 32);
    combo.move(120, 180);
    host.show();
    QTRY_VERIFY(host.isVisible());
    QWidget *popup = combo.view()->window();
    QVERIFY(popup);
    PopupGeometryProbe probe;
    probe.combo = &combo;
    probe.popup = popup;
    popup->installEventFilter(&probe);
    combo.view()->viewport()->installEventFilter(&probe);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    combo.setCurrentIndex(1);
    QSignalSpy firstOpenScrollChanges(combo.view()->verticalScrollBar(),
                                      &QScrollBar::valueChanged);
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QTRY_VERIFY(!probe.selectedCenterAtShow.isNull());
    QCOMPARE(popup->geometry(), probe.geometryAtShow);
    const QPoint comboCenterOnFirstOpen = combo.mapToGlobal(combo.rect().center());
    const QRect firstOpenSelectedRect = combo.view()->visualRect(
        combo.model()->index(1, combo.modelColumn(), combo.rootModelIndex()));
    const QPoint firstOpenSelectedCenter = combo.view()->viewport()->mapToGlobal(
        firstOpenSelectedRect.center());
    QVERIFY2(qAbs(probe.selectedCenterAtShow.y() - comboCenterOnFirstOpen.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                            .arg(probe.selectedCenterAtShow.y())
                            .arg(comboCenterOnFirstOpen.y())
                            .arg(popup->x()).arg(popup->y())
                            .arg(popup->width()).arg(popup->height())
                            .arg(firstOpenSelectedRect.x()).arg(firstOpenSelectedRect.y())
                            .arg(firstOpenSelectedRect.width()).arg(firstOpenSelectedRect.height())));
    QCOMPARE(firstOpenSelectedCenter, probe.selectedCenterAtShow);
    QCOMPARE(combo.view()->verticalScrollBar()->value(), probe.scrollValueAtShow);
    QCOMPARE(firstOpenScrollChanges.count(), 0);
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), probe.geometryAtShow);
    QCOMPARE(combo.view()->viewport()->mapToGlobal(
                 combo.view()->visualRect(combo.model()->index(1, 0)).center()),
             probe.selectedCenterAtShow);
    QCOMPARE(combo.view()->verticalScrollBar()->value(), probe.scrollValueAtShow);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    QVERIFY(probe.layoutsAfterShow <= 1);
    QVERIFY(combo.view()->palette().color(QPalette::Text).lightness() > 128);
    QVERIFY(combo.view()->palette().color(QPalette::Window).lightness() < 128);
    const QColor runtimeAccent(220, 40, 80);
    style->setAccentColor(runtimeAccent);
    QTRY_COMPARE(combo.view()->palette().color(QPalette::Highlight), runtimeAccent);
    combo.hidePopup();
    combo.setCurrentIndex(0);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    probe.reset();
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QVERIFY(combo.view()->palette().color(QPalette::Text).lightness() < 128);
    QVERIFY(combo.view()->palette().color(QPalette::Window).lightness() > 128);
    const QPoint comboCenter = combo.mapToGlobal(combo.rect().center());
    const QRect firstRow = combo.view()->visualRect(combo.model()->index(0, 0));
    const QPoint selectedCenter = combo.view()->viewport()->mapToGlobal(
        firstRow.center());
    QVERIFY2(qAbs(selectedCenter.y() - comboCenter.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                            .arg(selectedCenter.y()).arg(comboCenter.y())
                            .arg(popup->x()).arg(popup->y())
                            .arg(popup->width()).arg(popup->height())
                            .arg(firstRow.x()).arg(firstRow.y())
                            .arg(firstRow.width()).arg(firstRow.height())));
    // A direct QComboBox::showPopup() can require one synchronous correction while
    // QEvent::Show is being dispatched.  That happens before the popup is composed;
    // only geometry changes after the completed Show event can produce a visible jump.
    QVERIFY(probe.movesAfterShow <= 1);
    const QRect settledGeometry = popup->geometry();
    probe.movesAfterShow = 0;
    probe.resizesAfterShow = 0;
    probe.layoutsAfterShow = 0;
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), settledGeometry);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    QVERIFY(probe.layoutsAfterShow <= 1);

    const QImage lightPopup = combo.view()->viewport()->grab().toImage();
    const QRect secondRow = combo.view()->visualRect(combo.model()->index(1, 0));
    bool darkTextPixel = false;
    for (int y = secondRow.top(); y <= secondRow.bottom() && !darkTextPixel; ++y) {
        for (int x = 32; x < lightPopup.width() - 8; ++x) {
            const QColor pixel = lightPopup.pixelColor(x, y);
            if (pixel.alpha() > 100 && pixel.lightness() < 100) {
                darkTextPixel = true;
                break;
            }
        }
    }
    QVERIFY(darkTextPixel);
    QCOMPARE(firstRow.height(), 40);
    const QColor accent = combo.palette().color(QPalette::Highlight);
    bool accentPill = false;
    for (int y = firstRow.center().y() - 9;
         y <= firstRow.center().y() + 9 && !accentPill; ++y) {
        for (int x = 4; x <= 10; ++x) {
            const QColor pixel = lightPopup.pixelColor(x, y);
            const int distance = qAbs(pixel.red() - accent.red())
                + qAbs(pixel.green() - accent.green())
                + qAbs(pixel.blue() - accent.blue());
            if (pixel.alpha() > 100 && distance < 80) {
                accentPill = true;
                break;
            }
        }
    }
    QVERIFY(accentPill);

    combo.hidePopup();
    probe.reset();
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    const QRect reopenedRow = combo.view()->visualRect(
        combo.model()->index(combo.currentIndex(), combo.modelColumn(),
                             combo.rootModelIndex()));
    const QPoint reopenedSelectedCenter = combo.view()->viewport()->mapToGlobal(
        reopenedRow.center());
    QVERIFY(qAbs(reopenedSelectedCenter.y() - comboCenter.y()) <= 4);
    const QRect reopenedGeometry = popup->geometry();
    QVERIFY(probe.movesAfterShow <= 1);
    probe.movesAfterShow = 0;
    probe.resizesAfterShow = 0;
    probe.layoutsAfterShow = 0;
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), reopenedGeometry);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    QVERIFY(probe.layoutsAfterShow <= 1);

    QCOMPARE(combo.style()->standardIcon(QStyle::SP_ArrowDown).isNull(), false);
    const QRect second = combo.view()->visualRect(combo.model()->index(1, 0));
    QTest::mouseMove(combo.view()->viewport(), second.center());
    QTest::mouseClick(combo.view()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, second.center());
    QCOMPARE(combo.currentIndex(), 1);
    style->setAccentColor({});
}

void WinUI3StyleTest::comboReleaseActivationAndMarkerMotion()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QWidget host;
    host.resize(440, 320);
    QComboBox combo(&host);
    combo.addItems({QStringLiteral("First"), QStringLiteral("Selected"),
                    QStringLiteral("Last")});
    combo.setCurrentIndex(1);
    combo.resize(220, 32);
    combo.move(100, 140);
    host.show();
    QTRY_VERIFY(host.isVisible());

    QAbstractItemView *view = combo.view();
    QVERIFY(view);
    QWidget *popup = view->window();
    QVERIFY(popup);
    QVERIFY(!view->isVisible());

    // WinUI opens a ComboBox on the button release.  In particular, a press
    // must not create the popup or perform its first layout pass.
    const QPoint comboCenter = combo.rect().center();
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QVERIFY(!view->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(&combo, "_winui_press_progress") > 0.0,
                             150);

    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTRY_VERIFY(view->isVisible());
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));

    // The popup's outer 4px bands must survive the view layout and be equal
    // on both sides.  Account for the frame border by measuring the viewport
    // rather than relying on the private container's child hierarchy.
    const QRect viewportInPopup(
        view->viewport()->mapTo(popup, QPoint(0, 0)),
        view->viewport()->size());
    QVERIFY(viewportInPopup.top() >= 4);
    QVERIFY(popup->height() - viewportInPopup.bottom() - 1 >= 4);

    const QModelIndex selectedIndex = combo.model()->index(
        combo.currentIndex(), combo.modelColumn(), combo.rootModelIndex());
    const QRect selectedRow = view->visualRect(selectedIndex);
    QVERIFY(selectedRow.isValid());
    const QPoint selectedCenter = view->viewport()->mapToGlobal(
        selectedRow.center());
    const QPoint expectedCenter = combo.mapToGlobal(combo.rect().center());
    QVERIFY(qAbs(selectedCenter.y() - expectedCenter.y()) <= 4);

    // The selected item's marker is the only ComboBox-specific animation:
    // the XAML template keeps its 3px width and compresses ScaleY from 1 to
    // 0.625 over 167ms while the pointer is held.
    const QColor background = view->viewport()->palette().color(QPalette::Window);
    const QColor accent = view->viewport()->palette().color(QPalette::Highlight);
    QStyleOptionViewItem item;
    item.initFrom(view->viewport());
    item.rect = QRect(0, 0, view->viewport()->width(), selectedRow.height());
    item.direction = Qt::LeftToRight;
    item.state = QStyle::State_Enabled | QStyle::State_Selected;
    item.features = QStyleOptionViewItem::HasDisplay;
    item.text = combo.currentText();
    item.index = selectedIndex;

    const auto renderMarker = [&](qreal press, Qt::LayoutDirection direction) {
        item.direction = direction;
        setFrame(view->viewport(), "_winui_press_progress", press);
        QImage image(item.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(background);
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ItemViewItem, &item, &painter,
                           view->viewport());
        return image;
    };
    const auto markerHeight = [&](const QImage &image,
                                  Qt::LayoutDirection direction) {
        const int left = direction == Qt::RightToLeft
            ? image.width() - 12 : 0;
        const int right = direction == Qt::RightToLeft
            ? image.width() - 1 : 11;
        int top = image.height();
        int bottom = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = qMax(0, left); x <= qMin(image.width() - 1, right); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alpha() > 120 && colorDistance(pixel, accent) < 100) {
                    top = qMin(top, y);
                    bottom = qMax(bottom, y);
                }
            }
        }
        return bottom >= top ? bottom - top + 1 : 0;
    };

    const QImage normalMarker = renderMarker(0.0, Qt::LeftToRight);
    const QImage pressedMarker = renderMarker(1.0, Qt::LeftToRight);
    const int normalHeight = markerHeight(normalMarker, Qt::LeftToRight);
    const int pressedHeight = markerHeight(pressedMarker, Qt::LeftToRight);
    QVERIFY2(normalHeight >= 14,
             qPrintable(QStringLiteral("normal marker height=%1")
                            .arg(normalHeight)));
    QVERIFY2(pressedHeight >= 8 && pressedHeight <= 12,
             qPrintable(QStringLiteral("pressed marker height=%1")
                            .arg(pressedHeight)));
    QVERIFY(pressedHeight < normalHeight);

    // Exercise the real item event path as well as the deterministic pixel
    // probe above. The held press must visibly shorten the selected marker,
    // not merely update an internal animation property.
    setFrame(view->viewport(), "_winui_press_progress", 0.0);
    view->viewport()->repaint();
    const auto liveMarkerHeight = [&] {
        QCoreApplication::processEvents();
        return markerHeight(view->viewport()->grab().toImage(),
                            Qt::LeftToRight);
    };
    const int liveNormalHeight = liveMarkerHeight();
    QTest::mouseMove(view->viewport(), selectedRow.center());
    QTest::mousePress(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      selectedRow.center());
    QVERIFY2(view->isVisible(), "Combo popup closed on item press");
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") > 0.0);
    QTRY_VERIFY_WITH_TIMEOUT(
        frameReal(view->viewport(), "_winui_press_progress") > 0.95, 300);
    QTRY_VERIFY_WITH_TIMEOUT(liveMarkerHeight() <= 12, 400);
    const int livePressedHeight = liveMarkerHeight();
    QVERIFY2(liveNormalHeight >= 14,
             qPrintable(QStringLiteral("live normal marker height=%1")
                            .arg(liveNormalHeight)));
    QVERIFY2(livePressedHeight >= 8 && livePressedHeight <= 12,
             qPrintable(QStringLiteral("live pressed marker height=%1")
                            .arg(livePressedHeight)));
    QVERIFY(livePressedHeight < liveNormalHeight);
    QTest::mouseRelease(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                        selectedRow.center());
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") < 0.05);
    QCOMPARE(combo.currentIndex(), 1);

    combo.hidePopup();

    // Releasing outside cancels the pending open and must not leave a stale
    // mouse grab or press state. RTL follows the same release contract.
    combo.setLayoutDirection(Qt::RightToLeft);
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QVERIFY(!view->isVisible());
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier,
                        QPoint(-20, -20));
    QTest::qWait(20);
    QVERIFY(!view->isVisible());
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTRY_VERIFY(view->isVisible());
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));

    const QImage rtlMarker = renderMarker(0.0, Qt::RightToLeft);
    QVERIFY(markerHeight(rtlMarker, Qt::RightToLeft) >= 14);
    combo.hidePopup();
}

void WinUI3StyleTest::comboPopupAssociationLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(360, 240);
    auto *combo = new QComboBox(&host);
    combo->addItems({QStringLiteral("One"), QStringLiteral("Two"),
                     QStringLiteral("Three")});
    combo->setCurrentIndex(1);
    host.show();
    QTRY_VERIFY(host.isVisible());

    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QWidget *popup = combo->view()->window();
    QVERIFY(popup);
    combo->hidePopup();

    // Re-polishing a combo must discard the old association and allow the
    // next show cycle to establish it again without a first-frame jump.
    style->unpolish(combo);
    style->polish(combo);
    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QCOMPARE(combo->view()->window(), popup);
    QVERIFY(combo->view()->visualRect(combo->model()->index(1, 0)).isValid());

    QPointer<QComboBox> guardedCombo(combo);
    delete combo;
    QVERIFY(guardedCombo.isNull());
}

void WinUI3StyleTest::comboChevronMotion()
{
    QComboBox combo;
    combo.addItems({QStringLiteral("One"), QStringLiteral("Two")});
    combo.resize(180, 32);
    combo.show();
    QEnterEvent enter(combo.rect().center(), combo.rect().center(),
                      combo.mapToGlobal(combo.rect().center()));
    QCoreApplication::sendEvent(&combo, &enter);
    QTest::qWait(30);
    QCOMPARE(frameReal(&combo, "_winui_combo_chevron_progress"), 0.0);

    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") > 0.9);
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier,
                        combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") < -0.05);
    QTRY_VERIFY(qAbs(frameReal(&combo, "_winui_combo_chevron_progress"))
                < 0.01);
    combo.hidePopup();
}

void WinUI3StyleTest::comboChevronGeometry()
{
    QComboBox combo;
    combo.addItems({QStringLiteral("One"), QStringLiteral("Two")});
    combo.resize(220, 32);
    combo.show();
    setFrame(&combo, "_winui_combo_chevron_progress", 0.0);

    const auto renderChevron = [&](Qt::LayoutDirection direction) {
        QStyleOptionComboBox option;
        option.initFrom(&combo);
        option.rect = combo.rect();
        option.direction = direction;
        option.subControls = QStyle::SC_ComboBoxArrow;
        option.state = QStyle::State_Enabled;

        QImage image(combo.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option,
                                              &painter, &combo);
        }

        const QRect logicalGlyphBox(option.rect.right() - 14 - 12 + 1,
                                    option.rect.top()
                                        + (option.rect.height() - 12) / 2,
                                    12, 12);
        const QRect logicalChevron(logicalGlyphBox.adjusted(1, 1, -1, -1));
        const QRect expected = QStyle::visualRect(direction, option.rect,
                                                  logicalChevron);
        QRect ink;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 24)
                    ink |= QRect(x, y, 1, 1);
            }
        }
        return qMakePair(expected, ink);
    };

    const auto ltr = renderChevron(Qt::LeftToRight);
    const auto rtl = renderChevron(Qt::RightToLeft);
    QVERIFY(!ltr.second.isEmpty());
    QVERIFY(!rtl.second.isEmpty());
    QVERIFY(ltr.first.contains(ltr.second.topLeft())
            && ltr.first.contains(ltr.second.bottomRight()));
    QVERIFY(rtl.first.contains(rtl.second.topLeft())
            && rtl.first.contains(rtl.second.bottomRight()));
    QVERIFY(ltr.second.width() <= 9);
    QVERIFY(ltr.second.height() <= 6);
    QVERIFY(rtl.second.width() <= 9);
    QVERIFY(rtl.second.height() <= 6);
    QCOMPARE(ltr.first.size(), QSize(10, 10));
    QCOMPARE(rtl.first.size(), QSize(10, 10));
    QCOMPARE(ltr.first.center().y(), rtl.first.center().y());
    QCOMPARE(ltr.first.center().x() + rtl.first.center().x(),
             combo.rect().left() + combo.rect().right() - 1);
}

void WinUI3StyleTest::numberBoxSubcontrolContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(150, 32);
    spin.show();

    auto *editor = spin.findChild<QLineEdit *>();
    QVERIFY(editor);
    QCOMPARE(editor->parentWidget(), &spin);
    QStyleOptionFrame editorOption;
    editorOption.initFrom(editor);
    editorOption.rect = QRect(0, 0, 60, 24);
    editorOption.state |= QStyle::State_MouseOver | QStyle::State_HasFocus;
    const QColor sentinel(17, 33, 49);
    QImage editorPanel(editorOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
    editorPanel.fill(sentinel);
    {
        QPainter painter(&editorPanel);
        spin.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &editorOption,
                                    &painter, editor);
        spin.style()->drawPrimitive(QStyle::PE_FrameLineEdit, &editorOption,
                                    &painter, editor);
    }
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.center()), sentinel);
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.topLeft()), sentinel);

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled
        | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
        | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                     QStyle::SC_SpinBoxEditField,
                                                     &spin);
    const QRect up = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                   QStyle::SC_SpinBoxUp, &spin);
    const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                     QStyle::SC_SpinBoxDown,
                                                     &spin);
    QCOMPARE(up.size(), QSize(36, spin.height()));
    QCOMPARE(down.size(), QSize(36, spin.height()));
    QCOMPARE(up.top(), down.top());
    QCOMPARE(up.right() + 1, down.left());
    QCOMPARE(edit.right() + 1, up.left());
    QVERIFY(spin.sizeHint().width() >= 120);
    QLineEdit lineEdit;
    QCOMPARE(spin.sizeHint().height(), qMax(32, lineEdit.sizeHint().height()));

    option.direction = Qt::RightToLeft;
    const QRect rtlEdit = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, &spin);
    const QRect rtlUp = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect rtlDown = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, &spin);
    QCOMPARE(rtlDown.right() + 1, rtlUp.left());
    QCOMPARE(rtlUp.right() + 1, rtlEdit.left());
    option.direction = Qt::LeftToRight;

    QImage focused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent)
            < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent)
            < 80);

    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), 47);
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, down.center());
    QCOMPARE(spin.value(), 46);
    spin.setValue(spin.maximum());
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), spin.maximum());
}

void WinUI3StyleTest::verticalNumberBoxContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(150, 32);
    WinUI3::Style::setVerticalSpinButtons(&spin);
    QVERIFY(WinUI3::Style::hasVerticalSpinButtons(&spin));
    QCOMPARE(spin.property(WinUI3::Style::VerticalSpinButtonsProperty).toBool(), true);
    spin.show();

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled
        | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
        | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const auto geometry = [&](QStyle::SubControl control) {
        return spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                            control, &spin);
    };
    const QRect edit = geometry(QStyle::SC_SpinBoxEditField);
    const QRect up = geometry(QStyle::SC_SpinBoxUp);
    const QRect down = geometry(QStyle::SC_SpinBoxDown);
    QCOMPARE(up.size(), QSize(32, 16));
    QCOMPARE(down.size(), QSize(32, 16));
    QCOMPARE(up.left(), down.left());
    QCOMPARE(up.bottom() + 1, down.top());
    QCOMPARE(edit.right() + 1, up.left());
    QVERIFY(!up.intersects(down));

    option.direction = Qt::RightToLeft;
    const QRect rtlEdit = geometry(QStyle::SC_SpinBoxEditField);
    const QRect rtlUp = geometry(QStyle::SC_SpinBoxUp);
    const QRect rtlDown = geometry(QStyle::SC_SpinBoxDown);
    QCOMPARE(rtlUp.left(), rtlDown.left());
    QCOMPARE(rtlUp.bottom() + 1, rtlDown.top());
    QCOMPARE(rtlUp.right() + 1, rtlEdit.left());
    option.direction = Qt::LeftToRight;

    QImage focused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent)
            < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent)
            < 80);

    const int separatorX = edit.right();
    const QColor separator = focused.pixelColor(separatorX, spin.rect().center().y());
    QVERIFY(separator != focused.pixelColor(separatorX + 1,
                                             spin.rect().center().y()));

    QImage rtlFocused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    rtlFocused.fill(Qt::transparent);
    option.direction = Qt::RightToLeft;
    {
        QPainter painter(&rtlFocused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QRect rtlEditForSeparator = geometry(QStyle::SC_SpinBoxEditField);
    const int rtlSeparatorX = rtlEditForSeparator.left();
    QVERIFY(rtlFocused.pixelColor(rtlSeparatorX, spin.rect().center().y())
            != rtlFocused.pixelColor(rtlSeparatorX + 1,
                                     spin.rect().center().y()));

    option.state &= ~QStyle::State_HasFocus;
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), 47);
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, down.center());
    QCOMPARE(spin.value(), 46);

    WinUI3::Style::setVerticalSpinButtons(&spin, false);
    QVERIFY(!WinUI3::Style::hasVerticalSpinButtons(&spin));
}

void WinUI3StyleTest::spinBoxFocusUnderlinePixelContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(160, 40);
    spin.show();

    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto verify = [&](bool vertical, Qt::LayoutDirection direction) {
        WinUI3::Style::setVerticalSpinButtons(&spin, vertical);
        spin.setLayoutDirection(direction);
        QCoreApplication::processEvents();

        QStyleOptionSpinBox option;
        option.initFrom(&spin);
        option.rect = spin.rect();
        option.direction = direction;
        option.frame = true;
        option.buttonSymbols = spin.buttonSymbols();
        option.stepEnabled = QAbstractSpinBox::StepUpEnabled
            | QAbstractSpinBox::StepDownEnabled;
        option.subControls = QStyle::SC_SpinBoxFrame
            | QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxUp
            | QStyle::SC_SpinBoxDown;
        option.state |= QStyle::State_HasFocus;

        QImage image(spin.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(spin.palette().color(QPalette::Window));
        {
            QPainter painter(&image);
            spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                             &painter, &spin);
        }

        const int underlineY = spin.rect().bottom() - 1;
        QVERIFY2(distance(image.pixelColor(spin.rect().center().x(), underlineY),
                          accent) < 80,
                 vertical ? "vertical underline center missing"
                          : "horizontal underline center missing");
        // The rounded clip follows the WinUI TextBox/NumberBox outline: the
        // line is present a few pixels in from each end, but never paints the
        // two rounded bottom corners.
        QVERIFY(distance(image.pixelColor(3, underlineY), accent) < 100);
        QVERIFY(distance(image.pixelColor(spin.width() - 4, underlineY), accent)
                < 100);
        QVERIFY(distance(image.pixelColor(spin.rect().left(), underlineY), accent)
                > 80);
        QVERIFY(distance(image.pixelColor(spin.rect().right(), underlineY), accent)
                > 80);
    };

    verify(false, Qt::LeftToRight);
    verify(false, Qt::RightToLeft);
    verify(true, Qt::LeftToRight);
    verify(true, Qt::RightToLeft);
}

void WinUI3StyleTest::checkboxAcceptAnimation()
{
    QCheckBox check(QStringLiteral("Animated accept"));
    check.resize(check.sizeHint());
    check.show();
    check.setChecked(true);
    QTest::qWait(55);
    const qreal midway = frameReal(&check, "_winui_check_progress");
    QVERIFY2(midway > 0.0 && midway < 1.0, qPrintable(QString::number(midway)));
    // After the initial generated hold, the accept stroke must still be in a
    // visibly partial state instead of completing in an imperceptible ~20 ms.
    QVERIFY2(midway < 0.90, qPrintable(QString::number(midway)));
    QTRY_VERIFY(frameReal(&check, "_winui_check_progress") > 0.99);
}

void WinUI3StyleTest::checkboxGlyphGeometryContract()
{
    QCheckBox check(QStringLiteral("Check"));
    check.resize(120, 32);
    check.show();
    setFrame(&check, "_winui_check_progress", 1.0);

    QStyleOptionButton option;
    option.initFrom(&check);
    option.rect = QRect(2, 6, 20, 20);
    option.state = QStyle::State_Enabled | QStyle::State_On;
    QImage image(24, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(check.palette().color(QPalette::Window));
    {
        QPainter painter(&image);
        check.style()->drawPrimitive(QStyle::PE_IndicatorCheckBox,
                                     &option, &painter, &check);
    }

    const QColor background = check.palette().color(QPalette::Window);
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const int distance = qAbs(pixel.red() - background.red())
                + qAbs(pixel.green() - background.green())
                + qAbs(pixel.blue() - background.blue());
            if (pixel.alpha() > 0 && distance > 12)
                bounds |= QRect(x, y, 1, 1);
        }
    }
    QCOMPARE(check.style()->pixelMetric(QStyle::PM_IndicatorWidth,
                                        nullptr, &check), 20);
    QCOMPARE(check.style()->pixelMetric(QStyle::PM_IndicatorHeight,
                                        nullptr, &check), 20);
    const int checkTextWidth = check.fontMetrics().size(
        Qt::TextShowMnemonic, check.text()).width();
    QVERIFY(check.sizeHint().width() >= checkTextWidth + 36);
    QRadioButton radio(QStringLiteral("Radio"));
    const int radioTextWidth = radio.fontMetrics().size(
        Qt::TextShowMnemonic, radio.text()).width();
    QVERIFY(radio.sizeHint().width() >= radioTextWidth + 36);
    QVERIFY(bounds.width() >= 19);
    QVERIFY(bounds.height() >= 19);
    QVERIFY(bounds.width() <= 21);
    QVERIFY(bounds.height() <= 21);
}

void WinUI3StyleTest::lightModeIndicatorOnAccentIsWhite()
{
    // AccentFillColorDefault is intentionally lighter than the raw system
    // accent in light mode. Its luminance must not make WinUI's control glyphs
    // switch to black: checked indicators and the toggle knob use white ink.
    const QColor white(Qt::white);
    const auto hasWhitePixel = [&white](const QImage &image, const QRect &rect) {
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                if (image.rect().contains(x, y)
                    && colorDistance(image.pixelColor(x, y), white) < 8)
                    return true;
            }
        }
        return false;
    };

    QCheckBox check;
    check.resize(32, 32);
    check.show();
    setFrame(&check, "_winui_check_progress", 1.0);
    QStyleOptionButton checkOption;
    checkOption.initFrom(&check);
    checkOption.rect = QRect(6, 6, 20, 20);
    checkOption.state = QStyle::State_Enabled | QStyle::State_On;
    QImage checkImage(check.size(), QImage::Format_ARGB32_Premultiplied);
    checkImage.fill(check.palette().color(QPalette::Window));
    {
        QPainter painter(&checkImage);
        check.style()->drawPrimitive(QStyle::PE_IndicatorCheckBox,
                                     &checkOption, &painter, &check);
    }
    QVERIFY2(hasWhitePixel(checkImage, checkOption.rect),
             "light checked checkbox has no white WinUI checkmark");

    QRadioButton radio;
    radio.resize(32, 32);
    radio.show();
    setFrame(&radio, "_winui_check_progress", 1.0);
    QStyleOptionButton radioOption;
    radioOption.initFrom(&radio);
    radioOption.rect = QRect(6, 6, 20, 20);
    radioOption.state = QStyle::State_Enabled | QStyle::State_On;
    QImage radioImage(radio.size(), QImage::Format_ARGB32_Premultiplied);
    radioImage.fill(radio.palette().color(QPalette::Window));
    {
        QPainter painter(&radioImage);
        radio.style()->drawPrimitive(QStyle::PE_IndicatorRadioButton,
                                     &radioOption, &painter, &radio);
    }
    QVERIFY2(hasWhitePixel(radioImage, radioOption.rect),
             "light checked radio has no white WinUI dot");

    WinUI3::Style::setToggleSwitch(&check);
    check.resize(64, 32);
    check.setChecked(true);
    setFrame(&check, "_winui_toggle_position", 1.0);
    QStyleOptionButton toggleOption;
    toggleOption.initFrom(&check);
    toggleOption.rect = check.rect();
    toggleOption.state = QStyle::State_Enabled | QStyle::State_On;
    QImage toggleImage(check.size(), QImage::Format_ARGB32_Premultiplied);
    toggleImage.fill(check.palette().color(QPalette::Window));
    {
        QPainter painter(&toggleImage);
        check.style()->drawControl(QStyle::CE_CheckBox, &toggleOption,
                                   &painter, &check);
    }
    const QRect toggleTrack(check.rect().left(), check.rect().center().y() - 10,
                            40, 20);
    const QRect toggleKnob(toggleTrack.right() - 16,
                           toggleTrack.center().y() - 8, 16, 16);
    QVERIFY2(hasWhitePixel(toggleImage, toggleKnob),
             "light checked toggle has no white WinUI on-knob");
}

void WinUI3StyleTest::darkModeIndicatorOnAccentIsBlack()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    const QPalette palette = style->standardPalette();
    const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);
    QCOMPARE(t.controlOnAccentPrimary, QColor(Qt::black));

    const auto hasBlackPixel = [](const QImage &image, const QRect &rect) {
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                if (image.rect().contains(x, y)
                    && colorDistance(image.pixelColor(x, y), QColor(Qt::black)) < 8)
                    return true;
            }
        }
        return false;
    };
    const auto renderIndicator = [&](QStyle::PrimitiveElement element,
                                     QAbstractButton &button) {
        button.resize(32, 32);
        setFrame(&button, "_winui_check_progress", 1.0);
        QStyleOptionButton option;
        option.initFrom(&button);
        option.palette = palette;
        option.rect = QRect(6, 6, 20, 20);
        option.state = QStyle::State_Enabled | QStyle::State_On;
        QImage image(button.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(palette.color(QPalette::Window));
        QPainter painter(&image);
        style->drawPrimitive(element, &option, &painter, &button);
        return qMakePair(image, option.rect);
    };

    QCheckBox check;
    const auto checkRender = renderIndicator(QStyle::PE_IndicatorCheckBox, check);
    QVERIFY2(hasBlackPixel(checkRender.first, checkRender.second),
             "dark checked checkbox has no black WinUI checkmark");
    QRadioButton radio;
    const auto radioRender = renderIndicator(QStyle::PE_IndicatorRadioButton, radio);
    QVERIFY2(hasBlackPixel(radioRender.first, radioRender.second),
             "dark checked radio has no black WinUI dot");

    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    toggle.resize(64, 32);
    toggle.setChecked(true);
    setFrame(&toggle, "_winui_toggle_position", 1.0);
    QStyleOptionButton option;
    option.initFrom(&toggle);
    option.palette = palette;
    option.rect = toggle.rect();
    option.state = QStyle::State_Enabled | QStyle::State_On;
    QImage image(toggle.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(palette.color(QPalette::Window));
    {
        QPainter painter(&image);
        style->drawControl(QStyle::CE_CheckBox, &option, &painter, &toggle);
    }
    const QRect track(toggle.rect().left(), toggle.rect().center().y() - 10,
                      40, 20);
    const QRect knob(track.right() - 16, track.center().y() - 8, 16, 16);
    QVERIFY2(hasBlackPixel(image, knob),
             "dark checked toggle has no black WinUI on-knob");
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3StyleTest::customAccentKeepsThemeTextSeparateFromControlInk()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QPalette palette = qApp->palette();
    const QColor paleAccent(255, 240, 0);
    palette.setColor(QPalette::Highlight, paleAccent);
    palette.setColor(QPalette::Accent, paleAccent);
    const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);

    // WinUI resolves both roles from the theme; in Light they are white.
    QCOMPARE(t.textOnAccentPrimary, QColor(Qt::white));
    QCOMPARE(t.controlOnAccentPrimary, QColor(Qt::white));
#endif
}

void WinUI3StyleTest::checkboxGapHitTest()
{
    QCheckBox check(QStringLiteral("A label with a real hit gap"));
    check.resize(check.sizeHint());
    check.show();
    QTRY_VERIFY(check.isVisible());

    QStyleOptionButton option;
    option.initFrom(&check);
    const QRect indicator = check.style()->subElementRect(
        QStyle::SE_CheckBoxIndicator, &option, &check);
    const QRect contents = check.style()->subElementRect(
        QStyle::SE_CheckBoxContents, &option, &check);
    const QRect clickRect = check.style()->subElementRect(
        QStyle::SE_CheckBoxClickRect, &option, &check);
    QVERIFY(indicator.right() + 1 < contents.left());
    const QPoint gap((indicator.right() + contents.left()) / 2,
                     indicator.center().y());
    QVERIFY(check.rect().contains(gap));
    QVERIFY(clickRect.contains(gap));

    QSignalSpy clicked(&check, &QAbstractButton::clicked);
    QTest::mouseClick(&check, Qt::LeftButton, Qt::NoModifier, gap);
    QCOMPARE(clicked.count(), 1);
    QVERIFY(check.isChecked());
}

void WinUI3StyleTest::checkboxDisabledStopsAnimation()
{
    QCheckBox check(QStringLiteral("Disabled during transition"));
    check.resize(180, 32);
    check.show();
    check.setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(
        frameReal(&check, "_winui_check_progress") > 0.0, 150);
    QVERIFY(frameReal(&check, "_winui_check_progress") < 1.0);

    check.setEnabled(false);
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 1.0);
    QTest::qWait(230);
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 1.0);
    QCOMPARE(frameReal(&check, "_winui_hover_progress"), 0.0);
    QCOMPARE(frameReal(&check, "_winui_press_progress"), 0.0);
}

void WinUI3StyleTest::radioStateMotion()
{
    QRadioButton radio(QStringLiteral("Animated radio"));
    radio.setChecked(true);
    radio.resize(radio.sizeHint());
    radio.show();
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&radio, &enter);
    QTest::qWait(90);
    const qreal hover = frameReal(&radio, "_winui_hover_progress");
    QVERIFY2(hover > 0.0 && hover < 1.0, qPrintable(QString::number(hover)));
    QTRY_VERIFY(frameReal(&radio, "_winui_hover_progress") > 0.99);
}

void WinUI3StyleTest::radioDotDpiGeometry()
{
    QRadioButton radio(QStringLiteral("Radio"));
    radio.resize(32, 32);
    radio.show();
    setFrame(&radio, "_winui_check_progress", 1.0);

    const qreal dpr = radio.devicePixelRatioF();
    const auto renderDot = [&](qreal hover, qreal press) {
        setFrame(&radio, "_winui_hover_progress", hover);
        setFrame(&radio, "_winui_press_progress", press);
        QStyleOptionButton option;
        option.initFrom(&radio);
        option.rect = QRect(0, 0, 20, 20);
        option.state = QStyle::State_Enabled | QStyle::State_On;
        if (hover > 0.5)
            option.state |= QStyle::State_MouseOver;
        if (press > 0.5)
            option.state |= QStyle::State_Sunken;

        QImage image(QSize(qRound(20 * dpr), qRound(20 * dpr)),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(radio.palette().color(QPalette::Window));
        QPainter painter(&image);
        radio.style()->drawPrimitive(QStyle::PE_IndicatorRadioButton,
                                     &option, &painter, &radio);

        const QColor dot = image.pixelColor(image.width() / 2,
                                            image.height() / 2);
        QRect bounds;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (colorDistance(image.pixelColor(x, y), dot) < 18)
                    bounds |= QRect(x, y, 1, 1);
            }
        }
        return bounds;
    };

    const int expectedRest = qRound(12 * dpr);
    const int expectedHover = qRound(14 * dpr);
    const int expectedPressed = qRound(10 * dpr);
    const QRect rest = renderDot(0.0, 0.0);
    const QRect hover = renderDot(1.0, 0.0);
    const QRect pressed = renderDot(1.0, 1.0);
    QVERIFY(qAbs(rest.width() - expectedRest) <= 2);
    QVERIFY(qAbs(rest.height() - expectedRest) <= 2);
    QVERIFY(qAbs(hover.width() - expectedHover) <= 2);
    QVERIFY(qAbs(hover.height() - expectedHover) <= 2);
    QVERIFY(qAbs(pressed.width() - expectedPressed) <= 2);
    QVERIFY(qAbs(pressed.height() - expectedPressed) <= 2);
}

void WinUI3StyleTest::menuSizingContract()
{
    QMenu menu;
    menu.setLayoutDirection(Qt::RightToLeft);
    QStyleOptionMenuItem option;
    option.initFrom(&menu);
    option.menuItemType = QStyleOptionMenuItem::Normal;
    option.font = menu.font();
    option.text = QStringLiteral("Open a recent project with a deliberately long name\tCtrl+Shift+O");
    const QFontMetrics metrics(option.font);
    const int expected = 42
        + metrics.horizontalAdvance(QStringLiteral("Open a recent project with a deliberately long name"))
        + 16 + 20 + metrics.horizontalAdvance(QStringLiteral("Ctrl+Shift+O"));
    const QSize result = menu.style()->sizeFromContents(QStyle::CT_MenuItem, &option,
                                                        QSize(), &menu);
    QVERIFY2(result.width() >= expected,
             qPrintable(QStringLiteral("%1 < %2").arg(result.width()).arg(expected)));
    QVERIFY(result.height() >= 36);

    const QString popupLabel = QStringLiteral("Open recent project");
    const int popupExpected = 42 + metrics.horizontalAdvance(popupLabel) + 16;
    auto *action = menu.addAction(popupLabel);
    action->setCheckable(true);
    PopupGeometryProbe probe;
    probe.popup = &menu;
    menu.installEventFilter(&probe);
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    const QRect firstGeometry = menu.geometry();
    QCOMPARE(firstGeometry, probe.geometryAtShow);
    QTest::qWait(60);
    QCOMPARE(menu.geometry(), firstGeometry);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    menu.hide();
    probe.reset();
    menu.popup(QPoint(80, 80));
    QTRY_VERIFY(menu.isVisible());
    QCOMPARE(menu.geometry(), firstGeometry);
    QCOMPARE(menu.geometry(), probe.geometryAtShow);
    QTest::qWait(60);
    QCOMPARE(menu.geometry(), firstGeometry);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    const QRect actionRect = menu.actionGeometry(action);
    QVERIFY(actionRect.width() >= popupExpected);
    QTest::mouseMove(&menu, actionRect.center());
    QCOMPARE(menu.activeAction(), action);
    QTest::mouseClick(&menu, Qt::LeftButton, Qt::NoModifier,
                      actionRect.center());
    QVERIFY(action->isChecked());
}

void WinUI3StyleTest::menuSubmenuChevronGeometry()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    constexpr int chevronSlotSize = 16;
    constexpr int chevronRightPadding = 9;
    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        QStyleOptionMenuItem option;
        option.rect = QRect(0, 0, 240, 36);
        option.direction = direction;
        option.palette = qApp->palette();
        option.state = QStyle::State_Enabled;
        option.menuItemType = QStyleOptionMenuItem::SubMenu;
        option.font = qApp->font();
        option.fontMetrics = QFontMetrics(option.font);

        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuItem, &option, &painter);
        }

        QRect ink;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 20)
                    ink |= QRect(x, y, 1, 1);
            }
        }
        QVERIFY2(!ink.isEmpty(), "submenu chevron produced no pixels");

        const QRect logicalSlot(
            option.rect.right() - chevronRightPadding - chevronSlotSize + 1,
            option.rect.center().y() - chevronSlotSize / 2,
            chevronSlotSize, chevronSlotSize);
        const QRect slot = QStyle::visualRect(direction, option.rect,
                                              logicalSlot);
        QVERIFY(slot.contains(ink.topLeft()));
        QVERIFY(slot.contains(ink.bottomRight()));
        // The WinUI template uses FontSize=12 in a 16px Viewbox. On the
        // reference font that produces an approximately 8px-tall visible
        // chevron, rather than the old 16px icon-engine paint (12px tall).
        QVERIFY(ink.width() <= 8);
        // Some font engines expose a one-pixel antialiasing fringe around the
        // 8px body. The old icon path was 12px tall, so 9 remains a strict
        // regression ceiling while keeping the DPI/font test portable.
        QVERIFY(ink.height() <= 9);
        QVERIFY(ink.width() >= 4);
        QVERIFY(ink.height() >= 6);
        QCOMPARE(slot.size(), QSize(chevronSlotSize, chevronSlotSize));
        QVERIFY(qAbs(slot.center().y() - option.rect.center().y()) <= 1);
    }
}

void WinUI3StyleTest::menuPaintTabParsing()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    auto render = [style](const QString &text) {
        QStyleOptionMenuItem option;
        option.rect = QRect(0, 0, 320, 36);
        option.direction = Qt::LeftToRight;
        option.palette = qApp->palette();
        option.state = QStyle::State_Enabled;
        option.menuItemType = QStyleOptionMenuItem::Normal;
        option.font = qApp->font();
        option.fontMetrics = QFontMetrics(option.font);
        option.text = text;

        QImage result(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        style->drawControl(QStyle::CE_MenuItem, &option, &painter);
        return result;
    };

    // QString::split() historically rendered only fields 0 and 1. Keep a
    // third tabbed field from changing pixels while replacing that allocation
    // heavy parsing in the paint path.
    const QImage twoFields = render(QStringLiteral("Open\tCtrl+O"));
    const QImage extraField = render(QStringLiteral("Open\tCtrl+O\tignored"));
    QVERIFY(twoFields == extraField);
}

void WinUI3StyleTest::menuBarOnlyActiveActionIsHighlighted()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QMenuBar bar;
    bar.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("layer"));
    QAction *file = bar.addAction(QStringLiteral("File"));
    QAction *view = bar.addAction(QStringLiteral("View"));
    bar.resize(180, 32);
    bar.show();
    (void)QTest::qWaitForWindowExposed(&bar);
    setFrame(&bar, "_winui_hover_progress", 1.0);

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        bar.setLayoutDirection(direction);
        const QRect fileRect = bar.actionGeometry(file);
        const QRect viewRect = bar.actionGeometry(view);
        QVERIFY(!fileRect.intersects(viewRect));

        QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
        const QColor surface = bar.palette().color(QPalette::Window);
        image.fill(Qt::black);
        QStyleOption emptyArea;
        emptyArea.initFrom(&bar);
        emptyArea.rect = image.rect();
        {
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuBarEmptyArea, &emptyArea,
                               &painter, &bar);
        }
        QCOMPARE(image.pixelColor(image.width() - 2, image.height() / 2), surface);
        auto drawItem = [&](QAction *action, const QRect &rect,
                            QStyle::State state) {
            QStyleOptionMenuItem option;
            option.initFrom(&bar);
            option.rect = rect;
            option.state = state;
            option.text = action->text();
            option.font = bar.font();
            option.fontMetrics = QFontMetrics(option.font);
            option.menuItemType = QStyleOptionMenuItem::Normal;
            QPainter painter(&image);
            style->drawControl(QStyle::CE_MenuBarItem, &option,
                               &painter, &bar);
        };
        drawItem(file, fileRect, QStyle::State_Enabled | QStyle::State_Selected);
        drawItem(view, viewRect, QStyle::State_Enabled);

        const QPoint fileSample(fileRect.left() + 4, fileRect.top() + 4);
        const QPoint viewSample(viewRect.left() + 4, viewRect.top() + 4);
        QVERIFY(image.pixelColor(fileSample) != surface);
        QCOMPARE(image.pixelColor(viewSample), surface);
        drawItem(file, fileRect, QStyle::State_Enabled);
        QCOMPARE(image.pixelColor(fileSample), surface);
    }

    QListWidget navigation;
    navigation.setProperty(WinUI3::Style::NavigationViewProperty, true);
    navigation.setProperty(WinUI3::Style::SurfaceProperty,
                           QStringLiteral("layer"));
    navigation.addItem(QStringLiteral("Controls"));
    navigation.resize(220, 120);
    navigation.show();
    QTRY_VERIFY(navigation.isVisible());
    const QColor navigationWindow = navigation.palette().color(QPalette::Window);
    QCOMPARE(navigation.palette().color(QPalette::Base), navigationWindow);
    QCOMPARE(navigationWindow.alpha(), 255);
}

void WinUI3StyleTest::groupBoxContract()
{
    QGroupBox group(QStringLiteral("Enable diagnostics"));
    group.setCheckable(true);
    group.setChecked(false);
    group.resize(320, 120);
    group.show();

    QStyleOptionGroupBox option;
    option.initFrom(&group);
    option.text = group.title();
    option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel
        | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxContents;
    const QRect indicator = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                          QStyle::SC_GroupBoxCheckBox,
                                                          &group);
    const QRect label = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                      QStyle::SC_GroupBoxLabel,
                                                      &group);
    const QRect contents = group.style()->subControlRect(QStyle::CC_GroupBox, &option,
                                                         QStyle::SC_GroupBoxContents,
                                                         &group);
    QCOMPARE(indicator.size(), QSize(20, 20));
    QVERIFY(contents.top() > indicator.bottom());

    const QPoint gap((indicator.right() + label.left()) / 2,
                     indicator.center().y());
    QVERIFY(!indicator.contains(gap));
    QVERIFY(!label.contains(gap));
    QCOMPARE(group.style()->hitTestComplexControl(QStyle::CC_GroupBox, &option,
                                                   gap, &group),
             QStyle::SC_GroupBoxCheckBox);
    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier, gap);
    QVERIFY(group.isChecked());
    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier, gap);
    QVERIFY(!group.isChecked());

    group.setChecked(true);
    QTest::qWait(55);
    const qreal midway = frameReal(&group, "_winui_check_progress");
    QVERIFY(midway > 0.0 && midway < 1.0);
    QTRY_VERIFY(frameReal(&group, "_winui_check_progress") > 0.99);

    QTest::mouseClick(&group, Qt::LeftButton, Qt::NoModifier,
                      indicator.center());
    QVERIFY(!group.isChecked());
    QCOMPARE(frameReal(&group, "_winui_check_progress"), 0.0);

    group.setLayoutDirection(Qt::RightToLeft);
    option.initFrom(&group);
    option.text = group.title();
    option.subControls = QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxLabel
        | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxContents;
    const QRect rtlIndicator = group.style()->subControlRect(
        QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxCheckBox, &group);
    const QRect rtlLabel = group.style()->subControlRect(
        QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxLabel, &group);
    const QPoint rtlGap((rtlLabel.right() + rtlIndicator.left()) / 2,
                        rtlIndicator.center().y());
    QCOMPARE(group.style()->hitTestComplexControl(QStyle::CC_GroupBox, &option,
                                                   rtlGap, &group),
             QStyle::SC_GroupBoxCheckBox);
}

void WinUI3StyleTest::splitterHandleContract()
{
    ExposedSplitter splitter(Qt::Horizontal);
    splitter.addWidget(new QLabel(QStringLiteral("Left")));
    splitter.addWidget(new QLabel(QStringLiteral("Right")));
    splitter.resize(420, 120);
    splitter.show();
    auto *handle = splitter.handle(1);
    QVERIFY(handle);
    QCOMPARE(handle->width(), 6);

    // The offscreen platform may report the synthetic cursor over a newly
    // shown handle. Establish a known rest state before testing the explicit
    // Enter transition; otherwise the controller can legitimately ignore a
    // duplicate Enter and leave the frame registry at its initial value.
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(handle, &leave);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") < 0.01);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(handle, &enter);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") > 0.0);
    QTRY_VERIFY(frameReal(handle, "_winui_hover_progress") > 0.99);

    const QList<int> beforeDrag = splitter.sizes();
    const QPoint handleCenter = handle->rect().center();
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, handleCenter);
    QTest::mouseMove(handle, handleCenter + QPoint(60, 0), 20);
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier,
                        handleCenter + QPoint(60, 0));
    QVERIFY(splitter.sizes().at(0) > beforeDrag.at(0));
    splitter.moveSplitter(-1000, 1);
    QVERIFY(splitter.sizes().at(0) >= 0);
    splitter.moveSplitter(splitter.width() + 1000, 1);
    QVERIFY(splitter.sizes().at(1) >= 0);

    ExposedSplitter vertical(Qt::Vertical);
    vertical.addWidget(new QLabel(QStringLiteral("Top")));
    vertical.addWidget(new QLabel(QStringLiteral("Bottom")));
    vertical.resize(180, 300);
    vertical.show();
    auto *verticalHandle = vertical.handle(1);
    QVERIFY(verticalHandle);
    QCOMPARE(verticalHandle->height(), 6);
    const QList<int> beforeVerticalDrag = vertical.sizes();
    const QPoint verticalCenter = verticalHandle->rect().center();
    QTest::mousePress(verticalHandle, Qt::LeftButton, Qt::NoModifier, verticalCenter);
    QTest::mouseMove(verticalHandle, verticalCenter + QPoint(0, 45), 20);
    QTest::mouseRelease(verticalHandle, Qt::LeftButton, Qt::NoModifier,
                        verticalCenter + QPoint(0, 45));
    QVERIFY(vertical.sizes().at(0) > beforeVerticalDrag.at(0));
    vertical.moveSplitter(-1000, 1);
    QVERIFY(vertical.sizes().at(0) >= 0);
    vertical.moveSplitter(vertical.height() + 1000, 1);
    QVERIFY(vertical.sizes().at(1) >= 0);
}

void WinUI3StyleTest::splitterGripPixelAlignment()
{
    QSplitter splitter(Qt::Horizontal);
    splitter.addWidget(new QLabel(QStringLiteral("Left")));
    splitter.addWidget(new QLabel(QStringLiteral("Right")));

    const auto gripCenter = [&splitter](const QRect &rect, bool horizontal,
                                        qreal dpr) {
        constexpr int logicalWidth = 180;
        constexpr int logicalHeight = 140;
        QImage image(qCeil(logicalWidth * dpr), qCeil(logicalHeight * dpr),
                     QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
        QStyleOption option;
        option.initFrom(&splitter);
        option.rect = rect;
        option.state = QStyle::State_Enabled;
        if (horizontal)
            option.state |= QStyle::State_Horizontal;
        {
            QPainter painter(&image);
            splitter.style()->drawControl(QStyle::CE_Splitter, &option,
                                          &painter, splitter.handle(1));
        }
        qreal weighted = 0.0;
        qreal weight = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const qreal alpha = qAlpha(image.pixel(x, y));
                weighted += alpha * (horizontal ? x + 0.5 : y + 0.5);
                weight += alpha;
            }
        }
        const qreal actual = weight > 0.0 ? weighted / weight : -1.0;
        const qreal logicalCenter = horizontal
            ? QRectF(rect).center().x() : QRectF(rect).center().y();
        const qreal expected = qRound(logicalCenter * dpr - 0.5) + 0.5;
        return qMakePair(actual, expected);
    };

    for (const qreal dpr : {1.0, 1.25, 1.5, 2.0}) {
        for (const bool horizontal : {true, false}) {
            const QRect even = horizontal ? QRect(20, 20, 6, 100)
                                          : QRect(20, 20, 100, 6);
            const QRect odd = horizontal ? QRect(21, 21, 7, 99)
                                         : QRect(21, 21, 99, 7);
            const auto evenCenter = gripCenter(even, horizontal, dpr);
            const auto oddCenter = gripCenter(odd, horizontal, dpr);
            QVERIFY2(evenCenter.first >= 0.0,
                     qPrintable(QStringLiteral("DPR %1 %2 even empty")
                                    .arg(dpr).arg(horizontal ? "H" : "V")));
            QVERIFY2(oddCenter.first >= 0.0,
                     qPrintable(QStringLiteral("DPR %1 %2 odd empty")
                                    .arg(dpr).arg(horizontal ? "H" : "V")));
            QVERIFY2(qAbs(evenCenter.first - evenCenter.second) < 0.75,
                     qPrintable(QStringLiteral("DPR %1 %2 even=%3 expected=%4")
                                    .arg(dpr).arg(horizontal ? "H" : "V")
                                    .arg(evenCenter.first).arg(evenCenter.second)));
            QVERIFY2(qAbs(oddCenter.first - oddCenter.second) < 0.75,
                     qPrintable(QStringLiteral("DPR %1 %2 odd=%3 expected=%4")
                                    .arg(dpr).arg(horizontal ? "H" : "V")
                                    .arg(oddCenter.first).arg(oddCenter.second)));
        }
    }
}

void WinUI3StyleTest::dockWidgetContract()
{
    QMainWindow host;
    auto *dock = new QDockWidget(QStringLiteral("Inspector"), &host);
    dock->setWidget(new QLabel(QStringLiteral("Dock content")));
    host.addDockWidget(Qt::RightDockWidgetArea, dock);
    host.resize(520, 260);
    host.show();

    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent,
                                       nullptr, dock), 6);
    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth,
                                       nullptr, dock), 1);
    QCOMPARE(host.style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin,
                                       nullptr, dock), 8);
    const QImage image = host.grab().toImage();
    QVERIFY(!image.isNull());
    QCOMPARE(image.size(), host.size());
}

void WinUI3StyleTest::sliderGeometryContract()
{
    const auto verifyEndPoints = [](QSlider &slider) {
        QStyleOptionSlider option;
        slider.setValue(slider.minimum());
        option.initFrom(&slider);
        option.orientation = slider.orientation();
        option.minimum = slider.minimum();
        option.maximum = slider.maximum();
        option.sliderPosition = slider.sliderPosition();
        option.sliderValue = slider.value();
        option.upsideDown = slider.orientation() == Qt::Horizontal
            ? (slider.invertedAppearance()
               != (slider.layoutDirection() == Qt::RightToLeft))
            : !slider.invertedAppearance();
        const QRect groove = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, &slider);
        const QRect minimumHandle = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
        QCOMPARE(minimumHandle.size(), QSize(18, 18));
        QCOMPARE(slider.orientation() == Qt::Horizontal ? groove.height()
                                                       : groove.width(), 4);
        const int minimumCenter = slider.orientation() == Qt::Horizontal
            ? minimumHandle.center().x() : minimumHandle.center().y();
        const int minimumEnd = slider.orientation() == Qt::Horizontal
            ? (option.upsideDown ? groove.right() : groove.left())
            : (option.upsideDown ? groove.bottom() : groove.top());
        QCOMPARE(minimumCenter, minimumEnd);

        slider.setValue(slider.maximum());
        option.sliderPosition = slider.sliderPosition();
        option.sliderValue = slider.value();
        const QRect maximumHandle = slider.style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
        const int maximumCenter = slider.orientation() == Qt::Horizontal
            ? maximumHandle.center().x() : maximumHandle.center().y();
        const int maximumEnd = slider.orientation() == Qt::Horizontal
            ? (option.upsideDown ? groove.left() : groove.right())
            : (option.upsideDown ? groove.top() : groove.bottom());
        QCOMPARE(maximumCenter, maximumEnd);
    };

    QSlider horizontal(Qt::Horizontal);
    horizontal.setRange(0, 100);
    horizontal.resize(320, 32);
    verifyEndPoints(horizontal);
    horizontal.setLayoutDirection(Qt::RightToLeft);
    verifyEndPoints(horizontal);
    horizontal.setInvertedAppearance(true);
    verifyEndPoints(horizontal);

    QSlider vertical(Qt::Vertical);
    vertical.setRange(0, 100);
    vertical.resize(32, 320);
    verifyEndPoints(vertical);
    vertical.setInvertedAppearance(true);
    verifyEndPoints(vertical);
}

void WinUI3StyleTest::sliderStateMotion()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(40);
    slider.resize(320, 32);
    slider.show();

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&slider, &leave);
    QTRY_VERIFY(frameReal(&slider, "_winui_hover_progress") < 0.01);
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&slider, &enter);
    QTest::qWait(100);
    const qreal hoverMidway = frameReal(&slider, "_winui_hover_progress");
    QVERIFY(hoverMidway > 0.0 && hoverMidway < 1.0);
    QTRY_VERIFY(frameReal(&slider, "_winui_hover_progress") > 0.99);

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTest::qWait(100);
    const qreal pressMidway = frameReal(&slider, "_winui_press_progress");
    QVERIFY(pressMidway > 0.0 && pressMidway < 1.0);
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTRY_VERIFY(frameReal(&slider, "_winui_press_progress") < 0.01);
}

void WinUI3StyleTest::sliderDragInteraction()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(20);
    slider.resize(320, 40);
    slider.show();

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    const QRect handle = slider.style()->subControlRect(QStyle::CC_Slider, &option,
                                                        QStyle::SC_SliderHandle,
                                                        &slider);
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QMouseEvent move(QEvent::MouseMove, QPointF(handle.center() + QPoint(120, 0)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&slider, &move);
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                        handle.center() + QPoint(120, 0));
    QVERIFY(slider.value() > 20);
}

void WinUI3StyleTest::sliderValueToolTipAndFocus()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(42);
    slider.resize(320, 40);
    slider.move(300, 300);
    slider.show();

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);

    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, handle.center());
    QTRY_VERIFY(frameBool(&slider, "_winui_slider_tooltip_visible"));
    QCOMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
             QStringLiteral("42"));
    if (QGuiApplication::platformName() != QStringLiteral("offscreen")) {
        QTRY_VERIFY(slider.findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"))->isVisible());
    }
    QMouseEvent move(QEvent::MouseMove, QPointF(handle.center() + QPoint(90, 0)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&slider, &move);
    QTRY_VERIFY(slider.value() > 42);
    QTRY_COMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
                 QString::number(slider.value()));
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                        handle.center() + QPoint(90, 0));
    QTRY_VERIFY(!frameBool(&slider, "_winui_slider_tooltip_visible"));
    if (auto *tip = slider.findChild<QWidget *>(
            QStringLiteral("_winui_slider_value_tip"))) {
        QTRY_VERIFY(!tip->isVisible());
    }

    const auto render = [&slider](QStyleOptionSlider renderOption) {
        QImage image(renderOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        slider.style()->drawComplexControl(QStyle::CC_Slider, &renderOption,
                                            &painter, &slider);
        return image;
    };
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.rect = slider.rect();

    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&slider, &mouseFocus);
    option.state |= QStyle::State_HasFocus;
    const QImage mouseFocused = render(option);
    QVERIFY(!frameBool(&slider, "_winui_focus_visible"));

    QFocusEvent tabFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&slider, &tabFocus);
    QVERIFY(frameBool(&slider, "_winui_focus_visible"));
    const QImage keyboardFocused = render(option);
    QVERIFY(mouseFocused != keyboardFocused);

    slider.clearFocus();
    slider.setEnabled(false);
    const int disabledValue = slider.value();
    QTest::mouseClick(&slider, Qt::LeftButton, Qt::NoModifier,
                      slider.rect().center() + QPoint(80, 0));
    QCOMPARE(slider.value(), disabledValue);
    option.initFrom(&slider);
    option.orientation = slider.orientation();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.rect = slider.rect();
    const QImage disabled = render(option);
    QVERIFY(disabled != mouseFocused);
}

void WinUI3StyleTest::scrollBarContract()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(12, 300);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.height(), 12);
    QCOMPARE(increase.height(), 12);
    QCOMPARE(thumb.width(), 12);
    QVERIFY(thumb.height() >= 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleY = thumb.center().y();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(2, sampleY), background)
            < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(4, sampleY), background)
            > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(11, sampleY), background)
            > 10);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    const int beforeArrow = bar.value();
    QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect movedThumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    const int beforeDrag = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, movedThumb.center());
    QMouseEvent move(QEvent::MouseMove,
                     QPointF(movedThumb.center() + QPoint(0, 45)),
                     Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&bar, &move);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier,
                        movedThumb.center() + QPoint(0, 45));
    QVERIFY(bar.value() > beforeDrag);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    const QImage collapsed = bar.grab().toImage();
    QCOMPARE(collapsed.pixelColor(0, collapsed.height() / 2),
             bar.palette().color(QPalette::Window));
}

void WinUI3StyleTest::scrollBarHorizontalAndReentry()
{
    DisableAnimationsGuard animations;
    QScrollBar bar(Qt::Horizontal);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(30);
    bar.resize(300, 12);
    bar.show();

    QStyleOptionSlider option;
    option.initFrom(&bar);
    option.orientation = bar.orientation();
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    option.pageStep = bar.pageStep();
    option.upsideDown = false;
    const QRect decrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect increase = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    const QRect thumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    QCOMPARE(decrease.width(), 12);
    QCOMPARE(increase.width(), 12);
    QCOMPARE(thumb.height(), 12);
    QVERIFY(thumb.width() >= 30);
    QCOMPARE(bar.style()->pixelMetric(QStyle::PM_ScrollBarSliderMin,
                                       &option, &bar), 30);

    const auto colorDistance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    setFrame(&bar, "_winui_hover_progress", 0.0);
    QImage collapsedGeometry = bar.grab().toImage();
    const QColor background = bar.palette().color(QPalette::Window);
    const int sampleX = thumb.center().x();
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 2), background)
            < 5);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 4), background)
            > 10);
    QVERIFY(colorDistance(collapsedGeometry.pixelColor(sampleX, 11), background)
            > 10);

    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    const int beforeArrow = bar.value();
    QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    const qreal pressed = frameReal(&bar, "_winui_press_progress");
    QVERIFY(pressed > 0.0);
    QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, increase.center());
    QVERIFY(bar.value() > beforeArrow);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&bar, &leave);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 0.0);
    QCoreApplication::sendEvent(&bar, &enter);
    QCOMPARE(frameReal(&bar, "_winui_hover_progress"), 1.0);

    bar.setLayoutDirection(Qt::RightToLeft);
    option.direction = Qt::RightToLeft;
    option.upsideDown = true;
    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect rtlDecrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine, &bar);
    const QRect rtlIncrease = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine, &bar);
    QVERIFY(rtlDecrease.left() > rtlIncrease.left());

    option.sliderPosition = bar.sliderPosition();
    option.sliderValue = bar.value();
    const QRect currentThumb = bar.style()->subControlRect(
        QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, &bar);
    bar.setEnabled(false);
    const QImage disabled = bar.grab().toImage();
    QCOMPARE(disabled.pixelColor(currentThumb.center()),
             bar.palette().color(QPalette::Window));
}

void WinUI3StyleTest::scrollAreaScrollBarIntegration()
{
    QScrollArea area;
    auto *content = new QWidget;
    content->resize(760, 620);
    area.setWidget(content);
    area.resize(260, 180);
    area.show();

    QScrollBar *vertical = area.verticalScrollBar();
    QScrollBar *horizontal = area.horizontalScrollBar();
    QTRY_VERIFY(vertical->isVisible());
    QTRY_VERIFY(horizontal->isVisible());
    QCOMPARE(vertical->width(), 12);
    QCOMPARE(horizontal->height(), 12);
    QVERIFY(vertical->maximum() > 0);
    QVERIFY(horizontal->maximum() > 0);

    vertical->setFocus(Qt::TabFocusReason);
    const int keyboardBefore = vertical->value();
    QTest::keyClick(vertical, Qt::Key_Down);
    QVERIFY(vertical->value() > keyboardBefore);

    const int verticalBefore = vertical->value();
    QTest::mouseClick(vertical, Qt::LeftButton, Qt::NoModifier,
                      QPoint(vertical->width() / 2, vertical->height() - 6));
    QVERIFY(vertical->value() > verticalBefore);
    const int horizontalBefore = horizontal->value();
    QTest::mouseClick(horizontal, Qt::LeftButton, Qt::NoModifier,
                      QPoint(horizontal->width() - 6, horizontal->height() / 2));
    QVERIFY(horizontal->value() > horizontalBefore);
}

void WinUI3StyleTest::tabViewContract()
{
    QTabWidget tabs;
    tabs.setLayoutDirection(Qt::RightToLeft);
    tabs.setTabsClosable(true);
    tabs.addTab(new QWidget, QStringLiteral("First"));
    tabs.addTab(new QWidget, QStringLiteral("Second"));
    tabs.resize(420, 220);
    tabs.show();

    QTabBar *bar = tabs.tabBar();
    QVERIFY(bar);
    QCOMPARE(bar->tabRect(0).height(), 32);
    QVERIFY(bar->tabRect(0).width() >= 100);
    QCOMPARE(tabs.style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth,
                                       nullptr, bar), 32);
    QCOMPARE(tabs.style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight,
                                       nullptr, bar), 24);
    QVERIFY(!tabs.style()->standardIcon(QStyle::SP_TabCloseButton).isNull());

    QStyleOptionTab selected;
    selected.rect = QRect(0, 0, 120, 32);
    selected.shape = QTabBar::RoundedNorth;
    selected.palette = tabs.palette();
    selected.state = QStyle::State_Enabled | QStyle::State_Selected;
    QImage image(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(tabs.palette().color(QPalette::Window));
    {
        QPainter painter(&image);
        tabs.style()->drawControl(QStyle::CE_TabBarTabShape, &selected,
                                  &painter, bar);
    }
    const QColor accent = tabs.palette().color(QPalette::Highlight);
    QVERIFY(image.pixelColor(selected.rect.center().x(),
                             selected.rect.bottom() - 2) != accent);
    QVERIFY(image.pixelColor(selected.rect.center().x(), 5) != accent);
}

void WinUI3StyleTest::listViewContract()
{
    QListWidget list;
    list.addItems({QStringLiteral("Documents"), QStringLiteral("Pictures")});
    list.setCurrentRow(0);
    list.resize(320, 160);
    list.show();
    QTRY_VERIFY(list.isVisible());
    QCOMPARE(list.sizeHintForRow(0), 40);

    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 300, 40);
    option.index = list.model()->index(0, 0);
    option.text = QStringLiteral("Documents");
    option.features = QStyleOptionViewItem::HasDisplay;
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    QImage selected(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    selected.fill(list.palette().color(QPalette::Base));
    {
        QPainter painter(&selected);
        list.style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                  &painter, list.viewport());
    }
    const QColor accent = list.palette().color(QPalette::Highlight);
    bool indicatorFound = false;
    for (int y = 8; y < 32; ++y) {
        const QColor pixel = selected.pixelColor(2, y);
        if (qAbs(pixel.red() - accent.red()) < 12
            && qAbs(pixel.green() - accent.green()) < 12
            && qAbs(pixel.blue() - accent.blue()) < 12) {
            indicatorFound = true;
            break;
        }
    }
    QVERIFY(indicatorFound);
}

void WinUI3StyleTest::itemViewGutterContract()
{
    QListWidget list;
    auto *item = new QListWidgetItem(QStringLiteral("Checked item"), &list);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);
    list.resize(320, 80);
    list.show();
    QTRY_VERIFY(list.isVisible());

    QStyleOptionViewItem option;
    option.initFrom(list.viewport());
    option.widget = list.viewport();
    option.rect = QRect(0, 0, 300, 40);
    option.index = list.model()->index(0, 0);
    option.features = QStyleOptionViewItem::HasDisplay
        | QStyleOptionViewItem::HasDecoration
        | QStyleOptionViewItem::HasCheckIndicator;
    option.decorationSize = QSize(16, 16);
    option.direction = Qt::LeftToRight;
    const QRect listCheck = list.style()->subElementRect(
        QStyle::SE_ItemViewItemCheckIndicator, &option, list.viewport());
    const QRect listDecoration = list.style()->subElementRect(
        QStyle::SE_ItemViewItemDecoration, &option, list.viewport());
    const QRect listText = list.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &option, list.viewport());
    QVERIFY(listCheck.left() >= 12);
    QVERIFY(listDecoration.left() >= 12);
    QVERIFY(listText.left() >= 12);
    QVERIFY(listCheck.right() < listText.left());

    option.direction = Qt::RightToLeft;
    const QRect rtlText = list.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &option, list.viewport());
    QVERIFY(rtlText.right() <= option.rect.right() - 12);

    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Root")});
    auto *child = new QTreeWidgetItem(root, {QStringLiteral("Child")});
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    auto treeOption = option;
    treeOption.widget = tree.viewport();
    treeOption.direction = Qt::LeftToRight;
    treeOption.index = tree.indexFromItem(root);
    const QRect rootText = tree.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &treeOption, tree.viewport());
    treeOption.index = tree.indexFromItem(child);
    const QRect childText = tree.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &treeOption, tree.viewport());
    QCOMPARE(childText.left() - rootText.left(), tree.indentation());

    QTableWidget table(1, 1);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());
    auto tableOption = option;
    tableOption.widget = table.viewport();
    tableOption.index = table.model()->index(0, 0);
    tableOption.direction = Qt::LeftToRight;
    const QRect tableText = table.style()->subElementRect(
        QStyle::SE_ItemViewItemText, &tableOption, table.viewport());
    QVERIFY(tableText.left() < listText.left());
}

void WinUI3StyleTest::treeViewContract()
{
    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Workspace")});
    new QTreeWidgetItem(root, {QStringLiteral("src")});
    tree.expandAll();
    tree.resize(320, 180);
    tree.show();
    QTRY_VERIFY(tree.isVisible());
    QCOMPARE(tree.sizeHintForIndex(tree.indexFromItem(root)).height(), 28);

    QStyleOption branch;
    branch.initFrom(tree.viewport());
    branch.rect = QRect(0, 0, 20, 28);
    branch.state = QStyle::State_Enabled | QStyle::State_Children
        | QStyle::State_Open;
    QImage image(branch.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        tree.style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branch,
                                    &painter, tree.viewport());
    }
    bool glyphFound = false;
    for (int y = 0; y < image.height() && !glyphFound; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 40) {
                glyphFound = true;
                break;
            }
        }
    }
    QVERIFY(glyphFound);
}

void WinUI3StyleTest::treeSelectionMarkerLeadingEdge()
{
    QTreeWidget tree;
    tree.setHeaderHidden(true);
    auto *root = new QTreeWidgetItem(&tree, {QStringLiteral("Root")});
    auto *child = new QTreeWidgetItem(root, {QStringLiteral("Indented child")});
    tree.expandAll();
    tree.resize(320, 120);
    tree.show();
    QTRY_VERIFY(tree.isVisible());

    const auto renderSelected = [&](Qt::LayoutDirection direction) {
        tree.setLayoutDirection(direction);
        QStyleOptionViewItem option;
        option.initFrom(tree.viewport());
        option.widget = tree.viewport();
        option.direction = direction;
        option.rect = tree.visualRect(tree.indexFromItem(child));
        option.index = tree.indexFromItem(child);
        option.text = child->text(0);
        option.features = QStyleOptionViewItem::HasDisplay;
        option.state = QStyle::State_Enabled | QStyle::State_Selected;

        QImage image(tree.viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(tree.palette().color(QPalette::Base));
        QPainter painter(&image);
        tree.style()->drawControl(QStyle::CE_ItemViewItem, &option,
                                  &painter, tree.viewport());
        return image;
    };

    const QColor accent = tree.palette().color(QPalette::Highlight);
    const auto hasAccentNear = [&](const QImage &image, int left) {
        const int y = tree.visualRect(tree.indexFromItem(child)).center().y();
        for (int x = left; x < left + 8; ++x)
            if (image.rect().contains(x, y)
                && colorDistance(image.pixelColor(x, y), accent) < 18)
                return true;
        return false;
    };
    const QImage ltr = renderSelected(Qt::LeftToRight);
    QVERIFY(hasAccentNear(ltr, 0));
    const QImage rtl = renderSelected(Qt::RightToLeft);
    const int rightLeading = rtl.width() - 8;
    QVERIFY(hasAccentNear(rtl, rightLeading));
}

void WinUI3StyleTest::tableHeaderContract()
{
    QTableWidget table(2, 2);
    table.setLayoutDirection(Qt::RightToLeft);
    table.setHorizontalHeaderLabels({QStringLiteral("Control"),
                                     QStringLiteral("State")});
    table.setSortingEnabled(true);
    table.resize(420, 180);
    table.show();
    QTRY_VERIFY(table.isVisible());
    QVERIFY(table.horizontalHeader()->height() >= 32);
    QVERIFY(table.verticalHeader()->sectionSize(0) >= 36);

    QStyleOptionHeader option;
    option.initFrom(table.horizontalHeader());
    option.rect = QRect(0, 0, 180, 32);
    option.text = QStringLiteral("Control");
    option.textAlignment = Qt::AlignLeft;
    option.sortIndicator = QStyleOptionHeader::SortDown;
    QImage header(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    header.fill(Qt::transparent);
    {
        QPainter painter(&header);
        table.style()->drawControl(QStyle::CE_HeaderSection, &option,
                                   &painter, table.horizontalHeader());
        table.style()->drawControl(QStyle::CE_HeaderLabel, &option,
                                   &painter, table.horizontalHeader());
    }
    QVERIFY(!header.isNull());
    QVERIFY(header.pixelColor(option.rect.right() - 16,
                              option.rect.center().y()).alpha() > 0);
}

void WinUI3StyleTest::tableSortIndicatorGeometryContract()
{
    QTableWidget table(1, 1);
    table.resize(240, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const auto render = [&table](Qt::LayoutDirection direction, bool sorted,
                                 int height) {
        QStyleOptionHeader option;
        option.initFrom(table.horizontalHeader());
        option.rect = QRect(0, 0, 180, height);
        option.direction = direction;
        option.text.clear();
        option.textAlignment = Qt::AlignLeft;
        option.sortIndicator = sorted ? QStyleOptionHeader::SortDown
                                      : QStyleOptionHeader::None;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_Header, &option, &painter,
                                   table.horizontalHeader());
        return image;
    };

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                 Qt::RightToLeft}) {
        const QImage sorted = render(direction, true, 33);
        const QImage unsorted = render(direction, false, 33);
        QRect difference;
        for (int y = 0; y < sorted.height(); ++y) {
            for (int x = 0; x < sorted.width(); ++x) {
                if (sorted.pixel(x, y) != unsorted.pixel(x, y))
                    difference |= QRect(x, y, 1, 1);
            }
        }
        QVERIFY(!difference.isEmpty());
        // The chevron ink does not fill its 16 px slot; verify that the ink
        // remains inside the vertically centered slot for an odd header.
        QVERIFY(difference.top() >= 8);
        QVERIFY(difference.bottom() <= 24);
        if (direction == Qt::LeftToRight)
            QVERIFY(difference.left() > 130);
        else
            QVERIFY(difference.right() < 40);
    }
}

void WinUI3StyleTest::tableEditingPaintContract()
{
    QTableWidget table(1, 1);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.widget = table.viewport();
    option.rect = QRect(0, 0, 240, 36);
    option.index = table.model()->index(0, 0);
    option.text = QStringLiteral("Painted underneath editor");
    option.icon = WinUI3::icon(WinUI3::Icon::Settings);
    option.features = QStyleOptionViewItem::HasDisplay
        | QStyleOptionViewItem::HasDecoration;
    option.state = QStyle::State_Enabled | QStyle::State_Selected
        | QStyle::State_Editing;

    const auto render = [&](const QStyleOptionViewItem &source) {
        QImage image(source.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(table.palette().color(QPalette::Base));
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &source,
                                   &painter, table.viewport());
        return image;
    };

    const QImage editing = render(option);
    auto withoutDisplay = option;
    withoutDisplay.features &= ~QStyleOptionViewItem::HasDisplay;
    const QImage expected = render(withoutDisplay);
    QCOMPARE(editing, expected);

    auto unselected = option;
    unselected.state &= ~QStyle::State_Selected;
    QVERIFY(editing.pixelColor(120, 18) != render(unselected).pixelColor(120, 18));
    QVERIFY(editing.pixelColor(8, 18).alpha() > 0);
}

void WinUI3StyleTest::tableLiveEditorSuppressesDisplay()
{
    QTableWidget table(1, 1);
    table.setItem(0, 0, new QTableWidgetItem(
        QStringLiteral("Painted underneath the live editor")));
    table.setEditTriggers(QAbstractItemView::AllEditTriggers);
    table.resize(320, 80);
    table.show();
    QTRY_VERIFY(table.isVisible());

    const QModelIndex index = table.model()->index(0, 0);
    table.editItem(table.item(0, 0));
    QTRY_VERIFY_WITH_TIMEOUT(!table.findChildren<QLineEdit *>().isEmpty(), 1000);
    QLineEdit *editor = table.findChildren<QLineEdit *>().constFirst();
    QVERIFY(editor->isVisible());
    QVERIFY(editor->property("_winui_table_editor").toBool());
    QStyleOptionViewItem option;
    option.initFrom(table.viewport());
    option.widget = table.viewport();
    option.rect = table.visualRect(index);
    option.index = index;
    option.text = QStringLiteral("Painted underneath the live editor");
    option.features = QStyleOptionViewItem::HasDisplay;
    option.state = QStyle::State_Enabled;

    const auto render = [&](const QStyleOptionViewItem &source) {
        QImage image(source.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(table.palette().color(QPalette::Base));
        QPainter painter(&image);
        table.style()->drawControl(QStyle::CE_ItemViewItem, &source,
                                   &painter, table.viewport());
        return image;
    };

    auto withoutDisplay = option;
    withoutDisplay.features &= ~QStyleOptionViewItem::HasDisplay;
    QCOMPARE(render(option), render(withoutDisplay));
}

void WinUI3StyleTest::richEditBoxContract()
{
    QTextEdit editor;
    editor.setPlainText(QStringLiteral("RichEditBox state"));
    editor.resize(320, 120);
    editor.show();
    editor.setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(editor.hasFocus());
    QStyleOptionFrame option;
    option.initFrom(&editor);
    option.rect = editor.rect();
    option.state |= QStyle::State_HasFocus;
    QImage focused(editor.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(editor.palette().color(QPalette::Window));
    {
        QPainter painter(&focused);
        editor.style()->drawPrimitive(QStyle::PE_Frame, &option, &painter,
                                      &editor);
    }
    const QColor accent = editor.palette().color(QPalette::Accent);
    const QColor underline = focused.pixelColor(focused.width() / 2,
                                                 focused.height() - 2);
    const int distance = qAbs(underline.red() - accent.red())
        + qAbs(underline.green() - accent.green())
        + qAbs(underline.blue() - accent.blue());
    QVERIFY2(distance < 100, qPrintable(QString::number(distance)));
}

void WinUI3StyleTest::contentDialogContract()
{
    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Dialog content")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                        | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    if (auto *primary = buttons->button(QDialogButtonBox::Ok))
        primary->setDefault(true);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QVERIFY(dialog.minimumWidth() >= 320);
    QVERIFY(dialog.minimumHeight() >= 184);
    QCOMPARE(layout->contentsMargins(), QMargins(24, 24, 24, 24));
    QCOMPARE(layout->spacing(), 12);
    QCOMPARE(WinUI3::Style::controlRole(buttons->button(QDialogButtonBox::Ok)),
             WinUI3::ControlRole::Accent);
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxInformation).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxWarning).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxCritical).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxQuestion).isNull());
}

void WinUI3StyleTest::contentDialogScrimLifecycle()
{
    QWidget parent;
    parent.resize(640, 480);
    QDialog dialog(&parent);
    WinUI3::Style::setContentDialog(&dialog);
    parent.show();
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    const auto scrims = parent.findChildren<QWidget *>(
        QStringLiteral("_winui_content_dialog_scrim"));
    QCOMPARE(scrims.size(), 1);
    QVERIFY(scrims.first()->isVisible());
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    parent.resize(700, 500);
    qApp->processEvents();
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    dialog.hide();
    qApp->processEvents();
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(parent.findChildren<QWidget *>(
        QStringLiteral("_winui_content_dialog_scrim")).isEmpty());
}

void WinUI3StyleTest::readOnlyActionRestoration()
{
    QLineEdit edit(QStringLiteral("Custom actions remain visible"));
    edit.setClearButtonEnabled(true);
    QAction leading(QStringLiteral("Leading"), &edit);
    leading.setIcon(WinUI3::icon(WinUI3::Icon::Search));
    QAction trailing(QStringLiteral("Trailing"), &edit);
    trailing.setIcon(WinUI3::icon(WinUI3::Icon::Settings));
    edit.addAction(&leading, QLineEdit::LeadingPosition);
    edit.addAction(&trailing, QLineEdit::TrailingPosition);
    edit.resize(320, 32);
    edit.show();
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    QAbstractButton *clearButton = nullptr;
    for (QAbstractButton *button : edit.findChildren<QAbstractButton *>()) {
        const bool custom = leading.associatedObjects().contains(button)
            || trailing.associatedObjects().contains(button);
        if (!custom) {
            clearButton = button;
            break;
        }
    }
    QVERIFY(clearButton);
    const auto customButtons = [&] {
        int visible = 0;
        for (QAbstractButton *button : edit.findChildren<QAbstractButton *>())
            if (button != clearButton && button->isVisible())
                ++visible;
        return visible;
    };
    QVERIFY(customButtons() >= 2);
    edit.setReadOnly(true);
    QTRY_VERIFY(!clearButton->isVisible());
    QVERIFY(customButtons() >= 2);
    edit.setReadOnly(false);
    QTRY_VERIFY(clearButton->isVisible());
    QVERIFY(customButtons() >= 2);
}

void WinUI3StyleTest::animatedStackEffectsAndInterruption()
{
    WinUI3::AnimatedStack stack;
    auto *first = new QLabel(QStringLiteral("First"));
    auto *second = new QLabel(QStringLiteral("Second"));
    auto *effect = new QGraphicsOpacityEffect;
    effect->setOpacity(0.7);
    first->setGraphicsEffect(effect);
    stack.addWidget(first);
    stack.addWidget(second);
    stack.setDuration(120);
    stack.resize(240, 80);
    stack.show();
    stack.setCurrentIndex(1);
    QCOMPARE(second->geometry(), stack.rect());
    QTest::qWait(25);
    QVERIFY(stack.isAnimating());
    stack.setCurrentIndex(0);
    QTRY_VERIFY(!stack.isAnimating());
    QCOMPARE(first->graphicsEffect(), effect);
    stack.setCurrentIndex(1);
    QTest::qWait(20);
    stack.removeWidget(second);
    QTRY_VERIFY(!stack.isAnimating());
    QCOMPARE(first->graphicsEffect(), effect);

    auto *third = new QLabel(QStringLiteral("Third"));
    stack.addWidget(third);
    stack.setCurrentIndex(stack.indexOf(third));
    QTest::qWait(20);
    stack.hide();
    QTRY_VERIFY(!stack.isAnimating());
    stack.show();
    QCOMPARE(stack.currentWidget(), third);
    QCOMPARE(first->graphicsEffect(), effect);

    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    stack.setCurrentIndex(0);
    QVERIFY(!stack.isAnimating());
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    WinUI3::AnimatedStack replacementStack;
    replacementStack.setDuration(100);
    replacementStack.addWidget(new QLabel(QStringLiteral("Outgoing")));
    auto *incoming = new QLabel(QStringLiteral("Incoming"));
    replacementStack.addWidget(incoming);
    replacementStack.resize(240, 80);
    replacementStack.show();
    replacementStack.setCurrentIndex(1);
    QTRY_VERIFY(replacementStack.isAnimating());
    auto *applicationEffect = new QGraphicsOpacityEffect;
    applicationEffect->setOpacity(0.63);
    incoming->setGraphicsEffect(applicationEffect);
    QTRY_VERIFY(!replacementStack.isAnimating());
    QCOMPARE(incoming->graphicsEffect(), applicationEffect);
    QCOMPARE(applicationEffect->opacity(), 0.63);
}

void WinUI3StyleTest::animatedStackLifecycleStress()
{
    WinUI3::AnimatedStack stack;
    stack.setDuration(1000);
    for (int i = 0; i < 7; ++i)
        stack.addWidget(new QLabel(QStringLiteral("Page %1").arg(i)));
    stack.resize(320, 120);
    stack.show();
    QCoreApplication::processEvents();

    auto settle = [&stack] {
        if (auto *group = stack.findChild<QParallelAnimationGroup *>(
                QStringLiteral("_winui_animated_stack_group"),
                Qt::FindDirectChildrenOnly)) {
            group->setCurrentTime(group->duration());
            QCoreApplication::processEvents();
        }
        QCOMPARE(stack.findChildren<QParallelAnimationGroup *>(
                     QStringLiteral("_winui_animated_stack_group"),
                     Qt::FindDirectChildrenOnly).size(), 0);
        QCOMPARE(stack.findChildren<QWidget *>(
                     QStringLiteral("_winui_animated_stack_overlay"),
                     Qt::FindDirectChildrenOnly).size(), 0);
    };

    stack.setCurrentIndex(1);
    QVERIFY(stack.isAnimating());
    stack.resize(480, 160);
    auto overlays = stack.findChildren<QWidget *>(
        QStringLiteral("_winui_animated_stack_overlay"),
        Qt::FindDirectChildrenOnly);
    QCOMPARE(overlays.size(), 1);
    QCOMPARE(overlays.constFirst()->geometry(), stack.rect());

    // Remove a non-current page while the source/cible pair is alive. The
    // target pointer, rather than its old numeric index, must remain final.
    stack.removeWidget(stack.widget(5));
    QCOMPARE(stack.currentWidget(), stack.widget(1));
    settle();

    // Remove the outgoing/source page itself while the target is entering.
    stack.setCurrentIndex(2);
    QWidget *outgoing = stack.widget(1);
    QVERIFY(outgoing);
    stack.removeWidget(outgoing);
    QVERIFY(!stack.isAnimating());
    QCOMPARE(stack.currentWidget(), stack.widget(1));
    settle();

    // Removing the incoming page must fall back to the guarded outgoing page.
    stack.setCurrentIndex(2);
    QVERIFY(stack.isAnimating());
    QWidget *incoming = stack.currentWidget();
    stack.removeWidget(incoming);
    QVERIFY(!stack.isAnimating());
    QVERIFY(stack.currentWidget());
    settle();

    // Remove the page currently being displayed, then exercise a long burst
    // of direction reversals. No iteration may create more than one live
    // animation group or overlay.
    QWidget *current = stack.currentWidget();
    stack.removeWidget(current);
    QVERIFY(stack.currentWidget());
    for (int i = 0; i < 100; ++i) {
        const int target = i % stack.count();
        stack.setCurrentIndex(target,
                             i % 3 == 0 ? WinUI3::AnimatedStack::Transition::Backward
                                         : WinUI3::AnimatedStack::Transition::Forward);
        QVERIFY(stack.findChildren<QParallelAnimationGroup *>(
                     QStringLiteral("_winui_animated_stack_group"),
                     Qt::FindDirectChildrenOnly).size() <= 1);
        QVERIFY(stack.findChildren<QWidget *>(
                    QStringLiteral("_winui_animated_stack_overlay"),
                    Qt::FindDirectChildrenOnly).size() <= 1);
        if (i % 10 == 0) {
            stack.resize(320 + i, 120 + (i % 4) * 10);
            overlays = stack.findChildren<QWidget *>(
                QStringLiteral("_winui_animated_stack_overlay"),
                Qt::FindDirectChildrenOnly);
            if (!overlays.isEmpty())
                QCOMPARE(overlays.constFirst()->geometry(), stack.rect());
        }
    }
    settle();
    QCOMPARE(stack.currentWidget()->geometry(), stack.rect());
}

void WinUI3StyleTest::progressAnimationAndOrientations()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QProgressBar bar;
    bar.setRange(0, 0);
    bar.resize(240, 24);
    bar.show();
    QImage first = bar.grab().toImage();
    QTest::qWait(40);
    QImage second = bar.grab().toImage();
    QVERIFY(first != second);
    auto *timer = style->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                             Qt::FindDirectChildrenOnly);
    QVERIFY(timer);
    QVERIFY(timer->isActive());
    QVERIFY(!bar.findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                     Qt::FindDirectChildrenOnly));

    bar.hide();
    QTRY_VERIFY(!timer->isActive());
    bar.show();
    QTRY_VERIFY(timer->isActive());

    bar.setRange(0, 100);
    bar.setValue(40);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    bar.setRange(0, 0);
    bar.grab();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    bar.setOrientation(Qt::Vertical);
    bar.resize(24, 180);
    bar.setInvertedAppearance(true);
    bar.setEnabled(false);
    QVERIFY(!bar.grab().isNull());

    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
    QTRY_VERIFY(!timer->isActive());
    const QImage frozenFirst = bar.grab().toImage();
    QTest::qWait(40);
    const QImage frozenSecond = bar.grab().toImage();
    QCOMPARE(frozenFirst, frozenSecond);
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    style->unpolish(&bar);
    QVERIFY(!timer->isActive());
}

void WinUI3StyleTest::progressTextAndDisabledPaletteContract()
{
    QProgressBar bar;
    bar.setRange(0, 100);
    bar.setValue(42);
    bar.setFormat(QStringLiteral("Transferred %p%"));

    bar.setTextVisible(false);
    const QSize bareHint = bar.sizeHint();
    bar.setTextVisible(true);
    const QSize textHint = bar.sizeHint();
    QVERIFY(textHint.height() >= bareHint.height() + 6);

    QPalette palette = bar.palette();
    const QColor activeText(210, 20, 10);
    const QColor disabledText(10, 20, 210);
    palette.setColor(QPalette::Active, QPalette::WindowText, activeText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    bar.setPalette(palette);
    bar.setEnabled(false);

    QStyleOptionProgressBar option;
    option.initFrom(&bar);
    option.rect = QRect(QPoint(0, 0), QSize(220, textHint.height()));
    option.minimum = bar.minimum();
    option.maximum = bar.maximum();
    option.progress = bar.value();
    option.text = bar.text();
    option.textVisible = true;

    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    bar.style()->drawControl(QStyle::CE_ProgressBarLabel, &option, &painter, &bar);
    painter.end();

    bool foundDisabledText = false;
    bool foundActiveText = false;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() == 0)
                continue;
            foundDisabledText |= pixel.blue() > pixel.red() + 40;
            foundActiveText |= pixel.red() > pixel.blue() + 40;
        }
    }
    QVERIFY(foundDisabledText);
    QVERIFY(!foundActiveText);
}

void WinUI3StyleTest::progressTimerScalingAndLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(420, 420);
    QList<QProgressBar *> bars;
    for (int i = 0; i < 100; ++i) {
        auto *bar = new QProgressBar(&host);
        bar->setRange(0, 0);
        bar->setGeometry((i % 10) * 42, (i / 10) * 42, 40, 24);
        bars.append(bar);
    }
    host.show();
    QCoreApplication::processEvents();

    const auto timers = style->findChildren<QTimer *>(
        QStringLiteral("_winui_progress_timer"), Qt::FindDirectChildrenOnly);
    QCOMPARE(timers.size(), 1);
    auto *timer = timers.constFirst();
    QVERIFY(timer->isActive());
    for (QProgressBar *bar : bars) {
        QVERIFY(!bar->findChild<QTimer *>(QStringLiteral("_winui_progress_timer"),
                                          Qt::FindDirectChildrenOnly));
    }

    host.hide();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
    host.show();
    QCoreApplication::processEvents();
    QVERIFY(timer->isActive());

    delete bars.takeLast();
    QVERIFY(timer->isActive());
    qDeleteAll(bars);
    bars.clear();
    QCoreApplication::processEvents();
    QVERIFY(!timer->isActive());
}

void WinUI3StyleTest::callbackCoalescingAndAnimationReuse()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    {
        QSlider slider(Qt::Horizontal);
        slider.setRange(0, 100);
        slider.resize(320, 40);
        slider.show();
        QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier,
                          slider.rect().center());
        auto *timer = slider.findChild<QTimer *>(
            QStringLiteral("_winui_slider_tooltip_timer"),
            Qt::FindDirectChildrenOnly);
        QVERIFY(timer);
        QSignalSpy callbacks(timer, &QTimer::timeout);
        for (int i = 0; i < 1000; ++i) {
            slider.setValue(i % 100);
            QMouseEvent move(QEvent::MouseMove,
                             QPointF(slider.rect().center()), Qt::NoButton,
                             Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(&slider, &move);
        }
        slider.setValue(77);
        QVERIFY(timer->isActive());
        QCoreApplication::processEvents();
        QCOMPARE(callbacks.count(), 1);
        QCOMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
                 QStringLiteral("77"));
        QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier,
                            slider.rect().center());
        QVERIFY(!timer->isActive());
    }

    {
        QScrollBar scrollBar(Qt::Vertical);
        scrollBar.setRange(0, 100);
        scrollBar.resize(12, 300);
        scrollBar.show();
        QEvent enter(QEvent::Enter);
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&scrollBar, &enter);
        auto *timer = scrollBar.findChild<QTimer *>(
            QStringLiteral("_winui_scrollbar_timer"),
            Qt::FindDirectChildrenOnly);
        QVERIFY(timer);
        QSignalSpy callbacks(timer, &QTimer::timeout);
        for (int i = 0; i < 100; ++i) {
            QCoreApplication::sendEvent(&scrollBar, &leave);
            QCoreApplication::sendEvent(&scrollBar, &enter);
        }
        QCoreApplication::sendEvent(&scrollBar, &leave);
        QCOMPARE(scrollBar.findChildren<QTimer *>(
                     QStringLiteral("_winui_scrollbar_timer"),
                     Qt::FindDirectChildrenOnly).size(), 1);
        QVERIFY(timer->isActive());
        QTest::qWait(550);
        QCOMPARE(callbacks.count(), 1);
        scrollBar.setEnabled(false);
        QVERIFY(!timer->isActive());
    }

    const int baseline = style->findChildren<QVariantAnimation *>().size();
    auto *button = new QPushButton(QStringLiteral("animation lifecycle"));
    button->show();
    QEvent enter(QEvent::Enter);
    QEvent leave(QEvent::Leave);
    for (int i = 0; i < 1000; ++i) {
        QCoreApplication::sendEvent(button, &enter);
        QCoreApplication::sendEvent(button, &leave);
    }
    const int afterFirstStorm = style->findChildren<QVariantAnimation *>().size();
    QVERIFY(afterFirstStorm <= baseline + 2);
    for (int i = 0; i < 1000; ++i) {
        QCoreApplication::sendEvent(button, &enter);
        QCoreApplication::sendEvent(button, &leave);
    }
    QCOMPARE(style->findChildren<QVariantAnimation *>().size(), afterFirstStorm);
    delete button;
    QCoreApplication::processEvents();
    QCOMPARE(style->findChildren<QVariantAnimation *>().size(), baseline);
}

void WinUI3StyleTest::sliderExtremeRangeTicks()
{
    QSlider slider(Qt::Horizontal);
    slider.setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    slider.setValue(0);
    slider.setTickPosition(QSlider::TicksBelow);
    slider.setTickInterval(1);
    slider.resize(480, 40);
    slider.show();
    QVERIFY(!slider.grab().isNull());

    QStyleOptionSlider option;
    option.initFrom(&slider);
    option.rect = slider.rect();
    option.minimum = slider.minimum();
    option.maximum = slider.maximum();
    option.sliderPosition = slider.sliderPosition();
    option.sliderValue = slider.value();
    option.orientation = Qt::Horizontal;
    option.upsideDown = false;
    const QRect handle = slider.style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, &slider);
    QVERIFY(handle.isValid());
}

void WinUI3StyleTest::rtlGeometryAndHitTesting()
{
    QComboBox combo;
    combo.addItems({QStringLiteral("One"), QStringLiteral("Two")});
    combo.setLayoutDirection(Qt::RightToLeft);
    combo.resize(220, 32);
    combo.show();
    QStyleOptionComboBox comboOption;
    comboOption.initFrom(&combo);
    comboOption.rect = combo.rect();
    comboOption.direction = Qt::RightToLeft;
    const QRect arrow = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, &combo);
    const QRect edit = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY(arrow.left() == combo.rect().left());
    QVERIFY(edit.right() < combo.rect().right());
    QVERIFY(edit.left() == arrow.right() + 1);
    QVERIFY(!arrow.intersects(edit));
    QCOMPARE(combo.style()->hitTestComplexControl(
                 QStyle::CC_ComboBox, &comboOption, arrow.center(), &combo),
             QStyle::SC_ComboBoxArrow);

    QGroupBox group(QStringLiteral("RTL group"));
    group.setCheckable(true);
    group.setLayoutDirection(Qt::RightToLeft);
    group.resize(240, 100);
    QStyleOptionGroupBox groupOption;
    groupOption.initFrom(&group);
    groupOption.rect = group.rect();
    groupOption.direction = Qt::RightToLeft;
    groupOption.subControls = QStyle::SC_GroupBoxFrame
        | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxLabel;
    const QRect check = group.style()->subControlRect(
        QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxCheckBox, &group);
    QVERIFY(check.left() > group.rect().center().x());
    QCOMPARE(group.style()->hitTestComplexControl(
                 QStyle::CC_GroupBox, &groupOption, check.center(), &group),
             QStyle::SC_GroupBoxCheckBox);

    QStyleOptionMenuItem submenu;
    submenu.rect = QRect(0, 0, 200, 36);
    submenu.direction = Qt::RightToLeft;
    submenu.palette = qApp->palette();
    submenu.state = QStyle::State_Enabled;
    submenu.menuItemType = QStyleOptionMenuItem::SubMenu;
    QImage menuImage(submenu.rect.size(), QImage::Format_ARGB32_Premultiplied);
    menuImage.fill(Qt::transparent);
    {
        QPainter painter(&menuImage);
        combo.style()->drawControl(QStyle::CE_MenuItem, &submenu, &painter);
    }
    bool submenuGlyphAtVisualEnd = false;
    for (int y = 0; y < menuImage.height() && !submenuGlyphAtVisualEnd; ++y)
        for (int x = 0; x < 30; ++x)
            if (menuImage.pixelColor(x, y).alpha() > 20) {
                submenuGlyphAtVisualEnd = true;
                break;
            }
    QVERIFY(submenuGlyphAtVisualEnd);

    QStyleOptionTab tab;
    tab.rect = QRect(0, 0, 160, 32);
    tab.direction = Qt::RightToLeft;
    tab.palette = qApp->palette();
    tab.state = QStyle::State_Enabled;
    tab.icon = WinUI3::icon(WinUI3::Icon::Settings);
    QImage tabImage(tab.rect.size(), QImage::Format_ARGB32_Premultiplied);
    tabImage.fill(Qt::transparent);
    {
        QPainter painter(&tabImage);
        combo.style()->drawControl(QStyle::CE_TabBarTabLabel, &tab, &painter);
    }
    bool tabIconOnRight = false;
    for (int y = 0; y < tabImage.height() && !tabIconOnRight; ++y)
        for (int x = tabImage.width() / 2; x < tabImage.width(); ++x)
            if (tabImage.pixelColor(x, y).alpha() > 20) {
                tabIconOnRight = true;
                break;
            }
    QVERIFY(tabIconOnRight);

    QStyleOptionHeader header;
    header.rect = QRect(0, 0, 180, 32);
    header.direction = Qt::RightToLeft;
    header.palette = qApp->palette();
    header.state = QStyle::State_Enabled;
    header.sortIndicator = QStyleOptionHeader::SortUp;
    QImage sorted(header.rect.size(), QImage::Format_ARGB32_Premultiplied);
    sorted.fill(Qt::transparent);
    QImage unsorted = sorted;
    {
        QPainter painter(&sorted);
        combo.style()->drawControl(QStyle::CE_Header, &header, &painter);
    }
    header.sortIndicator = QStyleOptionHeader::None;
    {
        QPainter painter(&unsorted);
        combo.style()->drawControl(QStyle::CE_Header, &header, &painter);
    }
    int rightmostSortDifference = -1;
    for (int y = 0; y < sorted.height(); ++y)
        for (int x = 0; x < sorted.width(); ++x)
            if (sorted.pixel(x, y) != unsorted.pixel(x, y))
                rightmostSortDifference = qMax(rightmostSortDifference, x);
    QVERIFY(rightmostSortDifference >= 0);
    QVERIFY(rightmostSortDifference < 40);

    QSlider slider(Qt::Horizontal);
    slider.setLayoutDirection(Qt::RightToLeft);
    slider.setRange(0, 100);
    slider.setValue(50);
    slider.resize(240, 40);
    slider.show();
    QStyleOptionSlider sliderOption;
    sliderOption.initFrom(&slider);
    sliderOption.rect = slider.rect();
    sliderOption.orientation = Qt::Horizontal;
    sliderOption.minimum = slider.minimum();
    sliderOption.maximum = slider.maximum();
    sliderOption.sliderPosition = slider.sliderPosition();
    sliderOption.upsideDown = true;
    const QRect sliderHandle = slider.style()->subControlRect(
        QStyle::CC_Slider, &sliderOption, QStyle::SC_SliderHandle, &slider);
    QCOMPARE(slider.style()->hitTestComplexControl(
                 QStyle::CC_Slider, &sliderOption, sliderHandle.center(), &slider),
             QStyle::SC_SliderHandle);
    const int before = slider.value();
    QTest::mouseClick(&slider, Qt::RightButton, Qt::NoModifier,
                      QPoint(slider.width() - 4, slider.height() / 2));
    QTest::mouseClick(&slider, Qt::MiddleButton, Qt::NoModifier,
                      QPoint(slider.width() - 4, slider.height() / 2));
    QCOMPARE(slider.value(), before);
}

void WinUI3StyleTest::itemViewMouseFocusReset()
{
    QListWidget list;
    list.addItems({QStringLiteral("First"), QStringLiteral("Second")});
    list.resize(240, 100);
    list.show();
    QTRY_VERIFY(list.hasFocus());
    QFocusEvent keyboardFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(list.viewport(), &keyboardFocus);
    QVERIFY(frameBool(&list, "_winui_focus_visible"));
    QTest::mouseClick(list.viewport(), Qt::LeftButton, Qt::NoModifier,
                      list.visualItemRect(list.item(0)).center());
    QVERIFY(!frameBool(&list, "_winui_focus_visible"));
    QVERIFY(!frameBool(list.viewport(), "_winui_focus_visible"));
}

void WinUI3StyleTest::checkboxAndRadioUncheckMotion()
{
    QCheckBox check(QStringLiteral("Check"));
    check.resize(check.sizeHint());
    check.show();
    check.setChecked(true);
    QTRY_VERIFY(frameReal(&check, "_winui_check_progress") > 0.99);
    const QImage checkedImage = check.grab().toImage();
    check.setChecked(false);
    // AnimatedAcceptVisualSource removes NormalOnToNormalOff immediately;
    // only the acceptance path is animated.
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 0.0);
    const QImage uncheckedImage = check.grab().toImage();
    setFrame(&check, "_winui_check_progress", 0.5);
    const QImage checkMidpoint = check.grab().toImage();
    QVERIFY(checkMidpoint != checkedImage);
    QVERIFY(checkMidpoint != uncheckedImage);
    setFrame(&check, "_winui_check_progress", 0.0);
    check.setChecked(true);
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 0.0);
    QTRY_VERIFY(frameReal(&check, "_winui_check_progress") > 0.99);

    QWidget host;
    auto *one = new QRadioButton(QStringLiteral("One"), &host);
    auto *two = new QRadioButton(QStringLiteral("Two"), &host);
    one->move(0, 0);
    two->move(0, 36);
    one->setChecked(true);
    host.show();
    QTRY_VERIFY(frameReal(one, "_winui_check_progress") > 0.99);
    const QImage radioChecked = one->grab().toImage();
    two->setChecked(true);
    QVERIFY(frameReal(one, "_winui_check_progress") > 0.99);
    QTRY_VERIFY(frameReal(one, "_winui_check_progress") < 0.01);
    const QImage radioUnchecked = one->grab().toImage();
    setFrame(one, "_winui_check_progress", 0.5);
    const QImage radioMidpoint = one->grab().toImage();
    QVERIFY(radioMidpoint != radioChecked);
    QVERIFY(radioMidpoint != radioUnchecked);
}

void WinUI3StyleTest::navigationModelReconnectAndScroll()
{
    QListView view;
    view.setLayoutDirection(Qt::RightToLeft);
    WinUI3::Style::setNavigationView(&view);
    QStandardItemModel first(40, 1);
    for (int row = 0; row < first.rowCount(); ++row)
        first.setData(first.index(row, 0), QStringLiteral("First %1").arg(row));
    view.setModel(&first);
    view.setCurrentIndex(first.index(10, 0));
    view.resize(260, 120);
    view.show();
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    view.scrollTo(first.index(10, 0), QAbstractItemView::PositionAtCenter);
    const qreal before = frameReal(view.viewport(),
                                   "_winui_navigation_indicator_y");

    QStandardItemModel second(40, 1);
    for (int row = 0; row < second.rowCount(); ++row)
        second.setData(second.index(row, 0), QStringLiteral("Second %1").arg(row));
    view.setModel(&second);
    auto *replacementSelection = new QItemSelectionModel(&second, &view);
    view.setSelectionModel(replacementSelection);
    view.setCurrentIndex(second.index(20, 0));
    view.scrollTo(second.index(20, 0), QAbstractItemView::PositionAtCenter);
    QCoreApplication::processEvents();
    const qreal after = frameReal(view.viewport(),
                                  "_winui_navigation_indicator_y");
    QVERIFY(std::isfinite(after));
    QVERIFY(before != after || view.currentIndex().row() == 20);

    second.clear();
    QCoreApplication::processEvents();
    QVERIFY(!view.currentIndex().isValid());
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());
}

void WinUI3StyleTest::navigationDelegateLifecycle()
{
    QListView view;
    view.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
    view.resize(280, 140);
    QStandardItemModel model(40, 1);
    for (int row = 0; row < model.rowCount(); ++row)
        model.setData(model.index(row, 0), QStringLiteral("Item %1").arg(row));
    view.setModel(&model);
    view.setCurrentIndex(model.index(4, 0));
    const QPalette originalViewPalette = view.palette();
    const QPalette originalViewportPalette = view.viewport()->palette();
    const QFrame::Shape originalFrameShape = view.frameShape();
    const bool originalViewportAutoFill = view.viewport()->autoFillBackground();
    const bool originalViewportOpaque =
        view.viewport()->testAttribute(Qt::WA_OpaquePaintEvent);
    WinUI3::Style::setNavigationView(&view);
    view.show();
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    QCOMPARE(view.palette().color(QPalette::Base), QColor(Qt::transparent));
    QCOMPARE(view.viewport()->palette().color(QPalette::Base),
             QColor(Qt::transparent));
    QCOMPARE(view.frameShape(), QFrame::NoFrame);

    // Re-entering while the previous delegate is deferred for deletion must
    // not create a second model/scrollbar subscription.
    for (int cycle = 0; cycle < 6; ++cycle) {
        WinUI3::Style::setNavigationView(&view, false);
        WinUI3::Style::setNavigationView(&view, true);
    }
    QCoreApplication::processEvents();
    QVERIFY(view.itemDelegate());
    QVERIFY(view.property("_winui_navigation_delegate").isValid());

    auto *external = new QStyledItemDelegate(&view);
    view.setItemDelegate(external);
    WinUI3::Style::setNavigationView(&view, false);
    QCoreApplication::processEvents();
    QCOMPARE(view.itemDelegate(), external);
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    // The original delegate can disappear before restoration. The style must
    // install a valid owned fallback instead of restoring a dangling pointer.
    auto *original = new QStyledItemDelegate(&view);
    view.setItemDelegate(original);
    WinUI3::Style::setNavigationView(&view, true);
    QTRY_VERIFY(view.property("_winui_navigation_delegate").isValid());
    delete original;
    WinUI3::Style::setNavigationView(&view, false);
    QCoreApplication::processEvents();
    QVERIFY(view.itemDelegate());
    QVERIFY(!view.property("_winui_navigation_delegate").isValid());
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    // A model reset while the indicator is moving must leave no stale target.
    WinUI3::Style::setNavigationView(&view, true);
    view.setCurrentIndex(model.index(20, 0));
    model.clear();
    QCoreApplication::processEvents();
    QVERIFY(!frameValue(view.viewport(),
                        "_winui_navigation_indicator_y").isValid());

    WinUI3::Style::setNavigationView(&view, false);
    QCOMPARE(view.palette(), originalViewPalette);
    QCOMPARE(view.viewport()->palette(), originalViewportPalette);
    QCOMPARE(view.frameShape(), originalFrameShape);
    QCOMPARE(view.viewport()->autoFillBackground(), originalViewportAutoFill);
    QCOMPARE(view.viewport()->testAttribute(Qt::WA_OpaquePaintEvent),
             originalViewportOpaque);

    QListView opaqueView;
    const QPalette opaquePalette = opaqueView.palette();
    WinUI3::Style::setNavigationView(&opaqueView);
    opaqueView.show();
    QCoreApplication::processEvents();
    QCOMPARE(opaqueView.palette().color(QPalette::Base),
             opaquePalette.color(QPalette::Base));
    opaqueView.setProperty(WinUI3::Style::BackdropProperty,
                           QStringLiteral("mica"));
    QTRY_COMPARE(opaqueView.palette().color(QPalette::Base),
                 QColor(Qt::transparent));
    opaqueView.setProperty(WinUI3::Style::BackdropProperty,
                           QStringLiteral("none"));
    QTRY_COMPARE(opaqueView.palette().color(QPalette::Base),
                 opaquePalette.color(QPalette::Base));

    QWidget backdropHost;
    QListView inheritedView(&backdropHost);
    WinUI3::Style::setNavigationView(&inheritedView);
    backdropHost.show();
    QCoreApplication::processEvents();
    const bool inheritedViewPaletteExplicit =
        inheritedView.testAttribute(Qt::WA_SetPalette);
    const bool inheritedViewportPaletteExplicit =
        inheritedView.viewport()->testAttribute(Qt::WA_SetPalette);
    backdropHost.setProperty(WinUI3::Style::BackdropProperty,
                             QStringLiteral("mica"));
    QTRY_COMPARE(inheritedView.palette().color(QPalette::Base),
                 QColor(Qt::transparent));
    backdropHost.setProperty(WinUI3::Style::BackdropProperty,
                             QStringLiteral("none"));
    QTRY_COMPARE(inheritedView.testAttribute(Qt::WA_SetPalette),
                 inheritedViewPaletteExplicit);
    QTRY_COMPARE(inheritedView.viewport()->testAttribute(Qt::WA_SetPalette),
                 inheritedViewportPaletteExplicit);
}

void WinUI3StyleTest::runtimeAppearanceAndDialogLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget window;
    window.resize(200, 80);
    window.show();
    QTRY_VERIFY(window.isVisible());

    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_VERIFY(window.palette().color(QPalette::Window).lightness() < 128);
    const QColor accent(210, 45, 90);
    style->setAccentColor(accent);
    QTRY_COMPARE(qApp->palette().color(QPalette::Highlight), accent);
    QVERIFY(qApp->palette().color(QPalette::Accent) != accent);
    auto *watchdog = style->findChild<QTimer *>(
        QStringLiteral("_winui_system_appearance_watchdog"));
    QVERIFY(watchdog);
    QVERIFY(!watchdog->isActive());

    style->setThemeMode(WinUI3::ThemeMode::System);
    QVERIFY(watchdog->isActive());
    QCOMPARE(watchdog->interval(), 15000);

    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Lifecycle")));
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    dialog.hide();
    QTRY_VERIFY(!dialog.property("_winui_dialog_animating").toBool());
    QCOMPARE(dialog.windowOpacity(), 1.0);
    QTRY_VERIFY(!dialog.findChild<QParallelAnimationGroup *>(
        QStringLiteral("_winui_dialog_animation")));
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QTRY_VERIFY(!dialog.property("_winui_dialog_animating").toBool());
    QCOMPARE(dialog.windowOpacity(), 1.0);

    dialog.hide();
    dialog.show();
    dialog.hide();
    dialog.show();
    QVERIFY(dialog.findChildren<QParallelAnimationGroup *>(
                QStringLiteral("_winui_dialog_animation"),
                Qt::FindDirectChildrenOnly).size() <= 1);
    dialog.hide();
    QVERIFY(!dialog.property("_winui_dialog_animating").toBool());
    QCOMPARE(dialog.windowOpacity(), 1.0);
    QVERIFY(dialog.findChildren<QParallelAnimationGroup *>(
                QStringLiteral("_winui_dialog_animation"),
                Qt::FindDirectChildrenOnly).isEmpty());

    style->setAccentColor({});
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3StyleTest::dpiGeometry()
{
    QPushButton button(QStringLiteral("DPI"));
    button.resize(button.sizeHint());
    button.show();
    const qreal devicePixelRatio = button.devicePixelRatioF();
    QVERIFY(devicePixelRatio >= 1.0);
    if (qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        bool ok = false;
        const qreal requestedScale = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
        if (ok)
            QVERIFY(qAbs(devicePixelRatio - requestedScale) < 0.05);
    }
    const QPixmap grabbed = button.grab();
    QVERIFY(qAbs(grabbed.width() - qRound(button.width() * grabbed.devicePixelRatioF()))
            <= 2);
    QVERIFY(qAbs(grabbed.height() - qRound(button.height() * grabbed.devicePixelRatioF()))
            <= 2);
    QVERIFY(button.style()->pixelMetric(QStyle::PM_DefaultFrameWidth,
                                        nullptr, &button) >= 1);

    QComboBox combo;
    combo.addItem(QStringLiteral("DPI"));
    combo.resize(220, 32);
    combo.show();
    QStyleOptionComboBox comboOption;
    comboOption.initFrom(&combo);
    comboOption.rect = combo.rect();
    const QRect comboArrow = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, &combo);
    QCOMPARE(comboArrow.size(), QSize(38, 32));

    QScrollBar scrollBar(Qt::Vertical);
    scrollBar.setRange(0, 100);
    scrollBar.setPageStep(20);
    scrollBar.resize(12, 300);
    scrollBar.show();
    QStyleOptionSlider scrollOption;
    scrollOption.initFrom(&scrollBar);
    scrollOption.orientation = scrollBar.orientation();
    scrollOption.minimum = scrollBar.minimum();
    scrollOption.maximum = scrollBar.maximum();
    scrollOption.sliderPosition = scrollBar.sliderPosition();
    scrollOption.sliderValue = scrollBar.value();
    scrollOption.pageStep = scrollBar.pageStep();
    scrollOption.upsideDown = false;
    const QRect scrollThumb = scrollBar.style()->subControlRect(
        QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSlider,
        &scrollBar);
    QCOMPARE(scrollThumb.width(), 12);
    QCOMPARE(scrollBar.style()->pixelMetric(QStyle::PM_ScrollBarExtent,
                                            &scrollOption, &scrollBar), 12);
}

void WinUI3StyleTest::dpiHitTestContracts()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QComboBox combo;
    combo.addItem(QStringLiteral("Combo text"));
    combo.resize(220, 32);
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(42);
    spin.resize(180, 32);
    QToolButton tool;
    tool.setText(QStringLiteral("Tool button"));
    tool.setPopupMode(QToolButton::MenuButtonPopup);
    tool.resize(220, 36);
    QSlider slider(Qt::Horizontal);
    slider.setRange(0, 100);
    slider.setValue(50);
    slider.resize(320, 32);
    QScrollBar scrollBar(Qt::Horizontal);
    scrollBar.setRange(0, 100);
    scrollBar.setPageStep(25);
    scrollBar.setValue(40);
    scrollBar.resize(260, 12);
    setFrame(&scrollBar, "_winui_hover_progress", 1.0);
    QGroupBox group(QStringLiteral("Group title"));
    group.setCheckable(true);
    group.setChecked(true);
    group.resize(320, 120);

    const qreal dpr = combo.devicePixelRatioF();
    QVERIFY(dpr >= 1.0);
    if (qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        bool ok = false;
        const qreal requested = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
        if (ok)
            QVERIFY2(qAbs(dpr - requested) < 0.05,
                     qPrintable(QStringLiteral("DPR=%1 requested=%2")
                                    .arg(dpr).arg(requested)));
    }

    const auto verifyImage = [&](const QWidget &widget,
                                 QStyle::ComplexControl control,
                                 const QStyleOptionComplex &option,
                                 const QList<QRect> &inkRegions) {
        const QImage image = renderComplex(style, control, &option, &widget, dpr);
        QCOMPARE(image.size(), QSize(qRound(option.rect.width() * dpr),
                                     qRound(option.rect.height() * dpr)));
        const QColor background = widget.palette().color(QPalette::Window);
        int totalInk = inkPixels(image, option.rect, dpr, background);
        QVERIFY(totalInk > 0);
        for (const QRect &region : inkRegions)
            if (region.isValid())
                QVERIFY2(inkPixels(image, region, dpr, background) > 0,
                         qPrintable(QStringLiteral("no ink in %1,%2 %3x%4")
                                        .arg(region.x()).arg(region.y())
                                        .arg(region.width()).arg(region.height())));
    };

    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                Qt::RightToLeft}) {
        combo.setLayoutDirection(direction);
        QStyleOptionComboBox comboOption;
        comboOption.initFrom(&combo);
        comboOption.direction = direction;
        comboOption.rect = combo.rect();
        comboOption.currentText = combo.currentText();
        comboOption.subControls = QStyle::SC_ComboBoxFrame
            | QStyle::SC_ComboBoxEditField | QStyle::SC_ComboBoxArrow;
        const QRect comboEdit = style->subControlRect(
            QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxEditField,
            &combo);
        const QRect comboArrow = style->subControlRect(
            QStyle::CC_ComboBox, &comboOption, QStyle::SC_ComboBoxArrow, &combo);
        QVERIFY(comboEdit.isValid());
        QVERIFY(comboArrow.isValid());
        QVERIFY(!comboEdit.intersects(comboArrow));
        if (direction == Qt::LeftToRight)
            QCOMPARE(comboEdit.right() + 1, comboArrow.left());
        else
            QCOMPARE(comboArrow.right() + 1, comboEdit.left());
        verifyHitSurface(style, QStyle::CC_ComboBox, &comboOption, &combo);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ComboBox, &comboOption,
                                               comboArrow.center(), &combo),
                 QStyle::SC_ComboBoxArrow);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ComboBox, &comboOption,
                                               comboEdit.center(), &combo),
                 QStyle::SC_ComboBoxEditField);
        verifyImage(combo, QStyle::CC_ComboBox, comboOption,
                    {comboEdit, comboArrow});

        spin.setLayoutDirection(direction);
        QStyleOptionSpinBox spinOption;
        spinOption.initFrom(&spin);
        spinOption.direction = direction;
        spinOption.rect = spin.rect();
        spinOption.frame = true;
        spinOption.buttonSymbols = spin.buttonSymbols();
        spinOption.stepEnabled = QAbstractSpinBox::StepUpEnabled
            | QAbstractSpinBox::StepDownEnabled;
        spinOption.subControls = QStyle::SC_SpinBoxFrame
            | QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxUp
            | QStyle::SC_SpinBoxDown;
        const QRect spinEdit = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxEditField, &spin);
        const QRect spinUp = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxUp, &spin);
        const QRect spinDown = style->subControlRect(
            QStyle::CC_SpinBox, &spinOption, QStyle::SC_SpinBoxDown, &spin);
        QVERIFY(spinEdit.isValid() && spinUp.isValid() && spinDown.isValid());
        QVERIFY(!spinEdit.intersects(spinUp));
        QVERIFY(!spinEdit.intersects(spinDown));
        QVERIFY(!spinUp.intersects(spinDown));
        verifyHitSurface(style, QStyle::CC_SpinBox, &spinOption, &spin);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinUp.center(), &spin),
                 QStyle::SC_SpinBoxUp);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinDown.center(), &spin),
                 QStyle::SC_SpinBoxDown);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_SpinBox, &spinOption,
                                               spinEdit.center(), &spin),
                 QStyle::SC_SpinBoxEditField);
        verifyImage(spin, QStyle::CC_SpinBox, spinOption,
                    {spinEdit, spinUp, spinDown});

        tool.setLayoutDirection(direction);
        QStyleOptionToolButton toolOption;
        toolOption.initFrom(&tool);
        toolOption.direction = direction;
        toolOption.rect = tool.rect();
        toolOption.text = tool.text();
        toolOption.features = QStyleOptionToolButton::MenuButtonPopup;
        toolOption.subControls = QStyle::SC_ToolButton
            | QStyle::SC_ToolButtonMenu;
        const QRect toolMain = style->subControlRect(
            QStyle::CC_ToolButton, &toolOption, QStyle::SC_ToolButton, &tool);
        const QRect toolMenu = style->subControlRect(
            QStyle::CC_ToolButton, &toolOption, QStyle::SC_ToolButtonMenu, &tool);
        QVERIFY(toolMain.isValid() && toolMenu.isValid());
        QVERIFY(!toolMain.intersects(toolMenu));
        QCOMPARE(toolMain.united(toolMenu), tool.rect());
        verifyHitSurface(style, QStyle::CC_ToolButton, &toolOption, &tool);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ToolButton, &toolOption,
                                               toolMain.center(), &tool),
                 QStyle::SC_ToolButton);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ToolButton, &toolOption,
                                               toolMenu.center(), &tool),
                 QStyle::SC_ToolButtonMenu);
        verifyImage(tool, QStyle::CC_ToolButton, toolOption,
                    {toolMain, toolMenu});

        slider.setLayoutDirection(direction);
        QStyleOptionSlider sliderOption;
        sliderOption.initFrom(&slider);
        sliderOption.direction = direction;
        sliderOption.rect = slider.rect();
        sliderOption.orientation = slider.orientation();
        sliderOption.minimum = slider.minimum();
        sliderOption.maximum = slider.maximum();
        sliderOption.sliderPosition = slider.sliderPosition();
        sliderOption.sliderValue = slider.value();
        sliderOption.upsideDown = direction == Qt::RightToLeft;
        sliderOption.subControls = QStyle::SC_SliderGroove
            | QStyle::SC_SliderHandle;
        const QRect sliderGroove = style->subControlRect(
            QStyle::CC_Slider, &sliderOption, QStyle::SC_SliderGroove, &slider);
        const QRect sliderHandle = style->subControlRect(
            QStyle::CC_Slider, &sliderOption, QStyle::SC_SliderHandle, &slider);
        QVERIFY(sliderGroove.isValid() && sliderHandle.isValid());
        QVERIFY(sliderGroove.intersects(sliderHandle));
        verifyHitSurface(style, QStyle::CC_Slider, &sliderOption, &slider,
                         sliderGroove, {sliderHandle});
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_Slider, &sliderOption,
                                               sliderHandle.center(), &slider),
                 QStyle::SC_SliderHandle);
        verifyImage(slider, QStyle::CC_Slider, sliderOption,
                    {sliderGroove, sliderHandle});

        scrollBar.setLayoutDirection(direction);
        QStyleOptionSlider scrollOption;
        scrollOption.initFrom(&scrollBar);
        scrollOption.direction = direction;
        scrollOption.rect = scrollBar.rect();
        scrollOption.orientation = scrollBar.orientation();
        scrollOption.minimum = scrollBar.minimum();
        scrollOption.maximum = scrollBar.maximum();
        scrollOption.sliderPosition = scrollBar.sliderPosition();
        scrollOption.sliderValue = scrollBar.value();
        scrollOption.pageStep = scrollBar.pageStep();
        scrollOption.upsideDown = direction == Qt::RightToLeft;
        scrollOption.subControls = QStyle::SC_ScrollBarSubLine
            | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubPage
            | QStyle::SC_ScrollBarAddPage | QStyle::SC_ScrollBarGroove
            | QStyle::SC_ScrollBarSlider;
        const QRect scrollSub = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSubLine,
            &scrollBar);
        const QRect scrollAdd = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarAddLine,
            &scrollBar);
        const QRect scrollThumb = style->subControlRect(
            QStyle::CC_ScrollBar, &scrollOption, QStyle::SC_ScrollBarSlider,
            &scrollBar);
        QVERIFY(scrollSub.isValid() && scrollAdd.isValid() && scrollThumb.isValid());
        verifyHitSurface(style, QStyle::CC_ScrollBar, &scrollOption, &scrollBar);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollSub.center(), &scrollBar),
                 QStyle::SC_ScrollBarSubLine);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollAdd.center(), &scrollBar),
                 QStyle::SC_ScrollBarAddLine);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_ScrollBar, &scrollOption,
                                               scrollThumb.center(), &scrollBar),
                 QStyle::SC_ScrollBarSlider);
        verifyImage(scrollBar, QStyle::CC_ScrollBar, scrollOption,
                    {scrollSub, scrollAdd, scrollThumb});

        group.setLayoutDirection(direction);
        QStyleOptionGroupBox groupOption;
        groupOption.initFrom(&group);
        groupOption.direction = direction;
        groupOption.rect = group.rect();
        groupOption.text = group.title();
        groupOption.subControls = QStyle::SC_GroupBoxFrame
            | QStyle::SC_GroupBoxLabel | QStyle::SC_GroupBoxCheckBox
            | QStyle::SC_GroupBoxContents;
        const QRect groupIndicator = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxCheckBox, &group);
        const QRect groupLabel = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxLabel, &group);
        const QRect groupContents = style->subControlRect(
            QStyle::CC_GroupBox, &groupOption, QStyle::SC_GroupBoxContents, &group);
        QVERIFY(groupIndicator.isValid() && groupLabel.isValid()
                && groupContents.isValid());
        verifyHitSurface(style, QStyle::CC_GroupBox, &groupOption, &group);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_GroupBox, &groupOption,
                                               groupIndicator.center(), &group),
                 QStyle::SC_GroupBoxCheckBox);
        QCOMPARE(style->hitTestComplexControl(QStyle::CC_GroupBox, &groupOption,
                                               groupContents.center(), &group),
                 QStyle::SC_GroupBoxContents);
        verifyImage(group, QStyle::CC_GroupBox, groupOption,
                    {groupIndicator, groupLabel, groupContents});
    }
}

QTEST_MAIN(WinUI3StyleTest)
#include "tst_winui3style.moc"
