# REFACTOR_PLAN.md — WinUI3Style: PoC → production-ready QStyle

## 1. Audit findings (verified on tree @ `778dd4f`, Windows, Qt 5.12+/6.5+)

### 1.1 Architecture: a facade + per-domain renderers (already sound)

- `Style` (`include/winui3style/winui3style.h:46`, `src/winui3style.cpp:3153` non-blank
  lines) is a thin `QProxyStyle` facade: `drawPrimitive` (`:2028`), `drawControl`
  (`:2237`) and `drawComplexControl` (`:2514`) dispatch to per-domain `draw*`
  functions returning bool; metrics/geometry/hints/icons delegate 1:1 to
  `winui3style_contracts_p.cpp` (`pixelMetric :79`, `sizeFromContents :92`,
  `subElementRect :380`, `subControlRect :477`, `styleHint :491`, `standardIcon
  :532`); `hitTestComplexControl` delegates to `winui3geometry_p.*`.
  Fallback for uncovered elements is explicit: `Q_ASSERT_X(!covered*)` in debug
  (`winui3style.cpp:2232,2509,2525`) then `QProxyStyle` → `QCommonStyle`.
- Layers already exist and match the mission's target organization:
  - **Theme/tokens**: `winui3tokens_p.h` (327 lines: `Tokens` struct, durations,
    radii, `veiledCard`/`cardRefillTone`, `buildTokens`); `winui3theme_p.*`
    (system probes + `standardPalette`); source contract
    (`tests/check_source_contracts.cmake:35-54`) already gates `QColor(`
    numerics to `winui3tokens_p.h`/`winui3paint_p.cpp`/`winui3geometry_p.cpp`/
    `winui3theme_p.cpp`.
  - **Metrics/geometry**: `winui3density_p.h` (301 lines: Standard/Compact
    tables), `winui3geometry_p.*` (null-option contract), `winui3style_contracts_p.*`.
  - **Paint primitives**: `winui3paint_p.*` (rounded rects, focus ring, grayscale
    text, chevron, snapping) — real shared home with contracts, not dead code.
  - **Helpers**: `winui3helpers_p.h` (323 lines: widget classification, backdrop
    erase recipes 1/2/3, `paintsDirectlyOnBackdrop` gate).
  - **Lifecycle**: `winui3surfaces_p.*` (backdrop/popup/dialog surfaces),
    `winui3interactions_p.*` (event filter → animation driver),
    `winui3animations_p.*` + `winui3frameproperties_p.*` (frame registry),
    `winui3backdrop.cpp` (DWM), `winui3appearancewatcher_p.*` (native theme
    notifications), `winui3icons.cpp` (Fluent glyphs), `winui3tableeditors_p.*`.
  - **Compound widgets** (`include/`: `NavigationView`, `SettingsCard`,
    `AnimatedStack`, `ToggleSwitch` thin wrapper) follow METHODOLOGY §1-kind-4
    (delegate chrome to the style).
- **Conclusion: no architectural rewrite is needed.** The target architecture is
  the current one, cleaned up. New files/classes only where a function exceeds
  ~200 lines with separable per-element branches.

### 1.2 Real problems (ranked by risk × payoff)

1. **God renderers**: `drawButtonPrimitive` (`winui3buttons_p.cpp:85-706`,
   ~620 lines), `drawViewControl` (`winui3viewrenderers_p.cpp:385-746`, ~360),
   `drawViewPrimitive` (`:124-385`, ~260), `drawMenuControl`
   (`winui3menus_p.cpp:141-355`, ~215). One function per element-kind each
   handling 5–10 element types with deep `if (element == …)` chains. Risk:
   a fix for one element breaks a sibling; reviewers must read 600 lines to
   change a button.
2. **Residual facade logic in `winui3style.cpp`**: `drawPrimitive` keeps
   calendar-lane, card-viewport, card-interior and popup-tint branches
   (`:2042-2137`, ~95 lines); `drawControl` keeps button-erase, SettingsCard
   frame, ComboBox label, ProgressBar groove/contents/label (`:2240-2456+`).
   These duplicate renderer-owned concerns and bypass the `draw*` delegation.
