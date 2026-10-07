#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

// Forward declarations: users of this header never see the sqlite3 C API.
struct sqlite3;
struct sqlite3_stmt;

namespace ortha {

class SqliteError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// A prepared SQL statement. Finalized automatically when it goes out of scope.
class Statement {
  public:
    // Parameter indices start at 1, as in SQL ("?1").
    Statement& bind(int index, std::string_view value);
    Statement& bind(int index, std::int64_t value);
    Statement& bind_null(int index);

    // Executes one step. Returns true while a result row is available.
    bool step();

    // Column indices start at 0. NULL reads as "" / 0.
    std::string column_text(int col) const;
    std::int64_t column_int(int col) const;
    bool column_is_null(int col) const;

  private:
    friend class Database;
    explicit Statement(sqlite3_stmt* stmt);

    struct Finalizer {
        void operator()(sqlite3_stmt* stmt) const noexcept;
    };
    std::unique_ptr<sqlite3_stmt, Finalizer> stmt_;
};

// An open database connection. Closed automatically when it goes out of scope.
class Database {
  public:
    // Opens (or creates) the database file. ":memory:" opens a temporary in-memory database.
    explicit Database(const std::string& utf8_path);

    // Runs one or more SQL statements that return no rows.
    void exec(const std::string& sql);
    Statement prepare(const std::string& sql);

    // Runs fn atomically: everything is rolled back if fn throws. Can be nested.
    void transaction(const std::function<void()>& fn);

    // Schema version stored in the database file (PRAGMA user_version).
    int user_version();
    void set_user_version(int version);

  private:
    struct Closer {
        void operator()(sqlite3* db) const noexcept;
    };
    std::unique_ptr<sqlite3, Closer> db_;
};

} // namespace ortha
