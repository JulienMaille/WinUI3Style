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

class WinUI3EditorsTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void textBoxInteraction();
    void clearButtonStateContract();
    void clearButtonFocusAndGlyphContract();
    void textBoxStateMatrix();
    void themeComboSizingContract();
    void indeterminateProgressDeterminism();
    void comboPopupContract();
    void comboPopupSelectedItemKeepsIcon();
    void comboReleaseActivationAndMarkerMotion();
    void comboPopupAssociationLifecycle();
    void comboChevronMotion();
    void comboChevronGeometry();
    void numberBoxSubcontrolContract();
    void verticalNumberBoxContract();
    void spinBoxFocusUnderlinePixelContract();
};

void WinUI3EditorsTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
}

void WinUI3EditorsTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
    }
}

void WinUI3EditorsTest::cleanup()
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


void WinUI3EditorsTest::textBoxInteraction()
{
    QLineEdit edit;
    edit.setText(QStringLiteral("Clear me"));
    edit.setPlaceholderText(QStringLiteral("Placeholder"));
    edit.setClearButtonEnabled(true);
    edit.resize(240, 32);
    edit.show();
    QEvent enter(QEvent::Enter);
    QCoreApplication::sendEvent(&edit, &enter);
    QCOMPARE(frameReal(&edit, "_winui_hover_progress"), 1.0);

    QStyleOptionFrame option;
    option.initFrom(&edit);
    option.rect = edit.rect();
    const QRect ltrContents = edit.style()->subElementRect(
        QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(ltrContents, edit.rect().adjusted(10, 5, -6, -6));
    option.direction = Qt::RightToLeft;
    const QRect rtlContents = edit.style()->subElementRect(
        QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(rtlContents, edit.rect().adjusted(6, 5, -10, -6));
    option.direction = Qt::LeftToRight;

    QVERIFY(!edit.style()->standardIcon(QStyle::SP_LineEditClearButton,
                                         nullptr, &edit).isNull());
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    const auto *clearButton = edit.findChildren<QAbstractButton *>().constFirst();
    QStyleOptionToolButton clearOption;
    clearOption.initFrom(clearButton);
    QCOMPARE(edit.style()->sizeFromContents(QStyle::CT_ToolButton, &clearOption,
                                             QSize(16, 16), clearButton),
             QSize(30, 32));

    QFocusEvent mouseFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&edit, &mouseFocus);
    QCOMPARE(frameReal(&edit, "_winui_focus_progress"), 1.0);
    QVERIFY(!frameBool(&edit, "_winui_focus_visible"));

    QImage focused(edit.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        edit.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option,
                                    &painter, &edit);
    }
    const QColor accent = edit.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    QVERIFY(distance(focused.pixelColor(edit.rect().center().x(),
                                         edit.rect().bottom() - 1), accent) < 80);
    QVERIFY(distance(focused.pixelColor(5, edit.rect().bottom() - 1), accent) < 100);

    QFocusEvent tabFocus(QEvent::FocusIn, Qt::TabFocusReason);
    QCoreApplication::sendEvent(&edit, &tabFocus);
    QVERIFY(frameBool(&edit, "_winui_focus_visible"));
}

