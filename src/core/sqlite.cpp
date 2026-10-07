#include "core/sqlite.hpp"

#include <sqlite3.h>

namespace ortha {

namespace {

[[noreturn]] void throw_error(sqlite3* db, const std::string& context) {
    throw SqliteError(context + ": " + sqlite3_errmsg(db));
}

} // namespace

// ---- Statement ----

Statement::Statement(sqlite3_stmt* stmt) : stmt_(stmt) {}

void Statement::Finalizer::operator()(sqlite3_stmt* stmt) const noexcept { sqlite3_finalize(stmt); }

Statement& Statement::bind(int index, std::string_view value) {
    if (sqlite3_bind_text(stmt_.get(), index, value.data(), static_cast<int>(value.size()),
                          SQLITE_TRANSIENT) != SQLITE_OK)
        throw_error(sqlite3_db_handle(stmt_.get()), "bind");
    return *this;
}

Statement& Statement::bind(int index, std::int64_t value) {
    if (sqlite3_bind_int64(stmt_.get(), index, value) != SQLITE_OK)
        throw_error(sqlite3_db_handle(stmt_.get()), "bind");
    return *this;
}

Statement& Statement::bind_null(int index) {
    if (sqlite3_bind_null(stmt_.get(), index) != SQLITE_OK)
        throw_error(sqlite3_db_handle(stmt_.get()), "bind");
    return *this;
}

bool Statement::step() {
    switch (sqlite3_step(stmt_.get())) {
    case SQLITE_ROW:
        return true;
    case SQLITE_DONE:
        return false;
    default:
        throw_error(sqlite3_db_handle(stmt_.get()), "step");
    }
}

std::string Statement::column_text(int col) const {
    const auto* text = sqlite3_column_text(stmt_.get(), col);
    if (text == nullptr)
        return {};
    return {reinterpret_cast<const char*>(text),
            static_cast<std::size_t>(sqlite3_column_bytes(stmt_.get(), col))};
}

std::int64_t Statement::column_int(int col) const { return sqlite3_column_int64(stmt_.get(), col); }

bool Statement::column_is_null(int col) const {
    return sqlite3_column_type(stmt_.get(), col) == SQLITE_NULL;
}

// ---- Database ----

void Database::Closer::operator()(sqlite3* db) const noexcept { sqlite3_close_v2(db); }

Database::Database(const std::string& utf8_path) {
    sqlite3* raw = nullptr;
    const int rc =
        sqlite3_open_v2(utf8_path.c_str(), &raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    db_.reset(raw); // take ownership first, so the handle is closed even on error
    if (rc != SQLITE_OK)
        throw_error(raw, "cannot open database '" + utf8_path + "'");
    // Another ortha instance may be writing at the same moment: wait instead of failing.
    sqlite3_busy_timeout(db_.get(), 2000);
    exec("PRAGMA foreign_keys = ON");
}

void Database::exec(const std::string& sql) {
    if (sqlite3_exec(db_.get(), sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
        throw_error(db_.get(), "exec");
}

Statement Database::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) !=
        SQLITE_OK)
        throw_error(db_.get(), "prepare");
    return Statement(stmt);
}

void Database::transaction(const std::function<void()>& fn) {
    // Savepoints (unlike BEGIN/COMMIT) can be nested.
    exec("SAVEPOINT ortha_tx");
    try {
        fn();
    } catch (...) {
        exec("ROLLBACK TO ortha_tx");
        exec("RELEASE ortha_tx");
        throw;
    }
    exec("RELEASE ortha_tx");
}

int Database::user_version() {
    auto stmt = prepare("PRAGMA user_version");
    stmt.step();
    return static_cast<int>(stmt.column_int(0));
}

void Database::set_user_version(int version) {
    exec("PRAGMA user_version = " + std::to_string(version));
}

} // namespace ortha
