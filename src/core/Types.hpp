// =============================================================================
// Types.hpp — Types de base et constantes nommées de SmithMatch
// =============================================================================
// Rôle : centraliser le type « nombre complexe » et TOUTES les constantes
// numériques du projet (règle n°6 : aucune valeur magique dans le code).
//
// Unités : tout est en SI (Ω, S, H, F, Hz). Les préfixes (nH, pF, GHz…)
// n'apparaissent que dans l'affichage et la saisie (voir Units.hpp).
// =============================================================================
#pragma once

#include <complex>

namespace smith {

// Un nombre complexe en double précision : Z = R + jX, Y = G + jB, Γ…
// (en électronique on écrit « j » ; en C++ c'est std::complex<double>).
using Complex = std::complex<double>;

// --- Constantes mathématiques ------------------------------------------------
inline constexpr double kPi    = 3.14159265358979323846;
inline constexpr double kTwoPi = 2.0 * kPi;

// --- Identité de l'application -----------------------------------------------
inline constexpr const char* kAppName    = "SmithMatch";
inline constexpr const char* kAppVersion = "0.2.0";

} // namespace smith
