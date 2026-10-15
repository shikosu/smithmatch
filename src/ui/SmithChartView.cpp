// =============================================================================
// SmithChartView.cpp — Dessin de l'abaque de Smith
// =============================================================================
// Principe : tout est calculé dans le plan Γ (le disque unité), puis chaque
// point est converti en pixels par gammaToScreen(). Les courbes de grille sont
// obtenues en ÉCHANTILLONNANT la transformation Γ = (z − 1)/(z + 1) (pas de
// calcul d'arcs à la main) : on fait varier x (ou r) de façon non linéaire
// avec une tangente pour bien couvrir jusqu'à l'infini.
//
// Analogie : c'est comme tracer un graphe point par point : on calcule 192
// points de chaque courbe et on les relie par de petits segments.
// =============================================================================
#include "ui/SmithChartView.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/Network.hpp"
#include "core/RfMath.hpp"
#include "core/Units.hpp"
#include "ui/LearnText.hpp"
#include "ui/Theme.hpp"

namespace smith::ui {

namespace {

// Valeurs de la grille (normalisées par Z₀), pour r, x, g et b.
constexpr std::array<double, 5> kGridValues = {0.2, 0.5, 1.0, 2.0, 5.0};
constexpr int kGridSamples = 192;        // points par courbe de grille (≥ 128 demandés)
constexpr double kHalfPi = kPi / 2.0;
constexpr double kAngleMargin = 1.0e-6;  // évite tan(π/2) = ∞ exactement

// Tailles en pixels pour un facteur d'interface « ui » = 1 (ajustées par le zoom d'interface).
constexpr float kChartMargin    = 36.0f;   // marge pour les étiquettes autour du cercle
constexpr float kPathThickness  = 2.6f;
constexpr float kGridThickness  = 1.0f;
constexpr float kMarkerRadius   = 5.0f;
constexpr float kArrowSize      = 9.0f;
constexpr float kDashLength     = 7.0f;
constexpr float kDashGap        = 5.0f;
constexpr float kLabelPadding   = 3.0f;
constexpr double kHoverEdgeEps  = 1.0e-9;

// Γ d'une impédance normalisée z : (z − 1)/(z + 1).
Complex gammaOfNormalizedZ(Complex z) { return (z - 1.0) / (z + 1.0); }

// Cercle de résistance r = const : on fait varier x = tan(θ) de −∞ à +∞.
std::vector<Complex> makeResistanceCircle(double r) {
    std::vector<Complex> points;
    points.reserve(kGridSamples + 2);
    points.emplace_back(1.0, 0.0);   // x = −∞ : Γ = 1 (circuit ouvert)
    for (int i = 1; i < kGridSamples; ++i) {
        const double theta = -kHalfPi + kPi * static_cast<double>(i) / static_cast<double>(kGridSamples);
        points.push_back(gammaOfNormalizedZ(Complex(r, std::tan(theta))));
    }
    points.emplace_back(1.0, 0.0);   // x = +∞
    return points;
}

// Arc de réactance x = const (x > 0 : moitié haute) : on fait varier r = tan(θ) de 0 à +∞.
std::vector<Complex> makeReactanceArc(double x) {
    std::vector<Complex> points;
    points.reserve(kGridSamples + 1);
    for (int i = 0; i < kGridSamples; ++i) {
        const double theta = (kHalfPi - kAngleMargin) * static_cast<double>(i) / static_cast<double>(kGridSamples);
        points.push_back(gammaOfNormalizedZ(Complex(std::tan(theta), x)));
    }
    points.emplace_back(1.0, 0.0);   // r = +∞ : Γ = 1
    return points;
}

// Contexte de dessin partagé par toutes les fonctions : où dessiner, où est le centre
// du cercle unité, son rayon en pixels, et le facteur d'échelle de l'interface.
struct Frame {
    ImDrawList* drawList = nullptr;
    Point2 center;
    double radius = 1.0;
    float ui = 1.0f;       // facteur d'échelle de l'interface (police, HiDPI)
};

ImVec2 toPixel(const Frame& f, Complex gamma) {
    const Point2 p = gammaToScreen(gamma, f.center, f.radius);
    return ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
}

// Convertit une liste de Γ en pixels (avec une symétrie éventuelle Γ → −Γ pour l'admittance).
void fillScreenPoints(const Frame& f, const std::vector<Complex>& gammas, bool mirror,
                      std::vector<ImVec2>& out) {
    out.clear();
    out.reserve(gammas.size());
    for (const Complex& g : gammas) {
        out.push_back(toPixel(f, mirror ? -g : g));
    }
}

void drawPolyline(const Frame& f, const std::vector<ImVec2>& pts, const ImVec4& color, float thickness) {
    if (pts.size() >= 2) {
        f.drawList->AddPolyline(pts.data(), static_cast<int>(pts.size()), toU32(color), thickness, 0);
    }
}

// Polyligne en tirets : on parcourt les segments en cumulant la longueur dessinée.
void drawDashedPolyline(const Frame& f, const std::vector<ImVec2>& pts, const ImVec4& color, float thickness) {
    const float dash = kDashLength * f.ui;
    const float gap = kDashGap * f.ui;
    const ImU32 col = toU32(color);
    float phase = 0.0f;       // position dans le cycle « trait + trou »
    const float cycle = dash + gap;
    for (std::size_t i = 1; i < pts.size(); ++i) {
        ImVec2 a = pts[i - 1];
        const ImVec2 b = pts[i];
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        float remaining = std::sqrt(dx * dx + dy * dy);
        if (remaining < 1.0e-6f) {
            continue;
        }
        const float ux = dx / remaining;
        const float uy = dy / remaining;
        while (remaining > 0.0f) {
            const bool inDash = phase < dash;
            const float untilChange = inDash ? (dash - phase) : (cycle - phase);
            const float step = std::min(untilChange, remaining);
            const ImVec2 next(a.x + ux * step, a.y + uy * step);
            if (inDash) {
                f.drawList->AddLine(a, next, col, thickness);
            }
            a = next;
            remaining -= step;
            phase += step;
            if (phase >= cycle - 1.0e-6f) {
                phase = 0.0f;
            }
        }
    }
}

// Étiquette avec un petit fond sombre (lisible même sur les courbes).
void drawLabel(const Frame& f, ImVec2 topLeft, const ImVec4& color, const std::string& text, bool background) {
    const ImVec2 size = ImGui::CalcTextSize(text.c_str());
    if (background) {
        const float pad = kLabelPadding * f.ui;
        f.drawList->AddRectFilled(ImVec2(topLeft.x - pad, topLeft.y - pad / 2),
                                  ImVec2(topLeft.x + size.x + pad, topLeft.y + size.y + pad / 2),
                                  toU32(withAlpha(kColorChartBg, 0.85f)), 3.0f * f.ui);
    }
    f.drawList->AddText(topLeft, toU32(color), text.c_str());
}

// Étiquette centrée sur un point, repoussée vers l'extérieur du cercle unité
// (direction « dir » = vecteur unitaire écran du centre vers le point).
void drawOutsideLabel(const Frame& f, ImVec2 anchor, ImVec2 dir, const ImVec4& color, const std::string& text) {
    const ImVec2 size = ImGui::CalcTextSize(text.c_str());
    // Demi-étendue du texte dans la direction « dir » pour ne pas toucher le cercle.
    const float shift = std::fabs(dir.x) * size.x * 0.5f + std::fabs(dir.y) * size.y * 0.5f + 4.0f * f.ui;
    const ImVec2 centerOfText(anchor.x + dir.x * shift, anchor.y + dir.y * shift);
    f.drawList->AddText(ImVec2(centerOfText.x - size.x * 0.5f, centerOfText.y - size.y * 0.5f),
                        toU32(color), text.c_str());
}

// Comme drawOutsideLabel, mais À L'INTÉRIEUR du cercle unité (direction « dir » inversée),
// avec un fond sombre. Sert aux étiquettes de la grille d'admittance : un arc b = −2 et un
// arc x = +0,5 aboutissent au même point du cercle ; on évite ainsi qu'elles se superposent.
void drawInsideLabel(const Frame& f, ImVec2 anchor, ImVec2 dir, const ImVec4& color, const std::string& text) {
    const ImVec2 size = ImGui::CalcTextSize(text.c_str());
    const float shift = std::fabs(dir.x) * size.x * 0.5f + std::fabs(dir.y) * size.y * 0.5f + 3.0f * f.ui;
    const ImVec2 centerOfText(anchor.x - dir.x * shift, anchor.y - dir.y * shift);
    const ImVec2 topLeft(centerOfText.x - size.x * 0.5f, centerOfText.y - size.y * 0.5f);
    drawLabel(f, topLeft, color, text, true);
}

// Triangle plein à l'extrémité d'un arc pour montrer le sens du déplacement.
void drawArrowHead(const Frame& f, ImVec2 tip, ImVec2 from, const ImVec4& color) {
    float dx = tip.x - from.x;
    float dy = tip.y - from.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0e-3f) {
        return;
    }
    dx /= len;
    dy /= len;
    const float size = kArrowSize * f.ui;
    const ImVec2 base(tip.x - dx * size, tip.y - dy * size);
    const ImVec2 left(base.x - dy * size * 0.5f, base.y + dx * size * 0.5f);
    const ImVec2 right(base.x + dy * size * 0.5f, base.y - dx * size * 0.5f);
    f.drawList->AddTriangleFilled(tip, left, right, toU32(color));
}

const ImVec4& colorForType(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor: return kColorSeriesL;
        case ElementType::SeriesCapacitor: return kColorSeriesC;
        case ElementType::ShuntInductor: return kColorShuntL;
        case ElementType::ShuntCapacitor: return kColorShuntC;
    }
    return kColorText;
}

