#include <winui3style/toggleswitch.h>

#include <winui3style/winui3style.h>

namespace WinUI3 {

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QCheckBox(parent)
{
    Style::setToggleSwitch(this);
}

ToggleSwitch::ToggleSwitch(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    Style::setToggleSwitch(this);
}

QString ToggleSwitch::onText() const
{
    return property(Style::ToggleSwitchOnTextProperty).toString();
}

void ToggleSwitch::setOnText(const QString &text)
{
    setProperty(Style::ToggleSwitchOnTextProperty, text);
}

QString ToggleSwitch::offText() const
{
    return property(Style::ToggleSwitchOffTextProperty).toString();
}

void ToggleSwitch::setOffText(const QString &text)
{
    setProperty(Style::ToggleSwitchOffTextProperty, text);
}

} // namespace WinUI3
