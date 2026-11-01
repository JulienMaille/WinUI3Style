// SPDX-License-Identifier: LGPL-2.1-or-later
// ContentDialog surface tests, split out of tst_winui3surfaces.cpp (plan step
// S6: keep every domain binary under the 60 KB ratchet). Test-function names
// are kept identical, like the earlier domain splits.
#include <winui3style/winui3style.h>

#include "../src/winui3tokens_p.h"

#include <QAbstractButton>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QEvent>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QMargins>
#include <QMessageBox>
#include <QPalette>
#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QStyle>
#include <QStyleOptionDockWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

#include "winui3testhelpers.h"

class WinUI3DialogsTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void contentDialogContract();
    void messageBoxContentDialogContract();
    void wizardSurfaceContract();
    void wizardUsesModernStyleHint();
    void wizardOpenThemeSwitchLifecycle();
    void persistentDialogSurvivesThemeSwitch();
    void contentDialogScrimLifecycle();
    void contentDialogScrimOwnershipCleared();
    void messageBoxSyncGeometryConverges();
    void dockTitleButtonsStayFlatAndInset();
};

void WinUI3DialogsTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3DialogsTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3DialogsTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3DialogsTest::contentDialogContract()
{
    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Dialog content")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    if (auto *primary = buttons->button(QDialogButtonBox::Ok))
        primary->setDefault(true);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QVERIFY(dialog.minimumWidth() >= 320);
    QVERIFY(dialog.minimumHeight() >= 184);
    QCOMPARE(layout->contentsMargins(), QMargins(24, 24, 24, 24));
    QCOMPARE(layout->spacing(), 12);
    QCOMPARE(WinUI3::Style::controlRole(buttons->button(QDialogButtonBox::Ok)),
             WinUI3::ControlRole::Accent);
    QVERIFY(!buttons->autoFillBackground());
    QCOMPARE(dialog.palette().color(QPalette::Window), buttons->palette().color(QPalette::Window));
    QTRY_VERIFY(dialog.findChild<QWidget *>(QStringLiteral("_winui_content_dialog_footer_surface"),
                                            Qt::FindDirectChildrenOnly));
    QWidget *footer = dialog.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QTRY_COMPARE(footer->width(), dialog.width());
    QCOMPARE(footer->x(), 0);
    QCOMPARE(footer->geometry().bottom(), dialog.rect().bottom());
    QVERIFY(footer->height() > buttons->height());
    const int buttonsTop = buttons->mapTo(&dialog, QPoint(0, 0)).y();
    const int buttonsBottom = buttonsTop + buttons->height() - 1;
    const int upperInset = buttonsTop - footer->geometry().top();
    const int lowerInset = footer->geometry().bottom() - buttonsBottom;
    QVERIFY(qAbs(upperInset - lowerInset) <= 1);
    const QImage renderedDialog = dialog.grab().toImage();
    const QColor contentPixel = renderedDialog.pixelColor(5, 5);
    const QColor footerPixel = renderedDialog.pixelColor(5, renderedDialog.height() - 5);
    QVERIFY(colorDistance(contentPixel, footerPixel) > 4);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(dialog.palette().color(QPalette::Window), QColor(0x2C, 0x2C, 0x2C));
    const QImage darkDialog = dialog.grab().toImage();
    QCOMPARE(darkDialog.pixelColor(5, 5), QColor(0x2C, 0x2C, 0x2C));
    QCOMPARE(darkDialog.pixelColor(5, darkDialog.height() - 5), QColor(0x20, 0x20, 0x20));
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxInformation).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxWarning).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxCritical).isNull());
    QVERIFY(!dialog.style()->standardIcon(QStyle::SP_MessageBoxQuestion).isNull());
}