// Dessine un réseau complet (un arc par élément). « dashed » = aperçu du solveur.
void drawNetworkPaths(const Frame& f, const ChartScene& scene, const ElementList& network, bool dashed,
                      std::vector<ImVec2>& scratch) {
    for (std::size_t i = 0; i < network.size(); ++i) {
        const std::vector<Complex> arc = sampledPath(network, i, scene.zLoad, scene.f0Hz, scene.z0, 64);
        fillScreenPoints(f, arc, false, scratch);
        if (scratch.size() < 2) {
            continue;
        }
        const ImVec4& color = dashed ? kColorPreview : colorForType(network[i].type);
        const bool highlighted = !dashed && static_cast<int>(i) == scene.highlightedElement;
        const float thickness = (highlighted ? kPathThickness * 1.8f : kPathThickness) * f.ui;
        if (dashed) {
            drawDashedPolyline(f, scratch, color, thickness * 0.8f);
        } else {
            drawPolyline(f, scratch, color, thickness);
        }

        // Flèche à l'arrivée (on prend un point un peu en arrière pour une direction stable).
        const std::size_t n = scratch.size();
        const std::size_t back = n > 4 ? n - 4 : 0;
        if (!dashed) {
            drawArrowHead(f, scratch[n - 1], scratch[back], color);
        }

        // Pastille numérotée au milieu de l'arc, légèrement décalée de la courbe.
        if (!dashed) {
            const ImVec2 mid = scratch[n / 2];
            const ImVec2 before = scratch[n / 2 - 1];
            float dx = mid.x - before.x;
            float dy = mid.y - before.y;
            const float len = std::max(std::sqrt(dx * dx + dy * dy), 1.0e-6f);
            dx /= len;
            dy /= len;
            const float offset = 12.0f * f.ui;
            const ImVec2 badge(mid.x - dy * offset, mid.y + dx * offset);
            const float r = 8.0f * f.ui;
            f.drawList->AddCircleFilled(badge, r, toU32(withAlpha(kColorChartBg, 0.9f)));
            f.drawList->AddCircle(badge, r, toU32(color), 0, 1.5f * f.ui);
            const std::string number = std::to_string(i + 1);
            const ImVec2 size = ImGui::CalcTextSize(number.c_str());
            f.drawList->AddText(ImVec2(badge.x - size.x * 0.5f, badge.y - size.y * 0.5f), toU32(color),
                                number.c_str());
        }
    }
}