3. **`winui3surfaces_p.cpp` (1698 lines) mixes 4 responsibilities**: backdrop
   sync (chrome/islands/scroll-chain/container links), popup surfaces
   (`preparePopupSurface`, ~300 lines), ContentDialog state + footer widget,
   LineEdit helpers + slider tooltip. Its header embeds a ~50-line
   `guardIslandScrollArea` lambda.
4. **Snapshot/property-key sprawl** (`_winui_*` keys in
   `winui3style_properties_p.h` + backdrop/surface/nav keys): partially cleaned
   (commits `4577464`, `b3761bf`), but polish/unpolish still save overlapping
   state (autofill/opaque/hover/role/margins) in `Style::polish`/`unpolish`
   (`winui3style.cpp:2613-3255`) alongside the backdrop-surface keys.
5. **Qt-version surface**: Qt 5.12+ and Qt 6.5+ supported
   (`CMakeLists.txt:25-33`); shims centralized in `winui3qtcompat_p.h`, but
   ~15 scattered `#if QT_VERSION` / `#ifdef Q_OS_WIN` sites remain in
   `winui3style.cpp`, `winui3surfaces_p.cpp`, `winui3interactions_p.cpp`,
   `winui3menus_p.cpp`, `winui3icons.cpp`, `settingscard.cpp`.
6. **Test binaries at the 60 KB ratchet**: `tst_winui3buttons/surfaces/combobox/
   views/micabackdrop/interaction.cpp` all 57–61 KB
   (`tests/check_source_contracts.cmake:78-87` fails the build above 61440 B).
   No room for new mechanism tests in the biggest binaries.

### 1.3 What is NOT a problem (do not touch)

- Token centralization, QSS ban, license headers, `Q_NAMESPACE_EXPORT` guard:
  enforced by `check_source_contracts.cmake`; green.
- Paint primitives, density tables, geometry null-contracts, frame registry,
  icon provider: cohesive, tested, documented.
- Snapshot matrix (70 baselines, strict RGBA, double-run determinism) and the
  4-level validation policy (`spec/METHODOLOGY.md` §5): the non-regression
  harness already exists — Phase 2 is about baselining, not building it.
- Public API (`include/winui3style/*.h`): stable, no changes planned.

## 2. Target architecture (the current one, enforced)

```
Style (facade, thin) ──delegates──▶ per-domain draw*() ──use──▶ paint/geometry/token helpers
  │ drawPrimitive/Control/ComplexControl   │ buttons/menus/views/complex/editors/range/toolgroup
  │ metrics/geometry/hints/icons           │ contracts_p → density/geometry
  │ polish/unpolish/eventFilter            │ surfaces / interactions / animations+frameprops
  └─ theme/appearance                      └─ theme_p / appearancewatcher / tokens_p (values only)
```

Rules: `Style::draw*` = dispatch only (no element branches); one function per
element-kind, ≤ ~150 lines, delegating shared sub-steps to `PaintPrivate`;
tokens only in `winui3tokens_p.h` (+3 grandfathered homes); backdrop recipes
only via `winui3helpers_p.h`; no new `QStyle` subclasses, no QSS, no new deps.

## 3. Migration steps (small commits, each green on full CTest + snapshot matrix)

- **S0 — Baseline (Phase 2, no code change)**: tag `refactor-baseline`;
  record Qt version, OS build, DPI, fonts; run full CTest + snapshot matrix
  twice; archive `snapshot-report.json` + hashes as the reference.
  - *Measured 2026-10-13 (station, `build-simplify`, Qt 6.12 x86, Release)*:
    CTest 51/52 — only `winui3style_surfaces_unit` fails on the two known
    wizard cases (`wizardSurfaceContract`, `wizardOpenThemeSwitchLifecycle`,
    pre-existing Qt-version palette drift, unrelated to refactoring).
  - *Snapshot matrix: BLOCKED in this environment.* `build/` (Qt 6.9.2) is
    stale — its Qt tree was removed, CMake re-configure fails on the Qt5
    fallback. `build-gallery` (Qt 6.12 x64) builds, but
    `winui3style_gallery --capture-dir` hangs after 3 files (menu captures)
    under `QT_QPA_PLATFORM=offscreen` here, including bare `--help` with no
    output. Last good `build/snapshot-report.json` is dated 2026-10-02
    (Qt 6.9.2 era). **No refactor step may claim visual non-regression until
    the matrix runs green — on the reviewer's machine or CI.**
