// SPDX-License-Identifier: LGPL-2.1-or-later
// SettingsCard interaction contracts: expansion, designer bindings,
// trailing-widget clicks/height, chevron/header stability, load
// performance, expansion inside a scrolling page. Split out of
// tst_winui3interaction.cpp per the 60 KB test-size ratchet (S6 follow-up);
// test-function names kept identical so the DPI reruns in
// tests/CMakeLists.txt keep working. settingsCardInteractiveFrames stays in
// the interaction binary: it is DPI-pinned there. Every
// grab()/pixelColor() assertion pairs with a QCOMPARE token/geometry
// assertion on the same state.
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/lib/winui3frameproperties_p.h"
#include "../src/lib/winui3helpers_p.h"
#include "../src/lib/winui3settingscardmotion_p.h"
#include "../src/lib/winui3style_properties_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QLabel>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QCalendarWidget>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDockWidget>
#include <QFocusEvent>
#include <QFrame>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QHBoxLayout>
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

namespace {

// Stock SettingsExpander composition: no compiled widget. The card is a plain
// QFrame with winuiSettingsCard, its header is a QWidget, and the collapsible
// content is a direct child carrying winuiSettingsCardExpansion. Geometry is
// the pinned SettingsExpanderHeaderPadding (16,16,4,16) plus the 8 px trailing
// chevron margin, i.e. 44 px reserved on the header's trailing edge.
struct StockSettingsCard
{
    QFrame *card = nullptr;
    QWidget *header = nullptr;
    QWidget *details = nullptr;
};

StockSettingsCard makeStockSettingsCard(const QString &density = QString())
{
    StockSettingsCard parts;
    parts.card = new QFrame;
    parts.card->setProperty(WinUI3::Style::SettingsCardProperty, true);
    if (!density.isEmpty())
        parts.card->setProperty("winuiDensity", density);
    auto *cardLayout = new QVBoxLayout(parts.card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    parts.header = new QWidget;
    auto *headerLayout = new QHBoxLayout(parts.header);
    headerLayout->setContentsMargins(16, 16, 44, 16);
    headerLayout->addWidget(new QLabel(QStringLiteral("Advanced options")));

    parts.details = new QWidget;
    parts.details->setProperty(WinUI3::Style::SettingsCardExpansionProperty, true);
    auto *detailsLayout = new QVBoxLayout(parts.details);
    detailsLayout->addWidget(new QLabel(QStringLiteral("Expanded details")));

    cardLayout->addWidget(parts.header);
    cardLayout->addWidget(parts.details);
    return parts;
}

} // namespace

class WinUI3SettingsCardTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void settingsCardExpansion();
    void settingsCardContentGeometry_data();
    void settingsCardContentGeometry();
    void settingsCardNestedExpansionSeparator_data();
    void settingsCardNestedExpansionSeparator();
    void settingsCardInheritedDirectionAndTrailingInset_data();
    void settingsCardInheritedDirectionAndTrailingInset();
    void settingsCardDesignerPropertyBindings();
    void settingsCardTrailingWidgetsReceiveClicks();
    void settingsCardTrailingWidgetsHaveUniformHeight();
    void settingsCardChevronAndStableHeader();
    void settingsCardExpansionLoad();
    void settingsCardExpansionInScrollingPage();
    void settingsCardStockHeaderGeometry_data();
    void settingsCardStockHeaderGeometry();
    void settingsCardStockExpansionInteraction();
    void settingsCardStockExpansionAnimationLifecycle_data();
    void settingsCardStockExpansionAnimationLifecycle();
    void settingsCardStockTrailingCardStaysInert();
};

