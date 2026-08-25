# WinUI replication methodology

This file is the acceptance policy for WinUI3Style. A control is not considered
covered because it builds, resembles Fluent at rest, or appears in the demo.
Coverage means that its geometry, tokens, glyphs, input states, motion, focus
modality, and popup behavior have evidence and regression checks.

## Why the first implementation failed

The initial implementation started from a generic Qt style, changed the most
obvious colors and radii, and judged mostly static screenshots. That approach
allowed several systemic errors:

- undocumented Phantom/Fusion fallbacks were mistaken for implemented controls;
- a visually plausible mark was substituted for the actual WinUI selection
  model, such as confusing ComboBox selection with a checked menu item;
- mouse-only states and transition midpoints were not captured;
- mouse focus and keyboard focus were treated as the same state;
- popup layout was validated by whether it opened, not by its icon, label,
  shortcut, submenu, and selection columns;
- custom widgets duplicated input, accessibility, and animation behavior that
  Qt already provides;
- a crash-free gallery screenshot was treated as validation.

The later improvements worked because they reversed the order: first pin the
official behavior and resources, then implement the Qt mapping, then reject the
result unless state-level tests and live interaction agree.

## Required workflow

### 1. Declare the mapping before coding

For every Qt widget, record one of these mappings in `coverage.md`:

1. **Direct equivalent** — a specific WinUI control and Microsoft resource file.
2. **Semantic variant** — a native Qt widget with a `winui*` dynamic property.
   Qt retains semantics, signals, focus, accessibility, and hit testing; QStyle
   owns every visual and animated state.
3. **Consistency extension** — Qt has no WinUI equivalent. The design must be
   derived from pinned Fluent tokens and neighboring controls and documented as
   an extension, never presented as an official template.
4. **Compound behavior** — no Qt widget supplies the required child management
   or transition behavior. A helper class is allowed only for composition and
   behavior. It must delegate Fluent chrome and control-state painting to the
   style wherever Qt exposes a style entry point.
5. **Not covered** — the widget is allowed to fall back, but may not be claimed
   by the README, demo, or coverage table as WinUI-complete.

No silent base-style fallback is permitted for categories 1–4.

