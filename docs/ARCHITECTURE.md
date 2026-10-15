# Architecture de SmithMatch

Ce document explique **comment le programme est organisé** : quels modules existent, ce que chacun fait, et par où passent les données. Il est écrit pour quelqu'un qui connaît l'électronique mais découvre le C++ : à chaque fois que c'est possible, on compare avec un montage électronique.

## 1. Vue d'ensemble : deux étages séparés

Imagine une chaîne audio : un **amplificateur** (qui fait le travail) et une **façade** avec boutons et afficheurs (qui parle à l'utilisateur). SmithMatch suit le même principe :

```
┌──────────────────────────────────────────────────────────────┐
│  src/ui/   — la façade (Dear ImGui + OpenGL)                  │
│  fenêtres, boutons, abaque dessiné, courbes, schéma           │
└───────────────────────────┬──────────────────────────────────┘
                            │ appelle des fonctions
┌───────────────────────────▼──────────────────────────────────┐
│  src/core/ — le moteur de calcul (C++ standard + JSON)        │
│  Γ, VSWR, solveur L/Pi/T, balayage, fichiers…                 │
│  AUCUNE dépendance graphique (règle n° 4 de CLAUDE.md)        │
└──────────────────────────────────────────────────────────────┘
```

Pourquoi cette séparation ?

- **Les tests** (`tests/`, doctest) ne touchent que `core/` : on peut vérifier toute la physique sans ouvrir une fenêtre.
- **La justesse** : tous les calculs sont dans un seul endroit, relus et testés avec des valeurs de référence (Pozar…).
- **L'avenir** : une version web ou un autre habillage graphique pourrait réutiliser `core/` tel quel.

Dans CMake, cela se voit ainsi : `smith_core` est une bibliothèque statique qui ne lie **que** `nlohmann_json`. L'application `smithmatch` lie `smith_core` + ImGui + ImPlot + GLFW + OpenGL + nativefiledialog + stb. Les tests `smith_tests` lient `smith_core` + doctest.

```
                 ┌─────────────┐
                 │ smith_core  │◄──────── nlohmann_json
                 └──▲───────▲──┘
        ┌───────────┘       └────────────┐
┌───────┴────────┐               ┌───────┴────────┐
│  smithmatch    │               │  smith_tests   │
│ (l'application)│               │   (doctest)    │
│ imgui, implot, │               └────────────────┘
│ glfw, nfd, stb │
└────────────────┘
```

## 2. Carte des dossiers

```
src/main.cpp        Point d'entrée : crée la fenêtre GLFW, ImGui, ImPlot, lance App.
src/core/           Moteur de calcul pur.
src/ui/             Interface (panneaux, abaque, schéma, thème).
src/ui/platform/    Tout ce qui dépend du système : boîtes de dialogue, capture PNG.
tests/              Tests unitaires de core/ (doctest).
cmake/              Dépendances (FetchContent), embarquement de la police, packaging.
assets/fonts/       Police Inter (+ sa licence), intégrée à l'exécutable.
docs/               Cette documentation.
```

## 3. Le moteur `src/core/`

| Fichier | Rôle | Analogie |
|---|---|---|
| `Types.hpp` | `Complex`, `kPi`, valeurs par défaut, tolérances nommées | le tableau des constantes d'un datasheet |
| `Units.cpp` | affichage (`38.98 nH`) et lecture (`4,7n`, `2.4 GHz`) | l'afficheur et le clavier d'un multimètre |
| `RfMath.cpp` | Γ, z↔Γ, VSWR, return loss, S11, pertes de désadaptation, Γ↔pixel | la règle de calcul de l'abaque |
| `Element.cpp` | bobines et condensateurs (série/shunt), réactance et susceptance, conversion X→L/C | le catalogue de composants |
| `Load.cpp` | comment la charge évolue avec la fréquence (3 modèles) | le modèle d'antenne |
| `Network.cpp` | liste d'éléments, impédance à chaque nœud, arcs échantillonnés pour le dessin | la netlist |
| `ESeries.cpp` | E12 / E24, arrondi logarithmique | le tiroir de composants normalisés |
| `Validation.cpp` | contrôle des saisies (Z0 > 0, f dans 1 kHz–1 THz, …) | les fusibles d'entrée |
| `Solver.cpp` | réseaux L, Pi, T | le concepteur automatique |
| `Sweep.cpp` | balayage en fréquence, bande passante, export CSV | l'analyseur de réseau |
| `Project.cpp` | l'état complet du document + lecture/écriture JSON | le fichier de projet |
| `FileIo.cpp` | lecture/écriture sûre de fichiers texte (UTF-8) | — |

### Conventions qui traversent tout le moteur

1. **Unités SI partout** : Ω, S, H, F, Hz. Les préfixes (nH, pF, GHz) n'existent que dans `Units` (affichage et saisie).
2. **Le réseau est une liste** (`ElementList`) dont l'**indice 0 est le plus proche de la charge**. Le dernier élément est côté source.
3. **Fonctions pures** : mêmes entrées, mêmes sorties, aucun état caché. Les erreurs *attendues* (pas de solution, entrée invalide, fichier incorrect) sont renvoyées dans une structure (`ok`, `message`) et **jamais** par exception.
4. **Pas de valeur « magique »** : chaque seuil a un nom (`kZeroRelTolerance`, `kVswrGoodLimit`…).
5. **`switch` exhaustifs sans `default`** : si on ajoute un type d'élément, le compilateur signale chaque endroit à compléter.

### `Project` : un type valeur

```cpp
struct Project {
    double z0;  double f0Hz;  Complex zSource;
    LoadSpec load;  ElementList network;
    SweepSettings sweep;  ESeriesKind eSeries;
};
```

`Project` est **copiable** : copier un projet, c'est prendre un instantané complet. C'est ce qui rendra « annuler / rétablir » presque gratuit en v0.3 (on empilera des copies).

## 4. L'interface `src/ui/`

| Fichier | Rôle |
|---|---|
| `App` | boucle principale, mise en page à la main (séparateurs déplaçables), menus, raccourcis, fichiers, dialogues |
| `Theme` | **toutes** les couleurs nommées (`kColorSeriesL`…), thème sombre, polices |
| `Widgets` | champ numérique avec validation, en-têtes, séparateurs, infobulles |
| `SetupPanel` | Z0, f0, source, charge + modèle, série E |
| `ResultsPanel` | Zin, Γ, VSWR, RL, S11, pertes, qualité (valeurs idéales et E-series) |
| `NetworkPanel` | mode manuel : ajouter / éditer / réordonner / supprimer les éléments |
| `SolverPanel` | mode automatique : topologie L/Pi/T, Q, liste de solutions, Apply |
| `SmithChartView` | dessin de l'abaque (grilles, trajets colorés, repères, lieu, survol) |
| `SweepPanel` | réglages du balayage + courbes ImPlot (|S11|, VSWR, bande passante) |
| `SchematicView` | schéma électrique (source à gauche, charge à droite) |
| `LearnText`, `HelpWindows` | textes pédagogiques et fenêtres d'aide, regroupés au même endroit |
| `platform/FileDialogs` | boîtes « Ouvrir / Enregistrer » natives (nativefiledialog-extended) |
| `platform/ImageExport` | capture OpenGL → PNG (`glReadPixels` + stb_image_write) |

### Principe « immediate mode » d'ImGui

Avec une interface classique (Qt, WinForms), on crée des widgets une fois, puis on réagit à des événements. Avec **Dear ImGui**, c'est l'inverse : **à chaque image (≈ 60 fois par seconde), on redessine toute l'interface** en appelant des fonctions (`ImGui::Button("OK")` renvoie `true` si on vient de cliquer). Il n'y a pas d'objets-boutons à synchroniser : l'unique source de vérité est le `Project`.

Conséquence très pratique : quand l'utilisateur modifie une valeur, **tout est recalculé et redessiné à l'image suivante** — abaque, résultats, courbes et schéma restent toujours cohérents.

## 5. Flux de données d'une image

```
                   ┌─────────────────────────┐
  clavier / souris │        App::drawFrame    │
 ─────────────────►│                         │
                   └──┬──────────────────────┘
                      │ 1. runSweep(project, réseau)           → sweepIdeal_
                      │    runSweep(project, réseau arrondi E) → sweepRounded_
                      │
   ┌──────────────────┼─────────────────────────────────────┐
   ▼                  ▼                                     ▼
SetupPanel       SmithChartView                        SweepPanel
NetworkPanel     (ChartScene = projet + aperçu         SchematicView
SolverPanel    solveur + lieu du balayage)          ResultsPanel
   │ modifient le Project (renvoient true si changé)
   ▼
 project_  ──►  modified_ = true  ──►  titre « * », boîte « Unsaved changes »
```

- Les panneaux **reçoivent le `Project` par référence** et le modifient directement (saisie, ajout d'élément, « Apply »). Ils renvoient `true` quand quelque chose a changé ; l'`App` positionne alors `modified_`.
- L'abaque ne lit pas le `Project` : il reçoit une **`ChartScene`** préparée par `App::buildScene()` (charge, réseau, aperçu du solveur en pointillés, lieu de Zin(f), élément survolé).
- Le survol d'une solution (AUTO-MATCH) ou d'un élément (NETWORK, SCHEMATIC) remonte à l'`App`, qui le transmet à l'abaque à l'image suivante.

## 6. Un calcul de bout en bout

Que se passe-t-il quand on clique « Apply » sur une solution du solveur ?

1. `solveL(zSource, zLoad, f0)` (dans `Solver.cpp`) calcule les solutions en formules fermées. Chacune est une `ElementList` dans la convention du réseau (indice 0 côté charge).
2. `SolverPanel` copie la liste choisie dans `project.network` (après confirmation si le réseau n'était pas vide).
3. À l'image suivante, `inputImpedance(network, zLoad, f0)` (dans `Network.cpp`) refait le calcul **indépendamment du solveur** : on part de Z_L, on applique chaque élément (`Z + jX` pour un série, `1/(1/Z + jB)` pour un shunt).
4. `evaluateMatch(zIn, zSource)` donne Γm, VSWR, RL… affichés dans RESULTS ; `sampledPath` fournit les arcs colorés de l'abaque.

Les tests utilisent exactement cette double vérification : les composants calculés par le solveur sont comparés à des références publiées, **et** on recalcule Zin avec `Network.cpp` pour exiger |Γm| < 10⁻⁹.

## 7. Fichiers et exports

- **Projet** : `toJson` / `fromJson` (`Project.cpp`), format `.smith.json` avec `"format"` et `"version"` (voir `docs/MATHS.md` pour les formules, `docs/USER_GUIDE.md` pour l'usage). Règle du projet : si le schéma change, on incrémente `kProjectFileVersion` **et** on garde la lecture des anciennes versions.
- **Écriture sûre** : `writeTextFile` écrit dans `fichier.tmp` puis renomme ; un plantage ne détruit jamais l'ancien fichier.
- **CSV** : `sweepToCsv` (core, testé). **PNG** : `ImageExport` capture le rectangle de l'abaque **après** le rendu de l'image et **avant** l'échange des tampons ; l'échelle du framebuffer (Retina) est prise en compte et l'image est retournée verticalement (OpenGL a son origine en bas).

## 8. Points d'extension prévus pour la v0.3 (non implémentés)

| Évolution | Ce qui est déjà prêt |
|---|---|
| Résistances, lignes, stubs | `ElementType` est un `enum class` ; `Element` possède un champ `params` réservé ; les `switch` exhaustifs signaleront chaque endroit à compléter |
| Q fini des composants | champ `quality` optionnel à ajouter dans `Element` |
| Charge Touchstone `.s1p` | `LoadModel` accepte une nouvelle valeur ; `loadImpedanceAt` est la seule fonction à adapter |
| Glisser-déposer sur l'abaque | chaque arc est une fonction de `core` paramétrée par la valeur du composant (`sampledArc`) |
| Annuler / rétablir | `Project` est un type valeur copiable |
| Version web | aucune API système en dehors de `src/ui/platform/` |
