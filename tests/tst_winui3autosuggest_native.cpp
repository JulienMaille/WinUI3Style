// SPDX-License-Identifier: LGPL-2.1-or-later
// Native, opt-in PrintWindow evidence for the Gallery AutoSuggestBox.
#include "../demo/gallerywindow.h"
#include "winui3testhelpers.h"

#include <QCompleter>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QTextStream>
#include <QTest>

class WinUI3AutoSuggestNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void galleryAutoSuggestPrintWindow_data();
    void galleryAutoSuggestPrintWindow();
};

void WinUI3AutoSuggestNativeTest::galleryAutoSuggestPrintWindow_data()
{
    QTest::addColumn<int>("theme");
    QTest::newRow("light") << 1;
    QTest::newRow("dark") << 2;
}

void WinUI3AutoSuggestNativeTest::galleryAutoSuggestPrintWindow()
{
    const QString destination = qEnvironmentVariable("WINUI3STYLE_AUTOSUGGEST_CAPTURE_DIR");
    if (destination.isEmpty())
        QSKIP("Set WINUI3STYLE_AUTOSUGGEST_CAPTURE_DIR to enable native captures");

    QFETCH(int, theme);
    QApplication::setCursorFlashTime(0);
    const QString prefix = theme == 1 ? QStringLiteral("light-") : QStringLiteral("dark-");
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
    GalleryWindow window;
    window.resize(1180, 780);
    window.move(60, 60);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.raise();
    window.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(theme == 1 ? WinUI3::ThemeMode::Light : WinUI3::ThemeMode::Dark);
    auto *themeCombo = window.findChild<QComboBox *>(QStringLiteral("themeCombo"));
    QVERIFY(themeCombo);
    themeCombo->setCurrentIndex(theme);
    QCOMPARE(static_cast<int>(style->themeMode()), theme);
    QCoreApplication::processEvents();

    const qreal dpr = window.devicePixelRatioF();
    QDir output(destination);
    QVERIFY2(output.mkpath(QStringLiteral(".")), qPrintable(output.absolutePath()));

    auto *edit = window.findChild<QLineEdit *>(QStringLiteral("autoSuggestEdit"));
    QVERIFY(edit);
    QVERIFY(edit->completer());
    QCOMPARE(edit->completer()->completionMode(), QCompleter::PopupCompletion);
    for (QWidget *parent = edit->parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto *scroll = qobject_cast<QScrollArea *>(parent)) {
            scroll->ensureWidgetVisible(edit);
            break;
        }
    }
    QCoreApplication::processEvents();

    QFile metadata(output.filePath(prefix + QStringLiteral("autosuggest-native.txt")));
    QVERIFY(metadata.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream log(&metadata);
    log << "theme=" << style->property("themeMode").toInt() << "\n"
        << "density=" << style->property("densityMode").toInt() << "\n"
        << "dpr=" << dpr << "\n"
        << "window-size=" << window.frameGeometry().size().width() << 'x'
        << window.frameGeometry().size().height() << "\n"
        << "base-rgba=" << edit->palette().color(QPalette::Base).rgba() << "\n";

    const auto capture = [&](const QString &name, QWidget *widget, QImage *saved = nullptr) {
        QVERIFY(widget);
        QVERIFY(widget->isVisible());
        const QImage frame = nativeWindowFrame(widget->window()->winId());
        QVERIFY2(!frame.isNull(), qPrintable(name));
        // PrintWindow includes the invisible Windows resize border, unlike
        // QWidget::frameGeometry() on Windows 11. Its bounds are native pixels.
        RECT bounds{};
        QVERIFY(GetWindowRect(reinterpret_cast<HWND>(widget->window()->winId()), &bounds));
        const QSize expected(bounds.right - bounds.left, bounds.bottom - bounds.top);
        QCOMPARE(frame.size(), expected);
        QVERIFY(frame.save(output.filePath(name + QStringLiteral(".png"))));
        if (saved)
            *saved = frame;
        log << name << "=" << frame.width() << 'x' << frame.height() << "\n";
    };

    // Main-window rest is the outside-scope before frame for the popup run.
    edit->clearFocus();
    QTest::mouseMove(&window, QPoint(4, 4));
    QTest::qWait(250);
    capture(prefix + QStringLiteral("gallery-rest"), &window);

    edit->setFocus();
    QTest::keyClicks(edit, QStringLiteral("a"));
    QCoreApplication::processEvents();
    auto *popup = edit->completer()->popup();
    QVERIFY(popup);
    QVERIFY(QTest::qWaitForWindowExposed(popup));
    QTest::qWait(80);
    QCOMPARE(popup->window()->isVisible(), true);
    QVERIFY(popup->height() > 0);
    QVERIFY(popup->width() >= edit->width());
    const QRect popupGeometry = popup->window()->frameGeometry();
    QVERIFY(popupGeometry.isValid());
    log << "popup-base-rgba=" << popup->palette().color(QPalette::Base).rgba() << "\n";
    log << "popup-effective=" << popup->property("_winui_backdrop_effective").toInt() << "\n";
    capture(prefix + QStringLiteral("autosuggest-popup-rest"), popup);
    auto *view = popup;
    const QModelIndex hoverIndex = view->model()->index(1, 0);
    QVERIFY(hoverIndex.isValid());
    const QRect hoverRow = view->visualRect(hoverIndex);
    QVERIFY(hoverRow.isValid());
    QTest::mouseMove(view->viewport(), QPoint(-2, -2));
    QTest::qWait(80);
    QTest::mouseMove(view->viewport(), hoverRow.center());
    // Qt 6's QWidget overload only warps QCursor; Windows need not deliver a
    // move for that warp. Also route a Qt window-system move to the owned popup.
    QTest::mouseMove(popup->windowHandle(), popup->mapFromGlobal(QCursor::pos()));
    QTest::qWait(80);
    QCOMPARE(view->indexAt(view->viewport()->mapFromGlobal(QCursor::pos())), hoverIndex);
    QImage hoveredFrame;
    capture(prefix + QStringLiteral("autosuggest-popup-hover"), popup, &hoveredFrame);
    const QRect restRow = view->visualRect(view->model()->index(0, 0));
    QCOMPARE(hoverRow.height(), 40);
    const QPoint hoverProbe =
            view->viewport()->mapTo(popup, QPoint(hoverRow.right() - 24, hoverRow.center().y()));
    const QPoint restProbe(hoverProbe.x(), restRow.center().y());
    QVERIFY(hoveredFrame.rect().contains(hoverProbe * dpr));
    QVERIFY(hoveredFrame.pixelColor(hoverProbe * dpr) != hoveredFrame.pixelColor(restProbe * dpr));
    QTest::mouseMove(&window, QPoint(4, 4));
    QTest::mouseMove(window.windowHandle(), QPoint(4, 4));
    QTest::qWait(80);
    QVERIFY(!popup->frameGeometry().contains(QCursor::pos()));
    QImage leftFrame;
    capture(prefix + QStringLiteral("autosuggest-popup-leave"), popup, &leftFrame);
    log << "popup-geometry=" << popupGeometry.x() << ',' << popupGeometry.y() << ' '
        << popupGeometry.width() << 'x' << popupGeometry.height() << "\n";
    metadata.close();
    // The already-resolved Base RGB must survive both native row states.
    // Probe outside the pill, away from its text and antialiased corners.
    QColor expectedBase = popup->palette().color(QPalette::Base);
    expectedBase.setAlpha(255);
    const QPoint edgeProbe = view->viewport()->mapTo(popup, QPoint(1, hoverRow.center().y()));
    QCOMPARE(popup->frameGeometry(), popupGeometry);
    QCOMPARE(hoveredFrame.pixelColor(edgeProbe * dpr), expectedBase);
    QCOMPARE(leftFrame.pixelColor(edgeProbe * dpr), expectedBase);
    popup->hide();
}

QTEST_MAIN(WinUI3AutoSuggestNativeTest)
#include "tst_winui3autosuggest_native.moc"
