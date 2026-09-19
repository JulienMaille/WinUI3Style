#include <winui3style/navigationview.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "../src/winui3backdrop_p.h"
#include "../src/winui3density_p.h"
#include "winui3testhelpers.h"

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QFormLayout>
#include <QRadioButton>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
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
#include <QStyleOptionViewItem>
#include <QTabWidget>
#include <QTableView>
#include <QTest>
#include <QTimeEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>

namespace {

struct OfficialCompactWidgets
{
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
                QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta") }, &autoSuggestBox));

        comboBox.addItems({ QStringLiteral("First item"), QStringLiteral("Second item") });
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
    void autoSuggestPopupRebasesPaletteAndHasNoSelectionGlyph();
    void autoSuggestKeyboardSelectsCurrentRow();
    void hiddenCompleterPopupFollowsDensitySwitch();
    void suggestersCompleteAndActivateInBothDensities();
    void runtimeThemeChangeRefreshesOpenComboPopup();
    void openComboPopupFollowsDensitySwitch();
    void runtimeThemeChangeRefreshesOpenCompleterPopup();
    void compactNumberBoxAndUnlistedControlGeometry();
    void menuDensityPreservesPopupGeometry_data();
    void menuDensityPreservesPopupGeometry();
    void inheritedCompactNumberBoxMatchesTextBoxHeight();
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

    const QList<QWidget *> officialWidgets = { &widgets.textBox,        &widgets.passwordBox,
                                               &widgets.autoSuggestBox, &widgets.comboBox,
                                               &widgets.datePicker,     &widgets.timePicker,
                                               &widgets.listView,       &widgets.treeView,
                                               &widgets.navigationView };
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
    const int menuActionStandard =
            widgets.menuBar.actionGeometry(widgets.menuBar.actions().constFirst()).height();
    QVERIFY(menuBarStandard >= 36);
    QVERIFY(menuActionStandard >= 32);

    // This is the exact Designer/gallery path: writing the public Q_PROPERTY
    // must invoke the setter and invalidate already-visible widgets.
    QVERIFY(style.setProperty("densityMode", int(WinUI3::DensityMode::Compact)));
    QCoreApplication::processEvents();
    layout.activate();
    QCoreApplication::processEvents();

    const QList<QPair<int, int>> editorHeights = {
        { widgets.textBox.sizeHint().height(), textStandard },
        { widgets.passwordBox.sizeHint().height(), passwordStandard },
        { widgets.autoSuggestBox.sizeHint().height(), autoSuggestStandard },
        { widgets.comboBox.sizeHint().height(), comboStandard },
        { widgets.datePicker.sizeHint().height(), dateStandard },
        { widgets.timePicker.sizeHint().height(), timeStandard }
    };
    for (const auto &[compact, standard] : editorHeights) {
        QCOMPARE(compact, 24);
        QVERIFY(compact < standard);
    }
    QCOMPARE(rowHeight(widgets.listView), 32);
    QCOMPARE(rowHeight(widgets.treeView), 24);
    QCOMPARE(widgets.navigationView.navigationList()->sizeHintForRow(0), 32);
    const int menuBarCompact = widgets.menuBar.sizeHint().height();
    const int menuActionCompact =
            widgets.menuBar.actionGeometry(widgets.menuBar.actions().constFirst()).height();
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
            QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta") }, &autoSuggestBox));
    QComboBox comboBox(&panel);
    QDateEdit datePicker(&panel);
    QTimeEdit timePicker(&panel);
    QCheckBox checkBox(QStringLiteral("Option"), &panel);
    QRadioButton radioButton(QStringLiteral("Choice"), &panel);
    QListView listView(&panel);
    QStandardItemModel listModel(1, 1, &listView);
    listModel.setData(listModel.index(0, 0), QStringLiteral("Item"));
    listView.setModel(&listModel);
    applyStyleToWidgets(style,
                        QList<QWidget *>{ &panel, &textBox, &autoSuggestBox, &comboBox, &datePicker,
                                          &timePicker, &checkBox, &radioButton, &listView });
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
    for (int height : { textCompact, autoSuggestCompact, comboCompact, dateCompact, timeCompact,
                        checkCompact, radioCompact })
        QCOMPARE(height, 24);
    QCOMPARE(rowHeight(listView), 32);
    autoSuggestBox.completer()->setCompletionPrefix(QStringLiteral("A"));
    autoSuggestBox.completer()->complete();
    QCoreApplication::processEvents();
    QCOMPARE(rowHeight(*autoSuggestBox.completer()->popup()), 32);
    autoSuggestBox.completer()->popup()->hide();

