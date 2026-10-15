// =============================================================================
// FileDialogs.hpp — Boîtes de dialogue « Ouvrir / Enregistrer » natives
// =============================================================================
// Rôle : isoler ici tout ce qui dépend du système (Windows / macOS) pour les fichiers.
// Utilise nativefiledialog-extended. Les chemins sont en UTF-8.
// =============================================================================
#pragma once

#include <optional>
#include <string>

namespace smith::ui::platform {

// Demande un fichier à ouvrir. std::nullopt si l'utilisateur annule (ou en cas d'erreur).
//  - filterName / extensions : ex. "SmithMatch project", "json"
std::optional<std::string> openFileDialog(const std::string& filterName, const std::string& extensions);

// Demande où enregistrer. « defaultName » : nom proposé (ex. "project.smith.json").
// « requiredSuffix » (ex. ".smith.json") est ajouté au chemin s'il manque.
std::optional<std::string> saveFileDialog(const std::string& filterName, const std::string& extensions,
                                          const std::string& defaultName, const std::string& requiredSuffix);

// Nom de la touche de raccourci à afficher dans les menus : « Cmd+ » sur macOS, « Ctrl+ » ailleurs.
const char* shortcutModifierName();

} // namespace smith::ui::platform
