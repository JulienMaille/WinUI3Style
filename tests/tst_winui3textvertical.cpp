// SPDX-License-Identifier: LGPL-2.1-or-later
// Cross-widget guard for issue #119: WinUI 3 centres text in a padded box whose
// bottom inset exceeds its top inset, so every direct mapping paints its label
// one logical pixel above the geometric centre. The pinned XAML pairs are
// collected in src/lib/winui3tokens_p.h. Each test pairs a rendered-ink
// assertion with a QCOMPARE on the geometry or token of the same state.
#include <winui3style/winui3style.h>

#include "../src/lib/winui3tokens_p.h"

#include <QComboBox>
#include <QImage>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionComboBox>
#include <QStyleOptionMenuItem>
#include <QtTest>

#include "winui3testhelpers.h"

class WinUI3TextVerticalTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void winuiPaddingPairIsPinned();
    void lineEditKeepsAsymmetricContents();
    void comboLabelUsesWinuiVerticalPadding();
    void menuItemLabelUsesWinuiVerticalPadding();
};

void WinUI3TextVerticalTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3TextVerticalTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3TextVerticalTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

namespace {
int firstInkRow(const QImage &image, const QImage *baseline)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 8; x < image.width() - 8; ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (baseline) {
                const QColor plain = baseline->pixelColor(x, y);
                if (pixel.alpha() != plain.alpha() || pixel.red() != plain.red()
                    || pixel.green() != plain.green() || pixel.blue() != plain.blue())
                    return y;
            } else if (pixel.alpha() > 0) {
                return y;
            }
        }
    }
    return -1;
}

int referenceInkRow(const QRect &rect, const QFont &font, const QString &text, int width,
                    int height)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QFont grayFont = font;
    grayFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(grayFont.styleStrategy()
                                                                | QFont::NoSubpixelAntialias));
    painter.setFont(grayFont);
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic, text);
    return firstInkRow(image, nullptr);
}
} // namespace

void WinUI3TextVerticalTest::winuiPaddingPairIsPinned()
{
    // The single derived offset must keep agreeing with the pinned Button pair
    // (Button_themeresources.xaml @ 9f89c2da, docs/winui-2.4/manifest.json).
    QCOMPARE(WinUI3::Private::ButtonPaddingBottom - WinUI3::Private::ButtonPaddingTop, 1);
    QCOMPARE(WinUI3::Private::WinuiVerticalContentCenterOffset,
             WinUI3::Private::ButtonPaddingBottom - WinUI3::Private::ButtonPaddingTop);
}

void WinUI3TextVerticalTest::lineEditKeepsAsymmetricContents()
{
    // TextBox_themeresources_perf2026.xaml pins TextControlThemePadding 10,5,10,6.
    // The existing Qt mapping (10,5,-6,-6) already encodes it: pin it so a
    // future "symmetric cleanup" cannot silently reintroduce #119 here.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QLineEdit editor;
    editor.setFixedSize(160, 32);
    QStyleOptionFrame option;
    option.initFrom(&editor);
    option.rect = editor.rect();
    const QRect contents = style->subElementRect(QStyle::SE_LineEditContents, &option, &editor);
    QCOMPARE(contents, QRect(10, 5, 144, 21));
    QCOMPARE(contents.top() - option.rect.top(), WinUI3::Private::ButtonPaddingTop);
    QCOMPARE(option.rect.bottom() - contents.bottom(), WinUI3::Private::ButtonPaddingBottom);
}

void WinUI3TextVerticalTest::comboLabelUsesWinuiVerticalPadding()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QComboBox combo;
    combo.addItems({ QStringLiteral("Blue"), QStringLiteral("Green") });
    combo.setFixedSize(160, 32);
    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = QRect(0, 0, 160, 32);
    option.currentText = combo.currentText();
    option.state = QStyle::State_Enabled;

    // Geometry assertion paired with the ink assertion below.
    const QRect editField = style->subControlRect(QStyle::CC_ComboBox, &option,
                                                  QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY(!editField.isEmpty());
    QCOMPARE(editField.center().y(), option.rect.center().y());

    QImage image(160, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QStyleOptionComboBox blank = option;
    blank.currentText.clear();
    QImage empty(160, 32, QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    {
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ComboBoxLabel, &option, &painter, &combo);
        QPainter emptyPainter(&empty);
        style->drawControl(QStyle::CE_ComboBoxLabel, &blank, &emptyPainter, &combo);
    }
    const int styledTop = firstInkRow(image, &empty);
    QVERIFY2(styledTop >= 0, "the closed combo label painted no ink");
    const int symmetricTop = referenceInkRow(editField, combo.font(), option.currentText, 160, 32);
    QVERIFY2(symmetricTop >= 0, "the symmetric combo reference painted no ink");
    QCOMPARE(styledTop, symmetricTop - WinUI3::Private::WinuiVerticalContentCenterOffset);
}

void WinUI3TextVerticalTest::menuItemLabelUsesWinuiVerticalPadding()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QMenu menu;
    QStyleOptionMenuItem option;
    option.initFrom(&menu);
    option.menuItemType = QStyleOptionMenuItem::Normal;
    option.font = menu.font();
    option.state = QStyle::State_Enabled;
    option.rect = QRect(0, 0, 200, 32);
    option.text = QStringLiteral("Open");

    const QRect symmetric(42, option.rect.top(), 150, option.rect.height());
    QImage image(200, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QStyleOptionMenuItem blank = option;
    blank.text.clear();
    QImage empty(200, 32, QImage::Format_ARGB32_Premultiplied);
    empty.fill(Qt::transparent);
    {
        QPainter painter(&image);
        style->drawControl(QStyle::CE_MenuItem, &option, &painter, &menu);
        QPainter emptyPainter(&empty);
        style->drawControl(QStyle::CE_MenuItem, &blank, &emptyPainter, &menu);
    }
    const int styledTop = firstInkRow(image, &empty);
    QVERIFY2(styledTop >= 0, "the menu item label painted no ink");
    const int symmetricTop = referenceInkRow(symmetric, option.font, option.text, 200, 32);
    QVERIFY2(symmetricTop >= 0, "the symmetric menu reference painted no ink");
    QCOMPARE(styledTop, symmetricTop - WinUI3::Private::WinuiVerticalContentCenterOffset);
}

QTEST_MAIN(WinUI3TextVerticalTest)
#include "tst_winui3textvertical.moc"
