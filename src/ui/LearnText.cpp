// =============================================================================
// LearnText.cpp — Textes pédagogiques (voir LearnText.hpp)
// =============================================================================
#include "ui/LearnText.hpp"

#include <cmath>

#include "core/Units.hpp"

namespace smith::ui::learn {

const char* chartOrientationHint() {
    return "Top half = inductive, bottom half = capacitive  |  left = short circuit, right = open circuit";
}

const char* quantityTip(Quantity quantity) {
    switch (quantity) {
        case Quantity::Z0:
            return "Reference impedance of the chart (usually 50 \xCE\xA9). The centre of the chart is the "
                   "point where Z = Z0.\n\nNormalised impedance: z = Z / Z0";
        case Quantity::Frequency:
            return "Centre frequency f0 at which the network is designed. All reactances are computed "
                   "at this frequency:\n\nX(L) = 2\xCF\x80 f L      X(C) = -1 / (2\xCF\x80 f C)";
        case Quantity::SourceImpedance:
            return "Impedance of the generator (or of the stage that drives the network). "
                   "The match is perfect when the impedance seen by the source equals its "
                   "complex conjugate Zs*.\n\nIf Zs is real (e.g. 50 \xCE\xA9) the target is the centre of the chart.";
        case Quantity::LoadImpedance:
            return "Impedance of the load (antenna, amplifier input...) at f0, as Z = R + jX. "
                   "X > 0 means inductive, X < 0 capacitive.";
        case Quantity::ReflectionInput:
            return "The same load, entered as a reflection coefficient with respect to Z0.\n\n"
                   "\xCE\x93 = (Z - Z0) / (Z + Z0)\nmust satisfy |\xCE\x93| < 1 for a passive load.";
        case Quantity::ZIn:
            return "Impedance seen by the source when it looks into the network, with the load connected at the end. "
                   "Goal: Zin = Zs* (the point marked 'target' on the chart).";
        case Quantity::GammaM:
            return "Reflection coefficient seen by the source: the part of the incoming wave "
                   "that bounces back. 0 = nothing is reflected (perfect match), 1 = everything is.\n\n"
                   "\xCE\x93m = (Zin - Zs*) / (Zin + Zs)";
        case Quantity::Vswr:
            return "Voltage Standing Wave Ratio. 1 = perfect match; the larger it is, the more power "
                   "is reflected. 2 means about 11 % of the power is reflected.\n\n"
                   "VSWR = (1 + |\xCE\x93|) / (1 - |\xCE\x93|)";
        case Quantity::ReturnLoss:
            return "How many dB the reflected wave is below the incident wave. Higher is better: "
                   "20 dB means only 1 % of the power is reflected.\n\n"
                   "RL = -20 \xC2\xB7 log10 |\xCE\x93|";
        case Quantity::S11:
            return "Same information as the return loss, with the opposite sign (always negative for a "
                   "passive circuit). -20 dB is a good match.\n\n"
                   "|S11| = 20 \xC2\xB7 log10 |\xCE\x93|";
        case Quantity::MismatchLoss:
            return "Power lost because of the reflection: the share of the available power that is not "
                   "delivered to the load.\n\n"
                   "ML = -10 \xC2\xB7 log10 (1 - |\xCE\x93|\xC2\xB2)";
        case Quantity::ESeries:
            return "Standard component values sold in shops. E12 has 12 values per decade "
                   "(10 % spacing), E24 has 24 (5 %). The nearest value is chosen on a logarithmic scale, "
                   "because the relative error is what matters.";
        case Quantity::NominalQ:
            return "Nominal Q of the Pi / T network, defined by the virtual resistance Rv in the middle "
                   "of the design (Pi: Rv = Rhigh / (1 + Q\xC2\xB2); T: Rv = Rlow \xC2\xB7 (1 + Q\xC2\xB2)). "
                   "A higher Q gives a narrower bandwidth. "
                   "Q must be greater than Qmin = sqrt(Rmax / Rmin - 1).";
        case Quantity::Quality:
            return "Green: VSWR \xE2\x89\xA4 1.5.  Orange: VSWR \xE2\x89\xA4 2.  Red: VSWR > 2.";
    }
    return "";
}

const char* loadModelTip(LoadModel model) {
    switch (model) {
        case LoadModel::Constant:
            return "The load keeps the same impedance at every frequency (ideal, simplest). "
                   "The frequency sweep then only shows the effect of the matching network itself.";
        case LoadModel::SeriesEquivalent:
            return "The load behaves like a resistor R in series with an inductor (if X > 0) or a capacitor "
                   "(if X < 0), whose value is computed at f0. Typical of a small antenna or a bond wire.";
        case LoadModel::ParallelEquivalent:
            return "The load behaves like a conductance G in parallel with a capacitor (if B > 0) or an "
                   "inductor (if B < 0), computed at f0. Typical of a transistor input.";
    }
    return "";
}

std::string explainElement(const Element& element, double f0Hz, double z0, Complex zBefore) {
    const bool series = isSeries(element.type);
    const std::string head = std::string(typeName(element.type)) + ", " +
                             formatEng(element.value, valueUnit(element.type)) + ": ";
    const char* direction = nullptr;
    switch (element.type) {
        case ElementType::SeriesInductor:
        case ElementType::ShuntCapacitor: direction = "clockwise"; break;
        case ElementType::SeriesCapacitor:
        case ElementType::ShuntInductor: direction = "counter-clockwise"; break;
    }

    const std::string freq = formatEng(f0Hz, "Hz");
    if (series) {
        const double x = reactanceAt(element, f0Hz);
        const double r = zBefore.real() / z0;
        return head + "adds " + std::string(x >= 0.0 ? "+" : "\xE2\x88\x92") + "j" + formatSig(std::fabs(x), 3) +
               " \xCE\xA9 at " + freq + " \xE2\x86\x92 moves " + direction + " along the r = " + formatSig(r, 3) +
               " circle.";
    }
    const double b = susceptanceAt(element, f0Hz);
    if (std::abs(zBefore) < kTinyImpedance) {
        return head + "placed across a short circuit: no effect.";
    }
    const Complex yBefore = 1.0 / zBefore;
    const double g = yBefore.real() * z0;
    return head + "adds " + std::string(b >= 0.0 ? "+" : "\xE2\x88\x92") + "j" + formatSig(std::fabs(b) * 1000.0, 3) +
           " mS at " + freq + " \xE2\x86\x92 moves " + direction + " along the g = " + formatSig(g, 3) + " circle.";
}

const char* qualityLabel(double vswr) {
    if (vswr <= kVswrGoodLimit) {
        return "Good";
    }
    if (vswr <= kVswrOkLimit) {
        return "Acceptable";
    }
    return "Poor";
}

} // namespace smith::ui::learn
