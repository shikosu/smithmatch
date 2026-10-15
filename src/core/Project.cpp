// =============================================================================
// Project.cpp — Projet par défaut et petits calculs sur le projet
// (la sauvegarde / lecture JSON sera ajoutée à l'étape « Fichiers »)
// =============================================================================
#include "core/Project.hpp"

#include "core/Network.hpp"

namespace smith {

Project defaultProject() {
    Project project;
    project.sweep.fMinHz = project.f0Hz / 2.0;
    project.sweep.fMaxHz = project.f0Hz * 2.0;
    return project;
}

Complex projectLoadAt(const Project& project, double fHz) {
    return loadImpedanceAt(project.load, project.f0Hz, fHz);
}

Complex projectInputImpedanceAt(const Project& project, double fHz) {
    return inputImpedance(project.network, projectLoadAt(project, fHz), fHz);
}

Complex projectInputImpedanceAt(const Project& project, const ElementList& network, double fHz) {
    return inputImpedance(network, projectLoadAt(project, fHz), fHz);
}

} // namespace smith
