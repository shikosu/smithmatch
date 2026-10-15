// =============================================================================
// FileIo.cpp — Implémentation de la lecture / écriture de fichiers texte
// =============================================================================
#include "core/FileIo.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

namespace smith {

namespace {

// Chemin UTF-8 → std::filesystem::path (u8path gère correctement Windows et macOS).
std::filesystem::path toPath(const std::string& utf8) {
    return std::filesystem::u8path(utf8);
}

TextFileResult failure(const std::string& message) {
    TextFileResult r;
    r.ok = false;
    r.message = message;
    return r;
}

} // namespace

TextFileResult readTextFile(const std::string& pathUtf8) {
    std::ifstream file(toPath(pathUtf8), std::ios::binary);
    if (!file) {
        return failure("Cannot open file for reading: " + pathUtf8);
    }
    TextFileResult r;
    r.text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return failure("Error while reading file: " + pathUtf8);
    }
    r.ok = true;
    return r;
}

TextFileResult writeTextFile(const std::string& pathUtf8, const std::string& text) {
    const std::filesystem::path target = toPath(pathUtf8);
    std::filesystem::path temp = target;
    temp += ".tmp";

    {
        std::ofstream file(temp, std::ios::binary | std::ios::trunc);
        if (!file) {
            return failure("Cannot open file for writing: " + pathUtf8);
        }
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.flush();
        if (!file) {
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            return failure("Error while writing file: " + pathUtf8);
        }
    }

    std::error_code error;
    std::filesystem::rename(temp, target, error);     // remplace l'ancien fichier
    if (error) {
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        return failure("Cannot save file: " + error.message());
    }
    TextFileResult r;
    r.ok = true;
    return r;
}

} // namespace smith
