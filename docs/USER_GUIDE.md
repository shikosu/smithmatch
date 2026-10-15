# Guide d'utilisation de SmithMatch

SmithMatch t'aide à **adapter une charge à une source** avec des bobines (L) et des condensateurs (C), en voyant ce que chaque composant fait sur l'**abaque de Smith**.

> **Analogie** : adapter une antenne, c'est comme régler la pente d'une rampe entre deux trottoirs de hauteurs différentes. Si la rampe est mal réglée, la balle (la puissance) rebondit (réflexion). L'abaque de Smith est la carte qui montre, pour chaque composant ajouté, où l'on arrive sur la rampe.

## 1. Les zones de l'écran

```
┌───────────────┬───────────────────────────┬──────────────────────┐
│ SETUP         │                           │ NETWORK (manual)     │
│ RESULTS       │      ABAQUE DE SMITH      │ AUTO-MATCH (solver)  │
├───────────────┴────────────┬──────────────┴──────────────────────┤
│ SWEEP : |S11| et VSWR      │ SCHEMATIC : le schéma électrique    │
└────────────────────────────┴─────────────────────────────────────┘
```

Les séparateurs entre les zones se tirent à la souris. En bas à gauche de la fenêtre : la barre d'état (messages, erreurs en rouge, nom du fichier, « * » si modifié).

## 2. Exemple complet : une antenne 25 − j15 Ω à 2,4 GHz

C'est le projet qui s'affiche au démarrage : une antenne Wi-Fi dont l'impédance est Z_L = 25 − j15 Ω à f₀ = 2,4 GHz, reliée à un circuit de 50 Ω.

### Étape 1 — Lire le problème

Dans **SETUP** : Z0 = 50 Ω, f0 = 2.4 GHz, source Zs = 50 + j0 Ω, charge ZL = 25 − j15 Ω.

Dans **RESULTS**, avant tout composant :

| Grandeur | Valeur |
|---|---|
| Zin | 25 − j15 Ω |
| \|Γm\| | 0.3812 |
| VSWR | 2.232 |
| Return loss | 8.377 dB |
| Qualité | **Poor** (rouge) |

Sur l'abaque, le point rouge **ZL** est dans la moitié basse (capacitif), à gauche de la croix blanche **target** : le centre est la cible (50 Ω). Passe la souris sur l'abaque : une infobulle donne l'impédance, l'admittance et Γ sous le curseur.

### Étape 2 — Adapter à la main (NETWORK)

Objectif : amener le point **Zin** (jaune) sur la croix **target**. Idée : un **L série** *monte* le point (il ajoute +jX, on tourne en sens horaire sur le cercle de résistance constante), un **C shunt** le *fait tourner* sur un cercle de conductance constante.

1. Clique **+ Series L** (le premier élément est toujours côté charge). Règle sa valeur à **2.6526 nH** (tape `2.6526n` dans le champ, ou tire le curseur).
   - Le trajet orange part de ZL et monte jusqu'à 25 + j25 Ω. La ligne « after » du panneau l'indique.
2. Clique **+ Shunt C** et tape **1.3263p**.
   - Le trajet vert redescend sur le centre : **Zin = 50 + j0 Ω, VSWR = 1** (RESULTS passe au vert, **Good**).

Le schéma en bas à droite montre le circuit : source à gauche, **C** vers la masse, **L** en série, charge à droite.

> **Mode Learn** (menu *View → Learn mode*, activé par défaut) : des infobulles et une phrase sous chaque élément expliquent ce qu'il fait sur l'abaque (« adds +j40 Ω at 2.4 GHz → moves clockwise along the r = 0.5 circle »).

### Étape 3 — Laisser le solveur trouver les solutions (AUTO-MATCH)

1. Efface le réseau (*Clear network*).
2. Dans **AUTO-MATCH**, garde la topologie **L**. Le solveur propose **2 solutions** :
   - **#1 Low-pass** : Series L 2.653 nH → Shunt C 1.326 pF
   - **#2 High-pass** : Series C 6.631 pF → Shunt L 3.316 nH
3. **Passe la souris** sur une solution : son trajet s'affiche en **pointillés blancs** sur l'abaque (aperçu).
4. Clique **Apply** : le réseau est copié dans NETWORK (une confirmation est demandée si un réseau existait déjà).

