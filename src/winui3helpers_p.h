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
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
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

inline bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget)
            && widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

inline bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
            && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

inline bool comboBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
            && qobject_cast<const QComboBox *>(widget->parentWidget());
}

// The trailing clear/completer button inside a QLineEdit.
inline bool textBoxHelperButton(const QWidget *widget)
{
    if (!widget)
        return false;
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

// Backdrop erase policy (single strategy; mapping first per METHODOLOGY section 1).
// Every painter that covers a live material with a fill follows one recipe below.
// The gate (paintsDirectlyOnBackdrop) is shared; only the recipe differs by
// surface kind. New painters must pick a recipe here, never hand-roll a
// CompositionMode_Source path at the call site.
//
// Recipe 1, erase-then-fill (default): eraseForBackdrop, then paint the fill.
//   Buttons, combo/spin/slider/scrollbar frames, PE_Widget islands, CE labels,
//   CC_ToolButton/CC_GroupBox, item rows, dock panels, navigation rows.
// Recipe 2, erase-means-transparent: eraseForBackdrop, then return with no
//   fill. Menu-bar rest state, subtle-at-rest buttons outside group cards.
// Recipe 3, Source-composite of the translucent surface (acrylic popups only):
//   clearForBackdropFill. Menu item hover/pressed pills over a composited
//   acrylic presenter: the native erase is a no-op there, so the already
//   translucent surface roles are Source-blended explicitly and the pill
//   composites exactly one SubtleFill layer.
// Focus visuals only appear after actual keyboard interaction.
inline bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && framePropertyRegistry().value(widget, focusVisibleProperty).toBool();
}

// Preserve QVariant's existing bool conversion as well as the named surfaces.
inline bool isContentLayerSurface(const QVariant &surface, const QString &name)
{
    return surface.toBool() || name.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
            || name.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0;
}

inline bool isContentLayerSurface(const QVariant &surface)
{
    return isContentLayerSurface(surface, surface.toString());
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
    if (!widget)
        return false;
    const QWidget *win = widget->window();
    if (!win)
        return false;
    // Explicit enabled check: a present-but-None (0) "_winui_backdrop"
    // property means the backdrop is disabled and must never enable
    // Source-clear (isValid alone would still punch holes).
    const QVariant backdrop = win->property("_winui_backdrop");
    if (backdrop.toInt() <= static_cast<int>(Backdrop::None))
        return false;
    if (win->testAttribute(Qt::WA_TranslucentBackground)
        && backdropEffectiveSurface(win) != BackdropSurface::Composited)
        return false;
    // Self-check: the ancestor walk below starts at parentWidget(), so
    // without this an opaque content/layer island reports true for itself
    // and Source-clears over its own opaque fill (hole punch). Opaque
    // self never paints directly on the backdrop; a translucent self
    // (zero-alpha Window role, the island erase) falls through so the
    // walk above still resolves the backdrop source.
    if (isContentLayerSurface(widget->property(Style::SurfaceProperty))) {
        if (widget->palette().color(QPalette::Window).alpha() != 0)
            return false;
    }
    for (const QWidget *parent = widget->parentWidget(); parent && parent != win;
         parent = parent->parentWidget()) {
        if (isContentLayerSurface(parent->property(Style::SurfaceProperty))) {
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
        // Intersect, never replace: hover/scroll repaints arrive with a
        // dirty-region clip already set. Replacing it widens the Source
        // clear to the full widget rect and wipes sibling content (the
        // hover-vanish: backpanel painted over widgets on live Mica).
        painter->setClipPath(clip, Qt::IntersectClip);
    }
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(rect, Qt::transparent);
    painter->restore();
    return true;
}

// Recipe 3 implementation: Source-blend the translucent popup surface so a
// state pill composites exactly one layer over acrylic. Gated the same way
// as eraseForBackdrop; returns true when the fill ran. Pass the popup
// surface radius to clip the Source fill inside the rounded surface: the
// row rect is square, so an unclipped fill overpaints the surface corner
// cutouts with a square halo. Same intersect-not-replace clip contract as
// eraseForBackdrop.
inline bool clearForBackdropFill(QPainter *painter, const QWidget *widget, const QRect &rect,
                                 const QColor &fill, qreal radius = 0.0)
{
    if (!painter || !paintsDirectlyOnBackdrop(widget))
        return false;
    painter->save();
    if (radius > 0.0) {
        QPainterPath clip;
        clip.addRoundedRect(QRectF(rect), radius, radius);
        // Intersect, never replace: see eraseForBackdrop.
        painter->setClipPath(clip, Qt::IntersectClip);
    }
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(rect, fill);
    painter->restore();
    return true;
}
} // namespace WinUI3::Private
