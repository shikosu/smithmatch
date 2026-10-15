// =============================================================================
// App.cpp — Boucle principale et mise en page de l'interface
// =============================================================================
// Mise en page (fenêtre unique, panneaux redimensionnables à la souris) :
//
//   ┌────────────┬───────────────────────┬────────────┐
//   │ SETUP      │                       │ NETWORK    │
//   │ RESULTS    │     SMITH CHART       │ AUTO-MATCH │
//   ├────────────┴──────────┬────────────┴────────────┤
//   │ SWEEP                 │ SCHEMATIC               │
//   ├───────────────────────┴─────────────────────────┤
//   │ barre d'état                                     │
//   └──────────────────────────────────────────────────┘
// =============================================================================
#include "ui/App.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>

#include "core/Types.hpp"
#include "ui/ResultsPanel.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

namespace {
// Dimensions minimales des zones (en points non mis à l'échelle).
constexpr float kMinLeftWidth = 280.0f;
constexpr float kMaxLeftWidth = 560.0f;
constexpr float kMinRightWidth = 300.0f;
constexpr float kMaxRightWidth = 600.0f;
constexpr float kMinCenterWidth = 280.0f;
constexpr float kMinBottomHeight = 140.0f;
constexpr float kMinTopHeight = 300.0f;
constexpr float kMinPanelHeight = 120.0f;
constexpr float kMinSchematicWidth = 280.0f;
constexpr float kMinSweepWidth = 320.0f;
constexpr const char* kUntitled = "Untitled";
} // namespace

App::App(GLFWwindow* window, const Fonts& fonts, const std::string& /*initialFile*/)
    : window_(window), fonts_(fonts), project_(defaultProject()) {
    updateWindowTitle();
}

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
        updateWindowTitle();

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

void App::setStatus(const std::string& text, bool isError) {
    statusText_ = text;
    statusIsError_ = isError;
}

