# WinUI3Style — feuille de route

> État : 11 septembre 2026, `main` @ `91cbd67`. Suite 41/41 verte (natif exclu) ; gates `source_contracts` et `designer_gallery` vertes.
> `spec/coverage.md` reste « source-audited » partout : comparaison WinUI live obligatoire avant tout « verified ».

## Issues GitHub ouvertes

- [ ] **P2 — [#2 Promote warnings to errors](https://github.com/JulienMaille/WinUI3Style/issues/2)** — `WINUI3STYLE_WARNINGS_AS_ERRORS` limité à la lib (`src/CMakeLists.txt:87-90`, ON en CI) ; reste : autres cibles/compilateurs, build Debug `/W4` complet.
- [ ] **P2 — [#1 Split `drawButtonControl` per element](https://github.com/JulienMaille/WinUI3Style/issues/1)** — `src/winui3buttons_p.cpp:381` monolithique (QPushButton/QToolButton/CommandLink/checkable/disabled) ; prévoir non-régression par élément × thème/densité.

## P0 — garde-fous

- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).

## P1 — backdrop et surfaces

- [ ] Stratégie backdrop unique AutoSuggestBox/ComboBox/menus/popups (aujourd'hui : `eraseForBackdrop` par contrôle, pas de comparaison unifiée).

## P1 — navigation, champs, états

- [ ] NavigationView : revalidation métriques WinUI + contraste hover/pressed/selected trois thèmes (comportement testé : `navigationTransition`, `navigationInteractiveFrames`, `renderCommonStates`).
- [ ] AutoSuggestBox : clavier, hit-test, stratégie backdrop (couvert : sous-chaînes, palette popup, thème ouvert).
- [ ] Démos Compact galerie manquantes : PasswordBox, AutoSuggestBox, TimePicker, ListView, TreeView, NavigationView, MenuBar (métriques+tests OK pour les 10 via `OfficialCompactWidgets` ; `.ui` n'a que TextBox/ComboBox/DatePicker/CheckBox/Radio/NumberBox).

## P2 — architecture, accessibilité, robustesse, tests

- [ ] Remplacer `WinUI3::SettingsCard` promu par `QFrame` + `winuiSettingsCard=true` (`demo/gallerywindow.ui:119-139`, `check_designer_gallery.cmake:27-44` verrouille le promu).
- [ ] Live keyboard : focus/Tab/Space/Enter/popups (offscreen partiel via `inputModalityFocus`).
- [ ] Contrastes Light/Dark (pas de test de ratio dédié) + `prefers-reduced-motion` OS (seul `WINUI3STYLE_DISABLE_ANIMATIONS` existe).
- [ ] Coût polish/drawControl/animations sur grandes listes + audit allocs `QPixmap`/`QIcon`/`QPainterPath` chemins chauds (bench `benchmarks/render_benchmark.cpp:48` existe, pas de verdict).
- [ ] Restaurer les tests d'activation au relâchement + marker click-and-hold des ComboBox.
- [ ] Assertions géométrie restantes : séparateurs, insets de popup.
- [ ] `CHANGELOG.md` (projet 0.1.0 + `RELEASE_NOTES.md` existent) + page matrice compat Windows/Qt (manifest `spec/winui-2.4/manifest.json`, CI Qt 6.8.3/6.11.1 + Qt5.12-mingw).

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
