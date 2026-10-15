// =============================================================================
// App.cpp — Boucle principale et squelette de l'interface (étape 1)
// =============================================================================
#include "ui/App.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "core/Types.hpp"

namespace smith::ui {

App::App(GLFWwindow* window, const Fonts& fonts, const std::string& /*initialFile*/)
    : window_(window), fonts_(fonts) {}

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
// drawFrame : une fenêtre ImGui qui occupe toute la fenêtre du système.
// -----------------------------------------------------------------------------
void App::drawFrame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    const ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("##host", nullptr, hostFlags);
    drawMenuBar();

    ImGui::PushFont(fonts_.semibold, kTitleFontSize);
    ImGui::TextColored(kColorHeader, "%s v%s", kAppName, kAppVersion);
    ImGui::PopFont();
    ImGui::TextUnformatted("Glyph check: \xCE\xA9 \xC2\xB5 \xCE\x93 \xC2\xB1 \xC2\xB0 "
                           "\xE2\x89\xA4 \xE2\x89\xA5 \xE2\x88\x9E \xCF\x80");
    ImGui::TextUnformatted(u8"Same text, UTF-8 literal: Ω µ Γ ± ° ≤ ≥ ∞ π");
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
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

} // namespace smith::ui