    autoSuggestBox.setCompleter(new QCompleter(
            QStringList{ QStringLiteral("Alpha"), QStringLiteral("Alpine") }, &autoSuggestBox));
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
    QVERIFY(!autoSuggestBox.completer()
                     ->popup()
                     ->property(WinUI3::Style::DensityProperty)
                     .isValid());
    autoSuggestBox.setStyle(&style);
}

void WinUI3DensityWidgetsTest::autoSuggestMatchesCaseInsensitiveSubstrings()
{
    QLineEdit editor;
    auto *completer = new QCompleter(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                                  QStringLiteral("Settings") },
                                     &editor);
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

void WinUI3DensityWidgetsTest::autoSuggestPopupRebasesPaletteAndHasNoSelectionGlyph()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QLineEdit editor;
    auto *completer =
            new QCompleter(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                        QStringLiteral("Gamma"), QStringLiteral("Delta") },
                           &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(300, editor.sizeHint().height());
    editor.show();
    editor.setFocus();
    QTest::keyClicks(&editor, QStringLiteral("a"));
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    completer->popup()->hide();

    style.setThemeMode(WinUI3::ThemeMode::Light);
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QCoreApplication::processEvents();
    auto *popup = completer->popup();
    const QPalette palette = popup->viewport()->palette();
    QVERIFY(palette.color(QPalette::Base).lightness() > 180);
    QVERIFY(palette.color(QPalette::Text).lightness() < 100);
    // Granted acrylic popups carry the translucent recipe (no auto-fill;
    // Qt leaves the erase to the style's backdrop branches); refused and
    // offscreen popups keep the opaque auto-filled surface. Gate on the
    // published effective state, never on pixels alone.
    if (WinUI3::Private::backdropEffectiveSurface(popup)
        == WinUI3::Private::BackdropSurface::Composited)
        QVERIFY(!popup->viewport()->autoFillBackground());
    else
        QVERIFY(popup->viewport()->autoFillBackground());
    QVERIFY(!popup->viewport()->testAttribute(Qt::WA_OpaquePaintEvent));

