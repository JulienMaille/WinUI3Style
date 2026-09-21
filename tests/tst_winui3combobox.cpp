// SPDX-License-Identifier: LGPL-2.1-or-later
// Domain split of tst_winui3style.cpp (plan step 8): test-function names
// kept identical so the DPI reruns in tests/CMakeLists.txt keep working.
#include <winui3style/animatedstack.h>
#include <winui3style/navigationview.h>
#include <winui3style/settingscard.h>
#include <winui3style/toggleswitch.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>
#include <winui3style/winui3icons.h>

#include "../src/winui3frameproperties_p.h"
#include "../src/winui3helpers_p.h"
#include "../src/winui3tokens_p.h"

#include <QLabel>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAccessible>
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDockWidget>
#include <QFocusEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QGroupBox>
#include <QHeaderView>
#include <QImage>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScrollArea>
#include <QSplitter>
#include <QSignalSpy>
#include <QSizeGrip>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyleFactory>
#include <QStyleOptionGroupBox>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSlider>
#include <QStyleOptionSpinBox>
#include <QStyleOptionTab>
#include <QStyledItemDelegate>
#include <QStyleOptionToolButton>
#include <QSlider>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QToolBar>
#include <QTimer>
#include <QTest>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#include <QtMath>
#include <QStandardItemModel>

#include <cmath>
#include <limits>

#include "winui3testhelpers.h"

class WinUI3ComboBoxTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void themeComboSizingContract();
    void comboPopupContract();
    void comboPopupSelectedItemKeepsIcon();
    void comboReleaseActivationAndMarkerMotion();
    void comboPopupAssociationLifecycle();
    void comboChevronMotion();
    void comboChevronGeometry();
    void comboOpenPopupKeyboardCurrentPaintsHoverPill();
    void comboOpenPopupKeyboardNavMovesHoverPill();
    void autoSuggestHoverPillMatchesMenuPill();
    void autoSuggestCompositedHoverRebuildsRowFrame();
    void autoSuggestResolvedFramePreservesRgb_data();
    void autoSuggestResolvedFramePreservesRgb();
};

void WinUI3ComboBoxTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3ComboBoxTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3ComboBoxTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    if (QWidget *focus = qApp->focusWidget())
        focus->clearFocus();
    qApp->processEvents();
}

void WinUI3ComboBoxTest::themeComboSizingContract()
{
    // Keep this in lockstep with GalleryWindow's command-bar theme selector:
    // the longest item must remain available before the toolbar is shown.
    QComboBox combo;
    combo.addItems(
            { QStringLiteral("System theme"), QStringLiteral("Light"), QStringLiteral("Dark") });
    combo.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo.setMinimumContentsLength(combo.itemText(0).size());
    combo.setMinimumWidth(combo.sizeHint().width());

    const int textWidth = QFontMetrics(combo.font()).horizontalAdvance(combo.itemText(0));
    QVERIFY(combo.minimumWidth() >= textWidth);
    combo.resize(combo.minimumWidth(), 32);
    combo.show();
    QTRY_VERIFY(combo.isVisible());

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.currentText = combo.currentText();
    const QRect edit = combo.style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                     QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY2(edit.width() >= textWidth,
             qPrintable(QStringLiteral("edit width %1 < text width %2")
                                .arg(edit.width())
                                .arg(textWidth)));
    // Closed-text elision guard (user 2026-09-10): a default-width combo
    // showing its longest item must not elide. The edit slot reserves the
    // arrow + left padding, so the minimum width must cover text + chrome.
    const int chrome = combo.minimumWidth() - edit.width();
    QVERIFY2(chrome >= 0, qPrintable(QStringLiteral("negative chrome %1").arg(chrome)));
    QCOMPARE(combo.fontMetrics().elidedText(combo.itemText(0), Qt::ElideRight, edit.width()),
             combo.itemText(0));
    // Default policy path: without AdjustToContents the style minimum
    // (120 px) must still fit short items without elision.
    QComboBox plain;
    plain.addItems({ QStringLiteral("Blue"), QStringLiteral("Green"), QStringLiteral("Red") });
    plain.resize(120, 32);
    plain.show();
    QTRY_VERIFY(plain.isVisible());
    QStyleOptionComboBox plainOption;
    plainOption.initFrom(&plain);
    plainOption.rect = plain.rect();
    plainOption.currentText = plain.currentText();
    const QRect plainEdit = plain.style()->subControlRect(QStyle::CC_ComboBox, &plainOption,
                                                          QStyle::SC_ComboBoxEditField, &plain);
    QCOMPARE(plain.fontMetrics().elidedText(plain.currentText(), Qt::ElideRight, plainEdit.width()),
             plain.currentText());

    combo.setEditable(true);
    QVERIFY(combo.lineEdit());
    for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft }) {
        combo.setLayoutDirection(direction);
        QCoreApplication::processEvents();
        option.initFrom(&combo);
        option.rect = combo.rect();
        option.direction = direction;
        option.editable = true;
        option.currentText = combo.currentText();
        const QRect labelSlot = combo.style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                              QStyle::SC_ComboBoxEditField, &combo);
        QStyleOptionFrame editorOption;
        editorOption.initFrom(combo.lineEdit());
        editorOption.rect = combo.lineEdit()->rect();
        editorOption.direction = direction;
        const QRect editorContents = combo.lineEdit()
                                             ->style()
                                             ->subElementRect(QStyle::SE_LineEditContents,
                                                              &editorOption, combo.lineEdit())
                                             .translated(combo.lineEdit()->pos());
        if (direction == Qt::LeftToRight)
            QCOMPARE(editorContents.left(), labelSlot.left());
        else
            QCOMPARE(editorContents.right(), labelSlot.right());
    }
    // Editable text-alignment guard (TODO 2026-09-10): the private editor
    // widget geometry is Qt-owned (pos.x is framework layout, not the text
    // origin). What matters is the text itself: editor contents must start
    // at the closed-label slot in both directions (already asserted above).
}

