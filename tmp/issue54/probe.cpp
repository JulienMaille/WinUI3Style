// Issue #54 probe: who puts an icon on a QMessageBox button?
// Builds a QDialogButtonBox/QMessageBox under the winui3 style, once with the
// native platform hint and once with a proxy that reports the KDE hint value
// (ShowIconsOnPushButtons defaults to true on KDE Plasma).
#include <QApplication>
#include <QDialogButtonBox>
#include <QFile>
#include <QMessageBox>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyleFactory>
#include <QTextStream>

class KdeHintProxy final : public QProxyStyle
{
public:
    explicit KdeHintProxy(QStyle *base) : QProxyStyle(base) {}

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *returnData) const override
    {
        if (hint == QStyle::SH_DialogButtonBox_ButtonsHaveIcons)
            return 1; // KDE: ShowIconsOnPushButtons=true
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

static void dump(QTextStream &out, const char *label, QStyle *style)
{
    const QIcon yes = style->standardIcon(QStyle::SP_DialogYesButton);
    const QIcon no = style->standardIcon(QStyle::SP_DialogNoButton);
    const QIcon save = style->standardIcon(QStyle::SP_DialogSaveButton);
    out << label << " hint="
        << style->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)
        << " yes=" << yes.name() << " no=" << no.name() << " save=" << save.name()
        << " yesIsSave=" << (yes.cacheKey() == save.cacheKey()) << "\n";

    QMessageBox box(QMessageBox::Warning, QStringLiteral("probe"), QStringLiteral("text"),
                    QMessageBox::Yes | QMessageBox::No);
    box.ensurePolished();
    for (QAbstractButton *button : box.buttons()) {
        const QIcon icon = button->icon();
        out << "  button " << button->text() << " iconNull=" << icon.isNull()
            << " name=" << icon.name() << "\n";
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QFile file(QStringLiteral("probe.out"));
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    QStyle *winui = QStyleFactory::create(QStringLiteral("winui3"));
    if (!winui) {
        out << "winui3 style plugin not found\n";
        return 2;
    }
    QApplication::setStyle(winui); // app style, as in a real WinUI3Style app
    dump(out, "native hint:", winui);

    QStyle *proxy = new KdeHintProxy(QStyleFactory::create(QStringLiteral("winui3")));
    QApplication::setStyle(proxy);
    dump(out, "KDE hint   :", proxy);
    return 0;
}

