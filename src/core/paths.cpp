#include "core/paths.hpp"

#include <cstdlib>

namespace ortha {

namespace fs = std::filesystem;

#ifdef _WIN32

namespace {
// _wgetenv keeps non-ASCII characters (e.g. in user names) intact on Windows.
const wchar_t* env(const wchar_t* name) {
    const wchar_t* value = _wgetenv(name);
    return (value != nullptr && *value != L'\0') ? value : nullptr;
}
} // namespace

fs::path default_db_path() {
    if (auto custom = env(L"ORTHA_DB"))
        return custom;
    if (auto appdata = env(L"APPDATA"))
        return fs::path(appdata) / "ortha" / "ortha.db";
    return fs::path("ortha") / "ortha.db";
}

#else

namespace {
const char* env(const char* name) {
    const char* value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}
} // namespace

fs::path default_db_path() {
    if (auto custom = env("ORTHA_DB"))
        return custom;
    if (auto xdg = env("XDG_DATA_HOME"))
        return fs::path(xdg) / "ortha" / "ortha.db";
    if (auto home = env("HOME"))
        return fs::path(home) / ".local" / "share" / "ortha" / "ortha.db";
    return fs::path("ortha") / "ortha.db";
}

#endif

} // namespace ortha
