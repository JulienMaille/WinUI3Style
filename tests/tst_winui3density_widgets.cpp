#include <winui3style/navigationview.h>
#include <winui3style/winui3style.h>

#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QHeaderView>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTableView>
#include <QTest>
#include <QTimeEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>

namespace {

struct OfficialCompactWidgets {
    QLineEdit textBox;
    QLineEdit passwordBox;
    QLineEdit autoSuggestBox;
    QComboBox comboBox;
    QDateEdit datePicker;
    QTimeEdit timePicker;
    QListView listView;
    QTreeView treeView;
    QMenuBar menuBar;
    WinUI3::NavigationView navigationView;

    explicit OfficialCompactWidgets(QWidget *parent)
        : textBox(parent),
          passwordBox(parent),
          autoSuggestBox(parent),
          comboBox(parent),
          datePicker(parent),
          timePicker(parent),
          listView(parent),
          treeView(parent),
          menuBar(parent),
          navigationView(parent)
    {
        passwordBox.setEchoMode(QLineEdit::Password);
        autoSuggestBox.setPlaceholderText(QStringLiteral("Search"));
        autoSuggestBox.setCompleter(new QCompleter(
            QStringList{QStringLiteral("Alpha"), QStringLiteral("Beta")},
            &autoSuggestBox));

        comboBox.addItems({QStringLiteral("First item"), QStringLiteral("Second item")});
        datePicker.setCalendarPopup(true);
        timePicker.setTime(QTime(12, 30));

        auto *listModel = new QStandardItemModel(2, 1, &listView);
        listModel->setData(listModel->index(0, 0), QStringLiteral("First item"));
        listModel->setData(listModel->index(1, 0), QStringLiteral("Second item"));
        listView.setModel(listModel);

        auto *treeModel = new QStandardItemModel(2, 1, &treeView);
        treeModel->setData(treeModel->index(0, 0), QStringLiteral("First item"));
        treeModel->setData(treeModel->index(1, 0), QStringLiteral("Second item"));
        treeView.setModel(treeModel);
        treeView.setHeaderHidden(true);

        menuBar.addAction(QStringLiteral("File"));
        menuBar.addAction(QStringLiteral("View"));

        navigationView.addPage(new QWidget, QIcon(), QStringLiteral("Settings"));
    }
};

void settle(QWidget &root)
{
    root.resize(900, 900);
    root.show();
    QVERIFY(QTest::qWaitForWindowExposed(&root, 1000) || root.isVisible());
    QCoreApplication::processEvents();
    if (root.layout())
        root.layout()->activate();
    QCoreApplication::processEvents();
}

template <typename Range>
void applyStyleToWidgets(WinUI3::Style &style, const Range &widgets)
{
    for (QWidget *widget : widgets)
        widget->setStyle(&style);
}

int rowHeight(const QAbstractItemView &view)
{
    const QModelIndex index = view.model()->index(0, 0);
    return view.visualRect(index).height();
}

} // namespace

class WinUI3DensityWidgetsTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void allOfficialWidgetsUseCompactMetricsAfterRealLayout();
    void inheritedDensityWorksForRealWidgets();
    void compactDoesNotResizeUnlistedControls();
};

void WinUI3DensityWidgetsTest::init()
{
    qApp->setStyle(new WinUI3::Style);
}

void WinUI3DensityWidgetsTest::cleanup()
{
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
}

