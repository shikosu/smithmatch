// =============================================================================
// SchematicView.cpp — Dessin du schéma électrique (ImDrawList)
// =============================================================================
// Toutes les formes sont tracées avec des lignes. Pour dessiner un composant dans
// n'importe quelle direction (horizontal pour un série, vertical pour un shunt), on
// utilise un repère local : « axe » (le long du composant) et « normale » (perpendiculaire).
//     point = début + axe·t + normale·v
// =============================================================================
#include "ui/SchematicView.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/ESeries.hpp"
#include "core/Types.hpp"
#include "core/Units.hpp"
#include "ui/Theme.hpp"
#include "ui/Widgets.hpp"

namespace smith::ui {

namespace {

// Dimensions en points (avant mise à l'échelle HiDPI).
constexpr float kComponentLength = 46.0f;    // longueur d'un composant le long de son axe
constexpr float kMinSpacing = 118.0f;        // distance minimale entre deux éléments
constexpr float kMargin = 18.0f;
constexpr float kSourceRadius = 15.0f;
constexpr float kLoadWidth = 24.0f;
constexpr float kLoadHeight = 46.0f;
constexpr float kPlateHalfSize = 11.0f;      // demi-longueur d'une plaque de condensateur
constexpr float kPlateGap = 4.0f;            // demi-écart entre les plaques
constexpr float kHumpRadius = 5.8f;          // rayon d'une boucle de bobine
constexpr int kHumpCount = 4;
constexpr int kArcSegments = 12;
constexpr float kWireThickness = 1.6f;
constexpr float kPartThickness = 2.2f;
constexpr float kGroundHalfWidth = 11.0f;
constexpr float kMinHeight = 150.0f;
constexpr float kHoverMargin = 8.0f;
constexpr float kPiFloat = static_cast<float>(kPi);

struct Axes {
    ImVec2 origin;   // début du composant
    ImVec2 axis;     // vecteur unitaire le long du composant
    ImVec2 normal;   // vecteur unitaire perpendiculaire
    ImVec2 at(float t, float v) const {
        return ImVec2(origin.x + axis.x * t + normal.x * v, origin.y + axis.y * t + normal.y * v);
    }
};

void line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float thickness) { dl->AddLine(a, b, color, thickness); }

// Bobine : fils d'entrée/sortie puis « kHumpCount » demi-cercles.
void drawInductor(ImDrawList* dl, const Axes& ax, float length, ImU32 color, float s) {
    const float r = kHumpRadius * s;
    const float coil = 2.0f * r * static_cast<float>(kHumpCount);
    const float lead = (length - coil) * 0.5f;
    line(dl, ax.at(0.0f, 0.0f), ax.at(lead, 0.0f), color, kPartThickness * s);
    for (int h = 0; h < kHumpCount; ++h) {
        const float centerT = lead + r + 2.0f * r * static_cast<float>(h);
        ImVec2 pts[kArcSegments + 1];
        for (int i = 0; i <= kArcSegments; ++i) {
            const float a = kPiFloat * (1.0f - static_cast<float>(i) / static_cast<float>(kArcSegments));   // π → 0
            pts[i] = ax.at(centerT + r * std::cos(a), r * std::sin(a));
        }
        dl->AddPolyline(pts, kArcSegments + 1, color, kPartThickness * s, ImDrawFlags_None);
    }
    line(dl, ax.at(lead + coil, 0.0f), ax.at(length, 0.0f), color, kPartThickness * s);
}

// Condensateur : deux fils et deux plaques parallèles.
void drawCapacitor(ImDrawList* dl, const Axes& ax, float length, ImU32 color, float s) {
    const float mid = length * 0.5f;
    const float gap = kPlateGap * s;
    const float half = kPlateHalfSize * s;
    line(dl, ax.at(0.0f, 0.0f), ax.at(mid - gap, 0.0f), color, kPartThickness * s);
    line(dl, ax.at(mid + gap, 0.0f), ax.at(length, 0.0f), color, kPartThickness * s);
    line(dl, ax.at(mid - gap, -half), ax.at(mid - gap, half), color, kPartThickness * s);
    line(dl, ax.at(mid + gap, -half), ax.at(mid + gap, half), color, kPartThickness * s);
}

// Symbole de masse : trois traits de plus en plus courts.
void drawGround(ImDrawList* dl, ImVec2 top, ImU32 color, float s) {
    const float w = kGroundHalfWidth * s;
    line(dl, top, ImVec2(top.x, top.y + 6.0f * s), color, kWireThickness * s);
    for (int i = 0; i < 3; ++i) {
        const float y = top.y + 6.0f * s + 4.0f * s * static_cast<float>(i);
        const float k = 1.0f - 0.3f * static_cast<float>(i);
        line(dl, ImVec2(top.x - w * k, y), ImVec2(top.x + w * k, y), color, kWireThickness * s);
    }
}

// Texte centré horizontalement sur x, dont le haut est à y.
void centeredText(ImDrawList* dl, float x, float y, ImU32 color, const std::string& text) {
    const ImVec2 size = ImGui::CalcTextSize(text.c_str());
    dl->AddText(ImVec2(x - size.x * 0.5f, y), color, text.c_str());
}

// Étiquette « L1 » / « C2 » : lettre du type + numéro d'élément (comme dans NETWORK).
std::string elementLabel(const Element& e, std::size_t index) {
    return std::string(isInductor(e.type) ? "L" : "C") + std::to_string(index + 1);
}

} // namespace

