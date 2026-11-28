// SPDX-License-Identifier: LGPL-2.1-or-later
// Vertical NumberBox divider contract split out of tst_winui3editors.cpp
// to respect the 60 KB test-size ratchet.
#include <winui3style/winui3style.h>

#include "../src/lib/winui3tokens_p.h"
#include "winui3testhelpers.h"

#include <QImage>
#include <QPainter>
#include <QSpinBox>
#include <QStyleOptionSpinBox>
#include <QtTest>
#include <cstdio>

class WinUI3SpinSeparatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void verticalSeparatorMatchesSplitButtonInset_data();
    void verticalSeparatorMatchesSplitButtonInset();
    void compactVerticalNumberBoxKeepsTwoToOneRatio();
    void compactInlineSpinButtonsKeepAspect();
};

void WinUI3SpinSeparatorTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3SpinSeparatorTest::verticalSeparatorMatchesSplitButtonInset_data()
{
    QTest::addColumn<bool>("rtl");
    QTest::newRow("ltr") << false;
    QTest::newRow("rtl") << true;
}

void WinUI3SpinSeparatorTest::verticalSeparatorMatchesSplitButtonInset()
{
    QFETCH(bool, rtl);
    QSpinBox spin;
    WinUI3::Style::setVerticalSpinButtons(&spin);
    spin.resize(150, 32);
    spin.show();

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
            | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
    option.direction = rtl ? Qt::RightToLeft : Qt::LeftToRight;

    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxEditField, &spin);
    const QRect up =
            spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    QCOMPARE(up.size(), QSize(32, 16));
    QVERIFY(!edit.intersects(up));

    QImage frame(spin.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    {
        QPainter painter(&frame);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
    }
    const int separatorX = rtl ? edit.left() : edit.right();
    // The SplitButton divider is inset 4 px from both ends of its half:
    // compare the divider column with the edit field immediately beside it.
    const int insideX = rtl ? separatorX + 1 : separatorX - 1;
    for (const int y : { edit.top() + 4, edit.bottom() - 5 }) {
        QVERIFY2(frame.pixelColor(separatorX, y) != frame.pixelColor(insideX, y),
                 "divider must be visible at the 4 px inset");
    }
    for (const int y : { edit.top() + 1, edit.bottom() - 1 }) {
        QCOMPARE(frame.pixelColor(separatorX, y), frame.pixelColor(insideX, y));
    }
}

void WinUI3SpinSeparatorTest::compactVerticalNumberBoxKeepsTwoToOneRatio()
{
    QSpinBox spin;
    WinUI3::Style::setVerticalSpinButtons(&spin);
    spin.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("compact"));
    spin.resize(150, 24);
    spin.show();

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
            | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxEditField, &spin);
    const QRect up =
            spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxDown, &spin);
    // Standard is 32x16 per half; Compact's 24 px editor scales both axes,
    // keeping the same 2:1 ratio instead of squeezing only the height.
    QCOMPARE(up.size(), QSize(24, 12));
    QCOMPARE(down.size(), QSize(24, 12));
    QCOMPARE(edit.right() + 1, up.left());
    QCOMPARE(up.bottom() + 1, down.top());
}

void WinUI3SpinSeparatorTest::compactInlineSpinButtonsKeepAspect()
{
    QSpinBox spin;
    spin.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("compact"));
    spin.resize(150, 24);
    spin.show();

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
            | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const QRect up =
            spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxDown, &spin);
    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxEditField, &spin);
    // Standard inline buttons are 36x32; Compact's 24 px editor keeps that
    // aspect at 27x24 rather than squeezing only the height.
    QCOMPARE(up.size(), QSize(27, 24));
    QCOMPARE(down.size(), QSize(27, 24));
    QCOMPARE(up.top(), down.top());
    QCOMPARE(up.right() + 1, down.left());
    QCOMPARE(edit.right() + 1, up.left());
}

QTEST_MAIN(WinUI3SpinSeparatorTest)
#include "tst_winui3spinseparator.moc"
