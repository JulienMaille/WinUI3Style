// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/navigationview.h>
#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3style.h>

#include "../src/winui3backdrop_p.h"
#include "../src/winui3density_p.h"
#include "winui3testhelpers.h"

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QFormLayout>
#include <QRadioButton>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QTabWidget>
#include <QTableView>
#include <QTest>
#include <QTimeEdit>
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>

class WinUI3CalendarSurfaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void calendarPopupRemainsReadableInLightAndDark();
    void calendarNavigationBarLaneSharesOpaqueSurface();
    void calendarKeepPathRebasesTextRolesAcrossThemeSwitch();
};

void WinUI3CalendarSurfaceTest::init()
{
    qApp->setStyle(new WinUI3::Style);
}

void WinUI3CalendarSurfaceTest::cleanup()
{
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
}

void WinUI3CalendarSurfaceTest::calendarPopupRemainsReadableInLightAndDark()
{
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Light);
    QWidget root;
    root.setStyle(&style);
    QDateEdit date(&root);
    date.setCalendarPopup(true);
    date.setDate(QDate(2026, 8, 15));
    date.resize(200, 40);
    // Dialogs-page persistentCalendar: the inline grid keeps the content
    // surface and must share the popup's circle-select day chrome. Same
    // calendarPopupView day chrome, different surfaces: only the tint
    // (flyout vs content) may differ by design.
    QCalendarWidget inlineCalendar;
    inlineCalendar.setStyle(&style);
    inlineCalendar.setSelectedDate(QDate(2026, 8, 15));
    inlineCalendar.resize(360, 240);
    root.show();
    inlineCalendar.show();
    QCoreApplication::processEvents();
    for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
        if (style.themeMode() != mode)
            style.setThemeMode(mode);
        QCoreApplication::processEvents();
        QTest::mouseClick(&date, Qt::LeftButton, {}, QPoint(date.width() - 5, date.height() / 2));
        QCoreApplication::processEvents();
        auto *calendar = date.calendarWidget();
        QVERIFY(calendar);
        // The offscreen QPA plugin does not always route a synthetic click to
        // QDateTimeEdit's private arrow subcontrol after a live palette flip.
        // Showing the already-configured calendar is equivalent to the popup
        // window that the edit creates and keeps this test focused on its
        // rendered day grid.
        if (!calendar->isVisible()) {
            calendar->show();
            QCoreApplication::processEvents();
        }
        QVERIFY(calendar->isVisible());
        auto *view = calendar->findChild<QAbstractItemView *>(
                QStringLiteral("qt_calendar_calendarview"));
        QVERIFY(view);
        QVERIFY(view->isVisible());
        auto *navigation =
                calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
        QVERIFY(navigation);
        QCOMPARE(navigation->backgroundRole(), QPalette::Window);
        // Mechanism first: no native Highlight rect/strip may survive on any
        QCOMPARE(view->palette().color(QPalette::Highlight), QColor(Qt::transparent));
        const WinUI3::Private::Tokens modeTokens =
                WinUI3::Private::buildTokens(style.standardPalette());
        QCOMPARE(view->palette().color(QPalette::HighlightedText), modeTokens.textPrimary);
        // The popup recipe depends on the live grant: refused and offscreen
        // popups resolve the opaque flyout surface, while a granted
        // compositor carries the same RGB with the theme translucency
        // (178 dark / 242 light, the documented popup recipe). Gate on the
        // published effective state.
        if (WinUI3::Private::backdropEffectiveSurface(calendar->window())
            == WinUI3::Private::BackdropSurface::Composited) {
            QColor tinted = WinUI3::Private::popupSurfaceColor(style.standardPalette());
            tinted.setAlpha(modeTokens.dark ? 178 : 242);
            QCOMPARE(view->palette().color(QPalette::Base), tinted);
        } else {
            QCOMPARE(view->palette().color(QPalette::Base),
                     WinUI3::Private::popupSurfaceColor(style.standardPalette()));
        }
        auto *monthButton =
                calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_monthbutton"));
        QVERIFY(monthButton);
        QTest::mouseMove(monthButton, monthButton->rect().center());
        QCoreApplication::processEvents();
        const QModelIndex dayIndex = view->model()->index(2, 3);
        QVERIFY(dayIndex.isValid());
        QStyleOptionViewItem dayOption;
        dayOption.initFrom(view->viewport());
        dayOption.rect = view->visualRect(dayIndex);
        dayOption.index = dayIndex;
        dayOption.text = dayIndex.data(Qt::DisplayRole).toString();
        dayOption.features = QStyleOptionViewItem::HasDisplay;
        const QRect dayTextRect =
                style.subElementRect(QStyle::SE_ItemViewItemText, &dayOption, view->viewport());
        QVERIFY2(dayTextRect.width() >= dayOption.rect.width() / 2,
                 "calendar day text must not be squeezed into the menu icon gutter");
        QVERIFY2(dayTextRect.contains(dayOption.rect.center()),
                 "calendar day text must remain centered in its day cell");
        QStyleOptionViewItem selectedDay = dayOption;
        selectedDay.state |= QStyle::State_Selected;
        QImage selectedImage(selectedDay.rect.size(), QImage::Format_ARGB32_Premultiplied);
        selectedImage.fill(Qt::transparent);
        selectedDay.rect.moveTopLeft(QPoint());
        {
            QPainter painter(&selectedImage);
            style.drawControl(QStyle::CE_ItemViewItem, &selectedDay, &painter, view->viewport());
        }
        const QColor selectedCenter = selectedImage.pixelColor(selectedImage.rect().center());
        QVERIFY2(selectedCenter.alpha() > 0,
                 "calendar selection must render a visible WinUI item surface");
        QVERIFY2(colorDistance(selectedCenter, modeTokens.accentFill) < 64,
                 "calendar selected day must render the accent circle");
        const QColor selectedCorner = selectedImage.pixelColor(0, 0);
        QVERIFY2(selectedCorner.alpha() > 0, "calendar cells must paint an opaque popup surface");
        QVERIFY2(selectedCorner.rgba() != selectedCenter.rgba(),
                 "calendar selection must not fill the complete Qt table cell");
        const QPalette palette = view->palette();
        const QColor text = palette.color(QPalette::Text);
        const QColor background = palette.color(QPalette::Base);
        QVERIFY(text.isValid());
        QVERIFY(background.isValid());
        if (mode == WinUI3::ThemeMode::Dark)
            QVERIFY2(qGray(text.rgb()) > 150, "dark calendar text must use a light foreground");
        else
            QVERIFY2(qGray(text.rgb()) < 100, "light calendar text must use a dark foreground");

        const QImage image = view->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QRect headerCell = view->visualRect(view->model()->index(0, 1));
        QVERIFY(headerCell.isValid());
        const QColor headerSurface = image.pixelColor(headerCell.topLeft() + QPoint(2, 2));
        if (mode == WinUI3::ThemeMode::Dark)
            QVERIFY2(qGray(headerSurface.rgb()) < 100,
                     "hovering the month must not turn the dark weekday row white");
        else
            QVERIFY2(qGray(headerSurface.rgb()) > 180,
                     "the light weekday row must remain a light popup surface");
        int readablePixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                const int gray = qGray(pixel.rgb());
                if ((mode == WinUI3::ThemeMode::Dark && gray > 150)
                    || (mode == WinUI3::ThemeMode::Light && gray < 100))
                    ++readablePixels;
            }
        }
        QVERIFY2(readablePixels > 10, "calendar day cells must contain visible date glyphs");
        // Inline grid: same neutralized Highlight, same circle chrome, but
        // the content-surface Base (tokens.layer), never the flyout tint.
        auto *inlineView = inlineCalendar.findChild<QAbstractItemView *>(
                QStringLiteral("qt_calendar_calendarview"));
        QVERIFY(inlineView);
        QCOMPARE(inlineView->palette().color(QPalette::Highlight), QColor(Qt::transparent));
        QCOMPARE(inlineView->palette().color(QPalette::HighlightedText), modeTokens.textPrimary);
        // Standalone inline calendar: InputActive over Window, no group card.
        QImage inlineSurface(1, 1, QImage::Format_ARGB32_Premultiplied);
        inlineSurface.fill(modeTokens.surface);
        {
            QPainter painter(&inlineSurface);
            painter.fillRect(inlineSurface.rect(), modeTokens.editorFocusedFill);
        }
        QCOMPARE(inlineView->palette().color(QPalette::Base), inlineSurface.pixelColor(0, 0));
        auto *inlineNav =
                inlineCalendar.findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
        QVERIFY(inlineNav);
        QCOMPARE(inlineNav->backgroundRole(), QPalette::Window);
        // Mica contract: the header is the content surface, never the
        // flyout tint and never translucent. Same Window-role surface in
        // mica and non-mica; only DWM behind it changes.
        QCOMPARE(inlineNav->palette().color(QPalette::Window).alpha(), 255);
        QCOMPARE(inlineNav->palette().color(QPalette::Window),
                 inlineCalendar.palette().color(QPalette::Window));
        const QModelIndex inlineDayIndex = inlineView->model()->index(2, 3);
        QVERIFY(inlineDayIndex.isValid());
        QStyleOptionViewItem inlineDayOption;
        inlineDayOption.initFrom(inlineView->viewport());
        inlineDayOption.rect = inlineView->visualRect(inlineDayIndex);
        QVERIFY(!inlineDayOption.rect.isEmpty());
        inlineDayOption.index = inlineDayIndex;
        inlineDayOption.text = inlineDayIndex.data(Qt::DisplayRole).toString();
        inlineDayOption.features = QStyleOptionViewItem::HasDisplay;
        inlineDayOption.state |= QStyle::State_Selected;
        QImage inlineSelected(inlineDayOption.rect.size(), QImage::Format_ARGB32_Premultiplied);
        inlineSelected.fill(Qt::transparent);
        inlineDayOption.rect.moveTopLeft(QPoint());
        {
            QPainter painter(&inlineSelected);
            style.drawControl(QStyle::CE_ItemViewItem, &inlineDayOption, &painter,
                              inlineView->viewport());
        }
        const QColor inlineCenter = inlineSelected.pixelColor(inlineSelected.rect().center());
        QVERIFY2(inlineCenter.alpha() > 0,
                 "inline calendar selection must render a visible WinUI item surface");
        QVERIFY2(colorDistance(inlineCenter, modeTokens.accentFill) < 64,
                 "inline selected day must render the same accent circle as the popup");
        QVERIFY2(inlineSelected.pixelColor(0, 0).rgba() != inlineCenter.rgba(),
                 "inline selection must not fill the complete Qt table cell");
        calendar->hide();
        QCoreApplication::processEvents();
    }
}