void WinUI3SettingsCardTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3SettingsCardTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3SettingsCardTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3SettingsCardTest::settingsCardExpansion()
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
    setAnimationTime(&card, 35);
    const qreal pressed = frameReal(&card, "_winui_press_progress");
    QVERIFY2(pressed > 0.0 && pressed < 1.0, qPrintable(QString::number(pressed)));
    QTest::mouseRelease(&card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QVERIFY(card.isExpanded());
    QCOMPARE(spy.count(), 1);
    auto *animation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(animation);
    QCOMPARE(animation->duration(), WinUI3::Private::SettingsExpanderExpandDuration);
    QCOMPARE(animation->easingCurve(), WinUI3::Private::settingsExpanderCurve(true));
    setAnimationTime(&card, 280);
    const qreal expanding = card.property("expansionProgress").toReal();
    QVERIFY(expanding > 0.0 && expanding < 1.0);
    card.setExpanded(false);
    QCOMPARE(card.property("expansionProgress").toReal(), expanding);
    QCOMPARE(animation->duration(), WinUI3::Private::SettingsExpanderCollapseDuration);
    QCOMPARE(animation->easingCurve(), WinUI3::Private::settingsExpanderCurve(false));
    setAnimationTime(&card, 90);
    const qreal reverse = card.property("expansionProgress").toReal();
    QVERIFY(reverse > 0.0 && reverse < 1.0);
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.01);
}

void WinUI3SettingsCardTest::settingsCardDesignerPropertyBindings()
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

    auto *header = card->findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    QVERIFY(header);
    QTest::mouseClick(header, Qt::LeftButton, Qt::NoModifier, header->rect().center());
    QTRY_VERIFY(card->isExpanded());
    QVERIFY(details->isVisible());
}

void WinUI3SettingsCardTest::settingsCardTrailingWidgetsReceiveClicks()
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
    combo->addItems({ QStringLiteral("Automatic"), QStringLiteral("Manual") });
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
        return QApplication::widgetAt(widget->mapToGlobal(widget->rect().center()));
    };

    QWidget *toggleTarget = targetAt(toggle);
    QVERIFY2(toggleTarget == toggle || toggle->isAncestorOf(toggleTarget),
             "SettingsCard swallowed the trailing toggle hit area");
    const QPoint togglePoint =
            toggleTarget->mapFromGlobal(toggle->mapToGlobal(toggle->rect().center()));
    QTest::mouseClick(toggleTarget, Qt::LeftButton, Qt::NoModifier, togglePoint);
    QCOMPARE(toggle->isChecked(), false);

    QWidget *comboTarget = targetAt(combo);
    QVERIFY2(comboTarget == combo || combo->isAncestorOf(comboTarget),
             "SettingsCard swallowed the trailing combo-box hit area");
    const QPoint comboPoint =
            comboTarget->mapFromGlobal(combo->mapToGlobal(combo->rect().center()));
    QTest::mouseClick(comboTarget, Qt::LeftButton, Qt::NoModifier, comboPoint);
    QTRY_VERIFY(combo->view()->isVisible());
    combo->hidePopup();

    auto *title = expandableCard->findChild<QLabel *>(QStringLiteral("_winui_settings_card_title"));
    QVERIFY(title);
    QWidget *headerTarget = targetAt(title);
    QVERIFY(headerTarget);
    QTest::mouseClick(headerTarget, Qt::LeftButton, Qt::NoModifier,
                      headerTarget->mapFromGlobal(title->mapToGlobal(title->rect().center())));
    QCOMPARE(expandableCard->isExpanded(), true);
}

void WinUI3SettingsCardTest::settingsCardTrailingWidgetsHaveUniformHeight()
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
    combo->addItems(
            { QStringLiteral("Automatic"), QStringLiteral("Notify me"), QStringLiteral("Manual") });
    comboCard->setTrailingWidget(combo);
    layout->addWidget(comboCard);

    host.resize(560, host.sizeHint().height());
    host.show();
    QCoreApplication::processEvents();

    QCOMPARE(toggleCard->height(), comboCard->height());
    QCOMPARE(toggleCard->sizeHint().height(), comboCard->sizeHint().height());
    QCOMPARE(toggleCard->minimumSizeHint().height(), comboCard->minimumSizeHint().height());
}

