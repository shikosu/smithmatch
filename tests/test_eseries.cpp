// =============================================================================
// test_eseries.cpp — Tests des séries E12 / E24 (spec §6.7)
// =============================================================================
#include <doctest/doctest.h>

#include "core/ESeries.hpp"

using namespace smith;

TEST_CASE("E12 rounding examples from the specification") {
    CHECK(nearestESeries(4.9e-9, ESeriesKind::E12) == doctest::Approx(4.7e-9).epsilon(1e-9));
    CHECK(nearestESeries(47e-9, ESeriesKind::E12) == doctest::Approx(47e-9).epsilon(1e-9));
}

TEST_CASE("E12 rounding crosses the decade boundary: 9.5 pF -> 10 pF") {
    CHECK(nearestESeries(9.5e-12, ESeriesKind::E12) == doctest::Approx(10e-12).epsilon(1e-9));
}

TEST_CASE("E24 rounding examples from the specification") {
    CHECK(nearestESeries(1.04, ESeriesKind::E24) == doctest::Approx(1.0).epsilon(1e-9));
    CHECK(nearestESeries(1.06, ESeriesKind::E24) == doctest::Approx(1.1).epsilon(1e-9));
    CHECK(nearestESeries(0.33e-12, ESeriesKind::E24) == doctest::Approx(0.33e-12).epsilon(1e-9));
}

TEST_CASE("Rounding is logarithmic, not linear") {
    // Entre 1.0 et 1.2 (E12) la frontière log est √(1.0×1.2) ≈ 1.0954.
    CHECK(nearestESeries(1.09, ESeriesKind::E12) == doctest::Approx(1.0).epsilon(1e-9));
    CHECK(nearestESeries(1.10, ESeriesKind::E12) == doctest::Approx(1.2).epsilon(1e-9));
    // Entre 8.2 et 10 : frontière √(82) ≈ 9.055.
    CHECK(nearestESeries(9.0, ESeriesKind::E12) == doctest::Approx(8.2).epsilon(1e-9));
    CHECK(nearestESeries(9.1, ESeriesKind::E12) == doctest::Approx(10.0).epsilon(1e-9));
}

TEST_CASE("Exact series values are unchanged, whatever the decade") {
    for (double decade : {1e-12, 1e-9, 1e-6, 1.0, 1e3}) {
        for (double m : {1.0, 1.2, 2.2, 4.7, 8.2}) {
            const double v = m * decade;
            CHECK(nearestESeries(v, ESeriesKind::E12) == doctest::Approx(v).epsilon(1e-9));
        }
    }
}

TEST_CASE("Exact powers of ten are stable") {
    for (double v : {1e-12, 1e-11, 1e-9, 1e-6, 1.0, 10.0, 1e6}) {
        CHECK(nearestESeries(v, ESeriesKind::E12) == doctest::Approx(v).epsilon(1e-9));
        CHECK(nearestESeries(v, ESeriesKind::E24) == doctest::Approx(v).epsilon(1e-9));
    }
}

TEST_CASE("None and invalid values are returned unchanged") {
    CHECK(nearestESeries(4.9e-9, ESeriesKind::None) == 4.9e-9);
    CHECK(nearestESeries(0.0, ESeriesKind::E12) == 0.0);
    CHECK(nearestESeries(-3.0, ESeriesKind::E12) == -3.0);
}

TEST_CASE("snapNetwork rounds every element and keeps types") {
    const ElementList network = {makeElement(ElementType::SeriesInductor, 2.65e-9),
                                 makeElement(ElementType::ShuntCapacitor, 1.33e-12)};
    const ElementList snapped = snapNetwork(network, ESeriesKind::E12);
    REQUIRE(snapped.size() == 2);
    CHECK(snapped[0].type == ElementType::SeriesInductor);
    CHECK(snapped[0].value == doctest::Approx(2.7e-9).epsilon(1e-9));
    CHECK(snapped[1].type == ElementType::ShuntCapacitor);
    CHECK(snapped[1].value == doctest::Approx(1.2e-12).epsilon(1e-9));
    // L'original n'est pas modifié (types valeur).
    CHECK(network[0].value == 2.65e-9);
}
