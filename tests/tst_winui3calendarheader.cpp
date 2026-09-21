// SPDX-License-Identifier: LGPL-2.1-or-later
// Uniform-acrylic calendar header (spec/coverage.md QCalendarWidget row):
// Body uses the effective popup tint (opaque on fallback); header/lane
// must have indistinguishable idle fill/stroke. Stock QDateEdit input,
// light/dark and standard/compact; captures pair same-state geometry contracts.
#include <winui3style/winui3style.h>

#include "winui3testhelpers.h"

#include "../src/winui3tokens_p.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QGroupBox>
#include <QImage>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyleOptionSlider>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtTest>
#include <QLibrary>
#include <QFile>
#include <QCryptographicHash>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#endif

class WinUI3CalendarHeaderTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void calendarHeaderMatchesPopupTintAcrossThemeSwitch_data();
    void calendarHeaderMatchesPopupTintAcrossThemeSwitch();
    void inlineCalendarUniformSurface_data();
    void inlineCalendarUniformSurface();
    void micaScrollBarsAndNavigationPanelShareMaterial_data();
    void micaScrollBarsAndNavigationPanelShareMaterial();
};

void WinUI3CalendarHeaderTest::initTestCase()
{
    qApp->setStyle(new WinUI3::Style(WinUI3::ThemeMode::Light));
#ifdef Q_OS_WIN
    wchar_t modulePath[32768] = {};
    const HMODULE module = GetModuleHandleW(L"winui3style.dll");
    QVERIFY(module);
    QVERIFY(GetModuleFileNameW(module, modulePath, 32768));
    QFile dll(QString::fromWCharArray(modulePath));
    QVERIFY(dll.open(QIODevice::ReadOnly));
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QVERIFY(hash.addData(&dll));
    qInfo() << "loaded style DLL=" << dll.fileName() << "SHA256=" << hash.result().toHex();
#endif
}

void WinUI3CalendarHeaderTest::init()
{
    if (auto *style = qobject_cast<WinUI3::Style *>(qApp->style())) {
        style->setThemeMode(WinUI3::ThemeMode::Light);
        style->setAccentColor({});
        style->setDensityMode(WinUI3::DensityMode::Standard);
    }
}

void WinUI3CalendarHeaderTest::cleanup()
{
    for (QWidget *widget : qApp->topLevelWidgets()) {
        if (widget->windowType() == Qt::Popup || widget->windowType() == Qt::ToolTip)
            widget->hide();
        else
            widget->close();
    }
    qApp->processEvents();
}

void WinUI3CalendarHeaderTest::calendarHeaderMatchesPopupTintAcrossThemeSwitch_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("darkFirst");
    QTest::newRow("standard-light-first") << false << false;
    QTest::newRow("compact-light-first") << true << false;
    QTest::newRow("standard-dark-first") << false << true;
    QTest::newRow("compact-dark-first") << true << true;
}

