// SPDX-License-Identifier: LGPL-2.1-or-later
// ContentDialog surface tests, split out of tst_winui3surfaces.cpp to keep
// every domain binary under the 60 KB ratchet. Test-function names
// are kept identical, like the earlier domain splits.
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/lib/winui3tokens_p.h"

#include <QAbstractButton>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QEvent>
#include <QFont>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QMargins>
#include <QMessageBox>
#include <QPalette>
#include <QPoint>
#include <QPointer>
#include <QResizeEvent>
#include <QScopeGuard>
#include <QSize>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionDockWidget>
#include <QTest>
#include <QToolButton>

#include "../src/lib/winui3density_p.h"
#include "../src/lib/winui3helpers_p.h"
#include <QStyleOptionToolButton>
#include "../src/lib/winui3style_properties_p.h"
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
    void dialogStandardButtonIconsUseDistinctFluentGlyphs();
    void messageBoxRichTextLabelsDoNotOverlap();
    void wizardSurfaceContract();
    void wizardUsesModernStyleHint();
    void wizardOpenThemeSwitchLifecycle();
    void wizardHeaderUsesUniformSurface();
    void persistentDialogSurvivesThemeSwitch();
    void applicationStyleReplacementQueuesDialogFooterDeletion();
    void contentDialogScrimLifecycle();
    void contentDialogScrimOwnershipCleared();
    void messageBoxSyncGeometryConverges();
    void dockTitleButtonsStayFlatAndInset();
    void dockTitleComposition_data();
    void dockTitleComposition();
    void dockTitleOwnershipLifecycle();
    void dockTitleStyleSwitch_data();
    void dockTitleStyleSwitch();
    void dockTitleButtonBackdrop_data();
    void dockTitleButtonBackdrop();
    void dockTitleLargeFont_data();
    void dockTitleLargeFont();
    void dockTitleModifiedPlaceholder();
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

void WinUI3DialogsTest::dialogStandardButtonIconsUseDistinctFluentGlyphs()
{
    QStyle *style = QApplication::style();
    QCOMPARE(style->standardIcon(QStyle::SP_DialogSaveButton).cacheKey(),
             WinUI3::icon(WinUI3::Icon::Save).cacheKey());
    QCOMPARE(style->standardIcon(QStyle::SP_DialogYesButton).cacheKey(),
             WinUI3::icon(WinUI3::Icon::Accept).cacheKey());
    QCOMPARE(style->standardIcon(QStyle::SP_DialogOkButton).cacheKey(),
             WinUI3::icon(WinUI3::Icon::ReturnKey).cacheKey());
}

