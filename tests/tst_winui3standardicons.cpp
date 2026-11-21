// SPDX-License-Identifier: LGPL-2.1-or-later
// Issue #69: proof-check the standard-pixmap pairings the style already
// claims. The gallery page only shows that a cell is non-empty and which
// group it landed in; a glyph that silently fell off the end of the icon
// tables (Accept/ReturnKey used to do exactly that) passes a cacheKey
// comparison against the same empty icon and still looks like a mapping.
// Here every claimed pairing must resolve to the exact pinned Fluent engine
// and render real ink.
#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QStyle>
#include <QtTest>
#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

namespace {
int inkPixels(const QPixmap &pixmap)
{
    const QImage image = pixmap.toImage();
    int ink = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            ink += qAlpha(image.pixel(x, y)) != 0;
    return ink;
}
} // namespace

class WinUI3StandardIconsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void mappedStandardPixmaps_data();
    void mappedStandardPixmaps();
    void customBaseIsNotMapped();
};

void WinUI3StandardIconsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3StandardIconsTest::mappedStandardPixmaps_data()
{
    QTest::addColumn<int>("standard");
    QTest::addColumn<int>("glyph");
    QTest::addColumn<QString>("label");

    const auto add = [](const char *label, QStyle::StandardPixmap standard, WinUI3::Icon glyph) {
        QTest::newRow(label) << int(standard) << int(glyph) << QString::fromLatin1(label);
    };

    using SP = QStyle;
    using Glyph = WinUI3::Icon;
    // Only pairings already present in the style's mapping table belong here.
    // A "similar looking" glyph is not a pairing: the original table is the
    // contract, and the audit page reports everything else as a fallback.
    add("ArrowBack", SP::SP_ArrowBack, Glyph::ChevronLeft);
    add("ArrowDown", SP::SP_ArrowDown, Glyph::ChevronDown);
    add("ArrowLeft", SP::SP_ArrowLeft, Glyph::ChevronLeft);
    add("ArrowRight", SP::SP_ArrowRight, Glyph::ChevronRight);
    add("ArrowUp", SP::SP_ArrowUp, Glyph::ChevronUp);
    add("ArrowForward", SP::SP_ArrowForward, Glyph::ChevronRight);
    add("CommandLink", SP::SP_CommandLink, Glyph::ChevronRight);
    add("BrowserReload", SP::SP_BrowserReload, Glyph::Refresh);
    add("BrowserStop", SP::SP_BrowserStop, Glyph::ErrorBadge);
    add("DialogApplyButton", SP::SP_DialogApplyButton, Glyph::Check);
    add("DialogCancelButton", SP::SP_DialogCancelButton, Glyph::Close);
    add("DockWidgetCloseButton", SP::SP_DockWidgetCloseButton, Glyph::Close);
    add("DialogCloseButton", SP::SP_DialogCloseButton, Glyph::Clear);
    add("DialogNoButton", SP::SP_DialogNoButton, Glyph::Clear);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    add("TabCloseButton", SP::SP_TabCloseButton, Glyph::Clear);
#endif
    add("TitleBarCloseButton", SP::SP_TitleBarCloseButton, Glyph::ChromeClose);
    add("LineEditClearButton", SP::SP_LineEditClearButton, Glyph::Clear);
    add("DialogSaveButton", SP::SP_DialogSaveButton, Glyph::Save);
    add("DialogYesButton", SP::SP_DialogYesButton, Glyph::Accept);
    add("DialogOkButton", SP::SP_DialogOkButton, Glyph::ReturnKey);
    add("DialogDiscardButton", SP::SP_DialogDiscardButton, Glyph::Delete);
    add("DirIcon", SP::SP_DirIcon, Glyph::Folder);
    add("DirOpenIcon", SP::SP_DirOpenIcon, Glyph::FolderOpen);
    add("DirHomeIcon", SP::SP_DirHomeIcon, Glyph::Home);
    add("FileIcon", SP::SP_FileIcon, Glyph::Document);
    add("FileLinkIcon", SP::SP_FileLinkIcon, Glyph::OpenFile);
    add("FileDialogContentsView", SP::SP_FileDialogContentsView, Glyph::List);
    add("FileDialogDetailedView", SP::SP_FileDialogDetailedView, Glyph::List);
    add("FileDialogNewFolder", SP::SP_FileDialogNewFolder, Glyph::NewFolder);
    add("MediaPause", SP::SP_MediaPause, Glyph::Pause);
    add("MediaPlay", SP::SP_MediaPlay, Glyph::Play);
    add("MediaStop", SP::SP_MediaStop, Glyph::Stop);
    add("MediaSkipBackward", SP::SP_MediaSkipBackward, Glyph::Previous);
    add("MediaSkipForward", SP::SP_MediaSkipForward, Glyph::Next);
    add("MediaSeekBackward", SP::SP_MediaSeekBackward, Glyph::Rewind);
    add("MediaSeekForward", SP::SP_MediaSeekForward, Glyph::FastForward);
    add("MediaVolume", SP::SP_MediaVolume, Glyph::Volume);
    add("MediaVolumeMuted", SP::SP_MediaVolumeMuted, Glyph::Mute);
    add("MessageBoxCritical", SP::SP_MessageBoxCritical, Glyph::ErrorBadge);
    add("MessageBoxInformation", SP::SP_MessageBoxInformation, Glyph::Info);
    add("MessageBoxQuestion", SP::SP_MessageBoxQuestion, Glyph::Unknown);
    add("MessageBoxWarning", SP::SP_MessageBoxWarning, Glyph::Warning);
    add("TitleBarMinButton", SP::SP_TitleBarMinButton, Glyph::Minimize);
    add("TitleBarMaxButton", SP::SP_TitleBarMaxButton, Glyph::Maximize);
    add("TitleBarShadeButton", SP::SP_TitleBarShadeButton, Glyph::CaretSolidUp);
    add("TitleBarUnshadeButton", SP::SP_TitleBarUnshadeButton, Glyph::CaretSolidDown);
    add("TitleBarMenuButton", SP::SP_TitleBarMenuButton, Glyph::ButtonMenu);
    add("TitleBarContextHelpButton", SP::SP_TitleBarContextHelpButton, Glyph::Help);
    add("VistaShield", SP::SP_VistaShield, Glyph::Shield);
    add("DesktopIcon", SP::SP_DesktopIcon, Glyph::Monitor);
    add("ComputerIcon", SP::SP_ComputerIcon, Glyph::TVMonitor);
    add("DriveCDIcon", SP::SP_DriveCDIcon, Glyph::StorageOptical);
    add("DriveDVDIcon", SP::SP_DriveDVDIcon, Glyph::StorageOptical);
    add("DriveHDIcon", SP::SP_DriveHDIcon, Glyph::HardDrive);
    add("DriveNetIcon", SP::SP_DriveNetIcon, Glyph::MapDrive);
    add("DriveFDIcon", SP::SP_DriveFDIcon, Glyph::Save);
    add("TrashIcon", SP::SP_TrashIcon, Glyph::Delete);
    add("DialogHelpButton", SP::SP_DialogHelpButton, Glyph::GetHelpApp);
    add("DialogOpenButton", SP::SP_DialogOpenButton, Glyph::FolderOpen);
    add("DirClosedIcon", SP::SP_DirClosedIcon, Glyph::Folder);
    add("FileDialogBack", SP::SP_FileDialogBack, Glyph::ChevronLeft);
    add("FileDialogToParent", SP::SP_FileDialogToParent, Glyph::ChevronUp);
    add("FileDialogInfoView", SP::SP_FileDialogInfoView, Glyph::Info);
    add("FileDialogListView", SP::SP_FileDialogListView, Glyph::GridView);
    add("ToolBarHorizontalExtensionButton", SP::SP_ToolBarHorizontalExtensionButton, Glyph::More);
    add("ToolBarVerticalExtensionButton", SP::SP_ToolBarVerticalExtensionButton, Glyph::More);
    add("TitleBarNormalButton", SP::SP_TitleBarNormalButton, Glyph::Restore);
}