void SchematicView::draw(const Project& p) {
    panelHeader("SCHEMATIC");
    const float s = uiScale();
    const std::size_t n = p.network.size();

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float height = std::max(avail.y, kMinHeight * s);

    // --- Largeur nécessaire : source + éléments + charge --------------------------------------------
    const float sourceBlock = (kMargin + 2.0f * kSourceRadius + 30.0f) * s;
    const float loadBlock = (kLoadWidth + 30.0f + kMargin) * s;
    const float freeWidth = avail.x - sourceBlock - loadBlock;
    const float spacing = std::max(kMinSpacing * s, n > 0 ? freeWidth / static_cast<float>(n) : 0.0f);
    const float totalWidth = sourceBlock + loadBlock + spacing * static_cast<float>(n) +
                             (n == 0 ? std::max(freeWidth, 80.0f * s) : 0.0f);

    // Réserve la zone (déclenche la barre de défilement horizontale si besoin).
    ImGui::Dummy(ImVec2(std::max(totalWidth, avail.x), height - ImGui::GetStyle().ItemSpacing.y));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 wire = toU32(kColorSchematicWire);
    const ImU32 labelColor = toU32(kColorSchematicLabel);
    const ImU32 dimColor = toU32(kColorTextDim);

    // --- Géométrie : fil principal en haut, rail de masse en bas -------------------------------------------
    const float wireY = origin.y + height * 0.40f;
    const float railY = origin.y + height * 0.80f;
    const float xSource = origin.x + (kMargin + kSourceRadius) * s;
    const float xFirst = origin.x + sourceBlock;                       // début de la zone des éléments
    const float xLoad = origin.x + std::max(totalWidth, avail.x) - (kMargin + kLoadWidth * 0.5f) * s;

    // Rail de masse sur toute la longueur.
    line(dl, ImVec2(xSource, railY), ImVec2(xLoad, railY), wire, kWireThickness * s);
    drawGround(dl, ImVec2(xSource, railY), wire, s);

    // --- Source : cercle avec une sinusoïde + Zs -------------------------------------------------------------------
    const ImVec2 sourceCenter(xSource, (wireY + railY) * 0.5f);
    dl->AddCircle(sourceCenter, kSourceRadius * s, wire, 0, kPartThickness * s);
    {
        ImVec2 wave[17];
        for (int i = 0; i <= 16; ++i) {
            const float t = static_cast<float>(i) / 16.0f;
            wave[i] = ImVec2(sourceCenter.x + (t - 0.5f) * 1.5f * kSourceRadius * s,
                             sourceCenter.y - std::sin(t * 2.0f * kPiFloat) * 0.45f * kSourceRadius * s);
        }
        dl->AddPolyline(wave, 17, wire, kWireThickness * s, ImDrawFlags_None);
    }
    line(dl, ImVec2(xSource, wireY), ImVec2(xSource, sourceCenter.y - kSourceRadius * s), wire, kWireThickness * s);
    line(dl, ImVec2(xSource, sourceCenter.y + kSourceRadius * s), ImVec2(xSource, railY), wire, kWireThickness * s);
    dl->AddText(ImVec2(xSource + (kSourceRadius + 6.0f) * s, sourceCenter.y - ImGui::GetTextLineHeight() * 0.5f),
                labelColor, ("Zs\n" + formatComplex(p.zSource, 4) + " \xCE\xA9").c_str());

    // --- Fil principal de la source jusqu'à la charge -------------------------------------------------------------------
    line(dl, ImVec2(xSource, wireY), ImVec2(xLoad, wireY), wire, kWireThickness * s);

    // --- Charge : rectangle entre le fil et le rail --------------------------------------------------------------------------
    const ImVec2 loadTop(xLoad, (wireY + railY) * 0.5f - kLoadHeight * 0.5f * s);
    const ImVec2 loadBottom(xLoad, (wireY + railY) * 0.5f + kLoadHeight * 0.5f * s);
    line(dl, ImVec2(xLoad, wireY), loadTop, wire, kWireThickness * s);
    line(dl, loadBottom, ImVec2(xLoad, railY), wire, kWireThickness * s);
    dl->AddRect(ImVec2(xLoad - kLoadWidth * 0.5f * s, loadTop.y), ImVec2(xLoad + kLoadWidth * 0.5f * s, loadBottom.y),
                wire, 2.0f * s, kPartThickness * s, ImDrawFlags_None);
    {
        const std::string text = "ZL\n" + formatComplex(p.load.zAtF0, 4) + " \xCE\xA9";
        const ImVec2 size = ImGui::CalcTextSize(text.c_str());
        dl->AddText(ImVec2(xLoad - kLoadWidth * 0.5f * s - 8.0f * s - size.x, (loadTop.y + loadBottom.y) * 0.5f - size.y * 0.5f),
                    labelColor, text.c_str());
    }

    // --- Éléments : parcours inversé (indice n-1 près de la source → à gauche) -------------------------------------------
    hovered_ = -1;
    const bool windowHovered = ImGui::IsWindowHovered();
    const ImVec2 mouse = ImGui::GetMousePos();

    if (n == 0) {
        const char* text = "No element: the source drives the load directly.";
        centeredText(dl, (xSource + xLoad) * 0.5f, wireY - 26.0f * s, dimColor, text);
        return;
    }

    const bool withESeries = p.eSeries != ESeriesKind::None;
    for (std::size_t k = 0; k < n; ++k) {
        const std::size_t index = n - 1 - k;                 // k = position visuelle depuis la gauche
        const Element& e = p.network[index];
        const float cx = xFirst + spacing * (static_cast<float>(k) + 0.5f);
        const ImVec4& base = colorForElementType(e.type);

        // Zone de survol de l'élément (rectangle autour du composant et de son étiquette).
        const float halfW = std::min(spacing * 0.5f, 60.0f * s);
        const ImVec2 hitMin(cx - halfW, wireY - 34.0f * s);
        const ImVec2 hitMax(cx + halfW, isSeries(e.type) ? wireY + 30.0f * s : railY);
        const bool hovered = windowHovered && mouse.x >= hitMin.x - kHoverMargin && mouse.x <= hitMax.x + kHoverMargin &&
                             mouse.y >= hitMin.y && mouse.y <= hitMax.y;
        if (hovered) {
            hovered_ = static_cast<int>(index);
            dl->AddRectFilled(hitMin, hitMax, toU32(withAlpha(base, 0.10f)), 6.0f * s);
        }
        const ImU32 color = toU32(base);

        // Efface le fil derrière un composant série pour que le symbole ne soit pas barré.
        const float len = kComponentLength * s;
        std::string valueText = formatEng(e.value, valueUnit(e.type), 4);
        std::string eText = withESeries ? std::string(eSeriesName(p.eSeries)) + ": " +
                                              formatEng(nearestESeries(e.value, p.eSeries), valueUnit(e.type), 3)
                                        : std::string();
        if (isSeries(e.type)) {
            // Composant horizontal centré sur cx, posé sur le fil.
            Axes ax{ImVec2(cx - len * 0.5f, wireY), ImVec2(1.0f, 0.0f), ImVec2(0.0f, -1.0f)};
            dl->AddRectFilled(ImVec2(cx - len * 0.5f, wireY - 2.0f * s), ImVec2(cx + len * 0.5f, wireY + 2.0f * s),
                              toU32(kColorPanelBg));
            if (isInductor(e.type)) {
                drawInductor(dl, ax, len, color, s);
            } else {
                drawCapacitor(dl, ax, len, color, s);
            }
            const float labelY = wireY - 32.0f * s;
            centeredText(dl, cx, labelY, color, elementLabel(e, index));
            centeredText(dl, cx, wireY + 14.0f * s, labelColor, valueText);
            if (withESeries) {
                centeredText(dl, cx, wireY + 14.0f * s + ImGui::GetTextLineHeight(), dimColor, eText);
            }
        } else {
            // Composant vertical : de la jonction sur le fil vers le rail, au milieu de la hauteur.
            const float partLen = len;
            const float midY = (wireY + railY) * 0.5f;
            Axes ax{ImVec2(cx, midY - partLen * 0.5f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)};
            line(dl, ImVec2(cx, wireY), ImVec2(cx, midY - partLen * 0.5f), wire, kWireThickness * s);
            line(dl, ImVec2(cx, midY + partLen * 0.5f), ImVec2(cx, railY), wire, kWireThickness * s);
            dl->AddCircleFilled(ImVec2(cx, wireY), 3.0f * s, wire);          // point de jonction
            dl->AddCircleFilled(ImVec2(cx, railY), 3.0f * s, wire);
            if (isInductor(e.type)) {
                drawInductor(dl, ax, partLen, color, s);
            } else {
                drawCapacitor(dl, ax, partLen, color, s);
            }
            const float textX = cx + (kPlateHalfSize + 8.0f) * s;
            dl->AddText(ImVec2(textX, midY - ImGui::GetTextLineHeight()), color, elementLabel(e, index).c_str());
            dl->AddText(ImVec2(textX, midY), labelColor, valueText.c_str());
            if (withESeries) {
                dl->AddText(ImVec2(textX, midY + ImGui::GetTextLineHeight()), dimColor, eText.c_str());
            }
        }
    }
}

} // namespace smith::ui
