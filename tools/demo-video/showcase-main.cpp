#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCursor>
#include <QDateEdit>
#include <QDir>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionSlider>
#include <QStyleOptionButton>
#include <QStyleOptionSpinBox>
#include <QAbstractSpinBox>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QWindow>
#include <dwmapi.h>
#include <QPixmap>
#include <QTabBar>
#include <QTabWidget>
#include <QAbstractItemView>
#include <QTimeEdit>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <windows.h>
#include <functional>

namespace {
struct Panel {
    QWidget *root{};
    QStackedWidget *pages{};
    QCheckBox *check{};
    QRadioButton *radioB{};
    QCheckBox *wifi{};
    QCheckBox *bluetooth{};
    QComboBox *combo{};
    QSpinBox *spin{};
    QLineEdit *edit{};
    QSlider *slider{};
    QPushButton *standard{};
    QPushButton *accent{};
    QPushButton *destructive{};
    QPushButton *more{};
    QDateEdit *date{};
    QTimeEdit *time{};
    QTabWidget *tabs{};
    QScrollBar *scroll{};
};

QWidget *page(QStackedWidget *stack)
{
    auto *w = new QWidget(stack);
    w->setAutoFillBackground(true);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(12, 14, 12, 12);
    layout->setSpacing(14);
    return w;
}

void add(QWidget *w, QWidget *child, Qt::Alignment align = {})
{
    static_cast<QVBoxLayout *>(w->layout())->addWidget(child, 0, align);
}

void finish(QStackedWidget *stack, QWidget *w)
{
    static_cast<QVBoxLayout *>(w->layout())->addStretch();
    stack->addWidget(w);
}

void moveCursor(QObject *context, const QPoint &target, std::function<void()> arrived)
{
    const QPoint start = QCursor::pos();
    constexpr int steps = 15;
    for (int step = 1; step <= steps; ++step) {
        QTimer::singleShot(step * 16, Qt::PreciseTimer, context, [start, target, step, arrived] {
            const qreal t = qreal(step) / steps;
            const qreal eased = t * t * (3.0 - 2.0 * t);
            QCursor::setPos(start + (target - start) * eased);
            if (step == steps && arrived)
                arrived();
        });
    }
}

Panel makePanel(QWidget *parent, bool dark)
{
    Panel p;
    p.root = new QWidget(parent);
    p.root->setAutoFillBackground(true);
    p.root->setProperty("winuiThemeLabel", dark ? "Dark" : "Light");
    auto *rootLayout = new QVBoxLayout(p.root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    p.pages = new QStackedWidget(p.root);
    p.pages->setAutoFillBackground(true);
    rootLayout->addWidget(p.pages, 1);

    QWidget *one = page(p.pages);
    auto *selection = new QWidget(one);
    auto *selectionLayout = new QVBoxLayout(selection);
    selectionLayout->setContentsMargins(0, 0, 0, 0);
    selectionLayout->setSpacing(12);
    p.check = new QCheckBox(QStringLiteral("Check box"));
    auto *radioA = new QRadioButton(QStringLiteral("Option A"));
    p.radioB = new QRadioButton(QStringLiteral("Option B"));
    auto *group = new QButtonGroup(one);
    group->addButton(radioA); group->addButton(p.radioB); radioA->setChecked(true);
    p.wifi = new QCheckBox(QStringLiteral("Wi-Fi"));
    p.wifi->setProperty("winuiToggleSwitch", true);
    p.wifi->setProperty("winuiOnText", QStringLiteral("On"));
    p.wifi->setProperty("winuiOffText", QStringLiteral("Off"));
    p.bluetooth = new QCheckBox(QStringLiteral("Bluetooth"));
    p.bluetooth->setProperty("winuiToggleSwitch", true);
    p.bluetooth->setProperty("winuiOnText", QStringLiteral("On"));
    p.bluetooth->setProperty("winuiOffText", QStringLiteral("Off"));
    p.bluetooth->setChecked(true);
    selectionLayout->addWidget(p.check); selectionLayout->addWidget(radioA);
    selectionLayout->addWidget(p.radioB); selectionLayout->addWidget(p.wifi);
    selectionLayout->addWidget(p.bluetooth);
    add(one, selection, Qt::AlignHCenter);
    finish(p.pages, one);

    QWidget *two = page(p.pages);
    p.spin = new QSpinBox; p.spin->setRange(0, 100); p.spin->setValue(38);
    p.combo = new QComboBox;
    p.combo->addItems({QStringLiteral("First item"), QStringLiteral("Second item"), QStringLiteral("Third item")});
    p.slider = new QSlider(Qt::Horizontal); p.slider->setRange(0, 100); p.slider->setValue(40);
    p.edit = new QLineEdit;
    p.edit->setPlaceholderText(QStringLiteral("Type something"));
    p.edit->setClearButtonEnabled(true);
    add(two, p.spin); add(two, p.combo); add(two, p.slider); add(two, p.edit);
    finish(p.pages, two);

    QWidget *three = page(p.pages);
    p.standard = new QPushButton(QStringLiteral("Standard"));
    p.accent = new QPushButton(QStringLiteral("Accent"));
    p.accent->setProperty("winuiControlRole", QStringLiteral("accent"));
    p.destructive = new QPushButton(QStringLiteral("Delete"));
    p.destructive->setProperty("winuiControlRole", QStringLiteral("destructive"));
    p.more = new QPushButton(QStringLiteral("More"));
    auto *menu = new QMenu(p.more);
    const auto item = [menu](QStyle::StandardPixmap pixmap, const QString &text) {
        menu->addAction(menu->style()->standardIcon(pixmap, nullptr, menu), text);
    };
    item(QStyle::SP_FileIcon, QStringLiteral("New document"));
    item(QStyle::SP_DirOpenIcon, QStringLiteral("Open folder"));
    item(QStyle::SP_DialogSaveButton, QStringLiteral("Save"));
    item(QStyle::SP_BrowserReload, QStringLiteral("Refresh"));
    p.more->setMenu(menu);
    auto *buttons = new QWidget(three);
    auto *grid = new QGridLayout(buttons);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);
    grid->addWidget(p.standard, 0, 0); grid->addWidget(p.accent, 0, 1);
    grid->addWidget(p.more, 1, 0); grid->addWidget(p.destructive, 1, 1);
    auto *progress = new QProgressBar; progress->setRange(0, 0);
    add(three, buttons); add(three, progress);
    finish(p.pages, three);

    QWidget *four = page(p.pages);
    static_cast<QVBoxLayout *>(four->layout())->setSpacing(12);
    p.date = new QDateEdit(QDate(2026, 11, 10)); p.date->setDisplayFormat(QStringLiteral("dd/MM/yy"));
    p.time = new QTimeEdit(QTime(9, 30));
    p.tabs = new QTabWidget; p.tabs->setFixedHeight(66);
    p.tabs->addTab(new QLabel(QStringLiteral("Overview")), QStringLiteral("Home"));
    p.tabs->addTab(new QLabel(QStringLiteral("Recent files")), QStringLiteral("Recent"));
    p.scroll = new QScrollBar(Qt::Horizontal); p.scroll->setRange(0, 100); p.scroll->setPageStep(15); p.scroll->setValue(20);
    add(four, p.date); add(four, p.time); add(four, p.tabs); add(four, p.scroll);
    finish(p.pages, four);
    return p;
}

void click(QWidget *w, QWidget *window, const QPoint &at = QPoint(-1, -1))
{
    QPoint pos = at.x() < 0 ? w->rect().center() : at;
    if (at.x() < 0 && qobject_cast<QCheckBox *>(w) && !w->property("winuiToggleSwitch").toBool()) {
        QStyleOptionButton option;
        option.initFrom(w);
        const QRect indicator = w->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &option, w);
        pos = indicator.bottomRight() - QPoint(2, 2);
    }
    Q_UNUSED(window);
    moveCursor(w, w->mapToGlobal(pos), [w] {
        INPUT down{}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        QTimer::singleShot(120, w, [] {
            INPUT up{}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &up, sizeof(INPUT));
        });
    });
}

