// SPDX-License-Identifier: LGPL-2.1-or-later
// Native live repro of the reported Gallery regression (2026-10-12):
// Dark + Mica ON, switch to Light while granted, then Mica OFF must leave
// the navigation pane and client chrome on the Light theme, opaque. Drives
// the REAL GalleryWindow toggle path and the REAL DWM grant on the windows
// platform (skipped honestly offscreen). Companion to the offscreen recipe
// tests; the verdict here is presented pixels, not palette roles.
#include "../src/gallery/gallerywindow.h"

#include "winui3testhelpers.h"

#include "../src/lib/winui3backdrop_p.h"
#include "../src/lib/winui3surfaces_p.h"
#include "../src/lib/winui3tokens_p.h"

#include <QAction>
#include <QBackingStore>
#include <QCheckBox>
#include <QDockWidget>
#include <QComboBox>
#include <QGuiApplication>
#include <QListWidget>
#include <QMenuBar>
#include <QPaintEvent>
#include <QScrollArea>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif
#include <QLibrary>
#include <QWindow>

namespace {

class BackingStorePixelProbe final : public QObject
{
public:
    BackingStorePixelProbe(QWidget *widget, QPoint sample) : m_sample(sample)
    {
        widget->installEventFilter(this);
    }

    QColor color() const { return m_color; }
    void reset() { m_color = QColor(); }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() != QEvent::Paint
            || !static_cast<QPaintEvent *>(event)->region().contains(m_sample))
            return false;

        // Qt has painted the widget background and the paint device is still valid.
        const auto *widget = static_cast<QWidget *>(watched);
        QBackingStore *store = widget->backingStore();
        const QPaintDevice *device = store ? store->paintDevice() : nullptr;
        if (device && device->devType() == QInternal::Image) {
            const auto *image = static_cast<const QImage *>(device);
            const QPoint pixel =
                    widget->mapTo(widget->window(), m_sample) * widget->devicePixelRatioF();
            if (image->rect().contains(pixel))
                m_color = image->pixelColor(pixel);
        }
        return false;
    }

private:
    QPoint m_sample;
    QColor m_color;
};

} // namespace

class WinUI3MicaNativeTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void liveThemeSwitchWhileGrantedThenDisable();
    void dockWidgetsDemoRemainsAvailable();
    void popupGrantAliveRejectsRevokedEffect();
    void micaToggleClearsVacatedClientPixels_data();
    void micaToggleClearsVacatedClientPixels();
};

void WinUI3MicaNativeTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Dark));
}

void WinUI3MicaNativeTest::liveThemeSwitchWhileGrantedThenDisable()
{
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("requires the live DWM compositor");
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *navigation = gallery.findChild<QListWidget *>(QStringLiteral("navigationList"));
    auto *micaToggle = gallery.findChild<QCheckBox *>(QStringLiteral("micaToggle"));
    auto *themeCombo = gallery.findChild<QComboBox *>(QStringLiteral("themeCombo"));
    QVERIFY(navigation && micaToggle);
    // The Gallery theme switcher may be absent under some .ui revisions;
    // fall back to the style API (same property the combo writes).
    if (!themeCombo)
        qWarning() << "themeCombo not found, driving themeMode directly";

    // Dark + Mica ON through the Gallery's own toggle path.
    micaToggle->setChecked(true);
    QTRY_VERIFY_WITH_TIMEOUT(WinUI3::Private::backdropEffectiveSurface(&gallery)
                                     == WinUI3::Private::BackdropSurface::Composited,
                             5000);
    QCOMPARE(gallery.palette().color(QPalette::Window).alpha(), 0);

    // Switch to Light while granted, exactly like the live report.
    if (themeCombo)
        themeCombo->setCurrentIndex(themeCombo->findText(QStringLiteral("Light")));
    else
        style->setThemeMode(WinUI3::ThemeMode::Light);
    QTest::qWait(500);

    // Mica OFF through the Gallery's own toggle path.
    micaToggle->setChecked(false);
    QTest::qWait(500);
    QVERIFY(WinUI3::Private::backdropEffectiveSurface(&gallery)
            != WinUI3::Private::BackdropSurface::Composited);

    // Check the Gallery's restored theme. The dock regression below samples live paints.
    const QImage frame = gallery.grab().toImage();
    QVERIFY(!frame.isNull());
    const QColor windowInk = frame.pixelColor(frame.width() / 2, frame.height() / 2);
    QCOMPARE(windowInk.alpha(), 255);
    const QPalette lightPalette = style->standardPalette();
    const QColor expectedWindow = lightPalette.color(QPalette::Window);
    QCOMPARE(gallery.palette().color(QPalette::Window), expectedWindow);
    QVERIFY(qAbs(windowInk.red() - expectedWindow.red()) <= 8);
    QVERIFY(qAbs(windowInk.green() - expectedWindow.green()) <= 8);
    QVERIFY(qAbs(windowInk.blue() - expectedWindow.blue()) <= 8);
    QCOMPARE(navigation->palette().color(QPalette::Active, QPalette::Text),
             lightPalette.color(QPalette::Active, QPalette::Text));
}

