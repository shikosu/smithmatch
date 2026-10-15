// =============================================================================
// LearnText.hpp — Tous les textes pédagogiques de l'interface (mode « Learn »)
// =============================================================================
// Rôle : centraliser les phrases d'aide au même endroit, pour pouvoir les
// relire, les corriger ou les traduire facilement sans fouiller le code des vues.
// =============================================================================
#pragma once

#include <string>

#include "core/Element.hpp"
#include "core/Load.hpp"
#include "core/Types.hpp"

namespace smith::ui::learn {

// Petite légende sous l'abaque (orientation du diagramme).
const char* chartOrientationHint();

// Les grandeurs qui ont une infobulle de définition.
enum class Quantity {
    Z0, Frequency, SourceImpedance, LoadImpedance, ReflectionInput,
    ZIn, GammaM, Vswr, ReturnLoss, S11, MismatchLoss, ESeries, NominalQ, Quality
};

// Définition simple + formule (plusieurs lignes). Jamais nullptr.
const char* quantityTip(Quantity quantity);

// Explication de chaque modèle de charge.
const char* loadModelTip(LoadModel model);

// Une phrase qui explique ce que fait un élément du réseau sur l'abaque, par exemple :
// « Series L, 2.65 nH: adds +j40 Ω at 2.4 GHz → moves clockwise along the r = 0.5 circle. »
//  - zBefore : impédance vue juste AVANT cet élément (Ω)
std::string explainElement(const Element& element, double f0Hz, double z0, Complex zBefore);

// Jugement de qualité à partir du VSWR : « Good », « Acceptable » ou « Poor ».
const char* qualityLabel(double vswr);

} // namespace smith::ui::learn
