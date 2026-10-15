// =============================================================================
// FileIo.hpp — Lecture / écriture de fichiers texte (UTF-8)
// =============================================================================
// Rôle : charger et enregistrer un fichier entier, avec des erreurs renvoyées sous
// forme de message (pas d'exception). Les chemins sont en UTF-8, y compris sous
// Windows (accents dans le nom d'un dossier, par exemple).
// =============================================================================
#pragma once

#include <string>

namespace smith {

struct TextFileResult {
    bool ok = false;
    std::string message;   // raison de l'échec si ok = false
    std::string text;      // contenu lu (readTextFile)
};

// Lit tout le fichier.
TextFileResult readTextFile(const std::string& pathUtf8);

// Écrit (ou remplace) le fichier. On écrit d'abord dans un fichier temporaire voisin puis on
// le renomme : si l'écriture échoue en cours de route, l'ancien fichier reste intact.
TextFileResult writeTextFile(const std::string& pathUtf8, const std::string& text);

} // namespace smith
