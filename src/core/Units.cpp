// =============================================================================
// Units.cpp — Implémentation de la notation ingénieur
// =============================================================================
#include "core/Units.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <locale>
#include <sstream>

namespace smith {

namespace {

// Le micro « µ » en UTF-8 existe sous deux codes Unicode proches :
// U+00B5 (signe micro, celui d'un clavier français) et U+03BC (lettre grecque mu).
constexpr const char* kMicroSign = "\xC2\xB5";    // µ (U+00B5), utilisé à l'affichage
constexpr const char* kGreekMu   = "\xCE\xBC";    // μ (U+03BC), accepté à la saisie
constexpr const char* kOmegaSign = "\xCE\xA9";    // Ω (U+03A9)
constexpr const char* kOhmSign   = "\xE2\x84\xA6";// Ω (U+2126, symbole ohm)

// Préfixes utilisés à l'AFFICHAGE, du plus petit au plus grand.
constexpr int kMinPrefixExp = -15;
constexpr int kMaxPrefixExp = 12;
const char* displayPrefix(int exponent) {
    switch (exponent) {
        case -15: return "f";
        case -12: return "p";
        case -9:  return "n";
        case -6:  return kMicroSign;
        case -3:  return "m";
        case 0:   return "";
        case 3:   return "k";
        case 6:   return "M";
        case 9:   return "G";
        case 12:  return "T";
        default:  return "";
    }
}

// Supprime les zéros inutiles après la virgule : "50.00" → "50", "2.500" → "2.5".
std::string stripTrailingZeros(std::string text) {
    if (text.find('.') == std::string::npos) {
        return text;
    }
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

// Écrit |value| avec « sigDigits » chiffres significatifs (sans signe).
std::string formatPositive(double magnitude, int sigDigits) {
    // Nombre de chiffres avant la virgule = floor(log10(x)) + 1.
    // Décimales à afficher = chiffres significatifs − chiffres avant la virgule.
    const int digitsBefore = static_cast<int>(std::floor(std::log10(magnitude))) + 1;
    int decimals = sigDigits - digitsBefore;
    if (decimals < 0) {
        decimals = 0;
    }
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.*f", decimals, magnitude);
    return stripTrailingZeros(buffer);
}

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

// Supprime les espaces au début et à la fin.
std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

// Vrai si « text » est exactement une unité connue (qu'on ignore).
bool isKnownUnit(const std::string& text) {
    return text.empty() || text == "Hz" || text == "H" || text == "F" || text == "S" ||
           text == "ohm" || text == "Ohm" || text == "ohms" || text == kOmegaSign ||
           text == kOhmSign;
}

// Reconnaît un préfixe au début de « text ». Renvoie le nombre d'octets du
// préfixe (0 si aucun) et son exposant dans « exponent ».
std::size_t matchPrefix(const std::string& text, int& exponent) {
    if (startsWith(text, kMicroSign)) { exponent = -6; return 2; }
    if (startsWith(text, kGreekMu))   { exponent = -6; return 2; }
    if (text.empty()) { return 0; }
    switch (text[0]) {
        case 'f': exponent = -15; return 1;
        case 'p': exponent = -12; return 1;
        case 'n': exponent = -9;  return 1;
        case 'u': exponent = -6;  return 1;
        case 'm': exponent = -3;  return 1;
        case 'k':
        case 'K': exponent = 3;   return 1;
        case 'M': exponent = 6;   return 1;
        case 'G': exponent = 9;   return 1;
        case 'T': exponent = 12;  return 1;
        default:  return 0;
    }
}

} // namespace

std::string formatSig(double value, int sigDigits) {
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value < 0.0 ? "-\xE2\x88\x9E" : "\xE2\x88\x9E";   // ∞
    }
    if (value == 0.0) {
        return "0";
    }
    const std::string body = formatPositive(std::fabs(value), sigDigits);
    return (value < 0.0 ? "-" : "") + body;
}

std::string formatEng(double value, const std::string& unit, int sigDigits) {
    const std::string suffix = unit.empty() ? std::string() : (" " + unit);
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return std::string(value < 0.0 ? "-" : "") + "\xE2\x88\x9E" + suffix;   // ∞
    }
    if (value == 0.0) {
        return "0" + suffix;
    }

