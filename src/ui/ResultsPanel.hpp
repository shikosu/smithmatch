// =============================================================================
// ResultsPanel.hpp — Panneau RESULTS : Zin, Γ, VSWR, return loss, S11…
// =============================================================================
#pragma once

#include "core/Project.hpp"

namespace smith::ui {

// Affiche les grandeurs d'adaptation à f₀ pour le réseau courant (valeurs idéales)
// et, si une série E est choisie, avec les valeurs arrondies.
void drawResultsPanel(const Project& project, bool learnMode);

} // namespace smith::ui
