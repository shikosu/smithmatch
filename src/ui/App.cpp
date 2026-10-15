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
#include <cmath>
#include <filesystem>
#include <optional>

#include "core/ESeries.hpp"
#include "core/FileIo.hpp"
#include "core/RfMath.hpp"
#include "core/Types.hpp"
#include "ui/HelpWindows.hpp"
#include "ui/ResultsPanel.hpp"
#include "ui/platform/FileDialogs.hpp"
#include "ui/platform/ScreenCapture.hpp"
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
constexpr const char* kProjectFilterName = "SmithMatch project";
constexpr const char* kProjectExtensions = "json";
constexpr const char* kProjectSuffix = ".smith.json";
constexpr const char* kUnsavedPopupName = "Unsaved changes";
} // namespace

App::App(GLFWwindow* window, const Fonts& fonts, const std::string& initialFile)
    : window_(window), fonts_(fonts), project_(defaultProject()) {
    // Le rappel de fermeture retrouve l'App grâce au « user pointer » de la fenêtre.
    glfwSetWindowUserPointer(window_, this);
    glfwSetWindowCloseCallback(window_, &App::onWindowCloseRequested);
    if (!initialFile.empty()) {
        loadFromPath(initialFile);
    }
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

        // Une capture PNG demandée pendant l'image précédente se fait pendant cette image-ci
        // (le menu et la boîte de dialogue ont alors disparu de l'écran).
        pngCaptureNow_ = !pngRequestPath_.empty();

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
        capturePngIfRequested();       // doit se faire AVANT l'échange des tampons
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
    scene.previewNetwork = autoMatchPanel_.previewNetwork();
    if (chartOptions_.sweepLocus && sweepIdeal_.ok) {
        // Lieu de Z_in(f) : chaque point est Γ = (Z − Z0)/(Z + Z0), comme tout point de l'abaque.
        scene.sweepLocus.reserve(sweepIdeal_.points.size());
        for (const SweepPoint& point : sweepIdeal_.points) {
            scene.sweepLocus.push_back(gammaFromZ(point.zIn, project_.z0));
        }
        scene.sweepF0Index = sweepIdeal_.f0Index;
    }
    scene.highlightedElement =
        networkPanel_.hoveredElement() >= 0 ? networkPanel_.hoveredElement() : schematicView_.hoveredElement();
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
    handleShortcuts();

    // Balayages du réseau courant (idéal, et arrondi à la série E si choisie) : utilisés par
    // l'abaque (lieu), le panneau SWEEP et la bande passante.
    sweepIdeal_ = runSweep(project_, project_.network);
    hasRounded_ = project_.eSeries != ESeriesKind::None;
    if (hasRounded_) {
        sweepRounded_ = runSweep(project_, snapNetwork(project_.network, project_.eSeries));
    }

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
    ChartOptions options = chartOptions_;
    options.suppressHover = pngCaptureNow_;      // pas de croix de survol dans l'export
    chart_.draw(buildScene(), options);
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
    if (autoMatchPanel_.draw(project_, chartOptions_.learnMode)) {
        modified_ = true;
    }
    ImGui::EndChild();

    // --- Séparateur horizontal puis rangée du bas : SWEEP + SCHEMATIC -------------------------------
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + topH));
    splitter("##split_bottom", false, avail.x, bottomHeight_, kMinBottomHeight, 600.0f, true);

    const float bottomY = origin.y + topH + split;
    const float maxSweep = std::max(kMinSweepWidth * s, avail.x - kMinSchematicWidth * s - split);
    const float sweepW = std::clamp(sweepWidth_ * s, kMinSweepWidth * s, maxSweep);
    ImGui::SetCursorScreenPos(ImVec2(origin.x, bottomY));
    ImGui::BeginChild("sweep", ImVec2(sweepW, bottomH), ImGuiChildFlags_Borders);
    if (sweepPanel_.draw(project_, sweepIdeal_, hasRounded_ ? &sweepRounded_ : nullptr, chartOptions_.learnMode)) {
        modified_ = true;
    }
    ImGui::EndChild();
    ImGui::SetCursorScreenPos(ImVec2(origin.x + sweepW, bottomY));
    splitter("##split_sweep", true, bottomH, sweepWidth_, kMinSweepWidth, 1600.0f, false);
    ImGui::SetCursorScreenPos(ImVec2(origin.x + sweepW + split, bottomY));
    ImGui::BeginChild("schematic", ImVec2(avail.x - sweepW - split, bottomH), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_HorizontalScrollbar);
    schematicView_.draw(project_);
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // --- Barre d'état ----------------------------------------------------------------------------------
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + bodyHeight));
    drawStatusBar(avail.x);

    ImGui::End();
    drawDialogs();
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
    const std::string mod = platform::shortcutModifierName();
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New", (mod + "N").c_str())) {
            requestAction(PendingAction::NewProject);
        }
        if (ImGui::MenuItem("Open...", (mod + "O").c_str())) {
            requestAction(PendingAction::OpenFile);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", (mod + "S").c_str())) {
            saveProject();
        }
        if (ImGui::MenuItem("Save As...", (mod + "Shift+S").c_str())) {
            saveProjectAs();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Export Smith chart as PNG...")) {
            exportPng();
        }
        if (ImGui::MenuItem("Export sweep as CSV...")) {
            exportCsv();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit")) {
            requestAction(PendingAction::Quit);
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
        if (ImGui::MenuItem("Quick guide")) {
            showQuickGuide_ = true;
        }
        if (ImGui::MenuItem("About SmithMatch")) {
            showAbout_ = true;
        }
        ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
}