    const double magnitude = std::fabs(value);
    // Exposant « ingénieur » : le plus grand multiple de 3 ≤ log10(valeur).
    int exponent = 3 * static_cast<int>(std::floor(std::floor(std::log10(magnitude)) / 3.0));
    if (exponent < kMinPrefixExp) { exponent = kMinPrefixExp; }
    if (exponent > kMaxPrefixExp) { exponent = kMaxPrefixExp; }

    double scaled = magnitude / std::pow(10.0, exponent);
    std::string body = formatPositive(scaled, sigDigits);

    // L'arrondi peut donner « 1000 » (ex. 999.97 avec 4 chiffres) : on passe au préfixe suivant.
    if (std::stod(body) >= 1000.0 && exponent < kMaxPrefixExp) {
        exponent += 3;
        scaled = magnitude / std::pow(10.0, exponent);
        body = formatPositive(scaled, sigDigits);
    }
    // Espace entre le nombre et « préfixe + unité » (pas d'espace final s'il n'y a ni l'un ni l'autre).
    const std::string suffixText = std::string(displayPrefix(exponent)) + unit;
    return std::string(value < 0.0 ? "-" : "") + body + (suffixText.empty() ? "" : " ") + suffixText;
}

std::optional<double> parseEng(const std::string& rawText) {
    // Virgule décimale française → point.
    std::string text = trim(rawText);
    for (char& c : text) {
        if (c == ',') {
            c = '.';
        }
    }
    if (text.empty()) {
        return std::nullopt;
    }

    // --- 1. Isoler le nombre : [signe] chiffres [. chiffres] [e [signe] chiffres] ---
    std::size_t pos = 0;
    if (text[pos] == '+' || text[pos] == '-') {
        ++pos;
    }
    std::size_t digitCount = 0;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0) {
        ++pos;
        ++digitCount;
    }
    if (pos < text.size() && text[pos] == '.') {
        ++pos;
        while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0) {
            ++pos;
            ++digitCount;
        }
    }
    if (digitCount == 0) {
        return std::nullopt;   // pas de chiffre : "abc", "nan", "inf", "-", ".", …
    }
    // Exposant « e-9 » : consommé seulement s'il est suivi d'au moins un chiffre
    // (sinon le « e » n'est pas un exposant et la fin sera rejetée plus bas).
    if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E')) {
        std::size_t lookahead = pos + 1;
        if (lookahead < text.size() && (text[lookahead] == '+' || text[lookahead] == '-')) {
            ++lookahead;
        }
        if (lookahead < text.size() && std::isdigit(static_cast<unsigned char>(text[lookahead])) != 0) {
            while (lookahead < text.size() &&
                   std::isdigit(static_cast<unsigned char>(text[lookahead])) != 0) {
                ++lookahead;
            }
            pos = lookahead;
        }
    }

    // Conversion texte → double, indépendante de la langue du système
    // (locale « classic » : toujours le point comme séparateur décimal).
    std::istringstream stream(text.substr(0, pos));
    stream.imbue(std::locale::classic());
    double number = 0.0;
    stream >> number;
    if (stream.fail()) {
        return std::nullopt;
    }

    // --- 2. Le reste : préfixe éventuel + unité éventuelle ------------------------
    const std::string rest = trim(text.substr(pos));
    double multiplier = 1.0;
    if (!isKnownUnit(rest)) {
        // Pas une unité seule : ça doit être « préfixe » ou « préfixe + unité ».
        int exponent = 0;
        const std::size_t prefixBytes = matchPrefix(rest, exponent);
        if (prefixBytes == 0 || !isKnownUnit(trim(rest.substr(prefixBytes)))) {
            return std::nullopt;
        }
        multiplier = std::pow(10.0, exponent);
    }

    const double result = number * multiplier;
    if (!std::isfinite(result)) {
        return std::nullopt;
    }
    return result;
}

} // namespace smith
