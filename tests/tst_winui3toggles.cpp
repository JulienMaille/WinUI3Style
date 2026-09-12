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

class WinUI3TogglesTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void toggleConvenienceWidget();
    void toggleInteraction();
    void togglePressedThumbGeometry();
    void toggleDragInteraction();
    void toggleRtlGeometryAndInteraction();
    void checkboxAcceptAnimation();
    void checkboxGlyphGeometryContract();
    void lightModeIndicatorOnAccentIsWhite();
    void darkModeIndicatorOnAccentIsBlack();
    void customAccentKeepsThemeTextSeparateFromControlInk();
    void checkboxGapHitTest();
    void checkboxDisabledStopsAnimation();
    void radioStateMotion();
    void radioRapidClickResponsiveness();
    void radioDotDpiGeometry();
    void checkboxAndRadioUncheckMotion();
};

void WinUI3TogglesTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3TogglesTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3TogglesTest::cleanup()
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

void WinUI3TogglesTest::toggleConvenienceWidget()
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
    const qreal beforeTextChange = frameReal(&toggle, "_winui_toggle_position");
    QVERIFY(beforeTextChange > 0.0 && beforeTextChange < 0.99);
    toggle.setOnText(QStringLiteral("Enabled"));
    const qreal afterTextChange = frameReal(&toggle, "_winui_toggle_position");
    QVERIFY(afterTextChange < 0.99);
    QVERIFY(std::abs(afterTextChange - beforeTextChange) < 0.15);
    QTRY_VERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.99);
}

void WinUI3TogglesTest::toggleInteraction()
{
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    WinUI3::Style::setToggleSwitchText(&toggle, QStringLiteral("On"), QStringLiteral("Off"));
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

void WinUI3TogglesTest::togglePressedThumbGeometry()
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
    option.state = QStyle::State_Enabled | QStyle::State_On | QStyle::State_MouseOver
            | QStyle::State_Sunken;
    QImage image(toggle.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        toggle.style()->drawControl(QStyle::CE_CheckBox, &option, &painter, &toggle);
    }

    const QRectF track(option.rect.left(), option.rect.center().y() - 10, 40, 20);
    QRect whiteInk;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() > 200 && pixel.red() > 240 && pixel.green() > 240
                && pixel.blue() > 240)
                whiteInk |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(!whiteInk.isEmpty());
    QVERIFY(whiteInk.width() <= 17);
    QVERIFY(whiteInk.height() <= 14);
    QVERIFY2(track.right() - whiteInk.right() >= 3.0,
             "pressed on-thumb must retain WinUI's trailing inset");
}

void WinUI3TogglesTest::toggleDragInteraction()
{
    QCheckBox toggle;
    toggle.setProperty(WinUI3::Style::ToggleSwitchProperty, true);
    toggle.setProperty(WinUI3::Style::ToggleSwitchOnTextProperty, QStringLiteral("On"));
    toggle.setProperty(WinUI3::Style::ToggleSwitchOffTextProperty, QStringLiteral("Off"));
    toggle.resize(toggle.sizeHint());
    toggle.show();
    QVERIFY(WinUI3::Style::isToggleSwitch(&toggle));
    QCOMPARE(toggle.style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, &toggle), 40);

    QSignalSpy clicked(&toggle, &QAbstractButton::clicked);
    QTest::mousePress(&toggle, Qt::LeftButton, Qt::NoModifier, QPoint(10, 20));
    QMouseEvent move(QEvent::MouseMove, QPointF(32, 20), QPointF(32, 20), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &move);
    QVERIFY(frameBool(&toggle, "_winui_toggle_dragging"));
    QVERIFY(frameReal(&toggle, "_winui_toggle_position") > 0.9);

    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(32, 20), QPointF(32, 20), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &release);
    QVERIFY(toggle.isChecked());
    QVERIFY(!frameBool(&toggle, "_winui_toggle_dragging"));
    QCOMPARE(clicked.count(), 1);
}

void WinUI3TogglesTest::toggleRtlGeometryAndInteraction()
{
    DisableAnimationsGuard animations;
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    WinUI3::Style::setToggleSwitchText(&toggle, QStringLiteral("On"), QStringLiteral("Off"));
    toggle.setLayoutDirection(Qt::RightToLeft);
    toggle.resize(140, 32);
    toggle.setChecked(true);
    toggle.show();
    QVERIFY(QTest::qWaitForWindowExposed(&toggle));

    // QRect::right() is inclusive. The RTL track must therefore start at
    // right - 39, exactly mirroring the LTR 40-pixel slot. The old right - 40
    // origin left one stale pixel outside the widget and disagreed with the
    // drag hit region.
    const QRect expectedTrack(toggle.rect().right() - 39, toggle.rect().center().y() - 10, 40, 20);
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
        toggle.style()->drawControl(QStyle::CE_CheckBox, &option, &painter, &toggle);
    }
    // QPalette::Accent exists from Qt 6.6; on Qt 5.12 the selection accent
    // (QPalette::Highlight) is the equivalent role.
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QColor accent = toggle.palette().color(QPalette::Accent);
#else
    const QColor accent = toggle.palette().color(QPalette::Highlight);