void WinUI3EditorsTest::clearButtonStateContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QLineEdit edit(QStringLiteral("Clear me"));
    edit.setClearButtonEnabled(true);
    edit.resize(240, 32);
    edit.show();
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    QAbstractButton *clearButton = edit.findChildren<QAbstractButton *>().constFirst();
    QVERIFY(clearButton);
    QTRY_VERIFY(clearButton->testAttribute(Qt::WA_Hover));
    QTest::mouseMove(&edit, QPoint(8, edit.rect().center().y()));
    QTRY_VERIFY(!clearButton->underMouse());
    const QImage runtimeNormal = edit.grab().toImage();
    const QRect helperRect = clearButton->geometry().intersected(edit.rect());
    QVERIFY2(helperRect.isValid(), qPrintable(QString::fromLatin1(
        "private clear button has no editor intersection")));
    QVERIFY(helperRect.height() < edit.height());
    QVERIFY(helperRect.top() > edit.rect().top());
    QRectF surfaceGeometry(helperRect);
    surfaceGeometry.setTop(edit.rect().top() + 3.0);
    surfaceGeometry.setBottom(edit.rect().bottom() - 2.0);
    const qreal surfaceSide = surfaceGeometry.height();
    if (edit.layoutDirection() == Qt::RightToLeft) {
        surfaceGeometry.setLeft(edit.rect().left() + 3.0);
        surfaceGeometry.setRight(surfaceGeometry.left() + surfaceSide);
    } else {
        surfaceGeometry.setRight(edit.rect().right() - 2.0);
        surfaceGeometry.setLeft(surfaceGeometry.right() - surfaceSide);
    }
    const QRect surfaceRect = surfaceGeometry.toAlignedRect();
    QVERIFY(surfaceRect.isValid());
    QVERIFY(qAbs(surfaceGeometry.width() - surfaceGeometry.height()) < 0.01);
    QVERIFY(qAbs(surfaceGeometry.center().x() - helperRect.center().x()) <= 1.5);
    QVERIFY(qAbs(surfaceGeometry.center().y() - edit.rect().center().y()) <= 1.0);
    UpdateRequestProbe parentRepaint;
    edit.installEventFilter(&parentRepaint);
    QCoreApplication::processEvents();
    parentRepaint.updateRequests = 0;
    parentRepaint.paints = 0;
    QTest::mouseMove(clearButton, clearButton->rect().center());
    QTRY_VERIFY(clearButton->underMouse());
    QTRY_COMPARE(frameReal(clearButton, "_winui_hover_progress"), 1.0);
    QCoreApplication::processEvents();
    const QImage runtimeHover = edit.grab().toImage();
    QVERIFY(parentRepaint.updateRequests > 0);
    QVERIFY(runtimeNormal != runtimeHover);
    const QPoint surfaceCenter(surfaceRect.center().x(), surfaceRect.center().y());
    for (const QPoint &corner : {surfaceRect.topLeft(), surfaceRect.topRight(),
                                 surfaceRect.bottomLeft(), surfaceRect.bottomRight()})
        QCOMPARE(runtimeHover.pixelColor(corner), runtimeNormal.pixelColor(corner));
    QCOMPARE(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.top() - 1),
             runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.top() - 1));
    QCOMPARE(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.bottom() + 1),
             runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.bottom() + 1));
    QVERIFY(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.top())
            != runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.top()));
    QVERIFY(runtimeHover.pixelColor(surfaceCenter.x(), surfaceRect.bottom())
            != runtimeNormal.pixelColor(surfaceCenter.x(), surfaceRect.bottom()));
    int visiblyChangedPixels = 0;
    int maximumChannelDelta = 0;
    for (int y = helperRect.top(); y <= helperRect.bottom(); ++y) {
        for (int x = helperRect.left(); x <= helperRect.right(); ++x) {
            const QColor before = runtimeNormal.pixelColor(x, y);
            const QColor after = runtimeHover.pixelColor(x, y);
            const int delta = std::max({qAbs(before.red() - after.red()),
                                        qAbs(before.green() - after.green()),
                                        qAbs(before.blue() - after.blue())});
            maximumChannelDelta = qMax(maximumChannelDelta, delta);
            if (delta >= 3)
                ++visiblyChangedPixels;
        }
    }
    QVERIFY2(visiblyChangedPixels >= 100,
             qPrintable(QStringLiteral("changed=%1 maxDelta=%2")
                            .arg(visiblyChangedPixels)
                            .arg(maximumChannelDelta)));
    QVERIFY2(maximumChannelDelta >= 6,
             qPrintable(QStringLiteral("maxDelta=%1")
                            .arg(maximumChannelDelta)));
    QTest::mouseMove(&edit, QPoint(8, edit.rect().center().y()));
    QTRY_COMPARE(frameReal(clearButton, "_winui_hover_progress"), 0.0);
    setFrame(clearButton, "_winui_hover_progress", 0.0);
    setFrame(clearButton, "_winui_press_progress", 0.0);

    parentRepaint.updateRequests = 0;
    QTest::mousePress(clearButton, Qt::LeftButton, Qt::NoModifier,
                      clearButton->rect().center());
    QTRY_COMPARE(frameReal(clearButton, "_winui_press_progress"), 1.0);
    QCoreApplication::processEvents();
    QVERIFY(parentRepaint.updateRequests > 0);
    const QImage runtimePressed = edit.grab().toImage();
    QVERIFY(runtimeHover != runtimePressed);
    QTest::mouseRelease(clearButton, Qt::LeftButton, Qt::NoModifier,
                        clearButton->rect().center());

    QStyleOptionToolButton option;
    option.initFrom(clearButton);
    option.rect = QRect(QPoint(), QSize(30, 32));
    option.palette = clearButton->palette();
    option.icon = clearButton->icon().isNull()
        ? style->standardIcon(QStyle::SP_LineEditClearButton, nullptr, &edit)
        : clearButton->icon();
    option.iconSize = QSize(16, 16);

    const auto render = [&](QStyle::State state, qreal hover, qreal press) {
        setFrame(clearButton, "_winui_hover_progress", hover);
        setFrame(clearButton, "_winui_press_progress", press);
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(edit.palette().color(QPalette::Window));
        option.state = state;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_PanelButtonTool, &option,
                             &painter, clearButton);
        style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                           &painter, clearButton);
        return image;
    };

    const QImage normal = render(QStyle::State_Enabled, 0.0, 0.0);
    const QImage pointerOver = render(QStyle::State_Enabled | QStyle::State_MouseOver,
                                      1.0, 0.0);
    const QImage pressed = render(QStyle::State_Enabled | QStyle::State_MouseOver
                                  | QStyle::State_Sunken, 1.0, 1.0);
    // The private helper's own primitive intentionally stays transparent;
    // hover is painted once, at editor scope, so it cannot be clipped by the
    // private child button's small geometry.
    QCOMPARE(normal, pointerOver);
    QVERIFY(pointerOver != pressed);

    // DeleteButton keeps the secondary glyph on pointer-over and switches to
    // the tertiary foreground only while pressed.
    const auto glyphOnly = [&](QStyle::State state) {
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        option.state = state;
        QPainter painter(&image);
        style->drawControl(QStyle::CE_ToolButtonLabel, &option,
                           &painter, clearButton);
        return image;
    };
    const QImage normalGlyph = glyphOnly(QStyle::State_Enabled);
    const QImage hoverGlyph = glyphOnly(QStyle::State_Enabled | QStyle::State_MouseOver);
    const QImage pressedGlyph = glyphOnly(QStyle::State_Enabled | QStyle::State_Sunken);
    QCOMPARE(normalGlyph, hoverGlyph);
    QVERIFY(normalGlyph != pressedGlyph);
}

