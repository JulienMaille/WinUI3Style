// SPDX-License-Identifier: LGPL-2.1-or-later
// Property-memo invalidation contracts split out of tst_winui3buttons.cpp
// to respect the 60 KB test-size ratchet.
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QProxyStyle>
#include <QPushButton>
#include <QtTest>

#include "winui3testhelpers.h"

class DeclaredRoleButton final : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QString winuiControlRole MEMBER role)

public:
    QString role = QStringLiteral("accent");
};

class WinUI3PaintMemoTest final : public QObject, private WinUI3StandardTestFixture
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void roleReadsTrackUnobservedWidgets();
    void declaredRoleRemainsLive();
};

void WinUI3PaintMemoTest::initTestCase()
{
    initializeStandardWinUI3TestStyle();
}

void WinUI3PaintMemoTest::init()
{
    resetStandardWinUI3TestStyle();
}

void WinUI3PaintMemoTest::cleanup()
{
    cleanupStandardWinUI3TestWidgets();
}

void WinUI3PaintMemoTest::roleReadsTrackUnobservedWidgets()
{
    using WinUI3::ControlRole;
    using WinUI3::Style;
    QProxyStyle otherStyle;
    QPushButton button;
    QVERIFY(!button.testAttribute(Qt::WA_WState_Polished));
    button.setProperty(Style::ControlRoleProperty, QStringLiteral("accent"));
    QCOMPARE(Style::controlRole(&button), ControlRole::Accent);
    button.setProperty(Style::ControlRoleProperty, QStringLiteral("destructive"));
    QCOMPARE(Style::controlRole(&button), ControlRole::Destructive);
    button.setProperty(Style::ControlRoleProperty, {});
    QCOMPARE(Style::controlRole(&button), ControlRole::Standard);

    // A polished widget can belong to another style that does not install
    // WinUI3Style's invalidation filter.
    button.setStyle(&otherStyle);
    button.ensurePolished();
    QVERIFY(button.testAttribute(Qt::WA_WState_Polished));
    QCOMPARE(Style::controlRole(&button), ControlRole::Standard);
    button.setProperty(Style::ControlRoleProperty, QStringLiteral("subtle"));
    QCOMPARE(Style::controlRole(&button), ControlRole::Subtle);

    button.setStyle(nullptr);
    QCOMPARE(Style::controlRole(&button), ControlRole::Subtle);
    button.setStyle(&otherStyle);
    button.setProperty(Style::ControlRoleProperty, QStringLiteral("accent"));
    QCOMPARE(Style::controlRole(&button), ControlRole::Accent);
    button.setStyle(nullptr);
}

void WinUI3PaintMemoTest::declaredRoleRemainsLive()
{
    using WinUI3::ControlRole;
    using WinUI3::Style;
    DeclaredRoleButton button;
    button.ensurePolished();
    QCOMPARE(Style::controlRole(&button), ControlRole::Accent);
    // Writes to a declared property need not deliver DynamicPropertyChange.
    button.role = QStringLiteral("destructive");
    QCOMPARE(Style::controlRole(&button), ControlRole::Destructive);
}

QTEST_MAIN(WinUI3PaintMemoTest)
#include "tst_winui3paintmemo.moc"
