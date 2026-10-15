// =============================================================================
// App.hpp — L'application : boucle principale, menu, mise en page
// =============================================================================
#pragma once

#include <string>

#include "core/Project.hpp"
#include "ui/SmithChartView.hpp"
#include "ui/Theme.hpp"

struct GLFWwindow;

namespace smith::ui {

class App {
public:
    // window      : fenêtre GLFW déjà créée (avec son contexte OpenGL)
    // fonts       : polices chargées par loadFonts()
    // initialFile : projet à ouvrir au démarrage (chaîne vide = projet par défaut)
    App(GLFWwindow* window, const Fonts& fonts, const std::string& initialFile);

    // Boucle principale : tourne jusqu'à la fermeture de la fenêtre.
    void run();

private:
    void drawFrame();     // construit toute l'interface d'une image
    void drawMenuBar();   // menus File / View / Help
    ChartScene buildScene() const;   // prépare ce que l'abaque doit dessiner

    GLFWwindow* window_;
    Fonts fonts_;

    Project project_;             // l'état du document
    SmithChartView chart_;
    ChartOptions chartOptions_;
};

} // namespace smith::ui
