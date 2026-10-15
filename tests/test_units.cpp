// =============================================================================
// test_units.cpp — Tests de la notation ingénieur (spec §6.8)
// =============================================================================
#include <doctest/doctest.h>

#include <cmath>
#include <limits>

#include "core/Units.hpp"

using smith::formatEng;
using smith::formatSig;
using smith::parseEng;

namespace {
// Vérifie que parseEng(text) réussit et vaut « expected » (tolérance relative 1e-12).
void checkParse(const std::string& text, double expected) {
    INFO("input: \"" << text << "\"");
    const auto value = parseEng(text);
    REQUIRE(value.has_value());
    CHECK(*value == doctest::Approx(expected).epsilon(1e-12));
}
void checkRejected(const std::string& text) {
    INFO("input: \"" << text << "\"");
    CHECK_FALSE(parseEng(text).has_value());
}
} // namespace

TEST_CASE("parseEng accepts the formats listed in the specification") {
    checkParse("4.7n", 4.7e-9);
    checkParse("4.7 nH", 4.7e-9);
    checkParse("4.7nH", 4.7e-9);
    checkParse("4,7 nH", 4.7e-9);     // virgule décimale
    checkParse("4,7n", 4.7e-9);
    checkParse("100p", 1e-10);
    checkParse("2.2u", 2.2e-6);
    checkParse("2.2\xC2\xB5", 2.2e-6);   // µ (U+00B5)
    checkParse("2.2\xCE\xBC", 2.2e-6);   // μ (U+03BC)
    checkParse("1.5k", 1500.0);
    checkParse("2.4G", 2.4e9);
    checkParse("2.4 GHz", 2.4e9);
    checkParse("50", 50.0);
}

TEST_CASE("parseEng handles other realistic inputs") {
    checkParse("1e-9", 1e-9);
    checkParse("-15", -15.0);
    checkParse("+3.3", 3.3);
    checkParse("  12  ", 12.0);
    checkParse("10 k\xCE\xA9", 1e4);         // 10 kΩ
    checkParse("50 \xCE\xA9", 50.0);         // 50 Ω (unité seule, pas de préfixe)
    checkParse("2.2 pF", 2.2e-12);
    checkParse("3 MHz", 3e6);                // M = méga
    checkParse("3 mH", 3e-3);                // m = milli
    checkParse("100 Hz", 100.0);             // « H » de Hz n'est pas un préfixe
    checkParse("1 H", 1.0);
    checkParse("5 F", 5.0);
    checkParse(".5", 0.5);
    checkParse("5.", 5.0);
}

TEST_CASE("parseEng rejects invalid input") {
    checkRejected("abc");
    checkRejected("");
    checkRejected("   ");
    checkRejected("nan");
    checkRejected("inf");
    checkRejected("-inf");
    checkRejected("infinity");
    checkRejected("-");
    checkRejected(".");
    checkRejected("4.7 xyz");
    checkRejected("4.7q");
    checkRejected("1e");           // exposant incomplet
    checkRejected("1e999");        // dépassement
    checkRejected("1e999G");
    checkRejected("4.7 nHH");
    checkRejected("4..7");
    checkRejected("0x10");
}

TEST_CASE("formatEng uses engineering prefixes") {
    CHECK(formatEng(3.898e-8, "H") == "38.98 nH");
    CHECK(formatEng(2.4e9, "Hz") == "2.4 GHz");
    CHECK(formatEng(50.0, "\xCE\xA9") == "50 \xCE\xA9");
    CHECK(formatEng(1.5916e-12, "F") == "1.592 pF");
    CHECK(formatEng(2.6526e-9, "H") == "2.653 nH");
    CHECK(formatEng(2.2e-6, "H") == "2.2 \xC2\xB5H");   // µ (U+00B5)
    CHECK(formatEng(1500.0, "\xCE\xA9") == "1.5 k\xCE\xA9");
    CHECK(formatEng(100.0, "\xCE\xA9") == "100 \xCE\xA9");
    CHECK(formatEng(-15.0, "\xCE\xA9") == "-15 \xCE\xA9");
    CHECK(formatEng(0.0, "F") == "0 F");
    CHECK(formatEng(4.7, "") == "4.7");
}

TEST_CASE("formatEng rounding can promote to the next prefix") {
    // 999.97 avec 4 chiffres s'arrondit à 1000 : on veut « 1 k » et non « 1000 ».
    CHECK(formatEng(999.97e-9, "H") == "1 \xC2\xB5H");
    CHECK(formatEng(999.4, "") == "999.4");
}

TEST_CASE("formatEng special values") {
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(formatEng(inf, "dB") == "\xE2\x88\x9E dB");
    CHECK(formatEng(-inf, "dB") == "-\xE2\x88\x9E dB");
    CHECK(formatEng(std::nan(""), "H") == "NaN");
}

TEST_CASE("formatSig") {
    CHECK(formatSig(1.23456, 4) == "1.235");
    CHECK(formatSig(0.0, 4) == "0");
    CHECK(formatSig(-0.5, 3) == "-0.5");
    CHECK(formatSig(12345.6, 4) == "12346");
}

TEST_CASE("formatEng then parseEng round-trips within display precision") {
    for (double v : {1.0e-12, 4.7e-9, 3.898e-8, 2.2e-6, 0.033, 47.0, 1500.0, 2.4e9}) {
        const auto parsed = parseEng(formatEng(v, "H", 6));
        REQUIRE(parsed.has_value());
        CHECK(*parsed == doctest::Approx(v).epsilon(1e-5));
    }
}