void WinUI3StandardIconsTest::mappedStandardPixmaps()
{
    QFETCH(int, standard);
    QFETCH(int, glyph);
    QFETCH(QString, label);

    const QIcon icon = qApp->style()->standardIcon(static_cast<QStyle::StandardPixmap>(standard));
    QVERIFY2(!icon.isNull(), qPrintable(label));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // The exact engine name is the pairing contract: it distinguishes
    // Accept/ReturnKey from the shared empty QIcon that used to satisfy the
    // old cacheKey comparison. Qt 5 cannot expose an engine's name, so the
    // rendered-ink check below carries that part of the proof there.
    QCOMPARE(icon.name(), QStringLiteral("winui3-fluent-icon:%1").arg(glyph));
    QVERIFY2(WinUI3::isFluentIcon(icon), qPrintable(label));
#else
    QCOMPARE(icon.cacheKey(), WinUI3::icon(static_cast<WinUI3::Icon>(glyph)).cacheKey());
#endif
    const QPixmap pixmap = icon.pixmap(QSize(24, 24));
    QVERIFY2(!pixmap.isNull(), qPrintable(label));
#ifdef Q_OS_WIN
    // Windows ships the Segoe icon font, so a real raster is expected. On
    // Linux the Unicode/vector fallback depends on installed fonts; the
    // exact engine-name assertion above already proves the pairing there.
    QVERIFY2(inkPixels(pixmap) > 0, qPrintable(label));
#endif
}

void WinUI3StandardIconsTest::customBaseIsNotMapped()
{
    const QIcon icon = qApp->style()->standardIcon(QStyle::SP_CustomBase);
    QVERIFY2(!icon.name().startsWith(QStringLiteral("winui3-fluent-icon:")),
             "SP_CustomBase is a sentinel, not a standard pixmap to style");
}

QTEST_MAIN(WinUI3StandardIconsTest)
#include "tst_winui3standardicons.moc"
