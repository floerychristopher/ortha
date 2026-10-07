#include "core/bible_verses.hpp"

#include <catch2/catch_test_macros.hpp>

#include <set>
#include <string>
#include <vector>

using namespace ortha;

namespace {

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}

const BibleVerse& verse(std::string_view reference) {
    for (const auto& v : bible_verses())
        if (v.reference == reference)
            return v;
    FAIL("missing verse " << reference);
    throw; // unreachable
}

} // namespace

TEST_CASE("bible verses are exactly the chosen references, in order") {
    const std::vector<std::string> expected{
        "Johannes 3,16",
        "Römer 5,8",
        "1. Johannes 4,8",
        "1. Johannes 4,19",
        "Epheser 2,8",
        "Römer 8,38–39",
        "Klagelieder 3,22–23",
        "Psalm 103,8",
        "Psalm 136,1",
        "Titus 3,5",
        "Johannes 14,6",
        "Johannes 11,25",
        "Matthäus 11,28",
        "Markus 10,45",
        "Apostelgeschichte 4,12",
        "Römer 10,9",
        "Römer 6,23",
        "2. Korinther 5,17",
        "Galater 2,20",
        "Philipper 1,21",
        "Sprüche 3,5–6",
        "Hebräer 11,1",
        "Markus 11,24",
        "Philipper 4,13",
        "Psalm 37,5",
        "Psalm 56,4",
        "Jeremia 17,7",
        "2. Korinther 5,7",
        "Römer 10,17",
        "Hebräer 11,6",
        "Philipper 4,6–7",
        "1. Petrus 5,7",
        "Jesaja 41,10",
        "Josua 1,9",
        "Psalm 23,4",
        "Psalm 46,2",
        "Matthäus 6,34",
        "Johannes 14,27",
        "Psalm 34,5",
        "Psalm 55,23",
        "Jesaja 40,31",
        "Psalm 31,25",
        "Römer 15,13",
        "Jeremia 29,11",
        "Psalm 121,1–2",
        "Psalm 27,1",
        "2. Timotheus 1,7",
        "Nehemia 8,10",
        "Psalm 18,3",
        "Habakuk 3,17–18",
        "Matthäus 22,37–39",
        "1. Korinther 13,4–7",
        "Johannes 13,34",
        "1. Korinther 16,14",
        "Kolosser 3,14",
        "Epheser 4,32",
        "Römer 12,10",
        "Galater 6,2",
        "Matthäus 5,44",
        "Lukas 6,31",
        "Matthäus 6,9–13",
        "Jeremia 33,3",
        "1. Thessalonicher 5,16–18",
        "Markus 11,25",
        "Johannes 15,7",
        "Psalm 50,15",
        "Jakobus 5,16",
        "Lukas 18,1",
        "Philipper 4,6",
        "Psalm 145,18",
        "Psalm 119,105",
        "Sprüche 4,23",
        "Sprüche 16,3",
        "Sprüche 16,9",
        "Psalm 1,1–3",
        "Micha 6,8",
        "Prediger 3,1",
        "Sprüche 15,1",
        "Sprüche 27,17",
        "Jakobus 1,5",
        "1. Johannes 1,9",
        "Psalm 103,12",
        "Jesaja 1,18",
        "Psalm 51,12",
        "Lukas 15,7",
        "2. Chronik 7,14",
        "Epheser 1,7",
        "Kolosser 3,13",
        "Lukas 23,34",
        "Psalm 32,1",
        "Johannes 3,36",
        "Johannes 14,2–3",
        "Offenbarung 21,4",
        "Römer 8,18",
        "1. Korinther 15,55",
        "1. Thessalonicher 4,16–17",
        "Philipper 3,20",
        "Titus 2,13",
        "Offenbarung 22,20",
        "4. Mose 6,24–26"
    };
    REQUIRE(bible_verses().size() == expected.size());
    std::set<std::string_view> seen;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& v = bible_verses()[i];
        INFO(expected[i]);
        CHECK(v.reference == expected[i]);
        CHECK_FALSE(v.text.empty());
        CHECK(seen.insert(v.reference).second);
    }
}

TEST_CASE("bible verses use the Luther 1912 text with German verse numbers") {
    CHECK(starts_with(verse("Johannes 3,16").text, "Also hat Gott die Welt geliebt, daß er"));
    // German psalm numbering counts the superscription as a verse.
    CHECK(starts_with(verse("Psalm 51,12").text, "Schaffe in mir, Gott, ein reines Herz"));
    CHECK(starts_with(verse("Psalm 46,2").text, "Gott ist unsre Zuversicht und Stärke."));
    CHECK(starts_with(verse("Psalm 31,25").text, "Seid getrost und unverzagt"));
    CHECK(verse("Psalm 119,105").text ==
          "Dein Wort ist meines Fußes Leuchte und ein Licht auf meinem Wege.");
    CHECK(starts_with(verse("Philipper 4,6").text, "Sorget nichts! sondern in allen Dingen"));
    // Passages join their verses.
    CHECK(starts_with(verse("Matthäus 6,9–13").text, "Darum sollt ihr also beten: Unser Vater"));
    CHECK(verse("Matthäus 6,9–13").text.ends_with("in Ewigkeit. Amen."));
    CHECK(verse("4. Mose 6,24–26").text.ends_with("und gebe dir Frieden."));
}
