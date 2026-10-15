// =============================================================================
// Widgets.hpp — Petites briques d'interface réutilisables
// =============================================================================
// Rôle : champs de saisie en notation ingénieur avec contrôle de validité,
// en-têtes de panneaux, séparateurs redimensionnables, pastille de qualité.
// =============================================================================
#pragma once

#include <imgui.h>

#include <functional>
#include <string>

#include "core/Validation.hpp"

namespace smith::ui {

// Facteur d'échelle de l'interface (1.0 = 15 px de police ; plus grand en HiDPI).
float uiScale();

// État mémorisé d'un champ de saisie (une instance par champ).
struct FieldState {
    std::string text;        // texte en cours d'édition
    bool focused = false;    // le champ était actif à l'image précédente
    bool invalid = false;    // la saisie en cours est invalide
    std::string error;       // message d'erreur à afficher
};

// Comment afficher la valeur quand le champ n'est pas en cours d'édition.
enum class FieldFormat {
    Engineering,   // « 2.4 GHz », « 4.7 nH » (préfixes SI)
    Plain          // « 25 Ω », « 0.333 » (nombre tel quel + unité)
};

using Validator = std::function<ValidationResult(double)>;

// Champ de saisie d'un nombre.
//  - id       : identifiant ImGui unique (ex. "##z0")
//  - value    : la valeur SI, modifiée SEULEMENT si la saisie est valide
//  - unit     : unité affichée (« Hz », « Ω », « H »… ou "" )
//  - width    : largeur en pixels (0 = largeur disponible)
// Une saisie invalide entoure le champ de rouge, affiche le message dessous, et
// conserve la dernière valeur valide. Renvoie true quand « value » a changé.
bool numberInput(const char* id, double& value, const std::string& unit, FieldFormat format,
                 FieldState& state, float width, const Validator& validate);

// Titre de panneau (police semi-grasse, couleur d'accent) suivi d'un filet.
void panelHeader(const char* title);

// Infobulle de texte (affichée si l'élément précédent est survolé et « text » non vide).
void helpTooltip(const char* text);

// Séparateur qu'on tire à la souris. « vertical » = poignée verticale (change une largeur).
// « sizeUnscaled » est exprimé en points non mis à l'échelle ; renvoie true si modifié.
bool splitter(const char* id, bool vertical, float length, float& sizeUnscaled, float minUnscaled,
              float maxUnscaled, bool invertDelta);

// Petit disque coloré (indicateur de qualité) aligné sur la ligne de texte courante.
void colorDot(const ImVec4& color);

} // namespace smith::ui
