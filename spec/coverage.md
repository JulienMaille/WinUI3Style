# Control coverage ledger

## Issue #28 — wrapped-style palette overrides (2026-10-29)

Mapping: Qt's `QStyle::polish(QPalette &)` is the contract a style uses to
rewrite the application palette, and a `QProxyStyle` wrapping `WinUI3::Style`
uses that hook to tint `QPalette::Highlight` / `HighlightedText`. WinUI's
selection accent is `SystemAccentColor`, which the style sources from
`QPalette::Highlight` (`Tokens::selectionAccent`) and paints as the 3 px
item-view selection pill. A wrapper's tint therefore has to survive the style
install and every later theme/accent refresh.

Defect: `Style::polish(QApplication *)` installed the bare `standardPalette()`,
discarding the palette the wrapper had just polished; `refreshApplicationAppearance()`
did the same on each theme/accent change. The style now routes the standard
palette through the application style's own `polish` hook before installing it
(`polishedApplicationPalette`), so the outermost style in the chain — including
a wrapper — decides the final palette. Without a wrapper this is a no-op, since
`Style::polish(QPalette &)` returns `standardPalette()`.

Evidence: `proxyStylePolishTintReachesApplicationPalette`,
`proxyStylePolishTintReachesSelectionMarker` and
`proxyStylePolishTintSurvivesThemeAndAccentRefresh` reproduce the lost tint
against the reporter's proxy (application palette kept the WinUI accent
`#696cc8` instead of the `#ffc107` tint) and pass GREEN after the fix, each
paired with a `Tokens::selectionAccent` assertion on the same palette. The
direct-palette variants `applicationHighlightOverrideDrivesTokens` /
`applicationHighlightOverrideDrivesSelectionMarker` keep the plain-style path
covered. Live Gallery comparison with the reporter's own proxy remains.

## Integration review — 2026-10-11

NavigationView's existing runtime-theme/backdrop mapping is reopened for
Dark/System → Mica ON → Compact → scroll → Mica OFF → Light. Live A/B on
the unmodified remote SHA `a96b4a8` and the integration build, both Qt 6.8.3,
shows white navigation ink left on a light shell. The Gallery mechanism test
`galleryNavigationPaletteFollowsThemeAfterMicaDisable` is RED on lost palette
ownership with either DLL: style palette writes and parent re-resolution leave
stale child snapshots that are mistaken for application overrides. Both paths
reuse `setBackdropPalette` to preserve ownership snapshots; application palette
overrides and the NavigationView inheritance/lifecycle assertions remain intact.

The user approved the branch's rendering on 2026-10-11 (their Gallery used
Qt 6.11.1), initially excluding the four Light/Dark, Standard/Compact ComboBox
popup captures. After review they explicitly retained the accent marker alone
at rest and the background pill on pointer-hover/keyboard traversal. The
rebuilt native Qt 6.11.1 Gallery shows the pill on pointer-hover and the new
keyboard-current row after Down. The four references now record the approved
rest state, not a permanent selected-row background. All 70 offscreen frames
from the pure remote SHA and the integration build were pixel-identical under
official Qt 6.8.3; the Qt 6.11.1 integration captures are also pixel-identical,
including the four newly enumerated Palette Lab captures. The matrix remains
strict (RGB and alpha); neither tolerances nor assertions were relaxed.

Calendar capture state is pinned to the approved 2026-10-11 fixture. The
midnight rollover to October 12 reproduced a 1,712-pixel selection change,
not a QStyle rendering change. `gallerySnapshotPinsAndRestoresCalendarDate`
is RED before pinning and checks the fixed capture selection plus restoration
of the live selected date and displayed month. Mapping and rendering stay intact.

The existing Top-level backdrop / Mica lifecycle mapping is reopened for
reparenting while active. `reparentedBackdropSurfacesRestore` reproduces the
lost palette/viewport attributes in both directions (bare ↔ opaque card),
Light/Dark × Standard/Compact, and checks disable with and without a resync.
Restoration follows saved state rather than current ancestry. No token values
or Qt→WinUI mappings change; live compositor evidence remains required.

The existing ComboBox mapping also covers `comboPopupOuterGeometry`: 16
theme/density/count/selection cases, three opens each. RED shows an 8 px resize
after Show. DropdownContentMargin is now installed before Qt sizes and places
the container; the Show-time row loop, resize and scroll-range override are
removed. All row, inset, viewport and first-Show assertions remain intact.

Status meanings are defined in `METHODOLOGY.md`. `Verified` requires live mouse,
keyboard, theme, and motion evidence. Automated tests alone can raise a row only
to `source-audited`; missing evidence remains explicit in the last column.

