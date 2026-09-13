// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3interaction.cpp: SettingsCard interactivity
// contracts live here so the interaction binary stays under the 60 KB
// source-contracts ratchet.
#include <winui3style/settingscard.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3tokens_p.h"

#include <QCheckBox>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QSignalSpy>
#include <QStyleOption>
#include <QTest>
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
    QVERIFY(chevron && chevron->isVisible());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    card.setExpanded(true);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronUp));
    card.setExpanded(false);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    card.setLayoutDirection(Qt::RightToLeft);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
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
