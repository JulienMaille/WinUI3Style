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
    void indeterminateProgressDeterminism();
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
    const QRect ltrContents =
            edit.style()->subElementRect(QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(ltrContents, edit.rect().adjusted(10, 5, -6, -6));
    option.direction = Qt::RightToLeft;
    const QRect rtlContents =
            edit.style()->subElementRect(QStyle::SE_LineEditContents, &option, &edit);
    QCOMPARE(rtlContents, edit.rect().adjusted(6, 5, -10, -6));
    option.direction = Qt::LeftToRight;

    QVERIFY(!edit.style()->standardIcon(QStyle::SP_LineEditClearButton, nullptr, &edit).isNull());
    QTRY_VERIFY(!edit.findChildren<QAbstractButton *>().isEmpty());
    const auto *clearButton = edit.findChildren<QAbstractButton *>().constFirst();
    QStyleOptionToolButton clearOption;
    clearOption.initFrom(clearButton);
    QCOMPARE(edit.style()->sizeFromContents(QStyle::CT_ToolButton, &clearOption, QSize(16, 16),
                                            clearButton),
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
        edit.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, &edit);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QColor accent = edit.palette().color(QPalette::Accent);
#else
    const QColor accent = edit.palette().color(QPalette::Highlight);
#endif
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
    QVERIFY(distance(focused.pixelColor(edit.rect().center().x(), edit.rect().bottom() - 1), accent)
            < 80);
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
    QVERIFY2(helperRect.isValid(),
             qPrintable(QString::fromLatin1("private clear button has no editor intersection")));
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
    for (const QPoint &corner : { surfaceRect.topLeft(), surfaceRect.topRight(),
                                  surfaceRect.bottomLeft(), surfaceRect.bottomRight() })
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
            const int delta = std::max({ qAbs(before.red() - after.red()),
                                         qAbs(before.green() - after.green()),
                                         qAbs(before.blue() - after.blue()) });
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
             qPrintable(QStringLiteral("maxDelta=%1").arg(maximumChannelDelta)));
    QTest::mouseMove(&edit, QPoint(8, edit.rect().center().y()));
    QTRY_COMPARE(frameReal(clearButton, "_winui_hover_progress"), 0.0);
    setFrame(clearButton, "_winui_hover_progress", 0.0);
    setFrame(clearButton, "_winui_press_progress", 0.0);

    parentRepaint.updateRequests = 0;
    QTest::mousePress(clearButton, Qt::LeftButton, Qt::NoModifier, clearButton->rect().center());
    QTRY_COMPARE(frameReal(clearButton, "_winui_press_progress"), 1.0);
    QCoreApplication::processEvents();
    QVERIFY(parentRepaint.updateRequests > 0);
    const QImage runtimePressed = edit.grab().toImage();
    QVERIFY(runtimeHover != runtimePressed);
    QTest::mouseRelease(clearButton, Qt::LeftButton, Qt::NoModifier, clearButton->rect().center());

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
        style->drawPrimitive(QStyle::PE_PanelButtonTool, &option, &painter, clearButton);
        style->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, clearButton);
        return image;
    };

    const QImage normal = render(QStyle::State_Enabled, 0.0, 0.0);
    const QImage pointerOver = render(QStyle::State_Enabled | QStyle::State_MouseOver, 1.0, 0.0);
    const QImage pressed = render(
            QStyle::State_Enabled | QStyle::State_MouseOver | QStyle::State_Sunken, 1.0, 1.0);
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
        style->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, clearButton);
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
        edit.style()->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, clearButton);
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
                                .arg(ink.x())
                                .arg(ink.y())
                                .arg(ink.width())
                                .arg(ink.height())));
    QVERIFY2(qAbs(ink.center().y() - officialCenter.y()) <= 2.0,
             qPrintable(QStringLiteral("ink=%1,%2 %3x%4")
                                .arg(ink.x())
                                .arg(ink.y())
                                .arg(ink.width())
                                .arg(ink.height())));

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
        edit.style()->drawControl(QStyle::CE_ToolButtonLabel, &liveOption, &painter, clearButton);
    }
    QRect liveInk;
    for (int y = 0; y < liveGlyph.height(); ++y)
        for (int x = 0; x < liveGlyph.width(); ++x)
            if (liveGlyph.pixelColor(x, y).alpha() > 24)
                liveInk |= QRect(x, y, 1, 1);
    QVERIFY(!liveInk.isEmpty());
    const QPointF inkCenter =
            QPointF(clearButton->geometry().topLeft()) + QPointF(liveInk.center());
    const qreal surfaceTop = edit.rect().top() + 3.0;
    const qreal surfaceBottom = edit.rect().bottom() - 2.0;
    const qreal surfaceSide = surfaceBottom - surfaceTop;
    const QPointF surfaceCenter(edit.rect().right() - 2.0 - surfaceSide / 2.0,
                                (surfaceTop + surfaceBottom) / 2.0);
    QVERIFY2(qAbs(inkCenter.x() - surfaceCenter.x()) <= 1.0,
             qPrintable(QStringLiteral("ink x=%1 surface x=%2")
                                .arg(inkCenter.x())
                                .arg(surfaceCenter.x())));
    QVERIFY2(qAbs(inkCenter.y() - surfaceCenter.y()) <= 1.0,
             qPrintable(QStringLiteral("ink y=%1 surface y=%2")
                                .arg(inkCenter.y())
                                .arg(surfaceCenter.y())));
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

    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        WinUI3::Style style(mode);
        const QPalette palette = style.standardPalette();
        QCOMPARE(palette.color(QPalette::Highlight), style.accentColor());
        const QColor expectedOnAccent =
                qGray(style.accentColor().rgb()) < 128 ? QColor(Qt::white) : QColor(Qt::black);
        QCOMPARE(palette.color(QPalette::HighlightedText), expectedOnAccent);
        QVERIFY(palette.color(QPalette::Disabled, QPalette::Text).alpha()
                < palette.color(QPalette::Active, QPalette::Text).alpha());
        QVERIFY(palette.color(QPalette::Disabled, QPalette::PlaceholderText).alpha()
                < palette.color(QPalette::Active, QPalette::PlaceholderText).alpha());

        const QImage normal = renderPanel(style, QStyle::State_Enabled);
        const QImage focused = renderPanel(style, QStyle::State_Enabled | QStyle::State_HasFocus);
        const QImage disabled = renderPanel(style, QStyle::State_None);
        QVERIFY(normal != focused);
        QVERIFY(normal != disabled);
        QCOMPARE(disabled.pixelColor(disabled.rect().center().x(), disabled.rect().bottom() - 1),
                 disabled.pixelColor(4, disabled.rect().bottom() - 1));
        QVERIFY(focused.pixelColor(focused.rect().center().x(), focused.rect().bottom() - 1)
                != disabled.pixelColor(disabled.rect().center().x(), disabled.rect().bottom() - 1));
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
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::Text), QColor(0, 0, 0, 92));
    QCOMPARE(disabled.palette().color(QPalette::Disabled, QPalette::PlaceholderText),
             QColor(0, 0, 0, 92));
}

