#include "core/schema.hpp"

#include "core/date.hpp"
#include "core/uuid.hpp"

#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

namespace ortha {

namespace {

constexpr const char* kSchemaV1 = R"sql(
CREATE TABLE tasks (
    id           TEXT PRIMARY KEY,
    title        TEXT NOT NULL,
    notes        TEXT NOT NULL DEFAULT '',
    due_date     TEXT,
    priority     INTEGER NOT NULL DEFAULT 0,
    done         INTEGER NOT NULL DEFAULT 0,
    completed_at TEXT,
    recurrence   TEXT,
    created_at   TEXT NOT NULL,
    updated_at   TEXT NOT NULL,
    deleted      INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE task_tags (
    task_id TEXT NOT NULL REFERENCES tasks(id),
    tag     TEXT NOT NULL,
    PRIMARY KEY (task_id, tag)
);
CREATE TABLE subtasks (
    id         TEXT PRIMARY KEY,
    task_id    TEXT NOT NULL REFERENCES tasks(id),
    title      TEXT NOT NULL,
    done       INTEGER NOT NULL DEFAULT 0,
    position   INTEGER NOT NULL,
    updated_at TEXT NOT NULL,
    deleted    INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX idx_tasks_due ON tasks(due_date);
)sql";

constexpr const char* kSchemaV2 = R"sql(
CREATE TABLE goals (
    id         TEXT PRIMARY KEY,
    title      TEXT NOT NULL,
    weekdays   INTEGER NOT NULL,
    start_date TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    deleted    INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE goal_entries (
    goal_id    TEXT NOT NULL REFERENCES goals(id),
    day        TEXT NOT NULL,
    status     TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    PRIMARY KEY (goal_id, day)
);
)sql";

constexpr const char* kSchemaV3 = R"sql(
ALTER TABLE goals ADD COLUMN symbol TEXT NOT NULL DEFAULT '';
)sql";

constexpr const char* kSchemaV4 = R"sql(
CREATE TABLE routines (
    id           TEXT PRIMARY KEY,
    name         TEXT NOT NULL,
    start_minute INTEGER NOT NULL,
    end_minute   INTEGER,
    identity     TEXT NOT NULL DEFAULT '',
    created_at   TEXT NOT NULL,
    updated_at   TEXT NOT NULL,
    deleted      INTEGER NOT NULL DEFAULT 0
);
ALTER TABLE goals ADD COLUMN routine_id TEXT REFERENCES routines(id);
ALTER TABLE goals ADD COLUMN position INTEGER NOT NULL DEFAULT 0;
)sql";

// Every goal belongs to a routine from version 4 on: existing goals move into "My routine"
// (at 0:00), in the order they were created.
void move_goals_into_routine(Database& db) {
    std::vector<std::string> ids;
    auto select = db.prepare("SELECT id FROM goals WHERE deleted = 0 ORDER BY created_at, rowid");
    while (select.step())
        ids.push_back(select.column_text(0));
    if (ids.empty())
        return;
    const auto routine_id = make_uuid();
    const auto now = now_utc_iso();
    db.prepare("INSERT INTO routines (id, name, start_minute, created_at, updated_at) "
               "VALUES (?, 'My routine', 0, ?, ?)")
        .bind(1, routine_id)
        .bind(2, now)
        .bind(3, now)
        .step();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        db.prepare("UPDATE goals SET routine_id = ?, position = ? WHERE id = ?")
            .bind(1, routine_id)
            .bind(2, static_cast<std::int64_t>(i))
            .bind(3, ids[i])
            .step();
    }
}

// kSteps[i] upgrades from version i to version i + 1: the SQL, then (optionally) code that
// moves existing data.
struct Step {
    const char* sql = nullptr;
    void (*then)(Database&) = nullptr;
};
constexpr Step kSteps[] = {{kSchemaV1}, {kSchemaV2}, {kSchemaV3}, {kSchemaV4, move_goals_into_routine}};
static_assert(std::size(kSteps) == kSchemaVersion);

} // namespace

void migrate(Database& db, int target) {
    const int version = db.user_version();
    if (version > kSchemaVersion)
        throw SqliteError("this database was created by a newer version of ortha (schema " +
                          std::to_string(version) + ")");
    for (int v = version; v < target; ++v) {
        db.transaction([&] {
            db.exec(kSteps[v].sql);
            if (kSteps[v].then)
                kSteps[v].then(db);
            db.set_user_version(v + 1);
        });
    }
}

} // namespace ortha
