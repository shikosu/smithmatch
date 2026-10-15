// =============================================================================
// Element.hpp — Un composant du réseau d'adaptation (bobine ou condensateur)
// =============================================================================
// Rôle : décrire un élément L/C série ou shunt, calculer sa réactance /
// susceptance à une fréquence, l'appliquer à une impédance, et convertir une
// réactance en valeur de composant. Conventions : CLAUDE.md §5.1, §5.2, §5.5.
//
// Rappel des signes :
//   L série  : X = +ωL            C série  : X = −1/(ωC)
//   C shunt  : B = +ωC            L shunt  : B = −1/(ωL)
// =============================================================================
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/Types.hpp"

namespace smith {

enum class ElementType { SeriesInductor, SeriesCapacitor, ShuntInductor, ShuntCapacitor };
// v0.3 : SeriesResistor, ShuntResistor, TransmissionLine, OpenStub, ShortStub

struct Element {
    ElementType type = ElementType::SeriesInductor;
    double value = 0.0;                    // H pour une bobine, F pour un condensateur (toujours > 0)
    std::map<std::string, double> params;  // réservé v0.3 (longueur, Z0 de ligne, Q…) ; vide en v0.2
};

// Le réseau est une liste ordonnée ; l'indice 0 est l'élément le plus proche de la charge.
using ElementList = std::vector<Element>;

// --- Propriétés d'un type ------------------------------------------------------
bool isSeries(ElementType type);     // vrai pour série, faux pour shunt
bool isInductor(ElementType type);   // vrai pour bobine, faux pour condensateur
const char* typeName(ElementType type);       // "Series L", "Series C", "Shunt L", "Shunt C"
const char* typeJsonName(ElementType type);   // "series_inductor", … (format de fichier)
std::optional<ElementType> typeFromJsonName(const std::string& name);
const char* valueUnit(ElementType type);      // "H" ou "F"

// Construit un élément (valeur en SI).
Element makeElement(ElementType type, double value);

// --- Réactance / susceptance ---------------------------------------------------
// Réactance X (Ω) d'un élément SÉRIE à la fréquence f. Renvoie 0 pour un shunt.
double reactanceAt(const Element& element, double fHz);

// Susceptance B (S) d'un élément SHUNT à la fréquence f. Renvoie 0 pour un série.
double susceptanceAt(const Element& element, double fHz);

// --- Application à une impédance -------------------------------------------------
// Série  : Z_out = Z_in + jX.
// Shunt  : Y_out = Y_in + jB (avec Y = 1/Z) puis Z_out = 1/Y_out.
//   Garde-fous : Z_in ≈ 0 (court-circuit) → un shunt ne change rien ;
//                Y_out ≈ 0 (circuit ouvert) → renvoie kOpenCircuitOhm.
Complex apply(const Element& element, Complex zPrev, double fHz);

// Variantes « bas niveau » utilisées aussi pour tracer les arcs :
Complex addSeriesReactance(Complex zPrev, double reactance);       // Z + jX
Complex addShuntSusceptance(Complex zPrev, double susceptance);    // 1/(1/Z + jB)

// --- Conversion réactance → composant (à f₀) -------------------------------------
// Série : « xOrB » est une réactance X (Ω) ; shunt : une susceptance B (S).
//   X > 0 → L = X/ω       X < 0 → C = −1/(ωX)
//   B > 0 → C = B/ω       B < 0 → L = −1/(ωB)
// « scale » est l'ordre de grandeur du problème (Ω pour un série, S pour un shunt) :
// si |xOrB| ≤ 1e-9·scale, l'élément est supprimé (fil ou circuit ouvert) → std::nullopt.
std::optional<Element> elementFromReactance(double xOrB, double f0Hz, bool series, double scale);

// --- Arc tracé sur l'abaque ------------------------------------------------------
// Points Γ (par rapport à z0) obtenus en faisant varier la réactance (ou la
// susceptance) de l'élément de 0 jusqu'à sa valeur finale à la fréquence f,
// à partir de l'impédance zPrev vue avant l'élément. « nSamples » points (≥ 2),
// le premier est Γ(zPrev) et le dernier Γ(apply(element, zPrev, f)).
// Fonction de core, paramétrée par la valeur du composant : réutilisable pour le
// glisser-déposer de la v0.3.
std::vector<Complex> sampledArc(const Element& element, Complex zPrev, double fHz, double z0,
                                int nSamples = 64);

} // namespace smith
