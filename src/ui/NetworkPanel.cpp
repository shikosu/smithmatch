// =============================================================================
// NetworkPanel.cpp — Implémentation du panneau NETWORK
// =============================================================================
#include "ui/NetworkPanel.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/Network.hpp"
#include "core/Units.hpp"
#include "ui/LearnText.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

namespace {

// Bornes des curseurs logarithmiques (SI) : de 1 pH à 1 mH, de 1 fF à 1 µF.
constexpr double kSliderMinInductance = 1.0e-12;
constexpr double kSliderMaxInductance = 1.0e-3;
constexpr double kSliderMinCapacitance = 1.0e-15;
constexpr double kSliderMaxCapacitance = 1.0e-6;

// Valeur initiale « sensée » d'un nouvel élément : sa réactance (ou |1/susceptance|)
// vaut Z0 à f0. L = Z0/ω ; C = 1/(ω·Z0).
Element newElement(ElementType type, double z0, double f0Hz) {
    const double omega = angularFrequency(f0Hz);
    const double value = isInductor(type) ? z0 / omega : 1.0 / (omega * z0);
    return makeElement(type, value);
}

} // namespace

bool NetworkPanel::draw(Project& p, bool learnMode) {
    bool changed = false;
    panelHeader("NETWORK (manual)");

    // --- Boutons d'ajout (côté source = fin de liste) --------------------------------
    struct AddButton { const char* label; ElementType type; const char* tip; };
    const AddButton buttons[] = {
        {"+ Series L", ElementType::SeriesInductor, "Add a series inductor (in line with the signal)."},
        {"+ Series C", ElementType::SeriesCapacitor, "Add a series capacitor (in line with the signal)."},
        {"+ Shunt L", ElementType::ShuntInductor, "Add an inductor from the line to ground."},
        {"+ Shunt C", ElementType::ShuntCapacitor, "Add a capacitor from the line to ground."}};
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;
    for (int i = 0; i < 4; ++i) {
        ImGui::PushStyleColor(ImGuiCol_Text, colorForElementType(buttons[i].type));
        if (ImGui::Button(buttons[i].label, ImVec2(buttonWidth, 0.0f))) {
            p.network.push_back(newElement(buttons[i].type, p.z0, p.f0Hz));
            changed = true;
        }
        ImGui::PopStyleColor();
        if (learnMode) {
            helpTooltip(buttons[i].tip);
        }
        if (i % 2 == 0) {
            ImGui::SameLine();
        }
    }

    ImGui::BeginDisabled(p.eSeries == ESeriesKind::None || p.network.empty());
    if (ImGui::Button("Snap all to E-series", ImVec2(buttonWidth, 0.0f))) {
        p.network = snapNetwork(p.network, p.eSeries);
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(p.network.empty());
    if (ImGui::Button("Clear network", ImVec2(buttonWidth, 0.0f))) {
        p.network.clear();
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::Separator();

    if (fields_.size() != p.network.size()) {
        fields_.assign(p.network.size(), FieldState{});
    }
    hovered_ = -1;

    if (p.network.empty()) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorTextDim,
                           "No element yet. Add one with the buttons above, or let AUTO-MATCH propose a network.");
        ImGui::PopTextWrapPos();
        return changed;
    }

    // --- Liste des éléments -----------------------------------------------------------------
    const Complex zLoadF0 = p.load.zAtF0;
    const std::vector<Complex> nodes = nodeImpedances(p.network, zLoadF0, p.f0Hz);

    int moveUp = -1;
    int moveDown = -1;
    int remove = -1;
    for (std::size_t i = 0; i < p.network.size(); ++i) {
        Element& element = p.network[i];
        const int index = static_cast<int>(i);
        ImGui::PushID(index);
        ImGui::BeginGroup();

        // Ligne 1 : numéro, type (coloré), boutons alignés à droite.
        const float rowStartX = ImGui::GetCursorPosX();
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        const float buttonsWidth = 3.0f * ImGui::GetFrameHeight() + 2.0f * spacing;
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kColorTextDim, "%zu", i + 1);
        ImGui::SameLine();
        ImGui::PushFont(currentFonts().semibold, kBaseFontSize);
        ImGui::TextColored(colorForElementType(element.type), "%s", typeName(element.type));
        ImGui::PopFont();
        ImGui::SameLine(rowStartX + rowWidth - buttonsWidth);
        ImGui::BeginDisabled(i == 0);
        if (ImGui::Button("\xE2\x86\x91", ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
            moveUp = index;
        }
        ImGui::EndDisabled();
        helpTooltip("Move towards the load");
        ImGui::SameLine();
        ImGui::BeginDisabled(i + 1 == p.network.size());
        if (ImGui::Button("\xE2\x86\x93", ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
            moveDown = index;
        }
        ImGui::EndDisabled();
        helpTooltip("Move towards the source");
        ImGui::SameLine();
        if (ImGui::Button("\xC3\x97", ImVec2(ImGui::GetFrameHeight(), 0.0f))) {
            remove = index;
        }
        helpTooltip("Delete this element");

        // Ligne 2 : valeur (texte en notation ingénieur) + curseur logarithmique.
        const bool inductor = isInductor(element.type);
        const char* unit = valueUnit(element.type);
        if (numberInput("##value", element.value, unit, FieldFormat::Engineering, fields_[i],
                        90.0f * uiScale(), validateComponentValue)) {
            changed = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f);
        // Curseur logarithmique « maison » : le curseur d'ImGui perd de la précision pour les
        // très petites valeurs (H, F), donc on déplace un curseur LINÉAIRE t ∈ [0, 1] et on
        // en déduit la valeur : v = lo · (hi/lo)^t  (échelle logarithmique).
        const double lo = inductor ? kSliderMinInductance : kSliderMinCapacitance;
        const double hi = inductor ? kSliderMaxInductance : kSliderMaxCapacitance;
        const double clamped = std::clamp(element.value, lo, hi);
        float t = static_cast<float>(std::log(clamped / lo) / std::log(hi / lo));
        if (ImGui::SliderFloat("##slider", &t, 0.0f, 1.0f, "")) {
            element.value = lo * std::pow(hi / lo, static_cast<double>(t));
            changed = true;
        }
        helpTooltip("Drag to change the value on a logarithmic scale.");

        // Ligne 3 : valeur normalisée la plus proche et impédance du nœud après l'élément.
        std::string detail;
        if (p.eSeries != ESeriesKind::None) {
            detail += std::string(eSeriesName(p.eSeries)) + ": " +
                      formatEng(nearestESeries(element.value, p.eSeries), unit) + "   ";
        }
        detail += "after: " + formatComplex(nodes[i + 1], 4) + " \xCE\xA9";
        ImGui::TextColored(kColorTextDim, "%s", detail.c_str());

        // Ligne 4 (mode Learn) : ce que fait l'élément sur l'abaque.
        if (learnMode) {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextColored(withAlpha(colorForElementType(element.type), 0.9f), "%s",
                               learn::explainElement(element, p.f0Hz, p.z0, nodes[i]).c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::EndGroup();
        if (ImGui::IsItemHovered()) {
            hovered_ = index;
        }
        ImGui::Separator();
        ImGui::PopID();
    }

    // Modifications structurelles appliquées APRÈS la boucle (on ne modifie pas une liste qu'on parcourt).
    if (moveUp > 0) {
        std::swap(p.network[static_cast<std::size_t>(moveUp)], p.network[static_cast<std::size_t>(moveUp) - 1]);
        fields_.assign(p.network.size(), FieldState{});
        changed = true;
    } else if (moveDown >= 0) {
        std::swap(p.network[static_cast<std::size_t>(moveDown)], p.network[static_cast<std::size_t>(moveDown) + 1]);
        fields_.assign(p.network.size(), FieldState{});
        changed = true;
    } else if (remove >= 0) {
        p.network.erase(p.network.begin() + remove);
        fields_.assign(p.network.size(), FieldState{});
        changed = true;
    }
    return changed;
}

} // namespace smith::ui
