# Control coverage ledger

Status meanings are defined in `METHODOLOGY.md`. `Verified` requires live mouse,
keyboard, theme, and motion evidence. Automated tests alone can raise a row only
to `source-audited`; missing evidence remains explicit in the last column.

| Qt widget / variant | WinUI mapping | Mapping kind | Current status | Required state evidence |
|---|---|---|---|---|
| `QPushButton` | Button | Direct | source-audited | direct panel/label ownership, icon/text, menu reserve, rest/hover/down/disabled, mouse/keyboard focus and default/accent are under contract; live WinUI comparison remains |
| checkable `QPushButton` | ToggleButton | Direct | source-audited | direct off/on surface, hover/down, disabled-on and focus paths are captured; live comparison remains |
| `QToolButton` / toolbar | AppBarButton / CommandBar | Direct | source-audited | direct label painting, icon/text layouts, toggle, split-menu partition/hit test, dropdown glyph, hover/down and separator are covered; live overflow comparison remains |
| `QLineEdit` | TextBox | Direct | source-audited | discrete hover/focus, 10/5/6/6 padding, focused 1/1/1/2 border, Clear glyph, disabled palette, read-only input/clear suppression and selection covered by offscreen contracts; native light/dark mouse and keyboard comparison remains |
| `QTextEdit` / `QPlainTextEdit` | RichEditBox | Direct | source-audited | discrete rest/hover/focus surface, 2 px accent focus edge, text selection and scrolling tested; live disabled, read-only and dark keyboard pass remain |
| `QComboBox` | ComboBox | Direct | source-audited | closed states, popup, selected pill, row hover/down, dismissal |
| `QSpinBox` | NumberBox inline placement + optional stacked Qt variant | Direct + consistency extension | source-audited | horizontal 36 px and vertical 32 px button partitions, RTL, click and underline extent tested; dark, keyboard and bounds live pass remains |
| `QCheckBox` | CheckBox | Direct | source-audited | off/on/indeterminate, hover/down, animated accept, reversal, focus |
| `QCheckBox[winuiToggleSwitch=true]` | ToggleSwitch | Semantic variant | source-audited | offscreen rest/hover/click/drag contract; tested travel midpoint/reversal, disabled and focus; dark keyboard pass remains |
| `QRadioButton` | RadioButton | Direct | source-audited | off/on, hover/down dot sizes, group change, disabled, focus |
| `QSlider` | Slider | Direct | source-audited | official 4 px track, 18/22 px outer thumb, 10.32/14/8.52 px inner motion, value tooltip, LTR/RTL/inverted/vertical endpoints, drag, disabled, extreme-range ticks and mouse/keyboard focus covered by offscreen contracts; native light/dark comparison remains |
| `QProgressBar` | ProgressBar / ProgressRing substitute | Direct | source-audited | determinate/indeterminate, both axes, inversion, disabled, periodic repaint, deterministic freeze and timer cleanup are covered; live comparison remains |
| `QTabBar` / `QTabWidget` | TabView | Direct | source-audited | official 100×32 min item, 12 px type, 16 px icon, 32×24 close button, separators and selected attached surface; no legacy accent underline; live hover/down, overflow, drag/reorder and keyboard comparison remains |
| popup `QListView` | ComboBox item | Direct | source-audited | selected pill, hover/down, row height, scrolling |
| `QListView` / `QListWidget` | ListView | Direct | source-audited | official 40 px rows, padding, subtle hover/selected/down layers, accent selection pill, icons/checks, editing, disabled and keyboard focus; live pointer sequence remains |
| `QTreeView` / `QTreeWidget` | TreeView | Direct | source-audited | official 28 px rows, 4×2 margin, Fluent expanders, hierarchy, selected pill and keyboard focus; multi-column header is a consistency extension; live pointer sequence remains |
| `QTableView` / `QTableWidget` / headers | DataGrid-like extension | Consistency extension | source-audited | 36 px rows, subtle row selection, flat 32 px headers, sorting glyph, editing and scrolling tested; no core WinUI DataGrid is claimed; live pointer/resize pass remains |
| `QMenu` / `QMenuBar` | MenuFlyout / menu bar extension | Direct + extension | source-audited | check, icon, shortcut, submenu, long text, hover/down, Acrylic |
| `QScrollBar` | ScrollBar | Direct | source-audited | official 12 px extent, 8→12 px reveal, 400/500 ms delays, 167 ms reversal-safe expand/contract, arrows, 30 px thumb, RTL, disabled-zero-opacity and real `QScrollArea` interaction covered by offscreen contracts; native light/dark comparison remains |
| `QGroupBox` | Fluent card grouping | Consistency extension | source-audited | plain/checkable, hover/down check, disabled, focus |
| `QSplitter` | Fluent separator | Consistency extension | source-audited | both axes, hover, drag, clamp, release |
| `QDockWidget` | Fluent layer/card docking | Consistency extension | source-audited | dock/floating title, buttons, separator hover/drag, focus |
| opted-in `QDialog` / `QMessageBox` | ContentDialog | Compound mapping | source-audited | 320×184 minimum, 24 px content margin, 12 px spacing, accent default button, Fluent message glyphs and 83/250 ms show motion; live modality/dismissal/dark comparison remains |
| `QAbstractItemView[winuiNavigationView=true]` | NavigationView item | Semantic variant | source-audited | style-owned delegate, real click, selected-pill transition tested; dark/keyboard live pass remains |
| `QFrame[winuiSettingsCard=true]` / settings-card composition | Gallery settings card | Semantic variant + compound behavior | source-audited | style owns card chrome; helper owns composition/expansion, palette-aware icon and animation-disable behavior; dark live pass remains |

