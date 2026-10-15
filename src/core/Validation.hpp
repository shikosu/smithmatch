// =============================================================================
// Validation.hpp — Contrôle des entrées (réutilisé par l'interface)
// =============================================================================
// Rôle : vérifier qu'une valeur saisie a un sens physique AVANT de calculer.
// Chaque fonction renvoie « ok » ou un message d'erreur clair (en anglais,
// car c'est ce que l'utilisateur voit dans l'interface).
// =============================================================================
#pragma once

#include <string>
#include <utility>

#include "core/Types.hpp"

namespace smith {

struct ValidationResult {
    bool ok = true;
    std::string message;   // vide si ok

    static ValidationResult success() { return {}; }
    static ValidationResult failure(std::string text) { return {false, std::move(text)}; }
};

// Z₀ : réelle, finie, > 0.
ValidationResult validateZ0(double z0);

// f₀ (et toute fréquence de balayage) : finie, entre 1 kHz et 1 THz.
ValidationResult validateFrequency(double fHz);

// Impédance de source : parties finies, Re(Z_s) > 0.
ValidationResult validateSourceImpedance(Complex zSource);

// Impédance de charge : parties finies, Re(Z_L) > 0 (charge passive, pas purement réactive).
ValidationResult validateLoadImpedance(Complex zLoad);

// Valeur de composant (H ou F) : finie et strictement positive.
ValidationResult validateComponentValue(double value);

} // namespace smith
