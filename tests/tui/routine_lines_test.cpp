#include "tui/routine_lines.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace ortha;

namespace {

RoutineSummary routine(std::string id, std::vector<std::string> habit_ids) {
    RoutineSummary r;
    r.routine.id = std::move(id);
    for (auto& h : habit_ids) {
        GoalSummary g;
        g.goal.id = std::move(h);
        r.habits.push_back(g);
    }
    return r;
}

} // namespace

TEST_CASE("routine_lines lists headings, habits only where shown") {
    const std::vector<RoutineSummary> rs{routine("m", {"a", "b"}), routine("s", {"c"})};
    auto all = routine_lines(rs, [](std::size_t) { return true; });
    REQUIRE(all.size() == 5);
    CHECK_FALSE(all[0].habit);
    CHECK(all[2].habit == 1u);
    CHECK(all[3].routine == 1u);
    auto first_only = routine_lines(rs, [](std::size_t i) { return i == 0; });
    CHECK(first_only.size() == 4); // m, a, b, s
}

TEST_CASE("find_line finds a heading or habit again after a reload") {
    std::vector<RoutineSummary> rs{routine("m", {"a", "b"}), routine("s", {"c"})};
    auto lines = routine_lines(rs, [](std::size_t) { return true; });
    const auto key_b = line_key(rs, lines[2]);
    const auto key_s = line_key(rs, lines[3]);

    std::swap(rs[0].habits[0], rs[0].habits[1]); // b moved up
    lines = routine_lines(rs, [](std::size_t) { return true; });
    CHECK(find_line(rs, lines, key_b) == 1u);
    CHECK(find_line(rs, lines, key_s) == 3u);

    rs[0].habits.pop_back(); // a deleted: b's line is still found, a's key is gone
    lines = routine_lines(rs, [](std::size_t) { return true; });
    CHECK(find_line(rs, lines, key_b) == 1u);
    CHECK_FALSE(find_line(rs, lines, "m/a"));
}
