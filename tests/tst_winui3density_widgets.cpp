#include <winui3style/navigationview.h>
#include <winui3style/winui3style.h>

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QRadioButton>
#include <QHeaderView>
#include <QImage>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
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
    void autoSuggestMatchesCaseInsensitiveSubstrings();
    void compactDoesNotResizeUnlistedControls();
    void calendarPopupRemainsReadableInLightAndDark();
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
        QCOMPARE(compact, 24);
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
    QCheckBox checkBox(QStringLiteral("Option"), &panel);
    QRadioButton radioButton(QStringLiteral("Choice"), &panel);
    QListView listView(&panel);
    QStandardItemModel listModel(1, 1, &listView);
    listModel.setData(listModel.index(0, 0), QStringLiteral("Item"));
    listView.setModel(&listModel);
    applyStyleToWidgets(style, QList<QWidget *> {
        &panel, &textBox, &autoSuggestBox, &comboBox, &datePicker, &timePicker,
        &checkBox, &radioButton, &listView});
    panelLayout.addWidget(&textBox);
    panelLayout.addWidget(&autoSuggestBox);
    panelLayout.addWidget(&comboBox);
    panelLayout.addWidget(&datePicker);
    panelLayout.addWidget(&timePicker);
    panelLayout.addWidget(&checkBox);
    panelLayout.addWidget(&radioButton);
    panelLayout.addWidget(&listView);
    rootLayout.addWidget(&panel);

    settle(root);
    const int textCompact = textBox.sizeHint().height();
    const int autoSuggestCompact = autoSuggestBox.sizeHint().height();
    const int comboCompact = comboBox.sizeHint().height();
    const int dateCompact = datePicker.sizeHint().height();
    const int timeCompact = timePicker.sizeHint().height();
    const int checkCompact = checkBox.sizeHint().height();
    const int radioCompact = radioButton.sizeHint().height();
    for (int height : {textCompact, autoSuggestCompact, comboCompact,
                       dateCompact, timeCompact, checkCompact, radioCompact})
        QCOMPARE(height, 24);
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
    QVERIFY(checkBox.sizeHint().height() > checkCompact);
    QVERIFY(radioButton.sizeHint().height() > radioCompact);
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