void WinUI3MicaNativeTest::dockWidgetsDemoRemainsAvailable()
{
    GalleryWindow gallery;
    gallery.resize(1180, 780);
    gallery.show();
    QVERIFY(QTest::qWaitForWindowExposed(&gallery));
    auto *action = gallery.findChild<QAction *>(QStringLiteral("dockWidgetsAction"));
    QVERIFY(action);
    QCOMPARE(action->text(), QStringLiteral("Dock widgets"));
    QCOMPARE(action->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    QVERIFY(action->isCheckable());
    QVERIFY(!action->isChecked());
    QCOMPARE(gallery.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly).size(), 0);
    for (int cycle = 0; cycle < 2; ++cycle) {
        action->trigger();
        QVERIFY(action->isChecked());
        const auto docks =
                gallery.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(docks.size(), 4);
        QStringList titles;
        for (QDockWidget *dock : docks) {
            titles.append(dock->windowTitle());
            QCOMPARE(dock->features(),
                     QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable
                             | QDockWidget::DockWidgetFloatable);
        }
        titles.sort();
        QCOMPARE(titles,
                 QStringList({ QStringLiteral("Commit message"), QStringLiteral("Files"),
                               QStringLiteral("Output"), QStringLiteral("Project") }));
        action->trigger();
        QVERIFY(!action->isChecked());
        QTRY_COMPARE(
                gallery.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly).size(),
                0);
    }
}

void WinUI3MicaNativeTest::popupGrantAliveRejectsRevokedEffect()
{
    // Combo popup light bands (user 2026-10-16): a silently revoked DWM
    // grant left the cached Composited property lying, so the re-show kept
    // the translucent tint over a white backing store. The keep-probe must
    // consult the live DWM readback, not just the property.
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("requires the live DWM compositor");
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(WinUI3::ThemeMode::Dark);

    QWidget host;
    host.resize(560, 440);
    QComboBox combo(&host);
    combo.addItems({ QStringLiteral("Standard density"), QStringLiteral("Compact density") });
    combo.resize(200, 32);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    combo.showPopup();
    QVERIFY(QTest::qWaitForWindowExposed(combo.view()));
    QWidget *popup = combo.view()->window();
    QVERIFY(popup && popup->windowHandle());
    combo.hidePopup();

    // Force the incoherent state: published Composited, DWM effect revoked.
    // Resolved dynamically so the test links with no extra dependency.
    QLibrary dwm(QStringLiteral("dwmapi"));
    using SetAttribute = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
    using GetAttribute = HRESULT(WINAPI *)(HWND, DWORD, PVOID, DWORD);
    auto setAttribute = reinterpret_cast<SetAttribute>(dwm.resolve("DwmSetWindowAttribute"));
    auto getAttribute = reinterpret_cast<GetAttribute>(dwm.resolve("DwmGetWindowAttribute"));
    QVERIFY(setAttribute && getAttribute);
    HWND hwnd = reinterpret_cast<HWND>(popup->windowHandle()->winId());
    QVERIFY(IsWindow(hwnd));
    DWORD autoValue = 0; // DWMSBT_AUTO: no acrylic.
    QVERIFY(SUCCEEDED(setAttribute(hwnd, 38, &autoValue, sizeof(autoValue))));
    DWORD readback = 0;
    if (FAILED(getAttribute(hwnd, 38, &readback, sizeof(readback))))
        QSKIP("DWM readback unreadable here");
    if (readback != 0)
        QSKIP("DWM ignores the revoke here");
    popup->setProperty("_winui_backdrop_effective", 2); // published Composited
    // The live readback (refused) overrules the lying property.
    QVERIFY(!WinUI3::Private::popupBackdropGrantAlive(popup));
    // Geometry pairing on the same state: the probe changed no geometry.
    QCOMPARE(popup->contentsMargins().top(), 4);
    QCOMPARE(popup->contentsMargins().bottom(), 4);
    style->setThemeMode(WinUI3::ThemeMode::Dark);
}

