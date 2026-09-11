# WinUI3Style — feuille de route

> État de référence : 11 septembre 2026, `main` @ `948e018` (contrats chrome shell + dialog persistant au switch de thème).
> Suite : 41/41 verte (natif exclu) ; `inputModalityFocus` fixé en `611cfb2`, split combo en `b696887`.
> Gates `winui3style_source_contracts` et `winui3style_designer_gallery_contract` vertes.
> Boucle de travail : chaque lot met à jour ce fichier, est relu, commité sur `main` puis poussé (workflow actuel : `main` direct).
> Les cases cochées correspondent à des éléments présents dans le dépôt ; elles ne remplacent pas une validation visuelle WinUI en direct (`spec/coverage.md` reste « source-audited » partout, comparaison live obligatoire).

## Fermés récemment (trace, ne pas rouvrir sans régression)

- Issues [#6](https://github.com/JulienMaille/WinUI3Style/issues/6) (slide vertical 12 px + OutCubic, déviation délibérée documentée `spec/coverage.md:27`) et [#5](https://github.com/JulienMaille/WinUI3Style/issues/5) (lignes 36 px Compact conformes, effet Compact = MenuBar) — 2026-09-10.
- Issue [#4](https://github.com/JulienMaille/WinUI3Style/issues/4) : gate `QColor([0-9]` zéro exemption (`tests/check_source_contracts.cmake:29-50`, homes `winui3tokens_p.h`/`winui3paint_p.cpp`/`winui3geometry_p.cpp`/`winui3theme_p.cpp`).
- Ghosting Mica/Acrylic + hover-vanish (`IntersectClip`, `materialEraseRespectsDirtyRegionClip`), branches `codex/treeview-wizard-rebase` et `draft/mica-ghost-glyphs` mergées et supprimées.
- Wizard/dialogs Dark, deux surfaces, centrage (`wizardSurfaceContract`, `wizardUsesModernStyleHint`, `wizardOpenThemeSwitchLifecycle`, baselines `light/dark-wizard.png`).
- Combo texte fermé élidé + chrome icône 16+8 (`comboClosedTextContracts`, `themeComboSizingContract`), spin affixe (`spinBoxPrefixSuffixSizingContract`), toggle toolbar mica (`checkedToolbarToggleKeepsFillOverBackdrop`, `fa28570`).
- `tst_winui3editors.cpp` 60 199 B → `editors` 33 411 B + `tst_winui3combobox.cpp` 30 095 B (`b696887`, `winui3style_combobox_unit`) ; `interaction` 58 967 B et `views` 58 106 B restent sous 61 440 B.
- Repaint thème sans bug (`948e018`, contrats seuls) : `themeSwitchRebasesChromeShell` (fenêtre+menu+toolbar+status), `persistentDialogSurvivesThemeSwitch` (contenu+footer Light→Dark→Light) ; popups/wizard persistants déjà couverts. Les 2 fail-first n'étaient pas des bugs (statusbar=`popupSurfaceColor`, footer=fill commande).

## Issues GitHub encore ouvertes

- [ ] **P2 — [#2 Promote warnings to errors](https://github.com/JulienMaille/WinUI3Style/issues/2)** — `WINUI3STYLE_WARNINGS_AS_ERRORS` limité à la lib (`src/CMakeLists.txt:87-90`, ON en CI `ci.yml:105`) ; reste : autres cibles/compilateurs, build Debug `/W4` complet.
- [ ] **P2 — [#1 Split `drawButtonControl` per element](https://github.com/JulienMaille/WinUI3Style/issues/1)** — `src/winui3buttons_p.cpp:381` monolithique (QPushButton/QToolButton/CommandLink/checkable/disabled) ; prévoir non-régression par élément × thème/densité.
- [ ] **P1 — [#5 (suivi) relayout popup ouvert au switch de densité** — le verdict by-design est clos ; le repositionnement d'un popup déjà ouvert reste à reproduire séparément.

## P0 — garde-fous

- [ ] **Run CI GitHub Actions vert à lier** — `.github/workflows/ci.yml` existe (matrice Qt 6.8.3/6.11.1 × Debug/Release + job Qt5.12-MinGW, CTest, snapshots, smoke) mais aucune exécution distante n'est liée ici : ouvrir `https://github.com/JulienMaille/WinUI3Style/actions`, vérifier le run du push `b696887`, coller son URL dans cette ligne ; si rouge, diagnostiquer depuis les artefacts (`snapshot-debug-*`, `ctest-verbose.log`).
- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).

## P1 — backdrop, repaint et surfaces

- [ ] Stratégie backdrop unique AutoSuggestBox/ComboBox/menus/popups (aujourd'hui : `eraseForBackdrop` par contrôle, pas de comparaison unifiée).
- [ ] Backdrop resize/occlusion/déplacement (toggle on/off + scroll couverts : `backdropLifecycleContract`, `backdropToggleOffRestoresShellSurfaces`, `islandScrollPostsFullViewportRepaint`).

## P1 — navigation, champs, états

- [ ] NavigationView : revalidation métriques WinUI + contraste hover/pressed/selected trois thèmes (comportement testé : `navigationTransition`, `navigationInteractiveFrames`, `renderCommonStates`).
- [ ] AutoSuggestBox : clavier, hit-test, stratégie backdrop (couvert : sous-chaînes, palette popup, thème ouvert).
- [ ] Démos Compact galerie manquantes : PasswordBox, AutoSuggestBox, TimePicker, ListView, TreeView, NavigationView, MenuBar (métriques+tests OK pour les 10 via `OfficialCompactWidgets` ; `.ui` n'a que TextBox/ComboBox/DatePicker/CheckBox/Radio/NumberBox).

## P2 — architecture, accessibilité, robustesse, tests

- [ ] Remplacer `WinUI3::SettingsCard` promu par `QFrame` + `winuiSettingsCard=true` (`demo/gallerywindow.ui:119-139`, `check_designer_gallery.cmake:27-44` verrouille le promu).
- [ ] Focus/Tab/Space/Enter/popups : passer le live keyboard (offscreen partiel, `inputModalityFocus` flake).
- [ ] Contrastes Light/Dark (pas de test de ratio dédié) + `prefers-reduced-motion` OS (seul `WINUI3STYLE_DISABLE_ANIMATIONS` existe).
- [ ] Coût polish/drawControl/animations sur grandes listes + audit allocs `QPixmap`/`QIcon`/`QPainterPath` chemins chauds (bench `benchmarks/render_benchmark.cpp:48` existe, pas de verdict).
- [ ] Restaurer les tests d'activation au relâchement + marker click-and-hold des ComboBox.
- [ ] Assertions géométrie restantes : séparateurs, insets de popup.
- [ ] `CHANGELOG.md` (projet 0.1.0 + `RELEASE_NOTES.md` existent) + page matrice compat Windows/Qt (manifest `spec/winui-2.4/manifest.json` + CI Qt 6.8.3/6.11.1 + Qt5.12-mingw : builds OK, page publiée manquante).

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Ordre recommandé

1. ~~Issues #6/#5~~ Fait 2026-09-10. Reste le suivi relayout #5 ci-dessus.
2. Flake `inputModalityFocus` + ratchet 60 KB (débloque la gate), puis run CI vert lié.
3. Repaint thème complet + resize/occlusion backdrop + dialogs persistants.
4. NavigationView métriques/contrastes + AutoSuggestBox clavier/hit-test + démos Compact manquantes.
5. `drawButtonControl` split (#1) + `/WX` étendu (#2).
6. QWidget restants, `CHANGELOG.md`, matrice compat publiée.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
