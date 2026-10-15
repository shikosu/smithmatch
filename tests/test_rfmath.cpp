// =============================================================================
// test_rfmath.cpp — Tests des formules RF (spec §6.1)
// =============================================================================
#include <doctest/doctest.h>

#include <cmath>
#include <limits>

#include "core/RfMath.hpp"

using namespace smith;

namespace {
constexpr double kZ0 = 50.0;
constexpr double kInf = std::numeric_limits<double>::infinity();
}

TEST_CASE("Z = Z0 gives a perfect match") {
    const Complex g = gammaFromZ(Complex(50.0, 0.0), kZ0);
    CHECK(std::abs(g) == doctest::Approx(0.0).epsilon(1e-9));
    CHECK(vswr(std::abs(g)) == doctest::Approx(1.0));
    CHECK(returnLossDb(std::abs(g)) == kInf);
}

TEST_CASE("Z = 100 ohm on 50 ohm") {
    const Complex g = gammaFromZ(Complex(100.0, 0.0), kZ0);
    CHECK(g.real() == doctest::Approx(1.0 / 3.0).epsilon(1e-9));
    CHECK(g.imag() == doctest::Approx(0.0).epsilon(1e-9));
    CHECK(vswr(std::abs(g)) == doctest::Approx(2.0).epsilon(1e-9));
    CHECK(returnLossDb(std::abs(g)) == doctest::Approx(9.5424).epsilon(1e-5));
}

TEST_CASE("Z = 25 ohm on 50 ohm") {
    const Complex g = gammaFromZ(Complex(25.0, 0.0), kZ0);
    CHECK(g.real() == doctest::Approx(-1.0 / 3.0).epsilon(1e-9));
    CHECK(vswr(std::abs(g)) == doctest::Approx(2.0).epsilon(1e-9));
}

TEST_CASE("Z = 50 + j50 ohm") {
    const Complex g = gammaFromZ(Complex(50.0, 50.0), kZ0);
    CHECK(g.real() == doctest::Approx(0.2).epsilon(1e-9));
    CHECK(g.imag() == doctest::Approx(0.4).epsilon(1e-9));
    CHECK(std::abs(g) == doctest::Approx(0.44721).epsilon(1e-5));
    CHECK(vswr(std::abs(g)) == doctest::Approx(2.6180).epsilon(1e-4));
    CHECK(returnLossDb(std::abs(g)) == doctest::Approx(6.9897).epsilon(1e-5));
    CHECK(s11Db(std::abs(g)) == doctest::Approx(-6.9897).epsilon(1e-5));
}

TEST_CASE("Mismatch loss") {
    CHECK(mismatchLossDb(1.0 / 3.0) == doctest::Approx(0.5115).epsilon(1e-4));
    CHECK(mismatchLossDb(0.0) == doctest::Approx(0.0).epsilon(1e-12));
    CHECK(mismatchLossDb(1.0) == kInf);
}

TEST_CASE("VSWR limits") {
    CHECK(vswr(1.0) == kInf);
    CHECK(vswr(1.0 - 1e-13) == kInf);
    CHECK(vswr(0.0) == 1.0);
    CHECK(std::isfinite(vswr(0.999)));
}

TEST_CASE("zFromGamma(gammaFromZ(Z)) round trip") {
    for (const Complex z : {Complex(50.0, 0.0), Complex(25.0, -15.0), Complex(200.0, -100.0),
                            Complex(0.5, 300.0), Complex(1000.0, 1000.0), Complex(12.3, 0.0)}) {
        const Complex back = zFromGamma(gammaFromZ(z, kZ0), kZ0);
        CHECK(back.real() == doctest::Approx(z.real()).epsilon(1e-9));
        CHECK(back.imag() == doctest::Approx(z.imag()).epsilon(1e-9));
    }
}

TEST_CASE("Open circuit and short circuit stay finite") {
    CHECK(gammaFromZ(Complex(0.0, 0.0), kZ0).real() == doctest::Approx(-1.0));
    const Complex open = zFromGamma(Complex(1.0, 0.0), kZ0);
    CHECK(std::isfinite(open.real()));
    CHECK(open.real() > 1e200);
}

TEST_CASE("gammaFromY matches gammaFromZ(1/Y)") {
    const Complex y(0.02, 0.013);
    const Complex a = gammaFromY(y, kZ0);
    const Complex b = gammaFromZ(1.0 / y, kZ0);
    CHECK(a.real() == doctest::Approx(b.real()).epsilon(1e-12));
    CHECK(a.imag() == doctest::Approx(b.imag()).epsilon(1e-12));
}

TEST_CASE("matchGamma reduces to the classic Gamma for a real source") {
    const Complex zin(100.0, 0.0);
    const Complex g = matchGamma(zin, Complex(50.0, 0.0));
    CHECK(g.real() == doctest::Approx(1.0 / 3.0).epsilon(1e-12));
}

TEST_CASE("matchGamma is zero when Zin = conj(Zs) for a complex source") {
    const Complex zs(30.0, 20.0);
    CHECK(std::abs(matchGamma(std::conj(zs), zs)) < 1e-15);
    // Contre-exemple : Zin = Zs (et non son conjugué) n'est PAS adapté.
    CHECK(std::abs(matchGamma(zs, zs)) > 0.1);
}

TEST_CASE("Screen mapping: +j is above the centre (smaller y)") {
    const Point2 center{400.0, 300.0};
    const double radius = 200.0;
    const Point2 up = gammaToScreen(Complex(0.0, 1.0), center, radius);
    CHECK(up.y < center.y);
    CHECK(up.x == doctest::Approx(center.x));
    CHECK(up.y == doctest::Approx(100.0));
    const Point2 right = gammaToScreen(Complex(1.0, 0.0), center, radius);
    CHECK(right.x == doctest::Approx(600.0));   // Γ = +1 (circuit ouvert) à droite
    const Point2 left = gammaToScreen(Complex(-1.0, 0.0), center, radius);
    CHECK(left.x == doctest::Approx(200.0));    // Γ = −1 (court-circuit) à gauche
}

TEST_CASE("Screen mapping round trip") {
    const Point2 center{512.5, 384.25};
    for (const Complex g : {Complex(0.3, -0.4), Complex(-0.9, 0.1), Complex(0.0, 0.0)}) {
        const Complex back = screenToGamma(gammaToScreen(g, center, 321.0), center, 321.0);
        CHECK(back.real() == doctest::Approx(g.real()).epsilon(1e-12));
        CHECK(back.imag() == doctest::Approx(g.imag()).epsilon(1e-12));
    }
}