void WinUI3CalendarHeaderTest::calendarHeaderMatchesPopupTintAcrossThemeSwitch()
{
    QFETCH(bool, compact);
    QFETCH(bool, darkFirst);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    QWidget root;
    QDateEdit date(&root);
    // Stock galleryDatePicker configuration; no surface/role overrides.
    date.setCalendarPopup(true);
    date.setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    date.setDate(QDate(2026, 10, 15));
    date.resize(date.sizeHint());
    root.resize(500, 400);
    root.show();
    QVERIFY(QTest::qWaitForWindowExposed(&root));
    auto *calendar = date.calendarWidget();
    QVERIFY(calendar);

    for (const auto mode : { darkFirst ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light,
                             darkFirst ? WinUI3::ThemeMode::Light : WinUI3::ThemeMode::Dark }) {
        style->setThemeMode(mode);
        QCoreApplication::processEvents();
        for (int epoch = 0; epoch != 2; ++epoch) {
            qInfo() << "theme=" << (mode == WinUI3::ThemeMode::Light ? "light" : "dark")
                    << "density=" << (compact ? "compact" : "standard") << "open epoch=" << epoch
                    << "platform=" << QGuiApplication::platformName();
            // QDateTimeEdit's calendarPopup path uses CC_ComboBox/Arrow,
            // not the numeric spinbox step buttons.
            QStyleOptionComboBox option;
            option.initFrom(&date);
            option.editable = true;
            option.frame = date.hasFrame();
            const QRect arrow = date.style()->subControlRect(QStyle::CC_ComboBox, &option,
                                                             QStyle::SC_ComboBoxArrow, &date);
            QVERIFY(!arrow.isEmpty());
            QCOMPARE(date.style()->hitTestComplexControl(QStyle::CC_ComboBox, &option,
                                                         arrow.center(), &date),
                     QStyle::SC_ComboBoxArrow);
            qInfo() << "closed anchor=" << QRect(date.mapToGlobal(QPoint()), date.size())
                    << "arrow=" << arrow << "date=" << date.date();
            QTest::mouseMove(&date, arrow.center());
            QTest::mousePress(&date, Qt::LeftButton, {}, arrow.center());
            QTest::mouseRelease(&date, Qt::LeftButton, {}, arrow.center());
            QTRY_VERIFY_WITH_TIMEOUT(calendar->isVisible(), 2000);
            QWidget *popup = calendar->window();
            QVERIFY(popup->isVisible());
            QCOMPARE(popup->windowType(), Qt::Popup);
            QCOMPARE(date.calendarWidget(), calendar);
            QCOMPARE(calendar->selectedDate(), QDate(2026, 10, 15));
            // Real pointer leave, no injected hover properties or repolish.
            QTest::mouseMove(&root, root.rect().bottomRight());
            QTest::qWait(500);
            qInfo() << "input succeeded; popup=" << popup->geometry()
                    << "DPR=" << popup->devicePixelRatioF();

            const bool composited = WinUI3::Private::backdropEffectiveSurface(popup)
                    == WinUI3::Private::BackdropSurface::Composited;
            QColor expected = WinUI3::Private::popupSurfaceColor(style->standardPalette());
            if (composited)
                expected.setAlpha(WinUI3::Private::tokens(style->standardPalette()).dark ? 178
                                                                                         : 242);
            // Read the real published effective recipe; Qt::Popup alone does
            // not grant acrylic. QWidget captures below measure Qt ink, not
            // desktop wallpaper, and cannot establish native blur/alpha.
            qInfo() << "effective composited=" << composited << "expected tint=" << expected;
            if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
                QCOMPARE(composited, false);
            auto *view = calendar->findChild<QAbstractItemView *>(
                    QStringLiteral("qt_calendar_calendarview"));
            auto *navigation =
                    calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
            QVERIFY(view);
            QVERIFY(navigation);
            QCOMPARE(navigation->backgroundRole(), QPalette::Window);
            QCOMPARE(view->palette().color(QPalette::Highlight), QColor(Qt::transparent));
            QCOMPARE(view->palette().color(QPalette::HighlightedText),
                     WinUI3::Private::tokens(style->standardPalette()).textPrimary);

            // Capture the entire actual popup, not independently erased child
            // grabs. Geometry and token contracts apply to this same idle state.
            const QRect headerRect(navigation->mapTo(popup, QPoint()), navigation->size());
            QCOMPARE(headerRect.size(), navigation->rect().size());
            const QImage frame = popup->grab().toImage().convertToFormat(QImage::Format_ARGB32);
            QVERIFY(!frame.isNull());
            const qreal dpr = frame.devicePixelRatio();
            const auto pixel = [&](const QPoint &p) {
                return frame.pixelColor(qFloor((p.x() + 0.5) * dpr), qFloor((p.y() + 0.5) * dpr));
            };
            // Weekday cell's upper-left interior: no selected day, week number,
            // grid edge or text. Require measured font clearance before sampling.
            const QRect cell = view->visualRect(view->model()->index(0, 1));
            QVERIFY(cell.height() > view->fontMetrics().height() + 4);
            QCOMPARE(view->visualRect(view->model()->index(0, 1)), cell);
            const QPoint bodyPoint = view->viewport()->mapTo(popup, cell.topLeft() + QPoint(1, 1));
            const QColor bodyPixel = pixel(bodyPoint);
            qInfo() << "body patch=" << bodyPoint << "actual=" << bodyPixel
                    << "expected=" << expected;
            // Premultiplied-alpha conversion can round an RGB channel by one.
            QVERIFY2(colorDistance(bodyPixel, expected) <= 3,
                     qPrintable(QStringLiteral("body pixel actual %1 expected %2")
                                        .arg(bodyPixel.name(QColor::HexArgb),
                                             expected.name(QColor::HexArgb))));

            const QStringList laneNames = { QStringLiteral("qt_calendar_prevmonth"),
                                            QStringLiteral("qt_calendar_monthbutton"),
                                            QStringLiteral("qt_calendar_yearbutton"),
                                            QStringLiteral("qt_calendar_nextmonth") };
            QRegion emptyHeader(navigation->rect());
            for (QWidget *child :
                 navigation->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
                if (child->isVisible())
                    emptyHeader -= child->geometry();
            }
            for (const QString &name : laneNames) {
                auto *button = calendar->findChild<QToolButton *>(name);
                QVERIFY(button);
                QVERIFY(!button->underMouse());
                QVERIFY(!button->isDown());
                QVERIFY(!button->isChecked());
                const QRect rect(button->mapTo(navigation, QPoint()), button->size());
                QCOMPARE(rect.size(), button->rect().size());
                // Scan the center of the top edge and the fill just below it,
                // stopping before the measured font/icon envelope. This avoids
                // rounded corners, labels, chevrons and the month menu glyph.
                const int inkHeight =
                        qMax(button->fontMetrics().height(), button->iconSize().height());
                const int clearance = (rect.height() - inkHeight) / 2;
                QVERIFY(clearance >= 2);
                qInfo() << name << "popup rect=" << popup->rect() << "header rect=" << headerRect
                        << "button rect in header=" << rect << "ink clearance=" << clearance;
                QString mismatch;
                for (int y = rect.top(); y < rect.top() + clearance; ++y) {
                    QPoint adjacent;
                    bool found = false;
                    int distance = navigation->width();
                    for (int x = 0; x < navigation->width(); ++x) {
                        if (emptyHeader.contains(QPoint(x, y))
                            && qAbs(x - rect.center().x()) < distance) {
                            adjacent = QPoint(x, y);
                            distance = qAbs(x - rect.center().x());
                            found = true;
                        }
                    }
                    QVERIFY2(found, "No glyph-free adjacent header sample");
                    const QColor backdrop = pixel(navigation->mapTo(popup, adjacent));
                    QCOMPARE(navigation->palette().color(QPalette::Window), expected);
                    QVERIFY2(colorDistance(backdrop, expected) <= 3,
                             "Header must match the body recipe, not merely equally transparent "
                             "buttons");
                    for (int dx = -1; dx <= 1; ++dx) {
                        const QPoint sample(rect.center().x() + dx, y);
                        const QColor actual = pixel(navigation->mapTo(popup, sample));
                        if (dx == 0)
                            qInfo() << "strip popup point=" << navigation->mapTo(popup, sample)
                                    << "actual=" << actual << "adjacent point=" << adjacent
                                    << "adjacent=" << backdrop;
                        if (colorDistance(actual, backdrop) > 3 && mismatch.isEmpty())
                            mismatch = QStringLiteral(
                                               "%1 idle edge/fill at %2,%3 actual %4 adjacent %5")
                                               .arg(name)
                                               .arg(sample.x())
                                               .arg(sample.y())
                                               .arg(actual.name(QColor::HexArgb),
                                                    backdrop.name(QColor::HexArgb));
                    }
                }
                QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
                QCOMPARE(WinUI3::Style::controlRole(button), WinUI3::ControlRole::Subtle);
                // An unpainted/transparent child may legitimately inherit the
                // header. Only an actual background fill must match its recipe;
                // Subtle's unused Button palette role need not equal Window.
                if (button->autoFillBackground()
                    && button->palette().color(button->backgroundRole()).alpha() != 0)
                    QCOMPARE(button->palette().color(button->backgroundRole()), expected);
            }
            // Body roles are owned by the popup recipe, not merely 'valid'.
            for (QWidget *surface : { popup, static_cast<QWidget *>(calendar),
                                      static_cast<QWidget *>(view), view->viewport() }) {
                qInfo() << "body roles" << surface->metaObject()->className()
                        << surface->objectName();
                QCOMPARE(surface->palette().color(QPalette::Window), expected);
                QCOMPARE(surface->palette().color(QPalette::Base), expected);
            }
            if (navigation->autoFillBackground()
                && navigation->palette().color(navigation->backgroundRole()).alpha() != 0)
                QCOMPARE(navigation->palette().color(navigation->backgroundRole()), expected);

            QTest::keyClick(popup, Qt::Key_Escape);
            QTRY_VERIFY_WITH_TIMEOUT(!popup->isVisible(), 2000);
            qInfo() << "Escape dismissed epoch=" << epoch;
        }
    }
}

