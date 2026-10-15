// =============================================================================
// Units.hpp — Affichage et saisie en « notation ingénieur »
// =============================================================================
// Rôle : c'est le SEUL endroit où existent les préfixes (p, n, µ, m, k, M, G…).
// Partout ailleurs on calcule en unités SI pures (règle n°5).
//
//   formatEng(3.898e-8, "H")  →  "38.98 nH"
//   parseEng("4.7 nH")        →  4.7e-9
// =============================================================================
#pragma once

#include <optional>
#include <string>

#include "core/Types.hpp"

namespace smith {

// Formate une valeur avec le préfixe SI qui donne un nombre entre 1 et 999,
// et « sigDigits » chiffres significatifs (zéros inutiles supprimés).
// Exemples : (3.898e-8,"H") → "38.98 nH" ; (50,"Ω") → "50 Ω" ; (2.4e9,"Hz") → "2.4 GHz".
// Cas particuliers : 0 → "0 <unité>", ±infini → "∞ <unité>", NaN → "NaN".
std::string formatEng(double value, const std::string& unit, int sigDigits = 4);

// Formate un nombre avec « sigDigits » chiffres significatifs, sans préfixe ni unité
// (zéros inutiles supprimés). Exemple : (1.23456, 4) → "1.235".
std::string formatSig(double value, int sigDigits = 4);

// Formate un nombre complexe « a + jb » (ou « a − jb ») avec « sigDigits » chiffres
// significatifs par partie. Exemple : (25, −15) → "25 − j15". Pas d'unité ajoutée.
std::string formatComplex(Complex value, int sigDigits = 4);

// Lit un nombre saisi par l'utilisateur. Accepte :
//   "4.7n", "4.7 nH", "4.7nH", "100p", "2.2u", "2.2µ", "1.5k", "2.4G", "2.4 GHz", "50",
//   "4,7n" (virgule décimale), "1e-9", "-15".
// Préfixes reconnus : f p n u µ m k K M G T (m = milli, M = méga ; sensible à la casse).
// Unités reconnues (ignorées) : Hz H F Ω ohm S.
// Renvoie std::nullopt pour tout le reste ("abc", vide, "nan", "inf", dépassement…).
std::optional<double> parseEng(const std::string& text);

} // namespace smith
