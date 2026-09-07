// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
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

#include "winui3testhelpers.h"

class WinUI3ButtonsTest final : public QObject
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
    void systemThemeMatchesResolvedExplicitTheme();
    void buttonToolButtonAndIconContracts();
    void buttonPressedStateFollowsQtState();
    void buttonPressedForegroundRoles();
    void coloredIconCacheReuseAndPixelContract();
    void iconPixmapCacheDprAndPalette();
    void buttonPressedPulseContract();
    void commandLinkButtonContract();
    void disabledButtonHasNoInteractionState();
    void toolButtonIconVerticalCenter();
    void toolbarButtonCornerSymmetry();
    void controlRoles();
};

void WinUI3ButtonsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3ButtonsTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3ButtonsTest::cleanup()
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

void WinUI3ButtonsTest::palettes()
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
    QCOMPARE(light.standardPalette().color(QPalette::PlaceholderText), QColor(0, 0, 0, 158));
    QCOMPARE(dark.standardPalette().color(QPalette::Button), QColor(255, 255, 255, 15));
    QCOMPARE(dark.standardPalette().color(QPalette::PlaceholderText), QColor(255, 255, 255, 197));
}

void WinUI3ButtonsTest::paletteDerivedTokensMatchWinUIConstants()
{
    // Guard the palette-derived token pipeline: building tokens from the
    // style's own standard palette must reproduce the WinUI theme resources
    // byte-for-byte, so the refactor is render-neutral for default palettes.
    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        WinUI3::Style style(mode);
        const QPalette palette = style.standardPalette();
        const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);
        const bool dark = mode == WinUI3::ThemeMode::Dark;

        QCOMPARE(t.dark, dark);
        QCOMPARE(t.textPrimary, dark ? QColor(255, 255, 255) : QColor(0, 0, 0, 228));
        QCOMPARE(t.textSecondary, dark ? QColor(255, 255, 255, 197) : QColor(0, 0, 0, 158));
        QCOMPARE(t.textTertiary, dark ? QColor(255, 255, 255, 135) : QColor(0, 0, 0, 114));
        QCOMPARE(t.textDisabled, dark ? QColor(255, 255, 255, 93) : QColor(0, 0, 0, 92));
        QCOMPARE(t.layer, dark ? QColor(58, 58, 58, 76) : QColor(255, 255, 255, 128));
        QCOMPARE(t.control, dark ? QColor(255, 255, 255, 15) : QColor(255, 255, 255, 179));
        QCOMPARE(t.controlHover, dark ? QColor(255, 255, 255, 21) : QColor(249, 249, 249, 128));
        QCOMPARE(t.controlPressed, dark ? QColor(255, 255, 255, 8) : QColor(229, 229, 229, 179));
        QCOMPARE(t.controlDisabled, dark ? QColor(255, 255, 255, 11) : QColor(249, 249, 249, 77));
        QCOMPARE(t.subtleHover, dark ? QColor(255, 255, 255, 15) : QColor(0, 0, 0, 9));
        QCOMPARE(t.subtlePressed, dark ? QColor(255, 255, 255, 10) : QColor(0, 0, 0, 6));
        QCOMPARE(t.stroke, dark ? QColor(255, 255, 255, 18) : QColor(0, 0, 0, 15));
        QCOMPARE(t.strokeSecondary, dark ? QColor(255, 255, 255, 24) : QColor(0, 0, 0, 41));
        QCOMPARE(t.strokeStrong, dark ? QColor(255, 255, 255, 139) : QColor(0, 0, 0, 114));

        const QColor popup = WinUI3::Private::popupSurfaceColor(palette);
        QCOMPARE(popup, dark ? QColor(44, 44, 44) : QColor(252, 252, 252));
    }
}

void WinUI3ButtonsTest::customWidgetPaletteDrivesTokensAndPaint()
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
            const int chroma = pixel.red() - qMax(pixel.green(), pixel.blue());
            if (chroma > 60) {
                foundRedInk = true;
                break;
            }
        }
    }
    QVERIFY2(foundRedInk, "button painted with a custom palette shows no custom-ink pixels");
}

void WinUI3ButtonsTest::systemAccentRampIsAtomic()
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
    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
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