void WinUI3EditorsTest::clearButtonFocusAndGlyphContract()
{
    QWidget host;
    QVBoxLayout layout(&host);
    QLineEdit edit(QStringLiteral("Clear me"));
    edit.setClearButtonEnabled(true);
    QPushButton other(QStringLiteral("Other"));
    layout.addWidget(&edit);
    layout.addWidget(&other);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    QAbstractButton *clearButton = edit.findChildren<QAbstractButton *>().constFirst();

    other.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!clearButton->isVisible());
    edit.clear();
    edit.setText(QStringLiteral("Programmatic text"));
    QTRY_VERIFY(!clearButton->isVisible());
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(clearButton->isVisible());
    other.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(!clearButton->isVisible());
    edit.setFocus(Qt::MouseFocusReason);
    QTRY_VERIFY(clearButton->isVisible());

    QStyleOptionToolButton option;
    option.initFrom(clearButton);
    option.rect = QRect(QPoint(), QSize(30, 32));
    option.icon = clearButton->icon();
    option.iconSize = QSize(16, 16); // Qt default; style must enforce WinUI 12 px.
    option.state = QStyle::State_Enabled;
    QImage glyph(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
    glyph.fill(Qt::transparent);
    {
        QPainter painter(&glyph);
        edit.style()->drawControl(QStyle::CE_ToolButtonLabel, &option,
                                  &painter, clearButton);
    }
    QRect ink;
    for (int y = 0; y < glyph.height(); ++y) {
        for (int x = 0; x < glyph.width(); ++x) {
            if (glyph.pixelColor(x, y).alpha() > 24)
                ink |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(!ink.isEmpty());
    QVERIFY(ink.width() <= 12);
    QVERIFY(ink.height() <= 12);
    const QPointF officialCenter(13.0, 16.0);
    QVERIFY2(qAbs(ink.center().x() - officialCenter.x()) <= 2.0,
             qPrintable(QStringLiteral("ink=%1,%2 %3x%4")
                            .arg(ink.x()).arg(ink.y())
                            .arg(ink.width()).arg(ink.height())));
    QVERIFY2(qAbs(ink.center().y() - officialCenter.y()) <= 2.0,
             qPrintable(QStringLiteral("ink=%1,%2 %3x%4")
                            .arg(ink.x()).arg(ink.y())
                            .arg(ink.width()).arg(ink.height())));

    // Validate the real private-button geometry against the editor-scoped
    // hover surface.  A synthetic 30x32 slot previously allowed an X that was
    // visibly low/right in a live TextBox.
    QStyleOptionToolButton liveOption;
    liveOption.initFrom(clearButton);
    liveOption.rect = clearButton->rect();
    liveOption.icon = clearButton->icon();
    liveOption.iconSize = QSize(16, 16);
    liveOption.state = QStyle::State_Enabled | QStyle::State_MouseOver;
    QImage liveGlyph(clearButton->size(), QImage::Format_ARGB32_Premultiplied);
    liveGlyph.fill(Qt::transparent);
    {
        QPainter painter(&liveGlyph);
        edit.style()->drawControl(QStyle::CE_ToolButtonLabel, &liveOption,
                                  &painter, clearButton);
    }
    QRect liveInk;
    for (int y = 0; y < liveGlyph.height(); ++y)
        for (int x = 0; x < liveGlyph.width(); ++x)
            if (liveGlyph.pixelColor(x, y).alpha() > 24)
                liveInk |= QRect(x, y, 1, 1);
    QVERIFY(!liveInk.isEmpty());
    const QPointF inkCenter = QPointF(clearButton->geometry().topLeft())
        + QPointF(liveInk.center());
    const qreal surfaceTop = edit.rect().top() + 3.0;
    const qreal surfaceBottom = edit.rect().bottom() - 2.0;
    const qreal surfaceSide = surfaceBottom - surfaceTop;
    const QPointF surfaceCenter(edit.rect().right() - 2.0 - surfaceSide / 2.0,
                                (surfaceTop + surfaceBottom) / 2.0);
    QVERIFY2(qAbs(inkCenter.x() - surfaceCenter.x()) <= 1.0,
             qPrintable(QStringLiteral("ink x=%1 surface x=%2")
                            .arg(inkCenter.x()).arg(surfaceCenter.x())));
    QVERIFY2(qAbs(inkCenter.y() - surfaceCenter.y()) <= 1.0,
             qPrintable(QStringLiteral("ink y=%1 surface y=%2")
                            .arg(inkCenter.y()).arg(surfaceCenter.y())));
}

void WinUI3EditorsTest::textBoxStateMatrix()
{
    auto renderPanel = [](WinUI3::Style &style, QStyle::State state) {
        QStyleOptionFrame option;
        option.rect = QRect(0, 0, 240, 32);
        option.palette = style.standardPalette();
        option.state = state;
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style.drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter);
        return image;
    };

    for (const WinUI3::ThemeMode mode : {WinUI3::ThemeMode::Light,
                                         WinUI3::ThemeMode::Dark}) {
        WinUI3::Style style(mode);
        const QPalette palette = style.standardPalette();
        QCOMPARE(palette.color(QPalette::Highlight), style.accentColor());
        const QColor expectedOnAccent = qGray(style.accentColor().rgb()) < 128
            ? QColor(Qt::white) : QColor(Qt::black);
        QCOMPARE(palette.color(QPalette::HighlightedText), expectedOnAccent);
        QVERIFY(palette.color(QPalette::Disabled, QPalette::Text).alpha() <
                palette.color(QPalette::Active, QPalette::Text).alpha());
        QVERIFY(palette.color(QPalette::Disabled, QPalette::PlaceholderText).alpha() <
                palette.color(QPalette::Active, QPalette::PlaceholderText).alpha());

        const QImage normal = renderPanel(style, QStyle::State_Enabled);
        const QImage focused = renderPanel(style, QStyle::State_Enabled
                                                     | QStyle::State_HasFocus);
        const QImage disabled = renderPanel(style, QStyle::State_None);
        QVERIFY(normal != focused);
        QVERIFY(normal != disabled);
        QCOMPARE(disabled.pixelColor(disabled.rect().center().x(),
                                     disabled.rect().bottom() - 1),
                 disabled.pixelColor(4, disabled.rect().bottom() - 1));
        QVERIFY(focused.pixelColor(focused.rect().center().x(),
                                   focused.rect().bottom() - 1)
                != disabled.pixelColor(disabled.rect().center().x(),
                                       disabled.rect().bottom() - 1));
    }

    QLineEdit readOnly(QStringLiteral("Selection remains available"));
    readOnly.setClearButtonEnabled(true);
    readOnly.resize(240, 32);
    readOnly.show();
    readOnly.setReadOnly(true);
    readOnly.selectAll();
    QCOMPARE(readOnly.selectedText(), readOnly.text());
    const QString before = readOnly.text();
    QTest::keyClicks(&readOnly, QStringLiteral("blocked"));
    QCOMPARE(readOnly.text(), before);
    QTRY_VERIFY(!readOnly.findChildren<QAbstractButton *>().isEmpty());
    QTRY_VERIFY(!readOnly.findChildren<QAbstractButton *>().constFirst()->isVisible());

    QLineEdit disabled(QStringLiteral("Disabled text"));
    disabled.setPlaceholderText(QStringLiteral("Disabled placeholder"));
    disabled.setEnabled(false);
    disabled.resize(240, 32);
    disabled.show();
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::Text),
             QColor(0, 0, 0, 92));
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::PlaceholderText),
             QColor(0, 0, 0, 92));
}

