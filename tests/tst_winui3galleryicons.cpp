// SPDX-License-Identifier: LGPL-2.1-or-later
// Issue #69 Icons page: every QStyle::StandardPixmap gets exactly one preview
// cell, the style-answered ones carry a Fluent glyph, the rest land in the
// base-fallback or no-glyph group, a palette change re-renders the rasters,
// and a style comparison switch rebuilds the sheet instead of appending a
// second copy. Driven on the REAL GalleryWindow (gallerywindow.cpp/.h/.ui)
// through offscreen QPA.
//
// Qt 5.12's QLabel::pixmap() returns const QPixmap *, Qt 6's returns
// const QPixmap &; this helper hides the difference so the rest of the test
// is version agnostic.
#include "../src/gallery/gallerywindow.h"

#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMetaEnum>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QtTest>
#include <winui3style/winui3style.h>

namespace {
const QPixmap labelPixmap(const QLabel *label)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return label->pixmap(); // const QPixmap &, cheap implicitly-shared copy
#else
    return *label->pixmap(); // const QPixmap *, deref then copy
#endif
}
} // namespace

class WinUI3GalleryIconsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void galleryIconsPageListsEveryStandardPixmap();
};

void WinUI3GalleryIconsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3GalleryIconsTest::galleryIconsPageListsEveryStandardPixmap()
{
    const QMetaEnum standards = QMetaEnum::fromType<QStyle::StandardPixmap>();
    QVERIFY(standards.isValid());
    const int standardCount = standards.keyCount();
    QVERIFY(standardCount > 0);

    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QListWidget *navigation = nullptr;
    QStackedWidget *pages = nullptr;
    QScrollArea *iconsPage = nullptr;
    QGroupBox *fluentGroup = nullptr;
    QGroupBox *baseGroup = nullptr;
    QGroupBox *missingGroup = nullptr;
    QLabel *note = nullptr;
    navigation = window.findChild<QListWidget *>(QStringLiteral("navigationList"));
    pages = window.findChild<QStackedWidget *>(QStringLiteral("pages"));
    iconsPage = window.findChild<QScrollArea *>(QStringLiteral("iconsPage"));
    fluentGroup = window.findChild<QGroupBox *>(QStringLiteral("fluentIconsGroup"));
    baseGroup = window.findChild<QGroupBox *>(QStringLiteral("baseIconsGroup"));
    missingGroup = window.findChild<QGroupBox *>(QStringLiteral("missingIconsGroup"));
    note = window.findChild<QLabel *>(QStringLiteral("iconsNote"));
    QVERIFY(navigation && pages && iconsPage && fluentGroup && baseGroup && missingGroup && note);

    int iconsRow = -1;
    for (int row = 0; row < navigation->count(); ++row) {
        if (navigation->item(row)->text() == QStringLiteral("Icons"))
            iconsRow = row;
    }
    QVERIFY(iconsRow >= 0);
    QCOMPARE(navigation->count(), pages->count());
    navigation->setCurrentRow(iconsRow);
    QCOMPARE(pages->currentWidget(), iconsPage);
    QCOMPARE(pages->currentIndex(), iconsRow);

    const auto previews = [](QGroupBox *group) {
        QList<QLabel *> result;
        for (QLabel *label : group->findChildren<QLabel *>()) {
            if (label->property("standardPixmapValue").isValid())
                result.append(label);
        }
        return result;
    };
    const qreal dpr = window.devicePixelRatioF();
    const QSize expectedSize(qRound(24 * dpr), qRound(24 * dpr));
    const auto checkPreviews = [&](const QList<QLabel *> &labels) {
        for (QLabel *label : labels) {
            if (labelPixmap(label).isNull()) {
                // Ancestor-dependent SP_* with neither a Fluent glyph nor a base
                // icon must say so instead of rendering an empty cell.
                if (label->text().isEmpty())
                    return false;
            } else if (labelPixmap(label).size() != expectedSize) {
                return false;
            }
        }
        return true;
    };
    const QList<QLabel *> fluent = previews(fluentGroup);
    const QList<QLabel *> fallback = previews(baseGroup);
    const QList<QLabel *> missing = previews(missingGroup);
    QCOMPARE(fluent.size() + fallback.size() + missing.size(), standardCount);
    QVERIFY(checkPreviews(fluent));
    QVERIFY(checkPreviews(fallback));
    QVERIFY(checkPreviews(missing));
    // The group a cell lands in must match what the style actually returned:
    // a Fluent glyph is never empty, and a "no glyph" cell never carries art.
    for (QLabel *label : fluent)
        QVERIFY(!labelPixmap(label).isNull());
    for (QLabel *label : missing) {
        QVERIFY(labelPixmap(label).isNull());
        QVERIFY(!label->text().isEmpty());
    }
    // The WinUI 3 style answers a non-empty share of the enumerators with its
    // own glyphs; a sheet that claims everything is a fallback is broken.
    QVERIFY(!fluent.isEmpty());
    QVERIFY(fluentGroup->title().contains(QStringLiteral("%1").arg(fluent.size())));
    QVERIFY(baseGroup->title().contains(QStringLiteral("%1").arg(fallback.size())));
    QVERIFY(missingGroup->title().contains(QStringLiteral("%1").arg(missing.size())));
    QVERIFY(note->text().contains(QStringLiteral("%1").arg(standardCount)));

    // A palette change must re-render the rasters: QLabel stores the pixmap the
    // style drew in the old theme's ink, so nothing recolours by itself.
    const QImage lightInk = labelPixmap(fluent.first()).toImage();
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    qApp->processEvents();
    const QList<QLabel *> darkPreviews = previews(fluentGroup);
    QCOMPARE(darkPreviews.size(), fluent.size());
    QVERIFY(checkPreviews(darkPreviews));
    QVERIFY(labelPixmap(darkPreviews.first()).toImage() != lightInk);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    qApp->processEvents();

    // Switching comparison style changes the answer for every enumerator, so
    // the sheet is rebuilt from the live style, never appended to.
    auto *windows11Action = window.findChild<QAction *>(QStringLiteral("windows11StyleAction"));
    QVERIFY(windows11Action);
    if (windows11Action->isEnabled()) {
        windows11Action->trigger();
        qApp->processEvents();
        QCOMPARE(previews(fluentGroup).size() + previews(baseGroup).size()
                         + previews(missingGroup).size(),
                 standardCount);
        auto *winuiAction = window.findChild<QAction *>(QStringLiteral("winui3StyleAction"));
        QVERIFY(winuiAction);
        winuiAction->trigger();
        qApp->processEvents();
        QCOMPARE(previews(fluentGroup).size(), fluent.size());
        QCOMPARE(previews(fluentGroup).size() + previews(baseGroup).size()
                         + previews(missingGroup).size(),
                 standardCount);
    }
}

QTEST_MAIN(WinUI3GalleryIconsTest)
#include "tst_winui3galleryicons.moc"
