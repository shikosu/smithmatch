// =============================================================================
// test_solver.cpp — Tests du solveur L / Pi / T (spec §6.4 et §6.5)
// =============================================================================
// Principe : chaque solution est vérifiée de DEUX façons indépendantes :
//   1. les valeurs de composants comparées à des références publiées (Pozar) ;
//   2. la boucle complète : on recalcule Z_in avec Network.cpp et on exige
//      |Γ_m| < 1e-9 (le solveur et le calcul de réseau ne partagent aucune formule).
// =============================================================================
#include <doctest/doctest.h>

#include <algorithm>

#include "core/Network.hpp"
#include "core/RfMath.hpp"
#include "core/Solver.hpp"

using namespace smith;

namespace {

constexpr double kTightGamma = 1.0e-9;

// |Γ_m| obtenu en repassant par le calcul de réseau (indépendant du solveur).
double gammaOfSolution(const Solution& s, Complex zSource, Complex zLoad, double f0) {
    const Complex zIn = inputImpedance(s.elements, zLoad, f0);
    return std::abs(matchGamma(zIn, zSource));
}

// Cherche une solution de topologie donnée dont l'élément 0 est du type donné.
const Solution* findSolution(const SolverResult& r, SolutionTopology topo, ElementType firstType) {
    for (const Solution& s : r.solutions) {
        if (s.topology == topo && !s.elements.empty() && s.elements[0].type == firstType) {
            return &s;
        }
    }
    return nullptr;
}

int countTopology(const SolverResult& r, SolutionTopology topo) {
    return static_cast<int>(std::count_if(r.solutions.begin(), r.solutions.end(),
                                          [topo](const Solution& s) { return s.topology == topo; }));
}

} // namespace

