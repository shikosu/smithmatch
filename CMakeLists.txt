# =============================================================================
# CMakeLists.txt — Fichier de construction principal de SmithMatch
# =============================================================================
# CMake est un « générateur de projet » : il lit ce fichier et produit, selon
# la machine, un projet Visual Studio (Windows), des Makefiles ou Ninja (macOS,
# Linux). On obtient ainsi le même logiciel partout avec une seule description.
#
# Cibles produites :
#   smith_core   bibliothèque statique : le moteur de calcul (aucune dépendance graphique)
#   smithmatch   l'application (SmithMatch.exe / SmithMatch.app)
#   smith_tests  les tests unitaires (lancés par ctest)
# =============================================================================

cmake_minimum_required(VERSION 3.24)

# Version minimale de macOS visée. DOIT être définie avant project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(SmithMatch
    VERSION 0.2.0
    DESCRIPTION "Impedance matching with an interactive Smith chart"
    LANGUAGES C CXX)

# --- Options configurables (cmake -DOPTION=ON/OFF) ---------------------------
option(SMITH_BUILD_APP          "Build the graphical application"            ON)
option(SMITH_BUILD_TESTS        "Build the unit tests"                       ON)
option(SMITH_WARNINGS_AS_ERRORS "Treat warnings in src/ and tests/ as errors" OFF)

# --- Réglages généraux du C++ -------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)            # du C++17 standard, pas d'extensions GNU

# Windows : runtime C++ « statique » (/MT) intégré dans le .exe.
# Ainsi SmithMatch.exe se lance seul, sans installer de DLL Visual C++.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Windows : les fichiers sources sont en UTF-8 (caractères Ω, µ, Γ…).
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# Par défaut (générateurs mono-configuration), on compile en Release.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --- Dépendances tierces (téléchargées par FetchContent) ---------------------
include(Dependencies)

# -----------------------------------------------------------------------------
# smith_enable_warnings(<cible>)
# Active un niveau d'avertissements élevé sur NOTRE code uniquement.
# Règle du projet : zéro warning dans src/ et tests/.
# -----------------------------------------------------------------------------
function(smith_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(SMITH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        # -Wconversion / -Wsign-conversion : détectent les conversions
        # implicites (double → float, int → size_t…), comme /W4 sous MSVC.
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
        if(SMITH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# =============================================================================
# smith_core — moteur de calcul pur (bibliothèque standard + nlohmann/json)
# =============================================================================
# Règle n°4 du projet : aucune dépendance graphique ici (ni ImGui, ni GLFW).
add_library(smith_core STATIC
    src/core/Units.cpp
    src/core/RfMath.cpp
    src/core/Element.cpp
    src/core/Load.cpp
    src/core/Network.cpp
    src/core/ESeries.cpp
    src/core/Validation.cpp
    src/core/Project.cpp
    src/core/Solver.cpp
    src/core/Sweep.cpp)
target_include_directories(smith_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Seule dépendance externe autorisée : nlohmann/json (bibliothèque d'en-têtes).
target_link_libraries(smith_core PUBLIC nlohmann_json::nlohmann_json)
smith_enable_warnings(smith_core)

# =============================================================================
# smithmatch — l'application graphique
# =============================================================================
if(SMITH_BUILD_APP)
    include(EmbedResource)

    add_executable(smithmatch WIN32 MACOSX_BUNDLE
        src/main.cpp
        src/ui/App.cpp
        src/ui/Theme.cpp
        src/ui/LearnText.cpp
        src/ui/Widgets.cpp
        src/ui/SetupPanel.cpp
        src/ui/ResultsPanel.cpp
        src/ui/NetworkPanel.cpp
        src/ui/AutoMatchPanel.cpp
        src/ui/SweepPanel.cpp
        src/ui/SmithChartView.cpp)

    # Polices intégrées dans l'exécutable (voir cmake/EmbedResource.cmake).
    smith_embed_resource(smithmatch
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Inter-Regular.ttf  kInterRegularTtf)
    smith_embed_resource(smithmatch
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/Inter-SemiBold.ttf kInterSemiBoldTtf)

    target_include_directories(smithmatch PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(smithmatch PRIVATE smith_core imgui implot glfw nfd stb OpenGL::GL)
    target_compile_definitions(smithmatch PRIVATE
        SMITH_VERSION="${PROJECT_VERSION}")
    smith_enable_warnings(smithmatch)

    # Nom du fichier produit : SmithMatch.exe / SmithMatch.app, placé dans build/.
    set_target_properties(smithmatch PROPERTIES
        OUTPUT_NAME SmithMatch
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    if(MSVC)
        # Application « fenêtrée » (pas de console noire) mais on garde main()
        # comme point d'entrée au lieu de WinMain().
        target_link_options(smithmatch PRIVATE /ENTRY:mainCRTStartup)
    endif()

    if(APPLE)
        # Bundle macOS : SmithMatch.app avec son Info.plist.
        set_target_properties(smithmatch PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in
            MACOSX_BUNDLE_GUI_IDENTIFIER        eu.teovidal.smithmatch
            MACOSX_BUNDLE_BUNDLE_NAME           SmithMatch
            MACOSX_BUNDLE_BUNDLE_VERSION        ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING  ${PROJECT_VERSION}
            MACOSX_BUNDLE_COPYRIGHT             "Copyright (c) 2026 Téo Vidal")
    endif()
endif()

# =============================================================================
# smith_tests — tests unitaires du moteur de calcul (doctest)
# =============================================================================
if(SMITH_BUILD_TESTS)
    enable_testing()
    add_executable(smith_tests
        tests/test_main.cpp
        tests/test_units.cpp
        tests/test_rfmath.cpp
        tests/test_elements_network.cpp
        tests/test_eseries.cpp
        tests/test_solver.cpp
        tests/test_sweep.cpp)
    target_link_libraries(smith_tests PRIVATE smith_core doctest::doctest)
    smith_enable_warnings(smith_tests)
    # Une seule entrée ctest qui exécute tous les cas de test doctest.
    add_test(NAME smith_tests COMMAND smith_tests)
endif()
