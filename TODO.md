# WinUI3Style — feuille de route

> État de référence : 7 septembre 2026, branche `codex/treeview-wizard-rebase`.
> Boucle de travail active depuis le 9 septembre 2026 : chaque lot met à jour ce fichier, est relu, commité sur la branche de travail, puis enchaîne le prochain fix facile. Les cases cochées correspondent à des éléments présents dans le dépôt ; elles ne remplacent pas une validation visuelle WinUI en direct.

## État actuel

- [x] Style livré comme `QStyle` (pas de QSS requis) et chargeable par plugin (`winui3`, `winui3compact`).
- [x] Thèmes Light, Dark et System.
- [x] Densités Standard et Compact, avec propriétés dynamiques `winuiDensity`/`DensityMode`.
- [x] Galerie principalement construite depuis un fichier `.ui`, avec une surface de démonstration des propriétés Designer.
- [x] Bibliothèque, galerie et tests compilent en Release sur l'environnement courant.
- [x] Suite CTest et matrice de snapshots passent sur la révision de référence.
- [ ] La couverture indiquée dans `spec/coverage.md` est encore « source-audited » : la comparaison WinUI live reste obligatoire.
- [ ] Lot en cours : fixer les items faciles dans l'ordre, committer par lot sur la branche de travail (jamais sur `main` protégé).

## Issues GitHub ouvertes (à intégrer au planning)