#endif
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.center()), accent) < 100);
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.left() - 1, expectedTrack.center().y()),
                          background)
            < 2);
    QVERIFY(colorDistance(image.pixelColor(expectedTrack.right(), expectedTrack.center().y()),
                          accent)
            < 100);

    // A click anywhere in the visual track remains a normal checkbox click.
    toggle.setChecked(false);
    QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier, expectedTrack.center());
    QVERIFY(toggle.isChecked());

    // In RTL, the unchecked knob is on the right and a drag toward the left
    // must turn the switch on. This exercises the same 40 x 20 rect used by
    // the renderer, including its inclusive right edge.
    toggle.setChecked(false);
    QCoreApplication::processEvents();
    const QPoint offKnob(expectedTrack.right() - 10, expectedTrack.center().y());
    const QPoint onKnob(expectedTrack.left() + 10, expectedTrack.center().y());
    QTest::mousePress(&toggle, Qt::LeftButton, Qt::NoModifier, offKnob);
    QMouseEvent move(QEvent::MouseMove, QPointF(onKnob), QPointF(onKnob), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &move);
    QVERIFY(frameBool(&toggle, "_winui_toggle_dragging"));
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(onKnob), QPointF(onKnob), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&toggle, &release);
    QVERIFY(toggle.isChecked());
    QVERIFY(!frameBool(&toggle, "_winui_toggle_dragging"));
}

void WinUI3TogglesTest::checkboxAcceptAnimation()
{
    QCheckBox check(QStringLiteral("Animated accept"));
    check.resize(check.sizeHint());
    check.show();

    const auto renderIndicator = [&](qreal progress, bool checked) {
        setFrame(&check, "_winui_check_progress", progress);
        QStyleOptionButton option;
        option.initFrom(&check);
        option.rect = QRect(0, 0, 20, 20);
        option.state = QStyle::State_Enabled | (checked ? QStyle::State_On : QStyle::State_Off);
        QImage image(20, 20, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        check.style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &check);
        return image;
    };
    const QImage checkedAtStart = renderIndicator(0.0, true);
    const QImage checkedAtEnd = renderIndicator(1.0, true);
    const QImage unchecked = renderIndicator(0.0, false);
    // Checked fill/stroke are discrete in the WinUI template; the corner is
    // outside the accept path and therefore must not fade with glyph progress.
    QCOMPARE(checkedAtStart.pixelColor(2, 2), checkedAtEnd.pixelColor(2, 2));
    QVERIFY(checkedAtStart.pixelColor(2, 2) != unchecked.pixelColor(2, 2));
    // The official transition starts at frame 15, not frame 0: there is no
    // second hold after AnimatedIcon has jumped to its Start marker.
    QVERIFY(renderIndicator(0.05, true) != checkedAtStart);

    setFrame(&check, "_winui_check_progress", 0.0);
    check.setChecked(true);
    QTest::qWait(55);
    const qreal midway = frameReal(&check, "_winui_check_progress");
    QVERIFY2(midway > 0.0 && midway < 1.0, qPrintable(QString::number(midway)));
    // After the initial generated hold, the accept stroke must still be in a
    // visibly partial state instead of completing in an imperceptible ~20 ms.
    QVERIFY2(midway < 0.90, qPrintable(QString::number(midway)));
    QTRY_VERIFY(frameReal(&check, "_winui_check_progress") > 0.99);
}

void WinUI3TogglesTest::checkboxGlyphGeometryContract()
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
        check.style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, &painter, &check);
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
    QCOMPARE(check.style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, &check), 20);
    QCOMPARE(check.style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, &check), 20);
    const int checkTextWidth = check.fontMetrics().size(Qt::TextShowMnemonic, check.text()).width();
    QVERIFY(check.sizeHint().width() >= checkTextWidth + 36);
    QRadioButton radio(QStringLiteral("Radio"));
    const int radioTextWidth = radio.fontMetrics().size(Qt::TextShowMnemonic, radio.text()).width();
    QVERIFY(radio.sizeHint().width() >= radioTextWidth + 36);
    QVERIFY(bounds.width() >= 19);
    QVERIFY(bounds.height() >= 19);
    QVERIFY(bounds.width() <= 21);
    QVERIFY(bounds.height() <= 21);
}

void WinUI3TogglesTest::lightModeIndicatorOnAccentIsWhite()
{
    // AccentFillColorDefault is intentionally lighter than the raw system
    // accent in light mode. Its luminance must not make WinUI's control glyphs
    // switch to black: checked indicators and the toggle knob use white ink.
    const QColor white(Qt::white);
    const auto hasWhitePixel = [&white](const QImage &image, const QRect &rect) {
        for (int y = rect.top(); y <= rect.bottom(); ++y) {
            for (int x = rect.left(); x <= rect.right(); ++x) {
                if (image.rect().contains(x, y) && colorDistance(image.pixelColor(x, y), white) < 8)
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
        check.style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOption, &painter, &check);
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
        radio.style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &radioOption, &painter,
                                     &radio);
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
        check.style()->drawControl(QStyle::CE_CheckBox, &toggleOption, &painter, &check);
    }
    const QRect toggleTrack(check.rect().left(), check.rect().center().y() - 10, 40, 20);
    const QRect toggleKnob(toggleTrack.right() - 16, toggleTrack.center().y() - 8, 16, 16);
    QVERIFY2(hasWhitePixel(toggleImage, toggleKnob),
             "light checked toggle has no white WinUI on-knob");
}

