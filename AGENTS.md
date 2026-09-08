# WinUI3Style agent guardrails (routes only — normative text lives in spec/).

- Read `spec/METHODOLOGY.md` first. It is the acceptance policy: mapping-first
  (§1), pinned WinUI evidence (§2), live-input sequence (§3), no QSS or
  per-widget Fluent paint (§4), four validation levels (§5), Verified bar + 
  reproduce-first defect protocol (§6).
- Mapping first: declare the Qt→WinUI mapping in `spec/coverage.md` before
  coding. No silent base-style fallback for covered controls.
- Tokens: `src/winui3tokens_p.h` owns values. Never add `QColor(` numeric
  literals in `src/*.cpp` (gated by `winui3style_source_contracts`).
- No QSS anywhere (`setStyleSheet` fails the build, including `.ui` files).
  No custom widgets: prefer stock widget + `winui*` property; QStyle owns
  visuals, states, animation.
- Helpers before new code: check `winui3paint_p.h`, `winui3geometry_p.h`,
  `winui3helpers_p.h` for an existing primitive (focus ring, chevron,
  snapping, toggle geometry). Copy per-site insets/radii into call args —
  never unify by judgment.
- Tests: every `grab()`/`pixelColor()` assertion pairs with a `QCOMPARE`
  token/geometry assertion on the same state. New geometry tests need
  Compact-parametrized cases (see `tst_winui3density*` patterns).
- Shared test helpers live in `tests/winui3testhelpers.h` — never copy-paste
  `frameReal`/`setFrame`/`colorDistance` per file. No `tst_*.cpp` over 60 KB.
- Defects: reproduce in the running Gallery first, add a failing test for the
  mechanism (not the screenshot), then fix; re-run the input sequence live.
- Regressions reopen the `coverage.md` row (METHODOLOGY §6). Benchmarks stay
  opt-in, never part of the red/green gate.
