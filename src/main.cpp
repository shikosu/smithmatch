// =============================================================================
// main.cpp — Point d'entrée de SmithMatch
// =============================================================================
// Rôle : préparer tout ce qui dépend de la plateforme AVANT de lancer l'appli :
//   1. GLFW  : crée la fenêtre et le contexte OpenGL 3.2 Core ;
//   2. ImGui : crée le contexte d'interface et branche les « backends »
//              (GLFW pour le clavier/souris, OpenGL3 pour le dessin) ;
//   3. lance ui::App, qui contient la boucle principale ;
//   4. libère tout proprement à la fermeture.
//
// Analogie : main() est l'alimentation et le câblage de la carte ; App est le
// microcontrôleur qui exécute le programme.
// =============================================================================
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <nfd.h>

#include <clocale>
#include <cstdio>
#include <string>

#include "ui/App.hpp"
#include "ui/Theme.hpp"

namespace {

// Taille de fenêtre par défaut et minimale, en points (avant mise à l'échelle HiDPI).
constexpr int kDefaultWindowWidth  = 1440;
constexpr int kDefaultWindowHeight = 900;
constexpr int kMinWindowWidth      = 1100;
constexpr int kMinWindowHeight     = 700;

// Version d'OpenGL demandée : 3.2 Core, la même sur Windows et macOS.
constexpr int kGlMajor = 3;
constexpr int kGlMinor = 2;
constexpr const char* kGlslVersion = "#version 150";   // GLSL qui correspond à OpenGL 3.2

// Appelée par GLFW en cas d'erreur (affichée dans la console de développement).
void glfwErrorCallback(int code, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

} // namespace

int main(int argc, char** argv) {
    // --- 1. GLFW : bibliothèque de fenêtrage ---------------------------------
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    // Contexte OpenGL 3.2 Core Profile + forward-compatible (obligatoire sur macOS).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGlMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGlMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifdef __APPLE__
    // Framebuffer à pleine résolution Retina (2 pixels par point).
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    // Facteur HiDPI de l'écran principal : 1.5 sous Windows à 150 %, 1.0 sur
    // macOS (où le système compte déjà en « points » et gère le Retina seul).
    const float mainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    const auto scaled = [mainScale](int points) {
        return static_cast<int>(static_cast<float>(points) * mainScale);
    };

    GLFWwindow* window = glfwCreateWindow(scaled(kDefaultWindowWidth), scaled(kDefaultWindowHeight),
                                          "SmithMatch", nullptr, nullptr);
    if (window == nullptr) {
        std::fprintf(stderr, "Failed to create a window with OpenGL %d.%d Core\n", kGlMajor, kGlMinor);
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, scaled(kMinWindowWidth), scaled(kMinWindowHeight),
                            GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // synchronisation verticale : pas plus de 60 images/s

    // Boîtes de dialogue natives (Ouvrir / Enregistrer). Sous Linux, GTK peut
    // changer la « locale » ; on remet le point décimal standard ensuite.
    NFD_Init();
    std::setlocale(LC_NUMERIC, "C");

    // --- 2. ImGui + ImPlot ----------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // pas de fichier imgui.ini écrit sur le disque
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    smith::ui::applyDarkTheme(mainScale);
    const smith::ui::Fonts fonts = smith::ui::loadFonts();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(kGlslVersion);

    // --- 3. Application -------------------------------------------------------
    // Un éventuel argument = fichier projet à ouvrir au démarrage.
    const std::string initialFile = (argc > 1) ? std::string(argv[1]) : std::string();
    {
        smith::ui::App app(window, fonts, initialFile);
        app.run();
    }

    // --- 4. Nettoyage (ordre inverse de la création) -------------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    NFD_Quit();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
