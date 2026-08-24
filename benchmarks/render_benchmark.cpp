#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QImage>
#include <QListView>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTabBar>
#include <QTableView>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace {

constexpr int DefaultRows = 10000;
constexpr int DefaultIterations = 80;
constexpr int Warmups = 5;

struct Options {
    int rows = DefaultRows;
    int iterations = DefaultIterations;
    bool icons = true;
};

double percentile(std::vector<double> values, double fraction)
{
    if (values.empty())
        return 0.0;
    std::sort(values.begin(), values.end());
    const int index = qBound(0, int(std::ceil(fraction * values.size())) - 1,
                             int(values.size()) - 1);
    return values[index];
}

void render(QWidget *widget, QImage *sink)
{
    sink->fill(Qt::transparent);
    widget->render(sink);
}

void report(const QString &name, QWidget *widget, int iterations,
            const std::function<void(int)> &beforeFrame)
{
    QImage image(widget->size(), QImage::Format_ARGB32_Premultiplied);
    for (int warmup = 0; warmup < Warmups; ++warmup) {
        beforeFrame(warmup);
        QCoreApplication::processEvents();
        render(widget, &image);
    }

    std::vector<double> samples;
    samples.reserve(iterations);
    for (int iteration = 0; iteration < iterations; ++iteration) {
        beforeFrame(iteration + Warmups);
        QCoreApplication::processEvents();
        QElapsedTimer timer;
        timer.start();
        render(widget, &image);
        samples.push_back(timer.nsecsElapsed() / 1000000.0);
    }

    qInfo().noquote() << QStringLiteral("%1 p50=%2 ms p95=%3 ms samples=%4")
                             .arg(name)
                             .arg(percentile(samples, 0.50), 0, 'f', 3)
                             .arg(percentile(samples, 0.95), 0, 'f', 3)
                             .arg(samples.size());
}

QIcon benchmarkIcon(int row)
{
    using WinUI3::Icon;
    constexpr Icon glyphs[] = {Icon::Folder, Icon::Settings, Icon::Search,
                               Icon::Edit, Icon::More};
    return WinUI3::icon(glyphs[row % (sizeof(glyphs) / sizeof(glyphs[0]))]);
}

void prepare(QWidget *widget, const QSize &size)
{
    widget->resize(size);
    widget->show();
    QCoreApplication::processEvents();
}

void benchmarkList(const Options &options)
{
    QStandardItemModel model;
    for (int row = 0; row < options.rows; ++row) {
        auto *item = new QStandardItem(QStringLiteral("List item %1").arg(row));
        if (options.icons)
            item->setIcon(benchmarkIcon(row));
        model.appendRow(item);
    }

    QListView view;
    view.setModel(&model);
    view.setUniformItemSizes(true);
    prepare(&view, QSize(720, 540));
    report(options.icons ? QStringLiteral("list-icons")
                         : QStringLiteral("list-no-icons"),
           &view, options.iterations, [&view](int frame) {
        view.verticalScrollBar()->setValue(
            (frame * 37) % qMax(1, view.verticalScrollBar()->maximum()));
    });
}

void benchmarkTree(const Options &options)
{
    QStandardItemModel model;
    model.setHorizontalHeaderLabels({QStringLiteral("Tree")});
    const int parentCount = qMax(1, options.rows / 100);
    for (int parentRow = 0; parentRow < parentCount; ++parentRow) {
        auto *parent = new QStandardItem(
            QStringLiteral("Group %1").arg(parentRow));
        if (options.icons)
            parent->setIcon(benchmarkIcon(parentRow));
        const int children = qMin(100, options.rows - parentRow * 100);
        for (int childRow = 0; childRow < children; ++childRow) {
            auto *child = new QStandardItem(QStringLiteral("Tree item %1.%2")
                                                 .arg(parentRow).arg(childRow));
            if (options.icons)
                child->setIcon(benchmarkIcon(parentRow + childRow));
            parent->appendRow(child);
        }
        model.appendRow(parent);
    }

    QTreeView view;
    view.setModel(&model);
    view.setUniformRowHeights(true);
    view.expandAll();
    prepare(&view, QSize(720, 540));
    report(options.icons ? QStringLiteral("tree-icons")
                         : QStringLiteral("tree-no-icons"),
           &view, options.iterations, [&view](int frame) {
        view.verticalScrollBar()->setValue(
            (frame * 29) % qMax(1, view.verticalScrollBar()->maximum()));
    });
}

