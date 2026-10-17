// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
// SettingsCard interaction contracts live in tst_winui3settingscard.cpp
// (S6 ratchet split); settingsCardInteractiveFrames stays here (DPI-pinned).
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3style_properties_p.h"
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
    void polishUnpolishRestoresEverySavedAttribute();
    void surfaceSnapshotOwnershipSymmetry();
    void runtimeExplicitPaletteOwnership();
    void ownedPaletteRefreshSurvivesOwnerRegistration();
    void accessibilityOwnershipContracts();
    void baseStyleContract();
    void settingsCardInteractiveFrames();
    void inputModalityFocus();
    void hoverAnimationProgresses();
    void readOnlyActionRestoration();
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

void WinUI3InteractionTest::ownedPaletteRefreshSurvivesOwnerRegistration()
{
    // Calendar palette changes can construct/polish children synchronously.
    // Registration must not invalidate the owner's active refresh traversal.
    struct RegisterOnPaletteChange final : QObject
    {
        QWidget *host = nullptr;
        bool fired = false;
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (!fired && event->type() == QEvent::PaletteChange) {
                fired = true;
                for (int i = 0; i < 256; ++i) {
                    auto *child = new QLabel(QStringLiteral("New palette owner"), host);
                    child->ensurePolished();
                }
            }
            return false;
        }
    } registration;
    QWidget host;
    QCalendarWidget calendar(&host);
    for (int i = 0; i < 32; ++i) {
        auto *child = new QLabel(QStringLiteral("Existing palette owner"), &host);
        child->ensurePolished();
    }
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    registration.host = &host;
    calendar.installEventFilter(&registration);
    host.setProperty("_winui_backdrop", 1);
    host.setProperty("_winui_backdrop_effective", 2);
    QVERIFY(registration.fired);
    QCOMPARE(host.findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly).size(), 288);
    host.setProperty("_winui_backdrop_effective", 0);
    const QPalette expected = qApp->palette();
    for (QLabel *child : host.findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly))
        QCOMPARE(child->palette().color(QPalette::Window), expected.color(QPalette::Window));
    calendar.removeEventFilter(&registration);
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

void WinUI3InteractionTest::polishUnpolishRestoresEverySavedAttribute()
{
    // S4 gate: Style::polish() saves a fixed set of widget state (palette,
    // autofill, WA_Hover, WA_OpaquePaintEvent, WA_TranslucentBackground,
    // WA_NoSystemBackground, contents margins, list spacing, frame shape,
    // control role); unpolish() must restore each from its pre-polish value.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QWidget widget;
    const QPalette originalPalette = widget.palette();
    widget.setAutoFillBackground(true);
    widget.setAttribute(Qt::WA_Hover, false);
    widget.setAttribute(Qt::WA_OpaquePaintEvent, false);
    widget.setAttribute(Qt::WA_TranslucentBackground, false);
    widget.setAttribute(Qt::WA_NoSystemBackground, false);
    widget.setContentsMargins(11, 12, 13, 14);
    const bool originalAutoFill = widget.autoFillBackground();
    const bool originalOpaque = widget.testAttribute(Qt::WA_OpaquePaintEvent);
    const bool originalTranslucent = widget.testAttribute(Qt::WA_TranslucentBackground);
    const bool originalNoSystem = widget.testAttribute(Qt::WA_NoSystemBackground);
    const bool originalHover = widget.testAttribute(Qt::WA_Hover);
    const QMargins originalMargins = widget.contentsMargins();
    QVERIFY(originalAutoFill);
    QVERIFY(!originalHover);
    QCOMPARE(originalMargins, QMargins(11, 12, 13, 14));

    style->polish(&widget);
    // Polish arms hover and must leave every other saved attribute alone.
    QVERIFY(widget.testAttribute(Qt::WA_Hover));
    QVERIFY(widget.autoFillBackground());
    QCOMPARE(widget.contentsMargins(), originalMargins);
    QCOMPARE(widget.testAttribute(Qt::WA_OpaquePaintEvent), originalOpaque);
    style->unpolish(&widget);
    QCOMPARE(widget.palette(), originalPalette);
    QVERIFY(!widget.testAttribute(Qt::WA_SetPalette));
    QCOMPARE(widget.autoFillBackground(), originalAutoFill);
    QCOMPARE(widget.testAttribute(Qt::WA_Hover), originalHover);
    QCOMPARE(widget.testAttribute(Qt::WA_OpaquePaintEvent), originalOpaque);
    QCOMPARE(widget.testAttribute(Qt::WA_TranslucentBackground), originalTranslucent);
    QCOMPARE(widget.testAttribute(Qt::WA_NoSystemBackground), originalNoSystem);
    QCOMPARE(widget.contentsMargins(), originalMargins);

    // A second polish must re-arm from the cleaned state, not a leftover one.
    style->polish(&widget);
    QVERIFY(widget.testAttribute(Qt::WA_Hover));
    style->unpolish(&widget);
    QCOMPARE(widget.autoFillBackground(), originalAutoFill);

    // Only popup list views get their spacing zeroed; a plain view keeps the
    // application value (see styleMutationRestoration).
    QListView list;
    list.setSpacing(9);
    const int originalListSpacing = list.spacing();
    QCOMPARE(originalListSpacing, 9);
    style->polish(&list);
    QCOMPARE(list.spacing(), originalListSpacing);
    style->unpolish(&list);
    QCOMPARE(list.spacing(), originalListSpacing);

    QWidget roleWidget;
    QVERIFY(!roleWidget.property("_winui_control_role").isValid());
    style->polish(&roleWidget);
    WinUI3::Style::setControlRole(&roleWidget, WinUI3::ControlRole::Accent);
    QCOMPARE(WinUI3::Style::controlRole(&roleWidget), WinUI3::ControlRole::Accent);
    style->unpolish(&roleWidget);
    QVERIFY(!roleWidget.property("_winui_control_role").isValid());

    // Documented asymmetry: unpolish() restores originalTranslucentBackground /
    // originalNoSystemBackground for every widget, but the only writers are
    // preparePopupSurface (popup windows) and the backdrop remember
    // (top-level windows, under a different key). For a plain child nothing
    // saves them, so those two blocks are no-ops here.
    QWidget opaque;
    opaque.setAttribute(Qt::WA_TranslucentBackground, true);
    opaque.setAttribute(Qt::WA_NoSystemBackground, true);
    style->polish(&opaque);
    QVERIFY(!opaque.property(WinUI3::Private::originalTranslucentBackgroundProperty).isValid());
    QVERIFY(!opaque.property(WinUI3::Private::originalNoSystemBackgroundProperty).isValid());
    style->unpolish(&opaque);
    QVERIFY(opaque.testAttribute(Qt::WA_TranslucentBackground));
    QVERIFY(opaque.testAttribute(Qt::WA_NoSystemBackground));
}