void WinUI3CalendarSurfaceTest::calendarNavigationBarLaneSharesOpaqueSurface()
{
    // Gallery repro: Dialogs&states persistentCalendar (inline) + synthesized
    // QDateEdit popup calendar. Mica (composited backdrop) vs normal must keep
    // the header lane uniform: empty stretches and month/year/prev/next parts
    // share the same opaque content-surface background.
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    const QList<WinUI3::DensityMode> densities = { WinUI3::DensityMode::Standard,
                                                   WinUI3::DensityMode::Compact };
    for (const WinUI3::DensityMode density : densities) {
        for (const WinUI3::ThemeMode mode : { WinUI3::ThemeMode::Light, WinUI3::ThemeMode::Dark }) {
            style.setThemeMode(mode);
            QCoreApplication::processEvents();
            for (const bool mica : { false, true }) {
                QWidget host;
                host.setStyle(&style);
                // Real grant path only: the previous revision faked Composited
                // by writing _winui_backdrop_effective directly, which asserts
                // a contract no real compositor can produce (an opaque lane
                // over a granted material). A genuine grant veils the lane;
                // a refused/offscreen grant keeps the opaque fallback.
                // Grant on the VISIBLE window (gallery + calendarheader path):
                // requesting Mica before show forces handle creation pre-show
                // and the show-time re-apply can transiently refuse, sticking
                // Painted with no re-grant. No real client grants pre-show.
                QCalendarWidget *calendar = new QCalendarWidget(&host);
                calendar->setStyle(&style);
                calendar->setSelectedDate(QDate(2026, 8, 15));
                calendar->resize(360, 240);
                if (density == WinUI3::DensityMode::Compact)
                    calendar->setProperty(WinUI3::Style::DensityProperty,
                                          QStringLiteral("compact"));
                host.resize(420, 320);
                host.show();
                QVERIFY(QTest::qWaitForWindowExposed(&host));
                QCoreApplication::processEvents();
                const bool granted = mica && WinUI3::applyBackdrop(&host, WinUI3::Backdrop::Mica)
                        && WinUI3::Private::backdropEffectiveSurface(&host)
                                == WinUI3::Private::BackdropSurface::Composited;
                QCoreApplication::processEvents();

                auto *navigation =
                        calendar->findChild<QWidget *>(QStringLiteral("qt_calendar_navigationbar"));
                QVERIFY2(navigation, "calendar navigation bar must exist");
                QCOMPARE(navigation->backgroundRole(), QPalette::Window);
                // Uniformity, not opacity, is the contract: a genuine grant
                // veils the lane in Dark (alpha < 255); Light's pinned
                // InputActive is opaque by definition, and the refused and
                // offscreen fallbacks stay opaque. Either way the lane must
                // equal the bar.
                if (granted && mode == WinUI3::ThemeMode::Dark)
                    QVERIFY2(navigation->palette().color(QPalette::Window).alpha() < 255,
                             "granted Mica must veil the lane, not bake it opaque");
                else
                    QCOMPARE(navigation->palette().color(QPalette::Window).alpha(), 255);

                const QStringList laneNames = { QStringLiteral("qt_calendar_monthbutton"),
                                                QStringLiteral("qt_calendar_yearbutton"),
                                                QStringLiteral("qt_calendar_prevmonth"),
                                                QStringLiteral("qt_calendar_nextmonth") };
                QList<QWidget *> laneChildren;
                for (const QString &name : laneNames) {
                    QWidget *child = calendar->findChild<QWidget *>(name);
                    QVERIFY2(child, qPrintable(QStringLiteral("missing lane child %1").arg(name)));
                    laneChildren << child;
                    // Mechanism: lane children must sit on the same
                    // Window surface as the bar, never a different role.
                    // Granted Mica veils that shared surface in Dark (Light
                    // InputActive is opaque by definition); the fallback
                    // keeps it opaque.
                    QCOMPARE(child->backgroundRole(), QPalette::Window);
                    if (granted && mode == WinUI3::ThemeMode::Dark)
                        QVERIFY2(child->palette().color(QPalette::Window).alpha() < 255,
                                 "granted Mica must veil lane children, not bake them opaque");
                    else
                        QCOMPARE(child->palette().color(QPalette::Window).alpha(), 255);
                    QCOMPARE(child->palette().color(QPalette::Window),
                             navigation->palette().color(QPalette::Window));
                }

                // CalendarView navigation has no idle fill/stroke. The style
                // restores its resolved calendar surface before Subtle state
                // overlays, including when the backdrop erase gate is open.
                for (QWidget *child : laneChildren) {
                    if (!qobject_cast<QAbstractButton *>(child))
                        continue;
                    QCOMPARE(WinUI3::Style::controlRole(child), WinUI3::ControlRole::Subtle);
                    // Suspect 2: rest/hover fills read the Button role while
                    // the bar reads Window. The lane palette repoints Button
                    // at the same opaque lane surface, so the Standard rest
                    // fill covers the erase with the lane color in both modes.
                    QCOMPARE(child->palette().color(QPalette::Button),
                             navigation->palette().color(QPalette::Window));
                }
                // The year editor frame (CC_SpinBox) also fills from the
                // Button role (t.control), while its text field reads Base:
                // both must be the opaque lane surface, never the app roles.
                if (QAbstractSpinBox *yearEditor = calendar->findChild<QAbstractSpinBox *>()) {
                    QCOMPARE(yearEditor->palette().color(QPalette::Base),
                             navigation->palette().color(QPalette::Window));
                    QCOMPARE(yearEditor->palette().color(QPalette::Button),
                             navigation->palette().color(QPalette::Window));
                }

                // Visual: empty stretch vs month-button rect from the same grab.
                // Mechanism-first (roles + shared Window palettes are asserted
                // above); the grab is a regression signal for the empty
                // stretch only. Child-button pixels are not comparable:
                // offscreen grabs composite the button text/antialiasing over
                // the lane, so a corner probe still lands on text coverage.
                const QImage laneImage =
                        navigation->grab().toImage().convertToFormat(QImage::Format_ARGB32);
                QVERIFY(!laneImage.isNull());
                auto childContains = [&](const QPoint &pt) {
                    for (const QWidget *child : laneChildren) {
                        const QRect inBar(child->mapTo(navigation, QPoint(0, 0)), child->size());
                        if (inBar.contains(pt))
                            return true;
                    }
                    return false;
                };
                QPoint emptyPt(-1, -1);
                const int probeY = navigation->rect().center().y();
                for (int x = 0; x < navigation->rect().width(); ++x) {
                    const QPoint candidate(x, probeY);
                    if (!childContains(candidate)) {
                        emptyPt = candidate;
                        break;
                    }
                }
                QVERIFY2(emptyPt.x() >= 0, "navigation bar must expose an empty stretch");
                const QColor emptyColor = laneImage.pixelColor(emptyPt);
                // Mechanism-first: the lane children paint from the same
                // Window surface contract as the bar (roles + palettes
                // asserted above). The empty stretch must render that surface;
                // the button rect is owned by the same contract and
                // is not pixel-compared (text coverage is not separable in a
                // grab).
                QVERIFY2(colorDistance(emptyColor, navigation->palette().color(QPalette::Window))
                                 < 36,
                         qPrintable(QStringLiteral("empty #%1 vs lane #%2")
                                            .arg(emptyColor.name())
                                            .arg(navigation->palette()
                                                         .color(QPalette::Window)
                                                         .name())));
                // Mica parity: the same shared lane surface with and without
                // the composited material — veiled under a genuine Dark grant
                // (Light InputActive is opaque by definition), opaque on the
                // refused/offscreen fallback. Only DWM behind the window may
                // otherwise change.
                QCOMPARE(emptyColor.alpha(), navigation->palette().color(QPalette::Window).alpha());
                if (granted && mode == WinUI3::ThemeMode::Dark)
                    QVERIFY2(navigation->palette().color(QPalette::Window).alpha() < 255,
                             "granted Mica must veil the lane surface");
                else
                    QCOMPARE(navigation->palette().color(QPalette::Window).alpha(), 255);
                host.hide();
                QCoreApplication::processEvents();
            }
        }
    }
}

