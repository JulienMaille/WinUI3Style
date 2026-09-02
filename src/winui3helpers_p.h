#pragma once

// Small, pure widget-classification and frame-state helpers shared by the
// per-element renderer translation units. Previously every TU kept its own
// copy in an anonymous namespace; the copies had started to drift, so they
// now live here as the single definition.

#include "winui3frameproperties_p.h"
#include "winui3style_properties_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>

namespace WinUI3::Private {

// Animated progress for a transient frame state (hover, press, focus...).
inline qreal progress(const QWidget *widget, const char *name,
                      qreal fallback = 0.0)
{
    return framePropertyRegistry().real(widget, name, fallback);
}

// A QCheckBox promoted to WinUI's ToggleSwitch control.
inline bool toggleSwitch(const QWidget *widget)
{
    return qobject_cast<const QCheckBox *>(widget)
        && widget->property(Style::ToggleSwitchProperty).toBool();
}

// The trailing clear/completer button inside a QLineEdit.
inline bool textBoxHelperButton(const QWidget *widget)
{
    return qobject_cast<const QAbstractButton *>(widget)
        && qobject_cast<const QLineEdit *>(widget->parentWidget());
}

// Whether a button-like surface pulses on press (all of them except the
// discrete TextBox helper button).
inline bool buttonPressPulse(const QWidget *widget)
{
    return !textBoxHelperButton(widget)
        && (qobject_cast<const QPushButton *>(widget)
            || qobject_cast<const QToolButton *>(widget));
}

// Focus visuals only appear after actual keyboard interaction.
inline bool keyboardFocusVisible(const QWidget *widget)
{
    return widget
        && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
}

// True only when the widget paints straight into an active translucent DWM
// backdrop. An intervening content/layer surface is opaque and must never be
// cleared, otherwise a child control would punch through that surface.
inline bool paintsDirectlyOnBackdrop(const QWidget *widget)
{
    if (!widget || !widget->window()
        || widget->window()->property("_winui_backdrop").toInt() == 0)
        return false;
    for (const QWidget *parent = widget->parentWidget();
         parent && parent != widget->window(); parent = parent->parentWidget()) {
        const QVariant surface = parent->property(Style::SurfaceProperty);
        const QString name = surface.toString();
        if (surface.toBool()
            || name.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
            || name.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0)
            return false;
    }
    return true;
}

} // namespace WinUI3::Private