    popup->setCurrentIndex(popup->model()->index(0, 0));
    QCoreApplication::processEvents();
    const QRect row = popup->visualRect(popup->model()->index(0, 0));
    QStyleOptionViewItem selected;
    selected.initFrom(popup->viewport());
    selected.widget = popup->viewport();
    selected.rect = QRect(0, 0, popup->viewport()->width(), row.height());
    selected.index = popup->model()->index(0, 0);
    selected.features = QStyleOptionViewItem::HasDisplay;
    selected.state = QStyle::State_Enabled | QStyle::State_Selected;
    // Render an empty selected suggestion so text bearings cannot be mistaken
    // for the menu-style check glyph that AutoSuggest must not display.
    QImage image(selected.rect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(palette.color(QPalette::Base));
    {
        QPainter painter(&image);
        popup->style()->drawControl(QStyle::CE_ItemViewItem, &selected, &painter,
                                    popup->viewport());
    }
    int contrastingPixelsInLeadingGlyphZone = 0;
    const QRect glyphZone(10, selected.rect.center().y() - 5, 20, 11);
    const QColor rowBackground = image.pixelColor(7, selected.rect.center().y());
    for (int y = glyphZone.top(); y <= glyphZone.bottom(); ++y)
        for (int x = glyphZone.left(); x <= glyphZone.right(); ++x)
            if (image.rect().contains(x, y) && image.pixelColor(x, y).alpha() > 128
                && qAbs(image.pixelColor(x, y).red() - rowBackground.red())
                                + qAbs(image.pixelColor(x, y).green() - rowBackground.green())
                                + qAbs(image.pixelColor(x, y).blue() - rowBackground.blue())
                        > 30) {
                ++contrastingPixelsInLeadingGlyphZone;
            }
    QCOMPARE(contrastingPixelsInLeadingGlyphZone, 0);

    const QImage firstFrame = popup->viewport()->grab().toImage();
    for (int frame = 0; frame < 6; ++frame) {
        popup->viewport()->repaint();
        QCoreApplication::processEvents();
    }
    QCOMPARE(popup->viewport()->grab().toImage(), firstFrame);

    // Shared pill contract: the hovered suggestion paints the same inset
    // 4,2 pill with the same 3px rounding as a menu row (see
    // paintPopupRowPill). Same fill token, same geometry, no check glyph.
    auto *suggestView = qobject_cast<QAbstractItemView *>(popup);
    QVERIFY(suggestView);
    const QModelIndex hoverIndex = popup->model()->index(1, 0);
    QRect hoverRect = popup->visualRect(hoverIndex);
    QVERIFY(hoverRect.isValid());
    QStyleOptionViewItem hovered;
    hovered.initFrom(popup->viewport());
    hovered.widget = popup->viewport();
    hovered.rect = QRect(0, 0, popup->viewport()->width(), hoverRect.height());
    hovered.index = hoverIndex;
    hovered.features = QStyleOptionViewItem::HasDisplay;
    hovered.text = QStringLiteral("Beta");
    hovered.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    QImage hoveredImage(hovered.rect.size(), QImage::Format_ARGB32_Premultiplied);
    hoveredImage.fill(palette.color(QPalette::Base));
    {
        QPainter painter(&hoveredImage);
        popup->style()->drawControl(QStyle::CE_ItemViewItem, &hovered, &painter, popup->viewport());
    }
    const QRect pill(hovered.rect.left() + 4, hovered.rect.top() + 2, hovered.rect.width() - 8,
                     hovered.rect.height() - 4);
    const QColor pillCenter = hoveredImage.pixelColor(pill.center());
    QVERIFY2(colorDistance(pillCenter, palette.color(QPalette::Base)) > 8,
             "hovered suggestion must paint the shared hover pill");
    const QColor pillEdge = hoveredImage.pixelColor(pill.left() - 2, pill.center().y());
    // Grabbed-pixel convention in this file: near-equality, not bitwise
    // QCOMPARE (backing-store round-trips and faint antialiasing can differ
    // below display precision). A real boxed spill still fails by distance.
    QVERIFY2(colorDistance(pillEdge, palette.color(QPalette::Base)) <= 3,
             qPrintable(QStringLiteral("pill edge #%1 vs Base #%2")
                                .arg(pillEdge.name(QColor::HexArgb),
                                     palette.color(QPalette::Base).name(QColor::HexArgb))));
    // Top edge of the 3px rounding: the corner pixel stays Base, the pixel
    // two rows in carries the pill. Same shape a menu row paints. Near-
    // equality per the grabbed-pixel convention above.
    QVERIFY2(colorDistance(hoveredImage.pixelColor(pill.left(), pill.top()),
                           palette.color(QPalette::Base))
                     <= 3,
             "pill corner must stay Base");
    QVERIFY2(colorDistance(hoveredImage.pixelColor(pill.left() + 2, pill.top() + 2),
                           palette.color(QPalette::Base))
                     > 8,
             "hovered suggestion must round the shared pill like a menu row");
}

void WinUI3DensityWidgetsTest::autoSuggestKeyboardSelectsCurrentRow()
{
    // QCompleter activation contract: the activated signal carries the
    // selected row's text to the editor. Same state, row plus text.
    QLineEdit editor;
    auto *completer = new QCompleter(
            QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") },
            &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(280, 36);
    editor.show();
    editor.setFocus();
    QTest::keyClicks(&editor, QStringLiteral("a"));
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QCOMPARE(completer->currentRow(), 0);
    QObject::connect(completer, QOverload<const QString &>::of(&QCompleter::activated), &editor,
                     [&](const QString &text) { editor.setText(text); });
    completer->setCurrentRow(1);
    QCOMPARE(completer->currentRow(), 1);
    Q_EMIT completer->activated(QStringLiteral("Beta"));
    QTRY_COMPARE(editor.text(), QStringLiteral("Beta"));
}

void WinUI3DensityWidgetsTest::hiddenCompleterPopupFollowsDensitySwitch()
{
    // Global density switch must reach hidden completer popups: the popup
    // carries a stale winuiDensity plus the private delegate caches its
    // sizeHint, so reopening without a re-sync keeps the old rows.
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setDensityMode(WinUI3::DensityMode::Standard);
    QLineEdit editor;
    auto *completer =
            new QCompleter(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                        QStringLiteral("Gamma"), QStringLiteral("Delta") },
                           &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(280, 32);
    editor.show();
    editor.setFocus();
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QCOMPARE(WinUI3::Style::densityMode(completer->popup()), WinUI3::DensityMode::Standard);
    QCOMPARE(rowHeight(*completer->popup()), 40);
    completer->popup()->hide();
    QTRY_VERIFY(!completer->popup()->isVisible());

    // Hidden switch: rows must follow without reopening first.
    style.setDensityMode(WinUI3::DensityMode::Compact);
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Style::densityMode(completer->popup()), WinUI3::DensityMode::Compact);
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QCOMPARE(WinUI3::Style::densityMode(completer->popup()), WinUI3::DensityMode::Compact);
    QCOMPARE(rowHeight(*completer->popup()), 32);
    completer->popup()->hide();

    style.setDensityMode(WinUI3::DensityMode::Standard);
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Style::densityMode(completer->popup()), WinUI3::DensityMode::Standard);
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QCOMPARE(rowHeight(*completer->popup()), 40);
    completer->popup()->hide();
}

