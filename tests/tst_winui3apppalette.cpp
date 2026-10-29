// SPDX-License-Identifier: LGPL-2.1-or-later
// Issue #28: an application palette override must reach WinUI3 paint.
//
// The reporter wraps the style in a QProxyStyle that tints QPalette::Highlight
// and QPalette::HighlightedText from polish(QPalette &). That hook is the Qt
// contract for a style to rewrite the application palette, so the tint has to
// survive the style install and every later theme/accent refresh.
#include <winui3style/winui3style.h>

#include "../src/winui3tokens_p.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QListWidget>
#include <QPainter>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyleOptionViewItem>
#include <QtTest>

#include "winui3testhelpers.h"

namespace {

// The reporter's tint (BUILD_DEV branch of their ThemeProxyStyle).
const QColor Tint(0xFF, 0xC1, 0x07);

// selectionMarkerRect() puts the 3 px selection pill at
// viewport.left() + itemSelectionMarkerInset, so column 4 is inside it for
// every density, and the row's vertical centre is inside its 16 px height.
constexpr int markerSampleX = 4;

QPalette tintedPalette(const QPalette &base)
{
    QPalette palette = base;
    palette.setColor(QPalette::Highlight, Tint);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    return palette;
}

// Issue #28 reporter's proxy: tint after the wrapped style polished.
class ThemeProxyStyle final : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void polish(QPalette &palette) override
    {
        QProxyStyle::polish(palette);
        palette.setColor(QPalette::Highlight, Tint);
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }
};

// Paint the selected first row of a list view with the application style.
QImage paintSelectedRow(QAbstractItemView &view)
{
    const QRect rect = view.visualRect(view.model()->index(0, 0));
    QStyleOptionViewItem option;
    option.initFrom(&view);
    option.rect = rect;
    option.state = QStyle::State_Enabled | QStyle::State_Selected;
    option.showDecorationSelected = true;
    QImage image(rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(view.palette().color(QPalette::Base));
    {
        QPainter painter(&image);
        qApp->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, &painter, &view);
    }
    return image;
}

QColor markerPixel(const QImage &row)
{
    return row.pixelColor(markerSampleX, row.height() / 2);
}

} // namespace

class WinUI3AppPaletteTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void applicationHighlightOverrideDrivesTokens();
    void applicationHighlightOverrideDrivesSelectionMarker();

    void proxyStylePolishTintReachesApplicationPalette();
    void proxyStylePolishTintReachesSelectionMarker();
    void proxyStylePolishTintSurvivesThemeAndAccentRefresh();

private:
    WinUI3::Style *installProxyStyle();
};

// A fresh style per test: the application palette is process state, so a tint
// from one test function must never be visible to the next.
void WinUI3AppPaletteTest::init()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3AppPaletteTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

WinUI3::Style *WinUI3AppPaletteTest::installProxyStyle()
{
    auto *winui = new WinUI3::Style(WinUI3::ThemeMode::Light);
    qApp->setStyle(new ThemeProxyStyle(winui));
    return winui;
}

void WinUI3AppPaletteTest::applicationHighlightOverrideDrivesTokens()
{
    qApp->setPalette(tintedPalette(qApp->palette()));
    QCOMPARE(qApp->palette().color(QPalette::Highlight), Tint);

    // Tokens derive selectionAccent from the palette, so the accent that paints
    // rows and controls must be the tinted one.
    QWidget probe;
    QCOMPARE(WinUI3::Private::tokens(probe.palette()).selectionAccent, Tint);

    // A widget polished afterwards must inherit the tint.
    QPushButton button(QStringLiteral("Probe"));
    QCOMPARE(button.palette().color(QPalette::Highlight), Tint);
}

void WinUI3AppPaletteTest::applicationHighlightOverrideDrivesSelectionMarker()
{
    QListWidget reference;
    reference.addItems({ QStringLiteral("First"), QStringLiteral("Second") });
    reference.resize(160, 60);
    const QColor stockMarker = markerPixel(paintSelectedRow(reference));
    QVERIFY2(
            colorDistance(stockMarker, reference.palette().color(QPalette::Base)) > 12,
            qPrintable(
                    QStringLiteral("no selection marker painted, got %1").arg(stockMarker.name())));

    qApp->setPalette(tintedPalette(qApp->palette()));
    QListWidget tinted;
    tinted.addItems({ QStringLiteral("First"), QStringLiteral("Second") });
    tinted.resize(160, 60);
    QCOMPARE(tinted.palette().color(QPalette::Highlight), Tint);
    const QColor tintedMarker = markerPixel(paintSelectedRow(tinted));

    QCOMPARE(WinUI3::Private::tokens(tinted.palette()).selectionAccent, Tint);
    QVERIFY2(colorDistance(tintedMarker, Tint) <= 12,
             qPrintable(QStringLiteral("QPalette::Highlight must reach the selection marker: "
                                       "got %1 instead of %2")
                                .arg(tintedMarker.name(), Tint.name())));
    QVERIFY2(colorDistance(tintedMarker, stockMarker) > 12,
             qPrintable(QStringLiteral("the override must repaint the selection marker: "
                                       "stock %1 vs tinted %2")
                                .arg(stockMarker.name(), tintedMarker.name())));
}

void WinUI3AppPaletteTest::proxyStylePolishTintReachesApplicationPalette()
{
    installProxyStyle();

    QCOMPARE(qApp->palette().color(QPalette::Highlight), Tint);
    QCOMPARE(qApp->palette().color(QPalette::HighlightedText), QColor(Qt::white));
    QCOMPARE(WinUI3::Private::tokens(qApp->palette()).selectionAccent, Tint);
}

void WinUI3AppPaletteTest::proxyStylePolishTintReachesSelectionMarker()
{
    installProxyStyle();

    QListWidget view;
    view.addItems({ QStringLiteral("First"), QStringLiteral("Second") });
    view.resize(160, 60);
    QCOMPARE(view.palette().color(QPalette::Highlight), Tint);
    const QColor marker = markerPixel(paintSelectedRow(view));

    // Paired token assertion for the same painted state: the pill's brush.
    QCOMPARE(WinUI3::Private::tokens(view.palette()).selectionAccent, Tint);
    QVERIFY2(colorDistance(marker, Tint) <= 12,
             qPrintable(QStringLiteral("the polished Highlight must paint the selection "
                                       "marker, got %1 instead of %2")
                                .arg(marker.name(), Tint.name())));
}

void WinUI3AppPaletteTest::proxyStylePolishTintSurvivesThemeAndAccentRefresh()
{
    WinUI3::Style *winui = installProxyStyle();
    QCOMPARE(qApp->palette().color(QPalette::Highlight), Tint);

    winui->setThemeMode(WinUI3::ThemeMode::Dark);
    QCOMPARE(qApp->palette().color(QPalette::Highlight), Tint);
    winui->setAccentColor(QColor(0x44, 0xaa, 0x44));
    QCOMPARE(qApp->palette().color(QPalette::Highlight), Tint);
    QCOMPARE(WinUI3::Private::tokens(qApp->palette()).selectionAccent, Tint);
}

QTEST_MAIN(WinUI3AppPaletteTest)
#include "tst_winui3apppalette.moc"
