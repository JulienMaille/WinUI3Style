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
The pinned `TextBox_themeresources_perf2026.xaml` places that button in a trailing
30 px column and applies `TextBoxInnerButtonMargin` `0,4,4,4` to its painted grid;
the 12 px glyph is centered in that grid. Compact keeps its width and margins,
so the surface is 26×24 at a 32 px editor height and 26×16 at 24 px, not a square.
Qt retains its private 22×18 hit rectangle and action spacing; the style aligns
both surface and glyph in editor coordinates, mirrored in RTL and following the
clear button when additional trailing actions move it inward.
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
flat strokes and the DataGrid column-header sort glyph. The pinned
CommunityToolkit.WinUI `DataGrid.xaml` (commit `080bdb9`) gives the header
`Padding="12,0,0,0"`, `FontSize="12"`, `MinHeight="32"`, and a trailing sort
column of `MinWidth="32"` whose 12 px `SortIcon` uses `SortIconAscending`
U+E74A and `SortIconDescending` U+E74B; the column exists for every header
(the glyph is opacity 0 while unsorted), so the label never reflows when a sort
starts. Header content is a `ContentPresenter` with
`HorizontalAlignment="Left"` inside the template's `Padding="12,0,0,0"`, so
column titles are leading-aligned, not centred; Qt's `AlignCenter` default is
honoured only when the model publishes a `TextAlignmentRole` for the section.
Cell text uses the pinned `DataGridTextColumn` margins (12 px on both sides,
`DATAGRIDTEXTCOLUMN_leftMargin`/`_rightMargin` on the generated `TextBlock`),
the same 12 px as the header's leading padding, so a cell and its header title
start on one edge. Native model/delegate editing, keyboard selection, column
resizing and accessibility remain owned by Qt.

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
focus, the 2 px accent underline spans the complete official control width,
including the button column; the up and down buttons keep their neutral surface
and own hover/pressed states. Their enabled states are independent at the
numeric bounds. The `QSpinBox` mapping deliberately replaces Qt's usual
vertically stacked button rectangles with this WinUI inline partition.

The optional `winuiVerticalSpinButtons` variant is a consistency extension for
Qt applications that need the familiar stacked layout. It keeps the same
WinUI surface, glyphs and input states, uses one 32 px trailing column split
into two non-overlapping vertical hit targets, and adds one neutral vertical
separator at the editable-field boundary. Its focus underline still spans the
complete control width. The separator is a stroke only, not a cell or bevel;
the variant is not presented as an official NumberBox template placement.

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

