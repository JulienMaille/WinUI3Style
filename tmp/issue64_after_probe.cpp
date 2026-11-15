#include <winui3style/winui3style.h>
#include "../src/lib/winui3frameproperties_p.h"
#include <QApplication>
#include <QCheckBox>
#include <QImage>
#include <QPainter>
#include <QStyleOptionButton>

static void setFrame(QObject *o, const char *n, const QVariant &v)
{
    WinUI3::Private::framePropertyRegistry().set(o, n, v);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setStyle(new WinUI3::Style(WinUI3::ThemeMode::Dark));
    QCheckBox toggle;
    WinUI3::Style::setToggleSwitch(&toggle);
    toggle.resize(80, 32);
    toggle.setChecked(true);
    setFrame(&toggle, "_winui_toggle_position", 1.0);
    setFrame(&toggle, "_winui_hover_progress", 1.0);
    setFrame(&toggle, "_winui_press_progress", 1.0);
    QStyleOptionButton option;
    option.initFrom(&toggle);
    option.rect = toggle.rect();
    option.state = QStyle::State_Enabled | QStyle::State_On | QStyle::State_MouseOver
            | QStyle::State_Sunken;
    QImage image(toggle.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(35, 39, 50));
    { QPainter p(&image); toggle.style()->drawControl(QStyle::CE_CheckBox, &option, &p, &toggle); }
    image.save(QStringLiteral("after_native.png"));
    return 0;
}