# Live Gallery observations

Reference environment captured from the installed Microsoft Store package on
2026-08-22:

- `Microsoft.WinUI3ControlsGallery` 2.9.3.0
- dependency `Microsoft.WindowsAppRuntime.2` 2.4.0.0
- 100% display scaling, light and dark theme

## Acceptance matrix

| Control | Rest | Pointer over | Pressed | Mouse focus | Keyboard focus | Popup/open |
|---|---:|---:|---:|---:|---:|---:|
| Button | required | required | required | no focus visual | dual focus stroke | n/a |
| TextBox | required | required | n/a | 2 px accent underline only | underline plus reveal focus | n/a |
| ComboBox | required | required | required | no reveal stroke | 2 px outer reveal stroke | Acrylic popup |
| CheckBox | 20 px box / 12 px glyph | required | required | no focus visual | reveal focus | 167 ms animated accept glyph |
| RadioButton | 20 px outer / 12 px dot | 14 px dot, 250 ms | 10 px dot, 250 ms | no focus visual | reveal focus | discrete checked-state switch |
| MenuFlyout item | 36 px Qt row | required | required | n/a | required | Acrylic presenter; icon, label, shortcut and submenu columns |
| ToggleSwitch | 40×20 / 12 knob | 14×14 knob, 83 ms | 17×14 knob, 83 ms | no reveal stroke | reveal stroke | 83 ms knob travel |
| Navigation item | transparent | subtle secondary | subtle tertiary | no reveal stroke | reveal stroke | 250 ms content transition |
| Slider | 4 px track / ~10 px inner thumb | 14 px inner thumb, 250 ms | ~8.5 px inner thumb, 250 ms | no focus visual | root focus stroke | n/a |
| ScrollBar | 8 px trailing thumb | 12 px thumb + track/buttons, 167 ms | 87.5% arrow scale | n/a | n/a | 400 ms expand / delayed contract |
| TabView item | transparent + separator | layer-alt secondary | layer-alt default | no focus visual | reveal focus | discrete selection; 200 ms reorder recovery |
| ListView item | transparent, 40 px min | subtle secondary | subtle tertiary | no focus visual | reveal focus | common state changes are discrete |
| TreeView item | transparent, 28 px | subtle secondary | subtle tertiary | no focus visual | reveal focus | common state changes are discrete |
| ContentDialog | solid base, 24 px inset | n/a | n/a | n/a | default button focus | opacity 83 ms + scale 250 ms |

TextBox common-state changes are discrete: Microsoft's template contains
setters for `PointerOver` and `Focused`, but no timed `VisualTransition` or
surface `Pressed` state. Its exact content padding is `10,5,6,6`; normal border
thickness is 1 px and focused thickness is `1,1,1,2`. The optional 30 px clear
button uses Segoe Fluent glyph U+E894 and owns independent hover/pressed states.
The Qt style therefore must not reuse Button's 83 ms brush transition for the
TextBox surface.

The ComboBox popup has an 8 px outer radius, 40 px item rows, 5×2 px item
margins, 3 px item radius, a 3×16 px selection pill, primary selected text,
and no check-mark selection marker. The popup uses Desktop Acrylic and a system
shadow. The closed control is 32 px high with a 4 px radius and a 12 px Segoe
Fluent chevron at a 14 px right margin.

The ComboBox chevron is not an open/closed rotation. Microsoft's template
sets `AnimatedChevronDownSmallVisualSource` to `Normal`, `PointerOver`, and
`Pressed`; its generated marker table makes Normal↔PointerOver a zero-length
transition. Press translates the 48 px source from y=24 to y=31.5 through
frame 9 (about 151 ms), which is 1.875 logical px at the 12 px rendered size.
Release runs from y=31.5 through y=21 and back to y=24 over about 300 ms, an
approximately 0.75 px upward rebound. Popup `Opened`/`Closed` states animate
the presenter separately and do not rotate this glyph.

Live mouse validation on 2026-08-22 confirmed that the selected ComboBox row
retains the vertical accent pill while another row is hovered. Mouse-open plus
Escape dismissal does not create a reveal focus ring on the closed official
control; therefore Escape alone is not treated as keyboard-navigation modality
by the Qt style.