| Qt widget / variant | WinUI mapping | Mapping kind | Current status | Required state evidence |
|---|---|---|---|---|
| `QPushButton` | Button; Qt `flat=true` selects the transparent flat-button visual unless an explicit `winuiControlRole` overrides it | Direct + Qt semantic variant | source-audited; #30 flat-property contract added | direct panel/label ownership, icon/text, menu reserve, rest/hover/down/disabled, mouse/keyboard focus and default/accent are under contract; `flatPushButtonUsesFlatVisual` checks no resting fill/stroke and a visible hover overlay in Light/Dark; default template remains 32 px while explicit large fonts grow the size hint (`largeFontControlSizing`); label text paints grayscale AA (`paintGrayscaleText`, `QFont::NoSubpixelAntialias`) matching WinUI neutral coverage — measured live 2026-09-10 Accent dark (ours ClearType fringes #bd855d/#4c85e8 vs official #8685a5/#4e4d60 neutrals); guard `buttonTextAntialiasesGrayscale`; custom `QPalette::ButtonText` on enabled standard buttons and pictograms was approved, while disabled text retains the WinUI disabled token (`buttonTextPaletteRoleIsHonored`, RED→GREEN Light/Dark); live WinUI comparison remains |
| `QCommandLinkButton` | Command-link composition | Consistency extension | partial — pressed-content regression reopened | 160×64 minimum, title/description ownership, enabled/disabled surface and native activation are covered. The composition derives from the pinned Button template in `winui-2.4/manifest.json`; content stays stationary on press, like neighboring Fluent buttons, rather than inheriting Qt's 2 px translation (`commandLinkPressKeepsContentPosition`, Light/Dark × Standard/Compact). Disabled Standard/command-link buttons inside an opaque card under a live backdrop rebuild the Active-group veiled card tone (`cardRefillTone`, never a forced-178 veil — the 2026-10-08 dark-Mica slab defect where Disabled Base white@8 was forced to white@178; `disabledInCardButtonKeepsCardToneOverComposited`, RED→GREEN Light/Dark × Standard/Compact). Live pointer/focus comparison remains; this is a Qt consistency extension, not an official WinUI CommandLink template. |
| checkable `QPushButton` | ToggleButton | Direct | source-audited | direct off/on surface, hover/down, disabled-on and focus paths are captured; live comparison remains |
| `QToolButton` / toolbar | AppBarButton / CommandBar | Direct; Qt `autoRaise=true` paints a no-chrome-at-rest surface and the normal Standard frame while hovered | source-audited; autoRaise contract added | direct label painting, icon/text layouts, toggle, split-menu partition/hit test, dropdown glyph, hover/down and separator are covered; autoRaise rest has no fill/stroke, hover shows the normal resting frame of a non-autoRaise button and press its pressed surface (`autoRaiseToolButtonUsesFlatVisual`, Light/Dark), while Qt's non-QToolButton dock title buttons (which also set `State_AutoRaise`) keep the dock chrome; `buttonTextPaletteRoleIsHonored` covers custom `QPalette::ButtonText` for enabled text-only tool buttons and preserves the WinUI disabled text token (RED→GREEN Light/Dark); live overflow comparison remains |
| `QToolTip` / tooltip label | ToolTip | Direct | partial | `PE_PanelTipLabel` owns the surface; `applicationTooltipPaletteRestoration` covers the static palette's application ownership on style removal, theme refresh and explicit runtime override. Official template source and full native hover/dismissal evidence still need pinning. |
| `QLineEdit` | TextBox | Direct | source-audited | discrete hover/focus, 10/5/6/6 padding, focused 1/1/1/2 border, Clear glyph, disabled palette, read-only input/clear suppression and selection covered by offscreen contracts; default Standard/Compact heights remain 32/24 px while large fonts grow the hint (`largeFontControlSizing`); `runtimeExplicitPaletteOwnership` protects an application palette installed after polish across theme changes and unpolish while an inherited peer still follows the theme; native light/dark mouse and keyboard comparison remains |
| `QTextEdit` / `QPlainTextEdit` | RichEditBox | Direct | source-audited | discrete rest/hover/focus surface, 2 px accent focus edge, text selection and scrolling tested; live disabled, read-only and dark keyboard pass remain |
| `QComboBox` | ComboBox | Direct | source-audited | closed states, popup, selected pill, row hover/down, dismissal; default Standard/Compact heights remain 32/24 px while large fonts grow the hint (`largeFontControlSizing`); open-popup keyboard-current paints the hover pill row-for-row via `popupRowIsKeyboardCurrent` (verified `comboOpenPopupKeyboardNavTracksHover`); live pointer/keyboard comparison remains; platform divergence 2026-10-28: on Wayland the popup open sweep and the entrance fade are skipped (`toplevelWindowEffectsUnsupported` — no toplevel `setOpacity`, popup-only grabs; first-open geometry evidence #27 still owed from a real compositor) |
| `QSpinBox` | NumberBox inline placement + optional stacked Qt variant | Direct + consistency extension | source-audited | horizontal 36 px and vertical 32 px button partitions, compact 24 px editor height, RTL, click and underline extent tested; dark, keyboard and bounds live pass remains |
| `QCheckBox` | CheckBox | Direct | source-audited | off/on/indeterminate, hover/down, animated accept, reversal, focus |
| `QCheckBox[winuiToggleSwitch=true]` | ToggleSwitch | Semantic variant | source-audited | offscreen rest/hover/click/drag contract; tested travel midpoint/reversal, disabled and focus; dark keyboard pass remains |
| `QRadioButton` | RadioButton | Direct | source-audited | off/on, hover/down dot sizes, group change, disabled, focus; checked-state switch is discrete (duration 0, `radioRapidClickResponsiveness`) — only dot hover/press sizes animate 250 ms Normal; rapid toggles track state like checkbox uncheck |
| `QSlider` | Slider | Direct | source-audited | official 4 px track, 18/22 px outer thumb, 10.32/14/8.52 px inner motion, value tooltip, LTR/RTL/inverted/vertical endpoints, drag, disabled, extreme-range ticks and mouse/keyboard focus covered by offscreen contracts; vertical `sizeHint()` RED 120×84 instead of 32×120 in both densities, corrected along the orientation axis, with the changed Light/Dark Gallery layout accepted by the user; inside opaque group cards the backdrop erase is skipped so the card fill survives on mica (`backdropSliderInsideOpaqueCardKeepsCardFill`, `backdropSliderRepaintDoesNotAccumulate`); native light/dark comparison remains |
| `QProgressBar` | ProgressBar / ProgressRing substitute | Direct | source-audited | determinate/indeterminate, both axes, inversion, disabled, periodic repaint, deterministic freeze and timer cleanup are covered; vertical `sizeHint()` RED 120×83 instead of 21×120 in both densities, corrected along the orientation axis, with the changed Light/Dark Gallery layout accepted by the user; live comparison remains |
| `QTabBar` / `QTabWidget` | TabView (horizontal); Qt dock-tab orientation is a consistency extension (vertical labels for East/West) | Direct + consistency extension | source-audited; #23 Linux Gallery reproduction confirmed 2026-10-28 (CI run 40753516879) | Existing horizontal contract: official 100×32 min item and 12 px default type, 16 px icon, 32×24 close button, separators and selected attached surface; explicit large font raises the hint and paints with matching metrics (`largeFontControlSizing`); no legacy accent underline; over a live backdrop the pane card keeps `veiledCard(t.layer)` with an alpha-0 gap (`tabPaneCardVeilsOverComposited`, reopened 2026-10-03). Issue #23's East/West dock tabs now rotate labels and size the real tab rectangles vertically; the Gallery capture shows the labels, while `dockTabLabelOrientation` checks East/West ink direction and horizontal label centering in Standard/Compact. Pixel-exact Windows baselines were refreshed for the intentional horizontal centering change. Live WinUI comparison and merge remain pending. |
| popup `QListView` | ComboBox item | Direct | source-audited | selected pill, hover/down, row height, scrolling; open-popup arrows move the view current index (selection follows) and repaint the hover pill on the new current row while the value row keeps only its accent marker (`comboOpenPopupKeyboardNavMovesHoverPill`); Wayland: the popup window slides 0 px because the open sweep is skipped there — geometry itself is unchanged and identical to XCB |
| `QComboBox` popup interior under a granted popup backdrop (composited `Qt::Popup` container + view/viewport) | ComboBox flyout surface over Desktop Acrylic | Direct (backdrop extension of the ComboBox and popup `QListView` rows above) | fixed 2026-10-04 (round-2 capture: fully transparent horizontal band in every combo popup under Mica → RED `comboPopupViewportRebuildsPopupTint` viewport rect at alpha 0 → GREEN popup-tint refill; Gallery Mica re-capture puts y124..131 / y100..107 back at the tint 178/242, popup interior pixel-identical to NONE but the 1px flyout stroke and corner arcs, NONE popups byte-identical) | Mapping: with a granted popup backdrop the whole popup is ONE Qt-side tint layer — `popupSurfaceColor` at the popup tint alpha (178 Dark / 242 Light) laid by `PE_PanelMenu` over the DWM acrylic — plus item ink on top; `preparePopupSurface`'s composited branch owns that recipe (Window + Base on the same tint, no autofill, no opaque-paint claim, `DropdownContentMargin` `0,4,0,4`); the Show-time +8 height compensation runs once per container lifetime so reopened cached popups stay rows+8 with no viewport tail. Nothing inside the popup may leave a Source-cleared row behind: alpha 0 there has no material to reveal, it is a hole through the window. Recipe 1 (erase-then-fill with the popup tint) for the popup's own view/viewport `PE_Widget`; row pills keep recipe 3 (`clearForBackdropFill`); the row/pill geometry is unchanged. Mechanism evidence: `comboPopupViewportRebuildsPopupTint` (RED viewport rect Source-cleared to alpha 0 where the popup tint must stand → GREEN tint token preserved), paired with `paintsDirectlyOnBackdrop` and `contentsMargins(0, 4, 0, 4)` asserts, Compact-parametrized. Round-2 marks: Mica `dark/light-combo-popup.png` band y124..131 (Standard, 200×136) and y100..107 (Compact, 200×112) at alpha 0, NONE mode reads 178/242 there. Live re-review of an open combo popup under Mica remains before Verified. |
| `QLineEdit` + `QCompleter` (PopupCompletion) | AutoSuggestBox | Semantic variant | partial — live local interaction observed, full parity pending | stock editor + completer keep semantics, signals, focus, and hit testing; QStyle owns the popup surface and every row visual. Shared `paintPopupRowPill`/`popupRowPillRect` geometry remains 4,2 insets and 3px radius. CE/PE preserve resolved Base RGB with opaque row frames: `autoSuggestResolvedFramePreservesRgb` covers Light/Dark × Standard/Compact × rest/hover/leave. `autoSuggestPopupHitTestAndFallback` checks typed completion, both row gutters, row height and opaque fallback in the same four variants. Live local Gallery on 2026-09-29: Dark Standard typing `a`, Down×2, Enter selected Beta; Dark Compact left-gutter click selected Beta; Light Compact Escape retained `a`; Light Mica on/off/reopen popup stayed readable without ghosting. The opt-in PrintWindow popup suite passes Light/Dark (4/4 including init/cleanup) with legible opaque row captures. The native SendInput test skips honestly if Windows refuses process foreground; a GREEN automated keyboard run and full official side-by-side remain pending. |
| `QListView` / `QListWidget` | ListView | Direct | source-audited | official 40 px rows, padding, subtle hover/selected/down layers, accent selection pill, icons/checks, editing, disabled and keyboard focus; live pointer sequence remains |
| `QTreeView` / `QTreeWidget` | TreeView | Direct | source-audited | official 28 px rows, 4×2 margin, Fluent expanders, hierarchy, selected pill and keyboard focus; multi-column header is a consistency extension; live pointer sequence remains |
| `QTableView` / `QTableWidget` / headers | DataGrid-like extension | Consistency extension | source-audited; #29 dark alternate-row contrast corrected 2026-10-28 | 36 px rows, subtle row selection, flat 32 px default headers, sorting glyph, editing and scrolling tested; explicit large header font paints with matching metrics (`largeFontControlSizing`); dark TableView reference is #2b Base / #2d alternate, versus pre-fix Gallery #27 / #3c; dark `AlternateBase` is now white@1 and `darkAlternateRowsStaySubtle` (Standard/Compact) checks rendered row geometry, palette token and <=3 RGB-level contrast. No core WinUI DataGrid is claimed; live pointer/resize pass remains. |
| `QMenu` / `QMenuBar` | MenuFlyout / menu bar extension | Direct + extension | reopened 2026-10-04 (in-card review defect; local native edge checks pass, full parity pending) | Existing menu contracts cover actions, layout, density, corners and same-HWND reopen. `menuMicaToggleKeepsPaintedEdges` repaired the Mica ON/OFF native acrylic band drift by recreating only hidden owned QMenu HWNDs. Short dark segments at the top/bottom border of the Dark Split popup came from combining Qt's raster mask with DWM rounding. The native path now uses DWM rounding alone when granted, keeping the raster fallback otherwise; native contracts check the grant, empty Qt mask and actual rounded corner capture. Live Gallery Mica ON/OFF and popup reopen no longer show the reported segments, and the user accepted the observed result. Opening motion remains the accepted 2026-09-09 deviation — on Wayland the divergence rule changed: the opening motion is skipped outright rather than approximated, because toplevel window opacity does not exist there (`toplevelWindowEffectsUnsupported`); full WinUI side-by-side verification remains. |
| `QCalendarWidget` inline + popup | CalendarView + popup extension | Consistency extension | partial — reopened 2026-09-17 (inline defect); 2026-09-18 Mica regression RED captured (calendar-mica-red4-native.txt: granted-Mica inline body #ff212121 baked opaque vs unflattened composited ink #ca212121; offscreen 14 passed/8 skipped, fallback rows green) | Actual reported control: demo persistentCalendar in dialogPartsGroup, Mica off, not QDateEdit popup. Pinned [CalendarView template](https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/CommonStyles/CalendarView_themeresources.xaml) places header and days on ONE Background border: CalendarViewBackground → ControlFillColorInputActiveBrush. Same-commit Common_themeresources_any.xaml defines Dark #B31E1E1E / Light #FFFFFFFF, exactly existing Tokens::editorFocusedFill. For the plain opaque Gallery host, resolve that brush once over the existing group card (Tokens::layer over Tokens::surface); the resulting body/header pixels are opaque. This is NOT popupSurfaceColor, not alpha-forcing the raw brush, and not a new hardcoded colour. Idle navigation background AND border are SubtleFillColorTransparent; pointer-over/pressed use Subtle Secondary/Tertiary, focus retains FocusStroke. No geometry change. Old inline Standard-button policy is superseded; conflicting tests remain unchanged pending parent acceptance: density_widgets lines 1005, 1013–1015 (Base=layer, header=Window), 1112–1119 (Standard + Button=Window). Popup effective acrylic/fallback contract and previous popup evidence remain separate. Initial draft RED logs had an invalid header bounding-box-center probe and incomplete host hierarchy: not acceptance evidence. Corrected fixture RED native + offscreen with Windows fonts: 8 rows each (Light/Dark × Standard/Compact × body/buttons), 0 skipped. Actual body #808080/#111111 vs resolved InputActive #FFFFFF/#212121; valid empty-header pixels #F3F3F3/#202020; previous-button top-center stroke #E5E5E5/#303030. Group card independently matches layer-over-window oracle (#F9F9F9/#272727). Header membership, glyph clearance and geometry asserted; no equality-only transparent acceptance. Source audit: calendar CE_ItemViewItem Source-replaces rows with unresolved translucent Base (#80FFFFFF/#4C3A3A3A), while header uses opaque Window and Standard buttons retain stroke. Logs calendarheader-inline-{native,offscreen}-final-red.txt; earlier draft/valid-red logs are probe-development failures, NOT acceptance evidence. Live screenshot calendar-live-after-foreground.png has the same dark body #111111 versus empty header #202020. 2026-09-18 Mica fix RED→GREEN: inlineCalendarSurface restored to transparent seed under Composited (an intermediate edit had reverted it to unconditional opaque); backdrop toggle now re-runs owned palettes via refreshOwnedPalettes on effective-surface change (previously only theme switches did, so Mica toggles stuck opaque); Composited publish moved after island sync (publishing first refreshed against stale opaque islands via paintsDirectlyOnBackdrop); refresh re-syncs chrome/islands after recompute (generic rebase otherwise re-opaques them); inline nav-bar branch Composited-aware and empty stretch Source-fills (was double-blending veiled-over-veiled toward opaque). Mica RED logs calendar-mica-red4-native.txt (dark body #ff212121 vs veiled #ca212121) now GREEN: calendarheader 22/22 native + unit, density 21/21 native + unit (includes real-grant lane contract replacing the faked-effective poison rows, and grant-aware popup/auto-suggest asserts). Gallery PID 23260 DLL 4D20E78B (matches build), Mica ON with live DWM grant (attr38=2) confirmed; live inline-calendar Mica pixels pending (Dialogs-page navigation unresponsive to automation: nav reports selected while content stays on Controls). Pre-existing native topLevels 6→5 failure unchanged, triaged separately, not weakened. |
| `QScrollBar` | ScrollBar | Direct | source-audited | official 12 px extent, 8→12 px reveal, 400/500 ms delays, 167 ms reversal-safe expand/contract, arrows, 30 px thumb, RTL, disabled-zero-opacity and real `QScrollArea` interaction covered by offscreen contracts; 2026-10-06 disabled-groove min-clamp (exact-178 forced the faint Disabled Base to an opaque white band in dark Mica; `disabledBarGrooveKeepsRoleAlpha`); native light/dark comparison remains |
| `QGroupBox` | Fluent card grouping | Consistency extension | source-audited | plain/checkable, hover/down check, disabled, focus; over a live backdrop the card keeps its `veiledCard(t.layer)` fill and child viewport/editor/splitter paints never Source-clear it (`groupCardChildKeepsCardFillOverComposited`, reopened 2026-10-03) |
| `QSplitter` | Fluent separator | Consistency extension | source-audited | both axes, hover, drag, clamp, release |
| `QDockWidget` | Fluent layer/card docking | Consistency extension | source-audited | dock/floating title, buttons, separator hover/drag, focus |
| `QStatusBar` / `QSizeGrip` | Window footer / resize affordance — **bare material, recipe 2 (chrome reveal)** | Consistency extension | reopened 2026-10-03 (round-2 capture mark) | Mapping corrected by the round-2 mark: the footer is not a layer surface. Over a live material `PE_PanelStatusBar` erases and returns with no Window fill — only the 1 px top stroke — so the material shows through (recipe 2, chrome reveal); the opaque Window fill below that branch is the non-Composited fallback only. Doc-only correction, no painter changed: the recipe the source implements is the one now named here. Top separator, automatic grip discovery, 16 px metric and all-corner glyph geometry stay as covered; live resize and high-DPI comparison remain |
| opted-in `QDialog` / `QMessageBox` | ContentDialog | Compound mapping | source-audited | distinct content and command/footer surfaces, 320×184 minimum, 24 px content margin, 12 px spacing, accent default button, Fluent message glyphs and 83/250 ms show motion; live modality/dismissal/dark comparison remains |
| `QWizard` / `QWizardPage` | Multi-step dialog composition | Consistency extension | source-audited | separate content/footer surfaces and automatic accent Next/Finish roles are covered; native navigation, cancellation, RTL and live WinUI comparison remain |
| `QAbstractItemView[winuiNavigationView=true]` | NavigationView item | Semantic variant | reopened 2026-10-04 (in-card review defect; local fixes, full parity pending); 2026-10-06 uniform-mica reversal (reviewer: no added pane backgrounds — shell pane and palette preview stay on the bare material) | style-owned delegate, real click, selected-pill transition, palette ownership and Mica/theme restoration tested; [live audit](NAVIGATIONVIEW_AUDIT_2026-09-28.md) pinned `Selected` / `PointerOverSelected` / `PressedSelected` to Secondary / Tertiary / Secondary. `navigationSelectedStateFill` reproduced the reversed order RED across Light/Dark × Standard/Compact, then passed GREEN after correcting those fills. `navigationIconlessTextInset` likewise reproduced the 42px iconless inset RED, then passed GREEN at 18px, retaining 42px when an icon exists; the pinned XAML has `IconCollapsed` column width 8px and content margin 4px. Commits were authorized by the user; Compact official instance, mouse-down frame, keyboard focus and motion remain unverified |
| `QFrame[winuiSettingsCard=true]` / settings-card composition | Gallery settings card | Semantic variant + compound behavior | source-audited | interactive iff an expandable child is bound (`isCardInteractive`); trailing-only cards keep resting fill and ignore header clicks; expandable chevron is ChevronDown collapsed and expanded; expanded host shares the card surface with no fill/border; dark live pass remains |
| `Top-level QWidget[winuiBackdrop=mica]` (+ `micaalt`, `acrylic`) | Window SystemBackdrop (Mica / Mica Alt / Desktop Acrylic via `DWMWA_SYSTEMBACKDROP_TYPE`) | Direct | partial — caption lifecycle reopened 2026-09-19 | `applyBackdrop` maps to DWM `DWMSBT_MAINWINDOW`/`TABBEDWINDOW`/`TRANSIENTWINDOW` + full-frame extend, opaque `Painted` fallback offscreen/refused; single erase policy in `winui3helpers_p.h` (shared gate + recipe 1 erase-then-fill default, recipe 2 erase-means-transparent, recipe 3 `clearForBackdropFill` for acrylic popup pills) + island scroll guard + chrome/content transparentize/restore covered by `backdropLifecycleContract`, `backdrop*DoesNotAccumulate`, `islandScrollPostsFullViewportRepaint`, `materialErase*`, theme/chrome/resize/expose/move contracts; caption evidence below; broader live compositor verification remains. |
| `QTabWidget` pane + item-view card inside `winuiSurface=content` under a live backdrop | TabView content card over SystemBackdrop (same veiled LayerFill card recipe as the Fluent group card) | Direct (backdrop extension of the TabView/ListView rows above) | reopened 2026-10-03 (user-confirmed Collections card defect) | Mapping: NONE mode keeps the opaque `t.control` pane card with a `t.surface` gap (unchanged, snapshot-matrix covered). Over a Composited material the pane keeps `veiledCard(t.layer)` — white@128 Light / (58,58,58)@76 Dark, the same token group cards use, clamped by `VeiledCardAlpha` — while the island gap around the card and the window chrome stay alpha 0. `QTabWidget` and its pages are keep-opaque container cards (recipe 1 fill via `PE_FrameTabWidget`, never transparentized by `syncContentSurfacesForBackdrop`), and a child `PE_Widget` Source-clear inside the card is skipped by the shared card guard. Mechanism evidence: `tabPaneCardVeilsOverComposited` (RED pane alpha 0 / transparent card canvas → GREEN veiled card tone + alpha-0 gap); recipe state re-scoped in `tabPaneCardLinksStayOpaqueWhileIslandSharesMaterial`. Live re-review of the Collections page under Mica remains required before Verified. |
| child viewport / editor / splitter / check / lineedit inside an opaque card under a live backdrop | child control over a card surface (WinUI paints children on the card, never a hole to the material) | Consistency extension (backdrop contract of the `QGroupBox` row above) | reopened 2026-10-04 (palette-lab preview defect: `CE_CheckBox` / `PE_PanelLineEdit` children of the non-`QGroupBox` `palettePreview` card still punch alpha-0 holes; viewport/editor/splitter guard unchanged) | Mapping: the card keeps its `veiledCard(t.layer)` fill, and a child that paints straight onto the material by the shared gate must never Source-clear the card fill — the slider/scrollbar precedent (`backdropSliderInsideOpaqueCardKeepsCardFill`, `scrollBarInsideOpaqueCardKeepsCardFill`) is now the shared `insideOpaqueCard` guard used by the `PE_Widget` erase path, the scroll-chain/bare-container sync, and the range-control erases. Viewport/editor/splitter children inside a card are not transparentized (no `WA_StyledBackground`, no zero-alpha Window role), and even a directly dispatched `PE_Widget` erase returns without clearing. Selected/hovered item rows rebuild the card tone under the pill (same erase-then-fill, `treeSelectionKeepsCardFillOverComposited`, 2026-10-06). Mechanism evidence: `groupCardChildKeepsCardFillOverComposited` (RED card canvas punched to alpha 0 under the viewport/editor → GREEN card tone survives, paired token/geometry asserts). Live re-review of the scrolled Dialogs page under Mica remains required before Verified. |
| Standard-flavour button child inside an opaque `QGroupBox` card under a live backdrop (gallery `floatDockButton` in `dockGroup`) | Fluent Button over the group card: ControlFillColorDefault composites ON the card's LayerFill, never on the bare material | Consistency extension (backdrop contract of the `QGroupBox` / card-child rows above) | fixed 2026-10-04 (round-2 capture: 32 px unfilled band → RED `standardButtonInsideCardKeepsCardFillOverComposited` seeded card canvas punched to alpha 0 → GREEN card-tone rebuild, with a backdrop-off pairing that fails when the rebuild is not tied to the erase; Gallery Mica re-capture band reads (91,91,91,87) Dark and white@217 Light — card tone ⊕ control fill — while card band y405..448 and island gap y388..403 stay 0-changed and NONE stays byte-identical) | Mapping: the button keeps its translucent control fill (dark `QPalette::Button` = white@15, `Mid`/`Midlight` strokes white@18/24) and lays it OVER `veiledCard(t.layer)` — in NONE the opaque card makes that composite read #343434, under Mica the same two layers read card@76 + white@15. Recipe 1 in two layers: the button's `eraseForBackdrop` Source-clears its rect and must rebuild the card tone first (`insideOpaqueCard`, the same guard the `PE_Widget` erase and the range-control erases already use), then `controlSurface` lays the control fill; the Subtle/Navigation rest branch already rebuilt the card tone this way and stays untouched. Mechanism evidence: `standardButtonInsideCardKeepsCardFillOverComposited` (RED seeded card canvas punched to alpha 0 by the panel erase → GREEN card tone survives under the translucent control fill), paired with `paintsDirectlyOnBackdrop` / card-tone `QCOMPARE` asserts, Compact-parametrized. Round-2 marks: `captures-mica/dark-page-3-scrolled.png` band y449..480 x309..1118 read white@15 only in Dark Mica — Light Mica read the fill alone at white@179, the same missing card tone just far less visible — while NONE keeps the opaque card composite #343434, while y405..448 (card band) and y388..403 (island gap) must NOT change. Live re-review of the scrolled Dialogs page under Mica remains before Verified. |
| `QWidget#palettePreview` palette-lab preview card under a live backdrop (gallery `palettePreview` + `previewCheck` / `previewEdit` children) | Fluent card surface over SystemBackdrop (same veiled LayerFill card recipe as the `QGroupBox` card) | Consistency extension (backdrop contract of the `QGroupBox` / card-child rows above) | reopened 2026-10-04 (reviewer live screenshot: Mica-dark preview card punched alpha-0 holes showing wallpaper) | Mapping: NONE keeps the opaque card composite (unchanged). Over a Composited material the `palettePreview` widget is a recipe-1 card (`PE_Widget` lays `cardVeil(t.layer)` — exactly `VeiledCardAlpha`, since the standard Base layer is itself translucent and min-clamping would wash the card to raw wallpaper; opaque windows keep the full card tone). The sync arms the plain card with `WA_StyledBackground` (remembered autofill off while armed, restored on toggle-off) so the veil has a dispatched painter in real paint — without the arm Qt autofills the rebased opaque palette and the card reads solid black in dark mode. It is a `keepOpaqueContainer` (via `isCardSurface`, already naming this object) so `syncContentSurfacesForBackdrop` never transparentizes it or arms its children, and the `insideOpaqueCard` child walk finds it (a structural ancestor match — `paintsDirectlyOnBackdrop`'s opaque-island walk cannot see the card, and never reaches this subtree). The `CE_PushButton` / `CE_CheckBox` (`previewCheck`) / `PE_PanelLineEdit` (`previewEdit`) children erase-then-refill the card tone (button Fix-B mirror): the panel erase Source-clears the child rect, then the card tone is rebuilt first and the control ink laid over it. Mechanism evidence: `palettePreviewCardKeepsCardFillOverComposited` (RED seeded card canvas punched to alpha 0 by the container/checkbox/lineedit erases → GREEN card tone survives under/with the control ink, paired with `paintsDirectlyOnBackdrop` / card-tone `QCOMPARE` asserts, dark/light x density). Live re-review of the Palette lab page under Mica remains before Verified. |
| NavigationView sample inside an opaque `QGroupBox` card under a live backdrop (gallery `standardDensityNav` / `compactDensityNav` in `densityComparisonGroup`) | NavigationView item over the group card: rest rows keep the card LayerFill, never a hole to the material | Semantic variant (backdrop contract of the NavigationView / `QGroupBox` rows above) | reopened 2026-10-04 (reviewer live screenshot: in-card nav punched alpha-0 holes showing wallpaper) | Mapping: NONE keeps the opaque card composite (unchanged). Over a Composited material both the top-level NavigationView and the palette preview stay on the bare material (recipe 2, reviewer-directed uniform-mica 2026-10-06: their NONE backgrounds are implicit, so any added fill reads as dirt on the wallpaper); the in-card sample is recipe 1: `prepareNavigationSurface` / `refreshNavigationPalette` early-out via `insideOpaqueCard` (never transparentized), the item-delegate erase is skipped in the card, and rest rows composite over `cardVeil(t.layer)` (exact `VeiledCardAlpha`). Mechanism evidence: `navSampleInCardKeepsCardFillOverComposited` (RED view Base/Window punched to alpha 0, GREEN opaque card tone survives, dark/light x density) paired with updated shell-pane asserts (top-level nav rest rows stay cleared, recipe-2 alpha-0 contract restored per reviewer uniform-mica direction). Live re-review of the density panels under Mica remains before Verified. |
| `QMenuBar` sample inside an opaque `QGroupBox` card under a live backdrop (gallery `standardDensityMenu` / `compactDensityMenu` in `densityComparisonGroup`) | Menu bar extension over the group card: bar keeps the card LayerFill, never a hole to the material | Direct + extension (backdrop contract of the `QMenu` / `QGroupBox` rows above) | reopened 2026-10-04 (reviewer live screenshot: in-card menu bar punched alpha-0 holes showing wallpaper) | Mapping: NONE keeps the opaque card composite (unchanged). Over a Composited material the shell menu bar keeps recipe 2 (erase-means-transparent reveal, UNCHANGED); the in-card sample is recipe 1: `makeChromeSurfacesTransparent` skips bars with `insideOpaqueCard`, and `PE_PanelMenuBar` / `CE_MenuBarEmptyArea` / `CE_MenuBarItem` refill `cardVeil(t.layer)` (exact `VeiledCardAlpha`) when the erase ran in a card (mirror of button Fix B). Mechanism evidence: `menuSampleInCardKeepsCardFillOverComposited` (RED bar Window punched to alpha 0, GREEN veiled card tone, dark/light x density) paired with `shellMenuBarRevealsMaterial` (top-level bar still alpha 0) and the opaque `QLineEdit` password reference. Live re-review of the density panels under Mica remains before Verified. |


The backdrop row is reopened again on 2026-09-28: the live Dark/Standard Gallery
retains old card images after Mica ON → OFF, and a repeated cycle leaves the
Controls viewport black instead of restoring the opaque dark surface. Switching
to Light after OFF preserves that black gap and reintroduces ghost images.
The existing offscreen lifecycle tests do not cover this native transition.
The targeted native test `backdropDisableRestoresOpaqueContent` now checks
opaque surface restoration, palette ownership, Dark → Light refresh, repeated
toggles, and scrolling. The live Gallery sequence was rechecked after the
fix; broader compositor configurations remain partial coverage.

### Reopened evidence — Mica card surfaces (2026-10-03)

Two user-confirmed defects from the side-by-side capture review reopen the
backdrop row above and the `QGroupBox`/`QTabWidget` rows; mapping and tokens
are unchanged, the card recipe is what moved.

1. **Collections card had no veil.** In NONE mode the tab-pane/list card
   paints an opaque card with a `t.surface` gap; under Mica the whole card
   region probed alpha 0 because `syncContentSurfacesForBackdrop`
   transparentized the `QTabWidget` + pane links and `PE_FrameTabWidget`
   followed recipe 2 (erase-means-transparent, hairline only). The recipe is
   now 1 for the tab card: erase-then-fill with `veiledCard(t.layer)` (the
   group-card token), while `QTabWidget` and its pages join
   `keepOpaqueContainer` so the sync never arms them; the island gap stays
   alpha 0. Mechanism test `tabPaneCardVeilsOverComposited` was added RED
   (pane canvas alpha 0 instead of the veiled card tone) before the fix.
2. **Children Source-clear the group-card fill.** Viewport/editor/splitter
   children inside a card were transparentized (zero-alpha Window +
   `WA_StyledBackground`), so Qt dispatched `PE_Widget` and the erase branch
   punched holes through the `veiledCard(t.layer)` fill painted moments
   earlier (visible after scrolling the Dialogs page). The slider/scrollbar
   card guard is generalized into the shared `insideOpaqueCard` walk in
   `winui3helpers_p.h` (structural: the card keeps its fill even when its
   Window role inherits the island's zero alpha), applied to the `PE_Widget`
   erase path and to the scroll-chain/bare-container sync. Mechanism test
   `groupCardChildKeepsCardFillOverComposited` was added RED (card canvas
   cleared to alpha 0 under the viewport erase).

The slider row's guard note above stays accurate; the guard is now shared
instead of private to `winui3complex_rangecontrols_p.cpp`. Existing recipe
assertions were re-scoped, not weakened: `tabPaneLinkTransparentizesWhileCard
StaysOpaque` encoded the defective transparent-pane recipe and was renamed to
`tabPaneCardLinksStayOpaqueWhileIslandSharesMaterial`.

## P1 and structural closure ledger

### Reopened evidence — shell nav pane veil (2026-10-06, reversed same day)

Side-by-side scan (dark/light `page-0`, y=400) shows the NONE nav column as a
tonal pane (dark #272727 / light #F9F9F9) separated from content by a
window-paper gap channel, while Mica leaves pane and gap uniformly alpha-0:
no pane contour survives (reviewer: "le bord du panneau", thrice-marked).
There is no 1px separator stroke in NONE to preserve — the contour IS the
tonal column — and a recipe-1 veil was built for it first (flatten-matched,
then card-like for live visibility). The reviewer rejected every added
background on sight ("fond dégueulasse", "mica uniforme"): surfaces whose
NONE background is implicit must show the bare material, period. The veil
(plus the palette-preview veil) was reverted the same day; both stay
recipe 2. The relational rule stands clarified: no NEW tonal step versus
the live parent. In-card samples are unaffected (card veil). Chrome around
it (menu bar, status bar, title) stays recipe 2. Mechanism evidence: shell
alpha-0 asserts restored, `palettePreview` transparent-design asserts (bare
strip/corner alpha 0, children keep their own fills, no veil arm).
Full-page snapshot baselines shift back; re-approval required.

### Reopened evidence — ComboBox popup outer geometry (2026-10-02)

The direct ComboBox/ComboBox-item mappings are unchanged, but their geometry
evidence is reopened: live Standard/Compact density popups have a larger lower
band than QMenu and split-button menus. Row-height tests do not establish the
full popup layout. `comboPopupOuterGeometry` checks exact 4 px outer bands,
complete visible rows, zero unnecessary scrolling, first-show height and
reopen stability in Light/Dark × Standard/Compact with two/three items and
initial selections 0/1. QMenu geometry and the pinned 40/32 px row sizes remain
outside this correction's scope.

RED: `build/combo-outer-red.txt` fails all 16 cases on first-open height
(80 instead of 88 for two Standard rows), with scrollbar maximum 1 and a
10 px scroller consuming viewport space. Installing the inset at polish,
before Qt's size negotiation, repairs this. Native validation then rejected
the old Show-only +8 px workaround (`build/combo-outer-windows.txt`, 16 failures);
removing that resize repairs the excess bottom space without changing tokens.
Final native/offscreen matrix passes all 16 cases across three opens each;
`comboPopupSurface` now also asserts exact outer/viewport geometry and captures
the native popup through PrintWindow (`build/combo-outer-native.png`).

The rebuilt Gallery DLL matches the library (SHA256 starts `186A1D16809F4BD8`).
Live local density/theme flyouts were replayed in System/Dark and Light,
Standard/Compact, including mouse selection, Escape and Up/Enter selection;
the extra band is no longer observed. This is not an official side-by-side
Verified pass. Release configure/build and all 23 domain suites pass. Full
CTest is 53/54: only the pre-existing strict RGB baseline mismatch remains
(dark-combo-popup, 6804 differing pixels, max delta 12). All 66 deterministic
captures are byte-identical to the pre-fix PoC captures, so no out-of-scope
visual change or baseline replacement was claimed at that time.

Superseding integration evidence, 2026-10-12: the popup sizing fix is committed
and the user-approved reference is now 70 frames, including Palette Lab.
Official Qt 6.11.1 Release passes 59/59 CTest and all 26 domain binaries;
the native outer/viewport assertions below remain intact. The former
selected-row background discrepancy was reviewed and the user retained the
accent marker alone at rest, with the fill on hover/keyboard traversal.
Earlier counts and DLL hashes above are historical evidence, not the current
release gate. No mapping, token or Verified status is changed.

### Reopened evidence — ComboBox popup row surface (2026-10-16)

Live defect: every ComboBox popup showed a lighter horizontal band at its top
and bottom; menus were unaffected. Measured on a live popup (offscreen probe,
same palette and geometry as the Gallery): the 4 px container margins read the
single flyout tint layer (alpha 178 Dark, flattened #1f1f1f), while the item
view area read alpha 232, i.e. the same 178 tint stacked twice (#282828). The
owner of the second layer is Qt, not the style: with `SH_ComboBox_Popup` = 1 Qt
installs `QComboMenuDelegate`, whose `paint` fills every row rect from the
palette Window role before it asks for `CE_MenuItem` (qcombobox.cpp). That role
is the same translucent flyout tint `PE_PanelMenu` already laid over the whole
popup rect, so each row stacked the surface a second time; `QMenu::paintEvent`
lays no per-row fill, which is why only combos drifted. The concurrent
popup-grant hardening (readback-confirmed grant, opaque fallback convergence)
is unrelated to this symptom and stays.

Fix: the combo row branch of `drawMenuItemControl` now Source-replaces its own
row rect with the SAME palette Window role (deliberately unclipped: clipping to
the row's rounded corners would round every row seam and keep the double layer
in the corner slivers), so a row and the container margins agree on exactly one
surface layer in both the composited and the opaque recipe. No token, geometry
or menu-row change: menu rows keep their recipe-3 `clearForBackdropFill` pill
path, and the hover pill still composites over the single layer.

Mechanism evidence: `comboRowKeepsSingleFlyoutSurfaceLayer` builds the two
upstream calls (container `PE_PanelMenu` over the whole popup rect, then Qt's
row fill and `CE_MenuItem` with the `QComboBox` as style widget) and asserts one
layer on the margin row and on the row body and its extreme corners, paired
with the `popupSurfaceColor` token and the `0,4,0,4` container margin geometry.
RED before the fix: row alpha 232 vs the expected 178. Local re-measurement
after the fix reads 178 on the margins and on every row. Live confirmation,
2026-10-17: the user reopened combo popups in the running Gallery (build-gallery
at `156b3f5`, DLL 19:50:58) and reports the top/bottom bands gone; the row is
verified again.

### Reopened evidence — Mica palette registry traversal (2026-10-01)

The Dark/System Gallery crashed on Mica activation in `refreshOwnedPalettes`.
A synchronous `PaletteChange` can polish new children and mutate the palette
owner registry while it is being traversed. `ownedPaletteRefreshSurvivesOwnerRegistration`
reproduces that mechanism with a calendar and newly polished labels: RED access
violation, then GREEN after both refresh phases use a stable `QPointer` snapshot.
The top-level SystemBackdrop mapping is unchanged; repeat the live activation,
disable and theme-switch sequence before claiming verified coverage.

Local PoC gate on 2026-10-01: Release configure/build, 53/53 CTest and all
23 current non-native domain executables pass. The installed plugin also
loads in an independent client linked only to Qt. Live Gallery Mica ON → OFF
→ Light restores readable opaque content without the observed crash/ghosts.
This is limited local evidence, not a Verified promotion.

Commit validation on 2026-10-02 initially returned 52/53 CTest:
`backdropDisableRestoresOpaqueContent` failed at its initial desktop sample,
before enabling Mica, with denied foreground focus and a wallpaper-only
Gallery capture. The subsequent interactive-desktop rerun passes 53/53 CTest
and 23/23 current non-native domain executables without changing assertions.
The user confirmed Gallery validation and authorized the remaining commits;
this does not establish full official WinUI parity or promote rows to Verified.

The native opaque-restoration fixture now stays above unrelated windows while
sampling the desktop. The previous Light #F0F0F0 versus #F3F3F3 failure came
from the official Gallery's DWM shadow crossing the probe: simultaneous
PrintWindow/Qt captures were #F3F3F3, and the same assertions passed without
that shadow. Keeping the fixture topmost preserves every color/palette/scroll
assertion and avoids accepting external shadow ink as control output.
Explicit AutoSuggest PrintWindow capture passes both themes; its four native
keyboard rows still skip when Windows denies OS foreground focus.

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
| Polish symmetry | palettes, autofill, hover/opaque attributes, margins, spacing, list spacing, navigation delegate/mouse tracking, timers and role properties are restored; a font explicitly set by the application after style polish survives unpolish while the style-owned default restores the pre-style font | `styleMutationRestoration`, `applicationFontOverrideSurvivesUnpolish` |
| Accent role separation | selection uses `SystemAccentColor`; control AccentFill uses the theme-specific ramp role; text-on-accent has separate theme roles | `palettes`, `runtimeAppearanceAndDialogLifecycle` |
| Palette-owned glyphs | Fluent glyphs are explicit foreground masks; arbitrary application icons retain their colors | `buttonToolButtonAndIconContracts` |
| Covered-style fallbacks | debug assertions reject covered primitives, controls and complex controls that reach `QCommonStyle`; covered content sizing is explicit | all render/state tests plus deterministic snapshot matrix |
| RTL and hit testing | ComboBox, menus, GroupBox, ToolButton, Slider, tabs and headers use visual geometry and direct hit testing | `rtlGeometryAndHitTesting`, `buttonToolButtonAndIconContracts` |
| Check/radio reverse motion | fill, check path and radio dot all consume the animated progress in both directions | `checkboxAndRadioUncheckMotion` |
| Navigation model lifecycle | delegate reconnects to replacement model and selection model and resynchronizes on scroll | `navigationModelReconnectAndScroll` |
| Dialog lifecycle | opacity-only show motion avoids layout geometry changes; hide/reopen clears animation state | `runtimeAppearanceAndDialogLifecycle`, `contentDialogContract` |
| Runtime theme/accent | System mode follows the platform color scheme, not the style-owned application palette (which only reports the theme the style last applied); the system scheme/accent watcher refreshes the style-owned application palette, owned palettes, open windows and opaque popups; an application palette explicitly replaced after polish is retained through theme changes and unpolish, including a child editor | `runtimeAppearanceAndDialogLifecycle`, `applicationPaletteOverrideSurvivesThemeAndUnpolish`, `systemModeFollowsPlatformSchemeOverStyleOwnedPalette`, `platformSchemeOutranksStyleOwnedPalette`, native `dialogThemeUpdate`, `comboPopupContract` |
| Wrapped application style palette | a `QProxyStyle` wrapping `WinUI3::Style` rewrites the application palette from `polish(QPalette &)`; the install and every theme/accent refresh run the standard palette through the application style's own polish hook, so a wrapper's `QPalette::Highlight` / `HighlightedText` tint reaches `Tokens::selectionAccent` and the painted selection instead of being replaced by the bare standard palette | `proxyStylePolishTintReachesApplicationPalette`, `proxyStylePolishTintReachesSelectionMarker`, `proxyStylePolishTintSurvivesThemeAndAccentRefresh`, `applicationHighlightOverrideDrivesTokens`, `applicationHighlightOverrideDrivesSelectionMarker` |
| Popup first frame | ComboBox selection/scroll are prepared before presentation; MenuFlyout insets are installed before its first size negotiation | `comboPopupContract`, `menuSizingContract`, native `comboPopupSurface` |

Rows marked `partial` or `not covered` are explicitly not claims of complete
WinUI fidelity. The audit must either close their missing states or narrow the
project's public coverage claims.

2026-10-26 CI repair: the existing content/card/navigation mappings explicitly
consume `PE_Widget` when no material paint applies, preserving Qt's no-op
background primitive instead of reaching the covered-path assertion. Debug
reproductions: `navigationSingleSnapshotRestoreOrder`, `micacard_unit`, and
`galleryThemeSwitchWhileGrantedThenDisable`. No mapping or Verified promotion.
The application lifecycle contract also keeps an uninstalled style's theme and
accent setters from refreshing unrelated application widgets
(`uninstalledStyleDoesNotRefreshApplication`); installed widget owners retain
their existing refresh path.