void WinUI3CalendarHeaderTest::inlineCalendarUniformSurface_data()
{
    QTest::addColumn<bool>("compact");
    QTest::addColumn<bool>("dark");
    QTest::addColumn<bool>("bodyContract");
    QTest::addColumn<bool>("micaCycle");
    for (bool compact : { false, true }) {
        for (bool dark : { false, true }) {
            for (bool body : { true, false }) {
                const QByteArray name = QByteArray(compact ? "compact-" : "standard-")
                        + (dark ? "dark-" : "light-") + (body ? "body" : "buttons");
                QTest::newRow(name.constData()) << compact << dark << body << false;
                QTest::newRow((name + "-mica-cycle").constData())
                        << compact << dark << body << true;
            }
        }
    }
}

// Live-gallery defect (Dialogs & states -> Persistent dialog parts):
// the INLINE QCalendarWidget must read as ONE opaque ControlFillColorInputActive
// surface (official CalendarViewBackground, in one bordered rect) with idle
// prev/next/month/year buttons carrying no own fill/border
// (CalendarViewNavigationButtonBackground/Border = SubtleFillColorTransparent);
// hover/press/focus stay painted. Popup acrylic is a recorded extension and
// must NOT leak here; the inline contract is theme/density independent of it.
void WinUI3CalendarHeaderTest::inlineCalendarUniformSurface()
{
    QFETCH(bool, compact);
    QFETCH(bool, dark);
    QFETCH(bool, bodyContract);
    QFETCH(bool, micaCycle);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    // DWM system-backdrop grants exist only on the native QPA. Offscreen
    // Mica epochs would exercise an incoherent hybrid (transparentized
    // islands under a windowless Painted state); the eight untouched rows
    // above already pin the fallback contract, so mark the cycle skipped.
    if (micaCycle && QGuiApplication::platformName() != QStringLiteral("windows"))
        QSKIP("Mica grant needs the native windows QPA; fallback rows cover the opaque contract");
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(compact ? WinUI3::DensityMode::Compact : WinUI3::DensityMode::Standard);
    // Gallery's surface-bearing ancestor chain, with unrelated sections omitted.
    // No calendar palette, formats, role or backdrop overrides.
    QMainWindow host;
    host.setObjectName(QStringLiteral("GalleryWindow"));
    auto *central = new QWidget(&host);
    central->setObjectName(QStringLiteral("centralWidget"));
    host.setCentralWidget(central);
    auto *shell = new QHBoxLayout(central);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);
    auto *pages = new QStackedWidget(central);
    pages->setObjectName(QStringLiteral("pages"));
    pages->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    shell->addWidget(pages);
    auto *dialogsPage = new QScrollArea;
    dialogsPage->setObjectName(QStringLiteral("dialogsPage"));
    dialogsPage->setFrameShape(QFrame::NoFrame);
    dialogsPage->setWidgetResizable(true);
    pages->addWidget(dialogsPage);
    QWidget *body = new QWidget;
    body->setObjectName(QStringLiteral("dialogsBody"));
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setSpacing(16);
    bodyLayout->setContentsMargins(28, 20, 28, 28);
    bodyLayout->setSizeConstraint(QLayout::SetMinimumSize);
    QGroupBox *group = new QGroupBox(QStringLiteral("Persistent dialog parts"), body);
    group->setObjectName(QStringLiteral("dialogPartsGroup"));
    auto *groupLayout = new QHBoxLayout(group);
    QCalendarWidget *calendar = new QCalendarWidget(group);
    calendar->setObjectName(QStringLiteral("persistentCalendar"));
    // Freeze only date/locale to the observed French September 2026 frame.
    calendar->setLocale(QLocale(QLocale::French, QLocale::France));
    calendar->setSelectedDate(QDate(2026, 9, 17));
    calendar->setMinimumSize(360, 240);
    groupLayout->addWidget(calendar);
    auto *commands = new QVBoxLayout;
    commands->addWidget(new QLabel(QStringLiteral("QDialogButtonBox command surface"), group));
    commands->addWidget(
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, group));
    commands->addStretch();
    groupLayout->addLayout(commands);
    bodyLayout->addWidget(group);
    dialogsPage->setWidget(body);
    host.resize(900, 450);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QTest::mouseMove(&host, QPoint(2, 2));
    calendar->clearFocus();
    const auto mode = dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light;
    // Keep the original eight rows untouched in behavior. New rows request
    // off/on/off/on through the public API on the same Gallery-shaped host.
    for (int epoch = 0; epoch < (micaCycle ? 4 : 1); ++epoch) {
        const bool requestMica = micaCycle && (epoch % 2 == 1);
        bool applied = false;
        if (micaCycle)
            applied = WinUI3::applyBackdrop(
                    &host, requestMica ? WinUI3::Backdrop::Mica : WinUI3::Backdrop::None);
        QCoreApplication::processEvents();
        QTest::qWait(100);
        bool granted = false;
#ifdef Q_OS_WIN
        if (QGuiApplication::platformName() == QStringLiteral("windows") && micaCycle) {
            QLibrary dwm(QStringLiteral("dwmapi"));
            using DwmGet = HRESULT(WINAPI *)(HWND, DWORD, PVOID, DWORD);
            const auto get = reinterpret_cast<DwmGet>(dwm.resolve("DwmGetWindowAttribute"));
            DWORD material = DWORD(-1);
            const HRESULT hr = get ? get(reinterpret_cast<HWND>(host.internalWinId()), 38,
                                         &material, sizeof(material))
                                   : E_NOTIMPL;
            qInfo() << "epoch=" << epoch << "requestMica=" << requestMica
                    << "applyBackdrop=" << applied << "host=" << host.internalWinId()
                    << "DwmGet38 hr=" << QString::number(quint32(hr), 16) << "value=" << material;
            granted = requestMica && applied && SUCCEEDED(hr) && material == 2;
        }
#endif
        const bool composited = WinUI3::Private::backdropEffectiveSurface(&host)
                == WinUI3::Private::BackdropSurface::Composited;
        QCOMPARE(composited, granted);
        QCOMPARE(style->themeMode(), mode);
        if (micaCycle)
            QCOMPARE(host.property("_winui_backdrop").toInt(),
                     int(requestMica ? WinUI3::Backdrop::Mica : WinUI3::Backdrop::None));
        QTest::qWait(60);
        qInfo() << "theme=" << (mode == WinUI3::ThemeMode::Light ? "light" : "dark")
                << "density=" << (compact ? "compact" : "standard")
                << "platform=" << QGuiApplication::platformName();
        const WinUI3::Private::Tokens tokens = WinUI3::Private::tokens(style->standardPalette());
        auto *navigation =
                calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
        auto *view = calendar->findChild<QAbstractItemView *>(
                QStringLiteral("qt_calendar_calendarview"));
        QVERIFY(navigation);
        QVERIFY(view);
        // Resolve the pinned brush over the actual non-Mica group recipe:
        // layer over window, then InputActive once. This is not popup acrylic.
        QImage oracle(1, 1, QImage::Format_ARGB32_Premultiplied);
        // Granted Mica has no opaque Window under the card. Preserve the
        // card + InputActive ink, not a wallpaper-dependent desktop RGB.
        // Light's pinned InputActive is opaque by definition; never lower it.
        if (granted)
            oracle.fill(Qt::transparent);
        else
            oracle.fill(tokens.surface);
        // Veil the card only when the style actually paints directly on the
        // live material (same predicate production uses). Painted fallback
        // (offscreen) honestly bakes the opaque card; granted Mica does not.
        const bool onMaterial = WinUI3::Private::paintsDirectlyOnBackdrop(group);
        QCOMPARE(onMaterial, granted);
        QPainter oraclePainter(&oracle);
        oraclePainter.fillRect(oracle.rect(),
                               WinUI3::Private::veiledCard(tokens.layer, onMaterial));
        const QColor expectedCard = oracle.pixelColor(0, 0);
        oraclePainter.fillRect(oracle.rect(), tokens.editorFocusedFill);
        oraclePainter.end();
        const QColor expected = oracle.pixelColor(0, 0);
        if (!granted)
            QCOMPARE(expected.alpha(), 255);
        QCOMPARE(tokens.editorFocusedFill.alpha(), dark ? 179 : 255);
        QCOMPARE(calendar->window(), static_cast<QWidget *>(&host));
        QCOMPARE(calendar->minimumSize(), QSize(360, 240));
        QCOMPARE(calendar->window(), static_cast<QWidget *>(&host));
        QCOMPARE(calendar->minimumSize(), QSize(360, 240));
        // Requested Mica resolves to alpha 0 on the island in BOTH effective
        // states: granted (live material behind) and the Painted fallback
        // (the window's own opaque fill backs the island). A None epoch must
        // carry the opaque content surface again.
        if (requestMica)
            QCOMPARE(pages->palette().color(QPalette::Window).alpha(), 0);
        else
            QCOMPARE(pages->palette().color(QPalette::Window), tokens.surface);
        QVERIFY(calendar->dateTextFormat().isEmpty());
        for (QWidget *node = calendar; node; node = node->parentWidget())
            qInfo() << "ancestor=" << node->metaObject()->className() << node->objectName()
                    << "Window=" << node->palette().color(QPalette::Window)
                    << "Base=" << node->palette().color(QPalette::Base)
                    << "surface=" << node->property(WinUI3::Style::SurfaceProperty)
                    << "backdrop=" << node->property(WinUI3::Style::BackdropProperty);
        qInfo() << "expected card=" << expectedCard << "InputActive=" << tokens.editorFocusedFill
                << "expected resolved calendar=" << expected
                << "view Base=" << view->palette().color(QPalette::Base)
                << "header Window=" << navigation->palette().color(QPalette::Window);

        // Real host composition: grab the composed window so erases/fills
        // behave exactly as in the running gallery, not a widget in isolation.
        const QImage frame = host.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        QVERIFY(!frame.isNull());
        const qreal dpr = frame.devicePixelRatio();
        const auto pixel = [&](const QPoint &p) {
            return frame.pixelColor(qFloor((p.x() + 0.5) * dpr), qFloor((p.y() + 0.5) * dpr));
        };
        // Weekday row's upper-left interior: glyph-free body sample.
        const QRect cell = view->visualRect(view->model()->index(0, 1));
        QVERIFY(cell.height() > view->fontMetrics().height() + 4);
        const QPoint bodyPoint = view->viewport()->mapTo(&host, cell.topLeft() + QPoint(1, 1));
        const QColor bodyPixel = pixel(bodyPoint);
        // Header empty stretch between lane children.
        QRegion emptyHeader(navigation->rect());
        for (QWidget *child :
             navigation->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
            if (child->isVisible())
                emptyHeader -= child->geometry();
        }
        QVERIFY(!emptyHeader.isEmpty());
        QPoint headerLocal(-1, -1);
        for (int x = 4; x < navigation->width() - 4; ++x) {
            const QPoint candidate(x, 3);
            if (emptyHeader.contains(candidate) && emptyHeader.contains(candidate - QPoint(2, 1))
                && emptyHeader.contains(candidate + QPoint(2, 1))) {
                headerLocal = candidate;
                break;
            }
        }
        QVERIFY(headerLocal.x() >= 0);
        QCOMPARE(emptyHeader.contains(headerLocal), true);
        QCOMPARE(view->visualRect(view->model()->index(0, 1)), cell);
        const QPoint headerPoint = navigation->mapTo(&host, headerLocal);
        const QColor headerPixel = pixel(headerPoint);
        const QPoint cardPoint = group->mapTo(&host, QPoint(8, group->height() - 8));
        qInfo() << "body patch=" << bodyPoint << "actual=" << bodyPixel << "header=" << headerPoint
                << "inside empty=" << emptyHeader.contains(headerLocal)
                << "header actual=" << headerPixel << "card actual=" << pixel(cardPoint);
        QVERIFY(colorDistance(pixel(cardPoint), expectedCard) <= 3);
        if (bodyContract) {
            // Raw-ink assertions per epoch: granted-Mica epochs must show the
            // InputActive ink floating on the material (alpha-preserving),
            // fallback/None epochs the baked opaque surface. Never require
            // desktop wallpaper pixels; widget grabs are Qt ink.
            if (granted) {
                QVERIFY2(colorDistance(bodyPixel, expected) <= 3,
                         qPrintable(QStringLiteral(
                                            "granted-Mica inline body %1 expected unflattened %2")
                                            .arg(bodyPixel.name(QColor::HexArgb),
                                                 expected.name(QColor::HexArgb))));
                QVERIFY2(colorDistance(headerPixel, expected) <= 3,
                         qPrintable(QStringLiteral("granted-Mica header %1 expected unflattened %2")
                                            .arg(headerPixel.name(QColor::HexArgb),
                                                 expected.name(QColor::HexArgb))));
            } else {
                QVERIFY2(colorDistance(bodyPixel, expected) <= 3,
                         qPrintable(QStringLiteral("fallback inline body %1 expected resolved "
                                                   "InputActive %2; header %3")
                                            .arg(bodyPixel.name(QColor::HexArgb),
                                                 expected.name(QColor::HexArgb),
                                                 headerPixel.name(QColor::HexArgb))));
                QVERIFY(colorDistance(headerPixel, expected) <= 3);
            }
            QCOMPARE(view->palette().color(QPalette::Base), expected);
            QCOMPARE(view->viewport()->palette().color(QPalette::Base), expected);
            QCOMPARE(navigation->palette().color(QPalette::Window), expected);
            if (granted)
                QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(view), true);
            continue;
        }
        // Granted Mica veils the header exactly like the body (same single-
        // composite ink asserted for body rows above); only the refused and
        // offscreen fallbacks require opaque ink here.
        if (granted)
            QVERIFY2(colorDistance(headerPixel, expected) <= 3,
                     qPrintable(QStringLiteral("granted-Mica header %1 expected unflattened %2")
                                        .arg(headerPixel.name(QColor::HexArgb),
                                             expected.name(QColor::HexArgb))));
        else
            QVERIFY(headerPixel.alpha() == 255);
        // Idle nav buttons: Subtle, no own fill/border. Every interior
        // button sample (top-edge strip below the glyph envelope) must equal
        // the adjacent header stretch pixel; a boxed button (own fill or
        // stroke) breaks equality inside its rect.
        const QStringList laneNames = { QStringLiteral("qt_calendar_prevmonth"),
                                        QStringLiteral("qt_calendar_monthbutton"),
                                        QStringLiteral("qt_calendar_yearbutton"),
                                        QStringLiteral("qt_calendar_nextmonth") };
        for (const QString &name : laneNames) {
            auto *button = calendar->findChild<QToolButton *>(name);
            QVERIFY(button);
            QVERIFY(!button->underMouse());
            QVERIFY(!button->isDown());
            const QRect rect(button->mapTo(&host, QPoint()), button->size());
            QCOMPARE(rect.size(), button->rect().size());
            qInfo() << "button role=" << int(WinUI3::Style::controlRole(button))
                    << "Window=" << button->palette().color(QPalette::Window)
                    << "Button=" << button->palette().color(QPalette::Button);
            const int inkHeight = qMax(button->fontMetrics().height(), button->iconSize().height());
            const int clearance = (rect.height() - inkHeight) / 2;
            QVERIFY(clearance >= 2);
            qInfo() << name << "rect=" << rect << "clearance=" << clearance;
            QString mismatch;
            for (int y = rect.top(); y < rect.top() + clearance; ++y) {
                const QColor adjacent = headerPixel;
                for (int dx = -1; dx <= 1; ++dx) {
                    const QPoint sample(rect.center().x() + dx, y);
                    const QColor actual = pixel(sample);
                    if (colorDistance(actual, adjacent) > 3 && mismatch.isEmpty())
                        mismatch =
                                QStringLiteral(
                                        "%1 idle fill/stroke at %2,%3 actual %4 expected header %5")
                                        .arg(name)
                                        .arg(sample.x())
                                        .arg(sample.y())
                                        .arg(actual.name(QColor::HexArgb),
                                             adjacent.name(QColor::HexArgb));
                }
            }
            QVERIFY2(mismatch.isEmpty(), qPrintable(mismatch));
            // User requirement: idle nav buttons must read Subtle (no own
            // fill/border). Conflicting old inline assertions may pin
            // Standard here; that conflict is recorded in coverage.md, and
            // this assertion governs the new contract.
            QCOMPARE(WinUI3::Style::controlRole(button), WinUI3::ControlRole::Subtle);
            // Granted-Mica erase gate: without Source-clear the button paints
            // its restored calendar surface over live material (boxed ghost);
            // with it the idle button is a real transparency hole in Qt ink.
            if (granted)
                QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(button), true);
        }
    }
}

