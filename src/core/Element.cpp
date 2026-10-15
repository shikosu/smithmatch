// =============================================================================
// Element.cpp — Implémentation des éléments L/C
// =============================================================================
#include "core/Element.hpp"

#include <cmath>

#include "core/RfMath.hpp"

namespace smith {

bool isSeries(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor:
        case ElementType::SeriesCapacitor: return true;
        case ElementType::ShuntInductor:
        case ElementType::ShuntCapacitor: return false;
    }
    return true;
}

bool isInductor(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor:
        case ElementType::ShuntInductor: return true;
        case ElementType::SeriesCapacitor:
        case ElementType::ShuntCapacitor: return false;
    }
    return true;
}

const char* typeName(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor: return "Series L";
        case ElementType::SeriesCapacitor: return "Series C";
        case ElementType::ShuntInductor: return "Shunt L";
        case ElementType::ShuntCapacitor: return "Shunt C";
    }
    return "?";
}

const char* typeJsonName(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor: return "series_inductor";
        case ElementType::SeriesCapacitor: return "series_capacitor";
        case ElementType::ShuntInductor: return "shunt_inductor";
        case ElementType::ShuntCapacitor: return "shunt_capacitor";
    }
    return "";
}

std::optional<ElementType> typeFromJsonName(const std::string& name) {
    for (ElementType type : {ElementType::SeriesInductor, ElementType::SeriesCapacitor,
                             ElementType::ShuntInductor, ElementType::ShuntCapacitor}) {
        if (name == typeJsonName(type)) {
            return type;
        }
    }
    return std::nullopt;
}

const char* valueUnit(ElementType type) { return isInductor(type) ? "H" : "F"; }

Element makeElement(ElementType type, double value) {
    Element element;
    element.type = type;
    element.value = value;
    return element;
}

// L série : X = +ωL ; C série : X = −1/(ωC).
double reactanceAt(const Element& element, double fHz) {
    const double omega = angularFrequency(fHz);
    switch (element.type) {
        case ElementType::SeriesInductor: return omega * element.value;
        case ElementType::SeriesCapacitor: return -1.0 / (omega * element.value);
        case ElementType::ShuntInductor:
        case ElementType::ShuntCapacitor: return 0.0;
    }
    return 0.0;
}

// C shunt : B = +ωC ; L shunt : B = −1/(ωL).
double susceptanceAt(const Element& element, double fHz) {
    const double omega = angularFrequency(fHz);
    switch (element.type) {
        case ElementType::ShuntCapacitor: return omega * element.value;
        case ElementType::ShuntInductor: return -1.0 / (omega * element.value);
        case ElementType::SeriesInductor:
        case ElementType::SeriesCapacitor: return 0.0;
    }
    return 0.0;
}

// Z_out = Z_in + jX
Complex addSeriesReactance(Complex zPrev, double reactance) {
    return zPrev + Complex(0.0, reactance);
}

// Y_out = Y_in + jB, Z_out = 1/Y_out
Complex addShuntSusceptance(Complex zPrev, double susceptance) {
    // Court-circuit : Y infinie, un shunt ne change rien (le court-circuit reste).
    if (std::abs(zPrev) < kTinyImpedance) {
        return zPrev;
    }
    const Complex yPrev = 1.0 / zPrev;
    const Complex yOut = yPrev + Complex(0.0, susceptance);
    // Circuit ouvert (Y = 0) : on renvoie une impédance énorme mais finie.
    if (std::abs(yOut) < kTinyImpedance) {
        return Complex(kOpenCircuitOhm, 0.0);
    }
    return 1.0 / yOut;
}

Complex apply(const Element& element, Complex zPrev, double fHz) {
    if (isSeries(element.type)) {
        return addSeriesReactance(zPrev, reactanceAt(element, fHz));
    }
    return addShuntSusceptance(zPrev, susceptanceAt(element, fHz));
}

std::optional<Element> elementFromReactance(double xOrB, double f0Hz, bool series, double scale) {
    if (std::abs(xOrB) <= kZeroRelTolerance * scale) {
        return std::nullopt;   // valeur nulle : l'élément n'existe pas
    }
    const double omega = angularFrequency(f0Hz);
    if (series) {
        // X > 0 → L = X/ω ; X < 0 → C = −1/(ωX)
        if (xOrB > 0.0) {
            return makeElement(ElementType::SeriesInductor, xOrB / omega);
        }
        return makeElement(ElementType::SeriesCapacitor, -1.0 / (omega * xOrB));
    }
    // B > 0 → C = B/ω ; B < 0 → L = −1/(ωB)
    if (xOrB > 0.0) {
        return makeElement(ElementType::ShuntCapacitor, xOrB / omega);
    }
    return makeElement(ElementType::ShuntInductor, -1.0 / (omega * xOrB));
}

std::vector<Complex> sampledArc(const Element& element, Complex zPrev, double fHz, double z0,
                                int nSamples) {
    if (nSamples < 2) {
        nSamples = 2;
    }
    const bool series = isSeries(element.type);
    // Valeur finale de la réactance (série) ou de la susceptance (shunt).
    const double finalValue = series ? reactanceAt(element, fHz) : susceptanceAt(element, fHz);

    std::vector<Complex> arc;
    arc.reserve(static_cast<std::size_t>(nSamples));
    for (int i = 0; i < nSamples; ++i) {
        // t va de 0 à 1 ; la réactance (ou susceptance) croît linéairement de 0 à sa valeur finale.
        const double t = static_cast<double>(i) / static_cast<double>(nSamples - 1);
        const Complex z = series ? addSeriesReactance(zPrev, t * finalValue)
                                 : addShuntSusceptance(zPrev, t * finalValue);
        arc.push_back(gammaFromZ(z, z0));
    }
    return arc;
}

} // namespace smith
