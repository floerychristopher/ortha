#pragma once

#include "core/sqlite.hpp"
#include "core/task.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ortha::test {

inline Task make_task(std::string title, std::optional<Date> due = {},
                      Priority priority = Priority::None) {
    Task t;
    t.title = std::move(title);
    t.due = due;
    t.priority = priority;
    return t;
}

inline std::vector<std::string> titles(const std::vector<Task>& tasks) {
    std::vector<std::string> out;
    for (const auto& t : tasks)
        out.push_back(t.title);
    return out;
}

// Runs a query with one text parameter and returns the first column of the first row.
inline std::int64_t scalar(Database& db, const std::string& sql, const std::string& param = {}) {
    auto stmt = db.prepare(sql);
    if (!param.empty())
        stmt.bind(1, param);
    stmt.step();
    return stmt.column_int(0);
}

} // namespace ortha::test
