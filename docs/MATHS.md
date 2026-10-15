# Les maths de SmithMatch

Toutes les formules utilisées par le moteur (`src/core/`), avec leurs conventions et des **exemples chiffrés qui sont aussi des tests** (`tests/`). Si un doute apparaît sur un résultat, ce document et les tests font foi.

> **Unités** : tout est calculé en SI (Ω, S, H, F, Hz). Les préfixes (nH, pF, GHz) n'apparaissent qu'à l'affichage.
> **Notation** : j² = −1 (convention électronique). ω = 2πf.

---

## 1. Grandeurs et signes

| Grandeur | Définition | Signe |
|---|---|---|
| Impédance | Z = R + jX | X > 0 inductif, X < 0 capacitif |
| Admittance | Y = 1/Z = G + jB | B > 0 capacitif, B < 0 inductif |
| L série | X = +ωL | |
| C série | X = −1/(ωC) | |
| C parallèle (shunt) | B = +ωC | |
| L parallèle (shunt) | B = −1/(ωL) | |

Mnémonique : *une bobine « s'oppose » au courant : X positif ; un condensateur « laisse passer » : sa susceptance est positive.*

**Conversion réactance → composant (à f₀)** :

- X > 0 → L = X/ω ; X < 0 → C = −1/(ωX)
- B > 0 → C = B/ω ; B < 0 → L = −1/(ωB)
- Si |X| (ou |B|) est négligeable (rapport < 10⁻⁹ par rapport à l'échelle du problème), **l'élément est supprimé** : un fil pour un série, un circuit ouvert pour un shunt.

Exemples (tests) : X = +100 Ω à 1 GHz → L = **15.9155 nH** ; X = −100 Ω à 1 GHz → C = **1.5915 pF**.

## 2. Le réseau d'adaptation

Le réseau est une **liste ordonnée** ; l'élément d'indice 0 est le plus proche de la charge. On part de Z₍nœud 0₎ = Z_L(f), puis :

- **élément série** : Z_sortie = Z_entrée + jX(f)
- **élément shunt** : Y_sortie = Y_entrée + jB(f), avec Y = 1/Z

Le dernier nœud est **Z_in**, l'impédance vue par la source. *Garde-fou* : si |Z| < 10⁻³⁰⁰ (court-circuit), l'admittance est infinie, un shunt ne change donc rien.

## 3. L'abaque de Smith

### 3.1 Coefficient de réflexion

Avec Z₀ l'impédance de référence de l'abaque (réelle, 50 Ω par défaut), z = Z/Z₀ et y = Y·Z₀ :

```
Γ = (Z − Z₀)/(Z + Z₀) = (z − 1)/(z + 1)        réciproque :  z = (1 + Γ)/(1 − Γ)
```

Orientation : Γ = +1 à droite (circuit ouvert), Γ = −1 à gauche (court-circuit), **moitié haute = inductive (x > 0)**. À l'écran : `pixel = centre + rayon·(Re Γ, −Im Γ)` — le signe « − » vient de l'axe Y de l'écran, orienté vers le bas.

Exemples (tests, Z₀ = 50 Ω) :

| Z | Γ | VSWR | Return loss |
|---|---|---|---|
| 50 | 0 | 1 | ∞ |
| 100 | +1/3 | 2 | 9.5424 dB |
| 25 | −1/3 | 2 | 9.5424 dB |
| 50 + j50 | 0.2 + j0.4 (\|Γ\| = 0.44721) | 2.6180 | 6.9897 dB |

### 3.2 Cercles de la grille (utilisés pour contrôler le dessin)

- cercle r = constante : centre (r/(1+r), 0), rayon 1/(1+r)
- arc x = constante : centre (1, 1/x), rayon 1/|x|
- cercle g = constante : centre (−g/(1+g), 0), rayon 1/(1+g)
- arc b = constante : centre (−1, −1/b), rayon 1/|b|

La grille **d'admittance** est le symétrique de la grille d'impédance par rapport au centre (Γ → −Γ). Le programme ne dessine pas ces formules : il **échantillonne** la transformation Γ = (z − 1)/(z + 1) sur 192 points ; les formules servent aux tests.

### 3.3 Sens de déplacement (vérifié par les tests)

| Élément ajouté | Se déplace… |
|---|---|
| L série (+jX) | sens **horaire** sur le cercle r = constante |
| C série (−jX) | sens anti-horaire sur le cercle r = constante |
| C shunt (+jB) | sens **horaire** sur le cercle g = constante |
| L shunt (−jB) | sens anti-horaire sur le cercle g = constante |

## 4. Qualité d'adaptation

### 4.1 Cible et Γ_m

L'adaptation parfaite transfère le maximum de puissance à la charge : il faut que l'impédance vue **soit le conjugué de la source**. La **cible** est donc **Z_s\*** (si Z_s = Z₀ réel : le centre de l'abaque). Pour une source complexe, on utilise les ondes de puissance :

```
Γ_m = (Z_in − Z_s*) / (Z_in + Z_s)
```

Si Z_s est réelle, c'est le Γ classique. **Γ_m = 0 ⇔ adaptation parfaite.**

### 4.2 Grandeurs dérivées de |Γ_m|

| Grandeur | Formule | Cas limites |
|---|---|---|
| \|S11\| (dB) | 20·log₁₀\|Γ_m\| (négatif) | −∞ si \|Γ_m\| < 10⁻¹⁵ |
| Return loss (dB) | −20·log₁₀\|Γ_m\| (positif) | « ∞ dB » si \|Γ_m\| < 10⁻¹⁵ |
| VSWR | (1 + \|Γ_m\|)/(1 − \|Γ_m\|) | 1 si Γ = 0 ; « ∞ » si \|Γ_m\| ≥ 1 − 10⁻¹² |
| Mismatch loss (dB) | −10·log₁₀(1 − \|Γ_m\|²) | ∞ si \|Γ_m\| ≥ 1 |

Exemple (test) : \|Γ\| = 1/3 → mismatch loss = **0.5115 dB**.

Jugement de qualité affiché : **Good** si VSWR ≤ 1.5, **Acceptable** si ≤ 2, sinon **Poor**.

## 5. Solveur en L

Un réseau en L = **un élément série + un élément shunt**. Deux ordres possibles, donc deux familles de solutions.

Notations : charge Z_L = R_L + jX_L, Y_L = G_L + jB_L ; cible Z_t = Z_s\* = R_t + jX_t, Y_t = 1/Z_t = G_t + jB_t.

### 5.1 « Shunt-first » (élément parallèle aux bornes de la charge, puis élément série)

L'idée : choisir B' (la susceptance totale après le shunt) pour que la partie réelle de l'impédance obtenue soit égale à R_t. On trouve une équation du second degré en B' :

```
d  = G_L / R_t − G_L²           (pas de solution si d < 0)
B' = ±√d                        (une seule solution si d = 0)
B_shunt  = B' − B_L
X_series = X_t + B' / (G_L² + B'²)
```

### 5.2 « Series-first » (élément série côté charge, puis élément parallèle)

Même raisonnement en admittance :

```
d  = R_L / G_t − R_L²           (pas de solution si d < 0)
X' = ±√d
X_series = X' − X_L
B_shunt  = B_t + X' / (R_L² + X'²)
```

- Le solveur renvoie **toutes** les solutions des deux familles (0 à 4).
- Tolérance sur d : si d < −10⁻¹²·(G_L/R_t) → aucune solution ; si |d| est sous ce seuil → d = 0.
- **Classement** : L série + C shunt = **Low-pass** ; C série + L shunt = **High-pass** ; autre = **Mixed**.
- **Q de la section** : |X'|/R_L (series-first) ou |B'|/G_L (shunt-first), affiché en mode Learn.

### 5.3 Exemple 1 — Pozar, *Microwave Engineering*, exemple 5.1

Z_L = 200 − j100 Ω, Z_s = Z₀ = 100 Ω, f = 500 MHz. Y_L = 1/Z_L = 4 mS + j2 mS.

- shunt-first : d = G_L/R_t − G_L² = 0.004/100 − 0.004² = 2.4·10⁻⁵ → B' = ±4.899 mS.
- B' = +4.899 mS : B_shunt = 4.899 − 2 = **2.899 mS** → C shunt = B/ω = **0.9228 pF** ; X_series = 0 + B'/(G_L² + B'²) = 0.004899/(1.6·10⁻⁵ + 2.4·10⁻⁵) = **122.47 Ω** → L série = **38.98 nH** (passe-bas).
- B' = −4.899 mS : **L shunt 46.14 nH** + **C série 2.599 pF** (passe-haut).
- series-first : d < 0 → **aucune solution**.

### 5.4 Exemple 2 — l'exemple de départ du programme

Z_L = 25 − j15 Ω, Z_s = Z₀ = 50 Ω, f = 2.4 GHz (ω = 1.5080·10¹⁰ rad/s). z = 0.5 − j0.3, **|Γ| = 0.3812, arg Γ = −137.73°, VSWR = 2.232, return loss = 8.377 dB, mismatch loss = 0.6819 dB.**

- shunt-first : d = G_L/R_t − G_L² < 0 → aucune solution.
- series-first : G_t = 1/50 = 0.02 S, d = R_L/G_t − R_L² = 25·50 − 625 = 625 → **X' = ±25 Ω**.
  - X' = +25 Ω : X_series = 25 − (−15) = **+40 Ω** → **L série = 40/ω = 2.6526 nH** ; B_shunt = 0 + 25/(625 + 625) = **0.02 S** → **C shunt = 1.3263 pF**.
  - X' = −25 Ω : X_series = −25 + 15 = **−10 Ω** → **C série = 1/(10 ω) = 6.6315 pF** ; B_shunt = −25/1250 = −0.02 S → **L shunt = 1/(0.02 ω) = 3.3157 nH**.

**Vérification à la main** de la première solution : Z = 25 − j15 + j40 = 25 + j25 Ω ; Y = 1/(25 + j25) = 0.02 − j0.02 S ; on ajoute +j0.02 S → Y = 0.02 S ; Z = **50 Ω** ✓. C'est l'enchaînement exact que le programme dessine sur l'abaque (arc orange puis arc vert, jusqu'au centre).

### 5.5 Exemple 3 — source complexe

Z_s = 30 + j20 Ω, Z_L = 80 − j40 Ω, 1 GHz : la cible est Z_s\* = 30 − j20 Ω. Le solveur trouve 2 solutions shunt-first et 0 series-first ; pour chacune on contrôle |Γ_m| = |(Z_in − Z_s\*)/(Z_in + Z_s)| < 10⁻⁹.

## 6. Solveurs Pi et T

Ils ajoutent un **degré de liberté** : le facteur de qualité Q, qui règle la **bande passante** (un Q élevé = bande étroite). Le principe est de coller deux réseaux en L autour d'une **résistance virtuelle R_v** :

```
Z_L ──[ L n°1 ]── R_v ──[ L n°2 ]── Z_s*
```

### 6.1 Pi (shunt – série – shunt)

```
R_pL = 1 / Re(1/Z_L)       R_pS = 1 / Re(1/Z_s*)          (résistances parallèles équivalentes)
Q_min = √( max(R_pL,R_pS)/min(R_pL,R_pS) − 1 )             → il faut Q > Q_min
R_v  = max(R_pL, R_pS) / (1 + Q²)
```

Construction : un L « shunt-first » de Z_L vers R_v, puis un L « series-first » de R_v vers Z_s\*. Les deux réactances série qui se touchent sont **fusionnées** (somme des X à f₀, puis conversion en L ou C selon le signe). Jusqu'à 2 × 2 = 4 combinaisons.

### 6.2 T (série – shunt – série)

```
Q_min : même formule, calculée sur les parties réelles série Re(Z_L) et Re(Z_s*)
R_v  = min(Re Z_L, Re Z_s*) · (1 + Q²)
```

Construction : un L « series-first » de Z_L vers R_v, puis un L « shunt-first » de R_v vers Z_s\*. Les deux susceptances shunt qui se touchent sont fusionnées.

### 6.3 Exemple (tests)

Z_s = 50 Ω, Z_L = 10 Ω, 100 MHz : **Q_min = √(50/10 − 1) = 2.0**.

- Pi, Q = 3 : R_v = 50/(1 + 9) = **5 Ω** ; 4 solutions, toutes avec |Γ_m| < 10⁻⁹.
- T, Q = 3 : R_v = 10·(1 + 9) = **100 Ω** ; 4 solutions, toutes avec |Γ_m| < 10⁻⁹.
- Q = 1.5 (< Q_min) → erreur claire « Q must be greater than Q_min = 2 », aucune solution.

> **Attention au mot « Q »** : c'est un Q **nominal**, défini par la résistance virtuelle R_v. Ce n'est pas le Q chargé exact du circuit complet ; mais il oriente directement la largeur de bande.

## 7. Modèles de charge (balayage)

La charge est saisie **à f₀**. Pour balayer, il faut savoir comment elle évolue ; la source Z_s est constante.

| Modèle | Z_L(f) |
|---|---|
| `Constant` | Z_L(f) = Z_L |
| `SeriesEquivalent` (**par défaut**) | R en série avec L si X_L > 0 : X(f) = X_L·f/f₀ ; avec C si X_L < 0 : X(f) = X_L·f₀/f |
| `ParallelEquivalent` | G en parallèle avec C si B_L > 0 : B(f) = B_L·f/f₀ ; avec L si B_L < 0 : B(f) = B_L·f₀/f |

À f = f₀, tous les modèles redonnent exactement Z_L. Exemple (test) : X_L = −15 Ω à 2.4 GHz → à 4.8 GHz, Im Z_L = **−7.5 Ω**.

## 8. Balayage et bande passante

- Les fréquences sont réparties en **linéaire** ou en **logarithmique** (rapport constant entre deux points), bornes incluses ; **f₀ est toujours ajouté** s'il est dans la plage, pour que le résultat à f₀ soit exact.
- Pour chaque fréquence : Z_L(f), puis Z_in(f) (calcul du réseau avec les X et B à cette fréquence), puis Γ_m, |S11|, VSWR.
- **Bande passante** : plage **contiguë** autour de f₀ où VSWR ≤ seuil. Les bords sont obtenus par **interpolation linéaire de |Γ_m|** entre le dernier point conforme et le premier point non conforme ; le seuil est converti en |Γ| = (S − 1)/(S + 1). (On interpole |Γ| plutôt que le VSWR, car |Γ| reste toujours fini.)
- Si VSWR(f₀) dépasse le seuil : pas de bande, message clair. Si la bande atteint le bord de la plage balayée, elle est signalée « ouverte » (affichée « > … »).
- Pour le tracé, |S11| est limité à −100 dB (et l'axe affiche −40…0 dB) ; le VSWR est écrêté à 10.

## 9. Séries normalisées

- **E12** : 1.0 1.2 1.5 1.8 2.2 2.7 3.3 3.9 4.7 5.6 6.8 8.2
- **E24** : 1.0 1.1 1.2 1.3 1.5 1.6 1.8 2.0 2.2 2.4 2.7 3.0 3.3 3.6 3.9 4.3 4.7 5.1 5.6 6.2 6.8 7.5 8.2 9.1

L'arrondi choisit la valeur la plus proche **en échelle logarithmique** (on minimise |ln(v/s)|), en incluant 10.0 (la décade suivante) parmi les candidats. Exemples (tests) : 4.9 nH → **4.7 nH** (E12) ; 9.5 pF → **10 pF** (E12, passage de décade) ; 1.04 → **1.0** et 1.06 → **1.1** (E24).

Pourquoi logarithmique ? Entre 1.0 et 1.2, la frontière est √(1.0 × 1.2) ≈ 1.095 et non 1.1 : les valeurs normalisées sont régulièrement espacées en **rapport**, pas en différence.

## 10. Entrées et formats

- **Saisie** : `4.7n`, `4,7 nH`, `100p`, `2.2u` / `2.2µ`, `1.5k`, `2.4 GHz`, `50` sont acceptés (virgule décimale comprise, signe moins typographique « − » compris) ; `abc`, vide, `nan`, `inf` sont refusés.
- **Validation** : Z₀ > 0 ; f₀ entre 1 kHz et 1 THz ; Re(Z_s) > 0 ; Re(Z_L) > 0 (charge passive et non purement réactive) ; valeurs de composants > 0 ; tout fini.
- **Affichage** : `formatEng(3.898e-8, "H")` → `38.98 nH`. Les valeurs minuscules dues au bruit d'arrondi s'affichent en notation scientifique (`2.5e-16`), et une partie négligeable d'un nombre complexe s'écrit 0 (`50 + j0`).
- **Fichier projet** : voir `docs/ARCHITECTURE.md` §7 et `docs/USER_GUIDE.md`.

## 11. Où sont les tests ?

| Sujet | Fichier |
|---|---|
| Conversions, formats, lecture | `tests/test_units.cpp` |
| Γ, VSWR, RL, S11, mismatch, écran | `tests/test_rfmath.cpp` |
| Éléments, sens de déplacement, nœuds, charge, validation | `tests/test_elements_network.cpp` |
| E12 / E24 | `tests/test_eseries.cpp` |
| Solveur L / Pi / T | `tests/test_solver.cpp` |
| Balayage, bande passante, modèles de charge, CSV | `tests/test_sweep.cpp` |
| Fichier projet JSON | `tests/test_project.cpp` |
| Lecture/écriture de fichiers | `tests/test_fileio.cpp` |
