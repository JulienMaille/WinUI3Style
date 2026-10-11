#pragma once

#include <QMainWindow>

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

    Ui::GalleryWindow *ui;
};
