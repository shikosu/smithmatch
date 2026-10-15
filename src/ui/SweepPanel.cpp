// =============================================================================
// SweepPanel.cpp — Implémentation du panneau SWEEP
// =============================================================================
#include "ui/SweepPanel.hpp"

#include <implot.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "core/Units.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

namespace {

constexpr float kSettingsWidth = 235.0f;          // largeur de la colonne de réglages (non mise à l'échelle)
constexpr double kPlotS11MinDb = -40.0;           // bas de l'axe |S11| (les valeurs plus basses sont écrêtées)
constexpr double kPlotS11MaxDb = 0.0;
constexpr double kPlotVswrMin = 1.0;
constexpr double kPlotVswrMax = 10.0;             // haut de l'axe VSWR (valeurs plus hautes écrêtées)
constexpr float kLineWeight = 1.8f;
constexpr float kThresholdLineWeight = 1.2f;

struct FrequencyUnit {
    double divisor;
    const char* name;
};

// Unité de l'axe des fréquences, choisie d'après f₀.
FrequencyUnit unitFor(double f0Hz) {
    if (f0Hz >= 1.0e9) {
        return {1.0e9, "GHz"};
    }
    if (f0Hz >= 1.0e6) {
        return {1.0e6, "MHz"};
    }
    if (f0Hz >= 1.0e3) {
        return {1.0e3, "kHz"};
    }
    return {1.0, "Hz"};
}

// Remplit les trois tableaux à partir d'un balayage (valeurs écrêtées pour le tracé).
void fillArrays(const SweepResult& sweep, double divisor, std::vector<double>& xs, std::vector<double>& s11,
                std::vector<double>& vswr) {
    xs.clear();
    s11.clear();
    vswr.clear();
    for (const SweepPoint& p : sweep.points) {
        xs.push_back(p.fHz / divisor);
        s11.push_back(std::clamp(p.s11Db, kPlotS11MinDb, kPlotS11MaxDb));
        vswr.push_back(std::isfinite(p.vswr) ? std::min(p.vswr, kPlotVswrMax) : kPlotVswrMax);
    }
}

// Style d'une courbe (couleur, épaisseur) au format ImPlot récent : une « spécification ».
ImPlotSpec lineSpec(const ImVec4& color, float weight, int flags = 0) {
    ImPlotSpec spec;
    spec.LineColor = color;
    spec.LineWeight = weight;
    spec.Flags = flags;
    return spec;
}

ValidationResult positiveFinite(double v) {
    return (std::isfinite(v) && v > 0.0) ? ValidationResult::success()
                                         : ValidationResult::failure("Value must be greater than 0.");
}

ValidationResult thresholdValidator(double v) {
    return (std::isfinite(v) && v > 1.0) ? ValidationResult::success()
                                         : ValidationResult::failure("VSWR threshold must be greater than 1.");
}

void settingRow(const char* label, const char* tip, bool learnMode) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (learnMode) {
        helpTooltip(tip);
    }
    ImGui::TableSetColumnIndex(1);
}

} // namespace