Removing a fallback is not complete when the call is merely deleted. Qt can
feed provisional or sentinel content sizes during first layout (a ComboBox
popup can temporarily pass the viewport height as one row's height). The style
must normalize such inputs to the mapped control metric. First-open tests with
a nonzero selected index are mandatory after any `sizeFromContents()` change;
otherwise a hidden base-style normalization can turn into a one-frame jump.

The base style is deliberately a plain `QCommonStyle`. It may supply generic
Qt text/icon layout and behavior for controls recorded as **not covered**, but
it is not a source of WinUI policy. A covered widget must explicitly own every
relevant paint path, metric, content size, geometry, style hint, hit-testing
dependency, and interaction policy. Changes to the base-style boundary require
the full interaction suite and deterministic light/dark capture comparison;
static rendering alone is not sufficient. This rule replaces the former
vendored PhantomStyle fallback, whose inherited menu tracking, slider pointer
mapping, padding, and menu-bar sizing were undocumented dependencies.

For every `QStyle::ComplexControl`, the mapping must also be decomposed before
painting into its independently interactive subcontrols. Record the rectangle,
z-order/occlusion, rest, hover, press, enabled-at-bound, and focus ownership of
each `SubControl`. A widget-wide `State_HasFocus` is not evidence that every
subcontrol receives the same focus treatment. Tests must assert the partition
and non-overlap of those rectangles and the visual extent of focus indicators.

### 2. Pin primary evidence

- Pin the installed WinUI 3 Gallery version and Windows App Runtime version.
- Link the exact Microsoft `microsoft-ui-xaml` theme-resource/template files.
- Extract exact geometry, typography, colors, glyph source, state ordering,
  duration, and easing values into `manifest.json` or `observations.md`.
- Use third-party repositories only as implementation clues, never as the
  visual contract.

Platform values used by one token family must come from one coherent snapshot.
In particular, never combine `SystemAccentColor` from one Windows API with
Light/Dark accent-ramp entries read from a different or stale source. Record the
raw runtime values, source, theme, Windows build, and Gallery frame together,
then assert that every derived accent role stays in that same hue family. A
test which only proves that `Highlight != Accent` is insufficient: it must also
reject a mixed-source ramp.

### 3. Observe the live control with real input

At 100% scaling, in both light and dark modes, inspect at least:

- rest;
- pointer enter and settled pointer-over;
- mouse-down before release;
- mouse-up and checked/selected state;
- pointer leave;
- disabled, checked-disabled, and indeterminate where applicable;
- focus obtained by mouse;
- focus obtained by Tab/Shift+Tab;
- Space/Enter keyboard press and release;
- popup open, selected-row hover, click, dismissal, and submenu expansion;
- drag start, midpoint, edge/clamp behavior, and release for draggable controls;
- animation start, at least one midpoint, reversal, and settled endpoint.

Still screenshots are insufficient for motion. Record the transition duration
from the official XAML and inspect live start/mid/end frames. Mouse input must be
sent as separate move, down, optional drag, and up actions so the pressed frame
is observable rather than skipped by a synthetic click.

Each live pass must identify the exact two active executables, their build or
commit, the monitor DPR, theme, Windows accent, reduced-motion setting, and the
control/page being compared. Capture the first presented frame as well as the
settled frame. A later stable frame cannot validate a transition that briefly
shows old content, an opaque pre-Acrylic surface, or stale geometry.

Popup surfaces require a repeatability sequence, not one successful opening:

1. record the closed anchor rectangle and current selection;
2. on a freshly constructed control whose initial selection is not the first
   item, open by mouse and record the popup rectangle, item-view scroll value,
   and selected-row global center at `Show`, first paint, and settled state;
3. capture the settled frame after at least 60 ms while monitoring popup
   `Move`/`Resize`, viewport layout, and scroll-value changes;
4. dismiss with Escape, reopen the same selection twice, and require identical
   presented and settled geometry;
5. change the selection, reopen, and distinguish a documented pre-show
   selected-row alignment from an erroneous post-show jump;
6. exercise row hover, mouse-down, release, keyboard traversal, submenu
   expansion, and dismissal outside the popup;
7. repeat in light and dark mode with a long label, shortcut, icon, checked
   action, separator, and submenu so column sizing and clipping are observable.

An internal `Move` emitted during Qt's synchronous `show()` setup is not by
itself a visible jump. The acceptance signal is that the first presented
geometry equals the settled geometry and remains repeatable. Conversely, a
stable final screenshot does not excuse a first-frame flash, late resize, or
one-frame legacy background. A fixed top-level popup rectangle is also not
proof of stability: its viewport may scroll or relayout inside that rectangle.
Live compositor inspection remains mandatory.

### 4. Implement in the style

- Prefer a standard Qt widget plus a documented dynamic property whenever the
  standard widget already has the correct semantics.
- Keep token values centralized in `winui3tokens_p.h`.
- Implement the complete Qt contract needed by the control: primitives,
  controls, complex controls, pixel metrics, content sizing, sub-element and
  sub-control geometry, style hints, and icons.
- Drive hover, press, focus, check, and position animation through style-owned
  dynamic properties. Reversal must continue from the current value.
- Preserve right-to-left layout, disabled state, keyboard operation, palette
  changes, high DPI, and runtime light/dark switching.
- QSS and per-widget Fluent paint code are forbidden.

### 5. Validate at four levels

1. **Contract tests** — exact metrics, sub-control rectangles, glyph choice,
   property contract, popup columns, and absence of QSS.
2. **Interaction tests** — real enter/down/drag/up and keyboard events; assert
   observable intermediate animation values as well as endpoints and reversal.
3. **Deterministic visual matrix** — light/dark captures for rest, hover,
   pressed, checked, disabled, keyboard focus, popup, and transition midpoint.
4. **Live side-by-side pass** — compare Qt with the pinned Gallery using the
   same theme, scale, input sequence, and logical crop. Mica/Acrylic are only
   accepted in this live compositor pass.

Image RMS is a regression signal, not the acceptance criterion. Geometry,
state semantics, motion, and focus modality can fail even when RMS is small.

The capture matrix must enumerate every enabled page and every enabled tab.
Transient or top-level surfaces (`QMenu`, ComboBox popup views, dialogs,
message boxes and detached docks) must be shown and captured independently;
they are not validated by a screenshot of their launcher. A missing capture is
a validation failure, not evidence that the base style handled the surface.

Rest-state captures must also be insulated from external input state. Before
each frame, disable mouse delivery to the gallery, move the physical pointer
outside the window, clear keyboard focus so a blinking caret cannot enter the
image, send `Leave` to descendants, and settle the event queue. Restore the
pointer afterward. Two complete capture runs must have identical file sets and
byte-identical PNG hashes; a mismatch is investigated by pixel bounds before
it can be dismissed as environmental noise.

### 5a. Repository automation

The repository implements the deterministic portion of this policy as the
`winui3style_snapshot_matrix` CTest. It launches the gallery twice in fresh
processes with animations and accessibility disabled before construction,
requires identical file manifests and image dimensions, and compares every
pixel. It then requires the same strict comparison against the approved
`spec/baselines/gallery`; a missing baseline or missing/extra state capture is
a failure. On Windows, offscreen captures explicitly use the system font
directory so Segoe and Fluent glyphs cannot silently become tofu boxes.
`tools/compare_images.py`
never resizes inputs and reports dimensions, RMS, differing-pixel count,
maximum channel delta, and the difference bounding box.

`winui3style_native` is a serial Windows CTest using the native platform
plugin. It exercises real tooltip, menu, ComboBox popup, dialog theme/accent
updates, and floating-dock cleanup/focus where the interactive desktop grants
focus. Fast offscreen contract tests remain separate. The DPI contract is run
at 100%, 125%, 150%, and 200% logical scale factors where Qt permits it. Native
focus or compositor behavior that the current session refuses is recorded as
skipped/blocked evidence rather than promoted to verified coverage.

### 6. Definition of done

A row may be marked **verified** in `coverage.md` only when:

- its mapping and official source are recorded;
- all applicable live states above were inspected;
- every required QStyle path is implemented without an undocumented fallback;
- interaction and geometry tests pass;
- light and dark deterministic captures were reviewed;
- any deliberate Qt/WinUI deviation is written down;
- the demo exposes the control and the states needed to reproduce the review.

Any later regression reopens the row. “Partial” is an acceptable status;
unearned “verified” is not.

A live defect report immediately invalidates the affected evidence even when
CTest remains green. First reproduce it in the running Gallery, add a test that
fails for the observed mechanism (not merely for the final screenshot), and
only then apply the fix. The repaired build must be relaunched and the same
input sequence repeated against the official Gallery before the row can return
to **verified**.