void clickSpinArrow(QAbstractSpinBox *spin, QWidget *window, QStyle::SubControl arrow)
{
    QStyleOptionSpinBox option;
    option.initFrom(spin);
    option.frame = spin->hasFrame();
    option.buttonSymbols = spin->buttonSymbols();
    option.stepEnabled = QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
    const QRect rect = spin->style()->subControlRect(QStyle::CC_SpinBox, &option, arrow, spin);
    click(spin, window, rect.center());
}

QCursor largerWindowsArrow()
{
    constexpr int size = 48;
    HCURSOR cursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, size, size, LR_SHARED));
    if (!cursor)
        return QCursor(Qt::ArrowCursor);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bitmap || !bits) {
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
        return QCursor(Qt::ArrowCursor);
    }
    HGDIOBJ old = SelectObject(dc, bitmap);
    memset(bits, 0, size * size * 4);
    DrawIconEx(dc, 0, 0, cursor, size, size, 0, nullptr, DI_NORMAL);
    QImage image(static_cast<const uchar *>(bits), size, size, QImage::Format_ARGB32_Premultiplied);
    const QPixmap pixmap = QPixmap::fromImage(image.copy());
    SelectObject(dc, old);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return QCursor(pixmap, 0, 0);
}

void openComboWithHover(QComboBox *combo, QWidget *window)
{
    Q_UNUSED(window);
    moveCursor(combo, combo->mapToGlobal(combo->rect().center()), [combo] {
    QTimer::singleShot(350, combo, [combo] {
        INPUT down{}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        QTimer::singleShot(180, combo, [] {
            INPUT up{}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &up, sizeof(INPUT));
        });
    });
    });
}

