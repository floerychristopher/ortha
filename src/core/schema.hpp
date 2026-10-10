#pragma once

#include "core/sqlite.hpp"

namespace ortha {

// Version 1: tasks, tags, subtasks. Version 2: goals and their daily entries.
// Version 3: goal symbols. Version 4: routines (goals become ordered habits of a routine).
inline constexpr int kSchemaVersion = 4;

// Brings the database schema up to `target` (default: the newest). Safe to call repeatedly.
// Throws SqliteError if the database was created by a newer version of ortha.
void migrate(Database& db, int target = kSchemaVersion);

} // namespace ortha
