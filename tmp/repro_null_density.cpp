#include <winui3style/winui3style.h>

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QStyleOptionComboBox>
#include <QStyleOptionFrame>
#include <QStyleOptionSpinBox>
#include <QFontMetrics>
#include <cstdio>

using WinUI3::DensityMode;
using WinUI3::Style;

static int heightOf(const QSize &s) { return s.height(); }

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // Application style: Standard.
    auto *appStyle = new Style(DensityMode::Standard);
    QApplication::setStyle(appStyle);

    // Standalone receiver style: Compact. NOT the application style.
    Style receiver(DensityMode::Compact);

    QFont f = app.font();
    f.setPixelSize(16);
    const QFontMetrics fm(f);

    QStyleOptionFrame lineOpt;
    lineOpt.fontMetrics = fm;
    QStyleOptionSpinBox spinOpt;
    spinOpt.fontMetrics = fm;
    QStyleOptionComboBox comboOpt;
    comboOpt.fontMetrics = fm;

    // Widgets bound to the receiver (the "correct" reference path).
    QLineEdit line; line.setStyle(&receiver); line.setFont(f);
    QSpinBox spin; spin.setStyle(&receiver); spin.setFont(f);
    QComboBox combo; combo.setStyle(&receiver); combo.setFont(f);

    const QSize contents(60, fm.height());

    printf("app style density    = %s\n",
           appStyle->densityMode() == DensityMode::Compact ? "Compact" : "Standard");
    printf("receiver density     = %s\n",
           receiver.densityMode() == DensityMode::Compact ? "Compact" : "Standard");
    printf("font height          = %d\n\n", fm.height());

    struct Row { const char *name; QStyle::ContentsType ct; const QStyleOption *opt; QWidget *w; };
    const Row rows[] = {
        { "CT_LineEdit", QStyle::CT_LineEdit, &lineOpt, &line },
        { "CT_SpinBox",  QStyle::CT_SpinBox,  &spinOpt, &spin },
        { "CT_ComboBox", QStyle::CT_ComboBox, &comboOpt, &combo },
    };

    printf("%-12s %-14s %-14s %-14s\n", "type", "null-widget", "bound-widget", "pure-Compact");
    for (const auto &r : rows) {
        const int nullH = heightOf(receiver.sizeFromContents(r.ct, r.opt, contents, nullptr));
        const int boundH = heightOf(receiver.sizeFromContents(r.ct, r.opt, contents, r.w));
        // Pure Compact reference: a Compact style that IS the app style, null widget.
        printf("%-12s %-14d %-14d %-14s\n", r.name, nullH, boundH,
               nullH == boundH ? "match" : "MIXED");
    }

    // Pure-Compact control: make a Compact style the application style, query null.
    auto *compactApp = new Style(DensityMode::Compact);
    QApplication::setStyle(compactApp);
    printf("\n-- with Compact application style (pure Compact reference) --\n");
    for (const auto &r : rows) {
        const int nullH = heightOf(receiver.sizeFromContents(r.ct, r.opt, contents, nullptr));
        printf("%-12s null-widget=%d\n", r.name, nullH);
    }
    return 0;
}