void chooseComboWithHover(QComboBox *combo, QWidget *window)
{
    QAbstractItemView *view = combo->view();
    if (qEnvironmentVariableIsSet("WINUI_TEST_NATIVE_REGION")) {
        QWidget *surface = view->window();
        const qreal scale = surface->devicePixelRatioF();
        const int w = qRound(surface->width() * scale), h = qRound(surface->height() * scale);
        const int diameter = qRound(16 * scale);
        HRGN region = CreateRoundRectRgn(0, 0, w + 1, h + 1, diameter, diameter);
        const int applied = region ? SetWindowRgn(reinterpret_cast<HWND>(surface->winId()), region, TRUE) : 0;
        if (region && !applied) DeleteObject(region);
        HRGN check = CreateRectRgn(0, 0, 0, 0);
        const int type = GetWindowRgn(reinterpret_cast<HWND>(surface->winId()), check);
        if (check) DeleteObject(check);
        QFile log(QDir::tempPath() + QStringLiteral("/winui-region-result.txt"));
        if (log.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&log); out << "applied=" << applied << " regionType=" << type << " size=" << w << 'x' << h << " diameter=" << diameter << '\n';
        }
    }
    QFile diagnostic(QDir::tempPath() + QStringLiteral("/winui-combo-inspect.txt"));
    if (diagnostic.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&diagnostic);
        QWidget *popup = view->window();
        DWORD backdrop = 999, corner = 999;
        DwmGetWindowAttribute(reinterpret_cast<HWND>(popup->winId()), 38, &backdrop, sizeof(backdrop));
        DwmGetWindowAttribute(reinterpret_cast<HWND>(popup->winId()), 33, &corner, sizeof(corner));
        out << "theme " << qEnvironmentVariable("WINUI_DEMO_THEME") << " DWM " << backdrop << " corner " << corner << " maskempty " << popup->mask().isEmpty() << " frame " << (qobject_cast<QFrame *>(popup) ? int(qobject_cast<QFrame *>(popup)->frameShape()) : -1) << "\n";
        QList<QWidget *> widgets = popup->findChildren<QWidget *>(); widgets.prepend(popup);
        for (QWidget *w : widgets) {
            out << w->metaObject()->className() << " name=" << w->objectName() << " geom=" << w->geometry().x() << ',' << w->geometry().y() << ',' << w->width() << ',' << w->height()
                << " alpha=" << w->testAttribute(Qt::WA_TranslucentBackground) << " opaque=" << w->testAttribute(Qt::WA_OpaquePaintEvent) << " autofill=" << w->autoFillBackground()
                << " styled=" << w->testAttribute(Qt::WA_StyledBackground) << " effective=" << w->property("_winui_backdrop_effective").toString()
                << " window=" << w->palette().color(QPalette::Window).name(QColor::HexArgb) << " base=" << w->palette().color(QPalette::Base).name(QColor::HexArgb) << '\n';
        }
    }
    const QModelIndex index = combo->model()->index(1, combo->modelColumn(), combo->rootModelIndex());
    const QPoint target = view->viewport()->mapToGlobal(view->visualRect(index).center());
    Q_UNUSED(window);
    moveCursor(combo, target, [combo] {
    QTimer::singleShot(350, combo, [combo] {
        INPUT down{}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        QTimer::singleShot(180, combo, [] {
            INPUT up{}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &up, sizeof(INPUT));
        });
    });
    });
}