void WinUI3DialogsTest::messageBoxContentDialogContract()
{
    QMessageBox dialog(QMessageBox::Information, QStringLiteral("WinUI 3 Style"),
                       QStringLiteral("This is a native Qt message box rendered by the style."),
                       QMessageBox::Ok);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QTRY_VERIFY(dialog.height() >= 184);
    QCOMPARE(dialog.layout()->contentsMargins(), QMargins(24, 24, 24, 24));
    QCOMPARE(dialog.layout()->sizeConstraint(), QLayout::SetMinimumSize);

    auto *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    QWidget *footer = dialog.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    QTRY_COMPARE(footer->width(), dialog.width());
    const int buttonsTop = buttons->mapTo(&dialog, QPoint(0, 0)).y();
    const int buttonsBottom = buttonsTop + buttons->height() - 1;
    QCOMPARE(buttonsTop - footer->geometry().top(), footer->geometry().bottom() - buttonsBottom);

    auto *label = dialog.findChild<QLabel *>(QStringLiteral("qt_msgbox_label"));
    QVERIFY(label);
    QCOMPARE(label->focusPolicy(), Qt::NoFocus);
    QVERIFY(dialog.button(QMessageBox::Ok)->hasFocus());
}

void WinUI3DialogsTest::wizardSurfaceContract()
{
    QWizard wizard;
    wizard.resize(520, 340);
    auto *first = new QWizardPage;
    first->setTitle(QStringLiteral("Welcome"));
    first->setSubTitle(QStringLiteral("A standard Qt wizard page."));
    auto *firstLayout = new QVBoxLayout(first);
    firstLayout->addWidget(new QLabel(QStringLiteral("Wizard content")));
    wizard.addPage(first);
    auto *second = new QWizardPage;
    second->setTitle(QStringLiteral("Finish"));
    wizard.addPage(second);
    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));

    QVERIFY(wizard.autoFillBackground());
    QVERIFY(first->autoFillBackground());
    QCOMPARE(WinUI3::Style::controlRole(wizard.button(QWizard::NextButton)),
             WinUI3::ControlRole::Accent);
    QCOMPARE(WinUI3::Style::controlRole(wizard.button(QWizard::FinishButton)),
             WinUI3::ControlRole::Accent);

    const QImage image = wizard.grab().toImage();
    const QColor content = first->palette().color(QPalette::Window);
    const QColor commands = wizard.palette().color(QPalette::Window);
    const auto containsColor = [](const QImage &source, const QColor &target) {
        for (int y = 0; y < source.height(); ++y)
            for (int x = 0; x < source.width(); ++x)
                if (colorDistance(source.pixelColor(x, y), target) <= 8)
                    return true;
        return false;
    };
    QVERIFY(containsColor(image, content));
    QVERIFY(containsColor(image, commands));

    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QWizard darkWizard;
    darkWizard.resize(520, 340);
    auto *darkPage = new QWizardPage;
    darkPage->setTitle(QStringLiteral("Welcome"));
    darkPage->setSubTitle(QStringLiteral("A standard Qt wizard page."));
    auto *darkLayout = new QVBoxLayout(darkPage);
    darkLayout->addWidget(new QLabel(QStringLiteral("Wizard content")));
    darkWizard.addPage(darkPage);
    darkWizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&darkWizard));
    QCOMPARE(darkWizard.palette().color(QPalette::Window), QColor(44, 44, 44));
    QCOMPARE(darkPage->palette().color(QPalette::Window), QColor(44, 44, 44));
    QCOMPARE(darkWizard.button(QWizard::FinishButton)->palette().color(QPalette::Window),
             QColor(32, 32, 32));
    QCOMPARE(WinUI3::Style::controlRole(darkWizard.button(QWizard::FinishButton)),
             WinUI3::ControlRole::Accent);
    auto *darkFooter = darkWizard.findChild<QWidget *>(
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(darkFooter);
    QTRY_VERIFY(darkFooter->isVisible());
    const QImage darkImage = darkWizard.grab().toImage();
    const QPoint darkBodyPixel = darkPage->mapTo(&darkWizard, QPoint(2, darkPage->height() / 2));
    QVERIFY(darkWizard.rect().contains(darkBodyPixel));
    QCOMPARE(darkImage.pixelColor(darkBodyPixel), QColor(44, 44, 44));
#ifdef Q_OS_WIN
    QCOMPARE(darkImage.pixelColor(darkWizard.width() / 2, darkFooter->geometry().top() + 2),
             QColor(32, 32, 32));
#endif

    // Gallery path: the app starts in System mode and the combo sets the
    // raw index (0=System, 1=Light, 2=Dark). The style must answer
    // ModernStyle even before any wizard exists (live defect: blue
    // Classic header + white footer; QWizardPrivate caches wizStyle at
    // construction from the app style).
    style->setThemeMode(WinUI3::ThemeMode::System);
    QCOMPARE(qApp->style()->styleHint(QStyle::SH_WizardStyle), int(QWizard::ModernStyle));

    style->setThemeMode(WinUI3::ThemeMode::Light);
    QWizard runtimeWizard;
    runtimeWizard.resize(520, 340);
    auto *runtimePage = new QWizardPage;
    runtimePage->setTitle(QStringLiteral("Runtime theme"));
    runtimeWizard.addPage(runtimePage);
    runtimeWizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&runtimeWizard));
    QCOMPARE(runtimePage->palette().color(QPalette::Window), QColor(252, 252, 252));
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(runtimePage->palette().color(QPalette::Window), QColor(44, 44, 44));
    QTRY_COMPARE(runtimeWizard.button(QWizard::NextButton)->palette().color(QPalette::Window),
                 QColor(32, 32, 32));
    auto *runtimeFooter = runtimeWizard.findChild<QWidget *>(
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(runtimeFooter);
    QTRY_VERIFY(runtimeFooter->isVisible());
}

