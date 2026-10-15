// =============================================================================
// test_main.cpp — Point d'entrée des tests unitaires (doctest)
// =============================================================================
// doctest fournit lui-même la fonction main() grâce à la macro ci-dessous.
// Chaque fichier tests/test_*.cpp contient des TEST_CASE qui sont découverts
// et exécutés automatiquement. Lancer : ctest --test-dir build --output-on-failure
// =============================================================================
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "core/Types.hpp"

// Test « de fumée » : vérifie simplement que la chaîne de test fonctionne.
TEST_CASE("Sanity: constants are defined") {
    CHECK(smith::kPi > 3.14159);
    CHECK(smith::kPi < 3.14160);
    CHECK(smith::kTwoPi == doctest::Approx(2.0 * smith::kPi));
}
