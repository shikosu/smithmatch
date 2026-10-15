// =============================================================================
// Network.cpp — Implémentation des calculs de réseau
// =============================================================================
#include "core/Network.hpp"

namespace smith {

std::vector<Complex> nodeImpedances(const ElementList& network, Complex zLoad, double fHz) {
    std::vector<Complex> nodes;
    nodes.reserve(network.size() + 1);
    nodes.push_back(zLoad);                       // nœud 0 : la charge
    for (const Element& element : network) {
        nodes.push_back(apply(element, nodes.back(), fHz));   // on remonte vers la source
    }
    return nodes;
}

Complex inputImpedance(const ElementList& network, Complex zLoad, double fHz) {
    Complex z = zLoad;
    for (const Element& element : network) {
        z = apply(element, z, fHz);
    }
    return z;
}

std::vector<Complex> sampledPath(const ElementList& network, std::size_t index, Complex zLoad,
                                 double fHz, double z0, int nSamples) {
    if (index >= network.size()) {
        return {};
    }
    // Impédance vue juste avant l'élément « index » = nœud « index ».
    Complex zPrev = zLoad;
    for (std::size_t i = 0; i < index; ++i) {
        zPrev = apply(network[i], zPrev, fHz);
    }
    return sampledArc(network[index], zPrev, fHz, z0, nSamples);
}

} // namespace smith