void WinUI3DialogsTest::wizardUsesModernStyleHint()
{
    // Live defect 2026-09-09: ClassicStyle draws a blue banner pixmap, a
    // hardcoded #003399 title and a white page; AeroStyle forces its own
    // white fills. ModernStyle takes the style palette in both themes.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWizard wizard;
    QCOMPARE(style->styleHint(QStyle::SH_WizardStyle, nullptr, &wizard), int(QWizard::ModernStyle));
    QCOMPARE(wizard.wizardStyle(), QWizard::ModernStyle);
}

void WinUI3DialogsTest::wizardOpenThemeSwitchLifecycle()
{
    // Live defect 2026-09-09: an open wizard kept a stale command-area
    // fill across Light/Dark switches (white footer in Dark). Switching
    // the theme with the wizard open must re-resolve the footer fill,
    // the page palette and the command-button palette, with no stale
    // theme pixels left behind.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QWizard wizard;
    wizard.resize(520, 340);
    auto *page = new QWizardPage;
    page->setTitle(QStringLiteral("Welcome"));
    wizard.addPage(page);
    wizard.show();
    QVERIFY(QTest::qWaitForWindowExposed(&wizard));
    auto *footer = wizard.findChild<QWidget *>(QStringLiteral("_winui_wizard_footer_surface"),
                                               Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    QTRY_VERIFY(footer->isVisible());
    QAbstractButton *next = wizard.button(QWizard::NextButton);
    QVERIFY(next);
    // QWizard's internal title/description labels must inherit the content
    // ink: a stale blue Link-role title on a dark page is unreadable (live
    // defect 2026-09-09).
    const auto titleLabels = wizard.findChildren<QLabel *>();
    QVERIFY(!titleLabels.isEmpty());
    const QColor darkContent(44, 44, 44);
    const QColor darkCommand(32, 32, 32);
    const QColor lightContent(252, 252, 252);
    const QColor lightCommand(243, 243, 243);
    QTRY_COMPARE(page->palette().color(QPalette::Window), darkContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), darkCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(255, 255, 255));
#ifdef Q_OS_WIN
    QCOMPARE(wizard.grab().toImage().pixelColor(wizard.width() / 2, footer->geometry().top() + 2),
             darkCommand);
#endif
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QTRY_COMPARE(page->palette().color(QPalette::Window), lightContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), lightCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(0, 0, 0, 228));
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    QTRY_COMPARE(page->palette().color(QPalette::Window), darkContent);
    QTRY_COMPARE(next->palette().color(QPalette::Window), darkCommand);
    for (QLabel *label : titleLabels)
        QTRY_COMPARE(label->palette().color(label->foregroundRole()), QColor(255, 255, 255));