void WinUI3ButtonsTest::systemThemeMatchesResolvedExplicitTheme()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    style->setThemeMode(WinUI3::ThemeMode::System);
    const QPalette systemPalette = style->standardPalette();
    const bool systemIsDark = qGray(systemPalette.color(QPalette::Window).rgb()) < 128;

    QMainWindow window;
    auto *menu = window.menuBar()->addMenu(QStringLiteral("File"));
    menu->addAction(QStringLiteral("Open"));
    auto *navigation = new QListWidget(&window);
    navigation->addItems({ QStringLiteral("Controls"), QStringLiteral("Settings") });
    window.setCentralWidget(navigation);
    window.resize(320, 180);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    qApp->processEvents();
    const QImage systemWindow = window.grab().toImage();
    menu->popup(window.mapToGlobal(QPoint(0, window.menuBar()->height())));
    QTRY_VERIFY(menu->isVisible());
    const QImage systemMenu = menu->grab().toImage();
    menu->hide();

    style->setThemeMode(systemIsDark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QCOMPARE(style->standardPalette(), systemPalette);
    qApp->processEvents();
    QCOMPARE(window.grab().toImage(), systemWindow);
    menu->popup(window.mapToGlobal(QPoint(0, window.menuBar()->height())));
    QTRY_VERIFY(menu->isVisible());
    QCOMPARE(menu->grab().toImage(), systemMenu);
    menu->hide();
}