void WinUI3SettingsCardTest::settingsCardChevronAndStableHeader()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Advanced settings"));
    card.setDescription(
            QStringLiteral("A description that remains in the same header while content expands."));
    auto *trailing = new QLabel(QStringLiteral("On"));
    card.setTrailingWidget(trailing);
    card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(460, card.sizeHint().height());
    card.show();
    QTRY_VERIFY(card.isVisible());

    auto *headerHost = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    auto *title = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_title"));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    QVERIFY(headerHost);
    QVERIFY(title);
    QVERIFY(chevron);
    QVERIFY(chevron->isVisible());
    const auto chevronRotation = [&] {
        return chevron->property("_winui_settings_card_chevron_rotation").toReal();
    };
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevronRotation(), 0.0);
    const QRect chevronInCard(chevron->mapTo(&card, chevron->rect().topLeft()), chevron->size());
    const QRect trailingInCard(trailing->mapTo(&card, trailing->rect().topLeft()),
                               trailing->size());
    QVERIFY(!chevronInCard.intersects(trailingInCard));

    // The compiled card delegates rotation to the style's painter transform:
    // the glyph raster must change at an intermediate angle while its layout
    // slot and reported state remain stable. This catches a missing paint
    // dispatch without relying on a whole-card visual baseline.
    card.setProperty("expansionProgress", 0.0);
    QCoreApplication::processEvents();
    QCOMPARE(chevronRotation(), 0.0);
    QCOMPARE(chevron->geometry().size(), QSize(32, 32));
    const QImage collapsedGlyph = card.grab().toImage().copy(chevronInCard);
    card.setProperty("expansionProgress", 0.5);
    QCoreApplication::processEvents();
    QCOMPARE(chevronRotation(), 90.0);
    QCOMPARE(chevron->geometry().size(), QSize(32, 32));
    const QImage midpointGlyph = card.grab().toImage().copy(chevronInCard);
    QVERIFY(collapsedGlyph != midpointGlyph);
    card.setProperty("expansionProgress", 0.0);

    const QRect titleGeometry = title->geometry();
    card.setExpanded(true);
    auto *compiledAnimation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(compiledAnimation);
    QCOMPARE(WinUI3::Private::SettingsExpanderChevronGlyphSize, 16);
    QCOMPARE(compiledAnimation->duration(), WinUI3::Private::SettingsExpanderExpandDuration);
    QCOMPARE(compiledAnimation->easingCurve().type(), QEasingCurve::BezierSpline);
    QCOMPARE(compiledAnimation->easingCurve().toCubicSpline(),
             WinUI3::Private::settingsExpanderCurve(true).toCubicSpline());
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.0);
    QCOMPARE(title->geometry(), titleGeometry);

    card.setLayoutDirection(Qt::RightToLeft);
    // ChevronDown artwork rotates to 180 deg (no Up swap).
    QTRY_COMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
                 static_cast<int>(WinUI3::Icon::ChevronDown));
    QTRY_VERIFY(card.property("expansionProgress").toReal() > 0.99);
    QVERIFY(qAbs(chevronRotation() - card.property("expansionProgress").toReal() * 180.0) < 0.01);
    card.setExpanded(false);
    QCOMPARE(compiledAnimation->duration(), WinUI3::Private::SettingsExpanderCollapseDuration);
    QCOMPARE(compiledAnimation->easingCurve().type(), QEasingCurve::BezierSpline);
    QCOMPARE(compiledAnimation->easingCurve().toCubicSpline(),
             WinUI3::Private::settingsExpanderCurve(false).toCubicSpline());
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.01);
    // Contract: collapsed 0 deg, expanded 180 deg.
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QVERIFY(qAbs(chevronRotation() - card.property("expansionProgress").toReal() * 180.0) < 0.01);
    QTRY_VERIFY(card.property("expansionProgress").toReal() < 0.001);
}

void WinUI3SettingsCardTest::settingsCardExpansionLoad()
{
    const QList<int> cardCounts = { 1, 10, 50 };
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
            card->setDescription(
                    QStringLiteral("A long description which exercises the stable header width "
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
                                        .arg(previousLayouts)
                                        .arg(layouts)));
            QVERIFY2(resizes <= previousResizes * 6 + count * 4,
                     qPrintable(QStringLiteral("resizes grew superlinearly: %1 -> %2")
                                        .arg(previousResizes)
                                        .arg(resizes)));
        }
        previousLayouts = layouts;
        previousResizes = resizes;

        host.hide();
        qDeleteAll(probes);
    }
}

