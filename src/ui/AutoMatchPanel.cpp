// =============================================================================
// AutoMatchPanel.cpp — Implémentation du panneau AUTO-MATCH
// =============================================================================
#include "ui/AutoMatchPanel.hpp"

#include <cmath>
#include <string>

#include "core/ESeries.hpp"
#include "core/RfMath.hpp"
#include "core/Solver.hpp"
#include "core/Sweep.hpp"
#include "core/Units.hpp"
#include "ui/LearnText.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

namespace {

constexpr const char* kArrow = "\xE2\x86\x92";   // →
constexpr double kQStep = 1.0;                   // « Q_min + 1 » du bouton d'aide

const ImVec4& qualityColor(double vswr) {
    if (vswr <= kVswrGoodLimit) {
        return kColorGood;
    }
    return vswr <= kVswrOkLimit ? kColorWarning : kColorBad;
}

std::string vswrText(double v) { return std::isinf(v) ? "\xE2\x88\x9E" : formatSig(v, 3); }

// Ligne d'éléments colorés : « Shunt C 922.8 fF → Series L 38.98 nH » (côté charge d'abord).
void drawElementLine(const ElementList& elements) {
    for (std::size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine(0.0f, 4.0f * uiScale());
            ImGui::TextColored(kColorTextDim, "%s", kArrow);
            ImGui::SameLine(0.0f, 4.0f * uiScale());
        }
        const std::string text = std::string(typeName(elements[i].type)) + " " +
                                 formatEng(elements[i].value, valueUnit(elements[i].type));
        ImGui::TextColored(colorForElementType(elements[i].type), "%s", text.c_str());
    }
}

} // namespace

