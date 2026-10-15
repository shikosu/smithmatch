// =============================================================================
// App.hpp — L'application : boucle principale, menu, mise en page
// =============================================================================
#pragma once

#include <string>

#include "core/Project.hpp"
#include "ui/AutoMatchPanel.hpp"
#include "ui/NetworkPanel.hpp"
#include "ui/SetupPanel.hpp"
#include "core/Sweep.hpp"
#include "ui/SmithChartView.hpp"
#include "ui/SweepPanel.hpp"
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
    void drawFrame();       // construit toute l'interface d'une image
    void drawMenuBar();     // menus File / View / Help
    void drawStatusBar(float width);
    ChartScene buildScene() const;   // prépare ce que l'abaque doit dessiner
    void setStatus(const std::string& text, bool isError);
    void updateWindowTitle();

    GLFWwindow* window_;
    Fonts fonts_;

    // --- Document -------------------------------------------------------------
    Project project_;             // l'état du document
    bool modified_ = false;       // modifications non enregistrées
    std::string fileName_;        // vide = « Untitled »

    // --- Vues et panneaux ---------------------------------------------------------
    SmithChartView chart_;
    ChartOptions chartOptions_;
    SetupPanel setupPanel_;
    NetworkPanel networkPanel_;
    AutoMatchPanel autoMatchPanel_;
    SweepPanel sweepPanel_;

    // Balayages recalculés à chaque image (quelques centaines de points : instantané).
    SweepResult sweepIdeal_;       // réseau courant, valeurs idéales
    SweepResult sweepRounded_;     // réseau arrondi à la série E
    bool hasRounded_ = false;      // une série E est choisie

    // --- Mise en page (en points, avant mise à l'échelle HiDPI) ------------------------
    float leftWidth_ = 350.0f;
    float rightWidth_ = 380.0f;
    float bottomHeight_ = 240.0f;
    float networkHeight_ = 330.0f;    // hauteur du panneau NETWORK dans la colonne de droite
    float sweepWidth_ = 700.0f;       // largeur du panneau SWEEP dans la rangée du bas

    // --- Barre d'état ------------------------------------------------------------------------
    std::string statusText_;
    bool statusIsError_ = false;
    std::string lastTitle_;
};

} // namespace smith::ui