Live text-AA measurement 2026-09-10 (Accent button, dark, 100%): the official
Gallery composites neutral grayscale coverage (fringes #8685a5/#4e4d60 around a
#1a1a20 core); Qt's default LCD subpixel left R/B fringes (#bd855d/#4c85e8
around #000000). The style therefore paints labels with
`QFont::NoSubpixelAntialias` (`paintGrayscaleText`), trading a slightly softer
LCD rendering for WinUI-neutral coverage on all outputs.

ToolTip is pinned to `ToolTip_themeresources.xaml` (`DefaultToolTipStyle`)
and not to the older system-XAML style in `dxaml/xcp/dxaml/themes/generic.xaml`.
The template is a single `ContentPresenter` with `TextWrapping="Wrap"`,
`CornerRadius={ThemeResource ControlCornerRadius}` (4), `BorderThickness` 1,
`Padding` = `ToolTipBorderPadding` `9,6,9,8`, `MaxWidth` = `ToolTipMaxWidth`
320, `FontSize` = `ToolTipContentThemeFontSize` 12, and `OpenStates` that fade
the root in and out. Its brushes are `AcrylicInAppFillColorDefaultBrush` with
`BackgroundSizing=InnerBorderEdge` and `SurfaceStrokeColorFlyoutBrush`. At the
manifest pin the acrylic falls back to `#2C2C2C` (Dark) and `#F9F9F9` (Light),
and the flyout stroke is black at 20% (`#33000000`, Dark) or 6% (`#0F000000`,
Light) - the same `flyoutStroke` token the menus use. A ToolTip therefore has a
*faint darkening* border in both themes; the earlier white-at-20% Dark stroke
lightened the edge where WinUI darkens it.

Qt owns the tooltip window (`QTipLabel`, a `QLabel` with
`Qt::ToolTip | BypassGraphicsProxyWidget`), so the style maps the template onto
it instead of restyling the control:

- Surface: `PE_PanelTipLabel` paints the card over the whole window rect with
  the stroke on the edge; the window itself takes DWM's
  `DWMWCP_ROUNDSMALL` corner (read back after the request, `ROUND` and then a
  window region are the fallbacks). A window region is a hard staircase and
  Windows composites the drop shadow around it, which was the dark rim the
  surface showed; the native corner also restores the composited shadow.
- Layout: `QTipLabel` builds a symmetric `1 + PM_ToolTipLabelFrameWidth` margin
  plus a 1 px leading indent, keeps a legacy `QLabel::margin()` that adds
  2 x margin to the size hint, and has no maximum width. On Show and on every
  resize the style resets the indent and the legacy margin, applies
  `contentsMargins` = `10,7,10,9` (1 px border + the template padding), caps
  the width at 320 with word wrap, and keeps the re-laid surface inside the
  screen's available geometry. Qt only re-sizes a reused tip (no second Show),
  so the Resize path carries the same layout.
- Typography: `QToolTip::setFont` is set from the application font at 12 px,
  owned and restored exactly like the tooltip palette.
- Background material: the tooltip window is opaque and paints the acrylic
  *fallback* colour. Qt cannot host an in-app acrylic backdrop on a tooltip
  window, and the style does not claim one; this is the template's own
  `FallbackColor`.

The Qt and WinUI lifetimes differ and the style does not pretend otherwise.
Qt opens the tip from a hover timer, hides it on Leave through a 300 ms grace
timer, on any application mouse press/release/wheel, and on the display
timeout (10 s + 40 ms per character past 100, or the explicit
`msecDisplayTime`); it reuses one label - and one HWND - while the pointer
stays inside the widget. Qt 6.12 has no Escape handler for tooltips (its
`QTipLabel` event filter handles keys only on macOS), so no Escape dismissal
is claimed. Live validation lives in
`WinUI3StyleNativeTest::tooltipLifecycleAndReuse`, which drives `QToolTip`
directly because a test session has no foreground desktop to deliver a real
hover: a longer text reuses the same HWND with `regionType 0` and the DWM
corner still granted, the Leave that ends a hover dismisses through the 300 ms
grace timer, a click dismisses immediately, an explicit display time expires,
and a tip placed at the screen corner stays inside the available geometry. The
hover timer that raises the tip is Qt's and is part of the live pass.
The composited DWM shadow is judged on a live desktop (`BitBlt`/`CopyFromScreen`
return black in the capture environment); the live pass over the Gallery's
theme combo, density combo, float dock button and auto-suggest field accepted
the shadow, border weight/colour and 12 px layout on 2026-11-03.

These observations are checked against the exact Microsoft XAML resource files
listed in `manifest.json`. A Qt capture is rejected if it merely renders without
crashing; every state above must be inspected or covered by a state-level test.

## Supplementary popup comparison — 2026-11-12 (not the 2.4 pin)

The installed official Gallery has updated to `Microsoft.WinUI3ControlsGallery`
3.0.4.0. It is self-contained, with `Microsoft.WindowsAppSDK`
2.5.4-experimental in its dependency manifest; the Home page reports Windows
App SDK 2.5 (experimental). This comparison does not replace the 2.4 reference
pin above.

Focused screen captures compared the official ComboBox, MenuFlyout,
DropDownButton MenuFlyout and SplitButton Flyout in Light and Dark against the
actual Qt Gallery ComboBox, menu button and split tool button, extracted into
a popup-only host. The active Qt style was confirmed as `WinUI3::Style`,
Qt 6.9.2, Standard density, `QT_SCALE_FACTOR=1`, Qt DPR 2 and HWND DPI 192 on
a 200% display. The earlier Standard/Compact DPI-mismatch regression remains
a separate test at Qt DPR 4.21875.

Rest, hover, held press, Escape and same-selection reopening were exercised
with native input. No untinted gray corner wedge was visible in the repaired
Qt panels. The official ComboBox and MenuFlyout native window rectangles
stayed fixed across sampled opening and repeated-open frames. The official
SplitButton sample instead hosts a Flyout inside the main window; its palette
content is not a MenuFlyout row-layout reference.

Original-resolution corner crops show an approximately 16-physical-pixel
radius (8 logical pixels) in both implementations, not identical antialiasing.
Straight top/bottom strokes occupy two physical rows. Stroke contrast differs
with material and backdrop: Qt menu/tool sides retain their stroke, but the
ComboBox's straight side stroke is not visible in these captures. The patch
outside the ComboBox's top-left curve is exposed backdrop, not an untinted
wedge inside the panel. These differences preclude a pixel-parity claim.

Opening motion is not pixel-equivalent: Qt moves the entire popup HWND by a
12-logical-pixel sweep and fades it, while official WinUI reveals content
inside a fixed popup HWND. Early captures were sampled after Show/input, not
synchronized to the first compositor-presented frame, and their nominal delays
do not establish exact animation durations. Official captures include shadow
margins; the Qt captures stop at the popup HWND rectangle, so outer-shadow
parity is not established. Full Verified status and pinned-2.4 parity are not
claimed by this supplementary pass.

Commit review also checks all four stale-buffer corners in Light/Dark and
Standard/Compact, and confirms the fallback radius query creates no native
handle. The 12-case native DPI-mismatch regression passes. The legacy native
menu snapshot suite rejects captures at Qt DPR 2 because it compares physical
pixmap sizes to logical widget sizes; it passes with `QT_SCALE_FACTOR=0.5`
(Qt DPR 1 on this 200% display). That existing test limitation is not repaired
or hidden by this corner change.

### Popup border audit (2026-11-14; pinned source, Linux runtime)

The issue #59 screenshot shows the official reference, not the faulty Qt window.
The ordinary button tooltip already uses the correct `ToolTipBorderBrush` →
`SurfaceStrokeColorFlyoutBrush`: Light `#0F000000`, Dark `#33000000`.
Do not lighten that token or change Windows DWM border handling without native
reproduction. The user chose to keep that original report blocked.

A broader user-requested audit found these distinct pinned brush mappings:

- Slider's `SetDefaultThumbToolTip` implementation in `dxaml/xcp/dxaml/lib/Slider_Partial.cpp`
  creates a standard `ToolTip` (`ctl::make<ToolTip>`); this is not declared in
  `Slider_themeresources.xaml`. Its value popup therefore shares
  `PE_PanelTipLabel` with button tooltips, rather than using the stronger editor
  `strokeSecondary` and an independent translucent fill recipe. Alpha-backed
  value-tip windows preserve their transparent exterior corners.
- `MenuFlyoutSeparatorBackground` resolves to `DividerStrokeColorDefaultBrush`:
  Light `#0F000000`, Dark `#15FFFFFF`. This is not Dark `ControlStrokeColorDefault`.
- `ContentDialogSeparatorBorderBrush` resolves to `CardStrokeColorDefaultBrush`:
  Light `#0F000000`, Dark `#19000000`. The QDialog/QMessageBox and QWizard command
  footer extensions share this semantic token; do not replace it with a white
  control stroke or a flyout outer border.
- CalendarDatePicker's outer presenter has zero border; its contained CalendarView
  owns one `CalendarViewBorderBrush` → `ControlStrokeColorDefaultBrush` boundary
  (Light black alpha 15, Dark white alpha 18). Qt's popup composition must
  provide one equivalent boundary, not zero or two. The presenter retains the
  popup's 8 px clip while CalendarView itself uses its pinned 4 px radius.

Menu outer borders, ComboBox dropdown borders, and AutoSuggest popup borders
already use the correct flyout token. Leave those mappings unchanged.
ContentDialog's native outer frame is separate (`SurfaceStrokeColorDefaultBrush`,
`#66757575`) and was not compositor-verified on Linux.