// -----------------------------------------------------------------------------
// Raccourcis clavier : Ctrl (Cmd sur Mac) + N, O, S, Maj+S.
// -----------------------------------------------------------------------------
void App::handleShortcuts() {
    if (ImGui::IsPopupOpen(kUnsavedPopupName)) {
        return;     // pendant la confirmation, on ne lance pas d'autre action
    }
    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N)) {
        requestAction(PendingAction::NewProject);
    } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O)) {
        requestAction(PendingAction::OpenFile);
    } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S)) {
        saveProjectAs();
    } else if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
        saveProject();
    }
}

// -----------------------------------------------------------------------------
// Fermeture de la fenêtre (croix, Alt+F4, Cmd+Q) : si le projet est modifié, on annule
// la fermeture et on demande confirmation.
// -----------------------------------------------------------------------------
void App::onWindowCloseRequested(GLFWwindow* window) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app != nullptr && app->modified_) {
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        app->requestAction(PendingAction::Quit);
    }
}

// Si le projet est modifié, ouvre « Unsaved changes » ; sinon exécute directement l'action.
void App::requestAction(PendingAction action) {
    if (modified_) {
        pendingAction_ = action;
        openUnsavedPopup_ = true;
    } else {
        performAction(action);
    }
}

void App::performAction(PendingAction action) {
    switch (action) {
        case PendingAction::None:
            break;
        case PendingAction::NewProject:
            project_ = defaultProject();
            modified_ = false;
            fileName_.clear();
            filePath_.clear();
            setStatus("New project.", false);
            break;
        case PendingAction::OpenFile: {
            const std::optional<std::string> path = platform::openFileDialog(kProjectFilterName, kProjectExtensions);
            if (path.has_value()) {
                loadFromPath(*path);
            }
            break;
        }
        case PendingAction::Quit:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;
    }
}

void App::setCurrentFile(const std::string& path) {
    filePath_ = path;
    fileName_ = std::filesystem::u8path(path).filename().u8string();
}

// Charge un projet. En cas d'erreur, le projet courant n'est PAS touché.
void App::loadFromPath(const std::string& path) {
    const TextFileResult file = readTextFile(path);
    if (!file.ok) {
        setStatus(file.message, true);
        return;
    }
    const ProjectLoadResult loaded = fromJson(file.text);
    if (!loaded.ok) {
        setStatus(loaded.message, true);
        return;
    }
    project_ = loaded.project;
    modified_ = false;
    setCurrentFile(path);
    setStatus("Opened " + fileName_ + ".", false);
}

bool App::writeProjectTo(const std::string& path) {
    const TextFileResult written = writeTextFile(path, toJson(project_));
    if (!written.ok) {
        setStatus(written.message, true);
        return false;
    }
    setCurrentFile(path);
    modified_ = false;
    setStatus("Saved " + fileName_ + ".", false);
    return true;
}

