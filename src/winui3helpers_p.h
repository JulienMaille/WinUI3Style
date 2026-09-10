// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Small, pure widget-classification and frame-state helpers shared by the
// per-element renderer translation units. Previously every TU kept its own
// copy in an anonymous namespace; the copies had started to drift, so they
// now live here as the single definition.

#include "winui3backdrop_p.h"
#include "winui3frameproperties_p.h"
#include "winui3style_properties_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadioButton>
#include <QRect>
#include <QToolButton>
#include <QWidget>

namespace WinUI3::Private {

// Animated progress for a transient frame state (hover, press, focus...).
inline qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0)
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
                || qobject_cast<const QToolButton *>(widget)
                // A 250 ms RadioButton state transition can otherwise be
                // reversed before producing a painted frame during rapid clicks.
                || qobject_cast<const QRadioButton *>(widget));
}

// Focus visuals only appear after actual keyboard interaction.
inline bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
}

// True only when the widget paints straight onto the window surface: either
// a live DWM material or an opaque window fill. Walk the parent chain for an
// intervening opaque content/layer surface and stop there: a child control
// must never Source-clear over an opaque island fill, otherwise it punches a
// hole through that surface. A translucent island (zero-alpha Window role)
// has no fill to punch through, so the walk continues past it: children
// above a live material must clear to transparent on every paint, otherwise
// scroll/hover/animation frames accumulate as permanent smear (the island
// itself almost never repaints, so its own erase cannot cover them). The
// walk starts at the parent, so the translucent island's own PE_Widget clear
// (the island erase, gated on its zero-alpha Window role) is unaffected.
// Critical subtlety: an opaque window ignores the backing store's alpha
// channel at presentation, so clearing there is always safe and required for
// animation frames not to accumulate. On a translucent window the alpha is
// presented, so clearing is allowed strictly when the compositor owns the
// material (Composited): clearing over a painted fallback, a failed DWM
// call, or pre-composited first frames reveals black or stale pixels instead
// of wallpaper tint.
inline bool paintsDirectlyOnBackdrop(const QWidget *widget)
{
    if (!widget || !widget->window())
        return false;
    if (!widget->window()->property("_winui_backdrop").isValid())
        return false;
    if (widget->window()->testAttribute(Qt::WA_TranslucentBackground)
        && backdropEffectiveSurface(widget->window()) != BackdropSurface::Composited)
        return false;
    for (const QWidget *parent = widget->parentWidget(); parent && parent != widget->window();
         parent = parent->parentWidget()) {
        const QVariant surface = parent->property(Style::SurfaceProperty);
        const QString name = surface.toString();
        if (surface.toBool() || name.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
            || name.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
            // Translucent islands are the material surface itself: no fill to
            // punch through, keep walking so children clear every frame.
            if (parent->palette().color(QPalette::Window).alpha() == 0)
                continue;
            return false;
        }
    }
    return true;
}
// Source-clear one rect over a live material, gated on paintsDirectlyOnBackdrop.
// Every painter below that lays a fill over a translucent island must call this
// first: Qt keeps backing-store rows between frames (blit + partial expose),
// so a fill without erase re-blends over stale rows (white hover fields,
// gray bands, retained scroll rows; only a resize heals). Returns true when
// the erase ran. painters that only lay text/glyphs over an already-erased
// rect (labels, item delegates) must NOT call it. Pass the fill radius to
// clip the clear inside the painted shape (antialiased corners outside the
// rounded rect would otherwise keep transparent-black dots).
inline bool eraseForBackdrop(QPainter *painter, const QWidget *widget, const QRect &rect,
                             qreal radius = 0.0)
{
    if (!painter || !paintsDirectlyOnBackdrop(widget))
        return false;
    painter->save();
    if (radius > 0.0) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rect), radius, radius);
        painter->setClipPath(clip);
    }
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(rect, Qt::transparent);
    painter->restore();
    return true;
}
} // namespace WinUI3::Private