void WinUI3SettingsCardTest::settingsCardExpansionInScrollingPage()
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
    combo->addItems(
            { QStringLiteral("Automatic"), QStringLiteral("Notify me"), QStringLiteral("Manual") });
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

    const QList<WinUI3::SettingsCard *> cards{ notifications, updates, advanced };
    QVector<QRect> cardGeometries;
    QVector<QRect> headerGeometries;
    for (WinUI3::SettingsCard *card : cards) {
        auto *header =
                card->findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
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

void WinUI3SettingsCardTest::settingsCardStockHeaderGeometry_data()
{
    QTest::addColumn<QString>("density");
    QTest::newRow("standard") << QStringLiteral("standard");
    QTest::newRow("compact") << QStringLiteral("compact");
}

void WinUI3SettingsCardTest::settingsCardStockHeaderGeometry()
{
    QFETCH(QString, density);
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    layout->setContentsMargins(20, 20, 20, 20);
    const StockSettingsCard parts = makeStockSettingsCard(density);
    layout->addWidget(parts.card);
    host.resize(480, 300);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardProperty).toBool(), true);
    QCOMPARE(WinUI3::Private::settingsCardExpansionHost(parts.card), parts.details);
    QCOMPARE(parts.card->focusPolicy(), Qt::StrongFocus);
    QCOMPARE(parts.details->maximumHeight(), 0);
    // The header bottom is the collapsible child's top edge in both states;
    // the renderer paints the chevron inside that band.
    QCOMPARE(WinUI3::Private::settingsCardHeaderHeight(parts.card), parts.header->height());
    QCOMPARE(parts.details->y(), parts.header->height());
    QCOMPARE(WinUI3::Private::progress(parts.card,
                                       WinUI3::Private::settingsCardExpansionProgressProperty),
             0.0);

    // SettingsExpanderChevronButtonWidth/Height 32 with Margin 0,0,8,0 and a
    // 16 px glyph: ink must stay inside the centered 16 px square, and the
    // 32 px box must end 8 px before the card's trailing edge.
    const QRect chevron(parts.card->width() - 8 - 32, (parts.header->height() - 32) / 2, 32, 32);
    QCOMPARE(chevron.right(), parts.card->width() - 9);
    const QRect glyph(chevron.center().x() - 8, chevron.center().y() - 8, 16, 16);
    const QImage image = parts.card->grab().toImage();
    const QColor surface = image.pixelColor(chevron.left() - 4, chevron.center().y());
    int inkCount = 0;
    QRect inkBounds;
    for (int y = chevron.top(); y <= chevron.bottom(); ++y) {
        for (int x = chevron.left(); x <= chevron.right(); ++x) {
            if (colorDistance(image.pixelColor(x, y), surface) <= 8)
                continue;
            ++inkCount;
            inkBounds =
                    inkBounds.isNull() ? QRect(x, y, 1, 1) : inkBounds.united(QRect(x, y, 1, 1));
        }
    }
    QVERIFY2(inkCount > 6, qPrintable(QString::number(inkCount)));
    QVERIFY2(glyph.contains(inkBounds),
             qPrintable(QStringLiteral("%1x%2 at %3,%4")
                                .arg(inkBounds.width())
                                .arg(inkBounds.height())
                                .arg(inkBounds.x())
                                .arg(inkBounds.y())));
}

