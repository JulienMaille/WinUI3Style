# WinUI3Style — feuille de route

> État local au 2 octobre 2026 : galerie validée par l'utilisateur et changements regroupés par problème. Release, 53/53 CTest et 23/23 exécutables de domaine non natifs verts lors de la relance sur le bureau interactif. L'échec précédent de `backdropDisableRestoresOpaqueContent` concernait sa capture initiale avant Mica, avec focus refusé et fond d'écran seul ; ses assertions sont restées intactes. Le test AutoSuggest natif opt-in de capture PrintWindow passait Light/Dark ; son sous-test clavier s'abstient quand Windows refuse le focus OS. Voir [l'audit NavigationView](spec/NAVIGATIONVIEW_AUDIT_2026-09-28.md).
> Les contrôles restent « source-audited » ou « partial » dans `spec/coverage.md` : aucune promotion à « verified » sans la comparaison WinUI live complète.

## Premier PoC publiable — validation locale du 1er octobre 2026

- [x] CommandLink : supprimer la translation instantanée de 2 px au clic, sans supprimer l'état pressé ; régression RED→GREEN Light/Dark × Standard/Compact, souris et Espace.
- [x] Mica : réparer le crash de la galerie lors de l'enregistrement de nouveaux propriétaires de palette pendant un `PaletteChange` ; test de mécanisme RED→GREEN.
- [x] Rejouer dans la galerie Mica ON → OFF → Light : absence de crash et de cartes fantômes dans les états observés. La comparaison complète des transitions avec WinUI reste partielle.
- [x] Release : configuration/build, 53/53 CTest, 23/23 exécutables de domaine actuels et client Qt indépendant chargeant les deux styles installés. Captures AutoSuggest natives Light/Dark : 4 réussites ; les 4 scénarios clavier sont ignorés honnêtement quand Windows refuse le focus OS.
- [ ] Conserver une publication explicitement **expérimentale**, avec Mica et les extensions Qt signalés comme partiels ; ne pas annoncer une parité WinUI complète.
- [x] Regrouper les changements locaux en commits par problème, avec leurs tests ; validation de la galerie confirmée par l'utilisateur. Aucun push ni publication implicite.
- [ ] Faire passer la CI sur la révision exacte à publier. Le contrôle `clang-format` révèle aussi des écarts déjà présents dans HEAD (notamment les tailles adaptées aux grandes polices) : ne pas confondre les tests fonctionnels verts avec une CI entièrement validée.
- [ ] Préparer une archive versionnée pour la configuration réellement validée (Windows x64, MSVC, Qt 6.9.2), avec démo, plugin, DLL du style, dépendances Qt/runtime, licences et instructions ; vérifier sur une machine propre sans Qt dans le PATH. Le plugin n'est pas une DLL autonome.
- [ ] Tag/release : à publier seulement après ces vérifications ; aucune release n'est créée par la validation locale.

## P0 — garde-fous

- [ ] Couvrir l'axe complet Light/Dark/System × Standard/Compact × états dans la matrice (`CMakeLists.txt:64-73`, `winui3style_snapshot_matrix`).
- [ ] Réparer les gates CI clang-tidy (Qt 6.4 installé pour un minimum 6.5) et Qt 5.12 (`Q_NAMESPACE_EXPORT` refusé par moc 5.12), puis publier le rapport/ratchet décrit dans [`spec/REFACTORING_PLAN.md`](spec/REFACTORING_PLAN.md).

## P1 — navigation, champs, états

- [x] NavigationView : fonds `Selected` / `PointerOverSelected` / `PressedSelected` corrigés selon le XAML épinglé, test RED→GREEN Light/Dark × Standard/Compact ; commit autorisé par l'utilisateur. La séquence live complète reste à compléter avant statut `Verified`. Voir [l'audit](spec/NAVIGATIONVIEW_AUDIT_2026-09-28.md).
- [ ] AutoSuggestBox : clavier, hit-test et cycle Mica on/off/reopen fonctionnels dans la galerie locale le 29 septembre 2026 ; tests hors écran verts. La comparaison complète avec la Gallery officielle et un run natif automatisé avec focus OS restent à faire avant statut « Verified ».
- [x] Démos Compact galerie : NavigationView et MenuBar sont déjà présents dans les deux panneaux Standard/Compact de `densityComparisonGroup` ; métriques et tests existent.

## P2 — architecture, accessibilité, robustesse, tests

- [x] NavigationView sans icône : inset corrigé de 42 à 18 px d'après l'état `IconCollapsed` (colonne 8 px) ; test RED→GREEN Light/Dark × Standard/Compact × LTR/RTL et démo reconstruite ; commit autorisé par l'utilisateur. Comparaison officielle complète encore partielle.
- [ ] Démo NavigationView : décider si l'on expose une composition complète (retour, repli, pied Settings) ; le mapping QStyle actuel ne couvre que l'item sémantique. Compléter les captures live du focus clavier, press et animation avant tout statut `Verified`.
- [ ] Exécuter le plan d'assainissement par petits lots, tests d'abord : split des tests saturés, renderers/contrats/surfaces, puis réduction symétrique de `polish`/`unpolish`/`eventFilter` ([`spec/REFACTORING_PLAN.md`](spec/REFACTORING_PLAN.md)).
- [ ] SettingsCard Designer : conserver le composant composé pour la carte expansible (le simple `QFrame[winuiSettingsCard=true]` ne fournit ni en-tête, ni chevron, ni hit-test/animation). Étudier séparément la conversion des deux cartes à contrôle de fin vers une composition Qt standard, avec parité visuelle et tests d'interaction avant toute migration ; `check_designer_gallery.cmake` verrouille actuellement le composant promu.
- [ ] Live keyboard : focus/Tab/Space/Enter/popups (offscreen partiel via `inputModalityFocus`).
- [ ] Contrastes Light/Dark : pas encore de test de ratio dédié.
- [ ] Animation réduite Windows : non appliquée pour l'instant ; la galerie conserve ses animations applicatives, sauf si `WINUI3STYLE_DISABLE_ANIMATIONS` est défini explicitement.
- [ ] `CHANGELOG.md` : tenu à jour (`86c063b`) + matrice compat (`spec/compatibility.md`) ; à maintenir à chaque release.

## P2 — couverture QWidget restante (`spec/WIDGET_BACKLOG.md`)

- [ ] QGraphicsView + QRubberBand, QKeySequenceEdit + QFontComboBox, QToolBox, QColumnView + QUndoView, QMdiArea/QMdiSubWindow, QLCDNumber + QDial, QFileDialog/QColorDialog/QFontDialog/QInputDialog/QProgressDialog non natifs.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
