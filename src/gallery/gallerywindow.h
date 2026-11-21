#pragma once

#include <QList>
#include <QMainWindow>
#include <QString>

class QDockWidget;
class QLabel;
class QStyle;

namespace Ui {
class GalleryWindow;
}

class GalleryWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit GalleryWindow(QWidget *parent = nullptr, bool captureMode = false);
    ~GalleryWindow() override;
    bool saveSnapshots(const QString &directory, const QString &backdrop = QStringLiteral("none"));

protected:
    void changeEvent(QEvent *event) override;

private:
    // Icons page: a QLabel holds a raster, so each preview keeps the
    // QStyle::StandardPixmap value it was rendered from and re-renders on a
    // palette or style change.
    struct IconPreview
    {
        QLabel *label = nullptr;
        // Qt Windows 11 reference drawn in the same cell, left of the active
        // style's glyph. Null when the windows11 style plugin is unavailable.
        QLabel *referenceLabel = nullptr;
        int standardPixmap = 0;
    };

    void configureGallery();
    void refreshCardIcons();
    void populateCollections();
    void populateIconSheet();
    void refreshIconSheet();
    void configurePaletteLab(bool captureMode);
    void setTheme(int index);
    void setComparisonStyle(const QString &key);

    Ui::GalleryWindow *ui;
    QList<QDockWidget *> m_demoDocks;
    QList<IconPreview> m_iconPreviews;
    // Style the Icons page was built against. Child widgets of a visible page
    // receive their own StyleChange while the application style has not moved,
    // so the pointer, not the event, decides whether the sheet is rebuilt
    // (rebuilding from those events would recurse through layout/show/polish).
    QStyle *m_iconSheetStyle = nullptr;
    QStyle *m_iconReferenceStyle = nullptr; // Qt reference style, owned by this window
    QString m_iconReferenceStyleName;
    bool m_iconSheetRebuildPending = false;
    int m_themeIndex = 0;
    int m_densityIndex = 0;
};
