// =============================================================================
// Project.cpp — Projet par défaut, petits calculs sur le projet, fichier JSON
// =============================================================================
#include "core/Project.hpp"

#include <nlohmann/json.hpp>

#include <cmath>

#include "core/Network.hpp"
#include "core/Validation.hpp"

namespace smith {

Project defaultProject() {
    Project project;
    project.sweep.fMinHz = project.f0Hz / 2.0;
    project.sweep.fMaxHz = project.f0Hz * 2.0;
    return project;
}

Complex projectLoadAt(const Project& project, double fHz) {
    return loadImpedanceAt(project.load, project.f0Hz, fHz);
}

Complex projectInputImpedanceAt(const Project& project, double fHz) {
    return inputImpedance(project.network, projectLoadAt(project, fHz), fHz);
}

Complex projectInputImpedanceAt(const Project& project, const ElementList& network, double fHz) {
    return inputImpedance(network, projectLoadAt(project, fHz), fHz);
}

// -----------------------------------------------------------------------------
// Fichier projet JSON
// -----------------------------------------------------------------------------
namespace {

using OrderedJson = nlohmann::ordered_json;   // garde l'ordre des clés (fichier lisible)

// Les clés du format (une seule fois, pour éviter les fautes de frappe).
constexpr const char* kKeyFormat = "format";
constexpr const char* kKeyVersion = "version";
constexpr const char* kKeyZ0 = "z0_ohm";
constexpr const char* kKeyF0 = "f0_hz";
constexpr const char* kKeySource = "source";
constexpr const char* kKeyLoad = "load";
constexpr const char* kKeyR = "r_ohm";
constexpr const char* kKeyX = "x_ohm";
constexpr const char* kKeyModel = "model";
constexpr const char* kKeyNetwork = "network";
constexpr const char* kKeyType = "type";
constexpr const char* kKeyValue = "value";
constexpr const char* kKeySweep = "sweep";
constexpr const char* kKeyFMin = "f_min_hz";
constexpr const char* kKeyFMax = "f_max_hz";
constexpr const char* kKeyPoints = "points";
constexpr const char* kKeyLog = "log_scale";
constexpr const char* kKeyVswr = "vswr_threshold";
constexpr const char* kKeyESeries = "e_series";

constexpr const char* kErrorPrefix = "Cannot open project: ";

ProjectLoadResult failure(const std::string& reason) {
    ProjectLoadResult result;
    result.ok = false;
    result.message = kErrorPrefix + reason;
    return result;
}

// Lit un nombre optionnel : champ absent → « out » inchangé (true) ; champ présent mais
// qui n'est pas un nombre fini → false et « error » rempli.
bool readNumber(const OrderedJson& object, const char* key, double& out, std::string& error) {
    const auto it = object.find(key);
    if (it == object.end()) {
        return true;
    }
    if (!it->is_number()) {
        error = std::string("\"") + key + "\" must be a number.";
        return false;
    }
    const double value = it->get<double>();
    if (!std::isfinite(value)) {
        error = std::string("\"") + key + "\" must be a finite number.";
        return false;
    }
    out = value;
    return true;
}

// Idem pour une chaîne de caractères.
bool readString(const OrderedJson& object, const char* key, std::string& out, bool& present, std::string& error) {
    const auto it = object.find(key);
    present = it != object.end();
    if (!present) {
        return true;
    }
    if (!it->is_string()) {
        error = std::string("\"") + key + "\" must be a text value.";
        return false;
    }
    out = it->get<std::string>();
    return true;
}

// Sous-objet optionnel (« source », « load », « sweep »). Renvoie nullptr s'il est absent.
bool readObject(const OrderedJson& parent, const char* key, const OrderedJson*& out, std::string& error) {
    out = nullptr;
    const auto it = parent.find(key);
    if (it == parent.end()) {
        return true;
    }
    if (!it->is_object()) {
        error = std::string("\"") + key + "\" must be an object.";
        return false;
    }
    out = &*it;
    return true;
}

} // namespace

std::string toJson(const Project& p) {
    OrderedJson j;
    j[kKeyFormat] = kProjectFormatName;
    j[kKeyVersion] = kProjectFileVersion;
    j[kKeyZ0] = p.z0;
    j[kKeyF0] = p.f0Hz;
    j[kKeySource] = {{kKeyR, p.zSource.real()}, {kKeyX, p.zSource.imag()}};
    j[kKeyLoad] = {{kKeyR, p.load.zAtF0.real()}, {kKeyX, p.load.zAtF0.imag()}, {kKeyModel, loadModelJsonName(p.load.model)}};
    OrderedJson network = OrderedJson::array();
    for (const Element& e : p.network) {
        network.push_back({{kKeyType, typeJsonName(e.type)}, {kKeyValue, e.value}});
    }
    j[kKeyNetwork] = network;
    j[kKeySweep] = {{kKeyFMin, p.sweep.fMinHz}, {kKeyFMax, p.sweep.fMaxHz}, {kKeyPoints, p.sweep.points},
                    {kKeyLog, p.sweep.logScale}, {kKeyVswr, p.sweep.vswrThreshold}};
    j[kKeyESeries] = eSeriesJsonName(p.eSeries);
    return j.dump(2) + "\n";
}

ProjectLoadResult fromJson(const std::string& text) {
    // parse(..., allow_exceptions = false) : en cas d'erreur, renvoie une valeur « discarded ».
    const OrderedJson j = OrderedJson::parse(text, nullptr, false);
    if (j.is_discarded()) {
        return failure("the file is not valid JSON.");
    }
    if (!j.is_object()) {
        return failure("the top level must be a JSON object.");
    }

    // --- Identification et version ----------------------------------------------------------------------
    const auto formatIt = j.find(kKeyFormat);
    if (formatIt == j.end() || !formatIt->is_string() || formatIt->get<std::string>() != kProjectFormatName) {
        return failure("this is not a SmithMatch project (missing or wrong \"format\").");
    }
    const auto versionIt = j.find(kKeyVersion);
    if (versionIt == j.end() || !versionIt->is_number_integer()) {
        return failure("missing or invalid \"version\".");
    }
    const long long version = versionIt->get<long long>();
    if (version > kProjectFileVersion) {
        return failure("this file was saved by a newer version of SmithMatch (file version " +
                       std::to_string(version) + ", this program reads up to version " +
                       std::to_string(kProjectFileVersion) + "). Please update SmithMatch.");
    }
    if (version < 1) {
        return failure("unsupported file version " + std::to_string(version) + ".");
    }
    // (Version 1 = version actuelle. Une future version 2 ajoutera ici la lecture « à l'ancienne » de la 1.)

    Project p = defaultProject();
    std::string error;

    // --- Z0, f0 ------------------------------------------------------------------------------------------------
    if (!readNumber(j, kKeyZ0, p.z0, error) || !readNumber(j, kKeyF0, p.f0Hz, error)) {
        return failure(error);
    }
    p.sweep.fMinHz = p.f0Hz / 2.0;      // valeurs par défaut du balayage si « sweep » est absent
    p.sweep.fMaxHz = p.f0Hz * 2.0;

    // --- Source et charge -----------------------------------------------------------------------------------
    const OrderedJson* sub = nullptr;
    if (!readObject(j, kKeySource, sub, error)) {
        return failure(error);
    }
    if (sub != nullptr) {
        double r = p.zSource.real();
        double x = p.zSource.imag();
        if (!readNumber(*sub, kKeyR, r, error) || !readNumber(*sub, kKeyX, x, error)) {
            return failure(error);
        }
        p.zSource = Complex(r, x);
    }
    if (!readObject(j, kKeyLoad, sub, error)) {
        return failure(error);
    }
    if (sub != nullptr) {
        double r = p.load.zAtF0.real();
        double x = p.load.zAtF0.imag();
        if (!readNumber(*sub, kKeyR, r, error) || !readNumber(*sub, kKeyX, x, error)) {
            return failure(error);
        }
        p.load.zAtF0 = Complex(r, x);
        std::string modelName;
        bool present = false;
        if (!readString(*sub, kKeyModel, modelName, present, error)) {
            return failure(error);
        }
        if (present) {
            const std::optional<LoadModel> model = loadModelFromJsonName(modelName);
            if (!model.has_value()) {
                return failure("unknown load model \"" + modelName + "\".");
            }
            p.load.model = *model;
        }
    }

    // --- Réseau ------------------------------------------------------------------------------------------------------
    p.network.clear();
    const auto networkIt = j.find(kKeyNetwork);
    if (networkIt != j.end()) {
        if (!networkIt->is_array()) {
            return failure("\"network\" must be a list.");
        }
        std::size_t index = 0;
        for (const OrderedJson& item : *networkIt) {
            ++index;
            if (!item.is_object()) {
                return failure("network element " + std::to_string(index) + " must be an object.");
            }
            std::string typeName;
            bool present = false;
            if (!readString(item, kKeyType, typeName, present, error)) {
                return failure(error);
            }
            const std::optional<ElementType> type = present ? typeFromJsonName(typeName) : std::nullopt;
            if (!type.has_value()) {
                return failure("network element " + std::to_string(index) + " has a missing or unknown \"type\".");
            }
            const auto valueIt = item.find(kKeyValue);
            if (valueIt == item.end() || !valueIt->is_number()) {
                return failure("network element " + std::to_string(index) + " has a missing or invalid \"value\".");
            }
            p.network.push_back(makeElement(*type, valueIt->get<double>()));
        }
    }

    // --- Balayage ------------------------------------------------------------------------------------------------------
    if (!readObject(j, kKeySweep, sub, error)) {
        return failure(error);
    }
    if (sub != nullptr) {
        double points = static_cast<double>(p.sweep.points);
        if (!readNumber(*sub, kKeyFMin, p.sweep.fMinHz, error) || !readNumber(*sub, kKeyFMax, p.sweep.fMaxHz, error) ||
            !readNumber(*sub, kKeyPoints, points, error) || !readNumber(*sub, kKeyVswr, p.sweep.vswrThreshold, error)) {
            return failure(error);
        }
        p.sweep.points = static_cast<int>(std::lround(points));
        const auto logIt = sub->find(kKeyLog);
        if (logIt != sub->end()) {
            if (!logIt->is_boolean()) {
                return failure("\"log_scale\" must be true or false.");
            }
            p.sweep.logScale = logIt->get<bool>();
        }
    }

    // --- Série E -----------------------------------------------------------------------------------------------------------
    {
        std::string name;
        bool present = false;
        if (!readString(j, kKeyESeries, name, present, error)) {
            return failure(error);
        }
        if (present) {
            const std::optional<ESeriesKind> kind = eSeriesFromJsonName(name);
            if (!kind.has_value()) {
                return failure("unknown E-series \"" + name + "\".");
            }
            p.eSeries = *kind;
        }
    }

    // --- Cohérence physique (mêmes règles que la saisie dans l'interface) -------------------------------------------
    ValidationResult check = validateZ0(p.z0);
    if (check.ok) {
        check = validateFrequency(p.f0Hz);
    }
    if (check.ok) {
        check = validateSourceImpedance(p.zSource);
    }
    if (check.ok) {
        check = validateLoadImpedance(p.load.zAtF0);
    }
    for (std::size_t i = 0; check.ok && i < p.network.size(); ++i) {
        check = validateComponentValue(p.network[i].value);
    }
    if (check.ok) {
        check = validateFrequency(p.sweep.fMinHz);
    }
    if (check.ok) {
        check = validateFrequency(p.sweep.fMaxHz);
    }
    if (!check.ok) {
        return failure(check.message);
    }
    if (p.sweep.points < kMinSweepPoints || p.sweep.points > kMaxSweepPoints) {
        return failure("\"points\" must be between " + std::to_string(kMinSweepPoints) + " and " +
                       std::to_string(kMaxSweepPoints) + ".");
    }
    if (!(p.sweep.vswrThreshold > 1.0)) {
        return failure("\"vswr_threshold\" must be greater than 1.");
    }

    ProjectLoadResult result;
    result.ok = true;
    result.project = p;
    return result;
}

} // namespace smith