Choisis **Pi** ou **T** pour avoir un degré de liberté en plus : le **Q nominal**. Un Q plus grand donne une bande plus étroite ; il doit être supérieur à **Q_min**, affiché sous le champ (sinon un message rouge l'explique).

### Étape 4 — Regarder la bande passante (SWEEP)

Le panneau **SWEEP** trace |S11| (courbe bleue, axe de gauche, en dB) et le **VSWR** (courbe orange, axe de droite) de f₀/2 à 2·f₀. La zone **verte** est la **bande passante** : là où le VSWR reste sous la limite (2 par défaut, champ *VSWR max*). Avec la solution #1, elle va d'environ **1,617 à 3,035 GHz** (1,42 GHz de large). La ligne blanche verticale marque f₀.

Réglages : *Start*, *Stop*, *Points*, échelle *Lin/Log*, *VSWR max*. Active **View → Sweep locus on chart** pour voir sur l'abaque la boucle que dessine Zin quand la fréquence varie (elle passe par le centre à f₀).

### Étape 5 — Passer aux vrais composants (série E)

Dans **SETUP**, choisis **E12** (par défaut) ou **E24**. Chaque valeur est arrondie à la valeur normalisée la plus proche : 2.653 nH → **2.7 nH**, 1.326 pF → **1.2 pF**. **RESULTS** affiche deux colonnes (valeurs idéales / *With E12*) : avec ces valeurs, Zin = 50.97 + j4.973 Ω et le VSWR passe de 1 à **1.106** — c'est encore très bon. Sur SWEEP, la courbe E12 (plus pâle) diffère légèrement de la courbe idéale. *Snap all to E-series* remplace les valeurs du réseau par les valeurs normalisées.

## 3. Référence des panneaux

### SETUP
| Champ | Rôle |
|---|---|
| Z0 | impédance de référence de l'abaque (50 Ω par défaut) |
| Frequency f0 | fréquence centrale d'adaptation |
| Source Zs R / jX | impédance de la source (la cible est son conjugué) |
| Load ZL R / jX | impédance de la charge **à f₀** |
| ZL as \|Γ\| / arg Γ | la même charge, saisie par son point sur l'abaque |
| Load model | comment la charge varie avec la fréquence : *Constant*, *Series equivalent* (R + L ou C en série, **par défaut**), *Parallel equivalent* |
| E-series | None, E12, E24 |

Les champs acceptent la **notation ingénieur** : `2.4G`, `433M`, `4.7n`, `4,7 nH`, `100p`, `2.2u`, `1.5k`. Une saisie invalide entoure le champ de **rouge**, affiche un message, et la dernière valeur valide est conservée.

### RESULTS
Zin, zin = Zin/Z0, |Γm|, arg Γm, VSWR, return loss, |S11|, mismatch loss, qualité (**Good** VSWR ≤ 1.5, **Acceptable** ≤ 2, **Poor** au-delà). Survole un nom de ligne (mode Learn) pour une explication.

### NETWORK (mode manuel)
Boutons *+ Series L / + Series C / + Shunt L / + Shunt C* ; pour chaque élément : valeur, curseur logarithmique, flèches ↑ ↓ pour **changer l'ordre** (↑ = vers la charge), × pour supprimer. L'élément n° 1 est **le plus proche de la charge**. Survoler un élément le met en valeur sur l'abaque.

### AUTO-MATCH
Choix **L / Pi / T**, Q nominal (Pi, T), puis liste de solutions : type (Low-pass, High-pass, Mixed), valeurs, VSWR (idéal et E-series), **bande passante**, bouton **Apply**. Messages possibles : « Load is already matched », « No L-network exists… Try a Pi or T network », « Q must be greater than Q_min = … ».

### SWEEP et SCHEMATIC
Voir l'étape 4 ci-dessus. Le schéma met en valeur l'élément survolé (et son arc sur l'abaque) et affiche aussi la valeur normalisée si une série est choisie.

## 4. Fichiers

| Action | Menu | Raccourci |
|---|---|---|
| Nouveau projet | File → New | Ctrl/Cmd + N |
| Ouvrir | File → Open… | Ctrl/Cmd + O |
| Enregistrer | File → Save | Ctrl/Cmd + S |
| Enregistrer sous | File → Save As… | Ctrl/Cmd + Maj + S |
| Exporter l'abaque en image | File → Export Smith chart as PNG… | |
| Exporter le balayage | File → Export sweep as CSV… | |

- Les projets sont des fichiers texte **`.smith.json`** (lisibles, versionnés avec le champ `"version"`). Un fichier d'une version plus récente que le programme est refusé avec un message clair, un fichier abîmé aussi (sans plantage).
- Si tu quittes ou ouvres un autre fichier avec des modifications non enregistrées, la boîte **Unsaved changes** propose *Save / Discard / Cancel*.
- Le **CSV** contient `f_hz,zin_re_ohm,zin_im_ohm,gamma_mag,gamma_deg,s11_db,vswr` (séparateur virgule, point décimal) : il s'ouvre dans un tableur (en cas de problème, utilise *Données → Importer* et choisis la virgule comme séparateur).
- Le **PNG** reproduit l'abaque tel qu'affiché (sans la croix de survol), à la résolution de ton écran (×2 sur écran Retina).

Exemple de fichier minimal :

```json
{
  "format": "smithmatch-project",
  "version": 1,
  "z0_ohm": 50,
  "f0_hz": 2.4e9,
  "source": { "r_ohm": 50, "x_ohm": 0 },
  "load": { "r_ohm": 25, "x_ohm": -15, "model": "series_equivalent" },
  "network": [
    { "type": "series_inductor", "value": 2.6526e-9 },
    { "type": "shunt_capacitor", "value": 1.3263e-12 }
  ],
  "sweep": { "f_min_hz": 1.2e9, "f_max_hz": 4.8e9, "points": 401, "log_scale": false, "vswr_threshold": 2.0 },
  "e_series": "E12"
}
```

Les champs inconnus sont ignorés ; un champ absent prend sa valeur par défaut. Types d'éléments : `series_inductor`, `series_capacitor`, `shunt_inductor`, `shunt_capacitor` (valeurs en H ou F).

## 5. Astuces et limites

- **Pas de solution en L ?** Certaines charges ne s'adaptent pas avec seulement deux éléments : essaie **Pi** ou **T**.
- **Pourquoi plusieurs solutions ?** Elles sont toutes valides à f₀ ; elles diffèrent par le **comportement hors de f₀** (passe-bas ou passe-haut, bande passante, sensibilité aux tolérances).
- **Charge qui varie avec la fréquence** : le modèle choisi dans SETUP change les courbes de SWEEP, pas le calcul à f₀.
- **Limites de la v0.2** : composants idéaux (pas de pertes ni de Q fini), pas de résistances, lignes ou stubs, pas d'import Touchstone, pas d'annuler/rétablir, pas de glisser-déposer sur l'abaque. Ces fonctions sont prévues pour une version ultérieure.
- Une **valeur négative ou nulle** de composant est refusée ; Re(ZL) doit être > 0.

## 6. Aide intégrée

Menu **Help → Quick guide** (résumé de ce guide) et **Help → About** (version, licence, crédits).