// S4 gate, extended to the arm/disarm orders (measured 2026-10-17 with a
// temporary probe): Style::polish() saves widget attributes, the ContentDialog
// arm forces its own temporary values onto the same widget, and Style::unpolish()
// is the one that hands the saved ones back. Whichever order runs, every saved
// attribute must return to its pre-polish value and no saved key may outlive the
// unpolish: a leftover key would make the next polish snapshot a temporary value.
void WinUI3InteractionTest::surfaceSnapshotOwnershipSymmetry()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    // polish -> arm (show) -> disarm (hide) -> unpolish.
    {
        QDialog dialog;
        WinUI3::Style::setContentDialog(&dialog);
        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(3, 4, 5, 6);
        layout->setSpacing(7);
        layout->setSizeConstraint(QLayout::SetMinimumSize);
        dialog.setMinimumSize(200, 100);
        dialog.setMaximumSize(640, 480);
        dialog.setAutoFillBackground(false);
        const QPalette originalPalette = dialog.palette();

        style->polish(&dialog);
        // The arm owns the dialog while it is open.
        QCOMPARE(dialog.minimumSize(), QSize(320, 184));
        QCOMPARE(layout->contentsMargins(), QMargins(24, 24, 24, 24));
        QCOMPARE(layout->spacing(), 12);
        QVERIFY(dialog.autoFillBackground());

        dialog.show();
        QTRY_VERIFY(dialog.isVisible());
        dialog.hide();
        QCoreApplication::processEvents();

        style->unpolish(&dialog);
        QCOMPARE(dialog.minimumSize(), QSize(200, 100));
        QCOMPARE(dialog.maximumSize(), QSize(640, 480));
        QCOMPARE(layout->contentsMargins(), QMargins(3, 4, 5, 6));
        QCOMPARE(layout->spacing(), 7);
        QCOMPARE(int(layout->sizeConstraint()), int(QLayout::SetMinimumSize));
        QCOMPARE(dialog.palette(), originalPalette);
        QVERIFY(!dialog.autoFillBackground());
        QVERIFY(!dialog.property(WinUI3::Private::originalPaletteProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalAutoFillProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalMinimumSizeProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalMaximumSizeProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalMarginsProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalSpacingProperty).isValid());
        QVERIFY(!dialog.property(WinUI3::Private::originalLayoutConstraintProperty).isValid());
    }

    // polish -> arm (show) -> unpolish while the dialog is still open: unpolish
    // is the only restorer left that can hand the values back, and the later
    // hide must not re-arm the cleaned dialog.
    {
        QDialog dialog;
        WinUI3::Style::setContentDialog(&dialog);
        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(3, 4, 5, 6);
        layout->setSpacing(7);
        dialog.setMinimumSize(200, 100);
        dialog.setAutoFillBackground(false);

        style->polish(&dialog);
        dialog.show();
        QTRY_VERIFY(dialog.isVisible());
        style->unpolish(&dialog);
        QCOMPARE(dialog.minimumSize(), QSize(200, 100));
        QCOMPARE(layout->contentsMargins(), QMargins(3, 4, 5, 6));
        QCOMPARE(layout->spacing(), 7);
        QVERIFY(!dialog.autoFillBackground());
        QVERIFY(!dialog.property(WinUI3::Private::originalAutoFillProperty).isValid());

        dialog.hide();
        QCoreApplication::processEvents();
        QCOMPARE(dialog.minimumSize(), QSize(200, 100));
        QCOMPARE(layout->contentsMargins(), QMargins(3, 4, 5, 6));
        QCOMPARE(layout->spacing(), 7);
    }

    // Popup window and its viewport: the popup arm writes the same autoFill /
    // opaque keys the polish snapshot uses, and unpolish restores both widgets
    // from them.
    {
        QListView popup(nullptr);
        popup.setWindowFlag(Qt::Popup);
        popup.ensurePolished();
        popup.viewport()->setAutoFillBackground(true);
        popup.viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);
        style->polish(&popup);
        style->polish(popup.viewport());
        popup.show();
        QTRY_VERIFY(popup.isVisible());
        popup.hide();
        QCoreApplication::processEvents();

        style->unpolish(popup.viewport());
        style->unpolish(&popup);
        QVERIFY(popup.viewport()->autoFillBackground());
        QVERIFY(popup.viewport()->testAttribute(Qt::WA_OpaquePaintEvent));
        QVERIFY(!popup.property(WinUI3::Private::originalOpaquePaintProperty).isValid());
        QVERIFY(!popup.property(WinUI3::Private::originalTranslucentBackgroundProperty).isValid());
        QVERIFY(!popup.property(WinUI3::Private::originalNoSystemBackgroundProperty).isValid());
        QVERIFY(!popup.viewport()->property(WinUI3::Private::originalAutoFillProperty).isValid());
    }
}

