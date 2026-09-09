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

class WinUI3InteractionTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void styleMutationRestoration();
    void accessibilityOwnershipContracts();
    void baseStyleContract();
    void settingsCardExpansion();
    void settingsCardDesignerPropertyBindings();
    void settingsCardTrailingWidgetsReceiveClicks();
    void settingsCardTrailingWidgetsHaveUniformHeight();
    void settingsCardChevronAndStableHeader();
    void settingsCardExpansionLoad();
    void settingsCardInteractiveFrames();
    void settingsCardExpansionInScrollingPage();
    void inputModalityFocus();
    void hoverAnimationProgresses();
    void readOnlyActionRestoration();
    void animatedStackEffectsAndInterruption();
    void animatedStackLifecycleStress();
    void rtlGeometryAndHitTesting();
    void runtimeAppearanceAndDialogLifecycle();
    void callbackCoalescingAndAnimationReuse();
};

void WinUI3InteractionTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3InteractionTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3InteractionTest::cleanup()
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

void WinUI3InteractionTest::styleMutationRestoration()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QCommonStyle replacementStyle;
    QLineEdit pendingHelperUpdate;
    style->polish(&pendingHelperUpdate);
    QVERIFY(frameValue(&pendingHelperUpdate, "_winui_line_edit_helper_update_pending").isValid());
    style->unpolish(&pendingHelperUpdate);
    QVERIFY(!frameValue(&pendingHelperUpdate, "_winui_line_edit_helper_update_pending").isValid());
    pendingHelperUpdate.setStyle(&replacementStyle);
    QCoreApplication::processEvents();
    QVERIFY(!frameValue(&pendingHelperUpdate, "_winui_line_edit_helper_update_pending").isValid());

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
    QCOMPARE(inheritedPaletteButton.palette().color(QPalette::ButtonText), QColor(147, 42, 73));

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

    // Unknown public properties must be ignored without crashing: a typo
    // like "Acennt" falls back to Standard, and an out-of-range numeric
    // role clamps to the same default instead of casting garbage.
    QWidget unknownRole;
    unknownRole.setProperty("winuiControlRole", QStringLiteral("Acennt"));
    QCOMPARE(WinUI3::Style::controlRole(&unknownRole), WinUI3::ControlRole::Standard);
    unknownRole.setProperty("winuiControlRole", 9999);
    QCOMPARE(WinUI3::Style::controlRole(&unknownRole), WinUI3::ControlRole::Standard);
    unknownRole.setProperty("winuiDensity", QStringLiteral("ultacompact"));
    QCOMPARE(WinUI3::Style::densityMode(&unknownRole), WinUI3::DensityMode::Standard);
    QWidget unknownSurface;
    unknownSurface.setProperty("winuiSurface", QStringLiteral("marble"));
    style->polish(&unknownSurface);
    style->unpolish(&unknownSurface);
}