void WinUI3MicaNativeTest::micaToggleClearsVacatedClientPixels_data()
{
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("styledBackground");
    for (bool dark : { false, true }) {
        for (bool compact : { false, true }) {
            for (bool styledBackground : { false, true }) {
                const QByteArray name = QByteArray(dark ? "dark" : "light")
                        + (compact ? "-compact" : "-standard")
                        + (styledBackground ? "-styled" : "-unstyled");
                QTest::newRow(name.constData()) << dark << compact << styledBackground;
            }
        }
    }
}

void WinUI3MicaNativeTest::micaToggleClearsVacatedClientPixels()
{
    if (QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("requires the live DWM compositor and raster backing store");
    QFETCH(bool, dark);
    QFETCH(bool, compact);
    QFETCH(bool, styledBackground);
    DisableAnimationsGuard animations;
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    QMainWindow window;
    window.setProperty(WinUI3::Style::DensityProperty, compact ? 1 : 0);
    window.setAttribute(Qt::WA_StyledBackground, styledBackground);
    window.setContentsMargins(0, 0, 0, 64);
    auto *dock = new QDockWidget(QStringLiteral("Inspector"), &window);
    dock->setWidget(new QPlainTextEdit(dock));
    window.addDockWidget(Qt::LeftDockWidgetArea, dock);
    window.resize(640, 420);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *marker = new QLabel(QStringLiteral("Transient client content"), &window);
    marker->setGeometry(window.width() - 160, window.height() - 60, 140, 32);
    marker->setAutoFillBackground(true);
    marker->setBackgroundRole(QPalette::Highlight);
    const QPoint markerSample(4, 4);
    const QPoint sample = marker->mapTo(&window, markerSample);
    const QColor seed = marker->palette().color(QPalette::Highlight);
    BackingStorePixelProbe markerProbe(marker, markerSample);
    BackingStorePixelProbe windowProbe(&window, sample);
    for (int cycle = 0; cycle < 2; ++cycle) {
        window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("mica"));
        QTRY_COMPARE(WinUI3::Private::backdropEffectiveSurface(&window),
                     WinUI3::Private::BackdropSurface::Composited);
        markerProbe.reset();
        marker->show();
        QTRY_COMPARE(markerProbe.color(), seed);
        windowProbe.reset();
        marker->hide();
        QCOMPARE(window.childAt(sample), nullptr);
        QCOMPARE(window.palette().color(QPalette::Window).alpha(), 0);
        QTRY_COMPARE(windowProbe.color(), QColor(Qt::transparent));
        QCOMPARE(window.testAttribute(Qt::WA_StyledBackground), true);
        QCOMPARE(window.testAttribute(Qt::WA_NoSystemBackground), false);
        windowProbe.reset();
        window.setProperty(WinUI3::Style::BackdropProperty, QStringLiteral("none"));
        QTRY_COMPARE(WinUI3::Private::backdropEffectiveSurface(&window),
                     WinUI3::Private::BackdropSurface::Solid);
        QCOMPARE(window.testAttribute(Qt::WA_StyledBackground), styledBackground);
        QCOMPARE(window.palette().color(QPalette::Window),
                 style->standardPalette().color(QPalette::Window));
        QTRY_COMPARE(windowProbe.color(), style->standardPalette().color(QPalette::Window));
    }
    window.close();
}

QTEST_MAIN(WinUI3MicaNativeTest)
#include "tst_winui3micanative.moc"