void WinUI3EditorsTest::themeComboSizingContract()
{
    // Keep this in lockstep with GalleryWindow's command-bar theme selector:
    // the longest item must remain available before the toolbar is shown.
    QComboBox combo;
    combo.addItems({QStringLiteral("System theme"), QStringLiteral("Light"),
                    QStringLiteral("Dark")});
    combo.setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combo.setMinimumContentsLength(combo.itemText(0).size());
    combo.setMinimumWidth(combo.sizeHint().width());

    const int textWidth = QFontMetrics(combo.font()).horizontalAdvance(
        combo.itemText(0));
    QVERIFY(combo.minimumWidth() >= textWidth);
    combo.resize(combo.minimumWidth(), 32);
    combo.show();
    QTRY_VERIFY(combo.isVisible());

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.currentText = combo.currentText();
    const QRect edit = combo.style()->subControlRect(
        QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &combo);
    QVERIFY2(edit.width() >= textWidth,
             qPrintable(QStringLiteral("edit width %1 < text width %2")
                            .arg(edit.width()).arg(textWidth)));

    combo.setEditable(true);
    QVERIFY(combo.lineEdit());
    for (const Qt::LayoutDirection direction : {Qt::LeftToRight,
                                                 Qt::RightToLeft}) {
        combo.setLayoutDirection(direction);
        QCoreApplication::processEvents();
        option.initFrom(&combo);
        option.rect = combo.rect();
        option.direction = direction;
        const QRect labelSlot = combo.style()->subControlRect(
            QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, &combo);
        QStyleOptionFrame editorOption;
        editorOption.initFrom(combo.lineEdit());
        editorOption.rect = combo.lineEdit()->rect();
        editorOption.direction = direction;
        const QRect editorContents = combo.lineEdit()->style()->subElementRect(
            QStyle::SE_LineEditContents, &editorOption, combo.lineEdit())
            .translated(combo.lineEdit()->pos());
        if (direction == Qt::LeftToRight)
            QCOMPARE(editorContents.left(), labelSlot.left());
        else
            QCOMPARE(editorContents.right(), labelSlot.right());
    }
}

