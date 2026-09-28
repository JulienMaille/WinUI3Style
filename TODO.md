# WinUI3Style — feuille de route

> État : 28 septembre 2026, `main` @ `33e89d3`. Dernière suite complète : 53/53 CTest + 26/26 binaires de domaine ; gates `source_contracts`, `designer_gallery`, snapshot et natif verts. [Audit NavigationView](spec/NAVIGATIONVIEW_AUDIT_2026-09-28.md) réalisé sans correctif visuel.
> `spec/coverage.md` reste « source-audited » partout : comparaison WinUI live obligatoire avant tout « verified ».

## P0 — garde-fous

- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).
- [ ] Réparer les gates CI clang-tidy (Qt 6.4 installé pour un minimum 6.5) et Qt 5.12 (`Q_NAMESPACE_EXPORT` refusé par moc 5.12), puis publier le rapport/ratchet décrit dans [`spec/REFACTORING_PLAN.md`](spec/REFACTORING_PLAN.md).

## P1 — navigation, champs, états

- [ ] NavigationView : corriger la correspondance des fonds `Selected` / `PointerOverSelected` / `PressedSelected` (Secondary / Tertiary / Secondary dans le XAML épinglé, ordre inversé dans `NavigationItemDelegate::paint()`). Test RED Light/Dark × Standard/Compact, puis validation visuelle par l'utilisateur avant commit. Voir [l'audit](spec/NAVIGATIONVIEW_AUDIT_2026-09-28.md).
- [ ] AutoSuggestBox : clavier, hit-test, stratégie backdrop (couvert : sous-chaînes, palette popup, thème ouvert).
- [ ] Démos Compact galerie manquantes : NavigationView, MenuBar (le reste ajouté ; métriques+tests OK via `OfficialCompactWidgets`).

## P2 — architecture, accessibilité, robustesse, tests

- [ ] NavigationView sans icône : `IconCollapsed` réduit la colonne d'icône officielle à 8 px, alors que le délégué garde un inset de texte fixe de 42 px. Établir la géométrie exacte, test RED puis correction séparée du lot des couleurs sélectionnées.
- [ ] Démo NavigationView : décider si l'on expose une composition complète (retour, repli, pied Settings) ; le mapping QStyle actuel ne couvre que l'item sémantique. Compléter les captures live du focus clavier, press et animation avant tout statut `Verified`.
- [ ] Exécuter le plan d'assainissement par petits lots, tests d'abord : split des tests saturés, renderers/contrats/surfaces, puis réduction symétrique de `polish`/`unpolish`/`eventFilter` ([`spec/REFACTORING_PLAN.md`](spec/REFACTORING_PLAN.md)).
- [ ] Remplacer `WinUI3::SettingsCard` promu par `QFrame` + `winuiSettingsCard=true` (`demo/gallerywindow.ui:119-139`, `check_designer_gallery.cmake:27-44` verrouille le promu).
- [ ] Live keyboard : focus/Tab/Space/Enter/popups (offscreen partiel via `inputModalityFocus`).
- [ ] Contrastes Light/Dark (pas de test de ratio dédié) + `prefers-reduced-motion` OS (seul `WINUI3STYLE_DISABLE_ANIMATIONS` existe).
- [ ] `CHANGELOG.md` : tenu à jour (`86c063b`) + matrice compat (`spec/compatibility.md`) ; à maintenir à chaque release.

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
