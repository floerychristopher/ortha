#pragma once

#include "core/sqlite.hpp"
#include "core/task.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ortha {

// Stores tasks in the SQLite database. Creates/migrates the schema on construction.
class TaskRepository {
  public:
    // Throws SqliteError if the database was created by a newer ortha version.
    explicit TaskRepository(Database& db);

    // Assigns id (if empty), subtask ids, created_at and updated_at.
    void insert(Task& task);
    // Saves all fields, tags and subtasks; bumps updated_at.
    void update(Task& task);
    // Soft delete: the row stays (for sync) but is hidden from all queries.
    void remove(const std::string& id);

    std::optional<Task> find(const std::string& id);
    // Sorted: open before done, then by due date (none last), priority, creation time.
    std::vector<Task> all(bool include_done);
    // The next `limit` open tasks: dated ones first (earliest first), then undated ones;
    // ties by priority. This is what the "Today" view shows.
    std::vector<Task> upcoming(int limit);
    // Number of open (not done, not deleted) tasks.
    int count_open();

    Database& database() { return db_; }

  private:
    void migrate();
    std::vector<Task> query(const std::string& where, std::optional<int> limit = std::nullopt);
    void load_details(Task& task);
    void save_details(Task& task, const std::string& now);

    Database& db_;
};

} // namespace ortha
