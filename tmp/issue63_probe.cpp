#include <winui3style/winui3style.h>
#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QImage>
#include <QPalette>
#include <cstdio>

static void dump(const char *stage, QTableView &table)
{
    QHeaderView *h = table.horizontalHeader();
    const QPalette hp = h->palette();
    const QVariant snap = h->property("_winui_implicit_palette_snapshot");
    const bool owned = h->property("_winui_theme_owned_palette").toBool();
    const bool setPal = h->testAttribute(Qt::WA_SetPalette);
    const bool match = snap.isValid() && hp == snap.value<QPalette>();
    fprintf(stderr,
            "[%s] header Base=%s Window=%s | WA_SetPalette=%d owned=%d snapValid=%d match=%d | "
            "view Base=%s\n",
            stage, hp.color(QPalette::Base).name().toUtf8().constData(),
            hp.color(QPalette::Window).name().toUtf8().constData(), int(setPal), int(owned),
            int(snap.isValid()), int(match),
            table.palette().color(QPalette::Base).name().toUtf8().constData());
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    auto *style = new WinUI3::Style(WinUI3::ThemeMode::Dark);
    QApplication::setStyle(style);
    QFont f(QStringLiteral("Segoe UI"), 10);
    app.setFont(f);

    QTableView table;
    table.setFont(f);
    auto *model = new QStandardItemModel(4, 3, &table);
    model->setHorizontalHeaderLabels({ "Control", "State", "Coverage" });
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 3; ++c)
            model->setItem(r, c, new QStandardItem(QStringLiteral("row%1").arg(r)));
    table.setModel(model);
    table.resize(560, 220);
    table.show();
    app.processEvents();
    dump("dark-initial", table);
    table.grab().save(QStringLiteral("t63_dark1.png"));

    style->setThemeMode(WinUI3::ThemeMode::Light);
    app.processEvents();
    dump("light", table);
    table.grab().save(QStringLiteral("t63_light.png"));

    style->setThemeMode(WinUI3::ThemeMode::Dark);
    app.processEvents();
    dump("dark-again", table);
    table.grab().save(QStringLiteral("t63_dark2.png"));
    return 0;
}