void WinUI3DialogsTest::messageBoxRichTextLabelsDoNotOverlap()
{
    // Reproducer for issue #51: QMessageBox keeps its title and informative
    // text in separate labels. A wrapped rich-text heading must not be moved
    // into the informative label when the ContentDialog body is centered.
    QMessageBox dialog(QMessageBox::Information, QStringLiteral("WinUI 3 Style"), {},
                       QMessageBox::No | QMessageBox::Yes);
    dialog.setTextFormat(Qt::RichText);
    dialog.setText(
            QStringLiteral("<h3>Version 0.1.0 of WinUI 3 Style has been<br/>released!</h3>"));
    dialog.setInformativeText(QStringLiteral(
            "Would you like to download the update now?<br/><br/>"
            "<strong>Change log:</strong><br/>"
            "Improved rich-text message layout, content spacing, and footer alignment."));
    dialog.setDefaultButton(QMessageBox::Yes);
    QFont messageFont = dialog.font();
    messageFont.setPointSize(12);
    dialog.setFont(messageFont);
    dialog.show();
    QTRY_VERIFY(dialog.isVisible());
    // The report's approximately 835x502 window and larger app font leave
    // excess body height and exercise the footer sync's centering path.
    dialog.setMinimumSize(640, 402);
    QTRY_COMPARE(dialog.size(), QSize(640, 402));

    auto *heading = dialog.findChild<QLabel *>(QStringLiteral("qt_msgbox_label"));
    auto *details = dialog.findChild<QLabel *>(QStringLiteral("qt_msgbox_informativelabel"));
    QVERIFY(heading);
    QVERIFY(details);
    QTRY_VERIFY(heading->isVisible());
    QTRY_VERIFY(details->isVisible());
    QVERIFY2(!heading->geometry().intersects(details->geometry()),
             "message heading overlaps the informative text");
    const int headingIndex = dialog.layout()->indexOf(heading);
    const int detailsIndex = dialog.layout()->indexOf(details);
    QVERIFY(headingIndex >= 0);
    QVERIFY(detailsIndex >= 0);
    const int headingLayoutY = dialog.layout()->itemAt(headingIndex)->geometry().y();
    const int detailsLayoutY = dialog.layout()->itemAt(detailsIndex)->geometry().y();
    // Simulate the title-label geometry update that arrives before the queued
    // footer sync on the affected QMessageBox layout. The sync may translate
    // the text block, but must preserve the relative gap between its labels.
    const int gapBeforeSync = details->geometry().top() - heading->geometry().bottom();
    heading->move(heading->x(), heading->y() + 24);
    const QSize size = dialog.size();
    QResizeEvent resizeEvent(size, size + QSize(1, 0));
    QCoreApplication::sendEvent(&dialog, &resizeEvent);
    QTRY_COMPARE(details->geometry().top() - heading->geometry().bottom(), gapBeforeSync);
    // Keep both labels tied to their layout rows after the sync settles.
    QCOMPARE(heading->y() - headingLayoutY, details->y() - detailsLayoutY);
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
    // Qt <= 6.11 rewrites wizStyle to AeroStyle on the first Show; the style
    // re-asserts ModernStyle during polish. See the dark-wizard block below.
    QCOMPARE(wizard.wizardStyle(), QWizard::ModernStyle);

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
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindChildrenRecursively);
    QVERIFY(darkFooter);
    QTRY_VERIFY(darkFooter->isVisible());
    // A zero-height strip is still "visible" and would silently paint nothing
    // (CI-only on Qt 6.8/6.11 until the host's own layout events re-synced it).
    // Pair the pixel below with the geometry that makes it meaningful.
    QTRY_VERIFY(darkFooter->height() >= 24);
    QVERIFY(darkFooter->mapTo(&darkWizard, QPoint(0, 0)).y() < darkWizard.height());
    // Mechanism guard for the pixel below: Qt <= 6.11 flips a wizard to
    // AeroStyle on its first Show even when SH_WizardStyle answers ModernStyle,
    // which offsets the content widget by the Vista title bar and moves the
    // command row out of the client rect. The style re-asserts ModernStyle in
    // polish; the content host must therefore still sit at the wizard's top.
    QCOMPARE(darkWizard.wizardStyle(), QWizard::ModernStyle);
    QCOMPARE(darkFooter->parentWidget()->mapTo(&darkWizard, QPoint(0, 0)).y(), 0);
    const QImage darkImage = darkWizard.grab().toImage();
    const QPoint darkBodyPixel = darkPage->mapTo(&darkWizard, QPoint(2, darkPage->height() / 2));
    QVERIFY(darkWizard.rect().contains(darkBodyPixel));
    QCOMPARE(darkImage.pixelColor(darkBodyPixel), QColor(44, 44, 44));
#ifdef Q_OS_WIN
    QCOMPARE(darkImage.pixelColor(darkWizard.width() / 2,
                                  darkFooter->mapTo(&darkWizard, QPoint(0, 2)).y()),
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
            QStringLiteral("_winui_wizard_footer_surface"), Qt::FindChildrenRecursively);
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
                                               Qt::FindChildrenRecursively);
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
    QCOMPARE(wizard.grab().toImage().pixelColor(wizard.width() / 2,
                                                footer->mapTo(&wizard, QPoint(0, 2)).y()),
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
    QTRY_COMPARE(wizard.grab().toImage().pixelColor(wizard.width() / 2,
                                                    footer->mapTo(&wizard, QPoint(0, 2)).y()),
                 darkCommand);
#endif
}