void key(WORD vk)
{
    INPUT in[2]{}; in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = vk;
    in[1] = in[0]; in[1].ki.dwFlags = KEYEVENTF_KEYUP; SendInput(2, in, sizeof(INPUT));
}

// One key per tick so the recording shows real typing, not a pasted string.
void typeText(QObject *context, const QString &text, int intervalMs = 95)
{
    for (int i = 0; i < text.size(); ++i) {
        const ushort ch = text.at(i).unicode();
        QTimer::singleShot(i * intervalMs, Qt::PreciseTimer, context, [ch] {
            INPUT in[2]{};
            in[0].type = INPUT_KEYBOARD; in[0].ki.wScan = ch; in[0].ki.dwFlags = KEYEVENTF_UNICODE;
            in[1] = in[0]; in[1].ki.dwFlags |= KEYEVENTF_KEYUP;
            SendInput(2, in, sizeof(INPUT));
        });
    }
}

QWidget *clearButton(QLineEdit *edit)
{
    for (QToolButton *button : edit->findChildren<QToolButton *>()) {
        if (button->isVisible())
            return button;
    }
    return nullptr;
}

void clickMenuItem(QMenu *menu, int index)
{
    const QRect item = menu->actionGeometry(menu->actions().at(index));
    moveCursor(menu, menu->mapToGlobal(item.center()), [menu] {
        QTimer::singleShot(450, menu, [] {
            INPUT down{}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &down, sizeof(INPUT));
            QTimer::singleShot(120, [] {
                INPUT up{}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                SendInput(1, &up, sizeof(INPUT));
            });
        });
    });
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOverrideCursor(largerWindowsArrow());
    const bool dark = qEnvironmentVariable("WINUI_DEMO_THEME").compare(QStringLiteral("Dark"), Qt::CaseInsensitive) == 0;
    if (QStyle *style = QStyleFactory::create(QStringLiteral("winui3"))) {
        style->setProperty("themeMode", dark ? 2 : 1); // WinUI3::ThemeMode::Dark/Light
        app.setStyle(style);
    }
    QWidget window;
    window.setWindowTitle(dark ? QStringLiteral("WinUI 3 style — Dark") : QStringLiteral("WinUI 3 style — Light"));
    window.setFixedSize(256, 256);
    auto *layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    Panel p = makePanel(&window, dark);
    layout->addWidget(p.root);
    window.show();
    // Square corners keep desktop pixels out of the cropped client capture.
    const DWORD squareCorners = 1; // DWMWCP_DONOTROUND
    DwmSetWindowAttribute(reinterpret_cast<HWND>(window.winId()), 33, &squareCorners, sizeof(squareCorners));
    if (qEnvironmentVariableIsSet("WINUI_INSPECT_POPUP")) {
        p.pages->setCurrentIndex(1);
        QTimer::singleShot(3500, p.combo, [&p] { p.combo->showPopup(); });
        QTimer::singleShot(5000, p.combo, [&p, &window] {
            if (qEnvironmentVariableIsSet("WINUI_TEST_NATIVE_REGION"))
                chooseComboWithHover(p.combo, &window);
            if (qEnvironmentVariableIsSet("WINUI_TEST_NO_NATIVE_ACRYLIC")) {
                const int none = 1;
                DwmSetWindowAttribute(reinterpret_cast<HWND>(p.combo->view()->window()->winId()), 38, &none, sizeof(none));
            }
        });
        return app.exec();
    }
    // Allow the capture process to attach to the live window before the first
    // input; this keeps the entire 167 ms checkmark reveal inside the recording.
    // Coarse timers may drift 5% (over a second at this length) and reorder scenes.
    auto after = [](int ms, QObject *ctx, auto fn) { QTimer::singleShot(2000 + ms, Qt::PreciseTimer, ctx, fn); };
    after(500, p.check, [&p, &window] { click(p.check, &window); });
    after(1200, p.radioB, [&p, &window] { click(p.radioB, &window); });
    after(2200, p.wifi, [&p, &window] { click(p.wifi, &window); });
    after(3000, p.bluetooth, [&p, &window] { click(p.bluetooth, &window); });
    after(3900, p.pages, [&p] { p.pages->setCurrentIndex(1); });
    // Scene 2 follows the visual order: SpinBox, ComboBox, Slider, LineEdit.
    after(4500, p.spin, [&p, &window] { clickSpinArrow(p.spin, &window, QStyle::SC_SpinBoxUp); });
    after(5200, p.spin, [&p, &window] { clickSpinArrow(p.spin, &window, QStyle::SC_SpinBoxDown); });
    after(5900, p.combo, [&p, &window] { openComboWithHover(p.combo, &window); });
    after(7400, p.combo, [&p, &window] { chooseComboWithHover(p.combo, &window); });
    after(8500, p.slider, [&p, &window] {
        QStyleOptionSlider option;
        option.initFrom(p.slider);
        option.orientation = p.slider->orientation();
        option.minimum = p.slider->minimum(); option.maximum = p.slider->maximum();
        option.sliderPosition = p.slider->sliderPosition(); option.sliderValue = p.slider->value();
        option.upsideDown = false;
        const QPoint handle = p.slider->style()->subControlRect(QStyle::CC_Slider, &option,
                              QStyle::SC_SliderHandle, p.slider).center();
        const int from = handle.x();
        const int y = handle.y();
        const int to = p.slider->width() * 4 / 5;
        moveCursor(p.slider, p.slider->mapToGlobal(QPoint(from, y)), [slider = p.slider, y, from, to] {
        INPUT down{}; down.type = INPUT_MOUSE; down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(INPUT));
        for (int step = 1; step <= 40; ++step) {
            QTimer::singleShot(step * 28, Qt::PreciseTimer, slider, [slider, y, from, to, step] {
                QCursor::setPos(slider->mapToGlobal(QPoint(from + (to - from) * step / 40, y)));
                if (step == 40) {
                    INPUT up{}; up.type = INPUT_MOUSE; up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &up, sizeof(INPUT));
                }
            });
        }
        });
    });
    after(10100, p.edit, [&p, &window] { click(p.edit, &window); });
    after(10600, p.edit, [&p] { typeText(p.edit, QStringLiteral("WinUI in Qt")); });
    after(12300, p.edit, [&p, &window] {
        if (QWidget *clear = clearButton(p.edit))
            click(clear, &window);
    });
    after(13200, p.edit, [&p] { typeText(p.edit, QStringLiteral("Fluent widgets")); });
    after(15000, p.pages, [&p] { p.pages->setCurrentIndex(2); });
    after(15600, p.standard, [&p, &window] { click(p.standard, &window); });
    after(16500, p.accent, [&p, &window] { click(p.accent, &window); });
    after(17400, p.destructive, [&p, &window] { click(p.destructive, &window); });
    after(18400, p.more, [&p, &window] { click(p.more, &window); });
    after(19500, p.more, [&p] {
        QMenu *menu = p.more->menu();
        moveCursor(menu, menu->mapToGlobal(menu->actionGeometry(menu->actions().at(1)).center()), {});
    });
    after(20200, p.more, [&p] { clickMenuItem(p.more->menu(), 2); });
    after(21800, p.pages, [&p] { p.pages->setCurrentIndex(3); });
    after(22400, p.date, [&p, &window] { clickSpinArrow(p.date, &window, QStyle::SC_SpinBoxUp); });
    after(23400, p.time, [&p, &window] { clickSpinArrow(p.time, &window, QStyle::SC_SpinBoxDown); });
    after(24500, p.tabs->tabBar(), [&p, &window] { click(p.tabs->tabBar(), &window, p.tabs->tabBar()->tabRect(1).center()); });
    after(25400, p.scroll, [&p, &window] { click(p.scroll, &window, QPoint(p.scroll->width() * 3 / 4, p.scroll->height() / 2)); });
    // Exercise the documented application property on the real window, not a
    // fabricated material overlay in the video compositor.
    after(26800, &window, [&window] { window.setProperty("winuiBackdrop", QStringLiteral("mica")); });
    return app.exec();
}
