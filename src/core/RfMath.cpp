// =============================================================================
// RfMath.cpp — Implémentation des formules radiofréquence
// =============================================================================
#include "core/RfMath.hpp"

#include <cmath>
#include <limits>

namespace smith {

namespace {
constexpr double kInfinity = std::numeric_limits<double>::infinity();
}

// Γ = (Z − Z₀)/(Z + Z₀)
Complex gammaFromZ(Complex z, double z0) {
    const Complex denominator = z + z0;
    if (std::abs(denominator) < kTinyImpedance) {
        return Complex(kInfinity, 0.0);   // cas non physique : on évite la division par zéro
    }
    return (z - z0) / denominator;
}

// Z = Z₀·(1 + Γ)/(1 − Γ)
Complex zFromGamma(Complex gamma, double z0) {
    const Complex denominator = 1.0 - gamma;
    if (std::abs(denominator) < kZeroRelTolerance * kZeroRelTolerance) {
        return Complex(kOpenCircuitOhm, 0.0);   // Γ = 1 : circuit ouvert
    }
    return z0 * (1.0 + gamma) / denominator;
}

// Γ = (1 − y)/(1 + y) avec y = Y·Z₀ (admittance normalisée)
Complex gammaFromY(Complex y, double z0) {
    const Complex yn = y * z0;
    const Complex denominator = 1.0 + yn;
    if (std::abs(denominator) < kTinyImpedance) {
        return Complex(kInfinity, 0.0);
    }
    return (1.0 - yn) / denominator;
}

// Γ_m = (Z_in − Z_s*)/(Z_in + Z_s)
Complex matchGamma(Complex zIn, Complex zSource) {
    const Complex denominator = zIn + zSource;
    if (std::abs(denominator) < kTinyImpedance) {
        return Complex(kInfinity, 0.0);
    }
    return (zIn - std::conj(zSource)) / denominator;
}

// VSWR = (1 + |Γ|)/(1 − |Γ|)
double vswr(double gammaMag) {
    if (gammaMag >= 1.0 - kVswrInfinityGammaMargin) {
        return kInfinity;
    }
    if (gammaMag <= 0.0) {
        return 1.0;
    }
    return (1.0 + gammaMag) / (1.0 - gammaMag);
}

// RL = −20·log10|Γ|
double returnLossDb(double gammaMag) {
    if (gammaMag < kReturnLossInfiniteGamma) {
        return kInfinity;
    }
    return -20.0 * std::log10(gammaMag);
}

// S11 = 20·log10|Γ|
double s11Db(double gammaMag) {
    if (gammaMag < kReturnLossInfiniteGamma) {
        return -kInfinity;
    }
    return 20.0 * std::log10(gammaMag);
}

// ML = −10·log10(1 − |Γ|²)
double mismatchLossDb(double gammaMag) {
    const double transmitted = 1.0 - gammaMag * gammaMag;   // part de puissance transmise
    if (transmitted <= 0.0) {
        return kInfinity;
    }
    return -10.0 * std::log10(transmitted);
}

MatchMetrics evaluateMatch(Complex zIn, Complex zSource) {
    MatchMetrics m;
    m.zIn = zIn;
    m.gammaM = matchGamma(zIn, zSource);
    m.gammaMag = std::abs(m.gammaM);
    m.gammaAngleDeg = std::arg(m.gammaM) * 180.0 / kPi;
    m.vswr = vswr(m.gammaMag);
    m.returnLossDb = returnLossDb(m.gammaMag);
    m.s11Db = s11Db(m.gammaMag);
    m.mismatchLossDb = mismatchLossDb(m.gammaMag);
    return m;
}

// pixel = centre + rayon·(Re Γ, −Im Γ)
Point2 gammaToScreen(Complex gamma, Point2 center, double radiusPx) {
    return Point2{center.x + radiusPx * gamma.real(), center.y - radiusPx * gamma.imag()};
}

// Γ = ((px − cx)/rayon, −(py − cy)/rayon)
Complex screenToGamma(Point2 pixel, Point2 center, double radiusPx) {
    return Complex((pixel.x - center.x) / radiusPx, -(pixel.y - center.y) / radiusPx);
}

} // namespace smith
