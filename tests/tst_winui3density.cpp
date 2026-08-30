#include "winui3density_p.h"

#include <QApplication>
#include <QProxyStyle>
#include <QWidget>
#include <QtTest>

using WinUI3::Private::DensityMode;
using WinUI3::Private::densityMetrics;
using WinUI3::Private::densityModeFor;

class WinUI3DensityTest final : public QObject
{
    Q_OBJECT

private slots:
    void profilesExposeDocumentedSizing();
    void localDensityInheritsAndOverrides();
    void styleDensityIsUsedAsFallback();
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

    // Compact Sizing does not list these controls.  Their template slots are
    // consequently stable across the two profiles.
    QCOMPARE(standard.buttonHeight, compact.buttonHeight);
    QCOMPARE(standard.tabHeight, compact.tabHeight);
    QCOMPARE(standard.toggleTrackWidth, compact.toggleTrackWidth);
    QCOMPARE(standard.toggleTrackHeight, compact.toggleTrackHeight);
    QCOMPARE(standard.menuItemHeight, compact.menuItemHeight);
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

QTEST_MAIN(WinUI3DensityTest)
#include "tst_winui3density.moc"
