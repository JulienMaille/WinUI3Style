// SPDX-License-Identifier: LGPL-2.1-or-later
// Card-nested editor control samples over a live Mica backdrop. Split out of
// tst_winui3micacard.cpp because that binary is at the 60 KB ratchet (the
// Settings-page ComboBox regression, issue #72). Every grab()/pixelColor()
// assertion pairs with a QCOMPARE token/geometry assertion on the same state.
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "../src/lib/winui3surfaces_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QComboBox>
#include <QStyleOptionComboBox>

class WinUI3MicaCardEditorTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void comboBoxInsideCardKeepsCardFillOverComposited();
};

void WinUI3MicaCardEditorTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3MicaCardEditorTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3MicaCardEditorTest::cleanup()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style()))
        style->setThemeMode(WinUI3::ThemeMode::Light);
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

void WinUI3MicaCardEditorTest::comboBoxInsideCardKeepsCardFillOverComposited()
{
    // Settings-page ComboBox (issue #72): a QComboBox inside a SettingsCard /
    // group card must keep the card's veiled tone under a granted Composited
    // backdrop. Cards carry no SurfaceProperty, so paintsDirectlyOnBackdrop()
    // passes for the combo and its frame is Source-cleared — like the button
    // case in the mica-backdrop binary — but the combo used the bare erase
    // plus a parent-SurfaceProperty refill that never runs for a card. The
    // rounded control surface then left the corner cutouts at alpha 0 and the
    // live material showed through (the reported glitch). Dark/light x
    // Standard/Compact x group/SettingsCard x composited/NONE; every pixel
    // assert pairs with a token/geometry assertion on the same state.
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    const auto checkComboOverCard = [&](bool dark, int density, bool settingsCard,
                                        bool composited) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        style->setProperty("densityMode", density);
        QWidget window;
        if (composited) {
            window.setProperty("_winui_backdrop", 1);
            window.setProperty("_winui_backdrop_effective", 2); // Composited
        }
        auto *island = new QWidget(&window);
        island->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
        island->setGeometry(0, 0, 480, 320);
        QWidget *card = settingsCard ? static_cast<QWidget *>(new WinUI3::SettingsCard(island))
                                     : static_cast<QWidget *>(new QGroupBox(island));
        card->setGeometry(40, 40, 400, 240);
        auto *combo = new QComboBox(card);
        combo->addItems({ QStringLiteral("Automatic"), QStringLiteral("Notify me") });
        combo->setGeometry(60, 140, 160, 32);
        window.resize(480, 320);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        if (composited)
            WinUI3::Private::syncContentSurfacesForBackdrop(&window);
        // Token/geometry pairs for the pixel asserts below: the gate passes
        // only with a granted backdrop (a card carries no SurfaceProperty),
        // the card walk finds the ancestor either way, and the tone is the
        // veiled refill the owning card itself paints.
        QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(combo), composited);
        QVERIFY(WinUI3::Private::insideOpaqueCard(combo));
        // The card that owns the surface paints a different role per family:
        // a group card uses the Base veil, a SettingsCard uses t.control
        // (QPalette::Button). The refill must match that exact role, or the
        // control's rounded corners read as darker patches (issue #72).
        const QPalette::ColorRole cardRole =
                settingsCard ? QPalette::Button : QPalette::Base;
        const QColor cardTone = WinUI3::Private::veiledCard(
                card->palette().color(QPalette::Active, cardRole), composited);
        QCOMPARE(cardTone.alpha(),
                 composited ? qMin(card->palette().color(cardRole).alpha(),
                                   WinUI3::Private::VeiledCardAlpha)
                            : card->palette().color(cardRole).alpha());
        QCOMPARE(WinUI3::Private::cardRefillToneFor(combo, composited), cardTone);
        QCOMPARE(combo->geometry(), QRect(60, 140, 160, 32));
        // Mechanism: the real CC_ComboBox path over a seeded card canvas keeps
        // the card tone at the rounded corner the control surface never
        // reaches (RED over Mica: alpha 0 — the reported glitch; RED over
        // NONE: a second veiled layer punched through the opaque card), and
        // the translucent control fill composites ON the card tone once.
        QImage canvas(combo->size(), QImage::Format_ARGB32_Premultiplied);
        canvas.fill(cardTone);
        QStyleOptionComboBox option;
        option.initFrom(combo);
        option.rect = QRect(QPoint(), combo->size());
        option.state |= QStyle::State_Enabled;
        option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
        {
            QPainter painter(&canvas);
            style->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, combo);
        }
        const QColor corner = canvas.pixelColor(0, 0);
        QCOMPARE(corner.alpha(), cardTone.alpha());
        QVERIFY(colorDistance(corner, cardTone) <= 4);
        QImage want(1, 1, QImage::Format_ARGB32_Premultiplied);
        want.fill(cardTone);
        {
            QPainter painter(&want);
            painter.fillRect(want.rect(), WinUI3::Private::tokens(combo->palette()).control);
        }
        const QColor expected = want.pixelColor(0, 0);
        const QColor center = canvas.pixelColor(canvas.rect().center());
        QCOMPARE(center.alpha(), expected.alpha());
        QVERIFY(colorDistance(center, expected) <= 4);
    };
    for (const bool dark : { false, true }) {
        for (const int density : { 0, 1 }) {
            for (const bool settingsCard : { false, true }) {
                checkComboOverCard(dark, density, settingsCard, true);
                checkComboOverCard(dark, density, settingsCard, false);
            }
        }
    }
}

QTEST_MAIN(WinUI3MicaCardEditorTest)

#include "tst_winui3micacardeditor.moc"
