#include "core/paths.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

using namespace ortha;

namespace {

void set_env(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void unset_env(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

} // namespace

TEST_CASE("ORTHA_DB overrides the default path") {
    set_env("ORTHA_DB", "/tmp/x/test.db");
    CHECK(default_db_path() == std::filesystem::path("/tmp/x/test.db"));
    unset_env("ORTHA_DB");
    CHECK(default_db_path().filename() == "ortha.db");
    CHECK(default_db_path().parent_path().filename() == "ortha");
}

#ifndef _WIN32
TEST_CASE("XDG_DATA_HOME is respected on Linux") {
    unset_env("ORTHA_DB");
    set_env("XDG_DATA_HOME", "/tmp/xdg");
    CHECK(default_db_path() == std::filesystem::path("/tmp/xdg/ortha/ortha.db"));
    unset_env("XDG_DATA_HOME");
    set_env("HOME", "/home/someone");
    CHECK(default_db_path() == std::filesystem::path("/home/someone/.local/share/ortha/ortha.db"));
}
#endif
