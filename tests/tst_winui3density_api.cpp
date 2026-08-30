#include <winui3style/winui3style.h>
#include <winui3style/navigationview.h>

#include <QApplication>
#include <QCoreApplication>
#include <QMetaProperty>
#include <QComboBox>
#include <QDateEdit>
#include <QHeaderView>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalSpy>
#include <QStyleOption>
#include <QStyleFactory>
#include <QTableView>
#include <QTest>
#include <QTreeView>
#include <QVBoxLayout>

class DensityApiLayoutProbe final : public QWidget
{
public:
    using QWidget::QWidget;
    int layoutRequests = 0;

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::LayoutRequest)
            ++layoutRequests;
        return QWidget::event(event);
    }
};

class WinUI3DensityApiTest final : public QObject
{
    Q_OBJECT

private slots:
    void publicPropertyAndConstructors();
    void inheritedLocalProperty();
    void runtimeInvalidatesLayout();
    void geometryContractsAndInvariants();
    void navigationViewRelayoutsAtRuntime();
    void pluginAliases();
};

void WinUI3DensityApiTest::publicPropertyAndConstructors()
{
    WinUI3::Style style(WinUI3::ThemeMode::Light,
                        WinUI3::DensityMode::Compact);
    QCOMPARE(style.themeMode(), WinUI3::ThemeMode::Light);
    QCOMPARE(style.densityMode(), WinUI3::DensityMode::Compact);

    WinUI3::Style densityOnly(WinUI3::DensityMode::Compact);
    QCOMPARE(densityOnly.densityMode(), WinUI3::DensityMode::Compact);

    const int propertyIndex = style.metaObject()->indexOfProperty("densityMode");
    QVERIFY(propertyIndex >= 0);
    const QMetaProperty property = style.metaObject()->property(propertyIndex);
    QVERIFY(property.hasNotifySignal());

    QSignalSpy changed(&style, &WinUI3::Style::densityChanged);
    style.setDensityMode(WinUI3::DensityMode::Standard);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.at(0).at(0).value<WinUI3::DensityMode>(),
             WinUI3::DensityMode::Standard);
}

void WinUI3DensityApiTest::inheritedLocalProperty()
{
    WinUI3::Style style(WinUI3::DensityMode::Standard);
    QWidget root;
    QWidget panel(&root);
    QPushButton button(&panel);
    root.setStyle(&style);

    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Standard);
    root.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("dense"));
    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Compact);
    root.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("normal"));
    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Standard);
    root.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("compact"));
    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Compact);
    QCOMPARE(WinUI3::Style::densityMode(&button), WinUI3::DensityMode::Compact);

    WinUI3::Style::setDensityMode(&panel, WinUI3::DensityMode::Standard);
    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Standard);
    WinUI3::Style::clearDensityMode(&panel);
    QCOMPARE(style.effectiveDensityMode(&button), WinUI3::DensityMode::Compact);
}

