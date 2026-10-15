// =============================================================================
// NetworkPanel.hpp — Panneau NETWORK (mode manuel)
// =============================================================================
// Rôle : ajouter, éditer, réordonner et supprimer les éléments L/C du réseau.
// Chaque modification met à jour immédiatement l'abaque, les résultats, le
// balayage et le schéma (ils relisent tous le même Project à chaque image).
// =============================================================================
#pragma once

#include <vector>

#include "core/Project.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

class NetworkPanel {
public:
    // Dessine le panneau. Renvoie true si le projet a été modifié.
    bool draw(Project& project, bool learnMode);

    // Indice de l'élément survolé à l'image précédente (-1 = aucun) : l'abaque le met en valeur.
    int hoveredElement() const { return hovered_; }

private:
    std::vector<FieldState> fields_;   // un champ de saisie par élément
    int hovered_ = -1;
};

} // namespace smith::ui
