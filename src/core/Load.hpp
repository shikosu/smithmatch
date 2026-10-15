// =============================================================================
// Load.hpp — Modèle de la charge (comment Z_L varie avec la fréquence)
// =============================================================================
// Rôle : l'utilisateur donne Z_L à la fréquence centrale f₀. Pour un balayage,
// il faut savoir comment elle évolue ailleurs : c'est le « modèle de charge »
// (CLAUDE.md §5.8). La source Z_s, elle, est constante en fréquence.
// =============================================================================
#pragma once

#include "core/Types.hpp"

namespace smith {

enum class LoadModel {
    Constant,            // Z_L(f) = Z_L pour toute fréquence
    SeriesEquivalent,    // R en série avec L (si X > 0) ou C (si X < 0), calculés à f₀
    ParallelEquivalent   // G en parallèle avec C (si B > 0) ou L (si B < 0), calculés à f₀
};
// v0.3 : Touchstone (table Z(f) issue d'un fichier .s1p)

struct LoadSpec {
    Complex zAtF0 {kDefaultLoadR, kDefaultLoadX};   // impédance à f₀ (Ω)
    LoadModel model = LoadModel::SeriesEquivalent;
};

const char* loadModelName(LoadModel model);       // "Constant", "Series equivalent", …
const char* loadModelJsonName(LoadModel model);   // "constant", "series_equivalent", …

// Impédance de la charge à la fréquence fHz (f₀ = fréquence où zAtF0 est donnée).
//   Constant    : Z_L(f) = Z_L
//   Série       : R + jX(f), avec X(f) = X₀·f/f₀ si X₀ > 0 (bobine : X = ωL)
//                                      X(f) = X₀·f₀/f si X₀ < 0 (condensateur : X = −1/(ωC))
//   Parallèle   : 1/(G + jB(f)), avec B(f) = B₀·f/f₀ si B₀ > 0 (condensateur : B = ωC)
//                                       B(f) = B₀·f₀/f si B₀ < 0 (bobine : B = −1/(ωL))
// À f = f₀, renvoie exactement zAtF0.
Complex loadImpedanceAt(const LoadSpec& load, double f0Hz, double fHz);

} // namespace smith
