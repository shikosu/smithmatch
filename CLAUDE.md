# CLAUDE.md — SmithMatch

> Ce fichier est lu automatiquement par Claude Code à chaque session.
> Il contient les **règles permanentes** du projet. En cas de doute, ce fichier fait foi.

---

## 1. Le projet en une phrase

**SmithMatch** est une application de bureau open source (licence MIT, Windows + macOS) écrite en C++17.
Elle sert à concevoir des **réseaux d'adaptation d'impédance à bobines (L) et condensateurs (C)**
à l'aide d'un **abaque de Smith interactif** : mode manuel, solveur automatique (L, Pi, T)
et balayage en fréquence.

- Interface utilisateur : **en anglais**, thème **sombre**.
- Priorité n°1 : **justesse des calculs**. Priorité n°2 : **clarté** (code et interface).

---

## 2. Qui est le mainteneur (important pour ta façon de travailler)

- Téo, étudiant en BUT GEII (bac+1). Bon niveau en électronique, débutant en C++.
  Il **comprend ce que fait une fonction**, mais **n'écrit pas encore de code lui-même**.
- **Parle-lui en français**, avec un vocabulaire simple mais techniquement exact,
  et utilise des analogies concrètes quand c'est utile.
- Quand tu crées ou modifies du code, **explique-lui bloc par bloc** ce que fait chaque partie et pourquoi.
- À la fin de chaque étape, fais un **résumé court** : ce qui a été fait, comment le tester, ce qui reste.
- Ne suppose jamais qu'il sait utiliser un outil (CMake, Git, débogueur). Donne les commandes exactes.
- Machines : un PC Windows et un Mac Apple Silicon. Éditeur : VS Code.

---

## 3. Règles absolues

1. **Ne jamais affirmer qu'une chose fonctionne sans l'avoir vérifiée** (compilation, tests, exécution).
   Si tu ne peux pas vérifier (par exemple le build macOS depuis Windows), dis-le explicitement.
2. **Tous les tests doivent passer** avant de considérer une étape comme terminée : `ctest` à 0 échec.
3. **Zéro warning** dans le code du projet (`src/`, `tests/`). Les warnings des dépendances tierces sont tolérés.
4. **Le dossier `src/core/` ne dépend d'aucune bibliothèque graphique**
   (ni ImGui, ni GLFW, ni OpenGL). Seulement la bibliothèque standard C++ et nlohmann/json.
5. **Unités SI partout en interne** : Ω, S, H, F, Hz. Les préfixes (nH, pF, GHz…)
   n'existent **que** dans l'affichage et la saisie (`core/Units`).
6. **Aucune valeur « magique »** : les constantes ont un nom (`kPi`, `kDefaultZ0`, …).
7. Ne pas ajouter de dépendance sans la justifier et sans vérifier qu'elle est compatible avec la licence MIT.
8. Toute nouvelle formule physique doit être accompagnée **d'au moins un test unitaire avec une valeur de référence**.
9. Ne jamais casser le format de fichier projet : si le schéma JSON change, incrémenter `version`
   et garder la lecture des anciennes versions.

---

## 4. Style de code

- **C++17**, CMake ≥ 3.24.
- Noms en **anglais** : `PascalCase` pour les types, `camelCase` pour les fonctions et variables,
  `kPascalCase` pour les constantes, membres privés suffixés `_` (ex. `value_`).
- **Commentaires en français**, détaillés et pédagogiques :
  - un en-tête par fichier qui explique son rôle ;
  - un commentaire avant chaque fonction (rôle, paramètres, unités, valeur retournée) ;
  - un commentaire avant chaque bloc logique non évident ;
  - pour chaque formule physique, écrire la formule en clair dans le commentaire.
- Préférer le **simple et lisible** au « malin » : `enum class` + `switch` plutôt que des templates
  ou de l'héritage complexe. Toujours un `switch` exhaustif (pas de `default:` qui masquerait un oubli,
  pour que le compilateur signale un cas manquant).
