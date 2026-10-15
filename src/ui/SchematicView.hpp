// =============================================================================
// SchematicView.hpp — Schéma électrique du réseau d'adaptation
// =============================================================================
// Rôle : dessiner le circuit : la source à GAUCHE, la charge à DROITE, et les éléments
// entre les deux. Comme le réseau est stocké « côté charge d'abord » (indice 0 = près de
// la charge), on parcourt la liste À L'ENVERS pour dessiner de gauche à droite.
// Série = composant sur le fil principal ; shunt = composant entre le fil et la masse.
// =============================================================================
#pragma once

#include "core/Project.hpp"

namespace smith::ui {

class SchematicView {
public:
    // Dessine le schéma dans la fenêtre ImGui courante (qui doit permettre le défilement
    // horizontal : le schéma s'étire si le réseau est long).
    void draw(const Project& project);

    // Indice de l'élément survolé à l'image précédente (-1 = aucun) : l'abaque le met en valeur.
    int hoveredElement() const { return hovered_; }

private:
    int hovered_ = -1;
};

} // namespace smith::ui