void WinUI3ButtonsTest::buttonToolButtonAndIconContracts()
{
    const QIcon fluent = WinUI3::icon(WinUI3::Icon::Search);
    QVERIFY(WinUI3::isFluentIcon(fluent));
    QIcon lastGenerated;
    for (int index = 0; index < 2500; ++index)
        lastGenerated = WinUI3::icon(WinUI3::Icon::Search);
    QVERIFY(WinUI3::isFluentIcon(fluent));
    QVERIFY(WinUI3::isFluentIcon(lastGenerated));
    const QPixmap red = WinUI3::iconPixmap(fluent, QSize(20, 20), 1.0, QColor(220, 20, 40));
    const QPixmap blue = WinUI3::iconPixmap(fluent, QSize(20, 20), 1.0, QColor(20, 60, 220));
    QVERIFY(!red.isNull());
    QVERIFY(red.toImage() != blue.toImage());

    QPixmap applicationPixmap(16, 16);
    applicationPixmap.fill(QColor(20, 200, 40));
    const QIcon applicationIcon(applicationPixmap);
    QVERIFY(!WinUI3::isFluentIcon(applicationIcon));
    const QImage preserved =
            WinUI3::iconPixmap(applicationIcon, QSize(16, 16), 1.0, QColor(220, 20, 40)).toImage();
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

    // The toolbar "More", "New", and "Text tool" buttons must resolve to
    // Fluent glyphs rather than falling through to platform default Qt icons
    // (green Qt-logo regression).
    const QIcon toolbarMore = qApp->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
    const QIcon toolbarNew = qApp->style()->standardIcon(QStyle::SP_FileIcon);
    const QIcon toolbarText = qApp->style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    QVERIFY(WinUI3::isFluentIcon(toolbarMore));
    QVERIFY(WinUI3::isFluentIcon(toolbarNew));
    QVERIFY(WinUI3::isFluentIcon(toolbarText));

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
    toolbar.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("layer"));
    toolbar.show();
    QVERIFY(!toolbar.grab().isNull());

    QStyleOption toolbarPanel;
    toolbarPanel.initFrom(&toolbar);
    toolbarPanel.rect = QRect(0, 0, 80, 24);
    QImage toolbarPanelImage(toolbarPanel.rect.size(), QImage::Format_ARGB32_Premultiplied);
    toolbarPanelImage.fill(Qt::black);
    {
        QPainter painter(&toolbarPanelImage);
        toolbar.style()->drawControl(QStyle::CE_ToolBar, &toolbarPanel, &painter, &toolbar);
    }
    QCOMPARE(toolbarPanelImage.pixelColor(70, 12), toolbar.palette().color(QPalette::Window));

    QStyleOptionToolButton option;
    option.initFrom(tool);
    option.rect = tool->rect();
    option.features = QStyleOptionToolButton::MenuButtonPopup;
    const QRect main = tool->style()->subControlRect(QStyle::CC_ToolButton, &option,
                                                     QStyle::SC_ToolButton, tool);
    const QRect drop = tool->style()->subControlRect(QStyle::CC_ToolButton, &option,
                                                     QStyle::SC_ToolButtonMenu, tool);
    QVERIFY(!main.intersects(drop));
    QCOMPARE(tool->style()->hitTestComplexControl(QStyle::CC_ToolButton, &option, main.center(),
                                                  tool),
             QStyle::SC_ToolButton);
    QCOMPARE(tool->style()->hitTestComplexControl(QStyle::CC_ToolButton, &option, drop.center(),
                                                  tool),
             QStyle::SC_ToolButtonMenu);

    QStyleOption separator;
    separator.palette = toolbar.palette();
    separator.rect = QRect(0, 0, 24, 24);
    separator.state = QStyle::State_Enabled | QStyle::State_Horizontal;
    QImage horizontalToolbarSeparator(separator.rect.size(), QImage::Format_ARGB32_Premultiplied);
    horizontalToolbarSeparator.fill(Qt::transparent);
    {
        QPainter painter(&horizontalToolbarSeparator);
        toolbar.style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &separator, &painter,
                                       &toolbar);
    }
    separator.state = QStyle::State_Enabled;
    QImage verticalToolbarSeparator(separator.rect.size(), QImage::Format_ARGB32_Premultiplied);
    verticalToolbarSeparator.fill(Qt::transparent);
    {
        QPainter painter(&verticalToolbarSeparator);
        toolbar.style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &separator, &painter,
                                       &toolbar);
    }
    QVERIFY(horizontalToolbarSeparator != verticalToolbarSeparator);

    QStyleOptionToolButton hoverOption;
    hoverOption.initFrom(tool);
    hoverOption.rect = tool->rect();
    hoverOption.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    const QColor toolbarSurface = toolbar.palette().color(QPalette::Window);
    QImage hoverTrail(tool->size(), QImage::Format_ARGB32_Premultiplied);
    hoverTrail.fill(toolbarSurface);
    WinUI3::Private::framePropertyRegistry().set(tool, "_winui_hover_progress", 1.0);
    {
        QPainter painter(&hoverTrail);
        tool->style()->drawPrimitive(QStyle::PE_PanelButtonTool, &hoverOption, &painter, tool);
    }
    QVERIFY(hoverTrail.pixelColor(tool->rect().center()) != toolbarSurface);
    hoverOption.state = QStyle::State_Enabled;
    WinUI3::Private::framePropertyRegistry().set(tool, "_winui_hover_progress", 0.0);
    {
        QPainter painter(&hoverTrail);
        tool->style()->drawPrimitive(QStyle::PE_PanelButtonTool, &hoverOption, &painter, tool);
    }
    QCOMPARE(hoverTrail.pixelColor(tool->rect().center()), toolbarSurface);
}

