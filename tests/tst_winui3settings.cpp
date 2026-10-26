// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3interaction.cpp: SettingsCard interactivity
// contracts live here so the interaction binary stays under the 60 KB
// source-contracts ratchet.
#include <winui3style/settingscard.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3tokens_p.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QSignalSpy>
#include <QStyleOption>
#include <QScopedPointer>
#include <QTest>
#include <QToolTip>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include "winui3testhelpers.h"

class WinUI3SettingsTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void settingsCardTrailingOnlyCardsAreNotInteractive();
    void settingsCardExpanderGlyphRotatesWithState();
    void settingsCardChevronMidpointRotates();
    void settingsCardChevronReversalContinuesFromCurrentValue();
    void settingsCardChevronInstantWhenAnimationsDisabled();
    void settingsCardActivatedSlotMayClearExpandableWidget();
    void settingsCardActivatedSlotMayDeleteCard();
    void settingsCardTrailingWidgetSurvivesExternalDelete();
    void settingsCardExpandedHostSharesCardSurface();
    void plainShapedFrameFallsThroughToBaseStyle();
    void uninstalledStyleDoesNotRefreshApplication();
};

void WinUI3SettingsTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3SettingsTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3SettingsTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3SettingsTest::settingsCardTrailingOnlyCardsAreNotInteractive()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Notifications"));
    auto *toggle = new QCheckBox;
    card.setTrailingWidget(toggle);
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    QVERIFY(!card.isCardInteractive());
    QCOMPARE(card.focusPolicy(), Qt::NoFocus);
    auto *header = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    QVERIFY(header);
    QSignalSpy activated(&card, &WinUI3::SettingsCard::activated);
    QTest::mouseClick(header, Qt::LeftButton, Qt::NoModifier, header->rect().center());
    QCOMPARE(activated.count(), 0);
    QCOMPARE(card.isExpanded(), false);
    QStyleOption option;
    option.initFrom(&card);
    option.rect = card.rect();
    option.state |= QStyle::State_MouseOver | QStyle::State_Sunken;
    QImage image(card.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    card.style()->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &card);
    painter.end();
    QCOMPARE(image.pixelColor(card.rect().center()),
             WinUI3::Private::tokens(card.palette()).control);
}

void WinUI3SettingsTest::settingsCardExpanderGlyphRotatesWithState()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    auto *animation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(chevron && chevron->isVisible());
    QVERIFY(animation);
    // Collapsed endpoint: base ChevronDown artwork, zero rotation.
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
    card.setExpanded(true);
    animation->setCurrentTime(animation->duration());
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.99);
    // Expanded endpoint: same base artwork rotated 180 deg, not a glyph swap.
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 180.0);
    card.setExpanded(false);
    animation->setCurrentTime(animation->duration());
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.01);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
    // RTL shares the vertical expand motion: still ChevronDown at rest.
    card.setLayoutDirection(Qt::RightToLeft);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
}

void WinUI3SettingsTest::settingsCardChevronMidpointRotates()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    QVERIFY(chevron && chevron->isVisible());
    const QPixmap collapsed = labelPixmap(chevron);
    QVERIFY(!collapsed.isNull());
    // Drive the midpoint directly through the expansion progress contract:
    // the glyph must re-render as a true rotation, not an endpoint snap.
    card.setProperty("expansionProgress", 0.5);
    QCOMPARE(card.property("expansionProgress").toReal(), 0.5);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 90.0);
    const QPixmap midpoint = labelPixmap(chevron);
    QVERIFY(!midpoint.isNull());
    QVERIFY(midpoint.cacheKey() != collapsed.cacheKey());
    card.setProperty("expansionProgress", 1.0);
    const QPixmap expanded = labelPixmap(chevron);
    QVERIFY(!expanded.isNull());
    QVERIFY(midpoint.cacheKey() != expanded.cacheKey());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 180.0);
    card.setProperty("expansionProgress", 0.0);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
}

void WinUI3SettingsTest::settingsCardChevronReversalContinuesFromCurrentValue()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    auto *animation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(chevron && chevron->isVisible());
    QVERIFY(animation);
    card.setExpanded(true);
    animation->setCurrentTime(animation->duration() / 2);
    QCoreApplication::processEvents();
    const qreal midpoint = card.property("expansionProgress").toReal();
    QVERIFY2(midpoint > 0.0 && midpoint < 1.0, qPrintable(QString::number(midpoint)));
    const qreal midRotation = chevron->property("_winui_settings_card_chevron_rotation").toReal();
    QVERIFY2(midRotation > 0.0 && midRotation < 180.0, qPrintable(QString::number(midRotation)));
    QCOMPARE(midRotation, midpoint * 180.0);
    // Reversing mid-flight must continue from the live value, not restart.
    card.setExpanded(false);
    QCOMPARE(animation->endValue().toReal(), 0.0);
    QVERIFY(qAbs(animation->startValue().toReal() - midpoint) < 0.01);
    animation->setCurrentTime(animation->duration());
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.01);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
}