void WinUI3InteractionTest::accessibilityOwnershipContracts()
{
    QLineEdit editor;
    editor.setAccessibleName(QStringLiteral("Project name"));
    editor.setAccessibleDescription(QStringLiteral("Name of the current project"));
    QCOMPARE(editor.accessibleName(), QStringLiteral("Project name"));
    QCOMPARE(editor.accessibleDescription(), QStringLiteral("Name of the current project"));
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

void WinUI3InteractionTest::baseStyleContract()
{
    WinUI3::Style style(WinUI3::ThemeMode::Light);
    QVERIFY(style.baseStyle());
    QCOMPARE(QString::fromLatin1(style.baseStyle()->metaObject()->className()),
             QStringLiteral("QCommonStyle"));
    QCOMPARE(style.styleHint(QStyle::SH_Menu_MouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_MenuBar_MouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_ListMouseTracking), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_Popup), 1);
    QCOMPARE(style.styleHint(QStyle::SH_ComboBox_PopupFrameStyle), int(QFrame::NoFrame));
    QCOMPARE(style.styleHint(QStyle::SH_Slider_AbsoluteSetButtons), int(Qt::LeftButton));
    QCOMPARE(style.styleHint(QStyle::SH_ToolButtonStyle), int(Qt::ToolButtonFollowStyle));

    QStyleOptionMenuItem item;
    item.font = qApp->font();
    item.fontMetrics = QFontMetrics(item.font);
    const QSize contents(item.fontMetrics.horizontalAdvance(QStringLiteral("File")),
                         item.fontMetrics.height());
    const QSize result = style.sizeFromContents(QStyle::CT_MenuBarItem, &item, contents);
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

void WinUI3InteractionTest::settingsCardExpansion()
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
    QVERIFY2(pressed > 0.0 && pressed < 1.0, qPrintable(QString::number(pressed)));
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

void WinUI3InteractionTest::settingsCardDesignerPropertyBindings()
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

void WinUI3InteractionTest::settingsCardTrailingWidgetsReceiveClicks()
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

void WinUI3InteractionTest::settingsCardTrailingWidgetsHaveUniformHeight()
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

void WinUI3InteractionTest::settingsCardChevronAndStableHeader()
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
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronRight));
    const QRect chevronInCard(chevron->mapTo(&card, chevron->rect().topLeft()), chevron->size());
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

void WinUI3InteractionTest::settingsCardExpansionLoad()
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

void WinUI3InteractionTest::settingsCardInteractiveFrames()
{
    WinUI3::SettingsCard card;
    card.setTitle(QStringLiteral("Display").toUpper());
    card.setDescription(
            QStringLiteral("The header must keep its geometry while the details are revealed."));
    card.setExpandableWidget(new SolidPage(QColor(40, 120, 200), QStringLiteral("Details")));
    card.resize(480, 240);
    card.show();
    QCoreApplication::processEvents();

    auto *header = card.findChild<QWidget *>(QStringLiteral("_winui_settings_card_headerHost"));
    auto *title = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_title"));
    auto *description =
            card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_description"));
    auto *chevron = card.findChild<QLabel *>(QStringLiteral("_winui_settings_card_chevron"));
    auto *animation = card.findChild<QVariantAnimation *>(
            QStringLiteral("_winui_settings_card_expansion_animation"));
    QVERIFY(header);
    QVERIFY(title);
    QVERIFY(description);
    QVERIFY(chevron);
    QVERIFY(animation);
    QVERIFY(chevron->isVisible());
    QVERIFY(!labelPixmap(chevron).isNull());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronRight));

    const QRect headerGeometry = header->geometry();
    const QRect titleGeometry = title->geometry();
    const QRect descriptionGeometry = description->geometry();
    const QImage collapsed = card.grab().toImage();
    const QPixmap collapsedChevron = labelPixmap(chevron);

    QTest::mouseClick(&card, Qt::LeftButton, Qt::NoModifier, header->geometry().center());
    QCOMPARE(card.isExpanded(), true);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    const QImage first = card.grab().toImage();
    QTRY_VERIFY(labelPixmap(chevron).toImage() != collapsedChevron.toImage());
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
    QVERIFY(!labelPixmap(chevron).isNull());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QEvent dprChange(QEvent::DevicePixelRatioChange);
    QCoreApplication::sendEvent(&card, &dprChange);
#endif
    QVERIFY(!labelPixmap(chevron).isNull());
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

void WinUI3InteractionTest::settingsCardExpansionInScrollingPage()
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

void WinUI3InteractionTest::inputModalityFocus()
{
    QPushButton button(QStringLiteral("Focus"));
    button.resize(button.sizeHint());
    button.show();
    QMouseEvent mousePress(QEvent::MouseButtonPress, QPointF(4, 4), Qt::LeftButton, Qt::LeftButton,
                           Qt::NoModifier);
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

void WinUI3InteractionTest::hoverAnimationProgresses()
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
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(&button, "_winui_hover_progress") > 0.0, 150);
    const qreal midway = frameReal(&button, "_winui_hover_progress");
    QVERIFY2(midway > 0.0 && midway < 1.0, qPrintable(QString::number(midway)));
    QTRY_VERIFY(frameReal(&button, "_winui_hover_progress") > 0.99);
    QCOMPARE(probe.frameChanges, 0);
    QVERIFY(!button.property("_winui_hover_progress").isValid());
}

