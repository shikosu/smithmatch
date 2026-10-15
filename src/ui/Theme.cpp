// =============================================================================
// Theme.cpp — Mise en place du thème sombre et chargement des polices
// =============================================================================
#include "ui/Theme.hpp"

#include <cstddef>

// Données des polices, générées à la compilation par cmake/EmbedResource.cmake
// à partir de assets/fonts/*.ttf. Ce sont de simples tableaux d'octets.
namespace smith::assets {
extern const unsigned char kInterRegularTtf[];
extern const std::size_t   kInterRegularTtfSize;
extern const unsigned char kInterSemiBoldTtf[];
extern const std::size_t   kInterSemiBoldTtfSize;
} // namespace smith::assets

namespace smith::ui {

namespace {
Fonts g_fonts;   // polices chargées (une seule fois, au démarrage)
}

const Fonts& currentFonts() { return g_fonts; }

const ImVec4& colorForElementType(ElementType type) {
    switch (type) {
        case ElementType::SeriesInductor: return kColorSeriesL;
        case ElementType::SeriesCapacitor: return kColorSeriesC;
        case ElementType::ShuntInductor: return kColorShuntL;
        case ElementType::ShuntCapacitor: return kColorShuntC;
    }
    return kColorText;
}

// -----------------------------------------------------------------------------
// applyDarkTheme : part du thème sombre d'ImGui, puis ajuste couleurs,
// arrondis et espacements pour un rendu sobre et aéré.
// -----------------------------------------------------------------------------
void applyDarkTheme(float scale) {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // --- Formes : arrondis légers, bordures fines, espacements aérés --------
    style.WindowRounding    = 0.0f;   // panneaux fixes : pas d'arrondi extérieur
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2(10.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 3.0f);
    style.ItemSpacing       = ImVec2(8.0f, 5.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.CellPadding       = ImVec2(6.0f, 3.0f);
    style.ScrollbarSize     = 12.0f;
    style.GrabMinSize       = 10.0f;

    // --- Couleurs (toutes viennent des constantes nommées de Theme.hpp) -----
    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                 = kColorText;
    c[ImGuiCol_TextDisabled]         = kColorTextDim;
    c[ImGuiCol_WindowBg]             = kColorWindowBg;
    c[ImGuiCol_ChildBg]              = kColorPanelBg;
    c[ImGuiCol_PopupBg]              = kColorPanelBg;
    c[ImGuiCol_Border]               = kColorBorder;
    c[ImGuiCol_FrameBg]              = kColorFrameBg;
    c[ImGuiCol_FrameBgHovered]       = kColorFrameHover;
    c[ImGuiCol_FrameBgActive]        = kColorFrameActive;
    c[ImGuiCol_TitleBg]              = kColorPanelBg;
    c[ImGuiCol_TitleBgActive]        = kColorPanelBg;
    c[ImGuiCol_MenuBarBg]            = kColorPanelBg;
    c[ImGuiCol_ScrollbarBg]          = withAlpha(kColorWindowBg, 0.0f);
    c[ImGuiCol_ScrollbarGrab]        = kColorFrameHover;
    c[ImGuiCol_ScrollbarGrabHovered] = kColorFrameActive;
    c[ImGuiCol_ScrollbarGrabActive]  = kColorAccent;
    c[ImGuiCol_CheckMark]            = kColorAccent;
    c[ImGuiCol_SliderGrab]           = kColorAccent;
    c[ImGuiCol_SliderGrabActive]     = kColorAccentHover;
    c[ImGuiCol_Button]               = kColorFrameBg;
    c[ImGuiCol_ButtonHovered]        = kColorFrameHover;
    c[ImGuiCol_ButtonActive]         = kColorFrameActive;
    c[ImGuiCol_Header]               = withAlpha(kColorAccent, 0.25f);
    c[ImGuiCol_HeaderHovered]        = withAlpha(kColorAccent, 0.35f);
    c[ImGuiCol_HeaderActive]         = withAlpha(kColorAccent, 0.50f);
    c[ImGuiCol_Separator]            = kColorBorder;
    c[ImGuiCol_SeparatorHovered]     = kColorAccent;
    c[ImGuiCol_SeparatorActive]      = kColorAccentHover;
    c[ImGuiCol_ResizeGrip]           = withAlpha(kColorAccent, 0.0f);
    c[ImGuiCol_Tab]                  = kColorFrameBg;
    c[ImGuiCol_TabHovered]           = kColorFrameHover;
    c[ImGuiCol_TabSelected]          = withAlpha(kColorAccent, 0.35f);
    c[ImGuiCol_TableHeaderBg]        = kColorFrameBg;
    c[ImGuiCol_TableBorderStrong]    = kColorBorder;
    c[ImGuiCol_TableBorderLight]     = kColorBorder;
    c[ImGuiCol_TableRowBgAlt]        = withAlpha(kColorText, 0.03f);
    c[ImGuiCol_TextSelectedBg]       = withAlpha(kColorAccent, 0.35f);
    c[ImGuiCol_ModalWindowDimBg]     = withAlpha(kColorWindowBg, 0.6f);

    // --- HiDPI : on agrandit tous les espacements et les polices -------------
    // Sur macOS « scale » vaut 1 : le système gère déjà les points Retina.
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;
}

// -----------------------------------------------------------------------------
// loadFonts : charge Inter Regular (police par défaut) et Inter SemiBold.
// Depuis ImGui 1.92, les glyphes sont créés à la demande : Ω, µ, Γ, ∞… sont
// donc disponibles sans déclarer de plages de caractères.
// -----------------------------------------------------------------------------
Fonts loadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // Les données restent dans l'exécutable : ImGui ne doit PAS les libérer.
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;

    // ImGui attend un void* (non const) même s'il ne modifie pas les données.
    void* regularData  = const_cast<unsigned char*>(assets::kInterRegularTtf);
    void* semiboldData = const_cast<unsigned char*>(assets::kInterSemiBoldTtf);

    Fonts fonts;
    fonts.regular  = io.Fonts->AddFontFromMemoryTTF(
        regularData, static_cast<int>(assets::kInterRegularTtfSize), kBaseFontSize, &config);
    fonts.semibold = io.Fonts->AddFontFromMemoryTTF(
        semiboldData, static_cast<int>(assets::kInterSemiBoldTtfSize), kBaseFontSize, &config);
    g_fonts = fonts;
    return fonts;
}

} // namespace smith::ui