- `using Complex = std::complex<double>;` (défini dans `core/Types.hpp`).
- Les fonctions de calcul sont **pures** (pas d'état global) : mêmes entrées → mêmes sorties.
- Erreurs de calcul attendues (pas de solution, entrée invalide) : retourner un résultat structuré
  (ex. `struct SolverResult { bool ok; std::string message; std::vector<Solution> solutions; }`),
  **pas d'exceptions** dans `core/` pour les cas prévus.
- Fichiers : un `.hpp` + un `.cpp` par module. `#pragma once`.

---

## 5. Conventions physiques (À RESPECTER STRICTEMENT)

### 5.1 Grandeurs et signes
| Grandeur | Définition | Signe |
|---|---|---|
| Impédance | Z = R + jX | X > 0 inductif, X < 0 capacitif |
| Admittance | Y = 1/Z = G + jB | B > 0 capacitif, B < 0 inductif |
| Pulsation | ω = 2πf | f en Hz |
| L série | X = +ωL | |
| C série | X = −1/(ωC) | |
| C parallèle (shunt) | B = +ωC | |
| L parallèle (shunt) | B = −1/(ωL) | |

Les valeurs de composants (L en H, C en F) sont **toujours strictement positives**.

### 5.2 Réseau
- Le réseau est une **liste ordonnée d'éléments**. **L'élément d'indice 0 est le plus proche de la charge.**
  Le dernier élément est côté source.
- Nœud 0 = Z_L(f). Nœud k = impédance vue après l'élément k (en regardant vers la charge).
  Le dernier nœud est **Z_in**, l'impédance vue par la source.
- Élément série : Z_out = Z_in + jX(f).
- Élément parallèle : Y_out = Y_in + jB(f), avec Y = 1/Z.
- Garde-fous : si |Z| < 1e-300, l'admittance est infinie, donc un shunt ne change rien (court-circuit maintenu).
  Tester les cas limites, ne jamais diviser par zéro sans contrôle.

### 5.3 Abaque et réflexion
- Impédance de référence de l'abaque : **Z₀** (réelle, > 0, par défaut 50 Ω). Normalisation z = Z/Z₀, y = Y·Z₀.
- Point tracé sur l'abaque : **Γ = (Z − Z₀)/(Z + Z₀)**. Réciproque : z = (1 + Γ)/(1 − Γ).
- Orientation : Γ = +1 à droite (circuit ouvert), Γ = −1 à gauche (court-circuit),
  **moitié haute = inductive (x > 0)**, moitié basse = capacitive.
- Passage écran : pixel = centre + rayon·(Re Γ, **−Im Γ**). L'axe Y de l'écran est inversé.
- Déplacements (vérifiés) :
  - L série : sens horaire sur un cercle r = constante ; C série : sens anti-horaire.
  - C shunt : sens horaire sur un cercle g = constante ; L shunt : sens anti-horaire.
- Formules des cercles (utiles pour les tests ; pour le dessin, on échantillonne la transformation) :
  - cercle r : centre (r/(1+r), 0), rayon 1/(1+r)
  - arc x : centre (1, 1/x), rayon 1/|x|
  - cercle g : centre (−g/(1+g), 0), rayon 1/(1+g)
  - arc b : centre (−1, −1/b), rayon 1/|b| (b > 0 dans la moitié basse)
  - (v0.3) arc Q constant : centre (0, −1/Q) pour la moitié haute et (0, +1/Q) pour la basse, rayon √(1 + 1/Q²)

### 5.4 Qualité d'adaptation
- La **cible** de l'adaptation est **Z_s\*** (conjugué de l'impédance de source). Si Z_s = Z₀ (réel), la cible est le centre de l'abaque.
- Coefficient de réflexion côté source (ondes de puissance) : **Γ_m = (Z_in − Z_s\*)/(Z_in + Z_s)**.
  Si Z_s est réel, il se réduit au Γ classique.
- À partir de |Γ_m| :
  - |S11| (dB) = 20·log10|Γ_m| (négatif)
  - Return loss (dB) = −20·log10|Γ_m| (positif)
  - VSWR = (1 + |Γ_m|)/(1 − |Γ_m|). Afficher « ∞ » si |Γ_m| ≥ 1 − 1e-12, et VSWR = 1 si Γ = 0.
  - Mismatch loss (dB) = −10·log10(1 − |Γ_m|²)
- Afficher « ∞ dB » pour la return loss quand |Γ_m| < 1e-15.