bool SweepPanel::draw(Project& p, const SweepResult& ideal, const SweepResult* rounded, bool learnMode) {
    bool changed = false;
    panelHeader("SWEEP");

    const float s = uiScale();

    // --- Colonne de gauche : réglages et bande passante -----------------------------------------------
    ImGui::BeginChild("sweepSettings", ImVec2(kSettingsWidth * s, 0.0f), 0);
    if (ImGui::BeginTable("sweepFields", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 0.55f);

        settingRow("Start", "Lowest frequency of the sweep.", learnMode);
        changed |= numberInput("##fmin", p.sweep.fMinHz, "Hz", FieldFormat::Engineering, fMin_, ImGui::GetContentRegionAvail().x, positiveFinite);
        settingRow("Stop", "Highest frequency of the sweep.", learnMode);
        changed |= numberInput("##fmax", p.sweep.fMaxHz, "Hz", FieldFormat::Engineering, fMax_, ImGui::GetContentRegionAvail().x, positiveFinite);

        settingRow("Points", "Number of frequency points (f0 is always added).", learnMode);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputInt("##points", &p.sweep.points, 50, 200)) {
            p.sweep.points = std::clamp(p.sweep.points, kMinSweepPoints, kMaxSweepPoints);
            changed = true;
        }

        settingRow("Scale", "Linear or logarithmic frequency axis.", learnMode);
        if (ImGui::RadioButton("Lin", !p.sweep.logScale)) {
            p.sweep.logScale = false;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Log", p.sweep.logScale)) {
            p.sweep.logScale = true;
            changed = true;
        }

        settingRow("VSWR max", "Bandwidth = contiguous range around f0 where VSWR stays below this value.",
                   learnMode);
        changed |= numberInput("##thr", p.sweep.vswrThreshold, "", FieldFormat::Plain, threshold_, ImGui::GetContentRegionAvail().x,
                               thresholdValidator);
        ImGui::EndTable();
    }

    // Bande passante (valeurs idéales, puis valeurs arrondies si une série E est choisie).
    if (!ideal.ok) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(kColorError, "%s", ideal.message.c_str());
        ImGui::PopTextWrapPos();
    } else {
        const BandwidthResult bw = bandwidth(ideal, p.sweep.vswrThreshold);
        ImGui::PushTextWrapPos(0.0f);
        if (bw.ok) {
            ImGui::TextColored(kColorGood, "Bandwidth: %s", formatBandwidth(bw).c_str());
            ImGui::TextColored(kColorTextDim, "%s \xE2\x80\x93 %s", formatEng(bw.fLowHz, "Hz", 4).c_str(),
                               formatEng(bw.fHighHz, "Hz", 4).c_str());
        } else {
            ImGui::TextColored(kColorWarning, "%s", bw.message.c_str());
        }
        if (rounded != nullptr && rounded->ok) {
            const BandwidthResult bwR = bandwidth(*rounded, p.sweep.vswrThreshold);
            ImGui::TextColored(kColorTextDim, "With %s: %s", eSeriesName(p.eSeries), formatBandwidth(bwR).c_str());
        }
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // --- Courbes ---------------------------------------------------------------------------------------------------
    if (!ideal.ok) {
        return changed;
    }
    const FrequencyUnit unit = unitFor(p.f0Hz);
    fillArrays(ideal, unit.divisor, xs_, s11_, vswr_);
    const bool withRounded = rounded != nullptr && rounded->ok;
    if (withRounded) {
        fillArrays(*rounded, unit.divisor, xsRounded_, s11Rounded_, vswrRounded_);
    }

    // Couleurs de l'appli appliquées à ImPlot (le thème d'ImGui ne les couvre pas).
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, kColorPanelBg);
    ImPlot::PushStyleColor(ImPlotCol_PlotBg, kColorChartBg);
    ImPlot::PushStyleColor(ImPlotCol_PlotBorder, kColorBorder);
    ImPlot::PushStyleColor(ImPlotCol_AxisGrid, withAlpha(kColorChartImpedanceGrid, 0.35f));
    ImPlot::PushStyleColor(ImPlotCol_AxisText, kColorTextDim);
    ImPlot::PushStyleColor(ImPlotCol_LegendBg, withAlpha(kColorPanelBg, 0.85f));
    ImPlot::PushStyleColor(ImPlotCol_LegendBorder, kColorBorder);
    ImPlot::PushStyleColor(ImPlotCol_LegendText, kColorText);

    const std::string xLabel = std::string("Frequency (") + unit.name + ")";
    if (ImPlot::BeginPlot("##sweepPlot", ImVec2(-1.0f, -1.0f), ImPlotFlags_NoTitle)) {
        ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str());
        ImPlot::SetupAxis(ImAxis_Y1, "|S11| (dB)");
        ImPlot::SetupAxis(ImAxis_Y2, "VSWR", ImPlotAxisFlags_AuxDefault);
        if (p.sweep.logScale) {
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, ideal.points.front().fHz / unit.divisor,
                                ideal.points.back().fHz / unit.divisor, ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, kPlotS11MinDb, kPlotS11MaxDb, ImPlotCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y2, kPlotVswrMin, kPlotVswrMax, ImPlotCond_Once);
        ImPlot::SetupLegend(ImPlotLocation_North, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);
        ImPlot::SetupFinish();

        // Bande passante : rectangle vert translucide sur toute la hauteur du tracé.
        const BandwidthResult bw = bandwidth(ideal, p.sweep.vswrThreshold);
        if (bw.ok) {
            const ImPlotRect limits = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1);
            const ImVec2 a = ImPlot::PlotToPixels(bw.fLowHz / unit.divisor, limits.Y.Max, ImAxis_X1, ImAxis_Y1);
            const ImVec2 b = ImPlot::PlotToPixels(bw.fHighHz / unit.divisor, limits.Y.Min, ImAxis_X1, ImAxis_Y1);
            ImPlot::PushPlotClipRect();
            ImPlot::GetPlotDrawList()->AddRectFilled(a, b, toU32(kColorPlotBand));
            ImPlot::PopPlotClipRect();
        }

        // |S11| sur l'axe gauche.
        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
        if (withRounded) {
            ImPlot::PlotLine((std::string("S11 ") + eSeriesName(p.eSeries)).c_str(), xsRounded_.data(),
                             s11Rounded_.data(), static_cast<int>(xsRounded_.size()),
                             lineSpec(kColorPlotS11ESer, kLineWeight));
        }
        ImPlot::PlotLine("S11 ideal", xs_.data(), s11_.data(), static_cast<int>(xs_.size()),
                         lineSpec(kColorPlotS11, kLineWeight));

        // f0 : ligne verticale.
        const double f0Plot = p.f0Hz / unit.divisor;
        ImPlot::PlotInfLines("f0", &f0Plot, 1, lineSpec(kColorPlotF0, kThresholdLineWeight, ImPlotItemFlags_NoLegend));

        // VSWR sur l'axe droit.
        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        if (withRounded) {
            ImPlot::PlotLine((std::string("VSWR ") + eSeriesName(p.eSeries)).c_str(), xsRounded_.data(),
                             vswrRounded_.data(), static_cast<int>(xsRounded_.size()),
                             lineSpec(kColorPlotVswrESer, kLineWeight));
        }
        ImPlot::PlotLine("VSWR ideal", xs_.data(), vswr_.data(), static_cast<int>(xs_.size()),
                         lineSpec(kColorPlotVswr, kLineWeight));
        const double threshold = p.sweep.vswrThreshold;
        ImPlot::PlotInfLines("VSWR limit", &threshold, 1,
                             lineSpec(kColorPlotThreshold, kThresholdLineWeight, static_cast<int>(ImPlotInfLinesFlags_Horizontal) | static_cast<int>(ImPlotItemFlags_NoLegend)));
        ImPlot::EndPlot();
    }
    ImPlot::PopStyleColor(8);
    return changed;
}

} // namespace smith::ui