void WinUI3DensityWidgetsTest::allOfficialWidgetsUseCompactMetricsAfterRealLayout()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    QWidget root;
    root.setStyle(&style);
    QVBoxLayout layout(&root);
    OfficialCompactWidgets widgets(&root);

    const QList<QWidget *> officialWidgets = {
        &widgets.textBox, &widgets.passwordBox, &widgets.autoSuggestBox,
        &widgets.comboBox, &widgets.datePicker, &widgets.timePicker,
        &widgets.listView, &widgets.treeView, &widgets.navigationView};
    applyStyleToWidgets(style, officialWidgets);
    widgets.menuBar.setStyle(&style);
    widgets.navigationView.navigationList()->setStyle(&style);
    for (QWidget *widget : officialWidgets)
        layout.addWidget(widget);
    layout.addWidget(&widgets.menuBar);

    settle(root);

    const int textStandard = widgets.textBox.sizeHint().height();
    const int passwordStandard = widgets.passwordBox.sizeHint().height();
    const int autoSuggestStandard = widgets.autoSuggestBox.sizeHint().height();
    const int comboStandard = widgets.comboBox.sizeHint().height();
    const int dateStandard = widgets.datePicker.sizeHint().height();
    const int timeStandard = widgets.timePicker.sizeHint().height();
    QVERIFY(textStandard >= 32);
    QVERIFY(passwordStandard >= 32);
    QVERIFY(autoSuggestStandard >= 32);
    QVERIFY(comboStandard >= 32);
    QVERIFY(dateStandard >= 32);
    QVERIFY(timeStandard >= 32);
    QCOMPARE(rowHeight(widgets.listView), 40);
    QCOMPARE(rowHeight(widgets.treeView), 28);
    QCOMPARE(widgets.navigationView.navigationList()->sizeHintForRow(0), 40);
    const int menuBarStandard = widgets.menuBar.sizeHint().height();
    const int menuActionStandard = widgets.menuBar.actionGeometry(
        widgets.menuBar.actions().constFirst()).height();
    QVERIFY(menuBarStandard >= 36);
    QVERIFY(menuActionStandard >= 32);

    // This is the exact Designer/gallery path: writing the public Q_PROPERTY
    // must invoke the setter and invalidate already-visible widgets.
    QVERIFY(style.setProperty("densityMode", int(WinUI3::DensityMode::Compact)));
    QCoreApplication::processEvents();
    layout.activate();
    QCoreApplication::processEvents();

    const QList<QPair<int, int>> editorHeights = {
        {widgets.textBox.sizeHint().height(), textStandard},
        {widgets.passwordBox.sizeHint().height(), passwordStandard},
        {widgets.autoSuggestBox.sizeHint().height(), autoSuggestStandard},
        {widgets.comboBox.sizeHint().height(), comboStandard},
        {widgets.datePicker.sizeHint().height(), dateStandard},
        {widgets.timePicker.sizeHint().height(), timeStandard}};
    for (const auto &[compact, standard] : editorHeights) {
        QVERIFY(compact >= 24);
        QVERIFY(compact < standard);
    }
    QCOMPARE(rowHeight(widgets.listView), 32);
    QCOMPARE(rowHeight(widgets.treeView), 24);
    QCOMPARE(widgets.navigationView.navigationList()->sizeHintForRow(0), 32);
    const int menuBarCompact = widgets.menuBar.sizeHint().height();
    const int menuActionCompact = widgets.menuBar.actionGeometry(
        widgets.menuBar.actions().constFirst()).height();
    QVERIFY(menuBarCompact >= 28);
    QVERIFY(menuActionCompact >= 24);
    QVERIFY(menuBarCompact < menuBarStandard);
    QVERIFY(menuActionCompact < menuActionStandard);

    // ComboBox popup rows are part of the documented compact family too.
    widgets.comboBox.showPopup();
    QCoreApplication::processEvents();
    QCOMPARE(rowHeight(*qobject_cast<QAbstractItemView *>(widgets.comboBox.view())), 32);
    widgets.comboBox.hidePopup();
}

