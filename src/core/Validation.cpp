// =============================================================================
// Validation.cpp — Implémentation des contrôles d'entrée
// =============================================================================
#include "core/Validation.hpp"

#include <cmath>

#include "core/Units.hpp"

namespace smith {

namespace {
bool isFinite(Complex z) { return std::isfinite(z.real()) && std::isfinite(z.imag()); }
}

ValidationResult validateZ0(double z0) {
    if (!std::isfinite(z0)) {
        return ValidationResult::failure("Z0 must be a finite number.");
    }
    if (z0 <= 0.0) {
        return ValidationResult::failure("Z0 must be greater than 0 \xCE\xA9.");
    }
    return ValidationResult::success();
}

ValidationResult validateFrequency(double fHz) {
    if (!std::isfinite(fHz)) {
        return ValidationResult::failure("Frequency must be a finite number.");
    }
    if (fHz < kMinFrequencyHz || fHz > kMaxFrequencyHz) {
        return ValidationResult::failure("Frequency must be between " +
                                         formatEng(kMinFrequencyHz, "Hz") + " and " +
                                         formatEng(kMaxFrequencyHz, "Hz") + ".");
    }
    return ValidationResult::success();
}

ValidationResult validateSourceImpedance(Complex zSource) {
    if (!isFinite(zSource)) {
        return ValidationResult::failure("Source impedance must be finite.");
    }
    if (zSource.real() <= 0.0) {
        return ValidationResult::failure("Source resistance Re(Zs) must be greater than 0 \xCE\xA9.");
    }
    return ValidationResult::success();
}

ValidationResult validateLoadImpedance(Complex zLoad) {
    if (!isFinite(zLoad)) {
        return ValidationResult::failure("Load impedance must be finite.");
    }
    if (zLoad.real() <= 0.0) {
        return ValidationResult::failure(
            "Load resistance Re(ZL) must be greater than 0 \xCE\xA9 "
            "(a passive load cannot be purely reactive).");
    }
    return ValidationResult::success();
}

ValidationResult validateComponentValue(double value) {
    if (!std::isfinite(value)) {
        return ValidationResult::failure("Component value must be a finite number.");
    }
    if (value <= 0.0) {
        return ValidationResult::failure("Component value must be greater than 0.");
    }
    return ValidationResult::success();
}

} // namespace smith