void WinUI3EditorsTest::indeterminateProgressDeterminism()
{
    const bool animationSettingExisted =
            qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
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
        spin.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &editorOption, &painter, editor);
        spin.style()->drawPrimitive(QStyle::PE_FrameLineEdit, &editorOption, &painter, editor);
    }
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.center()), sentinel);
    QCOMPARE(editorPanel.pixelColor(editorOption.rect.topLeft()), sentinel);

    QStyleOptionSpinBox option;
    option.initFrom(&spin);
    option.rect = spin.rect();
    option.frame = true;
    option.buttonSymbols = spin.buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
            | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const QRect edit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxEditField, &spin);
    const QRect up =
            spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect down = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                    QStyle::SC_SpinBoxDown, &spin);
    QCOMPARE(up.size(), QSize(36, spin.height()));
    QCOMPARE(down.size(), QSize(36, spin.height()));
    QCOMPARE(up.top(), down.top());
    QCOMPARE(up.right() + 1, down.left());
    QCOMPARE(edit.right() + 1, up.left());
    QVERIFY(spin.sizeHint().width() >= 120);
    QLineEdit lineEdit;
    QCOMPARE(spin.sizeHint().height(), qMax(32, lineEdit.sizeHint().height()));

    option.direction = Qt::RightToLeft;
    const QRect rtlEdit = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                       QStyle::SC_SpinBoxEditField, &spin);
    const QRect rtlUp =
            spin.style()->subControlRect(QStyle::CC_SpinBox, &option, QStyle::SC_SpinBoxUp, &spin);
    const QRect rtlDown = spin.style()->subControlRect(QStyle::CC_SpinBox, &option,
                                                       QStyle::SC_SpinBoxDown, &spin);
    QCOMPARE(rtlDown.right() + 1, rtlUp.left());
    QCOMPARE(rtlUp.right() + 1, rtlEdit.left());
    option.direction = Qt::LeftToRight;

    QImage focused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    focused.fill(Qt::transparent);
    option.state |= QStyle::State_HasFocus;
    {
        QPainter painter(&focused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QColor accent = spin.palette().color(QPalette::Accent);
#else
    const QColor accent = spin.palette().color(QPalette::Highlight);
#endif
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent) < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent) < 80);

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
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
            | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

    const auto geometry = [&](QStyle::SubControl control) {
        return spin.style()->subControlRect(QStyle::CC_SpinBox, &option, control, &spin);
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
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QColor accent = spin.palette().color(QPalette::Accent);
#else
    const QColor accent = spin.palette().color(QPalette::Highlight);
#endif
    const auto distance = [](const QColor &a, const QColor &b) {
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
    const int underlineY = spin.rect().bottom() - 1;
    QVERIFY(distance(focused.pixelColor(edit.center().x(), underlineY), accent) < 80);
    QVERIFY(distance(focused.pixelColor(down.center().x(), underlineY), accent) < 80);

    const int separatorX = edit.right();
    const QColor separator = focused.pixelColor(separatorX, spin.rect().center().y());
    QVERIFY(separator != focused.pixelColor(separatorX + 1, spin.rect().center().y()));

    QImage rtlFocused(spin.size(), QImage::Format_ARGB32_Premultiplied);
    rtlFocused.fill(Qt::transparent);
    option.direction = Qt::RightToLeft;
    {
        QPainter painter(&rtlFocused);
        spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
    }
    const QRect rtlEditForSeparator = geometry(QStyle::SC_SpinBoxEditField);
    const int rtlSeparatorX = rtlEditForSeparator.left();
    QVERIFY(rtlFocused.pixelColor(rtlSeparatorX, spin.rect().center().y())
            != rtlFocused.pixelColor(rtlSeparatorX + 1, spin.rect().center().y()));

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
        return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
    };
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    const QColor accent = spin.palette().color(QPalette::Accent);
#else
    const QColor accent = spin.palette().color(QPalette::Highlight);
#endif
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
        option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
        option.subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField
                | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
        option.state |= QStyle::State_HasFocus;

        QImage image(spin.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(spin.palette().color(QPalette::Window));
        {
            QPainter painter(&image);
            spin.style()->drawComplexControl(QStyle::CC_SpinBox, &option, &painter, &spin);
        }

        const int underlineY = spin.rect().bottom() - 1;
        QVERIFY2(distance(image.pixelColor(spin.rect().center().x(), underlineY), accent) < 80,
                 vertical ? "vertical underline center missing"
                          : "horizontal underline center missing");
        // The rounded clip follows the WinUI TextBox/NumberBox outline: the
        // line is present a few pixels in from each end, but never paints the
        // two rounded bottom corners.
        QVERIFY(distance(image.pixelColor(3, underlineY), accent) < 100);
        QVERIFY(distance(image.pixelColor(spin.width() - 4, underlineY), accent) < 100);
        QVERIFY(distance(image.pixelColor(spin.rect().left(), underlineY), accent) > 80);
        QVERIFY(distance(image.pixelColor(spin.rect().right(), underlineY), accent) > 80);
    };

    verify(false, Qt::LeftToRight);
    verify(false, Qt::RightToLeft);
    verify(true, Qt::LeftToRight);
    verify(true, Qt::RightToLeft);
}

QTEST_MAIN(WinUI3EditorsTest)
#include "tst_winui3editors.moc"
