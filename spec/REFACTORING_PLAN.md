# WinUI3Style — plan d'assainissement architectural

> Baseline : 21 septembre 2026, `main` @ `80f6094` (PR #11 fusionnée).
> Ce plan réduit le couplage et la taille des hubs sans modifier les mappings,
> tokens ou géométries WinUI. `spec/METHODOLOGY.md` reste la règle d'acceptation.

## 1. Diagnostic : pourquoi le dépôt a grossi

La croissance n'est pas due à une cause unique et le nombre total de lignes ne
doit pas être confondu avec la dette architecturale.

Sur les 150 derniers commits first-parent, avec la même mesure de lignes non
vides :

| Périmètre | Baseline `68921e16` | `80f6094` | Évolution |
|---|---:|---:|---:|
| code livré (`src`, `include`, `demo`, `plugin`) | 7 923 | 15 146 | +91 % |
| tests C++ | 4 802 | 15 596 | +225 % |

À la baseline actuelle, le comptage physique donne 16 281 lignes livrées et
16 689 lignes de tests. Les tests ont donc grandi beaucoup plus vite que le
produit : états interactifs, thèmes, densités, DPI, popups, snapshots et voies
natives sont maintenant couverts séparément. Supprimer cette couverture pour
faire baisser le LOC serait une régression, pas un assainissement.

`winui3style.cpp` a lui-même diminué d'environ 1 950 lignes sur ces 150 commits
(~4 831 vers 3 043 lignes physiques). Les extractions ont créé des modules
spécialisés, mais de nouvelles responsabilités transversales ont ensuite
regrossi la façade et deux nouveaux hubs :

| Fichier livré | Lignes physiques | Responsabilités actuellement mélangées |
|---|---:|---|
| `src/winui3style.cpp` | 3 043 | façade QStyle, registres, palettes, polish/unpolish, apparence, densité, cycle de vie |
| `src/winui3surfaces_p.cpp` | 1 450 | backdrop, îlots, dialogs, popups, LineEdit, tooltip Slider, calendriers |
| `src/winui3interactions_p.cpp` | 913 | souris, clavier, focus, animations, popups, dialogs, sliders, propriétés |
| `src/winui3viewrenderers_p.cpp` | 739 | items, calendriers, arbres, tabs, splitters, docks, headers |
| `src/settingscard.cpp` | 721 | composition publique et comportement d'expansion |
| `src/winui3buttons_p.cpp` | 720 | boutons, outils, check/radio, toggle, labels |
| `src/winui3style_contracts_p.cpp` | 589 | métriques, tailles, rects, hints, icônes |

Les principaux facteurs sont :

1. La parité WinUI réelle exige des chemins distincts pour thème, densité,
   état, DPI, Qt 5/6, offscreen et compositor natif. Une abstraction générique
   erronée masque rapidement un détail de géométrie ou d'ordre d'événement.
2. Les corrections backdrop/popup ont ajouté des machines d'état de cycle de
   vie dans des fichiers initialement consacrés au rendu. Elles sont utiles,
   mais leur propriétaire n'est plus clair.
3. Les extractions précédentes ont séparé le dessin, tout en laissant
   `StylePrivate` construire une large table de callbacks et posséder tous les
   registres. Le couplage a été déplacé vers la façade au lieu de disparaître.
4. Les fichiers de rendu ont été découpés par vague historique, pas toujours
   par responsabilité : six `ComplexControl` sans lien partagent encore un
   fichier, comme items/tabs/docks/headers dans le renderer de vues.
5. Les séquences `polish`/`unpolish` et setup/restore sont volontairement
   symétriques. Les factoriser par jugement casse la restauration ; elles
   doivent migrer ensemble derrière une même frontière de domaine.
6. Les commentaires de mécanisme sont longs mais normatifs pour les bugs de
   compositor et de première frame. Les supprimer pour gagner des lignes ne
   réduit ni la complexité ni le risque.

La dette utile à traiter n'est donc pas « trop de fonctionnalités », mais le
nombre de responsabilités et de domaines qu'une correction doit traverser.

## 2. Contraintes non négociables

- Aucun changement silencieux de `spec/coverage.md`, des tokens, des rayons,
  insets, durées ou géométries.
- Aucun QSS, aucun widget Fluent de contournement ; les visuels restent au
  `QStyle`.
- Réutiliser `winui3paint_p.h`, `winui3geometry_p.h` et
  `winui3helpers_p.h` avant tout nouveau helper.
- Centraliser seulement les classifications et blocs strictement identiques.
  Les géométries par site restent locales et documentées.
- Déplacer `polish` et `unpolish` symétriquement, et setup/restore dans le même
  lot. L'ordre des événements ne change jamais dans une extraction mécanique.
- Un concern par commit ; ne jamais mélanger refactor comportement-neutre,
  correction et modification de tokens.
- Aucun test affaibli, commenté ou re-scopé. Les assertions pixel restent
  appariées à leurs assertions de token/géométrie du même état.

## 3. Mesures de succès

Le LOC total peut augmenter temporairement lors d'une extraction (headers,
interfaces et tests). Les métriques principales sont :

