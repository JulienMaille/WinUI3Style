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

} // namespace WinUI3::Private
