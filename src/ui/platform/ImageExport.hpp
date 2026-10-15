// =============================================================================
// ImageExport.hpp — Capture d'une zone de la fenêtre OpenGL en fichier PNG
// =============================================================================
// Rôle : exporter l'abaque tel qu'il est affiché. Appeler JUSTE APRÈS le rendu de l'image
// et AVANT glfwSwapBuffers (le tampon arrière contient alors l'image complète).
// =============================================================================
#pragma once

#include <string>

namespace smith::ui::platform {

struct CaptureResult {
    bool ok = false;
    std::string message;    // raison de l'échec
};

// Capture le rectangle (x, y, largeur, hauteur) exprimé en PIXELS du framebuffer, origine en
// HAUT à gauche (comme ImGui, une fois multiplié par l'échelle Retina). Écrit un PNG.
CaptureResult saveFramebufferRegionPng(const std::string& pathUtf8, int x, int y, int width, int height,
                                       int framebufferHeight);

} // namespace smith::ui::platform