void WinUI3InteractionTest::readOnlyActionRestoration()
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
        const bool custom =
                actionAssociatedWith(&leading, button) || actionAssociatedWith(&trailing, button);
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

void WinUI3InteractionTest::animatedStackEffectsAndInterruption()
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

void WinUI3InteractionTest::animatedStackLifecycleStress()
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
                    QStringLiteral("_winui_animated_stack_group"), Qt::FindDirectChildrenOnly)) {
            group->setCurrentTime(group->duration());
            QCoreApplication::processEvents();
        }
        QCOMPARE(stack.findChildren<QParallelAnimationGroup *>(
                              QStringLiteral("_winui_animated_stack_group"),
                              Qt::FindDirectChildrenOnly)
                         .size(),
                 0);
        QCOMPARE(stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
                                               Qt::FindDirectChildrenOnly)
                         .size(),
                 0);
    };

    stack.setCurrentIndex(1);
    QVERIFY(stack.isAnimating());
    stack.resize(480, 160);
    auto overlays = stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
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
                             Qt::FindDirectChildrenOnly)
                        .size()
                <= 1);
        QVERIFY(stack.findChildren<QWidget *>(QStringLiteral("_winui_animated_stack_overlay"),
                                              Qt::FindDirectChildrenOnly)
                        .size()
                <= 1);
        if (i % 10 == 0) {
            stack.resize(320 + i, 120 + (i % 4) * 10);
            overlays = stack.findChildren<QWidget *>(
                    QStringLiteral("_winui_animated_stack_overlay"), Qt::FindDirectChildrenOnly);
            if (!overlays.isEmpty())
                QCOMPARE(overlays.constFirst()->geometry(), stack.rect());
        }
    }
    settle();
    QCOMPARE(stack.currentWidget()->geometry(), stack.rect());
}

