// =============================================================================
// Theme.hpp — Couleurs nommées, thème sombre et polices de l'interface
// =============================================================================
// Rôle : TOUTES les couleurs de l'appli sont définies ici, avec un nom
// (ex. kColorSeriesL). Les vues ne contiennent jamais de couleur « en dur » :
// pour changer l'apparence, on ne modifie que ce fichier.
//
// Couleurs au format ImVec4 (rouge, vert, bleu, opacité), chaque composante
// entre 0 et 1. Les textes ont un contraste ≥ 4.5:1 sur le fond (norme WCAG AA).
// =============================================================================
#pragma once

#include <imgui.h>

namespace smith::ui {

// -----------------------------------------------------------------------------
// Petite aide : crée une couleur à partir d'un code hexadécimal 0xRRGGBB
// (comme dans un logiciel de dessin) et d'une opacité entre 0 et 1.
// -----------------------------------------------------------------------------
constexpr ImVec4 hexColor(unsigned int rgb, float alpha = 1.0f) {
    return ImVec4(static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f,
                  static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
                  static_cast<float>(rgb & 0xFFu) / 255.0f,
                  alpha);
}

// Version « avec une autre opacité » d'une couleur existante.
constexpr ImVec4 withAlpha(const ImVec4& color, float alpha) {
    return ImVec4(color.x, color.y, color.z, alpha);
}

// Conversion vers le format entier 32 bits utilisé par ImDrawList (dessin bas niveau).
inline ImU32 toU32(const ImVec4& color) { return ImGui::ColorConvertFloat4ToU32(color); }

// --- Fonds et texte de l'interface --------------------------------------------
inline constexpr ImVec4 kColorWindowBg   = hexColor(0x14161A);
inline constexpr ImVec4 kColorPanelBg    = hexColor(0x1A1D22);
inline constexpr ImVec4 kColorFrameBg    = hexColor(0x252A31);
inline constexpr ImVec4 kColorFrameHover = hexColor(0x2F353E);
inline constexpr ImVec4 kColorFrameActive= hexColor(0x3A414C);
inline constexpr ImVec4 kColorBorder     = hexColor(0x2C3138);
inline constexpr ImVec4 kColorText       = hexColor(0xE6E8EB);   // contraste ≈ 14:1
inline constexpr ImVec4 kColorTextDim    = hexColor(0x9AA4B0);   // contraste ≈ 7:1
inline constexpr ImVec4 kColorAccent     = hexColor(0x4C9AFF);
inline constexpr ImVec4 kColorAccentHover= hexColor(0x6AAEFF);
inline constexpr ImVec4 kColorHeader     = hexColor(0x8FC1FF);   // titres de panneaux

// --- Messages et indicateurs de qualité ---------------------------------------
inline constexpr ImVec4 kColorGood    = hexColor(0x3DD68C);   // VSWR ≤ 1.5
inline constexpr ImVec4 kColorWarning = hexColor(0xF5A524);   // VSWR ≤ 2
inline constexpr ImVec4 kColorBad     = hexColor(0xFF5C5C);   // VSWR > 2
inline constexpr ImVec4 kColorError   = hexColor(0xFF6B6B);   // saisie invalide, erreurs

// --- Abaque de Smith -----------------------------------------------------------
inline constexpr ImVec4 kColorChartBg            = hexColor(0x101215);
inline constexpr ImVec4 kColorChartUnitCircle    = hexColor(0xB8C0CC);
inline constexpr ImVec4 kColorChartImpedanceGrid = hexColor(0x5E6B7D, 0.85f);
inline constexpr ImVec4 kColorChartAdmittanceGrid= hexColor(0xB48A5A, 0.45f);
inline constexpr ImVec4 kColorChartLabel         = hexColor(0x9AA4B0);
inline constexpr ImVec4 kColorChartAdmLabel      = hexColor(0xC9A27A);

// Les 4 types d'éléments (trajets sur l'abaque, schéma, légende).
inline constexpr ImVec4 kColorSeriesL = hexColor(0xFF9F43);   // orange
inline constexpr ImVec4 kColorSeriesC = hexColor(0x4CC9F0);   // cyan
inline constexpr ImVec4 kColorShuntL  = hexColor(0xF368E0);   // magenta
inline constexpr ImVec4 kColorShuntC  = hexColor(0x7BED9F);   // vert clair

// Repères posés sur l'abaque.
inline constexpr ImVec4 kColorLoadMarker   = hexColor(0xFF6B6B);  // Z_L
inline constexpr ImVec4 kColorTargetMarker = hexColor(0xFFFFFF);  // Z_s*
inline constexpr ImVec4 kColorZinMarker    = hexColor(0xFFD93D);  // Z_in
inline constexpr ImVec4 kColorNodeMarker   = hexColor(0xD0D6DE);  // nœuds intermédiaires
inline constexpr ImVec4 kColorVswrCircle   = hexColor(0xFFD93D, 0.55f);
inline constexpr ImVec4 kColorSweepLocus   = hexColor(0xA78BFA);  // lieu de Z_in(f)
inline constexpr ImVec4 kColorPreview      = hexColor(0xFFFFFF, 0.75f); // aperçu solveur
inline constexpr ImVec4 kColorHoverCross   = hexColor(0xFFFFFF, 0.35f);

// Courbes du balayage.
inline constexpr ImVec4 kColorPlotS11      = hexColor(0x4C9AFF);
inline constexpr ImVec4 kColorPlotVswr     = hexColor(0xFF9F43);
inline constexpr ImVec4 kColorPlotS11ESer  = hexColor(0x4C9AFF, 0.55f);
inline constexpr ImVec4 kColorPlotVswrESer = hexColor(0xFF9F43, 0.55f);
inline constexpr ImVec4 kColorPlotF0       = hexColor(0xE6E8EB, 0.6f);
inline constexpr ImVec4 kColorPlotThreshold= hexColor(0xFF5C5C, 0.7f);
inline constexpr ImVec4 kColorPlotBand     = hexColor(0x3DD68C, 0.12f);

// Schéma électrique.
inline constexpr ImVec4 kColorSchematicWire  = hexColor(0xB8C0CC);
inline constexpr ImVec4 kColorSchematicLabel = hexColor(0xE6E8EB);

// --- Tailles de base (en « points » avant mise à l'échelle HiDPI) -----------
inline constexpr float kBaseFontSize  = 15.0f;
inline constexpr float kSmallFontSize = 13.0f;
inline constexpr float kTitleFontSize = 16.0f;

// Les polices chargées (une graisse normale, une demi-grasse pour les titres).
struct Fonts {
    ImFont* regular  = nullptr;
    ImFont* semibold = nullptr;
};

// Applique le thème sombre au style ImGui. « scale » = facteur HiDPI
// (1.0 sur un écran normal, 1.5 sous Windows à 150 %…).
void applyDarkTheme(float scale);

// Charge les polices Inter intégrées à l'exécutable (aucun fichier externe).
Fonts loadFonts();

} // namespace smith::ui
