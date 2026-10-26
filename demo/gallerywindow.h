#pragma once

#include <QMainWindow>
#include <QString>

namespace Ui {
class GalleryWindow;
}

class GalleryWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit GalleryWindow(QWidget *parent = nullptr);
    ~GalleryWindow() override;
    bool saveSnapshots(const QString &directory, const QString &backdrop = QStringLiteral("none"));

private:
    void configureGallery();
    void populateCollections();
    void configurePaletteLab();
    void setTheme(int index);
    void setComparisonStyle(const QString &key);

    Ui::GalleryWindow *ui;
    int m_themeIndex = 0;
    int m_densityIndex = 0;
};