void WinUI3CalendarHeaderTest::micaScrollBarsAndNavigationPanelShareMaterial_data()
{
    QTest::addColumn<bool>("dark");
    QTest::newRow("light") << false;
    QTest::newRow("dark") << true;
}

// Live-gallery defect: with Mica ON both scrollbars and the navigation
// panel stay opaque in the running gallery (DWM attr 38 == 2 on the main
// window). Gallery shape mirrored here: QMainWindow host with a left
// navigationPanel plus a central QScrollArea page carrying enough content
// that BOTH scrollbars are visible. Show + exposed FIRST, then grant via
// the public WinUI3::applyBackdrop(Mica) on the visible host; the grant is
// verified by DwmGetWindowAttribute(38) S_OK value 2. Granted rows assert
// the armed island recipe per bar/panel; refused/unreadable rows assert the
// honest opaque fallback (never a faked _winui_backdrop_effective).
// Mapping: ScrollBar Direct (WinUI ScrollBar, transparent rest groove) and
// the shell sync in winui3surfaces_p.cpp (content islands + centralWidget /
// navigationPanel transparentize to alpha 0, bars join the chain because
// they carry style-owned explicit palettes). An opaque bar/panel under a
// granted material is therefore the defect, not the contract.
void WinUI3CalendarHeaderTest::micaScrollBarsAndNavigationPanelShareMaterial()
{
    QFETCH(bool, dark);
    auto *style = qobject_cast<WinUI3::Style *>(qApp->style());
    QVERIFY(style);
    style->setThemeMode(dark ? WinUI3::ThemeMode::Dark : WinUI3::ThemeMode::Light);
    style->setDensityMode(WinUI3::DensityMode::Standard);

    QMainWindow host;
    host.setObjectName(QStringLiteral("GalleryWindow"));
    auto *central = new QWidget(&host);
    central->setObjectName(QStringLiteral("centralWidget"));
    host.setCentralWidget(central);
    auto *shell = new QHBoxLayout(central);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);
    auto *navPanel = new QWidget(central);
    navPanel->setObjectName(QStringLiteral("navigationPanel"));
    navPanel->setFixedWidth(220);
    auto *navLayout = new QVBoxLayout(navPanel);
    navLayout->setContentsMargins(8, 8, 8, 8);
    auto *navList = new QListWidget(navPanel);
    navList->setObjectName(QStringLiteral("navigationList"));
    navList->setProperty(WinUI3::Style::NavigationViewProperty, true);
    for (const QString &entry : { QStringLiteral("Controls"), QStringLiteral("Collections"),
                                  QStringLiteral("Settings"), QStringLiteral("Dialogs") })
        navList->addItem(entry);
    navLayout->addWidget(navList);
    shell->addWidget(navPanel);
    auto *pages = new QStackedWidget(central);
    pages->setObjectName(QStringLiteral("pages"));
    pages->setProperty(WinUI3::Style::SurfaceProperty, QStringLiteral("content"));
    shell->addWidget(pages, 1);
    auto *area = new QScrollArea;
    area->setObjectName(QStringLiteral("controlsPage"));
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    pages->addWidget(area);
    auto *body = new QWidget;
    body->setObjectName(QStringLiteral("controlsBody"));
    body->setMinimumSize(900, 900);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->addWidget(new QLabel(QStringLiteral("Controls"), body));
    for (int row = 0; row < 24; ++row)
        bodyLayout->addWidget(new QLabel(QStringLiteral("Row %1 content line").arg(row), body));
    area->setWidget(body);
    host.resize(700, 500);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    QCoreApplication::processEvents();
    auto *vBar = area->verticalScrollBar();
    auto *hBar = area->horizontalScrollBar();
    QVERIFY(vBar);
    QVERIFY(hBar);
    QTRY_VERIFY(vBar->isVisible());
    QTRY_VERIFY(hBar->isVisible());
    vBar->setValue(vBar->maximum() / 2);
    hBar->setValue(hBar->maximum() / 2);
    QCoreApplication::processEvents();
    // Rest state: park the pointer where no bar can hover-expand.
    QTest::mouseMove(&host, QPoint(4, 4));
    QTest::qWait(120);

    const bool applied = WinUI3::applyBackdrop(&host, WinUI3::Backdrop::Mica);
    QCoreApplication::processEvents();
    QTest::qWait(120);
    bool granted = false;
