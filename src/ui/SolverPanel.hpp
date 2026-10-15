// =============================================================================
// SolverPanel.hpp — Panneau AUTO-MATCH (solveur automatique L / Pi / T)
// =============================================================================
// Rôle : choisir la topologie (L, Pi ou T), régler le Q nominal pour Pi/T, lister
// les solutions du solveur (core/Solver) avec leur qualité d'adaptation, aperçu
// en pointillés sur l'abaque au survol, et bouton « Apply » qui copie la solution
// dans le réseau du projet (avec confirmation si le réseau n'est pas vide).
// =============================================================================
#pragma once

#include "core/Element.hpp"
#include "core/Project.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

class SolverPanel {
public:
    // Dessine le panneau. Renvoie true si le projet a été modifié (solution appliquée).
    bool draw(Project& project, bool learnMode);

    // Réseau à montrer en pointillés sur l'abaque (solution survolée) ; vide = aucun aperçu.
    const ElementList& previewNetwork() const { return preview_; }

private:
    enum class Topology { L, Pi, T };

    Topology topology_ = Topology::L;
    double q_ = 3.0;                 // Q nominal demandé (Pi / T)
    FieldState qField_;
    ElementList preview_;            // solution survolée à l'image précédente
    ElementList pending_;            // solution en attente de confirmation
    bool askConfirmation_ = false;   // ouvrir la fenêtre de confirmation à cette image
};

} // namespace smith::ui