void WinUI3EditorsTest::indeterminateProgressDeterminism()
{
    const bool animationSettingExisted = qEnvironmentVariableIsSet(
        "WINUI3STYLE_DISABLE_ANIMATIONS");
    const QByteArray previousSetting = qgetenv("WINUI3STYLE_DISABLE_ANIMATIONS");
    qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");

    QProgressBar bar;
    bar.setRange(0, 0);
    bar.resize(320, 24);
    bar.show();
    QImage first(bar.size(), QImage::Format_ARGB32_Premultiplied);
    first.fill(Qt::transparent);
    bar.render(&first);
    QTest::qWait(25);
    QImage second(bar.size(), QImage::Format_ARGB32_Premultiplied);
    second.fill(Qt::transparent);
    bar.render(&second);

    if (animationSettingExisted)
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", previousSetting);
    else
        qunsetenv("WINUI3STYLE_DISABLE_ANIMATIONS");

    QCOMPARE(first, second);
}

void WinUI3EditorsTest::comboPopupContract()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({QStringLiteral("Blue"), QStringLiteral("Green"), QStringLiteral("Red")});
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
    QSignalSpy firstOpenScrollChanges(combo.view()->verticalScrollBar(),
                                      &QScrollBar::valueChanged);
    combo.showPopup();
    QTRY_VERIFY(combo.view()->isVisible());
    QTRY_VERIFY(!probe.selectedCenterAtShow.isNull());
    QCOMPARE(popup->geometry(), probe.geometryAtShow);
    const QPoint comboCenterOnFirstOpen = combo.mapToGlobal(combo.rect().center());
    const QRect firstOpenSelectedRect = combo.view()->visualRect(
        combo.model()->index(1, combo.modelColumn(), combo.rootModelIndex()));
    const QPoint firstOpenSelectedCenter = combo.view()->viewport()->mapToGlobal(
        firstOpenSelectedRect.center());
    QVERIFY2(qAbs(probe.selectedCenterAtShow.y() - comboCenterOnFirstOpen.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                            .arg(probe.selectedCenterAtShow.y())
                            .arg(comboCenterOnFirstOpen.y())
                            .arg(popup->x()).arg(popup->y())
                            .arg(popup->width()).arg(popup->height())
                            .arg(firstOpenSelectedRect.x()).arg(firstOpenSelectedRect.y())
                            .arg(firstOpenSelectedRect.width()).arg(firstOpenSelectedRect.height())));
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
    const QPoint selectedCenter = combo.view()->viewport()->mapToGlobal(
        firstRow.center());
    QVERIFY2(qAbs(selectedCenter.y() - comboCenter.y()) <= 4,
             qPrintable(QStringLiteral("selected=%1 combo=%2 popup=%3,%4,%5,%6 row=%7,%8,%9,%10")
                            .arg(selectedCenter.y()).arg(comboCenter.y())
                            .arg(popup->x()).arg(popup->y())
                            .arg(popup->width()).arg(popup->height())
                            .arg(firstRow.x()).arg(firstRow.y())
                            .arg(firstRow.width()).arg(firstRow.height())));
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
        QTRY_VERIFY_WITH_TIMEOUT(
            qAbs(popup->y() - lightGeometry.y()) <= 12, 1000);
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
    for (int y = firstRow.center().y() - 9;
         y <= firstRow.center().y() + 9 && !accentPill; ++y) {
        for (int x = 4; x <= 10; ++x) {
            const QColor pixel = lightPopup.pixelColor(x, y);
            const int distance = qAbs(pixel.red() - accent.red())
                + qAbs(pixel.green() - accent.green())
                + qAbs(pixel.blue() - accent.blue());
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
    const QRect reopenedRow = combo.view()->visualRect(
        combo.model()->index(combo.currentIndex(), combo.modelColumn(),
                             combo.rootModelIndex()));
    const QPoint reopenedSelectedCenter = combo.view()->viewport()->mapToGlobal(
        reopenedRow.center());
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
    QTest::mouseClick(combo.view()->viewport(), Qt::LeftButton,
                      Qt::NoModifier, second.center());
    QCOMPARE(combo.currentIndex(), 1);
    style->setAccentColor({});
}

void WinUI3EditorsTest::comboPopupSelectedItemKeepsIcon()
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
        combo.style()->drawControl(QStyle::CE_MenuItem, &rendered, &painter,
                                   &combo);
        return image;
    };

    const QImage withIcon = render(true);
    const QImage markerOnly = render(false);
    int changedIconPixels = 0;
    const QRect iconSlot(12, 12, 16, 16);
    for (int y = iconSlot.top(); y <= iconSlot.bottom(); ++y) {
        for (int x = iconSlot.left(); x <= iconSlot.right(); ++x) {
            if (colorDistance(withIcon.pixelColor(x, y),
                              markerOnly.pixelColor(x, y)) > 12)
                ++changedIconPixels;
        }
    }
    QVERIFY2(changedIconPixels > 30,
             qPrintable(QStringLiteral("selected popup icon pixels=%1")
                            .arg(changedIconPixels)));
}