#ifdef Q_OS_WIN
    if (QGuiApplication::platformName() == QStringLiteral("windows")) {
        QLibrary dwm(QStringLiteral("dwmapi"));
        using DwmGet = HRESULT(WINAPI *)(HWND, DWORD, PVOID, DWORD);
        const auto get = reinterpret_cast<DwmGet>(dwm.resolve("DwmGetWindowAttribute"));
        DWORD material = DWORD(-1);
        const HRESULT hr = get
                ? get(reinterpret_cast<HWND>(host.internalWinId()), 38, &material, sizeof(material))
                : E_NOTIMPL;
        qInfo() << "mica bars/nav epoch theme=" << (dark ? "dark" : "light")
                << "applied=" << applied << "hr=" << QString::number(quint32(hr), 16)
                << "attr38=" << material;
        granted = applied && SUCCEEDED(hr) && material == 2;
    }
#else
    Q_UNUSED(applied);
#endif
    const bool composited = WinUI3::Private::backdropEffectiveSurface(&host)
            == WinUI3::Private::BackdropSurface::Composited;
    // Never fake the effective state: the published recipe must equal the
    // live DWM readback on every row.
    QCOMPARE(composited, granted);
    QCOMPARE(host.property("_winui_backdrop").toInt(), int(WinUI3::Backdrop::Mica));

    const QList<QWidget *> targets = { vBar, hBar, navPanel };
    for (QWidget *target : targets) {
        qInfo() << "target=" << target->objectName() << target->metaObject()->className()
                << "Window=" << target->palette().color(QPalette::Window)
                << "autoFill=" << target->autoFillBackground()
                << "styled=" << target->testAttribute(Qt::WA_StyledBackground)
                << "opaque=" << target->testAttribute(Qt::WA_OpaquePaintEvent)
                << "onBackdrop=" << WinUI3::Private::paintsDirectlyOnBackdrop(target);
    }
    qInfo() << "island guard=" << area->property("_winui_island_scroll_guard")
            << "pages Window=" << pages->palette().color(QPalette::Window)
            << "viewport Window=" << area->viewport()->palette().color(QPalette::Window);

    if (granted) {
        // Armed recipe per the shell sync (winui3surfaces_p.cpp): content
        // islands + centralWidget/navigationPanel + the full scrolled chain
        // including BOTH bars transparentize to alpha 0 with no autofill and
        // style-owned erase. WA_OpaquePaintEvent is stock-preserved by the
        // sync (bars keep Qt's opaque claim, the panel keeps its plain
        // QWidget flag); the transparency contract is alpha + no-fill +
        // StyledBackground + open gate + transparent track pixels, not the
        // untouched flag.
        for (QWidget *bar : { vBar, hBar }) {
            QCOMPARE(bar->palette().color(QPalette::Window).alpha(), 0);
            QCOMPARE(bar->autoFillBackground(), false);
            QCOMPARE(bar->testAttribute(Qt::WA_StyledBackground), true);
            QCOMPARE(bar->testAttribute(Qt::WA_OpaquePaintEvent), true);
            QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(bar), true);
        }
        QCOMPARE(navPanel->palette().color(QPalette::Window).alpha(), 0);
        QCOMPARE(navPanel->autoFillBackground(), false);
        QCOMPARE(navPanel->testAttribute(Qt::WA_StyledBackground), true);
        QCOMPARE(navPanel->testAttribute(Qt::WA_OpaquePaintEvent), false);
        QCOMPARE(WinUI3::Private::paintsDirectlyOnBackdrop(navPanel), true);
        QVERIFY(area->property("_winui_island_scroll_guard").isValid());

        const QImage frame = host.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        QVERIFY(!frame.isNull());
        const qreal dpr = frame.devicePixelRatio();
        const auto pixel = [&](const QPoint &p) {
            return frame.pixelColor(qFloor((p.x() + 0.5) * dpr), qFloor((p.y() + 0.5) * dpr));
        };
        const QRect vHostRect(vBar->mapTo(&host, QPoint()), vBar->size());
        const QRect hHostRect(hBar->mapTo(&host, QPoint()), hBar->size());
        QCOMPARE(vHostRect.size(), vBar->rect().size());
        QCOMPARE(hHostRect.size(), hBar->rect().size());
        // Glyph-free track patches: top strip of the vertical bar and left
        // strip of the horizontal bar while both thumbs sit centered.
        const QPoint vTrackHost = vHostRect.topLeft() + QPoint(vHostRect.width() / 2, 2);
        const QPoint hTrackHost = hHostRect.topLeft() + QPoint(2, hHostRect.height() / 2);
        QVERIFY(vHostRect.contains(vTrackHost));
        QVERIFY(hHostRect.contains(hTrackHost));
        QStyleOptionSlider vOpt;
        vOpt.initFrom(vBar);
        vOpt.rect = vBar->rect();
        vOpt.orientation = Qt::Vertical;
        vOpt.minimum = vBar->minimum();
        vOpt.maximum = vBar->maximum();
        vOpt.sliderPosition = vBar->value();
        const QRect vThumb = style->subControlRect(QStyle::CC_ScrollBar, &vOpt,
                                                   QStyle::SC_ScrollBarSlider, vBar);
        QVERIFY(!vThumb.contains(vBar->mapFrom(&host, vTrackHost)));
        QStyleOptionSlider hOpt;
        hOpt.initFrom(hBar);
        hOpt.rect = hBar->rect();
        hOpt.orientation = Qt::Horizontal;
        hOpt.minimum = hBar->minimum();
        hOpt.maximum = hBar->maximum();
        hOpt.sliderPosition = hBar->value();
        const QRect hThumb = style->subControlRect(QStyle::CC_ScrollBar, &hOpt,
                                                   QStyle::SC_ScrollBarSlider, hBar);
        QVERIFY(!hThumb.contains(hBar->mapFrom(&host, hTrackHost)));
        // Adjacent island samples: viewport strip just inside each bar.
        const QPoint vAdjacentHost = vHostRect.topLeft() + QPoint(-4, 8);
        const QPoint hAdjacentHost = hHostRect.topLeft() + QPoint(8, -4);
        QVERIFY(host.rect().contains(vAdjacentHost));
        QVERIFY(host.rect().contains(hAdjacentHost));
        // Nav empty patch: layout margin (2,2) sits outside every child.
        const QPoint navHost = navPanel->mapTo(&host, QPoint(2, 2));
        for (QWidget *child :
             navPanel->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
            if (child->isVisible())
                QVERIFY(!QRect(child->mapTo(navPanel, QPoint()), child->size())
                                 .contains(QPoint(2, 2)));
        }
        const QColor vTrack = pixel(vTrackHost);
        const QColor hTrack = pixel(hTrackHost);
        const QColor vAdjacent = pixel(vAdjacentHost);
        const QColor hAdjacent = pixel(hAdjacentHost);
        const QColor navPixel = pixel(navHost);
        qInfo() << "granted pixels vTrack=" << vTrackHost << vTrack << "adjacent=" << vTrackHost
                << vAdjacent << "hTrack=" << hTrackHost << hTrack << "adjacent=" << hAdjacentHost
                << hAdjacent << "nav=" << navHost << navPixel;
        // Same-state QCOMPARE pair for every grab: role alphas above plus
        // geometry (host rects vs widget rects, thumb exclusion) on this
        // exact presented frame.
        QVERIFY2(vTrack.alpha() == 0,
                 qPrintable(QStringLiteral("granted vertical track must be transparent, got %1")
                                    .arg(vTrack.name(QColor::HexArgb))));
        QVERIFY2(hTrack.alpha() == 0,
                 qPrintable(QStringLiteral("granted horizontal track must be transparent, got %1")
                                    .arg(hTrack.name(QColor::HexArgb))));
        QVERIFY2(navPixel.alpha() == 0,
                 qPrintable(QStringLiteral("granted nav panel must be transparent, got %1")
                                    .arg(navPixel.name(QColor::HexArgb))));
        QVERIFY2(colorDistance(vTrack, vAdjacent) <= 12,
                 "vertical track must share the adjacent island surface, not an opaque band");
        QVERIFY2(colorDistance(hTrack, hAdjacent) <= 12,
                 "horizontal track must share the adjacent island surface, not an opaque band");
        return;
    }

    // Refused/unreadable grant: honest opaque fallback on every target.
    // Bars paint their Window fill, so their track pixels stay opaque.
    // The naked navigationPanel paints no fill of its own (no autoFill):
    // its mechanism contract is the opaque Window role + geometry, paired
    // with the same grab; its pixels read through to the shell behind it,
    // so no opaque pixel is asserted there.
    for (QWidget *target : targets)
        QCOMPARE(target->palette().color(QPalette::Window).alpha(), 255);
    // Offscreen Painted fallback: offscreen never grants, so the stranded
    // navigationPanel keeps its Painted-era translucent-window gate open
    // (harmless without a compositor and not the user defect). Assert the
    // honest states instead of an invented opaque gate: opaque Window roles
    // everywhere, bars keep their stock opaque claim, panel keeps its plain
    // flag and no autofill.
    QCOMPARE(vBar->testAttribute(Qt::WA_OpaquePaintEvent), true);
    QCOMPARE(hBar->testAttribute(Qt::WA_OpaquePaintEvent), true);
    QCOMPARE(navPanel->testAttribute(Qt::WA_OpaquePaintEvent), false);
    QCOMPARE(navPanel->autoFillBackground(), false);
    const QImage frame = host.grab().toImage().convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!frame.isNull());
    const qreal dpr = frame.devicePixelRatio();
    const auto pixel = [&](const QPoint &p) {
        return frame.pixelColor(qFloor((p.x() + 0.5) * dpr), qFloor((p.y() + 0.5) * dpr));
    };
    const QRect vHostRect(vBar->mapTo(&host, QPoint()), vBar->size());
    const QRect navHostRect(navPanel->mapTo(&host, QPoint()), navPanel->size());
    QCOMPARE(vHostRect.size(), vBar->rect().size());
    QCOMPARE(navHostRect.size(), navPanel->rect().size());
    const QPoint vTrackHost = vHostRect.topLeft() + QPoint(vHostRect.width() / 2, 2);
    const QPoint navHost = navPanel->mapTo(&host, QPoint(2, 2));
    qInfo() << "fallback pixels vTrack=" << vTrackHost << pixel(vTrackHost) << "nav=" << navHost
            << pixel(navHost);
    QCOMPARE(pixel(vTrackHost).alpha(), 255);
    QCOMPARE(host.rect().contains(navHost), true);
}

QTEST_MAIN(WinUI3CalendarHeaderTest)
#include "tst_winui3calendarheader.moc"