Side-by-side live validation on 2026-08-23 established the popup positioning
contract. The official ComboBox positions its presenter before display so the
current selected-row center overlays the closed control. Changing the selection
from the first to the second row therefore moves the popup's initial top edge by
exactly one row; this is intentional anchoring, not a post-show jump. Reopening
the same selection produces the same geometry. The Qt style opts into
`SH_ComboBox_Popup`, whose Qt 6.9 implementation computes this selected-row
alignment and final geometry before `show()`, and consequently bypasses Qt's
legacy 150 ms `qScrollEffect` path. Tests require no geometry change during the
settled period and exact geometry on same-selection reopening.

The first-opening regression case must start with a non-zero current index on a
new ComboBox. Qt performs a second `scrollTo(currentIndex, PositionAtCenter)`
after calling `show()`; monitoring only the popup window rectangle can therefore
miss a viewport-content jump. WinUI3Style pre-creates and lays out the popup on
mouse or keyboard activation, and repeats the selected-row positioning during
the synchronous `Show` event before the first paint. The regression test records
the selected-row center and scrollbar value at `Show` and after `showPopup()`.

The same 2026-08-23 pass covered light and dark `QMenu` presenters, Escape
dismissal, keyboard row traversal, menu-bar switching, checked actions,
separators, submenus, icons, shortcuts, and a deliberately long action label.
Menus remained fixed after presentation and the long label and shortcut were
not truncated. WinUI MenuFlyout opens below its anchor; unlike ComboBox it has
no selected-row alignment rule.

CheckBox uses WinUI's `AnimatedAcceptVisualSource`; the Qt implementation
reveals the two-segment accept path over the 167 ms fast duration. RadioButton
does not animate its checked-state opacity in Microsoft's template. It does
animate its checked dot from 12 px at rest to 14 px on pointer-over and 10 px
while pressed, both with the 250 ms normal duration.

Slider uses a 4 px rounded track with 14 px pre/post content margins and an
18 px Qt hit-layout thumb. The template paints a separate outer thumb surface
and animates only the inner accent ellipse: its resource base is 12 px with
scales 0.86 at rest, 1.167 on pointer-over and 0.71 while pressed. The active
track is anchored to the minimum end, so its direction follows orientation,
RTL and `invertedAppearance`; it must never be hard-coded left-to-right.
Live Gallery validation on 2026-08-22 confirmed the distinct outer surface,
small rest core, larger pointer-over core, active-track direction, drag motion,
settled value and value tooltip on the official range example.
The Qt style now owns the same press/drag value tooltip for plain `QSlider`;
its text follows the live integer value, the popup is positioned from the
thumb, and release or focus loss dismisses it.

ScrollBar reserves a 12 px extent. Its collapsed mouse thumb is 8 px thick and
offset 2 px toward the trailing edge; expansion animates it to 12 px over
167 ms after a 400 ms begin time. The expanded state reveals the Acrylic track
and 8 px arrow buttons. Contraction is delayed and returns the thumb to 8 px;
the Qt implementation retains the official 30 px minimum thumb length and
87.5% pressed-arrow scale.
Disabled ScrollBarThumb state opacity is zero in the Microsoft template, so a
disabled Qt scrollbar paints no thumb, track or arrows. Re-entry during the
500 ms contraction delay cancels contraction; re-entry after contraction has
started reverses immediately from the current thickness instead of collapsing
and revealing again.

TabView is not represented by an accent underline. Each item is at least
100×32 px (240 px maximum width), uses 12 px header text, a 16 px icon and a
32×24 px close button. The selected item uses the solid tertiary background,
semibold primary text, top/side card stroke, and an open bottom edge joined to
the content; unselected items use 1 px separators with 8 px vertical margins.
Pointer-over, pressed and selection brushes are discrete setters. The 200 ms
animation in the template belongs to recovery from reorder hints, not ordinary
tab selection.

