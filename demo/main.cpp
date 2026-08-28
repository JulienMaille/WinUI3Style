#include "gallerywindow.h"

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("WinUI 3 Style Gallery"));
    application.setOrganizationName(QStringLiteral("WinUI3Style"));
    application.setStyle(new WinUI3::Style(WinUI3::ThemeMode::System));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("WinUI 3 Qt Widgets gallery"));
    parser.addHelpOption();
    QCommandLineOption captureOption(QStringLiteral("capture-dir"),
                                     QStringLiteral("Write deterministic light/dark gallery PNGs."),
                                     QStringLiteral("directory"));
    parser.addOption(captureOption);
    parser.process(application);

    const bool captureMode = parser.isSet(captureOption);
    if (captureMode) {
        // Gallery construction must not schedule hover, caret, popup, or
        // navigation animations before the deterministic capture policy is in
        // force.
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
        QApplication::setCursorFlashTime(0);
    }

    GalleryWindow window;
    window.resize(1180, 780);
    WinUI3::applyBackdrop(&window, WinUI3::Backdrop::Mica);
    window.show();
    if (captureMode) {
        QTimer::singleShot(250, &application, [&application, &window, &parser, captureOption] {
            const bool saved = window.saveSnapshots(parser.value(captureOption));
            application.exit(saved ? 0 : 2);
        });
    }
    return application.exec();
}
