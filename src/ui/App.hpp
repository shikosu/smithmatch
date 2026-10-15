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
#include "ui/SchematicView.hpp"
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
    // Actions qui peuvent perdre des modifications (on demande confirmation si le projet est modifié).
    enum class PendingAction { None, NewProject, OpenFile, Quit };

    void drawFrame();       // construit toute l'interface d'une image
    void drawMenuBar();     // menus File / View / Help
    void drawStatusBar(float width);
    void drawDialogs();     // « Unsaved changes », Quick guide, About
    void handleShortcuts(); // Ctrl/Cmd + N, O, S, Maj+S

    // --- Fichiers ---------------------------------------------------------------------------
    void requestAction(PendingAction action);    // demande confirmation si nécessaire, puis exécute
    void performAction(PendingAction action);
    void loadFromPath(const std::string& path);
    bool saveProject();                          // false si annulé ou en erreur
    bool saveProjectAs();
    bool writeProjectTo(const std::string& path);
    void exportPng();
    void exportCsv();
    void capturePngIfRequested();                // appelé juste après le rendu, avant l'échange des tampons
    void setCurrentFile(const std::string& path);
    static void onWindowCloseRequested(GLFWwindow* window);
    ChartScene buildScene() const;   // prépare ce que l'abaque doit dessiner
    void setStatus(const std::string& text, bool isError);
    void updateWindowTitle();

    GLFWwindow* window_;
    Fonts fonts_;

    // --- Document -------------------------------------------------------------
    Project project_;             // l'état du document
    bool modified_ = false;       // modifications non enregistrées
    std::string fileName_;        // nom affiché ; vide = « Untitled »
    std::string filePath_;        // chemin complet (UTF-8) ; vide = jamais enregistré

    // --- Vues et panneaux ---------------------------------------------------------
    SmithChartView chart_;
    ChartOptions chartOptions_;
    SetupPanel setupPanel_;
    NetworkPanel networkPanel_;
    AutoMatchPanel autoMatchPanel_;
    SweepPanel sweepPanel_;
    SchematicView schematicView_;

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

    // --- Dialogues et exports en attente ------------------------------------------------------------
    PendingAction pendingAction_ = PendingAction::None;
    bool openUnsavedPopup_ = false;
    bool showQuickGuide_ = false;
    bool showAbout_ = false;
    std::string pngRequestPath_;     // chemin demandé pour l'export PNG (vide = aucun)
    bool pngCaptureNow_ = false;     // la capture se fait à la fin de CETTE image

    // --- Barre d'état ------------------------------------------------------------------------
    std::string statusText_;
    bool statusIsError_ = false;
    std::string lastTitle_;
};

} // namespace smith::ui