// Titre de la fenêtre : « SmithMatch — fichier.smith.json * » (étoile si modifié).
void App::updateWindowTitle() {
    const std::string title = std::string(kAppName) + " \xE2\x80\x94 " + (fileName_.empty() ? kUntitled : fileName_) +
                              (modified_ ? " *" : "");
    if (title != lastTitle_) {
        glfwSetWindowTitle(window_, title.c_str());
        lastTitle_ = title;
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
    scene.highlightedElement = networkPanel_.hoveredElement();
    return scene;
}

// -----------------------------------------------------------------------------
// drawFrame : une fenêtre ImGui qui occupe toute la fenêtre du système, divisée
// à la main en zones (positions calculées, pas de tableau) pour des séparateurs précis.
// -----------------------------------------------------------------------------
void App::drawFrame() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    const ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##host", nullptr, hostFlags);
    ImGui::PopStyleVar();
    drawMenuBar();

    const float s = uiScale();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float split = 6.0f * s;
    const float statusHeight = ImGui::GetTextLineHeight() + 10.0f * s;
    const float bodyHeight = avail.y - statusHeight;

    // --- Dimensions effectives (bornées pour que tout reste utilisable) ------------
    const float maxBottom = std::max(kMinBottomHeight * s, bodyHeight - kMinTopHeight * s - split);
    const float bottomH = std::clamp(bottomHeight_ * s, kMinBottomHeight * s, maxBottom);
    const float topH = bodyHeight - bottomH - split;

    float leftW = leftWidth_ * s;
    float rightW = rightWidth_ * s;
    const float maxSides = avail.x - kMinCenterWidth * s - 2.0f * split;
    if (leftW + rightW > maxSides) {       // fenêtre étroite : on réduit les côtés proportionnellement
        const float k = maxSides / (leftW + rightW);
        leftW *= k;
        rightW *= k;
    }
    const float centerW = avail.x - leftW - rightW - 2.0f * split;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * s, 8.0f * s));

    // --- Colonne de gauche : SETUP + RESULTS ---------------------------------------------
    ImGui::SetCursorScreenPos(origin);
    ImGui::BeginChild("left", ImVec2(leftW, topH), ImGuiChildFlags_Borders);
    if (setupPanel_.draw(project_, chartOptions_.learnMode)) {
        modified_ = true;
    }
    ImGui::Spacing();
    drawResultsPanel(project_, chartOptions_.learnMode);
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(origin.x + leftW, origin.y));
    splitter("##split_left", true, topH, leftWidth_, kMinLeftWidth, kMaxLeftWidth, false);

    // --- Centre : abaque de Smith ------------------------------------------------------------
    ImGui::PopStyleVar();
    ImGui::SetCursorScreenPos(ImVec2(origin.x + leftW + split, origin.y));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kColorPanelBg);
    ImGui::BeginChild("chart", ImVec2(centerW, topH), 0,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    chart_.draw(buildScene(), chartOptions_);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * s, 8.0f * s));

    ImGui::SetCursorScreenPos(ImVec2(origin.x + leftW + split + centerW, origin.y));
    splitter("##split_right", true, topH, rightWidth_, kMinRightWidth, kMaxRightWidth, true);

    // --- Colonne de droite : NETWORK + AUTO-MATCH ----------------------------------------------
    const float rightX = origin.x + leftW + split + centerW + split;
    const float networkH = std::clamp(networkHeight_ * s, kMinPanelHeight * s, topH - kMinPanelHeight * s - split);
    ImGui::SetCursorScreenPos(ImVec2(rightX, origin.y));
    ImGui::BeginChild("network", ImVec2(rightW, networkH), ImGuiChildFlags_Borders);
    if (networkPanel_.draw(project_, chartOptions_.learnMode)) {
        modified_ = true;
    }
    ImGui::EndChild();
    ImGui::SetCursorScreenPos(ImVec2(rightX, origin.y + networkH));
    splitter("##split_net", false, rightW, networkHeight_, kMinPanelHeight, 900.0f, false);
    ImGui::SetCursorScreenPos(ImVec2(rightX, origin.y + networkH + split));
    ImGui::BeginChild("automatch", ImVec2(rightW, topH - networkH - split), ImGuiChildFlags_Borders);
    panelHeader("AUTO-MATCH (solver)");
    ImGui::TextColored(kColorTextDim, "Coming in the next step.");
    ImGui::EndChild();

    // --- Séparateur horizontal puis rangée du bas : SWEEP + SCHEMATIC -------------------------------
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + topH));
    splitter("##split_bottom", false, avail.x, bottomHeight_, kMinBottomHeight, 600.0f, true);

    const float bottomY = origin.y + topH + split;
    const float maxSweep = std::max(kMinSweepWidth * s, avail.x - kMinSchematicWidth * s - split);
    const float sweepW = std::clamp(sweepWidth_ * s, kMinSweepWidth * s, maxSweep);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, bottomY));
    ImGui::BeginChild("sweep", ImVec2(sweepW, bottomH), ImGuiChildFlags_Borders);
    panelHeader("SWEEP");
    ImGui::TextColored(kColorTextDim, "Coming in a later step.");
    ImGui::EndChild();
    ImGui::SetCursorScreenPos(ImVec2(origin.x + sweepW, bottomY));
    splitter("##split_sweep", true, bottomH, sweepWidth_, kMinSweepWidth, 1600.0f, false);
    ImGui::SetCursorScreenPos(ImVec2(origin.x + sweepW + split, bottomY));
    ImGui::BeginChild("schematic", ImVec2(avail.x - sweepW - split, bottomH), ImGuiChildFlags_Borders);
    panelHeader("SCHEMATIC");
    ImGui::TextColored(kColorTextDim, "Coming in a later step.");
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- Barre d'état ----------------------------------------------------------------------------------
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + bodyHeight));
    drawStatusBar(avail.x);

    ImGui::End();
}

// Barre d'état : message à gauche (rouge si erreur), nom du fichier à droite, « * » si modifié.
void App::drawStatusBar(float width) {
    const float s = uiScale();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, kColorPanelBg);
    ImGui::BeginChild("status", ImVec2(width, ImGui::GetTextLineHeight() + 10.0f * s), 0,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetCursorPos(ImVec2(10.0f * s, 5.0f * s));
    if (statusText_.empty()) {
        ImGui::TextColored(kColorTextDim, "Ready.");
    } else {
        ImGui::TextColored(statusIsError_ ? kColorError : kColorText, "%s", statusText_.c_str());
    }
    const std::string right = (fileName_.empty() ? std::string(kUntitled) : fileName_) + (modified_ ? " *" : "");
    const float textWidth = ImGui::CalcTextSize(right.c_str()).x;
    ImGui::SameLine(width - textWidth - 10.0f * s);
    ImGui::TextColored(kColorTextDim, "%s", right.c_str());
    ImGui::EndChild();
    ImGui::PopStyleColor();
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