void WinUI3InteractionTest::runtimeExplicitPaletteOwnership()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QLineEdit editor;
    QLineEdit inherited;
    editor.setText(QStringLiteral("Palette override"));
    editor.show();
    inherited.show();
    QTRY_VERIFY(editor.isVisible());

    QPalette custom = editor.palette();
    const QColor userText(31, 85, 143);
    custom.setColor(QPalette::Text, userText);
    editor.setPalette(custom);
    QVERIFY(editor.testAttribute(Qt::WA_SetPalette));

    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QCOMPARE(editor.palette().color(QPalette::Text), userText);
    QCOMPARE(inherited.palette().color(QPalette::Text),
             style->standardPalette().color(QPalette::Text));
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QCOMPARE(editor.palette().color(QPalette::Text), userText);
    QCOMPARE(inherited.palette().color(QPalette::Text),
             style->standardPalette().color(QPalette::Text));
    style->unpolish(&editor);
    QCOMPARE(editor.palette().color(QPalette::Text), userText);
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
    const auto chevronRotation = [&] {
        return chevron->property("_winui_settings_card_chevron_rotation").toReal();
    };
    QVERIFY(!labelPixmap(chevron).isNull());
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    QCOMPARE(chevronRotation(), 0.0);

    const QRect headerGeometry = header->geometry();
    const QRect titleGeometry = title->geometry();
    const QRect descriptionGeometry = description->geometry();
    const QImage collapsed = card.grab().toImage();

    QTest::mouseClick(&card, Qt::LeftButton, Qt::NoModifier, header->geometry().center());
    QCOMPARE(card.isExpanded(), true);
    // Glyph angle tracks expansion progress.
    QTRY_VERIFY(qAbs(chevronRotation() - card.property("expansionProgress").toReal() * 180.0)
                < 0.01);
    QCOMPARE(chevron->property("_winui_settings_card_chevron_glyph").toInt(),
             static_cast<int>(WinUI3::Icon::ChevronDown));
    const QImage first = card.grab().toImage();
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
    const qreal mid = card.property("expansionProgress").toReal();
    QVERIFY(mid > 0.0 && mid < 1.0);
    QCOMPARE(chevronRotation(), mid * 180.0);

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
    QCOMPARE(chevronRotation(), 180.0);
}

void WinUI3InteractionTest::inputModalityFocus()
{
    QPushButton button(QStringLiteral("Focus"));
    button.resize(button.sizeHint());
    button.show();
    QMouseEvent mousePress(QEvent::MouseButtonPress, QPointF(4, 4), QPointF(4, 4), Qt::LeftButton,
                           Qt::LeftButton, Qt::NoModifier);
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

// The AnimatedStack widget-lifetime tests (animatedStackEffectsAndInterruption,
// animatedStackLifecycleStress, animatedStackFinishRetrigger,
// animatedStackDeferredReplay) live in tst_winui3animatedstack.cpp — see the
// domain-split note at tests/CMakeLists.txt.
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
            QMouseEvent move(QEvent::MouseMove, QPointF(slider.rect().center()),
                             QPointF(slider.rect().center()), Qt::NoButton, Qt::LeftButton,
                             Qt::NoModifier);
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
