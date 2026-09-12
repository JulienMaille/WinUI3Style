# WinUI3Style — feuille de route

> État : 12 septembre 2026, `main` @ `d8d26d1`. Suite 41/41 verte (natif exclu) ; Release `/WX` + Debug `/W4` propres ; gates `source_contracts` et `designer_gallery` vertes.
> `spec/coverage.md` reste « source-audited » partout : comparaison WinUI live obligatoire avant tout « verified ».

## P0 — garde-fous

- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).

## P1 — navigation, champs, états

- [ ] NavigationView : revalidation métriques WinUI + contraste hover/pressed/selected trois thèmes (comportement testé : `navigationTransition`, `navigationInteractiveFrames`, `renderCommonStates`).
- [ ] AutoSuggestBox : clavier, hit-test, stratégie backdrop (couvert : sous-chaînes, palette popup, thème ouvert).
- [ ] Démos Compact galerie manquantes : NavigationView, MenuBar (le reste ajouté ; métriques+tests OK via `OfficialCompactWidgets`).

## P2 — architecture, accessibilité, robustesse, tests

- [ ] Remplacer `WinUI3::SettingsCard` promu par `QFrame` + `winuiSettingsCard=true` (`demo/gallerywindow.ui:119-139`, `check_designer_gallery.cmake:27-44` verrouille le promu).
- [ ] Live keyboard : focus/Tab/Space/Enter/popups (offscreen partiel via `inputModalityFocus`).
- [ ] Contrastes Light/Dark (pas de test de ratio dédié) + `prefers-reduced-motion` OS (seul `WINUI3STYLE_DISABLE_ANIMATIONS` existe).
- [ ] `CHANGELOG.md` : tenu à jour (`86c063b`) + matrice compat (`spec/compatibility.md`) ; à maintenir à chaque release.

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
