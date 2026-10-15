// =============================================================================
// test_project.cpp — Tests du fichier projet JSON (spec §6.9)
// =============================================================================
#include <doctest/doctest.h>

#include <string>

#include "core/Project.hpp"

using namespace smith;

namespace {

Project sampleProject() {
    Project p = defaultProject();
    p.z0 = 75.0;
    p.f0Hz = 868e6;
    p.zSource = Complex(30.0, 20.0);
    p.load.zAtF0 = Complex(80.0, -40.0);
    p.load.model = LoadModel::ParallelEquivalent;
    p.network = {makeElement(ElementType::ShuntCapacitor, 1.2345678901234e-12),
                 makeElement(ElementType::SeriesInductor, 38.98e-9),
                 makeElement(ElementType::ShuntInductor, 1.0 / 3.0 * 1e-8)};
    p.sweep.fMinHz = 400e6;
    p.sweep.fMaxHz = 1.6e9;
    p.sweep.points = 201;
    p.sweep.logScale = true;
    p.sweep.vswrThreshold = 1.5;
    p.eSeries = ESeriesKind::E24;
    return p;
}

} // namespace

TEST_CASE("Project JSON round trip is exact") {
    const Project original = sampleProject();
    const ProjectLoadResult loaded = fromJson(toJson(original));
    REQUIRE(loaded.ok);
    const Project& q = loaded.project;
    CHECK(q.z0 == original.z0);
    CHECK(q.f0Hz == original.f0Hz);
    CHECK(q.zSource == original.zSource);
    CHECK(q.load.zAtF0 == original.load.zAtF0);
    CHECK(q.load.model == original.load.model);
    REQUIRE(q.network.size() == original.network.size());
    for (std::size_t i = 0; i < q.network.size(); ++i) {
        CHECK(q.network[i].type == original.network[i].type);
        CHECK(q.network[i].value == original.network[i].value);   // aller-retour sans perte
    }
    CHECK(q.sweep.fMinHz == original.sweep.fMinHz);
    CHECK(q.sweep.fMaxHz == original.sweep.fMaxHz);
    CHECK(q.sweep.points == original.sweep.points);
    CHECK(q.sweep.logScale == original.sweep.logScale);
    CHECK(q.sweep.vswrThreshold == original.sweep.vswrThreshold);
    CHECK(q.eSeries == original.eSeries);
    // Et le texte re-sérialisé est identique.
    CHECK(toJson(q) == toJson(original));
}

TEST_CASE("Default project round trip with an empty network") {
    const Project original = defaultProject();
    const ProjectLoadResult loaded = fromJson(toJson(original));
    REQUIRE(loaded.ok);
    CHECK(loaded.project.network.empty());
    CHECK(loaded.project.load.zAtF0 == original.load.zAtF0);
}

TEST_CASE("Specification example file is accepted") {
    const std::string text = R"({
      "format": "smithmatch-project", "version": 1, "z0_ohm": 50, "f0_hz": 2.4e9,
      "source": { "r_ohm": 50, "x_ohm": 0 },
      "load": { "r_ohm": 25, "x_ohm": -15, "model": "series_equivalent" },
      "network": [ { "type": "series_inductor", "value": 2.6526e-9 } ],
      "sweep": { "f_min_hz": 1.2e9, "f_max_hz": 4.8e9, "points": 401, "log_scale": false, "vswr_threshold": 2.0 },
      "e_series": "E12" })";
    const ProjectLoadResult r = fromJson(text);
    REQUIRE(r.ok);
    CHECK(r.project.network.size() == 1);
    CHECK(r.project.network[0].type == ElementType::SeriesInductor);
    CHECK(r.project.eSeries == ESeriesKind::E12);
}

