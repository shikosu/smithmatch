// =============================================================================
// Solver.cpp — Implémentation du solveur L / Pi / T
// =============================================================================
// Idée générale : un réseau en L = UN élément série + UN élément shunt. Il y a
// deux façons de les ordonner, et chacune se résout avec une équation du second
// degré (d'où au plus 2 solutions par ordre : les deux signes de la racine).
//
// Pi et T sont construits en « collant » deux réseaux en L bout à bout autour
// d'une résistance virtuelle R_v choisie à partir du Q voulu :
//     Z_L  ──[L n°1]──  R_v  ──[L n°2]──  Z_s*
// Les deux éléments qui se touchent au milieu (même nature) sont fusionnés.
// =============================================================================
#include "core/Solver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

#include "core/RfMath.hpp"
#include "core/Units.hpp"
#include "core/Validation.hpp"

namespace smith {

namespace {

// Tolérance relative sur le discriminant d (CLAUDE.md §5.6).
constexpr double kDiscriminantTolerance = 1.0e-12;
// Si |Γ_m| est sous ce seuil, la charge est déjà adaptée.
constexpr double kAlreadyMatchedGamma = 1.0e-9;

// Un réseau en L « brut » : la réactance série X (Ω) et la susceptance shunt B (S)
// valent des NOMBRES RELATIFS (signe = nature L ou C), pas encore des composants.
struct RawL {
    double seriesX = 0.0;
    double shuntB = 0.0;
    double q = 0.0;          // Q de l'étage = |réactance intermédiaire| / partie réelle
};

// --- Shunt-first -----------------------------------------------------------------
// Élément parallèle aux bornes de la charge, puis élément série vers la cible.
//   d  = G_L/R_t − G_L²           (pas de solution si d < 0)
//   B' = ±√d
//   B_shunt  = B' − B_L
//   X_series = X_t + B'/(G_L² + B'²)
std::vector<RawL> solveShuntFirst(Complex zLoad, Complex zTarget) {
    std::vector<RawL> out;
    const Complex yLoad = 1.0 / zLoad;
    const double gl = yLoad.real();
    const double bl = yLoad.imag();
    const double rt = zTarget.real();
    const double xt = zTarget.imag();

    const double scale = gl / rt;
    double d = gl / rt - gl * gl;
    if (d < -kDiscriminantTolerance * scale) {
        return out;                                   // aucune solution
    }
    if (std::abs(d) <= kDiscriminantTolerance * scale) {
        d = 0.0;                                      // racine double : une seule solution
    }
    const double root = std::sqrt(d);
    const int count = (d == 0.0) ? 1 : 2;
    for (int i = 0; i < count; ++i) {
        const double bPrime = (i == 0) ? root : -root;
        RawL raw;
        raw.shuntB = bPrime - bl;
        raw.seriesX = xt + bPrime / (gl * gl + bPrime * bPrime);
        raw.q = std::abs(bPrime) / gl;
        out.push_back(raw);
    }
    return out;
}

// --- Series-first ----------------------------------------------------------------
// Élément série côté charge, puis élément parallèle vers la cible.
//   d  = R_L/G_t − R_L²           (pas de solution si d < 0)
//   X' = ±√d
//   X_series = X' − X_L
//   B_shunt  = B_t + X'/(R_L² + X'²)
std::vector<RawL> solveSeriesFirst(Complex zLoad, Complex zTarget) {
    std::vector<RawL> out;
    const Complex yTarget = 1.0 / zTarget;
    const double gt = yTarget.real();
    const double bt = yTarget.imag();
    const double rl = zLoad.real();
    const double xl = zLoad.imag();

    const double scale = rl / gt;
    double d = rl / gt - rl * rl;
    if (d < -kDiscriminantTolerance * scale) {
        return out;
    }
    if (std::abs(d) <= kDiscriminantTolerance * scale) {
        d = 0.0;
    }
    const double root = std::sqrt(d);
    const int count = (d == 0.0) ? 1 : 2;
    for (int i = 0; i < count; ++i) {
        const double xPrime = (i == 0) ? root : -root;
        RawL raw;
        raw.seriesX = xPrime - xl;
        raw.shuntB = bt + xPrime / (rl * rl + xPrime * xPrime);
        raw.q = std::abs(xPrime) / rl;
        out.push_back(raw);
    }
    return out;
}

// Construit un élément série (X en Ω) ou shunt (B en S) ; std::nullopt si la valeur est nulle.
std::optional<Element> seriesElement(double x, double f0Hz, double scaleOhm) {
    return elementFromReactance(x, f0Hz, true, scaleOhm);
}
std::optional<Element> shuntElement(double b, double f0Hz, double scaleSiemens) {
    return elementFromReactance(b, f0Hz, false, scaleSiemens);
}

// Un élément « brut » d'un réseau : série (X) ou shunt (B).
struct RawElement {
    bool series;
    double value;    // X (Ω) ou B (S)
};

// Fusionne les éléments voisins de même nature (série+série : on additionne les X ;
// shunt+shunt : on additionne les B), supprime ceux qui sont nuls, puis convertit
// en composants. L'ordre est celui du réseau (indice 0 côté charge).
ElementList buildElements(std::vector<RawElement> raw, double f0Hz, double scaleOhm, double scaleSiemens) {
    // Fusion des voisins de même nature, en recommençant tant que quelque chose change
    // (un élément nul supprimé peut rendre deux autres voisins).
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t i = 0; i < raw.size(); ++i) {
            const double scale = raw[i].series ? scaleOhm : scaleSiemens;
            if (std::abs(raw[i].value) <= kZeroRelTolerance * scale) {
                raw.erase(raw.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
            if (i + 1 < raw.size() && raw[i].series == raw[i + 1].series) {
                raw[i].value += raw[i + 1].value;
                raw.erase(raw.begin() + static_cast<std::ptrdiff_t>(i) + 1);
                changed = true;
                break;
            }
        }
    }
    ElementList elements;
    for (const RawElement& r : raw) {
        const auto element = r.series ? seriesElement(r.value, f0Hz, scaleOhm)
                                      : shuntElement(r.value, f0Hz, scaleSiemens);
        if (element.has_value()) {
            elements.push_back(*element);
        }
    }
    return elements;
}

// Classement Low-pass / High-pass / Mixed d'une liste d'éléments.
SolutionType classify(const ElementList& elements) {
    bool allLow = true;
    bool allHigh = true;
    for (const Element& e : elements) {
        const bool lowPassKind = (e.type == ElementType::SeriesInductor || e.type == ElementType::ShuntCapacitor);
        allLow = allLow && lowPassKind;
        allHigh = allHigh && !lowPassKind;
    }
    if (elements.empty()) {
        return SolutionType::Mixed;
    }
    if (allLow) {
        return SolutionType::LowPass;
    }
    return allHigh ? SolutionType::HighPass : SolutionType::Mixed;
}

// Échelles « de l'ordre de grandeur du problème » pour décider qu'une valeur est nulle.
double impedanceScale(Complex zSource, Complex zLoad) {
    return std::max({std::abs(zSource), std::abs(zLoad), std::numeric_limits<double>::min()});
}

// Contrôle des entrées communes à tous les solveurs.
bool validateInputs(Complex zSource, Complex zLoad, double f0Hz, SolverResult& result) {
    ValidationResult check = validateFrequency(f0Hz);
    if (check.ok) {
        check = validateSourceImpedance(zSource);
    }
    if (check.ok) {
        check = validateLoadImpedance(zLoad);
    }
    if (!check.ok) {
        result.ok = false;
        result.message = check.message;
        return false;
    }
    return true;
}

} // namespace

const char* solutionTypeName(SolutionType type) {
    switch (type) {
        case SolutionType::LowPass: return "Low-pass";
        case SolutionType::HighPass: return "High-pass";
        case SolutionType::Mixed: return "Mixed";
    }
    return "?";
}

const char* solutionTopologyName(SolutionTopology topo) {
    switch (topo) {
        case SolutionTopology::LShuntFirst: return "L (shunt first)";
        case SolutionTopology::LSeriesFirst: return "L (series first)";
        case SolutionTopology::Pi: return "Pi";
        case SolutionTopology::T: return "T";
    }
    return "?";
}

// -----------------------------------------------------------------------------
// Réseau en L
// -----------------------------------------------------------------------------
SolverResult solveL(Complex zSource, Complex zLoad, double f0Hz) {
    SolverResult result;
    if (!validateInputs(zSource, zLoad, f0Hz, result)) {
        return result;
    }
    result.ok = true;

    const Complex zTarget = std::conj(zSource);

    // Déjà adapté ? (Z_in = Z_L en l'absence d'élément.)
    if (std::abs(matchGamma(zLoad, zSource)) < kAlreadyMatchedGamma) {
        result.alreadyMatched = true;
        result.message = "Load is already matched: no element is needed.";
        return result;
    }

    const double scaleOhm = impedanceScale(zSource, zLoad);
    const double scaleSiemens = 1.0 / scaleOhm;

    // Solutions « shunt-first » : l'élément shunt est à l'indice 0 (côté charge).
    for (const RawL& raw : solveShuntFirst(zLoad, zTarget)) {
        Solution s;
        s.topology = SolutionTopology::LShuntFirst;
        s.nominalQ = raw.q;
        s.elements = buildElements({{false, raw.shuntB}, {true, raw.seriesX}}, f0Hz, scaleOhm, scaleSiemens);
        s.type = classify(s.elements);
        result.solutions.push_back(s);
    }
    // Solutions « series-first » : l'élément série est à l'indice 0.
    for (const RawL& raw : solveSeriesFirst(zLoad, zTarget)) {
        Solution s;
        s.topology = SolutionTopology::LSeriesFirst;
        s.nominalQ = raw.q;
        s.elements = buildElements({{true, raw.seriesX}, {false, raw.shuntB}}, f0Hz, scaleOhm, scaleSiemens);
        s.type = classify(s.elements);
        result.solutions.push_back(s);
    }

    if (result.solutions.empty()) {
        result.message = "No L-network exists for this load and source. Try a Pi or T network.";
    }
    return result;
}

// -----------------------------------------------------------------------------
// Q minimal des réseaux Pi et T
// -----------------------------------------------------------------------------
namespace {
double qMinFromResistances(double r1, double r2) {
    const double high = std::max(r1, r2);
    const double low = std::min(r1, r2);
    return std::sqrt(std::max(high / low - 1.0, 0.0));
}
// Résistance parallèle équivalente : R_p = 1/Re(1/Z).
double parallelResistance(Complex z) { return 1.0 / (1.0 / z).real(); }
} // namespace

double piQMin(Complex zSource, Complex zLoad) {
    if (!validateSourceImpedance(zSource).ok || !validateLoadImpedance(zLoad).ok) {
        return 0.0;
    }
    return qMinFromResistances(parallelResistance(zLoad), parallelResistance(std::conj(zSource)));
}

double tQMin(Complex zSource, Complex zLoad) {
    if (!validateSourceImpedance(zSource).ok || !validateLoadImpedance(zLoad).ok) {
        return 0.0;
    }
    return qMinFromResistances(zLoad.real(), zSource.real());
}

namespace {

// Vérifie q > Q_min ; sinon remplit un message d'erreur clair.
bool checkQ(double q, double qMin, SolverResult& result) {
    if (!std::isfinite(q) || q <= 0.0) {
        result.ok = false;
        result.message = "Q must be a positive number.";
        return false;
    }
    if (q <= qMin) {
        result.ok = false;
        result.message = "Q must be greater than Q_min = " + formatSig(qMin, 3) + ".";
        return false;
    }
    return true;
}

} // namespace

// -----------------------------------------------------------------------------
// Pi : shunt – série – shunt
//   R_v = max(R_pL, R_pS)/(1 + Q²)
//   L n°1 « shunt-first » de Z_L vers R_v ; L n°2 « series-first » de R_v vers Z_s*.
//   Les deux réactances série se touchent au milieu : on les additionne.
// -----------------------------------------------------------------------------
SolverResult solvePi(Complex zSource, Complex zLoad, double f0Hz, double q) {
    SolverResult result;
    if (!validateInputs(zSource, zLoad, f0Hz, result)) {
        return result;
    }
    const Complex zTarget = std::conj(zSource);
    result.qMin = piQMin(zSource, zLoad);
    if (!checkQ(q, result.qMin, result)) {
        return result;
    }
    result.ok = true;

    const double rpl = parallelResistance(zLoad);
    const double rps = parallelResistance(zTarget);
    const double rv = std::max(rpl, rps) / (1.0 + q * q);

    const double scaleOhm = impedanceScale(zSource, zLoad);
    const double scaleSiemens = 1.0 / scaleOhm;

    const std::vector<RawL> first = solveShuntFirst(zLoad, Complex(rv, 0.0));
    const std::vector<RawL> second = solveSeriesFirst(Complex(rv, 0.0), zTarget);
    for (const RawL& a : first) {
        for (const RawL& b : second) {
            Solution s;
            s.topology = SolutionTopology::Pi;
            s.nominalQ = q;
            // Ordre côté charge → côté source : shunt(a), série(a), série(b), shunt(b).
            s.elements = buildElements({{false, a.shuntB}, {true, a.seriesX}, {true, b.seriesX}, {false, b.shuntB}},
                                       f0Hz, scaleOhm, scaleSiemens);
            s.type = classify(s.elements);
            result.solutions.push_back(s);
        }
    }
    if (result.solutions.empty()) {
        result.message = "No Pi network found for this Q.";
    }
    return result;
}

// -----------------------------------------------------------------------------
// T : série – shunt – série
//   R_v = min(Re Z_L, Re Z_s*)·(1 + Q²)
//   L n°1 « series-first » de Z_L vers R_v ; L n°2 « shunt-first » de R_v vers Z_s*.
//   Les deux susceptances shunt se touchent au milieu : on les additionne.
// -----------------------------------------------------------------------------
SolverResult solveT(Complex zSource, Complex zLoad, double f0Hz, double q) {
    SolverResult result;
    if (!validateInputs(zSource, zLoad, f0Hz, result)) {
        return result;
    }
    const Complex zTarget = std::conj(zSource);
    result.qMin = tQMin(zSource, zLoad);
    if (!checkQ(q, result.qMin, result)) {
        return result;
    }
    result.ok = true;

    const double rv = std::min(zLoad.real(), zTarget.real()) * (1.0 + q * q);

    const double scaleOhm = impedanceScale(zSource, zLoad);
    const double scaleSiemens = 1.0 / scaleOhm;

    const std::vector<RawL> first = solveSeriesFirst(zLoad, Complex(rv, 0.0));
    const std::vector<RawL> second = solveShuntFirst(Complex(rv, 0.0), zTarget);
    for (const RawL& a : first) {
        for (const RawL& b : second) {
            Solution s;
            s.topology = SolutionTopology::T;
            s.nominalQ = q;
            // Ordre côté charge → côté source : série(a), shunt(a), shunt(b), série(b).
            s.elements = buildElements({{true, a.seriesX}, {false, a.shuntB}, {false, b.shuntB}, {true, b.seriesX}},
                                       f0Hz, scaleOhm, scaleSiemens);
            s.type = classify(s.elements);
            result.solutions.push_back(s);
        }
    }
    if (result.solutions.empty()) {
        result.message = "No T network found for this Q.";
    }
    return result;
}

} // namespace smith
