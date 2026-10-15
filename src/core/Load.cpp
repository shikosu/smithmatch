// =============================================================================
// Load.cpp — Impédance de la charge en fonction de la fréquence
// =============================================================================
#include "core/Load.hpp"

#include <cmath>

namespace smith {

const char* loadModelName(LoadModel model) {
    switch (model) {
        case LoadModel::Constant: return "Constant";
        case LoadModel::SeriesEquivalent: return "Series equivalent";
        case LoadModel::ParallelEquivalent: return "Parallel equivalent";
    }
    return "?";
}

const char* loadModelJsonName(LoadModel model) {
    switch (model) {
        case LoadModel::Constant: return "constant";
        case LoadModel::SeriesEquivalent: return "series_equivalent";
        case LoadModel::ParallelEquivalent: return "parallel_equivalent";
    }
    return "";
}

Complex loadImpedanceAt(const LoadSpec& load, double f0Hz, double fHz) {
    // À la fréquence centrale (ou pour le modèle constant) : valeur saisie, telle quelle.
    if (load.model == LoadModel::Constant || fHz == f0Hz) {
        return load.zAtF0;
    }

    switch (load.model) {
        case LoadModel::SeriesEquivalent: {
            const double r = load.zAtF0.real();
            const double x0 = load.zAtF0.imag();
            // Bobine (X₀ > 0) : X croît comme f. Condensateur (X₀ < 0) : |X| décroît comme 1/f.
            const double x = (x0 >= 0.0) ? x0 * (fHz / f0Hz) : x0 * (f0Hz / fHz);
            return Complex(r, x);
        }
        case LoadModel::ParallelEquivalent: {
            const Complex y0 = 1.0 / load.zAtF0;
            const double g = y0.real();
            const double b0 = y0.imag();
            // Condensateur (B₀ > 0) : B croît comme f. Bobine (B₀ < 0) : |B| décroît comme 1/f.
            const double b = (b0 >= 0.0) ? b0 * (fHz / f0Hz) : b0 * (f0Hz / fHz);
            return 1.0 / Complex(g, b);
        }
        case LoadModel::Constant: break;   // déjà traité plus haut
    }
    return load.zAtF0;
}

std::optional<LoadModel> loadModelFromJsonName(const std::string& name) {
    for (const LoadModel model : {LoadModel::Constant, LoadModel::SeriesEquivalent, LoadModel::ParallelEquivalent}) {
        if (name == loadModelJsonName(model)) {
            return model;
        }
    }
    return std::nullopt;
}

} // namespace smith