void WinUI3SettingsTest::settingsCardChevronInstantWhenAnimationsDisabled()
{
    DisableAnimationsGuard guard;
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    auto *animation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(chevron && chevron->isVisible());
    QVERIFY(animation);
    QVERIFY(animation->state() == QAbstractAnimation::Stopped);
    card.setExpanded(true);
    QCOMPARE(card.property("expansionProgress").toReal(), 1.0);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 180.0);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QVERIFY(animation->state() == QAbstractAnimation::Stopped);
    card.setExpanded(false);
    QCOMPARE(card.property("expansionProgress").toReal(), 0.0);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
}

void WinUI3SettingsTest::settingsCardActivatedSlotMayClearExpandableWidget()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    QVERIFY(card.isCardInteractive());
    QSignalSpy activated(&card, &WinUI3::SettingsCard::activated);
    connect(&card, &WinUI3::SettingsCard::activated, &card, [&card] {
        // Re-entrant clear: the keyPressEvent tail must re-check
        // m_expandableWidget instead of blindly toggling expansion.
        card.setExpandableWidget(nullptr);
    });
    QTest::keyClick(&card, Qt::Key_Space);
    QCOMPARE(activated.count(), 1);
    QVERIFY(card.expandableWidget() == nullptr);
    QCOMPARE(card.isExpanded(), false);
    QVERIFY(!card.isCardInteractive());
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    QVERIFY(chevron && !chevron->isVisible());
}

void WinUI3SettingsTest::settingsCardActivatedSlotMayDeleteCard()
{
    auto *card = new WinUI3::SettingsCard;
    card->setTitle(QStringLiteral("Advanced options"));
    card->setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card->resize(420, card->sizeHint().height());
    card->show();
    QVERIFY(QTest::qWaitForWindowExposed(card));
    auto *header = card->findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    QVERIFY(header);
    const QPointer<WinUI3::SettingsCard> guard(card);
    QSignalSpy activated(card, &WinUI3::SettingsCard::activated);
    connect(card, &WinUI3::SettingsCard::activated, card, [card] {
        // Re-entrant deletion: without the QPointer guard in
        // mouseReleaseEvent the post-emit dereference of
        // m_expandableWidget would touch freed memory.
        delete card;
    });
    QTest::mousePress(header, Qt::LeftButton, Qt::NoModifier, header->rect().center());
    QTest::mouseRelease(header, Qt::LeftButton, Qt::NoModifier, header->rect().center());
    QCOMPARE(activated.count(), 1);
    QVERIFY(guard.isNull());
    qApp->processEvents();
}

void WinUI3SettingsTest::settingsCardTrailingWidgetSurvivesExternalDelete()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Notifications"));
    auto *stale = new QCheckBox;
    card.setTrailingWidget(stale);
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    QCOMPARE(card.trailingWidget(), stale);
    // External delete: the card's pointer must auto-null instead of
    // dangling, or the next setTrailingWidget() would call removeWidget()
    // and setParent() on freed memory.
    const QPointer<QWidget> doomed(stale);
    delete stale;
    QVERIFY(doomed.isNull());
    QVERIFY(card.trailingWidget() == nullptr);
    // Rebinding, resize, and event delivery must not touch freed memory.
    auto *fresh = new QCheckBox;
    card.setTrailingWidget(fresh);
    QCOMPARE(card.trailingWidget(), fresh);
    // The header layout re-parents layout-managed children to its host
    // widget, so ownership means "inside the header", not "direct child".
    auto *header = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    QVERIFY(header);
    QCOMPARE(fresh->parentWidget(), header);
    card.resize(400, card.sizeHint().height());
    QCoreApplication::processEvents();
    QCOMPARE(card.trailingWidget(), fresh);
}

void WinUI3SettingsTest::settingsCardExpandedHostSharesCardSurface()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced options"));
    auto *details = new QLabel(QStringLiteral("Expanded details"));
    card.setExpandableWidget(details);
    card.resize(420, card.sizeHint().height());
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    auto *host = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_expandableHost"));
    QVERIFY(host);
    QVERIFY(!host->autoFillBackground());
    card.setExpanded(true);
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.99);
    QVERIFY(!host->testAttribute(Qt::WA_OpaquePaintEvent));
    QVERIFY(card.grab().toImage().pixelColor(host->mapTo(&card, host->rect().center())).alpha()
            > 200);
}