void WinUI3ComboBoxTest::comboPopupContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({ QStringLiteral("Blue"), QStringLiteral("Green"), QStringLiteral("Red") });
    combo.resize(200, 32);
    combo.move(120, 180);
    host.show();
    QTRY_VERIFY(host.isVisible());
    QWidget *popup = combo.view()->window();
    QVERIFY(popup);
    PopupGeometryProbe probe;
    probe.combo = &combo;
    probe.popup = popup;
    popup->installEventFilter(&probe);
    combo.view()->viewport()->installEventFilter(&probe);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
    combo.setCurrentIndex(1);
    QSignalSpy firstOpenScrollChanges(combo.view()->verticalScrollBar(), &QScrollBar::valueChanged);
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QTRY_VERIFY(!probe.selectedCenterAtShow.isNull());
    QCOMPARE(popup->geometry(), probe.geometryAtShow);
    const QPoint comboCenterOnFirstOpen = combo.mapToGlobal(combo.rect().center());
    const QRect firstOpenSelectedRect = combo.view()->visualRect(
            combo.model()->index(1, combo.modelColumn(), combo.rootModelIndex()));
    const QPoint firstOpenSelectedCenter =
            combo.view()->viewport()->mapToGlobal(firstOpenSelectedRect.center());
    QVERIFY2(qAbs(probe.selectedCenterAtShow.y() - comboCenterOnFirstOpen.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                                .arg(probe.selectedCenterAtShow.y())
                                .arg(comboCenterOnFirstOpen.y())
                                .arg(popup->x())
                                .arg(popup->y())
                                .arg(popup->width())
                                .arg(popup->height())
                                .arg(firstOpenSelectedRect.x())
                                .arg(firstOpenSelectedRect.y())
                                .arg(firstOpenSelectedRect.width())
                                .arg(firstOpenSelectedRect.height())));
    QCOMPARE(firstOpenSelectedCenter, probe.selectedCenterAtShow);
    QCOMPARE(combo.view()->verticalScrollBar()->value(), probe.scrollValueAtShow);
    QCOMPARE(firstOpenScrollChanges.count(), 0);
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), probe.geometryAtShow);
    QCOMPARE(combo.view()->viewport()->mapToGlobal(
                     combo.view()->visualRect(combo.model()->index(1, 0)).center()),
             probe.selectedCenterAtShow);
    QCOMPARE(combo.view()->verticalScrollBar()->value(), probe.scrollValueAtShow);
    // The open slide holds the popup 12 px off-anchor mid-flight on real
    // platforms (skipped offscreen); geometry must converge, not freeze.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        QCOMPARE(probe.movesAfterShow, 0);
        QCOMPARE(probe.resizesAfterShow, 0);
    }
    QVERIFY(probe.layoutsAfterShow <= 1);
    QVERIFY(combo.view()->palette().color(QPalette::Text).lightness() > 128);
    QVERIFY(combo.view()->palette().color(QPalette::Window).lightness() < 128);
    const QColor runtimeAccent(220, 40, 80);
    style->setAccentColor(runtimeAccent);
    QTRY_COMPARE(combo.view()->palette().color(QPalette::Highlight), runtimeAccent);
    combo.hidePopup();
    combo.setCurrentIndex(0);
    style->setThemeMode(WinUI3::ThemeMode::Light);
    probe.reset();
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QVERIFY(combo.view()->palette().color(QPalette::Text).lightness() < 128);
    QVERIFY(combo.view()->palette().color(QPalette::Window).lightness() > 128);
    const QPoint comboCenter = combo.mapToGlobal(combo.rect().center());
    const QRect firstRow = combo.view()->visualRect(combo.model()->index(0, 0));
    const QPoint selectedCenter = combo.view()->viewport()->mapToGlobal(firstRow.center());
    QVERIFY2(qAbs(selectedCenter.y() - comboCenter.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                                .arg(selectedCenter.y())
                                .arg(comboCenter.y())
                                .arg(popup->x())
                                .arg(popup->y())
                                .arg(popup->width())
                                .arg(popup->height())
                                .arg(firstRow.x())
                                .arg(firstRow.y())
                                .arg(firstRow.width())
                                .arg(firstRow.height())));
    // A direct QComboBox::showPopup() can require one synchronous correction while
    // QEvent::Show is being dispatched.  That happens before the popup is composed;
    // only geometry changes after the completed Show event can produce a visible jump.
    QVERIFY(probe.movesAfterShow <= 1);
    // Offscreen: no open slide, geometry must freeze at Show.
    // Live: the slide converges within 167 ms; settle first, then freeze.
    const QRect lightGeometry = popup->geometry();
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        QTest::qWait(60);
        QCOMPARE(popup->geometry(), lightGeometry);
        QCOMPARE(probe.movesAfterShow, 0);
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(popup->y() - lightGeometry.y()) <= 12, 1000);
        QTest::qWait(300);
        QCOMPARE(popup->geometry().width(), lightGeometry.width());
        QCOMPARE(popup->geometry().height(), lightGeometry.height());
        probe.movesAfterShow = 0;
    }
    QCOMPARE(probe.resizesAfterShow, 0);
    QVERIFY(probe.layoutsAfterShow <= 1);

    const QImage lightPopup = combo.view()->viewport()->grab().toImage();
    const QRect secondRow = combo.view()->visualRect(combo.model()->index(1, 0));
    bool darkTextPixel = false;
    for (int y = secondRow.top(); y <= secondRow.bottom() && !darkTextPixel; ++y) {
        for (int x = 32; x < lightPopup.width() - 8; ++x) {
            const QColor pixel = lightPopup.pixelColor(x, y);
            if (pixel.alpha() > 100 && pixel.lightness() < 100) {
                darkTextPixel = true;
                break;
            }
        }
    }
    QVERIFY(darkTextPixel);
    QCOMPARE(firstRow.height(), 40);
    const QColor accent = combo.palette().color(QPalette::Highlight);
    bool accentPill = false;
    for (int y = firstRow.center().y() - 9; y <= firstRow.center().y() + 9 && !accentPill; ++y) {
        for (int x = 4; x <= 10; ++x) {
            const QColor pixel = lightPopup.pixelColor(x, y);
            const int distance = qAbs(pixel.red() - accent.red())
                    + qAbs(pixel.green() - accent.green()) + qAbs(pixel.blue() - accent.blue());
            if (pixel.alpha() > 100 && distance < 80) {
                accentPill = true;
                break;
            }
        }
    }
    QVERIFY(accentPill);

    combo.hidePopup();
    probe.reset();
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    const QRect reopenedRow = combo.view()->visualRect(combo.model()->index(
            combo.currentIndex(), combo.modelColumn(), combo.rootModelIndex()));
    const QPoint reopenedSelectedCenter =
            combo.view()->viewport()->mapToGlobal(reopenedRow.center());
    QVERIFY(qAbs(reopenedSelectedCenter.y() - comboCenter.y()) <= 4);
    const QRect reopenedGeometry = popup->geometry();
    QVERIFY(probe.movesAfterShow <= 1);
    probe.movesAfterShow = 0;
    probe.resizesAfterShow = 0;
    probe.layoutsAfterShow = 0;
    QTest::qWait(60);
    QCOMPARE(popup->geometry(), reopenedGeometry);
    QCOMPARE(probe.movesAfterShow, 0);
    QCOMPARE(probe.resizesAfterShow, 0);
    QVERIFY(probe.layoutsAfterShow <= 1);

    QCOMPARE(combo.style()->standardIcon(QStyle::SP_ArrowDown).isNull(), false);
    const QRect second = combo.view()->visualRect(combo.model()->index(1, 0));
    QTest::mouseMove(combo.view()->viewport(), second.center());
    QTest::mouseClick(combo.view()->viewport(), Qt::LeftButton, Qt::NoModifier, second.center());
    QCOMPARE(combo.currentIndex(), 1);
    style->setAccentColor({});
}

