// =============================================================================
// test_elements_network.cpp — Tests des éléments, du réseau, de la charge
// et de la validation (spec §6.2, §6.3, §6.6 modèle de charge)
// =============================================================================
#include <doctest/doctest.h>

#include <cmath>
#include <limits>

#include "core/Element.hpp"
#include "core/Load.hpp"
#include "core/Network.hpp"
#include "core/RfMath.hpp"
#include "core/Validation.hpp"

using namespace smith;

namespace {
constexpr double kZ0 = 50.0;
constexpr double kF1GHz = 1.0e9;

// Angle (rad) du vecteur « point − centre » : sert à voir le sens de rotation.
double angleAround(Complex gamma, Complex center) { return std::arg(gamma - center); }
}

// --- Conversions réactance → composant (§6.2) -----------------------------------

TEST_CASE("X = +100 ohm at 1 GHz is L = 15.9155 nH") {
    const auto e = elementFromReactance(100.0, kF1GHz, true, kZ0);
    REQUIRE(e.has_value());
    CHECK(e->type == ElementType::SeriesInductor);
    CHECK(e->value == doctest::Approx(15.9155e-9).epsilon(1e-5));
}

TEST_CASE("X = -100 ohm at 1 GHz is C = 1.5915 pF") {
    const auto e = elementFromReactance(-100.0, kF1GHz, true, kZ0);
    REQUIRE(e.has_value());
    CHECK(e->type == ElementType::SeriesCapacitor);
    CHECK(e->value == doctest::Approx(1.5915e-12).epsilon(1e-4));
}

TEST_CASE("B > 0 is a shunt capacitor, B < 0 a shunt inductor") {
    // B = +10 mS à 1 GHz → C = B/ω = 1.59155 pF
    const auto c = elementFromReactance(0.01, kF1GHz, false, 1.0 / kZ0);
    REQUIRE(c.has_value());
    CHECK(c->type == ElementType::ShuntCapacitor);
    CHECK(c->value == doctest::Approx(0.01 / (kTwoPi * kF1GHz)).epsilon(1e-12));
    // B = −10 mS à 1 GHz → L = −1/(ωB) = 15.9155 nH
    const auto l = elementFromReactance(-0.01, kF1GHz, false, 1.0 / kZ0);
    REQUIRE(l.has_value());
    CHECK(l->type == ElementType::ShuntInductor);
    CHECK(l->value == doctest::Approx(15.9155e-9).epsilon(1e-5));
}

TEST_CASE("A zero reactance removes the element") {
    CHECK_FALSE(elementFromReactance(0.0, kF1GHz, true, kZ0).has_value());
    CHECK_FALSE(elementFromReactance(1e-12, kF1GHz, true, kZ0).has_value());   // 1e-12 ≤ 1e-9·50
    CHECK_FALSE(elementFromReactance(0.0, kF1GHz, false, 1.0 / kZ0).has_value());
    CHECK(elementFromReactance(1e-3, kF1GHz, true, kZ0).has_value());
}

TEST_CASE("Reactance and susceptance signs (CLAUDE.md 5.1)") {
    const double f = 2.4e9;
    const double omega = kTwoPi * f;
    CHECK(reactanceAt(makeElement(ElementType::SeriesInductor, 2e-9), f) ==
          doctest::Approx(omega * 2e-9));
    CHECK(reactanceAt(makeElement(ElementType::SeriesCapacitor, 1e-12), f) ==
          doctest::Approx(-1.0 / (omega * 1e-12)));
    CHECK(susceptanceAt(makeElement(ElementType::ShuntCapacitor, 1e-12), f) ==
          doctest::Approx(omega * 1e-12));
    CHECK(susceptanceAt(makeElement(ElementType::ShuntInductor, 2e-9), f) ==
          doctest::Approx(-1.0 / (omega * 2e-9)));
    // Un série n'a pas de susceptance, un shunt n'a pas de réactance.
    CHECK(susceptanceAt(makeElement(ElementType::SeriesInductor, 2e-9), f) == 0.0);
    CHECK(reactanceAt(makeElement(ElementType::ShuntCapacitor, 1e-12), f) == 0.0);
}

