// =============================================================================
// HelpWindows.hpp — Fenêtres « Quick guide » et « About » du menu Help
// =============================================================================
#pragma once

namespace smith::ui {

// Fenêtre d'aide rapide (résumé du guide utilisateur). « open » passe à false quand on la ferme.
void drawQuickGuide(bool& open);

// Boîte « About » : version, licence, crédits des bibliothèques. « open » comme ci-dessus.
void drawAbout(bool& open);

} // namespace smith::ui
