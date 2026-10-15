// =============================================================================
// ESeries.hpp — Valeurs normalisées E12 / E24
// =============================================================================
// Rôle : arrondir une valeur de composant à la valeur normalisée la plus proche
// (celles qu'on trouve vraiment dans le commerce). L'arrondi se fait en échelle
// LOGARITHMIQUE (CLAUDE.md §5.9) : on minimise |ln(v/s)|, car l'écart relatif
// compte plus que l'écart absolu.
// =============================================================================
#pragma once

#include "core/Element.hpp"

namespace smith {

enum class ESeriesKind { None, E12, E24 };

const char* eSeriesName(ESeriesKind kind);        // "None", "E12", "E24"
const char* eSeriesJsonName(ESeriesKind kind);    // "none", "E12", "E24"

// Valeur normalisée la plus proche (même unité SI que « value »).
// kind = None → renvoie « value » inchangée. value ≤ 0 ou non finie → inchangée.
// Exemples E12 : 4.9e-9 → 4.7e-9 ; 9.5e-12 → 1e-11 (passage de décade).
double nearestESeries(double value, ESeriesKind kind);

// Copie du réseau où chaque valeur est arrondie à la série choisie.
ElementList snapNetwork(const ElementList& network, ESeriesKind kind);

} // namespace smith