## P1 and structural closure ledger

The following items are required regression contracts, independently of the
per-control live-verification status above:

| Audit item | Implementation evidence | Regression evidence |
|---|---|---|
| Read-only TextBox actions | only Qt's private clear affordance is suppressed; action-backed buttons are untouched | `readOnlyActionRestoration` |
| Indeterminate progress repaint | style-owned 16 ms timer runs only for `minimum == maximum`, stops for determinate and is deleted by `unpolish()` | `progressAnimationAndOrientations` |
| Non-left slider input | style press state, tooltip and absolute-set policy are left-button-only | `rtlGeometryAndHitTesting`, `sliderDragInteraction` |
| Extreme slider ranges | tick arithmetic is `qint64`, capped to at most about 100 intervals and has an overflow-safe terminal condition | `sliderExtremeRangeTicks` |
| AnimatedStack effects/lifecycle | application effects are never replaced; snapshot/effect cleanup survives reversal, removal and hide/reopen | `animatedStackEffectsAndInterruption` |
| Polish symmetry | palettes, autofill, hover/opaque attributes, margins, spacing, list spacing, navigation delegate/mouse tracking, timers and role properties are restored | `styleMutationRestoration` |
| Accent role separation | selection uses `SystemAccentColor`; control AccentFill uses the theme-specific ramp role; text-on-accent has separate theme roles | `palettes`, `runtimeAppearanceAndDialogLifecycle` |
| Palette-owned glyphs | Fluent glyphs are explicit foreground masks; arbitrary application icons retain their colors | `buttonToolButtonAndIconContracts` |
| Covered-style fallbacks | debug assertions reject covered primitives, controls and complex controls that reach `QCommonStyle`; covered content sizing is explicit | all render/state tests plus deterministic snapshot matrix |
| RTL and hit testing | ComboBox, menus, GroupBox, ToolButton, Slider, tabs and headers use visual geometry and direct hit testing | `rtlGeometryAndHitTesting`, `buttonToolButtonAndIconContracts` |
| Check/radio reverse motion | fill, check path and radio dot all consume the animated progress in both directions | `checkboxAndRadioUncheckMotion` |
| Navigation model lifecycle | delegate reconnects to replacement model and selection model and resynchronizes on scroll | `navigationModelReconnectAndScroll` |
| Dialog lifecycle | opacity-only show motion avoids layout geometry changes; hide/reopen clears animation state | `runtimeAppearanceAndDialogLifecycle`, `contentDialogContract` |
| Runtime theme/accent | system scheme/accent watcher refreshes application palette, owned palettes, open windows, backdrops and popups | `runtimeAppearanceAndDialogLifecycle`, native `dialogThemeUpdate`, `comboPopupContract` |
| Popup first frame | ComboBox selection/scroll are prepared before presentation; MenuFlyout insets are installed before its first size negotiation | `comboPopupContract`, `menuSizingContract`, native `comboPopupSurface` |
| Backdrop teardown | Mica/Acrylic preserve explicit or inherited palettes plus all QWidget background attributes; a direct `None` request is idempotent | native `backdropStateRestoration` |

Rows marked `partial` or `not covered` are explicitly not claims of complete
WinUI fidelity. The audit must either close their missing states or narrow the
project's public coverage claims.
