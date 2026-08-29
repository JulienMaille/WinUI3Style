#pragma once

#include <QMainWindow>

namespace WinUI3 { class NavigationView; }

class GalleryWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit GalleryWindow(QWidget *parent = nullptr);
    bool saveSnapshots(const QString &directory);

private:
    QWidget *controlsPage();
    QWidget *collectionsPage();
    QWidget *settingsPage();
    QWidget *dialogsPage();
    QWidget *palettePage();
    void setTheme(int index);

    WinUI3::NavigationView *m_navigation = nullptr;
};