TEST_CASE("Element -> reactance -> element round trip") {
    for (ElementType t : {ElementType::SeriesInductor, ElementType::SeriesCapacitor,
                          ElementType::ShuntInductor, ElementType::ShuntCapacitor}) {
        const Element original = makeElement(t, 3.3e-12);
        const double f = 1.7e9;
        const bool series = isSeries(t);
        const double xb = series ? reactanceAt(original, f) : susceptanceAt(original, f);
        const auto back = elementFromReactance(xb, f, series, series ? kZ0 : 1.0 / kZ0);
        REQUIRE(back.has_value());
        CHECK(back->type == t);
        CHECK(back->value == doctest::Approx(original.value).epsilon(1e-12));
    }
}

TEST_CASE("Type names and JSON names round trip") {
    for (ElementType t : {ElementType::SeriesInductor, ElementType::SeriesCapacitor,
                          ElementType::ShuntInductor, ElementType::ShuntCapacitor}) {
        const auto back = typeFromJsonName(typeJsonName(t));
        REQUIRE(back.has_value());
        CHECK(*back == t);
    }
    CHECK_FALSE(typeFromJsonName("resistor").has_value());
}

// --- apply / réseau ---------------------------------------------------------------

TEST_CASE("apply: series adds jX, shunt adds jB to the admittance") {
    const double f = 2.4e9;
    const Element ls = makeElement(ElementType::SeriesInductor, 2.6526e-9);
    const Complex z1 = apply(ls, Complex(25.0, -15.0), f);
    CHECK(z1.real() == doctest::Approx(25.0).epsilon(1e-9));
    CHECK(z1.imag() == doctest::Approx(25.0).epsilon(1e-4));   // −15 + 40

    const Element cp = makeElement(ElementType::ShuntCapacitor, 1.3263e-12);
    const Complex z2 = apply(cp, z1, f);                       // Y = 0.02 − j0.02 + j0.02
    CHECK(z2.real() == doctest::Approx(50.0).epsilon(1e-4));
    CHECK(std::abs(z2.imag()) < 1e-2);
}

TEST_CASE("apply: a shunt on a short circuit changes nothing; no division by zero") {
    const Element cp = makeElement(ElementType::ShuntCapacitor, 1e-12);
    const Complex z = apply(cp, Complex(0.0, 0.0), 1e9);
    CHECK(z == Complex(0.0, 0.0));
    CHECK(std::isfinite(z.real()));
}

TEST_CASE("apply: a lossless shunt resonance gives a huge but finite impedance") {
    // Y = −j B_L + j B_C = 0 → circuit ouvert. Z_prev = jX pur : Y = −j/X.
    const double f = 1e9;
    const double omega = kTwoPi * f;
    const Complex zPrev(0.0, 100.0);          // Y = −j0.01
    const Element cp = makeElement(ElementType::ShuntCapacitor, 0.01 / omega);   // B = +0.01
    const Complex z = apply(cp, zPrev, f);
    CHECK(std::isfinite(z.real()));
    CHECK(std::isfinite(z.imag()));
    CHECK(std::abs(z) > 1e6);
}

TEST_CASE("nodeImpedances: n elements give n+1 nodes, last = Zin") {
    const double f = 2.4e9;
    const ElementList net = {makeElement(ElementType::SeriesInductor, 2.6526e-9),
                             makeElement(ElementType::ShuntCapacitor, 1.3263e-12)};
    const Complex zl(25.0, -15.0);
    const auto nodes = nodeImpedances(net, zl, f);
    REQUIRE(nodes.size() == 3);
    CHECK(nodes[0] == zl);
    CHECK(nodes[1].imag() == doctest::Approx(25.0).epsilon(1e-4));
    CHECK(nodes[2].real() == doctest::Approx(50.0).epsilon(1e-4));
    CHECK(inputImpedance(net, zl, f) == nodes.back());
}

TEST_CASE("Empty network: Zin = ZL") {
    const Complex zl(25.0, -15.0);
    CHECK(inputImpedance({}, zl, 1e9) == zl);
    CHECK(nodeImpedances({}, zl, 1e9).size() == 1);
}

