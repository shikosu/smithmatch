// =============================================================================
// Solver.hpp — Solveur automatique de réseaux d'adaptation (L, Pi, T)
// =============================================================================
// Rôle : à partir de Z_s, Z_L et f₀, calculer TOUS les réseaux L/C qui amènent
// l'impédance d'entrée Z_in sur la cible Z_s* (conjugué de la source).
// Formules : CLAUDE.md §5.6 (réseau en L) et §5.7 (Pi et T, facteur Q nominal).
//
// Convention de sortie : chaque solution contient la liste d'éléments DANS LA
// CONVENTION DU RÉSEAU (indice 0 = côté charge), directement copiable dans
// Project::network.
//
// Pas d'exception : les cas « pas de solution » ou « entrée invalide » sont
// décrits dans SolverResult (ok / message).
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "core/Element.hpp"
#include "core/Types.hpp"

namespace smith {

// Comportement fréquentiel d'une solution (CLAUDE.md §5.6).
enum class SolutionType {
    LowPass,    // uniquement des L série et des C shunt
    HighPass,   // uniquement des C série et des L shunt
    Mixed
};

// Forme du réseau.
enum class SolutionTopology {
    LShuntFirst,    // L : élément parallèle côté charge, puis élément série
    LSeriesFirst,   // L : élément série côté charge, puis élément parallèle
    Pi,             // shunt – série – shunt
    T               // série – shunt – série
};

const char* solutionTypeName(SolutionType type);          // "Low-pass", "High-pass", "Mixed"
const char* solutionTopologyName(SolutionTopology topo);  // "L (shunt first)", "L (series first)", "Pi", "T"

struct Solution {
    ElementList elements;      // indice 0 = côté charge
    SolutionType type = SolutionType::Mixed;
    SolutionTopology topology = SolutionTopology::LShuntFirst;
    double nominalQ = 0.0;     // Q nominal (L : déduit de la réactance intermédiaire ; Pi/T : valeur demandée)
};

struct SolverResult {
    bool ok = false;               // entrées valides et calcul effectué
    bool alreadyMatched = false;   // la charge est déjà adaptée : aucun élément nécessaire
    std::string message;           // explication (erreur, « pas de solution », « déjà adapté »…)
    std::vector<Solution> solutions;
    double qMin = 0.0;             // Pi/T : Q minimal admissible (0 pour un réseau en L)
};

// Réseau en L : toutes les solutions des deux topologies (0 à 4 au total).
SolverResult solveL(Complex zSource, Complex zLoad, double f0Hz);

// Pi (shunt – série – shunt) avec un Q nominal « q » ; exige q > Q_min.
SolverResult solvePi(Complex zSource, Complex zLoad, double f0Hz, double q);

// T (série – shunt – série) avec un Q nominal « q » ; exige q > Q_min.
SolverResult solveT(Complex zSource, Complex zLoad, double f0Hz, double q);

// Q minimal pour un Pi ou un T : √(Rmax/Rmin − 1). Renvoie 0 si les entrées sont invalides.
//  Pi : R_pL = 1/Re(1/Z_L) et R_pS = 1/Re(1/Z_s*) (résistances parallèles équivalentes)
//  T  : Re(Z_L) et Re(Z_s) (résistances série)
double piQMin(Complex zSource, Complex zLoad);
double tQMin(Complex zSource, Complex zLoad);

} // namespace smith
