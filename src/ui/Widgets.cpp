// =============================================================================
// Widgets.cpp — Implémentation des briques d'interface
// =============================================================================
#include "ui/Widgets.hpp"

#include <algorithm>
#include <cstring>

#include "core/Units.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

float uiScale() { return ImGui::GetFontSize() / kBaseFontSize; }

namespace {

// Texte affiché quand le champ n'est pas en cours d'édition.
std::string formatForDisplay(double value, const std::string& unit, FieldFormat format) {
    if (format == FieldFormat::Engineering) {
        return formatEng(value, unit);
    }
    const std::string number = formatSig(value, 6);
    return unit.empty() ? number : number + " " + unit;
}

} // namespace

bool numberInput(const char* id, double& value, const std::string& unit, FieldFormat format,
                 FieldState& state, float width, const Validator& validate) {
    bool changed = false;

    // Hors édition, le champ montre toujours la valeur courante (elle peut avoir
    // changé ailleurs : solveur, ouverture de fichier, saisie en Γ…).
    if (!state.focused && !state.invalid) {
        state.text = formatForDisplay(value, unit, format);
    }

    char buffer[96];
    std::strncpy(buffer, state.text.c_str(), sizeof buffer - 1);
    buffer[sizeof buffer - 1] = '\0';

    if (width > 0.0f) {
        ImGui::SetNextItemWidth(width);
    }
    const bool wasInvalid = state.invalid;
    if (wasInvalid) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, withAlpha(kColorError, 0.18f));
    }
    const bool edited = ImGui::InputText(id, buffer, sizeof buffer, ImGuiInputTextFlags_AutoSelectAll);
    const bool active = ImGui::IsItemActive();
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    if (wasInvalid) {
        ImGui::PopStyleColor();
        ImGui::GetWindowDrawList()->AddRect(itemMin, itemMax, toU32(kColorError), 4.0f, 1.5f * uiScale(), 0);
    }

    if (edited) {
        state.text = buffer;
        const std::optional<double> parsed = parseEng(state.text);
        if (!parsed.has_value()) {
            state.invalid = true;
            state.error = "Not a valid number (examples: 4.7n, 2.4G, 50, 1e-9).";
        } else {
            const ValidationResult check = validate ? validate(*parsed) : ValidationResult::success();
            if (!check.ok) {
                state.invalid = true;
                state.error = check.message;
            } else {
                value = *parsed;           // dernière valeur valide
                state.invalid = false;
                state.error.clear();
                changed = true;
            }
        }
    }

    // Quand on quitte le champ, une saisie invalide est abandonnée (retour à la valeur valide).
    if (ImGui::IsItemDeactivated()) {
        state.invalid = false;
        state.error.clear();
    }
    state.focused = active;

    if (state.invalid) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorError, "%s", state.error.c_str());
        ImGui::PopTextWrapPos();
    }
    return changed;
}

void panelHeader(const char* title) {
    ImGui::PushFont(nullptr, kTitleFontSize);
    ImGui::TextColored(kColorHeader, "%s", title);
    ImGui::PopFont();
    ImGui::Separator();
}

void helpTooltip(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool splitter(const char* id, bool vertical, float length, float& sizeUnscaled, float minUnscaled,
              float maxUnscaled, bool invertDelta) {
    const float scale = uiScale();
    const float thickness = 6.0f * scale;
    const ImVec2 size = vertical ? ImVec2(thickness, length) : ImVec2(length, thickness);
    ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (hovered || active) {
        ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }

    // Trait fin au milieu de la poignée, coloré au survol.
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    const ImVec4 color = active ? kColorAccentHover : (hovered ? kColorAccent : kColorBorder);
    if (vertical) {
        const float x = (a.x + b.x) * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(x, a.y + 2.0f), ImVec2(x, b.y - 2.0f), toU32(color), 2.0f);
    } else {
        const float y = (a.y + b.y) * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x + 2.0f, y), ImVec2(b.x - 2.0f, y), toU32(color), 2.0f);
    }

    bool changed = false;
    // On ignore l'image où le bouton vient d'être pressé : la souris peut y avoir « sauté »
    // jusqu'à la poignée, et ce déplacement ne doit pas compter comme un glissement.
    if (active && !ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
        float delta = vertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y;
        delta = (invertDelta ? -delta : delta) / scale;
        const float next = std::clamp(sizeUnscaled + delta, minUnscaled, maxUnscaled);
        changed = next != sizeUnscaled;
        sizeUnscaled = next;
    }
    return changed;
}

void colorDot(const ImVec4& color) {
    const float radius = 5.0f * uiScale();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float height = ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(pos.x + radius, pos.y + height * 0.5f), radius, toU32(color));
    ImGui::Dummy(ImVec2(radius * 2.0f + 4.0f * uiScale(), height));
}

} // namespace smith::ui