void WinUI3EditorsTest::comboReleaseActivationAndMarkerMotion()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);

    QWidget host;
    host.resize(440, 320);
    QComboBox combo(&host);
    combo.addItems({QStringLiteral("First"), QStringLiteral("Selected"),
                    QStringLiteral("Last")});
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
    QTRY_VERIFY_WITH_TIMEOUT(frameReal(&combo, "_winui_press_progress") > 0.0,
                             150);

    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QTRY_VERIFY(view->isVisible());
    QCOMPARE(popup->contentsMargins(), QMargins(0, 4, 0, 4));

    // The popup's outer 4px bands must survive the view layout and be equal
    // on both sides.  Account for the frame border by measuring the viewport
    // rather than relying on the private container's child hierarchy.
    const QRect viewportInPopup(
        view->viewport()->mapTo(popup, QPoint(0, 0)),
        view->viewport()->size());
    QVERIFY(viewportInPopup.top() >= 4);
    QVERIFY(popup->height() - viewportInPopup.bottom() - 1 >= 4);

    const QModelIndex selectedIndex = combo.model()->index(
        combo.currentIndex(), combo.modelColumn(), combo.rootModelIndex());
    // Live: the open slide converges within 167 ms; the anchor check must
    // run after it lands (offscreen has no slide).
    const QRect selectedRow = view->visualRect(selectedIndex);
    QVERIFY(selectedRow.isValid());
    if (QGuiApplication::platformName() == QStringLiteral("offscreen")) {
        const QPoint selectedCenter = view->viewport()->mapToGlobal(
            selectedRow.center());
        const QPoint expectedCenter = combo.mapToGlobal(combo.rect().center());
        QVERIFY(qAbs(selectedCenter.y() - expectedCenter.y()) <= 4);
    } else {
        QTRY_VERIFY_WITH_TIMEOUT(
            qAbs(view->viewport()
                     ->mapToGlobal(
                         view->visualRect(selectedIndex).center())
                     .y()
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
        style->drawControl(QStyle::CE_ItemViewItem, &item, &painter,
                           view->viewport());
        return image;
    };
    const auto markerHeight = [&](const QImage &image,
                                  Qt::LayoutDirection direction) {
        const int left = direction == Qt::RightToLeft
            ? image.width() - 12 : 0;
        const int right = direction == Qt::RightToLeft
            ? image.width() - 1 : 11;
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
             qPrintable(QStringLiteral("normal marker height=%1")
                            .arg(normalHeight)));
    QVERIFY2(pressedHeight >= 8 && pressedHeight <= 12,
             qPrintable(QStringLiteral("pressed marker height=%1")
                            .arg(pressedHeight)));
    QVERIFY(pressedHeight < normalHeight);

    // Exercise the real item event path as well as the deterministic pixel
    // probe above. The held press must visibly shorten the selected marker,
    // not merely update an internal animation property.
    setFrame(view->viewport(), "_winui_press_progress", 0.0);
    view->viewport()->repaint();
    const auto liveMarkerHeight = [&] {
        QCoreApplication::processEvents();
        return markerHeight(view->viewport()->grab().toImage(),
                            Qt::LeftToRight);
    };
    const int liveNormalHeight = liveMarkerHeight();
    QTest::mouseMove(view->viewport(), selectedRow.center());
    QTest::mousePress(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      selectedRow.center());
    QVERIFY2(view->isVisible(), "Combo popup closed on item press");
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") > 0.0);
    QTRY_VERIFY_WITH_TIMEOUT(
        frameReal(view->viewport(), "_winui_press_progress") > 0.95, 300);
    QTRY_VERIFY_WITH_TIMEOUT(liveMarkerHeight() <= 12, 400);
    const int livePressedHeight = liveMarkerHeight();
    QVERIFY2(liveNormalHeight >= 14,
             qPrintable(QStringLiteral("live normal marker height=%1")
                            .arg(liveNormalHeight)));
    QVERIFY2(livePressedHeight >= 8 && livePressedHeight <= 12,
             qPrintable(QStringLiteral("live pressed marker height=%1")
                            .arg(livePressedHeight)));
    QVERIFY(livePressedHeight < liveNormalHeight);
    QTest::mouseRelease(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                        selectedRow.center());
    QTRY_VERIFY(frameReal(view->viewport(), "_winui_press_progress") < 0.05);
    QCOMPARE(combo.currentIndex(), 1);

    combo.hidePopup();

    // Releasing outside cancels the pending open and must not leave a stale
    // mouse grab or press state. RTL follows the same release contract.
    combo.setLayoutDirection(Qt::RightToLeft);
    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, comboCenter);
    QVERIFY(!view->isVisible());
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier,
                        QPoint(-20, -20));
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