void WinUI3ComboBoxTest::comboPopupSelectedItemKeepsIcon()
{
    QComboBox combo;
    combo.resize(200, 32);

    QPixmap iconPixmap(16, 16);
    iconPixmap.fill(Qt::transparent);
    {
        QPainter iconPainter(&iconPixmap);
        iconPainter.fillRect(QRect(3, 2, 10, 12), Qt::black);
    }

    QStyleOptionMenuItem item;
    item.initFrom(&combo);
    item.rect = QRect(0, 0, 200, 40);
    item.menuItemType = QStyleOptionMenuItem::Normal;
    item.state = QStyle::State_Enabled | QStyle::State_Selected;
    item.checked = true;
    item.text = QStringLiteral("Document");
    item.icon = QIcon(iconPixmap);

    const auto render = [&](bool withIcon) {
        QStyleOptionMenuItem rendered = item;
        if (!withIcon)
            rendered.icon = {};
        QImage image(rendered.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(rendered.palette.color(QPalette::Window));
        QPainter painter(&image);
        combo.style()->drawControl(QStyle::CE_MenuItem, &rendered, &painter, &combo);
        return image;
    };

    const QImage withIcon = render(true);
    const QImage markerOnly = render(false);
    int changedIconPixels = 0;
    const QRect iconSlot(12, 12, 16, 16);
    for (int y = iconSlot.top(); y <= iconSlot.bottom(); ++y) {
        for (int x = iconSlot.left(); x <= iconSlot.right(); ++x) {
            if (colorDistance(withIcon.pixelColor(x, y), markerOnly.pixelColor(x, y)) > 12)
                ++changedIconPixels;
        }
    }
    QVERIFY2(changedIconPixels > 30,
             qPrintable(QStringLiteral("selected popup icon pixels=%1").arg(changedIconPixels)));
}

void WinUI3ComboBoxTest::comboReleaseActivationAndMarkerMotion()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QWidget host;
    host.resize(440, 320);
    QComboBox combo(&host);
    combo.addItems({ QStringLiteral("First"), QStringLiteral("Selected"), QStringLiteral("Last") });
    combo.setCurrentIndex(1);
    combo.resize(220, 32);
    combo.move(100, 140);
    host.show();
    QTRY_VERIFY(host.isVisible());

    QAbstractItemView *view = combo.view();
    QVERIFY(view);
    QWidget *popup = view->window();
    QVERIFY(popup);
    QVERIFY(!view->isVisible());

    // WinUI opens a ComboBox on the button release.  In particular, a press
    // must not create the popup or perform its first layout pass.
    const QPoint comboCenter = combo.rect().center();
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QVERIFY(!view->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(&combo, "_winui_press_progress") > 0.0, 150);

    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTRY_VERIFY(view->isVisible());
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));

    // The popup's outer 4px bands must survive the view layout and be equal
    // on both sides.  Account for the frame border by measuring the viewport
    // rather than relying on the private container's child hierarchy.
    const QRect viewportInPopup(view->viewport()->mapTo(popup, QPoint(0, 0)),
                                view->viewport()->size());
    QVERIFY(viewportInPopup.top() >= 4);
    QVERIFY(popup->height() - viewportInPopup.bottom() - 1 >= 4);

    const QModelIndex selectedIndex =
            combo.model()->index(combo.currentIndex(), combo.modelColumn(), combo.rootModelIndex());
    // Live: the open slide converges within 167 ms; the anchor check must
    // run after it lands (offscreen has no slide).
    const QRect selectedRow = view->visualRect(selectedIndex);
    QVERIFY(selectedRow.isValid());
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        const QPoint selectedCenter = view->viewport()->mapToGlobal(selectedRow.center());
        const QPoint expectedCenter = combo.mapToGlobal(combo.rect().center());
        QVERIFY(qAbs(selectedCenter.y() - expectedCenter.y()) <= 4);
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(
                qAbs(view->viewport()->mapToGlobal(view->visualRect(selectedIndex).center()).y()
                     - combo.mapToGlobal(combo.rect().center()).y())
                        <= 4,
                1000);
    }

    // The selected item's marker is the only ComboBox-specific animation:
    // the XAML template keeps its 3px width and compresses ScaleY from 1 to
    // 0.625 over 167ms while the pointer is held.
    const QColor background = view->viewport()->palette().color(QPalette::Window);
    const QColor accent = view->viewport()->palette().color(QPalette::Highlight);
    QStyleOptionViewItem item;
    item.initFrom(view->viewport());
    item.rect = QRect(0, 0, view->viewport()->width(), selectedRow.height());
    item.direction = Qt::LeftToRight;
    item.state = QStyle::State_Enabled | QStyle::State_Selected;
    item.features = QStyleOptionViewItem::HasDisplay;
    item.text = combo.currentText();
    item.index = selectedIndex;

    const auto renderMarker = [&](qreal press, Qt::LayoutDirection direction) {
        item.direction = direction;
        setFrame(view->viewport(), "_winui_press_progress", press);
        QImage image(item.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(background);
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ItemViewItem, &item, &painter, view->viewport());
        return image;
    };
    const auto markerHeight = [&](const QImage &image, Qt::LayoutDirection direction) {
        const int left = direction == Qt::RightToLeft ? image.width() - 12 : 0;
        const int right = direction == Qt::RightToLeft ? image.width() - 1 : 11;
        int top = image.height();
        int bottom = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = qMax(0, left); x <= qMin(image.width() - 1, right); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alpha() > 120 && colorDistance(pixel, accent) < 100) {
                    top = qMin(top, y);
                    bottom = qMax(bottom, y);
                }
            }
        }
        return bottom >= top ? bottom - top + 1 : 0;
    };

    const QImage normalMarker = renderMarker(0.0, Qt::LeftToRight);
    const QImage pressedMarker = renderMarker(1.0, Qt::LeftToRight);
    const int normalHeight = markerHeight(normalMarker, Qt::LeftToRight);
    const int pressedHeight = markerHeight(pressedMarker, Qt::LeftToRight);
    QVERIFY2(normalHeight >= 14,
             qPrintable(QStringLiteral("normal marker height=%1").arg(normalHeight)));
    QVERIFY2(pressedHeight >= 8 && pressedHeight <= 12,
             qPrintable(QStringLiteral("pressed marker height=%1").arg(pressedHeight)));
    QVERIFY(pressedHeight < normalHeight);

    // Exercise the real item event path as well as the deterministic pixel
    // probe above. The held press must visibly shorten the selected marker,
    // not merely update an internal animation property.
    setFrame(view->viewport(), "_winui_press_progress", 0.0);
    view->viewport()->repaint();
    const auto liveMarkerHeight = [&] {
        QCoreApplication::processEvents();
        return markerHeight(view->viewport()->grab().toImage(), Qt::LeftToRight);
    };
    const int liveNormalHeight = liveMarkerHeight();
    QTest::mouseMove(view->viewport(), selectedRow.center());
    QTest::mousePress(view->viewport(), Qt::LeftButton, Qt::NoModifier, selectedRow.center());
    QVERIFY2(view->isVisible(), "Combo popup closed on item press");
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") > 0.0);
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(view->viewport(), "_winui_press_progress") > 0.95, 300);
    QTRY_VERIFY_WITH_TIMEOUT(liveMarkerHeight() <= 12, 400);
    const int livePressedHeight = liveMarkerHeight();
    QVERIFY2(liveNormalHeight >= 14,
             qPrintable(QStringLiteral("live normal marker height=%1").arg(liveNormalHeight)));
    QVERIFY2(livePressedHeight >= 8 && livePressedHeight <= 12,
             qPrintable(QStringLiteral("live pressed marker height=%1").arg(livePressedHeight)));
    QVERIFY(livePressedHeight < liveNormalHeight);
    QTest::mouseRelease(view->viewport(), Qt::LeftButton, Qt::NoModifier, selectedRow.center());
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") < 0.05);
    QCOMPARE(combo.currentIndex(), 1);

    combo.hidePopup();

    // Releasing outside cancels the pending open and must not leave a stale
    // mouse grab or press state. RTL follows the same release contract.
    combo.setLayoutDirection(Qt::RightToLeft);
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QVERIFY(!view->isVisible());
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, QPoint(-20, -20));
    QTest::qWait(20);
    QVERIFY(!view->isVisible());
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTRY_VERIFY(view->isVisible());
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));

    const QImage rtlMarker = renderMarker(0.0, Qt::RightToLeft);
    QVERIFY(markerHeight(rtlMarker, Qt::RightToLeft) >= 14);
    combo.hidePopup();
}

