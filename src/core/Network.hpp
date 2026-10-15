// =============================================================================
// Network.hpp — Calculs sur la liste d'éléments (impédances de nœuds, trajet)
// =============================================================================
// Rôle : à partir de l'impédance de la charge à la fréquence f, parcourir la
// liste d'éléments (indice 0 = côté charge) et calculer l'impédance vue à
// chaque nœud. Nœud 0 = Z_L ; nœud k = impédance vue après l'élément k−1 ;
// dernier nœud = Z_in, l'impédance vue par la source (CLAUDE.md §5.2).
// =============================================================================
#pragma once

#include <vector>

#include "core/Element.hpp"

namespace smith {

// Impédances de tous les nœuds : taille n+1 pour n éléments.
//   nodes[0] = zLoad ; nodes[k+1] = apply(network[k], nodes[k], fHz).
std::vector<Complex> nodeImpedances(const ElementList& network, Complex zLoad, double fHz);

// Impédance d'entrée Z_in (dernier nœud). Sans élément : Z_in = Z_L.
Complex inputImpedance(const ElementList& network, Complex zLoad, double fHz);

// Arc de l'élément numéro « index » (Γ par rapport à z0), voir sampledArc().
// Si index est hors limites, renvoie une liste vide.
std::vector<Complex> sampledPath(const ElementList& network, std::size_t index, Complex zLoad,
                                 double fHz, double z0, int nSamples = 64);

} // namespace smith
