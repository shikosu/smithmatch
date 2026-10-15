// =============================================================================
// test_sweep.cpp — Tests du balayage, de la bande passante et du modèle de charge (spec §6.6)
// =============================================================================
#include <doctest/doctest.h>

#include <cmath>

#include "core/Load.hpp"
#include "core/RfMath.hpp"
#include "core/Solver.hpp"
#include "core/Sweep.hpp"

using namespace smith;

namespace {

// Projet de l'exemple de Pozar 5.1 : ZL = 200 − j100, Zs = Z0 = 100, f0 = 500 MHz.
Project pozarProject() {
    Project p = defaultProject();
    p.z0 = 100.0;
    p.f0Hz = 500e6;
    p.zSource = Complex(100.0, 0.0);
    p.load.zAtF0 = Complex(200.0, -100.0);
    p.load.model = LoadModel::SeriesEquivalent;
    p.sweep.fMinHz = 300e6;
    p.sweep.fMaxHz = 800e6;
    p.sweep.points = 51;      // pas de 10 MHz : 300 et 800 MHz sont des points
    p.sweep.logScale = false;
    return p;
}

ElementList pozarSolution1(const Project& p) {
    const SolverResult r = solveL(p.zSource, p.load.zAtF0, p.f0Hz);
    for (const Solution& s : r.solutions) {
        if (!s.elements.empty() && s.elements[0].type == ElementType::ShuntCapacitor) {
            return s.elements;
        }
    }
    return {};
}

} // namespace

TEST_CASE("Sweep with the Pozar solution: perfect match at f0, mismatch away from it") {
    Project p = pozarProject();
    p.network = pozarSolution1(p);
    REQUIRE(p.network.size() == 2);
    const SweepResult sweep = runSweep(p, p.network);
    REQUIRE(sweep.ok);
    REQUIRE(sweep.f0Index != SweepResult::kNoIndex);
    CHECK(sweep.points[sweep.f0Index].fHz == 500e6);
    CHECK(sweep.points[sweep.f0Index].gammaMag < 1e-9);
    CHECK(sweep.points.front().fHz == doctest::Approx(300e6));
    CHECK(sweep.points.back().fHz == doctest::Approx(800e6));
    CHECK(sweep.points.front().gammaMag > 0.01);
    CHECK(sweep.points.back().gammaMag > 0.01);
    // Triés par fréquence croissante.
    for (std::size_t i = 1; i < sweep.points.size(); ++i) {
        CHECK(sweep.points[i].fHz > sweep.points[i - 1].fHz);
    }
}

TEST_CASE("Bandwidth contains f0 and its edges sit at the VSWR threshold") {
    Project p = pozarProject();
    p.network = pozarSolution1(p);
    p.sweep.points = 501;     // balayage fin pour que l'interpolation soit très précise
    const SweepResult sweep = runSweep(p, p.network);
    const BandwidthResult bw = bandwidth(sweep, 2.0);
    REQUIRE(bw.ok);
    CHECK(bw.fLowHz < 500e6);
    CHECK(bw.fHighHz > 500e6);
    CHECK_FALSE(bw.lowOpen);
    CHECK_FALSE(bw.highOpen);
    CHECK(bw.widthHz == doctest::Approx(bw.fHighHz - bw.fLowHz));
    // Contrôle indépendant : aux bords, le VSWR recalculé directement vaut ≈ 2.
    for (const double f : {bw.fLowHz, bw.fHighHz}) {
        const MatchMetrics m = evaluateMatch(projectInputImpedanceAt(p, f), p.zSource);
        CHECK(m.vswr == doctest::Approx(2.0).epsilon(0.01));
    }
}

TEST_CASE("Bandwidth fails with an explicit message when VSWR(f0) exceeds the threshold") {
    Project p = pozarProject();          // réseau vide : VSWR(f0) > 2
    const SweepResult sweep = runSweep(p, p.network);
    REQUIRE(sweep.ok);
    const BandwidthResult bw = bandwidth(sweep, 2.0);
    CHECK_FALSE(bw.ok);
    CHECK(bw.message.find("no bandwidth") != std::string::npos);
}

TEST_CASE("Bandwidth reports an open band when it reaches the sweep edges") {
    Project p = pozarProject();
    p.network = pozarSolution1(p);
    p.sweep.fMinHz = 480e6;
    p.sweep.fMaxHz = 520e6;
    const SweepResult sweep = runSweep(p, p.network);
    const BandwidthResult bw = bandwidth(sweep, 2.0);
    REQUIRE(bw.ok);
    CHECK(bw.lowOpen);
    CHECK(bw.highOpen);
}