void WinUI3ButtonsTest::buttonPressedStateFollowsQtState()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ControlRole role :
         { WinUI3::ControlRole::Standard, WinUI3::ControlRole::Subtle }) {
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
        const QImage pressed =
                render(QStyle::State_Enabled | QStyle::State_MouseOver | QStyle::State_Sunken);
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

void WinUI3ButtonsTest::buttonPressedForegroundRoles()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        style->setThemeMode(mode);
        const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(style->standardPalette());
        QCOMPARE(t.textOnAccentSecondary,
                 mode == WinUI3::ThemeMode::Dark ? QColor(0, 0, 0, 128)
                                                 : QColor(255, 255, 255, 179));

        for (const WinUI3::ControlRole role :
             { WinUI3::ControlRole::Standard, WinUI3::ControlRole::Accent,
               WinUI3::ControlRole::Subtle, WinUI3::ControlRole::Destructive }) {
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
                QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
                QPainter painter(&image);
                style->drawControl(QStyle::CE_PushButtonLabel, &option, &painter, &button);
                return image;
            };
            const QImage normal = renderLabel(QStyle::State_Enabled);
            const QImage pressed = renderLabel(QStyle::State_Enabled | QStyle::State_Sunken);
            QVERIFY2(normal != pressed,
                     qPrintable(QStringLiteral("pressed foreground unchanged for mode=%1 role=%2")
                                        .arg(int(mode))
                                        .arg(int(role))));
        }
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3ButtonsTest::coloredIconCacheReuseAndPixelContract()
{
    QVERIFY(WinUI3::icon(static_cast<WinUI3::Icon>(-1)).isNull());
    QVERIFY(WinUI3::icon(static_cast<WinUI3::Icon>(999)).isNull());

    const QColor foreground(27, 108, 219, 231);
    const QSize logicalSize(20, 20);
    // WinUI3::Private::iconPixmap wraps QIcon::pixmap(QSize, qreal, Mode, State)
    // (Qt 6 only) with a Qt 5.12 fallback; one call path, both toolchains.
    const qreal devicePixelRatio = 1.5;

    const QIcon first = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    const QPixmap firstPixmap = WinUI3::Private::iconPixmap(first, logicalSize, devicePixelRatio,
                                                            QIcon::Normal, QIcon::Off);
    QVERIFY(!firstPixmap.isNull());

    const auto paintDirect = [logicalSize](const QIcon &source) {
        QImage image(logicalSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        source.paint(&painter, QRect(QPoint(), logicalSize), Qt::AlignCenter, QIcon::Normal,
                     QIcon::Off);
        return image;
    };
    const QImage firstDirectPaint = paintDirect(first);
    QVERIFY(!firstDirectPaint.isNull());

    // A repeated request for the same glyph/color must reuse the cached
    // engine. cacheKey equality is the allocation/lifetime proxy; comparing
    // every raster state guards the public output at the same time.
    const QIcon second = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    QCOMPARE(first.cacheKey(), second.cacheKey());
    for (const QIcon::Mode mode :
         { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected }) {
        for (const QIcon::State state : { QIcon::Off, QIcon::On }) {
            QCOMPARE(WinUI3::Private::iconPixmap(first, logicalSize, devicePixelRatio, mode, state)
                             .toImage(),
                     WinUI3::Private::iconPixmap(second, logicalSize, devicePixelRatio, mode, state)
                             .toImage());
        }
    }

    QCOMPARE(firstPixmap.devicePixelRatio(), devicePixelRatio);
    QVERIFY(first.cacheKey()
            != WinUI3::icon(WinUI3::Icon::ChevronDown, QColor(28, 108, 219, 231)).cacheKey());

    // Force the bounded cache past its capacity and compare a newly-created
    // engine with the original. This catches accidental changes to the
    // QIconEngine raster path while exercising the eviction policy.
    for (int index = 0; index < 300; ++index) {
        const QColor uniqueColor((index * 53) % 256, (index * 97) % 256, (index * 193) % 256,
                                 200 + (index % 56));
        (void)WinUI3::icon(WinUI3::Icon::ChevronDown, uniqueColor);
    }
    const QIcon rebuilt = WinUI3::icon(WinUI3::Icon::ChevronDown, foreground);
    QVERIFY(rebuilt.cacheKey() != first.cacheKey());
    QCOMPARE(WinUI3::Private::iconPixmap(rebuilt, logicalSize, devicePixelRatio, QIcon::Normal,
                                         QIcon::Off)
                     .toImage(),
             firstPixmap.toImage());
    QCOMPARE(paintDirect(rebuilt), firstDirectPaint);
}

void WinUI3ButtonsTest::iconPixmapCacheDprAndPalette()
{
    const QIcon fluent = WinUI3::icon(WinUI3::Icon::Search);
    const QColor foreground(30, 110, 220, 211);
    const QPixmap first = WinUI3::iconPixmap(fluent, QSize(20, 20), 1.5, foreground);
    const QPixmap second = WinUI3::iconPixmap(fluent, QSize(20, 20), 1.5, foreground);
    QVERIFY(!first.isNull());
    QCOMPARE(first.devicePixelRatioF(), 1.5);
    QCOMPARE(first.size(), QSize(30, 30));
    QCOMPARE(first.toImage(), second.toImage());

    const QPixmap differentDpr = WinUI3::iconPixmap(fluent, QSize(20, 20), 2.0, foreground);
    QVERIFY(!differentDpr.isNull());
    QCOMPARE(differentDpr.devicePixelRatioF(), 2.0);
    QCOMPARE(differentDpr.size(), QSize(40, 40));

    const QPixmap disabled =
            WinUI3::iconPixmap(fluent, QSize(20, 20), 1.5, foreground, QIcon::Disabled, QIcon::On);
    QVERIFY(!disabled.isNull());
    QCOMPARE(disabled.size(), QSize(30, 30));

    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    // Tokens derive from palette roles, so probe palettes must carry a
    // matching ink alongside the surface color, exactly like the light/dark
    // standard palettes do.
    auto renderArrow = [style](const QColor &windowColor, const QColor &inkColor) {
        QStyleOption option;
        option.rect = QRect(0, 0, 24, 24);
        option.state = QStyle::State_Enabled;
        option.palette = qApp->palette();
        option.palette.setColor(QPalette::Window, windowColor);
        option.palette.setColor(QPalette::WindowText, inkColor);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &option, &painter);
        return image;
    };
    const QImage light = renderArrow(Qt::white, QColor(0, 0, 0, 228));
    const QImage dark = renderArrow(Qt::black, QColor(255, 255, 255));
    QVERIFY(light != dark);
}