- **S1 — Facade thinning**: move `winui3style.cpp:2042-2137` (calendar lane,
  card viewport/interior, popup tint) into `winui3viewrenderers_p.cpp` /
  `winui3surfaces_p.cpp` helpers; move `drawControl` element branches
  (SettingsCard frame, ComboBox label, ProgressBar) into their renderers.
  Gate: `coveredPrimitive/coveredControl` asserts unchanged, matrix identical.
- **S2 — Split god renderers** (one commit each): `drawButtonPrimitive` →
  per-element functions (button/tool/check/radio/lineedit/branch); same for
  `drawViewPrimitive`/`drawViewControl`/`drawMenuControl`. No logic change;
  gate: matrix identical + domain suites green.
- **S3 — Surfaces split**: extract popup-surface (`preparePopupSurface` +
  mask/corner helpers) and dialog-state (incl. footer widget) from
  `winui3surfaces_p.cpp` into two new TUs; move `guardIslandScrollArea` body
  to the .cpp. Gate: backdrop/nav/popup suites + matrix.
- **S4 — Polish/unpolish audit**: unify overlapping `remember` keys between
  `Style::polish/unpolish` and backdrop-surface snapshots; add symmetry test
  (polish→unpolish restores every saved attribute). Gate: lifecycle suites.
  - *Measured 2026-10-17 (`build-simplify`, Qt 6.12 x86, offscreen probe of
    three orders — polish→arm(show)→disarm(hide)→unpolish, polish→arm→unpolish
    while armed, popup arm/disarm): the overlapping `_winui_original_*` keys are
    NOT a defect.* Every order has exactly one restorer, every saved attribute
    returns to its pre-polish value, and no saved key outlives the `unpolish`.
    `remember()` is write-if-absent, so a later arm silently adopts the value
    polish already saved — the same value, since both read the same attribute.
    Only the *namespace* is shared, not the values. The keys were therefore left
    as they are, and the symmetry gate (`surfaceSnapshotOwnershipSymmetry`,
    `tests/tst_winui3interaction.cpp`) now pins those three orders so a future
    unification of the mechanism cannot move restore timing unnoticed. Extending
    the unification itself is cosmetic until a failing order is found.
- **S5 — Qt-version hygiene**: route remaining scattered `#if`s through
  `winui3qtcompat_p.h` where mechanical; document the intentional DWM ones.
  Gate: build with `WINUI3STYLE_WARNINGS_AS_ERRORS=ON` + (if available) Qt5 CI.
- **S6 — Test headroom**: pre-split the next domain binary before it hits
  60 KB (buttons first); no test-logic change.

## 4. Risks & explicit limits

- **R1 — Silent pixel drift**: mitigated by the strict snapshot matrix on every
  step; any diff is investigated by bounds before acceptance.
- **R2 — Popup/backdrop paths are timing-sensitive** (DWM grants, WinIdChange
  re-entry, queued re-applies): S3 keeps recipe code verbatim; native suite
  (`winui3style_native`, `micanative`) is the gate, and refusals are
  skipped/blocked evidence, never green-washed.
- **R3 — Qt5 build**: this station builds Qt 6 only; Qt5-compat changes (S5)
  are compile-guarded but **cannot be verified here** — stated explicitly per
  change, CI (`.github/`) is the gate.
- **R4 — Live side-by-side (§5-level-4)**: cannot be automated here; each step
  ends with “live re-check required” for touched controls, reviewer-owned.
- **Out of scope**: new controls, token-value changes, mapping changes
  (`coverage.md`), public API changes, line-count targets.

## 5. Success check (maps to the mission's criterion)

A Qt developer can: find any color in `winui3tokens_p.h`, any metric in
`winui3density_p.h`, any paint path via `Style::draw*` → one
`draw<Element>` function ≤ ~150 lines; each change is covered by a mechanism
test + the snapshot matrix. Verified by: full CTest green (modulo the two
known wizard baseline failures), matrix byte-identical to S0, `coverage.md`
rows un-reopened.