void WinUI3EditorsTest::comboPopupAssociationLifecycle()
{
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    QWidget host;
    host.resize(360, 240);
    auto *combo = new QComboBox(&host);
    combo->addItems({QStringLiteral("One"), QStringLiteral("Two"),
                     QStringLiteral("Three")});
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

void WinUI3EditorsTest::comboChevronMotion()
{
    QComboBox combo;
    combo.addItems({QStringLiteral("One"), QStringLiteral("Two")});
    combo.resize(180, 32);
    combo.show();
    QEnterEvent enter(combo.rect().center(), combo.rect().center(),
                      combo.mapToGlobal(combo.rect().center()));
    QCoreApplication::sendEvent(&combo, &enter);
    QTest::qWait(30);
    QCOMPARE(frameReal(&combo, "_winui_combo_chevron_progress"), 0.0);

    QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") > 0.9);
    QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier,
                        combo.rect().center());
    QTRY_VERIFY(frameReal(&combo, "_winui_combo_chevron_progress") < -0.05);
    QTRY_VERIFY(qAbs(frameReal(&combo, "_winui_combo_chevron_progress"))
                < 0.01);
    combo.hidePopup();
}

void WinUI3EditorsTest::comboChevronGeometry()
{
    QComboBox combo;
    combo.addItems({QStringLiteral("One"), QStringLiteral("Two")});
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
            combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option,
                                              &painter, &combo);
        }

        const QRect logicalGlyphBox(option.rect.right() - 14 - 12 + 1,
                                    option.rect.top()
                                        + (option.rect.height() - 12) / 2,
                                    12, 12);
        const QRect logicalChevron(logicalGlyphBox.adjusted(1, 1, -1, -1));
        const QRect expected = QStyle::visualRect(direction, option.rect,
                                                  logicalChevron);
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
    QVERIFY(ltr.second.width() <= 9);
    QVERIFY(ltr.second.height() <= 6);
    QVERIFY(rtl.second.width() <= 9);
    QVERIFY(rtl.second.height() <= 6);
    QCOMPARE(ltr.first.size(), QSize(10, 10));
    QCOMPARE(rtl.first.size(), QSize(10, 10));
    QCOMPARE(ltr.first.center().y(), rtl.first.center().y());
    QCOMPARE(ltr.first.center().x() + rtl.first.center().x(),
             combo.rect().left() + combo.rect().right() - 1);
}

void WinUI3EditorsTest::numberBoxSubcontrolContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(150, 32);
    spin.show();

    auto *editor = spin.findChild<QLineEdit *>();
    QVERIFY(editor);
    QCOMPARE(editor->parentWidget(), &spin);
    QStyleOptionFrame editorOption;
    editorOption.initFrom(editor);
    editorOption.rect = QRect(0, 0, 60, 24);
    editorOption.state |= QStyle::State_MouseOver | QStyle::State_HasFocus;
    const QColor sentinel(17, 33, 49);
    QImage editorPanel(editorOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
    editorPanel.fill(sentinel);
    {
        QPainter painter(&editorPanel);
        spin.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &editorOption,
                                    &painter, editor);
        spin.style()->drawPrimitive(QStyle::PE_FrameLineEdit, &editorOption,
                                    &painter, editor);
    }
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.center()), sentinel);
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.topLeft()), sentinel);

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled
        | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
        | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                     QStyle::SC_SpinBoxEditField,
                                                     &spin);
    const QRect up = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                   QStyle::SC_SpinBoxUp, &spin);
    const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                     QStyle::SC_SpinBoxDown,
                                                     &spin);
    QCOMPARE(up.size(), QSize(36, spin.height()));
    QCOMPARE(down.size(), QSize(36, spin.height()));
    QCOMPARE(up.top(), down.top());
    QCOMPARE(up.right() + 1, down.left());
    QCOMPARE(edit.right() + 1, up.left());
    QVERIFY(spin.sizeHint().width() >= 120);
    QLineEdit lineEdit;
    QCOMPARE(spin.sizeHint().height(), qMax(32, lineEdit.sizeHint().height()));

    option.direction = Qt::RightToLeft;
    const QRect rtlEdit = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxEditField, &spin);
    const QRect rtlUp = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect rtlDown = spin.style()->subControlRect(
        QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxDown, &spin);
    QCOMPARE(rtlDown.right() + 1, rtlUp.left());
    QCOMPARE(rtlUp.right() + 1, rtlEdit.left());
    option.direction = Qt::LeftToRight;

    QImage focused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent)
            < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent)
            < 80);

    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), 47);
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, down.center());
    QCOMPARE(spin.value(), 46);
    spin.setValue(spin.maximum());
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), spin.maximum());
}

