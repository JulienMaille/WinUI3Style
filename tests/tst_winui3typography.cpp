// SPDX-License-Identifier: LGPL-2.1-or-later
#include <QtTest>

#include "winui3tokens_p.h"
#include "winui3typography_p.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>

using namespace WinUI3::Private;

class WinUI3TypographyTest : public QObject
{
    Q_OBJECT

private slots:
    void winuiTypographyFontContract();
};

// Issue #112: Title/Subtitle/BodyStrong inherit XamlAutoFontFamily + SemiBold
// from BaseTextBlockStyle (TextBlock_themeresources.xaml, microsoft-ui-xaml
// 9f89c2da). The old recipe put DemiBold on the Text named instance, which has
// no Semibold face and resolves to Bold; the pinned generic family resolves the
// same weight to the exact Semibold face and maps size onto the optical axis.
void WinUI3TypographyTest::winuiTypographyFontContract()
{
    const QString generic = winuiVariableFontFamily();
    if (generic != QLatin1String(VariableFontFamily))
        QSKIP("Segoe UI Variable is not installed on this host");

    const QByteArray previousOverride = qgetenv("WINUI3STYLE_APP_FONT");
    qunsetenv("WINUI3STYLE_APP_FONT");

    struct Role
    {
        QString name;
        int pixelSize;
    };
    const QList<Role> roles = {
        { QStringLiteral("title"), TitleFontSize },
        { QStringLiteral("subtitle"), SubtitleFontSize },
        { QStringLiteral("body-strong"), BodyStrongFontSize },
    };
    for (const Role &role : roles) {
        const QFont font = winuiStrongFont(QFont(QLatin1String(TextFontFamily)), role.pixelSize);
        const QFontInfo info(font);
        QCOMPARE(font.pixelSize(), role.pixelSize);
        QCOMPARE(info.family(), generic);
        QVERIFY2(info.exactMatch(),
                 qPrintable(role.name + QLatin1String(": ") + info.family() + QLatin1Char('/')
                            + info.styleName()));
        // FreeType (offscreen) reports the instance as "Semibold Small";
        // DirectWrite reports "Semibold". Both are the exact 600 face.
        QVERIFY2(info.styleName().contains(QLatin1String("semibold"), Qt::CaseInsensitive),
                 qPrintable(role.name + QLatin1String(": ") + info.styleName()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        QVERIFY(font.isVariableAxisSet(QFont::Tag("opsz")));
        QCOMPARE(font.variableAxisValue(QFont::Tag("opsz")), static_cast<float>(role.pixelSize));
#endif

        // RED mechanism: the recipe this fix replaces resolves to the wrong
        // face on the same host, so the test proves why the family matters.
        QFont old = QFont(QLatin1String(TextFontFamily));
        old.setPixelSize(role.pixelSize);
        old.setWeight(QFont::DemiBold);
        const QFontInfo oldInfo(old);
        QVERIFY2(!oldInfo.exactMatch(), "Text family must not have an exact Semibold face");
        QVERIFY2(oldInfo.styleName().contains(QLatin1String("bold"), Qt::CaseInsensitive),
                 qPrintable(role.name + QLatin1String(": old ") + oldInfo.styleName()));
    }

    // An application-provided family stays its own: only the style's default
    // families are promoted to the generic variable family.
    const QFont custom = winuiStrongFont(QFont(QStringLiteral("Arial")), BodyStrongFontSize);
    QCOMPARE(custom.family(), QStringLiteral("Arial"));
    QCOMPARE(custom.pixelSize(), BodyStrongFontSize);

    if (previousOverride.isEmpty())
        qunsetenv("WINUI3STYLE_APP_FONT");
    else
        qputenv("WINUI3STYLE_APP_FONT", previousOverride);
}

QTEST_MAIN(WinUI3TypographyTest)
#include "tst_winui3typography.moc"