void WinUI3DialogsTest::wizardHeaderUsesUniformSurface()
{
    // Issue #73: QWizard's ModernStyle header paints its QWizardHeader, title
    // label and placeholder widgets with the QPalette::Base role and
    // autoFillBackground(). WinUI's dark Base is #3A3A3A@76, so those stacked
    // translucent fills showed up as "weird lighter rectangles" inside the
    // header of an otherwise #2C2C2C page. The wizard surface must be one
    // uniform fill: every Base-role autofill in the wizard subtree resolves to
    // the opaque content surface color. Dark/light; every pixel assertion is
    // paired with a palette/geometry assertion on the same state.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (const bool dark : { false, true }) {
        style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
        QWizard wizard;
        wizard.resize(520, 340);
        auto *page = new QWizardPage;
        page->setTitle(QStringLiteral("Welcome"));
        page->setSubTitle(QStringLiteral("A standard Qt wizard page."));
        wizard.addPage(page);
        wizard.show();
        QVERIFY(QTest::qWaitForWindowExposed(&wizard));
        QCOMPARE(wizard.wizardStyle(), QWizard::ModernStyle);
        // The style refreshes wizard palettes on the queued post-Show pass.
        QTest::qWait(50);
        const QColor content = page->palette().color(QPalette::Window);
        QCOMPARE(content.alpha(), 255);
        QList<QWidget *> headerChrome;
        for (QWidget *child : wizard.findChildren<QWidget *>()) {
            if (child->backgroundRole() == QPalette::Base && child->autoFillBackground())
                headerChrome.append(child);
        }
        QVERIFY(!headerChrome.isEmpty());
        for (QWidget *child : headerChrome) {
            // Mechanism: no stacked Base lift over the content surface.
            QCOMPARE(child->palette().color(QPalette::Base), content);
        }
        QWidget *header = headerChrome.first();
        QVERIFY(!header->geometry().isEmpty());
        const QImage image = wizard.grab().toImage();
        const QPoint headerProbe =
                header->mapTo(&wizard, QPoint(header->width() - 8, header->height() / 2));
        const QPoint bodyProbe = page->mapTo(&wizard, QPoint(2, page->height() / 2));
        QVERIFY(wizard.rect().contains(headerProbe));
        QVERIFY(wizard.rect().contains(bodyProbe));
        // Pixel evidence: the header band reads the exact same uniform surface
        // as the page body (RED pre-fix: a lighter #303030 / #FEFEFE band).
        QCOMPARE(image.pixelColor(headerProbe), content);
        QCOMPARE(image.pixelColor(bodyProbe), content);
    }
    style->setThemeMode(WinUI3::ThemeMode::Light);
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