#ifdef Q_OS_WIN
    QTRY_COMPARE(
            wizard.grab().toImage().pixelColor(wizard.width() / 2, footer->geometry().top() + 2),
            darkCommand);
#endif
}

void WinUI3DialogsTest::persistentDialogSurvivesThemeSwitch()
{
    // Dialog persistant : ouvert en Light, switch Dark, puis retour Light.
    // Meme etat, deux assertions : contenu et footer suivent le theme,
    // sans garder le fill clair.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QDialog dialog;
    WinUI3::Style::setContentDialog(&dialog);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("Persistent")));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    QWidget *footer = dialog.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    const QColor lightContent = style->standardPalette().color(QPalette::Window);
    QTRY_COMPARE(dialog.palette().color(QPalette::Window),
                 WinUI3::Private::popupSurfaceColor(style->standardPalette()));
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    const QColor darkDialog = WinUI3::Private::popupSurfaceColor(style->standardPalette());
    const QColor darkCommand(32, 32, 32);
    QTRY_COMPARE(dialog.palette().color(QPalette::Window), darkDialog);
    QTRY_COMPARE(footer->palette().color(QPalette::Window), darkCommand);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    QTRY_COMPARE(dialog.palette().color(QPalette::Window),
                 WinUI3::Private::popupSurfaceColor(style->standardPalette()));
}

void WinUI3DialogsTest::contentDialogScrimLifecycle()
{
    QWidget parent;
    parent.resize(640, 480);
    QDialog dialog(&parent);
    WinUI3::Style::setContentDialog(&dialog);
    parent.show();
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    const auto scrims =
            parent.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"));
    QCOMPARE(scrims.size(), 1);
    QVERIFY(scrims.first()->isVisible());
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    parent.resize(700, 500);
    qApp->processEvents();
    QCOMPARE(scrims.first()->geometry(), parent.rect());
    dialog.hide();
    qApp->processEvents();
    qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(parent.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"))
                    .isEmpty());
}