All source statements above refer to manifest commit
`9f89c2da5a5502c263d9268fee224697c47ecb6e`:
`CommonStyles/{ToolTip,Slider,MenuFlyout,ContentDialog,CalendarView,CalendarDatePicker}_themeresources.xaml`
and `CommonStyles/Common_themeresources_any.xaml`.
New Gallery regression probes cover Light/Dark and Standard/Compact; these are
Qt runtime checks, not an official Windows Gallery side-by-side verification.

### Calendar month/year caption contrast follow-up (2026-11-14)

The pinned `CalendarViewHeaderNavigationButtonForeground` and PointerOver resources
map to `TextFillColorPrimaryBrush` in Light and Dark. Pressed maps to
`TextFillColorSecondaryBrush`; Disabled maps to `TextFillColorDisabledBrush`.
See [the pinned CalendarView header resources](https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/CalendarView/CalendarView_themeresources.xaml).

Qt 6.8.3 `QCalToolButton::paintEvent` copies `HighlightedText` into `ButtonText`
at rest. That legacy accent-header rule paints nearly white month/year text on
our neutral Light calendar surface. The existing QStyle label painter now uses
the pinned neutral text tokens only for the real calendar's month/year buttons.
Ordinary explicit ButtonText overrides and previous/next buttons do not change.

Authentic Gallery captures reproduce this defect before the fix. The regression
fails all four Light Standard/Compact × inline/popup rows before the fix and
passes all eight Light/Dark rows after it. Each row checks both captions in rest,
hover, pressed, and disabled states, with exact token and rendered-ink checks.
Native Windows compositor and official Gallery comparisons remain unverified.
