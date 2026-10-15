// =============================================================================
// Project.hpp — L'état complet d'un « document » SmithMatch
// =============================================================================
// Rôle : regrouper tout ce qui est sauvegardé dans un fichier projet : Z₀, f₀,
// source, charge, réseau, réglages du balayage, série E.
//
// C'est un TYPE VALEUR copiable : copier un Project = prendre un instantané
// complet (c'est ce qui rendra « annuler / rétablir » trivial en v0.3).
// Unités SI partout. Élément d'indice 0 = côté charge.
// =============================================================================
#pragma once

#include <string>

#include "core/ESeries.hpp"
#include "core/Element.hpp"
#include "core/Load.hpp"
#include "core/Types.hpp"

namespace smith {

struct SweepSettings {
    double fMinHz = 0.0;                         // fixé par defaultProject() à f₀/2
    double fMaxHz = 0.0;                         // fixé par defaultProject() à 2·f₀
    int points = kDefaultSweepPoints;
    bool logScale = false;
    double vswrThreshold = kDefaultVswrThreshold;   // seuil pour la bande passante
};

struct Project {
    double z0 = kDefaultZ0Ohm;                   // impédance de référence de l'abaque (Ω)
    double f0Hz = kDefaultF0Hz;                  // fréquence centrale (Hz)
    Complex zSource {kDefaultSourceR, kDefaultSourceX};   // impédance de source Z_s (Ω)
    LoadSpec load;                               // charge Z_L à f₀ + modèle
    ElementList network;                         // indice 0 = côté charge
    SweepSettings sweep;
    ESeriesKind eSeries = ESeriesKind::E12;
};

// --- Fichier projet (.smith.json) ------------------------------------------------
// Format : voir docs/ARCHITECTURE.md. Règles :
//  - « format » doit valoir kProjectFormatName ;
//  - « version » plus grande que kProjectFileVersion → refus avec un message clair ;
//  - les champs inconnus sont ignorés (compatibilité avec des versions futures) ;
//  - un champ connu absent prend sa valeur par défaut ; un champ présent mais invalide est refusé.
// Quand le schéma change : incrémenter kProjectFileVersion ET garder la lecture des anciennes versions.
inline constexpr const char* kProjectFormatName = "smithmatch-project";
inline constexpr int kProjectFileVersion = 1;

struct ProjectLoadResult {
    bool ok = false;
    std::string message;     // raison du refus si ok = false
    Project project;         // valide seulement si ok = true
};

// Sérialise le projet en JSON lisible (indentation de 2 espaces).
std::string toJson(const Project& project);

// Lit un projet depuis du texte JSON. Ne lève jamais d'exception : toute erreur
// (JSON mal formé, version trop récente, valeur invalide…) est renvoyée dans le résultat.
ProjectLoadResult fromJson(const std::string& text);

// Projet de démarrage : antenne 25 − j15 Ω, 50 Ω, 2.4 GHz, réseau vide,
// balayage de f₀/2 à 2·f₀, série E12.
Project defaultProject();

// Impédance de charge à la fréquence f, selon le modèle de charge du projet.
Complex projectLoadAt(const Project& project, double fHz);

// Impédance vue par la source à la fréquence f (charge + tout le réseau).
Complex projectInputImpedanceAt(const Project& project, double fHz);

// Même chose avec une autre liste d'éléments (ex. valeurs arrondies E-series,
// ou solution du solveur en aperçu).
Complex projectInputImpedanceAt(const Project& project, const ElementList& network, double fHz);

} // namespace smith