void WinUI3DensityWidgetsTest::inheritedDensityWorksForRealWidgets()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    QWidget root;
    root.setStyle(&style);
    QVBoxLayout rootLayout(&root);
    QWidget panel(&root);
    panel.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("compact"));
    QVBoxLayout panelLayout(&panel);
    QLineEdit textBox(&panel);
    QLineEdit autoSuggestBox(&panel);
    autoSuggestBox.setCompleter(new QCompleter(
        QStringList{QStringLiteral("Alpha"), QStringLiteral("Beta")},
        &autoSuggestBox));
    QComboBox comboBox(&panel);
    QDateEdit datePicker(&panel);
    QTimeEdit timePicker(&panel);
    QListView listView(&panel);
    QStandardItemModel listModel(1, 1, &listView);
    listModel.setData(listModel.index(0, 0), QStringLiteral("Item"));
    listView.setModel(&listModel);
    applyStyleToWidgets(style, QList<QWidget *> {
        &panel, &textBox, &autoSuggestBox, &comboBox, &datePicker, &timePicker,
        &listView});
    panelLayout.addWidget(&textBox);
    panelLayout.addWidget(&autoSuggestBox);
    panelLayout.addWidget(&comboBox);
    panelLayout.addWidget(&datePicker);
    panelLayout.addWidget(&timePicker);
    panelLayout.addWidget(&listView);
    rootLayout.addWidget(&panel);

    settle(root);
    const int textCompact = textBox.sizeHint().height();
    const int autoSuggestCompact = autoSuggestBox.sizeHint().height();
    const int comboCompact = comboBox.sizeHint().height();
    const int dateCompact = datePicker.sizeHint().height();
    const int timeCompact = timePicker.sizeHint().height();
    for (int height : {textCompact, autoSuggestCompact, comboCompact,
                       dateCompact, timeCompact})
        QVERIFY(height >= 24);
    QCOMPARE(rowHeight(listView), 32);
    autoSuggestBox.completer()->setCompletionPrefix(QStringLiteral("A"));
    autoSuggestBox.completer()->complete();
    QCoreApplication::processEvents();
    QCOMPARE(rowHeight(*autoSuggestBox.completer()->popup()), 32);
    autoSuggestBox.completer()->popup()->hide();

    autoSuggestBox.setCompleter(new QCompleter(
        QStringList{QStringLiteral("Alpha"), QStringLiteral("Alpine")},
        &autoSuggestBox));
    QTest::keyClick(&autoSuggestBox, Qt::Key_A);
    autoSuggestBox.completer()->setCompletionPrefix(QStringLiteral("A"));
    autoSuggestBox.completer()->complete();
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Style::densityMode(autoSuggestBox.completer()->popup()),
             WinUI3::DensityMode::Compact);
    QCOMPARE(rowHeight(*autoSuggestBox.completer()->popup()), 32);
    autoSuggestBox.completer()->popup()->hide();

    panel.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("standard"));
    QCoreApplication::processEvents();
    panelLayout.activate();
    QCoreApplication::processEvents();
    QVERIFY(textBox.sizeHint().height() > textCompact);
    QVERIFY(autoSuggestBox.sizeHint().height() > autoSuggestCompact);
    QVERIFY(comboBox.sizeHint().height() > comboCompact);
    QVERIFY(datePicker.sizeHint().height() > dateCompact);
    QVERIFY(timePicker.sizeHint().height() > timeCompact);
    QCOMPARE(rowHeight(listView), 40);
    autoSuggestBox.completer()->complete();
    QCoreApplication::processEvents();
    QCOMPARE(rowHeight(*autoSuggestBox.completer()->popup()), 40);
    autoSuggestBox.completer()->popup()->hide();

    std::unique_ptr<QStyle> fusion(QStyleFactory::create(QStringLiteral("Fusion")));
    autoSuggestBox.setStyle(fusion.get());
    QVERIFY(!autoSuggestBox.completer()->popup()
                 ->property(WinUI3::Style::DensityProperty).isValid());
    autoSuggestBox.setStyle(&style);
}

void WinUI3DensityWidgetsTest::compactDoesNotResizeUnlistedControls()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    QWidget root;
    root.setStyle(&style);
    QVBoxLayout layout(&root);
    QPushButton button(QStringLiteral("Button"), &root);
    QSpinBox spinBox(&root);
    QSlider slider(Qt::Horizontal, &root);
    QTabWidget tabs(&root);
    QTableView table(&root);
    QStandardItemModel tableModel(1, 1, &table);
    tableModel.setData(tableModel.index(0, 0), QStringLiteral("Cell"));
    table.setModel(&tableModel);
    QMenu menu(&root);
    QAction menuAction(QStringLiteral("Menu item"), &menu);
    menu.addAction(&menuAction);

    applyStyleToWidgets(style, QList<QWidget *> {
        &button, &spinBox, &slider, &tabs, &table, &menu});

    layout.addWidget(&button);
    layout.addWidget(&spinBox);
    layout.addWidget(&slider);
    layout.addWidget(&tabs);
    layout.addWidget(&table);
    settle(root);

    const QSize buttonStandard = button.sizeHint();
    const QSize spinStandard = spinBox.sizeHint();
    const QSize sliderStandard = slider.sizeHint();
    const QSize tabsStandard = tabs.sizeHint();
    const int tableRowStandard = table.rowHeight(0);
    const int headerStandard = table.horizontalHeader()->sectionSize(0);
    const int menuItemStandard = menu.actionGeometry(&menuAction).height();

    style.setDensityMode(WinUI3::DensityMode::Compact);
    QCoreApplication::processEvents();
    layout.activate();
    QCoreApplication::processEvents();

    QCOMPARE(button.sizeHint(), buttonStandard);
    QCOMPARE(spinBox.sizeHint(), spinStandard);
    QCOMPARE(slider.sizeHint(), sliderStandard);
    QCOMPARE(tabs.sizeHint(), tabsStandard);
    QCOMPARE(table.rowHeight(0), tableRowStandard);
    QCOMPARE(table.horizontalHeader()->sectionSize(0), headerStandard);
    QCOMPARE(menu.actionGeometry(&menuAction).height(), menuItemStandard);
}

QTEST_MAIN(WinUI3DensityWidgetsTest)
#include "tst_winui3density_widgets.moc"
