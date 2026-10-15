// =============================================================================
// ESeries.cpp — Implémentation de l'arrondi E12 / E24
// =============================================================================
#include "core/ESeries.hpp"

#include <array>
#include <cmath>

namespace smith {

namespace {

// Valeurs d'une décade (entre 1.0 inclus et 10.0 exclu), CLAUDE.md §5.9.
constexpr std::array<double, 12> kE12 = {1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};
constexpr std::array<double, 24> kE24 = {1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
                                         3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
constexpr double kNextDecade = 10.0;   // début de la décade suivante (candidat d'arrondi)

// Cherche, parmi les valeurs « series » + 10.0, celle qui minimise |ln(m/s)|.
template <std::size_t N>
double nearestMantissa(double mantissa, const std::array<double, N>& series) {
    double best = kNextDecade;
    double bestDistance = std::abs(std::log(mantissa / kNextDecade));
    for (double candidate : series) {
        const double distance = std::abs(std::log(mantissa / candidate));
        if (distance < bestDistance) {
            bestDistance = distance;
            best = candidate;
        }
    }
    return best;
}

} // namespace

const char* eSeriesName(ESeriesKind kind) {
    switch (kind) {
        case ESeriesKind::None: return "None";
        case ESeriesKind::E12: return "E12";
        case ESeriesKind::E24: return "E24";
    }
    return "?";
}

const char* eSeriesJsonName(ESeriesKind kind) {
    switch (kind) {
        case ESeriesKind::None: return "none";
        case ESeriesKind::E12: return "E12";
        case ESeriesKind::E24: return "E24";
    }
    return "none";
}

double nearestESeries(double value, ESeriesKind kind) {
    if (kind == ESeriesKind::None || !(value > 0.0) || !std::isfinite(value)) {
        return value;
    }
    // On écrit value = mantisse × 10^exposant, avec 1 ≤ mantisse < 10.
    int exponent = static_cast<int>(std::floor(std::log10(value)));
    double mantissa = value / std::pow(10.0, exponent);
    // Garde-fou contre les erreurs d'arrondi de log10 aux puissances exactes de 10.
    if (mantissa < 1.0) {
        mantissa *= 10.0;
        --exponent;
    } else if (mantissa >= kNextDecade) {
        mantissa /= 10.0;
        ++exponent;
    }

    double best = mantissa;
    switch (kind) {
        case ESeriesKind::E12: best = nearestMantissa(mantissa, kE12); break;
        case ESeriesKind::E24: best = nearestMantissa(mantissa, kE24); break;
        case ESeriesKind::None: break;
    }
    return best * std::pow(10.0, exponent);
}

ElementList snapNetwork(const ElementList& network, ESeriesKind kind) {
    ElementList snapped = network;
    for (Element& element : snapped) {
        element.value = nearestESeries(element.value, kind);
    }
    return snapped;
}

std::optional<ESeriesKind> eSeriesFromJsonName(const std::string& name) {
    // On teste chaque valeur de l'énumération : si on en ajoute une, le switch de eSeriesJsonName
    // oblige à la traiter, et elle devient automatiquement lisible ici.
    for (const ESeriesKind kind : {ESeriesKind::None, ESeriesKind::E12, ESeriesKind::E24}) {
        if (name == eSeriesJsonName(kind)) {
            return kind;
        }
    }
    return std::nullopt;
}

} // namespace smith
