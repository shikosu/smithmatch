# =============================================================================
# Dependencies.cmake — Téléchargement et configuration des bibliothèques tierces
# =============================================================================
# Rôle : récupérer chaque dépendance avec FetchContent (CMake la télécharge
# depuis GitHub au moment de la configuration), toujours sur un tag de release
# PRÉCIS. Ainsi, tout le monde (toi, la CI, un contributeur) compile exactement
# les mêmes versions : c'est reproductible.
#
# Analogie : c'est comme une nomenclature (BOM) de carte électronique avec la
# référence exacte de chaque composant, au lieu de « un régulateur 5 V quelconque ».
#
# Versions vérifiées le 2026-10-15 (tags existants, compilation testée) :
#   - Dear ImGui   v1.92.9b  (branche master, PAS docking). La v1.93.0 existe
#                            (sortie le 2026-10-14) mais elle avait un jour :
#                            on garde la dernière 1.92 éprouvée.
#   - ImPlot       commit 851cdbd (master, 2026-10-10) : le tag v1.0 ne compile
#                            pas avec ImGui ≥ 1.92.8 (voir plus bas).
#   - GLFW         3.5.1     (2026-07-31)
#   - nlohmann/json v3.12.0  (2025-04-11)
#   - nativefiledialog-extended v1.4.1 (2026-10-01)
#   - doctest      v2.5.3    (2026-07-07)
#   - stb          commit 2c980bb (2026-08-01) : le dépôt stb n'a pas de tags,
#                            on épingle donc un commit précis (hash complet).
# =============================================================================

include(FetchContent)

# Affiche la progression des téléchargements (utile la première fois : c'est long).
set(FETCHCONTENT_QUIET OFF)

# --- Politique CMake pour le runtime MSVC -------------------------------------
# Certaines dépendances déclarent une vieille version minimale de CMake. Sans
# cette ligne, elles ignoreraient CMAKE_MSVC_RUNTIME_LIBRARY et on aurait un
# mélange de runtimes (/MT et /MD) → erreur d'édition de liens LNK2038.
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

# Les dépendances sont des bibliothèques statiques (un seul .exe autonome).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# --- Tags / commits épinglés --------------------------------------------------
# Ce sont des variables de cache : on peut les surcharger pour essayer une autre
# version sans modifier ce fichier, ex. : cmake -B build -DSMITH_IMGUI_TAG=v1.93.0
#
# ImPlot : le tag v1.0 utilise l'ancienne signature de ImDrawList::AddPolyline()
# (paramètres « thickness » et « flags » inversés dans ImGui 1.92.8) et NE COMPILE
# PAS avec ImGui 1.92.9b. On épingle donc le commit 851cdbd de la branche master
# (2026-10-10), qui contient le correctif — vérifié en compilant.
set(SMITH_IMGUI_TAG   "v1.92.9b"  CACHE STRING "Dear ImGui tag (master branch, not docking)")
set(SMITH_IMPLOT_TAG  "851cdbdd60d1dfb64c23e987969694de0f547a6b" CACHE STRING "ImPlot commit")
set(SMITH_GLFW_TAG    "3.5.1"     CACHE STRING "GLFW tag")
set(SMITH_JSON_TAG    "v3.12.0"   CACHE STRING "nlohmann/json tag")
set(SMITH_NFD_TAG     "v1.4.1"    CACHE STRING "nativefiledialog-extended tag")
set(SMITH_DOCTEST_TAG "v2.5.3"    CACHE STRING "doctest tag")
set(SMITH_STB_COMMIT  "2c980bb59875b0d32144a71867fbdebb2f77cd20" CACHE STRING "stb commit")

# =============================================================================
# 1) nlohmann/json — lecture/écriture JSON (utilisé par src/core/Project)
# =============================================================================
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        ${SMITH_JSON_TAG}
    GIT_SHALLOW    TRUE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# =============================================================================
# 2) doctest — tests unitaires (en-tête unique)
# =============================================================================
if(SMITH_BUILD_TESTS)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        ${SMITH_DOCTEST_TAG}
        GIT_SHALLOW    TRUE)
    set(DOCTEST_WITH_TESTS               OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB  OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL               ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(doctest)
endif()

# Tout ce qui suit ne sert qu'à l'application graphique.
if(NOT SMITH_BUILD_APP)
    return()
endif()

# =============================================================================
# 3) GLFW — création de la fenêtre et du contexte OpenGL, clavier, souris
# =============================================================================
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        ${SMITH_GLFW_TAG}
    GIT_SHALLOW    TRUE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
# Runtime MSVC statique aussi pour GLFW (sinon GLFW force /MD).
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
# Linux (développement uniquement) : X11 suffit, pas besoin de Wayland.
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(glfw)

# =============================================================================
# 4) Dear ImGui — l'interface (pas de CMakeLists.txt fourni : on crée la cible)
# =============================================================================
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        ${SMITH_IMGUI_TAG}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
# SYSTEM : les avertissements des en-têtes ImGui ne « polluent » pas notre code.
target_include_directories(imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
target_compile_features(imgui PUBLIC cxx_std_17)
# On ne veut pas d'API obsolète (garantit qu'on écrit du code à jour).
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
if(APPLE)
    # macOS a déclaré OpenGL « déprécié » : on coupe ces avertissements.
    target_compile_definitions(imgui PUBLIC GL_SILENCE_DEPRECATION)
endif()

# =============================================================================
# 5) ImPlot — courbes |S11| et VSWR (même principe : on crée la cible)
# =============================================================================
FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        ${SMITH_IMPLOT_TAG})
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp)
target_include_directories(implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# =============================================================================
# 6) nativefiledialog-extended — boîtes « Ouvrir / Enregistrer » natives de l'OS
# =============================================================================
FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG        ${SMITH_NFD_TAG}
    GIT_SHALLOW    TRUE
    # Le sous-module « wayland-protocols » ne sert qu'à Linux/Wayland : inutile ici.
    GIT_SUBMODULES "")
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NFD_INSTALL     OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(NFD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(nfd)

# =============================================================================
# 7) stb — uniquement stb_image_write.h (écriture des PNG), en-tête unique
# =============================================================================
# Pas de GIT_SHALLOW ici (ni pour ImPlot) : un clone superficiel ne fonctionne
# qu'avec un tag ou une branche, pas avec un hash de commit.
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        ${SMITH_STB_COMMIT})
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${stb_SOURCE_DIR})