void benchmarkTable(const Options &options)
{
    constexpr int Columns = 6;
    QStandardItemModel model(options.rows, Columns);
    for (int row = 0; row < options.rows; ++row) {
        for (int column = 0; column < Columns; ++column) {
            auto *item = new QStandardItem(
                QStringLiteral("Cell %1/%2").arg(row).arg(column));
            if (options.icons && column == 0)
                item->setIcon(benchmarkIcon(row));
            model.setItem(row, column, item);
        }
    }
    model.setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("A"),
                                     QStringLiteral("B"), QStringLiteral("C"),
                                     QStringLiteral("D"), QStringLiteral("E")});

    QTableView view;
    view.setModel(&model);
    view.setAlternatingRowColors(false);
    view.horizontalHeader()->setStretchLastSection(true);
    prepare(&view, QSize(960, 540));
    report(options.icons ? QStringLiteral("table-icons")
                         : QStringLiteral("table-no-icons"),
           &view, options.iterations, [&view](int frame) {
        view.verticalScrollBar()->setValue(
            (frame * 23) % qMax(1, view.verticalScrollBar()->maximum()));
    });
}

void benchmarkRichSurface(const Options &options)
{
    QWidget surface;
    auto *layout = new QVBoxLayout(&surface);
    auto *toolbar = new QToolBar(&surface);
    for (int action = 0; action < 96; ++action)
        toolbar->addAction(options.icons ? benchmarkIcon(action) : QIcon(),
                           QStringLiteral("Action %1").arg(action));
    layout->addWidget(toolbar);

    auto *tabs = new QTabBar(&surface);
    for (int tab = 0; tab < 32; ++tab)
        tabs->addTab(options.icons ? benchmarkIcon(tab) : QIcon(),
                     QStringLiteral("Tab %1").arg(tab));
    layout->addWidget(tabs);

    auto *model = new QStandardItemModel(500, 6, &surface);
    for (int row = 0; row < model->rowCount(); ++row)
        for (int column = 0; column < model->columnCount(); ++column)
            model->setData(model->index(row, column),
                           QStringLiteral("Rich %1/%2").arg(row).arg(column));
    auto *table = new QTableView(&surface);
    table->setModel(model);
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table, 1);

    prepare(&surface, QSize(1100, 700));
    report(options.icons ? QStringLiteral("rich-surface-icons")
                         : QStringLiteral("rich-surface-no-icons"),
           &surface, options.iterations, [table](int frame) {
        table->verticalScrollBar()->setValue(
            (frame * 17) % qMax(1, table->verticalScrollBar()->maximum()));
    });
}

Options parseOptions(const QApplication &application)
{
    Options options;
    const QStringList arguments = application.arguments();
    for (int index = 1; index < arguments.size(); ++index) {
        const QString argument = arguments.at(index);
        if (argument == QStringLiteral("--no-icons")) {
            options.icons = false;
        } else if (argument.startsWith(QStringLiteral("--rows="))) {
            options.rows = qMax(1, argument.mid(7).toInt());
        } else if (argument.startsWith(QStringLiteral("--iterations="))) {
            options.iterations = qMax(1, argument.mid(13).toInt());
        }
    }
    return options;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    application.setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
    const Options options = parseOptions(application);

    benchmarkList(options);
    benchmarkTree(options);
    benchmarkTable(options);
    benchmarkRichSurface(options);
    return 0;
}