void WinUI3ComboBoxTest::comboPopupAssociationLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(360, 240);
    auto *combo = new QComboBox(&host);
    combo->addItems({ QStringLiteral("One"), QStringLiteral("Two"), QStringLiteral("Three") });
    combo->setCurrentIndex(1);
    host.show();
    QTRY_VERIFY(host.isVisible());

    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QWidget *popup = combo->view()->window();
    QVERIFY(popup);
    combo->hidePopup();

    // Re-polishing a combo must discard the old association and allow the
    // next show cycle to establish it again without a first-frame jump.
    style->unpolish(combo);
    style->polish(combo);
    combo->showPopup();
    QTRY_VERIFY(combo->view()->isVisible());
    QCOMPARE(combo->view()->window(), popup);
    QVERIFY(combo->view()->visualRect(combo->model()->index(1, 0)).isValid());

    QPointer<QComboBox> guardedCombo(combo);
    delete combo;
    QVERIFY(guardedCombo.isNull());
}

void WinUI3ComboBoxTest::comboChevronMotion()
{
    QComboBox combo;
    combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
    combo.resize(180, 32);
    combo.show();
    QEnterEvent enter(combo.rect().center(), combo.rect().center(),
                      combo.mapToGlobal(combo.rect().center()));
    QCoreApplication::sendEvent(&combo, &enter);
    QTest::qWait(30);
    QCOMPARE(frameReal(&combo, "_winui_combo_chevron_progress"), 0.0);

    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") > 0.9);
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") < -0.05);
    QTRY_VERIFY(qAbs(frameReal(&combo, "_winui_combo_chevron_progress")) < 0.01);
    combo.hidePopup();
}

void WinUI3ComboBoxTest::comboChevronGeometry()
{
    QComboBox combo;
    combo.addItems({ QStringLiteral("One"), QStringLiteral("Two") });
    combo.resize(220, 32);
    combo.show();
    setFrame(&combo, "_winui_combo_chevron_progress", 0.0);

    const auto renderChevron = [&](Qt::LayoutDirection direction) {
        QStyleOptionComboBox option;
        option.initFrom(&combo);
        option.rect = combo.rect();
        option.direction = direction;
        option.subControls = QStyle::SC_ComboBoxArrow;
        option.state = QStyle::State_Enabled;

        QImage image(combo.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            QPainter painter(&image);
            combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
        }

        const QRect logicalGlyphBox(option.rect.right() - 14 - 12 + 1,
                                    option.rect.top() + (option.rect.height() - 12) / 2, 12, 12);
        const QRect logicalChevron(logicalGlyphBox.adjusted(1, 1, -1, -1));
        const QRect expected = QStyle::visualRect(direction, option.rect, logicalChevron);
        QRect ink;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 24)
                    ink |= QRect(x, y, 1, 1);
            }
        }
        return qMakePair(expected, ink);
    };

    const auto ltr = renderChevron(Qt::LeftToRight);
    const auto rtl = renderChevron(Qt::RightToLeft);
    QVERIFY(!ltr.second.isEmpty());
    QVERIFY(!rtl.second.isEmpty());
    QVERIFY(ltr.first.contains(ltr.second.topLeft())
            && ltr.first.contains(ltr.second.bottomRight()));
    QVERIFY(rtl.first.contains(rtl.second.topLeft())
            && rtl.first.contains(rtl.second.bottomRight()));
    // Segoe Fluent raster extents drift across font versions (local Win11 vs
    // CI Server); ceilings stay inside the 10px box, well below the old path.
    QVERIFY(ltr.second.width() <= 11);
    QVERIFY(ltr.second.height() <= 8);
    QVERIFY(rtl.second.width() <= 11);
    QVERIFY(rtl.second.height() <= 8);
    QCOMPARE(ltr.first.size(), QSize(10, 10));
    QCOMPARE(rtl.first.size(), QSize(10, 10));
}

