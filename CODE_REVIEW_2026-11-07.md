# WinUI3Style code review — resolution

The fixes for original findings #1–#11 are implemented. This is a code-review
resolution, not a claim of complete live WinUI parity; `spec/coverage.md` records
remaining evidence requirements.

## 10. Animation-midpoint checks

`setAnimationTime` in `tests/winui3testhelpers.h` pauses active test animations,
seeks their timeline, and resumes them. SettingsCard, stock SettingsExpander,
ToggleSwitch, CheckBox, GroupBox and Slider midpoint checks no longer depend on
fixed wall-clock sleeps. Endpoint checks still exercise live timer completion;
SettingsCard reversal also checks continuity, duration and easing.

## Integration checks

The style-painted SettingsCard chevron tests now inspect rendered glyph frames
rather than obsolete QLabel pixmaps. Stock expansion animations retire on
completion and stop on application or per-widget style replacement. Clear-button
press semantics, open-menu density sizing, and Standard/Compact slider
pointer-value mapping have regression coverage. Only the four visually reviewed
slider baseline images changed.

## Validation (2026-11-13)

- Qt 6.12 Release: all 70 non-native CTests pass, including the pixel-exact
  two-run snapshot matrix; Debug motion suites pass ten consecutive runs each.
- Qt 6.11: all nine affected unit suites and the pixel-exact snapshot matrix
  pass. Qt 6.11 and 6.12 build/install/static-consumer contracts pass. Qt 5 is not
  installed locally; its dependency-template contract passes, but compilation
  remains a CI check.
- The rebuilt Qt Gallery's native Light/Dark × Standard/Compact input sequence
  passes at DPR 1 with animations enabled and identical source/Gallery DLL
  hashes. This is local Qt evidence, not pinned official Gallery parity.
- All six native CTests pass; individual foreground/compositor skips remain
  environment limitations, not parity evidence.
- Repository-wide clang-format 18.1.8 and source contracts pass.
