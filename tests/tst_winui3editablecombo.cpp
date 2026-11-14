// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3testhelpers.h"

#include "winui3density_p.h"

class WinUI3EditableComboTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void editableComboArrowHover();
    void editableComboEditorFocusActivatesOnRelease();
    void editableComboFocusedEditorPressKeepsFocusVisual();
    void mouseFocusForNonEditableControlsDoesNotAccessEditableCombo();
};

void WinUI3EditableComboTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3EditableComboTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3EditableComboTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3EditableComboTest::editableComboArrowHover()
{
    QWidget host;
    host.resize(360, 120);
    QComboBox combo(&host);
    combo.setEditable(true);
    combo.addItem(QStringLiteral("2416"));
    combo.setGeometry(120, 16, 200, 32);
    host.show();
    QTRY_VERIFY(host.isVisible());
    QVERIFY(combo.lineEdit());

    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    for (const WinUI3::DensityMode density :
         { WinUI3::DensityMode::Standard, WinUI3::DensityMode::Compact }) {
        WinUI3::Style::setDensityMode(&combo, density);
        const auto &metrics = WinUI3::Private::densityMetrics(density);
        combo.resize(200, metrics.textBoxHeight);
        QCOMPARE(combo.height(), metrics.textBoxHeight);
        for (const WinUI3::ThemeMode theme : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
            style->setThemeMode(theme);
            const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(combo.palette());
            for (const Qt::LayoutDirection direction : { Qt::LeftToRight, Qt::RightToLeft }) {
                combo.setLayoutDirection(direction);
                QStyleOptionComboBox option;
                option.initFrom(&combo);
                option.rect = combo.rect();
                option.direction = direction;
                option.editable = true;
                option.state = QStyle::State_Enabled | QStyle::State_HasFocus;
                option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
                const QRect arrow = combo.style()->subControlRect(
                        QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxArrow, &combo);
                QCOMPARE(arrow.height(), combo.height());
                QCOMPARE(direction == Qt::LeftToRight ? arrow.right() : arrow.left(),
                         direction == Qt::LeftToRight ? combo.rect().right() : combo.rect().left());

                const QPoint hoverPoint(arrow.left() + 5, arrow.center().y());
                const auto render = [&] {
                    QImage image(combo.size(), QImage::Format_ARGB32_Premultiplied);
                    image.fill(combo.palette().color(QPalette::Window));
                    QPainter painter(&image);
                    combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter,
                                                      &combo);
                    return image;
                };
                WinUI3::Private::framePropertyRegistry().set(
                        &combo, "_winui_editable_combo_arrow_hover", false);
                const QImage rest = render();
                QTest::mouseMove(&combo, hoverPoint);
                QCoreApplication::processEvents();
                QVERIFY(WinUI3::Private::framePropertyRegistry()
                                .value(&combo, "_winui_editable_combo_arrow_hover")
                                .toBool());
                const QImage hovered = render();
                const QColor restColor = rest.pixelColor(hoverPoint);
                QColor expectedHover = tokens.subtleHover;
                {
                    QImage mix(1, 1, QImage::Format_ARGB32_Premultiplied);
                    mix.fill(restColor);
                    QPainter painter(&mix);
                    painter.fillRect(mix.rect(), expectedHover);
                    painter.end();
                    expectedHover = mix.pixelColor(0, 0);
                }
                QVERIFY2(colorDistance(hovered.pixelColor(hoverPoint), restColor) > 6,
                         "editable arrow hover must paint a distinct overlay");
                QCOMPARE(colorDistance(hovered.pixelColor(hoverPoint), expectedHover) < 16, true);
            }

            combo.setEnabled(false);
            QCoreApplication::processEvents();
            QCOMPARE(WinUI3::Private::framePropertyRegistry()
                             .value(&combo, "_winui_editable_combo_arrow_hover")
                             .toBool(),
                     false);

            combo.setEnabled(true);
            combo.setLayoutDirection(Qt::RightToLeft);
            QStyleOptionComboBox popupOption;
            popupOption.initFrom(&combo);
            popupOption.rect = combo.rect();
            popupOption.direction = combo.layoutDirection();
            popupOption.editable = true;
            const QRect popupArrow = combo.style()->subControlRect(
                    QStyle::CC_ComboBox, &popupOption, QStyle::SC_ComboBoxArrow, &combo);
            QTest::mousePress(&combo, Qt::LeftButton, Qt::NoModifier, popupArrow.center());
            QVERIFY(!combo.view()->isVisible());
            QTest::mouseRelease(&combo, Qt::LeftButton, Qt::NoModifier, popupArrow.center());
            QTRY_VERIFY(combo.view()->isVisible());
            combo.hidePopup();
        }
    }
}

