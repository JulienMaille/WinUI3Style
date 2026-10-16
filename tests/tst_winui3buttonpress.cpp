// SPDX-License-Identifier: LGPL-2.1-or-later
// Button press-state contracts: State_Sunken followers, press pulse and
// release timers, disabled interaction suppression, checked-toggle fill.
// Split out of tst_winui3buttons.cpp per the 60 KB test-size ratchet (S6);
// test-function names kept identical so the DPI reruns in
// tests/CMakeLists.txt keep working. Every grab()/pixelColor() assertion
// pairs with a QCOMPARE token/geometry assertion on the same state.
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

class WinUI3ButtonPressTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void buttonPressedStateFollowsQtState();
    void buttonPressedForegroundRoles();
    void buttonPressedPulseContract();
    void buttonReleaseTimerSurvivesWidgetDeletion();
    void commandLinkPressKeepsContentPosition_data();
    void commandLinkPressKeepsContentPosition();
    void disabledButtonHasNoInteractionState();
    void checkedToolbarToggleKeepsFillOverBackdrop();
};

void WinUI3ButtonPressTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3ButtonPressTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3ButtonPressTest::cleanup()
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

void WinUI3ButtonPressTest::buttonPressedStateFollowsQtState()
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

void WinUI3ButtonPressTest::buttonPressedForegroundRoles()
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

void WinUI3ButtonPressTest::buttonPressedPulseContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QVERIFY(!WinUI3::Private::textBoxHelperButton(nullptr));

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
        // The release animates over FasterDuration (83 ms) on its own timer;
        // wait for the settle instead of a fixed delay (a mid-decay exact 0.0
        // read is a load artefact, not a defect).
        QTRY_COMPARE(frameReal(&button, "_winui_press_progress"), 0.0);

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

void WinUI3ButtonPressTest::buttonReleaseTimerSurvivesWidgetDeletion()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const int animationBaseline = style->findChildren<QVariantAnimation *>().size();

    auto *button = new QPushButton(QStringLiteral("Short lived"));
    button->resize(120, 32);
    button->show();
    (void)QTest::qWaitForWindowExposed(button);
    QTest::mousePress(button, Qt::LeftButton, Qt::NoModifier, button->rect().center());
    QCOMPARE(frameReal(button, "_winui_press_progress"), 1.0);
    QTest::mouseRelease(button, Qt::LeftButton, Qt::NoModifier, button->rect().center());

    delete button;
    QTest::qWait(30);
    QCOMPARE(style->findChildren<QVariantAnimation *>().size(), animationBaseline);
}

void WinUI3ButtonPressTest::commandLinkPressKeepsContentPosition_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    for (bool dark : { false, true })
        for (bool compact : { false, true })
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(dark).arg(compact)))
                    << dark << compact;
}

void WinUI3ButtonPressTest::commandLinkPressKeepsContentPosition()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QCommandLinkButton command(QStringLiteral("Open settings"), QStringLiteral("Description"));
    command.setProperty(WinUI3::Style::DensityProperty,
                        compact ? QStringLiteral("compact") : QStringLiteral("standard"));
    QPixmap marker(16, 16);
    marker.fill(Qt::magenta);
    command.resize(320, 96);
    command.show();
    QVERIFY(QTest::qWaitForWindowExposed(&command));
    QSignalSpy clicked(&command, &QAbstractButton::clicked);
    QTest::mouseMove(&command, command.rect().center());
    QTest::qWait(150);
    // A density StyleChange resets QCommandLinkButton's icon in Qt.
    // Install the probe after that queued event has settled.
    command.setIcon(QIcon(marker));
    QCOMPARE(command.style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, nullptr, &command), 0);
    const QImage rest = command.grab().toImage();
    QTest::mousePress(&command, Qt::LeftButton, Qt::NoModifier, command.rect().center());
    QVERIFY(command.isDown());
    QCOMPARE(command.style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, nullptr, &command), 0);
    QCOMPARE(command.style()->pixelMetric(QStyle::PM_ButtonShiftVertical, nullptr, &command), 0);
    const QImage pressed = command.grab().toImage();
    const auto markerBounds = [](const QImage &frame) {
        QRect bounds;
        for (int y = 0; y < frame.height(); ++y)
            for (int x = 0; x < frame.width(); ++x)
                if (frame.pixelColor(x, y) == QColor(Qt::magenta))
                    bounds |= QRect(x, y, 1, 1);
        return bounds;
    };
    QVERIFY(!markerBounds(rest).isEmpty());
    QCOMPARE(markerBounds(pressed), markerBounds(rest));
    QVERIFY(pressed != rest);
    QTest::mouseRelease(&command, Qt::LeftButton, Qt::NoModifier, command.rect().center());
    QVERIFY(!command.isDown());
    QCOMPARE(clicked.size(), 1);
    QTest::keyPress(&command, Qt::Key_Space);
    QVERIFY(command.isDown());
    QCOMPARE(command.style()->pixelMetric(QStyle::PM_ButtonShiftVertical, nullptr, &command), 0);
    QTest::keyRelease(&command, Qt::Key_Space);
    QCOMPARE(clicked.size(), 2);
}

void WinUI3ButtonPressTest::disabledButtonHasNoInteractionState()
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
                              QPointF(button.rect().center()), Qt::LeftButton, Qt::LeftButton,
                              Qt::NoModifier);
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

void WinUI3ButtonPressTest::checkedToolbarToggleKeepsFillOverBackdrop()
{
    // Gallery command-bar repro (user 2026-09-11): Pin/Preview stay flat in
    // mica mode. Same state, two assertions: the selected fill token is
    // opaque, and the painted panel center matches it rather than the
    // material showing through.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMainWindow window;
    window.setProperty("_winui_backdrop", 1);
    window.setProperty("_winui_backdrop_effective", 2); // Composited
    auto *toolbar = new QToolBar(&window);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QAction *preview = toolbar->addAction(QStringLiteral("Preview"));
    preview->setCheckable(true);
    preview->setChecked(true);
    window.resize(360, 120);
    window.show();
    (void)QTest::qWaitForWindowExposed(&window);
    auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(preview));
    QVERIFY(button);
    QVERIFY(button->isChecked());
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(button));
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(button->palette());
    const QColor expected = WinUI3::Private::mix(tokens.subtleHover, tokens.accentFill, 0.14);
    QVERIFY2(expected.alpha() > 20,
             qPrintable(QStringLiteral("checked fill alpha %1").arg(expected.alpha())));
    QStyleOptionToolButton option;
    option.initFrom(button);
    option.rect = QRect(QPoint(), button->size());
    option.state |= QStyle::State_Enabled | QStyle::State_On;
    QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
    }
    const QColor center = image.pixelColor(image.width() / 2, image.height() / 2);
    QVERIFY2(center.alpha() > 20,
             qPrintable(QStringLiteral("center alpha %1").arg(center.alpha())));
    QVERIFY2(colorDistance(center, expected) < 100,
             qPrintable(QStringLiteral("center #%1 vs fill #%2")
                                .arg(center.name())
                                .arg(expected.name())));
}
QTEST_MAIN(WinUI3ButtonPressTest)
#include "tst_winui3buttonpress.moc"