void WinUI3DensityWidgetsTest::suggestersCompleteAndActivateInBothDensities()
{
    // All three gallery suggesters share one wiring contract: case
    // insensitive, contains match, popup completion, then activation
    // carries the current row text to the editor.
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    for (const WinUI3::DensityMode mode :
         { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact }) {
        style.setDensityMode(mode);
        const int expectedRow = mode == WinUI3::DensityMode::Compact ? 32 : 40;
        for (int suggester = 0; suggester < 3; ++suggester) {
            QLineEdit editor;
            auto *completer = new QCompleter(
                    QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                 QStringLiteral("Gamma"), QStringLiteral("Delta"),
                                 QStringLiteral("Settings"), QStringLiteral("Controls") },
                    &editor);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            completer->setFilterMode(Qt::MatchContains);
            completer->setCompletionMode(QCompleter::PopupCompletion);
            editor.setCompleter(completer);
            editor.resize(280, expectedRow == 32 ? 24 : 32);
            editor.show();
            editor.setFocus();
            QCOMPARE(completer->caseSensitivity(), Qt::CaseInsensitive);
            QCOMPARE(completer->filterMode(), Qt::MatchContains);
            QCOMPARE(completer->completionMode(), QCompleter::PopupCompletion);
            QTest::keyClicks(&editor, QStringLiteral("ET"));
            QTRY_COMPARE(completer->completionCount(), 2);
            QTRY_VERIFY(completer->popup()->isVisible());
            QCOMPARE(WinUI3::Style::densityMode(completer->popup()), mode);
            QCOMPARE(rowHeight(*completer->popup()), expectedRow);
            // Geometry assert pairs the activation pixels: rows follow the
            // active profile through the same CT_ItemViewItem contract as
            // menu/combo rows (menuItemHeightInComboBox family).
            QStyleOptionViewItem rowOption;
            rowOption.initFrom(completer->popup()->viewport());
            rowOption.index = completer->popup()->model()->index(0, 0);
            QCOMPARE(editor.style()
                             ->sizeFromContents(QStyle::CT_ItemViewItem, &rowOption, QSize(),
                                                completer->popup())
                             .height(),
                     expectedRow);
            QCOMPARE(completer->currentRow(), 0);
            QObject::connect(completer, QOverload<const QString &>::of(&QCompleter::activated),
                             &editor, [&](const QString &text) { editor.setText(text); });
            completer->setCurrentRow(1);
            QCOMPARE(completer->currentRow(), 1);
            QCOMPARE(completer->currentCompletion(), QStringLiteral("Settings"));
            Q_EMIT completer->activated(completer->currentCompletion());
            QTRY_COMPARE(editor.text(), QStringLiteral("Settings"));
            completer->popup()->hide();
            QCoreApplication::processEvents();
        }
    }
    style.setDensityMode(WinUI3::DensityMode::Standard);
}

