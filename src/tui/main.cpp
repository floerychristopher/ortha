#include "core/goal_repository.hpp"
#include "core/paths.hpp"
#include "core/sqlite.hpp"
#include "core/task_repository.hpp"
#include "core/task_service.hpp"
#include "tui/app.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

int main() {
    try {
        const auto path = ortha::default_db_path();
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        const auto utf8 = path.u8string(); // SQLite expects UTF-8 file names
        ortha::Database db(std::string(utf8.begin(), utf8.end()));
        ortha::TaskRepository repo(db);
        ortha::TaskService service(repo);
        ortha::GoalRepository goals(db);
        ortha::App app(repo, service, goals);
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ortha: " << e.what() << '\n';
        return 1;
    }
}
