// =============================================================================
// ImageExport.cpp — Capture OpenGL → PNG (glReadPixels + stb_image_write)
// =============================================================================
#include "ui/platform/ImageExport.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <vector>

#include "core/FileIo.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace smith::ui::platform {

namespace {

constexpr int kChannels = 4;                               // RGBA
constexpr std::size_t kChannelsSize = static_cast<std::size_t>(kChannels);

// stb appelle cette fonction avec les octets du PNG au fur et à mesure : on les accumule.
void appendBytes(void* context, void* data, int size) {
    auto* out = static_cast<std::vector<unsigned char>*>(context);
    const auto* bytes = static_cast<const unsigned char*>(data);
    out->insert(out->end(), bytes, bytes + size);
}

} // namespace

CaptureResult saveFramebufferRegionPng(const std::string& pathUtf8, int x, int y, int width, int height,
                                       int framebufferHeight) {
    CaptureResult result;
    if (width <= 0 || height <= 0) {
        result.message = "Nothing to export (empty chart area).";
        return result;
    }

    // OpenGL a son origine EN BAS à gauche : on convertit le « y » (mesuré depuis le haut).
    const int glY = framebufferHeight - (y + height);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kChannelsSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(x, glY, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Retournement vertical : la première ligne lue est le BAS de l'image, le PNG attend le haut.
    const std::size_t rowBytes = static_cast<std::size_t>(width) * kChannelsSize;
    std::vector<unsigned char> row(rowBytes);
    for (int top = 0, bottom = height - 1; top < bottom; ++top, --bottom) {
        unsigned char* a = pixels.data() + static_cast<std::size_t>(top) * rowBytes;
        unsigned char* b = pixels.data() + static_cast<std::size_t>(bottom) * rowBytes;
        std::memcpy(row.data(), a, rowBytes);
        std::memcpy(a, b, rowBytes);
        std::memcpy(b, row.data(), rowBytes);
    }
    // Le fond de la fenêtre est opaque : on force l'opacité à 255 (certains pilotes renvoient alpha = 0).
    for (std::size_t i = kChannelsSize - 1; i < pixels.size(); i += kChannelsSize) {
        pixels[i] = 255;
    }

    // Encodage PNG en mémoire, puis écriture via FileIo (chemin UTF-8 correct sous Windows).
    std::vector<unsigned char> png;
    if (stbi_write_png_to_func(appendBytes, &png, width, height, kChannels, pixels.data(),
                               static_cast<int>(rowBytes)) == 0) {
        result.message = "PNG encoding failed.";
        return result;
    }
    const TextFileResult written = writeTextFile(pathUtf8, std::string(png.begin(), png.end()));
    if (!written.ok) {
        result.message = written.message;
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace smith::ui::platform