// Texte d'une impédance ou admittance (gère le cas « infini »).
std::string describeComplex(Complex value, int sig, const char* unit) {
    if (std::abs(value.real()) > 1.0e12 || std::abs(value.imag()) > 1.0e12) {
        return std::string("\xE2\x88\x9E ") + unit;
    }
    const std::string text = formatComplex(value, sig);
    return (unit[0] == '\0') ? text : text + " " + unit;
}

} // namespace

// -----------------------------------------------------------------------------
// Constructeur : calcule une fois les courbes de la grille d'impédance.
// -----------------------------------------------------------------------------
SmithChartView::SmithChartView() {
    for (std::size_t i = 0; i < kGridCount; ++i) {
        resistanceCircles_[i] = makeResistanceCircle(kGridValues[i]);
        reactanceArcsPos_[i] = makeReactanceArc(kGridValues[i]);
        // x < 0 : même courbe avec x → −x, ce qui conjugue Γ (symétrie par rapport à l'axe réel).
        reactanceArcsNeg_[i] = makeReactanceArc(-kGridValues[i]);
    }
}

// -----------------------------------------------------------------------------
// draw : une image complète de l'abaque.
// -----------------------------------------------------------------------------
void SmithChartView::draw(const ChartScene& scene, const ChartOptions& options) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 80.0f || avail.y < 80.0f) {
        return;
    }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##smithchart", avail);
    const bool hovered = ImGui::IsItemHovered();

    rectMin_ = origin;
    rectMax_ = ImVec2(origin.x + avail.x, origin.y + avail.y);

    // --- Géométrie : cercle unité carré, centré, avec une marge pour les étiquettes --
    Frame f;
    f.drawList = ImGui::GetWindowDrawList();
    f.ui = ImGui::GetFontSize() / kBaseFontSize;
    const float margin = kChartMargin * f.ui;
    const float radius = std::max(20.0f, std::min(avail.x, avail.y) * 0.5f - margin);
    f.center = Point2{static_cast<double>(origin.x + avail.x * 0.5f),
                      static_cast<double>(origin.y + avail.y * 0.5f)};
    f.radius = static_cast<double>(radius);
    const ImVec2 centerPx(static_cast<float>(f.center.x), static_cast<float>(f.center.y));

    f.drawList->PushClipRect(rectMin_, rectMax_, true);
    f.drawList->AddRectFilled(rectMin_, rectMax_, toU32(kColorPanelBg), 6.0f * f.ui);
    f.drawList->AddCircleFilled(centerPx, radius, toU32(kColorChartBg), 256);

    // --- Grilles ---------------------------------------------------------------
    const float gridThickness = kGridThickness * f.ui;
    if (options.admittanceGrid) {
        for (std::size_t i = 0; i < kGridCount; ++i) {
            fillScreenPoints(f, resistanceCircles_[i], true, scratch_);       // cercles g
            drawPolyline(f, scratch_, kColorChartAdmittanceGrid, gridThickness);
            fillScreenPoints(f, reactanceArcsPos_[i], true, scratch_);        // arcs b > 0 (moitié basse)
            drawPolyline(f, scratch_, kColorChartAdmittanceGrid, gridThickness);
            fillScreenPoints(f, reactanceArcsNeg_[i], true, scratch_);        // arcs b < 0 (moitié haute)
            drawPolyline(f, scratch_, kColorChartAdmittanceGrid, gridThickness);
        }
    }
    for (std::size_t i = 0; i < kGridCount; ++i) {
        fillScreenPoints(f, resistanceCircles_[i], false, scratch_);
        drawPolyline(f, scratch_, kColorChartImpedanceGrid, gridThickness);
        fillScreenPoints(f, reactanceArcsPos_[i], false, scratch_);
        drawPolyline(f, scratch_, kColorChartImpedanceGrid, gridThickness);
        fillScreenPoints(f, reactanceArcsNeg_[i], false, scratch_);
        drawPolyline(f, scratch_, kColorChartImpedanceGrid, gridThickness);
    }
    // Axe réel et cercle unité (r = 0).
    f.drawList->AddLine(ImVec2(centerPx.x - radius, centerPx.y), ImVec2(centerPx.x + radius, centerPx.y),
                        toU32(kColorChartImpedanceGrid), gridThickness);
    f.drawList->AddCircle(centerPx, radius, toU32(kColorChartUnitCircle), 256, 1.8f * f.ui);

    // --- Étiquettes de la grille ---------------------------------------------------
    ImGui::PushFont(nullptr, kSmallFontSize);
    for (double v : kGridValues) {
        const std::string text = formatSig(v, 2);
        // r : le cercle r coupe l'axe réel à Γ = (r − 1)/(r + 1) ; texte juste au-dessus de l'axe.
        const ImVec2 rAxis = toPixel(f, Complex((v - 1.0) / (v + 1.0), 0.0));
        const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        f.drawList->AddText(ImVec2(rAxis.x + 3.0f * f.ui, rAxis.y - textSize.y - 1.0f * f.ui),
                            toU32(kColorChartLabel), text.c_str());
        if (options.admittanceGrid) {
            // g : même position, symétrique, texte juste sous l'axe.
            const ImVec2 gAxis = toPixel(f, Complex(-(v - 1.0) / (v + 1.0), 0.0));
            f.drawList->AddText(ImVec2(gAxis.x + 3.0f * f.ui, gAxis.y + 1.0f * f.ui),
                                toU32(kColorChartAdmLabel), text.c_str());
        }
        // x : l'arc x = ±v rejoint le cercle unité en Γ = (jx − 1)/(jx + 1).
        for (const double sign : {1.0, -1.0}) {
            const Complex edge = gammaOfNormalizedZ(Complex(0.0, sign * v));
            const ImVec2 edgePx = toPixel(f, edge);
            ImVec2 dir(edgePx.x - centerPx.x, edgePx.y - centerPx.y);
            const float len = std::max(std::sqrt(dir.x * dir.x + dir.y * dir.y), 1.0e-6f);
            dir = ImVec2(dir.x / len, dir.y / len);
            drawOutsideLabel(f, edgePx, dir, kColorChartLabel, std::string(sign > 0 ? "+" : "\xE2\x88\x92") + text);
            if (options.admittanceGrid) {
                // b : symétrique (Γ → −Γ) ; b > 0 se trouve dans la moitié basse.
                const Complex edgeY = -edge;
                const ImVec2 edgeYPx = toPixel(f, edgeY);
                ImVec2 dirY(edgeYPx.x - centerPx.x, edgeYPx.y - centerPx.y);
                const float lenY = std::max(std::sqrt(dirY.x * dirY.x + dirY.y * dirY.y), 1.0e-6f);
                dirY = ImVec2(dirY.x / lenY, dirY.y / lenY);
                drawInsideLabel(f, edgeYPx, dirY, kColorChartAdmLabel,
                                std::string(sign > 0 ? "+" : "\xE2\x88\x92") + text);
            }
        }
    }
    ImGui::PopFont();

    // --- Repères --------------------------------------------------------------------
    const std::vector<Complex> nodesZ = nodeImpedances(scene.network, scene.zLoad, scene.f0Hz);
    const Complex zIn = nodesZ.back();
    const Complex gammaIn = gammaFromZ(zIn, scene.z0);

    // Cercle VSWR : centré sur l'origine, passe par Z_in.
    if (options.vswrCircle) {
        const double mag = std::min(std::abs(gammaIn), 1.0);
        std::vector<ImVec2>& circle = scratch_;
        circle.clear();
        constexpr int kCircleSamples = 180;
        for (int i = 0; i <= kCircleSamples; ++i) {
            const double a = kTwoPi * static_cast<double>(i) / static_cast<double>(kCircleSamples);
            circle.push_back(toPixel(f, std::polar(mag, a)));
        }
        drawDashedPolyline(f, circle, kColorVswrCircle, 1.6f * f.ui);
    }

    // Lieu du balayage Γ(Z_in(f)).
    if (options.sweepLocus && scene.sweepLocus.size() >= 2) {
        fillScreenPoints(f, scene.sweepLocus, false, scratch_);
        drawPolyline(f, scratch_, kColorSweepLocus, 2.0f * f.ui);
        if (scene.sweepF0Index < scratch_.size()) {
            const ImVec2 p = scratch_[scene.sweepF0Index];
            f.drawList->AddCircleFilled(p, 4.0f * f.ui, toU32(kColorSweepLocus));
            f.drawList->AddCircle(p, 6.5f * f.ui, toU32(kColorSweepLocus), 0, 1.5f * f.ui);
        }
    }

    // Aperçu du solveur (tirets) puis réseau courant.
    if (!scene.previewNetwork.empty()) {
        drawNetworkPaths(f, scene, scene.previewNetwork, true, scratch_);
    }
    drawNetworkPaths(f, scene, scene.network, false, scratch_);

    ImGui::PushFont(nullptr, kSmallFontSize);

    // Nœuds intermédiaires (entre le nœud 0 = Z_L et le dernier = Z_in).
    for (std::size_t i = 1; i + 1 < nodesZ.size(); ++i) {
        const ImVec2 p = toPixel(f, gammaFromZ(nodesZ[i], scene.z0));
        f.drawList->AddCircleFilled(p, 3.5f * f.ui, toU32(kColorNodeMarker));
    }

    // Cible Z_s* : anneau + croix blanche, étiquette « target ».
    {
        const ImVec2 p = toPixel(f, gammaFromZ(std::conj(scene.zSource), scene.z0));
        const float r = 8.0f * f.ui;
        f.drawList->AddCircle(p, r, toU32(kColorTargetMarker), 0, 1.6f * f.ui);
        f.drawList->AddLine(ImVec2(p.x - r, p.y), ImVec2(p.x + r, p.y), toU32(kColorTargetMarker), 1.2f * f.ui);
        f.drawList->AddLine(ImVec2(p.x, p.y - r), ImVec2(p.x, p.y + r), toU32(kColorTargetMarker), 1.2f * f.ui);
        drawLabel(f, ImVec2(p.x + r + 3.0f * f.ui, p.y + 3.0f * f.ui), kColorTargetMarker, "target", true);
    }

    // Charge Z_L (disque rouge) et entrée Z_in (disque jaune, seulement si le réseau n'est pas vide).
    {
        const ImVec2 p = toPixel(f, gammaFromZ(scene.zLoad, scene.z0));
        f.drawList->AddCircleFilled(p, kMarkerRadius * f.ui, toU32(kColorLoadMarker));
        f.drawList->AddCircle(p, (kMarkerRadius + 2.0f) * f.ui, toU32(kColorChartBg), 0, 1.5f * f.ui);
        drawLabel(f, ImVec2(p.x + 8.0f * f.ui, p.y - 18.0f * f.ui), kColorLoadMarker, "ZL", true);
    }
    if (!scene.network.empty()) {
        const ImVec2 p = toPixel(f, gammaIn);
        f.drawList->AddCircleFilled(p, kMarkerRadius * f.ui, toU32(kColorZinMarker));
        f.drawList->AddCircle(p, (kMarkerRadius + 2.0f) * f.ui, toU32(kColorChartBg), 0, 1.5f * f.ui);
        drawLabel(f, ImVec2(p.x + 8.0f * f.ui, p.y - 18.0f * f.ui), kColorZinMarker, "Zin", true);
    }

    // --- Légende des 4 couleurs (en haut à gauche) ---------------------------------------
    {
        struct Entry { const char* name; const ImVec4* color; };
        const Entry entries[] = {{"Series L", &kColorSeriesL}, {"Series C", &kColorSeriesC},
                                 {"Shunt L", &kColorShuntL},   {"Shunt C", &kColorShuntC}};
        const float lineHeight = ImGui::GetTextLineHeight() + 3.0f * f.ui;
        float y = origin.y + 10.0f * f.ui;
        const float x = origin.x + 12.0f * f.ui;
        for (const Entry& e : entries) {
            f.drawList->AddLine(ImVec2(x, y + lineHeight * 0.5f - 1.0f * f.ui),
                                ImVec2(x + 18.0f * f.ui, y + lineHeight * 0.5f - 1.0f * f.ui),
                                toU32(*e.color), 3.0f * f.ui);
            f.drawList->AddText(ImVec2(x + 24.0f * f.ui, y), toU32(kColorChartLabel), e.name);
            y += lineHeight;
        }
        if (options.admittanceGrid) {
            f.drawList->AddLine(ImVec2(x, y + lineHeight * 0.5f - 1.0f * f.ui),
                                ImVec2(x + 18.0f * f.ui, y + lineHeight * 0.5f - 1.0f * f.ui),
                                toU32(kColorChartAdmittanceGrid), 2.0f * f.ui);
            f.drawList->AddText(ImVec2(x + 24.0f * f.ui, y), toU32(kColorChartLabel), "Admittance grid");
        }
    }

    // --- Légende pédagogique (mode Learn) -------------------------------------------------
    if (options.learnMode) {
        const char* hint = learn::chartOrientationHint();
        const ImVec2 size = ImGui::CalcTextSize(hint);
        f.drawList->AddText(ImVec2(origin.x + (avail.x - size.x) * 0.5f, origin.y + avail.y - size.y - 8.0f * f.ui),
                            toU32(kColorTextDim), hint);
    }
    ImGui::PopFont();

    // --- Survol : croix + infobulle ---------------------------------------------------------
    if (hovered) {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const Complex g = screenToGamma(Point2{static_cast<double>(mouse.x), static_cast<double>(mouse.y)},
                                        f.center, f.radius);
        if (std::abs(g) <= 1.0 - kHoverEdgeEps) {
            f.drawList->AddCircle(mouse, 5.0f * f.ui, toU32(kColorHoverCross), 0, 1.2f * f.ui);

            const Complex z = zFromGamma(g, scene.z0);              // Ω
            const Complex zn = z / scene.z0;                        // normalisée
            const Complex y = (std::abs(z) < kTinyImpedance) ? Complex(kOpenCircuitOhm, 0.0) : 1.0 / z;  // S
            const Complex yn = y * scene.z0;
            const double mag = std::abs(g);
            const double angleDeg = std::arg(g) * 180.0 / kPi;
            const double vswrValue = vswr(mag);

            ImGui::BeginTooltip();
            ImGui::Text("z = %s", describeComplex(zn, 3, "").c_str());
            ImGui::Text("Z = %s", describeComplex(z, 4, "\xCE\xA9").c_str());
            ImGui::Text("y = %s", describeComplex(yn, 3, "").c_str());
            ImGui::Text("Y = %s", describeComplex(y * 1000.0, 4, "mS").c_str());
            ImGui::Text("|\xCE\x93| = %s,  arg \xCE\x93 = %s\xC2\xB0", formatSig(mag, 3).c_str(), formatSig(angleDeg, 4).c_str());
            ImGui::Text("VSWR = %s", formatSig(vswrValue, 3).c_str());
            ImGui::EndTooltip();
        }
    }

    f.drawList->PopClipRect();
}

} // namespace smith::ui