void WinUI3SettingsCardTest::settingsCardStockExpansionInteraction()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    layout->setContentsMargins(20, 20, 20, 20);
    const StockSettingsCard parts = makeStockSettingsCard();
    layout->addWidget(parts.card);
    host.resize(480, 320);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QCOMPARE(parts.details->maximumHeight(), 0);
    const int headerHeight = parts.header->height();

    const QPoint headerPoint(headerHeight, parts.header->height() / 2);
    QTest::mouseMove(parts.card, headerPoint);
    QTest::mousePress(parts.card, Qt::LeftButton, Qt::NoModifier, headerPoint);
    QTest::qWait(30);
    const qreal pressed = frameReal(parts.card, "_winui_press_progress");
    QVERIFY2(pressed > 0.0, qPrintable(QString::number(pressed)));
    QTest::mouseRelease(parts.card, Qt::LeftButton, Qt::NoModifier, headerPoint);

    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), true);
    setAnimationTime(parts.card, 80);
    const qreal mid = WinUI3::Private::progress(
            parts.card, WinUI3::Private::settingsCardExpansionProgressProperty);
    QVERIFY2(mid > 0.0 && mid < 1.0, qPrintable(QString::number(mid)));
    QVERIFY2(parts.details->maximumHeight() > 0
                     && parts.details->maximumHeight() < parts.details->sizeHint().height(),
             qPrintable(QString::number(parts.details->maximumHeight())));
    QTRY_COMPARE(WinUI3::Private::progress(parts.card,
                                           WinUI3::Private::settingsCardExpansionProgressProperty),
                 1.0);
    QCOMPARE(parts.details->maximumHeight(), QWIDGETSIZE_MAX);
    QTRY_COMPARE(parts.details->height(), parts.details->sizeHint().height());
    QCOMPARE(parts.details->y(), headerHeight);

    // Clicking inside the expanded content must not toggle the card.
    QTest::mouseClick(parts.details, Qt::LeftButton, Qt::NoModifier,
                      parts.details->rect().center());
    QTest::qWait(20);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), true);

    // Keyboard: Space collapses (167 ms), Enter expands again (333 ms).
    parts.card->setFocus();
    QTest::keyClick(parts.card, Qt::Key_Space);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), false);
    setAnimationTime(parts.card, 60);
    const qreal collapsing = WinUI3::Private::progress(
            parts.card, WinUI3::Private::settingsCardExpansionProgressProperty);
    QVERIFY2(collapsing > 0.0 && collapsing < 1.0, qPrintable(QString::number(collapsing)));
    QTRY_COMPARE(parts.details->maximumHeight(), 0);
    QTest::keyClick(parts.card, Qt::Key_Return);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), true);
    QTRY_COMPARE(WinUI3::Private::progress(parts.card,
                                           WinUI3::Private::settingsCardExpansionProgressProperty),
                 1.0);
}

void WinUI3SettingsCardTest::settingsCardStockExpansionAnimationLifecycle_data()
{
    QTest::addColumn<bool>("applicationStyle");
    QTest::newRow("application-style") << true;
    QTest::newRow("widget-style") << false;
}

void WinUI3SettingsCardTest::settingsCardStockExpansionAnimationLifecycle()
{
    QFETCH(bool, applicationStyle);
    WinUI3::Style replacement(WinUI3::ThemeMode::Light);
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    layout->setContentsMargins(20, 20, 20, 20);
    const StockSettingsCard parts = makeStockSettingsCard();
    layout->addWidget(parts.card);
    host.resize(480, 320);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    const QPoint headerPoint(40, parts.header->height() / 2);

    // Finished animations must not accumulate on a surviving card.
    QTest::mouseClick(parts.card, Qt::LeftButton, Qt::NoModifier, headerPoint);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), true);
    auto *animation = parts.card->findChild<QVariantAnimation *>();
    QVERIFY(animation);
    QPointer<QVariantAnimation> guarded(animation);
    animation->setCurrentTime(animation->duration());
    QTRY_VERIFY_WITH_TIMEOUT(!guarded, 3000);
    QVERIFY(parts.card->findChildren<QVariantAnimation *>().isEmpty());
    QCOMPARE(parts.details->maximumHeight(), QWIDGETSIZE_MAX);

    QTest::mouseClick(parts.card, Qt::LeftButton, Qt::NoModifier, headerPoint);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), false);
    auto *collapse = parts.card->findChild<QVariantAnimation *>();
    QVERIFY(collapse);
    QCOMPARE(collapse->state(), QAbstractAnimation::Running);
    QPointer<QVariantAnimation> guardedCollapse(collapse);
    if (applicationStyle)
        qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
    else
        parts.card->setStyle(&replacement);
    drainDeferredDeletion();
    QVERIFY2(!guardedCollapse, "style replacement left a tracked expansion alive");
    QVERIFY(parts.card->findChildren<QVariantAnimation *>().isEmpty());
    QCOMPARE(parts.details->maximumHeight(), 0);
    QCOMPARE(WinUI3::Private::progress(parts.card,
                                       WinUI3::Private::settingsCardExpansionProgressProperty),
             0.0);
    QTest::mouseClick(parts.card, Qt::LeftButton, Qt::NoModifier, headerPoint);
    QCOMPARE(parts.card->property(WinUI3::Style::SettingsCardExpandedProperty).toBool(), true);
    auto *replacementAnimation = parts.card->findChild<QVariantAnimation *>();
    QVERIFY(replacementAnimation);
    QCOMPARE(replacementAnimation->state(), QAbstractAnimation::Running);
    replacementAnimation->setCurrentTime(replacementAnimation->duration());
    drainDeferredDeletion();
    QVERIFY(parts.card->findChildren<QVariantAnimation *>().isEmpty());
    QCOMPARE(parts.details->maximumHeight(), QWIDGETSIZE_MAX);
    QCOMPARE(WinUI3::Private::progress(parts.card,
                                       WinUI3::Private::settingsCardExpansionProgressProperty),
             1.0);
}

