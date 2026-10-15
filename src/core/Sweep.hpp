// =============================================================================
// Sweep.hpp — Balayage en fréquence et calcul de la bande passante
// =============================================================================
// Rôle : pour un réseau donné, calculer Z_in(f), Γ_m(f), |S11|(f) et le VSWR(f) sur
// une plage de fréquences, puis en déduire la bande passante autour de f₀
// (zone contiguë où le VSWR reste sous un seuil).
//
// La charge évolue avec la fréquence selon son modèle (core/Load) ; la source est
// constante. Fonctions pures, aucune exception : une erreur de réglage donne
// ok = false et un message.
// =============================================================================
#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "core/Element.hpp"
#include "core/Project.hpp"
#include "core/Types.hpp"

namespace smith {

// Un point du balayage.
struct SweepPoint {
    double fHz = 0.0;
    Complex zIn;            // impédance vue par la source (Ω)
    double gammaMag = 0.0;  // |Γ_m|
    double gammaAngleDeg = 0.0;
    double s11Db = 0.0;     // 20·log10|Γ_m|, plafonné par le bas à kS11FloorDb (pour le tracé)
    double vswr = 1.0;      // peut valoir +∞ si |Γ_m| ≥ 1 − 1e-12
};

struct SweepResult {
    static constexpr std::size_t kNoIndex = std::numeric_limits<std::size_t>::max();

    bool ok = false;
    std::string message;                // raison de l'échec si ok = false
    std::vector<SweepPoint> points;     // triés par fréquence croissante
    std::size_t f0Index = kNoIndex;     // indice du point f₀ (kNoIndex si f₀ est hors de la plage)
};

// Balaye le réseau « network » avec les réglages du projet (p.sweep) : plage, nombre de
// points, échelle linéaire/logarithmique. f₀ est toujours ajouté comme point s'il est dans
// la plage (pour que le résultat à f₀ soit exact). La charge suit p.load ; la source p.zSource.
SweepResult runSweep(const Project& project, const ElementList& network);

// Bande passante autour de f₀.
struct BandwidthResult {
    bool ok = false;
    std::string message;            // pourquoi pas de bande (VSWR(f₀) trop haut, f₀ hors plage…)
    double fLowHz = 0.0;
    double fHighHz = 0.0;
    double widthHz = 0.0;           // fHigh − fLow
    double fractionalPercent = 0.0; // 100·width/f₀
    bool lowOpen = false;           // la bande atteint le bord bas de la plage balayée (elle est plus large)
    bool highOpen = false;          // idem pour le bord haut
};

// Cherche la plage CONTIGUË autour de f₀ où VSWR ≤ seuil. Les bords sont obtenus par
// interpolation linéaire de |Γ_m| entre le dernier point conforme et le premier point non
// conforme (|Γ_m| reste fini, contrairement au VSWR). Échec si VSWR(f₀) > seuil.
BandwidthResult bandwidth(const SweepResult& sweep, double vswrThreshold);

// Texte court pour l'interface : « 412 MHz (17.2 %) », « > 40 MHz » si la bande touche un bord
// de la plage balayée, ou « none » s'il n'y a pas de bande.
std::string formatBandwidth(const BandwidthResult& bw);

// Export CSV du balayage (séparateur « , », point décimal, en-tête, une ligne par point) :
//   f_hz,zin_re_ohm,zin_im_ohm,gamma_mag,gamma_deg,s11_db,vswr
// Remarque : s11_db est limité par le bas à kS11FloorDb (adaptation parfaite) et vswr vaut « inf »
// si |Γm| ≈ 1. Les nombres sont écrits avec 12 chiffres significatifs, quelle que soit la langue du système.
std::string sweepToCsv(const SweepResult& sweep);

// |Γ| correspondant à un VSWR donné : (S − 1)/(S + 1).
double gammaFromVswr(double vswrValue);

} // namespace smith