TEST_CASE("sampledPath endpoints match the node impedances") {
    const double f = 2.4e9;
    const ElementList net = {makeElement(ElementType::SeriesInductor, 2.6526e-9),
                             makeElement(ElementType::ShuntCapacitor, 1.3263e-12)};
    const Complex zl(25.0, -15.0);
    const auto nodes = nodeImpedances(net, zl, f);
    for (std::size_t i = 0; i < net.size(); ++i) {
        const auto path = sampledPath(net, i, zl, f, kZ0, 64);
        REQUIRE(path.size() == 64);
        const Complex start = gammaFromZ(nodes[i], kZ0);
        const Complex end = gammaFromZ(nodes[i + 1], kZ0);
        CHECK(std::abs(path.front() - start) < 1e-12);
        CHECK(std::abs(path.back() - end) < 1e-12);
    }
    CHECK(sampledPath(net, 5, zl, f, kZ0).empty());   // indice hors limites
}

// --- Sens de déplacement sur l'abaque (§6.3) ------------------------------------

TEST_CASE("Series +jX moves clockwise around the r = 0.5 circle") {
    const double r = 0.5;
    const Complex center(r / (1.0 + r), 0.0);
    const Complex z1(r * kZ0, 0.3 * kZ0);
    const Complex z2 = addSeriesReactance(z1, 0.02 * kZ0);       // petit +jX
    const double a1 = angleAround(gammaFromZ(z1, kZ0), center);
    const double a2 = angleAround(gammaFromZ(z2, kZ0), center);
    CHECK(a2 < a1);   // angle décroissant = sens horaire
    // Et le point reste sur le cercle r = 0.5.
    CHECK(std::abs(gammaFromZ(z2, kZ0) - center) == doctest::Approx(1.0 / (1.0 + r)).epsilon(1e-9));
}

TEST_CASE("Series -jX (capacitor) moves counter-clockwise") {
    const double r = 0.5;
    const Complex center(r / (1.0 + r), 0.0);
    const Complex z1(r * kZ0, 0.3 * kZ0);
    const Complex z2 = addSeriesReactance(z1, -0.02 * kZ0);
    CHECK(angleAround(gammaFromZ(z2, kZ0), center) > angleAround(gammaFromZ(z1, kZ0), center));
}

TEST_CASE("Starting exactly at z = 0.5, a series +jX moves clockwise (angle decreases from pi)") {
    const Complex center(1.0 / 3.0, 0.0);
    const Complex g1 = gammaFromZ(Complex(0.5 * kZ0, 0.0), kZ0);
    const Complex g2 = gammaFromZ(addSeriesReactance(Complex(0.5 * kZ0, 0.0), 0.01 * kZ0), kZ0);
    const double a1 = angleAround(g1, center);   // = π
    const double a2 = angleAround(g2, center);
    CHECK(a1 == doctest::Approx(kPi));
    CHECK(a2 < a1);
}

TEST_CASE("Shunt +jB moves clockwise around the g = 2 circle") {
    const double g = 2.0;
    const Complex center(-g / (1.0 + g), 0.0);
    const Complex y1(g / kZ0, 0.0);                  // y = 2 (normalisée)
    const Complex z1 = 1.0 / y1;
    const Complex z2 = addShuntSusceptance(z1, 0.1 / kZ0);       // petit +jB (b = 0.1)
    const double a1 = angleAround(gammaFromZ(z1, kZ0), center);
    const double a2 = angleAround(gammaFromZ(z2, kZ0), center);
    CHECK(a2 < a1);
    CHECK(std::abs(gammaFromZ(z2, kZ0) - center) == doctest::Approx(1.0 / (1.0 + g)).epsilon(1e-9));
}

TEST_CASE("Shunt -jB (inductor) moves counter-clockwise") {
    const double g = 2.0;
    const Complex center(-g / (1.0 + g), 0.0);
    const Complex z1 = kZ0 / Complex(g, 0.3);        // y = 2 + j0.3
    const Complex z2 = addShuntSusceptance(z1, -0.05 / kZ0);
    CHECK(angleAround(gammaFromZ(z2, kZ0), center) > angleAround(gammaFromZ(z1, kZ0), center));
}

