// =============================================================================
// SetupPanel.cpp — Implémentation du panneau SETUP
// =============================================================================
#include "ui/SetupPanel.hpp"

#include <cmath>

#include "core/RfMath.hpp"
#include "ui/LearnText.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

namespace {

// Une ligne « étiquette | champ » dans un tableau à 2 colonnes.
// Renvoie true quand la ligne vient d'être affichée (le champ se dessine ensuite).
void beginRow(const char* label, const char* tip, bool learnMode) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    if (learnMode) {
        helpTooltip(tip);
    }
    ImGui::TableSetColumnIndex(1);
}

ValidationResult finiteOnly(double v) {
    return std::isfinite(v) ? ValidationResult::success()
                            : ValidationResult::failure("Value must be a finite number.");
}

} // namespace

bool SetupPanel::draw(Project& p, bool learnMode) {
    bool changed = false;
    panelHeader("SETUP");

    const float labelWidth = 92.0f * uiScale();
    if (ImGui::BeginTable("setup", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
        ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch);

        // --- Z0 ----------------------------------------------------------------
        beginRow("Z0", learn::quantityTip(learn::Quantity::Z0), learnMode);
        changed |= numberInput("##z0", p.z0, "\xCE\xA9", FieldFormat::Plain, z0_, -1.0f, validateZ0);

        // --- f0 ------------------------------------------------------------------
        beginRow("Frequency f0", learn::quantityTip(learn::Quantity::Frequency), learnMode);
        {
            const double oldF0 = p.f0Hz;
            if (numberInput("##f0", p.f0Hz, "Hz", FieldFormat::Engineering, f0_, -1.0f, validateFrequency)) {
                changed = true;
                // Si la plage de balayage était la plage par défaut (f0/2 … 2·f0), elle suit f0.
                if (p.sweep.fMinHz == oldF0 / 2.0 && p.sweep.fMaxHz == oldF0 * 2.0) {
                    p.sweep.fMinHz = p.f0Hz / 2.0;
                    p.sweep.fMaxHz = p.f0Hz * 2.0;
                }
            }
        }

        // --- Source Zs ---------------------------------------------------------------
        beginRow("Source Zs  R", learn::quantityTip(learn::Quantity::SourceImpedance), learnMode);
        {
            double r = p.zSource.real();
            if (numberInput("##zsr", r, "\xCE\xA9", FieldFormat::Plain, zsR_, -1.0f,
                            [&](double v) { return validateSourceImpedance(Complex(v, p.zSource.imag())); })) {
                p.zSource = Complex(r, p.zSource.imag());
                changed = true;
            }
        }
        beginRow("Source Zs  jX", learn::quantityTip(learn::Quantity::SourceImpedance), learnMode);
        {
            double x = p.zSource.imag();
            if (numberInput("##zsx", x, "\xCE\xA9", FieldFormat::Plain, zsX_, -1.0f,
                            [&](double v) { return validateSourceImpedance(Complex(p.zSource.real(), v)); })) {
                p.zSource = Complex(p.zSource.real(), x);
                changed = true;
            }
        }

        // --- Charge ZL -------------------------------------------------------------------
        beginRow("Load ZL  R", learn::quantityTip(learn::Quantity::LoadImpedance), learnMode);
        {
            double r = p.load.zAtF0.real();
            if (numberInput("##zlr", r, "\xCE\xA9", FieldFormat::Plain, zlR_, -1.0f,
                            [&](double v) { return validateLoadImpedance(Complex(v, p.load.zAtF0.imag())); })) {
                p.load.zAtF0 = Complex(r, p.load.zAtF0.imag());
                changed = true;
            }
        }
        beginRow("Load ZL  jX", learn::quantityTip(learn::Quantity::LoadImpedance), learnMode);
        {
            double x = p.load.zAtF0.imag();
            if (numberInput("##zlx", x, "\xCE\xA9", FieldFormat::Plain, zlX_, -1.0f,
                            [&](double v) { return validateLoadImpedance(Complex(p.load.zAtF0.real(), v)); })) {
                p.load.zAtF0 = Complex(p.load.zAtF0.real(), x);
                changed = true;
            }
        }

        // --- Saisie alternative en Γ -----------------------------------------------------------
        const Complex gamma = gammaFromZ(p.load.zAtF0, p.z0);
        double mag = std::abs(gamma);
        double angleDeg = std::arg(gamma) * 180.0 / kPi;
        bool gammaEdited = false;
        beginRow("ZL as |\xCE\x93|", learn::quantityTip(learn::Quantity::ReflectionInput), learnMode);
        gammaEdited |= numberInput("##gmag", mag, "", FieldFormat::Plain, gammaMag_, -1.0f, [](double v) {
            if (!std::isfinite(v) || v < 0.0 || v >= 1.0) {
                return ValidationResult::failure("|\xCE\x93| must be between 0 and 1 (exclusive) for a passive load.");
            }
            return ValidationResult::success();
        });
        beginRow("ZL as arg \xCE\x93 (\xC2\xB0)", learn::quantityTip(learn::Quantity::ReflectionInput), learnMode);
        gammaEdited |= numberInput("##gang", angleDeg, "", FieldFormat::Plain, gammaAngle_, -1.0f, finiteOnly);
        if (gammaEdited) {
            const Complex newZ = zFromGamma(std::polar(mag, angleDeg * kPi / 180.0), p.z0);
            if (validateLoadImpedance(newZ).ok) {
                p.load.zAtF0 = newZ;
                changed = true;
            }
        }

        // --- Modèle de charge -------------------------------------------------------------------
        beginRow("Load model", "", false);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##loadmodel", loadModelName(p.load.model))) {
            for (LoadModel model : {LoadModel::Constant, LoadModel::SeriesEquivalent, LoadModel::ParallelEquivalent}) {
                const bool selected = (model == p.load.model);
                if (ImGui::Selectable(loadModelName(model), selected)) {
                    p.load.model = model;
                    changed = true;
                }
                helpTooltip(learn::loadModelTip(model));
            }
            ImGui::EndCombo();
        }
        helpTooltip(learn::loadModelTip(p.load.model));

        // --- Série E ----------------------------------------------------------------------------
        beginRow("E-series", learn::quantityTip(learn::Quantity::ESeries), learnMode);
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::BeginCombo("##eseries", eSeriesName(p.eSeries))) {
            for (ESeriesKind kind : {ESeriesKind::None, ESeriesKind::E12, ESeriesKind::E24}) {
                if (ImGui::Selectable(eSeriesName(kind), kind == p.eSeries)) {
                    p.eSeries = kind;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
    return changed;
}

} // namespace smith::ui
