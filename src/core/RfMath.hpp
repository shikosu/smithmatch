// =============================================================================
// RfMath.hpp — Formules radiofréquence de base
// =============================================================================
// Rôle : coefficient de réflexion Γ, conversions impédance ↔ Γ, VSWR, return
// loss, S11, pertes de désadaptation, et passage Γ ↔ pixels de l'écran.
//
// Toutes les fonctions sont PURES (mêmes entrées → mêmes sorties, pas d'état).
// Conventions : voir CLAUDE.md §5.3 et §5.4.
// =============================================================================
#pragma once

#include "core/Types.hpp"

namespace smith {

// Γ = (Z − Z₀)/(Z + Z₀). Z en Ω, Z₀ en Ω (réelle > 0). Résultat sans unité.
// Si Z + Z₀ = 0 (impédance non passive, hors usage normal), renvoie un Γ « infini ».
Complex gammaFromZ(Complex z, double z0);

// Réciproque : Z = Z₀·(1 + Γ)/(1 − Γ). Si Γ = 1 (circuit ouvert), renvoie kOpenCircuitOhm.
Complex zFromGamma(Complex gamma, double z0);

// Γ pour une ADMITTANCE normalisée : Γ = (1 − y)/(1 + y), avec y = Y·Z₀.
// (Même point de l'abaque que gammaFromZ(1/Y), mais sans division par zéro.)
Complex gammaFromY(Complex y, double z0);

// Coefficient de réflexion « côté source » (ondes de puissance) :
//   Γ_m = (Z_in − Z_s*) / (Z_in + Z_s)
// Si Z_s est réelle, c'est le Γ classique par rapport à Z_s. Γ_m = 0 ⇔ adaptation parfaite.
Complex matchGamma(Complex zIn, Complex zSource);

// VSWR = (1 + |Γ|)/(1 − |Γ|). Vaut 1 si |Γ| = 0 et +∞ si |Γ| ≥ 1 − 1e-12.
double vswr(double gammaMag);

// Return loss (dB, positive) = −20·log10|Γ|. +∞ si |Γ| < 1e-15.
double returnLossDb(double gammaMag);

// |S11| (dB, négatif) = 20·log10|Γ|. −∞ si |Γ| < 1e-15.
double s11Db(double gammaMag);

// Pertes de désadaptation (dB) = −10·log10(1 − |Γ|²). +∞ si |Γ| ≥ 1.
double mismatchLossDb(double gammaMag);

// Toutes les grandeurs d'adaptation calculées à partir de Z_in (affichées dans RESULTS).
struct MatchMetrics {
    Complex zIn;              // impédance d'entrée (Ω)
    Complex gammaM;           // Γ_m = (Z_in − Z_s*)/(Z_in + Z_s)
    double gammaMag = 0.0;    // |Γ_m|
    double gammaAngleDeg = 0.0;
    double vswr = 1.0;
    double returnLossDb = 0.0;
    double s11Db = 0.0;
    double mismatchLossDb = 0.0;
};
MatchMetrics evaluateMatch(Complex zIn, Complex zSource);

// Γ → pixel de l'écran : centre + rayon·(Re Γ, −Im Γ).
// Le signe « − » vient de l'axe Y de l'écran, orienté vers le bas.
Point2 gammaToScreen(Complex gamma, Point2 center, double radiusPx);

// Pixel de l'écran → Γ (opération inverse de gammaToScreen).
Complex screenToGamma(Point2 pixel, Point2 center, double radiusPx);

} // namespace smith