void WinUI3TogglesTest::darkModeIndicatorOnAccentIsBlack()
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
    const auto renderIndicator = [&](QStyle::PrimitiveElement element, QAbstractButton &button) {
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
    const QRect track(toggle.rect().left(), toggle.rect().center().y() - 10, 40, 20);
    const QRect knob(track.right() - 16, track.center().y() - 8, 16, 16);
    QVERIFY2(hasBlackPixel(image, knob), "dark checked toggle has no black WinUI on-knob");
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3TogglesTest::customAccentKeepsThemeTextSeparateFromControlInk()
{
    QPalette palette = qApp->palette();
    const QColor paleAccent(255, 240, 0);
    palette.setColor(QPalette::Highlight, paleAccent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    palette.setColor(QPalette::Accent, paleAccent);
#endif
    const WinUI3::Private::Tokens t = WinUI3::Private::buildTokens(palette);

    // WinUI resolves both roles from the theme; in Light they are white.
    QCOMPARE(t.textOnAccentPrimary, QColor(Qt::white));
    QCOMPARE(t.controlOnAccentPrimary, QColor(Qt::white));
}

void WinUI3TogglesTest::checkboxGapHitTest()
{
    QCheckBox check(QStringLiteral("A label with a real hit gap"));
    check.resize(check.sizeHint());
    check.show();
    QTRY_VERIFY(check.isVisible());

    QStyleOptionButton option;
    option.initFrom(&check);
    const QRect indicator =
            check.style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, &check);
    const QRect contents =
            check.style()->subElementRect(QStyle::SE_CheckBoxContents, &option, &check);
    const QRect clickRect =
            check.style()->subElementRect(QStyle::SE_CheckBoxClickRect, &option, &check);
    QVERIFY(indicator.right() + 1 < contents.left());
    const QPoint gap((indicator.right() + contents.left()) / 2, indicator.center().y());
    QVERIFY(check.rect().contains(gap));
    QVERIFY(clickRect.contains(gap));

    QSignalSpy clicked(&check, &QAbstractButton::clicked);
    QTest::mouseClick(&check, Qt::LeftButton, Qt::NoModifier, gap);
    QCOMPARE(clicked.count(), 1);
    QVERIFY(check.isChecked());
}

void WinUI3TogglesTest::checkboxDisabledStopsAnimation()
{
    QCheckBox check(QStringLiteral("Disabled during transition"));
    check.resize(180, 32);
    check.show();
    check.setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(&check, "_winui_check_progress") > 0.0, 150);
    QVERIFY(frameReal(&check, "_winui_check_progress") < 1.0);

    check.setEnabled(false);
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 1.0);
    QTest::qWait(230);
    QCOMPARE(frameReal(&check, "_winui_check_progress"), 1.0);
    QCOMPARE(frameReal(&check, "_winui_hover_progress"), 0.0);
    QCOMPARE(frameReal(&check, "_winui_press_progress"), 0.0);
}

void WinUI3TogglesTest::radioStateMotion()
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

void WinUI3TogglesTest::radioRapidClickResponsiveness()
{
    QWidget host;
    host.resize(180, 72);
    auto *one = new QRadioButton(QStringLiteral("One"), &host);
    auto *two = new QRadioButton(QStringLiteral("Two"), &host);
    one->setGeometry(0, 0, 160, 32);
    two->setGeometry(0, 36, 160, 32);
    one->setChecked(true);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    QSignalSpy oneClicked(one, &QAbstractButton::clicked);
    QSignalSpy twoClicked(two, &QAbstractButton::clicked);
    for (int click = 0; click < 8; ++click) {
        QRadioButton *target = (click % 2 == 0) ? two : one;
        QTest::mousePress(target, Qt::LeftButton, Qt::NoModifier, target->rect().center());
        qApp->processEvents();
        QCOMPARE(frameReal(target, "_winui_press_progress"), 1.0);
        QVERIFY(target->isDown());
        QTest::mouseRelease(target, Qt::LeftButton, Qt::NoModifier, target->rect().center());
        QVERIFY(target->isChecked());
    }

    QCOMPARE(oneClicked.count(), 4);
    QCOMPARE(twoClicked.count(), 4);
    QTest::qWait(320);
    QVERIFY(frameReal(one, "_winui_press_progress") < 0.05);
    QVERIFY(frameReal(two, "_winui_press_progress") < 0.05);
}

void WinUI3TogglesTest::radioDotDpiGeometry()
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
        radio.style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &option, &painter, &radio);

        const QColor dot = image.pixelColor(image.width() / 2, image.height() / 2);
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

void WinUI3TogglesTest::checkboxAndRadioUncheckMotion()
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

QTEST_MAIN(WinUI3TogglesTest)
#include "tst_winui3toggles.moc"
