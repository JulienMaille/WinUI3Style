# NavigationView — audit visuel du 28 septembre 2026

Lot d'audit uniquement : aucun correctif de rendu n'est inclus.

## Périmètre et référence

- Démo locale : `build/demo/Release/winui3style_gallery.exe`, `main` à `33e89d3` ; WinUI 3 Gallery installée : 2.9.3.0, dépendance Windows App Runtime 2.5.1.0 (`Get-AppxPackage`). L'animation de fenêtre Windows est activée (`MinAnimate=1`). Le paquet Gallery inclut la ressource `scale-100` ; le DPR effectif du moniteur n'a pas été mesuré séparément.
- Comparaison live des lignes de navigation au repos, survol et sélection en Light/Dark. La démo locale a également été examinée en densités Standard/Compact ; la page « Compact Sizing » de la Gallery officielle énumère NavigationView mais ne fournit pas, à elle seule, une instance NavigationView compact comparable. Ne pas confondre cette densité avec le `PaneDisplayMode=LeftCompact`, qui replie le volet sur ses icônes.
- Mapping du projet : `QAbstractItemView[winuiNavigationView=true]` est une *variante sémantique d'item*. Le `QListWidget` conserve les interactions Qt ; `NavigationItemDelegate` peint l'item. Ce n'est pas un remplacement de la composition complète d'un NavigationView.
- Source Microsoft épinglée : [NavigationView_themeresources.xaml](https://github.com/microsoft/microsoft-ui-xaml/blob/9f89c2da5a5502c263d9268fee224697c47ecb6e/controls/dev/NavigationView/NavigationView_themeresources.xaml). La [documentation Compact Sizing](https://learn.microsoft.com/en-us/windows/apps/develop/ui/controls/compact-sizing) décrit une ressource de densité applicable au niveau application, page ou contrôle, distincte des modes de volet du [NavigationView](https://learn.microsoft.com/en-us/windows/apps/design/controls/navigationview).

## Écarts confirmés

1. **P1 — fonds des états sélectionnés inversés.** Le template Microsoft lie `Selected` à `SubtleFillColorSecondaryBrush`, `PointerOverSelected` à `SubtleFillColorTertiaryBrush` et `PressedSelected` à `SubtleFillColorSecondaryBrush`. Dans `NavigationItemDelegate::paint()` (`src/navigationview_p.cpp:147-150`), notre sélection au repos utilise `t.subtlePressed` (Tertiary : alpha 6 Light / 10 Dark), le survol va vers `t.subtleHover` (Secondary : alpha 9 / 15), puis l'appui revient vers Tertiary. Cela inverse les trois états. La sélection locale paraît notamment trop faible dans l'exemple Light. Aucun test existant ne verrouille cette correspondance couleur état par état.
2. **P2 — item sans icône trop décalé.** Dans le template Microsoft, l'état `IconCollapsed` masque `IconBox` et réduit `IconColumn.Width` à 8. Le délégué local place toujours le texte à `option.rect.left()+42` (`src/navigationview_p.cpp:154-165`), même si `Qt::DecorationRole` n'a pas d'icône. Les deux listes Navigation Standard/Compact de la démo sont sans icônes et rendent visible cet espace réservé. L'inset final doit être dérivé du template et protégé par un test avant correction ; ne pas appliquer une translation arbitraire.

## Ce qui reste partiel

- Les lignes locales ont bien une hauteur 40 px en Standard et 32 px en Compact, et l'indicateur de sélection 3 × 16 px est présent. Les fonds de survol non sélectionné sont visuellement proches de la référence, sans mesure pixel certifiée. Les surfaces des deux hôtes ne sont pas identiques : ne pas attribuer au délégué tout écart de gris observé dans un screenshot global.
- La Gallery locale expose des items, mais pas le bouton de repli, le bouton retour, ni un « Settings » épinglé au pied du volet comme dans l'exemple WinUI. C'est un écart de **composition de démo**, pas la preuve d'un bug du peintre d'item QStyle.
- L'outil de capture live disponible n'isole pas les images mouse-down ni les midpoints de l'animation officielle. Focus clavier, états disabled et mouvement ne sont donc pas certifiés. Aucun statut `Verified` n'est acquis par cet audit.

## Lot suivant proposé

Corriger uniquement le P1 des fonds sélectionnés : ajouter d'abord un test RED Light/Dark × Standard/Compact avec assertions token/état et pixels jumelées, puis remplacer la sélection par les trois brushes officiels sans modifier géométrie, animation ni autres items. Refaire la séquence live, la matrice de captures et les suites complètes ; faire approuver le changement visuel avant commit. Traiter l'inset sans icône dans un lot distinct.
