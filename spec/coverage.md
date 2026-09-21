# Control coverage ledger

Status meanings are defined in `METHODOLOGY.md`. `Verified` requires live mouse,
keyboard, theme, and motion evidence. Automated tests alone can raise a row only
to `source-audited`; missing evidence remains explicit in the last column.

| Qt widget / variant | WinUI mapping | Mapping kind | Current status | Required state evidence |
|---|---|---|---|---|
| `QPushButton` | Button | Direct | source-audited | direct panel/label ownership, icon/text, menu reserve, rest/hover/down/disabled, mouse/keyboard focus and default/accent are under contract; label text paints grayscale AA (`paintGrayscaleText`, `QFont::NoSubpixelAntialias`) matching WinUI neutral coverage — measured live 2026-09-10 Accent dark (ours ClearType fringes #bd855d/#4c85e8 vs official #8685a5/#4e4d60 neutrals); guard `buttonTextAntialiasesGrayscale`; live WinUI comparison remains |
| `QCommandLinkButton` | Command-link composition | Consistency extension | source-audited | 160×64 minimum, title/description ownership, enabled/disabled surface and native activation are covered; live pointer/focus comparison remains |
| checkable `QPushButton` | ToggleButton | Direct | source-audited | direct off/on surface, hover/down, disabled-on and focus paths are captured; live comparison remains |
| `QToolButton` / toolbar | AppBarButton / CommandBar | Direct | source-audited | direct label painting, icon/text layouts, toggle, split-menu partition/hit test, dropdown glyph, hover/down and separator are covered; live overflow comparison remains |
| `QLineEdit` | TextBox | Direct | source-audited | discrete hover/focus, 10/5/6/6 padding, focused 1/1/1/2 border, Clear glyph, disabled palette, read-only input/clear suppression and selection covered by offscreen contracts; native light/dark mouse and keyboard comparison remains |
| `QTextEdit` / `QPlainTextEdit` | RichEditBox | Direct | source-audited | discrete rest/hover/focus surface, 2 px accent focus edge, text selection and scrolling tested; live disabled, read-only and dark keyboard pass remain |
| `QComboBox` | ComboBox | Direct | source-audited | closed states, popup, selected pill, row hover/down, dismissal; open-popup keyboard-current paints the hover pill row-for-row via `popupRowIsKeyboardCurrent` (verified `comboOpenPopupKeyboardNavTracksHover`); live pointer/keyboard comparison remains |
| `QSpinBox` | NumberBox inline placement + optional stacked Qt variant | Direct + consistency extension | source-audited | horizontal 36 px and vertical 32 px button partitions, compact 24 px editor height, RTL, click and underline extent tested; dark, keyboard and bounds live pass remains |
| `QCheckBox` | CheckBox | Direct | source-audited | off/on/indeterminate, hover/down, animated accept, reversal, focus |
| `QCheckBox[winuiToggleSwitch=true]` | ToggleSwitch | Semantic variant | source-audited | offscreen rest/hover/click/drag contract; tested travel midpoint/reversal, disabled and focus; dark keyboard pass remains |
| `QRadioButton` | RadioButton | Direct | source-audited | off/on, hover/down dot sizes, group change, disabled, focus; checked-state switch is discrete (duration 0, `radioRapidClickResponsiveness`) — only dot hover/press sizes animate 250 ms Normal; rapid toggles track state like checkbox uncheck |
| `QSlider` | Slider | Direct | source-audited | official 4 px track, 18/22 px outer thumb, 10.32/14/8.52 px inner motion, value tooltip, LTR/RTL/inverted/vertical endpoints, drag, disabled, extreme-range ticks and mouse/keyboard focus covered by offscreen contracts; inside opaque group cards the backdrop erase is skipped so the card fill survives on mica (`backdropSliderInsideOpaqueCardKeepsCardFill`, `backdropSliderRepaintDoesNotAccumulate`); native light/dark comparison remains |
| `QProgressBar` | ProgressBar / ProgressRing substitute | Direct | source-audited | determinate/indeterminate, both axes, inversion, disabled, periodic repaint, deterministic freeze and timer cleanup are covered; live comparison remains |
| `QTabBar` / `QTabWidget` | TabView | Direct | source-audited | official 100×32 min item, 12 px type, 16 px icon, 32×24 close button, separators and selected attached surface; no legacy accent underline; live hover/down, overflow, drag/reorder and keyboard comparison remains |
| popup `QListView` | ComboBox item | Direct | source-audited | selected pill, hover/down, row height, scrolling; open-popup arrows move the view current index (selection follows) and repaint the hover pill on the new current row while the value row keeps only its accent marker (`comboOpenPopupKeyboardNavMovesHoverPill`) |
| `QLineEdit` + `QCompleter` (PopupCompletion) | AutoSuggestBox | Semantic variant | partial — row-frame evidence reopened 2026-09-19 | stock editor + completer keep semantics, signals, focus, and hit testing; QStyle owns the popup surface and every row visual. Shared `paintPopupRowPill`/`popupRowPillRect` geometry remains 4,2 insets and 3px radius. CE/PE now preserve the resolved Base RGB with opaque row frames: `autoSuggestResolvedFramePreservesRgb` covers Light/Dark × Standard/Compact × rest/hover/leave; the existing composited-hover assertion is unchanged and passes offscreen. See RED/GREEN and PrintWindow evidence below. The completer view is frameless; the complete live pointer/keyboard pass remains incomplete. |
| `QListView` / `QListWidget` | ListView | Direct | source-audited | official 40 px rows, padding, subtle hover/selected/down layers, accent selection pill, icons/checks, editing, disabled and keyboard focus; live pointer sequence remains |
| `QTreeView` / `QTreeWidget` | TreeView | Direct | source-audited | official 28 px rows, 4×2 margin, Fluent expanders, hierarchy, selected pill and keyboard focus; multi-column header is a consistency extension; live pointer sequence remains |
| `QTableView` / `QTableWidget` / headers | DataGrid-like extension | Consistency extension | source-audited | 36 px rows, subtle row selection, flat 32 px headers, sorting glyph, editing and scrolling tested; no core WinUI DataGrid is claimed; live pointer/resize pass remains |
| `QMenu` / `QMenuBar` | MenuFlyout / menu bar extension | Direct + extension | source-audited | check, icon, shortcut, submenu and long-text contracts exist. Toggle-twice reopen keeps edge pixels, geometry, margins and mask (`menuToggleTwiceKeepsEdgeAndRegion`); the composited-recipe clear is pinned by `menuToggleCycleClearsCompositedRecipe`. Same-HWND reopen keeps the sync tint and re-asserts the grant deferred past Show dispatch; fresh-submenu deferred re-attempt past Show dispatch, refused-again stays opaque; refused → opaque shadow-only fallback (1px frame keeps standard DWM shadow without TransientWindow re-grant) (`menuBarChildToggleReopenConverges`, `menuSubmenuGrantConverges`). Density: `Compact.xaml` has no MenuFlyoutItem entry (presenter 32 only) — Microsoft ships MenuBar-only Compact. Deliberate extension: Compact menu-popup rows shrink 36 → 32 like list rows (`menuDensityPreservesPopupGeometry`). Opening motion is a deliberate deviation accepted live 2026-09-09: vertical 12 px slide with OutCubic, including lateral submenus — not WinUI anchor-side motion, judged good enough (#6 closed as deviation). Acrylic requires a live compositor (opaque fallback offscreen); the prior AnimateWindow measurement claim lacks a reproducible evidence record. Corner convergence: paint r8 + mask r8 at the 50% threshold + concentric stroke (lineage `menuBarChildToggleReopenConverges`; gates `popupCorners*`). Double-open visual parity (open1 vs open2 identical settled frames, standalone + InstantPopup tool-button) is pinned by `menuDoubleOpenKeepsVisualParity`. |
| `QCalendarWidget` inline + popup | CalendarView + popup extension | Consistency extension | partial — reopened 2026-09-17 (inline defect); 2026-09-18 Mica regression RED captured (calendar-mica-red4-native.txt: granted-Mica inline body #ff212121 baked opaque vs unflattened composited ink #ca212121; offscreen 14 passed/8 skipped, fallback rows green) | Actual reported control: demo persistentCalendar in dialogPartsGroup, Mica off, not QDateEdit popup. Pinned [CalendarView template](https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/CommonStyles/CalendarView_themeresources.xaml) places header and days on ONE Background border: CalendarViewBackground → ControlFillColorInputActiveBrush. Same-commit Common_themeresources_any.xaml defines Dark #B31E1E1E / Light #FFFFFFFF, exactly existing Tokens::editorFocusedFill. For the plain opaque Gallery host, resolve that brush once over the existing group card (Tokens::layer over Tokens::surface); the resulting body/header pixels are opaque. This is NOT popupSurfaceColor, not alpha-forcing the raw brush, and not a new hardcoded colour. Idle navigation background AND border are SubtleFillColorTransparent; pointer-over/pressed use Subtle Secondary/Tertiary, focus retains FocusStroke. No geometry change. Old inline Standard-button policy is superseded; conflicting tests remain unchanged pending parent acceptance: density_widgets lines 1005, 1013–1015 (Base=layer, header=Window), 1112–1119 (Standard + Button=Window). Popup effective acrylic/fallback contract and previous popup evidence remain separate. Initial draft RED logs had an invalid header bounding-box-center probe and incomplete host hierarchy: not acceptance evidence. Corrected fixture RED native + offscreen with Windows fonts: 8 rows each (Light/Dark × Standard/Compact × body/buttons), 0 skipped. Actual body #808080/#111111 vs resolved InputActive #FFFFFF/#212121; valid empty-header pixels #F3F3F3/#202020; previous-button top-center stroke #E5E5E5/#303030. Group card independently matches layer-over-window oracle (#F9F9F9/#272727). Header membership, glyph clearance and geometry asserted; no equality-only transparent acceptance. Source audit: calendar CE_ItemViewItem Source-replaces rows with unresolved translucent Base (#80FFFFFF/#4C3A3A3A), while header uses opaque Window and Standard buttons retain stroke. Logs calendarheader-inline-{native,offscreen}-final-red.txt; earlier draft/valid-red logs are probe-development failures, NOT acceptance evidence. Live screenshot calendar-live-after-foreground.png has the same dark body #111111 versus empty header #202020. 2026-09-18 Mica fix RED→GREEN: inlineCalendarSurface restored to transparent seed under Composited (an intermediate edit had reverted it to unconditional opaque); backdrop toggle now re-runs owned palettes via refreshOwnedPalettes on effective-surface change (previously only theme switches did, so Mica toggles stuck opaque); Composited publish moved after island sync (publishing first refreshed against stale opaque islands via paintsDirectlyOnBackdrop); refresh re-syncs chrome/islands after recompute (generic rebase otherwise re-opaques them); inline nav-bar branch Composited-aware and empty stretch Source-fills (was double-blending veiled-over-veiled toward opaque). Mica RED logs calendar-mica-red4-native.txt (dark body #ff212121 vs veiled #ca212121) now GREEN: calendarheader 22/22 native + unit, density 21/21 native + unit (includes real-grant lane contract replacing the faked-effective poison rows, and grant-aware popup/auto-suggest asserts). Gallery PID 23260 DLL 4D20E78B (matches build), Mica ON with live DWM grant (attr38=2) confirmed; live inline-calendar Mica pixels pending (Dialogs-page navigation unresponsive to automation: nav reports selected while content stays on Controls). Pre-existing native topLevels 6→5 failure unchanged, triaged separately, not weakened. |
| `QScrollBar` | ScrollBar | Direct | source-audited | official 12 px extent, 8→12 px reveal, 400/500 ms delays, 167 ms reversal-safe expand/contract, arrows, 30 px thumb, RTL, disabled-zero-opacity and real `QScrollArea` interaction covered by offscreen contracts; native light/dark comparison remains |
| `QGroupBox` | Fluent card grouping | Consistency extension | source-audited | plain/checkable, hover/down check, disabled, focus |
| `QSplitter` | Fluent separator | Consistency extension | source-audited | both axes, hover, drag, clamp, release |
| `QDockWidget` | Fluent layer/card docking | Consistency extension | source-audited | dock/floating title, buttons, separator hover/drag, focus |
| `QStatusBar` / `QSizeGrip` | Window footer / resize affordance | Consistency extension | source-audited | layer surface, top separator, automatic grip discovery, 16 px metric and all-corner glyph geometry are covered; live resize and high-DPI comparison remain |
| opted-in `QDialog` / `QMessageBox` | ContentDialog | Compound mapping | source-audited | distinct content and command/footer surfaces, 320×184 minimum, 24 px content margin, 12 px spacing, accent default button, Fluent message glyphs and 83/250 ms show motion; live modality/dismissal/dark comparison remains |
| `QWizard` / `QWizardPage` | Multi-step dialog composition | Consistency extension | source-audited | separate content/footer surfaces and automatic accent Next/Finish roles are covered; native navigation, cancellation, RTL and live WinUI comparison remain |
| `QAbstractItemView[winuiNavigationView=true]` | NavigationView item | Semantic variant | partial — palette lifecycle reopened 2026-09-20 | style-owned delegate, real click, selected-pill transition tested; palette ownership and Mica/theme restoration evidence below; complete dark/keyboard live pass remains |
| `QFrame[winuiSettingsCard=true]` / settings-card composition | Gallery settings card | Semantic variant + compound behavior | source-audited | interactive iff an expandable child is bound (`isCardInteractive`); trailing-only cards keep resting fill and ignore header clicks; expandable chevron is ChevronDown collapsed and expanded; expanded host shares the card surface with no fill/border; dark live pass remains |
| `Top-level QWidget[winuiBackdrop=mica]` (+ `micaalt`, `acrylic`) | Window SystemBackdrop (Mica / Mica Alt / Desktop Acrylic via `DWMWA_SYSTEMBACKDROP_TYPE`) | Direct | partial — caption lifecycle reopened 2026-09-19 | `applyBackdrop` maps to DWM `DWMSBT_MAINWINDOW`/`TABBEDWINDOW`/`TRANSIENTWINDOW` + full-frame extend, opaque `Painted` fallback offscreen/refused; single erase policy in `winui3helpers_p.h` (shared gate + recipe 1 erase-then-fill default, recipe 2 erase-means-transparent, recipe 3 `clearForBackdropFill` for acrylic popup pills) + island scroll guard + chrome/content transparentize/restore covered by `backdropLifecycleContract`, `backdrop*DoesNotAccumulate`, `islandScrollPostsFullViewportRepaint`, `materialErase*`, theme/chrome/resize/expose/move contracts; caption evidence below; broader live compositor verification remains. |

## P1 and structural closure ledger

### Reopened evidence — NavigationView palette lifecycle (2026-09-20)

Mapping, tokens and geometry are unchanged. Live standalone Gallery reproduced
System/Light → Mica on → Dark → Mica off leaving a light navigation background
and dark text. `navigationBackdropThemeRestore` reproduces the stale Text role
(#E4000000 instead of #FFFFFFFF), before the navigation fix, in
`build/navigation-theme-red.txt`. The two existing mechanism assertions also
fail unchanged in `build/navigation-fix-red.txt`: inherited palette ownership
is lost, and the top-level material alpha is overwritten on enable.

The owned-palette refresh now delegates temporary navigation palettes to their
existing state owner, preserving saved inheritance and rebasing style-owned
saved colors. The main-window refresh preserves applyBackdrop's alpha while
refreshing theme RGB. No existing assertion was removed or changed.
Release configuration/build pass. All 23 current split domain executables pass,
including NavigationView 13/13 (`build/navigation-domain-logs`); the leftover
pre-split `winui3style_tests.exe` is not a current CMake target and its stale
results are not acceptance evidence. Live rebuilt Gallery replay of the same
Light/Mica/Dark/disable sequence restores dark navigation with readable white
text; library and deployed demo DLL SHA256 both start `EA3F54B3BFC04DC1`.
The first full CTest run passed 51/52 (including strict snapshot matrix and
source/Designer contracts). The native rerun identified `comboPopupSurface`
first-paint alignment as its remaining failure; isolated runs with both the
preserved pre-fix DLL and rebuilt DLL then passed 3/3. This does not establish
the cause of that intermittent failure. No Verified promotion is made.
Final complete CTest rerun: **52/52 passed**, including the standard native
suite (84.63 s), in `build/navigation-final-ctest-rerun.txt`. The opt-in
AutoSuggest capture test still requires its explicit capture environment;
its default CTest result is not additional live evidence.

### Reopened evidence — AutoSuggestBox resolved row frame (2026-09-19)

Mapping and tokens are unchanged. `autoSuggestResolvedFramePreservesRgb` adds
12 renderer cases: Light/Dark × Standard/Compact × rest/hover/leave. Real popup
construction establishes the completer association and inherited density; direct
painting then supplies the already-resolved flyout RGB with the composited tint
alpha. This is a renderer contract, not proof of a native material grant.
The exact row-edge oracle rejects both opacity loss and a second color lift.
RED: `build/autosuggest-resolved-rgb-red.txt`, 12 failures, zero skips. Rest/leave
produce Light #FFFFFF instead of #FCFCFC and Dark #383838 instead of #2C2C2C;
hover produces alpha 242/178 instead of 255. Existing assertions are unchanged.
Earlier draft logs (`autosuggest-rgb-red.txt`, `autosuggest-rgb-lifecycle-red.txt`)
include incomplete association/density fixtures and are not acceptance evidence.

Live Gallery inspection at commit facba49 with the pinned official Gallery 2.9.3
did not reproduce the historical gross smear during the inspected hover sweeps.
The separate native run of `autoSuggestCompositedHoverRebuildsRowFrame` stops at
its opaque initial-Base assumption (actual alpha 242); its offscreen simulation
still failed at hovered-frame alpha before the fix. Desktop access subsequently
resumed: the Dark Gallery popup was visibly lighter than the official popup.

2026-09-20: both AutoSuggest paint paths now use the existing `withAlpha(Base,
255)` helper; no second `popupSurfaceColor` lift, token change, geometry change,
or calendar/ComboBox/menu policy change. Full ComboBox domain GREEN:
`build/autosuggest-combobox-green.txt`, 25 passed, zero failures/skips. This
includes all 12 new cases and the unchanged pre-existing assertions.

The opt-in `winui3style_autosuggest_native_tests` instantiates the real Gallery
and uses shared `nativeWindowFrame` (PrintWindow). Run with
`QT_QPA_PLATFORM=windows`, `WINUI3STYLE_DISABLE_ANIMATIONS=1`, and
`WINUI3STYLE_AUTOSUGGEST_CAPTURE_DIR=<output-directory>`. It captures the main
window and popup rest/hover/leave in Light/Dark, records DPR/palette/effective
material, and asserts delivered hover ink. Qt's QWidget mouseMove overload only
warps the cursor, so the fixture also routes a QWindow move; cursor coordinates
alone were insufficient evidence. The native resize border is included in the
frame dimensions. Early capture-fixture drafts are not acceptance evidence.

Final before/after evidence: `build/autosuggest-native-{before,after}-qpa`, logs
with the same prefix, 4 passed/zero failures/skips each. Before uses preserved
DLL SHA256 `1C0B683FE46CC8EB8F8E0CC268AF8959E48DFFDC33F9739F4EEA3592111D1C0F`;
after uses `6EB62A1C7BAD5B535F4FD91658D0AA235682898C7D486290987A539E6B692F4D`.
DPR is 1; popup effective material is Composited (2). The main-window material
is off in this fixture. Both main-window captures outside the popup scope are
pixel-identical (0 differing pixels, RMS 0). A preliminary Dark caption drift
was confined to the native title bar and disappeared after fixture settling;
the final comparison does not exclude that region. No Verified claim follows
from these captures; the standalone replay is recorded below.

The final native RGB assertions also reproduce the defect with the preserved
old DLL: `build/autosuggest-native-before-rgb.txt` has two failures (Light edge
#FAFAFA vs #FCFCFC; Dark #383838 vs #2C2C2C). The same fixture with the repaired
DLL passes 4/4 without skips (`build/autosuggest-native-after-rgb.txt`). Captures
are in the corresponding directories without `.txt`; both main-window frames
again compare with 0 differing pixels. Release configuration/build passes.
Full CTest: 51/52 passing (`build/autosuggest-final-ctest.txt`), with only the
pre-existing NavigationView failures remaining (`navigationDelegateLifecycle`
and `navigationBackdropDisableRepaintsOpaqueAfterRestore`). All 23 offscreen
domain executables were rerun: 22 pass, NavigationView retains those two failures
(`build/autosuggest-final-domain-logs`). Source/Designer contracts and the full
snapshot matrix pass; the standard native suite passes. The opt-in capture
suite was run explicitly above, not inferred from its default skipped cases.
After desktop unlock, the rebuilt standalone Gallery was replayed with Mica on
in System (currently Light) and explicit Dark: focus the actual AutoSuggest
editor, type `a`, hover Beta then Gamma, and observe Beta return to the uniform
rest surface without retained hover ink. Both themes passed this live sequence.
The demo remains open on the Dark popup. This limited replay does not promote
the coverage row to Verified. The previously blocking NavigationView failures
are now repaired by the separate palette-lifecycle batch above.

### Reopened evidence — native caption after backdrop disable (2026-09-19)

The existing top-level SystemBackdrop mapping and runtime theme contract remain
unchanged; their caption lifecycle evidence is **partial**. Live Gallery sequence
Dark → Mica on → Mica off → Light leaves a dark caption over light content.
`captionThemeAfterBackdropDisable` reproduces four Light/Dark sequences, including
a theme change while the material is enabled. RED: `build/caption-theme-red4.txt`,
four failures, zero skips. It checks native immersive-dark state plus PrintWindow
caption pixels against the same-state Window palette; caption/text DWM attributes
are set-only, so earlier getter-based drafts are not acceptance evidence.
The fix keeps disabled-backdrop captions in the theme refresh and resolves their
color from the current theme, not the saved pre-backdrop palette. GREEN:
`build/caption-theme-green.txt`, 6 passed, zero failures/skips. Gallery replay of
Dark → Mica on → Mica off → Light now restores the light caption. Loaded and built
DLL hashes match `1C0B683FE46CC8EB8F8E0CC268AF8959E48DFFDC33F9739F4EEA3592111D1C0F`.
Release build and snapshot matrix pass; full offscreen CTest retains the three
pre-existing failures (combobox, navigation, source-contract test-size gate).
Full native CTest passes (85 s); combined result is 47/50 CTest entries passing.
The 22 offscreen domain executables were also rerun: 20 pass, with the same
combobox/navigation failures (`build/theme-fix-domain-logs`).
This does not promote either mapping to Verified.

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
| Runtime theme/accent | system scheme/accent watcher refreshes application palette, owned palettes, open windows and opaque popups | `runtimeAppearanceAndDialogLifecycle`, native `dialogThemeUpdate`, `comboPopupContract` |
| Popup first frame | ComboBox selection/scroll are prepared before presentation; MenuFlyout insets are installed before its first size negotiation | `comboPopupContract`, `menuSizingContract`, native `comboPopupSurface` |

Rows marked `partial` or `not covered` are explicitly not claims of complete
WinUI fidelity. The audit must either close their missing states or narrow the
project's public coverage claims.