void WinUI3DensityWidgetsTest::runtimeThemeChangeRefreshesOpenComboPopup()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QWidget root;
    root.setStyle(&style);
    QComboBox combo(&root);
    combo.addItems({ QStringLiteral("First item"), QStringLiteral("Second item"),
                     QStringLiteral("Third item") });
    combo.setCurrentIndex(1);
    root.resize(420, 240);
    combo.setGeometry(40, 40, 220, 36);
    root.show();
    QCoreApplication::processEvents();

    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QWidget *popup = combo.view()->window();
    QVERIFY(popup);
    QVERIFY(popup->isVisible());
    const QSize initialSize = popup->size();
    const QColor darkBase = combo.view()->viewport()->palette().color(QPalette::Base);
    QVERIFY(qGray(darkBase.rgb()) < 128);

    style.setThemeMode(WinUI3::ThemeMode::Light);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    const QColor lightBase = combo.view()->viewport()->palette().color(QPalette::Base);
    QVERIFY2(qGray(lightBase.rgb()) > 180,
             "an open ComboBox popup must rebase its surface immediately");
    QVERIFY(qGray(combo.view()->viewport()->palette().color(QPalette::Text).rgb()) < 100);
    // Theme changes switch fonts/metrics, so the selected-row anchor is
    // legitimately recentered (a few px); only the size must be stable.
    QCOMPARE(popup->size(), initialSize);

    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    QVERIFY(qGray(combo.view()->viewport()->palette().color(QPalette::Base).rgb()) < 128);
    QCOMPARE(popup->size(), initialSize);

    style.setThemeMode(WinUI3::ThemeMode::System);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    const bool systemDark = qGray(qApp->palette().color(QPalette::Window).rgb()) < 128;
    const int systemPopupLightness =
            qGray(combo.view()->viewport()->palette().color(QPalette::Base).rgb());
    if (systemDark)
        QVERIFY(systemPopupLightness < 128);
    else
        QVERIFY(systemPopupLightness > 180);
    QCOMPARE(popup->size(), initialSize);
    combo.hidePopup();
    QCoreApplication::processEvents();
}

void WinUI3DensityWidgetsTest::openComboPopupFollowsDensitySwitch()
{
    // Popup open in Standard, switch to Compact: rows go 40 -> 32 and the
    // popup repositions. Same state, two assertions: row height and popup
    // geometry both follow density.
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setDensityMode(WinUI3::DensityMode::Standard);
    QWidget root;
    root.setStyle(&style);
    QComboBox combo(&root);
    combo.addItems({ QStringLiteral("First item"), QStringLiteral("Second item"),
                     QStringLiteral("Third item") });
    combo.setCurrentIndex(1);
    root.resize(420, 240);
    combo.setGeometry(40, 40, 220, 36);
    root.show();
    QCoreApplication::processEvents();
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QWidget *popup = combo.view()->window();
    QVERIFY(popup && popup->isVisible());
    QCOMPARE(rowHeight(*combo.view()), 40);
    const QRect standardPopup = popup->geometry();
    style.setDensityMode(WinUI3::DensityMode::Compact);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    QTRY_COMPARE(rowHeight(*combo.view()), 32);
    QVERIFY(popup->geometry() != standardPopup);
    combo.hidePopup();
    QCoreApplication::processEvents();
}

