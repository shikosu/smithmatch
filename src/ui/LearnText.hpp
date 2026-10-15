// =============================================================================
// LearnText.hpp — Tous les textes pédagogiques de l'interface (mode « Learn »)
// =============================================================================
// Rôle : centraliser les phrases d'aide au même endroit, pour pouvoir les
// relire, les corriger ou les traduire facilement sans fouiller le code des vues.
// =============================================================================
#pragma once

#include <string>

namespace smith::ui::learn {

// Petite légende sous l'abaque (orientation du diagramme).
const char* chartOrientationHint();

} // namespace smith::ui::learn
