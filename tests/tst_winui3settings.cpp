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
#include <QTest>
#include <QVariantAnimation>
#include <QVBoxLayout>

#include "winui3testhelpers.h"

class WinUI3SettingsTest final : public QObject
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
};

void WinUI3SettingsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3SettingsTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3SettingsTest::cleanup()
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

QTEST_MAIN(WinUI3SettingsTest)
#include "tst_winui3settings.moc"
