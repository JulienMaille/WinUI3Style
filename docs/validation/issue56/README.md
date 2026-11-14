# Issue 56: Settings container validation

## Scope and mapping

Issue: https://github.com/JulienMaille/WinUI3Style/issues/56

The existing stock QFrame/expansion QWidget composition remains in use. QStyle owns the separator, chevron, input state and expansion animation. The compiled SettingsCard helper uses the same content-margin and separator contract. The layout maps one DefaultSettingsExpanderItemStyle item; arbitrary multi-row content and ItemsHeader/ItemsFooter do not claim individual item-template coverage.

The icon starts at border 1 + padding 16 + leading margin 2 = 19 px. The icon occupies a 20 px label with 2 px leading and 20 px trailing margins. A 42 px lane must not center the label in the unused trailing space. The Gallery now mirrors the physical margins in RTL.

The expanded content combines `SettingsExpanderItemPadding` (58,8,44,8), `ExpanderContentDownBorderThickness` (1,0,1,1), item border (0,1,0,0), and `ContentPresenter.Margin` (0,-2,0,0). The effective margins are 59,7,45,9, mirrored horizontally in RTL. The header bottom border remains visible as a one-pixel separator. The item adds its own 1 px top border and MinHeight 52; after presenter overlap and content bottom border, the expansion band has minimum height 51. Header padding also includes the outer 1 px border, so header and content text align at x59.

`ExpanderContentBorderBrush` maps to `CardStrokeColorDefault`: black alpha 15 in Light and black alpha 25 in Dark. This is not the white-alpha-18 Dark `ControlStrokeColorDefault`, nor the flyout stroke. `ExpanderChevronForeground` maps to primary text in normal, hover and pressed states; disabled uses disabled text.

## Exact primary resources

- https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/CommonStyles/Common_themeresources_any.xaml
- https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/Expander/Expander_themeresources.xaml
- https://github.com/CommunityToolkit/Windows/blob/a2fa47ad72cb95ed81f3ff0885da627a5fd6c14e/components/SettingsControls/src/SettingsCard/SettingsCard.xaml
- https://github.com/CommunityToolkit/Windows/blob/a2fa47ad72cb95ed81f3ff0885da627a5fd6c14e/components/SettingsControls/src/SettingsExpander/SettingsExpander.xaml

The Toolkit sources replace the previous floating `main` citation with an immutable revision. The issue image shows WinUI Gallery 3.0.4.0; the repository manifest pins Gallery 2.9.3.0. The values above come from the exact resource files, not a guessed screenshot measurement.

## Authentic snapshots

`light-before-after.png` and `dark-before-after.png` contain expanded Gallery card crops for Standard and Compact density, before and after the repair. BEFORE uses unchanged origin/main `32b078eeae174dfbee589841502f26671cab7a5f`. AFTER uses this PR's source.

The capture driver instantiates the real GalleryWindow sources and Designer form. It opens the actual Settings page and sends separate mouse move/down/up events to the expansion header. It also replays keyboard expansion and collapse. It captures the native XCB screen with `QScreen::grabWindow(0, ...)`, not mock HTML or a recreated control. All images use Qt 6.8.3, Linux XCB, an isolated Xvnc display, an 1180×780 Gallery window, DPR 1, no backdrop, and settled rest state. The system font differs from Windows Segoe.

The panels add captions only. No image is resized or recolored. The RGBA canvas and alpha composition preserve transparency. BEFORE content margins are 58,8,44,8; AFTER margins are 59,7,45,9. The text changes only because the old sample incorrectly claimed that expansion has no border.

The supplied Windows reference image and pinned XAML support the contract. A live side-by-side Windows Gallery comparison is **not verified** in this Linux session. Coverage remains partial.

## Reproduce and test

The mechanism tests drive the real Gallery composition in Light/Dark × Standard/Compact × LTR/RTL. They assert icon geometry and visible ink, exact content margins, the separator's alpha-composited pixel, and the chevron's primary/disabled-text raster. Compiled-card tests assert full-width expansion, content margins, and exact separator pixels in both themes and densities.

The first run against baseline failed all 32 Gallery cases and all four initial compiled geometry cases. The compiled matrix subsequently added Dark rows. Existing settings-card interaction tests cover expansion, reversal, keyboard input, trailing controls, style replacement and lifecycle behavior.

An unmodified strict GCC baseline fails at the unused `foreground` local in `SettingsCard::refreshChevronState`. This PR does not include the PR60 cleanup. Until PR60 merges, the local build retains warnings-as-errors with the single documented `-Wno-error=unused-but-set-variable` exception.

## UI brief

Reuse the existing stock labels, layouts, card composition and style paint helpers. Add no widget type, QSS, or widget-local Fluent paint.

Applicable states: collapsed/expanded, hover, pressed, keyboard focus and Space, disabled, RTL, long content, animation and reversal, runtime themes, and both densities. Loading, empty, error/retry, permissions, network success and destructive confirmation do not apply to this static settings demonstration.

Not changed: unrelated card surface/corner styling, QToolBox, issue59 tooltips, PR60/61 behavior, and Windows animation preference.
