// SPDX-License-Identifier: LGPL-2.1-or-later
// Popup-row surface contracts split out of tst_winui3menus.cpp (60 KB ratchet).
#include <winui3style/winui3style.h>

#include "../src/lib/winui3menus_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QFontMetrics>
#include <QImage>
#include <QMenu>
#include <QPalette>
#include <QPainter>
#include <QStyleOptionMenuItem>
#include <QtTest>

class WinUI3MenuSurfaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void menuRowHoverHasSingleOpaqueSurface();
};

void WinUI3MenuSurfaceTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Dark));
}

void WinUI3MenuSurfaceTest::menuRowHoverHasSingleOpaqueSurface()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    QMenu menu;
    QPalette palette = style->standardPalette();
    // Non-Mica fallback: still carrying the translucent flyout tint while
    // the flyout panel itself is already opaque.
    QColor tint = palette.color(QPalette::Window);
    tint.setAlpha(178);
    palette.setColor(QPalette::Window, tint);
    menu.setPalette(palette);
    QCOMPARE(menu.palette().color(QPalette::Window).alpha(), 178);
    const QColor opaqueSurface = WinUI3::Private::popupSurfaceColor(palette);

    QStyleOptionMenuItem item;
    item.initFrom(&menu);
    QCOMPARE(item.palette.color(QPalette::Window).alpha(), 178);
    item.palette = palette;
    item.rect = QRect(0, 0, 200, 36);
    item.menuItemType = QStyleOptionMenuItem::Normal;
    item.checkType = QStyleOptionMenuItem::NotCheckable;
    item.text = QStringLiteral("Run normally");
    item.font = menu.font();
    item.fontMetrics = QFontMetrics(item.font);
    item.state = QStyle::State_Enabled | QStyle::State_Selected;

    QImage frame(item.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(opaqueSurface);
    {
        QPainter painter(&frame);
        style->drawControl(QStyle::CE_MenuItem, &item, &painter, &menu);
    }
    const QRect pill = WinUI3::Private::popupRowPillRect(item.rect, false).toRect();
    QCOMPARE(pill, QRect(4, 2, 192, 32));
    for (const QPoint at : { QPoint(0, 0), QPoint(1, 1), QPoint(100, 1),
                             QPoint(pill.left() - 1, pill.center().y()) }) {
        QCOMPARE(frame.pixelColor(at), opaqueSurface);
    }
    QVERIFY(frame.pixelColor(pill.center()) != opaqueSurface);
}

QTEST_MAIN(WinUI3MenuSurfaceTest)
#include "tst_winui3menusurface.moc"
