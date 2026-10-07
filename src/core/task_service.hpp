#pragma once

#include "core/task_repository.hpp"

#include <optional>
#include <string>

namespace ortha {

// Task operations that involve more than a plain save.
class TaskService {
  public:
    explicit TaskService(TaskRepository& repo);

    // Marks the task as done. Does nothing if it is already done or unknown.
    // Returns the follow-up task if one was created (recurring tasks).
    std::optional<Task> complete(const std::string& id, Date today);

    // Marks a done task as open again.
    void reopen(const std::string& id);

  private:
    TaskRepository& repo_;
};

} // namespace ortha