void WinUI3DensityWidgetsTest::runtimeThemeChangeRefreshesOpenCompleterPopup()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QLineEdit editor;
    auto *completer = new QCompleter(
            QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") },
            &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(280, 36);
    editor.show();
    editor.setFocus();
    QTest::keyClicks(&editor, QStringLiteral("a"));
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QWidget *popup = completer->popup();
    QVERIFY(popup);
    auto *popupView = qobject_cast<QAbstractItemView *>(popup);
    QVERIFY(popupView);
    const QRect initialGeometry = popup->geometry();
    QVERIFY(qGray(popupView->viewport()->palette().color(QPalette::Base).rgb()) < 128);

    style.setThemeMode(WinUI3::ThemeMode::Light);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    QVERIFY(qGray(popupView->viewport()->palette().color(QPalette::Base).rgb()) > 180);
    QVERIFY(qGray(popupView->viewport()->palette().color(QPalette::Text).rgb()) < 100);
    QCOMPARE(popup->geometry(), initialGeometry);

    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    QVERIFY(qGray(popupView->viewport()->palette().color(QPalette::Base).rgb()) < 128);
    QCOMPARE(popup->geometry(), initialGeometry);
    style.setThemeMode(WinUI3::ThemeMode::System);
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    const bool systemDark = qGray(qApp->palette().color(QPalette::Window).rgb()) < 128;
    const int systemPopupLightness =
            qGray(popupView->viewport()->palette().color(QPalette::Base).rgb());
    if (systemDark)
        QVERIFY(systemPopupLightness < 128);
    else
        QVERIFY(systemPopupLightness > 180);
    QCOMPARE(popup->geometry(), initialGeometry);
    popup->hide();
}

void WinUI3DensityWidgetsTest::compactNumberBoxAndUnlistedControlGeometry()
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

    applyStyleToWidgets(style,
                        QList<QWidget *>{ &button, &spinBox, &slider, &tabs, &table, &menu });

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
    QCOMPARE(menuItemStandard, 36);

    style.setDensityMode(WinUI3::DensityMode::Compact);
    QCoreApplication::processEvents();
    layout.activate();
    QCoreApplication::processEvents();

    QCOMPARE(button.sizeHint(), buttonStandard);
    QVERIFY(spinStandard.height() > 24);
    QCOMPARE(spinBox.sizeHint().height(), 24);
    QCOMPARE(spinBox.sizeHint().width(), spinStandard.width());
    QCOMPARE(slider.sizeHint(), sliderStandard);
    QCOMPARE(tabs.sizeHint(), tabsStandard);
    QCOMPARE(table.rowHeight(0), tableRowStandard);
    QCOMPARE(table.horizontalHeader()->sectionSize(0), headerStandard);
    QCOMPARE(menu.actionGeometry(&menuAction).height(), 32);
}

void WinUI3DensityWidgetsTest::menuDensityPreservesPopupGeometry_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("initialCompact");
    QTest::newRow("light-standard") << false << false;
    QTest::newRow("light-compact") << false << true;
    QTest::newRow("dark-standard") << true << false;
    QTest::newRow("dark-compact") << true << true;
}