void WinUI3ComboBoxTest::comboOpenPopupKeyboardCurrentPaintsHoverPill()
{
    // Keyboard current is hover: arrows move view->currentIndex() with no
    // MouseOver, so the current row must read exactly like the hovered row.
    // Same shared pill (paintPopupRowPill), same token, same geometry.
    QComboBox combo;
    combo.addItems({ QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") });
    combo.setStyle(qApp->style());
    combo.resize(220, 32);
    QAbstractItemView *view = combo.view();
    QVERIFY(view);
    view->resize(220, 3 * 40);
    const QModelIndex current = combo.model()->index(1, 0);
    view->setCurrentIndex(current);
    QCOMPARE(view->currentIndex(), current);
    const QRect rowRect(0, 0, view->viewport()->width() > 0 ? view->viewport()->width() : 220, 40);

    QStyleOptionViewItem currentOption;
    currentOption.initFrom(view->viewport());
    currentOption.widget = view->viewport();
    currentOption.rect = rowRect;
    currentOption.index = current;
    currentOption.features = QStyleOptionViewItem::HasDisplay;
    currentOption.text = QStringLiteral("Beta");
    currentOption.state = QStyle::State_Enabled;
    QImage currentImage(rowRect.size(), QImage::Format_ARGB32_Premultiplied);
    currentImage.fill(view->viewport()->palette().color(QPalette::Base));
    {
        QPainter painter(&currentImage);
        combo.style()->drawControl(QStyle::CE_ItemViewItem, &currentOption, &painter,
                                   view->viewport());
    }

    QStyleOptionViewItem hoveredOption = currentOption;
    hoveredOption.state |= QStyle::State_MouseOver;
    hoveredOption.index = combo.model()->index(0, 0);
    QImage hoveredImage(rowRect.size(), QImage::Format_ARGB32_Premultiplied);
    hoveredImage.fill(view->viewport()->palette().color(QPalette::Base));
    {
        QPainter painter(&hoveredImage);
        combo.style()->drawControl(QStyle::CE_ItemViewItem, &hoveredOption, &painter,
                                   view->viewport());
    }
    // Same pixels row-for-row: keyboard current and mouse hover are one
    // visual. Geometry token pairing: the rows share the row height.
    QCOMPARE(currentImage.size(), hoveredImage.size());
    QCOMPARE(currentImage, hoveredImage);
}

void WinUI3ComboBoxTest::comboOpenPopupKeyboardNavMovesHoverPill()
{
    // Live defect: arrow keys in an open combo popup moved the view's current
    // index (and its selection follows), but no row painted the hover pill:
    // the CE_MenuItem combo path drove hover purely from the live cursor.
    // Same state, one token/geometry pair: after Down, the new current row
    // carries the subtleHover pill (4,2 insets, 3px radius) and the value row
    // keeps only its accent marker.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Light);

    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({ QStringLiteral("Alpha"), QStringLiteral("Beta"), QStringLiteral("Gamma") });
    combo.setCurrentIndex(0);
    combo.resize(220, 32);
    combo.move(120, 180);
    host.show();
    QTRY_VERIFY(host.isVisible());
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());

    QAbstractItemView *view = combo.view();
    QCOMPARE(view->currentIndex().row(), 0);
    QTest::keyClick(view, Qt::Key_Down);
    QTRY_COMPARE(view->currentIndex().row(), 1);
    QCOMPARE(combo.currentIndex(), 0);
    QVERIFY(view->selectionModel()->isSelected(combo.model()->index(1, 0)));

    const QImage grab = view->viewport()->grab().toImage();
    const QColor background = view->viewport()->palette().color(QPalette::Window);
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(view->viewport()->palette());
    // Native popups composite over DWM acrylic (translucent Window role);
    // offscreen keeps the opaque fallback. Either way the painter resolves
    // the hover fill over the same palette roles.
    QVERIFY(background.alpha() > 0);
    // The subtleHover fill is translucent ink over the opaque popup surface:
    // resolve the expected on-screen color the same way the painter does.
    QColor expectedHover = tokens.subtleHover;
    {
        QImage mix(1, 1, QImage::Format_ARGB32_Premultiplied);
        mix.fill(background);
        QPainter painter(&mix);
        painter.fillRect(mix.rect(), expectedHover);
        painter.end();
        expectedHover = mix.pixelColor(0, 0);
    }
    // Deterministic unit probe of the CE_MenuItem combo path: render the
    // keyboard-current row exactly as the live delegate describes it after
    // arrow traversal (Selected, not checked, no MouseOver) and require the
    // shared hover pill. To keep the probe live-input honest, derive the
    // expected state from the running popup: the focused flag below copies
    // the live view focus, the Selected flag copies the live selection, so
    // a stale-Qt fallback cannot silently satisfy the paint contract.
    const bool liveHasFocus =
            view->hasFocus() || (view->viewport() && view->viewport()->hasFocus());
    const bool liveSelected = view->selectionModel()->isSelected(combo.model()->index(1, 0));
    QVERIFY2(liveSelected, "live popup must select the keyboard-current row");
    {
        QStyleOptionMenuItem menuOption;
        menuOption.initFrom(view->viewport());
        menuOption.rect = QRect(0, 0, 220, 40);
        menuOption.menuItemType = QStyleOptionMenuItem::Normal;
        menuOption.state = QStyle::State_Enabled
                | (liveSelected ? QStyle::State_Selected : QStyle::State_None)
                | (liveHasFocus ? QStyle::State_HasFocus : QStyle::State_None);
        menuOption.checked = false;
        menuOption.text = QStringLiteral("Beta");
        menuOption.font = combo.font();
        menuOption.fontMetrics = QFontMetrics(combo.font());
        QImage menuImage(menuOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
        menuImage.fill(background);
        {
            QPainter painter(&menuImage);
            style->drawControl(QStyle::CE_MenuItem, &menuOption, &painter, &combo);
        }
        QCOMPARE(menuImage.pixelColor(0, menuImage.height() / 2), background);
        // The subtleHover ink is alpha 9: a row without the pill differs from
        // the expected composite by ~27, while the painted pill is within
        // ~2. Require BOTH: the ink is measurably present (distance from the
        // row background) and it matches the subtleHover composite tightly.
        // Pre-fix, the CE_MenuItem combo path ignored keyboard current and
        // left the row at background -> the ink-presence leg fails.
        const QColor probeCenter =
                menuImage.pixelColor(menuImage.width() / 2, menuImage.height() / 2);
        QVERIFY2(colorDistance(probeCenter, background) > 6,
                 qPrintable(QStringLiteral("no pill ink on keyboard-current row: %1,%2,%3")
                                    .arg(probeCenter.red())
                                    .arg(probeCenter.green())
                                    .arg(probeCenter.blue())));
        QVERIFY2(colorDistance(probeCenter, expectedHover) < 20,
                 qPrintable(QStringLiteral("pill color %1,%2,%3 vs expected %4,%5,%6")
                                    .arg(probeCenter.red())
                                    .arg(probeCenter.green())
                                    .arg(probeCenter.blue())
                                    .arg(expectedHover.red())
                                    .arg(expectedHover.green())
                                    .arg(expectedHover.blue())));
    }

    const auto pillFillAt = [&](int row) -> QColor {
        const QRect rowRect = view->visualRect(combo.model()->index(row, 0));
        if (!rowRect.isValid())
            return QColor();
        return grab.pixelColor(rowRect.center().x(), rowRect.center().y());
    };
    // Mechanism: the keyboard-current row paints the shared hover fill.
    const QColor row1Fill = pillFillAt(1);
    QVERIFY2(row1Fill.isValid(), "row 1 rect invalid");
    QVERIFY2(colorDistance(row1Fill, expectedHover) < 48,
             qPrintable(QStringLiteral("row1=%1,%2,%3 expected hover %4,%5,%6")
                                .arg(row1Fill.red())
                                .arg(row1Fill.green())
                                .arg(row1Fill.blue())
                                .arg(expectedHover.red())
                                .arg(expectedHover.green())
                                .arg(expectedHover.blue())));
    // Geometry: the pill keeps the shared combo insets on the same row.
    // The selected-row accent marker lives in the leading gutter (x=4..10),
    // so probe the fill just right of it; the row edge itself must stay the
    // popup surface on both sides.
    const QRect row1 = view->visualRect(combo.model()->index(1, 0));
    QVERIFY(row1.width() > 16);
    QVERIFY2(colorDistance(grab.pixelColor(row1.left(), row1.center().y()), background) < 48,
             "pill must stay inside the row on the leading edge");
    QVERIFY2(colorDistance(grab.pixelColor(row1.right(), row1.center().y()), background) < 48,
             "pill must stay inside the row on the trailing edge");
    QVERIFY2(colorDistance(grab.pixelColor(row1.left() + 12, row1.center().y()), expectedHover)
                     < 48,
             "pill fill must start inside the shared inset");
    // The value row keeps only its accent marker, never the hover fill.
    const QColor row0Fill = pillFillAt(0);
    QVERIFY2(row0Fill.isValid(), "row 0 rect invalid");
    QVERIFY2(colorDistance(row0Fill, background) < 48,
             qPrintable(QStringLiteral("value row must not take the hover fill")));
    const QColor accent = view->viewport()->palette().color(QPalette::Highlight);
    bool markerInk = false;
    const QRect row0 = view->visualRect(combo.model()->index(0, 0));
    for (int x = 4; x <= 10 && !markerInk; ++x) {
        if (colorDistance(grab.pixelColor(x, row0.center().y()), accent) < 100)
            markerInk = true;
    }
    QVERIFY2(markerInk, "value row must keep its accent marker");
    combo.hidePopup();
}

void WinUI3ComboBoxTest::autoSuggestHoverPillMatchesMenuPill()
{
    // Unified popup lane: a QCompleter (AutoSuggestBox) hover row must paint
    // the identical inset pill as a menu row — same helper
    // (paintPopupRowPill), same 4,2 insets, same 3px radius, same fill
    // token — with text-gutter parity (16px leading when there is no
    // icon/check slot, 42px otherwise).
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Light);

    QLineEdit editor;
    auto *completer =
            new QCompleter(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                        QStringLiteral("Gamma"), QStringLiteral("Delta") },
                           &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(300, 32);
    editor.show();
    editor.setFocus();
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QAbstractItemView *popup = completer->popup();
    QVERIFY(popup);
    // The AutoSuggestBox flyout is the view window itself: a styled frame
    // would paint a square PE_Frame border over the rounded popup surface,
    // leaving the first/last hovered row's 4,2 pill flush against a square
    // edge — the reported ugly mouseover edges. The shared popup-surface
    // prep drops that frame (the DWM rounded region keeps the corners).
    if (auto *frame = qobject_cast<QFrame *>(popup))
        QCOMPARE(frame->frameShape(), QFrame::NoFrame);

    // Hover the second row with real pointer input, then grab the live
    // viewport. QCompleter does not move currentIndex/selection on hover —
    // the delegate paints the pill from the MouseOver item state — so the
    // grab assertions below are the hover-landed proof: fill at the hovered
    // row center, surface at its edges.
    const QModelIndex hoverIdx = popup->model()->index(1, 0);
    const QRect hoverRect = popup->visualRect(hoverIdx);
    QVERIFY(hoverRect.isValid());
    QTest::mouseMove(popup->viewport(), hoverRect.center());
    QCoreApplication::processEvents();
    const QImage live = popup->viewport()->grab().toImage();
    QCOMPARE(live.size(), popup->viewport()->size());
    const QColor base = popup->viewport()->palette().color(QPalette::Base);
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(popup->viewport()->palette());
    QColor expectedHover = tokens.subtleHover;
    {
        QImage mix(1, 1, QImage::Format_ARGB32_Premultiplied);
        mix.fill(base);
        QPainter painter(&mix);
        painter.fillRect(mix.rect(), expectedHover);
        painter.end();
        expectedHover = mix.pixelColor(0, 0);
    }
    QCOMPARE(colorDistance(live.pixelColor(hoverRect.center()), expectedHover) < 48, true);
    // subtleHover ink is alpha 9: a non-hovered row is only ~27 away from the
    // composite, so a loose threshold cannot tell a painted pill from an
    // unpainted row. Require the ink to be measurably present AND tight.
    const QColor hoverCenter = live.pixelColor(hoverRect.center());
    QVERIFY2(colorDistance(hoverCenter, base) > 6,
             qPrintable(QStringLiteral("no hover ink at row center: %1,%2,%3")
                                .arg(hoverCenter.red())
                                .arg(hoverCenter.green())
                                .arg(hoverCenter.blue())));
    QVERIFY2(colorDistance(hoverCenter, expectedHover) < 20,
             qPrintable(QStringLiteral("hover color %1,%2,%3 vs expected %4,%5,%6")
                                .arg(hoverCenter.red())
                                .arg(hoverCenter.green())
                                .arg(hoverCenter.blue())
                                .arg(expectedHover.red())
                                .arg(expectedHover.green())
                                .arg(expectedHover.blue())));
    // Identical 4,2 insets: surface color at the row corners, fill inside.
    QCOMPARE(colorDistance(live.pixelColor(hoverRect.left() + 1, hoverRect.center().y()), base)
                     < 48,
             true);
    QCOMPARE(colorDistance(live.pixelColor(hoverRect.left() + 6, hoverRect.center().y()),
                           expectedHover)
                     < 48,
             true);

    // Ghosting guard: sweeping the hover across rows must not accumulate
    // translucent fills. Move to the next row and require the OLD row to
    // read as plain surface again — a SourceOver Base rebuild would leave
    // the previous pill bleeding through (the live mouseover ghost).
    const QModelIndex nextIdx = popup->model()->index(2, 0);
    const QRect nextRect = popup->visualRect(nextIdx);
    QVERIFY(nextRect.isValid());
    QTest::mouseMove(popup->viewport(), nextRect.center());
    QCoreApplication::processEvents();
    const QImage swept = popup->viewport()->grab().toImage();
    QCOMPARE(swept.size(), popup->viewport()->size());
    QVERIFY2(colorDistance(swept.pixelColor(hoverRect.center()), base) < 48,
             qPrintable(QStringLiteral("stale hover ink at old row: %1,%2,%3")
                                .arg(swept.pixelColor(hoverRect.center()).red())
                                .arg(swept.pixelColor(hoverRect.center()).green())
                                .arg(swept.pixelColor(hoverRect.center()).blue())));
    QVERIFY2(colorDistance(swept.pixelColor(nextRect.center()), expectedHover) < 48,
             "hover pill must follow to the new row");

    // Same pill through the menu path: CE_MenuItem Selected on a 36px row
    // must produce the same insets and the same fill token.
    QMenu menu;
    QStyleOptionMenuItem menuOption;
    menuOption.initFrom(&menu);
    menuOption.rect = QRect(0, 0, hoverRect.width(), 36);
    menuOption.menuItemType = QStyleOptionMenuItem::Normal;
    menuOption.state = QStyle::State_Enabled | QStyle::State_Selected;
    menuOption.text = QStringLiteral("Beta");
    menuOption.font = menu.font();
    menuOption.fontMetrics = QFontMetrics(menu.font());
    const QColor menuSurface = menu.palette().color(QPalette::Window);
    QImage menuImg(menuOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
    menuImg.fill(menuSurface);
    {
        QPainter painter(&menuImg);
        style->drawControl(QStyle::CE_MenuItem, &menuOption, &painter, &menu);
    }
    QCOMPARE(menuImg.size(), menuOption.rect.size());
    QCOMPARE(colorDistance(menuImg.pixelColor(1, menuImg.height() / 2), menuSurface) < 48, true);
    QColor expectedMenuHover = tokens.subtleHover;
    {
        QImage mix(1, 1, QImage::Format_ARGB32_Premultiplied);
        mix.fill(menuSurface);
        QPainter painter(&mix);
        painter.fillRect(mix.rect(), expectedMenuHover);
        painter.end();
        expectedMenuHover = mix.pixelColor(0, 0);
    }
    QCOMPARE(colorDistance(menuImg.pixelColor(6, menuImg.height() / 2), expectedMenuHover) < 48,
             true);
    completer->popup()->hide();
}

void WinUI3ComboBoxTest::autoSuggestCompositedHoverRebuildsRowFrame()
{
    // Live defect (a): gallery Controls-page autoSuggestEdit typing +
    // mouse-moving across rows smears translucent fills on live mica. The
    // completer viewport deliberately keeps WA_OpaquePaintEvent off
    // (claiming it retains old frames as dark ghosts), so CE_ItemViewItem
    // rebuilds every row from the Base role — but on a composited
    // presenter that role is a TRANSLUCENT tint, and the PE pill path then
    // erases the row to transparent first. Row frames must rebuild from
    // the OPAQUE flyout color instead, so every repaint is idempotent.
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Light);

    QLineEdit editor;
    auto *completer =
            new QCompleter(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta"),
                                        QStringLiteral("Gamma"), QStringLiteral("Delta") },
                           &editor);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    editor.setCompleter(completer);
    editor.resize(300, 32);
    editor.show();
    editor.setFocus();
    completer->setCompletionPrefix(QStringLiteral("a"));
    completer->complete();
    QTRY_VERIFY(completer->popup()->isVisible());
    QAbstractItemView *popup = completer->popup();
    QVERIFY(popup);
    QWidget *viewport = popup->viewport();
    QVERIFY(viewport);
    // Dark-ghost lock: the viewport stays non-opaque; the fix must not
    // reclaim the opaque claim to stop the smear.
    QCOMPARE(viewport->testAttribute(Qt::WA_OpaquePaintEvent), false);

    // Live-input anchor: hover a row for real, then grab the viewport.
    const QModelIndex hoverIdx = popup->model()->index(1, 0);
    const QRect hoverRect = popup->visualRect(hoverIdx);
    QVERIFY(hoverRect.isValid());
    QTest::mouseMove(viewport, hoverRect.center());
    QCoreApplication::processEvents();
    QVERIFY(popup->isVisible());
    const QPixmap liveGrab = viewport->grab();
    QVERIFY(!liveGrab.isNull());
    QCOMPARE(liveGrab.size(), viewport->size());

    // Simulate the live composited recipe offscreen: Composited publish +
    // translucent Base/Window roles, exactly what the acrylic branch leaves
    // behind on a working DWM.
    const QColor opaqueBase = viewport->palette().color(QPalette::Base);
    QCOMPARE(opaqueBase.alpha(), 255);
    QWidget *window = popup->window();
    window->setProperty("_winui_backdrop", 3); // Acrylic
    window->setProperty("_winui_backdrop_effective", 2); // Composited
    QColor tint = opaqueBase;
    tint.setAlpha(242);
    QPalette poisoned = viewport->palette();
    poisoned.setColor(QPalette::Base, tint);
    poisoned.setColor(QPalette::Window, tint);
    viewport->setPalette(poisoned);

    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(viewport->palette());
    QColor expectedHover = tokens.subtleHover;
    {
        QImage mix(1, 1, QImage::Format_ARGB32_Premultiplied);
        mix.fill(opaqueBase);
        QPainter painter(&mix);
        painter.fillRect(mix.rect(), expectedHover);
        painter.end();
        expectedHover = mix.pixelColor(0, 0);
    }

    const QRect rowRect(0, 0, hoverRect.width(), hoverRect.height());
    QVERIFY(rowRect.width() > 16 && rowRect.height() > 8);
    const auto paintRow = [&](QImage &canvas, bool hovered) {
        QStyleOptionViewItem option;
        option.initFrom(viewport);
        option.widget = viewport;
        option.rect = rowRect;
        option.index = hoverIdx;
        option.features = QStyleOptionViewItem::HasDisplay;
        option.text = QStringLiteral("Beta");
        option.state =
                QStyle::State_Enabled | (hovered ? QStyle::State_MouseOver : QStyle::State_None);
        QPainter painter(&canvas);
        style->drawControl(QStyle::CE_ItemViewItem, &option, &painter, viewport);
    };

    // Mechanism 1: a hovered row keeps an opaque frame. Pre-fix the PE
    // erase clears the CE rebuild to transparent and the pill floats over
    // the hole (alpha ~9: the live dark ghost sitting on the hovered row).
    // Pair each grab/pixelColor assertion with a QCOMPARE on the same
    // state: center carries the composited hover ink, the row corner reads
    // the CE row surface between pill and row edge (the autosuggest pill
    // keeps the shared 4,2 insets, so x=1/2 sit outside the pill).
    QImage hoveredFrame(rowRect.size(), QImage::Format_ARGB32_Premultiplied);
    QVERIFY(!hoveredFrame.isNull() && !hoveredFrame.size().isEmpty());
    hoveredFrame.fill(tint);
    paintRow(hoveredFrame, true);
    const QColor hoveredCenter = hoveredFrame.pixelColor(hoveredFrame.rect().center());
    QCOMPARE(hoveredCenter.alpha(), 255);
    QCOMPARE(colorDistance(hoveredCenter, expectedHover) < 48, true);
    QCOMPARE(colorDistance(hoveredFrame.pixelColor(1, 1), opaqueBase) < 48, true);

    // Mechanism 2: hover-leave restores the plain opaque surface. Both
    // frames share one retained canvas like the Qt backing store; pre-fix
    // the old row keeps the translucent tint (alpha 242: the smear the
    // next mouseover blends over).
    paintRow(hoveredFrame, false);
    const QColor leftCenter = hoveredFrame.pixelColor(hoveredFrame.rect().center());
    QCOMPARE(leftCenter.alpha(), 255);
    QCOMPARE(colorDistance(leftCenter, opaqueBase) < 48, true);
    completer->popup()->hide();
}