void WinUI3InteractionTest::rtlGeometryAndHitTesting()
{
    QComboBox combo;
    combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
    combo.setLayoutDirection(Qt::RightToLeft);
    combo.resize(220, 32);
    combo.show();
    QStyleOptionComboBox comboOption;
    comboOption.initFrom(&combo);
    comboOption.rect = combo.rect();
    comboOption.direction = Qt::RightToLeft;
    const QRect arrow = combo.style()->subControlRect(QStyle::CC_ComboBox, &comboOption,
                                                      QStyle::SC_ComboBoxArrow, &combo);
    const QRect edit = combo.style()->subControlRect(QStyle::CC_ComboBox, &comboOption,
                                                     QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY(arrow.left() == combo.rect().left());
    QVERIFY(edit.right() < combo.rect().right());
    QVERIFY(edit.left() == arrow.right() + 1);
    QVERIFY(!arrow.intersects(edit));
    QCOMPARE(combo.style()->hitTestComplexControl(QStyle::CC_ComboBox, &comboOption, arrow.center(),
                                                  &combo),
             QStyle::SC_ComboBoxArrow);

    QGroupBox group(QStringLiteral("RTL group"));
    group.setCheckable(true);
    group.setLayoutDirection(Qt::RightToLeft);
    group.resize(240, 100);
    QStyleOptionGroupBox groupOption;
    groupOption.initFrom(&group);
    groupOption.rect = group.rect();
    groupOption.direction = Qt::RightToLeft;
    groupOption.subControls =
            QStyle::SC_GroupBoxFrame | QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxLabel;
    const QRect check = group.style()->subControlRect(QStyle::CC_GroupBox, &groupOption,
                                                      QStyle::SC_GroupBoxCheckBox, &group);
    QVERIFY(check.left() > group.rect().center().x());
    QCOMPARE(group.style()->hitTestComplexControl(QStyle::CC_GroupBox, &groupOption, check.center(),
                                                  &group),
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
    const QRect sliderHandle = slider.style()->subControlRect(QStyle::CC_Slider, &sliderOption,
                                                              QStyle::SC_SliderHandle, &slider);
    QCOMPARE(slider.style()->hitTestComplexControl(QStyle::CC_Slider, &sliderOption,
                                                   sliderHandle.center(), &slider),
             QStyle::SC_SliderHandle);
    const int before = slider.value();
    QTest::mouseClick(&slider, Qt::RightButton, Qt::NoModifier,
                      QPoint(slider.width() - 4, slider.height() / 2));
    QTest::mouseClick(&slider, Qt::MiddleButton, Qt::NoModifier,
                      QPoint(slider.width() - 4, slider.height() / 2));
    QCOMPARE(slider.value(), before);
}

void WinUI3InteractionTest::runtimeAppearanceAndDialogLifecycle()
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    QVERIFY(qApp->palette().color(QPalette::Accent) != accent);
#endif
    auto *watchdog =
            style->findChild<QTimer *>(QStringLiteral("_winui_system_appearance_watchdog"));
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
                          QStringLiteral("_winui_dialog_animation"), Qt::FindDirectChildrenOnly)
                    .size()
            <= 1);
    dialog.hide();
    QVERIFY(!dialog.property("_winui_dialog_animating").toBool());
    QCOMPARE(dialog.windowOpacity(), 1.0);
    QVERIFY(dialog.findChildren<QParallelAnimationGroup *>(
                          QStringLiteral("_winui_dialog_animation"), Qt::FindDirectChildrenOnly)
                    .isEmpty());

    style->setAccentColor({});
    style->setThemeMode(WinUI3::ThemeMode::Light);
}

void WinUI3InteractionTest::callbackCoalescingAndAnimationReuse()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    {
        QSlider slider(Qt::Horizontal);
        slider.setRange(0, 100);
        slider.resize(320, 40);
        slider.show();
        QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, slider.rect().center());
        auto *timer = slider.findChild<QTimer *>(QStringLiteral("_winui_slider_tooltip_timer"),
                                                 Qt::FindDirectChildrenOnly);
        QVERIFY(timer);
        QSignalSpy callbacks(timer, &QTimer::timeout);
        for (int i = 0; i < 1000; ++i) {
            slider.setValue(i % 100);
            QMouseEvent move(QEvent::MouseMove, QPointF(slider.rect().center()), Qt::NoButton,
                             Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(&slider, &move);
        }
        slider.setValue(77);
        QVERIFY(timer->isActive());
        QCoreApplication::processEvents();
        QCOMPARE(callbacks.count(), 1);
        QCOMPARE(frameValue(&slider, "_winui_slider_tooltip_value").toString(),
                 QStringLiteral("77"));
        QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, slider.rect().center());
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
        auto *timer = scrollBar.findChild<QTimer *>(QStringLiteral("_winui_scrollbar_timer"),
                                                    Qt::FindDirectChildrenOnly);
        QVERIFY(timer);
        QSignalSpy callbacks(timer, &QTimer::timeout);
        for (int i = 0; i < 100; ++i) {
            QCoreApplication::sendEvent(&scrollBar, &leave);
            QCoreApplication::sendEvent(&scrollBar, &enter);
        }
        QCoreApplication::sendEvent(&scrollBar, &leave);
        QCOMPARE(scrollBar
                         .findChildren<QTimer *>(QStringLiteral("_winui_scrollbar_timer"),
                                                 Qt::FindDirectChildrenOnly)
                         .size(),
                 1);
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

QTEST_MAIN(WinUI3InteractionTest)
#include "tst_winui3interaction.moc"