TEST_CASE("L solver, case 1 (Pozar 5.1): ZL = 200 - j100, Zs = 100, 500 MHz") {
    const Complex zs(100.0, 0.0), zl(200.0, -100.0);
    const double f0 = 500e6;
    const SolverResult r = solveL(zs, zl, f0);
    REQUIRE(r.ok);
    CHECK(countTopology(r, SolutionTopology::LShuntFirst) == 2);
    CHECK(countTopology(r, SolutionTopology::LSeriesFirst) == 0);
    REQUIRE(r.solutions.size() == 2);

    // Solution 1 : C shunt 0.9228 pF puis L série 38.98 nH (passe-bas).
    const Solution* a = findSolution(r, SolutionTopology::LShuntFirst, ElementType::ShuntCapacitor);
    REQUIRE(a != nullptr);
    REQUIRE(a->elements.size() == 2);
    CHECK(a->elements[0].value == doctest::Approx(0.9228e-12).epsilon(1e-3));
    CHECK(a->elements[1].type == ElementType::SeriesInductor);
    CHECK(a->elements[1].value == doctest::Approx(38.98e-9).epsilon(1e-3));
    CHECK(a->type == SolutionType::LowPass);

    // Solution 2 : L shunt 46.14 nH puis C série 2.599 pF (passe-haut).
    const Solution* b = findSolution(r, SolutionTopology::LShuntFirst, ElementType::ShuntInductor);
    REQUIRE(b != nullptr);
    REQUIRE(b->elements.size() == 2);
    CHECK(b->elements[0].value == doctest::Approx(46.14e-9).epsilon(1e-3));
    CHECK(b->elements[1].type == ElementType::SeriesCapacitor);
    CHECK(b->elements[1].value == doctest::Approx(2.599e-12).epsilon(1e-3));
    CHECK(b->type == SolutionType::HighPass);

    for (const Solution& s : r.solutions) {
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("L solver, case 2: ZL = 25 - j15, Zs = 50, 2.4 GHz") {
    const Complex zs(50.0, 0.0), zl(25.0, -15.0);
    const double f0 = 2.4e9;
    const SolverResult r = solveL(zs, zl, f0);
    REQUIRE(r.ok);
    CHECK(countTopology(r, SolutionTopology::LShuntFirst) == 0);
    CHECK(countTopology(r, SolutionTopology::LSeriesFirst) == 2);

    const Solution* a = findSolution(r, SolutionTopology::LSeriesFirst, ElementType::SeriesInductor);
    REQUIRE(a != nullptr);
    REQUIRE(a->elements.size() == 2);
    CHECK(a->elements[0].value == doctest::Approx(2.6526e-9).epsilon(1e-3));
    CHECK(a->elements[1].type == ElementType::ShuntCapacitor);
    CHECK(a->elements[1].value == doctest::Approx(1.3263e-12).epsilon(1e-3));

    const Solution* b = findSolution(r, SolutionTopology::LSeriesFirst, ElementType::SeriesCapacitor);
    REQUIRE(b != nullptr);
    REQUIRE(b->elements.size() == 2);
    CHECK(b->elements[0].value == doctest::Approx(6.6315e-12).epsilon(1e-3));
    CHECK(b->elements[1].type == ElementType::ShuntInductor);
    CHECK(b->elements[1].value == doctest::Approx(3.3157e-9).epsilon(1e-3));

    for (const Solution& s : r.solutions) {
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("L solver, case 3: complex source Zs = 30 + j20, ZL = 80 - j40, 1 GHz") {
    const Complex zs(30.0, 20.0), zl(80.0, -40.0);
    const double f0 = 1e9;
    const SolverResult r = solveL(zs, zl, f0);
    REQUIRE(r.ok);
    CHECK(countTopology(r, SolutionTopology::LShuntFirst) == 2);
    CHECK(countTopology(r, SolutionTopology::LSeriesFirst) == 0);
    for (const Solution& s : r.solutions) {
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("L solver, case 4: already matched load") {
    const SolverResult r = solveL(Complex(50.0, 0.0), Complex(50.0, 0.0), 1e9);
    CHECK(r.ok);
    CHECK(r.alreadyMatched);
    CHECK(r.solutions.empty());
    CHECK(r.message.find("already matched") != std::string::npos);
}

TEST_CASE("L solver, case 5: invalid load (Re ZL <= 0) is a validation error") {
    const SolverResult r = solveL(Complex(50.0, 0.0), Complex(-10.0, 5.0), 1e9);
    CHECK_FALSE(r.ok);
    CHECK_FALSE(r.message.empty());
    CHECK(r.solutions.empty());
}

TEST_CASE("L solver: invalid frequency is a validation error") {
    const SolverResult r = solveL(Complex(50.0, 0.0), Complex(25.0, 0.0), 0.0);
    CHECK_FALSE(r.ok);
}

TEST_CASE("Pi/T solvers: Qmin for Zs = 50, ZL = 10") {
    const Complex zs(50.0, 0.0), zl(10.0, 0.0);
    CHECK(piQMin(zs, zl) == doctest::Approx(2.0).epsilon(1e-9));
    CHECK(tQMin(zs, zl) == doctest::Approx(2.0).epsilon(1e-9));
}

TEST_CASE("Pi solver: Zs = 50, ZL = 10, 100 MHz, Q = 3") {
    const Complex zs(50.0, 0.0), zl(10.0, 0.0);
    const double f0 = 100e6;
    const SolverResult r = solvePi(zs, zl, f0, 3.0);
    REQUIRE(r.ok);
    CHECK(r.qMin == doctest::Approx(2.0).epsilon(1e-9));
    CHECK(r.solutions.size() == 4);
    // R_v = 50/(1 + 9) = 5 Ω : contrôle indirect — chaque étage L est vu avec Q = 3.
    for (const Solution& s : r.solutions) {
        CHECK(s.topology == SolutionTopology::Pi);
        CHECK(s.nominalQ == doctest::Approx(3.0));
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("T solver: Zs = 50, ZL = 10, 100 MHz, Q = 3") {
    const Complex zs(50.0, 0.0), zl(10.0, 0.0);
    const double f0 = 100e6;
    const SolverResult r = solveT(zs, zl, f0, 3.0);
    REQUIRE(r.ok);
    CHECK(r.solutions.size() == 4);
    // R_v = 10 × (1 + 9) = 100 Ω.
    for (const Solution& s : r.solutions) {
        CHECK(s.topology == SolutionTopology::T);
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("Pi/T solvers: Q below Q_min gives an explicit error") {
    const Complex zs(50.0, 0.0), zl(10.0, 0.0);
    const SolverResult pi = solvePi(zs, zl, 100e6, 1.5);
    CHECK_FALSE(pi.ok);
    CHECK(pi.message.find("Q_min") != std::string::npos);
    CHECK(pi.solutions.empty());
    const SolverResult t = solveT(zs, zl, 100e6, 1.5);
    CHECK_FALSE(t.ok);
    CHECK(t.message.find("Q_min") != std::string::npos);
}

TEST_CASE("Pi/T solvers: Q non positive or NaN is rejected") {
    const Complex zs(50.0, 0.0), zl(10.0, 0.0);
    CHECK_FALSE(solvePi(zs, zl, 100e6, -1.0).ok);
    CHECK_FALSE(solveT(zs, zl, 100e6, std::nan("")).ok);
}

TEST_CASE("Pi/T solvers work with a reactive load and a complex source") {
    const Complex zs(40.0, 15.0), zl(12.0, 20.0);
    const double f0 = 433e6;
    const SolverResult pi = solvePi(zs, zl, f0, piQMin(zs, zl) + 2.0);
    REQUIRE(pi.ok);
    REQUIRE_FALSE(pi.solutions.empty());
    for (const Solution& s : pi.solutions) {
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
    const SolverResult t = solveT(zs, zl, f0, tQMin(zs, zl) + 2.0);
    REQUIRE(t.ok);
    REQUIRE_FALSE(t.solutions.empty());
    for (const Solution& s : t.solutions) {
        CHECK(gammaOfSolution(s, zs, zl, f0) < kTightGamma);
    }
}

TEST_CASE("Solution names are stable") {
    CHECK(std::string(solutionTypeName(SolutionType::LowPass)) == "Low-pass");
    CHECK(std::string(solutionTypeName(SolutionType::HighPass)) == "High-pass");
    CHECK(std::string(solutionTypeName(SolutionType::Mixed)) == "Mixed");
}