void WinUI3DensityWidgetsTest::menuDensityPreservesPopupGeometry()
{
    QFETCH(bool, dark);
    QFETCH(bool, initialCompact);
    DisableAnimationsGuard animations;
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    const auto initial =
            initialCompact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard;
    const auto other =
            initialCompact ? WinUI3::DensityMode::Standard : WinUI3::DensityMode::Compact;
    style.setDensityMode(initial);
    QMenu menu;
    menu.setStyle(&style);
    QAction *action = menu.addAction(
            QStringLiteral("A deliberately long menu action label for density testing"));

    menu.popup(QPoint(40, 40));
    QTRY_VERIFY(menu.isVisible());
    const QRect initialAction = menu.actionGeometry(action);
    const QRect initialPopup = menu.geometry();
    const auto &metrics = WinUI3::Private::densityMetrics(initial);
    QCOMPARE(metrics.menuItemHorizontalPadding, 8);
    QCOMPARE(initialAction.height(), initial == WinUI3::DensityMode::Compact ? 32 : 36);
    QCOMPARE(initialAction.width(),
             42 + QFontMetrics(menu.font()).horizontalAdvance(action->text()) + 16);
    // Extension: Compact menu-popup rows shrink 36 -> 32 like list rows
    // (Microsoft ships MenuBar-only Compact). A hidden switch/reopen
    // follows the active profile; a visible popup keeps Qt's layout.
    for (const auto mode : { other, initial }) {
        style.setDensityMode(mode);
        const int expectedHeight = mode == WinUI3::DensityMode::Compact ? 32 : 36;
        QTRY_COMPARE_WITH_TIMEOUT(menu.actionGeometry(action).height(), expectedHeight, 1000);
        QTest::qWait(60);
        QCOMPARE(menu.actionAt(QPoint(menu.actionGeometry(action).center().x(),
                                      menu.actionGeometry(action).center().y())),
                 action);
        menu.hide();
        style.setDensityMode(mode == initial ? other : initial);
        menu.popup(initialPopup.topLeft());
        QTRY_VERIFY(menu.isVisible());
        const int reopenedHeight =
                (mode == initial ? other : initial) == WinUI3::DensityMode::Compact ? 32 : 36;
        QCOMPARE(menu.actionGeometry(action).height(), reopenedHeight);
    }
    menu.hide();
}

void WinUI3DensityWidgetsTest::inheritedCompactNumberBoxMatchesTextBoxHeight()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    QWidget panel;
    panel.setStyle(&style);
    QFormLayout layout(&panel);
    QLabel title(QStringLiteral("Compact (24 px editors)"), &panel);
    QLineEdit textBox(&panel);
    QComboBox comboBox(&panel);
    QDateEdit datePicker(&panel);
    QCheckBox checkBox(QStringLiteral("Checked"), &panel);
    QRadioButton radioButton(QStringLiteral("Selected"), &panel);
    QSpinBox numberBox(&panel);
    layout.addRow(&title);
    layout.addRow(QStringLiteral("TextBox"), &textBox);
    layout.addRow(QStringLiteral("ComboBox"), &comboBox);
    layout.addRow(QStringLiteral("DatePicker"), &datePicker);
    layout.addRow(QStringLiteral("CheckBox"), &checkBox);
    layout.addRow(QStringLiteral("RadioButton"), &radioButton);
    layout.addRow(QStringLiteral("NumberBox"), &numberBox);
    // uic assigns dynamic properties during retranslateUi(), after the child
    // controls and their layout items have already queried Standard metrics.
    // Reproduce that ordering so a stale QAbstractSpinBox size cache cannot
    // hide behind the simpler property-before-construction test path.
    QVERIFY(numberBox.sizeHint().height() > 24);
    panel.setProperty(WinUI3::Style::DensityProperty, QStringLiteral("compact"));
    panel.resize(400, 264);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel, 1000) || panel.isVisible());
    layout.activate();
    QCoreApplication::processEvents();

    QCOMPARE(WinUI3::Style::densityMode(&numberBox), WinUI3::DensityMode::Compact);
    QCOMPARE(numberBox.sizeHint().height(), textBox.sizeHint().height());
    QCOMPARE(numberBox.height(), textBox.height());
    QCOMPARE(numberBox.height(), 24);
}

QTEST_MAIN(WinUI3DensityWidgetsTest)
#include "tst_winui3density_widgets.moc"