void WinUI3DialogsTest::contentDialogScrimOwnershipCleared()
{
    // Finding A mechanism: the scrim is parented to the owner's window while
    // its lifetime is tracked on the dialog. Both teardown orders must be
    // safe: owner-first must clear the tracked pointer (no deleteLater() on
    // freed memory), dialog-first must not orphan the scrim on the owner.
    // Pair each grab() with a token/geometry assert on the same state.
    constexpr auto scrimProperty = "_winui_content_dialog_scrim";

    // Scrim destroyed independently (e.g. owner teardown while the dialog
    // outlives it): the destroyed signal clears the tracked property
    // synchronously, so a later teardown is a no-op instead of a
    // deleteLater() on freed memory.
    {
        auto *owner = new QWidget;
        owner->resize(640, 480);
        owner->show();
        auto *dialog = new QDialog(owner);
        WinUI3::Style::setContentDialog(dialog);
        dialog->show();
        QTRY_VERIFY(dialog->isVisible());
        QObject *tracked = dialog->property(scrimProperty).value<QObject *>();
        QVERIFY(tracked != nullptr);
        QPointer<QObject> guarded(tracked);
        QImage withScrim = dialog->grab().toImage();
        QVERIFY(!withScrim.isNull());
        QCOMPARE(dialog->property(scrimProperty).value<QObject *>(), tracked);
        QCOMPARE(static_cast<QWidget *>(tracked)->geometry(), owner->rect());
        // The dialog outlives the owner from here on, as in the report.
        dialog->setParent(nullptr);
        dialog->show();
        QTRY_VERIFY(dialog->isVisible());
        delete owner;
        qApp->processEvents();
        QVERIFY(guarded.isNull());
        QVERIFY(!dialog->property(scrimProperty).value<QObject *>());
        // Public Hide flow after the owner teardown: must be a no-op,
        // never a deleteLater() on the freed scrim.
        dialog->hide();
        qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
        qApp->processEvents();
        delete dialog;
    }

    // Scrim destroyed directly (independent teardown path): same contract.
    {
        QWidget owner;
        owner.resize(640, 480);
        owner.show();
        QDialog dialog(&owner);
        WinUI3::Style::setContentDialog(&dialog);
        dialog.show();
        QTRY_VERIFY(dialog.isVisible());
        QObject *tracked = dialog.property(scrimProperty).value<QObject *>();
        QVERIFY(tracked != nullptr);
        QImage withScrim = dialog.grab().toImage();
        QVERIFY(!withScrim.isNull());
        QCOMPARE(dialog.property(scrimProperty).value<QObject *>(), tracked);
        delete tracked;
        qApp->processEvents();
        QVERIFY(!dialog.property(scrimProperty).value<QObject *>());
        // Public Hide flow after the independent teardown: must be a
        // no-op, never a deleteLater() on the freed scrim.
        dialog.hide();
        qApp->processEvents();
        QVERIFY(owner.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"))
                        .isEmpty());
    }

    // Dialog dies first: the scrim must not orphan on the owner window.
    {
        QWidget owner;
        owner.resize(640, 480);
        owner.show();
        auto *dialog = new QDialog(&owner);
        WinUI3::Style::setContentDialog(dialog);
        dialog->show();
        QTRY_VERIFY(dialog->isVisible());
        QCOMPARE(
                owner.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim")).size(),
                1);
        const QImage withScrim = owner.grab().toImage();
        QVERIFY(!withScrim.isNull());
        QCOMPARE(withScrim.size(), owner.size());
        delete dialog;
        qApp->sendPostedEvents(nullptr, QEvent::DeferredDelete);
        qApp->processEvents();
        QVERIFY(owner.findChildren<QWidget *>(QStringLiteral("_winui_content_dialog_scrim"))
                        .isEmpty());
    }
}

