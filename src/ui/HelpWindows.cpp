// =============================================================================
// HelpWindows.cpp — Contenu des fenêtres d'aide (en anglais, comme l'interface)
// =============================================================================
#include "ui/HelpWindows.hpp"

#include <imgui.h>

#include "ui/Theme.hpp"
#include "ui/Widgets.hpp"

#ifndef SMITH_VERSION
#define SMITH_VERSION "dev"
#endif

namespace smith::ui {

namespace {

constexpr float kHelpWindowWidth = 520.0f;
constexpr float kHelpWindowHeight = 460.0f;

void heading(const char* text) {
    ImGui::Spacing();
    ImGui::PushFont(currentFonts().semibold, kBaseFontSize);
    ImGui::TextColored(kColorHeader, "%s", text);
    ImGui::PopFont();
}

void paragraph(const char* text) {
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
}

} // namespace

void drawQuickGuide(bool& open) {
    const float s = uiScale();
    ImGui::SetNextWindowSize(ImVec2(kHelpWindowWidth * s, kHelpWindowHeight * s), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Quick guide", &open)) {
        ImGui::End();
        return;
    }
    heading("1. Describe your problem (SETUP)");
    paragraph("Enter the reference impedance Z0, the centre frequency f0, the source impedance Zs and the load "
              "impedance ZL measured at f0. Values accept engineering notation: 2.4G, 4.7n, 100p, 2,2u.");
    heading("2. Read the Smith chart");
    paragraph("The target (white cross) is the conjugate of the source impedance. Top half = inductive, bottom half = "
              "capacitive. Hover the chart to read the impedance and admittance under the mouse.");
    heading("3. Build a network by hand (NETWORK)");
    paragraph("Add series or shunt inductors and capacitors. The first element is the closest to the load. Each "
              "element draws an arc on the chart: series elements move along constant-resistance circles, shunt "
              "elements along constant-conductance circles.");
    heading("4. Or let the solver work (AUTO-MATCH)");
    paragraph("Choose L, Pi or T. Hover a solution to preview it as a dashed path on the chart, then press Apply. "
              "For Pi and T, the nominal Q sets the bandwidth: a higher Q is narrower.");
    heading("5. Check the bandwidth (SWEEP)");
    paragraph("The sweep shows |S11| and VSWR versus frequency. The green area is the bandwidth where VSWR stays "
              "below the limit. Use View > Sweep locus on chart to see Zin(f) on the Smith chart.");
    heading("6. Real components (E-series)");
    paragraph("Pick E12 or E24 in SETUP: values are rounded to the nearest standard value and the results show the "
              "effect of that rounding.");
    heading("Files");
    paragraph("Projects are saved as .smith.json. File > Export lets you save the chart as PNG and the sweep as CSV.");
    ImGui::End();
}

void drawAbout(bool& open) {
    const float s = uiScale();
    ImGui::SetNextWindowSize(ImVec2(440.0f * s, 0.0f), ImGuiCond_Always);
    if (!ImGui::Begin("About SmithMatch", &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    ImGui::PushFont(currentFonts().semibold, kTitleFontSize);
    ImGui::TextColored(kColorHeader, "SmithMatch %s", SMITH_VERSION);
    ImGui::PopFont();
    paragraph("Impedance matching with an interactive Smith chart: L, Pi and T networks, sweep and E-series.");
    ImGui::Spacing();
    ImGui::TextUnformatted("Copyright (c) 2026 T\xC3\xA9o Vidal");
    ImGui::TextUnformatted("MIT License");
    heading("Built with");
    paragraph("Dear ImGui, ImPlot, GLFW, nlohmann/json, nativefiledialog-extended, stb_image_write, doctest "
              "(tests) and the Inter font (SIL Open Font License). Full licences: THIRD_PARTY_LICENSES.md.");
    ImGui::End();
}

} // namespace smith::ui