bool AutoMatchPanel::draw(Project& p, bool learnMode) {
    bool changed = false;
    preview_.clear();
    panelHeader("AUTO-MATCH (solver)");

    // --- Choix de la topologie ------------------------------------------------------------
    ImGui::TextUnformatted("Topology");
    ImGui::SameLine();
    int topo = static_cast<int>(topology_);
    ImGui::RadioButton("L", &topo, static_cast<int>(Topology::L));
    ImGui::SameLine();
    ImGui::RadioButton("Pi", &topo, static_cast<int>(Topology::Pi));
    ImGui::SameLine();
    ImGui::RadioButton("T", &topo, static_cast<int>(Topology::T));
    topology_ = static_cast<Topology>(topo);
    if (learnMode) {
        helpTooltip("L: one series + one shunt element (up to 4 solutions).\n"
                    "Pi: shunt - series - shunt.  T: series - shunt - series.\n"
                    "Pi and T let you choose the loaded Q, i.e. the bandwidth.");
    }

    const Complex zLoad = p.load.zAtF0;
    const bool usesQ = topology_ != Topology::L;

    // --- Q nominal (Pi / T seulement) -------------------------------------------------------------
    double qMin = 0.0;
    if (usesQ) {
        qMin = (topology_ == Topology::Pi) ? piQMin(p.zSource, zLoad) : tQMin(p.zSource, zLoad);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Nominal Q");
        if (learnMode) {
            helpTooltip("Q is set by the virtual resistance Rv in the middle of the network.\n"
                        "A higher Q gives a narrower bandwidth but a lower sensitivity to the load.\n"
                        "It must be greater than Q_min.");
        }
        ImGui::SameLine();
        const Validator positive = [](double v) {
            return (std::isfinite(v) && v > 0.0) ? ValidationResult::success()
                                                 : ValidationResult::failure("Q must be greater than 0.");
        };
        numberInput("##q", q_, "", FieldFormat::Plain, qField_, 80.0f * uiScale(), positive);
        ImGui::SameLine();
        if (ImGui::Button("Q_min + 1")) {
            q_ = qMin + kQStep;
            qField_ = FieldState{};
        }
        helpTooltip("Set Q just above its minimum.");
        ImGui::TextColored(kColorTextDim, "Q_min = %s", formatSig(qMin, 3).c_str());
    }

    // --- Calcul (instantané : quelques formules fermées) --------------------------------------
    SolverResult result;
    switch (topology_) {
        case Topology::L: result = solveL(p.zSource, zLoad, p.f0Hz); break;
        case Topology::Pi: result = solvePi(p.zSource, zLoad, p.f0Hz, q_); break;
        case Topology::T: result = solveT(p.zSource, zLoad, p.f0Hz, q_); break;
    }

    ImGui::Separator();
    if (!result.ok) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorError, "%s", result.message.c_str());
        ImGui::PopTextWrapPos();
        return changed;
    }
    if (result.alreadyMatched) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorGood, "%s", result.message.c_str());
        ImGui::PopTextWrapPos();
        return changed;
    }
    if (result.solutions.empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorWarning, "%s", result.message.c_str());
        ImGui::PopTextWrapPos();
        return changed;
    }

    ImGui::TextColored(kColorTextDim, "%zu solution%s", result.solutions.size(),
                       result.solutions.size() > 1 ? "s" : "");
    ImGui::BeginChild("solutions", ImVec2(0.0f, 0.0f), 0);

    // --- Liste des solutions --------------------------------------------------------------------------
    int applyIndex = -1;
    for (std::size_t i = 0; i < result.solutions.size(); ++i) {
        const Solution& s = result.solutions[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();

        // Ligne 1 : numéro, type, topologie, bouton Apply à droite.
        const float rowStartX = ImGui::GetCursorPosX();
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        const float applyWidth = ImGui::CalcTextSize("Apply").x + 2.0f * ImGui::GetStyle().FramePadding.x;
        ImGui::AlignTextToFramePadding();
        ImGui::PushFont(currentFonts().semibold, kBaseFontSize);
        ImGui::Text("#%zu  %s", i + 1, solutionTypeName(s.type));
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextColored(kColorTextDim, "%s", solutionTopologyName(s.topology));
        ImGui::SameLine(rowStartX + rowWidth - applyWidth);
        if (ImGui::Button("Apply")) {
            applyIndex = static_cast<int>(i);
        }
        helpTooltip("Replace the current network by this solution.");

        // Ligne 2 : éléments (côté charge d'abord).
        drawElementLine(s.elements);

        // Ligne 3 : qualité à f0, valeurs idéales puis (si choisie) série E.
        const MatchMetrics ideal = evaluateMatch(projectInputImpedanceAt(p, s.elements, p.f0Hz), p.zSource);
        std::string line = "VSWR " + vswrText(ideal.vswr);
        if (p.eSeries != ESeriesKind::None) {
            const ElementList snapped = snapNetwork(s.elements, p.eSeries);
            const MatchMetrics rounded = evaluateMatch(projectInputImpedanceAt(p, snapped, p.f0Hz), p.zSource);
            line += std::string("   |   ") + eSeriesName(p.eSeries) + ": VSWR " + vswrText(rounded.vswr);
            ImGui::TextColored(qualityColor(rounded.vswr), "%s", line.c_str());
        } else {
            ImGui::TextColored(qualityColor(ideal.vswr), "%s", line.c_str());
        }
        // Bande passante (VSWR ≤ seuil du balayage) de la solution, idéale puis arrondie.
        {
            std::string bwLine = "Bandwidth: " + formatBandwidth(bandwidth(runSweep(p, s.elements), p.sweep.vswrThreshold));
            if (p.eSeries != ESeriesKind::None) {
                const ElementList snapped = snapNetwork(s.elements, p.eSeries);
                bwLine += std::string("   |   ") + eSeriesName(p.eSeries) + ": " +
                          formatBandwidth(bandwidth(runSweep(p, snapped), p.sweep.vswrThreshold));
            }
            ImGui::TextColored(kColorTextDim, "%s", bwLine.c_str());
        }
        if (usesQ) {
            ImGui::TextColored(kColorTextDim, "Nominal Q = %s", formatSig(s.nominalQ, 3).c_str());
        } else if (learnMode) {
            ImGui::TextColored(kColorTextDim, "Loaded Q of the L section = %s", formatSig(s.nominalQ, 3).c_str());
        }
        ImGui::EndGroup();
        if (ImGui::IsItemHovered()) {
            preview_ = s.elements;     // l'abaque dessinera ce réseau en pointillés
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::EndChild();

    // --- Application (avec confirmation si le réseau actuel n'est pas vide) ----------------------------
    if (applyIndex >= 0) {
        const ElementList& chosen = result.solutions[static_cast<std::size_t>(applyIndex)].elements;
        if (p.network.empty()) {
            p.network = chosen;
            changed = true;
        } else {
            pending_ = chosen;
            askConfirmation_ = true;
        }
    }
    if (askConfirmation_) {
        ImGui::OpenPopup("Replace network?");
        askConfirmation_ = false;
    }
    if (ImGui::BeginPopupModal("Replace network?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("The current network will be replaced by the selected solution.");
        ImGui::Spacing();
        if (ImGui::Button("Replace", ImVec2(110.0f * uiScale(), 0.0f))) {
            p.network = pending_;
            changed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(110.0f * uiScale(), 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return changed;
}

} // namespace smith::ui
