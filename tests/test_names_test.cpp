#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/interfaces/catch_interfaces_registry_hub.hpp>
#include <catch2/interfaces/catch_interfaces_testcase.hpp>

#include <algorithm>
#include <string>

// CTest passes each test name to the test program on the command line. On Windows,
// characters outside ASCII (e.g. "·") get mangled on the way, the name no longer matches
// and the test does not run at all. So test names must be plain ASCII.
TEST_CASE("test names are plain ASCII") {
    for (const auto* info : Catch::getRegistryHub().getTestCaseRegistry().getAllInfos()) {
        const std::string& name = info->name;
        INFO(name);
        CHECK(std::all_of(name.begin(), name.end(),
                          [](char c) { return static_cast<unsigned char>(c) < 0x80; }));
    }
}
