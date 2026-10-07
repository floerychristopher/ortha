#include "core/schema.hpp"

#include <iterator>
#include <string>

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

// kSteps[i] upgrades from version i to version i + 1.
constexpr const char* kSteps[] = {kSchemaV1, kSchemaV2, kSchemaV3};
static_assert(std::size(kSteps) == kSchemaVersion);

} // namespace

void migrate(Database& db, int target) {
    const int version = db.user_version();
    if (version > kSchemaVersion)
        throw SqliteError("this database was created by a newer version of ortha (schema " +
                          std::to_string(version) + ")");
    for (int v = version; v < target; ++v) {
        db.transaction([&] {
            db.exec(kSteps[v]);
            db.set_user_version(v + 1);
        });
    }
}

} // namespace ortha
