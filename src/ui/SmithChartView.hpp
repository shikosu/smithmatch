// =============================================================================
// SmithChartView.hpp — L'abaque de Smith interactif
// =============================================================================
// Rôle : dessiner l'abaque (grilles, repères, trajet du réseau, cercle VSWR,
// lieu du balayage) et afficher une infobulle au survol de la souris.
//
// Cette classe ne CALCULE rien de la physique : elle reçoit une « scène »
// (ChartScene) déjà préparée et se contente de la dessiner. Toutes les
// coordonnées sont des Γ (nombres complexes) transformés en pixels par
// gammaToScreen() de core/RfMath.
// =============================================================================
#pragma once

#include <imgui.h>

#include <array>
#include <vector>

#include "core/Element.hpp"
#include "core/Types.hpp"

namespace smith::ui {

// Ce qu'il faut dessiner (rempli par l'application à chaque image).
struct ChartScene {
    double z0 = kDefaultZ0Ohm;
    Complex zSource {kDefaultSourceR, kDefaultSourceX};   // la cible est son conjugué
    Complex zLoad {kDefaultLoadR, kDefaultLoadX};          // Z_L à f₀
    double f0Hz = kDefaultF0Hz;
    ElementList network;                  // réseau courant (indice 0 côté charge)
    ElementList previewNetwork;           // aperçu du solveur (tirets) ; vide = aucun
    std::vector<Complex> sweepLocus;      // Γ(Z_in(f)) du balayage ; vide = aucun
    std::size_t sweepF0Index = 0;         // indice du point f₀ dans sweepLocus
    int highlightedElement = -1;          // élément à mettre en valeur (-1 = aucun)
};

// Options d'affichage (menu View).
struct ChartOptions {
    bool admittanceGrid = false;
    bool vswrCircle = true;
    bool sweepLocus = false;
    bool learnMode = true;
};

class SmithChartView {
public:
    SmithChartView();

    // Dessine l'abaque dans tout l'espace disponible de la fenêtre ImGui courante
    // (le cercle est carré et centré ; les étiquettes tiennent dans la marge).
    void draw(const ChartScene& scene, const ChartOptions& options);

    // Rectangle occupé par l'abaque à l'écran à la dernière image (coordonnées ImGui),
    // utilisé pour l'export PNG.
    ImVec2 rectMin() const { return rectMin_; }
    ImVec2 rectMax() const { return rectMax_; }

private:
    // Courbes de grille, calculées une fois pour toutes dans le plan Γ (grille d'IMPÉDANCE).
    // La grille d'admittance est son symétrique par rapport à l'origine (Γ → −Γ).
    static constexpr std::size_t kGridCount = 5;
    std::array<std::vector<Complex>, kGridCount> resistanceCircles_;   // cercles r = const
    std::array<std::vector<Complex>, kGridCount> reactanceArcsPos_;    // arcs x = +const
    std::array<std::vector<Complex>, kGridCount> reactanceArcsNeg_;    // arcs x = −const

    std::vector<ImVec2> scratch_;    // tampon réutilisé pour convertir Γ → pixels
    ImVec2 rectMin_ {0.0f, 0.0f};
    ImVec2 rectMax_ {0.0f, 0.0f};
};

} // namespace smith::ui