1. **Largeur d'impact** : une correction popup, dialog ou contrôle complexe ne
   doit plus modifier quatre domaines sans lien.
2. **Propriétaire unique** : chaque état mutable a un module propriétaire et
   une API étroite ; `StylePrivate` ne fait que composer ces modules.
3. **Routeurs courts** : `Style::polish`, `unpolish`, `eventFilter` et les
   dispatchers de dessin choisissent un domaine sans en implémenter la logique.
4. **Ratchet, pas quota arbitraire** : les hubs ne peuvent plus grossir sans
   mise à jour explicite de ce plan. Les extractions réduisent progressivement
   leur baseline.
5. **Marge de test** : aucun nouveau test n'est ajouté à un fichier au-dessus
   de 55 KiB ; il est d'abord déplacé dans un domaine cohérent.

Cibles après exécution du plan, sans en faire des motifs de refactor aveugle :

- `winui3style.cpp` < 2 200 lignes et sans logique de widget dans les routeurs ;
- aucun module de surface > 650 lignes ;
- `StyleInteractionController::eventFilter` réduit à un dispatch de domaine ;
- un fichier par famille de `ComplexControl` ou groupe réellement cohérent ;
- au moins 15 % de marge sous 60 KiB dans chaque fichier de tests actif.

## 4. Plan d'exécution ordonné

### Phase 0 — rendre les gates fiables et publier les métriques

1. Réparer les deux défauts CI indépendants du produit observés sur la PR #11 :
   le job clang-tidy installe Qt 6.4 alors que CMake exige Qt >= 6.5 ; le job
   Qt 5.12 échoue sur `Q_NAMESPACE_EXPORT` avant de compiler le style.
2. Ajouter un rapport read-only (non bloquant au premier commit) : taille des
   fichiers, span des fonctions, fan-in/fan-out interne et octets des tests.
3. Enregistrer la baseline dans le rapport, puis activer un ratchet uniquement
   sur les hubs existants et la taille des tests. Le total global reste une
   information, pas une gate.

### Phase 1 — redonner de la marge aux tests

Avant toute nouvelle couverture, scinder par responsabilité :

| Fichier | Taille actuelle | Première extraction |
|---|---:|---|
| `tst_winui3style_native.cpp` | 61 433 octets | popups/menus natifs vs fenêtres/dialogs |
| `tst_winui3views.cpp` | 61 401 octets | backdrop/scroll natif vs rendu de vues |
| `tst_winui3surfaces.cpp` | 60 982 octets | dialogs vs helpers/surfaces |
| `tst_winui3combobox.cpp` | 57 741 octets | AutoSuggest vs ComboBox |

Conserver les noms de fonctions utilisés par les reruns DPI et placer les
helpers partagés uniquement dans `tests/winui3testhelpers.h` ou un helper de
domaine unique.

### Phase 2 — dédupliquer les classifications pures

Un prédicat par commit, avec preuve d'équivalence :

- `verticalSpinButtons()` entre contrats et rendu complexe ;
- `spinBoxEditor()` / `comboBoxEditor()` entre boutons et contrats ;
- classification item-view/calendrier réellement identique entre contrats et
  renderer de vues.

Ne pas déplacer de constantes de padding, rayon ou position dans ces helpers.

