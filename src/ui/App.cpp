// =============================================================================
// App.cpp — Boucle principale et mise en page de l'interface
// =============================================================================
#include "ui/App.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "core/Types.hpp"

namespace smith::ui {

App::App(GLFWwindow* window, const Fonts& fonts, const std::string& /*initialFile*/)
    : window_(window), fonts_(fonts), project_(defaultProject()) {}

// -----------------------------------------------------------------------------
// run : à chaque tour de boucle (≈ 60 fois par seconde) on
//   1. récupère les événements clavier/souris,
//   2. reconstruit l'interface complète (principe « immediate mode » d'ImGui),
//   3. la dessine avec OpenGL et l'affiche.
// -----------------------------------------------------------------------------
void App::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        drawFrame();

        ImGui::Render();
        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        const ImVec4 bg = kColorWindowBg;
        glClearColor(bg.x, bg.y, bg.z, bg.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
}

// -----------------------------------------------------------------------------
// buildScene : transforme le projet en « scène » pour l'abaque.
// -----------------------------------------------------------------------------
ChartScene App::buildScene() const {
    ChartScene scene;
    scene.z0 = project_.z0;
    scene.zSource = project_.zSource;
    scene.zLoad = project_.load.zAtF0;
    scene.f0Hz = project_.f0Hz;
    scene.network = project_.network;
    return scene;
}

// -----------------------------------------------------------------------------
// drawFrame : une fenêtre ImGui qui occupe toute la fenêtre du système.
// -----------------------------------------------------------------------------
void App::drawFrame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    const ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::Begin("##host", nullptr, hostFlags);
    ImGui::PopStyleVar();
    drawMenuBar();
    chart_.draw(buildScene(), chartOptions_);
    ImGui::End();
}

void App::drawMenuBar() {
    if (!ImGui::BeginMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Quit")) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Admittance grid", nullptr, &chartOptions_.admittanceGrid);
        ImGui::MenuItem("VSWR circle", nullptr, &chartOptions_.vswrCircle);
        ImGui::MenuItem("Sweep locus on chart", nullptr, &chartOptions_.sweepLocus);
        ImGui::MenuItem("Learn mode", nullptr, &chartOptions_.learnMode);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

} // namespace smith::ui
