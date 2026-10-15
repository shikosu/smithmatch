# =============================================================================
# Packaging.cmake — Création des paquets distribuables avec CPack
# =============================================================================
#   macOS   : SmithMatch-<version>-macOS.dmg        (image disque, générateur « DragNDrop »)
#   Windows : SmithMatch-<version>-Windows-x64.zip  (SmithMatch.exe + licences)
#   Linux   : .tar.gz (uniquement pour tester la chaîne de packaging pendant le développement)
#
# Commande : cmake --build build --config Release --target package
# Le résultat est déposé dans build/ (ex. build/SmithMatch-0.2.0-Windows-x64.zip).
#
# L'application est liée statiquement (bibliothèques tierces, et runtime Visual C++ sous
# Windows) : aucun fichier .dll supplémentaire n'est à livrer.
# =============================================================================

if(NOT SMITH_BUILD_APP)
    return()
endif()

# --- Ce qui est installé (donc empaqueté) ------------------------------------------------------
# BUNDLE = SmithMatch.app (macOS) ; RUNTIME = SmithMatch.exe (Windows) ou binaire (Linux).
install(TARGETS smithmatch
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION .)

# Licences et lisez-moi, à côté de l'application (si les fichiers existent).
foreach(doc LICENSE THIRD_PARTY_LICENSES.md README.md)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${doc}")
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${doc}" DESTINATION .)
    endif()
endforeach()

# --- Réglages communs CPack ------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME            "SmithMatch")
set(CPACK_PACKAGE_VENDOR          "Téo Vidal")
set(CPACK_PACKAGE_VERSION         "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Impedance matching with an interactive Smith chart")
set(CPACK_RESOURCE_FILE_LICENSE   "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DIRECTORY       "${CMAKE_BINARY_DIR}")
set(CPACK_OUTPUT_FILE_PREFIX      "${CMAKE_BINARY_DIR}")

# --- Réglages par système ------------------------------------------------------------------------------------
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDZO")                  # image compressée
    set(CPACK_DMG_VOLUME_NAME "SmithMatch")
    set(CPACK_PACKAGE_FILE_NAME "SmithMatch-${PROJECT_VERSION}-macOS")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "SmithMatch-${PROJECT_VERSION}-Windows-x64")
else()
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "SmithMatch-${PROJECT_VERSION}-Linux-dev")
endif()

include(CPack)
