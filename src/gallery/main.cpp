#include "gallerywindow.h"

#include <winui3style/winui3style.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFont>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("WinUI 3 Style Gallery"));
    application.setOrganizationName(QStringLiteral("WinUI3Style"));
#if defined(EMSCRIPTEN)
    // Qt for WebAssembly ships without a runtime plugin loader, so the style
    // is linked into the executable instead of resolved by QStyleFactory.
    QStyle *style = new WinUI3::Style(WinUI3::ThemeMode::Light);
#else
    QStyle *style = QStyleFactory::create(QStringLiteral("winui3"));
    if (!style) {
        qCritical() << "The winui3 QStyle plugin could not be loaded.";
        return 1;
    }
#endif
    application.setStyle(style);
    // Give the gallery app an icon from the style's own Fluent glyphs;
    // no bundled image or resource file is needed.
    application.setWindowIcon(style->standardIcon(QStyle::SP_DirHomeIcon));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("WinUI 3 Qt Widgets gallery"));
    parser.addHelpOption();
    QCommandLineOption captureOption(QStringLiteral("capture-dir"),
                                     QStringLiteral("Write deterministic light/dark gallery PNGs."),
                                     QStringLiteral("directory"));
    parser.addOption(captureOption);
    QCommandLineOption fontPixelSizeOption(
            QStringLiteral("font-pixel-size"),
            QStringLiteral("Override the application font size for visual QA."),
            QStringLiteral("pixels"));
    parser.addOption(fontPixelSizeOption);
    QCommandLineOption backdropOption(QStringLiteral("backdrop"),
                                      QStringLiteral("Backdrop type: none or mica."),
                                      QStringLiteral("type"), QStringLiteral("none"));
    parser.addOption(backdropOption);
    parser.process(application);

    if (parser.isSet(fontPixelSizeOption)) {
        bool valid = false;
        const int pixels = parser.value(fontPixelSizeOption).toInt(&valid);
        if (!valid || pixels < 8 || pixels > 48) {
            qCritical() << "--font-pixel-size must be an integer from 8 to 48";
            return 1;
        }
        QFont font = application.font();
        font.setPixelSize(pixels);
        application.setFont(font);
    }

    const bool captureMode = parser.isSet(captureOption);
    if (captureMode) {
        // Gallery construction must not schedule hover, caret, popup, or
        // navigation animations before the deterministic capture policy is in
        // force.
        qputenv("WINUI3STYLE_DISABLE_ANIMATIONS", "1");
        QApplication::setCursorFlashTime(0);
    }

    GalleryWindow window(nullptr, captureMode);
    window.resize(1180, 780);
    window.show();
    if (captureMode) {
        // A persistent QTimer instead of QTimer::singleShot: the transient
        // single-shot timer can leave the dispatcher unable to deliver timers
        // created later in the capture (the settle waits), hanging offscreen
        // runs forever. Keep this timer alive for the whole capture.
        auto *startCapture = new QTimer(&application);
        startCapture->setSingleShot(true);
        QObject::connect(startCapture, &QTimer::timeout,
                         [&application, &window, &parser, captureOption, backdropOption] {
                             const bool saved = window.saveSnapshots(parser.value(captureOption),
                                                                     parser.value(backdropOption));
                             application.exit(saved ? 0 : 2);
                         });
        startCapture->start(250);
    }
    return application.exec();
}
