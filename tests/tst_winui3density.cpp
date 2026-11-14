#include "winui3density_p.h"

#include <winui3style/winui3style.h>

#include <QApplication>
#include <QComboBox>
#include <QFontMetrics>
#include <QLineEdit>
#include <QProxyStyle>
#include <QSpinBox>
#include <QStyleOptionComboBox>
#include <QStyleOptionFrame>
#include <QStyleOptionSpinBox>
#include <QWidget>
#include <QtTest>

using WinUI3::Private::densityMetrics;
using WinUI3::Private::DensityMode;
using WinUI3::Private::densityModeFor;

class WinUI3DensityTest final : public QObject
{
    Q_OBJECT

private slots:
    void profilesExposeDocumentedSizing();
    void localDensityInheritsAndOverrides();
    void styleDensityIsUsedAsFallback();
    void nullWidgetUsesApplicationStyleDensity();
    void nullWidgetSizingUsesReceiverStyleDensity_data();
    void nullWidgetSizingUsesReceiverStyleDensity();
};

void WinUI3DensityTest::profilesExposeDocumentedSizing()
{
    const auto &standard = densityMetrics(DensityMode::Standard);
    const auto &compact = densityMetrics(DensityMode::Compact);

    QCOMPARE(standard.textBoxHeight, 32);
    QCOMPARE(compact.textBoxHeight, 24);
    QCOMPARE(standard.comboBoxHeight, 32);
    QCOMPARE(compact.comboBoxHeight, 24);
    QCOMPARE(standard.menuBarItemHeight, 32);
    QCOMPARE(compact.menuBarItemHeight, 24);
    QCOMPARE(standard.listItemHeight, 40);
    QCOMPARE(compact.listItemHeight, 32);
    QCOMPARE(standard.treeItemHeight, 28);
    QCOMPARE(compact.treeItemHeight, 24);
    QCOMPARE(standard.navigationItemHeight, 40);
    QCOMPARE(compact.navigationItemHeight, 32);
    QCOMPARE(standard.comboPopupItemHeight, 40);
    QCOMPARE(compact.comboPopupItemHeight, 32);
    QCOMPARE(standard.menuItemHeight, compact.menuItemHeight);

    // Compact Sizing does not list these controls.  Their template slots are
    // consequently stable across the two profiles.
    QCOMPARE(standard.buttonHeight, compact.buttonHeight);
    QCOMPARE(standard.tabHeight, compact.tabHeight);
    QCOMPARE(standard.toggleTrackWidth, compact.toggleTrackWidth);
    QCOMPARE(standard.toggleTrackHeight, compact.toggleTrackHeight);
    QCOMPARE(standard.menuSeparatorHeight, compact.menuSeparatorHeight);
    QCOMPARE(standard.tableItemHeight, compact.tableItemHeight);
    QCOMPARE(standard.headerHeight, compact.headerHeight);
    QCOMPARE(standard.sliderGrooveMargin, compact.sliderGrooveMargin);
    QCOMPARE(standard.itemSelectionGutter, compact.itemSelectionGutter);
    QCOMPARE(standard.treeIndent, compact.treeIndent);
}

void WinUI3DensityTest::localDensityInheritsAndOverrides()
{
    QWidget root;
    QWidget child(&root);
    QWidget grandChild(&child);

    QCOMPARE(densityModeFor(&grandChild), DensityMode::Standard);

    root.setProperty("winuiDensity", QStringLiteral("compact"));
    QCOMPARE(densityModeFor(&grandChild), DensityMode::Compact);

    child.setProperty("winuiDensity", QStringLiteral("standard"));
    QCOMPARE(densityModeFor(&grandChild), DensityMode::Standard);

    grandChild.setProperty("winuiDensity", 1);
    QCOMPARE(densityModeFor(&grandChild), DensityMode::Compact);
}

void WinUI3DensityTest::styleDensityIsUsedAsFallback()
{
    QProxyStyle style;
    QWidget widget;
    widget.setProperty("densityMode", QStringLiteral("standard"));
    QCOMPARE(densityModeFor(&widget), DensityMode::Standard);

    // A style is a QObject, so the helper can consume a public Q_PROPERTY
    // without depending on the concrete Style class or its enum definition.
    widget.setStyle(&style);
    style.setProperty("densityMode", QStringLiteral("compact"));
    QCOMPARE(densityModeFor(&widget), DensityMode::Compact);
}

void WinUI3DensityTest::nullWidgetUsesApplicationStyleDensity()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::DensityMode::Compact));
    QCOMPARE(densityModeFor(nullptr), DensityMode::Compact);
}

void WinUI3DensityTest::nullWidgetSizingUsesReceiverStyleDensity_data()
{
    QTest::addColumn<bool>("compactReceiver");
    QTest::addColumn<int>("fontPixelSize");
    QTest::newRow("compact-receiver-16px") << true << 16;
    QTest::newRow("compact-receiver-24px") << true << 24;
    QTest::newRow("standard-receiver-16px") << false << 16;
    QTest::newRow("standard-receiver-24px") << false << 24;
}

// A standalone receiver Style sizing with a null widget must resolve density
// from the receiver, not leak the application Style's density. The bound
// widget (widget->style() == receiver) is the pure-receiver reference; the
// null-widget hint must agree with it for every density profile.
void WinUI3DensityTest::nullWidgetSizingUsesReceiverStyleDensity()
{
    QFETCH(bool, compactReceiver);
    QFETCH(int, fontPixelSize);

    // Application style deliberately differs from the receiver.
    qApp->setStyle(new WinUI3::Style(compactReceiver ? WinUI3::DensityMode::Standard
                                                     : WinUI3::DensityMode::Compact));
    WinUI3::Style receiver(compactReceiver ? WinUI3::DensityMode::Compact
                                           : WinUI3::DensityMode::Standard);

    QFont font = qApp->font();
    font.setPixelSize(fontPixelSize);
    const QFontMetrics fm(font);

    QStyleOptionFrame lineOption;
    lineOption.fontMetrics = fm;
    QStyleOptionSpinBox spinOption;
    spinOption.fontMetrics = fm;
    QStyleOptionComboBox comboOption;
    comboOption.fontMetrics = fm;

    QSpinBox spin;
    spin.setStyle(&receiver);
    spin.setFont(font);
    QLineEdit line;
    line.setStyle(&receiver);
    line.setFont(font);
    QComboBox combo;
    combo.setStyle(&receiver);
    combo.setFont(font);

    const QSize content(60, fm.height());
    QCOMPARE(receiver.sizeFromContents(QStyle::CT_SpinBox, &spinOption, content, nullptr).height(),
             receiver.sizeFromContents(QStyle::CT_SpinBox, &spinOption, content, &spin).height());
    QCOMPARE(receiver.sizeFromContents(QStyle::CT_LineEdit, &lineOption, content, nullptr).height(),
             receiver.sizeFromContents(QStyle::CT_LineEdit, &lineOption, content, &line).height());
    QCOMPARE(
            receiver.sizeFromContents(QStyle::CT_ComboBox, &comboOption, content, nullptr).height(),
            receiver.sizeFromContents(QStyle::CT_ComboBox, &comboOption, content, &combo).height());
}

QTEST_MAIN(WinUI3DensityTest)
#include "tst_winui3density.moc"