### 5.5 Conversion réactance → composant (à f₀)
- X > 0 → L = X/ω ; X < 0 → C = −1/(ωX)
- B > 0 → C = B/ω ; B < 0 → L = −1/(ωB)
- |X| (ou |B|) ≈ 0 (tolérance relative 1e-9 par rapport à l'échelle du problème) → **l'élément est supprimé**
  (fil pour un série, circuit ouvert pour un shunt).

### 5.6 Solveur en L (formules fermées, vérifiées numériquement)
Notations : charge Z_L = R_L + jX_L, Y_L = G_L + jB_L ; cible Z_t = Z_s\* = R_t + jX_t, Y_t = 1/Z_t = G_t + jB_t.

**Topologie « shunt-first »** (élément parallèle aux bornes de la charge, puis élément série vers la source) :
```
d  = G_L / R_t − G_L²          (pas de solution si d < 0)
B' = ±√d                       (une seule solution si d = 0)
B_shunt  = B' − B_L
X_series = X_t + B' / (G_L² + B'²)
```
**Topologie « series-first »** (élément série côté charge, puis élément parallèle vers la source) :
```
d  = R_L / G_t − R_L²          (pas de solution si d < 0)
X' = ±√d
X_series = X' − X_L
B_shunt  = B_t + X' / (R_L² + X'²)
```
- Le solveur renvoie **toutes** les solutions valides des deux topologies (0 à 4 au total).
- Tolérance sur d : relative à l'échelle (ex. `d < −1e-12·(G_L/R_t)` → aucune solution ; `|d|` sous ce seuil → d = 0).
- Classement : (L série + C shunt) = **Low-pass** ; (C série + L shunt) = **High-pass** ; autre = **Mixed**.

### 5.7 Solveurs Pi et T (facteur Q nominal)
- **Pi** (shunt – série – shunt) : R_pL = 1/Re(1/Z_L), R_pS = 1/Re(1/Z_s\*).
  R_v = max(R_pL, R_pS)/(1 + Q²). Il faut **Q > Q_min = √(max/min − 1)**, sinon erreur claire.
  Construction : L « shunt-first » de Z_L vers R_v, puis L « series-first » de R_v vers Z_s\*.
  On **fusionne les deux réactances série** (somme des X à f₀, puis conversion en L ou C selon le signe).
  On obtient jusqu'à 2×2 = 4 combinaisons.
- **T** (série – shunt – série) : R_v = min(Re Z_L, Re Z_s\*)·(1 + Q²), avec la même condition Q > Q_min
  calculée sur les parties réelles série.
  Construction : L « series-first » de Z_L vers R_v, puis L « shunt-first » de R_v vers Z_s\*.
  On **fusionne les deux susceptances shunt**.
- Dans l'interface, préciser que Q est un **Q nominal** (défini par la résistance virtuelle R_v).

### 5.8 Modèle de charge pour le balayage
La charge est saisie comme Z_L à f₀. Son évolution avec f dépend du modèle choisi :
- `Constant` : Z_L(f) = Z_L.
- `SeriesEquivalent` (**par défaut**) : R série + L série (si X_L > 0) ou C série (si X_L < 0), calculés à f₀.
- `ParallelEquivalent` : G // C (si B_L > 0) ou G // L (si B_L < 0), calculés à f₀.
- (v0.3) `Touchstone` : table Z(f) issue d'un fichier .s1p, interpolée.
La source Z_s est constante en fréquence.

### 5.9 Séries normalisées
- E12 : 1.0 1.2 1.5 1.8 2.2 2.7 3.3 3.9 4.7 5.6 6.8 8.2
- E24 : 1.0 1.1 1.2 1.3 1.5 1.6 1.8 2.0 2.2 2.4 2.7 3.0 3.3 3.6 3.9 4.3 4.7 5.1 5.6 6.2 6.8 7.5 8.2 9.1
- Arrondi = valeur la plus proche **en échelle logarithmique** : minimiser |ln(v/s)|,
  en incluant 10.0 (décade suivante) parmi les candidats.

---

## 6. Architecture (carte des dossiers)

```
src/core/   Moteur de calcul pur (testé). Aucune dépendance graphique.
src/ui/     Interface Dear ImGui (fenêtres, abaque, schéma, courbes).
src/main.cpp Point d'entrée : crée la fenêtre GLFW et lance ui::App.
tests/      Tests unitaires doctest, qui ne testent que src/core/.
docs/       Documentation en français (ARCHITECTURE.md, MATHS.md, BUILD.md, USER_GUIDE.md).
cmake/      Modules CMake (dépendances, packaging).
assets/     Police embarquée (avec sa licence), icônes.
```
- Cible CMake `smith_core` (bibliothèque statique) ← utilisée par `smithmatch` (l'appli) et `smith_tests`.
- **Points d'extension prévus pour la v0.3** (ne pas les implémenter sans demande, mais ne pas les bloquer) :
  - `ElementType` : ajout de `SeriesResistor`, `ShuntResistor`, `TransmissionLine`, `OpenStub`, `ShortStub`
    (la structure `Element` contient déjà un champ `params` réservé).
  - Q fini des composants (pertes) : champ `quality` optionnel dans `Element`.
  - `LoadModel::Touchstone` : chargement d'un fichier .s1p.
  - Glisser-déposer sur l'abaque : le calcul de chaque arc doit déjà être une fonction de `core`
    paramétrée par la valeur du composant.
  - Annuler/rétablir : l'état `Project` est un **type valeur copiable**, ce qui rend les instantanés triviaux.
  - Version web (Emscripten) : ne pas utiliser d'API spécifique à un OS en dehors de `src/ui/platform/`.

---

## 7. Commandes utiles

```bash
# Configurer (une fois, ou après modification de CMakeLists.txt)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compiler
cmake --build build --config Release --parallel

# Lancer les tests
ctest --test-dir build -C Release --output-on-failure

# Créer le paquet (.dmg sur macOS, .zip sur Windows)
cmake --build build --config Release --target package
```
- Exécutable Windows : `build/Release/SmithMatch.exe` (générateur Visual Studio).
- App macOS : `build/SmithMatch.app`.

---

## 8. Workflow Git

- Petits commits, un par étape cohérente, messages en anglais au format
  `type: description` (`feat:`, `fix:`, `test:`, `docs:`, `build:`, `refactor:`).
- Ne jamais committer le dossier `build/`.
- Avant chaque commit : compilation sans warning et `ctest` vert.
