# Compiler SmithMatch — pas à pas

Ce guide part de zéro. Il suppose que tu as un **PC Windows** et/ou un **Mac Apple Silicon**, et **VS Code** comme éditeur.

> **Vocabulaire** : *CMake* est l'outil qui lit `CMakeLists.txt` et prépare la compilation (comme un chef de chantier qui écrit le plan). Le *compilateur* (MSVC sous Windows, Clang sous macOS) transforme le C++ en programme. Un *terminal* est la fenêtre où l'on tape des commandes.

La première configuration **télécharge automatiquement** les bibliothèques (ImGui, GLFW…) depuis GitHub : il faut donc une connexion Internet, et ça prend une à deux minutes.

---

## A. Windows

### A.1 Installer les outils (une seule fois)

1. **Git** : <https://git-scm.com/download/win> (options par défaut).
2. **CMake** (version ≥ 3.24) : <https://cmake.org/download/> → *Windows x64 Installer*. Dans l'installeur, coche **« Add CMake to the system PATH »**.
3. **Build Tools for Visual Studio 2022** : <https://visualstudio.microsoft.com/downloads/> → section *Tools for Visual Studio* → *Build Tools for Visual Studio 2022*. Dans l'installeur, coche la charge de travail **« Développement Desktop en C++ »** (*Desktop development with C++*). C'est ce qui installe le compilateur MSVC.
4. **VS Code** (si ce n'est pas déjà fait) puis les extensions **C/C++** (`ms-vscode.cpptools`) et **CMake Tools** (`ms-vscode.cmake-tools`). Le dépôt les recommande automatiquement : VS Code propose de les installer à l'ouverture du dossier.

### A.2 Récupérer le code

Dans un terminal (PowerShell) :

```powershell
git clone <adresse-du-dépôt> smithmatch
cd smithmatch
```

### A.3 Compiler

```powershell
cmake -S . -B build
cmake --build build --config Release --parallel
```

- La première commande **configure** (télécharge les bibliothèques, génère le projet Visual Studio dans `build/`).
- La seconde **compile** en mode Release (optimisé).

### A.4 Lancer l'application et les tests

```powershell
.\build\Release\SmithMatch.exe
ctest --test-dir build -C Release --output-on-failure
```

Tu dois voir à la fin de `ctest` : `100% tests passed, 0 tests failed`.

### A.5 Créer le paquet `.zip`

```powershell
cmake --build build --config Release --target package
```

Le fichier `build\SmithMatch-0.2.0-Windows-x64.zip` contient `SmithMatch.exe` et les licences. L'exécutable est **autonome** : le runtime Visual C++ et toutes les bibliothèques sont liés statiquement, aucune `.dll` n'est à fournir.

---

## B. macOS (Apple Silicon ou Intel)

### B.1 Installer les outils (une seule fois)

```bash
xcode-select --install          # compilateur Clang + Git (une fenêtre s'ouvre : accepter)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"   # Homebrew (si absent)
brew install cmake              # CMake ≥ 3.24
```

Puis VS Code et ses extensions **C/C++** et **CMake Tools** comme ci-dessus.

### B.2 Compiler

```bash
git clone <adresse-du-dépôt> smithmatch
cd smithmatch
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

### B.3 Lancer et tester

```bash
open build/SmithMatch.app
ctest --test-dir build -C Release --output-on-failure
```

### B.4 Créer le paquet `.dmg`

```bash
cmake --build build --config Release --target package
```

Le fichier `build/SmithMatch-0.2.0-macOS.dmg` est créé. Pour un binaire **universel** (Apple Silicon **et** Intel), configure avec :

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
```

(c'est ce que fait la CI GitHub).

---

## C. Avec VS Code (les deux systèmes)

1. *Fichier → Ouvrir le dossier…* → `smithmatch`.
2. Palette de commandes (`Ctrl/Cmd + Maj + P`) → **CMake: Select a Kit** → choisis *Visual Studio Build Tools 2022 Release - amd64* (Windows) ou *Clang* (macOS).
3. **CMake: Configure**, puis **CMake: Build** (ou F7), puis **CMake: Run Tests**.
4. Pour lancer l'appli : **CMake: Run Without Debugging** (Ctrl+F5).

---

## D. Options utiles

| Option | Effet |
|---|---|
| `-DSMITH_WARNINGS_AS_ERRORS=ON` | un warning dans `src/` ou `tests/` fait échouer la compilation (la CI l'utilise) |
| `-DSMITH_BUILD_APP=OFF` | ne compile que le moteur et les tests (pas d'interface, plus rapide) |
| `-DSMITH_BUILD_TESTS=OFF` | ne compile pas les tests |
| `-DSMITH_IMGUI_TAG=…` (et `SMITH_GLFW_TAG`, …) | essayer une autre version d'une bibliothèque (versions épinglées dans `cmake/Dependencies.cmake`) |

## E. Problèmes fréquents

| Symptôme | Cause probable | Solution |
|---|---|---|
| `CMake Error: ... version 3.24 or higher is required` | CMake trop ancien | installer une version récente (voir A.1 / B.1) |
| Erreur au moment de télécharger une dépendance | pas d'Internet, ou proxy | vérifier la connexion, relancer `cmake -S . -B build` |
| Windows : `No CMAKE_CXX_COMPILER could be found` | charge de travail C++ non installée | relancer l'installeur Build Tools et cocher *Développement Desktop en C++* |
| macOS : l'appli téléchargée est bloquée | appli non signée | voir le README (Réglages Système → Confidentialité et sécurité → *Ouvrir quand même*). Une appli compilée **chez toi** n'est pas concernée. |
| Fenêtre noire / erreur OpenGL au lancement | pilote graphique ancien (OpenGL 3.2 requis) | mettre à jour le pilote graphique |
| Après un `git pull`, erreurs étranges | ancien cache de configuration | supprimer le dossier `build/` et refaire A.3 / B.2 |

## F. Intégration continue (GitHub Actions)

Le fichier `.github/workflows/build.yml` compile, teste et empaquette sur `windows-latest` et `macos-latest` à chaque push et pull request ; les paquets sont disponibles dans l'onglet *Actions → (exécution) → Artifacts*. Pousser un **tag** `v*` (ex. `git tag v0.2.0 && git push origin v0.2.0`) crée en plus une *Release* GitHub avec le `.zip` et le `.dmg`.
