#pragma once

#include <QStylePlugin>

class WinUI3StylePlugin final : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "winui3style.json")

public:
    // The plugin advertises both the default and compact density aliases.
    QStyle *create(const QString &key) override;
};
