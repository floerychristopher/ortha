#include "core/uuid.hpp"

#include <catch2/catch_test_macros.hpp>

#include <regex>

using namespace ortha;

TEST_CASE("make_uuid produces distinct v4 uuids") {
    const std::regex v4(R"([0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12})");
    auto a = make_uuid();
    auto b = make_uuid();
    CHECK(std::regex_match(a, v4));
    CHECK(a != b);
}