void WinUI3DialogsTest::applicationStyleReplacementQueuesDialogFooterDeletion()
{
    // Keep the fixture's next test on WinUI: this binary does not restore a
    // replacement Fusion style in cleanup(). Construct this guard before the
    // widget scope, so its new WinUI style only arrives after those widgets
    // have been destroyed.
    const auto restoreStyle =
            qScopeGuard([] { qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light)); });
    {
        QWizard wizard;
        wizard.resize(520, 340);
        wizard.addPage(new QWizardPage);

        QDialog dialog;
        WinUI3::Style::setContentDialog(&dialog);
        auto *dialogLayout = new QVBoxLayout(&dialog);
        dialogLayout->addWidget(new QLabel(QStringLiteral("Content dialog")));
        dialogLayout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok));

        QMessageBox messageBox(QMessageBox::Information, QStringLiteral("WinUI 3 Style"),
                               QStringLiteral("Message box content."), QMessageBox::Ok);

        wizard.show();
        dialog.show();
        messageBox.show();
        QTRY_VERIFY(wizard.isVisible());
        QTRY_VERIFY(dialog.isVisible());
        QTRY_VERIFY(messageBox.isVisible());

        QWidget *wizardFooter = wizard.findChild<QWidget *>(
                QStringLiteral("_winui_wizard_footer_surface"), Qt::FindChildrenRecursively);
        QWidget *dialogFooter = dialog.findChild<QWidget *>(
                QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
        QWidget *messageBoxFooter = messageBox.findChild<QWidget *>(
                QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
        QVERIFY(wizardFooter);
        QVERIFY(dialogFooter);
        QVERIFY(messageBoxFooter);
        QPointer<QWidget> wizardFooterGuard(wizardFooter);
        QPointer<QWidget> dialogFooterGuard(dialogFooter);
        QPointer<QWidget> messageBoxFooterGuard(messageBoxFooter);

        // This must exercise QApplication's allWidgets snapshot, rather than
        // calling Style::unpolish() directly. The old footer surfaces remain
        // alive for that snapshot but are no longer visible or findable.
        qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
        QVERIFY(wizardFooterGuard);
        QVERIFY(dialogFooterGuard);
        QVERIFY(messageBoxFooterGuard);
        QVERIFY(!wizardFooterGuard->isVisible());
        QVERIFY(!dialogFooterGuard->isVisible());
        QVERIFY(!messageBoxFooterGuard->isVisible());
        QVERIFY(!wizard.findChild<QWidget *>(QStringLiteral("_winui_wizard_footer_surface"),
                                             Qt::FindChildrenRecursively));
        QVERIFY(!dialog.findChild<QWidget *>(QStringLiteral("_winui_content_dialog_footer_surface"),
                                             Qt::FindDirectChildrenOnly));
        QVERIFY(!messageBox.findChild<QWidget *>(
                QStringLiteral("_winui_content_dialog_footer_surface"),
                Qt::FindDirectChildrenOnly));

        // Repolishing can create new named surfaces, but it must never reuse
        // the old snapshot members that are waiting for deferred destruction.
        qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
        QWidget *replacementWizardFooter = wizard.findChild<QWidget *>(
                QStringLiteral("_winui_wizard_footer_surface"), Qt::FindChildrenRecursively);
        QWidget *replacementDialogFooter = dialog.findChild<QWidget *>(
                QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
        QWidget *replacementMessageBoxFooter = messageBox.findChild<QWidget *>(
                QStringLiteral("_winui_content_dialog_footer_surface"), Qt::FindDirectChildrenOnly);
        QVERIFY(replacementWizardFooter);
        QVERIFY(replacementDialogFooter);
        QVERIFY(replacementMessageBoxFooter);
        QVERIFY(replacementWizardFooter != wizardFooterGuard.data());
        QVERIFY(replacementDialogFooter != dialogFooterGuard.data());
        QVERIFY(replacementMessageBoxFooter != messageBoxFooterGuard.data());

        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(!wizardFooterGuard);
        QVERIFY(!dialogFooterGuard);
        QVERIFY(!messageBoxFooterGuard);
    }
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

        // Keep the legacy internal-button hint, but the stock composition now
        // owns its panel through the explicit Subtle role (#87).
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
        QVERIFY(dock.titleBarWidget());
        for (QAbstractButton *button : dock.titleBarWidget()->findChildren<QAbstractButton *>()) {
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

void WinUI3DialogsTest::dockTitleComposition_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("vertical");
    QTest::addColumn<bool>("rtl");
    QTest::addColumn<bool>("dark");
    for (bool compact : { false, true })
        for (bool vertical : { false, true })
            for (bool rtl : { false, true })
                for (bool dark : { false, true })
                    QTest::newRow(qPrintable(QStringLiteral("%1-%2-%3-%4")
                                                     .arg(compact)
                                                     .arg(vertical)
                                                     .arg(rtl)
                                                     .arg(dark)))
                            << compact << vertical << rtl << dark;
}

void WinUI3DialogsTest::dockTitleComposition()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    DockTitleProbe dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QStyleOptionDockWidget option;
    dock.fillStyleOption(&option);
    // RED mechanism #87: Qt's internal title measures 35 px and a floating
    // dock uses native decoration unless a non-null titleBarWidget is installed.
    QCOMPARE(vertical ? option.rect.width() : option.rect.height(), compact ? 24 : 32);
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    QCOMPARE(title->metaObject(), &QWidget::staticMetaObject);
    auto *close = title->findChild<QToolButton *>(QStringLiteral("_winui_dock_close"));
    auto *floating = title->findChild<QToolButton *>(QStringLiteral("_winui_dock_float"));
    QVERIFY(close);
    QVERIFY(floating);
    QCOMPARE(close->metaObject(), &QToolButton::staticMetaObject);
    QCOMPARE(close->focusPolicy(), Qt::NoFocus);
    QCOMPARE(close->accessibleName(), QStringLiteral("Close"));
    QCOMPARE(floating->accessibleName(), QStringLiteral("Float"));
    const auto verifyGeometry = [&] {
        dock.fillStyleOption(&option);
        option.rect = title->rect();
        QCOMPARE(close->geometry(),
                 style->subElementRect(QStyle::SE_DockWidgetCloseButton, &option, &dock));
        QCOMPARE(floating->geometry(),
                 style->subElementRect(QStyle::SE_DockWidgetFloatButton, &option, &dock));
        QCOMPARE(close->size(), QSize(20, 20));
        QCOMPARE(close->iconSize(), QSize(12, 12));
        QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    };
    verifyGeometry();
    const auto t = WinUI3::Private::tokens(title->palette());
    const auto titlePixel = [&](const QColor &overlay) {
        QImage pixel(1, 1, QImage::Format_ARGB32_Premultiplied);
        pixel.fill(t.surface);
        QPainter painter(&pixel);
        painter.fillRect(pixel.rect(), t.layer);
        painter.fillRect(pixel.rect(), overlay);
        painter.end();
        return pixel.pixelColor(0, 0);
    };
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(close, &leave);
    QCoreApplication::sendEvent(floating, &leave);
    setFrame(close, WinUI3::Private::hoverProperty, 0.0);
    setFrame(close, WinUI3::Private::pressProperty, 0.0);
    const QPoint sample =
            close->mapTo(title, QPoint(3, close->height() / 2)) * title->devicePixelRatioF();
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(Qt::transparent));
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(close, &enter);
    QTRY_VERIFY(frameReal(close, WinUI3::Private::hoverProperty) > 0.99);
    QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(t.subtleHover));
    QTest::mousePress(close, Qt::LeftButton);
    QVERIFY(close->isDown());
    QCOMPARE(close->size(), QSize(20, 20));
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(t.subtlePressed));
    // Release outside the button, keeping the dock open for the lifecycle pass.
    QTest::mouseRelease(close, Qt::LeftButton, Qt::NoModifier, QPoint(-2, -2));
    dock.setEnabled(false);
    QVERIFY(!close->isEnabled());
    QCOMPARE(WinUI3::Style::controlRole(close), WinUI3::ControlRole::Subtle);
    QCOMPARE(title->grab().toImage().pixelColor(sample), titlePixel(Qt::transparent));
    dock.setEnabled(true);
    dock.setWindowTitle(QStringLiteral("Updated Inspector"));
    QCOMPARE(dock.titleBarWidget(), title);
    // Stock QWidget ignores title events, so Qt still handles double-click
    // float/redock rather than a helper implementing its own mouse semantics.
    const QPoint titlePoint =
            style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock).center();
    QTest::mouseDClick(title, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_VERIFY(dock.isFloating());
    QTest::mouseDClick(title, Qt::LeftButton, Qt::NoModifier, titlePoint);
    QTRY_VERIFY(!dock.isFloating());
    QTest::mouseClick(floating, Qt::LeftButton);
    QTRY_VERIFY(dock.isFloating());
    QCOMPARE(dock.titleBarWidget(), title);
    QVERIFY(dock.windowFlags() & Qt::FramelessWindowHint);
    dock.resize(380, 230);
    QCoreApplication::processEvents();
    verifyGeometry();
    QTest::mouseClick(floating, Qt::LeftButton);
    QTRY_VERIFY(!dock.isFloating());
    dock.setFeatures(vertical ? QDockWidget::DockWidgetVerticalTitleBar
                              : QDockWidget::NoDockWidgetFeatures);
    QTRY_VERIFY(close->isHidden());
    QTRY_VERIFY(floating->isHidden());
    QCoreApplication::processEvents();
    QCOMPARE(vertical ? title->sizeHint().width() : title->sizeHint().height(), compact ? 24 : 32);
    QVERIFY(!title->geometry().intersects(dock.widget()->geometry()));
    for (auto feature : { QDockWidget::DockWidgetClosable, QDockWidget::DockWidgetFloatable }) {
        dock.setFeatures(feature
                         | (vertical ? QDockWidget::DockWidgetVerticalTitleBar
                                     : QDockWidget::NoDockWidgetFeatures));
        QCoreApplication::processEvents();
        dock.fillStyleOption(&option);
        option.rect = title->rect();
        QToolButton *visible = feature == QDockWidget::DockWidgetClosable ? close : floating;
        QToolButton *hidden = feature == QDockWidget::DockWidgetClosable ? floating : close;
        QVERIFY(!visible->isHidden());
        QVERIFY(hidden->isHidden());
        const auto visibleElement = feature == QDockWidget::DockWidgetClosable
                ? QStyle::SE_DockWidgetCloseButton
                : QStyle::SE_DockWidgetFloatButton;
        const auto hiddenElement = feature == QDockWidget::DockWidgetClosable
                ? QStyle::SE_DockWidgetFloatButton
                : QStyle::SE_DockWidgetCloseButton;
        QCOMPARE(visible->geometry(), style->subElementRect(visibleElement, &option, &dock));
        QVERIFY(style->subElementRect(hiddenElement, &option, &dock).isEmpty());
        QVERIFY(!visible->geometry().intersects(
                style->subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, &dock)));
    }
    dock.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    QTRY_VERIFY(!close->isHidden());
    // A local inherited density change and orientation change must update the
    // fixed perpendicular extent, not leave the old minimum width/height.
    host.setProperty("winuiDensity", compact ? "standard" : "compact");
    QTRY_COMPARE(title->height(), compact ? 32 : 24);
    dock.setLayoutDirection(rtl ? Qt::LeftToRight : Qt::RightToLeft);
    QCoreApplication::processEvents();
    verifyGeometry();
    QTest::mouseClick(close, Qt::LeftButton);
    QTRY_VERIFY(!dock.isVisible());
}