void WinUI3SettingsCardTest::settingsCardStockTrailingCardStaysInert()
{
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    auto *card = new QFrame;
    card->setProperty(WinUI3::Style::SettingsCardProperty, true);
    auto *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->addWidget(new QLabel(QStringLiteral("Notifications")));
    auto *toggle = new QCheckBox;
    WinUI3::Style::setToggleSwitch(toggle);
    cardLayout->addWidget(toggle);
    layout->addWidget(card);
    host.resize(480, 160);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    QCOMPARE(WinUI3::Private::settingsCardExpansionHost(card), nullptr);
    QCOMPARE(WinUI3::Private::settingsCardInteractive(card), false);
    QCOMPARE(card->focusPolicy(), Qt::NoFocus);
    QTest::mouseClick(card, Qt::LeftButton, Qt::NoModifier, QPoint(40, 20));
    QTest::qWait(20);
    QCOMPARE(card->property(WinUI3::Style::SettingsCardExpandedProperty).isValid(), false);
    QCOMPARE(toggle->isChecked(), false);
}

void WinUI3SettingsCardTest::settingsCardContentGeometry_data()
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
void WinUI3SettingsCardTest::settingsCardContentGeometry()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    WinUI3::SettingsCard card;
    WinUI3::Style::setDensityMode(
            &card, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    card.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    card.setTitle(QStringLiteral("Advanced"));
    card.setIcon(WinUI3::icon(WinUI3::Icon::Settings));
    auto *details = new QLabel(QStringLiteral("Details"));
    card.setExpandableWidget(details);
    card.resize(480, 200);
    card.show();
    card.setExpanded(true);
    QTRY_COMPARE(card.property("expansionProgress").toReal(), 1.0);
    auto *host = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_expandableHost"));
    QCOMPARE(host->x(), 0);
    QCOMPARE(host->width(), card.width());
    QCOMPARE(host->layout()->contentsMargins(),
             rtl ? QMargins(45, 7, 59, 9) : QMargins(59, 7, 45, 9));
    QCOMPARE(details->mapTo(&card, QPoint()).x(), rtl ? 45 : 59);
    auto *title = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_title"));
    const int contentLeading = details->mapTo(&card, QPoint()).x() + (rtl ? details->width() : 0);
    const int titleLeading = title->mapTo(&card, QPoint()).x() + (rtl ? title->width() : 0);
    QCOMPARE(contentLeading, titleLeading);
    QCOMPARE(details->geometry().center().y(), (host->height() - 9 + 7 - 1) / 2);
    QCOMPARE(host->sizeHint().height(), 51);
    QVERIFY(host->height() >= 51);

    const int seam = host->y() - 1;
    const QImage image = card.grab().toImage();
    const auto t = WinUI3::Private::tokens(card.palette());
    QCOMPARE(t.cardStroke, QColor(0, 0, 0, dark ? 25 : 15));
    QImage expected(1, 1, QImage::Format_ARGB32_Premultiplied);
    expected.fill(image.pixelColor(card.width() / 2, seam - 1));
    QPainter painter(&expected);
    painter.fillRect(expected.rect(), t.cardStroke);
    painter.end();
    QCOMPARE(image.pixelColor(card.width() / 2, seam), expected.pixelColor(0, 0));
}

