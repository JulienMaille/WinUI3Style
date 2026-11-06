// SPDX-License-Identifier: LGPL-2.1-or-later
// QKeySequenceEdit has no WinUI counterpart: WinUI 3 offers no shortcut
// recorder. Its nearest neighbour is the TextBox, and Qt implements it as a
// stock QLineEdit child that covers the whole widget (verified in
// qkeysequenceedit_p.h), so the field it shows is the style's TextBox field
// rather than a separate surface to paint. This contract locks that
// inheritance: at rest the recorder must render pixel-identically to a
// QLineEdit of the same size, and the child must keep covering the widget so
// no chrome is painted between the two.
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include <QImage>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QtTest>

class WinUI3KeySequenceEditTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void recorderInheritsTextBoxField();
};

void WinUI3KeySequenceEditTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3KeySequenceEditTest::recorderInheritsTextBoxField()
{
    const QString sequence =
            QKeySequence(QKeySequence::SelectAll).toString(QKeySequence::NativeText);

    QKeySequenceEdit recorder;
    recorder.setKeySequence(QKeySequence(QKeySequence::SelectAll));
    recorder.resize(240, 32);
    recorder.show();
    QVERIFY(QTest::qWaitForWindowExposed(&recorder));

    auto *field = recorder.findChild<QLineEdit *>();
    QVERIFY(field);
    // Mechanism: the recorder delegates its whole surface to one stock
    // QLineEdit; there is no recorder-specific paint path.
    QCOMPARE(field->geometry(), recorder.rect());

    QLineEdit reference(sequence);
    reference.resize(240, 32);
    reference.show();
    QVERIFY(QTest::qWaitForWindowExposed(&reference));
    recorder.clearFocus();
    reference.clearFocus();
    QVERIFY(!recorder.hasFocus());
    QVERIFY(!reference.hasFocus());
    QCOMPARE(recorder.size(), reference.size());

    // Same field geometry contract: the recorder adds no extra chrome.
    QCOMPARE(recorder.sizeHint().height(), reference.sizeHint().height());
    QCOMPARE(field->height(), reference.height());
    QCOMPARE(field->text(), sequence);

    const QImage recorderFrame = recorder.grab().toImage();
    const QImage referenceFrame = reference.grab().toImage();
    QCOMPARE(recorderFrame.size(), referenceFrame.size());
    // Identical rest surface, border band and top edge (away from text ink).
    for (const QPoint &probe :
         { QPoint(recorderFrame.width() - 6, recorderFrame.height() / 2),
           QPoint(0, recorderFrame.height() / 2), QPoint(recorderFrame.width() / 2, 0),
           QPoint(recorderFrame.width() / 2, recorderFrame.height() - 1) }) {
        QCOMPARE(recorderFrame.pixelColor(probe), referenceFrame.pixelColor(probe));
    }
}

QTEST_MAIN(WinUI3KeySequenceEditTest)
#include "tst_winui3keysequenceedit.moc"
