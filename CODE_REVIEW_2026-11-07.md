# WinUI3Style code review — 2026-11-07

Remaining findings from the original read-only review. Findings #1, #2, #3, #4,
#5, #6, #8, #9, and #11 have been fixed and removed. Original finding numbers
are retained for cross-reference; gaps are intentional.

Current verification: Qt 6.12 Debug builds and CTests passed for the icons,
buttons, animated stack, and SettingsCard targets; the package-config and
covered-elements contracts passed. Native/compositor visual verification still
requires an interactive desktop.

---

## 7. Medium — icon paint hot path allocates QString keys and thrashes under animated tint

- Evidence: `src/winui3icons.cpp:544` builds an 11-field QString key per lookup
  (chained `.arg()` + `QString::number(dpr,'g',17)` + hex `colorKey`);
  `:576` rebuilds `iconName()` on every `QIcon::name()` call; `:744` does
  name parse + cache-key-to-hex + pixmapKey before the QHash lookup. Reached per
  paint via `src/winui3paint_p.cpp:39` (`paintThemedIcon`). Any animated
  foreground changes `colorKey` each frame → cache miss → full glyph
  rasterization + `CompositionMode_SourceIn` fill + QCache eviction churn.
- Measured (small-sample offscreen benchmark, Qt 6.12 Release): tree p50
  `2.64 ms` with icons vs `1.51 ms` without; list `1.39 ms` vs `1.10 ms`;
  rich surface `8.17 ms` vs `6.70 ms`. Tokens cached `~70 ns` vs uncached
  `~330 ns`.
- Fix: key the cache on a numeric POD struct (glyph, variant/cacheKey, sizes,
  dpr, rgba64, mode, state) with `qHash`, eliminating key QStrings; memoize
  `iconName()` (the engine's `m_icon` is immutable).
- Status: fixed in `src/winui3icons.cpp`: pixmap and colored-icon caches now use
  numeric value keys, and each icon engine builds its stable name once. The
  existing cache tests plus a different-foreground cache-separation assertion
  cover rendering and key dimensions. Qt 6.12 Debug build and icon/button CTests
  passed; the original Release benchmark was not rerun.

## 10. Medium — duplicated animation-midpoint test logic is load-flaky

- Evidence: observed failures under load at `tests/tst_winui3menus.cpp:776` and
  `tests/tst_winui3settingscard.cpp:191`; both passed in isolation. The robust
  pattern already exists at `tests/tst_winui3interaction.cpp:757` (wait for the
  first driver tick instead of a fixed wall-clock sleep), but fixed
  `qWait(35/55)` midpoint checks remain in menus, settings card, and toggles
  (`tests/tst_winui3toggles.cpp:188`, `:371`).
- Fix: one shared midpoint helper in `tests/winui3testhelpers.h`, preferably
  driven deterministically rather than by wall-clock sleeps.

---

## Cleared suspicions (verified, not reported)

RTL sliders (Qt encodes RTL in `upsideDown`; style honors it), Compact
toggle-drag (track metrics identical in both densities), combo press/grab/
release handling, menu popup animation reopen, FrameAnimationDriver
re-entrancy/lifetime, dialog scrim cross-destruction, theme watcher wiring,
palette cache invalidation, FramePropertyRegistry bounds/GUI-thread guards,
appearance watcher lifetime/debounce, popup rearm retry bounds, backdrop erase
recipes and scrim teardown, plugin ownership.
