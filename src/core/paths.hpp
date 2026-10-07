#pragma once

#include <filesystem>

namespace ortha {

// Where the task database lives:
//   $ORTHA_DB if set, otherwise
//   Windows: %APPDATA%\ortha\ortha.db
//   Linux:   $XDG_DATA_HOME/ortha/ortha.db, falling back to ~/.local/share/ortha/ortha.db
std::filesystem::path default_db_path();

} // namespace ortha