void WinUI3ButtonsTest::buttonPressedPulseContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ControlRole role :
         { WinUI3::ControlRole::Standard, WinUI3::ControlRole::Accent, WinUI3::ControlRole::Subtle,
           WinUI3::ControlRole::Destructive }) {
        QPushButton button(QStringLiteral("Rapid"));
        WinUI3::Style::setControlRole(&button, role);
        button.resize(120, 32);
        button.show();
        (void)QTest::qWaitForWindowExposed(&button);
        QTest::mouseMove(&button, button.rect().center());
        const QImage rest = button.grab().toImage();

        for (int click = 0; click < 5; ++click) {
            QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
            qApp->processEvents();
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 1.0);
            const QImage pressed = button.grab().toImage();
            QVERIFY2(pressed != rest, "a left press must produce a visible frame");
            QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
        }
        QTest::qWait(130);
        QVERIFY(frameReal(&button, "_winui_press_progress") < 0.1);

        QTest::mousePress(&button, Qt::RightButton, Qt::NoModifier, button.rect().center());
        QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);
        QTest::mouseRelease(&button, Qt::RightButton, Qt::NoModifier, button.rect().center());
        QTest::mousePress(&button, Qt::MiddleButton, Qt::NoModifier, button.rect().center());
        QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);
        QTest::mouseRelease(&button, Qt::MiddleButton, Qt::NoModifier, button.rect().center());
    }

    QToolButton tool;
    tool.setText(QStringLiteral("Rapid tool"));
    tool.resize(120, 32);
    tool.show();
    (void)QTest::qWaitForWindowExposed(&tool);
    QTest::mouseMove(&tool, tool.rect().center());
    const QImage toolRest = tool.grab().toImage();
    QTest::mousePress(&tool, Qt::LeftButton, Qt::NoModifier, tool.rect().center());
    qApp->processEvents();
    QCOMPARE(frameReal(&tool, "_winui_press_progress"), 1.0);
    QVERIFY(tool.grab().toImage() != toolRest);
    QTest::mouseRelease(&tool, Qt::LeftButton, Qt::NoModifier, tool.rect().center());
    QTest::qWait(130);
    QVERIFY(frameReal(&tool, "_winui_press_progress") < 0.1);
}

void WinUI3ButtonsTest::commandLinkButtonContract()
{
    QCommandLinkButton command(QStringLiteral("Open advanced settings"),
                               QStringLiteral("Configure optional features."));
    // QCommandLinkButton::sizeHint() is protected in Qt 5.12: size from the
    // style contract (160x64 minimum) instead of calling it directly.
    QStyleOptionButton option;
    option.initFrom(&command);
    option.text = command.text();
    const QSize hint =
            command.style()->sizeFromContents(QStyle::CT_PushButton, &option, QSize(), &command);
    command.resize(qMax(320, hint.width()), qMax(96, hint.height()));
    command.show();
    QVERIFY(QTest::qWaitForWindowExposed(&command));
    const QImage enabled = command.grab().toImage();
    command.setEnabled(false);

    QCommandLinkButton withoutDescription(QStringLiteral("Open advanced settings"));
    withoutDescription.resize(command.size());
    withoutDescription.show();
    QVERIFY(QTest::qWaitForWindowExposed(&withoutDescription));
    QVERIFY(withoutDescription.grab().toImage() != enabled);
}