void WinUI3EditableComboTest::editableComboEditorFocusActivatesOnRelease()
{
    QWidget host;
    host.resize(360, 120);
    QComboBox combo(&host);
    combo.setEditable(true);
    combo.addItem(QStringLiteral("2416"));
    combo.setGeometry(120, 16, 200, 32);
    host.show();
    QTRY_VERIFY(host.isVisible());
    QVERIFY(combo.lineEdit());

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.editable = true;
    option.state = QStyle::State_Enabled | QStyle::State_HasFocus;
    option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
    const QRect editor = combo.style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                        QStyle::SC_ComboBoxEditField, &combo);
    QCOMPARE(combo.style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth, &option, &combo), 1);
    const QPoint editorPoint = editor.center();
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(combo.palette());
    const QPoint underlinePoint(combo.rect().center().x(), combo.rect().bottom() - 1);
    const auto render = [&] {
        QImage image(combo.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(combo.palette().color(QPalette::Window));
        QPainter painter(&image);
        combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
        return image;
    };

    QTest::mousePress(combo.lineEdit(), Qt::LeftButton, Qt::NoModifier,
                      combo.lineEdit()->mapFromParent(editorPoint));
    QVERIFY(WinUI3::Private::framePropertyRegistry()
                    .value(&combo, "_winui_editable_combo_editor_press_pending")
                    .toBool());
    const QImage held = render();
    QVERIFY2(colorDistance(held.pixelColor(underlinePoint), tokens.accentFill) > 48,
             "editor press must defer the focused underline until release");

    QTest::mouseRelease(combo.lineEdit(), Qt::LeftButton, Qt::NoModifier,
                        combo.lineEdit()->mapFromParent(editorPoint));
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);
    const QImage released = render();
    QCOMPARE(colorDistance(released.pixelColor(underlinePoint), tokens.accentFill) < 48, true);
}

void WinUI3EditableComboTest::editableComboFocusedEditorPressKeepsFocusVisual()
{
    QWidget host;
    host.resize(360, 120);
    QComboBox combo(&host);
    combo.setEditable(true);
    combo.addItem(QStringLiteral("2416"));
    combo.setGeometry(120, 16, 200, 32);
    host.show();
    QTRY_VERIFY(host.isVisible());
    QLineEdit *editor = combo.lineEdit();
    QVERIFY(editor);
    editor->setFocus(Qt::TabFocusReason);
    QTRY_VERIFY(editor->hasFocus());

    QStyleOptionComboBox option;
    option.initFrom(&combo);
    option.rect = combo.rect();
    option.editable = true;
    option.state = QStyle::State_Enabled | QStyle::State_HasFocus;
    option.subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxArrow;
    const QRect editorRect = combo.style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                            QStyle::SC_ComboBoxEditField, &combo);
    QCOMPARE(combo.style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth, &option, &combo), 1);
    const QPoint editorPoint = editorRect.center();
    const QPoint underlinePoint(combo.rect().center().x(), combo.rect().bottom() - 1);
    const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(combo.palette());
    const auto render = [&] {
        QImage image(combo.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(combo.palette().color(QPalette::Window));
        QPainter painter(&image);
        combo.style()->drawComplexControl(QStyle::CC_ComboBox, &option, &painter, &combo);
        return image;
    };

    QTest::mousePress(editor, Qt::LeftButton, Qt::NoModifier, editor->mapFromParent(editorPoint));
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);
    const QImage held = render();
    QCOMPARE(colorDistance(held.pixelColor(underlinePoint), tokens.accentFill) < 48, true);
    QTest::mouseRelease(editor, Qt::LeftButton, Qt::NoModifier, editor->mapFromParent(editorPoint));

    WinUI3::Private::framePropertyRegistry().set(
            &combo, "_winui_editable_combo_editor_press_pending", true);
    QEvent ungrab(QEvent::UngrabMouse);
    QCoreApplication::sendEvent(editor, &ungrab);
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);

    WinUI3::Private::framePropertyRegistry().set(
            &combo, "_winui_editable_combo_editor_press_pending", true);
    editor->setEnabled(false);
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);
    editor->setEnabled(true);

    WinUI3::Private::framePropertyRegistry().set(
            &combo, "_winui_editable_combo_editor_press_pending", true);
    QEvent hide(QEvent::Hide);
    QCoreApplication::sendEvent(&combo, &hide);
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);

    WinUI3::Private::framePropertyRegistry().set(
            &combo, "_winui_editable_combo_editor_press_pending", true);
    combo.setEnabled(false);
    QCoreApplication::processEvents();
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);
}

void WinUI3EditableComboTest::mouseFocusForNonEditableControlsDoesNotAccessEditableCombo()
{
    QWidget host;
    host.resize(360, 120);
    QLineEdit lineEdit(&host);
    lineEdit.setGeometry(16, 16, 160, 32);
    QComboBox combo(&host);
    combo.addItem(QStringLiteral("2416"));
    combo.setGeometry(192, 16, 152, 32);
    host.show();
    QTRY_VERIFY(host.isVisible());

    QFocusEvent lineFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&lineEdit, &lineFocus);
    QFocusEvent comboFocus(QEvent::FocusIn, Qt::MouseFocusReason);
    QCoreApplication::sendEvent(&combo, &comboFocus);
    QCOMPARE(WinUI3::Private::framePropertyRegistry()
                     .value(&combo, "_winui_editable_combo_editor_press_pending")
                     .toBool(),
             false);
}

QTEST_MAIN(WinUI3EditableComboTest)
#include "tst_winui3editablecombo.moc"