TEST_CASE("Sweep: f0 outside the range gives no f0 index and no bandwidth") {
    Project p = pozarProject();
    p.sweep.fMinHz = 600e6;
    p.sweep.fMaxHz = 800e6;
    const SweepResult sweep = runSweep(p, p.network);
    REQUIRE(sweep.ok);
    CHECK(sweep.f0Index == SweepResult::kNoIndex);
    CHECK_FALSE(bandwidth(sweep, 2.0).ok);
}

TEST_CASE("Sweep: logarithmic scale has a constant frequency ratio") {
    Project p = pozarProject();
    p.sweep.logScale = true;
    p.sweep.fMinHz = 100e6;
    p.sweep.fMaxHz = 1e9;
    p.f0Hz = 2e9;                       // hors plage : aucun point ajouté
    p.sweep.points = 11;
    const SweepResult sweep = runSweep(p, p.network);
    REQUIRE(sweep.ok);
    REQUIRE(sweep.points.size() == 11);
    const double ratio = sweep.points[1].fHz / sweep.points[0].fHz;
    CHECK(ratio == doctest::Approx(std::pow(10.0, 0.1)).epsilon(1e-9));
    for (std::size_t i = 2; i < sweep.points.size(); ++i) {
        CHECK(sweep.points[i].fHz / sweep.points[i - 1].fHz == doctest::Approx(ratio).epsilon(1e-9));
    }
}

TEST_CASE("Sweep: invalid settings are reported, not computed") {
    Project p = pozarProject();
    p.sweep.fMinHz = 800e6;
    p.sweep.fMaxHz = 300e6;
    CHECK_FALSE(runSweep(p, p.network).ok);
    p = pozarProject();
    p.sweep.points = 3;
    CHECK_FALSE(runSweep(p, p.network).ok);
    p = pozarProject();
    p.sweep.fMinHz = -1.0;
    CHECK_FALSE(runSweep(p, p.network).ok);
}

TEST_CASE("S11 is floored for plotting when the match is perfect") {
    Project p = pozarProject();
    p.network = pozarSolution1(p);
    const SweepResult sweep = runSweep(p, p.network);
    CHECK(sweep.points[sweep.f0Index].s11Db >= kS11FloorDb);
    CHECK(std::isfinite(sweep.points[sweep.f0Index].s11Db));
}

TEST_CASE("Load model: Constant keeps ZL at every frequency") {
    LoadSpec load;
    load.zAtF0 = Complex(25.0, -15.0);
    load.model = LoadModel::Constant;
    const Complex z = loadImpedanceAt(load, 2.4e9, 4.8e9);
    CHECK(z.real() == doctest::Approx(25.0));
    CHECK(z.imag() == doctest::Approx(-15.0));
}

TEST_CASE("Load model: SeriesEquivalent with XL < 0 is a series capacitor, Im ZL(2 f0) = XL/2") {
    LoadSpec load;
    load.zAtF0 = Complex(25.0, -15.0);
    load.model = LoadModel::SeriesEquivalent;
    const Complex z = loadImpedanceAt(load, 2.4e9, 4.8e9);
    CHECK(z.real() == doctest::Approx(25.0));
    CHECK(z.imag() == doctest::Approx(-7.5));
}

TEST_CASE("Load model: SeriesEquivalent with XL > 0 is a series inductor, Im ZL(2 f0) = 2 XL") {
    LoadSpec load;
    load.zAtF0 = Complex(25.0, 15.0);
    load.model = LoadModel::SeriesEquivalent;
    const Complex z = loadImpedanceAt(load, 1e9, 2e9);
    CHECK(z.imag() == doctest::Approx(30.0));
}

TEST_CASE("Load model: ParallelEquivalent keeps G and scales B like a capacitor") {
    LoadSpec load;
    load.zAtF0 = Complex(50.0, 0.0) / (Complex(1.0, 0.0) + Complex(0.0, 1.0));   // Y = 0.02·(1 + j)
    load.model = LoadModel::ParallelEquivalent;
    const Complex y0 = 1.0 / load.zAtF0;
    const Complex y = 1.0 / loadImpedanceAt(load, 1e9, 2e9);
    CHECK(y.real() == doctest::Approx(y0.real()));
    CHECK(y.imag() == doctest::Approx(2.0 * y0.imag()));     // B > 0 : condensateur, B ∝ f
}

TEST_CASE("gammaFromVswr inverts the VSWR formula") {
    CHECK(gammaFromVswr(2.0) == doctest::Approx(1.0 / 3.0));
    CHECK(gammaFromVswr(1.0) == doctest::Approx(0.0));
    CHECK(vswr(gammaFromVswr(1.5)) == doctest::Approx(1.5));
}
