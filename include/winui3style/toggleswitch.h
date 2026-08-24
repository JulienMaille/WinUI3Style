#pragma once

#include <winui3style/winui3global.h>

#include <QCheckBox>

namespace WinUI3 {

class WINUI3STYLE_EXPORT ToggleSwitch final : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(QString onText READ onText WRITE setOnText)
    Q_PROPERTY(QString offText READ offText WRITE setOffText)

public:
    explicit ToggleSwitch(QWidget *parent = nullptr);
    explicit ToggleSwitch(const QString &text, QWidget *parent = nullptr);

    QString onText() const;
    void setOnText(const QString &text);

    QString offText() const;
    void setOffText(const QString &text);
};

} // namespace WinUI3
