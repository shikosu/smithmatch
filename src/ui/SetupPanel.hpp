// =============================================================================
// SetupPanel.hpp — Panneau SETUP : Z0, f0, source, charge, série E
// =============================================================================
#pragma once

#include "core/Project.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

class SetupPanel {
public:
    // Dessine le panneau. Renvoie true si le projet a été modifié.
    // learnMode : affiche les infobulles pédagogiques.
    bool draw(Project& project, bool learnMode);

private:
    FieldState z0_, f0_, zsR_, zsX_, zlR_, zlX_, gammaMag_, gammaAngle_;
};

} // namespace smith::ui