Snapshot de la liste GitHub : [issues ouvertes](https://github.com/JulienMaille/WinUI3Style/issues?q=is%3Aissue+is%3Aopen) — 5 issues ouvertes.

- [ ] **P1 — [#6 Animation ouverture menus : slide vertical-only + easing non conforme](https://github.com/JulienMaille/WinUI3Style/issues/6)**
  - Validation live 2026-09-09 (user) : motion OK, pas exactement WinUI mais jugé suffisant. Fermé comme déviation délibérée documentée dans `spec/coverage.md` (slide vertical 12 px + OutCubic, y compris sous-menus latéraux).
- [ ] **P1 — [#5 QMenu popup : géométrie non recalculée au changement de densité](https://github.com/JulienMaille/WinUI3Style/issues/5)**
  - Validation live 2026-09-09 (user) : compact ne tronque plus (OK, cf. géométrie) mais le popup ne diffère pas en Compact — non validé comme effet de densité. Conforme au contrat actuel (`menuItemHeight` 36 px + padding 8 px invariants, seul `QMenuBar` 12→8 compacte). Reproducteur précis encore requis avant correctif ou clôture by-design.
  - Changer Standard/Compact avec le popup ouvert et fermé.
  - Recalculer hauteur, padding, position et hit-test dès le changement de densité.
  - Tester la première frame et la géométrie stabilisée.
- [ ] **P2 — [#4 Retire `QColor(` grandfather list file-by-file](https://github.com/JulienMaille/WinUI3Style/issues/4)**
  - [x] `winui3menus_p.cpp` utilise le token existant `flyoutStroke` (valeurs Light/Dark identiques) ; exemption retirée du contrôle des couleurs codées en dur.
  - [x] `winui3surfaces_p.cpp` utilise les tokens `dialogCommandFill`/`dialogScrim` (valeurs Light/Dark identiques) ; gate `winui3style_source_contracts` verte, plus aucune exemption restante.
  - [x] `PE_PanelTipLabel` utilise les tokens `tooltipFill`/`tooltipStroke` (nouveau token `tooltipStroke`, valeurs Light/Dark couvertes par `paletteDerivedTokensMatchWinUIConstants`) ; rendu inchangé.
  - Périmètre #4 clarifié en boucle : gate `QColor([0-9]` ne porte que sur `src/*_p.cpp`+`src/*.cpp` hors `winui3tokens_p.h`/`winui3paint_p.cpp`/`winui3geometry_p.cpp`/`winui3theme_p.cpp` ; zéro littéral numérique hors homes (gate verte). Restent : `QColor(Qt::white/black/transparent)` (états système, exemption documentée), `Qt::transparent` direct en peinture (idem), `QColor(…)` numériques dans `winui3theme_p.cpp` (palette home approuvée) et valeurs attendues dans `tests/` (contrats, hors gate).
- [ ] **P2 — [#2 Promote warnings to errors (`/WX` + `-Werror`)](https://github.com/JulienMaille/WinUI3Style/issues/2)**
  - Première étape : option `WINUI3STYLE_WARNINGS_AS_ERRORS` privée à la bibliothèque, activée en CI MSVC. Warnings Qt/paramètre Release corrigés avec compatibilité Qt 5 ; build Release `/WX` et 40 CTests/snapshots réussis sur Qt 6.9.2. Extension aux autres cibles/compilateurs encore à valider. Boucle : build Debug `/W4` sans warning sur la lib, `buttons`+`density_widgets`+`contracts` verts en Debug Qt 6.9.2.
  - Nettoyer les warnings existants par cible.
  - Activer `/WX`/`-Werror` en CI de façon progressive, avec exceptions locales justifiées uniquement si nécessaire.
- [ ] **P2 — [#1 Split `drawButtonControl` per element](https://github.com/JulienMaille/WinUI3Style/issues/1)**
  - Séparer les chemins QPushButton, QToolButton, CommandLink, checkable/toggle et états disabled.
  - Préserver le rendu actuel avec tests de non-régression par élément et par thème/densité.

## P0 — intégration et garde-fous

- [ ] Ouvrir une PR depuis la branche de travail ; ne jamais pousser directement sur `main` protégé.
- [ ] Ajouter une CI propre : configure Debug/Release, build plugin + demo + tests, CTest, snapshots et artefacts de logs/captures.
  - Matrice Qt × Debug/Release et smoke d'installation ajoutés ; YAML validé localement, exécution GitHub Actions encore à confirmer. Boucle : 40/40 CTests verts en Release Qt 6.9.2 local (natif exclu), snapshot matrix incluse.
- [x] Tester l'installation dans un répertoire propre : plugin QStyle, headers publics et éventuelles DLL Qt uniquement.
  - Smokes Release et Debug réussis via `tools/install_smoke.ps1` avec Qt 6.9.2 local : installation temporaire, client lié uniquement à Qt Widgets, chargement isolé des deux clés du plugin.
- [x] Ajouter une note de release distinguant API publique, propriétés Designer et comportements expérimentaux : `RELEASE_NOTES.md`.
- [x] Vérifier que la DLL du style est autonome vis-à-vis du code de la galerie et qu'aucun header `WinUI3::*` n'est nécessaire à une application cliente standard.
  - Client Qt-only compilé et exécuté contre les installations Release et Debug ; aucune dépendance à la galerie ni inclusion WinUI3 requise pour charger le plugin.
- [ ] Bloquer les régressions visuelles avec une matrice Light/Dark/System × Standard/Compact × enabled/hover/pressed/disabled/focus.

## P1 — régressions et fidélité visuelle à traiter en premier

### Backdrop, repaint et surfaces

- [ ] Diagnostiquer les ghostings Mica/Acrylic lors du hover des boutons.
  - Revue actualisée du draft `origin/draft/mica-ghost-glyphs` au commit `dbd021c` : non intégré. Les deux erreurs de compilation signalées sur `4e64016` sont corrigées en amont ; la cible surfaces compile en Debug. Le test `islandScrollPostsFullViewportRepaint` échoue encore offscreen (aucun repaint complet observé) ; cause non établie, aucune exception ajoutée. La restauration de `WA_StyledBackground` manque et le chemin d'échec DWM ne restaure pas les surfaces enfants transparentisées. Worktree de revue : `D:/Dev/win11style-mica-review`.
- [ ] Comparer le backdrop des AutoSuggestBox, ComboBox, menus et popups ; appliquer une seule stratégie de surface/blur/repaint.
- [ ] Corriger le repaint lors du passage Light ↔ Dark ↔ System (fond principal, menu, panneau gauche, contour de fenêtre).
- [ ] Tester activation/désactivation du backdrop, redimensionnement, occlusion et déplacement de fenêtre.

### Dialogues et Wizard

- [ ] Corriger les deux surfaces WinUI des dialogs (contenu et command area), sans rectangle blanc résiduel. Repro live 2026-09-09 (user, screenshots) : QWizard Dark — fond de page noir, pied de page blanc cassé, boutons Next bleu clair/Cancel fantôme ; QWizard Light — pied de page gris/bandes bleues au hover, Next bleu foncé/Cancel blanc. Pistes : `refreshWizardSurface` ne peint que le footer mais pas les `QWizardPage`/labels (fond noir = page sans palette contenue ?), boutons wizard hors `commandPalette`, `WizardFooterSurface` non repeint au hover/theme, caption native bleue via DWM à vérifier.
- [ ] Corriger couleurs de titre, texte et boutons en Light/Dark/System.
- [ ] Corriger centrage vertical indépendant de la partie haute et de la barre de commandes.
- [ ] Réparer le Wizard en Dark : fond, pages, titre, boutons, disabled et navigation.
- [ ] Tester QMessageBox, QDialogButtonBox, QWizard et les dialogs persistants avec changement de thème.

### Navigation, arborescences et onglets

- [ ] Revalider NavigationView selon les métriques WinUI (sélection, compact/expanded, recherche, clavier, focus).
- [x] Réduire le padding gauche cumulatif de QTreeView/QTreeWidget à chaque niveau ; couvrir arbre normal et arbre avec cases à cocher dans la galerie. Validation live 2026-09-09 (user) : OK.
- [ ] Corriger les séparateurs de QTabBar/TabView adjacents à l'onglet sélectionné.
- [ ] Vérifier les états hover/pressed/selected et le contraste en trois thèmes.

### Champs, listes déroulantes et sélecteurs

- [ ] AutoSuggestBox : modèle de suggestions, ouverture/fermeture, clavier, sélection, hit-test, thème et backdrop ; ajouter une interaction réellement testée.
- [ ] ComboBox : ouverture au relâchement, animation du glyph, marker animé au click-and-hold, padding haut/bas, icône de l'item sélectionné et recalcul de taille.
- [ ] Corriger le chevron ComboBox et son centrage sans modifier la hauteur de ligne WinUI.
- [ ] LineEdit/TextBox : clear button visible uniquement quand le champ a le focus, glyph X correctement centré et hover centré.
- [ ] Corriger le décalage gauche de l'editable ComboBox et les largeurs de NumberBox en Compact.
- [ ] DatePicker/TimePicker : texte visible, calendrier lisible, hover Light/Dark, en-tête et cellules correctement contrastés.

### États et animations des contrôles

- [ ] Vérifier les rôles de couleur Accent en Light et Dark (l'accent Dark n'est pas une copie brute de Light).
- [ ] Corriger couleurs des checkmarks, radio rings, toggle thumb et indicateurs indeterminate dans chaque thème.
- [ ] Comparer les vitesses d'animation CheckBox, QPushButton, ToggleSwitch et QRadioButton ; supprimer le « rate » des clics radio.
- [ ] Ajouter les transitions de texte/foreground des boutons pendant pressed et click-and-hold.
- [ ] Corriger coins/rayon des boutons de toolbar dans tous les états, y compris pressed et groupes contigus.
- [ ] Corriger le bouton ToggleSwitch pressé (thumb, marge et rayon conformes à WinUI).
- [ ] Vérifier glyphes, taille et alignement des sous-menus, clear buttons et contrôles numériques.

### Géométrie restante

- [x] Corriger le splitter mal centré. Validation live 2026-09-09 (user) : OK. Couvert par `splitterHandleContract` + `splitterGripPixelAlignment` (DPR 100/125/150/200) ; le décalage 1px backlog (`spec/WIDGET_BACKLOG.md`) reste couvert par le snapping testé.
- [ ] Auditer l'ascenseur/scrollbar (épaisseur, hit area, hover, dark/light).
- [x] Vérifier les menus tronqués en Compact et la largeur minimale de tous les combos de la galerie. Validation live 2026-09-09 (user) : compact plus tronqué — OK.

## P1 — protocole de validation WinUI obligatoire

- [ ] Pour chaque contrôle, documenter la source Microsoft dans `spec/` et les métriques retenues.
- [ ] Tester les états au repos, hover, pressed, click-and-hold, focus clavier, disabled, indeterminate et RTL si pertinent.
- [ ] Capturer Light et Dark côte à côte avec mêmes dimensions et même contenu.
- [ ] Valider les popups sur première frame, frame intermédiaire et état stabilisé.
- [ ] Ne pas considérer une capture statique comme preuve d'une animation ou d'un hit-test.
- [ ] Rejouer les interactions avec souris pressée, maintenue puis relâchée ; couvrir les clics rapides répétés.

## P1 — mode Compact

Le mode Compact doit être explicite, héritable et réversible, et couvrir les contrôles WinUI suivants :

- [ ] ListView
- [ ] TextBox
- [ ] PasswordBox
- [ ] AutoSuggestBox
- [ ] ComboBox
- [ ] DatePicker
- [ ] TimePicker
- [ ] TreeView
- [ ] NavigationView
- [ ] MenuBar

Pour chacun : métriques de hauteur/padding, popup, focus, disabled, accessibilité, changement de densité à chaud et test galerie Standard ↔ Compact.

## P2 — couverture QWidget à compléter

La liste ci-dessous reprend `spec/WIDGET_BACKLOG.md` et doit être traitée par lots avec démo et tests :

- [ ] QGraphicsView et QRubberBand.
- [ ] QKeySequenceEdit et QFontComboBox.
- [ ] QToolBox.
- [ ] QColumnView et QUndoView.
- [ ] QMdiArea/QMdiSubWindow.
- [ ] QLCDNumber et QDial.
- [ ] QFileDialog, QColorDialog, QFontDialog, QInputDialog et QProgressDialog non natifs.
- [ ] Revalider QCommandLinkButton, QStatusBar, QSizeGrip et QWizard dans les trois thèmes.
- [ ] Ajouter dans la galerie tous les widgets déjà gérés mais encore absents, avec variantes normale/Compact et états disabled/RTL lorsque pertinents.
- [ ] Remplacer à terme le widget promu `WinUI3::SettingsCard` par un QFrame/QGroupBox standard avec propriété `winuiSettingsCard=true` si cela reste compatible Designer.

## P2 — architecture et API publique

- [ ] Garder la galerie découplée du style : privilégier widgets Qt standards, propriétés dynamiques et Designer.
- [ ] Réduire les appels `WinUI3::*` à l'API nécessaire (helpers de composition uniquement).
- [ ] Maintenir une séparation nette entre palette, métriques, animation, surfaces et primitives de dessin.
- [ ] Centraliser les rôles de palette (accent, texte, glyphes, surfaces, borders) et bannir les couleurs ad hoc.
- [ ] Définir le contrat des propriétés publiques : `ThemeMode`, `DensityMode`, `ControlRole`, `winuiDensity`, `winuiControlRole`, `winuiBackdrop`, `winuiSurface`, `winuiToggleSwitch`, `winuiSettingsCard`, `winuiNavigationView`, etc.
- [ ] Garantir que les propriétés inconnues sont ignorées sans crash et que le polish/unpolish est symétrique.
- [ ] Documenter la compatibilité Qt 6.5+ et le chemin de repli Qt 5 si maintenu.

## P2 — accessibilité, clavier et RTL

- [ ] Vérifier focus ring, ordre Tab, activation Space/Enter et navigation des popups.
- [ ] Vérifier les rôles accessibles et états checked/indeterminate/expanded/selected.
- [ ] Vérifier RTL, hit-test miroir, chevrons et icônes.
- [ ] Vérifier contrastes Light/Dark et réduction des animations.

## P2 — performance et robustesse

- [ ] Mesurer le coût de polish, drawControl et des animations sur grandes listes/tableaux.
- [ ] Éviter les allocations répétées de QPixmap/QIcon/QPainterPath dans les chemins chauds.
- [ ] Tester changement de palette/thème/densité pendant animation et pendant popup ouvert.
- [ ] Ajouter tests de durée de vie : destruction/recréation de popup, changement de modèle, changement de style à chaud.
- [ ] Nettoyer warnings puis activer `/WX`/`-Werror` (issue #2).

## P2 — qualité des tests

- [ ] Restaurer/maintenir les tests d'activation au relâchement et de marker click-and-hold des ComboBox.
- [ ] Ajouter les tests manquants signalés dans `spec/coverage.md` : slider input, progress indeterminate, fallback boundary, palette runtime, dialog lifecycle, navigation lifecycle, RTL.
- [ ] Ajouter assertions de géométrie pour padding, slot/glyph, surfaces et séparateurs.
- [ ] Garder les snapshots déterministes et documenter toute tolérance de pixels.
- [ ] Ajouter au moins un test d'intégration de la galerie en `.ui` pour éviter les régressions de layout après migration.

## P3 — distribution et maintenance

- [ ] Tester installation/désinstallation du plugin dans une application Qt vierge.
- [ ] Publier headers, CMake package/config et exemples Designer minimaux.
- [ ] Documenter dépendances runtime (Qt, DLL du style, éventuel backend Windows) et absence de dépendance à la galerie.
- [ ] Préparer changelog, versionnement et artefacts Windows x64.
- [ ] Ajouter une matrice de compatibilité Windows/Qt et une procédure de reproduction des captures.

## Ordre recommandé

1. Fermer les issues #6 et #5 avec tests popup/animation et validation live.
2. Corriger backdrop/repaint, dialogs/Wizard et palette/indicateurs Dark.
3. Stabiliser AutoSuggestBox, ComboBox, TextBox clear button et DatePicker.
4. Corriger NavigationView, TreeView, TabView, toolbar, splitter et scrollbar.
5. Finaliser Compact sur les dix contrôles WinUI et compléter la galerie.
6. Séparer `drawButtonControl`, retirer les couleurs codées en dur et activer les warnings en erreurs (#1, #4, #2).
7. Étendre la couverture QWidget, accessibilité, performance et packaging.

## Définition de terminé

Un lot n'est terminé que si le comportement est implémenté dans le `QStyle`, exposé via API/propriété standard quand possible, représenté dans la galerie, couvert par un test automatisé, validé dans Light/Dark/System et Standard/Compact, et documenté avec une référence WinUI. Une capture seule ou un build réussi ne suffit pas pour déclarer un correctif visuel ou une animation terminé.