### Phase 3 — scinder les renderers mécaniquement

Ordre du plus faible au plus fort risque :

1. `winui3complex_p.cpp` par familles Combo/Spin, Slider/ScrollBar,
   ToolButton/GroupBox ; garder le dispatcher et `coveredComplex()` inchangés.
2. `winui3viewrenderers_p.cpp` en items/calendrier, tabs, dock/splitter et
   headers.
3. `winui3buttons_p.cpp` en surfaces bouton/outils, sélection check/radio/toggle
   et labels.

Chaque commit est un déplacement pur validé par le domaine concerné, la
matrice snapshot, puis une comparaison d'images byte-identique hors scope.

### Phase 4 — scinder les contrats QStyle

Séparer `winui3style_contracts_p.cpp` selon les frontières virtuelles déjà
exposées :

- métriques et `sizeFromContents` ;
- `subElementRect` ;
- géométrie/hit-test des `ComplexControl` ;
- `styleHint` et icônes.

Les fonctions publiques de `Style` restent de minces délégations et les
assertions d'absence de fallback couvert restent intactes.

### Phase 5 — donner un propriétaire aux surfaces

Extraire de `winui3surfaces_p.cpp`, un domaine par commit :

1. surfaces de ContentDialog et scrim ;
2. helper-button de LineEdit ;
3. tooltip de Slider ;
4. popup/menu/combo/calendar ;
5. îlots backdrop/chrome et chaînes de scroll.

Conserver les fonctions d'entrée actuelles pendant la migration. Les retries
DWM, le premier frame popup et les recettes Source/SourceOver ne sont pas
réécrits lors de l'extraction.

### Phase 6 — réduire la façade en dernier

Créer des gestionnaires de cycle de vie étroits derrière la frontière de
callbacks existante :

- palettes et apparence ;
- connexions checkbox/radio/toggle ;
- progress/slider/scrollbar ;
- navigation/table/calendar ;
- popup/dialog.

Migrer pour chaque domaine : état + `polish` + `unpolish` + événements + tests,
dans le même lot. Ensuite seulement, réduire la table de callbacks et faire de
`StylePrivate` un compositeur de propriétaires plutôt qu'un propriétaire
global.

### Phase 7 — API et code potentiellement obsolète

Auditer avec compatibilité externe avant suppression :

- `Style(QStyle *base, ThemeMode, DensityMode)` n'a pas d'appel interne et son
  commentaire promeut Fusion alors que la méthodologie fixe QCommonStyle ;
- ne pas classer comme morts `ToggleSwitch`, `NavigationView`, `SettingsCard`
  ou `AnimatedStack`, qui sont des APIs publiques composées et testées ;
- distinguer les fallbacks de tests des fallbacks de production couverte.

Toute suppression publique passe par dépréciation documentée et cycle de
release, jamais par une simple recherche d'appels internes.

## 5. Gate de chaque lot

Après chaque commit :

1. `git diff --check` et clang-format 18.1.8 ;
2. configure/build Release ;
3. tests du domaine et cas Compact paramétrés ;
4. CTest complet, contrats source et galerie inclus ;
5. tous les exécutables de domaine ;
6. snapshot matrix ;
7. suite native si surfaces, événements ou fenêtres sont touchés ;
8. capture PrintWindow et comparaison live si un chemin de peinture change.

Une extraction n'est acceptée que si les pixels hors scope sont identiques et
si le nombre de responsabilités du hub diminue réellement. Déplacer 300 lignes
vers un fichier `utils` générique ne compte pas comme un progrès.

## 6. Ordre de livraison proposé

Les lots suivants sont volontairement petits et indépendants :

1. CI fiable + rapport de santé read-only ;
2. split des trois tests au plafond ;
3. helpers de classification exacts ;
4. split `ComplexControl` ;
5. split view/button renderers ;
6. split des contrats ;
7. split des surfaces ;
8. réduction symétrique polish/unpolish/eventFilter ;
9. revue API obsolète et activation finale des ratchets.

Ce séquencement évite de déplacer simultanément logique, tests et politique
visuelle. Il réduit d'abord le risque des changements suivants, puis traite les
hubs les plus sensibles une fois les frontières et les gates stabilisés.
