# WinUI3Style — showcase post

A post-able write-up for Reddit / Qt Discord / etc. written to be honest and
well-received: no hype, discloses AI assistance up front, leans on verifiable
artifacts (tests, baselines, pinned Microsoft sources) instead of adjectives.
Adjust tone per platform with the notes at the bottom.

---

## Title options

- WinUI3Style: a Windows 11 / WinUI 3 look for Qt Widgets — with zero QSS and zero custom widgets
- We made a QStyle that turns plain Qt Widgets into WinUI 3 controls (light/dark, Mica, Compact density, Fluent icons — no stylesheets, no custom widgets)
- [Showcase] WinUI 3 for Qt Widgets, the way Qt intended: one QStyle, no QSS, no subclassing

---

## Post body

**Hi everyone!**

I'd like to share a project I've been building: **WinUI3Style** — a Qt Widgets style that makes *stock* Qt controls look and behave like Windows 11 / WinUI 3 controls. It's open source:

- **Repo:** https://github.com/JulienMaille/WinUI3Style
- Works with **Qt 5.12 (MinGW) up to Qt 6.11 (MSVC)** — I develop it against SoulseekQt (a real-world Qt 5.12 app) and a demo gallery

### The two rules we set for ourselves

1. **No QSS.** Not a single stylesheet. Everything is done the way `QStyle` was designed: `drawPrimitive`/`drawControl`/`drawComplexControl`, palettes, pixel metrics, sub-element rects, and native DWM backdrop APIs (Mica on main windows, rounded opaque popup surfaces on Windows 11).
2. **No custom widgets.** A ToggleSwitch is just a `QCheckBox` with `winuiToggleSwitch=true`. A NavigationView is an ordinary item view. A ContentDialog is an ordinary `QDialog` with `winuiContentDialog=true`. A settings card is a `QFrame`. Your existing `.ui` files, models and delegates keep working — you can set most variants from Qt Designer's property editor.

As far as we can tell, this is the first time a Qt app can get a full Windows 11 / Fluent look **without stylesheets and without replacing widgets** — one `QStyleFactory::create("winui3")` (or the deployed plugin) is the whole integration.

### What's inside

- Light + dark themes, live system accent color, runtime theme switching
- **Standard and Compact density profiles** (WinUI's Compact Sizing scope: TextBox, ComboBox, DatePicker, ListView, TreeView, MenuBar get the real 24px treatment — inherited per-container, switchable at runtime)
- Animated interaction states (hover, press, focus reveal, checkbox accept, toggle thumb, combo chevron…) with WinUI's official durations and easing
- Segoe Fluent glyphs painted as palette-owned icon masks
- Mica main-window and opaque rounded popup surfaces via DWM
- Designer-driven opt-in variants (`winuiToggleSwitch`, `winuiBackdrop`, `winuiSettingsCard`, `winuiNavigationView`, `winuiDensity`…)
- A demo gallery that links **only stock Qt Widgets* as an executable example of the deployment model

### How we tried to make it trustworthy

I won't pretend the visual work was guessed. The repo contains:

- A **pinned visual specification**: WinUI 3 Gallery captures and the matching `microsoft-ui-xaml` sources from Windows App SDK 2.4 (`spec/winui-2.4/manifest.json`), with a documented capture/compare workflow (`tools/compare_images.py` — RMS, diff-pixel count, bounding box; no auto-resizing)
- **116 test functions across 8 domain binaries plus density/contract suites** (reproduce with `ctest -N` for the target count and per-binary `-functions` output for the function count; counts rot on every test addition so prose numbers are never asserted), including a deterministic offscreen light/dark snapshot matrix (DWM disabled, solid fallback base, pixel-identity against approved baselines), DPI 100–200 geometry contracts, RTL + hit-testing, animation timing, and mutation-restoration tests (every attribute the style touches must be restored on unpolish)
- A **per-control coverage ledger** (`spec/coverage.md`) that says *honestly* what's verified, what's only source-audited, and what's a deliberate Fluent consistency extension (we don't claim a core WinUI DataGrid, for example)

### Full transparency about AI

The bulk of the implementation was written with AI assistance (with me directing, reviewing, and testing every commit). I'm saying this up front because I think that's the honest way to present it — and because the interesting part is the *method*: the AI never "eyeballed" pixels. Every visual decision is traced to the pinned WinUI 2.4 sources, captured baselines, or a failing test. When the first compact-density attempt rendered wrong (text clipped in 24px editors), it was caught by the exact 24px `QCOMPARE` contracts and fixed with proper re-layout invalidation — the kind of bug you only find because the tests demand exact values.

### Try it

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=<your-Qt>
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Then run `build\demo\Release\winui3style_gallery.exe` — or load the style in your own app via `QStyleFactory::create("winui3")`.

**Questions, feedback, and especially bug reports against real WinUI behavior are very welcome.** If you have a favorite control state we haven't covered (the ledger lists the gaps), point me at the WinUI Gallery state and I'll chase it.

---

## Platform notes

- **r/qt** (Reddit): use the title verbatim, keep the body as-is; Reddit rewards honesty + artifacts. Attach 2–3 gallery screenshots (light page, dark page, compact-density page) as an imgur/gallery link — text-only posts underperform. Consider an animated GIF of theme switching + toggle + combo popup, that's what sells "no QSS, no custom widgets".
- **Qt Discord** (#general or #showcase): trim to ~40% length: the two rules, what's inside (5 bullets max), the transparency paragraph, repo link. Lead with "we" and end asking for feedback. Don't paste the build commands in chat, they're in the README.
- **Hacker News (Show HN)**: title as "Show HN: WinUI 3 look for Qt Widgets via a single QStyle — no stylesheets, no custom widgets". Body: lead with the two rules, then transparency, then the trust section (HN is maximally skeptical of AI code — the pinned-spec + full-test-suite story is the hook).
- **Qt bug/interest forums, KDE/Fluent theming Discourses, lobste.rs**: same body, adjust greeting.
- **Mastodon/Bluesky**: 2 short posts — (1) screenshot + "stock QCheckBox with winuiToggleSwitch=true, zero QSS", (2) repo link + the AI transparency line.

## Suggested media to attach

From `spec/baselines/gallery/` (or fresh `--capture-dir` captures):

1. `light-page-0.png` + `dark-page-0.png` (side by side)
2. `light-density-compact-page-*.png` (the 24px editors)
3. A short GIF: toggle switch click, combo popup open, theme flip — recorded off the live gallery
