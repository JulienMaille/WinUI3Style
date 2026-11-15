#include <winui3style/winui3style.h>
#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QImage>
#include <cstdio>

static void sample(const char *stage, QTableView &table)
{
    table.grab().save(QStringLiteral("hdr_%1.png").arg(stage));
    const QImage img = table.grab().toImage();
    const int headerY = table.horizontalHeader()->height() / 2;
    const int rowH = table.rowHeight(0);
    const int top = table.horizontalHeader()->height();
    const int x = img.width() / 2;
    const QColor header = img.pixelColor(110, headerY);
    const QColor odd = img.pixelColor(x, top + rowH / 2);          // row 1
    const QColor even = img.pixelColor(x, top + rowH + rowH / 2);  // row 2
    fprintf(stderr, "[%s] header=%s odd(row1)=%s even(row2)=%s\n", stage,
            header.name().toUtf8().constData(), odd.name().toUtf8().constData(),
            even.name().toUtf8().constData());
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    auto *style = new WinUI3::Style(WinUI3::ThemeMode::Dark);
    QApplication::setStyle(style);
    app.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    QTableView table;
    table.setAlternatingRowColors(true);
    auto *model = new QStandardItemModel(6, 3, &table);
    model->setHorizontalHeaderLabels({ "Control", "State", "Coverage" });
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 3; ++c)
            model->setItem(r, c, new QStandardItem(QStringLiteral("r%1").arg(r)));
    table.setModel(model);
    table.resize(560, 260);
    table.show();
    app.processEvents();
    sample("dark", table);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    app.processEvents();
    sample("light", table);
    return 0;
}