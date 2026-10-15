// =============================================================================
// ResultsPanel.cpp — Implémentation du panneau RESULTS
// =============================================================================
#include "ui/ResultsPanel.hpp"

#include <cmath>
#include <string>

#include "core/Network.hpp"
#include "core/RfMath.hpp"
#include "core/Units.hpp"
#include "ui/LearnText.hpp"
#include "ui/Theme.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

namespace {

constexpr const char* kOhm = "\xCE\xA9";

std::string vswrText(double v) { return std::isinf(v) ? "\xE2\x88\x9E" : formatSig(v, 4); }
std::string dbText(double v) {
    if (std::isinf(v)) {
        return std::string(v < 0.0 ? "-" : "") + "\xE2\x88\x9E dB";
    }
    return formatSig(v, 4) + " dB";
}

const ImVec4& qualityColor(double vswr) {
    if (vswr <= kVswrGoodLimit) {
        return kColorGood;
    }
    return vswr <= kVswrOkLimit ? kColorWarning : kColorBad;
}

// Cellule de texte (valeur) dans la colonne courante.
void cell(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }

} // namespace

void drawResultsPanel(const Project& p, bool learnMode) {
    panelHeader("RESULTS");

    const bool withESeries = p.eSeries != ESeriesKind::None;
    const ElementList snapped = snapNetwork(p.network, p.eSeries);
    const MatchMetrics ideal = evaluateMatch(projectInputImpedanceAt(p, p.f0Hz), p.zSource);
    const MatchMetrics rounded = evaluateMatch(projectInputImpedanceAt(p, snapped, p.f0Hz), p.zSource);

    const int columns = withESeries ? 3 : 2;
    if (!ImGui::BeginTable("results", columns, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
        return;
    }
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.8f);
    ImGui::TableSetupColumn("Ideal values", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    if (withESeries) {
        const std::string title = std::string("With ") + eSeriesName(p.eSeries);
        ImGui::TableSetupColumn(title.c_str(), ImGuiTableColumnFlags_WidthStretch, 1.0f);
    }
    ImGui::TableHeadersRow();

    // Une ligne : étiquette (+ infobulle) puis la valeur calculée pour chaque colonne.
    const auto row = [&](const char* label, learn::Quantity quantity, bool hasTip, const std::string& a,
                         const std::string& b) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        if (learnMode && hasTip) {
            helpTooltip(learn::quantityTip(quantity));
        }
        ImGui::TableSetColumnIndex(1);
        cell(a);
        if (withESeries) {
            ImGui::TableSetColumnIndex(2);
            cell(b);
        }
    };

    const auto zText = [&](const MatchMetrics& m) { return formatComplex(m.zIn, 4) + " " + kOhm; };
    const auto znText = [&](const MatchMetrics& m) { return formatComplex(m.zIn / p.z0, 3); };

    row("Zin", learn::Quantity::ZIn, true, zText(ideal), zText(rounded));
    row("zin = Zin/Z0", learn::Quantity::ZIn, true, znText(ideal), znText(rounded));
    row("|\xCE\x93m|", learn::Quantity::GammaM, true, formatSig(ideal.gammaMag, 4), formatSig(rounded.gammaMag, 4));
    // L'angle n'a pas de sens quand |\xCE\x93m| n'est que du bruit d'arrondi : on affiche un tiret.
    const auto angleText = [](const MatchMetrics& m) {
        return m.gammaMag < kZeroRelTolerance ? std::string("\xE2\x80\x94")
                                              : formatSig(m.gammaAngleDeg, 4) + "\xC2\xB0";
    };
    row("arg \xCE\x93m", learn::Quantity::GammaM, true, angleText(ideal), angleText(rounded));
    row("VSWR", learn::Quantity::Vswr, true, vswrText(ideal.vswr), vswrText(rounded.vswr));
    row("Return loss", learn::Quantity::ReturnLoss, true, dbText(ideal.returnLossDb), dbText(rounded.returnLossDb));
    row("|S11|", learn::Quantity::S11, true, dbText(ideal.s11Db), dbText(rounded.s11Db));
    row("Mismatch loss", learn::Quantity::MismatchLoss, true, dbText(ideal.mismatchLossDb),
        dbText(rounded.mismatchLossDb));

    // Indicateur de qualité : pastille verte / orange / rouge + mot.
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted("Match quality");
    if (learnMode) {
        helpTooltip(learn::quantityTip(learn::Quantity::Quality));
    }
    ImGui::TableSetColumnIndex(1);
    colorDot(qualityColor(ideal.vswr));
    ImGui::SameLine();
    ImGui::TextColored(qualityColor(ideal.vswr), "%s", learn::qualityLabel(ideal.vswr));
    if (withESeries) {
        ImGui::TableSetColumnIndex(2);
        colorDot(qualityColor(rounded.vswr));
        ImGui::SameLine();
        ImGui::TextColored(qualityColor(rounded.vswr), "%s", learn::qualityLabel(rounded.vswr));
    }
    ImGui::EndTable();
}

} // namespace smith::ui