void WinUI3CalendarSurfaceTest::calendarKeepPathRebasesTextRolesAcrossThemeSwitch()
{
    // Keep-path: reused popup HWND skips the full rebuild; text roles must
    // still follow a theme flip while hidden (fresh dark surface + stale
    // black text reads as black-on-dark).
    auto &style = *qobject_cast<WinUI3::Style *>(qApp->style());
    style.setThemeMode(WinUI3::ThemeMode::Light);
    QWidget root;
    root.setStyle(&style);
    QDateEdit date(&root);
    date.setCalendarPopup(true);
    date.setDate(QDate(2026, 8, 15));
    date.resize(200, 40);
    root.show();
    QCoreApplication::processEvents();
    QTest::mouseClick(&date, Qt::LeftButton, {}, QPoint(date.width() - 5, date.height() / 2));
    QCoreApplication::processEvents();
    auto *calendar = date.calendarWidget();
    QVERIFY(calendar);
    if (!calendar->isVisible()) {
        calendar->show();
        QCoreApplication::processEvents();
    }
    QVERIFY(calendar->isVisible());
    QWidget *popup = calendar->window();
    QVERIFY(popup);
    const QColor staleText = popup->palette().color(QPalette::Text);
    calendar->hide();
    QCoreApplication::processEvents();
    style.setThemeMode(WinUI3::ThemeMode::Dark);
    QCoreApplication::processEvents();
    QTest::mouseClick(&date, Qt::LeftButton, {}, QPoint(date.width() - 5, date.height() / 2));
    QCoreApplication::processEvents();
    QCOMPARE(date.calendarWidget(), calendar);
    if (!calendar->isVisible()) {
        calendar->show();
        QCoreApplication::processEvents();
    }
    QVERIFY(calendar->isVisible());
    QWidget *reopened = calendar->window();
    QVERIFY(reopened);
    const QPalette fresh = style.standardPalette();
    QCOMPARE(reopened->palette().color(QPalette::WindowText), fresh.color(QPalette::WindowText));
    QCOMPARE(reopened->palette().color(QPalette::Text), fresh.color(QPalette::Text));
    QCOMPARE(reopened->palette().color(QPalette::ButtonText), fresh.color(QPalette::ButtonText));
    QCOMPARE(reopened->palette().color(QPalette::HighlightedText),
             fresh.color(QPalette::HighlightedText));
    QVERIFY2(reopened->palette().color(QPalette::Text) != staleText,
             "reopened popup must not keep the pre-flip text role");
    auto *view =
            calendar->findChild<QAbstractItemView *>(QStringLiteral("qt_calendar_calendarview"));
    QVERIFY(view);
    const WinUI3::Private::Tokens darkTokens = WinUI3::Private::buildTokens(fresh);
    QCOMPARE(view->palette().color(QPalette::HighlightedText), darkTokens.textPrimary);
    const QImage image = view->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!image.isNull());
    calendar->hide();
    QCoreApplication::processEvents();
}
QTEST_MAIN(WinUI3CalendarSurfaceTest)
#include "tst_winui3calendarsurface.moc"