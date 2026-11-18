// SPDX-License-Identifier: LGPL-2.1-or-later
// Shortcut-recorder coverage split out of tst_winui3galleryprobe.cpp to stay
// under the 60 KB source ratchet. Drives the real Designer GalleryWindow.
#include "../src/gallery/gallerywindow.h"
#include "winui3testhelpers.h"

#include "../src/lib/winui3tokens_p.h"

#include <QApplication>
#include <QComboBox>
#include <QImage>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QRect>
#include <QStyle>
#include <QStyleFactory>
#include <QtTest>

namespace {

bool containsColor(const QImage &image, const QColor &color)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == color)
                return true;
        }
    }
    return false;
}

} // namespace

class WinUI3GalleryShortcutTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void galleryShortcutRecorderReachable();
    void galleryShortcutRecorderShowsLiveKeys();
    void galleryShortcutRecorderSplitsChords();
    void galleryShortcutRecorderDarkTheme();
};

void WinUI3GalleryShortcutTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3GalleryShortcutTest::init()
{
    if (!qobject_cast<WinUI3::Style *>(qApp->style()))
        qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    style->setThemeMode(WinUI3::ThemeMode::Light);
    style->setAccentColor({});
}

void WinUI3GalleryShortcutTest::galleryShortcutRecorderReachable()
{
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *recorder = window.findChild<QKeySequenceEdit *>(QStringLiteral("shortcutEdit"));
    QVERIFY(recorder);
    // The Gallery example keeps a preset so the rest state is legible.
    QCOMPARE(recorder->keySequence().toString(QKeySequence::PortableText),
             QStringLiteral("Ctrl+K"));
    auto *field = recorder->findChild<QLineEdit *>();
    QVERIFY(field);
    // Stock widget contract: the Gallery adds no code, so Qt's recorder keeps
    // its own editor focusable and hit-testable. The style paints around it.
    QVERIFY(field->focusPolicy() != Qt::NoFocus);
    QVERIFY(!field->testAttribute(Qt::WA_TransparentForMouseEvents));
    QCOMPARE(field->geometry(), recorder->rect());

    recorder->clearFocus();
    qApp->processEvents();
    const auto t = WinUI3::Private::tokens(recorder->palette());
    const QImage image = recorder->grab().toImage();
    const qreal dpr = image.devicePixelRatio();
    QRect caps;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) == t.accentFill)
                caps |= QRect(x, y, 1, 1);
        }
    }
    // Every pixel probe is paired with the pinned token geometry on the same
    // unfocused rest state: one 20 px cap band starting at the TextBox inset.
    QVERIFY2(caps.isValid(), "no accent keycap pixels");
    const int inset = qRound(WinUI3::Private::ShortcutFieldTextInset * dpr);
    const int capHeight = qRound(WinUI3::Private::ShortcutKeycapHeight * dpr);
    const int tolerance = qMax(1, qCeil(dpr));
    // A 1 px rounded-rect stroke eats the outer accent column/row.
    QVERIFY2(caps.left() >= inset && caps.left() <= inset + tolerance,
             "keycap band must start at the TextBox text inset");
    QVERIFY2(caps.height() >= capHeight - 2 * tolerance && caps.height() <= capHeight,
             "keycap band must be one pinned cap tall");
    QVERIFY(caps.width() > capHeight);

    // The style owns the text slot: an empty SE_LineEditContents for this
    // editor must keep Qt's line control from painting "Ctrl+K" over the caps
    // in its own TextBox ink.
    QVERIFY2(!containsColor(image, t.textPrimary),
             "native line-edit text painted over the keycaps");

    // Fallback contract: under any comparison style the keycap path is not
    // installed, so the stock line edit must show the sequence instead of
    // leaving an empty field.
    QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"));
    QVERIFY(fusion);
    QApplication::setStyle(fusion);
    qApp->processEvents();
    QCOMPARE(field->text(), QStringLiteral("Ctrl+K"));
    const QImage fallback = recorder->grab().toImage();
    const auto fallbackTokens = WinUI3::Private::tokens(recorder->palette());
    QVERIFY(!containsColor(fallback, fallbackTokens.accentFill));
}

