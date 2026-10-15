// =============================================================================
// test_fileio.cpp — Tests de la lecture / écriture de fichiers texte et projet
// =============================================================================
#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "core/FileIo.hpp"
#include "core/Project.hpp"

using namespace smith;

namespace {
// Dossier temporaire propre à ce test (supprimé à la fin).
struct TempDir {
    std::filesystem::path path;
    TempDir() {
        path = std::filesystem::temp_directory_path() / "smithmatch_test_fileio";
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};
} // namespace

TEST_CASE("Text file write then read returns the same bytes (including UTF-8)") {
    const TempDir dir;
    const std::string file = (dir.path / "essai \xC3\xA9t\xC3\xA9.txt").string();   // « essai été.txt »
    const std::string content = "line 1\nZL = 25 \xE2\x88\x92 j15 \xCE\xA9\n";
    REQUIRE(writeTextFile(file, content).ok);
    const TextFileResult r = readTextFile(file);
    REQUIRE(r.ok);
    CHECK(r.text == content);
}

TEST_CASE("Writing replaces an existing file and leaves no temporary file") {
    const TempDir dir;
    const std::string file = (dir.path / "a.txt").string();
    REQUIRE(writeTextFile(file, "old").ok);
    REQUIRE(writeTextFile(file, "new content").ok);
    CHECK(readTextFile(file).text == "new content");
    CHECK_FALSE(std::filesystem::exists(dir.path / "a.txt.tmp"));
}

TEST_CASE("Reading a missing file or writing into a missing folder gives an error message") {
    const TempDir dir;
    const TextFileResult read = readTextFile((dir.path / "nope.txt").string());
    CHECK_FALSE(read.ok);
    CHECK_FALSE(read.message.empty());
    const TextFileResult write = writeTextFile((dir.path / "no_such_dir" / "x.txt").string(), "x");
    CHECK_FALSE(write.ok);
    CHECK_FALSE(write.message.empty());
}

TEST_CASE("A project saved to disk and loaded back is identical") {
    const TempDir dir;
    Project p = defaultProject();
    p.network = {makeElement(ElementType::SeriesInductor, 2.6526e-9), makeElement(ElementType::ShuntCapacitor, 1.3263e-12)};
    const std::string file = (dir.path / "test.smith.json").string();
    REQUIRE(writeTextFile(file, toJson(p)).ok);
    const TextFileResult text = readTextFile(file);
    REQUIRE(text.ok);
    const ProjectLoadResult loaded = fromJson(text.text);
    REQUIRE(loaded.ok);
    CHECK(toJson(loaded.project) == toJson(p));
}