void WinUI3DialogsTest::messageBoxSyncGeometryConverges()
{
    // Finding C mechanism: syncGeometry() mutates constraint/min/max/size
    // from inside Resize/LayoutRequest/ChildAdded handling, which re-fires
    // the filter. Mutators must no-op once converged so an undersized
    // dialog (<184px) settles instead of ping-ponging forever.
    // Pair the grab() with a token/geometry assert on the same state.
    QMessageBox box(QMessageBox::Information, QStringLiteral("WinUI 3 Style"),
                    QStringLiteral("Undersized sync convergence."),
                    QMessageBox::Ok | QMessageBox::Cancel);
    box.show();
    QTRY_VERIFY(box.isVisible());
    // Force the undersized state the loop feeds on.
    box.resize(box.width(), 100);
    qApp->processEvents();
    QTest::qWait(50);
    qApp->processEvents();
    QTest::qWait(50);
    qApp->processEvents();
    QWidget *footer = box.findChild<QWidget *>(
            QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
    QVERIFY(footer);
    // Token assert on the same state: minimum asserted, constraint held.
    QVERIFY(box.minimumWidth() >= 320);
    QVERIFY(box.minimumHeight() >= 184);
    QCOMPARE(box.layout()->sizeConstraint(), QLayout::SetMinimumSize);
    // Geometry assert: the dialog converged (no shrinking ping-pong).
    QTRY_VERIFY(box.height() >= 184);
    const QSize settled = box.size();
    QTest::qWait(60);
    qApp->processEvents();
    QCOMPARE(box.size(), settled);
    const QImage rendered = box.grab().toImage();
    QVERIFY(!rendered.isNull());
    QCOMPARE(rendered.size(), settled);
    box.hide();
}

// QDockWidget::initStyleOption() is protected; this exposes the very option
// Qt hands the style when it lays out and paints the dock title bar.
class DockTitleProbe final : public QDockWidget
{
public:
    using QDockWidget::QDockWidget;

    void fillStyleOption(QStyleOptionDockWidget *option) const
    {
        const_cast<DockTitleProbe *>(this)->initStyleOption(option);
    }
};

void WinUI3DialogsTest::dockTitleButtonsStayFlatAndInset()
{
    // The assertions run once per density profile: the inset is a Fluent card
    // constant, so Compact must not inherit, shrink or grow it.
    const auto verifyDockTitleButtons = [](WinUI3::DensityMode mode) {
        // Live defect #43: the dock close button had no gap to the dock's end,
        // and QCommonStyle's SH_DockWidget_ButtonsHaveFrame gave these
        // Qt-internal buttons a stock tool-button frame. Fluent dock titles are
        // a card: the buttons stay flat, and every side keeps the same 8 px
        // content inset the title text already gets from
        // PM_DockWidgetTitleMargin.
        QMainWindow host;
        DockTitleProbe dock(QStringLiteral("Inspector"), &host);
        dock.setWidget(new QLabel(QStringLiteral("Dock content")));
        dock.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                         | QDockWidget::DockWidgetMovable);
        host.addDockWidget(Qt::RightDockWidgetArea, &dock);
        host.resize(520, 260);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        QStyle *style = host.style();
        QVERIFY(style);
        if (mode == WinUI3::DensityMode::Compact) {
            QVERIFY(style->setProperty("densityMode", int(WinUI3::DensityMode::Compact)));
            QCoreApplication::processEvents();
        }
        QStyleOptionDockWidget option;
        dock.fillStyleOption(&option);
        const QRect titleArea = option.rect;
        QVERIFY(!titleArea.isNull());

        // Qt consults this hint before it draws anything, so 0 keeps the frame
        // out of the dock title button's paint path entirely.
        QCOMPARE(style->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, &option, &dock), 0);

        // The pinned Fluent content inset: 8 px, the same value the title text
        // row already reports for PM_DockWidgetTitleMargin.
        QCOMPARE(style->pixelMetric(QStyle::PM_DockWidgetTitleMargin, &option, &dock), 8);
        const QRect closeRect =
                style->subElementRect(QStyle::SE_DockWidgetCloseButton, &option, &dock);
        const QRect floatRect =
                style->subElementRect(QStyle::SE_DockWidgetFloatButton, &option, &dock);
        QVERIFY(!closeRect.isNull());
        QVERIFY(!floatRect.isNull());
        // The trailing inset replaces QCommonStyle's flush placement: the close
        // button's right edge sits 8 px inside the title area, and the float
        // button keeps that inset one slot to the left instead of touching it.
        QCOMPARE(titleArea.right() - closeRect.right(), 8);
        QCOMPARE(titleArea.right() - floatRect.right(), 8 + closeRect.width() + 1);
        QVERIFY2(floatRect.right() < closeRect.left(),
                 qPrintable(QStringLiteral("float right=%1 close left=%2")
                                    .arg(floatRect.right())
                                    .arg(closeRect.left())));
        QVERIFY(!closeRect.intersects(floatRect));
        QCOMPARE(closeRect.height(), floatRect.height());
        // Both buttons share the title text's centered axis.
        QCOMPARE(closeRect.center().y(), floatRect.center().y());

        // Right-to-left mirrors the same inset off the title area's leading edge.
        QStyleOptionDockWidget mirrored = option;
        mirrored.direction = Qt::RightToLeft;
        const QRect mirroredClose =
                style->subElementRect(QStyle::SE_DockWidgetCloseButton, &mirrored, &dock);
        QVERIFY(!mirroredClose.isNull());
        QCOMPARE(mirroredClose.left() - titleArea.left(), 8);
        QCOMPARE(mirroredClose.width(), closeRect.width());
        QCOMPARE(mirroredClose.y(), closeRect.y());

        // The title row keeps the same 8 px inset on its leading edge. The base
        // style started it flush against the card's stroke, so "Inspector" read
        // as a clipped word; the row now begins where every other Fluent surface
        // starts its content.
        const QRect textRect =
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock);
        QVERIFY(!textRect.isNull());
        QCOMPARE(textRect.left() - titleArea.left(), 8);
        const QRect mirroredText =
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &mirrored, &dock);
        QCOMPARE(titleArea.right() - mirroredText.right(), 8);

        // The close glyph is still drawn, so a placement-only fix that emptied
        // the button would pass the hint and geometry checks above while
        // changing nothing the user sees.
        QAbstractButton *closeButton = nullptr;
        QAbstractButton *floatButton = nullptr;
        for (QAbstractButton *button : dock.findChildren<QAbstractButton *>()) {
            if (closeRect.contains(button->geometry().center()))
                closeButton = button;
            if (floatRect.contains(button->geometry().center()))
                floatButton = button;
        }
        QVERIFY2(closeButton, "qt dock title close button not found");
        QVERIFY2(floatButton, "qt dock title float button not found");
        // Qt sizes these glyphs from PM_SmallIconSize for the button widget but
        // from the slot metric for the layout, so the two only agree when the
        // title row carries the 12 px Fluent mark instead of the 16 px toolbar
        // one: 2 * PM_DockWidgetTitleBarButtonMargin + 12 == the slot width.
        QCOMPARE(style->pixelMetric(QStyle::PM_SmallIconSize, nullptr, closeButton), 12);
        QCOMPARE(style->pixelMetric(QStyle::PM_SmallIconSize, nullptr, floatButton), 12);
        QCOMPARE(closeButton->sizeHint(), closeRect.size());
        QCOMPARE(floatButton->sizeHint(), floatRect.size());
        // The float mark is the Fluent two-window glyph, not the stock Windows
        // restore pixmap whose filled faces and 2 px strokes outweighed the
        // close cross by more than 2:1.
        QVERIFY(WinUI3::isFluentIcon(
                style->standardIcon(QStyle::SP_TitleBarNormalButton, &option, &dock)));
        const qreal dpr = closeButton->devicePixelRatioF();
        // The whole dock is grabbed so the button and the card beside it come
        // from one image; both rects are in dock coordinates.
        const QImage dockImage = dock.grab().toImage();
        QVERIFY(!dockImage.isNull());
        // The card tone sampled in the gutter between the float and close
        // buttons, where no button can paint.
        const QColor cardFill =
                dockImage.pixelColor(qRound((floatRect.right() + closeRect.left()) / 2.0 * dpr),
                                     qRound(closeRect.center().y() * dpr));
        const int closeInk = inkPixels(dockImage, closeRect, dpr, cardFill);
        const int floatInk = inkPixels(dockImage, floatRect, dpr, cardFill);
        QVERIFY2(closeInk > 0, "dock title close glyph painted no ink");
        QVERIFY2(floatInk > 0, "dock title float glyph painted no ink");
        // Two 1 px outlines carry about the weight of one 1 px cross; the stock
        // restore mark carried twice that, which is the heaviness the live
        // review reported.
        QVERIFY2(floatInk * 2 <= closeInk * 3,
                 qPrintable(
                         QStringLiteral("float ink=%1 close ink=%2").arg(floatInk).arg(closeInk)));
    };

    verifyDockTitleButtons(WinUI3::DensityMode::Standard);
    verifyDockTitleButtons(WinUI3::DensityMode::Compact);
}

QTEST_MAIN(WinUI3DialogsTest)
#include "tst_winui3dialogs.moc"