void WinUI3SettingsCardTest::settingsCardNestedExpansionSeparator_data()
{
    settingsCardContentGeometry_data();
}
void WinUI3SettingsCardTest::settingsCardNestedExpansionSeparator()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    qobject_cast<WinUI3::Style *>(qApp->style())
            ->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QFrame outer;
    WinUI3::Style::setSettingsCard(&outer);
    WinUI3::Style::setDensityMode(
            &outer, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    outer.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    outer.resize(480, 400);
    auto *inner = new WinUI3::SettingsCard(&outer);
    inner->setTitle(QStringLiteral("Nested"));
    inner->setExpandableWidget(new QLabel(QStringLiteral("Details")));
    inner->setGeometry(80, 160, 300, 200);
    outer.show();
    inner->setExpanded(true);
    QTRY_COMPARE(inner->property("expansionProgress").toReal(), 1.0);
    auto *host = inner->findChild<QWidget *>(QStringLiteral("_winui_settings_card_expandableHost"));
    const int wrongSeam = host->y() - 1;
    QVERIFY(wrongSeam < inner->y());
    const QImage image = outer.grab().toImage();
    // A nested host has no right to add a separator to the outer card.
    QCOMPARE(image.pixelColor(20, wrongSeam), image.pixelColor(20, wrongSeam - 1));
}

void WinUI3SettingsCardTest::settingsCardInheritedDirectionAndTrailingInset_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("rtl");
    QTest::addColumn<bool>("expandable");
    for (bool compact : { false, true })
        for (bool rtl : { false, true })
            for (bool expandable : { false, true })
                QTest::newRow(qPrintable(
                        QStringLiteral("%1-%2-%3").arg(compact).arg(rtl).arg(expandable)))
                        << compact << rtl << expandable;
}
void WinUI3SettingsCardTest::settingsCardInheritedDirectionAndTrailingInset()
{
    QFETCH(bool, compact);
    QFETCH(bool, rtl);
    QFETCH(bool, expandable);
    QWidget parent;
    parent.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    WinUI3::SettingsCard card(&parent);
    WinUI3::Style::setDensityMode(
            &card, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    card.setTitle(QStringLiteral("Setting"));
    card.setIcon(WinUI3::icon(WinUI3::Icon::Settings));
    auto *trailing = new QPushButton(QStringLiteral("Action"));
    trailing->setFixedWidth(100);
    card.setTrailingWidget(trailing);
    if (expandable)
        card.setExpandableWidget(new QLabel(QStringLiteral("Details")));
    card.resize(600, 180);
    parent.resize(600, 180);
    parent.show();
    QCoreApplication::processEvents();
    const int x = trailing->mapTo(&card, QPoint()).x();
    QCOMPARE(rtl ? x : card.width() - x - trailing->width(), 17);
    auto *title = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_title"));
    const int titleX = title->mapTo(&card, QPoint()).x();
    QCOMPARE(rtl ? card.width() - titleX - title->width() : titleX, 59);
    auto *header = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    QCOMPARE(header->layout()->contentsMargins(), QMargins(17, 12, 17, 12));
    auto *grid = qobject_cast<QGridLayout *>(header->layout());
    QCOMPARE(grid->columnMinimumWidth(2), expandable ? 32 : 0);
    // Changing direction after construction must obey the same contract.
    card.setLayoutDirection(rtl ? Qt::LeftToRight : Qt::RightToLeft);
    QCoreApplication::processEvents();
    const int switchedX = trailing->mapTo(&card, QPoint()).x();
    QCOMPARE(rtl ? card.width() - switchedX - trailing->width() : switchedX, 17);
}

QTEST_MAIN(WinUI3SettingsCardTest)
#include "tst_winui3settingscard.moc"