void WinUI3ComboBoxTest::autoSuggestResolvedFramePreservesRgb_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<int>("phase");
    for (bool dark : { false, true }) {
        for (bool compact : { false, true }) {
            for (int phase = 0; phase < 3; ++phase) {
                const QByteArray name = QByteArray(dark ? "dark" : "light")
                        + (compact ? "-compact-" : "-standard-")
                        + (phase == 0           ? "rest"
                                   : phase == 1 ? "hover"
                                                : "leave");
                QTest::newRow(name.constData()) << dark << compact << phase;
            }
        }
    }
}

void WinUI3ComboBoxTest::autoSuggestResolvedFramePreservesRgb()
{
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(int, phase);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);

    QLineEdit editor;
    QCompleter completer(QStringList{ QStringLiteral("Alpha"), QStringLiteral("Beta") });
    editor.setCompleter(&completer);
    completer.setCompletionMode(QCompleter::PopupCompletion);
    QAbstractItemView *popup = completer.popup();
    QVERIFY(popup);
    WinUI3::Style::setDensityMode(
            &editor, compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    editor.resize(300, compact ? 24 : 32);
    editor.show();
    completer.complete();
    QTRY_VERIFY(popup->isVisible());
    popup->setCurrentIndex({});
    QWidget *viewport = popup->viewport();
    QVERIFY(viewport);

    // Pure renderer contract, not a native grant or live-hover proof. The
    // popup preparation already resolved the flyout RGB before assigning
    // the composited tint's alpha. Neither CE nor PE may apply that lift twice.
    const int channel = dark ? 44 : 252;
    const QColor opaqueBase(channel, channel, channel);
    QCOMPARE(WinUI3::Private::popupSurfaceColor(style->standardPalette()), opaqueBase);
    QColor tint = opaqueBase;
    tint.setAlpha(dark ? 178 : 242);
    QPalette palette = style->standardPalette();
    palette.setColor(QPalette::Window, tint);
    palette.setColor(QPalette::Base, tint);

    QStyleOptionViewItem option;
    option.initFrom(viewport);
    option.widget = viewport;
    option.palette = palette;
    option.index = popup->model()->index(1, 0);
    QVERIFY(option.index.isValid());
    option.features = QStyleOptionViewItem::HasDisplay;
    option.text = QStringLiteral("Beta");
    option.rect = QRect(0, 0, 300, compact ? 32 : 40);
    QCOMPARE(style->sizeFromContents(QStyle::CT_ItemViewItem, &option, QSize(80, 16), viewport)
                     .height(),
             option.rect.height());

    QImage frame(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);
    const auto paint = [&](bool hovered) {
        option.state =
                QStyle::State_Enabled | (hovered ? QStyle::State_MouseOver : QStyle::State_None);
        QPainter painter(&frame);
        style->drawControl(QStyle::CE_ItemViewItem, &option, &painter, viewport);
    };
    if (phase == 2)
        paint(true);
    paint(phase == 1);

    // Outside the 4,2 pill: exact RGB as well as alpha rejects both the
    // translucent PE overwrite and a second popupSurfaceColor lift in CE/PE.
    const QColor edge = frame.pixelColor(1, option.rect.center().y());
    QCOMPARE(frame.size(), option.rect.size());
    QCOMPARE(edge.alpha(), 255);
    QCOMPARE(edge.rgb(), opaqueBase.rgb());
}

QTEST_MAIN(WinUI3ComboBoxTest)
#include "tst_winui3combobox.moc"