TEST_CASE("Unknown fields are ignored") {
    std::string text = toJson(sampleProject());
    // On ajoute des champs inconnus à plusieurs niveaux.
    text.replace(text.find('{'), 1, R"({ "future_feature": [1, 2, 3], "author": "someone",)");
    const ProjectLoadResult r = fromJson(text);
    CHECK(r.ok);
    const std::string nested = R"({"format":"smithmatch-project","version":1,
        "load":{"r_ohm":40,"x_ohm":5,"model":"constant","extra":true},"colour":"red"})";
    const ProjectLoadResult r2 = fromJson(nested);
    REQUIRE(r2.ok);
    CHECK(r2.project.load.zAtF0 == Complex(40.0, 5.0));
}

TEST_CASE("Missing known fields take their default values") {
    const ProjectLoadResult r = fromJson(R"({"format":"smithmatch-project","version":1})");
    REQUIRE(r.ok);
    const Project d = defaultProject();
    CHECK(r.project.z0 == d.z0);
    CHECK(r.project.f0Hz == d.f0Hz);
    CHECK(r.project.sweep.fMinHz == doctest::Approx(d.f0Hz / 2.0));
}

TEST_CASE("A newer file version is refused with a clear message") {
    const ProjectLoadResult r = fromJson(R"({"format":"smithmatch-project","version":99})");
    CHECK_FALSE(r.ok);
    CHECK(r.message.find("newer version") != std::string::npos);
    CHECK(r.message.find("99") != std::string::npos);
}

TEST_CASE("Malformed or foreign files are refused without crashing") {
    CHECK_FALSE(fromJson("").ok);
    CHECK_FALSE(fromJson("{ not json").ok);
    CHECK_FALSE(fromJson("[1,2,3]").ok);
    CHECK_FALSE(fromJson("null").ok);
    CHECK_FALSE(fromJson(R"({"hello":"world"})").ok);
    CHECK_FALSE(fromJson(R"({"format":"other","version":1})").ok);
    CHECK_FALSE(fromJson(R"({"format":"smithmatch-project"})").ok);
    CHECK_FALSE(fromJson(R"({"format":"smithmatch-project","version":"1"})").ok);
    CHECK_FALSE(fromJson(R"({"format":"smithmatch-project","version":0})").ok);
    CHECK_FALSE(fromJson(std::string("\x00\x01\x02", 3)).ok);
}

TEST_CASE("Invalid values are refused with a message") {
    const std::string head = R"({"format":"smithmatch-project","version":1,)";
    CHECK_FALSE(fromJson(head + R"("z0_ohm":-5})").ok);
    CHECK_FALSE(fromJson(head + R"("z0_ohm":"fifty"})").ok);
    CHECK_FALSE(fromJson(head + R"("z0_ohm":null})").ok);
    CHECK_FALSE(fromJson(head + R"("f0_hz":0})").ok);
    CHECK_FALSE(fromJson(head + R"("load":{"r_ohm":-10,"x_ohm":0}})").ok);
    CHECK_FALSE(fromJson(head + R"("load":{"r_ohm":10,"x_ohm":0,"model":"magic"}})").ok);
    CHECK_FALSE(fromJson(head + R"("load":[1]})").ok);
    CHECK_FALSE(fromJson(head + R"("network":[{"type":"series_inductor","value":-1e-9}]})").ok);
    CHECK_FALSE(fromJson(head + R"("network":[{"type":"flux_capacitor","value":1e-9}]})").ok);
    CHECK_FALSE(fromJson(head + R"("network":[{"type":"series_inductor"}]})").ok);
    CHECK_FALSE(fromJson(head + R"("network":"none"})").ok);
    CHECK_FALSE(fromJson(head + R"("sweep":{"points":3}})").ok);
    CHECK_FALSE(fromJson(head + R"("sweep":{"vswr_threshold":0.5}})").ok);
    CHECK_FALSE(fromJson(head + R"("sweep":{"log_scale":"yes"}})").ok);
    CHECK_FALSE(fromJson(head + R"("e_series":"E96"})").ok);
    const ProjectLoadResult r = fromJson(head + R"("z0_ohm":-5})");
    CHECK(r.message.find("Z0") != std::string::npos);
}
