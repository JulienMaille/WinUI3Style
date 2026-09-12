# WinUI3Style — feuille de route

> État : 12 septembre 2026, `main` @ `e683d53`. Suite 41/41 verte (natif exclu) ; Release `/WX` + Debug `/W4` propres ; gates `source_contracts` et `designer_gallery` vertes.
> `spec/coverage.md` reste « source-audited » partout : comparaison WinUI live obligatoire avant tout « verified ».

## Issues GitHub ouvertes

- [x] **P2 — [#2 Promote warnings to errors](https://github.com/JulienMaille/WinUI3Style/issues/2)** — clos en `e683d53` : `winui3style_warnings` INTERFACE (`/W4`+`/WX`, `-Wall -Wextra -Wpedantic`+`-Werror`) routée lib/plugin/gallery/tests/benchmark ; 13 `QMouseEvent` Qt6.9 migrés + `[[maybe_unused]]` helpers.
- [x] **P2 — [#1 Split `drawButtonControl` per element](https://github.com/JulienMaille/WinUI3Style/issues/1)** — clos en `90d54b7` : 5 helpers `static` + dispatcher `switch`, corps à l'identique, garde `buttonControlOwnershipByElement` (5 éléments × Light/Dark × Standard/Compact).

## P0 — garde-fous

- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).

## P1 — backdrop et surfaces

- [x] Stratégie backdrop unique — clos en `897b01e` : politique 3 recettes dans `winui3helpers_p.h` (gate partagée, `clearForBackdropFill` pour pills acrylic), `winui3menus_p.cpp` routé, doc croisée `winui3surfaces_p.h` + `coverage.md:37`.

## P1 — navigation, champs, états

- [ ] NavigationView : revalidation métriques WinUI + contraste hover/pressed/selected trois thèmes (comportement testé : `navigationTransition`, `navigationInteractiveFrames`, `renderCommonStates`).
- [ ] AutoSuggestBox : clavier, hit-test, stratégie backdrop (couvert : sous-chaînes, palette popup, thème ouvert).
- [ ] Démos Compact galerie manquantes : NavigationView, MenuBar (`ed0987c` : PasswordBox, AutoSuggestBox, TimePicker, ListView, TreeView ajoutés ; métriques+tests OK via `OfficialCompactWidgets`).

## P2 — architecture, accessibilité, robustesse, tests

- [ ] Remplacer `WinUI3::SettingsCard` promu par `QFrame` + `winuiSettingsCard=true` (`demo/gallerywindow.ui:119-139`, `check_designer_gallery.cmake:27-44` verrouille le promu).
- [ ] Live keyboard : focus/Tab/Space/Enter/popups (offscreen partiel via `inputModalityFocus`).
- [ ] Contrastes Light/Dark (pas de test de ratio dédié) + `prefers-reduced-motion` OS (seul `WINUI3STYLE_DISABLE_ANIMATIONS` existe).
- [ ] Coût polish/drawControl/animations sur grandes listes + audit allocs `QPixmap`/`QIcon`/`QPainterPath` chemins chauds (bench `benchmarks/render_benchmark.cpp:48` existe, pas de verdict).
- [ ] Restaurer les tests d'activation au relâchement + marker click-and-hold des ComboBox.
- [x] Assertions géométrie : séparateurs + insets popup (`e683d53`, `menuSeparatorGeometryContract` : slot 7 px les 2 densités, ligne 12 px de chaque bord).
- [ ] `CHANGELOG.md` (projet 0.1.0 + `RELEASE_NOTES.md` existent) + page matrice compat Windows/Qt (manifest `spec/winui-2.4/manifest.json`, CI Qt 6.8.3/6.11.1 + Qt5.12-mingw).

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
