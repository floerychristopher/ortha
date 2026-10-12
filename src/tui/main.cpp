#include "core/goal_repository.hpp"
#include "core/paths.hpp"
#include "core/routine_repository.hpp"
#include "core/sqlite.hpp"
#include "core/task_repository.hpp"
#include "core/task_service.hpp"
#include "core/version.hpp"
#include "tui/app.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Options are answered before any database or UI is touched.
    if (argc > 1) {
        const std::string option = argv[1];
        if (option == "--version") {
            std::cout << "ortha " << ortha::kVersion << '\n';
            return 0;
        }
        std::cerr << "ortha: unknown option '" << option << "'\n";
        return 2;
    }
    try {
        const auto path = ortha::default_db_path();
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        const auto utf8 = path.u8string(); // SQLite expects UTF-8 file names
        ortha::Database db(std::string(utf8.begin(), utf8.end()));
        ortha::TaskRepository repo(db);
        ortha::TaskService service(repo);
        ortha::GoalRepository goals(db);
        ortha::RoutineRepository routines(db);
        ortha::App app(repo, service, goals, routines);
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ortha: " << e.what() << '\n';
        return 1;
    }
}
