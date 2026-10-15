// =============================================================================
// FileDialogs.cpp — Implémentation avec nativefiledialog-extended (NFD)
// =============================================================================
#include "ui/platform/FileDialogs.hpp"

#include <nfd.h>

namespace smith::ui::platform {

namespace {

// Libère le chemin renvoyé par NFD après l'avoir copié dans une std::string.
std::string takeAndFree(nfdu8char_t* path) {
    std::string copy(path);
    NFD_FreePathU8(path);
    return copy;
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

std::optional<std::string> openFileDialog(const std::string& filterName, const std::string& extensions) {
    const nfdu8filteritem_t filter{filterName.c_str(), extensions.c_str()};
    nfdu8char_t* path = nullptr;
    if (NFD_OpenDialogU8(&path, &filter, 1, nullptr) != NFD_OKAY) {
        return std::nullopt;       // annulé ou erreur
    }
    return takeAndFree(path);
}

std::optional<std::string> saveFileDialog(const std::string& filterName, const std::string& extensions,
                                          const std::string& defaultName, const std::string& requiredSuffix) {
    const nfdu8filteritem_t filter{filterName.c_str(), extensions.c_str()};
    nfdu8char_t* path = nullptr;
    if (NFD_SaveDialogU8(&path, &filter, 1, nullptr, defaultName.c_str()) != NFD_OKAY) {
        return std::nullopt;
    }
    std::string result = takeAndFree(path);
    if (!requiredSuffix.empty() && !endsWith(result, requiredSuffix)) {
        // « mon projet » → « mon projet.smith.json » ; « x.json » → « x.json.smith.json » serait laid :
        // on retire donc d'abord la dernière extension si elle fait partie du suffixe voulu.
        const std::size_t dot = requiredSuffix.rfind('.');
        const std::string lastExt = dot == std::string::npos ? std::string() : requiredSuffix.substr(dot);
        if (!lastExt.empty() && endsWith(result, lastExt)) {
            result.erase(result.size() - lastExt.size());
        }
        result += requiredSuffix;
    }
    return result;
}

const char* shortcutModifierName() {
#ifdef __APPLE__
    return "Cmd+";
#else
    return "Ctrl+";
#endif
}

} // namespace smith::ui::platform