void WinUI3DialogsTest::dockTitleLargeFont_data()
{
    dockTitleComposition_data();
}

void WinUI3DialogsTest::dockTitleLargeFont()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    DockTitleProbe dock(QStringLiteral("Large title"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(640, 360);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    QFont font = dock.font();
    font.setPixelSize(40);
    dock.setFont(font);
    font.setWeight(QFont::DemiBold);
    const int extent = qMax(20, QFontMetrics(font).height()) + (compact ? 4 : 12);
    QTRY_COMPARE(vertical ? title->width() : title->height(), extent);
    QCOMPARE(vertical ? title->sizeHint().width() : title->sizeHint().height(), extent);
    QTRY_VERIFY(!title->geometry().intersects(dock.widget()->geometry()));
    font.setPixelSize(12);
    dock.setFont(font);
    QTRY_COMPARE(vertical ? title->width() : title->height(), compact ? 24 : 32);
    QTRY_VERIFY(!title->geometry().intersects(dock.widget()->geometry()));
}

void WinUI3DialogsTest::dockTitleModifiedPlaceholder()
{
    QMainWindow host;
    DockTitleProbe dock(QStringLiteral("Document[*]"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Content")));
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.isVisible());
    QWidget *title = dock.titleBarWidget();
    QVERIFY(title);
    for (const QString &caption : { QStringLiteral("Document[*]"), QStringLiteral("Document[*][*]"),
                                    QStringLiteral("Document[*][*][*] - Editor[*]"),
                                    QStringLiteral("[*] title [*] suffix") }) {
        dock.setWindowTitle(caption);
        for (bool modified : { false, true, false }) {
            dock.setWindowModified(modified);
            QCoreApplication::processEvents();
            QStyleOptionDockWidget native;
            dock.fillStyleOption(&native);
            QStyleOptionDockWidget owned;
            WinUI3::Private::dockTitleStyleOption(title, &owned);
            QCOMPARE(owned.title, native.title);
        }
    }
}

void WinUI3DialogsTest::dockTitleOwnershipLifecycle()
{
    QMainWindow host;
    QDockWidget custom(QStringLiteral("Custom"), &host);
    auto *appTitle = new QWidget;
    custom.setTitleBarWidget(appTitle);
    host.addDockWidget(Qt::LeftDockWidgetArea, &custom);
    host.show();
    custom.ensurePolished();
    QCOMPARE(custom.titleBarWidget(), appTitle);
    QStyle *style = host.style();
    style->unpolish(&custom);
    QCOMPARE(custom.titleBarWidget(), appTitle);
    style->polish(&custom);
    QCOMPARE(custom.titleBarWidget(), appTitle);

    QDockWidget dock(QStringLiteral("Owned"), &host);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    dock.show();
    dock.ensurePolished();
    QPointer<QWidget> owned = dock.titleBarWidget();
    QVERIFY(owned);
    style->polish(&dock);
    QCOMPARE(dock.titleBarWidget(), owned.data());
    style->unpolish(&dock);
    QVERIFY(!dock.titleBarWidget());
    QVERIFY(owned); // Qt's style-switch allWidgets snapshot must stay valid.
    style->polish(&dock);
    QVERIFY(dock.titleBarWidget());
    QVERIFY(dock.titleBarWidget() != owned);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!owned);
    QPointer<QWidget> cleared = dock.titleBarWidget();
    dock.setTitleBarWidget(nullptr);
    style->polish(&dock);
    QVERIFY(dock.titleBarWidget());
    QVERIFY(dock.titleBarWidget() != cleared);
    QVERIFY(cleared);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!cleared);
    QPointer<QWidget> second = dock.titleBarWidget();
    auto *replacement = new QWidget;
    dock.setTitleBarWidget(replacement);
    style->polish(&dock);
    QCOMPARE(dock.titleBarWidget(), replacement);
    style->unpolish(&dock);
    QCOMPARE(dock.titleBarWidget(), replacement);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!second);
}

