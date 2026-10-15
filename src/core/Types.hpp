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

// Un point 2D en pixels (l'axe Y de l'écran pointe vers le BAS).
// Défini ici pour que core/ n'ait pas besoin d'ImGui (règle n°4).
struct Point2 {
    double x = 0.0;
    double y = 0.0;
};

// --- Constantes mathématiques ------------------------------------------------
inline constexpr double kPi    = 3.14159265358979323846;
inline constexpr double kTwoPi = 2.0 * kPi;

// Pulsation ω = 2πf (rad/s) à partir de la fréquence f (Hz).
inline constexpr double angularFrequency(double fHz) { return kTwoPi * fHz; }

// --- Identité de l'application -----------------------------------------------
inline constexpr const char* kAppName    = "SmithMatch";
inline constexpr const char* kAppVersion = "0.2.0";

// --- Valeurs par défaut du projet (exemple de départ : antenne 25 − j15 Ω) ---
inline constexpr double kDefaultZ0Ohm       = 50.0;
inline constexpr double kDefaultF0Hz        = 2.4e9;
inline constexpr double kDefaultSourceR     = 50.0;
inline constexpr double kDefaultSourceX     = 0.0;
inline constexpr double kDefaultLoadR       = 25.0;
inline constexpr double kDefaultLoadX       = -15.0;
inline constexpr int    kDefaultSweepPoints = 401;
inline constexpr double kDefaultVswrThreshold = 2.0;

// --- Plages de validité ------------------------------------------------------
inline constexpr double kMinFrequencyHz = 1.0e3;    // 1 kHz
inline constexpr double kMaxFrequencyHz = 1.0e12;   // 1 THz
inline constexpr int    kMinSweepPoints = 11;
inline constexpr int    kMaxSweepPoints = 10001;

// --- Seuils numériques -------------------------------------------------------
// Tolérance relative pour décider qu'une réactance/susceptance est « nulle ».
inline constexpr double kZeroRelTolerance = 1.0e-9;
// En dessous de ce module, une impédance est considérée comme un court-circuit
// (son admittance serait infinie : on ne divise jamais par zéro).
inline constexpr double kTinyImpedance = 1.0e-300;
// Impédance « infinie » (circuit ouvert) représentée par un grand nombre fini,
// pour que les calculs suivants ne produisent ni NaN ni inf.
inline constexpr double kOpenCircuitOhm = 1.0e300;
// |Γ| à partir duquel le VSWR est affiché « ∞ ».
inline constexpr double kVswrInfinityGammaMargin = 1.0e-12;
// |Γ| en dessous duquel la return loss est affichée « ∞ dB ».
inline constexpr double kReturnLossInfiniteGamma = 1.0e-15;

// --- Qualité d'adaptation (indicateur vert / orange / rouge) ------------------
// Balayage : plancher de |S11| (dB) utilisé pour le tracé (évite −∞ dans les courbes) et
// écart relatif sous lequel une fréquence est confondue avec f₀.
inline constexpr double kS11FloorDb = -100.0;
inline constexpr double kSameFrequencyRelTolerance = 1.0e-9;
inline constexpr double kVswrGoodLimit = 1.5;
inline constexpr double kVswrOkLimit   = 2.0;

} // namespace smith