void WinUI3ButtonsTest::disabledButtonHasNoInteractionState()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        style->setThemeMode(mode);
        for (const WinUI3::ControlRole role :
             { WinUI3::ControlRole::Standard, WinUI3::ControlRole::Subtle }) {
            QPushButton button(QStringLiteral("Disabled"));
            WinUI3::Style::setControlRole(&button, role);
            button.resize(140, 32);
            button.show();
            (void)QTest::qWaitForWindowExposed(&button);
            QTest::mouseMove(&button, button.rect().center());
            QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
            button.setEnabled(false);
            qApp->processEvents();
            QCOMPARE(frameReal(&button, "_winui_hover_progress"), 0.0);
            QCOMPARE(frameReal(&button, "_winui_press_progress"), 0.0);

            QEvent enter(QEvent::Enter);
            QCoreApplication::sendEvent(&button, &enter);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(button.rect().center()),
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
                style->drawControl(QStyle::CE_PushButton, &option, &painter, &button);
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
        QTest::mousePress(&tool, Qt::LeftButton, Qt::NoModifier, tool.rect().center());
        tool.setEnabled(false);
        qApp->processEvents();
        QCOMPARE(frameReal(&tool, "_winui_hover_progress"), 0.0);
        QCOMPARE(frameReal(&tool, "_winui_press_progress"), 0.0);
        QTest::qWait(220);
        QCOMPARE(frameReal(&tool, "_winui_hover_progress"), 0.0);
        QCOMPARE(frameReal(&tool, "_winui_press_progress"), 0.0);
    }
}

void WinUI3ButtonsTest::toolButtonIconVerticalCenter()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QPixmap sourcePixmap(16, 16);
    sourcePixmap.fill(QColor(220, 30, 40));
    const QIcon sourceIcon(sourcePixmap);
    for (const qreal dpr : { 1.0, 1.25, 1.5, 2.0 }) {
        for (const QSize size : { QSize(37, 33), QSize(33, 37) }) {
            QToolBar toolbar;
            toolbar.setOrientation(size.width() > size.height() ? Qt::Horizontal : Qt::Vertical);
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

            QImage image(qRound(size.width() * dpr), qRound(size.height() * dpr),
                         QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(dpr);
            image.fill(Qt::transparent);
            {
                QPainter painter(&image);
                style->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, &button);
            }

            qreal sumX = 0.0;
            qreal sumY = 0.0;
            qreal weight = 0.0;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const QColor pixel = image.pixelColor(x, y);
                    if (pixel.red() < 150 || pixel.green() > 100 || pixel.blue() > 100
                        || pixel.alpha() == 0)
                        continue;
                    const qreal alpha = pixel.alphaF();
                    sumX += (x + 0.5) / dpr * alpha;
                    sumY += (y + 0.5) / dpr * alpha;
                    weight += alpha;
                }
            }
            QVERIFY(weight > 0.0);
            const QRectF content = QRectF(option.rect).adjusted(4.0, 2.0, -4.0, -2.0);
            QVERIFY(qAbs(sumX / weight - content.center().x()) <= 0.5);
            QVERIFY(qAbs(sumY / weight - content.center().y()) <= 0.5);
        }
    }
}