bool App::saveProject() {
    if (filePath_.empty()) {
        return saveProjectAs();
    }
    return writeProjectTo(filePath_);
}

bool App::saveProjectAs() {
    const std::string suggestion = fileName_.empty() ? std::string("project") + kProjectSuffix : fileName_;
    const std::optional<std::string> path =
        platform::saveFileDialog(kProjectFilterName, kProjectExtensions, suggestion, kProjectSuffix);
    if (!path.has_value()) {
        return false;
    }
    return writeProjectTo(*path);
}

// Export CSV du balayage du réseau courant (valeurs idéales).
void App::exportCsv() {
    if (!sweepIdeal_.ok) {
        setStatus("Cannot export the sweep: " + sweepIdeal_.message, true);
        return;
    }
    const std::optional<std::string> path = platform::saveFileDialog("CSV file", "csv", "sweep.csv", ".csv");
    if (!path.has_value()) {
        return;
    }
    const TextFileResult written = writeTextFile(*path, sweepToCsv(sweepIdeal_));
    if (!written.ok) {
        setStatus(written.message, true);
        return;
    }
    setStatus("Exported " + std::filesystem::u8path(*path).filename().u8string() + ".", false);
}

// Export PNG : on demande seulement le chemin ici ; la capture a lieu à l'image suivante.
void App::exportPng() {
    const std::optional<std::string> path = platform::saveFileDialog("PNG image", "png", "smith-chart.png", ".png");
    if (path.has_value()) {
        pngRequestPath_ = *path;
    }
}

void App::capturePngIfRequested() {
    if (!pngCaptureNow_) {
        return;
    }
    const std::string path = pngRequestPath_;
    pngRequestPath_.clear();
    pngCaptureNow_ = false;

    // Rectangle de l'abaque : points ImGui → pixels du framebuffer (×2 sur Retina).
    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const int x0 = std::clamp(static_cast<int>(std::lround(chart_.rectMin().x * scale.x)), 0, fbWidth);
    const int y0 = std::clamp(static_cast<int>(std::lround(chart_.rectMin().y * scale.y)), 0, fbHeight);
    const int x1 = std::clamp(static_cast<int>(std::lround(chart_.rectMax().x * scale.x)), 0, fbWidth);
    const int y1 = std::clamp(static_cast<int>(std::lround(chart_.rectMax().y * scale.y)), 0, fbHeight);

    const platform::CaptureResult result = platform::saveFramebufferRegionPng(path, x0, y0, x1 - x0, y1 - y0, fbHeight);
    if (result.ok) {
        setStatus("Exported " + std::filesystem::u8path(path).filename().u8string() + ".", false);
    } else {
        setStatus("PNG export failed: " + result.message, true);
    }
}

// -----------------------------------------------------------------------------
// Dialogues : confirmation des modifications non enregistrées, aide, à propos.
// -----------------------------------------------------------------------------
void App::drawDialogs() {
    if (openUnsavedPopup_) {
        ImGui::OpenPopup(kUnsavedPopupName);
        openUnsavedPopup_ = false;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(kUnsavedPopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const std::string name = fileName_.empty() ? std::string(kUntitled) : fileName_;
        ImGui::Text("Save changes to \"%s\" before continuing?", name.c_str());
        ImGui::Spacing();
        const float w = 110.0f * uiScale();
        if (ImGui::Button("Save", ImVec2(w, 0.0f))) {
            ImGui::CloseCurrentPopup();
            const PendingAction action = pendingAction_;
            pendingAction_ = PendingAction::None;
            if (saveProject()) {            // si l'enregistrement est annulé ou échoue, on ne fait rien
                performAction(action);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(w, 0.0f))) {
            ImGui::CloseCurrentPopup();
            const PendingAction action = pendingAction_;
            pendingAction_ = PendingAction::None;
            modified_ = false;
            performAction(action);
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(w, 0.0f))) {
            ImGui::CloseCurrentPopup();
            pendingAction_ = PendingAction::None;
        }
        ImGui::EndPopup();
    }
    if (showQuickGuide_) {
        drawQuickGuide(showQuickGuide_);
    }
    if (showAbout_) {
        drawAbout(showAbout_);
    }
}

} // namespace smith::ui