TEST_CASE("Full-arc direction through sampledArc: series L and series C") {
    const Complex zl(0.5 * kZ0, 0.0);
    const Complex center(1.0 / 3.0, 0.0);
    const double f = 1e9;
    // Petite bobine série (X = +10 Ω) : sens horaire.
    Element l = makeElement(ElementType::SeriesInductor, 10.0 / (kTwoPi * f));
    auto arcL = sampledArc(l, zl, f, kZ0, 16);
    CHECK(angleAround(arcL.back(), center) < angleAround(arcL[1], center));
    // Petit condensateur série (X = −10 Ω) à partir de z = 0.5 + j0.3 : sens anti-horaire.
    const Complex zl2(0.5 * kZ0, 0.3 * kZ0);
    Element c = makeElement(ElementType::SeriesCapacitor, 1.0 / (kTwoPi * f * 10.0));
    auto arcC = sampledArc(c, zl2, f, kZ0, 16);
    CHECK(angleAround(arcC.back(), center) > angleAround(arcC.front(), center));
}

// --- Cercles et arcs de l'abaque (formules de CLAUDE.md §5.3) -------------------

TEST_CASE("Any z = r + jx lies on its r-circle and its x-arc") {
    for (double r : {0.2, 0.5, 1.0, 2.0, 5.0}) {
        for (double x : {-5.0, -1.0, -0.2, 0.2, 0.5, 1.0, 2.0, 5.0}) {
            const Complex g = gammaFromZ(Complex(r * kZ0, x * kZ0), kZ0);
            INFO("r=" << r << " x=" << x);
            // cercle r : centre (r/(1+r), 0), rayon 1/(1+r)
            CHECK(std::abs(g - Complex(r / (1.0 + r), 0.0)) ==
                  doctest::Approx(1.0 / (1.0 + r)).epsilon(1e-9));
            // arc x : centre (1, 1/x), rayon 1/|x|
            CHECK(std::abs(g - Complex(1.0, 1.0 / x)) == doctest::Approx(1.0 / std::abs(x)).epsilon(1e-9));
        }
    }
}

TEST_CASE("Any y = g + jb lies on its g-circle and its b-arc") {
    for (double g : {0.2, 0.5, 1.0, 2.0, 5.0}) {
        for (double b : {-5.0, -1.0, -0.2, 0.2, 0.5, 1.0, 2.0, 5.0}) {
            const Complex gamma = gammaFromY(Complex(g / kZ0, b / kZ0), kZ0);
            INFO("g=" << g << " b=" << b);
            // cercle g : centre (−g/(1+g), 0), rayon 1/(1+g)
            CHECK(std::abs(gamma - Complex(-g / (1.0 + g), 0.0)) ==
                  doctest::Approx(1.0 / (1.0 + g)).epsilon(1e-9));
            // arc b : centre (−1, −1/b), rayon 1/|b|
            CHECK(std::abs(gamma - Complex(-1.0, -1.0 / b)) ==
                  doctest::Approx(1.0 / std::abs(b)).epsilon(1e-9));
        }
    }
}

// --- Modèle de charge (§6.6) ------------------------------------------------------

TEST_CASE("Constant load model: Z_L(f) = Z_L for any frequency") {
    LoadSpec load;
    load.zAtF0 = Complex(25.0, -15.0);
    load.model = LoadModel::Constant;
    for (double f : {1e6, 1e9, 2.4e9, 50e9}) {
        CHECK(loadImpedanceAt(load, 2.4e9, f) == Complex(25.0, -15.0));
    }
}

TEST_CASE("Series equivalent with X < 0 behaves like a capacitor") {
    LoadSpec load;
    load.zAtF0 = Complex(25.0, -15.0);
    load.model = LoadModel::SeriesEquivalent;
    const double f0 = 2.4e9;
    // Exactement Z_L à f₀.
    CHECK(loadImpedanceAt(load, f0, f0) == Complex(25.0, -15.0));
    // À 2f₀, |X| est divisée par 2 (X = −1/(ωC)). La partie réelle ne change pas.
    const Complex z2 = loadImpedanceAt(load, f0, 2.0 * f0);
    CHECK(z2.real() == 25.0);
    CHECK(z2.imag() == doctest::Approx(-15.0 / 2.0).epsilon(1e-12));
    // À f₀/2, |X| double.
    CHECK(loadImpedanceAt(load, f0, 0.5 * f0).imag() == doctest::Approx(-30.0).epsilon(1e-12));
}