void WinUI3DensityApiTest::runtimeInvalidatesLayout()
{
    WinUI3::Style style;
    DensityApiLayoutProbe root;
    QVBoxLayout layout(&root);
    QPushButton button;
    layout.addWidget(&button);
    root.setStyle(&style);
    root.show();
    QCoreApplication::processEvents();
    root.layoutRequests = 0;

    style.setDensityMode(WinUI3::DensityMode::Compact);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
    QVERIFY(root.layoutRequests > 0);

    root.layoutRequests = 0;
    root.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("standard"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
    QVERIFY(root.layoutRequests > 0);
}

void WinUI3DensityApiTest::geometryContractsAndInvariants()
{
    WinUI3::Style style;
    QWidget root;
    root.setStyle(&style);

    QLineEdit line(&root);
    QComboBox combo(&root);
    QPushButton button(QStringLiteral("Button"), &root);
    QSpinBox spin(&root);
    QDateEdit date(&root);
    QMenuBar menuBar(&root);
    QMenu menu(&root);
    QListView list(&root);
    QTreeView tree(&root);
    QTableView table(&root);

    QStyleOptionFrame frame;
    frame.initFrom(&line);
    QStyleOptionComboBox comboOption;
    comboOption.initFrom(&combo);
    QStyleOptionButton buttonOption;
    buttonOption.initFrom(&button);
    QStyleOptionSpinBox spinOption;
    spinOption.initFrom(&spin);
    QStyleOptionSpinBox dateOption;
    dateOption.initFrom(&date);
    QStyleOptionMenuItem menuOption;
    menuOption.initFrom(&menu);
    menuOption.menuItemType = QStyleOptionMenuItem::Normal;
    menuOption.text = QStringLiteral("Menu item");
    QStyleOption generic;
    generic.initFrom(&root);

    const QSize content(64, 16);
    const QSize standardLine = style.sizeFromContents(
        QStyle::CT_LineEdit, &frame, content, &line);
    const QSize standardCombo = style.sizeFromContents(
        QStyle::CT_ComboBox, &comboOption, content, &combo);
    const QSize standardMenuBar = style.sizeFromContents(
        QStyle::CT_MenuBarItem, &generic, content, &menuBar);
    const QSize standardList = style.sizeFromContents(
        QStyle::CT_ItemViewItem, &generic, content, list.viewport());
    const QSize standardTree = style.sizeFromContents(
        QStyle::CT_ItemViewItem, &generic, content, tree.viewport());

    const QSize invariantButton = style.sizeFromContents(
        QStyle::CT_PushButton, &buttonOption, content, &button);
    const QSize invariantSpin = style.sizeFromContents(
        QStyle::CT_SpinBox, &spinOption, content, &spin);
    const QSize standardDate = style.sizeFromContents(
        QStyle::CT_SpinBox, &dateOption, content, &date);
    const QSize invariantMenu = style.sizeFromContents(
        QStyle::CT_MenuItem, &menuOption, content, &menu);
    const QSize invariantTable = style.sizeFromContents(
        QStyle::CT_ItemViewItem, &generic, content, table.viewport());
    const QSize invariantHeader = style.sizeFromContents(
        QStyle::CT_HeaderSection, &generic, content, table.horizontalHeader());
    const int invariantButtonMargin = style.pixelMetric(
        QStyle::PM_ButtonMargin, nullptr, &button);
    const int invariantHeaderDefault = style.pixelMetric(
        QStyle::PM_HeaderDefaultSectionSizeVertical, nullptr,
        table.horizontalHeader());

    root.setProperty(WinUI3::Style::DensityProperty,
                     QStringLiteral("compact"));

    QCOMPARE(standardLine.height(), 32);
    QCOMPARE(style.sizeFromContents(QStyle::CT_LineEdit, &frame, content, &line).height(), 24);
    QCOMPARE(standardCombo.height(), 32);
    QCOMPARE(style.sizeFromContents(QStyle::CT_ComboBox, &comboOption, content, &combo).height(), 24);
    QCOMPARE(standardMenuBar.height(), 32);
    QCOMPARE(style.sizeFromContents(QStyle::CT_MenuBarItem, &generic, content, &menuBar).height(), 24);
    QCOMPARE(standardList.height(), 40);
    QCOMPARE(style.sizeFromContents(QStyle::CT_ItemViewItem, &generic, content, list.viewport()).height(), 32);
    QCOMPARE(standardTree.height(), 28);
    QCOMPARE(style.sizeFromContents(QStyle::CT_ItemViewItem, &generic, content, tree.viewport()).height(), 24);
    QCOMPARE(standardDate.height(), 32);
    QCOMPARE(style.sizeFromContents(QStyle::CT_SpinBox, &dateOption, content,
                                    &date).height(), 24);

    QCOMPARE(style.sizeFromContents(QStyle::CT_PushButton, &buttonOption, content, &button), invariantButton);
    QCOMPARE(style.sizeFromContents(QStyle::CT_SpinBox, &spinOption, content, &spin), invariantSpin);
    QCOMPARE(style.sizeFromContents(QStyle::CT_MenuItem, &menuOption, content, &menu), invariantMenu);
    QCOMPARE(style.sizeFromContents(QStyle::CT_ItemViewItem, &generic, content, table.viewport()), invariantTable);
    QCOMPARE(style.sizeFromContents(QStyle::CT_HeaderSection, &generic, content, table.horizontalHeader()), invariantHeader);
    QCOMPARE(invariantButtonMargin, 8);
    QCOMPARE(style.pixelMetric(QStyle::PM_ButtonMargin, nullptr, &button),
             invariantButtonMargin);
    QCOMPARE(invariantHeaderDefault, 36);
    QCOMPARE(style.pixelMetric(QStyle::PM_HeaderDefaultSectionSizeVertical,
                               nullptr, table.horizontalHeader()),
             invariantHeaderDefault);
}

void WinUI3DensityApiTest::navigationViewRelayoutsAtRuntime()
{
    WinUI3::Style style;
    WinUI3::NavigationView navigation;
    navigation.setStyle(&style);
    navigation.navigationList()->setStyle(&style);
    navigation.addPage(new QWidget, QIcon(), QStringLiteral("Page"));
    navigation.show();
    QCoreApplication::processEvents();
    QCOMPARE(navigation.navigationList()->sizeHintForRow(0), 40);

    navigation.setProperty(WinUI3::Style::DensityProperty,
                           QStringLiteral("compact"));
    QCoreApplication::processEvents();
    QCOMPARE(navigation.navigationList()->sizeHintForRow(0), 32);
}

void WinUI3DensityApiTest::pluginAliases()
{
    const QStringList keys = QStyleFactory::keys();
    if (!keys.contains(QStringLiteral("winui3"), Qt::CaseInsensitive)
        || !keys.contains(QStringLiteral("winui3compact"), Qt::CaseInsensitive)) {
        QSKIP("The optional WinUI3 style plugin is not available");
    }
    QScopedPointer<QStyle> standard(
        QStyleFactory::create(QStringLiteral("winui3")));
    QScopedPointer<QStyle> compact(
        QStyleFactory::create(QStringLiteral("winui3compact")));
    QVERIFY(standard);
    QVERIFY(compact);
    QCOMPARE(qobject_cast<WinUI3::Style *>(standard.data())->densityMode(),
             WinUI3::DensityMode::Standard);
    QCOMPARE(qobject_cast<WinUI3::Style *>(compact.data())->densityMode(),
             WinUI3::DensityMode::Compact);
}

QTEST_MAIN(WinUI3DensityApiTest)
#include "tst_winui3density_api.moc"