void WinUI3EditorsTest::verticalNumberBoxContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(150, 32);
    WinUI3::Style::setVerticalSpinButtons(&spin);
    QVERIFY(WinUI3::Style::hasVerticalSpinButtons(&spin));
    QCOMPARE(spin.property(WinUI3::Style::VerticalSpinButtonsProperty).toBool(), true);
    spin.show();

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled
        | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
        | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const auto geometry = [&](QStyle::SubControl control) {
        return spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                            control, &spin);
    };
    const QRect edit = geometry(QStyle::SC_SpinBoxEditField);
    const QRect up = geometry(QStyle::SC_SpinBoxUp);
    const QRect down = geometry(QStyle::SC_SpinBoxDown);
    QCOMPARE(up.size(), QSize(32, 16));
    QCOMPARE(down.size(), QSize(32, 16));
    QCOMPARE(up.left(), down.left());
    QCOMPARE(up.bottom() + 1, down.top());
    QCOMPARE(edit.right() + 1, up.left());
    QVERIFY(!up.intersects(down));

    option.direction = Qt::RightToLeft;
    const QRect rtlEdit = geometry(QStyle::SC_SpinBoxEditField);
    const QRect rtlUp = geometry(QStyle::SC_SpinBoxUp);
    const QRect rtlDown = geometry(QStyle::SC_SpinBoxDown);
    QCOMPARE(rtlUp.left(), rtlDown.left());
    QCOMPARE(rtlUp.bottom() + 1, rtlDown.top());
    QCOMPARE(rtlUp.right() + 1, rtlEdit.left());
    option.direction = Qt::LeftToRight;

    QImage focused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent)
            < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent)
            < 80);

    const int separatorX = edit.right();
    const QColor separator = focused.pixelColor(separatorX, spin.rect().center().y());
    QVERIFY(separator != focused.pixelColor(separatorX + 1,
                                             spin.rect().center().y()));

    QImage rtlFocused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    rtlFocused.fill(Qt::transparent);
    option.direction = Qt::RightToLeft;
    {
        QPainter painter(&rtlFocused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                         &painter, &spin);
    }
    const QRect rtlEditForSeparator = geometry(QStyle::SC_SpinBoxEditField);
    const int rtlSeparatorX = rtlEditForSeparator.left();
    QVERIFY(rtlFocused.pixelColor(rtlSeparatorX, spin.rect().center().y())
            != rtlFocused.pixelColor(rtlSeparatorX + 1,
                                     spin.rect().center().y()));

    option.state &= ~QStyle::State_HasFocus;
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, up.center());
    QCOMPARE(spin.value(), 47);
    QTest::mouseClick(&spin, Qt::LeftButton, Qt::NoModifier, down.center());
    QCOMPARE(spin.value(), 46);

    WinUI3::Style::setVerticalSpinButtons(&spin, false);
    QVERIFY(!WinUI3::Style::hasVerticalSpinButtons(&spin));
}

void WinUI3EditorsTest::spinBoxFocusUnderlinePixelContract()
{
    QSpinBox spin;
    spin.setRange(0, 100);
    spin.setValue(46);
    spin.resize(160, 40);
    spin.show();

    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green())
            + qAbs(a.blue() - b.blue());
    };
    const QColor accent = spin.palette().color(QPalette::Accent);
    const auto verify = [&](bool vertical, Qt::LayoutDirection direction) {
        WinUI3::Style::setVerticalSpinButtons(&spin, vertical);
        spin.setLayoutDirection(direction);
        QCoreApplication::processEvents();

        QStyleOptionSpinBox option;
        option.initFrom(&spin);
        option.rect = spin.rect();
        option.direction = direction;
        option.frame = true;
        option.buttonSymbols = spin.buttonSymbols();
        option.stepEnabled = QAbstractSpinBox::StepUpEnabled
            | QAbstractSpinBox::StepDownEnabled;
        option.subControls = QStyle::SC_SpinBoxFrame
            | QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxUp
            | QStyle::SC_SpinBoxDown;
        option.state |= QStyle::State_HasFocus;

        QImage image(spin.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(spin.palette().color(QPalette::Window));
        {
            QPainter painter(&image);
            spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option,
                                             &painter, &spin);
        }

        const int underlineY = spin.rect().bottom() - 1;
        QVERIFY2(distance(image.pixelColor(spin.rect().center().x(), underlineY),
                          accent) < 80,
                 vertical ? "vertical underline center missing"
                          : "horizontal underline center missing");
        // The rounded clip follows the WinUI TextBox/NumberBox outline: the
        // line is present a few pixels in from each end, but never paints the
        // two rounded bottom corners.
        QVERIFY(distance(image.pixelColor(3, underlineY), accent) < 100);
        QVERIFY(distance(image.pixelColor(spin.width() - 4, underlineY), accent)
                < 100);
        QVERIFY(distance(image.pixelColor(spin.rect().left(), underlineY), accent)
                > 80);
        QVERIFY(distance(image.pixelColor(spin.rect().right(), underlineY), accent)
                > 80);
    };

    verify(false, Qt::LeftToRight);
    verify(false, Qt::RightToLeft);
    verify(true, Qt::LeftToRight);
    verify(true, Qt::RightToLeft);
}

QTEST_MAIN(WinUI3EditorsTest)
#include "tst_winui3editors.moc"
