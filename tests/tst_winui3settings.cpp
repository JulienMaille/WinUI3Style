// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3interaction.cpp: SettingsCard interactivity
// contracts live here so the interaction binary stays under the 60 KB
// source-contracts ratchet.
#include <winui3style/settingscard.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/lib/winui3tokens_p.h"

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

#include "../src/lib/winui3paint_p.h"

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
    void settingsCardCornerRadius_data();
    void settingsCardCornerRadius();
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
    QCOMPARE(chevron->size(), QSize(32, 32));
    QCOMPARE(WinUI3::Private::SettingsExpanderChevronGlyphSize, 16);
    const QRect slot(chevron->mapTo(&card, QPoint()), chevron->size());
    const QImage collapsed = card.grab(slot).toImage();
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 0.0);
    card.setProperty("expansionProgress", 0.5);
    QCOMPARE(card.property("expansionProgress").toReal(), 0.5);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevron->property("_winui_settings_card_chevron_rotation").toReal(), 90.0);
    QCOMPARE(chevron->size(), QSize(32, 32));
    const QImage midpoint = card.grab(slot).toImage();
    QVERIFY(midpoint != collapsed);
    card.setProperty("expansionProgress", 1.0);
    QCOMPARE(chevron->size(), QSize(32, 32));
    const QImage expanded = card.grab(slot).toImage();
    QVERIFY(midpoint != expanded);
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

// Issue #111: the SettingsCard frame must carry the pinned
// ControlCornerRadius (4), not the overlay/flyout radius (8).
void WinUI3SettingsTest::settingsCardCornerRadius_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("rtl");
    for (bool dark : { false, true })
        for (bool compact : { false, true })
            for (bool rtl : { false, true })
                QTest::newRow(
                        qPrintable(QStringLiteral("%1-%2-%3").arg(dark).arg(compact).arg(rtl)))
                        << dark << compact << rtl;
}

void WinUI3SettingsTest::settingsCardCornerRadius()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);

    // Pinned: SettingsCard.xaml (CommunityToolkit/Windows a2fa47ad) sets
    // CornerRadius={ThemeResource ControlCornerRadius} on DefaultSettingsCard-
    // Style, and CornerRadius_themeresources.xaml (microsoft-ui-xaml 9f89c2da)
    // defines ControlCornerRadius 4,4,4,4 and OverlayCornerRadius 8,8,8,8.
    // A settings card is a control surface, not an overlay/flyout surface.
    QCOMPARE(WinUI3::Private::ControlRadius, 4.0);
    QCOMPARE(WinUI3::Private::OverlayRadius, 8.0);

    QFrame card;
    card.setProperty(WinUI3::Style::SettingsCardProperty, true);
    WinUI3::Style::setDensityMode(
            &card, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    card.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    card.resize(240, 80);
    // Offscreen with no granted material: the erase branch is a no-op, so the
    // sentinel canvas below measures the painted shape only.
    QVERIFY(!WinUI3::Private::paintsDirectlyOnBackdrop(&card));

    QStyleOption option;
    option.initFrom(&card);
    option.rect = card.rect();
    option.state = QStyle::State_Enabled;

    const QColor sentinel(255, 0, 0);
    QImage canvas(card.size(), QImage::Format_ARGB32_Premultiplied);
    canvas.fill(sentinel);
    {
        QPainter painter(&canvas);
        style->drawControl(QStyle::CE_ShapedFrame, &option, &painter, &card);
    }
    const auto ink = [&](int x, int y) { return canvas.pixelColor(x, y) != sentinel; };

    // The card's top stroke is the first inked row; its leading edge is the
    // first inked column well below the corner.
    int topRow = -1;
    for (int y = 0; y < canvas.height() && topRow < 0; ++y)
        for (int x = canvas.width() / 4; x < canvas.width() - canvas.width() / 4; ++x)
            if (ink(x, y)) {
                topRow = y;
                break;
            }
    QVERIFY2(topRow >= 0, "the settings card painted no top edge");
    int leading = -1;
    for (int x = 0; x < canvas.width() && leading < 0; ++x)
        if (ink(x, topRow + 40))
            leading = x;
    QVERIFY2(leading >= 0, "the settings card painted no leading edge");

    // The pinned control surface for the very same fill/stroke tokens: the card
    // frame must be pixel-identical to it, so the call site carries
    // ControlCornerRadius rather than the overlay/flyout radius.
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(card.palette());
    QImage expected(card.size(), QImage::Format_ARGB32_Premultiplied);
    expected.fill(sentinel);
    {
        QPainter painter(&expected);
        WinUI3::PaintPrivate::controlSurface(&painter, QRectF(card.rect()), tokens.control,
                                             tokens.stroke, tokens.strokeSecondary,
                                             WinUI3::Private::ControlRadius);
    }
    QCOMPARE(canvas, expected);

    // Geometry pairing on the same state: the corner arc of the fill itself.
    // controlSurface() draws the fill inside the 1 DIP stroke ring, so the
    // fill's arc is centred at (r, r) with radius r - 1 when the card radius
    // is r: the interior colour first appears exactly r columns in from the
    // leading edge in the first row below the top stroke, and 2 px further in
    // in the second. The overlay/flyout radius (8) would read 8 and 5 instead.
    const QColor fillColor = canvas.pixelColor(canvas.width() / 2, canvas.height() / 2);
    QVERIFY2(fillColor != sentinel, "the settings card painted no fill");
    const auto fillStart = [&](int row) {
        for (int x = 0; x < canvas.width(); ++x)
            if (canvas.pixelColor(x, row) == fillColor)
                return x;
        return -1;
    };
    const int firstRow = fillStart(topRow + 1);
    const int secondRow = fillStart(topRow + 2);
    QVERIFY2(firstRow >= 0 && secondRow >= 0, "the settings card painted no fill corner");
    QCOMPARE(firstRow, static_cast<int>(WinUI3::Private::ControlRadius));
    QCOMPARE(secondRow, static_cast<int>(WinUI3::Private::ControlRadius) - 2);
    QCOMPARE(card.size(), QSize(240, 80));
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
