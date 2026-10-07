#include "core/task_repository.hpp"

#include "core/uuid.hpp"

namespace ortha {

namespace {

constexpr int kSchemaVersion = 1;

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

constexpr const char* kSelectTask =
    "SELECT id, title, notes, due_date, priority, done, completed_at, created_at, updated_at, "
    "recurrence "
    "FROM tasks WHERE deleted = 0";

constexpr const char* kOrderBy =
    " ORDER BY done, due_date IS NULL, due_date, priority DESC, created_at, rowid";

void bind_optional(Statement& stmt, int index, const std::optional<std::string>& value) {
    if (value)
        stmt.bind(index, *value);
    else
        stmt.bind_null(index);
}

std::optional<std::string> recurrence_text(const Task& task) {
    if (!task.recurrence)
        return std::nullopt;
    return serialize(*task.recurrence);
}

std::optional<std::string> due_text(const Task& task) {
    if (!task.due)
        return std::nullopt;
    return to_iso(*task.due);
}

Task read_task(const Statement& stmt) {
    Task t;
    t.id = stmt.column_text(0);
    t.title = stmt.column_text(1);
    t.notes = stmt.column_text(2);
    if (!stmt.column_is_null(3))
        t.due = from_iso(stmt.column_text(3));
    t.priority = static_cast<Priority>(stmt.column_int(4));
    t.done = stmt.column_int(5) != 0;
    if (!stmt.column_is_null(6))
        t.completed_at = stmt.column_text(6);
    t.created_at = stmt.column_text(7);
    t.updated_at = stmt.column_text(8);
    if (!stmt.column_is_null(9))
        t.recurrence = parse_recurrence(stmt.column_text(9)); // unreadable -> no recurrence
    return t;
}

} // namespace

TaskRepository::TaskRepository(Database& db) : db_(db) { migrate(); }

void TaskRepository::migrate() {
    const int version = db_.user_version();
    if (version > kSchemaVersion)
        throw SqliteError("this database was created by a newer version of ortha (schema " +
                          std::to_string(version) + ")");
    if (version == 0) {
        db_.transaction([&] {
            db_.exec(kSchemaV1);
            db_.set_user_version(1);
        });
    }
}

void TaskRepository::insert(Task& task) {
    if (task.id.empty())
        task.id = make_uuid();
    const auto now = now_utc_iso();
    task.created_at = now;
    task.updated_at = now;

    db_.transaction([&] {
        auto stmt = db_.prepare(
            "INSERT INTO tasks (id, title, notes, due_date, priority, done, completed_at, "
            "created_at, updated_at, recurrence) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        stmt.bind(1, task.id).bind(2, task.title).bind(3, task.notes);
        bind_optional(stmt, 4, due_text(task));
        stmt.bind(5, static_cast<std::int64_t>(task.priority));
        stmt.bind(6, std::int64_t{task.done});
        bind_optional(stmt, 7, task.completed_at);
        stmt.bind(8, task.created_at).bind(9, task.updated_at);
        bind_optional(stmt, 10, recurrence_text(task));
        stmt.step();
        save_details(task, now);
    });
}

void TaskRepository::update(Task& task) {
    const auto now = now_utc_iso();
    task.updated_at = now;

    db_.transaction([&] {
        auto stmt = db_.prepare(
            "UPDATE tasks SET title = ?, notes = ?, due_date = ?, priority = ?, done = ?, "
            "completed_at = ?, updated_at = ?, recurrence = ? WHERE id = ?");
        stmt.bind(1, task.title).bind(2, task.notes);
        bind_optional(stmt, 3, due_text(task));
        stmt.bind(4, static_cast<std::int64_t>(task.priority));
        stmt.bind(5, std::int64_t{task.done});
        bind_optional(stmt, 6, task.completed_at);
        stmt.bind(7, now);
        bind_optional(stmt, 8, recurrence_text(task));
        stmt.bind(9, task.id);
        stmt.step();
        save_details(task, now);
    });
}

void TaskRepository::save_details(Task& task, const std::string& now) {
    db_.prepare("DELETE FROM task_tags WHERE task_id = ?").bind(1, task.id).step();
    for (const auto& tag : task.tags)
        db_.prepare("INSERT OR IGNORE INTO task_tags (task_id, tag) VALUES (?, ?)")
            .bind(1, task.id)
            .bind(2, tag)
            .step();

    // Subtasks: mark all as deleted, then revive/insert the current ones.
    db_.prepare("UPDATE subtasks SET deleted = 1, updated_at = ? WHERE task_id = ? AND deleted = 0")
        .bind(1, now)
        .bind(2, task.id)
        .step();
    std::int64_t position = 0;
    for (auto& sub : task.subtasks) {
        if (sub.id.empty())
            sub.id = make_uuid();
        db_.prepare("INSERT INTO subtasks (id, task_id, title, done, position, updated_at, deleted) "
                    "VALUES (?, ?, ?, ?, ?, ?, 0) "
                    "ON CONFLICT(id) DO UPDATE SET title = excluded.title, done = excluded.done, "
                    "position = excluded.position, updated_at = excluded.updated_at, deleted = 0")
            .bind(1, sub.id)
            .bind(2, task.id)
            .bind(3, sub.title)
            .bind(4, std::int64_t{sub.done})
            .bind(5, position++)
            .bind(6, now)
            .step();
    }
}

void TaskRepository::remove(const std::string& id) {
    db_.prepare("UPDATE tasks SET deleted = 1, updated_at = ? WHERE id = ?")
        .bind(1, now_utc_iso())
        .bind(2, id)
        .step();
}

std::optional<Task> TaskRepository::find(const std::string& id) {
    auto stmt = db_.prepare(std::string(kSelectTask) + " AND id = ?");
    stmt.bind(1, id);
    if (!stmt.step())
        return std::nullopt;
    auto task = read_task(stmt);
    load_details(task);
    return task;
}

std::vector<Task> TaskRepository::all(bool include_done) {
    return query(include_done ? "" : " AND done = 0");
}

std::vector<Task> TaskRepository::upcoming(int limit) { return query(" AND done = 0", limit); }

int TaskRepository::count_open() {
    auto stmt = db_.prepare("SELECT COUNT(*) FROM tasks WHERE deleted = 0 AND done = 0");
    stmt.step();
    return static_cast<int>(stmt.column_int(0));
}

std::vector<Task> TaskRepository::query(const std::string& where, std::optional<int> limit) {
    auto stmt = db_.prepare(std::string(kSelectTask) + where + kOrderBy +
                            (limit ? " LIMIT ?" : ""));
    if (limit)
        stmt.bind(1, std::int64_t{*limit});
    std::vector<Task> tasks;
    while (stmt.step())
        tasks.push_back(read_task(stmt));
    for (auto& task : tasks)
        load_details(task);
    return tasks;
}

void TaskRepository::load_details(Task& task) {
    auto tags = db_.prepare("SELECT tag FROM task_tags WHERE task_id = ? ORDER BY tag");
    tags.bind(1, task.id);
    while (tags.step())
        task.tags.push_back(tags.column_text(0));

    auto subs = db_.prepare(
        "SELECT id, title, done FROM subtasks WHERE task_id = ? AND deleted = 0 ORDER BY position");
    subs.bind(1, task.id);
    while (subs.step())
        task.subtasks.push_back({subs.column_text(0), subs.column_text(1), subs.column_int(2) != 0});
}

} // namespace ortha
