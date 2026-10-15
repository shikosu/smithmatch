// =============================================================================
// Sweep.cpp — Implémentation du balayage et de la bande passante
// =============================================================================
#include "core/Sweep.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

#include "core/Network.hpp"
#include "core/RfMath.hpp"
#include "core/Units.hpp"
#include "core/Validation.hpp"

namespace smith {

namespace {

constexpr int kCsvSignificantDigits = 12;

// Évalue un point à la fréquence f.
SweepPoint evaluatePoint(const Project& p, const ElementList& network, double fHz) {
    SweepPoint point;
    point.fHz = fHz;
    point.zIn = inputImpedance(network, projectLoadAt(p, fHz), fHz);
    const MatchMetrics m = evaluateMatch(point.zIn, p.zSource);
    point.gammaMag = m.gammaMag;
    point.gammaAngleDeg = m.gammaAngleDeg;
    point.s11Db = std::max(m.s11Db, kS11FloorDb);   // −∞ → plancher pour le tracé
    point.vswr = m.vswr;
    return point;
}

SweepResult fail(const std::string& message) {
    SweepResult r;
    r.ok = false;
    r.message = message;
    return r;
}

} // namespace

double gammaFromVswr(double vswrValue) {
    if (std::isinf(vswrValue)) {
        return 1.0;
    }
    return (vswrValue - 1.0) / (vswrValue + 1.0);
}

SweepResult runSweep(const Project& p, const ElementList& network) {
    const SweepSettings& s = p.sweep;

    // --- Contrôle des réglages ------------------------------------------------------------
    ValidationResult check = validateFrequency(s.fMinHz);
    if (!check.ok) {
        return fail("Sweep start: " + check.message);
    }
    check = validateFrequency(s.fMaxHz);
    if (!check.ok) {
        return fail("Sweep stop: " + check.message);
    }
    if (!(s.fMinHz < s.fMaxHz)) {
        return fail("Sweep start must be lower than sweep stop.");
    }
    if (s.points < kMinSweepPoints || s.points > kMaxSweepPoints) {
        return fail("Number of points must be between " + std::to_string(kMinSweepPoints) + " and " +
                    std::to_string(kMaxSweepPoints) + ".");
    }

    // --- Fréquences : linéaires ou logarithmiques, bornes incluses -------------------------------------
    std::vector<double> freqs;
    freqs.reserve(static_cast<std::size_t>(s.points) + 1);
    const double steps = static_cast<double>(s.points - 1);
    for (int i = 0; i < s.points; ++i) {
        const double t = static_cast<double>(i) / steps;    // 0 → 1
        freqs.push_back(s.logScale ? s.fMinHz * std::pow(s.fMaxHz / s.fMinHz, t)
                                   : s.fMinHz + t * (s.fMaxHz - s.fMinHz));
    }
    freqs.back() = s.fMaxHz;   // évite l'erreur d'arrondi sur la borne haute

    // --- f₀ est ajouté s'il est dans la plage et pas déjà (quasi) présent --------------------------------
    if (p.f0Hz >= s.fMinHz && p.f0Hz <= s.fMaxHz) {
        const auto near = std::find_if(freqs.begin(), freqs.end(), [&](double f) {
            return std::abs(f - p.f0Hz) <= kSameFrequencyRelTolerance * p.f0Hz;
        });
        if (near != freqs.end()) {
            *near = p.f0Hz;                      // on cale exactement sur f₀
        } else {
            freqs.insert(std::upper_bound(freqs.begin(), freqs.end(), p.f0Hz), p.f0Hz);
        }
    }

    SweepResult result;
    result.ok = true;
    result.points.reserve(freqs.size());
    for (std::size_t i = 0; i < freqs.size(); ++i) {
        result.points.push_back(evaluatePoint(p, network, freqs[i]));
        if (freqs[i] == p.f0Hz) {
            result.f0Index = i;
        }
    }
    return result;
}

BandwidthResult bandwidth(const SweepResult& sweep, double vswrThreshold) {
    BandwidthResult out;
    if (!sweep.ok || sweep.points.empty()) {
        out.message = sweep.message.empty() ? "No sweep data." : sweep.message;
        return out;
    }
    if (!(vswrThreshold > 1.0) || std::isinf(vswrThreshold)) {
        out.message = "VSWR threshold must be greater than 1.";
        return out;
    }
    if (sweep.f0Index == SweepResult::kNoIndex) {
        out.message = "f0 is outside the swept range.";
        return out;
    }

    const std::vector<SweepPoint>& pts = sweep.points;
    const double gammaLimit = gammaFromVswr(vswrThreshold);
    const auto inBand = [&](std::size_t i) { return pts[i].gammaMag <= gammaLimit; };

    if (!inBand(sweep.f0Index)) {
        out.message = "VSWR at f0 (" + (std::isinf(pts[sweep.f0Index].vswr) ? std::string("inf")
                                                                              : formatSig(pts[sweep.f0Index].vswr, 3)) +
                      ") is above the threshold " + formatSig(vswrThreshold, 3) + ": no bandwidth.";
        return out;
    }

    // Franchissement entre un point conforme « good » et un point non conforme « bad » :
    // interpolation linéaire de |Γ| → fréquence où |Γ| = gammaLimit.
    const auto crossing = [&](std::size_t good, std::size_t bad) {
        const double g0 = pts[good].gammaMag;
        const double g1 = pts[bad].gammaMag;
        const double t = (g1 > g0) ? (gammaLimit - g0) / (g1 - g0) : 0.0;
        return pts[good].fHz + t * (pts[bad].fHz - pts[good].fHz);
    };

    // Vers le bas.
    std::size_t low = sweep.f0Index;
    while (low > 0 && inBand(low - 1)) {
        --low;
    }
    if (low == 0) {
        out.fLowHz = pts.front().fHz;
        out.lowOpen = true;
    } else {
        out.fLowHz = crossing(low, low - 1);
    }

    // Vers le haut.
    std::size_t high = sweep.f0Index;
    while (high + 1 < pts.size() && inBand(high + 1)) {
        ++high;
    }
    if (high + 1 == pts.size()) {
        out.fHighHz = pts.back().fHz;
        out.highOpen = true;
    } else {
        out.fHighHz = crossing(high, high + 1);
    }

    out.ok = true;
    out.widthHz = out.fHighHz - out.fLowHz;
    out.fractionalPercent = 100.0 * out.widthHz / pts[sweep.f0Index].fHz;
    return out;
}

std::string formatBandwidth(const BandwidthResult& bw) {
    if (!bw.ok) {
        return "none";
    }
    const bool open = bw.lowOpen || bw.highOpen;
    return std::string(open ? "> " : "") + formatEng(bw.widthHz, "Hz", 3) + " (" +
           formatSig(bw.fractionalPercent, 3) + " %)";
}

std::string sweepToCsv(const SweepResult& sweep) {
    std::ostringstream out;
    out.imbue(std::locale::classic());      // point décimal garanti
    out.precision(kCsvSignificantDigits);
    out << "f_hz,zin_re_ohm,zin_im_ohm,gamma_mag,gamma_deg,s11_db,vswr\n";
    for (const SweepPoint& p : sweep.points) {
        out << p.fHz << ',' << p.zIn.real() << ',' << p.zIn.imag() << ',' << p.gammaMag << ',' << p.gammaAngleDeg << ','
            << p.s11Db << ',';
        if (std::isinf(p.vswr)) {
            out << "inf";
        } else {
            out << p.vswr;
        }
        out << '\n';
    }
    return out.str();
}

} // namespace smith