TEST_CASE("Series equivalent with X > 0 behaves like an inductor") {
    LoadSpec load;
    load.zAtF0 = Complex(10.0, 20.0);
    load.model = LoadModel::SeriesEquivalent;
    const double f0 = 1e9;
    CHECK(loadImpedanceAt(load, f0, 2.0 * f0).imag() == doctest::Approx(40.0).epsilon(1e-12));
    CHECK(loadImpedanceAt(load, f0, 0.5 * f0).imag() == doctest::Approx(10.0).epsilon(1e-12));
}

TEST_CASE("Parallel equivalent keeps G and scales B") {
    LoadSpec load;
    load.zAtF0 = 1.0 / Complex(0.02, 0.01);     // G = 20 mS, B = +10 mS (condensateur en //)
    load.model = LoadModel::ParallelEquivalent;
    const double f0 = 1e9;
    CHECK(loadImpedanceAt(load, f0, f0) == load.zAtF0);
    const Complex y2 = 1.0 / loadImpedanceAt(load, f0, 2.0 * f0);
    CHECK(y2.real() == doctest::Approx(0.02).epsilon(1e-12));
    CHECK(y2.imag() == doctest::Approx(0.02).epsilon(1e-12));   // B double
    // Bobine en parallèle (B < 0) : |B| divisée par 2 à 2f₀.
    load.zAtF0 = 1.0 / Complex(0.02, -0.01);
    const Complex y3 = 1.0 / loadImpedanceAt(load, f0, 2.0 * f0);
    CHECK(y3.real() == doctest::Approx(0.02).epsilon(1e-12));
    CHECK(y3.imag() == doctest::Approx(-0.005).epsilon(1e-12));
}

// --- Validation -----------------------------------------------------------------------

TEST_CASE("Validation of Z0 and frequency") {
    CHECK(validateZ0(50.0).ok);
    CHECK_FALSE(validateZ0(0.0).ok);
    CHECK_FALSE(validateZ0(-50.0).ok);
    CHECK_FALSE(validateZ0(std::nan("")).ok);
    CHECK(validateFrequency(2.4e9).ok);
    CHECK(validateFrequency(1e3).ok);
    CHECK(validateFrequency(1e12).ok);
    CHECK_FALSE(validateFrequency(999.0).ok);
    CHECK_FALSE(validateFrequency(1.1e12).ok);
    CHECK_FALSE(validateFrequency(0.0).ok);
    CHECK_FALSE(validateFrequency(std::numeric_limits<double>::infinity()).ok);
    CHECK_FALSE(validateFrequency(999.0).message.empty());
}

TEST_CASE("Validation of source and load impedances") {
    CHECK(validateSourceImpedance(Complex(50.0, 0.0)).ok);
    CHECK(validateSourceImpedance(Complex(30.0, 20.0)).ok);
    CHECK_FALSE(validateSourceImpedance(Complex(0.0, 20.0)).ok);
    CHECK_FALSE(validateSourceImpedance(Complex(-5.0, 0.0)).ok);
    CHECK(validateLoadImpedance(Complex(25.0, -15.0)).ok);
    CHECK_FALSE(validateLoadImpedance(Complex(0.0, 10.0)).ok);     // purement réactive
    CHECK_FALSE(validateLoadImpedance(Complex(-1.0, 10.0)).ok);    // non passive
    CHECK_FALSE(validateLoadImpedance(Complex(std::nan(""), 0.0)).ok);
}

TEST_CASE("Validation of component values") {
    CHECK(validateComponentValue(1e-12).ok);
    CHECK_FALSE(validateComponentValue(0.0).ok);
    CHECK_FALSE(validateComponentValue(-1e-9).ok);
    CHECK_FALSE(validateComponentValue(std::numeric_limits<double>::infinity()).ok);
    CHECK_FALSE(validateComponentValue(std::nan("")).ok);
}