void WinUI3DialogsTest::dockTitleStyleSwitch_data()
{
    dockTitleComposition_data();
}

void WinUI3DialogsTest::dockTitleStyleSwitch()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    const auto restoreStyle =
            qScopeGuard([] { qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light)); });
    QMainWindow host;
    host.setProperty("winuiDensity", compact ? "compact" : "standard");
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    QDockWidget dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.show();
    dock.setFloating(true);
    QTRY_VERIFY(dock.isVisible());
    QPointer<QWidget> oldTitle = dock.titleBarWidget();
    QVERIFY(oldTitle);
    QPointer<QToolButton> oldClose =
            oldTitle->findChild<QToolButton *>(QStringLiteral("_winui_dock_close"));
    QVERIFY(oldClose);
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QVERIFY(!dock.titleBarWidget());
    QVERIFY(oldTitle);
    QVERIFY(oldClose);
    QVERIFY(oldTitle->isHidden());
    qApp->setStyle(new WinUI3::Style(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light));
    QWidget *newTitle = dock.titleBarWidget();
    QVERIFY(newTitle);
    QVERIFY(newTitle != oldTitle);
    QTRY_COMPARE(vertical ? newTitle->width() : newTitle->height(), compact ? 24 : 32);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!oldTitle);
    QVERIFY(!oldClose);
    dock.setFloating(false);
    QCOMPARE(dock.titleBarWidget(), newTitle);
}