void WinUI3ButtonsTest::toolbarButtonCornerSymmetry()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QToolBar toolbar;
    toolbar.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar.setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("layer"));
    QAction *before = toolbar.addAction(QStringLiteral("Before"));
    QAction *checked = toolbar.addAction(QStringLiteral("Checked"));
    QAction *after = toolbar.addAction(QStringLiteral("After"));
    Q_UNUSED(before);
    Q_UNUSED(after);
    checked->setCheckable(true);
    checked->setChecked(true);
    toolbar.show();
    (void)QTest::qWaitForWindowExposed(&toolbar);

    auto *button = qobject_cast<QToolButton *>(toolbar.widgetForAction(checked));
    QVERIFY(button);
    const QColor background = toolbar.palette().color(QPalette::Window);
    const auto cornerInk = [&](const QImage &image, bool right, bool bottom) {
        int count = 0;
        for (int y = 0; y < 6; ++y) {
            for (int x = 0; x < 6; ++x) {
                const int px = right ? image.width() - 1 - x : x;
                const int py = bottom ? image.height() - 1 - y : y;
                if (colorDistance(image.pixelColor(px, py), background) > 4)
                    ++count;
            }
        }
        return count;
    };
    const auto verifyCorners = [&](const QImage &image) {
        QCOMPARE(cornerInk(image, false, false), cornerInk(image, true, false));
        QCOMPARE(cornerInk(image, false, true), cornerInk(image, true, true));
        QCOMPARE(cornerInk(image, false, false), cornerInk(image, false, true));
    };
    verifyCorners(button->grab().toImage());

    QTest::mouseMove(button, button->rect().center());
    setFrame(button, "_winui_hover_progress", 1.0);
    button->update();
    qApp->processEvents();
    const QImage hovered = button->grab().toImage();
    verifyCorners(hovered);

    QTest::mousePress(button, Qt::LeftButton, Qt::NoModifier, button->rect().center());
    qApp->processEvents();
    const QImage pressed = button->grab().toImage();
    verifyCorners(pressed);
    const QPoint fillProbe(2, button->height() / 2);
    QVERIFY2(colorDistance(hovered.pixelColor(fillProbe), pressed.pixelColor(fillProbe)) > 2,
             "a checked toolbar button must retain visible press feedback");
    QTest::mouseRelease(button, Qt::LeftButton, Qt::NoModifier, button->rect().center());

    // Exercise the real command-bar composition path.  Grabbing the child
    // directly repaints it into a fresh pixmap and can hide clipping caused
    // by a translucent top-level backing store.
    QMainWindow host;
    host.setProperty("_winui_backdrop", 1);
    auto *micaToolbar = new QToolBar(&host);
    micaToolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    host.addToolBar(micaToolbar);
    QAction *leftAction = micaToolbar->addAction(QStringLiteral("Left"));
    QAction *middleAction = micaToolbar->addAction(QStringLiteral("Middle"));
    QAction *rightAction = micaToolbar->addAction(QStringLiteral("Right"));
    host.resize(480, 160);
    host.show();
    (void)QTest::qWaitForWindowExposed(&host);
    auto *micaButton = qobject_cast<QToolButton *>(micaToolbar->widgetForAction(middleAction));
    QVERIFY(micaButton);
    const QRect leftGeometry = micaToolbar->widgetForAction(leftAction)->geometry();
    const QRect rightGeometry = micaToolbar->widgetForAction(rightAction)->geometry();
    QVERIFY2(!leftGeometry.intersects(micaButton->geometry()),
             "toolbar action widgets must not overlap");
    QVERIFY2(!micaButton->geometry().intersects(rightGeometry),
             "toolbar action widgets must not overlap");
    QVERIFY2(micaToolbar->rect().contains(micaButton->geometry()),
             "toolbar action widgets must stay inside the toolbar paint area");
    QTest::mouseMove(micaButton, micaButton->rect().center());
    setFrame(micaButton, "_winui_hover_progress", 1.0);
    micaButton->update();
    qApp->processEvents();
    const QImage composed = micaToolbar->grab(micaButton->geometry()).toImage();
    const QImage isolated = micaButton->grab().toImage();
    QCOMPARE(composed, isolated);
}

void WinUI3ButtonsTest::controlRoles()
{
    QPushButton button;
    QCOMPARE(WinUI3::Style::controlRole(&button), WinUI3::ControlRole::Standard);
    WinUI3::Style::setControlRole(&button, WinUI3::ControlRole::Accent);
    QCOMPARE(WinUI3::Style::controlRole(&button), WinUI3::ControlRole::Accent);
}

QTEST_MAIN(WinUI3ButtonsTest)
#include "tst_winui3buttons.moc"