void WinUI3GalleryShortcutTest::galleryShortcutRecorderShowsLiveKeys()
{
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *recorder = window.findChild<QKeySequenceEdit *>(QStringLiteral("shortcutEdit"));
    QVERIFY(recorder);
    auto *field = recorder->findChild<QLineEdit *>();
    QVERIFY(field);

    recorder->clear();
    qApp->processEvents();
    QVERIFY(recorder->keySequence().isEmpty());
    const QImage rest = recorder->grab().toImage();

    recorder->setFocus();
    QVERIFY(recorder->hasFocus());
    QSignalSpy finished(recorder, &QKeySequenceEdit::keySequenceChanged);
    // Press only: the release starts Qt's one-second finish timer, which would
    // blur the distinction between live display and committed sequence.
    QTest::keyPress(recorder, Qt::Key_Control);
    QTest::keyPress(recorder, Qt::Key_K, Qt::ControlModifier);
    qApp->processEvents();
    // Qt updates its editor text immediately but emits keySequenceChanged only
    // when recording finishes. The caps read that text, so they already follow
    // the live keys.
    QCOMPARE(finished.count(), 0);
    const QString live = field->text();
    QVERIFY2(live.contains(QStringLiteral("Ctrl")), live.toUtf8().constData());
    QVERIFY2(live.contains(QStringLiteral("K")), live.toUtf8().constData());
    QVERIFY2(live.contains(QStringLiteral("...")), live.toUtf8().constData());

    const auto t = WinUI3::Private::tokens(recorder->palette());
    const QImage image = recorder->grab().toImage();
    // Focused caps use the pressed fill, so look for the cap ink and for a
    // frame that differs from the empty rest state instead of the rest fill.
    QVERIFY(containsColor(image, t.textOnAccentPrimary));
    QVERIFY(image != rest);
    QTest::keyRelease(recorder, Qt::Key_K, Qt::ControlModifier);
    QTest::keyRelease(recorder, Qt::Key_Control);
}

void WinUI3GalleryShortcutTest::galleryShortcutRecorderSplitsChords()
{
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *recorder = window.findChild<QKeySequenceEdit *>(QStringLiteral("shortcutEdit"));
    QVERIFY(recorder);
    auto *field = recorder->findChild<QLineEdit *>();
    QVERIFY(field);
    recorder->setKeySequence(QKeySequence(QStringLiteral("Ctrl+C, Ctrl+G")));
    recorder->clearFocus();
    qApp->processEvents();
    QCOMPARE(field->text().count(QStringLiteral("Ctrl")), 2);

    const auto t = WinUI3::Private::tokens(recorder->palette());
    const QImage image = recorder->grab().toImage();
    const qreal dpr = image.devicePixelRatio();
    // Sample above the 12 px text baseline inside the 20 px caps, where each
    // cap is a solid accent run. A comma must split chords, not glue keys.
    const int capHeight = qRound(WinUI3::Private::ShortcutKeycapHeight * dpr);
    const int y = (image.height() - capHeight) / 2 + qMax(1, qRound(2 * dpr));
    QVector<QPair<int, int>> runs;
    bool inRun = false;
    int runStart = 0;
    for (int x = 0; x < image.width(); ++x) {
        const bool accent = image.pixelColor(x, y) == t.accentFill;
        if (accent && !inRun)
            runStart = x;
        if (!accent && inRun)
            runs.append({ runStart, x - 1 });
        inRun = accent;
    }
    QCOMPARE(runs.size(), 4);
    // The '+' between the first two caps must be drawn on the field, not
    // swallowed into a cap label.
    const QColor fieldFill = image.pixelColor(2, y);
    bool separatorInk = false;
    for (int x = runs.at(0).second + 1; x < runs.at(1).first && !separatorInk; ++x) {
        for (int yy = y - qRound(8 * dpr); yy <= y + qRound(8 * dpr); ++yy) {
            const QColor pixel = image.pixelColor(x, yy);
            if (pixel != fieldFill && pixel != t.accentFill) {
                separatorInk = true;
                break;
            }
        }
    }
    QVERIFY(separatorInk);
}

void WinUI3GalleryShortcutTest::galleryShortcutRecorderDarkTheme()
{
    GalleryWindow window;
    window.resize(1180, 780);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *theme = window.findChild<QComboBox *>(QStringLiteral("themeCombo"));
    QVERIFY(theme);
    theme->setCurrentIndex(2); // Dark
    qApp->processEvents();
    auto *recorder = window.findChild<QKeySequenceEdit *>(QStringLiteral("shortcutEdit"));
    QVERIFY(recorder);
    recorder->clearFocus();
    qApp->processEvents();

    const auto t = WinUI3::Private::tokens(recorder->palette());
    QVERIFY(t.dark);
    const QImage image = recorder->grab().toImage();
    // Same cap geometry and role tokens as Light, resolved for Dark: the
    // fill/ink pair must both appear, not a hard-coded light-mode color.
    QVERIFY(containsColor(image, t.accentFill));
    QVERIFY(containsColor(image, t.textOnAccentPrimary));
    QVERIFY2(!containsColor(image, t.textPrimary),
             "native line-edit text painted over the keycaps");
}

QTEST_MAIN(WinUI3GalleryShortcutTest)
#include "tst_winui3galleryshortcut.moc"