void WinUI3DialogsTest::dockTitleButtonBackdrop_data()
{
    dockTitleComposition_data();
}

void WinUI3DialogsTest::dockTitleButtonBackdrop()
{
    QFETCH(bool, compact);
    QFETCH(bool, vertical);
    QFETCH(bool, rtl);
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    QMainWindow host;
    host.setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
    QDockWidget dock(QStringLiteral("Inspector"), &host);
    dock.setWidget(new QLabel(QStringLiteral("Dock content")));
    if (vertical)
        dock.setFeatures(dock.features() | QDockWidget::DockWidgetVerticalTitleBar);
    host.addDockWidget(Qt::RightDockWidgetArea, &dock);
    host.resize(520, 260);
    host.show();
    QTRY_VERIFY(dock.titleBarWidget());
    QWidget *title = dock.titleBarWidget();
    QTRY_COMPARE(vertical ? title->width() : title->height(), compact ? 24 : 32);
    // Isolated renderer contract only, not evidence of a native DWM grant.
    // The existing Mica suite uses the same effective-surface fixture.
    host.setProperty("_winui_backdrop", 1);
    host.setProperty("_winui_backdrop_effective", 2);
    QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(title));
    const auto t = WinUI3::Private::tokens(title->palette());
    const QColor surface = WinUI3::Private::cardRefillTone(title->palette(), true);
    const auto buttons = title->findChildren<QToolButton *>();
    QCOMPARE(buttons.size(), 2);
    for (auto *button : buttons) {
        QVERIFY(WinUI3::Private::paintsDirectlyOnBackdrop(button));
        for (int state : { 0, 1, 2, 3 }) {
            QStyleOptionToolButton option;
            option.initFrom(button);
            option.rect = button->rect();
            option.state = state == 3 ? QStyle::State_None : QStyle::State_Enabled;
            if (state == 1)
                option.state |= QStyle::State_MouseOver;
            if (state == 2)
                option.state |= QStyle::State_Sunken;
            setFrame(button, WinUI3::Private::hoverProperty, state == 1 ? 1.0 : 0.0);
            setFrame(button, WinUI3::Private::pressProperty, state == 2 ? 1.0 : 0.0);
            QCOMPARE(button->size(), QSize(20, 20));
            QCOMPARE(WinUI3::Style::controlRole(button), WinUI3::ControlRole::Subtle);
            QImage expected(button->size(), QImage::Format_ARGB32_Premultiplied);
            expected.fill(surface);
            {
                QPainter painter(&expected);
                painter.fillRect(expected.rect(),
                                 state == 1           ? t.subtleHover
                                         : state == 2 ? t.subtlePressed
                                                      : QColor(Qt::transparent));
            }
            QImage frame(button->size(), QImage::Format_ARGB32_Premultiplied);
            frame.fill(Qt::red);
            {
                QPainter painter(&frame);
                style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
            }
            QCOMPARE(frame.pixelColor(3, 10), expected.pixelColor(3, 10));
            QVERIFY(frame.pixelColor(3, 10).alpha() >= surface.alpha());
            const QImage first = frame;
            {
                QPainter painter(&frame);
                style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, button);
            }
            QCOMPARE(button->iconSize(), QSize(12, 12));
            QCOMPARE(frame, first); // No accumulation or command-shaped alpha holes.
        }
    }
}

QTEST_MAIN(WinUI3DialogsTest)
#include "tst_winui3dialogs.moc"