void WinUI3SettingsTest::plainShapedFrameFallsThroughToBaseStyle()
{
    // S1 extraction defect: drawChromeControl's trailing SizeGrip branch ran
    // for every element that passed its filter without returning, so plain
    // QFrames (CE_ShapedFrame without the settings-card property) got
    // SizeGrip dots at their bottom-right corner. The tail must only run
    // for CE_SizeGrip; anything else falls through to the base style.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QFrame frame;
    frame.resize(120, 40);
    QStyleOption option;
    option.initFrom(&frame);
    option.rect = frame.rect();
    option.state |= QStyle::State_Enabled;
    QImage canvas(120, 40, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    {
        QPainter painter(&canvas);
        // Full path: the base style paints the frame, never SizeGrip dots.
        style->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &frame);
    }
    // Bottom-right 16x16 corner holds only base-frame ink: assert no pixel
    // carries the SizeGrip dot color (textTertiary) there. Note textTertiary
    // is translucent (alpha ~114 in Light), so the gate must be alpha-aware:
    // an opaque-only scan would miss the dots entirely.
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(QApplication::palette());
    int dotPixels = 0;
    for (int y = 24; y < 40; ++y) {
        for (int x = 104; x < 120; ++x) {
            const QColor pixel = canvas.pixelColor(x, y);
            if (pixel.alpha() < 60)
                continue;
            if (qAbs(pixel.alpha() - tokens.textTertiary.alpha()) > 60)
                continue;
            const int dr = pixel.red() - tokens.textTertiary.red();
            const int dg = pixel.green() - tokens.textTertiary.green();
            const int db = pixel.blue() - tokens.textTertiary.blue();
            if (dr * dr + dg * dg + db * db < 30 * 30)
                ++dotPixels;
        }
    }
    QCOMPARE(dotPixels, 0);
    // Geometry pairing on the same state: the frame kept its size.
    QCOMPARE(frame.size(), QSize(120, 40));
}

void WinUI3SettingsTest::uninstalledStyleDoesNotRefreshApplication()
{
    QWidget gallery;
    auto *panel = new QWidget(&gallery);
    QPalette galleryPalette = gallery.palette();
    galleryPalette.setColor(QPalette::Window, QColor(40, 80, 120));
    gallery.setPalette(galleryPalette);
    gallery.setAutoFillBackground(true);
    gallery.setAttribute(Qt::WA_StyledBackground, true);
    QPalette panelPalette = panel->palette();
    panelPalette.setColor(QPalette::Window, QColor(110, 70, 35));
    panel->setPalette(panelPalette);
    panel->setAutoFillBackground(true);
    gallery.setProperty("_winui_backdrop", 0);
    gallery.setProperty("_winui_backdrop_effective", 2);
    gallery.resize(240, 160);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    QCoreApplication::processEvents();

    QStyle *const installedStyle = qApp->style();
    const QPalette applicationPalette = qApp->palette();
    const QPalette toolTipPalette = QToolTip::palette();
    const QPalette liveGalleryPalette = gallery.palette();
    const QPalette livePanelPalette = panel->palette();
    const auto galleryStyledBackground = gallery.testAttribute(Qt::WA_StyledBackground);
    const auto galleryAutoFill = gallery.autoFillBackground();
    const auto gallerySetPalette = gallery.testAttribute(Qt::WA_SetPalette);
    const auto panelStyledBackground = panel->testAttribute(Qt::WA_StyledBackground);
    const auto panelAutoFill = panel->autoFillBackground();
    const auto panelSetPalette = panel->testAttribute(Qt::WA_SetPalette);

    QScopedPointer<QStyle> preview(new WinUI3::Style);
    QVERIFY(preview);
    QVERIFY(preview.data() != installedStyle);
    const QColor accent(36, 94, 151);
    QVERIFY(preview->setProperty("themeMode", 2));
    QVERIFY(preview->setProperty("accentColor", accent));
    QCOMPARE(preview->standardPalette().color(QPalette::Window), QColor(32, 32, 32));
    QCOMPARE(preview->standardPalette().color(QPalette::Highlight), accent);
    QCoreApplication::processEvents();

    QCOMPARE(qApp->style(), installedStyle);
    QCOMPARE(qApp->palette(), applicationPalette);
    QCOMPARE(QToolTip::palette(), toolTipPalette);
    QCOMPARE(gallery.palette(), liveGalleryPalette);
    QCOMPARE(panel->palette(), livePanelPalette);
    QCOMPARE(gallery.testAttribute(Qt::WA_StyledBackground), galleryStyledBackground);
    QCOMPARE(gallery.autoFillBackground(), galleryAutoFill);
    QCOMPARE(gallery.testAttribute(Qt::WA_SetPalette), gallerySetPalette);
    QCOMPARE(panel->testAttribute(Qt::WA_StyledBackground), panelStyledBackground);
    QCOMPARE(panel->autoFillBackground(), panelAutoFill);
    QCOMPARE(panel->testAttribute(Qt::WA_SetPalette), panelSetPalette);
    QCOMPARE(gallery.property("_winui_backdrop").toInt(), 0);
    QCOMPARE(gallery.property("_winui_backdrop_effective").toInt(), 2);
}

QTEST_MAIN(WinUI3SettingsTest)
#include "tst_winui3settings.moc"
