// =============================================================================
// SweepPanel.hpp — Panneau SWEEP : réglages du balayage + courbes |S11| et VSWR
// =============================================================================
// Rôle : régler la plage de fréquences, le nombre de points, l'échelle et le seuil de
// VSWR, puis tracer (ImPlot) |S11| en dB (axe gauche) et le VSWR (axe droit) en fonction
// de la fréquence. Affiche f₀, le seuil et la bande passante. Si une série E est choisie,
// les courbes « valeurs arrondies » sont ajoutées en transparence.
// Les calculs (runSweep / bandwidth) sont faits par l'application, ce panneau ne fait
// que les afficher.
// =============================================================================
#pragma once

#include <vector>

#include "core/Project.hpp"
#include "core/Sweep.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

class SweepPanel {
public:
    // Dessine le panneau. « ideal » : balayage du réseau courant ; « rounded » : balayage du
    // réseau arrondi à la série E (nullptr si aucune série). Renvoie true si le projet a été modifié.
    bool draw(Project& project, const SweepResult& ideal, const SweepResult* rounded, bool learnMode);

private:
    FieldState fMin_, fMax_, threshold_;
    // Tampons réutilisés pour fournir des tableaux contigus à ImPlot.
    std::vector<double> xs_, s11_, vswr_, xsRounded_, s11Rounded_, vswrRounded_;
};

} // namespace smith::ui