ListView uses a 40 px minimum item height, `16,0,12,0` content padding, a
4 px item radius and a 3×16 px accent selection indicator with 1.5 px radius.
Selected and pointer-over items use the secondary subtle fill; selected hover
and pressed use the tertiary subtle fill. TreeView is denser: the presenter
uses a `4,2` margin, a 28 px minimum height, a 20 px content region and
`0,3,0,5` padding. Its disclosure affordance is a Fluent chevron and it does
not paint Qt's legacy branch lines. These template states are discrete.

Qt has no core widget equivalent to WinUI's community DataGrid. `QTableView`
is therefore a consistency extension, not a claimed direct mapping. It reuses
the item-view subtle state layers, 36 px control rhythm, secondary header text,
flat strokes and Fluent sort chevrons. Native model/delegate editing, keyboard
selection, column resizing and accessibility remain owned by Qt.

RichEditBox shares the common text-control background, border and focused
underline tokens with TextBox. Normal, pointer-over and focused surface changes
are discrete. ContentDialog uses a 320×184 minimum, 24 px content padding,
12 px title separation, 8 px between command buttons and an 8 px overlay
radius. Its entrance combines 0→1 opacity over 83 ms with a 1.05→1 scale over
250 ms. Qt top-level window framing remains platform-owned; the style owns the
dialog client surface, layout contract, primary command role and Fluent
message glyphs (Info E946, Warning E7BA, Error E783, Help E897).

ToggleSwitch was inspected with pointer positioning, click, and a real track
drag. The live control agreed with the resource template: 40×20 track, 12 px
rest knob, 14 px hover knob, and immediate checked-state commit at drag release.
The public Qt mapping is a native `QCheckBox` with `winuiToggleSwitch=true`.
The style's private event controller owns dragging and the 83 ms position
animation. `WinUI3::ToggleSwitch` is an optional public `QCheckBox` convenience
type for Designer and source compatibility; it adds no painting or animation.
Applications do not need their own ToggleSwitch subclass.

The navigation semantic variant likewise uses a plain Qt item view. A private
delegate installed by QStyle owns the selected background, hover/down surface,
keyboard focus visual, and animated accent pill. The `NavigationView` helper is
now only page/search composition and content transition behavior.

NumberBox must be audited as a complex control, not as a focused TextBox with
arrows painted on top. In the official inline placement, the spin-button column
is 72 px wide and is partitioned into two horizontal 36 px buttons. On mouse
focus, the 2 px accent underline stops at the editable-column boundary; the up
and down buttons keep their neutral lower edge and own hover/pressed surfaces.
Their enabled states are independent at the numeric bounds. The `QSpinBox`
mapping deliberately replaces Qt's usual vertically stacked button rectangles
with this WinUI inline partition.

The optional `winuiVerticalSpinButtons` variant is a consistency extension for
Qt applications that need the familiar stacked layout. It keeps the same
WinUI surface, glyphs, input states and editable-field focus underline, but
uses one 32 px trailing column split into two non-overlapping vertical hit
targets. It is not presented as an official NumberBox template placement.

Qt `QGroupBox` has no direct WinUI control equivalent. It is therefore treated
as a WinUI card: a translucent layer fill, subtle one-pixel stroke, six-pixel
corner radius, semibold header, and (when checkable) the same animated 20 px
CheckBox indicator. This is an explicit consistency extension, not presented as
an upstream WinUI template.

`QSplitter` and `QDockWidget` also have no direct WinUI equivalents. Their
resize affordance uses a six-pixel interactive target with a centered one-pixel
neutral stroke at rest, a two-pixel strong stroke on pointer-over, and a
three-pixel accent stroke while pressed. Dock frames use the subtle card stroke;
dock titles use the translucent layer fill, semibold body typography, and the
same 8 px content inset as other Fluent surfaces. These are consistency
extensions and are validated in both orientations through Qt's shared splitter
and dock-separator painting path.

These observations are checked against the exact Microsoft XAML resource files
listed in `manifest.json`. A Qt capture is rejected if it merely renders without
crashing; every state above must be inspected or covered by a state-level test.