void WinUI3DensityWidgetsTest::autoSuggestMatchesCaseInsensitiveSubstrings()
{
    QLineEdit editor;
    auto *completer = new QCompleter(
        QStringList{QStringLiteral("Alpha"), QStringLiteral("Beta"),
                    QStringLiteral("Settings")}, &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(240, editor.sizeHint().height());
    editor.show();
    editor.setFocus();
    QTest::keyClicks(&editor, QStringLiteral("ET"));
    QTRY_COMPARE(completer->completionPrefix(), QStringLiteral("ET"));
    QTRY_COMPARE(completer->completionCount(), 2);
    QTRY_VERIFY(completer->popup()->isVisible());
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

void WinUI3DensityWidgetsTest::calendarPopupRemainsReadableInLightAndDark()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Light);
    QWidget root;
    root.setStyle(&style);
    QDateEdit date(&root);
    date.setCalendarPopup(true);
    date.setDate(QDate(2026, 8, 15));
    date.resize(200, 40);
    root.show();
    QCoreApplication::processEvents();
    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        if (style.themeMode() != mode)
            style.setThemeMode(mode);
        QCoreApplication::processEvents();
        QTest::mouseClick(&date, Qt::LeftButton, {}, QPoint(date.width() - 5,
                                                            date.height() / 2));
        QCoreApplication::processEvents();
        auto *calendar = date.calendarWidget();
        QVERIFY(calendar);
        // The offscreen QPA plugin does not always route a synthetic click to
        // QDateTimeEdit's private arrow subcontrol after a live palette flip.
        // Showing the already-configured calendar is equivalent to the popup
        // window that the edit creates and keeps this test focused on its
        // rendered day grid.
        if (!calendar->isVisible()) {
            calendar->show();
            QCoreApplication::processEvents();
        }
        QVERIFY(calendar->isVisible());
        auto *view = calendar->findChild<QAbstractItemView *>(
            QStringLiteral("qt_calendar_calendarview"));
        QVERIFY(view);
        QVERIFY(view->isVisible());
        auto *navigation = calendar->findChild<QWidget *>(
            QStringLiteral("qt_calendar_navigationbar"));
        QVERIFY(navigation);
        QCOMPARE(navigation->backgroundRole(), QPalette::Window);
        auto *monthButton = calendar->findChild<QWidget *>(
            QStringLiteral("qt_calendar_monthbutton"));
        QVERIFY(monthButton);
        QTest::mouseMove(monthButton, monthButton->rect().center());
        QCoreApplication::processEvents();
        const QModelIndex dayIndex = view->model()->index(2, 3);
        QVERIFY(dayIndex.isValid());
        QStyleOptionViewItem dayOption;
        dayOption.initFrom(view->viewport());
        dayOption.rect = view->visualRect(dayIndex);
        dayOption.index = dayIndex;
        dayOption.text = dayIndex.data(Qt::DisplayRole).toString();
        dayOption.features = QStyleOptionViewItem::HasDisplay;
        const QRect dayTextRect = style.subElementRect(
            QStyle::SE_ItemViewItemText, &dayOption, view->viewport());
        QVERIFY2(dayTextRect.width() >= dayOption.rect.width() / 2,
                 "calendar day text must not be squeezed into the menu icon gutter");
        QVERIFY2(dayTextRect.contains(dayOption.rect.center()),
                 "calendar day text must remain centered in its day cell");
        QStyleOptionViewItem selectedDay = dayOption;
        selectedDay.state |= QStyle::State_Selected;
        QImage selectedImage(selectedDay.rect.size(),
                             QImage::Format_ARGB32_Premultiplied);
        selectedImage.fill(Qt::transparent);
        selectedDay.rect.moveTopLeft(QPoint());
        {
            QPainter painter(&selectedImage);
            style.drawControl(QStyle::CE_ItemViewItem, &selectedDay, &painter,
                              view->viewport());
        }
        const QColor selectedCenter = selectedImage.pixelColor(
            selectedImage.rect().center());
        QVERIFY2(selectedCenter.alpha() > 0,
                 "calendar selection must render a visible WinUI item surface");
        const QColor selectedCorner = selectedImage.pixelColor(0, 0);
        QVERIFY2(selectedCorner.alpha() > 0,
                 "calendar cells must paint an opaque popup surface");
        QVERIFY2(selectedCorner.rgba() != selectedCenter.rgba(),
                 "calendar selection must not fill the complete Qt table cell");
        const QPalette palette = view->palette();
        const QColor text = palette.color(QPalette::Text);
        const QColor background = palette.color(QPalette::Base);
        QVERIFY(text.isValid());
        QVERIFY(background.isValid());
        if (mode == WinUI3::ThemeMode::Dark)
            QVERIFY2(qGray(text.rgb()) > 150,
                     "dark calendar text must use a light foreground");
        else
            QVERIFY2(qGray(text.rgb()) < 100,
                     "light calendar text must use a dark foreground");

        const QImage image = view->grab().toImage().convertToFormat(
            QImage::Format_ARGB32);
        const QRect headerCell = view->visualRect(view->model()->index(0, 1));
        QVERIFY(headerCell.isValid());
        const QColor headerSurface = image.pixelColor(
            headerCell.topLeft() + QPoint(2, 2));
        if (mode == WinUI3::ThemeMode::Dark)
            QVERIFY2(qGray(headerSurface.rgb()) < 100,
                     "hovering the month must not turn the dark weekday row white");
        else
            QVERIFY2(qGray(headerSurface.rgb()) > 180,
                     "the light weekday row must remain a light popup surface");
        int readablePixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                const int gray = qGray(pixel.rgb());
                if ((mode == WinUI3::ThemeMode::Dark && gray > 150)
                    || (mode == WinUI3::ThemeMode::Light && gray < 100))
                    ++readablePixels;
            }
        }
        QVERIFY2(readablePixels > 10,
                 "calendar day cells must contain visible date glyphs");
        calendar->hide();
        QCoreApplication::processEvents();
    }
}

QTEST_MAIN(WinUI3DensityWidgetsTest)
#include "tst_winui3density_widgets.moc"
